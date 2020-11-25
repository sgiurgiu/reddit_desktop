#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
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
    bool isWindowOpen() const {return windowOpen;}
    void showWindow(int appFrameWidth,int appFrameHeight);
    using CommentsSignal = boost::signals2::signal<void(const std::string& id)>;
    void showCommentsListener(const typename CommentsSignal::slot_type& slot);
    void setFocused();
private:

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
    float maxScoreWidth = 0.f;
    float upvotesButtonsIdent = 0.f;
    CommentsSignal commentsSignal;
    bool willBeFocused = false;
};

#endif // SUBREDDITWINDOW_H
