#include "markdownnodeblockunorderedlist.h"
#include <imgui.h>

MarkdownNodeBlockUnorderedList::MarkdownNodeBlockUnorderedList(char mark):
    mark(mark)
{

}
void MarkdownNodeBlockUnorderedList::Render()
{
    ImGui::Indent();
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::Unindent();
}
