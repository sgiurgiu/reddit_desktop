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
    windowName = fmt::format("{}##{}",title,postId);
}
CommentsWindow::~CommentsWindow()
{
    if(postContentViewer) postContentViewer->stopPlayingMedia();
}
void CommentsWindow::setupListingConnection()
{
    if(listingConnection) return;

    listingConnection = client->makeListingClientConnection();
    listingConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
        auto self = weak.lock();
        if(!self || !self->windowOpen) return;
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
                self->loadListingsFromConnection(std::move(response.data),self);
           }
        }
    });

}
void CommentsWindow::loadComments()
{    
    setupListingConnection();
    listingConnection->list("/comments/"+postId+"?depth=5&limit=50&threaded=true&sort=confidence",token);
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse,
                                                std::shared_ptr<CommentsWindow> self)
{
    if(!windowOpen) return;
    if(listingResponse.json.is_array())
    {
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
            self->loadListingChildren(child["data"]["children"],self);
        }
    }
    else if (listingResponse.json.is_object())
    {
        if(listingResponse.json.contains("success") &&
                listingResponse.json["success"].is_boolean() &&
                listingResponse.json["success"].get<bool>() &&
                listingResponse.json.contains("jquery") &&
                listingResponse.json["jquery"].is_array())
        {
            for(const auto& jqueryObj : listingResponse.json["jquery"])
            {
                if(!jqueryObj.is_array()) continue;
                for(const auto& elem: jqueryObj)
                {
                    if(!elem.is_array()) continue;
                    for(const auto& childrenArray : elem)
                    {
                        if(!childrenArray.is_array()) continue;
                        self->loadListingChildren(childrenArray,self);
                    }
                }
            }
        }
    }
}
void CommentsWindow::loadListingChildren(const nlohmann::json& children,
                                         std::shared_ptr<CommentsWindow> self)
{
    if(!windowOpen || !children.is_array()) return;
    comments_list comments;
    post_ptr pp;
    std::optional<unloaded_children> unloadedPostComments;
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
        else if (kind == "more")
        {
            unloadedPostComments = std::make_optional<unloaded_children>(child["data"]);
        }
    }
    if(!comments.empty())
    {
        boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setComments,self,std::move(comments)));
    }
    if(pp)
    {
        boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setParentPost,self,std::move(pp)));
    }
    if(unloadedPostComments)
    {
        boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setUnloadedComments,self,std::move(unloadedPostComments)));
    }
}
void CommentsWindow::setUnloadedComments(std::optional<unloaded_children> children)
{
    unloadedPostComments = std::move(children);
    moreRepliesButtonText = fmt::format("load {} more comments##post_more_replies",unloadedPostComments->count);
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    listingErrorMessage.clear();
    if(receivedComments.empty()) return;
    if(comments.empty())
    {
        comments.reserve(receivedComments.size());
        std::move(receivedComments.begin(),receivedComments.end(),std::back_inserter(comments));
    }
    else
    {
        auto commentsParent = receivedComments.front().parentId;
        if(parentPost && commentsParent == parentPost->name)
        {
            comments.reserve(comments.size() + receivedComments.size());
            for(auto&& cmt : receivedComments)
            {
                comments.emplace_back(std::move(cmt));
            }
        }
        else
        {
            auto it = std::find_if(loadingMoreRepliesComments.begin(),loadingMoreRepliesComments.end(),
                                   [&commentsParent](const DisplayComment* c){
                return commentsParent == c->commentData.name;
            });
            if(it != loadingMoreRepliesComments.end())
            {
                (*it)->replies.reserve((*it)->replies.size() + receivedComments.size());
                for(auto&& cmt : receivedComments)
                {
                    (*it)->replies.emplace_back(std::move(cmt));
                }
                loadingMoreRepliesComments.erase(it);
            }
            else
            {
                //this is some programming error probably, something wonky happened
                std::cerr << "Cannot find comment "<<commentsParent<<" while loading its replies"<<std::endl;
            }
        }
    }
}
void CommentsWindow::setParentPost(post_ptr receivedParentPost)
{
    listingErrorMessage.clear();
    parentPost = receivedParentPost;
    if(postContentViewer)
    {
        postContentViewer->stopPlayingMedia();
        postContentViewer.reset();
    }
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
        if(c.commentData.unloadedChildren)
        {
            ImGui::SameLine();
            if(ImGui::Button(c.moreRepliesButtonText.c_str()))
            {
                std::string commaSeparator;
                std::string children;
                for(const auto& child:c.commentData.unloadedChildren->children)
                {
                    children+=std::exchange(commaSeparator,"%2C")+child;
                }
                loadingMoreRepliesComments.push_back(&c);
                listingConnection->list("/api/morechildren?link_id="+parentPost->name+
                                        "&sort=confidence&limit_children=false"+
                                        "&children="+children,token);
                c.commentData.unloadedChildren.reset();
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
    if(commentData.unloadedChildren)
    {
        moreRepliesButtonText = fmt::format("load {} more comments##{}_more_replies",commentData.unloadedChildren->count,commentData.name);
    }
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
    if(!postVotingConnection)
    {
        postVotingConnection = client->makeRedditVoteClientConnection();
        postVotingConnection->connectionCompleteHandler(
                    [weak=weak_from_this(),post=p.get(),voted=vote](const boost::system::error_code& ec,
                                                    const client_response<std::string>& response)
        {
            auto self = weak.lock();
            if(!self) return;
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
    }
    postVotingConnection->vote(p->name,token,vote);
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
    if(!commentVotingConnection)
    {
        commentVotingConnection = client->makeRedditVoteClientConnection();
        commentVotingConnection->connectionCompleteHandler(
                    [weak=weak_from_this(),c=c,voted=vote](const boost::system::error_code& ec,
                                                    const client_response<std::string>& response)
        {
            auto self = weak.lock();
            if(!self) return;
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
    }

    commentVotingConnection->vote(c->commentData.name,token,vote);
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
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)) && ImGui::IsWindowFocused())
    {
        loadingMoreRepliesComments.clear();
        comments.clear();
        loadComments();
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

    if(unloadedPostComments && ImGui::Button(moreRepliesButtonText.c_str()))
    {
        std::string commaSeparator;
        std::string children;
        for(const auto& child:unloadedPostComments->children)
        {
            children+=std::exchange(commaSeparator,"%2C")+child;
        }
        listingConnection->list("/api/morechildren?link_id="+parentPost->name+
                                "&sort=confidence&limit_children=false"+
                                "&children="+children,token);
        unloadedPostComments.reset();
    }

    ImGui::End();
}
void CommentsWindow::setFocused()
{
    willBeFocused = true;
}
