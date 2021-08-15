#ifndef MARKDOWNNODEHEAD_H
#define MARKDOWNNODEHEAD_H

#include "markdownnode.h"

class MarkdownNodeHead : public MarkdownNode
{
public:
    MarkdownNodeHead(unsigned level);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Head;
    }
private:
    unsigned level;
};

#endif // MARKDOWNNODEHEAD_H
