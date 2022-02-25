#include "redditlivethreadconnection.h"
#include <boost/url.hpp>

RedditLiveThreadConnection::RedditLiveThreadConnection(const boost::asio::any_io_executor& executor,
                                                       boost::asio::ssl::context& ssl_context):
    RedditWebSocketConnection(executor,ssl_context)
{

}
void RedditLiveThreadConnection::getUpdates(const std::string& url)
{
    boost::url_view urlParts(url);
    connect(urlParts.encoded_host().to_string(),
            urlParts.port().empty() ? "443" : urlParts.port().to_string(),
            urlParts.encoded_path().to_string()+"?"+urlParts.encoded_query().to_string());

}
void RedditLiveThreadConnection::messageReceived()
{
    auto str = boost::beast::buffers_to_string(readBuffer.data());
    auto json = nlohmann::json::parse(str);
    if(json.contains("type") && json["type"].is_string())
    {
        auto type = json["type"].get<std::string>();
        auto payload = json["payload"];
        if(type=="update")
        {
            if(payload.contains("kind") &&
               payload["kind"].is_string() &&
               payload["kind"].get<std::string>() == "LiveUpdate")
            {
                live_update_event event(payload["data"]);
                signal({},event);
            }
        }
        if(type == "embeds_ready")
        {
            live_update_event event;
            event.name = payload["liveupdate_id"].get<std::string>();
            for(const auto& embed:payload["mobile_embeds"])
            {
                event.embeds.emplace_back(embed);
            }
            signal({},event);
        }
    }

}
