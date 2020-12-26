#include "postcontentviewer.h"
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include "database.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <fmt/format.h>
#include "markdownrenderer.h"
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "macros.h"
#include "utils.h"
#include <iostream>

namespace
{
const std::vector<std::string> mediaDomains ={
    "www.clippituser.tv",
    "clippituser.tv",
    "streamable.com",
    "streamja.com",
    "v.redd.it",
    "youtube.com",
    "www.youtube.com",
    "youtu.be",
    "gfycat.com"
};
const std::vector<std::string> mediaExtensions = {
    ".gif",
    ".gifv",
    ".mp4",
    ".webv",
    ".avi",
    ".mkv"
};
}

namespace
{
static void* get_proc_address_mpv(void *fn_ctx, const char *name)
{
    UNUSED(fn_ctx);
    //std::cout << "get_proc_address_mpv:"<<name<<std::endl;
    return SDL_GL_GetProcAddress(name);
}
}

PostContentViewer::PostContentViewer(RedditClient* client,
                                     const boost::asio::io_context::executor_type& uiExecutor,
                                     post_ptr currentPost):
    client(client),uiExecutor(uiExecutor),currentPost(currentPost),
    mpvEventIOContextWork(boost::asio::make_work_guard(mpvEventIOContext))
{
    SDL_GetDesktopDisplayMode(0, &displayMode);
    using run_function = boost::asio::io_context::count_type(boost::asio::io_context::*)();
    auto bound_run_fuction = std::bind(static_cast<run_function>(&boost::asio::io_context::run),std::ref(mpvEventIOContext));
    mvpEventThread = std::thread(bound_run_fuction);
    mediaState.mediaAudioVolume = Database::getInstance()->getMediaAudioVolume();

    if(currentPost->isGallery && !currentPost->gallery.images.empty())
    {
        loadPostGalleryImages();
        return;
    }

    bool isMediaPost = std::find(mediaDomains.begin(),mediaDomains.end(),currentPost->domain) != mediaDomains.end();
    if(!isMediaPost)
    {
        auto idx = currentPost->url.find_last_of('.');
        if(idx != std::string::npos)
        {
            isMediaPost = std::find(mediaExtensions.begin(),mediaExtensions.end(),currentPost->url.substr(idx)) != mediaExtensions.end();
        }
    }

    if (!isMediaPost && currentPost->postHint != "image")
    {
        return;
    }
    if(currentPost->postHint == "image" &&
            !currentPost->url.empty() &&
            !currentPost->url.ends_with(".gifv"))
    {
        loadPostImage();
    }
    else if (currentPost->postHint != "self")
    {
        if(isMediaPost)
        {

            loadingPostContent = true;
            mediaStreamingConnection = client->makeMediaStreamingClientConnection();
            mediaStreamingConnection->streamAvailableHandler([this](std::string file) {
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setupMediaContext,this,file));
            });
            mediaStreamingConnection->errorHandler([this](int /*errorCode*/,const std::string& str){
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,this,str));
            });
            mediaStreamingConnection->streamMedia(currentPost.get());
        }
    }
}

