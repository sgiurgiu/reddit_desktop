#include "redditsrsubscriptionconnection.h"
#include "htmlparser.h"
#include <fmt/format.h>
#include "json.hpp"

RedditSRSubscriptionConnection::RedditSRSubscriptionConnection(const boost::asio::any_io_executor& executor,
                                                               boost::asio::ssl::context& ssl_context,
                                                               const std::string& host, const std::string& service,
                                                               const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{

}
void RedditSRSubscriptionConnection::updateSrSubscription(const std::vector<std::string>& subreddits,
                          SubscriptionAction action,
                          const access_token& token)
{
    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/subscribe");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");

    auto subredditsList = HtmlParser::escape(fmt::format("{}",fmt::join(subreddits,",")));

    request.body() = fmt::format("sr={}&action={}",
                                 subredditsList,(action == SubscriptionAction::Subscribe ? "sub" : "unsub"));

    request.prepare_payload();
    responseParser->get().body().clear();
    responseParser->get().clear();
    performRequest(std::move(request));

}

void RedditSRSubscriptionConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<bool> resp;
    resp.status = status;
    resp.body = body;
    resp.data = status == 200;
    signal({},std::move(resp));
}
