#include "aboutwindow.h"
#include "imgui.h"
#include "utils.h"
#include "fonts/IconsFontAwesome4.h"
#include "markdown/markdownnodetext.h"
#include "markdown/markdownnodelink.h"

#include <string_view>

namespace
{
constexpr auto ABOUT_WINDOW_POPUP_TITLE = "About Reddit Desktop";
constexpr std::string_view LINK_TEXT = "Github";
constexpr std::string_view FLUFF_TEXT = "Get the latest version from ";
constexpr std::string_view GLP_LINK_TEXT = "GPLv3";
constexpr std::string_view LICENSE_TEXT = "Licensed under ";
}

AboutWindow::AboutWindow():
    applicationLogo(Utils::GetApplicationIcon())
{
    auto githubLink = std::make_unique<MarkdownNodeLink>("http://github.com/sgiurgiu/reddit_desktop","Github repository");
    githubLink->AddChild(std::make_unique<MarkdownNodeText>(LINK_TEXT.data(),LINK_TEXT.size()));

    githubParagraph.AddChild(std::make_unique<MarkdownNodeText>(FLUFF_TEXT.data(),FLUFF_TEXT.size()));
    githubParagraph.AddChild(std::move(githubLink));

    auto gplLink = std::make_unique<MarkdownNodeLink>("https://www.gnu.org/licenses/gpl-3.0.en.html","GPLv3");
    gplLink->AddChild(std::make_unique<MarkdownNodeText>(GLP_LINK_TEXT.data(),GLP_LINK_TEXT.size()));

    licenseParagraph.AddChild(std::make_unique<MarkdownNodeText>(LICENSE_TEXT.data(),LICENSE_TEXT.size()));
    licenseParagraph.AddChild(std::move(gplLink));
}

void AboutWindow::setWindowShowing(bool flag)
{
    windowShowing = flag;
}
void AboutWindow::showAboutWindow(int appFrameWidth,int appFrameHeight)
{
    if(windowShowing)
    {
        ImGui::OpenPopup(ABOUT_WINDOW_POPUP_TITLE);
        windowShowing = false;
    }
    ImVec2 size(470.f, 180.f + ImGui::GetTextLineHeightWithSpacing()*3.f);
    ImGui::SetNextWindowSize(size,ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2((appFrameWidth-size.x)/2.f,(appFrameHeight-size.y)/2.f),ImGuiCond_Once);
    if (ImGui::BeginPopupModal(ABOUT_WINDOW_POPUP_TITLE,nullptr,ImGuiWindowFlags_None))
    {
        ImGui::Image((void*)(intptr_t)applicationLogo->textureId,
                           ImVec2(180,180),ImVec2(0, 0),ImVec2(1,1));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s",REDDITDESKTOP_STRING);
        ImGui::Text("%s",reinterpret_cast<const char*>("Copyright " ICON_FA_COPYRIGHT " 2021, Sergiu Giurgiu"));
        githubParagraph.Render();
        licenseParagraph.Render();
        ImGui::EndGroup();
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetTextLineHeightWithSpacing()*1.5f);
        if (ImGui::Button("OK", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
