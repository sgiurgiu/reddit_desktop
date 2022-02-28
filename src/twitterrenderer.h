#ifndef TWITTERRENDERER_H
#define TWITTERRENDERER_H

#include "redditclientproducer.h"
#include <string>
#include <memory>
#include "entities.h"

class TwitterRenderer : public std::enable_shared_from_this<TwitterRenderer>
{
public:
    TwitterRenderer(RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor,
                    const std::string& twitterBearerToken);
    void Render();
    void LoadTweet(const std::string& url);
    void SetThumbnail(const std::string& url);
private:
    void setErrorMessage(std::string errorMessage);
    void setTwitterResponse(tweet response);
private:
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    std::string twitterBearerToken;
    RedditClientProducer::TwitterClientConnection twitterConnection;
    std::string errorMessage;
    bool loadingTweetContent = false;
    tweet theTweet;
    std::string thumbnailUrl;
};

#endif // TWITTERRENDERER_H
