#include "userinformationwindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "resizableinputtextmultiline.h"
#include <iostream>

namespace
{
    constexpr auto USER_INFO_WINDOW_TITLE = "Messages and information";
}

UserInformationWindow::UserInformationWindow(const access_token& token,
                                             RedditClientProducer* client,
                                             const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor)
{
}
void UserInformationWindow::loadMessages()
{
    if(!createCommentConnection)
    {
        createCommentConnection = client->makeRedditCreateCommentClientConnection();
        createCommentConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                   client_response<listing> response)
       {
           auto self = weak.lock();
           if(!self || !self->showWindow) return;
           if(ec)
           {
               boost::asio::post(self->uiExecutor,
                        std::bind(&UserInformationWindow::setErrorMessage,self,ec.message()));
           }
           else
           {
               if(response.status >= 400)
               {
                   boost::asio::post(self->uiExecutor,
                                 std::bind(&UserInformationWindow::setErrorMessage,self,response.body));
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
                          if(things.is_array())
                          {
                              for(const auto& msgResponse : things)
                              {
                                  boost::asio::post(self->uiExecutor,
                                                    std::bind(&UserInformationWindow::loadMessageResponse,
                                                    self,std::move(msgResponse),(DisplayMessage*)response.userData));
                              }
                          }
                       }
                   }
               }
           }
       });
    }
    if(!markReplyReadConnection)
    {
        markReplyReadConnection = client->makeRedditMarkReplyReadClientConnection();
        markReplyReadConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                   client_response<std::string> response)
       {
           auto self = weak.lock();
           if(!self || !self->showWindow) return;
           if(ec)
           {
               boost::asio::post(self->uiExecutor,
                        std::bind(&UserInformationWindow::setErrorMessage,self,ec.message()));
           }
           else
           {
               if(response.status >= 400)
               {
                   boost::asio::post(self->uiExecutor,
                                 std::bind(&UserInformationWindow::setErrorMessage,self,response.body));
               }
               else
               {

                    boost::asio::post(self->uiExecutor,
                                    std::bind(&UserInformationWindow::setMessagesRead,
                                    self,(MarkMessagesType*)response.userData));
               }
           }
       });
    }

    loadMoreUnreadMessages();
    loadMoreAllMessages();
    loadMoreSentMessages();
}
void UserInformationWindow::setMessagesRead(MarkMessagesType* markedMessages)
{
    for(const auto& message : markedMessages->first)
    {
        message->msg.isNew = !markedMessages->second;
        message->updateButtonsText();
    }
    delete markedMessages;
}
void UserInformationWindow::loadMoreUnreadMessages()
{
    loadMessages("unread",&unreadMessages);
}
void UserInformationWindow::loadMoreAllMessages()
{
    loadMessages("inbox",&allMessages);
}
void UserInformationWindow::loadMoreSentMessages()
{
    loadMessages("sent",&sentMessages);
}
void UserInformationWindow::loadMessages(const std::string& kind,Messages* messages)
{
    std::string url = "/message/"+kind+"?count="+std::to_string(messages->count);
    if(!messages->after.empty())
    {
        url+= "&after="+messages->after;
    }
    if(!listingConnection)
    {
        listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler([self=shared_from_this()](const boost::system::error_code& ec,
                                              client_response<listing> response)
            {
                if(ec)
                {
                    boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::setErrorMessage,self,ec.message()));
                }
                else
                {
                    if(response.status >= 400)
                    {
                        boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::setErrorMessage,self,std::move(response.body)));
                    }
                    else
                    {
                        boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::loadListingsFromConnection,
                                                                     self,std::move(response.data),(Messages*)response.userData));
                    }
            }
        });
    }
    listingConnection->list(url,token,messages);
}
void UserInformationWindow::loadListingsFromConnection(listing listingResponse,Messages* messages)
{
    std::string kind;
    if(listingResponse.json.contains("kind") && listingResponse.json["kind"].is_string())
    {
        kind = listingResponse.json["kind"].get<std::string>();
    }
    if(kind != "Listing") return;
    if(listingResponse.json.contains("data") && listingResponse.json["data"].is_object())
    {
        const auto& data = listingResponse.json["data"];
        if(data.contains("children") && data["children"].is_array())
        {
            for(const auto& child : data["children"])
            {
                auto kind = child["kind"].get<std::string>();
                if (kind == "t4" || kind == "t1")
                {
                    messages->messages.emplace_back(message{child["data"], kind});
                }
            }
            messages->count = messages->messages.size();
            if(messages->count > 0)
            {
                messages->before = messages->messages.front().msg.name;
                messages->after = messages->messages.back().msg.name;
            }
        }
    }
}
void UserInformationWindow::loadMessageResponse(nlohmann::json response,DisplayMessage* parentMessage)
{
    std::string kind;
    if(response.contains("kind") && response["kind"].is_string())
    {
        kind = response["kind"].get<std::string>();
    }
    if(response.contains("data") && response["data"].is_object())
    {
        message msg{response["data"], kind};
        parentMessage->replies.emplace(parentMessage->replies.begin(),std::move(msg));
    }
}
void UserInformationWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = std::move(errorMessage);
}
void UserInformationWindow::DisplayMessage::updateButtonsText()
{
    upvoteButtonText = fmt::format("{}##{}_up",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),msg.name);
    downvoteButtonText = fmt::format("{}##{}_down",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),msg.name);
    saveButtonText = fmt::format("save##{}_save",msg.name);
    replyButtonText = fmt::format("reply##{}_reply",msg.name);
    replyIdText = fmt::format("##{}comment_reply",msg.name);
    saveReplyButtonText = fmt::format("Save##{}save_comment_reply",msg.name);
    postReplyPreviewCheckboxId = fmt::format("Show Preview##{}_postReplyPreview",msg.name);
    liveReplyPreviewText = fmt::format("Live Preview##{}_commentLivePreview",msg.name);
    markReadUnreadButtonText = fmt::format("{}##{}_commentMarkReadUnread",(msg.isNew?"Mark Read":"Mark Unread"),msg.name);
