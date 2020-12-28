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
#include "markdownrenderer.h"

class CommentsWindow : public std::enable_shared_from_this<CommentsWindow>
{
public:
    CommentsWindow(const std::string& postId,
                   const std::string& title,
                   const access_token& token,
                   RedditClient* client,
                   const boost::asio::io_context::executor_type& executor);
    ~CommentsWindow();
    void loadComments();
    bool isWindowOpen() const {return windowOpen;}
    void showWindow(int appFrameWidth,int appFrameHeight);
    std::string getPostId() const
    {
        return postId;
    }
    void setFocused();
private:
    struct DisplayComment;
    using DisplayCommentPtr = std::unique_ptr<DisplayComment>;
    using DisplayCommentList = std::vector<DisplayCommentPtr>;
    struct DisplayComment
    {
        DisplayComment(comment_ptr comment):
        comment(std::move(comment)),renderer(this->comment->body)
        {
            for(auto&& c : this->comment->replies)
            {
                replies.emplace_back(std::make_unique<DisplayComment>(std::move(c)));
            }
        }
        comment_ptr comment;
        MarkdownRenderer renderer;
        DisplayCommentList replies;
    };
    void loadListingsFromConnection(const listing& listingResponse);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(DisplayComment* c);
private:
    std::string postId;
    std::string title;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    std::string windowName;
    std::string listingErrorMessage;
    bool willBeFocused = false;
    DisplayCommentList comments;
    post_ptr parentPost;
    const boost::asio::io_context::executor_type& uiExecutor;
    std::shared_ptr<PostContentViewer> postContentViewer;
};

#endif // COMMENTSWINDOW_H
