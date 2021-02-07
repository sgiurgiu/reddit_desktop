#include "postcontentviewer.h"
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include "database.h"
#include "spinner/spinner.h"
#include <imgui.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "macros.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "mediawidget.h"

namespace
{
const std::vector<std::string> mediaDomains ={
    "www.clippituser.tv",
    "clippituser.tv",
    "streamable.com",
    "streamja.com",
    "streamvi.com",
    "streamwo.com",
    "streamye.com",
    "v.redd.it",
    "youtube.com",
    "www.youtube.com",
    "youtu.be",
    "gfycat.com",
    "imgur.com",
    "i.imgur.com",
    "redgifs.com",
    "www.redgifs.com"
};
const std::vector<std::string> mediaExtensions = {
    ".gif",
    ".gifv",
    ".mp4",
    ".webv",
    ".avi",
    ".mkv"
};
static void* get_proc_address_mpv(void *fn_ctx, const char *name)
{
    UNUSED(fn_ctx);
    //std::cout << "get_proc_address_mpv:"<<name<<std::endl;
    return SDL_GL_GetProcAddress(name);
}
}

PostContentViewer::PostContentViewer(RedditClientProducer* client,
                                     const boost::asio::any_io_executor& uiExecutor
                                     ):
    client(client),uiExecutor(uiExecutor)
{
    SDL_GetDesktopDisplayMode(0, &displayMode);
    mediaState.mediaAudioVolume = Database::getInstance()->getMediaAudioVolume();
    useMediaHwAccel = Database::getInstance()->getUseHWAccelerationForMedia();
}
void PostContentViewer::loadContent(post_ptr currentPost)
{
    this->currentPost = currentPost;
    currentPostSet = true;
    mediaButtonRestartText = fmt::format("{}##_restartMedia_{}",reinterpret_cast<const char*>(ICON_FA_REPEAT),currentPost->name);
    mediaButtonFastBackwardText = fmt::format("{}##_fastBackward_{}",reinterpret_cast<const char*>(ICON_FA_FAST_BACKWARD),currentPost->name);
    mediaButtonBackwardText = fmt::format("{}##_Backward_{}",reinterpret_cast<const char*>(ICON_FA_BACKWARD),currentPost->name);
    mediaButtonPlayText = fmt::format("{}##_playMedia_{}",reinterpret_cast<const char*>(ICON_FA_PLAY),currentPost->name);
    mediaButtonPauseText = fmt::format("{}##_pauseMedia_{}",reinterpret_cast<const char*>(ICON_FA_PAUSE),currentPost->name);
    mediaButtonForwardText = fmt::format("{}##_Forward_{}",reinterpret_cast<const char*>(ICON_FA_FORWARD),currentPost->name);
    mediaButtonFastForwardText = fmt::format("{}##_fastForward_{}",reinterpret_cast<const char*>(ICON_FA_FAST_FORWARD),currentPost->name);
    mediaSliderVolumeText = fmt::format("##_volumeMedia_{}",currentPost->name);
    galleryButtonPreviousText = fmt::format("{}##_previmage_{}",reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT),currentPost->name);
    galleryButtonNextText = fmt::format("{}##_nextimage_{}",reinterpret_cast<const char*>(ICON_FA_ARROW_RIGHT),currentPost->name);
    loopCheckboxText = fmt::format("Loop##_loop_{}",currentPost->name);
    mediaProgressSliderText = fmt::format("##_progressSlider_{}", currentPost->name);

    if(!currentPost->selfText.empty())
    {
        markdown = std::make_unique<MarkdownRenderer>(currentPost->selfText);
        loadingPostContent = false;
    }
    if(currentPost->isGallery && !currentPost->gallery.empty())
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
            auto mediaStreamingConnection = client->makeMediaStreamingClientConnection();
            mediaStreamingConnection->streamAvailableHandler([weak=weak_from_this()](HtmlParser::MediaLink link) {
                auto self = weak.lock();
                if(!self) return;
                switch(link.type)
                {
                case HtmlParser::MediaType::Video:
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&PostContentViewer::setupMediaContext,self,link.url));
                    break;
                case HtmlParser::MediaType::Image:
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&PostContentViewer::downloadPostImage,self,link.url));
                    break;
                case HtmlParser::MediaType::Gif:
                    break;
                default:
                    break;
                }

            });
            mediaStreamingConnection->errorHandler([weak = weak_from_this()](int /*errorCode*/,const std::string& str){
                auto self = weak.lock();
                if (!self) return;
                boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,self,str));
            });
            mediaStreamingConnection->streamMedia(currentPost.get());
        }
    }
}
void PostContentViewer::stopPlayingMedia(bool flag)
{
    mediaState.paused = mediaState.finished || flag;

    if(mpv && !mediaState.finished)
    {
        int shouldPause = flag;
        mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &shouldPause);
    }
    if(gif)
    {
        gif->paused = flag;
    }
}

