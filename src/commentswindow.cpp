#include "commentswindow.h"
#include <imgui.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "markdownrenderer.h"
#include "macros.h"
#include <iostream>
#include "database.h"

CommentsWindow::CommentsWindow(const std::string& postId,
                               const std::string& title,
                               const access_token& token,
                               RedditClient* client,
                               const boost::asio::io_context::executor_type& executor):
    postId(postId),token(token),client(client),connection(client->makeListingClientConnection()),
    uiExecutor(executor)
{    
    connection->connectionCompleteHandler([this](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
       if(ec)
       {
           boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,ec.message()));
       }
       else
       {
           if(response.status >= 400)
           {
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,response.body));
           }
           else
           {
                loadListingsFromConnection(response.data);
           }
       }
    });
    windowName = fmt::format("{}##{}",title,postId);
    connection->list("/comments/"+postId+"?depth=5&limit=50&threaded=true",token);
}
CommentsWindow::~CommentsWindow()
{
    connection->clearAllSlots();
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse)
{
    if(!listingResponse.json.is_array()) return;
    for(const auto& child:listingResponse.json)
    {
        if(!child.is_object())
        {
            continue;
        }
        if(!child.contains("kind") || child["kind"].get<std::string>() != "Listing")
        {
            continue;
        }
        if(!child.contains("data") || !child["data"].contains("children"))
        {
            continue;
        }
        loadListingChildren(child["data"]["children"]);
    }
}
void CommentsWindow::loadListingChildren(const nlohmann::json& children)
{
    if(!children.is_array()) return;
    comments_list comments;
    post_ptr pp;
    for(const auto& child: children)
    {
        if(!child.is_object())
        {
            continue;
        }
        if(!child.contains("kind") || !child.contains("data"))
        {
            continue;
        }
        auto kind = child["kind"].get<std::string>();
        if(kind == "t1")
        {
            //load comments
            comments.emplace_back(std::make_shared<comment>(child["data"]));
        }
        else if (kind == "t3")
        {
            //load post
            pp = std::make_shared<post>(child["data"]);
        }
    }
    if(!comments.empty())
    {
        boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setComments,this,std::move(comments)));
    }
    if(pp)
    {
        boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setParentPost,this,std::move(pp)));
    }
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    listingErrorMessage.clear();
    comments.reserve(comments.size() + receivedComments.size());
    std::move(receivedComments.begin(), receivedComments.end(), std::back_inserter(comments));
}
void CommentsWindow::setParentPost(post_ptr receivedParentPost)
{
    listingErrorMessage.clear();
    parent_post = receivedParentPost;
    postContentViewer = std::make_unique<PostContentViewer>(client,uiExecutor,parent_post);
}

void CommentsWindow::showComment(comment_ptr c)
{
    if(ImGui::TreeNodeEx(fmt::format("{} - {} points, {}",c->author,c->humanScore,c->humanReadableTimeDifference).c_str(),ImGuiTreeNodeFlags_DefaultOpen))
    {
        MarkdownRenderer::RenderMarkdown(c->body);
        ImGui::NewLine();
        ImGui::Button(fmt::format("{}##{}_up",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("{}##{}_down",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("save##{}_save",c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("reply##{}_reply",c->id).c_str());
        if(c->hasMoreReplies)
        {
            ImGui::SameLine();
            ImGui::Button(fmt::format("more replies##{}_more_replies",c->id).c_str());
        }
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
        for(const auto& reply : c->replies)
        {
            showComment(reply);
        }
        ImGui::TreePop();
    }
}
void CommentsWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!windowOpen) return;

    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_None))
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
    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    if(parent_post)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Bold)]);
        ImGui::TextWrapped("%s",parent_post->title.c_str());
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Light)]);
        ImGui::Text("Posted by %s %s",parent_post->author.c_str(),parent_post->humanReadableTimeDifference.c_str());
        ImGui::PopFont();
        ImGui::NewLine();

        postContentViewer->showPostContent();

        ImGui::Button(fmt::format("Comment##{}_comment",parent_post->id).c_str());

        ImGui::Separator();
    }
    for(const auto& c : comments)
    {
        showComment(c);
        ImGui::Separator();
    }

    ImGui::End();
}
void CommentsWindow::setFocused()
{
    willBeFocused = true;
}
