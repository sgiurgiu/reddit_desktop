#ifndef MARKDOWNNODETABLECELL_H
#define MARKDOWNNODETABLECELL_H

#include "markdownnode.h"

class MarkdownNodeTableCell : public MarkdownNode
{
public:
    MarkdownNodeTableCell(NodeAlign align);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableCell;
    }
private:
    NodeAlign align;
};

#endif // MARKDOWNNODETABLECELL_H
