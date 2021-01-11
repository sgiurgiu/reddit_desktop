#include "markdownnodehead.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeHead::MarkdownNodeHead(const MD_BLOCK_H_DETAIL* detail):
    detail(*detail)
{
}
void MarkdownNodeHead::Render()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium_Big)]);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.f,0.f));
}
