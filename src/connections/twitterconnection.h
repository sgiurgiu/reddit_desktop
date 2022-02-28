#ifndef TWITTERCONNECTION_H
#define TWITTERCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

using TwitterClientResponse = client_response<tweet>;

class TwitterConnection : public RedditGetConnection<TwitterClientResponse>
{
public:
    TwitterConnection(const boost::asio::any_io_executor& executor,
                      boost::asio::ssl::context& ssl_context,
                      const std::string& userAgent, const std::string& bearerToken);
    void GetTweet(const std::string& tweet,std::any userData = std::any());
    void GetTweetFromUrl(const std::string& url,std::any userData = std::any());
protected:
    using TwitterUserType = TwitterClientResponse::user_type;
    virtual void responseReceivedComplete(TwitterUserType userData);
private:
    std::string userAgent;
    std::string bearerToken;
};

#endif // TWITTERCONNECTION_H