PostContentViewer::~PostContentViewer()
{
    Database::getInstance()->setMediaAudioVolume(mediaState.mediaAudioVolume);
    if(mediaStreamingConnection)
    {
        mediaStreamingConnection->clearAllSlots();
    }
    if(mpv_gl)
    {
        mpv_render_context_free(mpv_gl);
    }
    if(mpv)
    {
        mpv_detach_destroy(mpv);
    }
    mpvEventIOContextWork.reset();
    mpvEventIOContext.stop();
    if(mvpEventThread.joinable())
    {
        mvpEventThread.join();
    }
    if(mediaFramebufferObject > 0)
    {
        glDeleteFramebuffers(1, &mediaFramebufferObject);
    }
}
void PostContentViewer::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = errorMessage;
    loadingPostContent = false;
}
void PostContentViewer::onMpvEvents(void* context)
{
    PostContentViewer* win=(PostContentViewer*)context;
    boost::asio::post(win->mpvEventIOContext.get_executor(),std::bind(&PostContentViewer::handleMpvEvents,win));
}
void PostContentViewer::handleMpvEvents()
{
    while (true)
    {
       mpv_event *mp_event = mpv_wait_event(mpv, 0);
       std::cout << "event:"<<mp_event->event_id<<std::endl;
       if (mp_event->event_id == MPV_EVENT_NONE)
           break;
       switch(mp_event->event_id)
       {
       case MPV_EVENT_PROPERTY_CHANGE:
       {
           mpv_event_property *prop = (mpv_event_property *)mp_event->data;
           std::cout << "prop change:"<<prop->name<<",format:"<<prop->format<<std::endl;
           std::string name(prop->name);
           if(prop->format == MPV_FORMAT_DOUBLE)
           {
               double value = *(double*)prop->data;
               //boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvDoublePropertyChanged,this,
               //                                             std::string(prop->name),val));
               if(name == "time-pos")
               {
                   mediaState.timePosition = value;
               }
               else if (name == "duration")
               {
                   mediaState.duration = value;
               }
               else if (name == "width")
               {
                   mediaState.width = value;
               }
               else if (name == "height")
               {
                   mediaState.height = value;
               }
           }
           else if(prop->format == MPV_FORMAT_FLAG)
           {
               int value = *(int*)prop->data;
               //boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvFlagPropertyChanged,this,
               //                                             std::string(prop->name),val));
               if(name == "pause")
               {
                   mediaState.paused = (bool)value;
               }
           }
           else if(prop->format == MPV_FORMAT_INT64)
           {
               //int64_t val = *(int64_t*)prop->data;
               //boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvInt64PropertyChanged,this,
               //                                             std::string(prop->name),val));
           }
           break;
       }
       case MPV_EVENT_VIDEO_RECONFIG:
       {
           /*int propValue;
           int width,height;
           mpv_get_property(mpv,"width",mpv_format::MPV_FORMAT_INT64,&propValue);
           width = propValue;
           mpv_get_property(mpv,"height",mpv_format::MPV_FORMAT_INT64,&propValue);
           height = propValue;
           std::cout << "width:"<<width<<",height:"<<height<<std::endl;*/
       }
           break;
       default:
           break;
       }
    }
}
void PostContentViewer::mpvInt64PropertyChanged(std::string name, int64_t value)
{
    std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if(name == "volume")
    {
        //mediaState.mediaAudioVolume = value;
    }
}
void PostContentViewer::mpvFlagPropertyChanged(std::string name, int value)
{
    std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if(name == "pause")
    {
        mediaState.paused = (bool)value;
    }
}
void PostContentViewer::mpvDoublePropertyChanged(std::string name, double value)
{
    std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if(name == "time-pos")
    {
        mediaState.timePosition = value;
    }
    else if (name == "duration")
    {
        mediaState.duration = value;
    }
    else if (name == "width")
    {
        mediaState.width = value;
    }
    else if (name == "height")
    {
        mediaState.height = value;
    }

}
void PostContentViewer::loadPostGalleryImages()
{
    loadingPostContent = true;
    for(size_t i=0;i<currentPost->gallery.images.size();i++)
    {
        const auto& galImage = currentPost->gallery.images.at(i);
        if(galImage->url.empty()) continue;
        auto resourceConnection = client->makeResourceClientConnection();
        resourceConnection->connectionCompleteHandler(
                    [this,index=i](const boost::system::error_code& ec,
                         const resource_response& response)
        {
            if(ec)
            {
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,this,ec.message()));
                return;
            }
            else if(response.status == 200)
            {
                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setPostGalleryImage,this,
                                                             data,width,height,channels,(int)index));
            }
        });
        resourceConnection->getResource(galImage->url);
    }
}
void PostContentViewer::setPostGalleryImage(unsigned char* data, int width, int height, int channels, int index)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    currentPost->gallery.images[index]->img = std::move(image);
    loadingPostContent = false;
}
void PostContentViewer::loadPostImage()
{
    loadingPostContent = true;
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
                [this,url=currentPost->url](const boost::system::error_code& ec,
                     const resource_response& response)
    {
        if(ec)
        {
            boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,this,ec.message()));
            return;
        }
        if(response.status == 200)
        {
            int* delays = nullptr;
            int count;
            int width, height, channels;
            if(url.ends_with(".gif"))
            {
                auto data = Utils::decodeGifData(response.data.data(),response.data.size(),
                                                   &width,&height,&channels,&count,&delays);
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setPostGif,this,
                                                             data,width,height,channels,count,delays));
            }
            else
            {
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::setPostImage,this,data,width,height,channels));
            }
        }
    });
    resourceConnection->getResource(currentPost->url);
}

