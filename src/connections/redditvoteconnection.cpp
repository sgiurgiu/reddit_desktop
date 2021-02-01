#include "redditvoteconnection.h"
#include <fmt/format.h>
#include "json.hpp"
#include <charconv>

RedditVoteConnection::RedditVoteConnection(const boost::asio::any_io_executor& executor,
                                           boost::asio::ssl::context& ssl_context,
                                           const std::string& host, const std::string& service,
                                           const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{
}
void RedditVoteConnection::vote(const std::string& id,const access_token& token, Voted vote, std::any userData)
{

    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/vote");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    auto body = fmt::format("dir={}&id={}",static_cast<int>(vote),id);
    request.body() = body;

    this->id = id;
    request.prepare_payload();

    enqueueRequest(std::move(request),std::move(userData));
}

void RedditVoteConnection::responseReceivedComplete(std::any userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<std::string> resp;
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
        resp.data = id;
    }
    else
    {
        resp.body = body;
    }
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}
