#include "commentswindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "markdownrenderer.h"
#include "macros.h"
#include <iostream>
#include "database.h"
#include "spinner/spinner.h"
#include "resizableinputtextmultiline.h"

namespace
{
    constexpr ImU32 commentsBarColors[] = {
        IM_COL32(255,1,0,255), // red,
        IM_COL32(3,206,3,255), // green,
        IM_COL32(255,116,0,255), // orange,
        IM_COL32(0,153,154,255), // some teal ... something,
        IM_COL32(255,171,1,255), // red orange,
        IM_COL32(18,65,170,255), // some blue-purple,
        IM_COL32(254,213,2,255), // yellow-orage,
        IM_COL32(57,20,178,255), // blue-purple,
        IM_COL32(254,255,2,255), // yellow,
        IM_COL32(115,9,174,255), // purple,
        IM_COL32(159,238,2,255), // yellow-green,
        IM_COL32(209,2,117,255), // red-purple,
    };
}

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

CommentsWindow::CommentsWindow(const std::string& commentContext,
                               const access_token& token,
                               RedditClientProducer* client,
                               const boost::asio::any_io_executor& executor):
    commentContext(commentContext),token(token),client(client),
    uiExecutor(executor)
{
    windowName = fmt::format("{}##{}",commentContext,commentContext);
}

