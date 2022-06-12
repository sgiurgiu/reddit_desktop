#include "m4dcmarkdownparser.h"

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
    M4DCMarkdownParser* r = (M4DCMarkdownParser*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode)
    {
        return -1;
    }
    std::unique_ptr<MarkdownNode> newNode;

    if(currentNode->GetNodeType() == MarkdownNode::NodeType::Link)
    {
        currentNode->setChildSkipped(true);
        return 0;
    }

    switch(type)
    {
        case MD_BLOCK_DOC:      /* noop */ break;
        case MD_BLOCK_QUOTE:    //RENDER_VERBATIM(r, "<blockquote>\n");
            newNode = std::make_unique<MarkdownNodeBlockQuote>();
        break;
        case MD_BLOCK_UL:       //RENDER_VERBATIM(r, "<ul>\n");
        {
            auto block_ul_detail = (const MD_BLOCK_UL_DETAIL*)detail;
            newNode = std::make_unique<MarkdownNodeBlockUnorderedList>(block_ul_detail->mark);
        }
        break;
        case MD_BLOCK_OL:       //render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail);
        {
            auto block_ol_detail = (const MD_BLOCK_OL_DETAIL*)detail;
            newNode = std::make_unique<MarkdownNodeBlockOrderedList>(block_ol_detail->start,block_ol_detail->mark_delimiter);
        }
        break;
        case MD_BLOCK_LI:       //render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail);
            newNode = std::make_unique<MarkdownNodeBlockListItem>();
        break;
        case MD_BLOCK_HR:       //RENDER_VERBATIM(r, (r->flags & MD_HTML_FLAG_XHTML) ? "<hr />\n" : "<hr>\n");
            newNode = std::make_unique<MarkdownNodeThematicBreak>();
        break;
        case MD_BLOCK_H:        //RENDER_VERBATIM(r, head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]);
        {
            auto head_detail = (const MD_BLOCK_H_DETAIL*)detail;
            newNode = std::make_unique<MarkdownNodeHead>(head_detail->level);
        }
        break;
        case MD_BLOCK_CODE:     //render_open_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail);
        {
            auto block_code_detail = (const MD_BLOCK_CODE_DETAIL*)detail;
            newNode = std::make_unique<MarkdownNodeBlockCode>("",
                                                              std::string(block_code_detail->lang.text,block_code_detail->lang.size),
                                                              std::string(block_code_detail->info.text,block_code_detail->info.size));
        }
        break;
        case MD_BLOCK_HTML:
            newNode = std::make_unique<MarkdownNodeBlockHtml>();
        break;
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
        {
            auto td_detail = (const MD_BLOCK_TD_DETAIL*)detail;
            NodeAlign align = NodeAlign::AlignDefault;
            switch(td_detail->align)
            {
                case MD_ALIGN_LEFT:
                    align = NodeAlign::AlignLeft;
                    break;
                case MD_ALIGN_CENTER:
                    align = NodeAlign::AlignCenter;
                    break;
                case MD_ALIGN_RIGHT:
                    align = NodeAlign::AlignRight;
                    break;
                default:
                    align = NodeAlign::AlignDefault;
                    break;
            }

            newNode = std::make_unique<MarkdownNodeTableCellHead>(align);
        }
        break;
        case MD_BLOCK_TD:       //render_open_td_block(r, "td", (MD_BLOCK_TD_DETAIL*)detail);
        {
            auto td_detail = (const MD_BLOCK_TD_DETAIL*)detail;
            NodeAlign align = NodeAlign::AlignDefault;
            switch(td_detail->align)
            {
                case MD_ALIGN_LEFT:
                    align = NodeAlign::AlignLeft;
                    break;
                case MD_ALIGN_CENTER:
                    align = NodeAlign::AlignCenter;
                    break;
                case MD_ALIGN_RIGHT:
                    align = NodeAlign::AlignRight;
                    break;
                default:
                    align = NodeAlign::AlignDefault;
                    break;
            }
            newNode = std::make_unique<MarkdownNodeTableCell>(align);
        }
        break;
    }

    if(newNode)
    {
        r->SetCurrentNode(newNode.get());
        currentNode->AddChild(std::move(newNode));
    }

    return 0;
}

