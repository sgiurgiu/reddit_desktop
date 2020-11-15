#include "redditlistingconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"

RedditListingConnection::RedditListingConnection(boost::asio::io_context& context,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& host, const std::string& service,
                             const std::string& userAgent):RedditConnection(context,ssl_context),
                             host(host),service(service),userAgent(userAgent)
{
}
void RedditListingConnection::listingComplete(const ListingSlot& slot)
{
    signal.connect(slot);
}
void RedditListingConnection::list(const std::string& target, const access_token& token)
{
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
    {
        boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                     boost::asio::error::get_ssl_category()};
        signal(ec,{});
        return;
    }

    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();
    using namespace std::placeholders;
    if(connected)
    {
        // Set a timeout on the operation
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        using namespace std::placeholders;
        auto writeMethod = std::bind(&RedditListingConnection::onWrite,shared_from_this(),
                                        _1,_2);

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(stream, request,writeMethod);
    }
    else
    {
        auto resolverMethod = std::bind(&RedditListingConnection::resolveComplete,shared_from_this(),
                                        _1,_2);
        resolver.async_resolve(host,service,resolverMethod);
    }
}

void RedditListingConnection::resolveComplete(const boost::system::error_code& ec,
                                             boost::asio::ip::tcp::resolver::results_type results)
{
    if(ec)
    {
        signal(ec,{});
        return;
    }
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto connectMethod = std::bind(&RedditListingConnection::onConnect,shared_from_this(),
                                    _1,_2);

    // Make the connection on the IP address we get from a lookup
    boost::beast::get_lowest_layer(stream).async_connect(results,connectMethod);
}

void RedditListingConnection::onConnect(const boost::system::error_code& ec,
                      boost::asio::ip::tcp::resolver::results_type::endpoint_type)
{
    if(ec)
    {
        signal(ec,{});
        return;
    }
    using namespace std::placeholders;
    auto handshakeMethod = std::bind(&RedditListingConnection::onHandshake,shared_from_this(),
                                    _1);

    stream.async_handshake(boost::asio::ssl::stream_base::client,handshakeMethod);

}
void RedditListingConnection::onHandshake(const boost::system::error_code& ec)
{
    if(ec)
    {
        signal(ec,{});
        return;
    }
    connected = true;
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto writeMethod = std::bind(&RedditListingConnection::onWrite,shared_from_this(),
                                    _1,_2);

    // Send the HTTP request to the remote host
    boost::beast::http::async_write(stream, request,writeMethod);
}
void RedditListingConnection::onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if(ec)
    {
        signal(ec,{});
        return;
    }

    using namespace std::placeholders;
    auto readMethod = std::bind(&RedditListingConnection::onRead,shared_from_this(),
                                    _1,_2);
    boost::beast::http::async_read(stream, buffer, response, readMethod);

}

void RedditListingConnection::onRead(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);
    if(ec)
    {
        signal(ec,{});
        return;
    }
    auto status = response.result_int();
    auto body = response.body();
    client_response<listing> response;
    response.status = status;
    if(status == 200)
    {
        response.data.json = nlohmann::json::parse(body);
    }
    else
    {
        response.body = body;
    }
    signal({},response);
}
