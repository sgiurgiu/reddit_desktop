#include "markdownnodelink.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "utils.h"
#include "markdownnodetext.h"

MarkdownNodeLink::MarkdownNodeLink(const MD_SPAN_A_DETAIL* detail):
    href(detail->href.text,detail->href.size),
    title(detail->title.text,detail->title.size)
{
}
void MarkdownNodeLink::Render()
{
    for(const auto& child: children)
    {
        if(child->GetNodeType() != NodeType::Text)
        {
            continue;
        }
        auto childPointer = child.get();
        MarkdownNodeText* textNode =  dynamic_cast<MarkdownNodeText*>(childPointer);
        if(!textNode)
        {
            continue;
        }
        auto text = textNode->GetText();
        //float widthLeft = ImGui::GetContentRegionAvail().x;
        float fontScale = 1.f;
        const char* text_start = text.c_str();
        const char* text_end = text.c_str()+text.size();
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        int textBreakRetries = 0;
        while(text_start < text_end)
        {
            const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
            const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA(fontScale, text_start, text_end, wrap_width );
            if(endPrevLine == text_start)
            {
                ImGui::Dummy(ImVec2(0.f,0.f));
                if(textBreakRetries<3)
                {
                    textBreakRetries++;
                    continue;
                }

                //render at least 1 character
                endPrevLine++;
            }
            ImVec4 color(0.5f,0.5f,1.f,1.f);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextEx(text_start, endPrevLine, ImGuiTextFlags_NoWidthForLargeClippedText);
            ImGui::PopStyleColor();
            text_start = endPrevLine;
            //Underline it
            {
                auto rectMax = ImGui::GetItemRectMax();
                auto rectMin = ImGui::GetItemRectMin();
                rectMin.y = rectMax.y;
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(color));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(href.c_str());
                ImGui::EndTooltip();
            }
            if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                Utils::openInBrowser(href);
            }
        }

        if(parent && parent->GetNodeType() != NodeType::TableCell && parent->GetNodeType() != NodeType::TableCellHead)
        {
            ImGui::SameLine();
        }
    }
}
