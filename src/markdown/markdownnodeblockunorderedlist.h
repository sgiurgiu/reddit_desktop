#ifndef MARKDOWNNODEBLOCKUNORDEREDLIST_H
#define MARKDOWNNODEBLOCKUNORDEREDLIST_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockUnorderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockUnorderedList(const MD_BLOCK_UL_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockUnorderedList;
    }
    char GetMarkChar() const
    {
        return detail.mark;
    }
private:
    MD_BLOCK_UL_DETAIL detail;
};

#endif // MARKDOWNNODEBLOCKUNORDEREDLIST_H
