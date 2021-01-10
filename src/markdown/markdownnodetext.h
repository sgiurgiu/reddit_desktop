#ifndef MARKDOWNNODETEXT_H
#define MARKDOWNNODETEXT_H

#include "markdownnode.h"

class MarkdownNodeText : public MarkdownNode
{
public:
    MarkdownNodeText();
    void Render() override;
};

#endif // MARKDOWNNODETEXT_H
