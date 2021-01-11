#ifndef MARKDOWNNODETABLECELLHEAD_H
#define MARKDOWNNODETABLECELLHEAD_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeTableCellHead : public MarkdownNode
{
public:
    MarkdownNodeTableCellHead(const MD_BLOCK_TD_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableCellHead;
    }
private:
    MD_BLOCK_TD_DETAIL detail;
};

#endif // MARKDOWNNODETABLECELLHEAD_H
