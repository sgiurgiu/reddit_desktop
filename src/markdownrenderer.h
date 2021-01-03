#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <cmark-gfm.h>
#include <memory>

class MarkdownRenderer
{
public:
    MarkdownRenderer(const std::string& textToRender);
    static void InitEngine();
    static void ReleaseEngine();
    void RenderMarkdown() const;
private:
    cmark_parser* createParser();
    void renderNode(cmark_node *node,cmark_event_type ev_type) const;
    void renderCode(const char* text, bool addFramePadding) const;
    void renderNumberedListItem(const char* text) const;
    static void cmakeStringDeleter(cmark_mem *, void *user_data);
private:
    struct cmark_node_deleter
    {
        void operator()(cmark_node* node)
        {
            cmark_node_free(node);
        }
    };
    std::unique_ptr<cmark_node,cmark_node_deleter> document;
    std::string text;
};

#endif // MARKDOWNRENDERER_H
