#ifndef MARKDOWNNODETABLEROW_H
#define MARKDOWNNODETABLEROW_H

#include "markdownnode.h"

class MarkdownNodeTableRow : public MarkdownNode
{
public:
    MarkdownNodeTableRow();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableRow;
    }
};

#endif // MARKDOWNNODETABLEROW_H
