#include "flairrenderer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "globalresourcescache.h"
#include "resizableglimage.h"

namespace
{
    template<typename T>
    void render(const T&)
    {
        //by default we're not painting anything
    }
    template<typename T>
    void retrieve_data(const T&,
                       const access_token& ,
                       RedditClientProducer* ,
                       const boost::asio::any_io_executor& )
    {
        //by default we're not retrieving anything
    }


    struct TextFlair
    {
        TextFlair(std::string t,
                  std::string textColor,
                  std::string backgroundColor):
            text(std::move(t))
        {
            if(textColor == "dark")
            {
                this->textColor = ImVec4(0.f,0.f,0.f,1.f);
            }
            this->backgroundColor = Utils::GetHTMLColor(backgroundColor);
        }

        std::string text;
        ImVec4 textColor = {1.f,1.f,1.f,1.f};
        ImVec4 backgroundColor;
    };

    void render(const TextFlair& textFlair)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;

        if(!textFlair.text.empty())
        {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::NotoMono_Regular)]);
            ImGui::PushStyleColor(ImGuiCol_Text,textFlair.textColor);
            auto textSize = ImGui::CalcTextSize(textFlair.text.c_str());
            const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
            const float line_height = ImGui::GetTextLineHeight();            
            window->DrawList->AddRectFilled(text_pos,
                                            ImVec2(text_pos.x+textSize.x,text_pos.y+line_height),
                                            ImGui::GetColorU32(textFlair.backgroundColor));
            ImGui::Text("%s",textFlair.text.c_str());
            ImGui::PopStyleColor();
            ImGui::PopFont();
            ImGui::SameLine();
        }
    }

    struct EmojiFlair
    {
        EmojiFlair(std::string id, std::string url,
                   std::string backgroundColor):
            id(std::move(id)),url(std::move(url))
        {
            this->backgroundColor = Utils::GetHTMLColor(backgroundColor);
        }
        std::string id;
        std::string url;
        ImVec4 backgroundColor;
    };
    void retrieve_data(const EmojiFlair& emojiFlair,
                       const access_token& token,
                       RedditClientProducer* client,
                       const boost::asio::any_io_executor& uiExecutor)
    {
        auto id = emojiFlair.id+emojiFlair.url;
        if(!emojiFlair.url.empty() && !GlobalResourcesCache::ContainsResource(id))
        {
            GlobalResourcesCache::LoadResource(token,client,uiExecutor,
                                               emojiFlair.url,id);
        }
    }
    void render(const EmojiFlair& emojiFlair)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        auto id = emojiFlair.id+emojiFlair.url;
        if(!emojiFlair.url.empty() && GlobalResourcesCache::ResourceLoaded(id))
        {
            ImVec2 size(ImGui::GetFontSize(),ImGui::GetFontSize());
            const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
            const float line_height = ImGui::GetTextLineHeight();
            window->DrawList->AddRectFilled(text_pos,
                                            ImVec2(text_pos.x+size.x,text_pos.y+line_height),
                                            ImGui::GetColorU32(emojiFlair.backgroundColor));

            ImGui::Image((void*)(intptr_t)GlobalResourcesCache::GetResource(id)->textureId,size);
            ImGui::SameLine(0.f,0.f);
        }
    }

} // anonymous namespace

FlairRenderer::FlairRenderer(post_ptr p)
{
    if(p->linkFlairType == "text")
    {
        flairs.emplace_back(TextFlair(p->linkFlairText,p->linkFlairTextColor,p->linkFlairBackgroundColor));
    }
    else if(p->linkFlairType == "richtext")
    {
        for(const auto& f : p->linkFlairsRichText)
        {
            if(f.e == "text")
            {
                flairs.emplace_back(TextFlair(f.t,p->linkFlairTextColor,p->linkFlairBackgroundColor));
            }
            else if(f.e == "emoji")
            {
                //TODO: add emoji loading support
                flairs.emplace_back(EmojiFlair(f.a,f.u,p->linkFlairBackgroundColor));
            }
        }
    }
}

void FlairRenderer::LoadFlair(const access_token& token,
                RedditClientProducer* client,
                const boost::asio::any_io_executor& uiExecutor)
{
    boost::asio::post(uiExecutor,[weak = weak_from_this(),token,client,uiExecutor](){
        auto self = weak.lock();
        if(!self) return;
        self->loadFlair(token,client,uiExecutor);
    });

}
void FlairRenderer::loadFlair(const access_token& token,
                RedditClientProducer* client,
                const boost::asio::any_io_executor& uiExecutor)
{
    for(const auto& f : flairs)
    {
        retrieve_data(f,token,client,uiExecutor);
    }
}

template<typename T>
void FlairRenderer::flair::flair_model<T>::render_() const
{
    render(data);
}
template<typename T>
void FlairRenderer::flair::flair_model<T>::retrieve_data_(const access_token& token,
                                                          RedditClientProducer* client,
                                                          const boost::asio::any_io_executor& uiExecutor) const
{
    retrieve_data(data,token,client,uiExecutor);
}

void FlairRenderer::Render()
{
    for(const auto& f : flairs)
    {
        render(f);
    }
    //ImGuiContext& g = *GImGui;
    //const auto padding = g.Style.FramePadding.x*2.f;
    //ImGui::Dummy(ImVec2(padding,ImGui::GetTextLineHeight()));
    ImGui::SameLine();
}
