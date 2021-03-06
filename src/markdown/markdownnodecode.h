#ifndef MARKDOWNNODECODE_H
#define MARKDOWNNODECODE_H

#include "markdownnode.h"

class MarkdownNodeCode : public MarkdownNode
{
public:
    MarkdownNodeCode();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Code;
    }
};

#endif // MARKDOWNNODECODE_H
