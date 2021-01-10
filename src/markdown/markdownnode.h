#ifndef MARKDOWNNODE_H
#define MARKDOWNNODE_H

#include <memory>
#include <vector>

class MarkdownNode
{
public:
    MarkdownNode();
    virtual void Render() = 0;
    void AddChild(std::unique_ptr<MarkdownNode> child);
    MarkdownNode* GetParent() const;
protected:
    MarkdownNode* parent = nullptr;
    std::vector<std::unique_ptr<MarkdownNode>> children;
};

#endif // MARKDOWNNODE_H
