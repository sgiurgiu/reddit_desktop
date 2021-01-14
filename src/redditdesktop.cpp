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
constexpr auto SUBREDDITS_WINDOW_TITLE = "My Subreddits";
}

RedditDesktop::RedditDesktop(boost::asio::io_context& uiContext):
    uiExecutor(uiContext.get_executor()),
    client("api.reddit.com","oauth.reddit.com",3),
    loginWindow(&client,uiExecutor),loginTokenRefreshTimer(uiContext)
{
    subredditsSortMethod[SubredditsSorting::None] = "None";
    subredditsSortMethod[SubredditsSorting::Alphabetical_Ascending] = "Alphabetical";
    subredditsSortMethod[SubredditsSorting::Alphabetical_Descending] = "Alphabetical (Inverse)";
    current_user = Database::getInstance()->getRegisteredUser();
    shouldBlurPictures= Database::getInstance()->getBlurNSFWPictures();
}

void RedditDesktop::loginCurrentUser()
{
    if(!current_user)
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
    loginConnection->connectionCompleteHandler([self=shared_from_this()](const boost::system::error_code& ec,const client_response<access_token>& token){
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
    client.setUserAgent(make_user_agent(current_user.value()));
    loginConnection->login(current_user.value());
}

void RedditDesktop::loginSuccessful(client_response<access_token> token)
{
    current_access_token = token;
    loginTokenRefreshTimer.expires_after(std::chrono::seconds(std::min(current_access_token.data.expires,10*60)));
    loginTokenRefreshTimer.async_wait([weak=weak_from_this()](const boost::system::error_code& ec){
        auto self = weak.lock();
        if(self && !ec)
        {
            self->refreshLoginToken();
        }
    });

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
                                                 const client_response<listing>& response)
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
    userInformationConnection->list("/api/v1/me",current_access_token.data);
}
void RedditDesktop::updateUserInformation(user_info info)
{
    info_user = std::move(info);
    unsortedSubscribedSubreddits.clear();
    loadSubreddits("/subreddits/mine/subscriber?show=all&limit=100",current_access_token.data);
    loadMultis("/api/multi/mine",current_access_token.data);
}
void RedditDesktop::loadMultis(const std::string& url, const access_token& token)
{
    auto multisConnection = client.makeListingClientConnection();
    multisConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                 const client_response<listing>& response)
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
                boost::asio::post(self->uiExecutor,
                                  std::bind(&RedditDesktop::setConnectionErrorMessage,self,response.body));
            }
            else
            {
                multireddit_list srs;
                for(const auto& child: response.data.json)
                {
                    srs.emplace_back(child["data"]);
                }

                boost::asio::post(self->uiExecutor,
                                  std::bind(&RedditDesktop::setUserMultis,self,std::move(srs)));
            }
        }
     });
     multisConnection->list(url,token);
}
void RedditDesktop::setUserMultis(multireddit_list multis)
{
    userMultis = std::move(multis);
}
void RedditDesktop::loadSubscribedSubreddits(subreddit_list srs)
{
    if(!srs.empty())
    {
        auto url = fmt::format("/subreddits/mine/subscriber?show=all&limit=100&after={}&count={}",srs.back().name,srs.size());
        loadSubreddits(url,current_access_token.data);
    }
    std::move(srs.begin(), srs.end(), std::back_inserter(unsortedSubscribedSubreddits));
    subscribedSubreddits = unsortedSubscribedSubreddits;
    sortSubscribedSubreddits();
}
void RedditDesktop::loadSubreddits(const std::string& url, const access_token& token)
{
    auto subscribedSubredditsConnection = client.makeListingClientConnection();
    subscribedSubredditsConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                 const client_response<listing>& response)
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
                auto listingChildren = response.data.json["data"]["children"];
                subreddit_list srs;
                for(const auto& child: listingChildren)
                {
                    srs.emplace_back(child["data"]);
                }

                boost::asio::post(self->uiExecutor,std::bind(&RedditDesktop::loadSubscribedSubreddits,self,std::move(srs)));
            }
        }
     });
     subscribedSubredditsConnection->list(url,token);
}
void RedditDesktop::sortSubscribedSubreddits()
{
    if(currentSubredditsSorting != SubredditsSorting::None)
    {
        std::stable_sort(subscribedSubreddits.begin(),subscribedSubreddits.end(),[this](const auto& lhs,const auto& rhs){
            auto ldisplayName = boost::to_lower_copy(lhs.sr.displayName);
            auto rdisplayName = boost::to_lower_copy(rhs.sr.displayName);
            if(currentSubredditsSorting == SubredditsSorting::Alphabetical_Ascending) {
                return ldisplayName < rdisplayName;
            } else {
                return ldisplayName > rdisplayName;
            }
        });
    }
    else
    {
        subscribedSubreddits = unsortedSubscribedSubreddits;
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
                                                                 title,current_access_token.data,
                                                                 &client,uiExecutor);
        using namespace std::placeholders;
        subredditWindow->showCommentsListener(std::bind(&RedditDesktop::addCommentsWindow,this,_1,_2));
        subredditWindow->loadSubreddit();
        subredditWindows.push_back(std::move(subredditWindow));
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
                                                               current_access_token.data,
                                                               &client,uiExecutor);
        using namespace std::placeholders;
        commentsWindow->addOpenSubredditHandler(std::bind(&RedditDesktop::addSubredditWindow,this,_1));
        commentsWindow->loadComments();
        commentsWindows.push_back(std::move(commentsWindow));
    }
    else
    {
        (*it)->setFocused();
    }
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
        sr->setAccessToken(current_access_token.data);
    }
    for(auto&& cm : commentsWindows)
    {
        cm->setAccessToken(current_access_token.data);
    }
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
    showSubredditsWindow();

    {
        auto it = subredditWindows.begin();
        while(it != subredditWindows.end())
        {
            if((*it)->isWindowOpen())
            {
                (*it)->showWindow(appFrameWidth,appFrameHeight);
                ++it;
            }
            else
            {
                it = subredditWindows.erase(it);
            }
        }
    }
    {
        auto it = commentsWindows.begin();
        while(it != commentsWindows.end())
        {
            if((*it)->isWindowOpen())
            {
                (*it)->showWindow(appFrameWidth,appFrameHeight);
                ++it;
            }
            else
            {
                it = commentsWindows.erase(it);
            }
        }
    }

    showOpenSubredditWindow();

    if(loginWindow.showLoginWindow())
    {
        current_user = loginWindow.getConfiguredUser();
        current_access_token = loginWindow.getAccessToken();
        Database::getInstance()->setRegisteredUser(current_user.value());
        client.setUserAgent(make_user_agent(current_user.value()));
        loginSuccessful(current_access_token);
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

    if (ImGui::BeginTabBar("SubredditsTabBar", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Subreddits"))
        {
            showSubredditsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Multireddits"))
        {
            showMultisTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}
void RedditDesktop::showMultisTab()
{
    for(const auto& sr : userMultis)
    {
        if(ImGui::Selectable(sr.displayName.c_str()))
        {
            addSubredditWindow(sr.path);
        }
    }
}
void RedditDesktop::showSubredditsTab()
{
    if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_REFRESH)))
    {
        //this clear is just for show, to tell the user that we're working, doing something
        subscribedSubreddits.clear();
        unsortedSubscribedSubreddits.clear();
        frontPageSubredditSelected = false;
        allSubredditSelected = false;
        loadSubreddits("/subreddits/mine/subscriber?show=all&limit=100",current_access_token.data);
    }
    ImGui::SameLine();
    ImGui::Text("Sort:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##subreddits_sorting_combo", subredditsSortMethod[currentSubredditsSorting].c_str()))
    {
        for(const auto& p : subredditsSortMethod)
        {
            if (ImGui::Selectable(p.second.c_str(), currentSubredditsSorting == p.first))
            {
                currentSubredditsSorting = p.first;
                sortSubscribedSubreddits();
            }
        }
        ImGui::EndCombo();
    }

    if(ImGui::Selectable("Front Page",frontPageSubredditSelected,ImGuiSelectableFlags_AllowDoubleClick))
    {
        frontPageSubredditSelected = true;
        allSubredditSelected = false;
        std::for_each(subscribedSubreddits.begin(),subscribedSubreddits.end(),
                       [](auto&& item){
            item.selected = false;
        });
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            addSubredditWindow("");
        }
    }
    if(ImGui::Selectable("All",allSubredditSelected,ImGuiSelectableFlags_AllowDoubleClick))
    {
        std::for_each(subscribedSubreddits.begin(),subscribedSubreddits.end(),
                       [](auto&& item){
            item.selected = false;
        });
        frontPageSubredditSelected = false;
        allSubredditSelected = true;
        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            addSubredditWindow("r/all");
        }
    }
    for(auto&& sr : subscribedSubreddits)
    {        
        if(ImGui::Selectable(sr.sr.displayName.c_str(),sr.selected,ImGuiSelectableFlags_AllowDoubleClick))
        {
            frontPageSubredditSelected = false;
            allSubredditSelected = false;
            std::for_each(subscribedSubreddits.begin(),subscribedSubreddits.end(),
                           [](auto&& item){
                item.selected = false;
            });
            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                addSubredditWindow(sr.sr.displayNamePrefixed);
            }
            sr.selected = true;
        }
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

            ImGui::EndMenu();
        }
        if(info_user)
        {
            const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
            static float infoButtonWidth = 0.0f;
            float pos = infoButtonWidth + itemSpacing;
            auto displayed_info = fmt::format("{} ({}-{})",info_user->name,info_user->humanLinkKarma,info_user->humanCommentKarma);
            if(info_user->hasMail)
            {
                displayed_info.append(reinterpret_cast<const char*>(" " ICON_FA_ENVELOPE_O));
            }
            if(info_user->hasModMail)
            {
                displayed_info.append(reinterpret_cast<const char*>(" " ICON_FA_BELL));
            }
            ImGui::SameLine(ImGui::GetWindowWidth()-pos);
            if(ImGui::Button(displayed_info.c_str()))
            {
                if(!userInfoWindow)
                {
                    userInfoWindow = std::make_shared<UserInformationWindow>(current_access_token.data,&client,uiExecutor);
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
    if (ImGui::MenuItem(reinterpret_cast<const char*>(ICON_FA_SEARCH " Open Subreddit"), "Ctrl+O"))
    {
        openSubredditWindow = true;
    }

    /*if (ImGui::BeginMenu("Open Recent"))
    {
        ImGui::MenuItem("fish_hat.h");
        ImGui::EndMenu();
    }*/
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
                                                     const client_response<names_list>& response)
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

    searchNamesConnection->search(selectedSubreddit,current_access_token.data);
}

void RedditDesktop::setSearchResultsNames(names_list names)
{
    searchedNamesList = std::move(names);
    searchingSubreddits = false;
}
