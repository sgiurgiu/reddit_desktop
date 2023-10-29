#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <boost/asio/any_io_executor.hpp>

#include "markdownnode.h"
#include <memory>

#include "../redditclientproducer.h"


class MarkdownParser
{
public:
    MarkdownParser();
    virtual ~MarkdownParser() = default;
    MarkdownNode* GetCurrentNode() const;
    void SetCurrentNode(MarkdownNode*);
    virtual std::unique_ptr<MarkdownNode> ParseText(const std::string& text) = 0;
    static std::unique_ptr<MarkdownParser> GetParser(RedditClientProducer* client,
                                                     const boost::asio::any_io_executor& uiExecutor);
protected:
    MarkdownNode* currentNode = nullptr;
};

#endif // MARKDOWNPARSER_H
