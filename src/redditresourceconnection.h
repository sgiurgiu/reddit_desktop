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
        boost::beast::http::empty_body,
        resource_response_body,
        const resource_response&
        >
{
public:
    RedditResourceConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,                             
                             const std::string& userAgent);
    void getResource(const std::string& url, void* userData = nullptr);
protected:
    virtual void sendRequest(request_t request) override;
    virtual void responseReceivedComplete(void* userData) override;
    virtual void handleLocationChange(const std::string& location) override;
    virtual void performRequest(request_t request) override;

private:
    std::string userAgent;
    std::string newHost;
    std::string newService;
};

#endif // REDDITRESOURCECONNECTION_H
