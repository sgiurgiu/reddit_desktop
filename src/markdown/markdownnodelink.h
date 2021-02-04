#ifndef MARKDOWNNODELINK_H
#define MARKDOWNNODELINK_H

#include "markdownnode.h"
#include <md4c.h>
#include <string>

class MarkdownNodeLink : public MarkdownNode
{
public:
    MarkdownNodeLink(const MD_SPAN_A_DETAIL*);
    void Render() override;
    virtual void AddChild(std::unique_ptr<MarkdownNode> child) override;
    NodeType GetNodeType() const override
    {
        return NodeType::Link;
    }
private:
    void componentLinkHandling();
private:
    std::string href;
    std::string title;
};

#endif // MARKDOWNNODELINK_H
