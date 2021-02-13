#include "subredditstylesheet.h"
#include "utils.h"
#include "macros.h"
#include "cssparser.h"
#include <imgui_internal.h>

SubredditStylesheet::SubredditStylesheet(const access_token& token,
                                         RedditClientProducer* client,
                                         const boost::asio::any_io_executor& executor):
    token(token),client(client),uiExecutor(executor),headerColor(206.f/255.f,227.f/255.f,248.f/255.f,1.f)
{
    headerPicture = Utils::GetRedditHeader();
}
void SubredditStylesheet::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = std::move(errorMessage);
}
void SubredditStylesheet::LoadSubredditStylesheet(const std::string& target)
{
    if(target.empty() || !target.starts_with("/r")) return;

    auto connection = client->makeListingClientConnection();
    connection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                client_response<listing> response)
    {
        auto self = weak.lock();
        if(!self) return;
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&SubredditStylesheet::setErrorMessage,self,ec.message()));
        }
        else if(response.status >= 400)
        {
            boost::asio::post(self->uiExecutor,std::bind(&SubredditStylesheet::setErrorMessage,self,std::move(response.body)));
        }
        else
        {
            boost::asio::post(self->uiExecutor,std::bind(&SubredditStylesheet::setSubredditStylesheet,
                                                        self,std::move(response.data)));
        }
    });
    connection->list(target+"/about/stylesheet",token);
}
void SubredditStylesheet::setSubredditStylesheet(listing listingResponse)
{
    auto stylesheetJson = listingResponse.json;
    std::string kind;
    if(stylesheetJson.contains("kind") && stylesheetJson["kind"].is_string())
    {
        kind = stylesheetJson["kind"].get<std::string>();
    }

    if(kind == "stylesheet" && stylesheetJson.contains("data") && stylesheetJson["data"].is_object())
    {
        stylesheet = subreddit_stylesheet(stylesheetJson["data"]);
        parseStylesheet();
        auto imageConnection = client->makeResourceClientConnection();
        imageConnection->connectionCompleteHandler(
                    [weak=weak_from_this()](const boost::system::error_code& ec,
                         resource_response response)
        {
            auto self = weak.lock();
            if(!self) return;
            if(!ec && response.status == 200)
            {
                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(self->uiExecutor,std::bind(&SubredditStylesheet::setBannerPicture,self,
                                                             std::move(data),width,height,channels));
            }
        });
        //imageConnection->getResource(subredditAbout->about.bannerBackgroundImage);
    }
}
void SubredditStylesheet::setBannerPicture(Utils::STBImagePtr data, int width, int height, int channels)
{
    UNUSED(channels);
    //headerPicture = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
}

void SubredditStylesheet::ShowHeader()
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    if(headerPicture)
    {
        float width = (float)headerPicture->width;
        float height = (float)headerPicture->height;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(headerColor));
        ImGui::BeginChild("headerCanvas", ImVec2(0.0f, height+ImGui::GetStyle().ItemSpacing.y*2), true, ImGuiWindowFlags_NoMove);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::Dummy(ImVec2(0,0));
        /*auto drawList = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 rectSize(ImGui::GetContentRegionAvail().x,height);
        drawList->AddRectFilled(p,ImVec2(p.x+rectSize.x,p.x+rectSize.y),ImGui::GetColorU32(headerColor));*/
        ImGui::Image((void*)(intptr_t)headerPicture->textureId,ImVec2(width,height));
        ImGui::EndChild();
    }
}
void SubredditStylesheet::parseStylesheet()
{
    if(!stylesheet.stylesheet.empty())
    {
        CSSParser parser(stylesheet.stylesheet);
    }
}
