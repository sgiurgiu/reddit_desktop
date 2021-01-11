#include "markdownnodeblockcode.h"
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "utils.h"
#include "markdownnodetext.h"

MarkdownNodeBlockCode::MarkdownNodeBlockCode(const MD_BLOCK_CODE_DETAIL* detail):
    detail(*detail),lang(detail->lang.text,detail->lang.size),
    info(detail->info.text,detail->info.size)
{
    //detail is "shallow" copied, due to the MD_ATTRIBUTE struct
    //which contains char* and other arrays
    //Don't touch info and lang members, unless you deep copy them
    //they definitely don't contain any data that we care about at the moment,
    //as is not like we're gonna implement a language highlighter
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
