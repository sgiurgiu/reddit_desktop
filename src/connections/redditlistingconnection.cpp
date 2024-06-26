#include "redditlistingconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/http.hpp>
#include <fmt/format.h>
#include "json.hpp"
#include "uri.h"

RedditListingConnection::RedditListingConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& host, const std::string& service,
                             const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{
}

void RedditListingConnection::list(const std::string& target, const access_token& token, std::any userData)
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
    request_t request;
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

    enqueueRequest(std::move(request),std::move(userData));
}
void RedditListingConnection::handleLocationChange(const std::string& location)
{
    Uri urlParts(location);
    auto target = urlParts.fullPath();

    {
        std::lock_guard<std::mutex> _(queuedRequestsMutex);
        BOOST_ASSERT(!queuedRequests.empty());
        auto userData = std::move(queuedRequests.front().userData);
        queuedRequests.pop_front();
        targetChangedSignal(std::move(target),std::move(userData));
    }
}

void RedditListingConnection::responseReceivedComplete(ListingUserType userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    ListingClientResponse resp;
    fillResponseHeaders(resp);
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
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}
