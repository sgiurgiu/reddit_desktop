#ifndef MARKDOWNNODEBREAK_H
#define MARKDOWNNODEBREAK_H

#include "markdownnode.h"

class MarkdownNodeBreak : public MarkdownNode
{
public:
    MarkdownNodeBreak();
    void Render() override;
};

#endif // MARKDOWNNODEBREAK_H
