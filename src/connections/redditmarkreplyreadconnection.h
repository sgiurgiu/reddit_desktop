#ifndef REDDITMARKREPLYREADCONNECTION_H
#define REDDITMARKREPLYREADCONNECTION_H

#include "redditconnection.h"
#include "redditconnection.h"
#include "entities.h"

#include <vector>
#include <string>

class RedditMarkReplyReadConnection : public RedditPostConnection<client_response<std::string>>
{
public:
    RedditMarkReplyReadConnection(const boost::asio::any_io_executor& executor,
                                  boost::asio::ssl::context& ssl_context,
                                  const std::string& host, const std::string& service,
                                  const std::string& userAgent);

    void markReplyRead(const std::vector<std::string>& ids,
                       const access_token& token, bool read,
                       std::any userData = std::any());
protected:
    virtual void responseReceivedComplete(std::any userData) override;
private:
    std::string userAgent;
};

#endif // REDDITMARKREPLYREADCONNECTION_H
