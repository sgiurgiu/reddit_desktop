#include "postcontentviewer.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "database.h"
#include "fonts/IconsFontAwesome4.h"
#include "spinner/spinner.h"
#include <fmt/format.h>
#include <imgui.h>

#include "macros.h"
#include "mediawidget.h"
#include "utils.h"
#include <GL/glew.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>

#ifdef RD_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif

namespace
{
const std::vector<std::string> mediaExtensions = { ".gif",  ".gifv", ".mp4",
                                                   ".webv", ".avi",  ".mkv" };
static void* get_proc_address_mpv(void* fn_ctx, const char* name)
{
    UNUSED(fn_ctx);
    // std::cout << "get_proc_address_mpv:"<<name<<std::endl;
    return (void*)glfwGetProcAddress(name);
}
} // namespace

PostContentViewer::PostContentViewer(RedditClientProducer* client,
                                     const access_token& token,
                                     const boost::asio::any_io_executor& uiExecutor)
: client(client), token(token), uiExecutor(uiExecutor)
{
    glfwGetFramebufferSize(glfwGetCurrentContext(), nullptr, &windowHeight);
    mediaState.mediaAudioVolume = Database::getInstance()->getMediaAudioVolume();
    useMediaHwAccel = Database::getInstance()->getUseHWAccelerationForMedia();
    useYoutubeDlder = Database::getInstance()->getUseYoutubeDownloader();
    mediaDomains = Database::getInstance()->getMediaDomains();
#ifndef MPV_RENDER_API_TYPE_SW
    useMediaHwAccel = true;
#endif
}
void PostContentViewer::loadCommentContent(
    const comment_media_metadata& commentMediaMetadata)
{
    mediaButtonRestartText = fmt::format(
        "{}##_restartMedia_{}", reinterpret_cast<const char*>(ICON_FA_REPEAT),
        commentMediaMetadata.id);
    mediaButtonFastBackwardText =
        fmt::format("{}##_fastBackward_{}",
                    reinterpret_cast<const char*>(ICON_FA_FAST_BACKWARD),
                    commentMediaMetadata.id);
    mediaButtonBackwardText = fmt::format(
        "{}##_Backward_{}", reinterpret_cast<const char*>(ICON_FA_BACKWARD),
        commentMediaMetadata.id);
    mediaButtonPlayText = fmt::format("{}##_playMedia_{}",
                                      reinterpret_cast<const char*>(ICON_FA_PLAY),
                                      commentMediaMetadata.id);
    mediaButtonPauseText = fmt::format(
        "{}##_pauseMedia_{}", reinterpret_cast<const char*>(ICON_FA_PAUSE),
        commentMediaMetadata.id);
    mediaButtonForwardText = fmt::format(
        "{}##_Forward_{}", reinterpret_cast<const char*>(ICON_FA_FORWARD),
        commentMediaMetadata.id);
    mediaButtonFastForwardText =
        fmt::format("{}##_fastForward_{}",
                    reinterpret_cast<const char*>(ICON_FA_FAST_FORWARD),
                    commentMediaMetadata.id);
    mediaSliderVolumeText =
        fmt::format("##_volumeMedia_{}", commentMediaMetadata.id);
    galleryButtonPreviousText = fmt::format(
        "{}##_previmage_{}", reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT),
        commentMediaMetadata.id);
    galleryButtonNextText = fmt::format(
        "{}##_nextimage_{}", reinterpret_cast<const char*>(ICON_FA_ARROW_RIGHT),
        commentMediaMetadata.id);
    loopCheckboxText = fmt::format("Loop##_loop_{}", commentMediaMetadata.id);
    mediaProgressSliderText =
        fmt::format("##_progressSlider_{}", commentMediaMetadata.id);

    if (commentMediaMetadata.commentMedia)
    {
        if (commentMediaMetadata.commentMedia->oemEmbed)
        {
            justPlayMedia = true;
            setupMediaContext(
                commentMediaMetadata.commentMedia->oemEmbed->providerUrl, true);
        }
    }
    else if (commentMediaMetadata.image)
    {
        downloadPostImage(commentMediaMetadata.image->url);
    }
    else if (!commentMediaMetadata.ext.empty())
    {
        justPlayMedia = true;
        setupMediaContext(commentMediaMetadata.ext, false);
    }
}
void PostContentViewer::loadContent(post_ptr currentPost)
{
    this->currentPost = currentPost;
    currentPostSet = true;
    mediaButtonRestartText = fmt::format(
        "{}##_restartMedia_{}", reinterpret_cast<const char*>(ICON_FA_REPEAT),
        currentPost->name);
    mediaButtonFastBackwardText =
        fmt::format("{}##_fastBackward_{}",
                    reinterpret_cast<const char*>(ICON_FA_FAST_BACKWARD),
                    currentPost->name);
    mediaButtonBackwardText = fmt::format(
        "{}##_Backward_{}", reinterpret_cast<const char*>(ICON_FA_BACKWARD),
        currentPost->name);
    mediaButtonPlayText = fmt::format("{}##_playMedia_{}",
                                      reinterpret_cast<const char*>(ICON_FA_PLAY),
                                      currentPost->name);
    mediaButtonPauseText = fmt::format(
        "{}##_pauseMedia_{}", reinterpret_cast<const char*>(ICON_FA_PAUSE),
        currentPost->name);
    mediaButtonForwardText = fmt::format(
        "{}##_Forward_{}", reinterpret_cast<const char*>(ICON_FA_FORWARD),
        currentPost->name);
    mediaButtonFastForwardText = fmt::format(
        "{}##_fastForward_{}",
        reinterpret_cast<const char*>(ICON_FA_FAST_FORWARD), currentPost->name);
    mediaSliderVolumeText = fmt::format("##_volumeMedia_{}", currentPost->name);
    galleryButtonPreviousText = fmt::format(
        "{}##_previmage_{}", reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT),
        currentPost->name);
    galleryButtonNextText = fmt::format(
        "{}##_nextimage_{}", reinterpret_cast<const char*>(ICON_FA_ARROW_RIGHT),
        currentPost->name);
    loopCheckboxText = fmt::format("Loop##_loop_{}", currentPost->name);
    mediaProgressSliderText =
        fmt::format("##_progressSlider_{}", currentPost->name);

    if (!currentPost->selfText.empty())
    {
        markdown = std::make_unique<MarkdownRenderer>(currentPost->selfText,
                                                      client, uiExecutor);
        loadingPostContent = false;
    }
    if (currentPost->isGallery && !currentPost->gallery.empty())
    {
        loadPostEmbeddedGalleryImages();
        return;
    }

    if (this->currentPost->allow_live_comments &&
        this->currentPost->postHint.empty() &&
        this->currentPost->url.starts_with("https://www.reddit.com/live/"))
    {
        liveThreadViewer =
            std::make_shared<LiveThreadViewer>(client, token, uiExecutor);
        liveThreadViewer->loadContent(this->currentPost->url);
        return;
    }

    if (this->currentPost->domain == "twitter.com" &&
        this->currentPost->postHint == "link" &&
        this->currentPost->url.starts_with("https://twitter.com/") &&
        this->currentPost->url.find("/status/") != std::string::npos)
    {
        auto twitterBearerOpt = Database::getInstance()->getTwitterAuthBearer();
        if (twitterBearerOpt)
        {
            twitterRenderer = std::make_shared<TwitterRenderer>(
                client, uiExecutor, twitterBearerOpt.value());
            twitterRenderer->LoadTweet(this->currentPost->url);
        }
    }

    bool isMediaPost = std::find(mediaDomains.begin(), mediaDomains.end(),
                                 currentPost->domain) != mediaDomains.end();
    if (!isMediaPost)
    {
        auto idx = currentPost->url.find_last_of('.');
        if (idx != std::string::npos)
        {
            isMediaPost =
                std::find(mediaExtensions.begin(), mediaExtensions.end(),
                          currentPost->url.substr(idx)) != mediaExtensions.end();
        }
    }

    if (!isMediaPost && currentPost->postHint != "image")
    {
        if (currentPost->url.starts_with("/r/"))
        {
            // this is a crosspost from another subreddit
            auto listingConnection = client->makeListingClientConnection();
            listingConnection->connectionCompleteHandler(
                [weak = weak_from_this()](const boost::system::error_code& ec,
                                          client_response<listing> response)
                {
                    auto self = weak.lock();
                    if (!self)
                        return;
                    if (ec)
                    {
                        boost::asio::post(
                            self->uiExecutor,
                            std::bind(&PostContentViewer::setErrorMessage, self,
                                      ec.message()));
                    }
                    else
                    {
                        if (response.status >= 400)
                        {
                            boost::asio::post(
                                self->uiExecutor,
                                std::bind(&PostContentViewer::setErrorMessage,
                                          self, response.body));
                        }
                        else
                        {
                            self->loadPostFromListing(std::move(response.data));
                        }
                    }
                });

            listingConnection->list(currentPost->url, token);
        }
        else
        {
            return;
        }
    }
    if (currentPost->postHint == "image" && !currentPost->url.empty() &&
        !currentPost->url.ends_with(".gifv"))
    {
        loadPostImage();
    }
    else if (currentPost->postHint != "self")
    {
        if (isMediaPost)
        {
            loadingPostContent = true;
            if (false)
            {
                setupMediaContext(currentPost->url, false);
            }
            else
            {
                auto urlDetectionConnection =
                    client->makeUrlDetectionClientConnection();
                urlDetectionConnection->streamAvailableHandler(
                    [weak = weak_from_this()](HtmlParser::MediaLink link)
                    {
                        auto self = weak.lock();
                        if (!self)
                            return;
                        switch (link.type)
                        {
                        case HtmlParser::MediaType::Video:
                            boost::asio::post(
                                self->uiExecutor,
                                std::bind(&PostContentViewer::setupMediaContext,
                                          self, *link.urls.begin(), link.useLink));
                            break;
                        case HtmlParser::MediaType::Image:
                            boost::asio::post(
                                self->uiExecutor,
                                std::bind(&PostContentViewer::downloadPostImage,
                                          self, *link.urls.begin()));
                            break;
                        case HtmlParser::MediaType::Gallery:
                            boost::asio::post(
                                self->uiExecutor,
                                std::bind(&PostContentViewer::loadPostGalleryImages,
                                          self, std::move(link.urls)));
                            break;
                        case HtmlParser::MediaType::Gif:
                            break;
                        default:
                            if (!link.urls.empty())
                            {
                                boost::asio::post(
                                    self->uiExecutor,
                                    std::bind(&PostContentViewer::setupMediaContext,
                                              self, *link.urls.begin(), false));
                            }
                            break;
                        }
                    });
                urlDetectionConnection->errorHandler(
                    [weak = weak_from_this()](int /*errorCode*/,
                                              const std::string& str)
                    {
                        auto self = weak.lock();
                        if (!self)
                            return;
                        boost::asio::post(
                            self->uiExecutor,
                            std::bind(&PostContentViewer::setErrorMessage, self,
                                      str));
                    });
                urlDetectionConnection->detectMediaUrl(currentPost.get());
            }
        }
    }
}
void PostContentViewer::loadPostFromListing(const listing& listingResponse)
{
    if (listingResponse.json.is_array())
    {
        for (const auto& child : listingResponse.json)
        {
            if (!child.is_object())
            {
                continue;
            }
            if (!child.contains("kind") ||
                child["kind"].get<std::string>() != "Listing")
            {
                continue;
            }
            if (!child.contains("data") || !child["data"].contains("children"))
            {
                continue;
            }
            for (const auto& grandChild : child["data"]["children"])
            {
                if (!grandChild.is_object())
                {
                    continue;
                }
                if (!grandChild.contains("kind") ||
                    !grandChild.contains("data"))
                {
                    continue;
                }
                auto kind = grandChild["kind"].get<std::string>();
                if (kind == "t3")
                {
                    // load post
                    auto pp = std::make_shared<post>(grandChild["data"]);
                    boost::asio::post(
                        uiExecutor,
                        std::bind(&PostContentViewer::loadContent, this, pp));
                }
            }
        }
    }
}
void PostContentViewer::stopPlayingMedia(bool flag)
{
    mediaState.paused = mediaState.finished || flag;

    if (mpv && !mediaState.finished)
    {
        int shouldPause = flag;
        mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &shouldPause);
    }
    if (gif)
    {
        gif->paused = flag;
    }
}

