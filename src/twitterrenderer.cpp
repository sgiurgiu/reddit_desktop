#include "twitterrenderer.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "utils.h"
#include "globalresourcescache.h"
#include "fonts/IconsFontAwesome4.h"
#include "date.h"

TwitterRenderer::TwitterRenderer(RedditClientProducer* client,
                                 const boost::asio::any_io_executor& uiExecutor,
                                 const std::string& twitterBearerToken):
    client(client),uiExecutor(uiExecutor),twitterBearerToken(twitterBearerToken)
{
    twitterConnection = client->makeTwitterConnection(twitterBearerToken);
    SDL_GetDesktopDisplayMode(0, &displayMode);
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

    ImGui::BeginGroup();
    {
        if(author)
        {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
            ImGui::Text("%s",author->name.c_str());
            if(author->verified)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.11f,0.6f,0.94f,1.f),"%s",reinterpret_cast<const char*>(ICON_FA_CHECK_CIRCLE));
            }
            ImGui::PopFont();
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
            ImGui::Text("@%s",author->username.c_str());
            ImGui::PopFont();
        }
        ImGui::TextWrapped("%s",theTweet.text.c_str());

        for(auto&& img : images)
        {
            ImGuiResizableGLImage(img.get(),displayMode.h * 0.5f);
        }
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("Tweet posted %s",createdAtLocal.c_str());
        ImGui::PopFont();
    }
    ImGui::EndGroup();

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
    {
        date::sys_time<std::chrono::milliseconds> tp;
        std::istringstream stream(theTweet.created_at);
        stream >> date::parse("%FT%TZ",tp);
        if(!stream.fail())
        {
            auto sp = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
            createdAtLocal = Utils::getHumanReadableTimeAgo(sp.count());
        }
    }

    for(const auto& user : theTweet.includes.users)
    {
        if(user.id == theTweet.author_id)
        {
            author = user;
        }
    }
    images.clear();
    images.resize(theTweet.includes.media.size());
    size_t imagesIndex = 0;
    for(const auto& media : theTweet.includes.media)
    {
        std::string imageUrl;
        if(media.type == "photo" && !media.url.empty())
        {
            imageUrl = media.url;
        }
        else if(media.type == "video" && !media.preview_image_url.empty()) // for not we only have this for video
        {
            imageUrl = media.preview_image_url;
        }
        if(!imageUrl.empty())
        {
            auto resourceConnection = client->makeResourceClientConnection();
            resourceConnection->connectionCompleteHandler([weak=weak_from_this()]
                                                          (const boost::system::error_code& ec,
                                                          resource_response response){
                auto self = weak.lock();
                if(!self) return;
                size_t imageIndex = std::any_cast<size_t>(response.userData);
                if(!ec && response.status == 200)
                {
                    int width, height, channels;
                    auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                    boost::asio::post(self->uiExecutor,std::bind(&TwitterRenderer::addTweetImage,self,
                                                                 imageIndex,std::move(data),width,height,channels));
                }
            });
            resourceConnection->getResource(imageUrl,imagesIndex);
            imagesIndex++;
        }
    }
}
void TwitterRenderer::addTweetImage(size_t index,Utils::STBImagePtr data, int width, int height, int channels)
{
    ((void)channels);

    if(index < images.size())
    {
        images[index] = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
    }
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
