#ifndef MARKDOWNNODETABLE_H
#define MARKDOWNNODETABLE_H

#include "markdownnode.h"

class MarkdownNodeTable : public MarkdownNode
{
public:
    MarkdownNodeTable();
    void Render() override;
};

#endif // MARKDOWNNODETABLE_H
