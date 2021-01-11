#include "markdownnodeblockorderedlist.h"
#include "markdownnodeblocklistitem.h"
#include <imgui.h>

MarkdownNodeBlockOrderedList::MarkdownNodeBlockOrderedList(const MD_BLOCK_OL_DETAIL* detail):
    detail(*detail)
{    
}
void MarkdownNodeBlockOrderedList::Render()
{
    ImGui::Indent();

    int index = 1;
    for(const auto& child : children)
    {
        if(child->GetNodeType() == NodeType::BlockListItem)
        {
            auto item = dynamic_cast<MarkdownNodeBlockListItem*>(child.get());
            if(item)
            {
                item->SetIndex(index);
            }
        }
        child->Render();
        index++;
    }
    ImGui::Unindent();
}
