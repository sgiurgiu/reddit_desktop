#include "redditdesktop.h"
#include "imgui_internal.h"
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "imgui_stdlib.h"
#include <fmt/format.h>
#include "database.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <thread>
#include <fstream>

namespace
{
constexpr auto OPENSUBREDDIT_WINDOW_POPUP_TITLE = "Open Subreddit";
constexpr auto IMPORTSUBS_WINDOW_POPUP_TITLE = "Import Subreddits from account";
constexpr auto ERROR_WINDOW_POPUP_TITLE = "Error Occurred";
constexpr auto MEDIA_DOMAINS_POPUP_TITLE = "Media Domains Management";
constexpr auto TWITTER_API_BEARER_TITLE = "Twitter API Auth Bearer";
constexpr auto BACKGROUND_COLOR_MENU = "Background Color";
constexpr auto BACKGROUND_COLOR_NAME = "BACKGROUND";
constexpr auto IPINFO_WINDOW_POPUP_TITLE = "Network Information";
constexpr auto EXPORT_SUBS_POPUP_TITLE = "Export Subscribed Subreddits";
constexpr auto IMPORT_FILE_SUBS_POPUP_TITLE = "Import Subreddits from file";
}

RedditDesktop::RedditDesktop(boost::asio::io_context& uiContext):
    uiExecutor(uiContext.get_executor()),
    client("api.reddit.com","oauth.reddit.com",std::max(2u, std::thread::hardware_concurrency())),
    loginWindow(&client,uiExecutor),loginTokenRefreshTimer(uiExecutor),
    loggingWindow(std::make_shared<LoggingWindow>(uiExecutor))
{
    auto db = Database::getInstance();
    registeredUsers = db->getRegisteredUsers();
    shouldBlurPictures= db->getBlurNSFWPictures();
    useMediaHwAccel = db->getUseHWAccelerationForMedia();
    subredditsAutoRefreshTimeout = db->getAutoRefreshTimeout();
    showRandomNSFW = db->getShowRandomNSFW();
    automaticallyArangeWindowsInGrid = db->getAutoArangeWindowsGrid();
    useYoutubeDl = db->getUseYoutubeDownloader();
    automaticallyLogIn = db->getAutomaticallyLogIn();
    auto bCol = db->getColor(BACKGROUND_COLOR_NAME,{backgroundColor.x,backgroundColor.y,backgroundColor.z,backgroundColor.w});
    backgroundColor.x = bCol[0];
    backgroundColor.y = bCol[1];
    backgroundColor.z = bCol[2];
    backgroundColor.w = bCol[3];
    exportSubsFile = Database::getInstance()->getLastExportedSubsFile();
    loggingWindow->setupLogging();
    loggingWindow->setWindowOpen(showLoggingWindow);
}

