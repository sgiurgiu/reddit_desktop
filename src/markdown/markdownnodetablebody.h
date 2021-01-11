#ifndef MARKDOWNNODETABLEBODY_H
#define MARKDOWNNODETABLEBODY_H

#include "markdownnode.h"

class MarkdownNodeTableBody : public MarkdownNode
{
public:
    MarkdownNodeTableBody();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TableBody;
    }
    size_t GetNumberOfCells() const;
};

#endif // MARKDOWNNODETABLEBODY_H
