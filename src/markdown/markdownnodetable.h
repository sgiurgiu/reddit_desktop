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
    void SetColumns(int columns)
    {
        this->columns = columns;
    }
private:
    int columns = -1;
};

#endif // MARKDOWNNODETABLE_H
