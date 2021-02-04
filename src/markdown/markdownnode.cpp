#include "markdownnode.h"
#include "markdownnodetext.h"

MarkdownNode::MarkdownNode()
{
}

void MarkdownNode::AddChild(std::unique_ptr<MarkdownNode> child)
{
    if(!children.empty() && (children.back()->GetNodeType() == NodeType::Text &&
                             child->GetNodeType() == NodeType::Text))
    {
        auto childText = dynamic_cast<MarkdownNodeText*>(child.get());
        auto lastChildText = dynamic_cast<MarkdownNodeText*>(children.back().get());

        lastChildText->AddText(childText->GetText());
    }
    else
    {
        child->parent = this;
        if(!individualComponentSignal.empty())
        {
            child->individualComponentSignal.connect([this](){
                individualComponentSignal();
            });
        }
        children.push_back(std::move(child));
    }

}
MarkdownNode* MarkdownNode::GetParent() const
{
    return parent;
}
