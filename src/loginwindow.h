#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include "redditclient.h"
#include <boost/asio/io_context.hpp>

class LoginWindow
{
public:
    LoginWindow(RedditClient* client, const boost::asio::io_context::executor_type& executor);
    void setShowLoginWindow(bool flag);
    bool showLoginWindow();
    user getConfiguredUser() const
    {
        return configuredUser;
    }
    client_response<access_token> getAccessToken() const
    {
        return token;
    }
    void setUser(const user& user)
    {
        configuredUser = user;
    }
private:
    void testingCompleted(const boost::system::error_code ec,const client_response<access_token> token);
private:
    bool showLoginInfoWindow = false;
    char username[255] = { 0 };
    char password[255] = { 0 };
    char clientId[255] = { 0 };
    char secret[255] = { 0 };
    char website[255] = { 0 };
    char appName[255] = { 0 };
    bool tested = false;
    bool testingInProgress = false;
    RedditClient* client;
    std::string testingErrorMessage;
    user configuredUser;
    client_response<access_token> token;
    RedditClient::RedditLoginClientConnection loginConnection;
    const boost::asio::io_context::executor_type& uiExecutor;
};

#endif // LOGINWINDOW_H
