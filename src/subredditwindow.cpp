#include "subredditwindow.h"
#include <imgui.h>

#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "spinner/spinner.h"

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
        windowName = fmt::format("{}##{}",subreddit,id);
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
            tmpPosts.emplace_back(std::make_shared<post>(child["data"]));
        }
    }
    boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setListings,this,std::move(tmpPosts),
                                                 std::move(listingResponse.json["data"]["before"]),
                                                 std::move(listingResponse.json["data"]["after"])));
}
void SubredditWindow::setListings(posts_list receivedPosts,nlohmann::json beforeJson,nlohmann::json afterJson)
{
    before.reset();
    after.reset();
    if(!beforeJson.is_null() && beforeJson.is_string())
    {
        before = beforeJson.get<std::string>();
    }
    if(!afterJson.is_null() && afterJson.is_string())
    {
        after = afterJson.get<std::string>();
    }
    listingErrorMessage.clear();    
    posts.clear();
    posts.reserve(receivedPosts.size());
    std::move(receivedPosts.begin(), receivedPosts.end(), std::back_inserter(posts));
    scrollToTop = true;
    for(auto&& p : posts)
    {
        auto thumbnail = p->thumbnail;
        if(!p->thumbnail.empty() && p->thumbnail != "self" && p->thumbnail != "default")
        {
            auto resourceConnection = client->makeResourceClientConnection(p->thumbnail);
            resourceConnection->connectionCompleteHandler(
                        [post=p.get(),this](const boost::system::error_code&,
                             const resource_response& response)
            {
                if(response.status == 200)
                {
                    int width, height, channels;
                    auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                    boost::asio::post(this->uiExecutor,std::bind(&SubredditWindow::setPostThumbnail,this,post,data,width,height,channels));
                }
            });
            resourceConnection->getResource();
        }
    }
}
void SubredditWindow::setPostThumbnail(post* p,unsigned char* data, int width, int height, int channels)
{
    auto image = Utils::loadImage(data,width,height,channels);
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
    if(willBeFocused)
    {
        ImGui::SetWindowFocus();
        willBeFocused = false;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)) && ImGui::GetIO().KeyCtrl && ImGui::IsWindowFocused())
    {
        windowOpen = false;
    }
    if(scrollToTop)
    {
        ImGui::SetScrollHereY(0.0f);
        scrollToTop = false;
    }

    showWindowMenu();

    //ImGuiStyle& style = ImGui::GetStyle();
   // auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x + style.ItemInnerSpacing.x;
    //auto availableWidth = ImGui::GetContentRegionAvailWidth();

    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    if(maxScoreWidth == 0.f)
    {
        maxScoreWidth = ImGui::CalcTextSize("99.9k").x  - (ImGui::GetStyle().FramePadding.x*2.f);
    }
    if(upvotesButtonsIdent == 0.f)
    {
        auto upvotesButtonsWidth = ImGui::CalcTextSize(reinterpret_cast<const char*>(ICON_FA_ARROW_UP)).x  + (ImGui::GetStyle().FramePadding.x);
        upvotesButtonsIdent = (maxScoreWidth - upvotesButtonsWidth) / 2.f - (ImGui::GetStyle().FramePadding.x);
    }

    for(auto&& p : posts)
    {                
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.0f,1.0f,1.0f,0.0f));
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();
        ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_UP));
        auto scoreIdent = (maxScoreWidth - ImGui::CalcTextSize(p->humanScore.c_str()).x)/2.f;
        ImGui::Dummy(ImVec2(scoreIdent,0.f));ImGui::SameLine();
        ImGui::Text("%s",p->humanScore.c_str());
        if(ImGui::GetItemRectSize().x > maxScoreWidth) maxScoreWidth = ImGui::GetItemRectSize().x;
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();
        ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN));        
        ImGui::PopStyleColor(1);
        ImGui::EndGroup();

        auto height = ImGui::GetCursorPosY();
        ImGui::SameLine();
        if(p->thumbnail_picture)
        {
            ImGui::Image((void*)(intptr_t)p->thumbnail_picture->textureId,
                         ImVec2(p->thumbnail_picture->width,p->thumbnail_picture->height));
            height = std::max(height,ImGui::GetCursorPosY());
            ImGui::SameLine();
        }

        ImGui::BeginGroup();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Bold)]);
        ImGui::Text("%s",p->subreddit.c_str());
        ImGui::PopFont();
        ImGui::SameLine();        
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Light)]);
        ImGui::Text("Posted by %s %s",p->author.c_str(),p->humanReadableTimeDifference.c_str());
        ImGui::PopFont();

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
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Light)]);
        ImGui::Text("(%s)",p->domain.c_str());
        ImGui::PopFont();

        auto normalPositionY = ImGui::GetCursorPosY();
        auto desiredPositionY = height - ImGui::GetFrameHeightWithSpacing();
        if(normalPositionY < desiredPositionY) ImGui::SetCursorPosY(desiredPositionY);
        if(ImGui::Button(p->commentsText.c_str()))
        {
            commentsSignal(p->id,p->title);
        }
        ImGui::EndGroup();
        ImGui::Separator();
    }   

    if(!posts.empty())
    {
        bool previousEnabled = before.has_value();
        bool nextEnabled = after.has_value();
        if(!previousEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT " Previous")) && previousEnabled)
        {
            connection->list(target+"?before="+before.value()+"&count="+std::to_string(currentCount),token);
            currentCount-=posts.size();
            before.reset();
        }
        if(!previousEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
        auto width = ImGui::GetWindowContentRegionMax().x;
        auto previousSize = ImGui::GetItemRectSize();
        ImGui::SameLine(width - previousSize.x);
        if(!nextEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if(ImGui::Button(reinterpret_cast<const char*>("Next " ICON_FA_ARROW_RIGHT),previousSize) && nextEnabled)
        {
            currentCount+=posts.size();
            connection->list(target+"?after="+after.value()+"&count="+std::to_string(currentCount),token);
            after.reset();
        }
        if(!nextEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
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

void SubredditWindow::showCommentsListener(const typename CommentsSignal::slot_type& slot)
{
    commentsSignal.connect(slot);
}
void SubredditWindow::setFocused()
{
    willBeFocused = true;
}
