#include "markdownnodebreak.h"
#include <imgui.h>

MarkdownNodeBreak::MarkdownNodeBreak()
{

}
void MarkdownNodeBreak::Render()
{
    ImGui::Dummy(ImVec2(0.f,0.f));
}
