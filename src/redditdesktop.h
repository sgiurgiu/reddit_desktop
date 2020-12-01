#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <boost/asio/io_context.hpp>

#include "subredditwindow.h"
#include "database.h"
#include "loginwindow.h"
#include "redditclient.h"
#include "commentswindow.h"

class RedditDesktop
{
public:
    RedditDesktop(const boost::asio::io_context::executor_type& executor,
                  Database* const db);

    void showDesktop();
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
private:
    void showSubredditsWindow();
    void showMainMenuBar();
    void showMenuFile();
    void showOpenSubredditWindow();
    void showErrorDialog();
    void setConnectionErrorMessage(std::string msg);
    void addSubredditWindow(std::string title);
    void loginSuccessful(client_response<access_token> token);
    void loadUserInformation(user_info_ptr info);
    void loadSubscribedSubreddits(subreddit_list srs,
                                  RedditClient::RedditListingClientConnection connection);
private:
    const boost::asio::io_context::executor_type& uiExecutor;
    RedditClient client;
    std::vector<std::unique_ptr<SubredditWindow>> subredditWindows;
    std::vector<std::unique_ptr<CommentsWindow>> commentsWindows;
    bool shouldQuit = false;
    bool openSubredditWindow = false;
    char selectedSubreddit[100] = { 0 };
    int windowsCount = 0;
    int appFrameHeight = 0;
    int appFrameWidth = 0;
    Database* db;
    std::optional<user> current_user;
    user_info_ptr info_user;
    LoginWindow loginWindow;
    client_response<access_token> current_access_token;
    bool showConnectionErrorDialog = false;
    std::string connectionErrorMessage;
    subreddit_list subscribedSubreddits;
    float topPosAfterMenuBar = 0.0f;
};


#endif // REDDITDESKTOP_H
