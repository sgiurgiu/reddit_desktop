#include "markdownnodetablecell.h"
#include <imgui.h>


MarkdownNodeTableCell::MarkdownNodeTableCell(NodeAlign align):
    align(align)
{

}
void MarkdownNodeTableCell::Render()
{
    ImGui::TableNextColumn();
    for(const auto& child : children)
    {
        child->Render();
    }
}
