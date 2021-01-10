#include "markdownnode.h"

MarkdownNode::MarkdownNode()
{
}

void MarkdownNode::AddChild(std::unique_ptr<MarkdownNode> child)
{
    child->parent = this;
    children.push_back(std::move(child));
}
MarkdownNode* MarkdownNode::GetParent() const
{
    return parent;
}
