#ifndef REDDITRESOURCECONNECTION_H
#define REDDITRESOURCECONNECTION_H

#include "redditconnection.h"

#include "entities.h"
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/signals2.hpp>

using response_byte = unsigned char;
using resource_response = client_response<std::vector<response_byte>>;
class RedditResourceConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::empty_body>,
        boost::beast::http::response<boost::beast::http::vector_body<response_byte>>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                  const resource_response&)>
        >
{
public:
    RedditResourceConnection(boost::asio::io_context& context,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& url,
                             const std::string& userAgent);
    void getResource(const access_token& token);
protected:
    virtual void responseReceivedComplete();
private:
    void onShutdown(const boost::system::error_code&);

private:
    std::string target;
    std::string userAgent;
};

#endif // REDDITRESOURCECONNECTION_H
