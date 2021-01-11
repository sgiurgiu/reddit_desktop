#ifndef MARKDOWNNODEBREAK_H
#define MARKDOWNNODEBREAK_H

#include "markdownnode.h"

class MarkdownNodeBreak : public MarkdownNode
{
public:
    MarkdownNodeBreak();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Break;
    }
};

#endif // MARKDOWNNODEBREAK_H