void PostContentViewer::setupMediaContext(std::string file)
{
    mpv = mpv_create();
    //mpv_set_option_string(mpv, "idle", "no");
    mpv_set_option_string(mpv, "config", "no");
    mpv_set_option_string(mpv, "cache", "yes");
    //int64_t maxBytes = 1024*1024*10;
    //mpv_set_option(mpv, "demuxer-max-bytes", MPV_FORMAT_INT64,&maxBytes);

    mpv_initialize(mpv);
    mpv_opengl_init_params gl_params = {get_proc_address_mpv, nullptr, nullptr};
    //gl_params.get_proc_address = &get_proc_address_mpv;
    int mpv_advanced_control = 1;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &mpv_advanced_control},
        {MPV_RENDER_PARAM_INVALID, 0}
    };
    mpv_render_context_create(&mpv_gl, mpv, params);

    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "height", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "width", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);

    double vol = mediaState.mediaAudioVolume;
    mpv_set_property(mpv,"volume",MPV_FORMAT_DOUBLE,&vol);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);

    mpv_set_wakeup_callback(mpv, &PostContentViewer::onMpvEvents, this);
    mpv_render_context_set_update_callback(mpv_gl, &PostContentViewer::mpvRenderUpdate, this);

    const char *cmd[] = {"loadfile", file.c_str(), nullptr};
    mpv_command_async(mpv, 0, cmd);
}

void PostContentViewer::mpvRenderUpdate(void *context)
{
    PostContentViewer* win=(PostContentViewer*)context;
    boost::asio::post(win->uiExecutor,std::bind(&PostContentViewer::setPostMediaFrame,win));
}

