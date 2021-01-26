#include "subredditslistwindow.h"
#include <fmt/format.h>
#include "imgui.h"
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"

namespace
{
constexpr auto SUBREDDITS_WINDOW_TITLE = "My Subreddits";
}

SubredditsListWindow::SubredditsListWindow(const access_token& token,
                                           RedditClientProducer* client,
                                           const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor)
{
}
void SubredditsListWindow::loadSubredditsList()
{
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

        if (ImGui::BeginTabItem("Multireddits"))
        {
            showMultisTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}
void SubredditsListWindow::showMultisTab()
{
    for(const auto& sr : userMultis)
    {
        if(ImGui::Selectable(sr.displayName.c_str()))
        {
            subredditSignal(sr.path);
        }
    }
}
void SubredditsListWindow::showSubredditsTab()
{
    if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_REFRESH)))
    {
        //this clear is just for show, to tell the user that we're working, doing something
        subscribedSubreddits.clear();
        unsortedSubscribedSubreddits.clear();
        frontPageSubredditSelected = false;
        allSubredditSelected = false;
        loadSubreddits("/subreddits/mine/subscriber?show=all&limit=100",token);
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
            subredditSignal("");
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
            subredditSignal("r/all");
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
    auto multisConnection = client->makeListingClientConnection();
    multisConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
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
     multisConnection->list(url,token);
}
void SubredditsListWindow::setUserMultis(multireddit_list multis)
{
    userMultis = std::move(multis);
}
void SubredditsListWindow::loadSubscribedSubreddits(subreddit_list srs)
{
    if(!srs.empty())
    {
        auto url = fmt::format("/subreddits/mine/subscriber?show=all&limit=100&after={}&count={}",srs.back().name,srs.size());
        loadSubreddits(url,token);
    }
    std::move(srs.begin(), srs.end(), std::back_inserter(unsortedSubscribedSubreddits));
    subscribedSubreddits = unsortedSubscribedSubreddits;
}
void SubredditsListWindow::loadSubreddits(const std::string& url, const access_token& token)
{
    auto subscribedSubredditsConnection = client->makeListingClientConnection();
    subscribedSubredditsConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
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
     subscribedSubredditsConnection->list(url,token);
}

