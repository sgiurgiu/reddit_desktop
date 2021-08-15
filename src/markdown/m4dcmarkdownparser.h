#ifndef M4DCMARKDOWNPARSER_H
#define M4DCMARKDOWNPARSER_H

#ifndef M4DC_ENABLED
#error "The M4DC parser is not enabled"
#endif

#include "markdownnode.h"
#include "markdownparser.h"

class M4DCMarkdownParser : public MarkdownParser
{
public:
    M4DCMarkdownParser();
    std::unique_ptr<MarkdownNode> ParseText(const std::string& text) override;
};

#endif // M4DCMARKDOWNPARSER_H
