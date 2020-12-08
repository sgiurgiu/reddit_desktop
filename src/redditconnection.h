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

template<typename Request,typename Response, typename Signal, bool isStreaming = false>
class RedditConnection : public std::enable_shared_from_this<RedditConnection<Request,Response,Signal,isStreaming>>
{
public:
    RedditConnection(boost::asio::io_context& context,
                     boost::asio::ssl::context& ssl_context,const std::string& host,
                     const std::string& service):
        resolver(boost::asio::make_strand(context)),
        stream(boost::asio::make_strand(context), ssl_context),buffer(1024*1024*50),
        host(host),service(service)
    {
        responseParser.body_limit((std::numeric_limits<std::uint64_t>::max)());
    }
    virtual ~RedditConnection()
    {
        clearAllSlots();
    }

    void clearAllSlots()
    {
        signal.disconnect_all_slots();
    }

    void connectionCompleteHandler(const typename Signal::slot_type& slot)
    {
        signal.connect(slot);        
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
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         boost::asio::error::get_ssl_category()};
            onError(ec);
            return;
        }

        using namespace std::placeholders;
        auto handshakeMethod = std::bind(&RedditConnection::onHandshake,this->shared_from_this(),_1);
        stream.async_handshake(boost::asio::ssl::stream_base::client,handshakeMethod);
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
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        using namespace std::placeholders;
        auto connectMethod = std::bind(&RedditConnection::onConnect,this->shared_from_this(),_1,_2);

        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(stream).async_connect(results,connectMethod);
    }

    void onHandshake(const boost::system::error_code& ec)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        sendRequest();
    }




protected:
    template<typename Derived>
    std::shared_ptr<Derived> shared_from_base()
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }

    virtual void onError(const boost::system::error_code& ec)
    {
        signal(ec,{});
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
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        using namespace std::placeholders;
        auto writeMethod = std::bind(&RedditConnection::onWrite,this->shared_from_this(),_1,_2);
        // Send the HTTP request to the remote host
        boost::beast::http::async_write(stream, request,writeMethod);
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

        using namespace std::placeholders;
        auto readMethod = std::bind(&RedditConnection::onRead,this->shared_from_this(),_1,_2);
        if constexpr (isStreaming)
        {
            boost::beast::http::async_read(stream, buffer, responseParser, readMethod);
        }
        else
        {
            boost::beast::http::async_read(stream, buffer, response, readMethod);
        }
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
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream;
    boost::beast::flat_buffer buffer; // (Must persist between reads)
    std::string host;
    std::string service;
    Request request;
    Response response;
    Signal signal;
    using parser_t = boost::beast::http::response_parser<typename Response::body_type>;
    parser_t responseParser;
};

#endif // REDDITCONNECTION_H
