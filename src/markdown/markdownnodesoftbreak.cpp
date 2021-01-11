#include "markdownnodesoftbreak.h"
#include <imgui.h>

MarkdownNodeSoftBreak::MarkdownNodeSoftBreak()
{

}
void MarkdownNodeSoftBreak::Render()
{
    ImGui::SameLine(0.0f,ImGui::CalcTextSize(" ").x);
}
