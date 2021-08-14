#ifndef MARKDOWNNODETABLECELLHEAD_H
#define MARKDOWNNODETABLECELLHEAD_H

#include "markdownnode.h"

class MarkdownNodeTableCellHead : public MarkdownNode
{
public:
    MarkdownNodeTableCellHead(NodeAlign align);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableCellHead;
    }
private:
    NodeAlign align;
};

#endif // MARKDOWNNODETABLECELLHEAD_H