PostContentViewer::~PostContentViewer()
{
    spdlog::debug("PostcontentViewer going down: {} ",
                  (currentPost ? currentPost->title : "<no post>"));
    // stopPlayingMedia();
    std::lock_guard<std::mutex> _(mediaMutex);

    destroying = true;

    if (mpv)
    {
        const char* cmd[] = { "quit", nullptr };
        mpv_command(mpv, cmd);
    }
    if (mpvRenderContext)
    {
        // mediaFramebufferObject = 0;
        // postPicture->textureId = 0;
        mpv_render_context_free(mpvRenderContext);
        mpvRenderContext = nullptr;
    }
    if (mpv)
    {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }

    if (mediaFramebufferObject > 0)
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
void PostContentViewer::loadPostGalleryImages(std::vector<std::string> urls)
{
    loadingPostContent = true;
    if (currentPost)
        currentPost->isGallery = true;
    int index = 0;
    for (const auto& galImage : urls)
    {
        // reserve space in the list to place the image here
        // at this index since we do not know the order the images arrive
        gallery.images.emplace_back();
        if (galImage.empty())
            continue;

        auto resourceConnection = client->makeResourceClientConnection();
        resourceConnection->connectionCompleteHandler(
            [weak = weak_from_this(), index](const boost::system::error_code& ec,
                                             resource_response response)
            {
                auto self = weak.lock();
                if (!self)
                    return;
                if (ec)
                {
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&PostContentViewer::setErrorMessage,
                                                self, ec.message()));
                    return;
                }
                else if (response.status == 200)
                {
                    int width, height, channels;
                    auto data = Utils::decodeImageData(
                        response.data.data(), response.data.size(), &width,
                        &height, &channels);
                    boost::asio::post(
                        self->uiExecutor,
                        std::bind(&PostContentViewer::setPostGalleryImage, self,
                                  data, width, height, channels, index));
                }
            });
        resourceConnection->getResource(galImage);
        index++;
    }
}
void PostContentViewer::loadPostEmbeddedGalleryImages()
{
    loadingPostContent = true;
    std::vector<std::string> urls{ currentPost->gallery.size() };
    std::transform(currentPost->gallery.begin(), currentPost->gallery.end(),
                   urls.begin(), [](const auto& g) { return g.url; });
    loadPostGalleryImages(std::move(urls));
}
void PostContentViewer::setPostGalleryImage(
    Utils::STBImagePtr data, int width, int height, int channels, int index)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data.get(), width, height, STBI_rgb_alpha);
    gallery.images[index] = std::move(image);
    loadingPostContent = false;
}
void PostContentViewer::loadPostImage()
{
    loadingPostContent = true;

    if (currentPost->domain == "imgur.com")
    {
        auto urlDetectionConnection = client->makeUrlDetectionClientConnection();
        urlDetectionConnection->streamAvailableHandler(
            [weak = weak_from_this()](HtmlParser::MediaLink link)
            {
                if (link.urls.empty())
                    return;
                auto self = weak.lock();
                if (!self)
                    return;
                self->downloadPostImage(*link.urls.begin());
            });
        urlDetectionConnection->errorHandler(
            [weak = weak_from_this()](int /*errorCode*/, const std::string& str)
            {
                auto self = weak.lock();
                if (!self)
                    return;
                boost::asio::post(
                    self->uiExecutor,
                    std::bind(&PostContentViewer::setErrorMessage, self, str));
            });
        urlDetectionConnection->detectMediaUrl(currentPost.get());
    }
    else
    {
        // has media preview
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
            setupMediaContext(mediaUrl, true);
        }
    }
}
void PostContentViewer::downloadPostImage(std::string url)
{
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
        [weak = weak_from_this(), url = url](const boost::system::error_code& ec,
                                             resource_response response)
        {
            auto self = weak.lock();
            if (!self)
                return;
            if (ec)
            {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&PostContentViewer::setErrorMessage,
                                            self, ec.message()));
                return;
            }
            if (response.status == 200)
            {
                int* delays = nullptr;
                int count;
                int width, height, channels;
                if (url.ends_with(".gif"))
                {
                    auto data = Utils::decodeGifData(
                        response.data.data(), response.data.size(), &width,
                        &height, &channels, &count, &delays);
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&PostContentViewer::setPostGif,
                                                self, data, width, height,
                                                channels, count, delays));
                }
                else
                {
                    auto data = Utils::decodeImageData(
                        response.data.data(), response.data.size(), &width,
                        &height, &channels);
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&PostContentViewer::setPostImage,
                                                self, data, width, height,
                                                channels));
                }
            }
        });
    resourceConnection->getResource(url);
}
void PostContentViewer::setupMediaContext(std::string file, bool useProvidedFile)
{
    if (file.empty() && currentPost)
    {
        file = currentPost->url;
    }
    std::lock_guard<std::mutex> _(mediaMutex);
    mpv = mpv_create();
    mpv_set_property_string(mpv, "sub-create-cc-track", "yes");
    mpv_set_property_string(mpv, "input-default-bindings", "no");
    mpv_set_property_string(mpv, "config", "no");
    mpv_set_property_string(mpv, "input-vo-keyboard", "no");
    mpv_set_property_string(mpv, "vo", "libmpv");
    if (useYoutubeDlder && !useProvidedFile)
    {
        file = "ytdl://" + currentPost->url;
        mpv_set_option_string(mpv, "ytdl", "yes");
        mpv_set_option_string(
            mpv,
            "ytdl-format", "webm[height<720]+bestaudio/bestvideo[height<720]+bestaudio/best[height<720]/best");
    }

    mpv_set_option_string(mpv, "cache", "yes");
    // int64_t maxBytes = 1024*1024*10;
    // mpv_set_option(mpv, "demuxer-max-bytes", MPV_FORMAT_INT64,&maxBytes);

    // int64_t cacheDefault = 150000;
    // int64_t cacheBackBuffer = 150000;
    int64_t cacheSecs = 30;
    // mpv_set_property(mpv, "cache-default", MPV_FORMAT_INT64, &cacheDefault);
    // mpv_set_property(mpv, "cache-backbuffer", MPV_FORMAT_INT64, &cacheBackBuffer);
    mpv_set_property(mpv, "cache-secs", MPV_FORMAT_INT64, &cacheSecs);
    mpv_set_property(mpv, "demuxer-readahead-secs", MPV_FORMAT_INT64, &cacheSecs);
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "height", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "width", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    // mpv_observe_property(mpv, 0, "demuxer-cache-state", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "demuxer-cache-time", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "demuxer-cache-duration", MPV_FORMAT_DOUBLE);

    if (mpv_initialize(mpv) < 0)
    {
        setErrorMessage("Could not initialize mpv");
        return;
    }

    mediaState.finished = false;
    double vol = mediaState.mediaAudioVolume;
    mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &vol);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    // TODO: figure out a way to pass these to a std::function bound to a
    // shared_from_this or weak_from_this
    mpv_set_wakeup_callback(mpv, &PostContentViewer::onMpvEvents, this);

    // mpv_set_property_string(mpv, "demuxer-cache-wait", "yes");

    // demuxer-cache-wait="yes|no

    // char* voProp = "libmpv";
    // char* hwdecInteropProp = "auto";
    // mpv_set_property(mpv, "vo", MPV_FORMAT_STRING,&voProp);
    // Enable opengl-hwdec-interop so we can set hwdec at runtime
    // mpv_set_property(mpv, "gpu-hwdec-interop",MPV_FORMAT_STRING,
    // &hwdecInteropProp);

    int mpv_advanced_control = 0;
    if (useMediaHwAccel)
    {
#ifdef RD_LINUX
        mpv_render_param display{ MPV_RENDER_PARAM_X11_DISPLAY,
                                  glfwGetX11Display() };
#endif
        mpv_opengl_init_params gl_params = { get_proc_address_mpv, nullptr };
        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_API_TYPE,
              const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
            { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params },
            { MPV_RENDER_PARAM_ADVANCED_CONTROL, &mpv_advanced_control },
#ifdef RD_LINUX
            { display },
#endif
            { MPV_RENDER_PARAM_INVALID, 0 }
        };
        mpv_render_context_create(&mpvRenderContext, mpv, params);
    }
    else
    {
#ifdef MPV_RENDER_API_TYPE_SW
        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_API_TYPE,
              const_cast<char*>(MPV_RENDER_API_TYPE_SW) },
            { MPV_RENDER_PARAM_ADVANCED_CONTROL, &mpv_advanced_control },
            { MPV_RENDER_PARAM_INVALID, 0 }
        };
        mpv_render_context_create(&mpvRenderContext, mpv, params);
