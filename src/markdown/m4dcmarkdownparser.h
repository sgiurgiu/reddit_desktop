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
    M4DCMarkdownParser(RedditClientProducer* client,
                       const boost::asio::any_io_executor& uiExecutor);
    std::unique_ptr<MarkdownNode> ParseText(const std::string& text) override;
public:
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
};

#endif // M4DCMARKDOWNPARSER_H
