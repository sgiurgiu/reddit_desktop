#ifndef MARKDOWNNODESTRIKE_H
#define MARKDOWNNODESTRIKE_H

#include "markdownnode.h"

class MarkdownNodeStrike : public MarkdownNode
{
public:
    MarkdownNodeStrike();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Strike;
    }
};

#endif // MARKDOWNNODESTRIKE_H
