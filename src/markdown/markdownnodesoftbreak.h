#ifndef MARKDOWNNODESOFTBREAK_H
#define MARKDOWNNODESOFTBREAK_H

#include "markdownnode.h"

class MarkdownNodeSoftBreak : public MarkdownNode
{
public:
    MarkdownNodeSoftBreak();
    void Render() override;
};

#endif // MARKDOWNNODESOFTBREAK_H
