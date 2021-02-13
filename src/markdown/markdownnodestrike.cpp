#include "markdownnodestrike.h"
#include <imgui.h>
#include <imgui_internal.h>

MarkdownNodeStrike::MarkdownNodeStrike()
{

}
void MarkdownNodeStrike::AddChild(std::unique_ptr<MarkdownNode> child)
{
    child->addIndividualComponentSignal([this]()
    {
        componentLinkHandling();
    });
    MarkdownNode::AddChild(std::move(child));
}
void MarkdownNodeStrike::componentLinkHandling()
{
    //strike it
    {
        auto rectMax = ImGui::GetItemRectMax();
        auto rectMin = ImGui::GetItemRectMin();
        rectMin.y += std::abs(rectMax.y-rectMin.y) / 2.f;
        rectMax.y = rectMin.y;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(ImGuiCol_Text));
    }
}

void MarkdownNodeStrike::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
}
