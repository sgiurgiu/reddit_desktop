#include "commentswindow.h"
#include <imgui.h>
#include <fmt/format.h>
#include "markdownrenderer.h"
#include "fonts/IconsFontAwesome4.h"
#include <SDL.h>
#include "spinner/spinner.h"
#include "macros.h"

CommentsWindow::CommentsWindow(const std::string& postId,
                               const std::string& title,
                               const access_token& token,
                               RedditClient* client,
                               const boost::asio::io_context::executor_type& executor):
    postId(postId),token(token),client(client),connection(client->makeListingClientConnection()),
    uiExecutor(executor)
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
}
CommentsWindow::~CommentsWindow()
{
    if(mediaStreamingConnection)
    {
        mediaStreamingConnection->clearAllSlots();
    }
    connection->clearAllSlots();
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
    if ((parent_post->postHint == "link" || parent_post->postHint == "self") &&
                 !parent_post->url.empty() &&
                 !parent_post->url.ends_with(".gif") &&
                 !parent_post->url.ends_with(".gifv") &&
                 !parent_post->url.ends_with(".mp4") &&
                 !parent_post->url.ends_with(".webv") &&
                 !parent_post->url.ends_with(".avi") &&
                 !parent_post->url.ends_with(".mkv"))
    {
        return;
    }
    if(parent_post->postHint == "image" &&
            !parent_post->url.empty() &&
            !parent_post->url.ends_with(".gifv"))
    {
        loadPostImage();
    }
    else if (!parent_post->postHint.empty() && parent_post->postHint != "self")
    {
        //if(parent_post->url.ends_with(".gifv"))
        {
            loadingPostData = true;
            //https://stackoverflow.com/questions/10715170/receiving-rtsp-stream-using-ffmpeg-library
            //https://stackoverflow.com/questions/6495523/ffmpeg-video-to-opengl-texture
            mediaStreamingConnection = client->makeMediaStreamingClientConnection();
            mediaStreamingConnection->framesAvailableHandler([this](uint8_t *data,int width, int height,int linesize) {
                if(mediaStreamingConnection)
                {
                    boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setPostMediaFrame,this,data,width,height,linesize));
                }
            });
            mediaStreamingConnection->errorHandler([this](int /*errorCode*/,const std::string& str){
                boost::asio::post(this->uiExecutor,std::bind(&CommentsWindow::setErrorMessage,this,str));
            });
            mediaStreamingConnection->streamMedia(parent_post->url);
        }
    }
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

void CommentsWindow::setPostMediaFrame(uint8_t *data,int width, int height,int linesize)
{
    UNUSED(linesize);
    loadingPostData = false;
    if(parent_post->post_picture)
    {
        glBindTexture(GL_TEXTURE_2D, parent_post->post_picture->textureId);
        glTexSubImage2D(
              GL_TEXTURE_2D,
              0,
              0,
              0,
              parent_post->post_picture->width,
              parent_post->post_picture->height,
              GL_RGB,
              GL_UNSIGNED_BYTE,
              data );
    }
    else
    {
        parent_post->post_picture = Utils::loadImage(data,width,height, 3);
    }
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
