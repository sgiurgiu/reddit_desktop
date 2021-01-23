#include "redditresourceconnection.h"
#include <boost/url.hpp>
#include <fmt/format.h>
#include <charconv>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http/parser.hpp>

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

void RedditResourceConnection::getResource(const std::string& url,void* userData)
{
    boost::url_view urlParts(url);

    service = urlParts.port().empty() ? urlParts.scheme().to_string() : urlParts.port().to_string();
    newHost = urlParts.encoded_host().to_string();
    auto target = urlParts.encoded_path().to_string();
    if(!urlParts.encoded_query().empty())
    {
        target+="?"+urlParts.encoded_query().to_string();
    }

    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, newHost);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);    
    request.prepare_payload();    
    enqueueRequest(std::move(request),userData);
}
void RedditResourceConnection::performRequest(request_t request)
{
    if(newHost != host)
    {
        stream.emplace(strand,ssl_context);
        host = newHost;
    }
    RedditConnection::performRequest(std::move(request));
}
void RedditResourceConnection::handleLocationChange(const std::string& location)
{
    getResource(location);
}
void RedditResourceConnection::sendRequest(request_t request)
{
    responseParser->body_limit(BUFFER_SIZE);
    RedditConnection::sendRequest(std::move(request));
}
void RedditResourceConnection::responseReceivedComplete(void* userData)
{
    auto status = responseParser->get().result_int();
    resource_response resp;
    resp.status = status;

    resp.data = responseParser->get().body();
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
    resp.userData = userData;
    signal({},resp);
}

