#include "markdownnodeblockquote.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeBlockQuote::MarkdownNodeBlockQuote()
{

}

void MarkdownNodeBlockQuote::Render()
{
    ImGui::Indent();
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_LightItalic)]);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::PopFont();
    ImGui::Unindent();
}
