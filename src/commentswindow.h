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
    void setAccessToken(const access_token& token)
    {
        this->token = token;
    }
    std::string getTitle() const
    {
        return title;
    }
    void setWindowPositionAndSize(ImVec2 pos,ImVec2 size)
    {
        windowPositionAndSizeSet = true;
        windowPos = pos;
        windowSize = size;
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
        std::string replyIdText;
        std::string saveReplyButtonText;
        std::string titleText;
        std::string postReplyPreviewCheckboxId;
        std::string liveReplyPreviewText;
        std::vector<DisplayComment> replies;
        bool loadingUnloadedReplies = false;
        std::string postReplyTextBuffer;
        bool postingReply = false;
        bool showingReplyArea = false;
        bool showingPreview = false;
        MarkdownRenderer previewRenderer;
        ImVec2 postReplyTextFieldSize;
        ImVec2 postReplyPreviewSize = {0,1};

        void updateButtonsText();
    };
    void loadListingsFromConnection(const listing& listingResponse,
                                    std::shared_ptr<CommentsWindow> self);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children,
                             std::shared_ptr<CommentsWindow> self,
                             bool append);
    void setComments(comments_list receivedComments, bool append);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(DisplayComment& c);
    void voteParentPost(post_ptr p, Voted vote);
    void updatePostVote(post* p, Voted vote);
    void voteComment(DisplayComment* c,Voted vote);
    void updateCommentVote(DisplayComment* c,Voted vote);
    void setupListingConnections();
    void setUnloadedComments(std::optional<unloaded_children> children);
    void loadUnloadedChildren(const std::optional<unloaded_children>& children);
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
    RedditClientProducer::RedditMoreChildrenClientConnection moreChildrenConnection;
    RedditClientProducer::RedditCreateCommentClientConnection createCommentConnection;

    using OpenSubredditSignal = boost::signals2::signal<void(std::string subreddit)>;
    OpenSubredditSignal openSubredditSignal;
    std::string postUpvoteButtonText;
    std::string postDownvoteButtonText;
    std::string openLinkButtonText;
    std::string commentButtonText;
    std::string postCommentTextFieldId;
    std::string moreRepliesButtonText;
    std::string loadingSpinnerIdText;
    std::string postCommentPreviewCheckboxId;
    std::vector<DisplayComment*> loadingMoreRepliesComments;
    std::optional<unloaded_children> unloadedPostComments;
    bool loadingUnloadedReplies = false;
    std::string postCommentTextBuffer;
    bool postingComment = false;
    bool showingPostPreview = false;
    bool loadingInitialComments = true;
    MarkdownRenderer postPreviewRenderer;
    ImVec2 postCommentTextFieldSize;
    ImVec2 postCommentPreviewSize = {0,1};
    bool windowPositionAndSizeSet = false;
    ImVec2 windowPos;
    ImVec2 windowSize;
};

using CommentsWindowPtr = std::shared_ptr<CommentsWindow>;

#endif // COMMENTSWINDOW_H
