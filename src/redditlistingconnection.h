#ifndef REDDITLISTINGCONNECTION_H
#define REDDITLISTINGCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

class RedditListingConnection : public RedditConnection<
        boost::beast::http::empty_body,
        boost::beast::http::string_body,
        const client_response<listing>&>
{
public:
    RedditListingConnection(const boost::asio::any_io_executor& executor,
                  boost::asio::ssl::context& ssl_context,
                  const std::string& host, const std::string& service,
                  const std::string& userAgent );

    void list(const std::string& target, const access_token& token, void* userData = nullptr);
protected:
    virtual void responseReceivedComplete(void* userData) override;
private:
    std::string userAgent;
};

#endif // REDDITLISTING_H
