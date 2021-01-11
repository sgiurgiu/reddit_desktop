#include "markdownnodetablecell.h"
#include <imgui.h>


MarkdownNodeTableCell::MarkdownNodeTableCell(const MD_BLOCK_TD_DETAIL* detail):
    detail(*detail)
{

}
void MarkdownNodeTableCell::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::NextColumn();
}
