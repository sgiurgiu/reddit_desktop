#include "redditdesktop.h"
#include "imgui.h"
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "imgui_internal.h"
#include <fmt/format.h>
#include <algorithm>

namespace
{
constexpr auto OPENSUBREDDIT_WINDOW_POPUP_TITLE = "Open Subreddit";
constexpr auto ERROR_WINDOW_POPUP_TITLE = "Error Occurred";
constexpr auto SUBREDDITS_WINDOW_TITLE = "My Subreddits";
}

RedditDesktop::RedditDesktop(const boost::asio::io_context::executor_type& executor,
                             Database* const db):uiExecutor(executor),
    client("api.reddit.com","oauth.reddit.com",3),db(db),loginWindow(&client,uiExecutor)
{
    assert(db != nullptr);
    current_user = db->getRegisteredUser();
    if(!current_user)
    {
        loginWindow.setShowLoginWindow(true);
    }
    else
    {
        auto loginConnection = client.makeLoginClientConnection();
        loginConnection->connectionCompleteHandler([this](const boost::system::error_code& ec,const client_response<access_token>& token){
            if(ec)
            {
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,ec.message()));
            }
            else
            {
                if(token.status >= 400)
                {
                    boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,token.body));
                }
                else
                {
                    boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::loginSuccessful,this,token));
                }
            }
        });
        client.setUserAgent(make_user_agent(current_user.value()));
        loginConnection->login(current_user.value());
    }
}
void RedditDesktop::loginSuccessful(client_response<access_token> token)
{
    current_access_token = token;
    auto listingconnection = client.makeListingClientConnection();
    listingconnection->connectionCompleteHandler([this](const boost::system::error_code& ec,
                                                 const client_response<listing>& response)
     {
        if(ec)
        {
            boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,ec.message()));
        }
        else
        {
            if(response.status >= 400)
            {
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,response.body));
            }
            else
            {
                auto info = std::make_shared<user_info>(response.data.json);
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::loadUserInformation,this,info));
            }
        }
     });
    listingconnection->list("/api/v1/me",current_access_token.data);

    //add front page window
    addSubredditWindow("");
}
void RedditDesktop::loadUserInformation(user_info_ptr info)
{
    info_user = info;
    auto listingconnection = client.makeListingClientConnection();
    listingconnection->connectionCompleteHandler([this,listingconnection](const boost::system::error_code& ec,
                                                 const client_response<listing>& response)
     {
        if(ec)
        {
            boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,ec.message()));
        }
        else
        {
            if(response.status >= 400)
            {
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,response.body));
            }
            else
            {
                auto listingChildren = response.data.json["data"]["children"];
                subreddit_list srs;
                for(const auto& child: listingChildren)
                {
                    srs.emplace_back(std::make_shared<subreddit>(child["data"]));
                }
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::loadSubscribedSubreddits,this,std::move(srs),listingconnection));
            }
        }
     });
    listingconnection->list("/subreddits/mine/subscriber?show=all&limit=100",current_access_token.data);
}
void RedditDesktop::loadSubscribedSubreddits(subreddit_list srs,RedditClient::RedditListingClientConnection connection)
{
    if(!srs.empty())
    {
        auto url = fmt::format("/subreddits/mine/subscriber?show=all&limit=100&after={}&count={}",srs.back()->name,srs.size());
        connection->list(url,current_access_token.data);
    }
    std::move(srs.begin(), srs.end(), std::back_inserter(subscribedSubreddits));
    std::stable_sort(subscribedSubreddits.begin(),subscribedSubreddits.end(),[](const auto& lhs,const auto& rhs){
        return lhs->displayName < rhs->displayName;
    });
}
void RedditDesktop::addSubredditWindow(std::string title)
{    
    subredditWindows.push_back(std::make_unique<SubredditWindow>(windowsCount++,title,current_access_token.data,&client,uiExecutor));
    subredditWindows.back()->showCommentsListener([this](const std::string& postId,const std::string& title){
        auto it = std::find_if(commentsWindows.begin(),commentsWindows.end(),[&postId](const std::unique_ptr<CommentsWindow>& win){
            return win->getPostId() == postId;
        });
        if(it == commentsWindows.end())
        {
            commentsWindows.push_back(std::make_unique<CommentsWindow>(postId,title,current_access_token.data,&client,uiExecutor));
        }
        else
        {
            (*it)->setFocused();
        }
    });
}
void RedditDesktop::setConnectionErrorMessage(std::string msg)
{
    showConnectionErrorDialog = true;
    connectionErrorMessage = msg;
}
void RedditDesktop::showDesktop()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Medium)]);//Roboto medium
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Q)) && ImGui::GetIO().KeyCtrl)
    {
        shouldQuit = true;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_O)) && ImGui::GetIO().KeyCtrl)
    {
        openSubredditWindow = true;
    }
    showMainMenuBar();
    showSubredditsWindow();
    for(auto&& win : subredditWindows)
    {
        if(win->isWindowOpen())
        {
            win->showWindow(appFrameWidth,appFrameHeight);
        }
    }

    for(auto&& win : commentsWindows)
    {
        if(win->isWindowOpen())
        {
            win->showWindow(appFrameWidth,appFrameHeight);
        }
    }

    auto startRemoveSubredditWindows = std::remove_if(subredditWindows.begin(),subredditWindows.end(),[](const auto& win) {
        return !win->isWindowOpen();
    });
    subredditWindows.erase(startRemoveSubredditWindows,subredditWindows.end());
    auto startRemoveCommentsWindows = std::remove_if(commentsWindows.begin(),commentsWindows.end(),[](const auto& win) {
        return !win->isWindowOpen();
    });
    commentsWindows.erase(startRemoveCommentsWindows,commentsWindows.end());

    showOpenSubredditWindow();

    if(loginWindow.showLoginWindow())
    {
        auto user = loginWindow.getConfiguredUser();
        auto token = loginWindow.getAccessToken();
        db->setRegisteredUser(user);
        client.setUserAgent(make_user_agent(user));
    }
    showErrorDialog();

    ImGui::PopFont();
}
void RedditDesktop::showSubredditsWindow()
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.2f,appFrameHeight*0.6f),ImGuiCond_FirstUseEver);

    ImGui::SetNextWindowPos(ImVec2(0,topPosAfterMenuBar));
    if(!ImGui::Begin(SUBREDDITS_WINDOW_TITLE,nullptr,ImGuiWindowFlags_NoMove))
    {
        ImGui::End();
        return;
    }

    if(ImGui::Selectable("Front Page"))
    {
        addSubredditWindow("");
    }
    if(ImGui::Selectable("All"))
    {
        addSubredditWindow("r/all");
    }
    for(const auto& sr : subscribedSubreddits)
    {
        if(ImGui::Selectable(sr->displayName.c_str()))
        {
            addSubredditWindow(sr->displayNamePrefixed);
        }
    }


    ImGui::End();
}

