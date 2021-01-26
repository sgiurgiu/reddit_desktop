#ifndef REDDITCLIENTPRODUCER_H
#define REDDITCLIENTPRODUCER_H

#include "entities.h"
#include "redditloginconnection.h"
#include "redditlistingconnection.h"
#include "redditresourceconnection.h"
#include "mediastreamingconnection.h"
#include "redditcreatepostconnection.h"
#include "redditsearchnamesconnection.h"
#include "redditvoteconnection.h"
#include "redditmorechildrenconnection.h"
#include "redditcreatecommentconnection.h"
#include "redditmarkreplyreadconnection.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <thread>
#include <boost/asio/ssl/context.hpp>
#include <string_view>
#include <string>
#include <vector>

class RedditClientProducer
{
public:
    RedditClientProducer(std::string_view authServer,std::string_view server, int clientThreadsCount);
    ~RedditClientProducer();
    using RedditLoginClientConnection = std::shared_ptr<RedditLoginConnection>;
    using RedditListingClientConnection = std::shared_ptr<RedditListingConnection>;
    using RedditResourceClientConnection = std::shared_ptr<RedditResourceConnection>;
    using MediaStreamingClientConnection = std::shared_ptr<MediaStreamingConnection>;
    using RedditCreatePostClientConnection = std::shared_ptr<RedditCreatePostConnection>;
    using RedditSearchNamesClientConnection = std::shared_ptr<RedditSearchNamesConnection>;
    using RedditVoteClientConnection = std::shared_ptr<RedditVoteConnection>;
    using RedditMoreChildrenClientConnection = std::shared_ptr<RedditMoreChildrenConnection>;
    using RedditCreateCommentClientConnection = std::shared_ptr<RedditCreateCommentConnection>;
    using RedditMarkReplyReadClientConnection = std::shared_ptr<RedditMarkReplyReadConnection>;
    RedditLoginClientConnection makeLoginClientConnection();
    RedditListingClientConnection makeListingClientConnection();
    RedditResourceClientConnection makeResourceClientConnection();
    MediaStreamingClientConnection makeMediaStreamingClientConnection();
    RedditCreatePostClientConnection makeCreatePostClientConnection();
    RedditSearchNamesClientConnection makeRedditSearchNamesClientConnection();
    RedditVoteClientConnection makeRedditVoteClientConnection();
    RedditMoreChildrenClientConnection makeRedditMoreChildrenClientConnection();
    RedditCreateCommentClientConnection makeRedditCreateCommentClientConnection();
    RedditMarkReplyReadClientConnection makeRedditMarkReplyReadClientConnection();
    void setUserAgent(const std::string& userAgent);

private:
    static constexpr auto service = "https";
    std::string authServer;
    std::string server;
    boost::asio::ssl::context ssl_context;
    boost::asio::thread_pool clientThreads;
    std::string userAgent;
    boost::asio::any_io_executor executor;
};

#endif // REDDITCLIENT_H
