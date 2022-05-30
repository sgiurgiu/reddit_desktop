#ifndef COMMENTSWINDOW_H
#define COMMENTSWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <tuple>

#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclientproducer.h"
#include "utils.h"
#include "postcontentviewer.h"
#include "markdownrenderer.h"
#include "awardsrenderer.h"
#include "flairrenderer.h"

class CommentsWindow : public std::enable_shared_from_this<CommentsWindow>
{
public:
    CommentsWindow(const std::string& postId,
                   const std::string& title,
                   const access_token& token,
                   RedditClientProducer* client,
                   const boost::asio::any_io_executor& executor);
    CommentsWindow(const std::string& commentContext,
                   const access_token& token,
                   RedditClientProducer* client,
                   const boost::asio::any_io_executor& executor);

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
    enum class CommentState
    {
        NONE,
        REPLYING,
        QUOTING,
        UPDATING,
        DELETING,
        DELETECONFIRMED,
        DELETED
    };
    struct DisplayComment
    {
        DisplayComment(comment cmt, const access_token& token,
                       RedditClientProducer* client,
                       const boost::asio::any_io_executor& executor):
        commentData(std::move(cmt)),renderer(this->commentData.body),
          awardsRenderer(std::make_shared<AwardsRenderer>(commentData)),
          flairRenderer(std::make_shared<FlairRenderer>(commentData))
        {
            for(auto&& c : this->commentData.replies)
            {
                replies.emplace_back(c, token, client, executor);
            }
            updateButtonsText();
            awardsRenderer->LoadAwards(token,client,executor);
            flairRenderer->LoadFlair(token,client,executor);
            if(commentData.author == "[deleted]")
            {
                //there doesn't seem to be any other way to detect
                //that a comment has been deleted. It's ... weird.
                state = CommentState::DELETED;
            }
        }
        comment commentData;
        MarkdownRenderer renderer;
        std::string upvoteButtonText;
        std::string downvoteButtonText;
        std::string saveButtonText;
        std::string replyButtonText;
        std::string quoteButtonText;
        std::string editButtonText;
        std::string moreRepliesButtonText;
        std::string spinnerIdText;
        std::string replyIdText;
        std::string saveReplyButtonText;
        std::string cancelReplyButtonText;
        std::string titleText;
        std::string commentScoreText;
        std::string postReplyPreviewCheckboxId;
        std::string liveReplyPreviewText;
        std::string deleteButtonText;
        std::string deleteYesButtonText;
        std::string deleteNoButtonText;

        std::vector<DisplayComment> replies;
        bool loadingUnloadedReplies = false;
        std::string postReplyTextBuffer;
        bool postingReplyInProgress = false;
        bool showingReplyArea = false;
        bool showingPreview = false;
        bool commentVisible = false;

        MarkdownRenderer previewRenderer;
        ImVec2 postReplyTextFieldSize;
        ImVec2 postReplyPreviewSize = {0,1};
        ImVec2 authorNameTextSize;
        ImVec2 commentScoreTextSize;
        ImVec2 commentTimeDiffTextSize;
        ImVec2 commentCharCountTextSize;
        std::string commentCharCountText;
        float markdownHeight = 10.f;
        std::shared_ptr<AwardsRenderer> awardsRenderer;
        std::shared_ptr<FlairRenderer> flairRenderer;
        CommentState state = CommentState::NONE;
        ImVec2 commentPosition;
        void updateButtonsText();
        void findText(const std::string& text);
        void clearFind();
    };
    void loadMoreChildrenListing(const listing& listingResponse,std::any userData);
    void loadListingsFromConnection(const listing& listingResponse);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children);
    using comments_tuple = std::tuple<comments_list,std::optional<unloaded_children>>;
    comments_tuple getJsonComments(const nlohmann::json& children);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(DisplayComment& c, int level = 0);
    void renderCommentContents(DisplayComment& c, int level);
    void renderCommentActionButtons(DisplayComment& c);
    void voteParentPost(Voted vote);
    void updatePostVote(Voted vote);
    void voteComment(DisplayComment* c,Voted vote);
    void updateCommentVote(std::string commentName,Voted vote);
    void setupListingConnections();
    void setUnloadedComments(std::optional<unloaded_children> children);
    template<typename T>
    void loadUnloadedChildren(const std::optional<unloaded_children>& children, T userData);
    DisplayComment* getComment(std::string commentName);
    DisplayComment* getChildComment(DisplayComment& c,const std::string& commentName);
    void loadCommentReply(const listing& listingResponse,std::any userData);
    bool commentNode(DisplayComment& c);
    void findText(const std::string& text);
private:
    std::string postId;
    std::string title;
    std::string commentContext;
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
    RedditClientProducer::RedditCommentClientConnection commentConnection;
    RedditClientProducer::RedditVoteClientConnection commentVotingConnection;

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
    std::string clearCommentButtonText;
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
    ImVec2 postReplyCharCountTextSize;
    std::string postReplyCharCountText;
    std::string textToFind;
    bool findTextFocusedAlready = false;
    bool findingStopped = true;

    struct PostUserData {};
    struct CommentUserData
    {
        std::string commentName;
        CommentState state = CommentState::NONE;
    };
};

using CommentsWindowPtr = std::shared_ptr<CommentsWindow>;

#endif // COMMENTSWINDOW_H
