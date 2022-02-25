#include "livethreadviewer.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <fmt/format.h>
#include <boost/url.hpp>
#include "utils.h"

LiveThreadViewer::LiveThreadViewer(RedditClientProducer* client,
                                   const access_token& token,
                                   const boost::asio::any_io_executor& uiExecutor):
    client(client),token(token),uiExecutor(uiExecutor)
{
    liveConnection = client->makeRedditLiveThreadClientConnection();
}
void LiveThreadViewer::showLiveThread()
{
    if(!errorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",errorMessage.c_str());
        return;
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

    for(const auto& event:liveEvents)
    {
        if(event->event.embeds.empty())
        {
            event->bodyRenderer.RenderMarkdown();
        }
        else
        {
            for(const auto& live:event->event.embeds)
            {
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium_Big)]);
                ImGui::TextWrapped("%s",live.title.c_str());
                ImGui::PopFont();
                ImGui::TextWrapped("%s",live.description.c_str());
            }
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
}
void LiveThreadViewer::addLiveEvent(live_update_event event)
{

    if(eventsMap.contains(event.name))
    {
        eventsMap[event.name]->event.embeds = event.embeds;
    }
    else
    {
        auto eventShared = std::make_shared<EventDisplay>(std::move(event));
        eventsMap[eventShared->event.name] = eventShared;
        liveEvents.insert(liveEvents.begin(),eventShared);
    }
}
void LiveThreadViewer::loadLiveThreadAbout(live_update_event_about liveEventAbout)
{
    int a = 1;
    this->eventAbout = std::move(liveEventAbout);
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
        auto eventShared = std::make_shared<EventDisplay>(child["data"]);
        eventsMap[eventShared->event.name] = eventShared;
        liveEvents.push_back(eventShared);
        loadingPostContent = false;
    }
}
void LiveThreadViewer::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = std::move(errorMessage);
}
