#include "redditlistingconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <charconv>
#include <fmt/format.h>
#include "json.hpp"

#include "utils.h"

RedditListingConnection::RedditListingConnection(boost::asio::io_context& context,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& host, const std::string& service,
                             const std::string& userAgent):
    RedditConnection(context,ssl_context,host,service),userAgent(userAgent)
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

    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(raw_json_target);
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();
    if(connected)
    {
        sendRequest();
    }
    else
    {
        resolveHost();
    }
}

void RedditListingConnection::sendRequest()
{
    connected = true;
    RedditConnection::sendRequest();
}

void RedditListingConnection::responseReceivedComplete()
{
    auto status = response.result_int();
    auto body = response.body();
    client_response<listing> resp;
    for(const auto& h : response)
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
        resp.data.json = nlohmann::json::parse(body);
    }
    else
    {
        resp.body = body;
    }
    signal({},resp);
}
