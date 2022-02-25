#ifndef REDDITLIVETHREADCONNECTION_H
#define REDDITLIVETHREADCONNECTION_H

#include "redditwebsocketconnection.h"
#include "entities.h"

class RedditLiveThreadConnection : public RedditWebSocketConnection<live_update_event>
{
public:
    RedditLiveThreadConnection(const boost::asio::any_io_executor& executor,
                               boost::asio::ssl::context& ssl_context);
    void getUpdates(const std::string& url);
protected:
    virtual void messageReceived() override;
};

#endif // REDDITLIVETHREADCONNECTION_H
