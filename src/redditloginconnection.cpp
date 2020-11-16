#include "redditloginconnection.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"

RedditLoginConnection::RedditLoginConnection(boost::asio::io_context& context,
                                             boost::asio::ssl::context& ssl_context,
                                             const std::string& host,
                                             const std::string& service):
    RedditConnection(context,ssl_context,host,service)
{
}


void RedditLoginConnection::login(const user& user)
{
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

    resolveHost();
}

void RedditLoginConnection::responseReceivedComplete()
{
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto shutdownMethod = std::bind(&RedditLoginConnection::onShutdown,
                                    shared_from_base<RedditLoginConnection>(),_1);

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
        if(json.contains("access_token") && json["access_token"].is_string())
        {
            token.data.token = json["access_token"].get<std::string>();
        }
        if(json.contains("token_type") && json["token_type"].is_string())
        {
            token.data.tokenType = json["token_type"].get<std::string>();
        }
        if(json.contains("expires_in") && json["expires_in"].is_number())
        {
            token.data.expires = json["expires_in"].get<int>();
        }
        if(json.contains("scope") && json["scope"].is_string())
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
