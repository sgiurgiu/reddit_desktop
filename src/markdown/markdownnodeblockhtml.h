#ifndef MARKDOWNNODEBLOCKHTML_H
#define MARKDOWNNODEBLOCKHTML_H

#include "markdownnode.h"

class MarkdownNodeBlockHtml : public MarkdownNode
{
public:
    MarkdownNodeBlockHtml();
    void Render() override;
};

#endif // MARKDOWNNODEBLOCKHTML_H
