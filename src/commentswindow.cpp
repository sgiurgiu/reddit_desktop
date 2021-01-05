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
                               RedditClientProducer* client,
                               const boost::asio::any_io_executor& executor):
    postId(postId),title(title),token(token),client(client),
    uiExecutor(executor)
{    

}
CommentsWindow::~CommentsWindow()
{
    if(postContentViewer) postContentViewer->stopPlayingMedia();
}
void CommentsWindow::loadComments()
{
    auto connection = client->makeListingClientConnection();
    connection->connectionCompleteHandler([self=shared_from_this()](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
        if(!self->windowOpen) return;
        if(ec)
        {
           boost::asio::post(self->uiExecutor,
                             std::bind(&CommentsWindow::setErrorMessage,self,ec.message()));
        }
        else
        {
           if(response.status >= 400)
           {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&CommentsWindow::setErrorMessage,self,response.body));
           }
           else
           {
                self->loadListingsFromConnection(response.data);
           }
        }
    });
    windowName = fmt::format("{}##{}",title,postId);
    connection->list("/comments/"+postId+"?depth=5&limit=50&threaded=true",token);
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse)
{
    if(!windowOpen || !listingResponse.json.is_array()) return;
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
    if(!windowOpen || !children.is_array()) return;
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
            comments.emplace_back(child["data"]);
        }
        else if (kind == "t3")
        {
            //load post
            pp = std::make_shared<post>(child["data"]);
        }
    }
    if(!comments.empty())
    {
        boost::asio::post(uiExecutor,std::bind(&CommentsWindow::setComments,shared_from_this(),std::move(comments)));
    }
    if(pp)
    {
        boost::asio::post(uiExecutor,std::bind(&CommentsWindow::setParentPost,shared_from_this(),std::move(pp)));
    }
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    listingErrorMessage.clear();
    comments.reserve(comments.size() + receivedComments.size());
    std::move(receivedComments.begin(),receivedComments.end(),std::back_inserter(comments));
}
void CommentsWindow::setParentPost(post_ptr receivedParentPost)
{
    listingErrorMessage.clear();
    parentPost = receivedParentPost;
    postContentViewer = std::make_shared<PostContentViewer>(client,uiExecutor);
    postContentViewer->loadContent(parentPost);
    postUpvoteButtonText = fmt::format("{}##_up{}",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),parentPost->name);
    postDownvoteButtonText = fmt::format("{}##_down{}",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),parentPost->name);
    openLinkButtonText = fmt::format("{}##_openLink{}",
                                       reinterpret_cast<const char*>(ICON_FA_EXTERNAL_LINK_SQUARE " Open Link"),
                                       parentPost->name);
    commentButtonText = fmt::format("Comment##{}_comment",parentPost->name);
}

void CommentsWindow::showComment(DisplayComment& c)
{
    if(ImGui::TreeNodeEx(c.titleText.c_str(),ImGuiTreeNodeFlags_DefaultOpen))
    {
        c.renderer.RenderMarkdown();
        ImGui::NewLine();

        if(c.commentData.voted == Voted::UpVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetUpVoteColor());
        }

        if(ImGui::Button(c.upvoteButtonText.c_str()))
        {
            voteComment(&c,c.commentData.voted == Voted::UpVoted ? Voted::NotVoted : Voted::UpVoted);
        }

        if(c.commentData.voted == Voted::UpVoted)
        {
            ImGui::PopStyleColor(1);
        }

        ImGui::SameLine();

        if(c.commentData.voted == Voted::DownVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetDownVoteColor());
        }

        if(ImGui::Button(c.downvoteButtonText.c_str()))
        {
            voteComment(&c,c.commentData.voted == Voted::DownVoted ? Voted::NotVoted : Voted::DownVoted);
        }

        if(c.commentData.voted == Voted::DownVoted)
        {
            ImGui::PopStyleColor(1);
        }

        ImGui::SameLine();
        if(ImGui::Button(c.saveButtonText.c_str()))
        {

        }
        ImGui::SameLine();
        if(ImGui::Button(c.replyButtonText.c_str()))
        {

        }
        if(c.commentData.hasMoreReplies)
        {
            ImGui::SameLine();
            if(ImGui::Button(c.moreRepliesButtonText.c_str()))
            {

            }
        }
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
        for(auto&& reply : c.replies)
        {
            showComment(reply);
        }
        ImGui::TreePop();
    }
}
void CommentsWindow::DisplayComment::updateButtonsText()
{
    upvoteButtonText = fmt::format("{}##{}_up",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),commentData.name);
    downvoteButtonText = fmt::format("{}##{}_down",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),commentData.name);
    saveButtonText = fmt::format("save##{}_save",commentData.name);
    replyButtonText = fmt::format("reply##{}_reply",commentData.name);
    moreRepliesButtonText = fmt::format("more replies##{}_more_replies",commentData.name);
