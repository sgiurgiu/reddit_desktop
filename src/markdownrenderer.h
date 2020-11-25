#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>

class MarkdownRenderer
{
public:
    static void RenderMarkdown(const char* id, const std::string& text);
private:
    MarkdownRenderer();
};

#endif // MARKDOWNRENDERER_H
