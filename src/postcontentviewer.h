#ifndef POSTCONTENTVIEWER_H
#define POSTCONTENTVIEWER_H

#include <boost/asio/any_io_executor.hpp>
#include "redditclientproducer.h"
#include <memory>
#include "entities.h"
#include <atomic>
#include "resizableglimage.h"
#include "markdownrenderer.h"
#include "utils.h"
#include "livethreadviewer.h"
#include "twitterrenderer.h"

#include <mutex>

struct mpv_handle;
struct mpv_render_context;

class PostContentViewer : public std::enable_shared_from_this<PostContentViewer>
{
public:
    PostContentViewer(RedditClientProducer* client,
                      const access_token& token,
                      const boost::asio::any_io_executor& uiExecutor
                      );
    ~PostContentViewer();
    void showPostContent();
    void stopPlayingMedia(bool flag = true);    
    void loadContent(post_ptr currentPost);
    void loadCommentContent(const comment_media_metadata& commentMediaMetadata);
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
    void setupMediaContext(std::string file, bool useProvidedFile = false);
    void static mpvRenderUpdate(void* context);
    void static onMpvEvents(void* context);
    void handleMpvEvents();
    void mpvDoublePropertyChanged(std::string name, double value);
    void mpvFlagPropertyChanged(std::string name, int value);
    void mpvInt64PropertyChanged(std::string name, int64_t value);
    void loadPostEmbeddedGalleryImages();
    void loadPostGalleryImages(std::vector<std::string> urls);
    void setPostGalleryImage(Utils::STBImagePtr data, int width, int height, int channels, int index);
    void setErrorMessage(std::string errorMessage);

    void showMediaControls(int width);
    void showGalleryControls(int width);
    void loadPostFromListing(const listing& listingResponse);
private:
    RedditClientProducer* client;
    access_token token;
    const boost::asio::any_io_executor& uiExecutor;
    post_ptr currentPost;    
    std::string errorMessage;
    int windowHeight;
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
        float cachedTime = 0;
        float cachedDuration = 0;
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
    std::atomic_bool destroying = {false};
    bool useMediaHwAccel = true;
    bool currentPostSet = false;
    bool justPlayMedia = false;
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
    bool useYoutubeDlder = false;
    std::vector<std::string> mediaDomains;
    std::shared_ptr<LiveThreadViewer> liveThreadViewer;
    std::shared_ptr<TwitterRenderer> twitterRenderer;
    std::mutex mediaMutex;
};

#endif // POSTCONTENTVIEWER_H
