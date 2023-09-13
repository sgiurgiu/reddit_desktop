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
        NotSet,
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
    void subscribeToCurrentChunk();
private:
    std::string userAgent;
    int totalRequests = 0;
    int completedRequests = 0;
    std::vector<std::vector<std::string>> subredditsChunks;
    std::vector<std::vector<std::string>>::const_iterator currentPos;
    SubscriptionAction action = SubscriptionAction::NotSet;
    access_token token;
    client_response<bool> resp;
};

#endif // REDDITSRSUBSCRIPTIONCONNECTION_H