void RedditDesktop::automaticallySetCurrentUser()
{
    if(!registeredUsers.empty())
    {
        currentUser = registeredUsers.front();
    }
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
    if(!currentUser) return;
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
            userInfoDisplay.append(fmt::format(" {}{}",infoUser->inboxCount, reinterpret_cast<const char*>(" " ICON_FA_ENVELOPE_O)));
        }
        if(infoUser->hasModMail)
        {
            userInfoDisplay.append(fmt::format(" {}{}",infoUser->inboxCount, reinterpret_cast<const char*>(" " ICON_FA_BELL)));
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
        subredditWindow->subscriptionChangedListener([this]() {
            boost::asio::post(this->uiExecutor,[this](){
                subredditsListWindow->loadSubredditsList();
            });
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

    if(subredditsListWindow)
    {
        ImGui::SetNextWindowPos(ImVec2(0,topPosAfterMenuBar));
        subredditsListWindow->showWindow(appFrameWidth,appFrameHeight);
    }

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
    showMediaDomainsManagementDialog();
    showTwitterAPIAuthBearerDialog();
    showImportSubsDialog();
    showNetworkInformationDialog();
    showExportSubsDialog();
    showImportFromFileSubsDialog();
    aboutWindow.showAboutWindow(appFrameWidth,appFrameHeight);

    if(proxySettingsWindow.showProxySettingsWindow())
    {
        subredditsListWindow.reset();
        subredditWindows.clear();
        commentsWindows.clear();
        userInfoWindow.reset();
        searchNamesConnection.reset();
        loggedInFirstTime = true;
        refreshLoginToken();
    }

    ImGui::PopFont();
}
void RedditDesktop::showTwitterAPIAuthBearerDialog()
{
    if(twitterAPIAuthBearerDialog)
    {
        twitterAPIAuthBearerDialog = false;
        ImGui::OpenPopup(TWITTER_API_BEARER_TITLE);
        auto twitterBearerOpt = Database::getInstance()->getTwitterAuthBearer();
        if(twitterBearerOpt)
        {
            twitterBearer = twitterBearerOpt.value();
        }
    }
    if(ImGui::BeginPopupModal(TWITTER_API_BEARER_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped(
            "Enter the Bearer Authorization token for Twitter API."
            "Get yours from https://developer.twitter.com");

        ImGui::Separator();
        ImGui::InputText("Twitter API Bearer",&twitterBearer);

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            Database::getInstance()->setTwitterAuthBearer(twitterBearer);
            ImGui::CloseCurrentPopup();
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
void RedditDesktop::showMediaDomainsManagementDialog()
{
    if(mediaDomainsManagementDialog)
    {
        ImGui::OpenPopup(MEDIA_DOMAINS_POPUP_TITLE);
        mediaDomainsManagementDialog = false;
        mediaDomains = Database::getInstance()->getMediaDomains();
        selectedMediaDomain.clear();
    }
    ImGui::SetNextWindowSize(ImVec2(375,380),ImGuiCond_Once);
    if(ImGui::BeginPopupModal(MEDIA_DOMAINS_POPUP_TITLE,nullptr,ImGuiWindowFlags_None))
    {

        ImGui::TextWrapped(
            "Manage the domains that will be considered as \"Media Domains\". "
            "Posts from these domains will be treated as containing media (video/audio)"
            " which we will try to find and play.");

        ImGui::Separator();

        bool activated = ImGui::InputTextWithHint("##addNewMediaDomain","Domain",
                                                  &newMediaDomain,ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, newMediaDomain.empty());
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (newMediaDomain.empty()?0.5f:1.f));
        activated |= ImGui::Button("Add");
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
        if(activated && !newMediaDomain.empty())
        {
            mediaDomains.push_back(newMediaDomain);
            Database::getInstance()->addMediaDomain(newMediaDomain);
            newMediaDomain.clear();
        }
        ImGui::SetNextItemWidth(-1.0f);
        if(ImGui::BeginListBox("##mediaDomainsPopup"))
        {
            for(const auto& domain : mediaDomains)
            {
                if(ImGui::Selectable(domain.c_str(), selectedMediaDomain == domain))
                {
                    selectedMediaDomain = domain;
                }
            }
            ImGui::EndListBox();
        }
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, selectedMediaDomain.empty());
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (selectedMediaDomain.empty()?0.5f:1.f));
        if (ImGui::Button("Remove", ImVec2(120, 0)))
        {
            mediaDomains.erase(std::remove_if(mediaDomains.begin(),mediaDomains.end(),[this](const auto& d){
                return d == selectedMediaDomain;
            }));
            Database::getInstance()->removeMediaDomain(selectedMediaDomain);
            selectedMediaDomain.clear();
        }
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
        ImGui::SameLine(0.0f, 120.f);
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            selectedMediaDomain.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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
            if (ImGui::Checkbox("Automatically Login", &automaticallyLogIn))
            {
                Database::getInstance()->setAutomaticallyLogIn(automaticallyLogIn);
            }
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
            if(ImGui::Checkbox("Use youtube-dl", &useYoutubeDl))
            {
                Database::getInstance()->setUseYoutubeDownloader(useYoutubeDl);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Use the youtube-dl script to download embedded media instead of the built-in algorithm");
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
            if(ImGui::MenuItem(MEDIA_DOMAINS_POPUP_TITLE))
            {
                mediaDomainsManagementDialog = true;
            }
            if(ImGui::MenuItem(TWITTER_API_BEARER_TITLE))
            {
                twitterAPIAuthBearerDialog = true;
            }
            if(ImGui::BeginMenu(BACKGROUND_COLOR_MENU))
            {
                float col[3] = { backgroundColor.x, backgroundColor.y,
                                  backgroundColor.z };
                if(ImGui::ColorPicker3("###background",col, ImGuiColorEditFlags_AlphaBar|
                                       ImGuiColorEditFlags_NoOptions|
                                       ImGuiColorEditFlags_NoInputs|
                                       ImGuiColorEditFlags_NoSidePreview))
                {
                    backgroundColor.x = col[0];
                    backgroundColor.y = col[1];
                    backgroundColor.z = col[2];
                    backgroundColor.w = 1.f;
                }
                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("Proxy Settings"))
            {
                proxySettingsWindow.setShowProxySettingsWindow(true);
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
        if(ImGui::BeginMenu("Help"))
        {
            if(ImGui::MenuItem("Network Information"))
            {
                showNetworkInfomation = true;
            }
            if(ImGui::MenuItem("About"))
            {
                aboutWindow.setWindowShowing(true);
            }
            ImGui::EndMenu();
        }
        if(infoUser)
        {
            const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
            static float infoButtonWidth = 0.0f;
            float pos = infoButtonWidth + itemSpacing;            
            ImGui::SameLine(ImGui::GetWindowWidth()-pos);
            //ImGui::Text("%ls",L"\x01f449");
            //ImGui::SameLine();
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
                    userInfoWindow->refreshHandler([weak = weak_from_this()](){
                        auto self = weak.lock();
                        if(!self) return;
                        boost::asio::post(self->uiExecutor,
                                          std::bind(&RedditDesktop::loadUserInformation,
                                          self));
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
        if(!registeredUsers.empty())
        {
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Register User"))
        {
            loginWindow.setShowLoginWindow(true);
        }
        ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Import Subs", currentUser.has_value()))
    {
        if (ImGui::MenuItem("From File..."))
        {
            importSubsFromFileDialog = true;
        }
        ImGui::Separator();
        for(const auto& u : registeredUsers)
        {
            auto current = currentUser.has_value() && currentUser->username == u.username;
            if (!current && ImGui::MenuItem(u.username.c_str()))
            {
                importSubsDialog = true;
                importingUser = std::make_optional<ImportingUser>();
                importingUser->importUser = u;
                importingUser->importingState = ImportingState::NotStarted;
                importingUser->importingStatusMessage = fmt::format("Importing subscriptions from user {}. Click Import to proceed.",u.username);
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Export Subs", nullptr, false, currentUser.has_value()))
    {
        exportSubsDialog = true;
    }
    if (ImGui::MenuItem("Logout", nullptr, false, currentUser.has_value()))
    {
        subredditsListWindow.reset();
        subredditWindows.clear();
        commentsWindows.clear();
        userInfoWindow.reset();
        currentAccessToken = client_response<access_token>();
        currentUser.reset();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(reinterpret_cast<const char *>(ICON_FA_POWER_OFF " Quit"), "Ctrl+Q"))
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
void RedditDesktop::saveBackgroundColor()
{
    Database::getInstance()->setColor(BACKGROUND_COLOR_NAME,{backgroundColor.x,backgroundColor.y,backgroundColor.z,backgroundColor.w});
}
void RedditDesktop::showImportSubsDialog()
{
    if(importSubsDialog && importingUser)
    {
        importSubsDialog = false;
        ImGui::OpenPopup(IMPORTSUBS_WINDOW_POPUP_TITLE);
    }
    if(importingUser.has_value() && ImGui::BeginPopupModal(IMPORTSUBS_WINDOW_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(importingUser->importingState == ImportingState::FinishedWithError)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
        ImGui::Text("%s",importingUser->importingStatusMessage.c_str());
        if(importingUser->importingState == ImportingState::FinishedWithError)
        {
            ImGui::PopStyleColor();
        }

        switch(importingUser->importingState)
        {
        case ImportingState::NotStarted:
            showImportNotStartedControls();
            break;
        case ImportingState::Importing:
            showImportingControls();
            break;
        case ImportingState::FinishedSuccessfully:
            [[fallthrough]];
        case ImportingState::FinishedWithError:
            showImportFinishedControls();
            break;
        }
        ImGui::EndPopup();
    }

}

void RedditDesktop::showImportNotStartedControls()
{
    ImGui::Separator();
    if (ImGui::Button("Import", ImVec2(120, 0)))
    {
        startImportingSubs();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(120, 0)) ||
        ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        importingUser.reset();
        ImGui::CloseCurrentPopup();
    }
}
void RedditDesktop::showImportingControls()
{
    ImGui::Separator();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) /*IsKeyPressed is intentionally removed in this state*/
    {
        importingUser.reset();
        ImGui::CloseCurrentPopup();
    }
}
void RedditDesktop::showImportFinishedControls()
{
    ImGui::Separator();
    if (ImGui::Button("Finish", ImVec2(120, 0)) ||
        ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        importingUser.reset();
        ImGui::CloseCurrentPopup();
    }
}

void RedditDesktop::startImportingSubs()
{
    importingUser->importingState = ImportingState::Importing;
    importingUser->importingStatusMessage = fmt::format("Logging in as {} ...",importingUser->importUser.username);
    auto clientProducer = std::make_shared<RedditClientProducer>("api.reddit.com","oauth.reddit.com",2);
    auto importLoginConnection = clientProducer->makeLoginClientConnection();
    importLoginConnection->connectionCompleteHandler([weak=weak_from_this(), clientProducer]
                           (const boost::system::error_code& ec, client_response<access_token> token){
           auto self = weak.lock();
           if(!self) return;
           if(ec)
           {
               boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,ec.message()));
           }
           else
           {
               if(token.status >= 400)
               {
                   boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,token.body));
               }
               else
               {
                   boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsLoginSuccess,self,std::move(token.data), clientProducer));
               }
           }
    });
    clientProducer->setUserAgent(make_user_agent(importingUser->importUser));
    importLoginConnection->login(importingUser->importUser);
}
void RedditDesktop::importingSubsError(std::string msg)
{
    if(!importingUser) return;
    importingUser->importingState = ImportingState::FinishedWithError;
    importingUser->importingStatusMessage = fmt::format("Error occurred: {}",std::move(msg));
}
void RedditDesktop::importingSubsLoginSuccess(access_token token, std::shared_ptr<RedditClientProducer> clientProducer)
{
    if(!importingUser) return;
    importingUser->importingStatusMessage = fmt::format("Loading {} subreddits ...",importingUser->importUser.username);
    loadImportingUserSubreddits("/subreddits/mine/subscriber?show=all&limit=100",std::move(token), clientProducer);
}

void RedditDesktop::loadImportingUserSubreddits(std::string url, access_token token,
                                 std::shared_ptr<RedditClientProducer> clientProducer,
                                 RedditClientProducer::RedditListingClientConnection connection)
{
    if(!connection)
    {
        connection = clientProducer->makeListingClientConnection();
        connection->connectionCompleteHandler([weak=weak_from_this(),clientProducer, connection, token]
                                              (const boost::system::error_code& ec,
                                                 client_response<listing> response)
        {
            auto self = weak.lock();
            if(!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,ec.message()));
            }
            else
            {
                if(response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,response.body));
                }
                else
                {
                    auto listingChildren = response.data.json["data"]["children"];
                    subreddit_list srs;
                    for(const auto& child: listingChildren)
                    {
                        subreddit sr{child["data"]};
                        if(sr.subredditType == "public")
                        {
                            srs.push_back(std::move(sr));
                        }
                    }

                    boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::collectImportingUserSubreddits,self,
                                                                  std::move(srs),std::move(token), std::move(clientProducer), std::move(connection)));
                }
            }
        });
    }

    connection->list(url, token);
}
void RedditDesktop::collectImportingUserSubreddits(subreddit_list subs, access_token token,
                                    std::shared_ptr<RedditClientProducer> clientProducer,
                                    RedditClientProducer::RedditListingClientConnection connection)
{
    if(!importingUser) return;
    if(!subs.empty())
    {
        auto url = fmt::format("/subreddits/mine/subscriber?show=all&limit=100&after={}&count={}",subs.back().name,importingUser->subredditsToImport.size());
        loadImportingUserSubreddits(url,std::move(token), std::move(clientProducer), std::move(connection));
        std::move(subs.begin(), subs.end(), std::back_inserter(importingUser->subredditsToImport));
    }
    else
    {
        importingUser->importingStatusMessage = fmt::format("Subscribing {} to {} subreddits ...",currentUser->username,importingUser->subredditsToImport.size());
        //we're done, we now subscribe to them
        subscribeToSubreddits();
    }
}

