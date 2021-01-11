#include "markdownnodetablebody.h"

MarkdownNodeTableBody::MarkdownNodeTableBody()
{

}
void MarkdownNodeTableBody::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
}
size_t MarkdownNodeTableBody::GetNumberOfCells() const
{
    if(children.empty()) return 0;
    if(children.front()->GetNodeType() == NodeType::TableRow)
    {
        return children.front()->GetChildrenCount();
    }
    return 0;
}
