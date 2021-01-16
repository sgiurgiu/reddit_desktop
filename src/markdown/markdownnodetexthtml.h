#ifndef MARKDOWNNODETEXTHTML_H
#define MARKDOWNNODETEXTHTML_H

#include "markdownnode.h"
#include <string>

class MarkdownNodeTextHtml : public MarkdownNode
{
public:
    MarkdownNodeTextHtml(const char* text,size_t size);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TextHtml;
    }
private:
    std::string text;
};

#endif // MARKDOWNNODETEXTHTML_H
