#include "commentswindow.h"
#include <imgui.h>
#include <fmt/format.h>
#include "markdownrenderer.h"
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "spinner/spinner.h"
#include "macros.h"
#include <iostream>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include "database.h"

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
    //std::cout << "get_proc_address_mpv:"<<name<<std::endl;
    return SDL_GL_GetProcAddress(name);
}
}

CommentsWindow::CommentsWindow(const std::string& postId,
                               const std::string& title,
                               const access_token& token,
                               RedditClient* client,
                               const boost::asio::io_context::executor_type& executor):
    postId(postId),token(token),client(client),connection(client->makeListingClientConnection()),
    uiExecutor(executor),mpvEventIOContextWork(boost::asio::make_work_guard(mpvEventIOContext))
{
    SDL_GetDesktopDisplayMode(0, &displayMode);
    connection->connectionCompleteHandler([this](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
       if(ec)
       {
           boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,ec.message()));
       }
       else
       {
           if(response.status >= 400)
           {
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,response.body));
           }
           else
           {
                loadListingsFromConnection(response.data);
           }
       }
    });
    windowName = fmt::format("{}##{}",title,postId);
    connection->list("/comments/"+postId+"?depth=5&limit=50&threaded=true",token);
    using run_function = boost::asio::io_context::count_type(boost::asio::io_context::*)();
    auto bound_run_fuction = std::bind(static_cast<run_function>(&boost::asio::io_context::run),std::ref(mpvEventIOContext));
    mvpEventThread = std::thread(bound_run_fuction);
    mediaState.mediaAudioVolume = Database::getInstance()->getMediaAudioVolume();
}
CommentsWindow::~CommentsWindow()
{
    Database::getInstance()->setMediaAudioVolume(mediaState.mediaAudioVolume);
    if(mediaStreamingConnection)
    {
        mediaStreamingConnection->clearAllSlots();
    }
    connection->clearAllSlots();
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
void CommentsWindow::onMpvEvents(void* context)
{
    CommentsWindow* win=(CommentsWindow*)context;
    boost::asio::post(win->mpvEventIOContext.get_executor(),std::bind(&CommentsWindow::handleMpvEvents,win));
}
void CommentsWindow::handleMpvEvents()
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
           std::cout << "prop change:"<<prop->name<<std::endl;
           if(prop->format == MPV_FORMAT_DOUBLE)
           {
               double val = *(double*)prop->data;
               boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::mpvDoublePropertyChanged,this,
                                                            std::string(prop->name),val));
           }
           break;
       }
       case MPV_EVENT_VIDEO_RECONFIG:
       {
           int propValue;
           int width,height;
           mpv_get_property(mpv,"width",mpv_format::MPV_FORMAT_INT64,&propValue);
           width = propValue;
           mpv_get_property(mpv,"height",mpv_format::MPV_FORMAT_INT64,&propValue);
           height = propValue;
           std::cout << "width:"<<width<<",height:"<<height<<std::endl;
       }
           break;
       default:
           break;
       }
    }
}
void CommentsWindow::mpvDoublePropertyChanged(std::string name, double value)
{
    if(name == "time-pos")
    {
        mediaState.timePosition = value;
        std::cout << "prop change timePosition:"<<mediaState.timePosition<<std::endl;
    }
    else if (name == "duration")
    {
        mediaState.duration = value;
        std::cout << "prop change duration:"<<mediaState.duration<<std::endl;
    }
}
void CommentsWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = errorMessage;
    loadingPostData = false;
}
void CommentsWindow::loadListingsFromConnection(const listing& listingResponse)
{
    if(!listingResponse.json.is_array()) return;
    for(const auto& child:listingResponse.json)
    {
        if(!child.is_object())
        {
            continue;
        }
        if(!child.contains("kind") || child["kind"].get<std::string>() != "Listing")
        {
            continue;
        }
        if(!child.contains("data") || !child["data"].contains("children"))
        {
            continue;
        }
        loadListingChildren(child["data"]["children"]);
    }
}
void CommentsWindow::loadListingChildren(const nlohmann::json& children)
{
    if(!children.is_array()) return;
    comments_list comments;
    post_ptr pp;
    for(const auto& child: children)
    {
        if(!child.is_object())
        {
            continue;
        }
        if(!child.contains("kind") || !child.contains("data"))
        {
            continue;
        }
        auto kind = child["kind"].get<std::string>();
        if(kind == "t1")
        {
            //load comments
            comments.emplace_back(std::make_shared<comment>(child["data"]));
        }
        else if (kind == "t3")
        {
            //load post
            pp = std::make_shared<post>(child["data"]);
        }
    }
    if(!comments.empty())
    {
        boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setComments,this,std::move(comments)));
    }
    if(pp)
    {
        boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setParentPost,this,std::move(pp)));
    }
}
void CommentsWindow::setComments(comments_list receivedComments)
{
    listingErrorMessage.clear();
    comments.reserve(comments.size() + receivedComments.size());
    std::move(receivedComments.begin(), receivedComments.end(), std::back_inserter(comments));
}
void CommentsWindow::setParentPost(post_ptr receivedParentPost)
{
    listingErrorMessage.clear();
    parent_post = receivedParentPost;

    if(parent_post->isGallery && !parent_post->gallery.images.empty())
    {
        loadPostGalleryImages();
        return;
    }

    bool isMediaPost = std::find(mediaDomains.begin(),mediaDomains.end(),parent_post->domain) != mediaDomains.end();
    if(!isMediaPost)
    {
        auto idx = parent_post->url.find_last_of('.');
        if(idx != std::string::npos)
        {
            isMediaPost = std::find(mediaExtensions.begin(),mediaExtensions.end(),parent_post->url.substr(idx)) != mediaExtensions.end();
        }
    }

    if (!isMediaPost && parent_post->postHint != "image")
    {
        return;
    }
    if(parent_post->postHint == "image" &&
            !parent_post->url.empty() &&
            !parent_post->url.ends_with(".gifv"))
    {
        loadPostImage();
    }
    else if (parent_post->postHint != "self")
    {
        if(isMediaPost)
        {

            loadingPostData = true;
            //https://stackoverflow.com/questions/10715170/receiving-rtsp-stream-using-ffmpeg-library
            //https://stackoverflow.com/questions/6495523/ffmpeg-video-to-opengl-texture
            mediaStreamingConnection = client->makeMediaStreamingClientConnection();
            mediaStreamingConnection->streamAvailableHandler([this](std::string file) {
                if(mediaStreamingConnection)
                {
                    boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setupMediaContext,this,file));
                }
            });
            mediaStreamingConnection->errorHandler([this](int /*errorCode*/,const std::string& str){
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,str));
            });
            mediaStreamingConnection->streamMedia(parent_post.get());
        }
    }
}
void CommentsWindow::loadPostGalleryImages()
{
    loadingPostData = true;
    for(size_t i=0;i<parent_post->gallery.images.size();i++)
    {
        const auto& galImage = parent_post->gallery.images.at(i);
        if(galImage->url.empty()) continue;
        auto resourceConnection = client->makeResourceClientConnection();
        resourceConnection->connectionCompleteHandler(
                    [this,index=i](const boost::system::error_code& ec,
                         const resource_response& response)
        {
            if(ec)
            {
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,ec.message()));
                return;
            }
            else if(response.status == 200)
            {
                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setPostGalleryImage,this,
                                                             data,width,height,channels,(int)index));
            }
        });
        resourceConnection->getResource(galImage->url);
    }
}
void CommentsWindow::setPostGalleryImage(unsigned char* data, int width, int height, int channels, int index)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    parent_post->gallery.images[index]->img = std::move(image);
    loadingPostData = false;
}
void CommentsWindow::loadPostImage()
{
    loadingPostData = true;
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
                [this,url=parent_post->url](const boost::system::error_code& ec,
                     const resource_response& response)
    {
        if(ec)
        {
            boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,ec.message()));
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
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setPostGif,this,
                                                             data,width,height,channels,count,delays));
            }
            else
            {
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setPostImage,this,data,width,height,channels));
            }
        }
    });
    resourceConnection->getResource(parent_post->url);
}

