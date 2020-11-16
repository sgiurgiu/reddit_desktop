#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <string>
#include <memory>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"
#include <glad/glad.h>

class SubredditWindow
{
public:
    SubredditWindow(int id, const std::string& subreddit,
                    const access_token& token,
                    RedditClient* client);
    bool isWindowOpen() {return true;}
    void showWindow(int appFrameWidth,int appFrameHeight);

private:
    void showWindowMenu();
    void loadListings(const listing& listingResponse);
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
        int commentsCount = 0;
        std::string subreddit;
        int score = 0;
        std::string url;
        std::vector<images_preview> previews;
        GLuint thumbnailTextureId = 0;
    };
    int id;
    std::string subreddit;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    std::string windowName;
    RedditClient::RedditListingClientConnection connection;
    std::string listingErrorMessage;
    std::vector<post> posts;
    std::string target;

};

#endif // SUBREDDITWINDOW_H
