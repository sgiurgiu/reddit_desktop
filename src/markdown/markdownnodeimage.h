#ifndef MARKDOWNNODEIMAGE_H
#define MARKDOWNNODEIMAGE_H

#include "markdownnode.h"
#include <md4c.h>
#include <string>

class MarkdownNodeImage : public MarkdownNode
{
public:
    MarkdownNodeImage(std::string src, std::string title);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Image;
    }
private:
    std::string src;
    std::string title;
};

#endif // MARKDOWNNODEIMAGE_H