void RedditDesktop::subscribeToSubreddits()
{
    if(!importingUser) return;
    if(importingUser->subredditsToImport.empty())
    {
        importingSubsError(fmt::format("Could not load any subreddits from {}",importingUser->importUser.username));
        return;
    }
    auto srSubscriptionConnection = client.makeRedditRedditSRSubscriptionClientConnection();
    srSubscriptionConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                                                  client_response<bool> response)
    {
        auto self = weak.lock();
        if(!self) return;
        if(!ec && response.data)
        {
            boost::asio::post(self->uiExecutor,[w = self->weak_from_this()]()
            {
                auto self = w.lock();
                if(!self || !self->importingUser) return;
                self->importingUser->importingStatusMessage = "Imported successfully";
                self->importingUser->importingState = ImportingState::FinishedSuccessfully;
                self->subredditsListWindow->loadSubredditsList();
            });
        }
        else
        {
            auto message = ec ? ec.message() : response.body;
            boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,message));
        }
    });
    std::vector<std::string> subreddits;
    subreddits.reserve(importingUser->subredditsToImport.size());
    std::transform(importingUser->subredditsToImport.cbegin(),importingUser->subredditsToImport.cend(),
                   std::back_inserter(subreddits),[](const auto& el){return el.name;});
    srSubscriptionConnection->updateSrSubscription(subreddits,RedditSRSubscriptionConnection::SubscriptionAction::Subscribe,currentAccessToken.data);
}

