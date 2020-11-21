#include "redditdesktop.h"
#include "imgui.h"
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "imgui_internal.h"

#include <algorithm>

namespace
{
constexpr auto OPENSUBREDDIT_WINDOW_POPUP_TITLE = "Open Subreddit";
constexpr auto ERROR_WINDOW_POPUP_TITLE = "Error Occurred";
}

RedditDesktop::RedditDesktop(const boost::asio::io_context::executor_type& executor):uiExecutor(executor),
    client("api.reddit.com","oauth.reddit.com",3),
    loginConnection(client.makeLoginClientConnection()),loginWindow(&client,uiExecutor)
{
    current_user = db.getRegisteredUser();
    if(!current_user)
    {
        loginWindow.setShowLoginWindow(true);
    }
    else
    {
        loginConnection->connectionCompleteHandler([this](const boost::system::error_code& ec,const client_response<access_token>& token){
            if(ec)
            {
                boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,ec.message(),token));
            }
            else
            {
                if(token.status >= 400)
                {
                    boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,this,token.body,token));
                }
                else
                {
                    boost::asio::post(this->uiExecutor,std::bind(&RedditDesktop::addSubredditWindow,this,"",token));
                }
            }
        });
        client.setUserAgent(make_user_agent(current_user.value()));
        loginConnection->login(current_user.value());
    }
}
void RedditDesktop::addSubredditWindow(std::string title, client_response<access_token> token)
{
    current_access_token = token;
    subredditWindows.push_back(std::make_unique<SubredditWindow>(windowsCount++,title,current_access_token.data,&client,this->uiExecutor));
}
void RedditDesktop::setConnectionErrorMessage(std::string msg,client_response<access_token> token)
{
    current_access_token = token;
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
    for(auto&& win : subredditWindows)
    {
        if(win->isWindowOpen())
        {
            win->showWindow(appFrameWidth,appFrameHeight);
        }
    }

    auto startRemove = std::remove_if(subredditWindows.begin(),subredditWindows.end(),[](const auto& win) {
        return !win->isWindowOpen();
    });
    subredditWindows.erase(startRemove,subredditWindows.end());

    showOpenSubredditWindow();

    if(loginWindow.showLoginWindow())
    {
        auto user = loginWindow.getConfiguredUser();
        auto token = loginWindow.getAccessToken();
        db.setRegisteredUser(user);
        client.setUserAgent(make_user_agent(user));
    }
    showErrorDialog();

    ImGui::PopFont();
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
        if (ImGui::BeginMenu("File"))
        {
            showMenuFile();
            ImGui::EndMenu();
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
        ImGui::Text("Enter new directory to scan\n");
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
            addSubredditWindow(selectedSubreddit,current_access_token);
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
