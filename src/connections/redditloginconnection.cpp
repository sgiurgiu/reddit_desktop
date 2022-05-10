#include "redditloginconnection.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"
#include "htmlparser.h"

namespace
{
    static login_error_category cat;
}

RedditLoginConnection::RedditLoginConnection(const boost::asio::any_io_executor& executor,
                                             boost::asio::ssl::context& ssl_context,
                                             const std::string& host,
                                             const std::string& service):
    RedditConnection(executor,ssl_context,host,service)
{
}

void RedditLoginConnection::login(const user& user)
{
    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/v1/access_token");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, make_user_agent(user));
    auto authentication = Utils::encode64(fmt::format("{}:{}",user.client_id,user.secret));
    request.set(boost::beast::http::field::authorization,fmt::format("Basic {}",authentication));
    request.body() = fmt::format("grant_type=password&username={}&password={}",HtmlParser::escape(user.username),HtmlParser::escape(user.password));
    request.prepare_payload();
    responseParser.emplace();
    performRequest(std::move(request));
}

void RedditLoginConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<access_token> token;
    token.status = status;
    boost::system::error_code ec = {};
    if(status == 200)
    {
        try
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
            if (json.contains("error") && json["error"].is_string())
            {
                auto error = json["error"].get<std::string>();
                token.body = error;
                token.status = 500;
                cat.setMessage(error);
                ec.assign(token.status,cat);
            }
            else
            {
                cat.setMessage("");
                ec.assign(status,cat);
            }
        }
        catch(const std::exception& ex)
        {
            token.body = ex.what();
            if(status < 500) token.status = 500;
            cat.setMessage(body);
            ec.assign(token.status,cat);
        }
    }
    else
    {
        token.body = body;
        cat.setMessage(body);
        ec.assign(token.status,cat);
    }
    signal(ec,std::move(token));
}
