#ifndef MARKDOWNNODETABLEBODY_H
#define MARKDOWNNODETABLEBODY_H

#include "markdownnode.h"

class MarkdownNodeTableBody : public MarkdownNode
{
public:
    MarkdownNodeTableBody();
    void Render() override;
};

#endif // MARKDOWNNODETABLEBODY_H
