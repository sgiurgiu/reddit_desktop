#ifndef REDDITDESKTOP_H
#define REDDITDESKTOP_H

#include <vector>
#include <memory>
#include <map>
#include <boost/asio/io_context.hpp>

#include "subredditwindow.h"
#include "loginwindow.h"
#include "redditclient.h"
#include "commentswindow.h"

class RedditDesktop : public std::enable_shared_from_this<RedditDesktop>
{
public:
    RedditDesktop(const boost::asio::io_context::executor_type& executor);

    void loginCurrentUser();
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
    void loadSubscribedSubreddits(subreddit_list srs);
    void loadSubreddits(const std::string& url, const access_token& token);
    void sortSubscribedSubreddits();
private:
    const boost::asio::io_context::executor_type& uiExecutor;
    RedditClient client;
    std::vector<std::shared_ptr<SubredditWindow>> subredditWindows;
    std::vector<std::shared_ptr<CommentsWindow>> commentsWindows;
    bool shouldQuit = false;
    bool openSubredditWindow = false;
    char selectedSubreddit[100] = { 0 };
    int windowsCount = 0;
    int appFrameHeight = 0;
    int appFrameWidth = 0;
    std::optional<user> current_user;
    user_info_ptr info_user;
    LoginWindow loginWindow;
    client_response<access_token> current_access_token;
    bool showConnectionErrorDialog = false;
    std::string connectionErrorMessage;
    subreddit_list subscribedSubreddits;
    subreddit_list unsortedSubscribedSubreddits;
    float topPosAfterMenuBar = 0.0f;
    enum class SubredditsSorting
    {
        None,
        Alphabetical_Ascending,
        Alphabetical_Descending
    };
    std::map<SubredditsSorting,std::string> subredditsSortMethod;
    SubredditsSorting currentSubredditsSorting = SubredditsSorting::None;
    bool shouldBlurPictures = true;
};


#endif // REDDITDESKTOP_H
