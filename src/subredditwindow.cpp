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

    connection->connectionCompleteHandler([this](const boost::system::error_code& ec,
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
            if(child["data"]["is_self"].is_boolean())
            {
                p.isSelf = child["data"]["is_self"].get<bool>();
            }
            if(child["data"]["thumbnail"].is_string())
            {
                p.thumbnail = child["data"]["thumbnail"].get<std::string>();
            }
            if(child["data"]["created"].is_number())
            {
                p.createdAt = child["data"]["created"].get<uint64_t>();
            }
            if(child["data"]["num_comments"].is_number())
            {
                p.commentsCount = child["data"]["num_comments"].get<int>();
            }
            if(child["data"]["subreddit"].is_string())
            {
                p.subreddit = child["data"]["subreddit"].get<std::string>();
            }
            if(child["data"]["score"].is_number())
            {
                p.score = child["data"]["score"].get<int>();
            }
            if(child["data"]["url"].is_string())
            {
                p.url = child["data"]["url"].get<std::string>();
            }
            if(child["data"].contains("preview") &&
                    child["data"]["preview"].is_object() &&
                    child["data"]["preview"]["images"].is_array())
            {
                for(const auto& img : child["data"]["preview"]["images"])
                {
                    images_preview preview;
                    if(img["source"].is_object())
                    {
                        preview.source.url = img["source"]["url"].get<std::string>();
                        preview.source.width = img["source"]["width"].get<int>();
                        preview.source.height = img["source"]["height"].get<int>();
                    }
                    if(img["resolutions"].is_array())
                    {
                        for(const auto& res : img["resolutions"]) {
                            image_target img_target;
                            img_target.url = res["url"].get<std::string>();
                            img_target.width = res["width"].get<int>();
                            img_target.height = res["height"].get<int>();
                            preview.resolutions.push_back(img_target);
                        }
                    }

                    p.previews.push_back(preview);
                }
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
        if(p.thumbnail != "self")
        {
            //ImGui::Image();
        }
        ImGui::TextWrapped("%s",p.title.c_str());
        ImGui::Separator();
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
