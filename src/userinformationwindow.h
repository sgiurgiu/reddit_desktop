#ifndef USERINFORMATIONWINDOW_H
#define USERINFORMATIONWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclient.h"

class UserInformationWindow
{
public:
    UserInformationWindow(const access_token& token,
                          RedditClient* client,
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
private:
    bool showWindow = false;
    access_token token;
    RedditClient* client;
    const boost::asio::any_io_executor& uiExecutor;
};

#endif // USERINFORMATIONWINDOW_H
