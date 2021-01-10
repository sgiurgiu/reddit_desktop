#ifndef MARKDOWNNODEEMPHASIS_H
#define MARKDOWNNODEEMPHASIS_H

#include "markdownnode.h"

class MarkdownNodeEmphasis : public MarkdownNode
{
public:
    MarkdownNodeEmphasis();
    void Render() override;
};

#endif // MARKDOWNNODEEMPHASIS_H
