#ifndef MARKDOWNNODETHEMATICBREAK_H
#define MARKDOWNNODETHEMATICBREAK_H

#include "markdownnode.h"

class MarkdownNodeThematicBreak : public MarkdownNode
{
public:
    MarkdownNodeThematicBreak();
    void Render() override;
};

#endif // MARKDOWNNODETHEMATICBREAK_H
