#ifndef REDDITCONNECTION_H
#define REDDITCONNECTION_H

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/signals2.hpp>
#include <boost/version.hpp>
#include <string>
#include <memory>
#include <optional>
#include <deque>
#include <mutex>
#include <any>

template<typename RequestBody,typename ResponseBody, typename ClientResponse>
class RedditConnection :
        public std::enable_shared_from_this<
            RedditConnection<RequestBody,ResponseBody,ClientResponse>
        >
{
protected:
    using response_parser_t = boost::beast::http::response_parser<ResponseBody>;
    using request_t = boost::beast::http::request<RequestBody>;
    using stream_t = boost::beast::ssl_stream<boost::beast::tcp_stream>;

public:
    RedditConnection(const boost::asio::any_io_executor& executor,
                     boost::asio::ssl::context& ssl_context,const std::string& host,
                     const std::string& service):
        ssl_context(ssl_context),strand(boost::asio::make_strand(executor)),
        resolver(strand),stream(std::make_optional<stream_t>(strand, ssl_context)),
        host(host),service(service),responseParser(std::make_optional<response_parser_t>())
    {
    }
    virtual ~RedditConnection()
    {
        clearAllSlots();
    }

    void clearAllSlots()
    {
        signal.disconnect_all_slots();
    }

    template<typename Slot>
    void connectionCompleteHandler(Slot slot)
    {
        signal.connect(std::move(slot));
    }
private:
    void onConnect(const boost::system::error_code& ec,
                          boost::asio::ip::tcp::resolver::results_type::endpoint_type,
                   request_t request)
    {
        if(ec)
        {
            onError(ec,std::move(request));
            return;
        }
        isSsl = !(service == "http" || service == "80" || service == "8080");
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (isSsl && ! SSL_set_tlsext_host_name(stream->native_handle(), host.c_str()))
        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         boost::asio::error::get_ssl_category()};
            onError(ec,std::move(request));
            return;
        }

        if(!isSsl)
        {
            sendRequest(std::move(request));
        }
        else
        {
            using namespace std::placeholders;
            auto handshakeMethod = std::bind(&RedditConnection::onHandshake,this->shared_from_this(),_1,std::move(request));
            stream->async_handshake(boost::asio::ssl::stream_base::client,handshakeMethod);
        }
    }

    void resolveComplete(const boost::system::error_code& ec,
                         boost::asio::ip::tcp::resolver::results_type results,
                         request_t request)
    {
        if(ec)
        {
            onError(ec,std::move(request));
            return;
        }
        // Set a timeout on the operation
        boost::beast::get_lowest_layer(stream.value()).expires_after(streamTimeout);
        using namespace std::placeholders;
        auto connectMethod = std::bind(&RedditConnection::onConnect,this->shared_from_this(),_1,_2,std::move(request));
        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(stream.value()).async_connect(results,connectMethod);
    }

    void onHandshake(const boost::system::error_code& ec,request_t request)
    {
        if(ec)
        {
            onError(ec,std::move(request));
            return;
        }
        connected = true;
        sendRequest(std::move(request));
    }


