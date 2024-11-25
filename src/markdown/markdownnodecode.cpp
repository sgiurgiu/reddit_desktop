#include "markdownnodecode.h"
#include <imgui.h>
#include "utils.h"
#include <imgui_internal.h>
#include "markdownnodetext.h"

MarkdownNodeCode::MarkdownNodeCode()
{

}
void MarkdownNodeCode::Render()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::NotoMono_Regular)]);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    //ImGuiContext& g = *GImGui;

    for(const auto& child: children)
    {
        if (window->SkipItems)
            break;

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
        auto size = ImGui::CalcTextSize(text.c_str());
        window->DrawList->AddRectFilled(window->DC.CursorPos,window->DC.CursorPos + size , ImGui::GetColorU32(ImVec4(0.2f,0.2f,0.2f,0.5f)));
        ImGui::Text("%s",text.c_str());
        ImGui::SameLine();
    }


    //std::string text((const char*)node->as.literal.data,node->as.literal.len);
    //renderCode(text.c_str(), false);ImGui::SameLine();

    ImGui::PopFont();

}
