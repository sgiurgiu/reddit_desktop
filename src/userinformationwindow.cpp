#include "userinformationwindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"

namespace
{
constexpr auto USER_INFO_WINDOW_TITLE = "Messages and information";

}

UserInformationWindow::UserInformationWindow(const access_token& token,
                                             RedditClient* client,
                                             const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor)
{

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


    ImGui::End();
}
