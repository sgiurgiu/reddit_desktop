#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <string>
#include <memory>

#include "markdown/markdownnode.h"
#include "markdown/markdownparser.h"
#include <boost/asio/any_io_executor.hpp>
#include <boost/algorithm/string/split.hpp>
#include <vector>

#include "redditclientproducer.h"

class MarkdownRenderer
{
public:
    MarkdownRenderer(RedditClientProducer* client,
                     const boost::asio::any_io_executor& uiExecutor);
    MarkdownRenderer(const std::string& textToRender, RedditClientProducer* client,
                     const boost::asio::any_io_executor& uiExecutor);
    void RenderMarkdown() const;
    MarkdownNode* GetDocument() const;
    MarkdownNode* GetCurrentNode() const;
    void SetCurrentNode(MarkdownNode*);
    void SetText(const std::string& textToRender);
    void FindText(const std::string& textToFind);
    void ClearFind();
private:
    void ParseCurrentText();
private:
    std::string text;
    std::unique_ptr<MarkdownNode> document;
    std::unique_ptr<MarkdownParser> parser;
    std::vector<boost::iterator_range<std::string::const_iterator>> matches;
};

#endif // MARKDOWNRENDERER_H
