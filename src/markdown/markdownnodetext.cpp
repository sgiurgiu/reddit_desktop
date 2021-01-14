#include "markdownnodetext.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "utils.h"

MarkdownNodeText::MarkdownNodeText(const char* text,size_t size):text(text,size)
{
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
    bool partOfCell = false;//(node->parent && node->parent->type == CMARK_NODE_TABLE_CELL);
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
    if(!partOfCell && nextWordFinish > endPrevLine)
    {
        //if, for whatever reason, this would break in the middle of a word (what we consider a word)
        //just don't make a new line and move on.
        endPrevLine = nextWordFinish;
        ImGui::Dummy(ImVec2(0.f,0.f));
        ImGui::TextUnformatted( text_start, endPrevLine );
        ImGui::SameLine(0.f,0.f);
    }
    else
    {
        ImGui::TextUnformatted( text_start, endPrevLine );
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
    }

    if(parent && parent->GetNodeType() != NodeType::TableCell && parent->GetNodeType() != NodeType::TableCellHead)
    {
        ImGui::SameLine();
    }
}
std::string MarkdownNodeText::GetText() const
{
    return text;
}
