#ifndef MARKDOWNNODESTRONG_H
#define MARKDOWNNODESTRONG_H

#include "markdownnode.h"

class MarkdownNodeStrong : public MarkdownNode
{
public:
    MarkdownNodeStrong();
    void Render() override;
};

#endif // MARKDOWNNODESTRONG_H
