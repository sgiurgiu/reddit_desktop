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
    virtual void AddChild(std::unique_ptr<MarkdownNode> child) override;
private:
    void componentLinkHandling();

};

#endif // MARKDOWNNODESTRIKE_H
