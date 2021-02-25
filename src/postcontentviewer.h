#ifndef POSTCONTENTVIEWER_H
#define POSTCONTENTVIEWER_H

#include <boost/asio/any_io_executor.hpp>
#include "redditclientproducer.h"
#include <memory>
#include "entities.h"
#include <SDL_video.h>
#include <atomic>
#include "resizableglimage.h"
#include "markdownrenderer.h"
#include "utils.h"

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
    void setPostImage(Utils::STBImagePtr data, int width, int height, int channels);
    void setPostGif(Utils::STBImagePtr data, int width, int height, int channels,
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
    void setPostGalleryImage(Utils::STBImagePtr data, int width, int height, int channels, int index);
    void setErrorMessage(std::string errorMessage);

    void showMediaControls(int width);
    void showGalleryControls(int width);
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
        int mediaAudioVolume = 0;
        bool paused = false;
        float duration = 0.0f;
        float timePosition = 0.0f;
        double width = 0.0;
        double height = 0.0;
        bool finished = false;
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
    std::string mediaButtonRestartText;
    std::string mediaButtonFastBackwardText;
    std::string mediaButtonBackwardText;
    std::string mediaButtonPlayText;
    std::string mediaButtonPauseText;
    std::string mediaButtonForwardText;
    std::string mediaButtonFastForwardText;
    std::string galleryButtonPreviousText;
    std::string galleryButtonNextText;
    std::string mediaSliderVolumeText;
    std::string mediaProgressSliderText;
    std::string durationText;
    std::string loopCheckboxText;
    bool mediaLoop = false;
};

#endif // POSTCONTENTVIEWER_H
