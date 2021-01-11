#include "markdownnodeemphasis.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeEmphasis::MarkdownNodeEmphasis()
{

}
void MarkdownNodeEmphasis::Render()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Italic)]);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::PopFont();
}
