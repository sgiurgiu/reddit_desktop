#include "markdownnodeblockhtml.h"

MarkdownNodeBlockHtml::MarkdownNodeBlockHtml()
{

}
void MarkdownNodeBlockHtml::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
    //std::string text((const char*)node->as.literal.data,node->as.literal.len);
    //ImGui::TextUnformatted(text.c_str());//ImGui::SameLine();
}
