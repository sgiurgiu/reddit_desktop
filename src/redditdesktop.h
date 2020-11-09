#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>

#include "subredditwindow.h"
#include "database.h"
#include "loginwindow.h"

class RedditDesktop
{
public:
    RedditDesktop();

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
private:
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
};


#endif // REDDITDESKTOP_H
