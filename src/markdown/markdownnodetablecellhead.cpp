#include "markdownnodetablecellhead.h"
#include <imgui.h>

MarkdownNodeTableCellHead::MarkdownNodeTableCellHead(const MD_BLOCK_TD_DETAIL* detail):
    detail(*detail)
{

}
void MarkdownNodeTableCellHead::Render()
{    
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::NextColumn();
}