void PostContentViewer::setPostMediaFrame()
{
    if(!currentPost) return;
    uint64_t flags = mpv_render_context_update(mpv_gl);
    if (!(flags & MPV_RENDER_UPDATE_FRAME))
    {
        return;
    }

    if(!currentPost->post_picture)
    {
        glGenFramebuffers(1, &mediaFramebufferObject);

        glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
        currentPost->post_picture = std::make_shared<gl_image>();
        glGenTextures(1, &currentPost->post_picture->textureId);
        glBindTexture(GL_TEXTURE_2D, currentPost->post_picture->textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)mediaState.width, (int)mediaState.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               currentPost->post_picture->textureId, 0);
        currentPost->post_picture->width = (int)mediaState.width;
        currentPost->post_picture->height = (int)mediaState.height;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
    mpv_opengl_fbo mpfbo{(int)mediaFramebufferObject,
                currentPost->post_picture->width,
                currentPost->post_picture->height, GL_RGBA};
    int flip_y{0};

    mpv_render_param params[] = {
                    {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
                    {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                    {MPV_RENDER_PARAM_INVALID,0}
                };
    int ret = mpv_render_context_render(mpv_gl, params);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(ret < 0)
    {
        std::cerr << "error rendering:"<<mpv_error_string(ret)<<std::endl;
        return;
    }
    loadingPostContent = false;
}
void PostContentViewer::setPostGif(unsigned char* data, int width, int height, int channels,
                int count, int* delays)
{
    auto gif = std::make_unique<post_gif>();

    std::vector<std::unique_ptr<gl_image>> images;
    int stride_bytes = width * channels;
    for (int i=0; i<count; ++i)
    {
        gif->images.emplace_back(std::make_unique<gif_image>(
                                     Utils::loadImage(data+stride_bytes * height * i, width, height, STBI_rgb_alpha),
                                     delays[i]));
    }

    free(delays);
    stbi_image_free(data);
    currentPost->gif = std::move(gif);
    loadingPostContent = false;
}

void PostContentViewer::setPostImage(unsigned char* data, int width, int height, int channels)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    currentPost->post_picture = std::move(image);
    loadingPostContent = false;
}

void PostContentViewer::showPostContent()
{
    gl_image_ptr display_image = currentPost->post_picture;

    if(currentPost->gif)
    {
        if(currentPost->gif->images[currentPost->gif->currentImage]->displayed)
        {
            auto lastDisplay = currentPost->gif->images[currentPost->gif->currentImage]->lastDisplay;
            auto diffSinceLastDisplay = std::chrono::steady_clock::now() - lastDisplay;
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diffSinceLastDisplay);
            const auto delay = std::chrono::duration<int,std::milli>(currentPost->gif->images[currentPost->gif->currentImage]->delay);
            if(ms > delay)
            {
                currentPost->gif->currentImage++;
                if(currentPost->gif->currentImage >= (int)currentPost->gif->images.size())
                {
                    currentPost->gif->currentImage = 0;
                    for(auto&& img:currentPost->gif->images)
                    {
                        img->displayed = false;
                        img->lastDisplay = std::chrono::steady_clock::time_point::min();
                    }
                }
                currentPost->gif->images[currentPost->gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
                currentPost->gif->images[currentPost->gif->currentImage]->displayed = true;
            }
        }
        else
        {
            currentPost->gif->images[currentPost->gif->currentImage]->displayed = true;
            currentPost->gif->images[currentPost->gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
        }
        display_image = currentPost->gif->images[currentPost->gif->currentImage]->img;
    }

    if(currentPost->isGallery && !currentPost->gallery.images.empty())
    {
        if(currentPost->gallery.currentImage < 0 || currentPost->gallery.currentImage >= (int)currentPost->gallery.images.size())
        {
            currentPost->gallery.currentImage = 0;
        }
        display_image = currentPost->gallery.images[currentPost->gallery.currentImage]->img;
    }

    if(display_image)
    {
        if(post_picture_width == 0.f && post_picture_height == 0.f)
        {
            auto width = (float)display_image->width;
            auto height = (float)display_image->height;
            auto availableWidth = ImGui::GetContentRegionAvail().x * 0.9f;
            if(availableWidth > 100 && width > availableWidth)
            {
                //scale the picture
                float scale = availableWidth / width;
                width = availableWidth;
                height = scale * height;
            }
            float maxPictureHeight = displayMode.h * 0.5f;
            if(maxPictureHeight > 100 && height > maxPictureHeight)
            {
                float scale = maxPictureHeight / height;
                height = maxPictureHeight;
                width = scale * width;
            }
            width = std::max(100.f,width);
            height = std::max(100.f,height);
            post_picture_width = width;
            post_picture_height = height;
            post_picture_ratio = post_picture_width / post_picture_height;
        }

        ImGui::ImageButton((void*)(intptr_t)display_image->textureId,
                           ImVec2(post_picture_width,post_picture_height),ImVec2(0, 0),ImVec2(1,1),0);

        if(ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax()) &&
                ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            //keep same aspect ratio
            auto x = ImGui::GetIO().MouseDelta.x;
            auto y = ImGui::GetIO().MouseDelta.y;
            auto new_width = post_picture_width + x;
            auto new_height = post_picture_height + y;
            auto new_area = new_width * new_height;
            new_width = std::sqrt(post_picture_ratio * new_area);
            new_height = new_area / new_width;
            post_picture_width = std::max(100.f,new_width);
            post_picture_height = std::max(100.f,new_height);
        }

        if(mediaState.duration > 0.0f)
        {
            float progress = mediaState.timePosition / mediaState.duration;
            auto progressHeight = 5.f;
            ImGui::ProgressBar(progress, ImVec2(post_picture_width, progressHeight),"");
            if(ImGui::Button(reinterpret_cast<const char*>(mediaState.paused ? ICON_FA_PLAY "##_playPauseMedia" : ICON_FA_PAUSE "##_playPauseMedia")))
            {
                int shouldPause = !mediaState.paused;
                mpv_set_property_async(mpv,0,"pause",MPV_FORMAT_FLAG,&shouldPause);
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(reinterpret_cast<const char*>(ICON_FA_VOLUME_UP));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.f);
            int volume = mediaState.mediaAudioVolume;
            if(ImGui::SliderInt("##_volumeMedia",&volume,0,100,""))
            {
                mediaState.mediaAudioVolume = volume;
                mpv_set_property_async(mpv,0,"volume",MPV_FORMAT_DOUBLE,&volume);
            }
        }
        if(currentPost->isGallery && !currentPost->gallery.images.empty())
        {
            if(ImGui::Button(fmt::format(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT "##{}_previmage"),currentPost->id).c_str()))
            {
                currentPost->gallery.currentImage--;
                if(currentPost->gallery.currentImage < 0) currentPost->gallery.currentImage = (int)currentPost->gallery.images.size() - 1;
            }
            auto btnSize = ImGui::GetItemRectSize();
            auto text = fmt::format("{}/{}",currentPost->gallery.currentImage+1,currentPost->gallery.images.size());
            auto textSize = ImGui::CalcTextSize(text.c_str());
            auto space = (post_picture_width - btnSize.x * 2.f);
            ImGui::SameLine((space - textSize.x / 2.f)/2.f);
            ImGui::Text("%s",text.c_str());
            auto windowWidth = ImGui::GetWindowContentRegionMax().x;
            auto remainingWidth = windowWidth - post_picture_width;
            ImGui::SameLine(windowWidth-remainingWidth-btnSize.x*2.f/3.f);
            if(ImGui::Button(fmt::format(reinterpret_cast<const char*>(ICON_FA_ARROW_RIGHT "##{}_nextimage"),currentPost->id).c_str()))
            {
                currentPost->gallery.currentImage++;
                if(currentPost->gallery.currentImage >= (int)currentPost->gallery.images.size()) currentPost->gallery.currentImage = 0;
            }
        }
    }

    if(!currentPost->selfText.empty())
    {
        MarkdownRenderer::RenderMarkdown(currentPost->selfText);
    }
    else if(!display_image && loadingPostContent)
    {
        constexpr std::string_view id = "###spinner_loading_data";
        ImGui::Spinner(id.data(),50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }

}
