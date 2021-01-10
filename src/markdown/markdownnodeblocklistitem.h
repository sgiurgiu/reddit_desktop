#ifndef MARKDOWNNODEBLOCKLISTITEM_H
#define MARKDOWNNODEBLOCKLISTITEM_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockListItem : public MarkdownNode
{
public:
    MarkdownNodeBlockListItem(const MD_BLOCK_LI_DETAIL*);
    void Render() override;
private:
    MD_BLOCK_LI_DETAIL detail;
};

#endif // MARKDOWNNODEBLOCKLISTITEM_H
