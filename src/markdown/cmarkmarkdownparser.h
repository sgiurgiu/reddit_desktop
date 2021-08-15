#ifndef CMARKMARKDOWNPARSER_H
#define CMARKMARKDOWNPARSER_H

#ifndef CMARK_ENABLED
#error "The Cmark parser is not enabled"
#endif

#include "markdownparser.h"
#include <cmark-gfm.h>

struct CMarkParserDeleter
{
    void operator()(cmark_parser* p)
    {
        if(p)
        {
            cmark_parser_free(p);
        }
    }
};

class CMarkMarkdownParser : public MarkdownParser
{
public:
    CMarkMarkdownParser();
    std::unique_ptr<MarkdownNode> ParseText(const std::string& text) override;
    static void InitCMarkEngine();
    static void ReleaseCMarkEngine();
private:
    void constructDocument(cmark_node *node,cmark_event_type ev_type);
private:
    std::unique_ptr<cmark_parser,CMarkParserDeleter> parser;
};

#endif // CMARKMARKDOWNPARSER_H
