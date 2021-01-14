#include "commentswindow.h"
#include <imgui.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "markdownrenderer.h"
#include "macros.h"
#include <iostream>
#include "database.h"
#include "spinner/spinner.h"
#include "resizableinputtextmultiline.h"

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
void CommentsWindow::setupListingConnections()
{
    if(moreChildrenConnection && listingConnection) return;

    auto completeHandler = [weak=weak_from_this()](const boost::system::error_code& ec,
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
    };
    if(!moreChildrenConnection)
    {
        moreChildrenConnection = client->makeRedditMoreChildrenClientConnection();
        moreChildrenConnection->connectionCompleteHandler(completeHandler);
    }
    if(!listingConnection)
    {
        listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler(completeHandler);
    }
    if(!createCommentConnection)
    {
        createCommentConnection = client->makeRedditCreateCommentClientConnection();
        createCommentConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
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
                   if(response.data.json.contains("json") && response.data.json["json"].is_object())
                   {
                       const auto& respJson = response.data.json["json"];
                       if(respJson.contains("data") && respJson["data"].is_object() &&
                               respJson["data"].contains("things") && respJson["data"]["things"].is_array())
                       {
                          auto& things = respJson["data"]["things"];
                          self->loadListingChildren(things,self,false);
                       }
                   }
               }
           }
       });
    }

}
void CommentsWindow::loadComments()
{    
    setupListingConnections();
    //depth=15&limit=100&threaded=true&
    listingConnection->list("/comments/"+postId+"?sort=confidence",token);
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse,
                                                std::shared_ptr<CommentsWindow> self)
{
    if(!windowOpen) return;
    loadingInitialComments = false;
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
            self->loadListingChildren(child["data"]["children"],self,true);
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
                        self->loadListingChildren(childrenArray,self,true);
                    }
                }
            }
        }
    }
}
void CommentsWindow::loadListingChildren(const nlohmann::json& children,
                                         std::shared_ptr<CommentsWindow> self,
                                         bool append)
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
        boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::setComments,self,std::move(comments), append));
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
    if(!children || children->count <= 0 || children->children.empty()) return;
    unloadedPostComments = std::move(children);
    moreRepliesButtonText = fmt::format("load {} more comments##post_more_replies",unloadedPostComments->count);
    loadingSpinnerIdText = fmt::format("##spinner_loading{}",unloadedPostComments->id);
}
void CommentsWindow::setComments(comments_list receivedComments, bool append)
{
    loadingInitialComments = false;
    postingComment = false;
    std::fill_n(postCommentTextBuffer, sizeof(postCommentTextBuffer), '\0');
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
                if(append)
                {
                    comments.emplace_back(std::move(cmt));
                }
                else
                {
                    comments.insert(comments.begin(),std::move(cmt));
                }
            }
            loadingUnloadedReplies = false;
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
                    if(append)
                    {
                        (*it)->replies.emplace_back(std::move(cmt));
                    }
                    else
                    {
                        (*it)->replies.insert((*it)->replies.begin(),std::move(cmt));
                    }
                }
                (*it)->loadingUnloadedReplies = false;
                (*it)->postingReply = false;
                std::fill_n((*it)->postReplyTextBuffer, sizeof((*it)->postReplyTextBuffer), '\0');
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
void CommentsWindow::loadUnloadedChildren(const std::optional<unloaded_children>& children)
{
   setupListingConnections();
   if(!children) return;
   moreChildrenConnection->list(children.value(),parentPost->name,token);
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
    commentButtonText = fmt::format("Save##{}_comment",parentPost->name);
    postCommentTextFieldId = fmt::format("##{}_postComment",parentPost->name);
    postCommentPreviewCheckboxId = fmt::format("Show Preview##{}_postCommentPreview",parentPost->name);
}

