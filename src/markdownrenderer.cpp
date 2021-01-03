#include "markdownrenderer.h"


#include <cmark-gfm-extension_api.h>
#include <cmark-gfm-core-extensions.h>
#include <syntax_extension.h>
#include <parser.h>
#include <registry.h>
#include <strikethrough.h>
#include <table.h>
#include <fmt/format.h>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "utils.h"

MarkdownRenderer::MarkdownRenderer(const std::string& textToRender):document(nullptr,cmark_node_deleter()),
    text(textToRender)
{
    auto parser = createParser();

    cmark_parser_feed(parser, text.c_str(), text.size());
    document.reset(cmark_parser_finish(parser));
    if (parser)
        cmark_parser_free(parser);

}

cmark_parser* MarkdownRenderer::createParser()
{
    int options = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE | CMARK_OPT_FOOTNOTES | CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE;
    auto parser = cmark_parser_new(options);
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

void MarkdownRenderer::renderNode(cmark_node *node,cmark_event_type ev_type) const
{
    bool entering = (ev_type == CMARK_EVENT_ENTER);
    if(node->extension)
    {
        auto type = node->type;
        if(type == CMARK_NODE_STRIKETHROUGH)
        {
            if(!entering)
            {
                auto rectMax = ImGui::GetItemRectMax();
                auto rectMin = ImGui::GetItemRectMin();
                rectMin.y += std::abs(rectMax.y-rectMin.y) / 2.f;
                rectMax.y = rectMin.y;
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(ImGuiCol_Text));
            }
        }
        else if(type == CMARK_NODE_TABLE)
        {
            if(entering)
            {
                ImGui::Columns(cmark_gfm_extensions_get_table_columns(node), nullptr, true);
                ImGui::Separator();
            }
            else
            {
                //ImGui::Separator();
                ImGui::Columns(1);
            }
        }
        else if(type == CMARK_NODE_TABLE_CELL)
        {
            //cmark_gfm_extensions_get_table_row_is_header(node);
            if(!entering)
            {
                //ImGui::NewLine();
                ImGui::NextColumn();
            }
        }
        else if(type == CMARK_NODE_TABLE_ROW)
        {
            if(!entering)
            {
               ImGui::Separator();
            }
        }
        return;
    }
    switch (node->type)
    {
    case CMARK_NODE_DOCUMENT:
    break;
    case CMARK_NODE_BLOCK_QUOTE:
        if(entering)
        {
            ImGui::Indent();
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_LightItalic)]);
        }
        else
        {
            ImGui::PopFont();
            ImGui::Unindent();
        }
    break;
    case CMARK_NODE_LIST:
    {
        if(node->parent && node->parent->type == CMARK_NODE_ITEM)
        {
            if(entering)
            {
                ImGui::Indent();
            }
            else
            {
                ImGui::Unindent();
            }
        }
    }
    break;
    case CMARK_NODE_ITEM:
    {
        if(entering)
        {
            if(node->as.list.list_type == CMARK_ORDERED_LIST)
                renderNumberedListItem((std::to_string(node->as.list.start)+".").c_str());
            else if(node->as.list.list_type == CMARK_BULLET_LIST)
                ImGui::Bullet();
            else
                renderNumberedListItem(" ");
        }
    }
    break;
    case CMARK_NODE_HEADING:
        if(entering)
        {
           ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium_Big)]);
        }
        else
        {
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(0.f,0.f));
        }
    break;
    case CMARK_NODE_CODE_BLOCK:
        ImGui::Indent();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::NotoMono_Regular)]);
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const float line_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), g.FontSize);

        ImGui::Dummy(ImVec2(0.0f,line_height));
        std::string text((const char*)node->as.code.literal.data,node->as.code.literal.len);
        renderCode(text.c_str(), true);
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
    {
        std::string text((const char*)node->as.literal.data,node->as.literal.len);
        text.erase(std::remove(text.begin(),text.end(),'\n'),text.end());
        if(node->parent && node->parent->type == CMARK_NODE_LINK)
        {
            cmark_node_set_user_data(node->parent,new std::string(text));
            cmark_node_set_user_data_free_func(node->parent,&MarkdownRenderer::cmakeStringDeleter);
        }
        else
        {
            float widthLeft = ImGui::GetContentRegionAvail().x;
            const char* text_start = text.c_str();
            const char* text_end = text.c_str()+text.size();
            float fontScale = 1.f;
            const char* nextWordFinish = text_start;
            while(nextWordFinish < text_end)
            {
                if((*nextWordFinish >= 48 && *nextWordFinish < 58) ||
                   (*nextWordFinish >= 65 && *nextWordFinish < 91) ||
                   (*nextWordFinish >= 97 && *nextWordFinish < 123))
                {
                    //while the current character is a number (0-9), or a letter (A-Z, a-z)
                    //consider it part of the word. Otherwise is a different thing
                    nextWordFinish++;
                }
                else
                {
                    break;
                }
            }

            const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text_start, text_end, widthLeft );
            if(nextWordFinish > endPrevLine)
            {
                //if, for whatever reason, this would break in the middle of a word (what we consider a word)
                //just don't, make a new line and move on.
                endPrevLine = nextWordFinish;
                ImGui::NewLine();
                ImGui::TextUnformatted( text_start, endPrevLine );
                ImGui::SameLine(0.f,0.f);
            }
            else
            {
                ImGui::TextUnformatted( text_start, endPrevLine );
            }


            widthLeft = ImGui::GetContentRegionAvail().x;
            while( endPrevLine < text_end )
            {
               const char* text = endPrevLine;
               if( *text == ' ' ) { ++text; } // skip a space at start of line
               endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text, text_end, widthLeft );
               ImGui::TextUnformatted( text, endPrevLine );
            }

            if(!(node->parent && node->parent->type == CMARK_NODE_TABLE_CELL))
            {
                ImGui::SameLine();
            }
        }
    }
    break;
    case CMARK_NODE_LINEBREAK:
        ImGui::Dummy(ImVec2(0.f,0.f));
    break;
    case CMARK_NODE_SOFTBREAK:
        ImGui::SameLine(0.0f,ImGui::CalcTextSize(" ").x);
    break;
    case CMARK_NODE_CODE:
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::NotoMono_Regular)]);
    {
        std::string text((const char*)node->as.literal.data,node->as.literal.len);
        renderCode(text.c_str(), false);ImGui::SameLine();
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
           ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
        }
        else
        {
           ImGui::PopFont();
        }
    break;
    case CMARK_NODE_EMPH:
        if(entering)
        {
           ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Italic)]);
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
                float widthLeft = ImGui::GetContentRegionAvail().x;
                float fontScale = 1.f;
                const char* text_start = text->c_str();
                const char* text_end = text->c_str()+text->size();
                const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text_start, text_end, widthLeft );
                if(endPrevLine < text_end) ImGui::NewLine();
                ImVec4 color(0.5f,0.5f,1.f,1.f);
                ImGui::TextColored(color,"%s",text->c_str());
                //Underline it
                {
                    auto rectMax = ImGui::GetItemRectMax();
                    auto rectMin = ImGui::GetItemRectMin();
                    rectMin.y = rectMax.y;
                    ImGuiWindow* window = ImGui::GetCurrentWindow();
                    window->DrawList->AddLine(rectMin,rectMax, ImGui::GetColorU32(color));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
                if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    std::string url((const char*)node->as.link.url.data,node->as.link.url.len);
                    Utils::openInBrowser(url);
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
    default:
        assert(false);
        break;
    }
}

