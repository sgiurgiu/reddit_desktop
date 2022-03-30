#ifndef USERINFORMATIONWINDOW_H
#define USERINFORMATIONWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclientproducer.h"
#include "markdownrenderer.h"
#include <imgui.h>
#include <map>

class UserInformationWindow : public std::enable_shared_from_this<UserInformationWindow>
{
public:
    UserInformationWindow(const access_token& token,
                          RedditClientProducer* client,
                          const boost::asio::any_io_executor& executor);
    void showUserInfoWindow(int appFrameWidth,int appFrameHeight);
    void shouldShowWindow(bool flag)
    {
        showWindow = flag;
    }
    bool isWindowShowing() const
    {
        return showWindow;
    }
    void loadMessages();
    template<typename S>
    void showContextHandler(S slot)
    {
        contextSignal.connect(slot);
    }
private:
    struct DisplayMessage;
    using DisplayMessageList = std::vector<DisplayMessage>;
    struct DisplayMessage
    {
        DisplayMessage(message message):
            msg(std::move(message)),renderer(this->msg.body)
        {
            updateButtonsText();
        }
        message msg;
        MarkdownRenderer renderer;
        MarkdownRenderer previewRenderer;
        std::string upvoteButtonText;
        std::string downvoteButtonText;
        std::string saveButtonText;
        std::string replyButtonText;
        std::string moreRepliesButtonText;
        std::string spinnerIdText;
        std::string replyIdText;
        std::string saveReplyButtonText;
        std::string titleText;
        std::string postReplyPreviewCheckboxId;
        std::string liveReplyPreviewText;
        std::string markReadUnreadButtonText;
        std::string contextButtonText;
        bool loadingUnloadedReplies = false;
        std::string postReplyTextBuffer;
        bool postingReply = false;
        bool showingReplyArea = false;
        bool showingPreview = false;
        ImVec2 postReplyTextFieldSize;
        ImVec2 postReplyPreviewSize = {0,1};
        DisplayMessageList replies;
        void updateButtonsText();
    };    
    struct Messages
    {
        std::string after;
        std::string before;
        int count = 0;
        DisplayMessageList messages;
    };
    using ParentMessageResponseData = std::pair<std::string,std::string>;
    void loadMoreUnreadMessages();
    void loadMoreAllMessages();
    void loadMoreSentMessages();
    void loadMessages(const std::string& kind);
    void setErrorMessage(std::string errorMessage);
    void loadListingsFromConnection(listing listingResponse,std::string messagesKind);
    void showMessage(DisplayMessage& msg, const std::string& kind);
    void loadMessageResponse(nlohmann::json,ParentMessageResponseData parentMsg);
    using DisplayMessageIds = std::vector<ParentMessageResponseData>;
    using MarkMessagesType = std::pair<DisplayMessageIds,bool>;
    void setMessagesRead(MarkMessagesType);
    void updateCommentVote(std::string kind, std::string msgName,Voted vote);
    void voteComment(const std::string& kind, DisplayMessage& c,Voted vote);
    DisplayMessage* getMessage(const std::string& kind,const std::string& name);
    DisplayMessage* getChildMessage(DisplayMessage& msg, const std::string& name);
private:
    bool showWindow = false;
    access_token token;
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    std::map<std::string,Messages> messages;
    std::string listingErrorMessage;
    RedditClientProducer::RedditCommentClientConnection createCommentConnection;
    RedditClientProducer::RedditListingClientConnection listingConnection;
    RedditClientProducer::RedditMarkReplyReadClientConnection markReplyReadConnection;
    RedditClientProducer::RedditVoteClientConnection commentVotingConnection;
    boost::signals2::signal<void(const std::string&)> contextSignal;

};

#endif // USERINFORMATIONWINDOW_H
