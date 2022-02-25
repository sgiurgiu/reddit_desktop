#ifndef LIVETHREADVIEWER_H
#define LIVETHREADVIEWER_H

#include <boost/asio/any_io_executor.hpp>
#include "redditclientproducer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include "entities.h"
#include "connections/redditlivethreadconnection.h"
#include "markdownrenderer.h"
#include "markdown/markdownnodelink.h"

#include <boost/asio/steady_timer.hpp>

class LiveThreadViewer : public std::enable_shared_from_this<LiveThreadViewer>
{
public:
    LiveThreadViewer(RedditClientProducer* client,
                     const access_token& token,
                     const boost::asio::any_io_executor& uiExecutor);
    void loadContent(const std::string& url);
    void showLiveThread();
private:
    void setErrorMessage(std::string errorMessage);
    void loadLiveThreadContentsChildren(const nlohmann::json& children);
    void loadLiveThreadAbout(live_update_event_about liveEventAbout);
    void addLiveEvent(live_update_event event);
    void rearmRefreshEventsTimeTimer();
    void updateEventsHumanTimeAgo();
    void updateEventEmbeds(std::string id,std::vector<live_update_event_embed> embeds);
    void updateActivityCount(int64_t count);
    void deleteEvent(std::string id);
    void strikeEvent(std::string id);
private:
    RedditClientProducer* client;
    access_token token;
    const boost::asio::any_io_executor& uiExecutor;
    std::string errorMessage;
    bool loadingPostContent = false;
    struct EventDisplay
    {
        EventDisplay(live_update_event event):event(std::move(event)),
            bodyRenderer(this->event.body)
        {
            updateHumanTime();
        }

        std::string humanTime;
        live_update_event event;
        MarkdownRenderer bodyRenderer;
        std::unique_ptr<MarkdownNodeLink> urlRenderer;
        void updateHumanTime();
    };

    std::unordered_map<std::string,std::shared_ptr<EventDisplay>> eventsMap;
    std::vector<std::shared_ptr<EventDisplay>> liveEvents;
    live_update_event_about eventAbout;
    RedditClientProducer::RedditLiveThreadClientConnection liveConnection;
    boost::asio::steady_timer refreshEventsTimeTimer;
    float timeTextWidth = 0.f;
    int64_t activityCount = 0;
};

#endif // LIVETHREADVIEWER_H
