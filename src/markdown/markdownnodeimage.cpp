#include "markdownnodeimage.h"
#include "../entities.h"

#include <map>
#include <string>
#include "macros.h"

namespace
{
static std::map<std::string,std::string> providers = {{"giphy","https://giphy.com/gifs/"}};
}
MarkdownNodeImage::MarkdownNodeImage(std::string src, std::string title, RedditClientProducer* client, const boost::asio::any_io_executor& uiExecutor):
    src(std::move(src)),title(std::move(title))
{
    UNUSED(client);
    UNUSED(uiExecutor);
    //src can be <provider>|<ID>
    //TODO : can do this later
    /*auto index = this->src.find('|');
    if(index != std::string::npos)
    {
        auto provider = this->src.substr(0, index);
        auto id = this->src.substr(index+1);
        auto p = std::make_shared<post>();

        postContentViewer = std::make_shared<PostContentViewer>(client,access_token{},uiExecutor);
        postContentViewer->loadContent(p);
    }
    */
}
void MarkdownNodeImage::Render()
{
    if(postContentViewer)
    {
        postContentViewer->showPostContent();
    }
}
