#ifndef MARKDOWNNODETEXTHTML_H
#define MARKDOWNNODETEXTHTML_H

#include "markdownnode.h"

class MarkdownNodeTextHtml : public MarkdownNode
{
public:
    MarkdownNodeTextHtml();
    void Render() override;
};

#endif // MARKDOWNNODETEXTHTML_H
