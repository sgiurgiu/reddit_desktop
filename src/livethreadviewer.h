#ifndef LIVETHREADVIEWER_H
#define LIVETHREADVIEWER_H

#include <boost/asio/any_io_executor.hpp>
#include "redditclientproducer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include "entities.h"
#include "connections/redditlivethreadconnection.h"
#include "connections/twitterconnection.h"
#include "markdownrenderer.h"
#include "markdown/markdownnodelink.h"
#include "resizableglimage.h"
#include <boost/asio/steady_timer.hpp>
#include "twitterrenderer.h"

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
    struct EventEmbedDisplay
    {
        EventEmbedDisplay(live_update_event_embed embed,
                          RedditClientProducer* client,
                          const boost::asio::any_io_executor& uiExecutor);
        std::unique_ptr<MarkdownNodeLink> urlRenderer;
        live_update_event_embed embed;
        std::shared_ptr<TwitterRenderer> twitterRenderer;
        void Render();
    };

    struct EventDisplay
    {
        EventDisplay(live_update_event event,
                     RedditClientProducer* client,
                     const boost::asio::any_io_executor& uiExecutor):
            event(std::move(event)),
            bodyRenderer(this->event.body,client,uiExecutor)
        {
            updateHumanTime();
            for(const auto& em : this->event.embeds)
            {
                embedsDisplay.emplace_back(em,client,uiExecutor);
            }
        }

        std::string humanTime;
        std::vector<EventEmbedDisplay> embedsDisplay;
        live_update_event event;
        MarkdownRenderer bodyRenderer;
        void updateHumanTime();
    };

    std::unordered_map<std::string,std::shared_ptr<EventDisplay>> eventsMap;
    std::vector<std::shared_ptr<EventDisplay>> liveEvents;
    live_update_event_about eventAbout;
    RedditClientProducer::RedditLiveThreadClientConnection liveConnection;
    boost::asio::steady_timer refreshEventsTimeTimer;
    float timeTextWidth = 0.f;
    int64_t activityCount = 0;
    ResizableGLImagePtr strickenImage;

};

#endif // LIVETHREADVIEWER_H
