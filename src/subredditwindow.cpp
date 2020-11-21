#include "subredditwindow.h"
#include <imgui.h>

#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "spinner/spinner.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <boost/asio/post.hpp>

SubredditWindow::SubredditWindow(int id, const std::string& subreddit,
                                 const access_token& token,
                                 RedditClient* client,
                                 const boost::asio::io_context::executor_type& executor):
    id(id),subreddit(subreddit),token(token),client(client),
    connection(client->makeListingClientConnection()),uiExecutor(executor)
{

    connection->connectionCompleteHandler([this](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
       if(ec)
       {
           boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,this,ec.message()));
       }
       else
       {
           if(response.status >= 400)
           {
               boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,this,response.body));
           }
           else
           {
               loadListingsFromConnection(response.data);
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
void SubredditWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void SubredditWindow::loadListingsFromConnection(const listing& listingResponse)
{
    posts_list tmpPosts;
    for(auto& child:listingResponse.json["data"]["children"])
    {
        auto kind = child["kind"].get<std::string>();
        if(kind == "t3")
        {
            auto p = std::make_unique<post>();
            p->title = child["data"]["title"].get<std::string>();
            if(child["data"]["selftext"].is_string())
            {
                p->selfText = child["data"]["selftext"].get<std::string>();
            }
            if(child["data"]["ups"].is_number())
            {
                p->ups = child["data"]["ups"].get<int>();
            }
            if(child["data"]["downs"].is_number())
            {
                p->downs = child["data"]["downs"].get<int>();
            }
            if(child["data"]["is_video"].is_boolean())
            {
                p->isVideo = child["data"]["is_video"].get<bool>();
            }
            if(child["data"]["is_self"].is_boolean())
            {
                p->isSelf = child["data"]["is_self"].get<bool>();
            }
            if(child["data"]["thumbnail"].is_string())
            {
                p->thumbnail = child["data"]["thumbnail"].get<std::string>();
            }
            if(child["data"]["created"].is_number())
            {
                p->createdAt = child["data"]["created"].get<uint64_t>();
            }
            if(child["data"]["num_comments"].is_number())
            {
                p->commentsCount = child["data"]["num_comments"].get<int>();
            }
            if(child["data"]["subreddit"].is_string())
            {
                p->subreddit = child["data"]["subreddit"].get<std::string>();
            }
            if(child["data"]["score"].is_number())
            {
                p->score = child["data"]["score"].get<int>();
            }
            if(child["data"]["url"].is_string())
            {
                p->url = child["data"]["url"].get<std::string>();
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

                    p->previews.push_back(preview);
                }
            }
            //p.thumbnail_picture.textureId = 1;
            tmpPosts.push_back(std::move(p));
        }
    }
    boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setListings,this,std::move(tmpPosts)));
}
void SubredditWindow::setListings(posts_list receivedPosts)
{
    listingErrorMessage.clear();    
    posts.clear();
    posts.reserve(receivedPosts.size());
    std::move(receivedPosts.begin(), receivedPosts.end(), std::back_inserter(posts));
    for(auto&& p : posts)
    {
        if(!p->thumbnail.empty() && p->thumbnail != "self")
        {
            auto resourceConnection = client->makeResourceClientConnection(p->thumbnail);
            resourceConnection->connectionCompleteHandler(
                        [post=p.get(),this](const boost::system::error_code&,
                             const resource_response& response)
            {
                if(response.status == 200)
                {
                    int width, height, channels;
                    auto data = stbi_load_from_memory(response.data.data(),response.data.size(),&width,&height,&channels,3);
                    boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setPostThumbnail,this,post,data,width,height,channels));
                }
            });
            resourceConnection->getResource(token);
        }
    }
}
void SubredditWindow::setPostThumbnail(post* p,unsigned char* data, int width, int height, int channels)
{
    auto image = Utils::LoadImage(data,width,height,channels);
    stbi_image_free(data);
    p->thumbnail_picture = std::move(image);
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

    //ImGuiStyle& style = ImGui::GetStyle();
   // auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x + style.ItemInnerSpacing.x;
    //auto availableWidth = ImGui::GetContentRegionAvailWidth();

    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    for(auto&& p : posts)
    {        

        ImGui::Columns(3);
        ImGui::BeginGroup();
        ImGui::Text("Upvotes: %d",p->ups);
        ImGui::Text("%d",p->score);
        ImGui::Text("Downvotes: %d",p->downs);
        ImGui::EndGroup();
        ImGui::NextColumn();
        //ImGui::GetCursorPosX();
        if(p->thumbnail_picture)
        {
            ImGui::Image((void*)(intptr_t)p->thumbnail_picture->textureId,
                         ImVec2(p->thumbnail_picture->width,p->thumbnail_picture->height));
        //    ImGui::SameLine();
        }

        ImGui::NextColumn();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Medium_Big)]);
        if(!p->title.empty())
        {
            ImGui::TextWrapped("%s",p->title.c_str());
        }
        else
        {
            ImGui::TextWrapped("<No Title>");
        }
        ImGui::PopFont();        
        auto commentsText = fmt::format(reinterpret_cast<const char*>(ICON_FA_COMMENTS " {} comments"),p->commentsCount);
        ImGui::Button(commentsText.c_str());

        ImGui::Columns(1);
        ImGui::Separator();
    }

    ImGui::End();
}



void SubredditWindow::showWindowMenu()
{
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)))
    {
        connection->list(target,token);
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
