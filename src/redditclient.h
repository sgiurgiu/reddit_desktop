#ifndef REDDITCLIENT_H
#define REDDITCLIENT_H

#include "entities.h"
#include "redditloginconnection.h"
#include "redditlistingconnection.h"
#include "redditresourceconnection.h"

#include <boost/asio/io_context.hpp>
#include <thread>
#include <boost/asio/ssl/context.hpp>
#include <string_view>
#include <string>
#include <vector>

class RedditClient
{
public:
    RedditClient(std::string_view authServer,std::string_view server, int clientThreadsCount);
    ~RedditClient();
    using RedditLoginClientConnection = std::shared_ptr<RedditLoginConnection>;
    using RedditListingClientConnection = std::shared_ptr<RedditListingConnection>;
    using RedditResourceClientConnection = std::shared_ptr<RedditResourceConnection>;
    RedditLoginClientConnection makeLoginClientConnection();
    RedditListingClientConnection makeListingClientConnection();
    RedditResourceClientConnection makeResourceClientConnection(const std::string& url);
    void setUserAgent(const std::string& userAgent);

private:
    static constexpr auto service = "https";
    std::string authServer;
    std::string server;
    boost::asio::io_context context;
    boost::asio::ssl::context ssl_context;
    std::vector<std::thread> clientThreads;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    std::string userAgent;
};

#endif // REDDITCLIENT_H
