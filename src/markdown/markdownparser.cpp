#include "markdownparser.h"

#ifdef M4DC_ENABLED
#include "m4dcmarkdownparser.h"
#endif

#ifdef CMARK_ENABLED
#include "cmarkmarkdownparser.h"
#endif

MarkdownParser::MarkdownParser()
{
}
MarkdownNode* MarkdownParser::GetCurrentNode() const
{
    return currentNode;
}
void MarkdownParser::SetCurrentNode(MarkdownNode* node)
{
    currentNode = node;
}
std::unique_ptr<MarkdownParser> MarkdownParser::GetParser(RedditClientProducer* client,
                                                          const boost::asio::any_io_executor& uiExecutor)
{
#if defined(M4DC_ENABLED)
    return std::make_unique<M4DCMarkdownParser>(client, uiExecutor);
#elif defined(CMARK_ENABLED)
    return std::make_unique<CMarkMarkdownParser>(client, uiExecutor);
#else
    return std::unique_ptr<MarkdownParser>();
#endif
}
