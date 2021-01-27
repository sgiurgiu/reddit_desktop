#include "resizableinputtextmultiline.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui_stdlib.h>

bool ResizableInputTextMultiline::InputText(const char* label, std::string* buf, ImVec2* size)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    bool ret = ImGui::InputTextMultiline(label,buf,*size);

    auto textSize = ImGui::GetItemRectMax() - ImGui::GetItemRectMin();
    auto lowerLeftCorner = window->DC.CursorPos  +
            ImVec2(ImGui::GetItemRectSize().x, -style.FramePadding.y - window->WindowBorderSize);
    const float triangleLength = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
    //auto triangleLength = (style.WindowRounding + style.FramePadding.y)*2.f;

    bool hovered = false;
    bool held = false;
    ImRect buttonRectangle(lowerLeftCorner - ImVec2(triangleLength,triangleLength),
                       lowerLeftCorner);
    ImRect holdingActiveArea = buttonRectangle;
    holdingActiveArea.Expand(triangleLength);
    ImGui::PushID("#RESIZE_TEXTMULTILINE");
    window->DC.CursorPos = buttonRectangle.Min;

    ImGui::InvisibleButton("", buttonRectangle.GetSize());
    hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
    held = ImGui::IsMouseDown(ImGuiMouseButton_Left) && holdingActiveArea.Contains(ImGui::GetMousePos());

    ImU32 color = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ResizeGripActive :
                                            hovered ? ImGuiCol_ResizeGripHovered :
                                                      ImGuiCol_ResizeGrip);
    window->DrawList->AddTriangleFilled(lowerLeftCorner-ImVec2(triangleLength,0.f),
                                        lowerLeftCorner-ImVec2(0.f,triangleLength),
                                        lowerLeftCorner,
                                        color);

    ImVec2 maxSize(window->SizeFull - ImVec2(triangleLength,triangleLength));
    if(hovered || held)
    {
        g.MouseCursor = ImGuiMouseCursor_ResizeNWSE;
    }
    if(held)
    {
        if(size->x <= 0 || size->y <= 0)
        {
            *size = textSize;
        }
        *size += g.IO.MouseDelta;
        if(size->x < 50) size->x = 50;
        if(size->y < 50) size->y = 50;
        if(size->x > maxSize.x) size->x = maxSize.x;
        if(size->y > maxSize.y) size->y = maxSize.y;
    }
    ImGui::PopID();
    return ret;
}
