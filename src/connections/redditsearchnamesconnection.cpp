#include "redditsearchnamesconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <charconv>
#include <fmt/format.h>
#include "json.hpp"
#include "htmlparser.h"
#include "utils.h"

RedditSearchNamesConnection::RedditSearchNamesConnection(const boost::asio::any_io_executor& executor,
                                                         boost::asio::ssl::context& ssl_context,
                                                         const std::string& host, const std::string& service,
                                                         const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{

}
void RedditSearchNamesConnection::search(const std::string& query, const access_token& token)
{
    auto escapedQuery = HtmlParser::escape(query);
    std::string target = fmt::format("/api/search_reddit_names?raw_json=1&exact=false&"
                                     "include_over_18=true&include_unadvertisable=true&query={}",
                                     escapedQuery);
    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();
    responseParser->get().body().clear();
    responseParser->get().clear();
    performRequest(std::move(request));
}
void RedditSearchNamesConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<names_list> resp;
    fillResponseHeaders(resp);
    resp.status = status;
    if(status == 200)
    {
        try
        {
            auto json = nlohmann::json::parse(body);
            if(json.contains("names") && json["names"].is_array())
            {
                std::move(json["names"].begin(),json["names"].end(),std::back_inserter(resp.data));
            }
            else
            {
                resp.body = body;
            }
        }
        catch(const std::exception& ex)
        {
            resp.body = ex.what();
            if(status < 500) resp.status = 500;
        }
    }
    else
    {
        resp.body = body;
    }
    signal({},std::move(resp));
}

