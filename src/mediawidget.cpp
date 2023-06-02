#include "mediawidget.h"

#include <imgui.h>
#include <imgui_internal.h>


int MediaWidget::mediaProgressBar(float mediaPosition, float mediaDuration,
                                  float cachedTime, const ImVec2& requestedSize)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return 0;

    static const ImU32 progressBarBackground = ImGui::GetColorU32(ImVec4(0.31f, 0.31f, 0.31f, 1.00f));
    static const ImU32 progressBarCached = ImGui::GetColorU32(ImVec4(0.16f, 0.29f, 0.48f, 1.f));
    static const ImU32 progressBarPlayed = ImGui::GetColorU32(ImVec4(0.26f, 0.59f, 0.98f, 1.f));
    static const ImU32 progressBarPosBtn = ImGui::GetColorU32(ImVec4(0.06f, 0.53f, 0.98f, 1.0f));

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
    auto cachedFraction = ImSaturate(cachedTime/mediaDuration);
    ImGui::RenderFrame(bb.Min, bb.Max, progressBarBackground, true, style.FrameRounding);
    //ImRect cachedRect(bb.Min, ImVec2(bb.Max.x * cachedFraction,bb.Max.y));
    ImGui::RenderRectFilledRangeH(window->DrawList, bb, progressBarCached,
                                  0.0f, cachedFraction, style.FrameRounding);

    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
    ImGui::RenderRectFilledRangeH(window->DrawList, bb, progressBarPlayed,
                                  0.0f, fraction, style.FrameRounding);
    ImVec2 bulletPos = fill_br;
    float radius = requestedSize.y / 2.f ;
    bulletPos.y -= radius;
    window->DrawList->AddCircleFilled(bulletPos, radius,
                                      progressBarPosBtn, 8);

    if(ImGui::IsMouseHoveringRect(bb.Min,bb.Max) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto mousePos = ImGui::GetMousePos();
        auto newFraction = (mousePos.x - bb.Min.x)/ (bb.Max.x - bb.Min.x);
        auto newMediaPosition = newFraction * mediaDuration;
        return newMediaPosition - mediaPosition;
    }

    return 0;
}
