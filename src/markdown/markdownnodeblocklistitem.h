#ifndef MARKDOWNNODEBLOCKLISTITEM_H
#define MARKDOWNNODEBLOCKLISTITEM_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockListItem : public MarkdownNode
{
public:
    MarkdownNodeBlockListItem(const MD_BLOCK_LI_DETAIL*);
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
    MD_BLOCK_LI_DETAIL detail;
    int index;
};

#endif // MARKDOWNNODEBLOCKLISTITEM_H
