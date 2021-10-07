#include "awardsrenderer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "resizableglimage.h"
#include <vector>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
namespace
{
    // all awards loaded so far.
    // a future optimization would be to cache them on disk
    static std::unordered_map<std::string,ResizableGLImagePtr> globalAwards;
}

AwardsRenderer::AwardsRenderer(post_ptr p) :
    totalAwardsReceived(p->totalAwardsReceived)
{
    // The vector has been sorted by awards
    // We only take the top 3
    size_t count = std::min(p->allAwardings.size(), (size_t)3);
    std::copy_n(p->allAwardings.begin(),count,std::back_insert_iterator(awards));

}
AwardsRenderer::AwardsRenderer(const comment& c) :
    totalAwardsReceived(c.totalAwardsReceived)
{
    size_t count = std::min(c.allAwardings.size(), (size_t)3);
    std::copy_n(c.allAwardings.begin(),count,std::back_insert_iterator(awards));
}
void AwardsRenderer::LoadAwards(const access_token& token,
                                RedditClientProducer* client,
                                const boost::asio::any_io_executor& uiExecutor)
{
    if(awards.empty()) return;
    totalAwardsText = fmt::format("{}",totalAwardsReceived);
    totalAwardsTextSize = ImGui::CalcTextSize(totalAwardsText.c_str());

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

    for(const auto& award : awards)
    {
        if(globalAwards.find(award.id) != globalAwards.end())
        {
            continue;
        }
        globalAwards[award.id] = nullptr;
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
    globalAwards[id] = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
}

float AwardsRenderer::RenderDirect(const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 size(ImGui::GetFontSize(),ImGui::GetFontSize());
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y+size.y));
    ImVec2 uv0(0, 0);
    ImVec2 uv1(1,1);
    ImVec4 tint_col(1,1,1,1);
    for(const auto& award : awards)
    {
        if(globalAwards.find(award.id) != globalAwards.end() && globalAwards[award.id])
        {
            window->DrawList->AddImage((void*)(intptr_t)globalAwards[award.id]->textureId, bb.Min,
                    bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col));
            bb.TranslateX(size.x);
        }
    }

    ImGui::RenderText(bb.Min,totalAwardsText.c_str());
    bb.TranslateX(totalAwardsTextSize.x);
    return bb.Max.x;
}
void AwardsRenderer::Render()
{
    ImVec2 size(ImGui::GetFontSize(),ImGui::GetFontSize());
    for(const auto& award : awards)
    {
        if(globalAwards.find(award.id) != globalAwards.end() && globalAwards[award.id])
        {
            ImGui::Image((void*)(intptr_t)globalAwards[award.id]->textureId,size);
            ImGui::SameLine();
        }
    }
    ImGui::TextUnformatted(totalAwardsText.c_str());
}

void AwardsRenderer::ClearAwards()
{
    globalAwards.clear();
}