void MarkdownRenderer::cmakeStringDeleter(cmark_mem *, void *user_data)
{
    if(user_data)
    {
        auto str = static_cast<std::string*>(user_data);
        delete str;
    }
}
void MarkdownRenderer::renderNumberedListItem(const char* text) const
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float line_height = 0.0f;//ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0))
    {
        ImGui::SameLine(0, style.FramePadding.x * 2);
        return;
    }

    auto size = ImGui::CalcTextSize(text);

    size -= ImVec2(g.FontSize/2.f,g.FontSize);
    //size.y -= g.FontSize;
    // Render and stay on same line
    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    window->DrawList->AddText(bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, line_height * 0.5f) - size , text_col, text);
    ImGui::SameLine(0, style.FramePadding.x * 2.0f);
}

void MarkdownRenderer::renderCode(const char* text, bool addFramePadding) const
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
void MarkdownRenderer::RenderMarkdown() const
{
    if(document)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Regular)]);//Roboto medium
        cmark_event_type ev_type;
        cmark_node *cur;
        cmark_iter *iter = cmark_iter_new(document.get());
        while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
        {
           cur = cmark_iter_get_node(iter);
           renderNode(cur, ev_type);
        }
        cmark_iter_free(iter);
        ImGui::PopFont();        
    }
    else
    {
        ImGui::TextWrapped("%s",text.c_str());
    }
}
void MarkdownRenderer::InitEngine()
{
    cmark_gfm_core_extensions_ensure_registered();
}
void MarkdownRenderer::ReleaseEngine()
{
    cmark_release_plugins();
}
