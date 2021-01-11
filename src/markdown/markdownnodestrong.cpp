#include "markdownnodestrong.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeStrong::MarkdownNodeStrong()
{

}
void MarkdownNodeStrong::Render()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::PopFont();
}
