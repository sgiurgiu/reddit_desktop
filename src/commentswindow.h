#ifndef COMMENTSWINDOW_H
#define COMMENTSWINDOW_H

#include <SDL_video.h>
#include <boost/asio/io_context.hpp>
#include <string>
#include <memory>
#include <boost/signals2.hpp>
#include "entities.h"
#include "redditclient.h"
#include "redditlistingconnection.h"
#include "utils.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

class CommentsWindow
{
public:
    CommentsWindow(const std::string& postId,
                   const std::string& title,
                   const access_token& token,
                   RedditClient* client,
                   const boost::asio::io_context::executor_type& executor);
    ~CommentsWindow();
    bool isWindowOpen() const {return windowOpen;}
    void showWindow(int appFrameWidth,int appFrameHeight);
    std::string getPostId() const
    {
        return postId;
    }
    void setFocused();
private:
    void loadListingsFromConnection(const listing& listingResponse);
    void setErrorMessage(std::string errorMessage);
    void loadListingChildren(const nlohmann::json& children);
    void setComments(comments_list receivedComments);
    void setParentPost(post_ptr receivedParentPost);
    void showComment(comment_ptr c);
    void setPostImage(unsigned char* data, int width, int height, int channels);
    void setPostGif(unsigned char* data, int width, int height, int channels,
                    int count, int* delays);
    void setPostMediaFrame();
    void loadPostImage();
    void setupMediaContext(std::string file);
    void static mpvRenderUpdate(void* context);
    void static onMpvEvents(void* context);
    void handleMpvEvents();
private:

    std::string postId;
    bool windowOpen = true;
    access_token token;
    RedditClient* client;
    RedditClient::RedditListingClientConnection connection;
    RedditClient::MediaStreamingClientConnection mediaStreamingConnection;
    std::string windowName;
    std::string listingErrorMessage;
    bool willBeFocused = false;
    comments_list comments;
    post_ptr parent_post;
    SDL_DisplayMode displayMode;
    float post_picture_width = 0.f;
    float post_picture_height = 0.f;
    float post_picture_ratio = 0.f;
    bool loadingPostData = false;
    const boost::asio::io_context::executor_type& uiExecutor;
    mpv_handle* mpv = nullptr;
    mpv_render_context *mpv_gl = nullptr;
    boost::asio::io_context mpvEventIOContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mpvEventIOContextWork;
    std::thread mvpEventThread;
    unsigned int mediaFramebufferObject = 0;

};

#endif // COMMENTSWINDOW_H
