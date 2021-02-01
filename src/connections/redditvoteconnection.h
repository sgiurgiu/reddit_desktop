#ifndef REDDITVOTECONNECTION_H
#define REDDITVOTECONNECTION_H

#include "entities.h"
#include "redditconnection.h"
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditVoteConnection : public RedditConnection<
        boost::beast::http::string_body,
        boost::beast::http::string_body,
        client_response<std::string>
        >
{
public:
    RedditVoteConnection(const boost::asio::any_io_executor& executor,
                         boost::asio::ssl::context& ssl_context,
                         const std::string& host, const std::string& service,
                         const std::string& userAgent);
    void vote(const std::string& id,const access_token& token, Voted vote, std::any userData);
protected:
    virtual void responseReceivedComplete(std::any userData) override;
private:
    std::string userAgent;
    std::string id;
};

#endif // REDDITVOTECONNECTION_H
