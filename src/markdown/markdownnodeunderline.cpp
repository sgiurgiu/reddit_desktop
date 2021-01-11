#include "markdownnodeunderline.h"

MarkdownNodeUnderline::MarkdownNodeUnderline()
{

}
void MarkdownNodeUnderline::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }

}
