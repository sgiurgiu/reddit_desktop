#ifndef MARKDOWNNODEIMAGE_H
#define MARKDOWNNODEIMAGE_H

#include "markdownnode.h"
#include <string>
#include "../postcontentviewer.h"


class MarkdownNodeImage : public MarkdownNode
{
public:
    MarkdownNodeImage(std::string src, std::string title, RedditClientProducer* client, const boost::asio::any_io_executor& uiExecutor);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Image;
    }
private:
    std::string src;
    std::string title;
    std::shared_ptr<PostContentViewer> postContentViewer;
};

#endif // MARKDOWNNODEIMAGE_H
