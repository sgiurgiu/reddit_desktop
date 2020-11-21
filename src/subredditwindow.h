#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <mutex>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"
#include "utils.h"


class SubredditWindow
{
public:
    SubredditWindow(int id, const std::string& subreddit,
                    const access_token& token,
                    RedditClient* client,
                    const boost::asio::io_context::executor_type& executor);
    bool isWindowOpen() {return true;}
    void showWindow(int appFrameWidth,int appFrameHeight);

private:
    struct image_target
    {
        std::string url;
        int width;
        int height;
    };
    struct images_preview
    {
        image_target source;
        std::vector<image_target> resolutions;
    };

    struct post
    {
        std::string title;
        std::string selfText;
        int ups = 0;
        int downs = 0;
        bool isVideo = false;
        bool isSelf = false;
        std::string thumbnail;
        uint64_t createdAt;
        std::string humanReadableTimeDifference;
        int commentsCount = 0;
        std::string subreddit;
        int score = 0;
        std::string humanScore;
        std::string url;
        std::vector<images_preview> previews;
        std::unique_ptr<Utils::gl_image> thumbnail_picture;
        std::string authorFullName;
        std::string author;
    };
    using posts_list = std::vector<std::shared_ptr<post>>;

    void showWindowMenu();
    void loadListingsFromConnection(const listing& listingResponse);
    void setListings(posts_list receivedPosts);
    void setErrorMessage(std::string errorMessage);
    void setPostThumbnail(post* p,unsigned char* data, int width, int height, int channels);
private:    
    int id;
    std::string subreddit;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    std::string windowName;
    RedditClient::RedditListingClientConnection connection;
    std::string listingErrorMessage;
    posts_list posts;
    std::string target;
    const boost::asio::io_context::executor_type& uiExecutor;
};

#endif // SUBREDDITWINDOW_H