void RedditDesktop::showErrorDialog()
{
    if(showConnectionErrorDialog)
    {
        ImGui::OpenPopup(ERROR_WINDOW_POPUP_TITLE);
        showConnectionErrorDialog = false;
    }
    if(ImGui::BeginPopupModal(ERROR_WINDOW_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(connectionErrorMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"Unkown error occurred");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"%s",connectionErrorMessage.c_str());
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RedditDesktop::showMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        topPosAfterMenuBar = ImGui::GetWindowSize().y;
        if (ImGui::BeginMenu("File"))
        {
            showMenuFile();
            ImGui::EndMenu();
        }
        if(info_user)
        {
            const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
            static float infoButtonWidth = 0.0f;
            float pos = infoButtonWidth + itemSpacing;
            auto displayed_info = fmt::format("{} ({}-{})",info_user->name,info_user->humanLinkKarma,info_user->humanCommentKarma);
            ImGui::SameLine(ImGui::GetWindowWidth()-pos);
            if(ImGui::Button(displayed_info.c_str()))
            {

            }
            infoButtonWidth = ImGui::GetItemRectSize().x;
        }
        ImGui::EndMainMenuBar();
    }
}

void RedditDesktop::showMenuFile()
{
    if (ImGui::MenuItem(reinterpret_cast<const char*>(ICON_FA_SEARCH " Open Subreddit"), "Ctrl+O"))
    {
        openSubredditWindow = true;
    }

    if (ImGui::BeginMenu("Open Recent"))
    {
        ImGui::MenuItem("fish_hat.h");
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(reinterpret_cast<const char*>(ICON_FA_POWER_OFF " Quit"), "Ctrl+Q"))
    {
        shouldQuit = true;
    }
}

void RedditDesktop::showOpenSubredditWindow()
{
    if(openSubredditWindow)
    {
        ImGui::OpenPopup(OPENSUBREDDIT_WINDOW_POPUP_TITLE);
        openSubredditWindow = false;
    }
    //ImGui::SetNextWindowSizeConstraints(ImVec2(0, -1),    ImVec2(FLT_MAX, -1));//horizontal
    if (ImGui::BeginPopupModal(OPENSUBREDDIT_WINDOW_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter subreddit name to open");
        char dummy_text[50] = { 0 };
        memset(dummy_text,'X',sizeof(dummy_text)-1);
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()
                && !ImGui::IsMouseClicked(0))
        {
           ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::PushItemWidth(ImGui::CalcTextSize(dummy_text).x);
        ImGui::InputText("##opening_subreddit",selectedSubreddit,IM_ARRAYSIZE(selectedSubreddit),
                         ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();
        ImGui::Separator();
        bool okDisabled = std::string_view(selectedSubreddit).empty();
        if(okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
        {
            addSubredditWindow(selectedSubreddit);
            ImGui::CloseCurrentPopup();
        }
        if (okDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
