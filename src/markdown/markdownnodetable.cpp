#include "markdownnodetable.h"
#include <imgui.h>
#include "markdownnodetablebody.h"
#include "markdownnodetableheader.h"

MarkdownNodeTable::MarkdownNodeTable()
{

}
void MarkdownNodeTable::Render()
{
    if(colums < 0)
    {
        if(children.empty()) return;
        if(children.front()->GetNodeType() == NodeType::TableHead)
        {
            auto head = dynamic_cast<MarkdownNodeTableHeader*>(children.front().get());
            if(head)
            {
                colums = (int)head->GetNumberOfCells();
            }
        }
        else if(children.front()->GetNodeType() == NodeType::TableBody)
        {
            auto body = dynamic_cast<MarkdownNodeTableBody*>(children.front().get());
            if(body)
            {
                colums = (int)body->GetNumberOfCells();
            }
        }
    }

    ImGui::Separator();
    ImGui::Columns(colums, nullptr, true);
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::Columns(1);
}
