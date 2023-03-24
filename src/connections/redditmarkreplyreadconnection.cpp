#include "redditmarkreplyreadconnection.h"
#include <fmt/format.h>
#include "json.hpp"
#include <charconv>
#include "htmlparser.h"

RedditMarkReplyReadConnection::RedditMarkReplyReadConnection(const boost::asio::any_io_executor& executor,
                                                             boost::asio::ssl::context& ssl_context,
                                                             const std::string& host, const std::string& service,
                                                             const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{

}
void RedditMarkReplyReadConnection::markReplyRead(const std::vector<std::string>& ids,
                                                  const access_token& token, bool read,
                                                  std::any userData)
{
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target(read ? "/api/read_message" : "/api/unread_message");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    std::string idsParam;
    std::string commaSeparator;
    for(const auto& id : ids)
    {
        idsParam+=std::exchange(commaSeparator,"%2C")+id;
    }
    request.body() = fmt::format("api_type=json&id={}",idsParam);
    request.prepare_payload();
    enqueueRequest(std::move(request), std::move(userData));
}

void RedditMarkReplyReadConnection::responseReceivedComplete(std::any userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<std::string> resp;
    fillResponseHeaders(resp);
    resp.status = status;
    resp.body = body;
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}
