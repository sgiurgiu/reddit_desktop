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
    void showSubredditsNodes();
    void showMultisNodes();
    void clearSelections();
    void showSubredditsTab();
    void showSearchTab();
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
    struct DisplayedMultiItem
    {
        DisplayedMultiItem(multireddit sr):
            sr(std::move(sr))
        {}
        multireddit sr;
        bool selected = false;
    };
    struct DisplayedSearchResultItem
    {
        DisplayedSearchResultItem(std::string sr):
            sr(std::move(sr))
        {}
        std::string sr;
        bool selected = false;
    };

    using DisplayedSubredditsList = std::vector<DisplayedSubredditItem>;
    using DisplayedMultisList = std::vector<DisplayedMultiItem>;
    DisplayedSubredditsList subscribedSubreddits;
    DisplayedSubredditsList filteredSubscribedSubreddits;
    DisplayedMultisList userMultis;
    DisplayedMultisList filteredUserMultis;
    std::vector<DisplayedSearchResultItem> searchedNamesList;
    std::string subredditsFilter;
    RedditClientProducer::RedditListingClientConnection loadSubscribedSubredditsConnection;
    RedditClientProducer::RedditListingClientConnection loadMultisConnection;
    RedditClientProducer::RedditSearchNamesClientConnection searchNamesConnection;
    bool searchingSubreddits = false;
    std::string search;
    std::string errorMessage;
};

#endif // SUBREDDITSLISTWINDOW_H
