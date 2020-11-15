#include "subredditwindow.h"
#include <imgui.h>

#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "spinner/spinner.h"

SubredditWindow::SubredditWindow(int id, const std::string& subreddit,
                                 const access_token& token,
                                 RedditClient* client):id(id),subreddit(subreddit),token(token),client(client),
    connection(client->makeListingClientConnection())
{

    connection->listingComplete([this](const boost::system::error_code& ec,
                                const client_response<listing>& response){
           if(ec)
           {
               listingErrorMessage = ec.message();
           }
           else
           {
               if(response.status >= 400)
               {
                   listingErrorMessage = response.body;
               }
               else
               {
                   listingErrorMessage.clear();
                   loadListings(response.data);
               }
           }
    });

    target = subreddit;
    if(subreddit.empty())
    {
        windowName = fmt::format("Front Page##{}",id);
        target = "/";
    }
    else
    {
        windowName = fmt::format("{}##",subreddit,id);
        if(!target.starts_with("r/") && !target.starts_with("/r/")) target = "/r/" + target;
    }

    if(!target.starts_with("/")) target = "/" + target;

    connection->list(target,token);
}
void SubredditWindow::loadListings(const listing& listingResponse)
{
    for(auto& child:listingResponse.json["data"]["children"])
    {
        auto kind = child["kind"].get<std::string>();
        if(kind == "t3")
        {
            post p;
            p.title = child["data"]["title"].get<std::string>();
            if(child["data"]["selftext"].is_string())
            {
                p.selfText = child["data"]["selftext"].get<std::string>();
            }
            if(child["data"]["ups"].is_number())
            {
                p.ups = child["data"]["ups"].get<int>();
            }
            if(child["data"]["downs"].is_number())
            {
                p.downs = child["data"]["downs"].get<int>();
            }
            if(child["data"]["is_video"].is_boolean())
            {
                p.isVideo = child["data"]["is_video"].get<bool>();
            }
            posts.push_back(p);
        }
    }
}
void SubredditWindow::showWindow(int appFrameWidth,int appFrameHeight)
{

    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!windowOpen) return;
    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_MenuBar))
    {
        ImGui::End();
        return;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)) && ImGui::GetIO().KeyCtrl)
    {
        windowOpen = false;
    }

    showWindowMenu();

    ImGuiStyle& style = ImGui::GetStyle();
    auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x + style.ItemInnerSpacing.x;
    auto availableWidth = ImGui::GetContentRegionAvailWidth();

    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }


    for(const auto& p : posts)
    {
        ImGui::TextWrapped("%s",p.title.c_str());
    }


    ImGui::End();
}

void SubredditWindow::showWindowMenu()
{
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)))
    {

    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Statistics","Ctrl+S",false))
            {

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Refresh","F5",false))
            {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
