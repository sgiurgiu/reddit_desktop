#ifndef MARKDOWNNODEHEAD_H
#define MARKDOWNNODEHEAD_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeHead : public MarkdownNode
{
public:
    MarkdownNodeHead(const MD_BLOCK_H_DETAIL*);
    void Render() override;
private:
    MD_BLOCK_H_DETAIL detail;
};

#endif // MARKDOWNNODEHEAD_H
