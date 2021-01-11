#include "markdownnodeblockunorderedlist.h"
#include <imgui.h>

MarkdownNodeBlockUnorderedList::MarkdownNodeBlockUnorderedList(const MD_BLOCK_UL_DETAIL* detail):
    detail(*detail)
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
