#include "awardsrenderer.h"
#include <imgui.h>
#include "resizableglimage.h"
#include <vector>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
namespace
{
    static std::unordered_map<std::string,ResizableGLImagePtr> awards;
}

AwardsRenderer::AwardsRenderer(post_ptr p):
    awardedPost(std::move(p))
{
}
void AwardsRenderer::LoadAwards(const access_token& token,
                                RedditClientProducer* client,
                                const boost::asio::any_io_executor& uiExecutor)
{
    if(awardedPost->allAwardings.empty()) return;
    if(awardedPost->totalAwardsReceived > 3)
    {
        totalAwardsText = fmt::format("{}",awardedPost->totalAwardsReceived-3);
    }
    boost::asio::post(uiExecutor,[weak = weak_from_this(),token,client,uiExecutor](){
        auto self = weak.lock();
        if(!self) return;
        self->loadAwards(token,client,uiExecutor);
    });
}

void AwardsRenderer::loadAwards(const access_token& token,
                                RedditClientProducer* client,
                                const boost::asio::any_io_executor& uiExecutor)
{
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
                [uiExecutor](const boost::system::error_code& ec,
                             resource_response response){
        std::string id = std::any_cast<std::string>(response.userData);
        if(!ec)
        {
            int width, height, channels;
            auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);

            boost::asio::post(uiExecutor,std::bind(AwardsRenderer::loadAwardImage,
                                                   std::move(id),
                                                   std::move(data),width,height,channels));
        }
        else
        {
            spdlog::error("Cannot load award for id {}: ", id, ec.message());
        }
    });

    // The vector has been sorted by awards
    // We only take the top 3
    size_t count = std::min(awardedPost->allAwardings.size(), (size_t)3);
    std::copy_n(awardedPost->allAwardings.begin(),count,std::back_insert_iterator(postAwards));
    for(const auto& award : postAwards)
    {
        if(awards.find(award.id) != awards.end())
        {
            continue;
        }
        awards[award.id] = nullptr;
        if(award.resizedStaticIcons.size() > 1)
        {
            resourceConnection->getResourceAuth(award.resizedStaticIcons[1].url,token,award.id);
        }
        else if(award.resizedIcons.size() > 1)
        {
            resourceConnection->getResourceAuth(award.resizedIcons[1].url,token,award.id);
        }
    }
}

void AwardsRenderer::loadAwardImage(std::string id,
                          Utils::STBImagePtr data, int width, int height,
                          int channels)
{
    ((void)channels);
    awards[id] = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
}

void AwardsRenderer::Render()
{
    ImVec2 size(ImGui::GetFontSize(),ImGui::GetFontSize());
    for(const auto& award : postAwards)
    {
        if(awards.find(award.id) != awards.end() && awards[award.id])
        {
            ImGui::Image((void*)(intptr_t)awards[award.id]->textureId,size);
            ImGui::SameLine();
        }
    }
    ImGui::TextUnformatted(totalAwardsText.c_str());
}

void AwardsRenderer::ClearAwards()
{
    awards.clear();
}
