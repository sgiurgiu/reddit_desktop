#ifndef MARKDOWNNODETABLE_H
#define MARKDOWNNODETABLE_H

#include "markdownnode.h"

class MarkdownNodeTable : public MarkdownNode
{
public:
    MarkdownNodeTable();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Table;
    }
private:
    int colums = -1;
};

#endif // MARKDOWNNODETABLE_H
