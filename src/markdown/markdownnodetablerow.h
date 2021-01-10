#ifndef MARKDOWNNODETABLEROW_H
#define MARKDOWNNODETABLEROW_H

#include "markdownnode.h"

class MarkdownNodeTableRow : public MarkdownNode
{
public:
    MarkdownNodeTableRow();
    void Render() override;
};

#endif // MARKDOWNNODETABLEROW_H
