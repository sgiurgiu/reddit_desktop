#ifndef MARKDOWNNODETABLEHEADER_H
#define MARKDOWNNODETABLEHEADER_H

#include "markdownnode.h"

class MarkdownNodeTableHeader : public MarkdownNode
{
public:
    MarkdownNodeTableHeader();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableHead;
    }
    size_t GetNumberOfCells() const;
};

#endif // MARKDOWNNODETABLEHEADER_H