void CommentsWindow::setupMediaContext(std::string file)
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

    mpv_set_wakeup_callback(mpv, &CommentsWindow::onMpvEvents, this);
    mpv_render_context_set_update_callback(mpv_gl, &CommentsWindow::mpvRenderUpdate, this);

    const char *cmd[] = {"loadfile", file.c_str(), NULL};
    mpv_command_async(mpv, 0, cmd);
}

void CommentsWindow::mpvRenderUpdate(void *context)
{
    CommentsWindow* win=(CommentsWindow*)context;
    boost::asio::post(win->uiExecutor,std::bind(&CommentsWindow::setPostMediaFrame,win));
}

void CommentsWindow::setPostMediaFrame()
{
    if(!parent_post) return;
    uint64_t flags = mpv_render_context_update(mpv_gl);
    if (!(flags & MPV_RENDER_UPDATE_FRAME))
    {
        return;
    }

    if(!parent_post->post_picture)
    {
        glGenFramebuffers(1, &mediaFramebufferObject);
        int propValue;
        int width,height;
        mpv_get_property(mpv,"width",mpv_format::MPV_FORMAT_INT64,&propValue);
        width = propValue;
        mpv_get_property(mpv,"height",mpv_format::MPV_FORMAT_INT64,&propValue);
        height = propValue;

        glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
        parent_post->post_picture = std::make_shared<gl_image>();
        glGenTextures(1, &parent_post->post_picture->textureId);
        glBindTexture(GL_TEXTURE_2D, parent_post->post_picture->textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, parent_post->post_picture->textureId, 0);
        parent_post->post_picture->width = width;
        parent_post->post_picture->height = height;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, mediaFramebufferObject);
    mpv_opengl_fbo mpfbo{(int)mediaFramebufferObject, parent_post->post_picture->width, parent_post->post_picture->height, 0};
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
    loadingPostData = false;    
}
void CommentsWindow::setPostGif(unsigned char* data, int width, int height, int channels,
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
    parent_post->gif = std::move(gif);
    loadingPostData = false;
}