protected:
    template<typename U>
    struct queued_request
    {

        queued_request(request_t request, U userData):
            request(std::move(request)),userData(std::move(userData))
        {}
        request_t request;
        U userData;
    };
    template<typename U>
    void enqueueRequest(request_t request, U userData)
    {
        std::lock_guard<std::mutex> _(queuedRequestsMutex);
        queuedRequests.emplace_back(std::move(request),std::move(userData));
        if(queuedRequests.size() == 1)
        {
            performRequest(std::move(queuedRequests.front().request));
        }
    }
    virtual void performRequest(request_t request)
    {
        readBuffer.clear();
        responseParser.emplace();
        if(boost::beast::get_lowest_layer(stream.value()).socket().is_open())
        {
            sendRequest(std::move(request));
        }
        else
        {
            resolveHost(std::move(request));
        }
    }
    template<typename Derived>
    std::shared_ptr<Derived> shared_from_base()
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }

    virtual void onError(const boost::system::error_code& ec,request_t request)
    {
        if(boost::beast::http::error::end_of_stream == ec)
        {
            //we got the connection closed on us, reconnect
            connected = false;
            stream.emplace(strand, ssl_context);
            responseParser.reset();
            responseParser.emplace();
            resolveHost(std::move(request));
        }
        else
        {
            signal(ec,{});
        }
    }

    virtual void onError(const boost::system::error_code& ec)
    {
        onError(ec,std::move(privateRequest));
    }

    void resolveHost(request_t request)
    {
        using namespace std::placeholders;
        auto resolverMethod = std::bind(&RedditConnection::resolveComplete,this->shared_from_this(),
                                        _1,_2,std::move(request));
        resolver.async_resolve(host,service,resolverMethod);
    }

    virtual void sendRequest(request_t request)
    {
        boost::beast::get_lowest_layer(stream.value()).expires_after(streamTimeout);
        using namespace std::placeholders;
        auto writeMethod = std::bind(&RedditConnection::onWrite,this->shared_from_this(),_1,_2);
        this->privateRequest = std::move(request);
        // Send the HTTP request to the remote host
        if(isSsl)
        {
            boost::beast::http::async_write(stream.value(), privateRequest, writeMethod);
        }
        else
        {
            boost::beast::http::async_write(stream.value().next_layer(), privateRequest, writeMethod);
        }
    }

    virtual void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if(ec)
        {
            onError(ec,std::move(this->privateRequest));
            return;
        }

        using namespace std::placeholders;
        auto readMethod = std::bind(&RedditConnection::onReadHeader,this->shared_from_this(),_1,_2);
        if(isSsl)
        {
            boost::beast::http::async_read_header(stream.value(), readBuffer, responseParser.value(), readMethod);
        }
        else
        {
            boost::beast::http::async_read_header(stream.value().next_layer(), readBuffer, responseParser.value(), readMethod);
        }
    }
    virtual void onReadHeader(const boost::system::error_code& ec,std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);
        if(ec)
        {
            onError(ec);
            return;
        }
        std::string location;
        for(const auto& h : responseParser->get())
        {
            if(h.name() == boost::beast::http::field::location)
            {
#if BOOST_VERSION < 108000
                location = h.value().to_string();
#else
                location = h.value();
#endif // BOOST_VERSION < 108000
                break;
            }
        }
        if(!location.empty())
        {
            auto status = responseParser->get().result();

            if(status == boost::beast::http::status::moved_permanently ||
               status == boost::beast::http::status::temporary_redirect ||
               status == boost::beast::http::status::permanent_redirect ||
               status == boost::beast::http::status::see_other ||
               status == boost::beast::http::status::found)
            {
                connected = false;
                stream.emplace(strand, ssl_context);
                readBuffer.clear();
                responseParser.reset();
                responseParser.emplace();
                handleLocationChange(location);
            }
        }
        else
        {
            using namespace std::placeholders;
            auto readMethod = std::bind(&RedditConnection::onRead,this->shared_from_this(),_1,_2);
            if(isSsl)
            {
                boost::beast::http::async_read(stream.value(), readBuffer, responseParser.value(), readMethod);
            }
            else
            {
                boost::beast::http::async_read(stream.value().next_layer(), readBuffer, responseParser.value(), readMethod);
            }
        }
    }
    virtual void handleLocationChange(const std::string& location)
    {
        boost::ignore_unused(location);
    }
    virtual void onRead(const boost::system::error_code& ec,std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if(ec)
        {
            onError(ec);
            return;
        }
        {
            std::lock_guard<std::mutex> _(queuedRequestsMutex);
            if(queuedRequests.empty())
            {
                responseReceivedComplete();
            }
            else
            {
                responseReceivedComplete(std::move(queuedRequests.front().userData));
                queuedRequests.pop_front();
                if(!queuedRequests.empty())
                {
                    performRequest(std::move(queuedRequests.front().request));
                }
            }
        }
    }

    virtual void responseReceivedComplete() {};
    virtual void responseReceivedComplete(typename ClientResponse::user_type userData) {
        boost::ignore_unused(userData);
    };

    void fillResponseHeaders(ClientResponse& resp)
    {
        for (const auto& h : responseParser->get())
        {
            if (h.name() == boost::beast::http::field::content_length)
            {
#if BOOST_VERSION < 108000
                std::string val = h.value().to_string();
#else
                std::string val = h.value();
#endif // BOOST_VERSION < 108000
                resp.contentLength = std::atol(val.c_str());
                //std::from_chars(val.data(), val.data() + val.size(), resp.contentLength);
            }
            else if (h.name() == boost::beast::http::field::content_type)
            {
#if BOOST_VERSION < 108000
                resp.contentType = h.value().to_string();
#else
                resp.contentType = h.value();
#endif // BOOST_VERSION < 108000
            }
        }
    }
protected:    
    boost::asio::ssl::context& ssl_context;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    boost::asio::ip::tcp::resolver resolver;
    std::optional<stream_t> stream;
    boost::beast::flat_buffer readBuffer; // (Must persist between reads)
    std::string host;
    std::string service;
    boost::signals2::signal<void(const boost::system::error_code&,ClientResponse)> signal;
    std::optional<response_parser_t> responseParser;
    bool isSsl = true;
    bool connected = false;
#ifdef REDDIT_DESKTOP_DEBUG
    std::chrono::seconds streamTimeout{120};
#else
    std::chrono::seconds streamTimeout{30};
#endif
    using queued_request_t = queued_request<typename ClientResponse::user_type>;
    request_t privateRequest;
    std::deque<queued_request_t> queuedRequests;
    std::mutex queuedRequestsMutex;
};

template<typename T>
using RedditGetConnection = RedditConnection<boost::beast::http::empty_body,
                                        boost::beast::http::string_body,
                                        T>;
template<typename T>
using RedditPostConnection = RedditConnection<boost::beast::http::string_body,
                                        boost::beast::http::string_body,
                                        T>;

#endif // REDDITCONNECTION_H
