#include "markdownrenderer.h"
#include <fmt/format.h>
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "utils.h"
#include <md4c.h>

#include "markdown/markdownnodeblockcode.h"
#include "markdown/markdownnodeblockhtml.h"
#include "markdown/markdownnodeblocklistitem.h"
#include "markdown/markdownnodeblockorderedlist.h"
#include "markdown/markdownnodeblockparagraph.h"
#include "markdown/markdownnodeblockquote.h"
#include "markdown/markdownnodeblockunorderedlist.h"
#include "markdown/markdownnodebreak.h"
#include "markdown/markdownnodecode.h"
#include "markdown/markdownnodeemphasis.h"
#include "markdown/markdownnodehead.h"
#include "markdown/markdownnodeimage.h"
#include "markdown/markdownnodelink.h"
#include "markdown/markdownnodesoftbreak.h"
#include "markdown/markdownnodestrong.h"
#include "markdown/markdownnodetable.h"
#include "markdown/markdownnodetablebody.h"
#include "markdown/markdownnodetablecell.h"
#include "markdown/markdownnodetablecellhead.h"
#include "markdown/markdownnodetableheader.h"
#include "markdown/markdownnodetablerow.h"
#include "markdown/markdownnodetext.h"
#include "markdown/markdownnodetextentity.h"
#include "markdown/markdownnodetexthtml.h"
#include "markdown/markdownnodethematicbreak.h"
#include "markdown/markdownnodeunderline.h"
#include "markdown/markdownnodedocument.h"
#include "markdown/markdownnodestrike.h"

