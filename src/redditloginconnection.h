#ifndef REDDITLOGINCONNECTION_H
#define REDDITLOGINCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditLoginConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::string_body>,
        boost::beast::http::response<boost::beast::http::string_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                     const client_response<access_token>&)>
        >
{
public:
    RedditLoginConnection(const boost::asio::any_io_executor& executor,
                          boost::asio::ssl::context& ssl_context,const std::string& host,
                          const std::string& service);

    void login(const user& user);
protected:
    virtual void responseReceivedComplete();
private:
    void onShutdown(const boost::system::error_code&);

};

#endif // REDDITLOGINCONNECTION_H
