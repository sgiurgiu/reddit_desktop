#include "markdownnodeblockcode.h"
#include <imgui.h>
#include <imgui_internal.h>

#include "utils.h"
#include "markdownnodetext.h"

MarkdownNodeBlockCode::MarkdownNodeBlockCode(std::string text, std::string lang, std::string info):
    text(std::move(text)),lang(std::move(lang)),info(std::move(info))
{
}
void MarkdownNodeBlockCode::Render()
{
    if(text.empty())
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
            text.append(textNode->GetText());
        }
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImGui::Indent();
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::NotoMono_Regular)]);

    const float line_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), g.FontSize);
    if (window->SkipItems)
        return;

    ImGui::Dummy(ImVec2(0.0f,line_height));

    auto size = ImGui::CalcTextSize(text.c_str());
    size += style.FramePadding * 2.f;
    window->DrawList->AddRectFilled(window->DC.CursorPos,window->DC.CursorPos + size , ImGui::GetColorU32(ImVec4(0.2f,0.2f,0.2f,0.5f)));
    ImGui::TextUnformatted(text.c_str());

    ImGui::Dummy(ImVec2(0.0f,line_height));

    ImGui::PopFont();
    ImGui::Unindent();
}
