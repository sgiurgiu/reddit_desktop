#ifndef REDDITLISTINGCONNECTION_H
#define REDDITLISTINGCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <memory>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditListingConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::empty_body>,
        boost::beast::http::response<boost::beast::http::string_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                  const client_response<listing>&)>
        >
{
public:
    RedditListingConnection(const boost::asio::any_io_executor& executor,
                  boost::asio::ssl::context& ssl_context,
                  const std::string& host, const std::string& service,
                  const std::string& userAgent );

    void list(const std::string& target, const access_token& token);
protected:
    virtual void responseReceivedComplete();
    virtual void sendRequest();
private:
    std::string userAgent;
    bool connected = false;
};

#endif // REDDITLISTING_H
