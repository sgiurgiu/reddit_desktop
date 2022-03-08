#include "globalresourcescache.h"
#include <unordered_map>
#include <queue>
#include <utility>
#include "resizableglimage.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <chrono>

namespace
{
    static constexpr int64_t MAX_RESOURCE_MEMORY = 150'000'000ll;
    static int64_t totalMemoryConsumed = 0;
    struct LoadedResource
    {
        LoadedResource(const std::string& id,
                       int64_t resourceMemory):
            id(id),dateLoaded(std::chrono::steady_clock::now()),
            resourceMemory(resourceMemory)
        {}
        std::string id;
        std::chrono::steady_clock::time_point dateLoaded;
        int64_t resourceMemory = 0;
    };

    struct loadedResourceGreater
    {
        bool operator()(const LoadedResource& i1, const LoadedResource& i2) const
        {
            return i1.dateLoaded > i2.dateLoaded;
        }
    };
    static std::priority_queue<LoadedResource, std::vector<LoadedResource>,loadedResourceGreater> oldestResources;

    // all resources loaded so far.
    // a future optimization would be to cache them on disk
    static std::unordered_map<std::string,ResizableGLImageSharedPtr> globalResources;
}

GlobalResourcesCache::GlobalResourcesCache()
{
}
void GlobalResourcesCache::ClearResources()
{
    globalResources.clear();
    totalMemoryConsumed = 0;
}
bool GlobalResourcesCache::ResourceLoaded(const std::string& resourceId)
{
    return ContainsResource(resourceId) && globalResources[resourceId];
}
bool GlobalResourcesCache::ContainsResource(const std::string& resourceId)
{
    return globalResources.find(resourceId) != globalResources.end();
}

RedditClientProducer::RedditResourceClientConnection
    GlobalResourcesCache::makeResourceConnection(RedditClientProducer* client,
                                                 const boost::asio::any_io_executor& uiExecutor)
{
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
                [uiExecutor](const boost::system::error_code& ec,
                             resource_response response){
        std::string id = "";
        if(response.userData.has_value() && response.userData.type() == typeid(std::string))
        {
            id = std::any_cast<std::string>(response.userData);
        }
        if(!ec)
        {

            int width, height, channels;
            auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);

            boost::asio::post(uiExecutor,std::bind(GlobalResourcesCache::loadResourceImage,
                                                   std::move(id),
                                                   std::move(data),width,height,channels));
        }
        else
        {
            spdlog::error("Cannot load resource for id {}: ", id, ec.message());
        }
    });
    return resourceConnection;
}

void GlobalResourcesCache::loadResourceImage(std::string id,
                              Utils::STBImagePtr data, int width, int height,
                              int channels)
{
    ((void)channels);
    globalResources[id] = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
    int64_t resourceMemory = (width*height*STBI_rgb_alpha);
    totalMemoryConsumed += resourceMemory;
    oldestResources.emplace(id,resourceMemory);
    while(totalMemoryConsumed > MAX_RESOURCE_MEMORY)
    {
        totalMemoryConsumed -= oldestResources.top().resourceMemory;
        oldestResources.pop();
    }
}
void GlobalResourcesCache::LoadResource(const access_token& token,
                  RedditClientProducer* client,
                  const boost::asio::any_io_executor& uiExecutor,
                  const std::string& url,
                  const std::string& resourceId)
{
    auto resourceConnection = makeResourceConnection(client, uiExecutor);
    globalResources[resourceId] = nullptr;
    resourceConnection->getResourceAuth(url,token,resourceId);
}

void GlobalResourcesCache::LoadResource(
                  RedditClientProducer* client,
                  const boost::asio::any_io_executor& uiExecutor,
                  const std::string& url,
                  const std::string& resourceId)
{
    auto resourceConnection = makeResourceConnection(client, uiExecutor);
    globalResources[resourceId] = nullptr;
    resourceConnection->getResource(url,resourceId);
}

ResizableGLImageSharedPtr GlobalResourcesCache::GetResource(const std::string& resourceId)
{
    if(ContainsResource(resourceId))
    {
        return globalResources[resourceId];
    }
    return nullptr;
}
