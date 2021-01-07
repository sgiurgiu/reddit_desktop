#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <map>
#include <boost/asio/io_context.hpp>

#include "subredditwindow.h"
#include "loginwindow.h"
#include "redditclientproducer.h"
#include "commentswindow.h"
#include "userinformationwindow.h"

class RedditDesktop : public std::enable_shared_from_this<RedditDesktop>
{
public:
    RedditDesktop(boost::asio::any_io_executor uiExecutor);
    ~RedditDesktop();
    void loginCurrentUser();
    void showDesktop();
    void closeWindow();
    bool quitSelected()
    {
        return shouldQuit;
    }
    void setAppFrameWidth(int width)
    {
        appFrameWidth = width;
    }
    void setAppFrameHeight(int height)
    {
        appFrameHeight = height;
    }
private:
    void showSubredditsWindow();
    void showSubredditsTab();
    void showMultisTab();
    void showMainMenuBar();
    void showMenuFile();
    void showOpenSubredditWindow();
    void showErrorDialog();
    void setConnectionErrorMessage(std::string msg);
    void addSubredditWindow(std::string title);
    void loginSuccessful(client_response<access_token> token);
    void loadUserInformation(user_info info);
    void loadSubscribedSubreddits(subreddit_list srs);
    void loadSubreddits(const std::string& url, const access_token& token);
    void sortSubscribedSubreddits();
    void loadMultis(const std::string& url, const access_token& token);
    void setUserMultis(multireddit_list multis);
    void searchSubreddits();
    void setSearchResultsNames(names_list names);
    void addCommentsWindow(std::string postId,std::string title);
private:
    boost::asio::any_io_executor uiExecutor;
    RedditClientProducer client;
    std::vector<SubredditWindowPtr> subredditWindows;
    std::vector<CommentsWindowPtr> commentsWindows;
    bool shouldQuit = false;
    bool openSubredditWindow = false;
    char selectedSubreddit[100] = { 0 };
    int windowsCount = 0;
    int appFrameHeight = 0;
    int appFrameWidth = 0;
    std::optional<user> current_user;
    std::optional<user_info> info_user;
    LoginWindow loginWindow;
    client_response<access_token> current_access_token;
    bool showConnectionErrorDialog = false;
    std::string connectionErrorMessage;
    struct DisplayedSubredditItem
    {
        DisplayedSubredditItem(subreddit sr):
            sr(std::move(sr))
        {}
        subreddit sr;
        bool selected = false;
    };
    using DisplayedSubredditsList = std::vector<DisplayedSubredditItem>;
    DisplayedSubredditsList subscribedSubreddits;
    DisplayedSubredditsList unsortedSubscribedSubreddits;
    bool frontPageSubredditSelected = false;
    bool allSubredditSelected = false;
    multireddit_list userMultis;
    names_list searchedNamesList;
    float topPosAfterMenuBar = 0.0f;
    enum class SubredditsSorting
    {
        None,
        Alphabetical_Ascending,
        Alphabetical_Descending
    };
    std::map<SubredditsSorting,std::string> subredditsSortMethod;
    SubredditsSorting currentSubredditsSorting = SubredditsSorting::None;
    bool shouldBlurPictures = true;
    RedditClientProducer::RedditSearchNamesClientConnection searchNamesConnection;
    bool searchingSubreddits = false;
    std::shared_ptr<UserInformationWindow> userInfoWindow;
};


#endif // REDDITDESKTOP_H
