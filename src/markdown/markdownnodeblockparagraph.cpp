#include "markdownnodeblockparagraph.h"
#include <imgui.h>
#include "utils.h"

MarkdownNodeBlockParagraph::MarkdownNodeBlockParagraph()
{

}
void MarkdownNodeBlockParagraph::Render()
{
    /*
        if(entering)
        {
            if (node->parent && node->parent->type == CMARK_NODE_ITEM &&
                node->prev == NULL)
            {
              // no blank line or .PP
            }
            else
            {
              ImGui::Dummy(ImVec2(0.f,0.f));
            }
        }
        else
        {
            ImGui::Dummy(ImVec2(0.f,0.f));
        }
*/
    if(parent && parent->GetNodeType() != NodeType::BlockListItem)
    {
        ImGui::Dummy(ImVec2(0.f,0.f));
    }
    for(const auto& child : children)
    {
        child->Render();
    }
    ImGui::Dummy(ImVec2(0.f,0.f));
}
