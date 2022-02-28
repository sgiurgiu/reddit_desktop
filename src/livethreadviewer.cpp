#include "livethreadviewer.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include <boost/url.hpp>
#include "utils.h"
#include "markdown/markdownnodetext.h"
#include <cinttypes>
#include "database.h"

namespace
{
    constexpr int MAX_EVENTS_COUNT = 100;
}
LiveThreadViewer::LiveThreadViewer(RedditClientProducer* client,
                                   const access_token& token,
                                   const boost::asio::any_io_executor& uiExecutor):
    client(client),token(token),uiExecutor(uiExecutor),refreshEventsTimeTimer(uiExecutor)
{
    liveConnection = client->makeRedditLiveThreadClientConnection();
}
void LiveThreadViewer::showLiveThread()
{
    if(!errorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",errorMessage.c_str());
    }

    if(loadingPostContent)
    {
        ImGui::Spinner("###spinner_loading_data",50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
        return;
    }

    if(eventAbout.state == "live")
    {
        ImGui::TextUnformatted("Thread is live");
    }
    else
    {
        ImGui::TextUnformatted("Thread is no longer live");
    }
    ImGui::SameLine();
    ImGui::Text("%" PRId64 " viewers",activityCount);

    for(const auto& event:liveEvents)
    {
        ImGui::BeginGroup();
            ImGui::Text("%s",event->humanTime.c_str());
            if(timeTextWidth < 1.f)
            {
                //calculate it here so that we use the appropriate font
                timeTextWidth = ImGui::CalcTextSize("XXXXXXXXXXXXX").x;
            }
            ImGui::Dummy(ImVec2(timeTextWidth,0.f));
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
            if(event->embedsDisplay.empty())
            {
                event->bodyRenderer.RenderMarkdown();
            }
            else
            {
                for(auto& live:event->embedsDisplay)
                {
                    live.Render();
                }
            }
        ImGui::EndGroup();
        if(event->event.stricken)
        {
            const auto rectMax = ImGui::GetItemRectMax();
            const auto rectMin = ImGui::GetItemRectMin();
            const ImGuiWindow* window = ImGui::GetCurrentWindow();
            const ImVec2 uv0(0, 0);
            const ImVec2 uv1(1,1);
            const ImVec4 tint_col(1,1,1,0.35f);
            window->DrawList->AddImage((void*)(intptr_t)strickenImage->textureId,rectMin,rectMax,uv0,uv1,ImGui::GetColorU32(tint_col));
        }
        ImGui::Separator();
    }
}
void LiveThreadViewer::loadContent(const std::string& url)
{
    loadingPostContent = true;
    boost::url_view urlParts(url);
    {
        auto listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                     client_response<listing> response){
            auto self = weak.lock();
            if (!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::setErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.data.json.contains("kind") && response.data.json["kind"].is_string() &&
                    response.data.json["kind"].get<std::string>() == "Listing"  &&
                   response.data.json.contains("data") && response.data.json["data"].is_object() &&
                   response.data.json["data"].contains("children") && response.data.json["data"]["children"].is_array())
                {
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&LiveThreadViewer::loadLiveThreadContentsChildren,
                                                self,std::move(response.data.json["data"]["children"])));
                }
            }
        });
        listingConnection->list(urlParts.encoded_path().to_string(),token);
    }

    {
        auto listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                     client_response<listing> response){
            auto self = weak.lock();
            if (!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::setErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.data.json.contains("kind") && response.data.json["kind"].is_string() &&
                    response.data.json["kind"].get<std::string>() == "LiveUpdateEvent"  &&
                   response.data.json.contains("data") && response.data.json["data"].is_object())
                {

                    boost::asio::post(self->uiExecutor,
                                      std::bind(&LiveThreadViewer::loadLiveThreadAbout,
                                                self,live_update_event_about(response.data.json["data"])));
                }

            }
        });
        listingConnection->list(urlParts.encoded_path().to_string()+"/about.json",token);
    }
    liveConnection->onUpdate([weak=weak_from_this()](const boost::system::error_code& ec,
                             live_update_event event){
        auto self = weak.lock();
        if (!self) return;

        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::setErrorMessage,self,ec.message()));
        }
        else
        {
            boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::addLiveEvent,self,std::move(event)));
        }
    });
    liveConnection->onEmbedsReady([weak=weak_from_this()](std::string id,std::vector<live_update_event_embed> embeds){
        auto self = weak.lock();
        if (!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::updateEventEmbeds,self,std::move(id),std::move(embeds)));
    });
    liveConnection->onCountUpdate([weak=weak_from_this()](int64_t count){
        auto self = weak.lock();
        if (!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::updateActivityCount,self,count));
    });
    liveConnection->onSettingsUpdate([weak=weak_from_this()](live_update_event_about about){
        auto self = weak.lock();
        if (!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::loadLiveThreadAbout,self,std::move(about)));
    });
    liveConnection->onDelete([weak=weak_from_this()](std::string id){
        auto self = weak.lock();
        if (!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::deleteEvent,self,std::move(id)));
    });
    liveConnection->onStrike([weak=weak_from_this()](std::string id){
        auto self = weak.lock();
        if (!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LiveThreadViewer::strikeEvent,self,std::move(id)));
    });
    strickenImage = Utils::GetStrickenImage();
}
void LiveThreadViewer::updateActivityCount(int64_t count)
{
    this->activityCount = count;
}
void LiveThreadViewer::updateEventEmbeds(std::string id,std::vector<live_update_event_embed> embeds)
{
    if(eventsMap.contains(id))
    {
        eventsMap[id]->embedsDisplay.clear();
        for(const auto& em : embeds)
        {
            eventsMap[id]->embedsDisplay.emplace_back(em,client,uiExecutor);
        }
        eventsMap[id]->event.embeds = std::move(embeds);
    }
}
void LiveThreadViewer::deleteEvent(std::string id)
{
    if(eventsMap.contains(id))
    {
        eventsMap.erase(id);
        auto it = std::find_if(liveEvents.begin(),liveEvents.end(),[id](const auto& event){
            return event->event.name == id;
        });
        if(it != liveEvents.end())
        {
            liveEvents.erase(it);
        }
    }
}
void LiveThreadViewer::strikeEvent(std::string id)
{
    if(eventsMap.contains(id))
    {
        eventsMap[id]->event.stricken = true;
    }
}

