#ifndef MARKDOWNNODEEMPHASIS_H
#define MARKDOWNNODEEMPHASIS_H

#include "markdownnode.h"

class MarkdownNodeEmphasis : public MarkdownNode
{
public:
    MarkdownNodeEmphasis();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Emphasis;
    }
};

#endif // MARKDOWNNODEEMPHASIS_H
