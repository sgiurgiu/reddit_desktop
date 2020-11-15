#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <string>
#include <memory>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"

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
    struct post
    {
        std::string title;
        std::string selfText;
        int ups = 0;
        int downs = 0;
        bool isVideo = false;

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
