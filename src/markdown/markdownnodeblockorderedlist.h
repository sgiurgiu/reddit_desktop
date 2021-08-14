#ifndef MARKDOWNNODEBLOCKORDEREDLIST_H
#define MARKDOWNNODEBLOCKORDEREDLIST_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockOrderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockOrderedList(unsigned start, char mark);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockOrderedList;
    }
    unsigned GetStartIndex() const
    {
        return startIndex;
    }
private:
    unsigned startIndex;
    char mark;
};

#endif // MARKDOWNNODEORDEREDLIST_H
