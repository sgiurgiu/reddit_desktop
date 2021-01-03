#ifndef USERINFORMATIONWINDOW_H
#define USERINFORMATIONWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclientproducer.h"
#include "markdownrenderer.h"

class UserInformationWindow : public std::enable_shared_from_this<UserInformationWindow>
{
public:
    UserInformationWindow(const access_token& token,
                          RedditClientProducer* client,
                          const boost::asio::any_io_executor& executor);
    void showUserInfoWindow(int appFrameWidth,int appFrameHeight);
    void shouldShowWindow(bool flag)
    {
        showWindow = flag;
    }
    bool isWindowShowing() const
    {
        return showWindow;
    }
    void loadMessages();
private:
    struct DisplayMessage
    {
        DisplayMessage(message message):
            msg(std::move(message)),renderer(this->msg.body)
        {
        }
        message msg;
        MarkdownRenderer renderer;
    };
    using DisplayMessageList = std::vector<DisplayMessage>;
    struct Messages
    {
        std::string after;
        std::string before;
        int count = 0;
        DisplayMessageList messages;
    };
    void loadMoreUnreadMessages();
    void loadMoreAllMessages();
    void loadMoreSentMessages();
    void loadMessages(const std::string& kind,Messages* messages);
    void setErrorMessage(std::string errorMessage);
    void loadListingsFromConnection(listing listingResponse,Messages* messages);
    void showMessage(const DisplayMessage& msg);
private:
    bool showWindow = false;
    access_token token;
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    Messages unreadMessages;
    Messages allMessages;
    Messages sentMessages;
    std::string listingErrorMessage;
};

#endif // USERINFORMATIONWINDOW_H
