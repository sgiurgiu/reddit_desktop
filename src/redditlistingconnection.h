#ifndef REDDITLISTINGCONNECTION_H
#define REDDITLISTINGCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <memory>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditListingConnection : public RedditConnection,
        public std::enable_shared_from_this<RedditListingConnection>
{
public:
    RedditListingConnection(boost::asio::io_context& context,
                  boost::asio::ssl::context& ssl_context,
                  const std::string& host, const std::string& service,
                  const std::string& userAgent );

    using ListingSignal = boost::signals2::signal<void(const boost::system::error_code&,
                          const client_response<listing>&)>;
    using ListingSlot = ListingSignal::slot_type;
    void listingComplete(const ListingSlot& slot);
    void list(const std::string& target, const access_token& token);

private:
    void resolveComplete(const boost::system::error_code& ec,
                          boost::asio::ip::tcp::resolver::results_type results);
    void onConnect(const boost::system::error_code& ec,
                          boost::asio::ip::tcp::resolver::results_type::endpoint_type);
    void onHandshake(const boost::system::error_code& ec);
    void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred);
    void onRead(const boost::system::error_code& ec,std::size_t bytesTransferred);
private:
    boost::beast::http::request<boost::beast::http::empty_body> request;
    boost::beast::http::response<boost::beast::http::string_body> response;
    boost::beast::flat_buffer buffer; // (Must persist between reads)
    std::string host;
    std::string service;
    std::string userAgent;
    bool connected = false;
    ListingSignal signal;
};

#endif // REDDITLISTING_H
