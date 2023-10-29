#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <chrono>
#include <boost/signals2.hpp>
#include <boost/asio/steady_timer.hpp>
#include <imgui.h>
#include "entities.h"
#include "redditclientproducer.h"
#include "resizableglimage.h"
#include "postcontentviewer.h"
#include "utils.h"
#include "subredditstylesheet.h"
#include "markdownrenderer.h"
#include "awardsrenderer.h"
#include "flairrenderer.h"

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
    template<typename S>
    void showSubredditListener(S slot)
    {
        subredditSignal.connect(slot);
    }
    template<typename S>
    void subscriptionChangedListener(S slot)
    {
        subscriptionChangedSignal.connect(slot);
    }

    void setFocused();
    ~SubredditWindow();
    std::string getSubreddit() const
    {
        return subredditTarget;
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
        this->subredditStylesheet->setAccessToken(token);
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
        PostDisplay(post_ptr p,const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& executor):
            post(std::move(p)),
            awardsRenderer(std::make_shared<AwardsRenderer>(post)),
            flairRenderer(std::make_shared<FlairRenderer>(post))
        {
            awardsRenderer->LoadAwards(token,client,executor);
            flairRenderer->LoadFlair(token,client,executor);
        }
        post_ptr post;
        std::optional<StandardRedditThumbnail> standardThumbnail;
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
        std::string subredditLinkText;
        std::chrono::steady_clock::time_point lastPostShowTime;
        std::shared_ptr<AwardsRenderer> awardsRenderer;
        std::shared_ptr<FlairRenderer> flairRenderer;
        void updateShowContentText();
    };
    using posts_list = std::vector<PostDisplay>;

    struct AboutDisplay
    {
        AboutDisplay(subreddit& about):
            description(about.description),
            submit(about.submitText)
        {}
        MarkdownRenderer description;
        MarkdownRenderer submit;
    };

    struct SortPosts
    {
        std::string label;
        std::string sort;
        bool selected = false;
    };

    void showWindowMenu();
    void setupConnections();
    void loadSubredditListings(const std::string& target,const access_token& token);
    void loadListingsFromConnection(listing listingResponse);
    void setListings(posts_list receivedPosts, nlohmann::json beforeJson,nlohmann::json afterJson);
    void setErrorMessage(std::string errorMessage);
    void setPostThumbnail(std::string postName,Utils::STBImagePtr data, int width, int height, int channels);
    void showNewTextPostDialog();
    void showNewLinkPostDialog();
    void submitNewPost(const post_ptr& p);
    void clearExistingPostsData();
    void votePost(post_ptr p,Voted voted);
    void updatePostVote(std::string postName,Voted voted);
    void pauseAllMedia();
    void setPostErrorMessage(std::string postName,std::string msg);
    void lookAndDestroyPostsContents();
    void refreshPosts();
    void rearmRefreshTimer();
    void changeSubreddit(std::string newSubreddit);
    void renderPostVotes(PostDisplay& p);
    float renderPostThumbnail(PostDisplay& p);
    void renderPostShowContentButton(PostDisplay& p);
    void renderPostCommentsButton(PostDisplay& p);
    void renderPostOpenLinkButton(PostDisplay& p);
    void loadAbout(listing aboutData);
    void showAboutWindow();
    void updateWindowsNames();
private:
    using CommentsSignal = boost::signals2::signal<void(std::string id,std::string title)>;
    using SubredditSignal = boost::signals2::signal<void(std::string)>;
    using SubscriptionChangedSignal = boost::signals2::signal<void(void)>;
    int id;
    std::optional<subreddit> subredditAbout;
    std::optional<AboutDisplay> subredditAboutDisplay;

    std::string subredditTarget;
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
    RedditClientProducer::RedditListingClientConnection aboutConnection;
    RedditClientProducer::RedditResourceClientConnection resourceConnection;
    RedditClientProducer::RedditVoteClientConnection voteConnection;
    RedditClientProducer::RedditSRSubscriptionClientConnection srSubscriptionConnection;
    float maxScoreWidth = 0.f;
    float upvotesButtonsIdent = 0.f;
    CommentsSignal commentsSignal;
    SubredditSignal subredditSignal;
    SubscriptionChangedSignal subscriptionChangedSignal;
    bool willBeFocused = false;
    std::optional<std::string> after;
    std::optional<std::string> before;
    int currentCount = 0;
    bool scrollToTop = false;
    bool shouldBlurPictures = true;
    bool newTextPostDialog = false;
    bool showingTextPostDialog = false;
    std::string newTextPostTitle;
    std::string newTextPostContent;
    std::string newLinkPost;
    bool newLinkPostDialog = false;
    bool showingLinkPostDialog = false;
    bool windowPositionAndSizeSet = false;
    ImVec2 windowPos;
    ImVec2 windowSize;
    boost::asio::steady_timer postsContentDestroyerTimer;
    boost::asio::steady_timer refreshTimer;
    bool autoRefreshEnabled = false;
    std::shared_ptr<SubredditStylesheet> subredditStylesheet;
    bool aboutSubredditWindowOpen = false;
    std::string aboutSubredditWindowName;
    std::vector<SortPosts> sortPosts = {
        {"Default","",true},
        {"Hot","/hot",false},
        {"New","/new",false},
        {"Rising","/rising",false},
        {"Controversial","/controversial",false},
        {"Top","/top",false},
    };
    std::string currentlySelectedSortPostLabel = "Default";
    float sortPostsLabelWidth = -1.f;
};

using SubredditWindowPtr = std::shared_ptr<SubredditWindow>;

#endif // SUBREDDITWINDOW_H
