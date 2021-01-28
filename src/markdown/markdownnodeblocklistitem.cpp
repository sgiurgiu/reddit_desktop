#include "markdownnodeblocklistitem.h"
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "utils.h"
#include "markdownnodetext.h"
#include "markdownnodeblockorderedlist.h"

MarkdownNodeBlockListItem::MarkdownNodeBlockListItem(const MD_BLOCK_LI_DETAIL* detail):
    detail(*detail)
{

}
void MarkdownNodeBlockListItem::Render()
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    NodeType parentType = parent->GetNodeType();
    if(parentType == NodeType::BlockUnorderedList)
    {
        ImGui::Bullet();
    }
    else
    {
        std::string text = " ";
        if(parentType == NodeType::BlockOrderedList)
        {
            text = std::to_string(index)+".";
        }
        const float line_height = 0.0f;//ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0))
        {
            ImGui::SameLine(0, style.FramePadding.x * 2);
            return;
        }
        auto size = ImGui::CalcTextSize(text.c_str());
        size -= ImVec2(g.FontSize/2.f,g.FontSize);
        ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        window->DrawList->AddText(bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, line_height * 0.5f) - size ,
                                  text_col, text.c_str());
        ImGui::SameLine(0, style.FramePadding.x * 2.0f);
    }
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::Dummy(ImVec2(0.f,0.f));
}
