#ifndef COMMENTSWINDOW_H
#define COMMENTSWINDOW_H


#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"
#include "utils.h"
#include "postcontentviewer.h"


class CommentsWindow
{
public:
    CommentsWindow(const std::string& postId,
                   const std::string& title,
                   const access_token& token,
                   RedditClient* client,
                   const boost::asio::io_context::executor_type& executor);
    ~CommentsWindow();
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
private:

    std::string postId;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    RedditClient::RedditListingClientConnection connection;    
    std::string windowName;
    std::string listingErrorMessage;
    bool willBeFocused = false;
    comments_list comments;
    post_ptr parent_post;    
    const boost::asio::io_context::executor_type& uiExecutor;
    std::unique_ptr<PostContentViewer> postContentViewer;
};

#endif // COMMENTSWINDOW_H
