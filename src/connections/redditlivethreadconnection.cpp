#include "redditlivethreadconnection.h"
#include "uri.h"

RedditLiveThreadConnection::RedditLiveThreadConnection(const boost::asio::any_io_executor& executor,
                                                       boost::asio::ssl::context& ssl_context):
    RedditWebSocketConnection(executor,ssl_context)
{

}
void RedditLiveThreadConnection::getUpdates(const std::string& url)
{
    Uri urlParts(url);
    connect(urlParts.host(),
            urlParts.port().empty() ? "443" : urlParts.port(),
            urlParts.fullPath()+"?"+urlParts.query());

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
        else if(type == "embeds_ready")
        {
            std::vector<live_update_event_embed> embeds;
            for(const auto& embed:payload["mobile_embeds"])
            {
                embeds.emplace_back(embed);
            }
            embedsReadySignal(payload["liveupdate_id"].get<std::string>(),std::move(embeds));
        }
        else if(type == "activity")
        {
            int64_t count = payload["count"].get<int64_t>();
            countUpdateSignal(count);
        }
        else if(type == "settings")
        {
            settingsUpdateSignal(live_update_event_about(payload));
        }
        else if(type == "delete")
        {
            deleteSignal(payload.get<std::string>());
        }
        else if(type == "strike")
        {
            strikeSignal(payload.get<std::string>());
        }
        else if(type == "complete")
        {
            completeSignal();
        }
    }

}
