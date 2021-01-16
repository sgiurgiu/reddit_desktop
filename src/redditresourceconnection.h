#ifndef REDDITRESOURCECONNECTION_H
#define REDDITRESOURCECONNECTION_H

#include "redditconnection.h"

#include "entities.h"
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

using response_byte = unsigned char;
using resource_response = client_response<std::vector<response_byte>>;
using resource_response_body = boost::beast::http::vector_body<response_byte>;
class RedditResourceConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::empty_body>,
        boost::beast::http::response<resource_response_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                  const resource_response&)>
        >
{
public:
    RedditResourceConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,                             
                             const std::string& userAgent);
    void getResource(const std::string& url);
protected:
    virtual void responseReceivedComplete() override;
    virtual void handleLocationChange(const std::string& location) override;

private:
    //boost::beast::http::response_parser<resource_response_body> parser;
    std::string userAgent;
};

#endif // REDDITRESOURCECONNECTION_H
