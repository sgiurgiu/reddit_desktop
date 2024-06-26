#ifndef TWITTERRENDERER_H
#define TWITTERRENDERER_H

#include <string>
#include <memory>
#include <vector>
#include "redditclientproducer.h"
#include "entities.h"
#include "resizableglimage.h"
#include "utils.h"

#include "RDRect.h"
#include "markdownrenderer.h"
#include "markdown/markdownnodelink.h"

class TwitterRenderer : public std::enable_shared_from_this<TwitterRenderer>
{
public:
    TwitterRenderer(RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor,
                    const std::string& twitterBearerToken);
    void Render() const;
    void LoadTweet(const std::string& url);
    void SetThumbnail(const std::string& url);
private:
    void setErrorMessage(std::string errorMessage);
    void setTwitterResponse(tweet response);
    void addTweetImage(size_t index,Utils::STBImagePtr data, int width, int height, int channels);
    void makeCreatedAtString();
    void extractTweetAuthor(const std::vector<tweet_user>& users);
    void splitTweetTextIntoEntities();
    void loadTweetImages();
    void loadReferencedTweets();
private:
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    std::string twitterBearerToken;
    RedditClientProducer::TwitterClientConnection twitterConnection;
    std::string errorMessage;
    bool loadingTweetContent = false;
    tweet theTweet;
    std::string thumbnailUrl;
    std::optional<tweet_user> author;
    std::vector<ResizableGLImagePtr> images;
    RDRect windowSize;
    std::string createdAtLocal;
    MarkdownRenderer tweetTextRenderer;
    std::vector<TwitterRenderer> referencedTweets;
    std::unique_ptr<MarkdownNodeLink> fullThreadUrlRenderer;
};

#endif // TWITTERRENDERER_H
