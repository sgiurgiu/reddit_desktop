#ifndef MARKDOWNNODEBLOCKUNORDEREDLIST_H
#define MARKDOWNNODEBLOCKUNORDEREDLIST_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockUnorderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockUnorderedList(char mark);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockUnorderedList;
    }
    char GetMarkChar() const
    {
        return mark;
    }
private:
    char mark;
};

#endif // MARKDOWNNODEBLOCKUNORDEREDLIST_H
