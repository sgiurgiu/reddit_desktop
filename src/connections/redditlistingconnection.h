#ifndef REDDITLISTINGCONNECTION_H
#define REDDITLISTINGCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <boost/signals2.hpp>

class RedditListingConnection : public RedditConnection<
        boost::beast::http::empty_body,
        boost::beast::http::string_body,
        client_response<listing>>
{
public:
    RedditListingConnection(const boost::asio::any_io_executor& executor,
                  boost::asio::ssl::context& ssl_context,
                  const std::string& host, const std::string& service,
                  const std::string& userAgent );

    void list(const std::string& target, const access_token& token, void* userData = nullptr);
    template<typename S>
    void targetChangedHandler(S slot)
    {
        targetChangedSignal.connect(slot);
    }
protected:
    virtual void responseReceivedComplete(void* userData) override;
    virtual void handleLocationChange(const std::string& location) override;

private:
    std::string userAgent;
    boost::signals2::signal<void(std::string newTarget, void* userData)> targetChangedSignal;
};

#endif // REDDITLISTING_H