void CommentsWindow::setupListingConnections()
{
    if(!moreChildrenConnection)
    {
        moreChildrenConnection = client->makeRedditMoreChildrenClientConnection();
        moreChildrenConnection->connectionCompleteHandler(
          [weak=weak_from_this()](const boost::system::error_code& ec,
                                  client_response<listing> response)
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
                      boost::asio::post(self->uiExecutor,
                                    std::bind(&CommentsWindow::loadMoreChildrenListing,self,
                                              std::move(response.data),std::move(response.userData)));
                  }
              }
          });
    }
    if(!listingConnection)
    {
        listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler(
            [weak=weak_from_this()](const boost::system::error_code& ec,
                                             client_response<listing> response)
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
                     self->loadListingsFromConnection(std::move(response.data));
                 }
             }
        });
    }
    if(!createCommentConnection)
    {
        createCommentConnection = client->makeRedditCreateCommentClientConnection();
        createCommentConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                   client_response<listing> response)
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
                   boost::asio::post(self->uiExecutor,
                                 std::bind(&CommentsWindow::loadCommentReply,self,
                                           std::move(response.data),std::move(response.userData)));
               }
           }
       });
    }
}
void CommentsWindow::loadCommentReply(const listing& listingResponse,std::any userData)
{
    if(!windowOpen) return;
    comments_tuple receivedComments;

    if(listingResponse.json.contains("json") && listingResponse.json["json"].is_object())
    {
        const auto& respJson = listingResponse.json["json"];
        if(respJson.contains("data") && respJson["data"].is_object() &&
                respJson["data"].contains("things") && respJson["data"]["things"].is_array())
        {
           auto& things = respJson["data"]["things"];
           receivedComments = getJsonComments(things);
        }
    }

    if(userData.has_value() && userData.type() == typeid(CommentUserData))
    {
        CommentUserData commentData = std::any_cast<CommentUserData>(userData);
        auto c = getComment(std::move(commentData.commentName));
        if(c)
        {
            c->postingReplyInProgress = false;
            c->postReplyTextBuffer.clear();
            c->showingPreview = false;
            c->previewRenderer.SetText(c->postReplyTextBuffer);
            c->replyingToComment = false;
            c->updatingComment = false;
            auto replies = std::move(std::get<0>(receivedComments));
            if (commentData.isUpdating)
            {
                if (!replies.empty())
                {
                    c->commentData.body = replies.begin()->body;
                    c->renderer.SetText(c->commentData.body);
                }
            }
            else
            {
                
                for (auto&& reply : replies)
                {
                    c->replies.emplace(c->replies.begin(), std::move(reply), token, client, uiExecutor);
                }
            }
        }
    }
    else if(userData.has_value() && userData.type() == typeid(PostUserData))
    {
        showingPostPreview = false;
        postingComment = false;
        postCommentTextBuffer.clear();
        postPreviewRenderer.SetText(postCommentTextBuffer);
        auto replies = std::move(std::get<0>(receivedComments));
        for(auto&& reply : replies)
        {
            comments.emplace(comments.begin(),std::move(reply), token, client, uiExecutor);
        }
    }
}
void CommentsWindow::loadComments()
{    
    setupListingConnections();
    //depth=15&limit=100&threaded=true&
    if(commentContext.empty())
    {
        listingConnection->list("/comments/"+postId+"?sort=confidence",token);
    }
    else
    {
        listingConnection->list(commentContext,token);
    }
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
}
void CommentsWindow::loadMoreChildrenListing(const listing& listingResponse,std::any userData)
{
    if(!windowOpen) return;
    //morechildren is a broken API. Unbelievably broken. It kinda works, which is the worst kind of "works"
    comments_tuple receivedComments;
    if (listingResponse.json.is_object())
    {
        if(listingResponse.json.contains("json") &&
                listingResponse.json["json"].is_object() &&
                listingResponse.json["json"].contains("data") &&
                listingResponse.json["json"]["data"].contains("things") &&
                listingResponse.json["json"]["data"]["things"].is_array())
        {
            receivedComments = getJsonComments(listingResponse.json["json"]["data"]["things"]);
        }
    }

    if(userData.has_value() && userData.type() == typeid(CommentUserData))
    {
        CommentUserData commentData = std::any_cast<CommentUserData>(userData);
        auto c = getComment(std::move(commentData.commentName));
        if(c)
        {
            c->loadingUnloadedReplies = false;
            c->commentData.unloadedChildren = std::move(std::get<1>(receivedComments));
            auto replies = std::move(std::get<0>(receivedComments));
            for(auto&& reply : replies)
            {
                c->replies.emplace_back(std::move(reply), token, client, uiExecutor);
            }
        }
    }
    else if(userData.has_value() && userData.type() == typeid(PostUserData))
    {
        loadingUnloadedReplies = false;
        unloadedPostComments = std::move(std::get<1>(receivedComments));
        auto replies = std::move(std::get<0>(receivedComments));
        for(auto&& reply : replies)
        {
            comments.emplace_back(std::move(reply), token, client, uiExecutor);
        }
    }
}
CommentsWindow::comments_tuple CommentsWindow::getJsonComments(const nlohmann::json& children)
{
    //this method is wrong. TODO: fix this. Reddit has a really bad, awful "morechildren" response
    if(!windowOpen || !children.is_array()) return std::make_tuple<comments_list,std::optional<unloaded_children>>({},{});
    comments_list comments;
    std::optional<unloaded_children> unloadedPostComments;
    auto currentUser = Database::getInstance()->getLoggedInUser();
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
            comments.emplace_back(child["data"], currentUser);
        }
        else if (kind == "more")
        {
            unloadedPostComments = std::make_optional<unloaded_children>(child["data"]);
        }
    }
    return std::make_tuple<comments_list,std::optional<unloaded_children>>(std::move(comments),std::move(unloadedPostComments));
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse)
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
            loadListingChildren(child["data"]["children"]);
        }
    }
}
void CommentsWindow::loadListingChildren(const nlohmann::json& children)
{
    if(!windowOpen || !children.is_array()) return;
    comments_list comments;
    post_ptr pp;
    std::optional<unloaded_children> unloadedPostComments;
    auto currentUser = Database::getInstance()->getLoggedInUser();
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
            comments.emplace_back(child["data"], currentUser);
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
        boost::asio::post(uiExecutor,std::bind(&CommentsWindow::setComments,this,std::move(comments)));
    }
    if(pp)
    {
        boost::asio::post(uiExecutor,std::bind(&CommentsWindow::setParentPost,this,std::move(pp)));
    }
    if(unloadedPostComments)
    {
        boost::asio::post(uiExecutor,std::bind(&CommentsWindow::setUnloadedComments,this,std::move(unloadedPostComments)));
    }
}
void CommentsWindow::setUnloadedComments(std::optional<unloaded_children> children)
{
    if(!children || children->count <= 0 || children->children.empty()) return;
    unloadedPostComments = std::move(children);
    moreRepliesButtonText = fmt::format("load {} more comments##post_more_replies",unloadedPostComments->count);
    loadingSpinnerIdText = fmt::format("##spinner_loading{}",unloadedPostComments->id);
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    loadingInitialComments = false;
    listingErrorMessage.clear();
    if(receivedComments.empty()) return;

    std::for_each(receivedComments.begin(),receivedComments.end(),[this](auto& comment){
        comments.emplace_back(std::move(comment), token, client, uiExecutor);
    });
}
template<typename T>
void CommentsWindow::loadUnloadedChildren(const std::optional<unloaded_children>& children, T userData)
{
   setupListingConnections();
   if(!children) return;
   moreChildrenConnection->list(children.value(),parentPost->name,token,std::move(userData));
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
    postContentViewer->stopPlayingMedia();
    postUpvoteButtonText = fmt::format("{}##_up{}",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),parentPost->name);
    postDownvoteButtonText = fmt::format("{}##_down{}",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),parentPost->name);
    openLinkButtonText = fmt::format("{}##_openLink{}",
                                       reinterpret_cast<const char*>(ICON_FA_EXTERNAL_LINK_SQUARE " Open Link"),
                                       parentPost->name);
    commentButtonText = fmt::format("Save##{}_comment",parentPost->name);
    clearCommentButtonText = fmt::format("Clear##{}_comment",parentPost->name);
    postCommentTextFieldId = fmt::format("##{}_postComment",parentPost->name);
    postCommentPreviewCheckboxId = fmt::format("Show Preview##{}_postCommentPreview",parentPost->name);
}
void CommentsWindow::renderCommentContents(DisplayComment& c, int level)
{
    ImGui::Columns(2,nullptr,false);
    float barWidth = 5.f;
    ImGui::SetColumnWidth(0,barWidth+(ImGui::GetStyle().ItemSpacing.y * 2.f));
    ImVec2 barStartPosition = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
    ImVec2 barSize = ImVec2(barWidth,c.markdownHeight);   // Resize canvas to what's available
    ImVec2 barEndPosition = ImVec2(barStartPosition.x + barSize.x, barStartPosition.y + barSize.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if(level > 0)
    {
        draw_list->AddRectFilled(barStartPosition, barEndPosition, commentsBarColors[level % sizeof(commentsBarColors)]);
    }
    ImGui::NextColumn();
    auto markdownYPos = ImGui::GetCursorScreenPos().y;
    c.renderer.RenderMarkdown();
    c.markdownHeight = ImGui::GetCursorScreenPos().y - markdownYPos;
    ImGui::Columns(1);
}
void CommentsWindow::renderCommentActionButtons(DisplayComment& c)
{
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

   /* ImGui::SameLine();
    if(ImGui::Button(c.saveButtonText.c_str()))
    {

    }*/
    if(parentPost && !parentPost->locked)
    {
        ImGui::SameLine();
        if(ImGui::Button(c.replyButtonText.c_str()))
        {
            c.showingReplyArea = true;
            c.replyingToComment = true;
        }
        ImGui::SameLine();
        if(ImGui::Button(c.quoteButtonText.c_str()))
        {
            c.showingReplyArea = true;
            c.replyingToComment = true;
            if(c.postReplyTextBuffer.empty())
            {
                std::istringstream comment(c.commentData.body);
                std::string quotedText;
                for(std::string line; std::getline(comment,line);)
                {
                    quotedText += "> " + line + "\n";
                }
                c.postReplyTextBuffer = quotedText + "\n\n";
            }
        }
        if (c.commentData.isUsersComment)
        {
            ImGui::SameLine();
            if (ImGui::Button(c.editButtonText.c_str()))
            {
                c.showingReplyArea = true;
                c.updatingComment = true;
                if (c.postReplyTextBuffer.empty())
                {
                    c.postReplyTextBuffer = c.commentData.body;
                }
            }
        }
        if(c.showingReplyArea)
        {

            if(ResizableInputTextMultiline::InputText(c.replyIdText.c_str(),&c.postReplyTextBuffer,
                                      &c.postReplyTextFieldSize) && c.showingPreview)
            {
                c.previewRenderer.SetText(c.postReplyTextBuffer);
            }
            bool saveDisabled = (c.postingReplyInProgress || c.postReplyTextBuffer.empty());
            if(saveDisabled)
            {
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if(ImGui::Button(c.saveReplyButtonText.c_str()) && !c.postingReplyInProgress)
            {
                c.showingReplyArea = false;
                c.postingReplyInProgress = true;
                if (c.replyingToComment)
                {
                    createCommentConnection->createComment(c.commentData.name, c.postReplyTextBuffer, token, CommentUserData{ c.commentData.name, false });
                }
                else if (c.updatingComment)
                {
                    createCommentConnection->updateComment(c.commentData.name, c.postReplyTextBuffer, token, CommentUserData{ c.commentData.name, true });
                }
            }
            if(saveDisabled)
            {
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
            }
            ImGui::SameLine();
            if(ImGui::Button(c.cancelReplyButtonText.c_str()))
            {
                c.showingReplyArea = false;
                c.postReplyTextBuffer.clear();
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
    }
}

bool CommentsWindow::commentNode(DisplayComment& c)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImU32 id = window->GetID((void*)&c);
    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + g.FontSize + g.Style.FramePadding.y*2));
    bool opened = ImGui::TreeNodeBehaviorIsOpen(id, ImGuiTreeNodeFlags_DefaultOpen);
    bool hovered, held;
    if (ImGui::ButtonBehavior(bb, id, &hovered, &held, true))
        window->DC.StateStorage->SetInt(id, opened ? 0 : 1);
    if (hovered || held)
        window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered));

    ImGui::RenderText(ImVec2(pos.x, pos.y + g.Style.FramePadding.y * 2),
                      reinterpret_cast<const char*>( opened ? ICON_FA_CHEVRON_CIRCLE_DOWN :ICON_FA_CHEVRON_CIRCLE_RIGHT));

    float arrowSize = g.FontSize + g.Style.FramePadding.y * 2;
    ImVec2 authorPos(pos.x + arrowSize + g.Style.ItemInnerSpacing.x, pos.y + g.Style.FramePadding.y);
    ImVec2 scorePos(authorPos.x+c.authorNameTextSize.x+g.Style.ItemInnerSpacing.x, authorPos.y);
    if(c.commentData.isSubmitter)
    {
        ImRect surroundingRect(ImVec2(authorPos.x - g.Style.ItemInnerSpacing.x, authorPos.y),
                               ImVec2(authorPos.x + c.authorNameTextSize.x ,
                                      authorPos.y + g.FontSize));
        window->DrawList->AddRectFilled(surroundingRect.Min,surroundingRect.Max,ImGui::GetColorU32(ImGuiCol_HeaderActive),5.f);
        ImGui::RenderText(authorPos,c.commentData.author.c_str());
        scorePos.x += g.Style.ItemInnerSpacing.x;
    }
    else
    {
        ImGui::RenderText(authorPos,c.commentData.author.c_str());
    }

    ImGui::RenderText(scorePos,c.commentScoreText.c_str());
    ImVec2 timeDiffPos(scorePos.x+c.commentScoreTextSize.x, scorePos.y);
    ImGui::RenderText(timeDiffPos,c.commentData.humanReadableTimeDifference.c_str());

    ImVec2 awardsPos(timeDiffPos.x+c.commentTimeDiffTextSize.x+g.Style.ItemInnerSpacing.x, scorePos.y);
    float nextPos = c.awardsRenderer->RenderDirect(awardsPos);
