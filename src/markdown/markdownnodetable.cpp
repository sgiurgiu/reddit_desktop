#include "markdownnodetable.h"
#include <imgui.h>
#include "markdownnodetablebody.h"
#include "markdownnodetableheader.h"

MarkdownNodeTable::MarkdownNodeTable()
{

}
void MarkdownNodeTable::Render()
{
    if(columns < 0)
    {
        if(children.empty()) return;
        if(children.front()->GetNodeType() == NodeType::TableHead)
        {
            auto head = dynamic_cast<MarkdownNodeTableHeader*>(children.front().get());
            if(head)
            {
                columns = (int)head->GetNumberOfCells();
            }
        }
        else if(children.front()->GetNodeType() == NodeType::TableBody)
        {
            auto body = dynamic_cast<MarkdownNodeTableBody*>(children.front().get());
            if(body)
            {
                columns = (int)body->GetNumberOfCells();
            }
        }
    }

    if(ImGui::BeginTable("markdowntable",columns,ImGuiTableFlags_Borders))
    {
        for(const auto& child : children)
        {
            child->Render();
        }
        ImGui::EndTable();
    }
}
