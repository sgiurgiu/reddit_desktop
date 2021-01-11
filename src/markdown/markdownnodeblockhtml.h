#ifndef MARKDOWNNODEBLOCKHTML_H
#define MARKDOWNNODEBLOCKHTML_H

#include "markdownnode.h"

class MarkdownNodeBlockHtml : public MarkdownNode
{
public:
    MarkdownNodeBlockHtml();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockHtml;
    }
};

#endif // MARKDOWNNODEBLOCKHTML_H
