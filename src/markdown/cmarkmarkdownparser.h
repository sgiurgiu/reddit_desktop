#ifndef CMARKMARKDOWNPARSER_H
#define CMARKMARKDOWNPARSER_H

#ifndef CMARK_ENABLED
#error "The Cmark parser is not enabled"
#endif

#include "markdownparser.h"

class CMarkMarkdownParser : public MarkdownParser
{
public:
    CMarkMarkdownParser();
    std::unique_ptr<MarkdownNode> ParseText(const std::string& text) override;
};

#endif // CMARKMARKDOWNPARSER_H
