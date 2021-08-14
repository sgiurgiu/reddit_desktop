#ifndef MARKDOWNNODEBLOCKLISTITEM_H
#define MARKDOWNNODEBLOCKLISTITEM_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockListItem : public MarkdownNode
{
public:
    MarkdownNodeBlockListItem();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockListItem;
    }
    void SetIndex(int index)
    {
        this->index = index;
    }
private:
    int index;
};

#endif // MARKDOWNNODEBLOCKLISTITEM_H
