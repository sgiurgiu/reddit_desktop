#ifndef REDDITSRSUBSCRIPTIONCONNECTION_H
#define REDDITSRSUBSCRIPTIONCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

class RedditSRSubscriptionConnection :
        public RedditPostConnection<client_response<bool>>
{
public:
    enum class SubscriptionAction
    {
        Subscribe,
        Unsubscribe
    };
    RedditSRSubscriptionConnection(const boost::asio::any_io_executor& executor,
                                   boost::asio::ssl::context& ssl_context,
                                   const std::string& host, const std::string& service,
                                   const std::string& userAgent);
    void updateSrSubscription(const std::vector<std::string>& subreddits,
                              SubscriptionAction action,
                              const access_token& token);
protected:
    virtual void responseReceivedComplete() override;
private:
    std::string userAgent;
};

#endif // REDDITSRSUBSCRIPTIONCONNECTION_H
