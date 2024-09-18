#include "redditsrsubscriptionconnection.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

RedditSRSubscriptionConnection::RedditSRSubscriptionConnection(
    const boost::asio::any_io_executor& executor,
    boost::asio::ssl::context& ssl_context,
    const std::string& host,
    const std::string& service,
    const std::string& userAgent)
: RedditConnection(executor, ssl_context, host, service), userAgent(userAgent)
{
}
void RedditSRSubscriptionConnection::updateSrSubscription(
    const std::vector<std::string>& subreddits,
    SubscriptionAction action,
    const access_token& token)
{
    this->action = action;
    this->token = token;
    int split_size = 10; // we found that subscribing to too many subreddits at
                         // once can error out
    for (size_t i = 0; i < subreddits.size(); i += split_size)
    {
        auto last = std::min(subreddits.size(), i + split_size);
        subredditsChunks.emplace_back(subreddits.begin() + i,
                                      subreddits.begin() + last);
    }
    currentPos = subredditsChunks.cbegin();
    resp.data = true;
    subscribeToCurrentChunk();
}

void RedditSRSubscriptionConnection::subscribeToCurrentChunk()
{
    if (action == SubscriptionAction::NotSet ||
        currentPos == subredditsChunks.cend())
        return;
    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/subscribe");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,
                fmt::format("Bearer {}", token.token));
    request.set(boost::beast::http::field::content_type,
                "application/x-www-form-urlencoded");

    auto subredditsList = fmt::format("{}", fmt::join(*currentPos, ","));

    request.body() =
        fmt::format("sr={}&action={}", subredditsList,
                    (action == SubscriptionAction::Subscribe ? "sub" : "unsub"));

    request.prepare_payload();
    responseParser->get().body().clear();
    responseParser->get().clear();
    performRequest(std::move(request));
}

void RedditSRSubscriptionConnection::responseReceivedComplete()
{
    ++currentPos;
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();

    if (status == 200 && resp.data)
    {
        resp.body = body;
        resp.data = status == 200;
        resp.status = status;
    }
    else if (resp.data)
    {
        resp.body = body;
        resp.data = false;
        resp.status = status;
    }

    if (currentPos == subredditsChunks.cend())
    {
        action = SubscriptionAction::NotSet;
        signal({}, resp);
    }
    else
    {
        subscribeToCurrentChunk();
    }
}
