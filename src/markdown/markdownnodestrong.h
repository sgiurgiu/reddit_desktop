#ifndef MARKDOWNNODESTRONG_H
#define MARKDOWNNODESTRONG_H

#include "markdownnode.h"

class MarkdownNodeStrong : public MarkdownNode
{
public:
    MarkdownNodeStrong();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Strong;
    }
};

#endif // MARKDOWNNODESTRONG_H
