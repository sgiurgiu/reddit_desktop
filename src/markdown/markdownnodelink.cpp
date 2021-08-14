#include "markdownnodelink.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "utils.h"
#include "markdownnodetext.h"

namespace
{
    static const ImVec4 linkColor(0.5f,0.5f,1.f,1.f);
    static const ImVec4 tooltipColor(0.9f, 0.9f, 0.9f, 1.f);
}
MarkdownNodeLink::MarkdownNodeLink(std::string href, std::string title):
    href(std::move(href)),title(std::move(title))
{
}
void MarkdownNodeLink::AddChild(std::unique_ptr<MarkdownNode> child)
{
    child->addIndividualComponentSignal([this]()
    {
        componentLinkHandling();
    });
    MarkdownNode::AddChild(std::move(child));
}
void MarkdownNodeLink::componentLinkHandling()
{
    //Underline it
    {
        auto rectMax = ImGui::GetItemRectMax();
        auto rectMin = ImGui::GetItemRectMin();
        rectMin.y = rectMax.y;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(linkColor));
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::BeginTooltip();
        ImGui::PushStyleColor(ImGuiCol_Text, tooltipColor);
        ImGui::TextUnformatted(href.c_str());
        ImGui::PopStyleColor();
        ImGui::EndTooltip();
    }
    if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        auto fixedHref = href;
        if (href.starts_with("/r/"))
        {
            fixedHref = "https://reddit.com"+href;
        }
        Utils::openInBrowser(fixedHref);
    }
}
void MarkdownNodeLink::Render()
{
    for(const auto& child: children)
    {        
        ImGui::PushStyleColor(ImGuiCol_Text, linkColor);
        child->Render();
        ImGui::PopStyleColor();

        if(parent && parent->GetNodeType() != NodeType::TableCell && parent->GetNodeType() != NodeType::TableCellHead)
        {
            ImGui::SameLine();
        }
    }
}
