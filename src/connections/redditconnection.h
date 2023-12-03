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

#include "proxy/proxy.h"
#include "proxy/socks5.hpp"
#include "proxy/socks4.hpp"
#include "../database.h"

#define MAX_RETRIES_ON_ERROR 5

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
        stream(std::make_optional<stream_t>(strand, ssl_context)),
        responseParser(std::make_optional<response_parser_t>()),host(host),service(service),
        resolver(strand)
    {
        proxy = Database::getInstance()->getProxy();
                //std::make_optional<proxy_t>("localhost", "1080", "", "", PROXY_TYPE_SOCKS5,true);
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
            connected = false;
            onError(ec,std::move(request));
            return;
        }
        connected = true;
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

        if(proxy && proxy->useProxy)
        {
            boost::system::error_code proxyEc;
            boost::asio::ip::tcp::resolver::results_type::endpoint_type connectedEndpoint;
            auto proxyResults = resolver.resolve(proxy->host, proxy->port);
            for(const auto& hostResult : results)
            {
                if(proxy->proxyType == proxy::PROXY_TYPE_SOCKS5)
                {
                    socks5::proxy_connect(stream->next_layer(),hostResult.endpoint(),proxyResults, proxyEc);
                }
                else if(proxy->proxyType == proxy::PROXY_TYPE_SOCKS4)
                {
                    socks4::proxy_connect(stream->next_layer(),hostResult.endpoint(),proxyResults, proxy->username, proxyEc);
                }
                else
                {
                    proxyEc = socks5::make_error_code(socks5::result_code::invalid_version);
                }
                if(proxyEc)
                {
                    auto& cat = proxyEc.category();
                    if(&cat == &socks5::get_result_category() || &cat == &socks4::get_result_category())
                    {
                        //it means the proxy cannot connect to the host
                        continue;
                    }
                    else
                    {
                        //there's a problem connecting to the proxy
                        onError(proxyEc,std::move(request));
                        return;
                    }
                }
                connectedEndpoint = hostResult.endpoint();
                break;
            }
            if(proxyEc)
            {
                onError(proxyEc,std::move(request));
                return;
            }
            boost::asio::post(stream->get_executor(),[self = this->shared_from_this(),
                              proxyEc = std::move(proxyEc), connectedEndpoint = std::move(connectedEndpoint), request = std::move(request)]() mutable
            {
                self->onConnect(std::move(proxyEc), std::move(connectedEndpoint), std::move(request));
            });
        }
        else
        {
            using namespace std::placeholders;
            auto connectMethod = std::bind(&RedditConnection::onConnect,this->shared_from_this(),_1,_2,std::move(request));
            // Make the connection on the IP address we get from a lookup
            boost::beast::get_lowest_layer(stream.value()).async_connect(results,connectMethod);
        }
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

    void onReadHeader(const boost::system::error_code& ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);
        if (ec)
        {
            onError(ec);
            return;
        }
        std::string location;
        for (const auto& h : responseParser->get())
        {
            if (h.name() == boost::beast::http::field::location)
            {
#if BOOST_VERSION < 108000
                location = h.value().to_string();
#else
                location = h.value();
#endif // BOOST_VERSION < 108000
                break;
            }
        }
        if (!location.empty())
        {
            auto status = responseParser->get().result();

            if (status == boost::beast::http::status::moved_permanently ||
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
            auto readMethod = std::bind(&RedditConnection::onRead, this->shared_from_this(), _1, _2);
            if (isSsl)
            {
                boost::beast::http::async_read(stream.value(), readBuffer, responseParser.value(), readMethod);
            }
            else
            {
                boost::beast::http::async_read(stream.value().next_layer(), readBuffer, responseParser.value(), readMethod);
            }
        }
    }

    template<typename U>
    struct queued_request
    {

        queued_request(request_t request, U userData) :
            request(std::move(request)), userData(std::move(userData))
        {}
        request_t request;
        U userData;
    };

    void resolveHost(request_t request)
    {
        using namespace std::placeholders;
        auto resolverMethod = std::bind(&RedditConnection::resolveComplete, this->shared_from_this(),
            _1, _2, std::move(request));
        resolver.async_resolve(host, service, resolverMethod);
    }

