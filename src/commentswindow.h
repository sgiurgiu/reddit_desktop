#ifndef COMMENTSWINDOW_H
#define COMMENTSWINDOW_H


#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclientproducer.h"
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
                   RedditClientProducer* client,
                   const boost::asio::any_io_executor& executor);
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
    //struct DisplayComment;
    //using DisplayCommentPtr = std::unique_ptr<DisplayComment>;
    //using DisplayCommentList = std::vector<DisplayComment>;
    struct DisplayComment
    {
        DisplayComment(comment cmt):
        commentData(std::move(cmt)),renderer(this->commentData.body)
        {
            for(auto&& c : this->commentData.replies)
            {
                replies.emplace_back(c);
            }
        }
        comment commentData;
        MarkdownRenderer renderer;
        std::vector<DisplayComment> replies;
    };
    void loadListingsFromConnection(const listing& listingResponse);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(const DisplayComment& c);
private:
    std::string postId;
    std::string title;
    bool windowOpen = true;
    access_token token;
    RedditClientProducer* client;
    std::string windowName;
    std::string listingErrorMessage;
    bool willBeFocused = false;
    std::vector<DisplayComment> comments;
    post_ptr parentPost;
    const boost::asio::any_io_executor& uiExecutor;
    std::shared_ptr<PostContentViewer> postContentViewer;
};

#endif // COMMENTSWINDOW_H
