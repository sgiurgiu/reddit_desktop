#ifndef MARKDOWNNODEBLOCKCODE_H
#define MARKDOWNNODEBLOCKCODE_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeBlockCode : public MarkdownNode
{
public:
    MarkdownNodeBlockCode(const MD_BLOCK_CODE_DETAIL*);
    void Render() override;
private:
    MD_BLOCK_CODE_DETAIL detail;
};

#endif // MARKDOWNNODEBLOCKCODE_H
