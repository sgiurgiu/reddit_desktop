#include "markdownnodetablecellhead.h"
#include <imgui.h>

MarkdownNodeTableCellHead::MarkdownNodeTableCellHead(NodeAlign align):
    align(align)
{

}
void MarkdownNodeTableCellHead::Render()
{    
    ImGui::TableNextColumn();
    for(const auto& child : children)
    {
        child->Render();
    }
}
