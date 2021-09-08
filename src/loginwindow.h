#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include "redditclientproducer.h"
#include <boost/asio/io_context.hpp>
#include "markdown/markdownnodeblockparagraph.h"

class LoginWindow
{
public:
    LoginWindow(RedditClientProducer* client, const boost::asio::any_io_executor& executor);
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
    void testingCompleted(const boost::system::error_code ec,client_response<access_token> token);
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
    RedditClientProducer* client;
    std::string testingErrorMessage;
    user configuredUser;
    client_response<access_token> token;
    RedditClientProducer::RedditLoginClientConnection loginConnection;
    const boost::asio::any_io_executor& uiExecutor;
    MarkdownNodeBlockParagraph helpParagraph;
    bool maskClientId = true;
    bool maskAppSecret = true;
};

#endif // LOGINWINDOW_H
