#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <cmark-gfm.h>

class MarkdownRenderer
{
public:
    MarkdownRenderer(const std::string& text);
    ~MarkdownRenderer();
    static void InitEngine();
    static void ReleaseEngine();
    void RenderMarkdown();
private:
    cmark_parser* createParser();
    void renderNode(cmark_node *node,cmark_event_type ev_type);
    void renderCode(const char* text, bool addFramePadding);
    void renderNumberedListItem(const char* text);
    static void cmakeStringDeleter(cmark_mem *, void *user_data);
private:
    cmark_node *document;
    std::string text;
};

#endif // MARKDOWNRENDERER_H
