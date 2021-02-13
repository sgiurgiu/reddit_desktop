#include "markdownnodeunderline.h"
#include <imgui.h>
#include <imgui_internal.h>

MarkdownNodeUnderline::MarkdownNodeUnderline()
{

}
void MarkdownNodeUnderline::AddChild(std::unique_ptr<MarkdownNode> child)
{
    child->addIndividualComponentSignal([this]()
    {
        componentLinkHandling();
    });
    MarkdownNode::AddChild(std::move(child));
}
void MarkdownNodeUnderline::componentLinkHandling()
{
    //underline it
    {
        auto rectMax = ImGui::GetItemRectMax();
        auto rectMin = ImGui::GetItemRectMin();
        rectMin.y = rectMax.y;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(ImGuiCol_Text));
    }
}

void MarkdownNodeUnderline::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
}