void CommentsWindow::setPostImage(unsigned char* data, int width, int height, int channels)
{
    UNUSED(channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    stbi_image_free(data);
    parent_post->post_picture = std::move(image);
    loadingPostData = false;
}
void CommentsWindow::showComment(comment_ptr c)
{
    if(ImGui::TreeNodeEx(fmt::format("{} - {} points, {}",c->author,c->humanScore,c->humanReadableTimeDifference).c_str(),ImGuiTreeNodeFlags_DefaultOpen))
    {
        MarkdownRenderer::RenderMarkdown(c->body);
        ImGui::NewLine();
        ImGui::Button(fmt::format("{}##{}_up",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("{}##{}_down",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("save##{}_save",c->id).c_str());ImGui::SameLine();
        ImGui::Button(fmt::format("reply##{}_reply",c->id).c_str());
        if(c->hasMoreReplies)
        {
            ImGui::SameLine();
            ImGui::Button(fmt::format("more replies##{}_more_replies",c->id).c_str());
        }
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize()/2.f));
        for(const auto& reply : c->replies)
        {
            showComment(reply);
        }
        ImGui::TreePop();
    }
}
void CommentsWindow::showWindow(int appFrameWidth,int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!windowOpen) return;

    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }
    if(willBeFocused)
    {
        ImGui::SetWindowFocus();
        willBeFocused = false;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)) && ImGui::GetIO().KeyCtrl && ImGui::IsWindowFocused())
    {
        windowOpen = false;
    }
    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    if(parent_post)
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Bold)]);
        ImGui::TextWrapped("%s",parent_post->title.c_str());
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Roboto_Light)]);
        ImGui::Text("Posted by %s %s",parent_post->author.c_str(),parent_post->humanReadableTimeDifference.c_str());
        ImGui::PopFont();
        ImGui::NewLine();

        gl_image_ptr display_image = parent_post->post_picture;

        if(parent_post->gif)
        {
            if(parent_post->gif->images[parent_post->gif->currentImage]->displayed)
            {
                auto lastDisplay = parent_post->gif->images[parent_post->gif->currentImage]->lastDisplay;
                auto diffSinceLastDisplay = std::chrono::steady_clock::now() - lastDisplay;
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diffSinceLastDisplay);
                const auto delay = std::chrono::duration<int,std::milli>(parent_post->gif->images[parent_post->gif->currentImage]->delay);
                if(ms > delay)
                {
                    parent_post->gif->currentImage++;
                    if(parent_post->gif->currentImage >= (int)parent_post->gif->images.size())
                    {
                        parent_post->gif->currentImage = 0;
                        for(auto&& img:parent_post->gif->images)
                        {
                            img->displayed = false;
                            img->lastDisplay = std::chrono::steady_clock::time_point::min();
                        }
                    }
                    parent_post->gif->images[parent_post->gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
                    parent_post->gif->images[parent_post->gif->currentImage]->displayed = true;
                }
            }
            else
            {
                parent_post->gif->images[parent_post->gif->currentImage]->displayed = true;
                parent_post->gif->images[parent_post->gif->currentImage]->lastDisplay = std::chrono::steady_clock::now();
            }
            display_image = parent_post->gif->images[parent_post->gif->currentImage]->img;
        }

        if(parent_post->isGallery && !parent_post->gallery.images.empty())
        {
            if(parent_post->gallery.currentImage < 0 || parent_post->gallery.currentImage >= (int)parent_post->gallery.images.size())
            {
                parent_post->gallery.currentImage = 0;
            }
            display_image = parent_post->gallery.images[parent_post->gallery.currentImage]->img;
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
                ImGui::ProgressBar(progress, ImVec2(post_picture_width, 0.0f));
            }
            if(parent_post->isGallery && !parent_post->gallery.images.empty())
            {
                if(ImGui::Button(fmt::format(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT "##{}_previmage"),parent_post->id).c_str()))
                {
                    parent_post->gallery.currentImage--;
                    if(parent_post->gallery.currentImage < 0) parent_post->gallery.currentImage = (int)parent_post->gallery.images.size() - 1;
                }
                auto btnSize = ImGui::GetItemRectSize();
                auto text = fmt::format("{}/{}",parent_post->gallery.currentImage+1,parent_post->gallery.images.size());
                auto textSize = ImGui::CalcTextSize(text.c_str());
                auto space = (post_picture_width - btnSize.x * 2.f);
                ImGui::SameLine((space - textSize.x / 2.f)/2.f);
                ImGui::Text("%s",text.c_str());
                auto windowWidth = ImGui::GetWindowContentRegionMax().x;
                auto remainingWidth = windowWidth - post_picture_width;
                ImGui::SameLine(windowWidth-remainingWidth-btnSize.x*2.f/3.f);
                if(ImGui::Button(fmt::format(reinterpret_cast<const char*>(ICON_FA_ARROW_RIGHT "##{}_nextimage"),parent_post->id).c_str()))
                {
                    parent_post->gallery.currentImage++;
                    if(parent_post->gallery.currentImage >= (int)parent_post->gallery.images.size()) parent_post->gallery.currentImage = 0;
                }
            }
        }

        if(!parent_post->selfText.empty())
        {
            MarkdownRenderer::RenderMarkdown(parent_post->selfText);
        }
        else if(!display_image && loadingPostData)
        {
            constexpr std::string_view id = "###spinner_loading_data";
            ImGui::Spinner(id.data(),50.f,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
        }
        ImGui::Button(fmt::format("Comment##{}_comment",parent_post->id).c_str());

        ImGui::Separator();
    }
    for(const auto& c : comments)
    {
        showComment(c);
        ImGui::Separator();
    }

    ImGui::End();
}
void CommentsWindow::setFocused()
{
    willBeFocused = true;
}
