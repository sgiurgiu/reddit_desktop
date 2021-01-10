#ifndef MARKDOWNNODETABLEHEADER_H
#define MARKDOWNNODETABLEHEADER_H

#include "markdownnode.h"

class MarkdownNodeTableHeader : public MarkdownNode
{
public:
    MarkdownNodeTableHeader();
    void Render() override;
};

#endif // MARKDOWNNODETABLEHEADER_H
