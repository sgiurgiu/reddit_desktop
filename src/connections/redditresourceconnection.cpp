#include "redditresourceconnection.h"
#include <fmt/format.h>
#include <charconv>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http/parser.hpp>
#include "uri.h"

namespace
{
//We're reading max 150MB as a resource
//For more than that we're gonna have to go streaming or something
constexpr auto BUFFER_SIZE = 150*1024*1024;
}

RedditResourceConnection::RedditResourceConnection(const boost::asio::any_io_executor& executor,
                                                   boost::asio::ssl::context& ssl_context,                                                   
                                                   const std::string& userAgent):
    RedditConnection(executor,ssl_context,"",""),userAgent(userAgent)
{    
}

RedditResourceConnection::request_t RedditResourceConnection::createRequest(const std::string& url)
{
    Uri urlParts(url);

    newService = urlParts.port().empty() ? urlParts.scheme() : urlParts.port();
    newHost = urlParts.host();
    if(host.empty())
    {
        host = newHost;
    }
    if(service.empty())
    {
        service = newService;
    }
    auto target = urlParts.fullPath();
    if(!urlParts.query().empty())
    {
        target+="?"+urlParts.query();
    }

    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, newHost);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    return request;
}
void RedditResourceConnection::getResourceAuth(const std::string& url, const access_token& token, std::any userData)
{
    auto request = createRequest(url);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();
    enqueueRequest(std::move(request),std::move(userData));
}
void RedditResourceConnection::getResource(const std::string& url,std::any userData)
{
    auto request = createRequest(url);
    request.prepare_payload();
    enqueueRequest(std::move(request),std::move(userData));
}
void RedditResourceConnection::performRequest(request_t request)
{
    if((newHost != host) || (service != newService))
    {
        stream.emplace(strand,ssl_context);
        host = newHost;
        service = newService;
    }

    RedditConnection::performRequest(std::move(request));
}
void RedditResourceConnection::handleLocationChange(const std::string& location)
{
    {
        std::lock_guard<std::mutex> _(queuedRequestsMutex);
        if(!queuedRequests.empty())
        {
            queuedRequests.pop_front();
        }
    }
    // we cannot handle auth request with a location change
    // hopefully that will never happen
    getResource(location);
}
void RedditResourceConnection::sendRequest(request_t request)
{
    responseParser->body_limit(BUFFER_SIZE);
    RedditConnection::sendRequest(std::move(request));
}
void RedditResourceConnection::responseReceivedComplete(std::any userData)
{
    auto status = responseParser->get().result_int();
    resource_response resp;
    resp.status = status;

    resp.data = responseParser->get().body();
    fillResponseHeaders(resp);
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}

