#include "commentswindow.h"
#include <imgui.h>
#include <fmt/format.h>
#include "markdownrenderer.h"

CommentsWindow::CommentsWindow(const std::string& postId,
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
    windowName = fmt::format("Comments of {}",postId);
    connection->list("/comments/"+postId,token);
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
    if(parent_post)
    {
        boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setParentPost,this,std::move(pp)));
    }
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    listingErrorMessage.clear();
    comments.reserve(receivedComments.size());
    std::move(receivedComments.begin(), receivedComments.end(), std::back_inserter(comments));
}
void CommentsWindow::setParentPost(post_ptr receivedParentPost)
{
    listingErrorMessage.clear();
    parent_post = receivedParentPost;
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
    for(const auto& c : comments)
    {
        MarkdownRenderer::RenderMarkdown(c->body);
        ImGui::Separator();
    }

    ImGui::End();
}
void CommentsWindow::setFocused()
{
    willBeFocused = true;
}
