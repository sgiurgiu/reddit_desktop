#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <optional>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include "subredditwindow.h"
#include "loginwindow.h"
#include "redditclientproducer.h"
#include "commentswindow.h"
#include "userinformationwindow.h"
#include "subredditslistwindow.h"
#include "log/loggingwindow.h"
#include "aboutwindow.h"

class RedditDesktop : public std::enable_shared_from_this<RedditDesktop>
{
public:
    RedditDesktop(boost::asio::io_context& uiContext);

    void loginCurrentUser();
    void showDesktop();
    void closeWindow();
    bool quitSelected() const
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
    ImVec4 getBackgroundColor() const
    {
        return backgroundColor;
    }
    void saveBackgroundColor();
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
    void showTwitterAPIAuthBearerDialog();
    void showImportSubsDialog();
    void showImportNotStartedControls();
    void showImportingControls();
    void showImportFinishedControls();
    void startImportingSubs();
    void importingSubsError(std::string msg);
    void importingSubsLoginSuccess(access_token token, std::shared_ptr<RedditClientProducer> clientProducer);
    void loadImportingUserSubreddits(std::string url, access_token token,
                                     std::shared_ptr<RedditClientProducer> clientProducer,
                                     RedditClientProducer::RedditListingClientConnection connection = RedditClientProducer::RedditListingClientConnection());

    void collectImportingUserSubreddits(subreddit_list subs, access_token token,
                                     std::shared_ptr<RedditClientProducer> clientProducer,
                                     RedditClientProducer::RedditListingClientConnection connection);
    void subscribeToSubreddits();
    void showNetworkInformationDialog();

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
    AboutWindow aboutWindow;
    bool twitterAPIAuthBearerDialog = false;
    std::string twitterBearer;
    ImVec4 backgroundColor =  {0.45f, 0.55f, 0.60f, 1.00f};
    bool changeBackgroundColorDialog = false;
    bool importSubsDialog = false;
    enum class ImportingState
    {
        NotStarted,
        Importing,
        FinishedSuccessfully,
        FinishedWithError
    };
    struct ImportingUser
    {
        ImportingState importingState = ImportingState::NotStarted;
        std::string importingStatusMessage;
        user importUser;
        subreddit_list subredditsToImport;
    };

    std::optional<ImportingUser> importingUser;
    bool showNetworkInfomation = false;
    std::optional<IpInfo> ipInfo;
    std::string ipInfoRetreivalErrorMessage;
};


#endif // REDDITDESKTOP_H
