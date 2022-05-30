#include "markdownnodetext.h"
#include <imgui.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui_internal.h>
#include "utils.h"

namespace
{
    static const ImVec4 highlightColor(0.8f,0.5f,0.5f,0.5f);
}

MarkdownNodeText::MarkdownNodeText(const char* text,size_t size):text(text,size)
{
    curatedText = this->text;
    curatedText.erase(std::remove(curatedText.begin(),curatedText.end(),'\n'),curatedText.end());
}
void MarkdownNodeText::AddText(const std::string& str)
{
    this->text.append(str);
    curatedText = this->text;
    curatedText.erase(std::remove(curatedText.begin(),curatedText.end(),'\n'),curatedText.end());
}
void MarkdownNodeText::Render()
{
    if(curatedText.empty())
    {
        ImGui::Dummy(ImVec2(0.f,0.f));
        return;
    }
    //float widthLeft = ImGui::GetContentRegionAvail().x;
    const char* text_start = curatedText.c_str();
    const char* text_end = curatedText.c_str()+curatedText.size();
    float fontScale = 1.f;
    const char* nextWordFinish = text_start;
    while(nextWordFinish < text_end)
    {
        if((*nextWordFinish >= 48 && *nextWordFinish < 58) ||
           (*nextWordFinish >= 65 && *nextWordFinish < 91) ||
           (*nextWordFinish >= 97 && *nextWordFinish < 123))
        {
            //while the current character is a number (0-9), or a letter (A-Z, a-z)
            //consider it part of the word. Otherwise is a different thing
            nextWordFinish++;
        }
        else
        {
            break;
        }
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
    const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text_start, text_end, wrap_width );
    if(endPrevLine == text_start)
    {
        //render at least 1 character
        endPrevLine++;
    }
    if(nextWordFinish > endPrevLine)
    {
        //if, for whatever reason, this would break in the middle of a word (what we consider a word)
        //just don't make a new line and move on.
        endPrevLine = nextWordFinish;
        ImGui::Dummy(ImVec2(0.f,0.f));

        ImGui::TextUnformatted( text_start, endPrevLine );
        highlightMatchedText(text_start,endPrevLine, text_start);

        individualComponentSignal();
        ImGui::SameLine(0.f,0.f);
    }
    else
    {
        ImGui::TextUnformatted( text_start, endPrevLine );
        highlightMatchedText(text_start,endPrevLine, text_start);
        individualComponentSignal();
    }

    //widthLeft = ImGui::GetContentRegionAvail().x;
    while( endPrevLine < text_end )
    {
       const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
       const char* text = endPrevLine;
       if( *text == ' ' ) { ++text; } // skip a space at start of line
       endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text, text_end, wrap_width );
       if(endPrevLine == text)
       {
           //render at least 1 character
           endPrevLine++;
       }
       ImGui::TextUnformatted( text, endPrevLine );
       highlightMatchedText(text,endPrevLine, text_start);


       individualComponentSignal();
    }

    if(parent && parent->GetNodeType() != NodeType::TableCell && parent->GetNodeType() != NodeType::TableCellHead)
    {
        ImGui::SameLine();
    }
}
void MarkdownNodeText::highlightMatchedText(const char* text, const char* text_end, const char* text_start)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    auto rectMin = ImGui::GetItemRectMin();
    auto textPosition = text - text_start;
    for(const auto& m : matches)
    {
        auto posStart = m.begin()-curatedText.begin()-textPosition;
        auto posEnd = m.end()-curatedText.begin()-textPosition;

        if (posStart < 0 && text+posEnd > text_end) continue;
        if(text+posStart > text_end) continue;

        if (posStart < 0)
        {
            posStart = 0;
        }

        if(text+posEnd > text_end)
        {
            posEnd = text_end-text_start;
        }

        const ImVec2 highlightTextSize = ImGui::CalcTextSize(text+posStart, text+posEnd, false, 0.f);
        const ImVec2 nothighlightTextSize = ImGui::CalcTextSize(text, text+posStart, false, 0.f);
        const ImVec2 startPos = rectMin+ImVec2(nothighlightTextSize.x,0.f);
        window->DrawList->AddRectFilled(startPos,startPos+highlightTextSize, ImGui::GetColorU32(highlightColor));
    }

}
std::string MarkdownNodeText::GetText() const
{
    return text;
}
void MarkdownNodeText::FindAndHighlightText(const std::string& textToFind)
{
    MarkdownNode::FindAndHighlightText(textToFind);
    boost::ifind_all(matches,curatedText,textToFind);
}
