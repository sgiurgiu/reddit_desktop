#ifndef MARKDOWNNODEDOCUMENT_H
#define MARKDOWNNODEDOCUMENT_H

#include "markdownnode.h"

class MarkdownNodeDocument : public MarkdownNode
{
public:
    MarkdownNodeDocument();
    void Render() override;
};

#endif // MARKDOWNNODEDOCUMENT_H
