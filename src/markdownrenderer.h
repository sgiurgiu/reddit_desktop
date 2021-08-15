#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <memory>

#include "markdown/markdownnode.h"
#include "markdown/markdownparser.h"

class MarkdownRenderer
{
public:
    MarkdownRenderer();
    MarkdownRenderer(const std::string& textToRender);
    void RenderMarkdown() const;
    MarkdownNode* GetDocument() const;
    MarkdownNode* GetCurrentNode() const;
    void SetCurrentNode(MarkdownNode*);
    void SetText(const std::string& textToRender);
private:
    void ParseCurrentText();
private:
    std::string text;
    std::unique_ptr<MarkdownNode> document;
    std::unique_ptr<MarkdownParser> parser;
};

#endif // MARKDOWNRENDERER_H
