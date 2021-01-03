#include "userinformationwindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"

namespace
{
    constexpr auto USER_INFO_WINDOW_TITLE = "Messages and information";
}

UserInformationWindow::UserInformationWindow(const access_token& token,
                                             RedditClientProducer* client,
                                             const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor)
{
}
void UserInformationWindow::loadMessages()
{
    loadMoreUnreadMessages();
    loadMoreAllMessages();
    loadMoreSentMessages();
}
void UserInformationWindow::loadMoreUnreadMessages()
{
    loadMessages("unread",&unreadMessages);
}
void UserInformationWindow::loadMoreAllMessages()
{
    loadMessages("inbox",&unreadMessages);
}
void UserInformationWindow::loadMoreSentMessages()
{
    loadMessages("sent",&unreadMessages);
}
void UserInformationWindow::loadMessages(const std::string& kind,Messages* messages)
{
    std::string url = "/message/"+kind+"?count="+std::to_string(messages->count);
    if(!unreadMessages.after.empty())
    {
        url+= "&after="+unreadMessages.after;
    }
    auto connection = client->makeListingClientConnection();
    connection->connectionCompleteHandler([self=shared_from_this(),messages](const boost::system::error_code& ec,
                                          const client_response<listing>& response)
    {
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::setErrorMessage,self,ec.message()));
        }
        else
        {
            if(response.status >= 400)
            {
                boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::setErrorMessage,self,std::move(response.body)));
            }
            else
            {
                boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::loadListingsFromConnection,
                                                             self,std::move(response.data),messages));
            }
        }
    });
    connection->list(url,token);
}
void UserInformationWindow::loadListingsFromConnection(listing listingResponse,Messages* messages)
{
    std::string kind;
    if(listingResponse.json.contains("kind") && listingResponse.json["kind"].is_string())
    {
        kind = listingResponse.json["kind"].get<std::string>();
    }
    if(kind != "Listing") return;
    if(listingResponse.json.contains("data") && listingResponse.json["data"].is_object())
    {
        const auto& data = listingResponse.json["data"];
        if(data.contains("children") && data["children"].is_array())
        {
            for(const auto& child : data["children"])
            {
                auto kind = child["kind"].get<std::string>();
                if (kind == "t4")
                {
                    messages->messages.emplace_back(child["data"]);
                }
            }
            messages->count = messages->messages.size();
            if(messages->count > 0)
            {
                messages->after = messages->messages.back().msg.id;
            }
        }
    }
}
void UserInformationWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = std::move(errorMessage);
}
void UserInformationWindow::showUserInfoWindow(int appFrameWidth,int appFrameHeight)
{
    if(!showWindow) return;
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.5,appFrameHeight*0.5),ImGuiCond_FirstUseEver);
    if(!ImGui::Begin(USER_INFO_WINDOW_TITLE,&showWindow,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("MessagesTabBar", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Unread"))
        {
            for(const auto& msg : unreadMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inbox"))
        {
            for(const auto& msg : allMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sent"))
        {
            for(const auto& msg : sentMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void UserInformationWindow::showMessage(const DisplayMessage& msg)
{
    ImGui::Text("%s",msg.msg.subject.c_str());
    msg.renderer.RenderMarkdown();
}
