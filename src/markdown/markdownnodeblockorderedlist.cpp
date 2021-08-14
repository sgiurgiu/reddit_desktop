#include "markdownnodeblockorderedlist.h"
#include "markdownnodeblocklistitem.h"
#include <imgui.h>

MarkdownNodeBlockOrderedList::MarkdownNodeBlockOrderedList(unsigned start, char mark):
    startIndex(start),mark(mark)
{    
}
void MarkdownNodeBlockOrderedList::Render()
{
    ImGui::Indent();

    auto index = GetStartIndex();
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
