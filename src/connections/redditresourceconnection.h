#ifndef REDDITRESOURCECONNECTION_H
#define REDDITRESOURCECONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <boost/beast/http.hpp>

using response_byte = unsigned char;
using resource_response = client_response<std::vector<response_byte>>;
using resource_response_body = boost::beast::http::vector_body<response_byte>;
class RedditResourceConnection : public RedditConnection<
        boost::beast::http::empty_body,
        resource_response_body,
        resource_response
        >
{
public:
    RedditResourceConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,                             
                             const std::string& userAgent);
    void getResource(const std::string& url, std::any userData = std::any());
    void getResourceAuth(const std::string& url, const access_token& token, std::any userData = std::any());
protected:
    virtual void sendRequest(request_t request) override;
    virtual void responseReceivedComplete(std::any userData) override;
    virtual void handleLocationChange(const std::string& location) override;
    virtual void performRequest(request_t request) override;
private:
    request_t createRequest(const std::string& url);
private:
    std::string userAgent;
    std::string newHost;
    std::string newService;
};

#endif // REDDITRESOURCECONNECTION_H
