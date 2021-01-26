#ifndef REDDITSEARCHNAMESCONNECTION_H
#define REDDITSEARCHNAMESCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <memory>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

using names_list = std::vector<std::string>;

class RedditSearchNamesConnection : public RedditConnection<
        boost::beast::http::empty_body,
        boost::beast::http::string_body,
        client_response<names_list>
        >
{
public:
    RedditSearchNamesConnection(const boost::asio::any_io_executor& executor,
                                boost::asio::ssl::context& ssl_context,
                                const std::string& host, const std::string& service,
                                const std::string& userAgent );
    void search(const std::string& query, const access_token& token);
protected:
    virtual void responseReceivedComplete() override;
private:
    std::string userAgent;
};

#endif // REDDITSEARCHNAMESCONNECTION_H
