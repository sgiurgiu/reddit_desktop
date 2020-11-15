#include "redditloginconnection.h"

#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"

RedditLoginConnection::RedditLoginConnection(boost::asio::io_context& context,
                                             boost::asio::ssl::context& ssl_context,
                                             const std::string& host,
                                             const std::string& service):
    RedditConnection(context,ssl_context),host(host),service(service)
{
}

void RedditLoginConnection::connectLoginComplete(const LoginSlot& slot)
{
    signal.connect(slot);
}

void RedditLoginConnection::login(const user& user)
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
    request.method(boost::beast::http::verb::post);
    request.target("/api/v1/access_token");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "close");
    request.set(boost::beast::http::field::user_agent, make_user_agent(user));
    auto authentication = Utils::encode64(fmt::format("{}:{}",user.client_id,user.secret));
    request.set(boost::beast::http::field::authorization,fmt::format("Basic {}",authentication));
    request.body() = fmt::format("grant_type=password&username={}&password={}",user.username,user.password);
    request.prepare_payload();
    using namespace std::placeholders;
    auto resolverMethod = std::bind(&RedditLoginConnection::resolveComplete,shared_from_this(),
                                    _1,_2);
    resolver.async_resolve(host,service,resolverMethod);
}

void RedditLoginConnection::resolveComplete(const boost::system::error_code& ec,
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
    auto connectMethod = std::bind(&RedditLoginConnection::onConnect,shared_from_this(),_1,_2);

    // Make the connection on the IP address we get from a lookup
    boost::beast::get_lowest_layer(stream).async_connect(results,connectMethod);
}

void RedditLoginConnection::onConnect(const boost::system::error_code& ec,
                      boost::asio::ip::tcp::resolver::results_type::endpoint_type)
{
    if(ec)
    {
        signal(ec,{});
        return;
    }
    using namespace std::placeholders;
    auto handshakeMethod = std::bind(&RedditLoginConnection::onHandshake,shared_from_this(),_1);

    stream.async_handshake(boost::asio::ssl::stream_base::client,handshakeMethod);

}
void RedditLoginConnection::onHandshake(const boost::system::error_code& ec)
{
    if(ec)
    {
        signal(ec,{});
        return;
    }
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto writeMethod = std::bind(&RedditLoginConnection::onWrite,shared_from_this(),_1,_2);

    // Send the HTTP request to the remote host
    boost::beast::http::async_write(stream, request,writeMethod);
}
void RedditLoginConnection::onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if(ec)
    {
        signal(ec,{});
        return;
    }

    using namespace std::placeholders;
    auto readMethod = std::bind(&RedditLoginConnection::onRead,shared_from_this(),_1,_2);
    boost::beast::http::async_read(stream, buffer, response, readMethod);

}

void RedditLoginConnection::onRead(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);
    if(ec)
    {
        signal(ec,{});
        return;
    }
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto shutdownMethod = std::bind(&RedditLoginConnection::onShutdown,shared_from_this(),_1);

    // Gracefully close the stream
    stream.async_shutdown(shutdownMethod);
}
void RedditLoginConnection::onShutdown(const boost::system::error_code&)
{
    auto status = response.result_int();
    auto body = response.body();
    client_response<access_token> token;
    token.status = status;
    if(status == 200)
    {
        auto json = nlohmann::json::parse(body);
        if(json.contains("access_token") && !json["access_token"].is_null())
        {
            token.data.token = json["access_token"].get<std::string>();
        }
        if(json.contains("token_type") && !json["token_type"].is_null())
        {
            token.data.tokenType = json["token_type"].get<std::string>();
        }
        if(json.contains("expires_in") && !json["expires_in"].is_null())
        {
            token.data.expires = json["expires_in"].get<int>();
        }
        if(json.contains("scope") && !json["scope"].is_null())
        {
            token.data.scope = json["scope"].get<std::string>();
        }
    }
    else
    {
        token.body = body;
    }
    signal({},token);
}
