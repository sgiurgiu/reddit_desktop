#ifndef REDDITCLIENTPRODUCER_H
#define REDDITCLIENTPRODUCER_H

#include "connections/redditloginconnection.h"
#include "connections/redditlistingconnection.h"
#include "connections/redditresourceconnection.h"
#include "connections/urldetectionconnection.h"
#include "connections/redditcreatepostconnection.h"
#include "connections/redditsearchnamesconnection.h"
#include "connections/redditvoteconnection.h"
#include "connections/redditmorechildrenconnection.h"
#include "connections/redditcommentconnection.h"
#include "connections/redditmarkreplyreadconnection.h"
#include "connections/redditsrsubscriptionconnection.h"
#include "connections/redditlivethreadconnection.h"
#include "connections/twitterconnection.h"
#include "connections/ipinfoconnection.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/ssl/context.hpp>
#include <string_view>
#include <string>

class RedditClientProducer
{
public:
    RedditClientProducer(std::string_view authServer,std::string_view server, int clientThreadsCount);
    ~RedditClientProducer();
    using RedditLoginClientConnection = std::shared_ptr<RedditLoginConnection>;
    using RedditListingClientConnection = std::shared_ptr<RedditListingConnection>;
    using RedditResourceClientConnection = std::shared_ptr<RedditResourceConnection>;
    using UrlDetectionClientConnection = std::shared_ptr<UrlDetectionConnection>;
    using RedditCreatePostClientConnection = std::shared_ptr<RedditCreatePostConnection>;
    using RedditSearchNamesClientConnection = std::shared_ptr<RedditSearchNamesConnection>;
    using RedditVoteClientConnection = std::shared_ptr<RedditVoteConnection>;
    using RedditMoreChildrenClientConnection = std::shared_ptr<RedditMoreChildrenConnection>;
    using RedditCommentClientConnection = std::shared_ptr<RedditCommentConnection>;
    using RedditMarkReplyReadClientConnection = std::shared_ptr<RedditMarkReplyReadConnection>;
    using RedditSRSubscriptionClientConnection = std::shared_ptr<RedditSRSubscriptionConnection>;
    using RedditLiveThreadClientConnection = std::shared_ptr<RedditLiveThreadConnection>;
    using TwitterClientConnection = std::shared_ptr<TwitterConnection>;
    using IpInfoClientConnection = std::shared_ptr<IPInfoConnection>;
    RedditLoginClientConnection makeLoginClientConnection();
    RedditListingClientConnection makeListingClientConnection();
    RedditResourceClientConnection makeResourceClientConnection();
    UrlDetectionClientConnection makeUrlDetectionClientConnection();
    RedditCreatePostClientConnection makeCreatePostClientConnection();
    RedditSearchNamesClientConnection makeRedditSearchNamesClientConnection();
    RedditVoteClientConnection makeRedditVoteClientConnection();
    RedditMoreChildrenClientConnection makeRedditMoreChildrenClientConnection();
    RedditCommentClientConnection makeRedditCommentClientConnection();
    RedditMarkReplyReadClientConnection makeRedditMarkReplyReadClientConnection();
    RedditSRSubscriptionClientConnection makeRedditRedditSRSubscriptionClientConnection();
    RedditLiveThreadClientConnection makeRedditLiveThreadClientConnection();
    TwitterClientConnection makeTwitterConnection(const std::string& twitterBearer);
    IpInfoClientConnection makeIpInfoClientConnection();
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
