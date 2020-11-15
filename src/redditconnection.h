#ifndef REDDITCONNECTION_H
#define REDDITCONNECTION_H

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>

class RedditConnection
{
public:
    RedditConnection(boost::asio::io_context& context,
                     boost::asio::ssl::context& ssl_context);
    virtual ~RedditConnection() = default;
protected:
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream;
};

#endif // REDDITCONNECTION_H
