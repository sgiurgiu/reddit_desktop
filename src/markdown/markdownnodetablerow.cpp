#include "markdownnodetablerow.h"
#include <imgui.h>

MarkdownNodeTableRow::MarkdownNodeTableRow()
{

}
void MarkdownNodeTableRow::Render()
{

    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::Separator();
}
