#include "redditlistingconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <charconv>
#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"

RedditListingConnection::RedditListingConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& host, const std::string& service,
                             const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{
}

void RedditListingConnection::list(const std::string& target, const access_token& token)
{
    auto raw_json_target = target;
    if(raw_json_target.find('?') == std::string::npos)
    {
        raw_json_target+="?raw_json=1";
    }
    else
    {
        raw_json_target+="&raw_json=1";
    }
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(raw_json_target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "application/json");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();
    performRequest();
}

void RedditListingConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<listing> resp;
    for(const auto& h : responseParser->get())
    {
        if(h.name() == boost::beast::http::field::content_length)
        {
            auto val = h.value();
            std::from_chars(val.data(),val.data()+val.size(),resp.contentLength);
        }
        else if(h.name() == boost::beast::http::field::content_type)
        {
            resp.contentType = h.value().to_string();
        }
    }
    resp.status = status;
    if(status == 200)
    {
        try
        {
            resp.data.json = nlohmann::json::parse(body);
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
    signal({},resp);
}
