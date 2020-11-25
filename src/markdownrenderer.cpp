#include "markdownrenderer.h"

#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>
#include <cmark-gfm-core-extensions.h>
#include <parser.h>
#include <registry.h>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "utils.h"

MarkdownRenderer::MarkdownRenderer()
{
}

namespace
{
    void cmark_string_deleter(cmark_mem *, void *user_data)
    {
        if(user_data)
        {
            auto str = static_cast<std::string*>(user_data);
            delete str;
        }
    }

    void NumberedListItem(const char* text)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const float line_height = 0.0f;//ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
        auto size = ImGui::CalcTextSize(text);
        auto size_standard = ImGui::CalcTextSize(".");
        size -= size_standard;
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0))
        {
            ImGui::SameLine(0, style.FramePadding.x * 2);
            return;
        }

        // Render and stay on same line
        ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        window->DrawList->AddText(bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, line_height * 0.5f) - size , text_col, text);
        ImGui::SameLine(0, style.FramePadding.x * 2.0f);
    }

    void CodeText(const char* text, bool addFramePadding)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        auto size = ImGui::CalcTextSize(text);
        if(addFramePadding)
        {
            size += style.FramePadding * 2.f;
        }
        window->DrawList->AddRectFilled(window->DC.CursorPos,window->DC.CursorPos + size , ImGui::GetColorU32(ImVec4(0.2f,0.2f,0.2f,0.5f)));
        ImGui::Text("%s",text);
    }

    cmark_parser *create_parser()
    {
        int options = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE | CMARK_OPT_FOOTNOTES;
        auto parser = cmark_parser_new_with_mem(options, cmark_get_arena_mem_allocator());
        {
            auto footnotes = cmark_find_syntax_extension("footnotes");
            auto table = cmark_find_syntax_extension("table");
            auto strikethrough = cmark_find_syntax_extension("strikethrough");
            auto autolink = cmark_find_syntax_extension("autolink");
            auto tagfilter = cmark_find_syntax_extension("tagfilter");
            auto tasklist = cmark_find_syntax_extension("tasklist");
            if(footnotes)
            {
                cmark_parser_attach_syntax_extension(parser, footnotes);
            }
            if(table)
            {
                cmark_parser_attach_syntax_extension(parser, table);
            }
            if(strikethrough)
            {
                cmark_parser_attach_syntax_extension(parser, strikethrough);
            }
            if(autolink)
            {
                cmark_parser_attach_syntax_extension(parser, autolink);
            }
            if(tagfilter)
            {
                cmark_parser_attach_syntax_extension(parser, tagfilter);
            }
            if(tasklist)
            {
                cmark_parser_attach_syntax_extension(parser, tasklist);
            }
        }
        return parser;
    }

    void render_node(cmark_node *node,cmark_event_type ev_type)
    {
        bool entering = (ev_type == CMARK_EVENT_ENTER);
        switch (node->type)
        {
        case CMARK_NODE_DOCUMENT:
        break;
        case CMARK_NODE_BLOCK_QUOTE:
            if(entering)
            {
                ImGui::Indent();
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_LightItalic)]);
            }
            else
            {
                ImGui::PopFont();
                ImGui::Unindent();
            }
        break;
        case CMARK_NODE_LIST:
        break;
        case CMARK_NODE_ITEM:
        {
            if(entering)
            {
                if(node->as.list.list_type == CMARK_ORDERED_LIST)
                    NumberedListItem((std::to_string(node->as.list.start)+".").c_str());
                else if(node->as.list.list_type == CMARK_BULLET_LIST)
                    ImGui::Bullet();
                else
                    NumberedListItem(" ");
            }
        }
        break;
        case CMARK_NODE_HEADING:
            if(entering)
            {
               ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Medium_Big)]);
            }
            else
            {
                ImGui::PopFont();
                ImGui::Dummy(ImVec2(0.f,0.f));
            }
        break;
        case CMARK_NODE_CODE_BLOCK:
            ImGui::Indent();
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::RobotoMono_Regular)]);
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImGuiContext& g = *GImGui;
            const ImGuiStyle& style = g.Style;
            const float line_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), g.FontSize);

            ImGui::Dummy(ImVec2(0.0f,line_height));
            std::string text((const char*)node->as.code.literal.data,node->as.code.literal.len);
            CodeText(text.c_str(), true);
            ImGui::Dummy(ImVec2(0.0f,line_height));
        }
            ImGui::PopFont();
            ImGui::Unindent();
        break;
        case CMARK_NODE_HTML_BLOCK:
        {
            std::string text((const char*)node->as.literal.data,node->as.literal.len);
            ImGui::TextUnformatted(text.c_str());//ImGui::SameLine();
        }
        break;
        case CMARK_NODE_CUSTOM_BLOCK:
        {
           // std::string text((const char*)node->as.custom.on_enter.data,node->as.literal.len);
           // ImGui::TextUnformatted(text.c_str());//ImGui::SameLine();
        }
        break;
        case CMARK_NODE_THEMATIC_BREAK:
            ImGui::Separator();
        break;
        case CMARK_NODE_PARAGRAPH:
            if(entering)
            {
                if (node->parent && node->parent->type == CMARK_NODE_ITEM &&
                    node->prev == NULL)
                {
                  // no blank line or .PP
                }
                else
                {
                  ImGui::Dummy(ImVec2(0.f,0.f));
                }
            }
            else
            {
                ImGui::Dummy(ImVec2(0.f,0.f));
            }
        break;
        case CMARK_NODE_TEXT:
            if(node->parent)
            {
                std::string text((const char*)node->as.literal.data,node->as.literal.len);
                text.erase(std::remove(text.begin(),text.end(),'\n'),text.end());
                if(node->parent->type == CMARK_NODE_LINK)
                {
                    cmark_node_set_user_data(node->parent,new std::string(text));
                    cmark_node_set_user_data_free_func(node->parent,cmark_string_deleter);
                }
                else
                {
                    ImGui::Text("%s",text.c_str());ImGui::SameLine();
                }
            }
           // ImGui::SameLine();
        break;
        case CMARK_NODE_LINEBREAK:
            ImGui::Dummy(ImVec2(0.f,0.f));
        break;
        case CMARK_NODE_SOFTBREAK:
            ImGui::SameLine(0.0f,ImGui::CalcTextSize(" ").x);
        break;
        case CMARK_NODE_CODE:
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::RobotoMono_Regular)]);
        {
            std::string text((const char*)node->as.literal.data,node->as.literal.len);
            CodeText(text.c_str(), false);ImGui::SameLine();
        }
            ImGui::PopFont();

        break;
        case CMARK_NODE_HTML_INLINE:
        {
            std::string text((const char*)node->as.literal.data,node->as.literal.len);
            ImGui::Text("%s",text.c_str());//ImGui::SameLine();
        }
        break;
        case CMARK_NODE_CUSTOM_INLINE:
        break;
        case CMARK_NODE_STRONG:
            if(entering)
            {
               ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Bold)]);
            }
            else
            {
                ImGui::PopFont();

            }
        break;
        case CMARK_NODE_EMPH:
            if(entering)
            {
               ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Italic)]);
            }
            else
            {
                ImGui::PopFont();

            }
        break;
        case CMARK_NODE_LINK:
        {
            if(!entering)
            {
                auto text = static_cast<std::string*>(cmark_node_get_user_data(node));
                if(text)
                {
                    if(ImGui::SmallButton(text->c_str()))
                    {
                        //return the URL????
                    }
                    ImGui::SameLine();
                }
            }
        }
        break;
        case CMARK_NODE_IMAGE:
        break;
        case CMARK_NODE_FOOTNOTE_DEFINITION:
        break;
        case CMARK_NODE_FOOTNOTE_REFERENCE:
        break;
        }
    }
}
void MarkdownRenderer::RenderMarkdown(const char* id, const std::string& text)
{
    cmark_gfm_core_extensions_ensure_registered();

    auto parser = create_parser();

    cmark_parser_feed(parser, text.c_str(), text.size());
    auto document = cmark_parser_finish(parser);

    if(document)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Regular)]);//Roboto medium
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), true, window_flags);

        cmark_event_type ev_type;
        cmark_node *cur;
        cmark_iter *iter = cmark_iter_new(document);
        while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
           cur = cmark_iter_get_node(iter);
           render_node(cur, ev_type);
         }        
        cmark_iter_free(iter);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }
    else
    {
        ImGui::TextWrapped("%s",text.c_str());
    }
    cmark_arena_reset();
    cmark_release_plugins();
}
