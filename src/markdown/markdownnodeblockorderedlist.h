#ifndef MARKDOWNNODEBLOCKORDEREDLIST_H
#define MARKDOWNNODEBLOCKORDEREDLIST_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockOrderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockOrderedList(const MD_BLOCK_OL_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockOrderedList;
    }
    unsigned GetStartIndex() const
    {
        return detail.start;
    }
private:
    MD_BLOCK_OL_DETAIL detail;
};

#endif // MARKDOWNNODEORDEREDLIST_H
