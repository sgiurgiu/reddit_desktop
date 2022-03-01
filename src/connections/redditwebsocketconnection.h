#ifndef REDDITWEBSOCKETCONNECTION_H
#define REDDITWEBSOCKETCONNECTION_H
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/strand.hpp>
#include <boost/signals2.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <string>
#include <memory>
#include <optional>
#include <chrono>

template<typename ClientUpdate>
class RedditWebSocketConnection :
        public std::enable_shared_from_this<RedditWebSocketConnection<ClientUpdate>>
{
protected:
    using stream_t = boost::beast::ssl_stream<boost::beast::tcp_stream>;
    using ws_stream_t = boost::beast::websocket::stream<stream_t>;
public:
    RedditWebSocketConnection(const boost::asio::any_io_executor& executor,
                     boost::asio::ssl::context& ssl_context):
        ssl_context(ssl_context),strand(boost::asio::make_strand(executor)),
        resolver(strand),stream(strand, ssl_context)
    {
    }
    virtual ~RedditWebSocketConnection()
    {
        clearAllSlots();
    }

    void clearAllSlots()
    {
        signal.disconnect_all_slots();
    }
    template<typename Slot>
    void onUpdate(Slot slot)
    {
        signal.connect(std::move(slot));
    }
    void close()
    {
        // Close the WebSocket connection
        stream.async_close(boost::beast::websocket::close_code::normal,
            boost::beast::bind_front_handler(
                &RedditWebSocketConnection::onClose,
                this->shared_from_this()));
    }
private:
    void onResolve(boost::beast::error_code ec,
                boost::asio::ip::tcp::resolver::results_type results)
    {
        if(ec)
        {
            onError(ec);
            return;
        }

        boost::beast::get_lowest_layer(stream).expires_after(streamTimeout);
        boost::beast::get_lowest_layer(stream).async_connect(
            results,
            boost::beast::bind_front_handler(&RedditWebSocketConnection::onConnect,this->shared_from_this()));
    }
protected:
    template<typename Derived>
    std::shared_ptr<Derived> shared_from_base()
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }
    virtual void onClose(boost::beast::error_code)
    {
    }
    void connect(const std::string& host, const std::string& port, const std::string& path)
    {
        this->host = host;
        this->path = path;
        resolver.async_resolve(host,port,
                    boost::beast::bind_front_handler(
                        &RedditWebSocketConnection::onResolve,this->shared_from_this()));
    }
    virtual void onError(const boost::system::error_code& ec)
    {
        if(boost::beast::http::error::end_of_stream == ec)
        {
            //we got the connection closed on us, reconnect
           // connected = false;
           // stream.emplace(strand, ssl_context);
            //resolveHost(std::move(request));
        }
        else
        {

        }
        signal(ec,{});
    }
    virtual void onConnect(const boost::system::error_code& ec,
                           boost::asio::ip::tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (! SSL_set_tlsext_host_name(stream.next_layer().native_handle(), host.c_str()))
        {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         boost::asio::error::get_ssl_category()};
            onError(ec);
            return;
        }

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host += ':' + std::to_string(ep.port());
        // Perform the SSL handshake
        stream.next_layer().async_handshake(
            boost::asio::ssl::stream_base::client,
            boost::beast::bind_front_handler(&RedditWebSocketConnection::onSSLHandshake,this->shared_from_this()));
    }

    virtual void onSSLHandshake(const boost::system::error_code& ec)
    {
        if(ec)
        {
            onError(ec);
            return;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        boost::beast::get_lowest_layer(stream).expires_never();
        // Set suggested timeout settings for the websocket
        stream.set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                        boost::beast::role_type::client));
        // Set a decorator to change the User-Agent of the handshake
        stream.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req)
        {
            req.set(boost::beast::http::field::user_agent,
                std::string(REDDITDESKTOP_VERSION) + " Websocket Client");
        }));
        // Perform the websocket handshake
        stream.async_handshake(host, path,
            boost::beast::bind_front_handler(&RedditWebSocketConnection::onHandshake,this->shared_from_this()));
    }

    virtual void onHandshake(const boost::system::error_code& ec)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        stream.text(true);
        readMessage();
    }

    virtual void readMessage()
    {        
        stream.async_read(
            readBuffer,
            boost::beast::bind_front_handler(
                &RedditWebSocketConnection::onRead,this->shared_from_this()));
    }

    virtual void onRead(boost::beast::error_code ec,std::size_t)
    {
        if(ec)
        {
            onError(ec);
            return;
        }
        messageReceived();
        readBuffer = {};
        readMessage();
    }

    virtual void messageReceived() = 0;

protected:
    boost::asio::ssl::context& ssl_context;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::flat_buffer readBuffer; // (Must persist between reads)
    std::string host;
    std::string path;
    std::string service;
    boost::signals2::signal<void(const boost::system::error_code&,ClientUpdate)> signal;
    ws_stream_t stream;
#ifdef REDDIT_DESKTOP_DEBUG
    std::chrono::seconds streamTimeout{120};
#else
    std::chrono::seconds streamTimeout{30};
#endif

};

#endif // REDDITWEBSOCKETCONNECTION_H