PostContentViewer::~PostContentViewer()
{
    std::cerr << "PostcontentViewer going down: " << (currentPost ? currentPost->title : "<no post>") << std::endl;
    //stopPlayingMedia();
    destroying = true;

    if(mpv)
    {
        const char *cmd[] = {"quit", nullptr};
        mpv_command(mpv,cmd);
    }
    if(mpvRenderContext)
    {
       // mediaFramebufferObject = 0;
       // postPicture->textureId = 0;
        mpv_render_context_free(mpvRenderContext);
        mpvRenderContext = nullptr;
    }
    if(mpv)
    {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }

    if(mediaFramebufferObject > 0)
    {
        glDeleteFramebuffers(1, &mediaFramebufferObject);
        mediaFramebufferObject = 0;
    }
}
void PostContentViewer::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = errorMessage;
    loadingPostContent = false;
}
void PostContentViewer::onMpvEvents(void* context)
{
    auto win=reinterpret_cast<PostContentViewer*>(context);
    if(!win) return;
    auto weak = win->weak_from_this();
    auto self = weak.lock();
    if (!self) return;
    if(self->destroying) return;
    boost::asio::post(win->uiExecutor,std::bind(&PostContentViewer::handleMpvEvents,self));
}
void PostContentViewer::handleMpvEvents()
{
    if(destroying) return;
    while (true)
    {
       mpv_event *mp_event = mpv_wait_event(mpv, 0);
       //std::cout << "event:"<<mp_event->event_id<<std::endl;
       if (mp_event->event_id == MPV_EVENT_NONE)
           break;
       switch(mp_event->event_id)
       {
       case MPV_EVENT_END_FILE:
       {
            mediaState.finished = true;
            mediaState.timePosition = mediaState.duration;
       }
           break;
       case MPV_EVENT_START_FILE:
       {
            mediaState.finished = false;
       }
           break;
       case MPV_EVENT_PROPERTY_CHANGE:
       {
           mpv_event_property *prop = (mpv_event_property *)mp_event->data;
          // std::cout << "prop change:"<<prop->name<<",format:"<<prop->format<<std::endl;
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
                   durationText = Utils::formatDuration(std::chrono::seconds((int)value));
               }
               else if (name == "width")
               {
                   mediaState.width = value;
               }
               else if (name == "height")
               {
                   mediaState.height = value;
               }
               else if (name == "volume")
               {
                   mediaState.mediaAudioVolume = value;
                   Database::getInstance()->setMediaAudioVolume(mediaState.mediaAudioVolume);
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
    UNUSED(value);
    //std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if(name == "volume")
    {
        //mediaState.mediaAudioVolume = value;
    }
}
void PostContentViewer::mpvFlagPropertyChanged(std::string name, int value)
{
   // std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if(name == "pause")
    {
        mediaState.paused = (bool)value;
    }
}
void PostContentViewer::mpvDoublePropertyChanged(std::string name, double value)
{
   // std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
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
    for(size_t i=0;i<currentPost->gallery.size();i++)
    {
        const auto& galImage = currentPost->gallery.at(i);
        gallery.images.emplace_back();
        if(galImage.url.empty()) continue;
        auto resourceConnection = client->makeResourceClientConnection();
        resourceConnection->connectionCompleteHandler(
                    [weak = weak_from_this(),index=i](const boost::system::error_code& ec,
                         resource_response response)
        {
            auto self = weak.lock();
            if (!self) return;
            if(ec)
            {
                boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,self,ec.message()));
                return;
            }
            else if(response.status == 200)
            {
                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setPostGalleryImage,self,
                                                             data,width,height,channels,(int)index));
            }
        });
        resourceConnection->getResource(galImage.url);
    }
}
void PostContentViewer::setPostGalleryImage(unsigned char* data, int width, int height, int channels, int index)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    gallery.images[index] = std::move(image);
    loadingPostContent = false;
}
void PostContentViewer::loadPostImage()
{
    loadingPostContent = true;

    if(currentPost->domain == "imgur.com")
    {
        auto mediaStreamingConnection = client->makeMediaStreamingClientConnection();
        mediaStreamingConnection->streamAvailableHandler([weak=weak_from_this()](HtmlParser::MediaLink link) {
            auto self = weak.lock();
            if (!self) return;
            self->downloadPostImage(link.url);
        });
        mediaStreamingConnection->errorHandler([weak = weak_from_this()](int /*errorCode*/,const std::string& str){
            auto self = weak.lock();
            if (!self) return;
            boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,self,str));
        });
        mediaStreamingConnection->streamMedia(currentPost.get());
    }
    else
    {
        //has media preview
        std::string mediaUrl;
        for (const auto& p : currentPost->previews)
        {
            for (const auto& v : p.variants)
            {
                if (v.kind == "mp4")
                {
                    mediaUrl = v.source.url;
                    break;
                }
            }
        }
        if (mediaUrl.empty())
        {
            downloadPostImage(currentPost->url);
        }
        else
        {
            setupMediaContext(mediaUrl);
        }
    }
}
void PostContentViewer::downloadPostImage(std::string url)
{
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
                [weak=weak_from_this(),url=url](const boost::system::error_code& ec,
                     resource_response response)
    {
        auto self = weak.lock();
        if (!self) return;
        if(ec)
        {
            boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setErrorMessage,self,ec.message()));
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
                boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setPostGif,self,
                                                             data,width,height,channels,count,delays));
            }
            else
            {
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(self->uiExecutor,std::bind(&PostContentViewer::setPostImage,self,data,width,height,channels));
            }
        }
    });
    resourceConnection->getResource(url);
}
void PostContentViewer::setupMediaContext(std::string file)
{
    mpv = mpv_create();
    //mpv_set_option_string(mpv, "idle", "no");
    mpv_set_option_string(mpv, "config", "no");
    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");

    //mpv_set_option_string(mpv, "cache", "yes");
    //int64_t maxBytes = 1024*1024*10;
    //mpv_set_option(mpv, "demuxer-max-bytes", MPV_FORMAT_INT64,&maxBytes);

    mpv_initialize(mpv);
    int64_t cacheDefault = 15000;
    int64_t cacheBackBuffer = 15000;
    int64_t cacheSecs = 10;
    mpv_set_property(mpv, "cache-default", MPV_FORMAT_INT64, &cacheDefault);
    mpv_set_property(mpv, "cache-backbuffer", MPV_FORMAT_INT64, &cacheBackBuffer);
    mpv_set_property(mpv, "cache-secs", MPV_FORMAT_INT64, &cacheSecs);
    //char* voProp = "libmpv";
    //char* hwdecInteropProp = "auto";
   // mpv_set_property(mpv, "vo", MPV_FORMAT_STRING,&voProp);
    // Enable opengl-hwdec-interop so we can set hwdec at runtime
    //mpv_set_property(mpv, "gpu-hwdec-interop",MPV_FORMAT_STRING, &hwdecInteropProp);

    int mpv_advanced_control = 1;
    if(useMediaHwAccel)
    {
        mpv_opengl_init_params gl_params = {get_proc_address_mpv, nullptr, nullptr};
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
            {MPV_RENDER_PARAM_ADVANCED_CONTROL, &mpv_advanced_control},
            {MPV_RENDER_PARAM_INVALID, 0}
        };
        mpv_render_context_create(&mpvRenderContext, mpv, params);
    }
    else
    {
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_SW)},
            {MPV_RENDER_PARAM_ADVANCED_CONTROL, &mpv_advanced_control},
            {MPV_RENDER_PARAM_INVALID, 0}
        };
        mpv_render_context_create(&mpvRenderContext, mpv, params);
    }
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "height", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "width", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);

    mediaState.finished = false;
    double vol = mediaState.mediaAudioVolume;
    mpv_set_property(mpv,"volume",MPV_FORMAT_DOUBLE,&vol);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    //TODO: figure out a way to pass these to a std::function bound to a shared_from_this or weak_from_this
    mpv_set_wakeup_callback(mpv, &PostContentViewer::onMpvEvents, this);
    mpv_render_context_set_update_callback(mpvRenderContext, &PostContentViewer::mpvRenderUpdate, this);
    std::cout << "playing URL:"<<file<<std::endl;
    const char *cmd[] = {"loadfile", file.c_str(), nullptr};
    mpv_command_async(mpv, 0, cmd);
}

