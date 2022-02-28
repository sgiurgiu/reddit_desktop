#include "twitterrenderer.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "utils.h"
#include "globalresourcescache.h"

TwitterRenderer::TwitterRenderer(RedditClientProducer* client,
                                 const boost::asio::any_io_executor& uiExecutor,
                                 const std::string& twitterBearerToken):
    client(client),uiExecutor(uiExecutor),twitterBearerToken(twitterBearerToken)
{
    twitterConnection = client->makeTwitterConnection(twitterBearerToken);
}
void TwitterRenderer::Render()
{
    if(!errorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",errorMessage.c_str());
    }
    if(loadingTweetContent)
    {
        ImGui::Spinner("###spinner_loading_data",50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
        return;
    }

    if(GlobalResourcesCache::ResourceLoaded(thumbnailUrl))
    {
        ImVec2 size(ImGui::GetFontSize(),ImGui::GetFontSize());
        ImGui::Image((void*)(intptr_t)GlobalResourcesCache::GetResource(thumbnailUrl)->textureId,size);
        ImGui::SameLine();
    }

    ImGui::TextWrapped("%s",theTweet.text.c_str());

}
void TwitterRenderer::LoadTweet(const std::string& url)
{
    twitterConnection->connectionCompleteHandler([weak = weak_from_this()](
                                                 const boost::system::error_code& ec,TwitterClientResponse response){
        auto self = weak.lock();
        if(!self) return;

        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&TwitterRenderer::setErrorMessage,self,ec.message()));
        }
        else if(response.status >= 400)
        {
            boost::asio::post(self->uiExecutor,std::bind(&TwitterRenderer::setErrorMessage,self,std::move(response.body)));
        }
        else
        {
            boost::asio::post(self->uiExecutor,std::bind(&TwitterRenderer::setTwitterResponse,self,std::move(response.data)));
        }
    });
    loadingTweetContent = true;
    twitterConnection->GetTweetFromUrl(url);
}
void TwitterRenderer::setTwitterResponse(tweet response)
{
    loadingTweetContent = false;
    theTweet = std::move(response);

}
void TwitterRenderer::setErrorMessage(std::string errorMessage)
{
    loadingTweetContent = false;
    this->errorMessage = std::move(errorMessage);
}
void TwitterRenderer::SetThumbnail(const std::string& url)
{
    thumbnailUrl = url;
    GlobalResourcesCache::LoadResource(client,uiExecutor,thumbnailUrl,thumbnailUrl);
}