#endif
    }
    mpv_render_context_set_update_callback(
        mpvRenderContext, &PostContentViewer::mpvRenderUpdate, this);
    spdlog::debug("playing URL: {}", file);
    const char* cmd[] = { "loadfile", file.c_str(), nullptr };
    mpv_command_async(mpv, 0, cmd);
    if (mediaState.paused)
    {
        int shouldPause = mediaState.paused;
        mediaState.paused = false;
        // mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG, &shouldPause);
    }
}

void PostContentViewer::mpvRenderUpdate(void* context)
{
    auto win = reinterpret_cast<PostContentViewer*>(context);
    if (!win)
        return;
    auto weak = win->weak_from_this();
    auto self = weak.lock();
    if (!self)
        return;
    if (self->destroying)
        return;
    boost::asio::post(win->uiExecutor,
                      std::bind(&PostContentViewer::setPostMediaFrame, self));
}
void PostContentViewer::onMpvEvents(void* context)
{
    auto win = reinterpret_cast<PostContentViewer*>(context);
    if (!win)
        return;
    auto weak = win->weak_from_this();
    auto self = weak.lock();
    if (!self)
        return;
    if (self->destroying)
        return;
    boost::asio::post(win->uiExecutor,
                      std::bind(&PostContentViewer::handleMpvEvents, self));
}
void PostContentViewer::handleMpvEvents()
{
    if (destroying)
        return;
    std::lock_guard<std::mutex> _(mediaMutex);
    while (true)
    {
        mpv_event* mp_event = mpv_wait_event(mpv, 0);

        if (mp_event->event_id == MPV_EVENT_NONE)
            break;
        switch (mp_event->event_id)
        {
        case MPV_EVENT_END_FILE:
        {
            mediaState.finished = true;
            mediaState.timePosition = mediaState.duration;
            if (mp_event->error < 0)
            {
                errorMessage = "Cannot play video file";
                // mpv_event_end_file* ev_data =
                // (mpv_event_end_file*)mp_event->data; ev_data->
            }
        }
        break;
        case MPV_EVENT_START_FILE:
        {
            mediaState.finished = false;
        }
        break;
        case MPV_EVENT_PROPERTY_CHANGE:
        {
            mpv_event_property* prop = (mpv_event_property*)mp_event->data;
            // std::cout << "prop
            // change:"<<prop->name<<",format:"<<prop->format<<std::endl;
            std::string name(prop->name);
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double value = *(double*)prop->data;
                // boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvDoublePropertyChanged,this,
                //                                              std::string(prop->name),val));
                if (name == "time-pos")
                {
                    mediaState.timePosition = value;
                }
                else if (name == "duration")
                {
                    mediaState.duration = value;
                    durationText =
                        Utils::formatDuration(std::chrono::seconds((int)value));
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
                    Database::getInstance()->setMediaAudioVolume(
                        mediaState.mediaAudioVolume);
                }
                else if (name == "demuxer-cache-time")
                {
                    mediaState.cachedTime = value;
                }
                else if (name == "demuxer-cache-duration")
                {
                    mediaState.cachedDuration = value;
                }
            }
            else if (prop->format == MPV_FORMAT_FLAG)
            {
                int value = *(int*)prop->data;
                // boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvFlagPropertyChanged,this,
                //                                              std::string(prop->name),val));
                if (name == "pause")
                {
                    mediaState.paused = (bool)value;
                }
            }
            else if (prop->format == MPV_FORMAT_INT64)
            {
                // int64_t val = *(int64_t*)prop->data;

                // boost::asio::post(this->uiExecutor,std::bind(&PostContentViewer::mpvInt64PropertyChanged,this,
                //                                              std::string(prop->name),val));
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
    // std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if (name == "volume")
    {
        // mediaState.mediaAudioVolume = value;
    }
}
void PostContentViewer::mpvFlagPropertyChanged(std::string name, int value)
{
    // std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if (name == "pause")
    {
        mediaState.paused = (bool)value;
    }
}
void PostContentViewer::mpvDoublePropertyChanged(std::string name, double value)
{
    // std::cout << "prop change :"<<name<<", val:"<<value<<std::endl;
    if (name == "time-pos")
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

// void resetOpenGlState()
//{
//     glBindBuffer(GL_ARRAY_BUFFER,0);
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, 0);
//     glDisable(GL_DEPTH_TEST);
//     glDisable(GL_STENCIL_TEST);
//     glDisable(GL_SCISSOR_TEST);
//     glColorMask(true, true, true, true);
//     glClearColor(0, 0, 0, 0);
//     glDepthMask(true);
//     glDepthFunc(GL_LESS);
//     glClearDepthf(1);
//     glStencilMask(0xff);
//     glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//     glStencilFunc(GL_ALWAYS, 0, 0xff);
//     glDisable(GL_BLEND);
//     glBlendFunc(GL_ONE, GL_ZERO);
//     glUseProgram(0);
// }
void PostContentViewer::setPostMediaFrame()
{
    std::lock_guard<std::mutex> _(mediaMutex);
    if ((!justPlayMedia && !currentPost) || !mpvRenderContext || destroying)
        return;

    uint64_t flags = mpv_render_context_update(mpvRenderContext);
    if (!(flags & MPV_RENDER_UPDATE_FRAME))
    {
        return;
    }

    if (!postPicture)
    {
        if (mediaState.width <= 0 || mediaState.height <= 0)
            return;
        if (useMediaHwAccel)
        {
            glGenFramebuffers(1, &mediaFramebufferObject);
            glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
        }

        postPicture = std::make_unique<ResizableGLImage>();
        glGenTextures(1, &postPicture->textureId);
        glBindTexture(GL_TEXTURE_2D, postPicture->textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)mediaState.width,
                     (int)mediaState.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);

        postPicture->width = (int)mediaState.width;
        postPicture->height = (int)mediaState.height;
        if (useMediaHwAccel)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, postPicture->textureId, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    if (postPicture->width <= 0 || postPicture->height <= 0)
    {
        std::cerr << "eroneus picture width :" << postPicture->width
                  << ", and height:" << postPicture->height << std::endl;
        return;
    }
    int ret = 0;
    if (useMediaHwAccel)
    {
        mpv_opengl_fbo mpfbo{ (int)mediaFramebufferObject, postPicture->width,
                              postPicture->height, GL_RGBA };
        int flip_y{ 0 };
        mpv_render_param params[] = { { MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo },
                                      { MPV_RENDER_PARAM_FLIP_Y, &flip_y },
                                      { MPV_RENDER_PARAM_INVALID, 0 } };
        ret = mpv_render_context_render(mpvRenderContext, params);
    }
    else
    {
#ifdef MPV_RENDER_API_TYPE_SW
        int rgbaBitsPerPixel = 32;
        size_t stride = (postPicture->width * rgbaBitsPerPixel + 7) / 8;
        std::unique_ptr<uint8_t[]> pixels =
            std::make_unique<uint8_t[]>(stride * postPicture->height);
        int size[2] = { postPicture->width, postPicture->height };
        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_SW_SIZE, size },
            { MPV_RENDER_PARAM_SW_FORMAT, (void*)"rgb0" },
            { MPV_RENDER_PARAM_SW_STRIDE, &stride },
            { MPV_RENDER_PARAM_SW_POINTER, pixels.get() },
            { MPV_RENDER_PARAM_INVALID, 0 }
        };
        ret = mpv_render_context_render(mpvRenderContext, params);
        if (ret == 0)
        {
            glBindTexture(GL_TEXTURE_2D, postPicture->textureId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, postPicture->width,
                            postPicture->height, GL_RGBA, GL_UNSIGNED_BYTE,
                            pixels.get());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
#endif
    }

    loadingPostContent = false;

    if (ret < 0)
    {
        std::cerr << "error rendering:" << mpv_error_string(ret) << std::endl;
    }
}
void PostContentViewer::setPostGif(Utils::STBImagePtr data,
                                   int width,
                                   int height,
                                   int channels,
                                   int count,
                                   int* delays)
{
    loadingPostContent = false;
    if (!data || width <= 0 || height <= 0 || count <= 0)
        return;
    auto gif = std::make_unique<post_gif>();
    gif->width = width;
    gif->height = height;
    int stride_bytes = width * channels;
    for (int i = 0; i < count; ++i)
    {
        gif->images.emplace_back(std::make_unique<gif_image>(
            Utils::loadImage(data.get() + stride_bytes * height * i, width,
                             height, STBI_rgb_alpha),
            delays[i]));
    }

    free(delays);
    this->gif = std::move(gif);
}

void PostContentViewer::setPostImage(Utils::STBImagePtr data,
                                     int width,
                                     int height,
                                     int channels)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data.get(), width, height, STBI_rgb_alpha);
    postPicture = std::move(image);
    loadingPostContent = false;
}

