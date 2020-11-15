#include "redditconnection.h"
#include <boost/asio/strand.hpp>

RedditConnection::RedditConnection(boost::asio::io_context& context,
                                   boost::asio::ssl::context& ssl_context):
    resolver(boost::asio::make_strand(context)),
    stream(boost::asio::make_strand(context), ssl_context)
{

}
