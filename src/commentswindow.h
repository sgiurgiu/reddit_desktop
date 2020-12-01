#ifndef COMMENTSWINDOW_H
#define COMMENTSWINDOW_H

#include <SDL_video.h>
#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"
#include "utils.h"

class CommentsWindow
{
public:
    CommentsWindow(const std::string& postId,
                   const std::string& title,
                   const access_token& token,
                   RedditClient* client,
                   const boost::asio::io_context::executor_type& executor);
    bool isWindowOpen() const {return windowOpen;}
    void showWindow(int appFrameWidth,int appFrameHeight);
    std::string getPostId() const
    {
        return postId;
    }
    void setFocused();
private:
    void loadListingsFromConnection(const listing& listingResponse);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(comment_ptr c);
    void setPostImage(unsigned char* data, int width, int height, int channels);
private:

    std::string postId;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    std::string windowName;
    RedditClient::RedditListingClientConnection connection;
    std::string listingErrorMessage;
    const boost::asio::io_context::executor_type& uiExecutor;
    bool willBeFocused = false;
    comments_list comments;
    post_ptr parent_post;
    SDL_DisplayMode displayMode;
    float post_picture_width = 0.f;
    float post_picture_height = 0.f;
    float post_picture_ratio = 0.f;
};

#endif // COMMENTSWINDOW_H
