#ifndef REDDITCREATEPOSTCONNECTION_H
#define REDDITCREATEPOSTCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>
#include <optional>

class RedditCreatePostConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::string_body>,
        boost::beast::http::response<boost::beast::http::string_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                     const client_response<post_ptr>&)>
    >
{
public:
    RedditCreatePostConnection(boost::asio::io_context& context,
                               boost::asio::ssl::context& ssl_context,
                               const std::string& host, const std::string& service,
                               const std::string& userAgent);
    void createPost(const post_ptr& p,bool sendReplies,
                    const access_token& token);
protected:
    virtual void responseReceivedComplete();
private:
    std::string userAgent;
    post_ptr p;
};

#endif // REDDITCREATEPOSTCONNECTION_H