void RedditDesktop::showNetworkInformationDialog()
{
    if(showNetworkInfomation)
    {
        showNetworkInfomation = false;
        ImGui::OpenPopup(IPINFO_WINDOW_POPUP_TITLE);
        auto ipInfoConnection = client.makeIpInfoClientConnection();
        ipInfoConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                       IpInfoResponse response){
            auto self = weak.lock();
            if(!self) return;
            if(ec || response.status >= 400)
            {
                boost::asio::post(self->uiExecutor,[w = self->weak_from_this(),message = ec.message()](){
                    auto self = w.lock();
                    if(!self) return;
                    self->ipInfoRetreivalErrorMessage = message;
                });
            }
            else
            {
                boost::asio::post(self->uiExecutor,[w = self->weak_from_this(), response](){
                    auto self = w.lock();
                    if(!self) return;
                    self->ipInfoRetreivalErrorMessage = "";
                    self->ipInfo = response.data;
                });
            }
        });
        ipInfoConnection->LoadIpInfo();
    }
    if(ImGui::BeginPopupModal(IPINFO_WINDOW_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(!ipInfoRetreivalErrorMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"%s",ipInfoRetreivalErrorMessage.c_str());
        }
        else
        {
            if(ipInfo)
            {
                ImGui::Text("IP: %s", ipInfo->ip.c_str());
                ImGui::Text("Host: %s", ipInfo->hostname.c_str());
                ImGui::Text("ISP: %s", ipInfo->org.c_str());
                ImGui::Text("City: %s", ipInfo->city.c_str());
                ImGui::Text("Region: %s", ipInfo->region.c_str());
                ImGui::Text("Country: %s", ipInfo->country.c_str());
                ImGui::Text("Timezone: %s", ipInfo->timezone.c_str());
            }
            else
            {
                ImGui::Text("Loading Network Information...");
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0)) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            ipInfo.reset();
            ipInfoRetreivalErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RedditDesktop::showExportSubsDialog()
{
    if(exportSubsDialog && currentUser)
    {
        exportSubsDialog = false;
        ImGui::OpenPopup(EXPORT_SUBS_POPUP_TITLE);
    }
    if(currentUser.has_value() && ImGui::BeginPopupModal(EXPORT_SUBS_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Export",&exportSubsFile);
        ImGui::Separator();
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            Database::getInstance()->setLastExportedSubsFile(exportSubsFile);
            auto subscriptions = subredditsListWindow->getLoadedSubscribedSubreddits();
            nlohmann::json subredditsArray = nlohmann::json::array();
            for(const auto& sr:subscriptions)
            {
                nlohmann::json subredditJsonObject = nlohmann::json::object();
                subredditJsonObject["id"] = sr.id;
                subredditJsonObject["name"] = sr.name;
                subredditJsonObject["display_name"] = sr.displayName;
                subredditJsonObject["display_name_prefixed"] = sr.displayNamePrefixed;
                subredditJsonObject["url"] = sr.url;
                subredditJsonObject["title"] = sr.title;
                subredditsArray.push_back(std::move(subredditJsonObject));
            }
            nlohmann::json outputJson = nlohmann::json::object();
            outputJson["subreddits"] = std::move(subredditsArray);
            try
            {
                std::ofstream out;
                out.open(exportSubsFile);
                if(!out)
                {
                    throw std::system_error(errno, std::system_category());
                }
                out << std::setw(4) << outputJson << std::endl;
            }
            catch(const std::exception& ex)
            {
                setConnectionErrorMessage(fmt::format("Cannot write to file {}, error {}",exportSubsFile,ex.what()));
            }
            catch(...)
            {
                setConnectionErrorMessage(fmt::format("Cannot write to file {}",exportSubsFile));
            }
            ImGui::CloseCurrentPopup();
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

void RedditDesktop::showImportFromFileSubsDialog()
{
    if(importSubsFromFileDialog && currentUser)
    {
        importSubsFromFileDialog = false;
        ImGui::OpenPopup(IMPORT_FILE_SUBS_POPUP_TITLE);
    }
    if(currentUser.has_value() && ImGui::BeginPopupModal(IMPORT_FILE_SUBS_POPUP_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Import",&exportSubsFile);
        ImGui::Separator();
        if (ImGui::Button("Open", ImVec2(120, 0)))
        {
            try
            {
                std::ifstream in;
                in.open(exportSubsFile);
                if(!in)
                {
                    throw std::system_error(errno, std::system_category());
                }
                auto json = nlohmann::json::parse(in);
                if(json.contains("subreddits") && json["subreddits"].is_array())
                {
                    std::vector<std::string> names;
                    std::transform(json["subreddits"].begin(),json["subreddits"].end(),std::back_inserter(names),
                        [](const auto& sr){
                            if(sr.is_object() && sr.contains("name") && sr["name"].is_string()) {
                                return sr["name"].template get<std::string>();
                            } else {
                                throw std::runtime_error("Invalid subreddits file");
                            }
                        });
                    auto srSubscriptionConnection = client.makeRedditRedditSRSubscriptionClientConnection();
                    srSubscriptionConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                                                                client_response<bool> response)
                    {
                        auto self = weak.lock();
                        if(!self) return;
                        if(!ec && response.data)
                        {
                            boost::asio::post(self->uiExecutor,[w = self->weak_from_this()]()
                            {
                                auto self = w.lock();
                                if(!self) return;
                                self->subredditsListWindow->loadSubredditsList();
                            });
                        }
                        else
                        {
                            auto message = ec ? ec.message() : response.body;
                            boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::importingSubsError,self,message));
                        }
                    });
                    srSubscriptionConnection->updateSrSubscription(std::move(names),RedditSRSubscriptionConnection::SubscriptionAction::Subscribe,currentAccessToken.data);
                }
                else
                {
                    throw std::runtime_error("Invalid subreddits file");
                }
            }
            catch(const std::exception& ex)
            {
                setConnectionErrorMessage(fmt::format("Cannot read from file {}, error {}",exportSubsFile,ex.what()));
            }
            catch(...)
            {
                setConnectionErrorMessage(fmt::format("Cannot read from file {}",exportSubsFile));
            }
            ImGui::CloseCurrentPopup();
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