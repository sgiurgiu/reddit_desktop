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
    template<typename S>
    void addOpenSubredditHandler(S slot)
    {
        openSubredditSignal.connect(slot);
    }

private:
    struct DisplayComment
    {
        DisplayComment(comment cmt):
        commentData(std::move(cmt)),renderer(this->commentData.body)
        {
            for(auto&& c : this->commentData.replies)
            {
                replies.emplace_back(c);
            }
            updateButtonsText();
        }
        comment commentData;
        MarkdownRenderer renderer;
        std::string upvoteButtonText;
        std::string downvoteButtonText;
        std::string saveButtonText;
        std::string replyButtonText;
        std::string moreRepliesButtonText;
        std::string spinnerIdText;
        std::string titleText;
        std::vector<DisplayComment> replies;
        bool loadingUnloadedReplies = false;
        void updateButtonsText();
    };
    void loadListingsFromConnection(const listing& listingResponse,
                                    std::shared_ptr<CommentsWindow> self);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children,
                             std::shared_ptr<CommentsWindow> self);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(DisplayComment& c);
    void voteParentPost(post_ptr p, Voted vote);
    void updatePostVote(post* p, Voted vote);
    void voteComment(DisplayComment* c,Voted vote);
    void updateCommentVote(DisplayComment* c,Voted vote);
    void setupListingConnection();
    void setUnloadedComments(std::optional<unloaded_children> children);
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
    RedditClientProducer::RedditListingClientConnection listingConnection;
    RedditClientProducer::RedditVoteClientConnection postVotingConnection;
    RedditClientProducer::RedditVoteClientConnection commentVotingConnection;
    using OpenSubredditSignal = boost::signals2::signal<void(std::string subreddit)>;
    OpenSubredditSignal openSubredditSignal;
    std::string postUpvoteButtonText;
    std::string postDownvoteButtonText;
    std::string openLinkButtonText;
    std::string commentButtonText;
    std::string moreRepliesButtonText;
    std::string loadingSpinnerIdText;
    std::vector<DisplayComment*> loadingMoreRepliesComments;
    std::optional<unloaded_children> unloadedPostComments;
    bool loadingUnloadedReplies = false;
};

using CommentsWindowPtr = std::shared_ptr<CommentsWindow>;

#endif // COMMENTSWINDOW_H
