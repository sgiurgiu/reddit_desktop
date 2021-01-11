#ifndef MARKDOWNNODEUNDERLINE_H
#define MARKDOWNNODEUNDERLINE_H

#include "markdownnode.h"

class MarkdownNodeUnderline : public MarkdownNode
{
public:
    MarkdownNodeUnderline();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Underline;
    }
};

#endif // MARKDOWNNODEUNDERLINE_H