void CommentsWindow::showComment(DisplayComment& c)
{
    if(ImGui::TreeNodeEx(c.titleText.c_str(),ImGuiTreeNodeFlags_DefaultOpen))
    {
        c.renderer.RenderMarkdown();
        //ImGui::NewLine();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
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
            c.showingReplyArea = !c.showingReplyArea;
        }
        if(c.showingReplyArea)
        {

            if(ResizableInputTextMultiline::InputText(c.replyIdText.c_str(),c.postReplyTextBuffer,sizeof(c.postReplyTextBuffer),
                                      &c.postReplyTextFieldSize) && c.showingPreview)
            {
                c.previewRenderer.SetText(c.postReplyTextBuffer);
            }

            if(ImGui::Button(c.saveReplyButtonText.c_str()) && !c.postingReply)
            {
                c.showingReplyArea = false;
                c.postingReply = true;
                loadingMoreRepliesComments.push_back(&c);
                createCommentConnection->createComment(c.commentData.name,std::string(c.postReplyTextBuffer),token);
            }
            ImGui::SameLine();
            if(ImGui::Checkbox(c.postReplyPreviewCheckboxId.c_str(),&c.showingPreview))
            {
                c.previewRenderer.SetText(c.postReplyTextBuffer);
            }
            if(c.showingPreview)
            {
                if(ImGui::BeginChild(c.liveReplyPreviewText.c_str(),c.postReplyPreviewSize,true))
                {
                    c.previewRenderer.RenderMarkdown();
                    auto endPos = ImGui::GetCursorPos();
                    c.postReplyPreviewSize.y = endPos.y + ImGui::GetTextLineHeight();

                }
                ImGui::EndChild();
            }
        }

        for(auto&& reply : c.replies)
        {
            showComment(reply);
        }
        if(c.commentData.unloadedChildren)
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
            if(ImGui::Button(c.moreRepliesButtonText.c_str()))
            {
                loadUnloadedChildren(c.commentData.unloadedChildren);
                loadingMoreRepliesComments.push_back(&c);
                c.commentData.unloadedChildren.reset();
                c.loadingUnloadedReplies = true;
            }
        }
        else if(c.loadingUnloadedReplies)
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
            ImGui::Spinner(c.spinnerIdText.c_str(),ImGui::GetFontSize() * 0.75f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
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
    replyIdText = fmt::format("##{}comment_reply",commentData.name);
    saveReplyButtonText = fmt::format("Save##{}save_comment_reply",commentData.name);
    if(commentData.unloadedChildren)
    {
        moreRepliesButtonText = fmt::format("load {} more comments##{}_more_replies",commentData.unloadedChildren->count,commentData.name);
        spinnerIdText = fmt::format("##spinner_loading{}",commentData.unloadedChildren->id);
    }
    postReplyPreviewCheckboxId = fmt::format("Show Preview##{}_postReplyPreview",commentData.name);
    liveReplyPreviewText = fmt::format("Live Preview##{}_commentLivePreview",commentData.name);
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
    auto commentVotingConnection = client->makeRedditVoteClientConnection();
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
        if(ResizableInputTextMultiline::InputText(postCommentTextFieldId.c_str(),postCommentTextBuffer,sizeof(postCommentTextBuffer),
                                  &postCommentTextFieldSize) && showingPostPreview)
        {
            postPreviewRenderer.SetText(postCommentTextBuffer);
        }
        if(ImGui::Button(commentButtonText.c_str()) && !postingComment)
        {
            postingComment = true;
            createCommentConnection->createComment(parentPost->name,std::string(postCommentTextBuffer),token);
        }
        ImGui::SameLine();
        if(ImGui::Checkbox(postCommentPreviewCheckboxId.c_str(),&showingPostPreview))
        {
            postPreviewRenderer.SetText(postCommentTextBuffer);
        }
        if(showingPostPreview)
        {
            if(ImGui::BeginChild("Live Preview##commentLivePreview",postCommentPreviewSize,true))
            {
                postPreviewRenderer.RenderMarkdown();
                auto endPos = ImGui::GetCursorPos();
                postCommentPreviewSize.y = endPos.y + ImGui::GetTextLineHeight();

            }
            ImGui::EndChild();
        }
        ImGui::NewLine();
    }

    if(loadingInitialComments)
    {
        ImGui::Spinner("###spinner_loading_comments",50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }

    for(auto&& c : comments)
    {
        showComment(c);
        ImGui::Separator();
    }

    if(unloadedPostComments && ImGui::Button(moreRepliesButtonText.c_str()))
    {
        loadUnloadedChildren(unloadedPostComments);
        unloadedPostComments.reset();
        loadingUnloadedReplies = true;
    }
    else if(loadingUnloadedReplies)
    {
        ImGui::Spinner(loadingSpinnerIdText.c_str(),ImGui::GetFontSize() * 0.75f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }

    ImGui::End();
}
void CommentsWindow::setFocused()
{
    willBeFocused = true;
}
