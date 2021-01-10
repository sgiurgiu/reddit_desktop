#ifndef MARKDOWNNODETEXTENTITY_H
#define MARKDOWNNODETEXTENTITY_H

#include "markdownnode.h"

class MarkdownNodeTextEntity : public MarkdownNode
{
public:
    MarkdownNodeTextEntity();
    void Render() override;
};

#endif // MARKDOWNNODETEXTENTITY_H
