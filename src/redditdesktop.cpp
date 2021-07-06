#include "redditdesktop.h"
#include "imgui.h"
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "imgui_internal.h"
#include <fmt/format.h>
#include <algorithm>
#include "database.h"
#include <iostream>
#include <boost/algorithm/string.hpp>

namespace
{
constexpr auto OPENSUBREDDIT_WINDOW_POPUP_TITLE = "Open Subreddit";
constexpr auto ERROR_WINDOW_POPUP_TITLE = "Error Occurred";
}

RedditDesktop::RedditDesktop(boost::asio::io_context& uiContext):
    uiExecutor(uiContext.get_executor()),
    client("api.reddit.com","oauth.reddit.com",3),
    loginWindow(&client,uiExecutor),loginTokenRefreshTimer(uiExecutor),
    loggingWindow(std::make_shared<LoggingWindow>(uiExecutor))
{
    auto db = Database::getInstance();
    registeredUsers = db->getRegisteredUsers();
    if(!registeredUsers.empty())
    {
        currentUser = registeredUsers.front();
    }
    shouldBlurPictures= db->getBlurNSFWPictures();
    useMediaHwAccel = db->getUseHWAccelerationForMedia();
    subredditsAutoRefreshTimeout = db->getAutoRefreshTimeout();
    showRandomNSFW = db->getShowRandomNSFW();
    automaticallyArangeWindowsInGrid = db->getAutoArangeWindowsGrid();

    loggingWindow->setupLogging();
    loggingWindow->setWindowOpen(showLoggingWindow);
}

void RedditDesktop::loginCurrentUser()
{
    if(!currentUser)
    {
        loginWindow.setShowLoginWindow(true);
    }
    else
    {
        refreshLoginToken();
    }
}
void RedditDesktop::refreshLoginToken()
{
    auto loginConnection = client.makeLoginClientConnection();
    loginConnection->connectionCompleteHandler([weak=weak_from_this()]
                                               (const boost::system::error_code& ec,
                                               client_response<access_token> token){
        auto self = weak.lock();
        if(!self) return;
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,ec.message()));
        }
        else
        {
            if(token.status >= 400)
            {
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,token.body));
            }
            else
            {
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::loginSuccessful,self,std::move(token)));
            }
        }
    });
    client.setUserAgent(make_user_agent(currentUser.value()));
    loginConnection->login(currentUser.value());
}

void RedditDesktop::loginSuccessful(client_response<access_token> token)
{
    Database::getInstance()->setLoggedInUser(currentUser.value());
    currentAccessToken = token;
    loginTokenRefreshTimer.expires_after(std::chrono::seconds(std::min(currentAccessToken.data.expires,10*60)));
    loginTokenRefreshTimer.async_wait([weak=weak_from_this()](const boost::system::error_code& ec){
        auto self = weak.lock();
        if(self && !ec)
        {
            self->refreshLoginToken();
        }
    });

    if(!subredditsListWindow)
    {
        subredditsListWindow = std::make_shared<SubredditsListWindow>(currentAccessToken.data,&client,uiExecutor);
        subredditsListWindow->showSubredditListener([weak=weak_from_this()](std::string str){
            auto self = weak.lock();
            if(!self) return;
            self->addSubredditWindow(str);
        });
        subredditsListWindow->setUserName(currentUser->username);
        subredditsListWindow->loadSubredditsList();
    }

    loadUserInformation();

    if(loggedInFirstTime)
    {
        //add front page window
        addSubredditWindow("");
        loggedInFirstTime = false;
    }
    else
    {
        updateWindowsTokenData();
    }
}
void RedditDesktop::loadUserInformation()
{
    auto userInformationConnection = client.makeListingClientConnection();
    userInformationConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                 client_response<listing> response)
     {
        auto self = weak.lock();
        if(!self) return;
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,ec.message()));
        }
        else
        {
            if(response.status >= 400)
            {
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,response.body));
            }
            else
            {
                user_info info(response.data.json);
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::updateUserInformation,self,std::move(info)));
            }
        }
     });
    userInformationConnection->list("/api/v1/me",currentAccessToken.data);
}
void RedditDesktop::updateUserInformation(user_info info)
{
    infoUser = std::move(info);
    if(infoUser)
    {
        userInfoDisplay = fmt::format("{} ({}-{})",infoUser->name,infoUser->humanLinkKarma,infoUser->humanCommentKarma);
        if(infoUser->hasMail)
        {
            userInfoDisplay.append(reinterpret_cast<const char*>(" " ICON_FA_ENVELOPE_O));
        }
        if(infoUser->hasModMail)
        {
            userInfoDisplay.append(reinterpret_cast<const char*>(" " ICON_FA_BELL));
        }
    }
}

