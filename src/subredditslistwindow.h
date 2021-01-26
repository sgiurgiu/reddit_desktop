#ifndef SUBREDDITSLISTWINDOW_H
#define SUBREDDITSLISTWINDOW_H

#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include <imgui.h>
#include "entities.h"
#include "redditclientproducer.h"


class SubredditsListWindow : public std::enable_shared_from_this<SubredditsListWindow>
{
public:
    SubredditsListWindow(const access_token& token,
                         RedditClientProducer* client,
                         const boost::asio::any_io_executor& executor);
    void loadSubredditsList();
    void showWindow(int appFrameWidth,int appFrameHeight);
    template<typename S>
    void showSubredditListener(S slot)
    {
        subredditSignal.connect(slot);
    }
    void setAccessToken(const access_token& token)
    {
        this->token = token;
    }
private:
    void loadSubscribedSubreddits(subreddit_list srs);
    void loadSubreddits(const std::string& url, const access_token& token);
    void loadMultis(const std::string& url, const access_token& token);
    void setUserMultis(multireddit_list multis);
    void searchSubreddits();
    void setSearchResultsNames(names_list names);
    void setErrorMessage(std::string message);
    void showSubredditsTab();
    void showMultisTab();

private:
    using SubredditSignal = boost::signals2::signal<void(std::string)>;
    access_token token;
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    SubredditSignal subredditSignal;
    struct DisplayedSubredditItem
    {
        DisplayedSubredditItem(subreddit sr):
            sr(std::move(sr))
        {}
        subreddit sr;
        bool selected = false;
    };
    using DisplayedSubredditsList = std::vector<DisplayedSubredditItem>;
    DisplayedSubredditsList subscribedSubreddits;
    DisplayedSubredditsList unsortedSubscribedSubreddits;
    bool frontPageSubredditSelected = false;
    bool allSubredditSelected = false;
    multireddit_list userMultis;
    names_list searchedNamesList;

    enum class SubredditsSorting
    {
        None,
        Alphabetical_Ascending,
        Alphabetical_Descending
    };
    std::map<SubredditsSorting,std::string> subredditsSortMethod;
    SubredditsSorting currentSubredditsSorting = SubredditsSorting::None;
    RedditClientProducer::RedditSearchNamesClientConnection searchNamesConnection;
    bool searchingSubreddits = false;
    std::string errorMessage;
};

#endif // SUBREDDITSLISTWINDOW_H
