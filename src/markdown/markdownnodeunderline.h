#ifndef MARKDOWNNODEUNDERLINE_H
#define MARKDOWNNODEUNDERLINE_H

#include "markdownnode.h"

class MarkdownNodeUnderline : public MarkdownNode
{
public:
    MarkdownNodeUnderline();
    void Render() override;
};

#endif // MARKDOWNNODEUNDERLINE_H
