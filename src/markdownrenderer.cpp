#include "markdownrenderer.h"
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "utils.h"
#include "markdown/markdownparser.h"

namespace
{
    static const ImVec4 highlightColor(0.8f,0.5f,0.5f,1.f);
}

MarkdownRenderer::MarkdownRenderer(RedditClientProducer* client,
                                   const boost::asio::any_io_executor& uiExecutor): MarkdownRenderer("",client,uiExecutor)
{}
MarkdownRenderer::MarkdownRenderer(const std::string& textToRender,RedditClientProducer* client,
                                   const boost::asio::any_io_executor& uiExecutor):
    text(textToRender), parser(MarkdownParser::GetParser(client,uiExecutor))

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
        if(matches.empty())
        {
            ImGui::TextWrapped("%s",text.c_str());
        }
        else
        {
            //this has a very bad rendering effect due to the text wrapped part
            //but this will never be actually called because we will always have a
            //markdown document to render
            auto it = text.begin();
            for(const auto& m : matches)
            {
                std::string s(it, m.begin());
                ImGui::TextWrapped("%s",s.c_str());
                ImGui::SameLine();

                s.assign(m.begin(),m.end());
                ImGui::PushStyleColor(ImGuiCol_Text, highlightColor);
                ImGui::TextWrapped("%s",s.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();
                it += m.size();
            }
            std::string s(it,text.end());
            ImGui::TextWrapped("%s",s.c_str());
            ImGui::SameLine();
        }
    }
}

void MarkdownRenderer::FindText(const std::string& textToFind)
{
    if(document)
    {
        document->FindAndHighlightText(textToFind);
    }
    else
    {
        boost::ifind_all(matches,text,textToFind);
    }
}
void MarkdownRenderer::ClearFind()
{
    if(document)
    {
        document->ClearFind();
    }
    matches.clear();
}
