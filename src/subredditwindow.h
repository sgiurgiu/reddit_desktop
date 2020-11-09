#ifndef SUBREDDITWINDOW_H
#define SUBREDDITWINDOW_H

#include <string>

class SubredditWindow
{
public:
    SubredditWindow(int id, const std::string& subreddit);
    bool isWindowOpen() {return true;}
    void showWindow(int appFrameWidth,int appFrameHeight);

private:
    void showWindowMenu();
private:
    int id;
    std::string subreddit;
    bool windowOpen = true;
};

#endif // SUBREDDITWINDOW_H
