#ifndef MARKDOWNNODETEXT_H
#define MARKDOWNNODETEXT_H

#include "markdownnode.h"
#include <string>

class MarkdownNodeText : public MarkdownNode
{
public:
    MarkdownNodeText(const char* text,size_t size);
    void Render() override;
    std::string GetText() const;
    NodeType GetNodeType() const override
    {
        return NodeType::Text;
    }
private:
    std::string text;
    std::string curatedText;
};

#endif // MARKDOWNNODETEXT_H
