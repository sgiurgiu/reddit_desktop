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
    void loadMoreUnreadMessages();
    void loadMoreAllMessages();
    void loadMoreSentMessages();
    void loadMessages(const std::string& kind,Messages* messages);
    void setErrorMessage(std::string errorMessage);
    void loadListingsFromConnection(listing listingResponse,Messages* messages);
    void showMessage(DisplayMessage& msg);
    void loadMessageResponse(nlohmann::json);
private:
    bool showWindow = false;
    access_token token;
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    Messages unreadMessages;
    Messages allMessages;
    Messages sentMessages;
    std::string listingErrorMessage;
    std::vector<DisplayMessage*> loadingMoreRepliesComments;
    RedditClientProducer::RedditCreateCommentClientConnection createCommentConnection;
    RedditClientProducer::RedditListingClientConnection listingConnection;
};

#endif // USERINFORMATIONWINDOW_H
