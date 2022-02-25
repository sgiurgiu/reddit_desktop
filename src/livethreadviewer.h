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
        }
        live_update_event event;
        MarkdownRenderer bodyRenderer;
    };

    std::unordered_map<std::string,std::shared_ptr<EventDisplay>> eventsMap;
    std::vector<std::shared_ptr<EventDisplay>> liveEvents;
    live_update_event_about eventAbout;
    RedditClientProducer::RedditLiveThreadClientConnection liveConnection;
};

#endif // LIVETHREADVIEWER_H
