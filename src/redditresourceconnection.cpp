#include "redditresourceconnection.h"
#include <boost/url.hpp>
#include <fmt/format.h>

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
}

void RedditResourceConnection::getResource(const access_token& token)
{
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "close");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    //request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.prepare_payload();

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
void RedditResourceConnection::onShutdown(const boost::system::error_code&)
{
    auto status = response.result_int();
    resource_response resp;
    resp.status = status;
    resp.data = response.body();
    signal({},resp);
}
