#ifndef POSTCONTENTVIEWER_H
#define POSTCONTENTVIEWER_H

#include <boost/asio/io_context.hpp>
#include "redditclientproducer.h"
#include <memory>
#include "entities.h"
#include <SDL_video.h>
#include <atomic>
#include "resizableglimage.h"
#include "markdownrenderer.h"

struct mpv_handle;
struct mpv_render_context;

class PostContentViewer : public std::enable_shared_from_this<PostContentViewer>
{
public:
    PostContentViewer(RedditClientProducer* client,
                      const boost::asio::any_io_executor& uiExecutor
                      );
    ~PostContentViewer();
    void showPostContent();
    void stopPlayingMedia(bool flag = true);    
    void loadContent(post_ptr currentPost);
    bool isCurrentPostSet() const
    {
        return currentPostSet;
    }
    std::vector<GLuint> getAndResetTextures();
    //GLuint getAndResetMediaFBO();
private:
    void setPostImage(unsigned char* data, int width, int height, int channels);
    void setPostGif(unsigned char* data, int width, int height, int channels,
                    int count, int* delays);
    void setPostMediaFrame();
    void loadPostImage();
    void downloadPostImage(std::string url);
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
    void resetOpenGlState();
private:
    RedditClientProducer* client;
    const boost::asio::any_io_executor& uiExecutor;
    post_ptr currentPost;    
    std::string errorMessage;
    SDL_DisplayMode displayMode;
    bool loadingPostContent = false;
    mpv_handle* mpv = nullptr;
    mpv_render_context *mpvRenderContext = nullptr;
    GLuint mediaFramebufferObject = 0;
    struct MediaState {
        std::atomic_int mediaAudioVolume;
        std::atomic_bool paused;
        std::atomic<float> duration;
        std::atomic<float> timePosition;
        std::atomic<double> width;
        std::atomic<double> height;
        std::atomic_bool finished;
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
        int width = 0;
        int height = 0;
        bool paused = false;
    };
    std::unique_ptr<post_gif> gif;
    ResizableGLImagePtr postPicture;
    struct post_gallery
    {
        std::vector<ResizableGLImagePtr> images;
        int currentImage = 0;
    };
    post_gallery gallery;
    std::unique_ptr<MarkdownRenderer> markdown;
    bool currentPostSet = false;
    std::atomic_bool destroying = {false};
    bool useMediaHwAccel = true;
};

#endif // POSTCONTENTVIEWER_H
