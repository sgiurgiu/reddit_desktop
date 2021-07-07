#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <map>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include "subredditwindow.h"
#include "loginwindow.h"
#include "redditclientproducer.h"
#include "commentswindow.h"
#include "userinformationwindow.h"
#include "subredditslistwindow.h"
#include "log/loggingwindow.h"

class RedditDesktop : public std::enable_shared_from_this<RedditDesktop>
{
public:
    RedditDesktop(boost::asio::io_context& uiContext);

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
    void showMainMenuBar();
    void showMenuFile();
    void showOpenSubredditWindow();
    void showErrorDialog();
    void setConnectionErrorMessage(std::string msg);
    void addSubredditWindow(std::string title);
    void loginSuccessful(client_response<access_token> token);
    void updateUserInformation(user_info info);
    void addCommentsWindow(std::string postId,std::string title);
    void refreshLoginToken();
    void loadUserInformation();
    void updateWindowsTokenData();
    void cleanupClosedWindows();
    void searchSubreddits();
    void setSearchResultsNames(names_list names);
    void addMessageContextWindow(std::string context);
    void arrangeWindowsGrid();
    void showMediaDomainsManagementDialog();
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
    std::optional<user> currentUser;
    std::vector<user> registeredUsers;
    std::optional<user_info> infoUser;
    LoginWindow loginWindow;
    client_response<access_token> currentAccessToken;
    bool showConnectionErrorDialog = false;
    std::string connectionErrorMessage;
    float topPosAfterMenuBar = 0.0f;
    bool shouldBlurPictures = true;
    RedditClientProducer::RedditSearchNamesClientConnection searchNamesConnection;
    bool searchingSubreddits = false;
    std::shared_ptr<UserInformationWindow> userInfoWindow;
    bool loggedInFirstTime = true;
    boost::asio::steady_timer loginTokenRefreshTimer;
    std::string userInfoDisplay;
    bool useMediaHwAccel = true;
    std::shared_ptr<SubredditsListWindow> subredditsListWindow;
    names_list searchedNamesList;
    int subredditsAutoRefreshTimeout;
    bool showRandomNSFW = false;
    bool automaticallyArangeWindowsInGrid = false;
    bool useYoutubeDl = false;
    std::shared_ptr<LoggingWindow> loggingWindow;
#ifdef REDDIT_DESKTOP_DEBUG
    bool showLoggingWindow = true;
#else
    bool showLoggingWindow = false;
#endif
    bool mediaDomainsManagementDialog = false;
    std::vector<std::string> mediaDomains;
    std::string newMediaDomain;
    std::string selectedMediaDomain;
};


#endif // REDDITDESKTOP_H
