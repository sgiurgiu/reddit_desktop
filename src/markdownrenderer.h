#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <memory>
#include <vector>

#include "markdown/markdownnode.h"

class MarkdownRenderer
{
public:
    MarkdownRenderer(const std::string& textToRender);
    static void InitEngine();
    static void ReleaseEngine();
    void RenderMarkdown() const;
    MarkdownNode* GetDocument() const;
    MarkdownNode* GetCurrentNode() const;
    void SetCurrentNode(MarkdownNode*);
private:
    cmark_parser* createParser();
    void renderNode(cmark_node *node,cmark_event_type ev_type) const;
    void renderCode(const char* text, bool addFramePadding) const;
    void renderNumberedListItem(const char* text) const;

private:
    std::string text;
    std::unique_ptr<MarkdownNode> document;
    MarkdownNode* currentNode = nullptr;
};

#endif // MARKDOWNRENDERER_H
