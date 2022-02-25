#ifndef REDDITLIVETHREADCONNECTION_H
#define REDDITLIVETHREADCONNECTION_H

#include <boost/signals2.hpp>
#include "redditwebsocketconnection.h"
#include "entities.h"

class RedditLiveThreadConnection : public RedditWebSocketConnection<live_update_event>
{
public:
    RedditLiveThreadConnection(const boost::asio::any_io_executor& executor,
                               boost::asio::ssl::context& ssl_context);
    void getUpdates(const std::string& url);
    template<typename Slot>
    void onEmbedsReady(Slot slot)
    {
        embedsReadySignal.connect(std::move(slot));
    }
    template<typename Slot>
    void onCountUpdate(Slot slot)
    {
        countUpdateSignal.connect(std::move(slot));
    }
    template<typename Slot>
    void onSettingsUpdate(Slot slot)
    {
        settingsUpdateSignal.connect(std::move(slot));
    }
    template<typename Slot>
    void onDelete(Slot slot)
    {
        deleteSignal.connect(std::move(slot));
    }
    template<typename Slot>
    void onStrike(Slot slot)
    {
        strikeSignal.connect(std::move(slot));
    }
    template<typename Slot>
    void onComplete(Slot slot)
    {
        completeSignal.connect(std::move(slot));
    }

protected:
    virtual void messageReceived() override;
private:
    boost::signals2::signal<void(std::string,std::vector<live_update_event_embed>)> embedsReadySignal;
    boost::signals2::signal<void(int64_t)> countUpdateSignal;
    boost::signals2::signal<void(live_update_event_about)> settingsUpdateSignal;
    boost::signals2::signal<void(std::string)> deleteSignal;
    boost::signals2::signal<void(std::string)> strikeSignal;
    boost::signals2::signal<void()> completeSignal;
};

#endif // REDDITLIVETHREADCONNECTION_H
