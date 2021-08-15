#include "markdownrenderer.h"
#include <fmt/format.h>
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "utils.h"
#include "markdown/markdownparser.h"

MarkdownRenderer::MarkdownRenderer(): MarkdownRenderer("")
{}
MarkdownRenderer::MarkdownRenderer(const std::string& textToRender):
    text(textToRender), parser(MarkdownParser::GetParser())

{
    ParseCurrentText();
}
void MarkdownRenderer::SetText(const std::string& textToRender)
{
    text = textToRender;
    ParseCurrentText();
}
void MarkdownRenderer::ParseCurrentText()
{
    if(parser)
    {
        document = parser->ParseText(text);
    }
}
MarkdownNode* MarkdownRenderer::GetDocument() const
{
    return document.get();
}

void MarkdownRenderer::RenderMarkdown() const
{
    if(document)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Regular)]);//Roboto medium
        document->Render();
        ImGui::PopFont();        
    }
    else
    {
        ImGui::TextWrapped("%s",text.c_str());
    }
}
