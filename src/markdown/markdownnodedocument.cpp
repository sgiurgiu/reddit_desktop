#include "markdownnodedocument.h"

MarkdownNodeDocument::MarkdownNodeDocument()
{

}
void MarkdownNodeDocument::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
}
