#include "markdownnodetableheader.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeTableHeader::MarkdownNodeTableHeader()
{

}
void MarkdownNodeTableHeader::Render()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::PopFont();
}

size_t MarkdownNodeTableHeader::GetNumberOfCells() const
{
    if(children.empty()) return 0;
    if(children.front()->GetNodeType() == NodeType::TableRow)
    {
        return children.front()->GetChildrenCount();
    }
    return 0;
}
