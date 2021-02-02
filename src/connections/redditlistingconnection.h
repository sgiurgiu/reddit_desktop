#ifndef REDDITLISTINGCONNECTION_H
#define REDDITLISTINGCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <boost/signals2.hpp>

using ListingClientResponse = client_response<listing>;

class RedditListingConnection : public RedditConnection<
        boost::beast::http::empty_body,
        boost::beast::http::string_body,
        ListingClientResponse>
{

public:
    RedditListingConnection(const boost::asio::any_io_executor& executor,
                  boost::asio::ssl::context& ssl_context,
                  const std::string& host, const std::string& service,
                  const std::string& userAgent );

    void list(const std::string& target, const access_token& token, std::any userData = std::any());
    template<typename S>
    void targetChangedHandler(S slot)
    {
        targetChangedSignal.connect(slot);
    }
protected:
    using ListingUserType =  typename ListingClientResponse::user_type;
    virtual void responseReceivedComplete(ListingUserType userData) override;
    virtual void handleLocationChange(const std::string& location) override;

private:
    std::string userAgent;
    boost::signals2::signal<void(std::string newTarget, ListingUserType userData)> targetChangedSignal;
};

#endif // REDDITLISTING_H
