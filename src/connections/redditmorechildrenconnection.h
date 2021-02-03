#ifndef REDDITMORECHILDRENCONNECTION_H
#define REDDITMORECHILDRENCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <memory>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditMoreChildrenConnection : public RedditPostConnection<client_response<listing>>
{
public:
    RedditMoreChildrenConnection(const boost::asio::any_io_executor& executor,
                                 boost::asio::ssl::context& ssl_context,
                                 const std::string& host, const std::string& service,
                                 const std::string& userAgent);
    void list(const unloaded_children& children, const std::string& linkId,
              const access_token& token,std::any userData = std::any());
protected:
    virtual void responseReceivedComplete(std::any userData) override;
private:
    std::string userAgent;

};

#endif // REDDITMORECHILDRENCONNECTION_H