void PostContentViewer::mpvRenderUpdate(void *context)
{
    auto win=reinterpret_cast<PostContentViewer*>(context);
    if(!win) return;
    auto weak = win->weak_from_this();
    auto self = weak.lock();
    if (!self) return;
    if(self->destroying) return;
    boost::asio::post(win->uiExecutor,std::bind(&PostContentViewer::setPostMediaFrame,self));
}
//void resetOpenGlState()
//{
//    glBindBuffer(GL_ARRAY_BUFFER,0);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_STENCIL_TEST);
//    glDisable(GL_SCISSOR_TEST);
//    glColorMask(true, true, true, true);
//    glClearColor(0, 0, 0, 0);
//    glDepthMask(true);
//    glDepthFunc(GL_LESS);
//    glClearDepthf(1);
//    glStencilMask(0xff);
//    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//    glStencilFunc(GL_ALWAYS, 0, 0xff);
//    glDisable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ZERO);
//    glUseProgram(0);
//}
void PostContentViewer::setPostMediaFrame()
{
    if(!currentPost || !mpvRenderContext || destroying) return;
    uint64_t flags = mpv_render_context_update(mpvRenderContext);
    if (!(flags & MPV_RENDER_UPDATE_FRAME))
    {
        return;
    }

    if(!postPicture)
    {
        if(mediaState.width <= 0 || mediaState.height <= 0) return;
        if(useMediaHwAccel)
        {
            glGenFramebuffers(1, &mediaFramebufferObject);
            glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
        }

        postPicture = std::make_unique<ResizableGLImage>();
        glGenTextures(1, &postPicture->textureId);
        glBindTexture(GL_TEXTURE_2D, postPicture->textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)mediaState.width, (int)mediaState.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        postPicture->width = (int)mediaState.width;
        postPicture->height = (int)mediaState.height;
        if(useMediaHwAccel)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                           postPicture->textureId, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    if(postPicture->width <= 0 || postPicture->height <=0 )
    {
        std::cerr << "eroneus picture width :"<<postPicture->width<<", and height:"<<postPicture->height<<std::endl;
        return;
    }
    int ret = 0;
    if(useMediaHwAccel)
    {
        mpv_opengl_fbo mpfbo{(int)mediaFramebufferObject,
                        postPicture->width,
                        postPicture->height, GL_RGBA};
        int flip_y{0};
        mpv_render_param params[] = {
                            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
                            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                            {MPV_RENDER_PARAM_INVALID,0}
                        };
        ret = mpv_render_context_render(mpvRenderContext, params);
    }
    else
    {
        int rgbaBitsPerPixel = 32;
        size_t stride = (postPicture->width * rgbaBitsPerPixel + 7) / 8;
        std::unique_ptr<uint8_t[]> pixels = std::make_unique<uint8_t[]>(stride*postPicture->height);
        int size[2] = {postPicture->width, postPicture->height};
        mpv_render_param params[] = {
                {MPV_RENDER_PARAM_SW_SIZE, size},
                {MPV_RENDER_PARAM_SW_FORMAT, (void*)"rgb0"},
                {MPV_RENDER_PARAM_SW_STRIDE, &stride},
                {MPV_RENDER_PARAM_SW_POINTER, pixels.get()},
                {MPV_RENDER_PARAM_INVALID,0}
        };
        ret = mpv_render_context_render(mpvRenderContext, params);
        if(ret == 0)
        {
            glBindTexture(GL_TEXTURE_2D, postPicture->textureId);
            glTexSubImage2D(GL_TEXTURE_2D,0,0,0,postPicture->width,postPicture->height,
                            GL_RGBA,GL_UNSIGNED_BYTE,pixels.get());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {

        }
    }


    loadingPostContent = false;

    if(ret < 0)
    {
        std::cerr << "error rendering:"<<mpv_error_string(ret)<<std::endl;
    }
}
void PostContentViewer::setPostGif(unsigned char* data, int width, int height, int channels,
                int count, int* delays)
{
    loadingPostContent = false;
    if(!data || width <= 0 || height <= 0 || count <= 0) return;
    auto gif = std::make_unique<post_gif>();
    gif->width = width;
    gif->height = height;
    int stride_bytes = width * channels;
    for (int i=0; i<count; ++i)
    {
        gif->images.emplace_back(std::make_unique<gif_image>(
                                     Utils::loadImage(data+stride_bytes * height * i, width, height, STBI_rgb_alpha),
                                     delays[i]));
    }

    free(delays);
    stbi_image_free(data);
    this->gif = std::move(gif);
}

void PostContentViewer::setPostImage(unsigned char* data, int width, int height, int channels)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    postPicture = std::move(image);
    loadingPostContent = false;
}

std::vector<GLuint> PostContentViewer::getAndResetTextures()
{
    std::vector<GLuint> textures;
    if(postPicture && postPicture->textureId > 0)
    {
        textures.push_back(std::exchange(postPicture->textureId,0));
    }
    if(gif)
    {
        for(auto&& img : gif->images)
        {
            if(img->img && img->img->textureId > 0)
            {
                textures.push_back(std::exchange(img->img->textureId,0));
            }
        }
    }
    if(currentPost && currentPost->isGallery && !currentPost->gallery.empty())
    {
        for(auto&& img : gallery.images)
        {
            if(img->textureId > 0)
            {
                textures.push_back(std::exchange(img->textureId,0));
            }
        }
    }

    return textures;
}
/*GLuint PostContentViewer::getAndResetMediaFBO()
{
    return 0;//std::exchange(mediaFramebufferObject,0);
}
*/
void PostContentViewer::showPostContent()
{

    if(!errorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",errorMessage.c_str());
        return;
    }

    ResizableGLImage* display_image = postPicture.get();

    if(gif && !gif->images.empty())
    {
        if(!gif->paused && gif->images[gif->currentImage]->displayed)
        {
            auto lastDisplay = gif->images[gif->currentImage]->lastDisplay;
            auto diffSinceLastDisplay = std::chrono::steady_clock::now() - lastDisplay;
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diffSinceLastDisplay);
            const auto delay = std::chrono::duration<int,std::milli>(gif->images[gif->currentImage]->delay);
            if(ms > delay)
            {
                gif->currentImage++;
                if(gif->currentImage >= (int)gif->images.size())
                {
                    gif->currentImage = 0;
                    for(auto&& img:gif->images)
                    {
                        img->displayed = false;
                        img->lastDisplay = std::chrono::steady_clock::time_point::min();
                    }
                }
                gif->images[gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
                gif->images[gif->currentImage]->displayed = true;
            }
        }
        else
        {
            gif->images[gif->currentImage]->displayed = true;
            gif->images[gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
        }
        display_image = gif->images[gif->currentImage]->img.get();
        display_image->resizedWidth = gif->width;
        display_image->resizedHeight = gif->height;
    }

    if(currentPost->isGallery && !currentPost->gallery.empty())
    {
        if(gallery.currentImage < 0 || gallery.currentImage >= (int)gallery.images.size())
        {
            gallery.currentImage = 0;
        }
        display_image = gallery.images[gallery.currentImage].get();
    }

    if(display_image)
    {
        if(!display_image->isResized)
        {
            float width = (float)display_image->width;
            float height = (float)display_image->height;
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
            display_image->resizedWidth = width;
            display_image->resizedHeight = height;
            display_image->pictureRatio = width / height;
            display_image->isResized = true;
            if(gif)
            {
                for(auto&& img:gif->images)
                {
                    img->img->resizedWidth = width;
                    img->img->resizedHeight = height;
                    img->img->pictureRatio = width / height;
                    img->img->isResized = true;
                }
            }
        }

        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::ImageButton((void*)(intptr_t)display_image->textureId,
                           ImVec2(display_image->resizedWidth,display_image->resizedHeight),ImVec2(0, 0),ImVec2(1,1),0);
        ImGui::PopStyleColor(3);

        if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) &&
                ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax()) &&
                ImGui::IsMouseDragging(ImGuiMouseButton_Left)
                )
        {
            //keep same aspect ratio
            auto x = ImGui::GetIO().MouseDelta.x;
            auto y = ImGui::GetIO().MouseDelta.y;
            auto new_width = display_image->resizedWidth + x;
            auto new_height = display_image->resizedHeight + y;
            auto new_area = new_width * new_height;
            new_width = std::sqrt(display_image->pictureRatio * new_area);
            new_height = new_area / new_width;
            display_image->resizedWidth = std::max(100.f,new_width);
            display_image->resizedHeight = std::max(100.f,new_height);
        }
        if(gif)
        {
            gif->width = display_image->resizedWidth;
            gif->height = display_image->resizedHeight;
        }

        if(mediaState.duration > 0.0f && mpvRenderContext)
        {            
            showMediaControls(display_image->resizedWidth);
        }
        if(currentPost->isGallery && !gallery.images.empty())
        {
            showGalleryControls(display_image->resizedWidth);
        }
    }

    if(markdown)
    {
        markdown->RenderMarkdown();
    }
    else if(!display_image && loadingPostContent)
    {
        ImGui::Spinner("###spinner_loading_data",50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }
}
void PostContentViewer::showGalleryControls(int width)
{
    if(ImGui::Button(galleryButtonPreviousText.c_str()))
    {
        gallery.currentImage--;
        if(gallery.currentImage < 0) gallery.currentImage = (int)gallery.images.size() - 1;
    }
    auto btnSize = ImGui::GetItemRectSize();
    auto text = fmt::format("{}/{}",gallery.currentImage+1,gallery.images.size());
    auto textSize = ImGui::CalcTextSize(text.c_str());
    auto space = (width - btnSize.x * 2.f);
    ImGui::SameLine((space - textSize.x / 2.f)/2.f);
    ImGui::Text("%s",text.c_str());
    auto windowWidth = ImGui::GetWindowContentRegionMax().x;
    auto remainingWidth = windowWidth - width;
    ImGui::SameLine(windowWidth-remainingWidth-btnSize.x*2.f/3.f);
    if(ImGui::Button(galleryButtonNextText.c_str()))
    {
        gallery.currentImage++;
        if(gallery.currentImage >= (int)gallery.images.size()) gallery.currentImage = 0;
    }
}
void PostContentViewer::showMediaControls(int width)
{
    auto progressHeight = ImGui::GetFontSize() * 0.5f;
    auto newTimeSeek = MediaWidget::mediaProgressBar(mediaState.timePosition, mediaState.duration,
                                                      ImVec2(width, progressHeight));
    if( newTimeSeek != 0)
    {
        auto seekStr = std::to_string(newTimeSeek);
        const char *cmd[] = {"seek",seekStr.c_str(), nullptr};
        mpv_command_async(mpv,0,cmd);
    }
    /*if(ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(Utils::formatDuration(std::chrono::seconds((int)mediaState.timePosition)).c_str());
        ImGui::EndTooltip();
    }*/
    if(mediaState.finished)
    {
        if(ImGui::Button(mediaButtonRestartText.c_str()))
        {
            const char *cmd[] = {"playlist-play-index","0", nullptr};
            mpv_command_async(mpv,0,cmd);
        }
    }
    else
    {
        if(ImGui::Button(mediaButtonFastBackwardText.c_str()))
        {
            const char *cmd[] = {"seek","-60", nullptr};
            mpv_command_async(mpv,0,cmd);
        }
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek backwards 60 seconds");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if(ImGui::Button(mediaButtonBackwardText.c_str()))
        {
            const char *cmd[] = {"seek","-10", nullptr};
            mpv_command_async(mpv,0,cmd);
        }
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek backwards 10 seconds");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if(ImGui::Button(mediaState.paused ? mediaButtonPlayText.c_str() : mediaButtonPauseText.c_str()))
        {
            int shouldPause = !mediaState.paused;
            mpv_set_property_async(mpv,0,"pause",MPV_FORMAT_FLAG,&shouldPause);
        }
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Play/Pause media");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if(ImGui::Button(mediaButtonForwardText.c_str()))
        {
            const char *cmd[] = {"seek","10", nullptr};
            mpv_command_async(mpv,0,cmd);
        }
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek forward 10 seconds");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if(ImGui::Button(mediaButtonFastForwardText.c_str()))
        {
            const char *cmd[] = {"seek","60", nullptr};
            mpv_command_async(mpv,0,cmd);
        }
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek forward 60 seconds");
            ImGui::EndTooltip();
        }

    }
    ImGui::SameLine();
    ImGui::TextUnformatted(reinterpret_cast<const char*>(ICON_FA_VOLUME_UP));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.f);
    int volume = mediaState.mediaAudioVolume;
    if(ImGui::SliderInt(mediaSliderVolumeText.c_str(),&volume,0,100,std::to_string(volume).c_str()))
    {
        mediaState.mediaAudioVolume = volume;
        double vold = volume;
        mpv_set_property_async(mpv,0,"volume",MPV_FORMAT_DOUBLE,&vold);
    }
    ImGui::SameLine();
    ImGui::Text("%s/%s",Utils::formatDuration(std::chrono::seconds((int)mediaState.timePosition)).c_str(),durationText.c_str());
    /*ImGui::SameLine();
    if(ImGui::Checkbox(loopCheckboxText.c_str(),&mediaLoop))
    {
        const char *cmd[] = {"loop-file",mediaLoop ? "inf" : "no", nullptr};
        mpv_command_async(mpv,0,cmd);
    }*/
}
