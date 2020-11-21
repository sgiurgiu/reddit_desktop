#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <boost/asio/io_context.hpp>

#include "subredditwindow.h"
#include "database.h"
#include "loginwindow.h"
#include "redditclient.h"

class RedditDesktop
{
public:
    RedditDesktop(const boost::asio::io_context::executor_type& executor);

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
    void showMainMenuBar();
    void showMenuFile();
    void showOpenSubredditWindow();
    void showErrorDialog();
    void setConnectionErrorMessage(std::string msg,client_response<access_token> token);
    void addSubredditWindow(std::string title, client_response<access_token> token);
private:
    const boost::asio::io_context::executor_type& uiExecutor;
    RedditClient client;
    RedditClient::RedditLoginClientConnection loginConnection;
    std::vector<std::unique_ptr<SubredditWindow>> subredditWindows;
    bool shouldQuit = false;
    bool openSubredditWindow = false;
    char selectedSubreddit[100] = { 0 };
    int windowsCount = 0;
    int appFrameHeight = 0;
    int appFrameWidth = 0;
    Database db;
    std::optional<user> current_user;
    LoginWindow loginWindow;
    client_response<access_token> current_access_token;
    bool showConnectionErrorDialog = false;
    std::string connectionErrorMessage;
};


#endif // REDDITDESKTOP_H