protected:

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

    virtual void sendRequest(request_t request)
    {
        boost::beast::get_lowest_layer(stream.value()).expires_after(streamTimeout);
        using namespace std::placeholders;
        auto writeMethod = std::bind(&RedditConnection::onWrite, this->shared_from_this(), _1, _2);
        this->privateRequest = std::move(request);
        // Send the HTTP request to the remote host
        if (isSsl)
        {
            boost::beast::http::async_write(stream.value(), privateRequest, writeMethod);
        }
        else
        {
            boost::beast::http::async_write(stream.value().next_layer(), privateRequest, writeMethod);
        }
    }

    virtual void performRequest(request_t request)
    {
        readBuffer.clear();
        responseParser.emplace();
        if(connected)
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

    virtual void onWrite(const boost::system::error_code& ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (ec)
        {
            onError(ec, std::move(this->privateRequest));
            return;
        }
        retriesOnError = 0;
        using namespace std::placeholders;
        auto readMethod = std::bind(&RedditConnection::onReadHeader, this->shared_from_this(), _1, _2);
        if (isSsl)
        {
            boost::beast::http::async_read_header(stream.value(), readBuffer, responseParser.value(), readMethod);
        }
        else
        {
            boost::beast::http::async_read_header(stream.value().next_layer(), readBuffer, responseParser.value(), readMethod);
        }
    }

    virtual void onRead(const boost::system::error_code& ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (ec)
        {
            onError(ec);
            return;
        }
        {
            std::lock_guard<std::mutex> _(queuedRequestsMutex);
            if (queuedRequests.empty())
            {
                responseReceivedComplete();
            }
            else
            {
                responseReceivedComplete(std::move(queuedRequests.front().userData));
                queuedRequests.pop_front();
                if (!queuedRequests.empty())
                {
                    performRequest(std::move(queuedRequests.front().request));
                }
            }
        }
    }

    virtual void onError(const boost::system::error_code& ec,request_t request)
    {
        if(boost::beast::http::error::end_of_stream == ec ||
            boost::asio::ssl::error::stream_truncated == ec)
        {            
            //we got the connection closed on us, reconnect
            connected = false;
            stream.emplace(strand, ssl_context);
            responseParser.reset();
            responseParser.emplace();
            if(retriesOnError < MAX_RETRIES_ON_ERROR)
            {
                retriesOnError++;
                resolveHost(std::move(request));
            }
            else
            {
                signal(ec,{});
            }
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


    virtual void handleLocationChange(const std::string& location)
    {
        boost::ignore_unused(location);
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
                try
                {
                    resp.contentLength = std::stol(val);
                }
                catch (...)
                {
                    resp.contentLength = 0;
                }
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
    std::optional<stream_t> stream;
    std::optional<response_parser_t> responseParser;
    std::string host;
    std::string service;
    boost::beast::flat_buffer readBuffer; // (Must persist between reads)
    bool isSsl = true;
    using queued_request_t = queued_request<typename ClientResponse::user_type>;
    std::deque<queued_request_t> queuedRequests;
    boost::signals2::signal<void(const boost::system::error_code&,ClientResponse)> signal;
    std::mutex queuedRequestsMutex;
    bool connected = false;

private:
    boost::asio::ip::tcp::resolver resolver;
#ifdef REDDIT_DESKTOP_DEBUG
    std::chrono::seconds streamTimeout{120};
#else
    std::chrono::seconds streamTimeout{30};
#endif
    request_t privateRequest;
    int retriesOnError = 0;
    std::optional<proxy::proxy_t> proxy;
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
