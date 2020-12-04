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

RedditResourceConnection::RedditResourceConnection(boost::asio::io_context& context,
                                                   boost::asio::ssl::context& ssl_context,
                                                   const std::string& url,
                                                   const std::string& userAgent):
    RedditConnection(context,ssl_context,"",""),userAgent(userAgent)
{
    boost::url_view urlParts(url);

    service = urlParts.port().empty() ? urlParts.scheme().to_string() : urlParts.port().to_string();
    host = urlParts.encoded_host().to_string();
    target = urlParts.encoded_path().to_string();
    if(!urlParts.encoded_query().empty())
    {
        target+="?"+urlParts.encoded_query().to_string();
    }
    parser.body_limit(BUFFER_SIZE);
}

void RedditResourceConnection::getResource()
{
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "close");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);    
    request.prepare_payload();
    response.clear();
    response.body().clear();
    parser.get().body().clear();
    parser.get().clear();
    resolveHost();
}

void RedditResourceConnection::responseReceivedComplete()
{
    // Set a timeout on the operation
    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    using namespace std::placeholders;
    auto shutdownMethod = std::bind(&RedditResourceConnection::onShutdown,
                                    shared_from_base<RedditResourceConnection>(),_1);

    // Gracefully close the stream
    stream.async_shutdown(shutdownMethod);
}
void RedditResourceConnection::onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if(ec)
    {
        onError(ec);
        return;
    }
    response.clear();

    using namespace std::placeholders;
    auto readMethod = std::bind(&RedditResourceConnection::onRead,this->shared_from_base<RedditResourceConnection>(),_1,_2);
    boost::beast::http::async_read(stream, buffer, parser, readMethod);
}

void RedditResourceConnection::onRead(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);
    if(ec)
    {
        onError(ec);
        return;
    }

    responseReceivedComplete();
}
void RedditResourceConnection::onShutdown(const boost::system::error_code&)
{
    auto status = response.result_int();
    resource_response resp;
    resp.status = status;

    resp.data = parser.get().body();
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
    signal({},resp);
}
