#include "markdownnodetablerow.h"
#include <imgui.h>

MarkdownNodeTableRow::MarkdownNodeTableRow()
{

}
void MarkdownNodeTableRow::Render()
{
    if(parent && parent->GetNodeType() == NodeType::TableHead)
    {
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    }
    else
    {
        ImGui::TableNextRow();
    }
    for(const auto& child : children)
    {
        child->Render();
    } 
}