int leave_block_callback(MD_BLOCKTYPE type, void* , void* userdata)
{
    //static const MD_CHAR* head[6] = { "</h1>\n", "</h2>\n", "</h3>\n", "</h4>\n", "</h5>\n", "</h6>\n" };
    if(type == MD_BLOCK_DOC) return 0;

    M4DCMarkdownParser* r = (M4DCMarkdownParser*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode)
    {
        return -1;
    }
    if(currentNode->isChildSkipped())
    {
        currentNode->setChildSkipped(false);
        return 0;
    }

    r->SetCurrentNode(currentNode->GetParent());

    return 0;
}

int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    M4DCMarkdownParser* r = (M4DCMarkdownParser*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode)
    {
        return -1;
    }
    std::unique_ptr<MarkdownNode> newNode;
    if(currentNode->GetNodeType() == MarkdownNode::NodeType::Link)
    {
        currentNode->setChildSkipped(true);
        return 0;
    }

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
        {
            auto span_a_detail = (const MD_SPAN_A_DETAIL*) detail;
            newNode = std::make_unique<MarkdownNodeLink>(
                        std::string(span_a_detail->href.text,span_a_detail->href.size),
                        std::string(span_a_detail->title.text,span_a_detail->title.size)
                        );
        }
        break;
        case MD_SPAN_IMG:               //render_open_img_span(r, (MD_SPAN_IMG_DETAIL*) detail);
        {
            auto img_detail = (const MD_SPAN_IMG_DETAIL*) detail;
            newNode = std::make_unique<MarkdownNodeImage>(
                        std::string(img_detail->src.text,img_detail->src.size),
                        std::string(img_detail->title.text,img_detail->title.size)
                        );
        }
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


int leave_span_callback(MD_SPANTYPE type, void* , void* userdata)
{
    if(type == MD_SPAN_LATEXMATH || type == MD_SPAN_LATEXMATH_DISPLAY ||
            type == MD_SPAN_WIKILINK)
    {
        return 0;
    }
    M4DCMarkdownParser* r = (M4DCMarkdownParser*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode)
    {
        return -1;
    }

    if(currentNode->isChildSkipped())
    {
        currentNode->setChildSkipped(false);
        return 0;
    }

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
    M4DCMarkdownParser* r = (M4DCMarkdownParser*) userdata;
    auto currentNode = r->GetCurrentNode();
    if(!currentNode) return -1;
    std::unique_ptr<MarkdownNode> newNode;

    switch(type)
    {
        //case MD_TEXT_NULLCHAR:  render_utf8_codepoint(r, 0x0000, render_verbatim);
        //break;
        case MD_TEXT_BR:        /*RENDER_VERBATIM(r, (r->image_nesting_level == 0
                                        ? ((r->flags & MD_HTML_FLAG_XHTML) ? "<br />\n" : "<br>\n")
                                        : " "));*/
            newNode = std::make_unique<MarkdownNodeBreak>();
        break;
        case MD_TEXT_SOFTBR:    //RENDER_VERBATIM(r, (r->image_nesting_level == 0 ? "\n" : " "));
            newNode = std::make_unique<MarkdownNodeSoftBreak>();
        break;
        case MD_TEXT_HTML:      //render_verbatim(r, text, size);
            newNode = std::make_unique<MarkdownNodeTextHtml>(text, size);
        break;
        case MD_TEXT_ENTITY:    //render_entity(r, text, size, render_html_escaped);
            newNode = std::make_unique<MarkdownNodeTextEntity>(text, size);
        break;
        default:                //render_html_escaped(r, text, size);
            newNode = std::make_unique<MarkdownNodeText>(text, size);
        break;
    }

    if(newNode)
    {
        currentNode->AddChild(std::move(newNode));
    }

    return 0;
}

void debug_log(const char* /*msg*/, void* /*userdata*/)
{

}

} //anonymous namespace

M4DCMarkdownParser::M4DCMarkdownParser()
{
}

std::unique_ptr<MarkdownNode> M4DCMarkdownParser::ParseText(const std::string& text)
{
    MD_PARSER parser = {
            0,
            MD_DIALECT_GITHUB|MD_FLAG_UNDERLINE|MD_FLAG_NOHTML,
            enter_block_callback,
            leave_block_callback,
            enter_span_callback,
            leave_span_callback,
            text_callback,
            debug_log,
            nullptr
        };
    auto document = std::make_unique<MarkdownNodeDocument>();
    currentNode = document.get();
    int parseResult = md_parse(text.c_str(), text.size(), &parser, (void*) this);
    if(parseResult)
    {
        document.reset();
    }
    currentNode = nullptr;
    return document;
}
