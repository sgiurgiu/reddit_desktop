#include "twitterconnection.h"
#include <charconv>
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/http.hpp>
#include <fmt/format.h>
#include "json.hpp"
#include "uri.h"
#include "utils.h"

TwitterConnection::TwitterConnection(const boost::asio::any_io_executor& executor,
                                     boost::asio::ssl::context& ssl_context,
                                     const std::string& userAgent, const std::string& bearerToken):
    RedditConnection(executor,ssl_context,"api.twitter.com","https"),
    userAgent(userAgent),bearerToken(bearerToken)
{
}
void TwitterConnection::GetTweetFromUrl(const std::string& url,std::any userData)
{
    Uri urlParts(url);
    //the kind of url we expect is: https://twitter.com/<user>/status/<tweetid>
    const auto path = urlParts.path();
    if(path.size() > 2)
    {
        auto tweetId = path[2];
        GetTweet(std::move(tweetId),std::move(userData));
    }
    else
    {
        //invalid URL....
    }
}
void TwitterConnection::GetTweet(const std::string& tweet,std::any userData)
{
    auto target = fmt::format("/2/tweets/{}?expansions=author_id,in_reply_to_user_id,attachments.media_keys,"
                              "referenced_tweets.id,entities.mentions.username,referenced_tweets.id.author_id&"
                              "tweet.fields=attachments,author_id,created_at,id,"
                              "in_reply_to_user_id,lang,possibly_sensitive,public_metrics,"
                              "referenced_tweets,text,withheld,entities,conversation_id&media.fields=media_key,height,"
                              "type,url,width,preview_image_url,duration_ms&"
                              "user.fields=id,name,username,verified",tweet);
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "application/json");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",bearerToken));
    request.prepare_payload();

    enqueueRequest(std::move(request),std::move(userData));
}
void TwitterConnection::responseReceivedComplete(TwitterUserType userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    TwitterClientResponse resp;
    resp.status = status;
    if(status == 200)
    {
        try
        {
            resp.data = tweet{nlohmann::json::parse(body)};
        }
        catch(const std::exception& ex)
        {
            resp.body = ex.what();
            if(status < 500) resp.status = 500;
        }
    }
    else
    {
        resp.body = body;
    }
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}