std::vector<GLuint> PostContentViewer::getAndResetTextures()
{
    std::vector<GLuint> textures;
    if (postPicture && postPicture->textureId > 0)
    {
        textures.push_back(std::exchange(postPicture->textureId, 0));
    }
    if (gif)
    {
        for (auto&& img : gif->images)
        {
            if (img->img && img->img->textureId > 0)
            {
                textures.push_back(std::exchange(img->img->textureId, 0));
            }
        }
    }
    if (currentPost && currentPost->isGallery && !currentPost->gallery.empty())
    {
        for (auto&& img : gallery.images)
        {
            if (img->textureId > 0)
            {
                textures.push_back(std::exchange(img->textureId, 0));
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

    if (!errorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s",
                           errorMessage.c_str());
        return;
    }

    ResizableGLImage* display_image = postPicture.get();

    if (gif && !gif->images.empty())
    {
        if (!gif->paused && gif->images[gif->currentImage]->displayed)
        {
            auto lastDisplay = gif->images[gif->currentImage]->lastDisplay;
            auto diffSinceLastDisplay =
                std::chrono::steady_clock::now() - lastDisplay;
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                diffSinceLastDisplay);
            const auto delay = std::chrono::duration<int, std::milli>(
                gif->images[gif->currentImage]->delay);
            if (ms > delay)
            {
                gif->currentImage++;
                if (gif->currentImage >= (int)gif->images.size())
                {
                    gif->currentImage = 0;
                    for (auto&& img : gif->images)
                    {
                        img->displayed = false;
                        img->lastDisplay =
                            std::chrono::steady_clock::time_point::min();
                    }
                }
                gif->images[gif->currentImage]->lastDisplay =
                    std::chrono::steady_clock::now();
                gif->images[gif->currentImage]->displayed = true;
            }
        }
        else
        {
            gif->images[gif->currentImage]->displayed = true;
            gif->images[gif->currentImage]->lastDisplay =
                std::chrono::steady_clock::now();
        }
        display_image = gif->images[gif->currentImage]->img.get();
        display_image->resizedWidth = gif->width;
        display_image->resizedHeight = gif->height;
    }

    if (currentPost && currentPost->isGallery && !gallery.images.empty())
    {
        if (gallery.currentImage < 0 ||
            gallery.currentImage >= (int)gallery.images.size())
        {
            gallery.currentImage = 0;
        }
        display_image = gallery.images[gallery.currentImage].get();
    }

    if (display_image)
    {
        if (currentPost && currentPost->isGallery && !gallery.images.empty())
        {
            showGalleryControls(display_image->resizedWidth);
        }

        ImGuiResizableGLImage(display_image, windowHeight * 0.5f);

        if (gif && !display_image->isResized)
        {
            for (auto&& img : gif->images)
            {
                img->img->resizedWidth = display_image->resizedWidth;
                img->img->resizedHeight = display_image->resizedHeight;
                img->img->pictureRatio = display_image->pictureRatio;
                img->img->isResized = true;
            }
        }
        if (gif)
        {
            gif->width = display_image->resizedWidth;
            gif->height = display_image->resizedHeight;
        }

        if (mediaState.duration > 0.0f && mpvRenderContext)
        {
            showMediaControls(display_image->resizedWidth);
        }
    }

    if (liveThreadViewer)
    {
        liveThreadViewer->showLiveThread();
    }
    if (twitterRenderer)
    {
        twitterRenderer->Render();
    }
    if (markdown)
    {
        markdown->RenderMarkdown();
    }
    else if (!display_image && loadingPostContent)
    {
        ImGui::Spinner("###spinner_loading_data", 50.f, 1,
                       ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }
}
void PostContentViewer::showGalleryControls(int)
{
    if (ImGui::Button(galleryButtonPreviousText.c_str()))
    {
        gallery.currentImage--;
        if (gallery.currentImage < 0)
            gallery.currentImage = (int)gallery.images.size() - 1;
    }
    auto text =
        fmt::format("{}/{}", gallery.currentImage + 1, gallery.images.size());
    ImGui::SameLine();
    ImGui::Text("%s", text.c_str());
    ImGui::SameLine();
    if (ImGui::Button(galleryButtonNextText.c_str()))
    {
        gallery.currentImage++;
        if (gallery.currentImage >= (int)gallery.images.size())
            gallery.currentImage = 0;
    }
}
void PostContentViewer::showMediaControls(int width)
{
    auto progressHeight = ImGui::GetFontSize() * 0.5f;
    auto newTimeSeek = MediaWidget::mediaProgressBar(
        mediaState.timePosition, mediaState.duration, mediaState.cachedTime,
        ImVec2(width, progressHeight));
    if (newTimeSeek != 0)
    {
        auto seekStr = std::to_string(newTimeSeek);
        const char* cmd[] = { "seek", seekStr.c_str(), nullptr };
        mpv_command_async(mpv, 0, cmd);
    }
    /*if(ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(Utils::formatDuration(std::chrono::seconds((int)mediaState.timePosition)).c_str());
        ImGui::EndTooltip();
    }*/
    if (mediaState.finished)
    {
        if (ImGui::Button(mediaButtonRestartText.c_str()))
        {
            const char* cmd[] = { "playlist-play-index", "0", nullptr };
            mpv_command_async(mpv, 0, cmd);
        }
    }
    else
    {
        if (ImGui::Button(mediaButtonFastBackwardText.c_str()))
        {
            const char* cmd[] = { "seek", "-60", nullptr };
            mpv_command_async(mpv, 0, cmd);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek backwards 60 seconds");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::Button(mediaButtonBackwardText.c_str()))
        {
            const char* cmd[] = { "seek", "-10", nullptr };
            mpv_command_async(mpv, 0, cmd);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek backwards 10 seconds");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if (ImGui::Button(mediaState.paused ? mediaButtonPlayText.c_str()
                                            : mediaButtonPauseText.c_str()))
        {
            int shouldPause = !mediaState.paused;
            errorMessage.clear();
            mpv_set_property_async(mpv, 0, "pause", MPV_FORMAT_FLAG,
                                   &shouldPause);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Play/Pause media");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if (ImGui::Button(mediaButtonForwardText.c_str()))
        {
            const char* cmd[] = { "seek", "10", nullptr };
            mpv_command_async(mpv, 0, cmd);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Seek forward 10 seconds");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if (ImGui::Button(mediaButtonFastForwardText.c_str()))
        {
            const char* cmd[] = { "seek", "60", nullptr };
            mpv_command_async(mpv, 0, cmd);
        }
        if (ImGui::IsItemHovered())
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
    if (ImGui::SliderInt(mediaSliderVolumeText.c_str(), &volume, 0, 100,
                         std::to_string(volume).c_str()))
    {
        mediaState.mediaAudioVolume = volume;
        double vold = volume;
        mpv_set_property_async(mpv, 0, "volume", MPV_FORMAT_DOUBLE, &vold);
    }
    ImGui::SameLine();
    ImGui::Text(
        "%s/%s",
        Utils::formatDuration(std::chrono::seconds((int)mediaState.timePosition))
            .c_str(),
        durationText.c_str());
    /*ImGui::SameLine();
    if(ImGui::Checkbox(loopCheckboxText.c_str(),&mediaLoop))
    {
        const char *cmd[] = {"loop-file",mediaLoop ? "inf" : "no", nullptr};
        mpv_command_async(mpv,0,cmd);
    }*/
}
