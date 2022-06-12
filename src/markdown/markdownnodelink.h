#ifndef MARKDOWNNODELINK_H
#define MARKDOWNNODELINK_H

#include "markdownnode.h"
#include <string>

class MarkdownNodeLink : public MarkdownNode
{
public:
    MarkdownNodeLink(std::string href, std::string title);
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
