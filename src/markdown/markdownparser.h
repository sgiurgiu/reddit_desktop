#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include "markdownnode.h"
#include <memory>

class MarkdownParser
{
public:
    MarkdownParser();
    virtual ~MarkdownParser() = default;
    MarkdownNode* GetCurrentNode() const;
    void SetCurrentNode(MarkdownNode*);
    virtual std::unique_ptr<MarkdownNode> ParseText(const std::string& text) = 0;
    static std::unique_ptr<MarkdownParser> GetParser();
protected:
    MarkdownNode* currentNode = nullptr;
};

#endif // MARKDOWNPARSER_H
