#include "subredditwindow.h"
#include <imgui.h>

#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "spinner/spinner.h"

SubredditWindow::SubredditWindow(int id, const std::string& subreddit):id(id),subreddit(subreddit)
{

}
void SubredditWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    std::string windowName =subreddit +"##"+ std::to_string(id);
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!windowOpen) return;
    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_MenuBar))
    {
        ImGui::End();
        return;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)) && ImGui::GetIO().KeyCtrl)
    {
        windowOpen = false;
    }

    showWindowMenu();

    ImGuiStyle& style = ImGui::GetStyle();
    auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x + style.ItemInnerSpacing.x;
    auto availableWidth = ImGui::GetContentRegionAvailWidth();



    ImGui::End();
}

void SubredditWindow::showWindowMenu()
{
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)))
    {

    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Statistics","Ctrl+S",false))
            {

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Refresh","F5",false))
            {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
