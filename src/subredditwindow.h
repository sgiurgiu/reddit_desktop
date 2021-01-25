#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include <imgui.h>
#include "entities.h"
#include "redditclientproducer.h"
#include "redditlistingconnection.h"
#include "resizableglimage.h"
#include "postcontentviewer.h"

class SubredditWindow : public std::enable_shared_from_this<SubredditWindow>
{
public:
    SubredditWindow(int id, const std::string& subreddit,
                    const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& executor);
    void loadSubreddit();
    bool isWindowOpen() const {return windowOpen;}
    void showWindow(int appFrameWidth,int appFrameHeight);
    template<typename S>
    void showCommentsListener(S slot)
    {
        commentsSignal.connect(slot);
    }
    void setFocused();
    ~SubredditWindow();
    std::string getSubreddit() const
    {
        return subreddit;
    }
    std::string getTitle() const
    {
        return title;
    }
    std::string getTarget() const
    {
        return target;
    }
    void setAccessToken(const access_token& token)
    {
        this->token = token;
    }
    void setWindowPositionAndSize(ImVec2 pos,ImVec2 size)
    {
        windowPositionAndSizeSet = true;
        windowPos = pos;
        windowSize = size;
    }
private:
    struct PostDisplay
    {
        PostDisplay(post_ptr p):post(std::move(p))
        {}
        post_ptr post;
        ResizableGLImagePtr thumbnailPicture;
        ResizableGLImagePtr blurredThumbnailPicture;
        bool shouldShowUnblurredImage = false;
        std::shared_ptr<PostContentViewer> postContentViewer;
        bool showingContent = false;
        std::string upvoteButtonText;
        std::string downvoteButtonText;
        std::string showContentButtonText;
        std::string openLinkButtonText;
        std::string errorMessageText;
        std::string votesChildText;
        void updateShowContentText();
    };
    using posts_list = std::vector<PostDisplay>;

    void showWindowMenu();
    void loadSubredditListings(const std::string& target,const access_token& token);
    void loadListingsFromConnection(const listing& listingResponse);
    void setListings(posts_list receivedPosts, nlohmann::json beforeJson,nlohmann::json afterJson);
    void setErrorMessage(std::string errorMessage);
    void setPostThumbnail(PostDisplay* p,unsigned char* data, int width, int height, int channels);
    void showNewTextPostDialog();
    void showNewLinkPostDialog();
    void submitNewPost(const post_ptr& p);
    void clearExistingPostsData();
    void votePost(post_ptr p,Voted voted);
    void updatePostVote(post* p,Voted voted);
    void pauseAllPosts();
    void setPostErrorMessage(PostDisplay* post,std::string msg);
private:    
    using CommentsSignal = boost::signals2::signal<void(std::string id,std::string title)>;
    int id;
    std::string subreddit;
    std::string subredditName;
    bool windowOpen = true;
    access_token token;
    RedditClientProducer* client;
    std::string windowName;
    std::string title;
    std::string listingErrorMessage;
    posts_list posts;
    std::string target;
    const boost::asio::any_io_executor& uiExecutor;
    RedditClientProducer::RedditListingClientConnection listingConnection;
    RedditClientProducer::RedditResourceClientConnection resourceConnection;
    RedditClientProducer::RedditVoteClientConnection voteConnection;
    float maxScoreWidth = 0.f;
    float upvotesButtonsIdent = 0.f;
    CommentsSignal commentsSignal;
    bool willBeFocused = false;
    std::optional<std::string> after;
    std::optional<std::string> before;
    int currentCount = 0;
    bool scrollToTop = false;
    bool shouldBlurPictures = true;
    bool newTextPostDialog = false;
    bool showingTextPostDialog = false;
    char newTextPostTitle[300] = {0};
    char newTextPostContent[3000] = {0};
    char newLinkPost[1000] = {0};
    bool newLinkPostDialog = false;
    bool showingLinkPostDialog = false;
    bool windowPositionAndSizeSet = false;
    ImVec2 windowPos;
    ImVec2 windowSize;
};

using SubredditWindowPtr = std::shared_ptr<SubredditWindow>;

#endif // SUBREDDITWINDOW_H
