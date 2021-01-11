#ifndef MARKDOWNNODETABLECELL_H
#define MARKDOWNNODETABLECELL_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeTableCell : public MarkdownNode
{
public:
    MarkdownNodeTableCell(const MD_BLOCK_TD_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableCell;
    }
private:
    MD_BLOCK_TD_DETAIL detail;
};

#endif // MARKDOWNNODETABLECELL_H
