#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
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
    using CommentsSignal = boost::signals2::signal<void(const std::string& id,const std::string& title)>;
    void showCommentsListener(const typename CommentsSignal::slot_type& slot);
    void setFocused();
    ~SubredditWindow();
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
private:    
    int id;
    std::string subreddit;
    std::string subredditName;
    bool windowOpen = true;
    access_token token;
    RedditClientProducer* client;
    std::string windowName;
    std::string listingErrorMessage;
    posts_list posts;
    std::string target;
    const boost::asio::any_io_executor& uiExecutor;
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
};

#endif // SUBREDDITWINDOW_H