void RedditDesktop::addSubredditWindow(std::string title)
{    
    auto it = std::find_if(subredditWindows.cbegin(),subredditWindows.cend(),
                           [&title](const std::shared_ptr<SubredditWindow>& win){
        return win->getSubreddit() == title || win->getTarget() == title;
    });
    if(it != subredditWindows.end())
    {
        (*it)->setFocused();
    }
    else
    {
        auto subredditWindow = std::make_shared<SubredditWindow>(windowsCount++,
                                                                 title,currentAccessToken.data,
                                                                 &client,uiExecutor);

        subredditWindow->showCommentsListener([this](std::string id, std::string title){
            boost::asio::post(this->uiExecutor, std::bind(&RedditDesktop::addCommentsWindow, this,std::move(id), std::move(title)));
        });
        subredditWindow->showSubredditListener([this](std::string title){
            boost::asio::post(this->uiExecutor, std::bind(&RedditDesktop::addSubredditWindow, this, std::move(title)));
        });
        subredditWindow->loadSubreddit();
        subredditWindow->setFocused();
        subredditWindows.push_back(std::move(subredditWindow));
        if(automaticallyArangeWindowsInGrid)
        {
            arrangeWindowsGrid();
        }
    }
}
void RedditDesktop::addCommentsWindow(std::string postId,std::string title)
{
    auto it = std::find_if(commentsWindows.cbegin(),commentsWindows.cend(),
                           [&postId](const std::shared_ptr<CommentsWindow>& win){
        return win->getPostId() == postId;
    });
    if(it == commentsWindows.end())
    {
        auto commentsWindow = std::make_shared<CommentsWindow>(postId,title,
                                                               currentAccessToken.data,
                                                               &client,uiExecutor);
        using namespace std::placeholders;
        commentsWindow->addOpenSubredditHandler([this](std::string title){
            boost::asio::post(this->uiExecutor, std::bind(&RedditDesktop::addSubredditWindow, this, std::move(title)));
        });
        commentsWindow->loadComments();
        commentsWindow->setFocused();
        commentsWindows.push_back(std::move(commentsWindow));
        if(automaticallyArangeWindowsInGrid)
        {
            arrangeWindowsGrid();
        }
    }
    else
    {
        (*it)->setFocused();
    }
}
void RedditDesktop::addMessageContextWindow(std::string context)
{
    auto commentsWindow = std::make_shared<CommentsWindow>(context,
                                                           currentAccessToken.data,
                                                           &client,uiExecutor);
    using namespace std::placeholders;
    commentsWindow->addOpenSubredditHandler([this](std::string title){
        boost::asio::post(this->uiExecutor, std::bind(&RedditDesktop::addSubredditWindow, this, std::move(title)));
    });
    commentsWindow->loadComments();
    commentsWindow->setFocused();
    commentsWindows.push_back(std::move(commentsWindow));
}
void RedditDesktop::setConnectionErrorMessage(std::string msg)
{
    showConnectionErrorDialog = true;
    connectionErrorMessage = msg;
}
void RedditDesktop::closeWindow()
{
    shouldQuit = true;
}
void RedditDesktop::updateWindowsTokenData()
{
    for(auto&& sr : subredditWindows)
    {
        sr->setAccessToken(currentAccessToken.data);
    }
    for(auto&& cm : commentsWindows)
    {
        cm->setAccessToken(currentAccessToken.data);
    }
    subredditsListWindow->setAccessToken(currentAccessToken.data);
}
void RedditDesktop::cleanupClosedWindows()
{
    auto startRemoveSubredditWindows = std::remove_if(subredditWindows.begin(),subredditWindows.end(),[](const auto& win) {
        return !win->isWindowOpen();
    });
    subredditWindows.erase(startRemoveSubredditWindows,subredditWindows.end());
    auto startRemoveCommentsWindows = std::remove_if(commentsWindows.begin(),commentsWindows.end(),[](const auto& win) {
        return !win->isWindowOpen();
    });
    commentsWindows.erase(startRemoveCommentsWindows,commentsWindows.end());
}
void RedditDesktop::showDesktop()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium)]);//Roboto medium
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Q)) && ImGui::GetIO().KeyCtrl)
    {
        shouldQuit = true;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_O)) && ImGui::GetIO().KeyCtrl)
    {
        openSubredditWindow = true;
    }
    showMainMenuBar();    

    int haveRemovedWindows = 0;
    for(const auto& srw : subredditWindows)
    {
        if(srw->isWindowOpen())
        {
            srw->showWindow(appFrameWidth,appFrameHeight);
        }
        else
        {
            ++haveRemovedWindows;
        }
    }
    for(const auto& cw : commentsWindows)
    {
        if(cw->isWindowOpen())
        {
            cw->showWindow(appFrameWidth,appFrameHeight);
        }
        else
        {
            ++haveRemovedWindows;
        }
    }

    if(haveRemovedWindows > 0)
    {
        boost::asio::post(uiExecutor,[weak=weak_from_this()](){
            auto self = weak.lock();
            if(!self) return;
            self->cleanupClosedWindows();
        });
    }

    if(subredditsListWindow)
    {
        ImGui::SetNextWindowPos(ImVec2(0,topPosAfterMenuBar));
        subredditsListWindow->showWindow(appFrameWidth,appFrameHeight);
    }
    showOpenSubredditWindow();

    if(loginWindow.showLoginWindow())
    {
        subredditsListWindow.reset();
        subredditWindows.clear();
        commentsWindows.clear();
        currentUser = loginWindow.getConfiguredUser();
        currentAccessToken = loginWindow.getAccessToken();
        Database::getInstance()->addRegisteredUser(currentUser.value());
        client.setUserAgent(make_user_agent(currentUser.value()));
        registeredUsers = Database::getInstance()->getRegisteredUsers();
        loggedInFirstTime = true;
        loginSuccessful(currentAccessToken);
    }
    if(userInfoWindow)
    {
        if(userInfoWindow->isWindowShowing())
        {
            userInfoWindow->showUserInfoWindow(appFrameWidth,appFrameHeight);
        }
        else
        {
            userInfoWindow.reset();
        }
    }
    showErrorDialog();

    if(loggingWindow->isWindowOpen())
    {
        loggingWindow->showWindow(appFrameWidth,appFrameHeight);
    }

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
void RedditDesktop::arrangeWindowsGrid()
{
    auto count = subredditWindows.size();
    count += commentsWindows.size();
    if(count > 0)
    {
        auto columnsDouble = std::sqrt(static_cast<double>(count));
        auto columns = static_cast<int>(std::round(columnsDouble));
        auto lines = static_cast<int>(std::ceil(count / columnsDouble));
        auto windowWidth = appFrameWidth / columns;
        auto windowHeight = (appFrameHeight-topPosAfterMenuBar) / lines;
        auto currentPosX = 0;
        auto currentPosY = topPosAfterMenuBar;

        for(const auto& sr: subredditWindows)
        {
            sr->setWindowPositionAndSize(ImVec2(currentPosX,currentPosY),ImVec2(windowWidth,windowHeight));
            currentPosX += windowWidth;
            if(currentPosX >= appFrameWidth)
            {
                currentPosX = 0;
                currentPosY += windowHeight;
            }
        }
        for(const auto& cm: commentsWindows)
        {
            cm->setWindowPositionAndSize(ImVec2(currentPosX,currentPosY),ImVec2(windowWidth,windowHeight));
            currentPosX += windowWidth;
            if(currentPosX >= appFrameWidth)
            {
                currentPosX = 0;
                currentPosY += windowHeight;
            }
        }
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
        if (ImGui::BeginMenu("Options"))
        {
            if(ImGui::Checkbox("Blur NSFW Thumbnails", &shouldBlurPictures))
            {
                Database::getInstance()->setBlurNSFWPictures(shouldBlurPictures);
            }
            if (ImGui::Checkbox("Show Random NSFW", &showRandomNSFW))
            {
                Database::getInstance()->setShowRandomNSFW(showRandomNSFW);
                subredditsListWindow->setShowRandomNSFW(showRandomNSFW);
            }
            if(ImGui::Checkbox("Hardware Accelerated Media", &useMediaHwAccel))
            {
                Database::getInstance()->setUseHWAccelerationForMedia(useMediaHwAccel);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Disable this if you experience crashes when playing media");
                ImGui::EndTooltip();
            }            
            if(ImGui::InputInt("Refresh (s)", &subredditsAutoRefreshTimeout, 30, 120))
            {
                if(subredditsAutoRefreshTimeout < 30) subredditsAutoRefreshTimeout = 30;
                Database::getInstance()->setAutoRefreshTimeout(subredditsAutoRefreshTimeout);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Subreddits refresh timeout, in seconds");
                ImGui::EndTooltip();
            }
            if(ImGui::Checkbox("Auto Arrange in Grid", &automaticallyArangeWindowsInGrid))
            {
                Database::getInstance()->setAutoArangeWindowsGrid(automaticallyArangeWindowsInGrid);
                if(automaticallyArangeWindowsInGrid)
                {
                    arrangeWindowsGrid();
                }
            }


            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Windows"))
        {
            if(ImGui::MenuItem("Grid Layout"))
            {
                arrangeWindowsGrid();
            }
            ImGui::Separator();
            for(const auto& sr: subredditWindows)
            {
                if(ImGui::MenuItem(sr->getTitle().c_str()))
                {
                    sr->setFocused();
                }
            }
            for(const auto& cm: commentsWindows)
            {
                if(ImGui::MenuItem(cm->getTitle().c_str()))
                {
                    cm->setFocused();
                }
            }
            ImGui::EndMenu();
        }
        if(infoUser)
        {
            const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
            static float infoButtonWidth = 0.0f;
            float pos = infoButtonWidth + itemSpacing;            
            ImGui::SameLine(ImGui::GetWindowWidth()-pos);
            if(ImGui::Button(userInfoDisplay.c_str()))
            {
                if(!userInfoWindow)
                {
                    userInfoWindow = std::make_shared<UserInformationWindow>(currentAccessToken.data,&client,uiExecutor);
                    userInfoWindow->showContextHandler([weak=weak_from_this()](const std::string& context){
                        auto self = weak.lock();
                        if(!self) return;
                        boost::asio::post(self->uiExecutor,
                                          std::bind(&RedditDesktop::addMessageContextWindow,
                                          self, context));
                    });
                    userInfoWindow->loadMessages();
                }
                userInfoWindow->shouldShowWindow(true);
            }
            infoButtonWidth = ImGui::GetItemRectSize().x;
        }
        ImGui::EndMainMenuBar();
    }
}

void RedditDesktop::showMenuFile()
{
    if (ImGui::MenuItem(reinterpret_cast<const char*>(ICON_FA_SEARCH " Open Subreddit"), "Ctrl+O",false,currentUser.has_value()))
    {
        openSubredditWindow = true;
    }
    ImGui::Separator();
    if(ImGui::BeginMenu("Login As"))
    {
        for(const auto& u : registeredUsers)
        {
            auto selected = currentUser.has_value() && currentUser->username == u.username;
            auto enabled = !selected;
            if (ImGui::MenuItem(u.username.c_str(),nullptr,selected,enabled))
            {
                currentUser = u;
                subredditsListWindow.reset();
                subredditWindows.clear();
                commentsWindows.clear();
                userInfoWindow.reset();
                loggedInFirstTime = true;
                refreshLoginToken();
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Register User"))
        {
            loginWindow.setShowLoginWindow(true);
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Logout",nullptr,false,currentUser.has_value()))
    {
        subredditsListWindow.reset();
        subredditWindows.clear();
        commentsWindows.clear();
        userInfoWindow.reset();
        currentAccessToken = client_response<access_token>();
        currentUser.reset();
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
    if (ImGui::BeginPopupModal(OPENSUBREDDIT_WINDOW_POPUP_TITLE,nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter subreddit name to open");
        char dummy_text[50] = { 0 };
        memset(dummy_text,'X',sizeof(dummy_text)-1);
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()
                && !ImGui::IsMouseClicked(0))
        {
           ImGui::SetKeyboardFocusHere(0);
        }
        auto dummySize = ImGui::CalcTextSize(dummy_text);
        ImGui::PushItemWidth(dummySize.x);
        ImGui::InputText("##opening_subreddit",selectedSubreddit,IM_ARRAYSIZE(selectedSubreddit),
                         ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        bool searchDisabled = std::string_view(selectedSubreddit).empty();
        if(searchDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_SEARCH "##searchSubreddit")))
        {
            searchSubreddits();
        }
        if (searchDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        if(!searchedNamesList.empty())
        {
            ImGui::BeginChild("##searchResults", ImVec2(0, dummySize.y*10), true);
            for(const auto& name : searchedNamesList)
            {
                if(ImGui::Selectable(name.c_str()))
                {
                    std::copy(name.begin(), name.end(), selectedSubreddit);
                    selectedSubreddit[name.size()] = '\0';
                }
            }
           ImGui::EndChild();
        }
        ImGui::Separator();
        bool okDisabled = searchDisabled || searchingSubreddits;
        if(okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)) ||
                (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) && !okDisabled))
        {
            addSubredditWindow(selectedSubreddit);
            ImGui::CloseCurrentPopup();
            searchNamesConnection.reset();
            searchedNamesList.clear();
        }
        if (okDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        if(searchingSubreddits)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)) && !searchingSubreddits))
        {
            ImGui::CloseCurrentPopup();
            searchNamesConnection.reset();
            searchedNamesList.clear();
        }
        if (searchingSubreddits)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::EndPopup();
    }
}
void RedditDesktop::searchSubreddits()
{
    searchedNamesList.clear();
    searchingSubreddits = true;
    if(!searchNamesConnection)
    {
        searchNamesConnection = client.makeRedditSearchNamesClientConnection();
        searchNamesConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                     client_response<names_list> response)
         {
            auto self = weak.lock();
            if(!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setConnectionErrorMessage,self,response.body));
                }
                else
                {
                    boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::setSearchResultsNames,self,std::move(response.data)));
                }
            }
         });
    }

    searchNamesConnection->search(selectedSubreddit,currentAccessToken.data);
}

void RedditDesktop::setSearchResultsNames(names_list names)
{
    searchedNamesList = std::move(names);
    searchingSubreddits = false;
}
