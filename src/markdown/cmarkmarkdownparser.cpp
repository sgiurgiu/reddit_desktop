#include "cmarkmarkdownparser.h"

#include <cmark-gfm-extension_api.h>
#include <cmark-gfm-core-extensions.h>
#include <syntax_extension.h>
#include <parser.h>
#include <registry.h>
#include <strikethrough.h>
#include <table.h>
#include <node.h>

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


namespace  {
struct cmark_node_deleter
{
    void operator()(cmark_node* node)
    {
        cmark_node_free(node);
    }
};
using cmark_node_ptr = std::unique_ptr<cmark_node,cmark_node_deleter>;
}

CMarkMarkdownParser::CMarkMarkdownParser()
{
    int options = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE | CMARK_OPT_FOOTNOTES | CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE;
    parser.reset(cmark_parser_new(options));
    auto footnotes = cmark_find_syntax_extension("footnotes");
    auto table = cmark_find_syntax_extension("table");
    auto strikethrough = cmark_find_syntax_extension("strikethrough");
    auto autolink = cmark_find_syntax_extension("autolink");
    auto tagfilter = cmark_find_syntax_extension("tagfilter");
    auto tasklist = cmark_find_syntax_extension("tasklist");
    auto spoiler = cmark_find_syntax_extension("spoiler");
    if(spoiler)
    {
        cmark_parser_attach_syntax_extension(parser.get(), spoiler);
    }
    if(footnotes)
    {
        cmark_parser_attach_syntax_extension(parser.get(), footnotes);
    }
    if(table)
    {
        cmark_parser_attach_syntax_extension(parser.get(), table);
    }
    if(strikethrough)
    {
        cmark_parser_attach_syntax_extension(parser.get(), strikethrough);
    }
    if(autolink)
    {
        cmark_parser_attach_syntax_extension(parser.get(), autolink);
    }
    if(tagfilter)
    {
        cmark_parser_attach_syntax_extension(parser.get(), tagfilter);
    }
    if(tasklist)
    {
        cmark_parser_attach_syntax_extension(parser.get(), tasklist);
    }
}
void CMarkMarkdownParser::constructDocument(cmark_node *node,cmark_event_type ev_type)
{
    auto curNode = GetCurrentNode();
    if(!curNode)
    {
        return;
    }
    std::unique_ptr<MarkdownNode> newNode;
    bool entering = (ev_type == CMARK_EVENT_ENTER);
    if(!entering)
    {
        if(curNode->GetParent())
        {
            SetCurrentNode(curNode->GetParent());
        }
        if(GetCurrentNode()->GetNodeType() == MarkdownNode::NodeType::TableHead)
        {
            //go up one more if we exited a header row
            SetCurrentNode(GetCurrentNode()->GetParent());
        }
        return;
    }
    bool leafNode = false;
    if(node->extension)
    {
        auto type = node->type;
        if(type == CMARK_NODE_STRIKETHROUGH && entering)
        {
            newNode = std::make_unique<MarkdownNodeStrike>();
        }
        else if(type == CMARK_NODE_TABLE && entering)
        {
            auto table = std::make_unique<MarkdownNodeTable>();
            table->SetColumns(cmark_gfm_extensions_get_table_columns(node));
            newNode = std::move(table);
        }
        else if(type == CMARK_NODE_TABLE_CELL && entering)
        {
            newNode = std::make_unique<MarkdownNodeTableCell>(NodeAlign::AlignDefault);
        }
        else if(type == CMARK_NODE_TABLE_ROW && entering)
        {
            if(cmark_gfm_extensions_get_table_row_is_header(node))
            {
                auto header = std::make_unique<MarkdownNodeTableHeader>();
                SetCurrentNode(header.get());
                curNode->AddChild(std::move(header));
                curNode = GetCurrentNode();
            }
            newNode = std::make_unique<MarkdownNodeTableRow>();
        }
    }
    else // if(node->extension)
    {
        switch (node->type)
        {
        case CMARK_NODE_DOCUMENT:
        break;
        case CMARK_NODE_BLOCK_QUOTE:
            newNode = std::make_unique<MarkdownNodeBlockQuote>();
        break;
        case CMARK_NODE_LIST:
            if(node->as.list.list_type == CMARK_ORDERED_LIST)
            {
                char delimiter = node->as.list.delimiter == CMARK_PAREN_DELIM ? ')' : '.';
                newNode = std::make_unique<MarkdownNodeBlockOrderedList>(node->as.list.start,delimiter);
            }
            else if(node->as.list.list_type == CMARK_BULLET_LIST)
            {
                newNode = std::make_unique<MarkdownNodeBlockUnorderedList>(node->as.list.bullet_char);
            }
        break;
        case CMARK_NODE_ITEM:
            newNode = std::make_unique<MarkdownNodeBlockListItem>();
        break;
        case CMARK_NODE_HEADING:
            newNode = std::make_unique<MarkdownNodeHead>(node->as.heading.level);
        break;
        case CMARK_NODE_CODE_BLOCK:
        {
            leafNode = true;
            std::string text((const char*)node->as.code.literal.data,node->as.code.literal.len);
            newNode = std::make_unique<MarkdownNodeBlockCode>(text,"","");
        }
        break;
        case CMARK_NODE_HTML_BLOCK:
        {
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeBlockHtml>();
            auto text = std::make_unique<MarkdownNodeText>((const char*)node->as.literal.data,node->as.literal.len);
            newNode->AddChild(std::move(text));
        }
        break;
        case CMARK_NODE_THEMATIC_BREAK:
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeThematicBreak>();
        break;
        case CMARK_NODE_PARAGRAPH:
            newNode = std::make_unique<MarkdownNodeBlockParagraph>();
        break;
        case CMARK_NODE_TEXT:
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeText>((const char*)node->as.literal.data,node->as.literal.len);
        break;
        case CMARK_NODE_LINEBREAK:
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeBreak>();
        break;
        case CMARK_NODE_SOFTBREAK:
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeSoftBreak>();
        break;
        case CMARK_NODE_CODE:
        {
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeCode>();
            auto text = std::make_unique<MarkdownNodeText>((const char*)node->as.literal.data,node->as.literal.len);
            newNode->AddChild(std::move(text));
        }
        break;
        case CMARK_NODE_HTML_INLINE:
            leafNode = true;
            newNode = std::make_unique<MarkdownNodeTextHtml>((const char*)node->as.literal.data,node->as.literal.len);
        break;
        case CMARK_NODE_STRONG:
            newNode = std::make_unique<MarkdownNodeStrong>();
        break;
        case CMARK_NODE_EMPH:
            newNode = std::make_unique<MarkdownNodeEmphasis>();
        break;
        case CMARK_NODE_LINK:
            newNode = std::make_unique<MarkdownNodeLink>(
                        std::string((const char*)node->as.link.url.data,node->as.link.url.len),
                        std::string((const char*)node->as.link.title.data,node->as.link.title.len)
                        );
        break;
        case CMARK_NODE_IMAGE:
            newNode = std::make_unique<MarkdownNodeImage>(
                        std::string((const char*)node->as.link.url.data,node->as.link.url.len),
                        std::string((const char*)node->as.link.title.data,node->as.link.title.len)
                        );
        break;
        case CMARK_NODE_NONE:
        break;
        case CMARK_NODE_CUSTOM_BLOCK:
        break;
        case CMARK_NODE_CUSTOM_INLINE:
        break;
        case CMARK_NODE_FOOTNOTE_REFERENCE:
        break;
        }
    }

    if(newNode)
    {
        if(!leafNode)
        {
            SetCurrentNode(newNode.get());
        }
        curNode->AddChild(std::move(newNode));
    }
}
std::unique_ptr<MarkdownNode> CMarkMarkdownParser::ParseText(const std::string& text)
{
    cmark_parser_feed(parser.get(), text.c_str(), text.size());
    cmark_node_ptr root(cmark_parser_finish(parser.get()));
    cmark_event_type ev_type;
    cmark_node *cur;
    cmark_iter *iter = cmark_iter_new(root.get());
    auto document = std::make_unique<MarkdownNodeDocument>();
    currentNode = document.get();
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
    {
       cur = cmark_iter_get_node(iter);
       constructDocument(cur, ev_type);
    }
    cmark_iter_free(iter);
    currentNode = nullptr;
    return document;
}

void CMarkMarkdownParser::InitCMarkEngine()
{
    cmark_gfm_core_extensions_ensure_registered();
}
void CMarkMarkdownParser::ReleaseCMarkEngine()
{
    cmark_release_plugins();
}

