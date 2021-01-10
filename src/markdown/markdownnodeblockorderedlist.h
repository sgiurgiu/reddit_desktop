#ifndef MARKDOWNNODEBLOCKORDEREDLIST_H
#define MARKDOWNNODEBLOCKORDEREDLIST_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockOrderedList : public MarkdownNode
{
public:
    MarkdownNodeBlockOrderedList(const MD_BLOCK_OL_DETAIL*);
    void Render() override;
private:
    MD_BLOCK_OL_DETAIL detail;
};

#endif // MARKDOWNNODEORDEREDLIST_H