#ifdef REDDIT_DESKTOP_DEBUG
    ImVec2 commentNamePos(nextPos+g.Style.ItemInnerSpacing.x + arrowSize, authorPos.y);
    ImGui::RenderText(commentNamePos,("("+c.commentData.name +")").c_str());
#endif

    ImGui::ItemSize(bb, g.Style.FramePadding.y);
    ImGui::ItemAdd(bb,id);

    if (opened)
        ImGui::TreePush((void*)&c);
    return opened;
}

void CommentsWindow::showComment(DisplayComment& c, int level)
{
    if(commentNode(c))
    {
        renderCommentContents(c,level);

        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));

        renderCommentActionButtons(c);

        for(auto&& reply : c.replies)
        {
            showComment(reply, level + 1);
        }
        if(c.commentData.unloadedChildren)
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
            if(ImGui::Button(c.moreRepliesButtonText.c_str()))
            {
                loadUnloadedChildren<CommentUserData>(c.commentData.unloadedChildren,{c.commentData.name});
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
    quoteButtonText = fmt::format("quote##{}_quote",commentData.name);
    editButtonText = fmt::format("edit##{}_edit", commentData.name);
    replyIdText = fmt::format("##{}comment_reply",commentData.name);
    saveReplyButtonText = fmt::format("Save##{}save_comment_reply",commentData.name);
    cancelReplyButtonText = fmt::format("Cancel##{}cancel_comment_reply",commentData.name);
    if(commentData.unloadedChildren)
    {
        moreRepliesButtonText = fmt::format("load {} more comments##{}_more_replies",commentData.unloadedChildren->count,commentData.name);
        spinnerIdText = fmt::format("##spinner_loading{}",commentData.unloadedChildren->id);
    }
    postReplyPreviewCheckboxId = fmt::format("Show Preview##{}_postReplyPreview",commentData.name);
    liveReplyPreviewText = fmt::format("Live Preview##{}_commentLivePreview",commentData.name);
    commentScoreText = " "+commentData.humanScore + " points, ";
    authorNameTextSize = ImGui::CalcTextSize(commentData.author.c_str());
    commentScoreTextSize = ImGui::CalcTextSize(commentScoreText.c_str());
    commentTimeDiffTextSize = ImGui::CalcTextSize(commentData.humanReadableTimeDifference.c_str());
#ifdef REDDIT_DESKTOP_DEBUG
    titleText = fmt::format("{}  {} points, {} ({})",commentData.author,
                            commentData.humanScore,commentData.humanReadableTimeDifference,
                            commentData.name);
#else
    titleText = fmt::format("{}  {} points, {}",commentData.author,
                            commentData.humanScore,commentData.humanReadableTimeDifference);
#endif
}
void CommentsWindow::voteParentPost(Voted vote)
{
    listingErrorMessage.clear();
    if(!postVotingConnection)
    {
        postVotingConnection = client->makeRedditVoteClientConnection();
        postVotingConnection->connectionCompleteHandler(
                    [weak=weak_from_this()](const boost::system::error_code& ec,
                                            client_response<std::string> response)
        {
            auto self = weak.lock();
            if(!self) return;
            Voted voted = std::any_cast<Voted>(response.userData);

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
                boost::asio::post(self->uiExecutor,std::bind(&CommentsWindow::updatePostVote,self, voted));
            }
        });
    }
    postVotingConnection->vote(parentPost->name,token,vote, vote);
}
void CommentsWindow::updatePostVote(Voted vote)
{
    auto oldVoted = parentPost->voted;
    parentPost->voted = vote;
    parentPost->humanScore = Utils::CalculateScore(parentPost->score,oldVoted,vote);
}
void CommentsWindow::voteComment(DisplayComment* c,Voted vote)
{
    listingErrorMessage.clear();
    if(!commentVotingConnection)
    {
        commentVotingConnection = client->makeRedditVoteClientConnection();
        commentVotingConnection->connectionCompleteHandler(
            [weak = weak_from_this()](const boost::system::error_code& ec,
                client_response<std::string> response)
        {
            auto self = weak.lock();
            if (!self) return;
            auto p = std::any_cast<std::pair<std::string, Voted>>(response.userData);
            if (ec)
            {
                boost::asio::post(self->uiExecutor, std::bind(&CommentsWindow::setErrorMessage, self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor, std::bind(&CommentsWindow::setErrorMessage, self, std::move(response.body)));
            }
            else
            {
                boost::asio::post(self->uiExecutor, std::bind(&CommentsWindow::updateCommentVote, self, std::move(p.first), p.second));
            }
        });
    }
    auto pair = std::make_pair<>(c->commentData.name, vote);
    commentVotingConnection->vote(c->commentData.name,token,vote,pair);
}
void CommentsWindow::updateCommentVote(std::string commentName,Voted vote)
{
    auto c = getComment(std::move(commentName));
    if(c)
    {
        auto oldVoted = c->commentData.voted;
        c->commentData.voted = vote;
        c->commentData.humanScore = Utils::CalculateScore(c->commentData.score,oldVoted,vote);
        c->updateButtonsText();
    }
}
CommentsWindow::DisplayComment* CommentsWindow::getComment(std::string commentName)
{
    for(auto&& c : comments)
    {
        if(c.commentData.name == commentName) return &c;
        auto child = getChildComment(c,commentName);
        if(child) return child;
    }

    return nullptr;
}
CommentsWindow::DisplayComment* CommentsWindow::getChildComment(DisplayComment& c,const std::string& commentName)
{
    for(auto&& child : c.replies)
    {
        if(child.commentData.name == commentName) return &child;
        auto subChild = getChildComment(child,commentName);
        if(subChild) return subChild;
    }
    return nullptr;
}

void CommentsWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_None))
    {
        if(postContentViewer) {
            postContentViewer->stopPlayingMedia();
            postContentViewer.reset();
        }
        ImGui::End();
        return;
    }
    if(!windowOpen)
    {
        if(postContentViewer) {
            postContentViewer->stopPlayingMedia();
            postContentViewer.reset();
        }
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
    if(windowPositionAndSizeSet)
    {
        windowPositionAndSizeSet = false;
        ImGui::SetWindowPos(windowPos);
        ImGui::SetWindowSize(windowSize);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)) && ImGui::IsWindowFocused())
    {
        comments.clear();
        loadComments();
    }

    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    if(parentPost)
    {
        if(parentPost->locked)
        {
            ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "Post is locked, you cannot comment.");
        }

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
        ImGui::TextWrapped("%s",parentPost->title.c_str());
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("Posted by %s %s",parentPost->author.c_str(),parentPost->humanReadableTimeDifference.c_str());
        ImGui::PopFont();
        ImGui::NewLine();

        if(postContentViewer)
        {
            postContentViewer->showPostContent();
        }
        else
        {
            postContentViewer = std::make_shared<PostContentViewer>(client,uiExecutor);
            postContentViewer->loadContent(parentPost);
            postContentViewer->stopPlayingMedia();
        }

        if(parentPost->voted == Voted::UpVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetUpVoteColor());
        }
        if(ImGui::Button(postUpvoteButtonText.c_str()))
        {
            voteParentPost(parentPost->voted == Voted::UpVoted ? Voted::NotVoted : Voted::UpVoted);
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
            voteParentPost(parentPost->voted == Voted::DownVoted ? Voted::NotVoted : Voted::DownVoted);
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
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(parentPost->url.c_str());
                ImGui::EndTooltip();
            }
        }
        if(!parentPost->subreddit.empty())
        {
            ImGui::SameLine();
            if(ImGui::Button("Subreddit##openSubredditWindow"))
            {
                openSubredditSignal(parentPost->subreddit);
            }
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(parentPost->subreddit.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::Separator();
        if(!parentPost->locked)
        {
            if(ResizableInputTextMultiline::InputText(postCommentTextFieldId.c_str(),&postCommentTextBuffer,
                                      &postCommentTextFieldSize) && showingPostPreview)
            {
                postPreviewRenderer.SetText(postCommentTextBuffer);
            }
            bool saveDisabled = (postingComment || postCommentTextBuffer.empty());
            if(saveDisabled)
            {
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if(ImGui::Button(commentButtonText.c_str()) && !postingComment && !postCommentTextBuffer.empty())
            {
                postingComment = true;
                createCommentConnection->createComment(parentPost->name,postCommentTextBuffer,token, PostUserData());
            }
            ImGui::SameLine();
            if(ImGui::Button(clearCommentButtonText.c_str()) && !postingComment && !postCommentTextBuffer.empty())
            {
                postCommentTextBuffer.clear();
            }
            if(saveDisabled)
            {
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
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
        loadingUnloadedReplies = true;
        loadUnloadedChildren<PostUserData>(unloadedPostComments,{});
        unloadedPostComments.reset();
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
