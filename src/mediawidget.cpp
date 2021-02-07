#include "mediawidget.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

int MediaWidget::mediaProgressBar(float mediaPosition, float mediaDuration, const ImVec2& requestedSize)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return 0;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(requestedSize, ImGui::CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0))
        return 0;

    // Render
    auto fraction = ImSaturate(mediaPosition/mediaDuration);
    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
    ImGui::RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_Button), 0.0f, fraction, style.FrameRounding);
    ImVec2 bulletPos = fill_br;
    float radius = requestedSize.y / 2.f ;
    bulletPos.y -= radius;
    window->DrawList->AddCircleFilled(bulletPos, radius ,
                                      ImGui::GetColorU32(ImGuiCol_ButtonActive), 8);

    if(ImGui::IsMouseHoveringRect(bb.Min,bb.Max) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto mousePos = ImGui::GetMousePos();
        auto newFraction = (mousePos.x - bb.Min.x)/ (bb.Max.x - bb.Min.x);
        auto newMediaPosition = newFraction * mediaDuration;
        return newMediaPosition - mediaPosition;
    }

    return 0;
}
