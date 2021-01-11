#include "markdownnodetextentity.h"

MarkdownNodeTextEntity::MarkdownNodeTextEntity(const char* text,size_t size):text(text,size)
{

}
void MarkdownNodeTextEntity::Render()
{
    for(const auto& child : children)
    {
        child->Render();
    }
}