void LiveThreadViewer::addLiveEvent(live_update_event event)
{
    deleteEvent(event.name);
    auto eventShared = std::make_shared<EventDisplay>(std::move(event),client,uiExecutor);
    eventsMap[eventShared->event.name] = eventShared;
    liveEvents.insert(liveEvents.begin(),eventShared);
    while(liveEvents.size() > MAX_EVENTS_COUNT)
    {
        eventsMap.erase(liveEvents.back()->event.name);
        liveEvents.pop_back();
    }
}
void LiveThreadViewer::loadLiveThreadAbout(live_update_event_about liveEventAbout)
{
    this->eventAbout = std::move(liveEventAbout);
    updateActivityCount(eventAbout.viewer_count);
    liveConnection->getUpdates(eventAbout.websocket_url);
}
void LiveThreadViewer::loadLiveThreadContentsChildren(const nlohmann::json& children)
{
    for(const auto& child : children)
    {
        if(child["kind"].get<std::string>() != "LiveUpdate")
        {
            continue;
        }
        auto eventShared = std::make_shared<EventDisplay>(child["data"],client,uiExecutor);
        eventsMap[eventShared->event.name] = eventShared;
        liveEvents.push_back(eventShared);
        loadingPostContent = false;
    }
    rearmRefreshEventsTimeTimer();
}
void LiveThreadViewer::rearmRefreshEventsTimeTimer()
{
    refreshEventsTimeTimer.expires_after(std::chrono::seconds(60));
    refreshEventsTimeTimer.async_wait([weak=weak_from_this()](const boost::system::error_code& ec){
        auto self = weak.lock();
        if(self && !ec)
        {
            self->updateEventsHumanTimeAgo();
        }
    });
}
void LiveThreadViewer::updateEventsHumanTimeAgo()
{
    for(const auto& event:liveEvents)
    {
        event->updateHumanTime();
    }
    rearmRefreshEventsTimeTimer();
}
void LiveThreadViewer::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = std::move(errorMessage);
}
void LiveThreadViewer::EventDisplay::updateHumanTime()
{
    humanTime = Utils::getHumanReadableTimeAgo(event.created_utc, true);
}
LiveThreadViewer::EventEmbedDisplay::EventEmbedDisplay(live_update_event_embed embed,
                                                       RedditClientProducer* client,
                                                       const boost::asio::any_io_executor& uiExecutor):
    embed(std::move(embed))
{
    if(!this->embed.url.empty())
    {
        urlRenderer = std::make_unique<MarkdownNodeLink>(this->embed.url,this->embed.url);
        urlRenderer->AddChild(std::make_unique<MarkdownNodeText>(this->embed.url.c_str(),this->embed.url.size()));
    }
    if(this->embed.provider_name == "Twitter")
    {
        auto twitterBearerOpt = Database::getInstance()->getTwitterAuthBearer();
        if(twitterBearerOpt)
        {
            twitterRenderer = std::make_shared<TwitterRenderer>(client,uiExecutor,twitterBearerOpt.value());
            twitterRenderer->LoadTweet(this->embed.url);
            twitterRenderer->SetThumbnail(this->embed.thumbnail_url);
        }
    }
}
void LiveThreadViewer::EventEmbedDisplay::Render()
{
    if(twitterRenderer)
    {
        twitterRenderer->Render();
    }
    else
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium_Big)]);
        ImGui::TextWrapped("%s",embed.title.c_str());
        ImGui::PopFont();
        ImGui::TextWrapped("%s",embed.description.c_str());
    }
    if(urlRenderer)
    {
        urlRenderer->Render();
    }
}
