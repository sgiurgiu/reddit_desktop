#ifndef GLOBALRESOURCESCACHE_H
#define GLOBALRESOURCESCACHE_H

#include <string>
#include "redditclientproducer.h"
#include <boost/asio/any_io_executor.hpp>
#include "utils.h"
#include "entities.h"

class GlobalResourcesCache
{
private:
    GlobalResourcesCache();    
    static RedditClientProducer::RedditResourceClientConnection makeResourceConnection(
            RedditClientProducer* client, const boost::asio::any_io_executor& uiExecutor);
    static void loadResourceImage(std::string id,
                                  Utils::STBImagePtr data, int width, int height,
                                  int channels);
public:
    static void ClearResources();
    static bool ResourceLoaded(const std::string& resourceId);
    static bool ContainsResource(const std::string& resourceId);
    static void LoadResource(const access_token& token,
                      RedditClientProducer* client,
                      const boost::asio::any_io_executor& uiExecutor,
                      const std::string& url,
                      const std::string& resourceId);
    static void LoadResource(
                      RedditClientProducer* client,
                      const boost::asio::any_io_executor& uiExecutor,
                      const std::string& url,
                      const std::string& resourceId);
    static ResizableGLImageSharedPtr GetResource(const std::string& resourceId);
};

#endif // GLOBALRESOURCESCACHE_H
