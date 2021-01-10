#ifndef MARKDOWNNODEBLOCKQUOTE_H
#define MARKDOWNNODEBLOCKQUOTE_H

#include "markdownnode.h"

class MarkdownNodeBlockQuote : public MarkdownNode
{
public:
    MarkdownNodeBlockQuote();
    void Render() override;
};

#endif // MARKDOWNNODEBLOCKQUOTE_H
