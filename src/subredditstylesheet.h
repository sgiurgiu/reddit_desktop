#ifndef SUBREDDITSTYLESHEET_H
#define SUBREDDITSTYLESHEET_H

#include "redditclientproducer.h"
#include "entities.h"
#include "resizableglimage.h"
#include "utils.h"

#include <string>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <imgui.h>

class SubredditStylesheet: public std::enable_shared_from_this<SubredditStylesheet>
{
public:
    SubredditStylesheet(const access_token& token,
                        RedditClientProducer* client,
                        const boost::asio::any_io_executor& executor);

    void setAccessToken(const access_token& token)
    {
        this->token = token;
    }
    void LoadSubredditStylesheet(const std::string& target);
    void ShowHeader();
private:
    void parseStylesheet();
    void setErrorMessage(std::string errorMessage);
    void setSubredditStylesheet(listing listingResponse);
    void setBannerPicture(Utils::STBImagePtr data, int width, int height, int channels);
private:
    access_token token;
    RedditClientProducer* client;
    std::string target;
    const boost::asio::any_io_executor& uiExecutor;
    subreddit_stylesheet stylesheet;
    ResizableGLImagePtr headerPicture;
    ResizableGLImagePtr bannerPicture;
    ImVec4 headerColor;
    std::string errorMessage;
};

#endif // SUBREDDITSTYLESHEET_H
