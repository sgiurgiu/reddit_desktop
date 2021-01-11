#include "markdownnodetexthtml.h"

#include <imgui.h>


MarkdownNodeTextHtml::MarkdownNodeTextHtml(const char* text,size_t size):text(text,size)
{

}
void MarkdownNodeTextHtml::Render()
{
    ImGui::Text("%s",text.c_str());
}
