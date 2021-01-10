#ifndef MARKDOWNNODEIMAGE_H
#define MARKDOWNNODEIMAGE_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeImage : public MarkdownNode
{
public:
    MarkdownNodeImage(const MD_SPAN_IMG_DETAIL*);
    void Render() override;
private:
    std::string src;
    std::string title;
};

#endif // MARKDOWNNODEIMAGE_H
