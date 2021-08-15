#include "cmarkmarkdownparser.h"

CMarkMarkdownParser::CMarkMarkdownParser()
{

}

std::unique_ptr<MarkdownNode> CMarkMarkdownParser::ParseText(const std::string& text)
{
    return std::unique_ptr<MarkdownNode>();
}
