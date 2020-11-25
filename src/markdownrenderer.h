#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <imgui.h>

class MarkdownRenderer
{
public:
    static void InitEngine();
    static void ReleaseEngine();
    static void RenderMarkdown(const std::string& text);
private:
    MarkdownRenderer();
};

#endif // MARKDOWNRENDERER_H
