#include "subredditslistwindow.h"
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include <boost/algorithm/string/case_conv.hpp>
#include "database.h"

namespace
{
constexpr auto SUBREDDITS_WINDOW_TITLE = "My Subreddits";
}

SubredditsListWindow::SubredditsListWindow(const access_token& token,
                                           RedditClientProducer* client,
                                           const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor)
{
    showRandomNSFW = Database::getInstance()->getShowRandomNSFW();
}
void SubredditsListWindow::loadSubredditsList()
{
    subscribedSubreddits.clear();
    filteredSubscribedSubreddits.clear();
    userMultis.clear();
    filteredUserMultis.clear();

    loadSubreddits("/subreddits/mine/subscriber?show=all&limit=100",token);
    loadMultis("/api/multi/mine",token);
}
void SubredditsListWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.2f,appFrameHeight*0.6f),ImGuiCond_FirstUseEver);

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
        if (ImGui::BeginTabItem("Search"))
        {
            showSearchTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }


    ImGui::End();
}
void SubredditsListWindow::showSearchTab()
{
    if(ImGui::InputTextWithHint("##searchsubreddits","Search (hit Enter to search)",&search,
                                ImGuiInputTextFlags_EnterReturnsTrue) && !searchingSubreddits)
    {
        searchSubreddits();
    }
    ImGui::SameLine();
    if(searchingSubreddits)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_SEARCH "##searchSubredditsbutton")) && !searchingSubreddits)
    {
        searchSubreddits();
    }
    if (searchingSubreddits)
    {
        ImGui::PopStyleVar();
    }
    for(auto&& name : searchedNamesList)
    {
        if(ImGui::Selectable(name.sr.c_str(),name.selected,ImGuiSelectableFlags_AllowDoubleClick))
        {
            std::ranges::for_each(searchedNamesList, [](auto&& item){
                item.selected = false;
            });
            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                subredditSignal(name.sr);
            }
            name.selected = true;
        }
    }
}
void SubredditsListWindow::searchSubreddits()
{
    searchedNamesList.clear();
    searchingSubreddits = true;
    if(!searchNamesConnection)
    {
        searchNamesConnection = client->makeRedditSearchNamesClientConnection();
        searchNamesConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                     client_response<names_list> response)
         {
            auto self = weak.lock();
            if(!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setErrorMessage,self,response.body));
                }
                else
                {
                    boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setSearchResultsNames,self,std::move(response.data)));
                }
            }
         });
    }

    searchNamesConnection->search(search,token);
}
void SubredditsListWindow::setSearchResultsNames(names_list names)
{
    std::move(names.begin(), names.end(), std::back_inserter(searchedNamesList));
    searchingSubreddits = false;
}
void SubredditsListWindow::showSubredditsTab()
{
    if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_REFRESH)))
    {
        //this clear is just for show, to tell the user that we're working, doing something
        subscribedSubreddits.clear();
        filteredSubscribedSubreddits.clear();
        userMultis.clear();
        filteredUserMultis.clear();
        loadSubreddits("/subreddits/mine/subscriber?show=all&limit=100",token);
        loadMultis("/api/multi/mine",token);
    }
    ImGui::SameLine();
    if(ImGui::InputTextWithHint("##filtersubreddits","Filter",&subredditsFilter))
    {
        filteredSubscribedSubreddits.clear();
        filteredUserMultis.clear();
        if(subredditsFilter.empty())
        {
            filteredSubscribedSubreddits = subscribedSubreddits;
            filteredUserMultis = userMultis;
        }
        else
        {
            auto comparator = [filter=subredditsFilter](const auto& item){
                return boost::algorithm::to_lower_copy(item.sr.displayName).find(boost::algorithm::to_lower_copy(filter)) != std::string::npos;
            };
            std::ranges::copy_if(subscribedSubreddits,std::back_inserter(filteredSubscribedSubreddits),comparator);
            std::ranges::copy_if(userMultis,std::back_inserter(filteredUserMultis),comparator);
        }
    }

    if(ImGui::TreeNodeEx("Multis",ImGuiTreeNodeFlags_DefaultOpen))
    {
        showMultisNodes();
        ImGui::TreePop();
    }
    if(ImGui::TreeNodeEx("Subscribed",ImGuiTreeNodeFlags_DefaultOpen))
    {
        showSubredditsNodes();
        ImGui::TreePop();
    }
}
void SubredditsListWindow::clearSelections()
{
    auto predicate = [](auto&& item){
        item.selected = false;
    };
    std::ranges::for_each(filteredUserMultis, predicate);
    std::ranges::for_each(filteredSubscribedSubreddits, predicate);
}
void SubredditsListWindow::showMultisNodes()
{
    for(auto&& sr : filteredUserMultis)
    {
        if(ImGui::Selectable(sr.sr.displayName.c_str(),sr.selected,ImGuiSelectableFlags_AllowDoubleClick))
        {
            clearSelections();
            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                subredditSignal(sr.sr.path);
            }
            sr.selected = true;
        }
    }
}
void SubredditsListWindow::showSubredditsNodes()
{
    for(auto&& sr : filteredSubscribedSubreddits)
    {
        if(ImGui::Selectable(sr.sr.displayName.c_str(),sr.selected,ImGuiSelectableFlags_AllowDoubleClick))
        {
            clearSelections();
            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                subredditSignal(sr.sr.displayNamePrefixed);
            }
            sr.selected = true;
        }
    }
}
void SubredditsListWindow::setErrorMessage(std::string message)
{
    errorMessage = std::move(message);
}
void SubredditsListWindow::loadMultis(const std::string& url, const access_token& token)
{
    if(!loadMultisConnection)
    {
        loadMultisConnection = client->makeListingClientConnection();
        loadMultisConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                 client_response<listing> response)
        {
            auto self = weak.lock();
            if(!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&SubredditsListWindow::setErrorMessage,self,response.body));
                }
                else
                {
                    multireddit_list srs;
                    for(const auto& child: response.data.json)
                    {
                        srs.emplace_back(child["data"]);
                    }

                    boost::asio::post(self->uiExecutor,
                                      std::bind(&SubredditsListWindow::setUserMultis,self,std::move(srs)));
                }
            }
        });
    }
    loadMultisConnection->list(url,token);
}
void SubredditsListWindow::setUserMultis(multireddit_list multis)
{
    multireddit frontPage;
    frontPage.displayName = "Front Page";
    frontPage.path = "";
    userMultis.emplace_back(std::move(frontPage));
    multireddit all;
    all.displayName = "All";
    all.path = "/r/all";
    userMultis.emplace_back(std::move(all));
    std::move(multis.begin(), multis.end(), std::back_inserter(userMultis));

    multireddit saved;
    saved.displayName = "saved";
    saved.path = "/user/"+username+"/saved";
    userMultis.emplace_back(std::move(saved));

    multireddit random;
    random.displayName = "random";
    random.path = "/r/random";
    userMultis.emplace_back(std::move(random));

    if(showRandomNSFW)
    {
        multireddit randomNsfw;
        randomNsfw.displayName = "random nsfw";
        randomNsfw.path = "/r/randnsfw";
        userMultis.emplace_back(std::move(randomNsfw));
    }

    filteredUserMultis = userMultis;
}
void SubredditsListWindow::loadSubscribedSubreddits(subreddit_list srs)
{
    if(!srs.empty())
    {
        auto url = fmt::format("/subreddits/mine/subscriber?show=all&limit=100&after={}&count={}",srs.back().name,srs.size());
        loadSubreddits(url,token);
    }
    std::move(srs.begin(), srs.end(), std::back_inserter(subscribedSubreddits));
    filteredSubscribedSubreddits = subscribedSubreddits;
}
void SubredditsListWindow::loadSubreddits(const std::string& url, const access_token& token)
{
    if(!loadSubscribedSubredditsConnection)
    {
        loadSubscribedSubredditsConnection = client->makeListingClientConnection();
        loadSubscribedSubredditsConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                                     client_response<listing> response)
         {
            auto self = weak.lock();
            if(!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setErrorMessage,self,ec.message()));
            }
            else
            {
                if(response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::setErrorMessage,self,response.body));
                }
                else
                {
                    auto listingChildren = response.data.json["data"]["children"];
                    subreddit_list srs;
                    for(const auto& child: listingChildren)
                    {
                        srs.emplace_back(child["data"]);
                    }

                    boost::asio::post(self->uiExecutor,std::bind(&SubredditsListWindow::loadSubscribedSubreddits,self,std::move(srs)));
                }
            }
        });
    }
    loadSubscribedSubredditsConnection->list(url,token);
}

