#ifndef MARKDOWNNODEBLOCKUNORDEREDLIST_H
#define MARKDOWNNODEBLOCKUNORDEREDLIST_H

#include "markdownnode.h"

class MarkdownNodeBlockUnorderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockUnorderedList();
    void Render() override;
};

#endif // MARKDOWNNODEBLOCKUNORDEREDLIST_H
