#ifndef POSTCONTENTVIEWER_H
#define POSTCONTENTVIEWER_H

#include <boost/asio/io_context.hpp>
#include "redditclient.h"
#include <memory>
#include "entities.h"
#include <SDL_video.h>
#include <atomic>
#include "resizableglimage.h"

struct mpv_handle;
struct mpv_render_context;

class PostContentViewer
{
public:
    PostContentViewer(RedditClient* client,
                      const boost::asio::io_context::executor_type& uiExecutor,
                      post_ptr currentPost);
    ~PostContentViewer();
    void showPostContent();
private:
    void setPostImage(unsigned char* data, int width, int height, int channels);
    void setPostGif(unsigned char* data, int width, int height, int channels,
                    int count, int* delays);
    void setPostMediaFrame();
    void loadPostImage();
    void setupMediaContext(std::string file);
    void static mpvRenderUpdate(void* context);
    void static onMpvEvents(void* context);
    void handleMpvEvents();
    void mpvDoublePropertyChanged(std::string name, double value);
    void mpvFlagPropertyChanged(std::string name, int value);
    void mpvInt64PropertyChanged(std::string name, int64_t value);
    void loadPostGalleryImages();
    void setPostGalleryImage(unsigned char* data, int width, int height, int channels, int index);
    void setErrorMessage(std::string errorMessage);
private:
    RedditClient* client;
    const boost::asio::io_context::executor_type& uiExecutor;
    post_ptr currentPost;
    RedditClient::MediaStreamingClientConnection mediaStreamingConnection;
    std::string errorMessage;
    SDL_DisplayMode displayMode;
    bool loadingPostContent = false;
    mpv_handle* mpv = nullptr;
    mpv_render_context *mpv_gl = nullptr;
    boost::asio::io_context mpvEventIOContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mpvEventIOContextWork;
    std::thread mvpEventThread;
    unsigned int mediaFramebufferObject = 0;
    struct MediaState {
        std::atomic_int mediaAudioVolume;
        std::atomic_bool paused;
        std::atomic<float> duration;
        std::atomic<float> timePosition;
        std::atomic<double> width;
        std::atomic<double> height;
    };
    MediaState mediaState;
    struct gif_image
    {
        gif_image(){}
        gif_image(ResizableGLImagePtr img, int delay):
            img(std::move(img)),delay(delay)
        {}
        ResizableGLImagePtr img;
        int delay;
        std::chrono::steady_clock::time_point lastDisplay;
        bool displayed = false;
    };

    struct post_gif
    {
        std::vector<std::unique_ptr<gif_image>> images;
        int currentImage = 0;
    };
    std::unique_ptr<post_gif> gif;
    ResizableGLImagePtr postPicture;
    struct post_gallery
    {
        std::vector<ResizableGLImagePtr> images;
        int currentImage = 0;
    };
    post_gallery gallery;

};

#endif // POSTCONTENTVIEWER_H