namespace
{

int enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    //static const MD_CHAR* head[6] = { "<h1>", "<h2>", "<h3>", "<h4>", "<h5>", "<h6>" };
    MarkdownRenderer* r = (MarkdownRenderer*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    std::unique_ptr<MarkdownNode> newNode;

    switch(type)
    {
        case MD_BLOCK_DOC:      /* noop */ break;
        case MD_BLOCK_QUOTE:    //RENDER_VERBATIM(r, "<blockquote>\n");
            newNode = std::make_unique<MarkdownNodeBlockQuote>();
        break;
        case MD_BLOCK_UL:       //RENDER_VERBATIM(r, "<ul>\n");
            newNode = std::make_unique<MarkdownNodeBlockUnorderedList>();
        break;
        case MD_BLOCK_OL:       //render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail);
            newNode = std::make_unique<MarkdownNodeBlockOrderedList>((const MD_BLOCK_OL_DETAIL*)detail);
        break;
        case MD_BLOCK_LI:       //render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail);
            newNode = std::make_unique<MarkdownNodeBlockListItem>((const MD_BLOCK_LI_DETAIL*)detail);
        break;
        case MD_BLOCK_HR:       //RENDER_VERBATIM(r, (r->flags & MD_HTML_FLAG_XHTML) ? "<hr />\n" : "<hr>\n");
            newNode = std::make_unique<MarkdownNodeThematicBreak>();
        break;
        case MD_BLOCK_H:        //RENDER_VERBATIM(r, head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]);
            newNode = std::make_unique<MarkdownNodeHead>((const MD_BLOCK_H_DETAIL*)detail);
        break;
        case MD_BLOCK_CODE:     //render_open_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail);
            newNode = std::make_unique<MarkdownNodeBlockCode>((const MD_BLOCK_CODE_DETAIL*)detail);
        break;
        case MD_BLOCK_HTML:     /* noop */ break;
        case MD_BLOCK_P:        //RENDER_VERBATIM(r, "<p>");
            newNode = std::make_unique<MarkdownNodeBlockParagraph>();
        break;
        case MD_BLOCK_TABLE:    //RENDER_VERBATIM(r, "<table>\n");
            newNode = std::make_unique<MarkdownNodeTable>();
        break;
        case MD_BLOCK_THEAD:    //RENDER_VERBATIM(r, "<thead>\n");
            newNode = std::make_unique<MarkdownNodeTableHeader>();
        break;
        case MD_BLOCK_TBODY:    //RENDER_VERBATIM(r, "<tbody>\n");
            newNode = std::make_unique<MarkdownNodeTableBody>();
        break;
        case MD_BLOCK_TR:       //RENDER_VERBATIM(r, "<tr>\n");
            newNode = std::make_unique<MarkdownNodeTableRow>();
        break;
        case MD_BLOCK_TH:       //render_open_td_block(r, "th", (MD_BLOCK_TD_DETAIL*)detail);
            newNode = std::make_unique<MarkdownNodeTableCellHead>((const MD_BLOCK_TD_DETAIL*)detail);
        break;
        case MD_BLOCK_TD:       //render_open_td_block(r, "td", (MD_BLOCK_TD_DETAIL*)detail);
            newNode = std::make_unique<MarkdownNodeTableCell>((const MD_BLOCK_TD_DETAIL*)detail);
        break;
    }

    if(newNode)
    {
        r->SetCurrentNode(newNode.get());
        currentNode->AddChild(std::move(newNode));
    }

    return 0;
}

int leave_block_callback(MD_BLOCKTYPE , void* , void* userdata)
{
    //static const MD_CHAR* head[6] = { "</h1>\n", "</h2>\n", "</h3>\n", "</h4>\n", "</h5>\n", "</h6>\n" };
    MarkdownRenderer* r = (MarkdownRenderer*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    r->SetCurrentNode(currentNode->GetParent());

    /*switch(type)
    {
        case MD_BLOCK_DOC:      break;
        case MD_BLOCK_QUOTE:    RENDER_VERBATIM(r, "</blockquote>\n"); break;
        case MD_BLOCK_UL:       RENDER_VERBATIM(r, "</ul>\n"); break;
        case MD_BLOCK_OL:       RENDER_VERBATIM(r, "</ol>\n"); break;
        case MD_BLOCK_LI:       RENDER_VERBATIM(r, "</li>\n"); break;
        case MD_BLOCK_HR:       break;
        case MD_BLOCK_H:        RENDER_VERBATIM(r, head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]); break;
        case MD_BLOCK_CODE:     RENDER_VERBATIM(r, "</code></pre>\n"); break;
        case MD_BLOCK_HTML:     break;
        case MD_BLOCK_P:        RENDER_VERBATIM(r, "</p>\n"); break;
        case MD_BLOCK_TABLE:    RENDER_VERBATIM(r, "</table>\n"); break;
        case MD_BLOCK_THEAD:    RENDER_VERBATIM(r, "</thead>\n"); break;
        case MD_BLOCK_TBODY:    RENDER_VERBATIM(r, "</tbody>\n"); break;
        case MD_BLOCK_TR:       RENDER_VERBATIM(r, "</tr>\n"); break;
        case MD_BLOCK_TH:       RENDER_VERBATIM(r, "</th>\n"); break;
        case MD_BLOCK_TD:       RENDER_VERBATIM(r, "</td>\n"); break;
    }*/

    return 0;
}

int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MarkdownRenderer* r = (MarkdownRenderer*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    std::unique_ptr<MarkdownNode> newNode;

    switch(type)
    {
        case MD_SPAN_EM:                //RENDER_VERBATIM(r, "<em>");
            newNode = std::make_unique<MarkdownNodeEmphasis>();
        break;
        case MD_SPAN_STRONG:            //RENDER_VERBATIM(r, "<strong>");
            newNode = std::make_unique<MarkdownNodeStrong>();
        break;
        case MD_SPAN_U:                 //RENDER_VERBATIM(r, "<u>");
            newNode = std::make_unique<MarkdownNodeUnderline>();
        break;
        case MD_SPAN_A:                 //render_open_a_span(r, (MD_SPAN_A_DETAIL*) detail);
            newNode = std::make_unique<MarkdownNodeLink>((const MD_SPAN_A_DETAIL*) detail);
        break;
        case MD_SPAN_IMG:               //render_open_img_span(r, (MD_SPAN_IMG_DETAIL*) detail);
            newNode = std::make_unique<MarkdownNodeImage>((const MD_SPAN_IMG_DETAIL*) detail);
        break;
        case MD_SPAN_CODE:              //RENDER_VERBATIM(r, "<code>");
            newNode = std::make_unique<MarkdownNodeCode>();
        break;
        case MD_SPAN_DEL:               //RENDER_VERBATIM(r, "<del>");
            newNode = std::make_unique<MarkdownNodeStrike>();
        break;
        default:
        break;
        /*
        case MD_SPAN_LATEXMATH:         RENDER_VERBATIM(r, "<x-equation>"); break;
        case MD_SPAN_LATEXMATH_DISPLAY: RENDER_VERBATIM(r, "<x-equation type=\"display\">"); break;
        case MD_SPAN_WIKILINK:          render_open_wikilink_span(r, (MD_SPAN_WIKILINK_DETAIL*) detail); break;
        */
    }

    if(newNode)
    {
        r->SetCurrentNode(newNode.get());
        currentNode->AddChild(std::move(newNode));
    }

    return 0;
}


int leave_span_callback(MD_SPANTYPE , void* , void* userdata)
{
    MarkdownRenderer* r = (MarkdownRenderer*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    r->SetCurrentNode(currentNode->GetParent());

    /*switch(type)
    {
        case MD_SPAN_EM:                RENDER_VERBATIM(r, "</em>"); break;
        case MD_SPAN_STRONG:            RENDER_VERBATIM(r, "</strong>"); break;
        case MD_SPAN_U:                 RENDER_VERBATIM(r, "</u>"); break;
        case MD_SPAN_A:                 RENDER_VERBATIM(r, "</a>"); break;
        case MD_SPAN_IMG:               break;
        case MD_SPAN_CODE:              RENDER_VERBATIM(r, "</code>"); break;
        case MD_SPAN_DEL:               RENDER_VERBATIM(r, "</del>"); break;
        case MD_SPAN_LATEXMATH:
        case MD_SPAN_LATEXMATH_DISPLAY: RENDER_VERBATIM(r, "</x-equation>"); break;
        case MD_SPAN_WIKILINK:          RENDER_VERBATIM(r, "</x-wikilink>"); break;
    }*/

    return 0;
}

int text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MarkdownRenderer* r = (MarkdownRenderer*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    std::unique_ptr<MarkdownNode> newNode;

    switch(type)
    {
        case MD_TEXT_NULLCHAR:  render_utf8_codepoint(r, 0x0000, render_verbatim);
        break;
        case MD_TEXT_BR:        RENDER_VERBATIM(r, (r->image_nesting_level == 0
                                        ? ((r->flags & MD_HTML_FLAG_XHTML) ? "<br />\n" : "<br>\n")
                                        : " "));
        break;
        case MD_TEXT_SOFTBR:    RENDER_VERBATIM(r, (r->image_nesting_level == 0 ? "\n" : " "));
        break;
        case MD_TEXT_HTML:      render_verbatim(r, text, size);
        break;
        case MD_TEXT_ENTITY:    render_entity(r, text, size, render_html_escaped);
        break;
        default:                render_html_escaped(r, text, size);
        break;
    }

    if(newNode)
    {
        currentNode->AddChild(std::move(newNode));
    }

    return 0;
}

}

MarkdownRenderer::MarkdownRenderer(const std::string& textToRender):text(textToRender)
{
    MD_PARSER parser = {
            0,
            MD_DIALECT_GITHUB,
            enter_block_callback,
            leave_block_callback,
            enter_span_callback,
            leave_span_callback,
            text_callback,
            nullptr,
            nullptr
        };
    document = std::make_unique<MarkdownNodeDocument>();
    currentNode = document.get();
    int parseResult = md_parse(text.c_str(), text.size(), &parser, (void*) this);
    if(parseResult)
    {
        document.reset();
        currentNode = nullptr;
    }
}
MarkdownNode* MarkdownRenderer::GetDocument() const
{
    return document.get();
}
MarkdownNode* MarkdownRenderer::GetCurrentNode() const
{
    return currentNode;
}
void MarkdownRenderer::SetCurrentNode(MarkdownNode* node)
{
    currentNode = node;
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
        if(text.empty())
        {
            ImGui::Dummy(ImVec2(0.f,0.f));
            break;
        }
        if(node->parent && node->parent->type == CMARK_NODE_LINK)
        {
            cmark_node_set_user_data(node->parent,new std::string(text));
            cmark_node_set_user_data_free_func(node->parent,&MarkdownRenderer::cmakeStringDeleter);
        }
        else
        {
            bool partOfCell = (node->parent && node->parent->type == CMARK_NODE_TABLE_CELL);
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
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
            const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text_start, text_end, wrap_width );
            if(endPrevLine == text_start)
            {
                //render at least 1 character
                endPrevLine++;
            }
            if(!partOfCell && nextWordFinish > endPrevLine)
            {
                //if, for whatever reason, this would break in the middle of a word (what we consider a word)
                //just don't make a new line and move on.
                endPrevLine = nextWordFinish;
                ImGui::Dummy(ImVec2(0.f,0.f));
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
               const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
               const char* text = endPrevLine;
               if( *text == ' ' ) { ++text; } // skip a space at start of line
               endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA( fontScale, text, text_end, wrap_width );
               if(endPrevLine == text)
               {
                   //render at least 1 character
                   endPrevLine++;
               }
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
                //float widthLeft = ImGui::GetContentRegionAvail().x;
                float fontScale = 1.f;
                const char* text_start = text->c_str();
                const char* text_end = text->c_str()+text->size();
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                int textBreakRetries = 0;
                while(text_start < text_end)
                {
                    const float wrap_width = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0.f);
                    const char* endPrevLine = ImGui::GetFont()->CalcWordWrapPositionA(fontScale, text_start, text_end, wrap_width );
                    if(endPrevLine == text_start)
                    {
                        ImGui::Dummy(ImVec2(0.f,0.f));
                        if(textBreakRetries<3)
                        {
                            textBreakRetries++;
                            continue;
                        }

                        //render at least 1 character
                        endPrevLine++;
                    }                    
                    ImVec4 color(0.5f,0.5f,1.f,1.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextEx(text_start, endPrevLine, ImGuiTextFlags_NoWidthForLargeClippedText);
                    ImGui::PopStyleColor();
                    text_start = endPrevLine;
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
                }

                if(!(node->parent && node->parent->type == CMARK_NODE_TABLE_CELL))
                {
                    ImGui::SameLine();
                }
            }
        }
    }
    break;
    default:
        assert(false);
        break;
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

        ImGui::PopFont();        
    }
    else
    {
        ImGui::TextWrapped("%s",text.c_str());
    }
}
void MarkdownRenderer::InitEngine()
{
}
void MarkdownRenderer::ReleaseEngine()
{
}
