#ifndef MARKDOWNNODEBLOCKPARAGRAPH_H
#define MARKDOWNNODEBLOCKPARAGRAPH_H

#include "markdownnode.h"

class MarkdownNodeBlockParagraph : public MarkdownNode
{
public:
    MarkdownNodeBlockParagraph();
    void Render() override;
};

#endif // MARKDOWNNODEBLOCKPARAGRAPH_H
