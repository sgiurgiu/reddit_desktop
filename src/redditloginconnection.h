#ifndef REDDITLOGINCONNECTION_H
#define REDDITLOGINCONNECTION_H

#include "redditconnection.h"
#include "entities.h"
#include <memory>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditLoginConnection : public RedditConnection,
        public std::enable_shared_from_this<RedditLoginConnection>
{
public:
    RedditLoginConnection(boost::asio::io_context& context,
                          boost::asio::ssl::context& ssl_context,const std::string& host,
                          const std::string& service);
    using LoginSignal = boost::signals2::signal<void(const boost::system::error_code&, const client_response<access_token>&)>;
    using LoginSlot = LoginSignal::slot_type;
    void connectLoginComplete(const LoginSlot& slot);
    void login(const user& user);
private:
    void resolveComplete(const boost::system::error_code& ec,
                          boost::asio::ip::tcp::resolver::results_type results);
    void onConnect(const boost::system::error_code& ec,
                          boost::asio::ip::tcp::resolver::results_type::endpoint_type);
    void onHandshake(const boost::system::error_code& ec);
    void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred);
    void onRead(const boost::system::error_code& ec,std::size_t bytesTransferred);
    void onShutdown(const boost::system::error_code&);
private:
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::http::response<boost::beast::http::string_body> response;
    boost::beast::flat_buffer buffer; // (Must persist between reads)
    LoginSignal signal;
    std::string host;
    std::string service;

};

#endif // REDDITLOGINCONNECTION_H
