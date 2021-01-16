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
#include <string>
#include <memory>
#include <optional>

template<typename Request,typename Response, typename Signal>
class RedditConnection : public std::enable_shared_from_this<RedditConnection<Request,Response,Signal>>
{
public:
    RedditConnection(const boost::asio::any_io_executor& executor,
                     boost::asio::ssl::context& ssl_context,const std::string& host,
                     const std::string& service):
        ssl_context(ssl_context),strand(boost::asio::make_strand(executor)),
        resolver(strand),stream(std::make_optional<stream_t>(strand, ssl_context)),
        host(host),service(service),responseParser(std::make_optional<parser_t>())
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
                          boost::asio::ip::tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        isSsl = !(service == "http" || service == "80" || service == "8080");
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (isSsl && ! SSL_set_tlsext_host_name(stream->native_handle(), host.c_str()))
        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         boost::asio::error::get_ssl_category()};
            onError(ec);
            return;
        }

        if(!isSsl)
        {
            sendRequest();
        }
        else
        {
            using namespace std::placeholders;
            auto handshakeMethod = std::bind(&RedditConnection::onHandshake,this->shared_from_this(),_1);
            stream->async_handshake(boost::asio::ssl::stream_base::client,handshakeMethod);
        }
    }

    void resolveComplete(const boost::system::error_code& ec,
                                                 boost::asio::ip::tcp::resolver::results_type results)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        // Set a timeout on the operation
        boost::beast::get_lowest_layer(stream.value()).expires_after(streamTimeout);
        using namespace std::placeholders;
        auto connectMethod = std::bind(&RedditConnection::onConnect,this->shared_from_this(),_1,_2);
        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(stream.value()).async_connect(results,connectMethod);
    }

    void onHandshake(const boost::system::error_code& ec)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        connected = true;
        sendRequest();
    }


protected:
    void performRequest()
    {
        if(boost::beast::get_lowest_layer(stream.value()).socket().is_open())
        {
            sendRequest();
        }
        else
        {
            resolveHost();
        }
    }
    template<typename Derived>
    std::shared_ptr<Derived> shared_from_base()
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }

    virtual void onError(const boost::system::error_code& ec)
    {
        if(boost::beast::http::error::end_of_stream == ec)
        {
            //we got the connection closed on us, reconnect
            connected = false;
            stream.emplace(strand, ssl_context);
            responseParser.emplace();
            resolveHost();
        }
        else
        {
            signal(ec,{});
        }
    }

    void resolveHost()
    {
        using namespace std::placeholders;
        auto resolverMethod = std::bind(&RedditConnection::resolveComplete,this->shared_from_this(),
                                        _1,_2);
        resolver.async_resolve(host,service,resolverMethod);
    }

    virtual void sendRequest()
    {
        boost::beast::get_lowest_layer(stream.value()).expires_after(streamTimeout);
        using namespace std::placeholders;
        auto writeMethod = std::bind(&RedditConnection::onWrite,this->shared_from_this(),_1,_2);
        // Send the HTTP request to the remote host
        if(isSsl)
        {
            boost::beast::http::async_write(stream.value(), request,writeMethod);
        }
        else
        {
            boost::beast::http::async_write(stream.value().next_layer(), request,writeMethod);
        }
    }

    virtual void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if(ec)
        {
            onError(ec);
            return;
        }
        response.clear();
        response.body().clear();
        responseParser->get().body().clear();
        responseParser->get().clear();


        using namespace std::placeholders;
        auto readMethod = std::bind(&RedditConnection::onReadHeader,this->shared_from_this(),_1,_2);
        if(isSsl)
        {
            boost::beast::http::async_read_header(stream.value(), buffer, responseParser.value(), readMethod);
        }
        else
        {
            boost::beast::http::async_read_header(stream.value().next_layer(), buffer, responseParser.value(), readMethod);
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
                location = h.value().to_string();
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
                boost::beast::http::async_read(stream.value(), buffer, responseParser.value(), readMethod);
            }
            else
            {
                boost::beast::http::async_read(stream.value().next_layer(), buffer, responseParser.value(), readMethod);
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

        responseReceivedComplete();
    }

    virtual void responseReceivedComplete() = 0;
protected:
    boost::asio::ssl::context& ssl_context;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    boost::asio::ip::tcp::resolver resolver;
    using stream_t = boost::beast::ssl_stream<boost::beast::tcp_stream>;
    std::optional<stream_t> stream;
    boost::beast::flat_buffer buffer; // (Must persist between reads)
    std::string host;
    std::string service;
    Request request;
    Signal signal;
    using parser_t = boost::beast::http::response_parser<typename Response::body_type>;
    std::optional<parser_t> responseParser;
    bool isSsl = true;
    bool connected = false;
#ifdef REDDIT_DESKTOP_DEBUG
    std::chrono::seconds streamTimeout{120};
#else
    std::chrono::seconds streamTimeout{30};
#endif
private:
    Response response;
};

#endif // REDDITCONNECTION_H
