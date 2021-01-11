#ifndef MARKDOWNNODEBLOCKQUOTE_H
#define MARKDOWNNODEBLOCKQUOTE_H

#include "markdownnode.h"

class MarkdownNodeBlockQuote : public MarkdownNode
{
public:
    MarkdownNodeBlockQuote();
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockQuote;
    }
};

#endif // MARKDOWNNODEBLOCKQUOTE_H