#ifdef REDDIT_DESKTOP_DEBUG
    titleText = fmt::format("{} - {} points, {} ({})",commentData.author,
                            commentData.humanScore,commentData.humanReadableTimeDifference,
                            commentData.name);
#else
    titleText = fmt::format("{} - {} points, {}",commentData.author,
                            commentData.humanScore,commentData.humanReadableTimeDifference);
#endif
}
void CommentsWindow::voteParentPost(post_ptr p, Voted vote)
{
    listingErrorMessage.clear();
    auto clientConnection = client->makeRedditVoteClientConnection();
    clientConnection->connectionCompleteHandler(
                [self=shared_from_this(),post=p.get(),voted=vote](const boost::system::error_code& ec,
                                                const client_response<std::string>& response)
    {
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,self,ec.message()));
        }
        else if(response.status >= 400)
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,self,std::move(response.body)));
        }
        else
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::updatePostVote,self,post,voted));
        }
    });

    clientConnection->vote(p->name,token,vote);
}
void CommentsWindow::updatePostVote(post* p, Voted vote)
{
    auto oldVoted = p->voted;
    p->voted = vote;
    p->humanScore = Utils::CalculateScore(p->score,oldVoted,vote);
}
void CommentsWindow::voteComment(DisplayComment* c,Voted vote)
{
    listingErrorMessage.clear();
    auto clientConnection = client->makeRedditVoteClientConnection();
    clientConnection->connectionCompleteHandler(
                [self=shared_from_this(),c=c,voted=vote](const boost::system::error_code& ec,
                                                const client_response<std::string>& response)
    {
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,self,ec.message()));
        }
        else if(response.status >= 400)
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,self,std::move(response.body)));
        }
        else
        {
            boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::updateCommentVote,self,c,voted));
        }
    });

    clientConnection->vote(c->commentData.name,token,vote);
}
void CommentsWindow::updateCommentVote(DisplayComment* c,Voted vote)
{
    auto oldVoted = c->commentData.voted;
    c->commentData.voted = vote;
    c->commentData.humanScore = Utils::CalculateScore(c->commentData.score,oldVoted,vote);
    c->updateButtonsText();
}

void CommentsWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!windowOpen)
    {
        if(postContentViewer) postContentViewer->stopPlayingMedia();
        return;
    }
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

    if(parentPost)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
        ImGui::TextWrapped("%s",parentPost->title.c_str());
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("Posted by %s %s",parentPost->author.c_str(),parentPost->humanReadableTimeDifference.c_str());
        ImGui::PopFont();
        ImGui::NewLine();

        postContentViewer->showPostContent();

        if(parentPost->voted == Voted::UpVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetUpVoteColor());
        }
        if(ImGui::Button(postUpvoteButtonText.c_str()))
        {
            voteParentPost(parentPost,parentPost->voted == Voted::UpVoted ? Voted::NotVoted : Voted::UpVoted);
        }
        if(parentPost->voted == Voted::UpVoted)
        {
            ImGui::PopStyleColor(1);
        }
        ImGui::SameLine();
        ImGui::Text("%s",parentPost->humanScore.c_str());
        ImGui::SameLine();
        if(parentPost->voted == Voted::DownVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetDownVoteColor());
        }
        if(ImGui::Button(postDownvoteButtonText.c_str()))
        {
            voteParentPost(parentPost,parentPost->voted == Voted::DownVoted ? Voted::NotVoted : Voted::DownVoted);
        }
        if(parentPost->voted == Voted::DownVoted)
        {
            ImGui::PopStyleColor(1);
        }

        ImGui::SameLine();
        ImGui::Button(commentButtonText.c_str());
        if(!parentPost->url.empty())
        {
            ImGui::SameLine();

            if(ImGui::Button(openLinkButtonText.c_str()))
            {
                Utils::openInBrowser(parentPost->url);
            }
        }
        if(!parentPost->subreddit.empty())
        {
            ImGui::SameLine();
            if(ImGui::Button("Subreddit##openSubredditWindow"))
            {
                openSubredditSignal(parentPost->subreddit);
            }

        }
        ImGui::Separator();
    }
    for(auto&& c : comments)
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