#ifdef REDDIT_DESKTOP_DEBUG
    titleText = fmt::format("{} - {}{} ({})",msg.author, msg.subject,(msg.isNew?" (unread)":""),msg.name);
#else
    titleText = fmt::format("{} - {}{}",msg.author,msg.subject,(msg.isNew?" (unread)":""));
#endif
}
void UserInformationWindow::voteComment(DisplayMessage* c,Voted vote)
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
            std::pair<DisplayMessage*, Voted>* p = static_cast<std::pair<DisplayMessage*, Voted>*>(response.userData);
            if (!p) return;

            if (ec)
            {
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::setErrorMessage, self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::setErrorMessage, self, std::move(response.body)));
            }
            else
            {
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::updateCommentVote, self, p->first, p->second));
            }
            delete p;
        });
    }
    std::pair<DisplayMessage*, Voted>* pair = new std::pair<DisplayMessage*, Voted>(c, vote);
    commentVotingConnection->vote(c->msg.name,token,vote,pair);
}
void UserInformationWindow::updateCommentVote(DisplayMessage* c,Voted vote)
{
    //auto oldVoted = c->msg.voted;
    c->msg.voted = vote;
    //c->updateButtonsText();
}
void UserInformationWindow::showUserInfoWindow(int appFrameWidth,int appFrameHeight)
{
    if(!showWindow) return;
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.5,appFrameHeight*0.5),ImGuiCond_FirstUseEver);
    if(!ImGui::Begin(USER_INFO_WINDOW_TITLE,&showWindow,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("MessagesTabBar", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Unread"))
        {
            if(unreadMessages.count > 0 && ImGui::Button("Mark All Read"))
            {
                DisplayMessagePtrList messagesPtrList;
                std::vector<std::string> ids;
                for(auto&& msg : unreadMessages.messages)
                {
                    messagesPtrList.push_back(&msg);
                    ids.push_back(msg.msg.name);
                }
                MarkMessagesType* markedMessages = new MarkMessagesType(std::move(messagesPtrList),true);
                markReplyReadConnection->markReplyRead(ids,token,true,markedMessages);
            }
            for(auto&& msg : unreadMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            if(!unreadMessages.messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreUnreadMessages();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inbox"))
        {
            for(auto&& msg : allMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            if(!allMessages.messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreAllMessages();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sent"))
        {
            for(auto&& msg : sentMessages.messages)
            {
                showMessage(msg);
                ImGui::Separator();
            }
            if(!sentMessages.messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreSentMessages();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void UserInformationWindow::showMessage(DisplayMessage& msg)
{
    if(ImGui::TreeNodeEx(msg.titleText.c_str(),ImGuiTreeNodeFlags_DefaultOpen))
    {
        msg.renderer.RenderMarkdown();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
        if(msg.msg.voted == Voted::UpVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetUpVoteColor());
        }
        if(ImGui::Button(msg.upvoteButtonText.c_str()))
        {
            voteComment(&msg,Voted::UpVoted);
        }
        if(msg.msg.voted == Voted::UpVoted)
        {
            ImGui::PopStyleColor(1);
        }
        ImGui::SameLine();
        if(msg.msg.voted == Voted::DownVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetDownVoteColor());
        }
        if(ImGui::Button(msg.downvoteButtonText.c_str()))
        {
            voteComment(&msg,Voted::DownVoted);
        }
        if(msg.msg.voted == Voted::DownVoted)
        {
            ImGui::PopStyleColor(1);
        }

        ImGui::SameLine();
        if(ImGui::Button(msg.replyButtonText.c_str()))
        {
            msg.showingReplyArea = !msg.showingReplyArea;
        }
        ImGui::SameLine();
        if(ImGui::Button(msg.markReadUnreadButtonText.c_str()))
        {
            DisplayMessagePtrList messagesPtrList;
            messagesPtrList.push_back(&msg);
            MarkMessagesType* markedMessages = new MarkMessagesType(std::move(messagesPtrList),msg.msg.isNew);
            markReplyReadConnection->markReplyRead({msg.msg.name},token,msg.msg.isNew,markedMessages);
        }
        if(msg.showingReplyArea)
        {
            if(ResizableInputTextMultiline::InputText(msg.replyIdText.c_str(),&msg.postReplyTextBuffer,
                                      &msg.postReplyTextFieldSize) && msg.showingPreview)
            {
                msg.previewRenderer.SetText(msg.postReplyTextBuffer);
            }

            if(ImGui::Button(msg.saveReplyButtonText.c_str()) && !msg.postingReply)
            {
                msg.showingReplyArea = false;
                msg.postingReply = true;
                createCommentConnection->createComment(msg.msg.name,msg.postReplyTextBuffer,token,&msg);
            }
            ImGui::SameLine();
            if(ImGui::Checkbox(msg.postReplyPreviewCheckboxId.c_str(),&msg.showingPreview))
            {
                msg.previewRenderer.SetText(msg.postReplyTextBuffer);
            }
            if(msg.showingPreview)
            {
                if(ImGui::BeginChild(msg.liveReplyPreviewText.c_str(),msg.postReplyPreviewSize,true))
                {
                    msg.previewRenderer.RenderMarkdown();
                    auto endPos = ImGui::GetCursorPos();
                    msg.postReplyPreviewSize.y = endPos.y + ImGui::GetTextLineHeight();
                }
                ImGui::EndChild();
            }
        }
        for(auto&& reply : msg.replies)
        {
            showMessage(reply);
        }
        ImGui::TreePop();
    }
}
