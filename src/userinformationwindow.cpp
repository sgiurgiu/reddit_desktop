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
    constexpr auto UNREAD_MESSAGES = "unread";
    constexpr auto INBOX_MESSAGES = "inbox";
    constexpr auto SENT_MESSAGES = "sent";
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
        createCommentConnection = client->makeRedditCommentClientConnection();
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
                                  if(response.userData.has_value() && response.userData.type() == typeid(ParentMessageResponseData))
                                  {
                                      boost::asio::post(self->uiExecutor,
                                                        std::bind(&UserInformationWindow::loadMessageResponse,
                                                        self,std::move(msgResponse),
                                                        std::any_cast<ParentMessageResponseData>(response.userData)));
                                  }
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
               else if(response.userData.has_value() && response.userData.type() == typeid(MarkMessagesType))
               {

                    boost::asio::post(self->uiExecutor,
                                    std::bind(&UserInformationWindow::setMessagesRead,
                                    self,std::any_cast<MarkMessagesType>(response.userData)));
               }
           }
       });
    }

    loadMoreUnreadMessages();
    loadMoreAllMessages();
    loadMoreSentMessages();
}
void UserInformationWindow::setMessagesRead(MarkMessagesType markedMessages)
{
    for(const auto& messageId : markedMessages.first)
    {
        auto msg = getMessage(messageId.first,messageId.second);
        if(msg)
        {
            msg->msg.isNew = !markedMessages.second;
            msg->updateButtonsText();
        }
    }
}
void UserInformationWindow::loadMoreUnreadMessages()
{
    loadMessages(UNREAD_MESSAGES);
}
void UserInformationWindow::loadMoreAllMessages()
{
    loadMessages(INBOX_MESSAGES);
}
void UserInformationWindow::loadMoreSentMessages()
{
    loadMessages(SENT_MESSAGES);
}
void UserInformationWindow::loadMessages(const std::string& kind)
{
    std::string url = "/message/"+kind+"?count="+std::to_string(messages[kind].count);
    if(!messages[kind].after.empty())
    {
        url+= "&after="+messages[kind].after;
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
                    else if(response.userData.has_value() && response.userData.type() == typeid(std::string))
                    {
                        boost::asio::post(self->uiExecutor,std::bind(&UserInformationWindow::loadListingsFromConnection,
                                                                     self,std::move(response.data),
                                                                     std::any_cast<std::string>(response.userData)));
                    }
            }
        });
    }
    listingConnection->list(url,token,kind);
}
void UserInformationWindow::loadListingsFromConnection(listing listingResponse,std::string messagesKind)
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
                    messages[messagesKind].messages.emplace_back(message{child["data"], kind});
                }
            }
            messages[messagesKind].count = messages[messagesKind].messages.size();
            if(messages[messagesKind].count > 0)
            {
                messages[messagesKind].before = messages[messagesKind].messages.front().msg.name;
                messages[messagesKind].after = messages[messagesKind].messages.back().msg.name;
            }
        }
    }
}
void UserInformationWindow::loadMessageResponse(nlohmann::json response,ParentMessageResponseData parentMsg)
{
    std::string kind;
    if(response.contains("kind") && response["kind"].is_string())
    {
        kind = response["kind"].get<std::string>();
    }
    if(response.contains("data") && response["data"].is_object())
    {
        message msg{response["data"], kind};
        auto parent = getMessage(parentMsg.first,parentMsg.second);
        if(parent)
        {
            parent->replies.emplace(parent->replies.begin(),std::move(msg));
        }
    }
}
UserInformationWindow::DisplayMessage* UserInformationWindow::getMessage(const std::string& kind,const std::string& name)
{
    for(auto&& msg: messages.at(kind).messages)
    {
        if(msg.msg.name == name) return &msg;
        auto child = getChildMessage(msg,name);
        if(child) return child;
    }
    return nullptr;
}
UserInformationWindow::DisplayMessage* UserInformationWindow::getChildMessage(DisplayMessage& msg, const std::string& name)
{
    for(auto&& child: msg.replies)
    {
        if(child.msg.name == name) return &child;
        auto subChild = getChildMessage(child,name);
        if(subChild) return subChild;
    }
    return nullptr;
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
    contextButtonText = fmt::format("context##{}_commentContext",msg.name);
    markReadUnreadButtonText = fmt::format("{}##{}_commentMarkReadUnread",(msg.isNew?"Mark Read":"Mark Unread"),msg.name);
#ifdef REDDIT_DESKTOP_DEBUG
    titleText = fmt::format("{} - {}{} ({})",msg.author, msg.subject,(msg.isNew?" (unread)":""),msg.name);
#else
    titleText = fmt::format("{} - {}{}",msg.author,msg.subject,(msg.isNew?" (unread)":""));
#endif
}
void UserInformationWindow::voteComment(const std::string& kind, DisplayMessage& c,Voted vote)
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

            if (ec)
            {
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::setErrorMessage, self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::setErrorMessage, self, std::move(response.body)));
            }
            else if(response.userData.has_value() && response.userData.type() == typeid(std::tuple<std::string, std::string, Voted>))
            {
                auto voted = std::any_cast<std::tuple<std::string, std::string, Voted>>(response.userData);
                boost::asio::post(self->uiExecutor, std::bind(&UserInformationWindow::updateCommentVote, self,
                                                              std::get<0>(voted), std::get<1>(voted),std::get<2>(voted)));
            }

        });
    }
    std::tuple<std::string, std::string, Voted> tuple(kind, c.msg.name, vote);
    commentVotingConnection->vote(c.msg.name,token,vote,tuple);
}
void UserInformationWindow::updateCommentVote(std::string kind, std::string msgName,Voted vote)
{
    auto msg = getMessage(kind,msgName);
    if(msg)
    {
        msg->msg.voted = vote;
    }
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
            if(messages[UNREAD_MESSAGES].count > 0 && ImGui::Button("Mark All Read"))
            {
                DisplayMessageIds messagesList;
                std::vector<std::string> ids;
                for(auto&& msg : messages[UNREAD_MESSAGES].messages)
                {
                    messagesList.push_back(std::make_pair<>(UNREAD_MESSAGES,msg.msg.name));
                    ids.push_back(msg.msg.name);
                }
                MarkMessagesType markedMessages{std::move(messagesList),true};
                markReplyReadConnection->markReplyRead(ids,token,true,markedMessages);
            }
            for(auto&& msg : messages[UNREAD_MESSAGES].messages)
            {
                showMessage(msg, UNREAD_MESSAGES);
                ImGui::Separator();
            }
            if(!messages[UNREAD_MESSAGES].messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreUnreadMessages();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inbox"))
        {
            for(auto&& msg : messages[INBOX_MESSAGES].messages)
            {
                showMessage(msg, INBOX_MESSAGES);
                ImGui::Separator();
            }
            if(!messages[INBOX_MESSAGES].messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreAllMessages();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sent"))
        {
            for(auto&& msg : messages[SENT_MESSAGES].messages)
            {
                showMessage(msg, SENT_MESSAGES);
                ImGui::Separator();
            }
            if(!messages[SENT_MESSAGES].messages.empty() && ImGui::Button("Load More"))
            {
                loadMoreSentMessages();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void UserInformationWindow::showMessage(DisplayMessage& msg, const std::string& kind)
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
            voteComment(kind,msg,Voted::UpVoted);
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
            voteComment(kind, msg,Voted::DownVoted);
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

        if(!msg.msg.context.empty())
        {
            ImGui::SameLine();
            if(ImGui::Button(msg.contextButtonText.c_str()))
            {
                contextSignal(msg.msg.context);
            }
        }

        ImGui::SameLine();
        if(ImGui::Button(msg.markReadUnreadButtonText.c_str()))
        {
            DisplayMessageIds messagesList;
            messagesList.push_back(std::make_pair<>(kind,msg.msg.name));
            MarkMessagesType markedMessages{std::move(messagesList),msg.msg.isNew};
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
                ParentMessageResponseData responseData = std::make_pair<>(kind,msg.msg.name);
                createCommentConnection->createComment(msg.msg.name,msg.postReplyTextBuffer,token,std::move(responseData));
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
            showMessage(reply, kind);
        }
        ImGui::TreePop();
    }
}
