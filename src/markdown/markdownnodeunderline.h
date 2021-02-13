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
    virtual void AddChild(std::unique_ptr<MarkdownNode> child) override;
private:
    void componentLinkHandling();
};

#endif // MARKDOWNNODEUNDERLINE_H
