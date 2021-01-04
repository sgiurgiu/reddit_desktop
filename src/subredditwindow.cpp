#include "subredditwindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "fonts/IconsFontAwesome4.h"
#include "utils.h"
#include "spinner/spinner.h"
#include "database.h"
#include <boost/asio/post.hpp>
#include "utils.h"

namespace
{
constexpr auto NEWTEXT_POST_POPUP_TITLE = "New Text Post";
constexpr auto NEWLINK_POST_POPUP_TITLE = "New Link";
}
SubredditWindow::SubredditWindow(int id, const std::string& subreddit,
                                 const access_token& token,
                                 RedditClientProducer* client,
                                 const boost::asio::any_io_executor& executor):
    id(id),subreddit(subreddit),token(token),client(client),
    uiExecutor(executor)
{
    shouldBlurPictures= Database::getInstance()->getBlurNSFWPictures();
}
SubredditWindow::~SubredditWindow()
{
    clearExistingPostsData();
}
void SubredditWindow::clearExistingPostsData()
{
    std::vector<GLuint> textures;
    std::vector<GLuint> fbos;
    textures.reserve(posts.size()*3);
    fbos.reserve(posts.size());
    for(auto&& p : posts)
    {
        if(p.thumbnailPicture && p.thumbnailPicture->textureId > 0)
        {
            textures.push_back(p.thumbnailPicture->textureId);
            p.thumbnailPicture->textureId = 0;
        }
        if(p.blurredThumbnailPicture && p.blurredThumbnailPicture->textureId > 0)
        {
            textures.push_back(p.blurredThumbnailPicture->textureId);
            p.blurredThumbnailPicture->textureId = 0;
        }
        if(p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia();
            auto postContentViewerTextures = p.postContentViewer->getAndResetTextures();
            std::move(postContentViewerTextures.begin(),postContentViewerTextures.end(),std::back_inserter(textures));
            auto fbo = p.postContentViewer->getAndResetMediaFBO();
            if(fbo > 0)
            {
                fbos.push_back(fbo);
            }
        }
    }
    glDeleteTextures(textures.size(),textures.data());
    glDeleteFramebuffers(fbos.size(), fbos.data());
}
void SubredditWindow::loadSubreddit()
{
    target = subreddit;
    if(subreddit.empty())
    {
        windowName = fmt::format("Front Page##{}",id);
        target = "/";
    }
    else
    {
        windowName = fmt::format("{}##{}",subreddit,id);
        if(!target.starts_with("r/") && !target.starts_with("/r/") &&
            (!target.starts_with("/user/") && target.find("/m/") == target.npos))
        {
            target = "/r/" + target;
        }
    }

    if(!target.starts_with("/")) target = "/" + target;
    loadSubredditListings(target,token);
}
void SubredditWindow::loadSubredditListings(const std::string& target,const access_token& token)
{
    auto connection = client->makeListingClientConnection();
    connection->connectionCompleteHandler([self=shared_from_this()](const boost::system::error_code& ec,
                                const client_response<listing>& response)
    {
       if(ec)
       {
           boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,self,ec.message()));
       }
       else if(response.status >= 400)
       {
           boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,self,std::move(response.body)));
       }
       else
       {
           boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::loadListingsFromConnection,
                                                        self,std::move(response.data)));
       }
    });

    connection->list(target,token);
}

void SubredditWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = std::move(errorMessage);
}
void SubredditWindow::loadListingsFromConnection(const listing& listingResponse)
{
    posts_list tmpPosts;

    for(auto& child:listingResponse.json["data"]["children"])
    {
        auto kind = child["kind"].get<std::string>();
        if(kind == "t3")
        {
            tmpPosts.emplace_back(std::make_shared<post>(child["data"]));
        }
    }    
    setListings(std::move(tmpPosts),
                std::move(listingResponse.json["data"]["before"]),
                std::move(listingResponse.json["data"]["after"]));
}
void SubredditWindow::setListings(posts_list receivedPosts,nlohmann::json beforeJson,nlohmann::json afterJson)
{
    before.reset();
    after.reset();
    if(!beforeJson.is_null() && beforeJson.is_string())
    {
        before = beforeJson.get<std::string>();
    }
    if(!afterJson.is_null() && afterJson.is_string())
    {
        after = afterJson.get<std::string>();
    }
    listingErrorMessage.clear();    
    clearExistingPostsData();
    posts = std::move(receivedPosts);
    scrollToTop = true;
    for(auto&& p : posts)
    {
        if(subredditName.empty())
        {
            subredditName = p.post->subredditName;
        }

        if(!p.post->thumbnail.empty() && p.post->thumbnail != "self" &&
                p.post->thumbnail != "default")
        {
            auto resourceConnection = client->makeResourceClientConnection();
            resourceConnection->connectionCompleteHandler(
                        [post=&p,self=shared_from_this()](const boost::system::error_code&,
                             const resource_response& response)
            {
                if(response.status == 200)
                {
                    int width, height, channels;
                    auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                    boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setPostThumbnail,self,post,data,width,height,channels));
                }
            });
            resourceConnection->getResource(p.post->thumbnail);
        }
    }
}
void SubredditWindow::setPostThumbnail(PostDisplay* p,unsigned char* data, int width, int height, int channels)
{
    ((void)channels);
    auto image = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    p->thumbnailPicture = std::move(image);
    if(p->post->over18 && shouldBlurPictures)
    {
        auto blurredImage = Utils::loadBlurredImage(data,width,height,STBI_rgb_alpha);
        p->blurredThumbnailPicture = std::move(blurredImage);
    }
    stbi_image_free(data);    
}
void SubredditWindow::showWindow(int appFrameWidth,int appFrameHeight)
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
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)) && ImGui::IsWindowFocused())
    {
        loadSubredditListings(target,token);
    }


    if(scrollToTop)
    {
        ImGui::SetScrollHereY(0.0f);
        scrollToTop = false;
    }

    //showWindowMenu();

    //ImGuiStyle& style = ImGui::GetStyle();
   // auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x + style.ItemInnerSpacing.x;
    //auto availableWidth = ImGui::GetContentRegionAvailWidth();

    if(!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",listingErrorMessage.c_str());
    }

    if(ImGui::Button("Submit New Post"))
    {
        newTextPostDialog = true;
    }
    ImGui::SameLine();
    if(ImGui::Button("Submit New Link"))
    {
        newLinkPostDialog = true;
    }

    if(maxScoreWidth == 0.f)
    {
        maxScoreWidth = ImGui::CalcTextSize("99.9k").x  - (ImGui::GetStyle().FramePadding.x*2.f);
    }
    if(upvotesButtonsIdent == 0.f)
    {
        auto upvotesButtonsWidth = ImGui::CalcTextSize(reinterpret_cast<const char*>(ICON_FA_ARROW_UP)).x  + (ImGui::GetStyle().FramePadding.x);
        upvotesButtonsIdent = (maxScoreWidth - upvotesButtonsWidth) / 2.f - (ImGui::GetStyle().FramePadding.x);
    }

    for(auto&& p : posts)
    {                
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.0f,1.0f,1.0f,0.0f));
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();
        ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_UP));
        auto scoreIdent = (maxScoreWidth - ImGui::CalcTextSize(p.post->humanScore.c_str()).x)/2.f;
        ImGui::Dummy(ImVec2(scoreIdent,0.f));ImGui::SameLine();
        ImGui::Text("%s",p.post->humanScore.c_str());
        if(ImGui::GetItemRectSize().x > maxScoreWidth) maxScoreWidth = ImGui::GetItemRectSize().x;
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();
        ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN));        
        ImGui::PopStyleColor(1);
        ImGui::EndGroup();

        auto height = ImGui::GetCursorPosY();
        ImGui::SameLine();

        if(p.post->over18 && shouldBlurPictures && p.blurredThumbnailPicture &&
                !p.shouldShowUnblurredImage)
        {
            ImGui::Image((void*)(intptr_t)p.blurredThumbnailPicture->textureId,
                         ImVec2(p.blurredThumbnailPicture->width,p.blurredThumbnailPicture->height));
            height = std::max(height,ImGui::GetCursorPosY());
            auto rectMin = ImGui::GetItemRectMin();
            auto rectMax = ImGui::GetItemRectMax();
            if(ImGui::IsMouseHoveringRect(rectMin,rectMax) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                p.shouldShowUnblurredImage = true;
            }
            ImGui::SameLine();
        }
        else if(p.thumbnailPicture)
        {
            ImGui::Image((void*)(intptr_t)p.thumbnailPicture->textureId,
                         ImVec2(p.thumbnailPicture->width,p.thumbnailPicture->height));
            height = std::max(height,ImGui::GetCursorPosY());
            auto rectMin = ImGui::GetItemRectMin();
            auto rectMax = ImGui::GetItemRectMax();
            if(ImGui::IsMouseHoveringRect(rectMin,rectMax) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                    p.blurredThumbnailPicture)
            {
                p.shouldShowUnblurredImage = false;
            }
            ImGui::SameLine();
        }

        ImGui::BeginGroup();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
        ImGui::Text("%s",p.post->subreddit.c_str());
        ImGui::PopFont();
        ImGui::SameLine();        
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("Posted by %s %s",p.post->author.c_str(),p.post->humanReadableTimeDifference.c_str());
        if(p.post->over18)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.f,0.f,0.f,1.f),"%s",reinterpret_cast<const char*>(ICON_FA_BOMB));
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Not Safe For Work");
                ImGui::EndTooltip();
            }
        }
        ImGui::PopFont();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium_Big)]);
        if(!p.post->title.empty())
        {
            ImGui::TextWrapped("%s",p.post->title.c_str());
        }
        else
        {
            ImGui::TextWrapped("<No Title>");
        }
        ImGui::PopFont();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("(%s)",p.post->domain.c_str());
        ImGui::PopFont();

        auto normalPositionY = ImGui::GetCursorPosY();
        auto desiredPositionY = height - ImGui::GetFrameHeightWithSpacing();
        if(normalPositionY < desiredPositionY) ImGui::SetCursorPosY(desiredPositionY);
        if(ImGui::Button(fmt::format("{}##showContent{}",
                                     reinterpret_cast<const char*>(
                                         p.showingContent ? ICON_FA_MINUS_SQUARE_O:ICON_FA_PLUS_SQUARE_O),
                                     p.post->id).c_str()))
        {
            p.showingContent = !p.showingContent;
            if(!p.showingContent && p.postContentViewer)
            {
                p.postContentViewer->stopPlayingMedia();
            }
        }
        ImGui::SameLine();
        if(ImGui::Button(p.post->commentsText.c_str()))
        {
            commentsSignal(p.post->id,p.post->title);
        }
        if(!p.post->url.empty())
        {
            ImGui::SameLine();

            if(ImGui::Button(fmt::format("{}##openLink{}",
                                         reinterpret_cast<const char*>(ICON_FA_EXTERNAL_LINK_SQUARE " Open"),
                                         p.post->id).c_str()))
            {
                Utils::openInBrowser(p.post->url);
            }
        }
        if(p.showingContent)
        {
            if(!p.postContentViewer)
            {
                p.postContentViewer = std::make_shared<PostContentViewer>(client,uiExecutor);
            }
            if(!p.postContentViewer->isCurrentPostSet())
            {
                p.postContentViewer->loadContent(p.post);
            }
            p.postContentViewer->showPostContent();
        }        

        ImGui::EndGroup();
        ImGui::Separator();
    }   

    if(!posts.empty())
    {
        bool previousEnabled = before.has_value();
        bool nextEnabled = after.has_value();
        if(!previousEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT " Previous")) && previousEnabled)
        {
            loadSubredditListings(target+"?before="+before.value()+"&count="+std::to_string(currentCount),token);
            currentCount-=posts.size();
            before.reset();
        }
        if(!previousEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
        auto width = ImGui::GetWindowContentRegionMax().x;
        auto previousSize = ImGui::GetItemRectSize();
        ImGui::SameLine(width - previousSize.x);
        if(!nextEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if(ImGui::Button(reinterpret_cast<const char*>("Next " ICON_FA_ARROW_RIGHT),previousSize) && nextEnabled)
        {
            currentCount+=posts.size();
            loadSubredditListings(target+"?after="+after.value()+"&count="+std::to_string(currentCount),token);
            after.reset();
        }
        if(!nextEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
    }

    showNewTextPostDialog();
    showNewLinkPostDialog();

    ImGui::End();
}
void SubredditWindow::showNewLinkPostDialog()
{
    auto windowWidth = ImGui::GetWindowWidth();
    if(newLinkPostDialog)
    {
        ImGui::OpenPopup(NEWLINK_POST_POPUP_TITLE);
        newLinkPostDialog = false;
        showingLinkPostDialog = true;
    }
    if (showingLinkPostDialog && ImGui::BeginPopupModal(NEWLINK_POST_POPUP_TITLE,nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("*Title:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()
                && !ImGui::IsMouseClicked(0))
        {
           ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SetNextItemWidth(windowWidth * 0.5f);
        ImGui::InputText("##newLinkPostTitle",newTextPostTitle,sizeof(newTextPostTitle));
        ImGui::Text("*Link:");
        ImGui::SetNextItemWidth(windowWidth * 0.5f);
        ImGui::InputText("##newLinkPostText",newLinkPost,sizeof(newLinkPost));

        bool okDisabled = std::string_view(newTextPostTitle).empty() ||
                std::string_view(newLinkPost).empty();
        if (okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            auto p = std::make_shared<post>();
            p->url = newLinkPost;
            p->title = newTextPostTitle;
            p->subreddit = subredditName;
            p->postHint="link";
            submitNewPost(p);
            showingLinkPostDialog = false;
            newTextPostTitle[0] = 0;
            newLinkPost[0] = 0;
            ImGui::CloseCurrentPopup();
        }
        if (okDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            showingLinkPostDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SubredditWindow::showNewTextPostDialog()
{
    auto windowWidth = ImGui::GetWindowWidth();
    if(newTextPostDialog)
    {
        ImGui::OpenPopup(NEWTEXT_POST_POPUP_TITLE);
        newTextPostDialog = false;
        showingTextPostDialog = true;
    }
    if (showingTextPostDialog && ImGui::BeginPopupModal(NEWTEXT_POST_POPUP_TITLE,nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("*Title:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()
                && !ImGui::IsMouseClicked(0))
        {
           ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SetNextItemWidth(windowWidth * 0.75f);
        ImGui::InputText("##newTextPostTitle",newTextPostTitle,sizeof(newTextPostTitle));
        ImGui::Text("Text:");
        ImGui::SetNextItemWidth(windowWidth * 0.75f);
        ImGui::InputTextMultiline("##newTextPostText",newTextPostContent,sizeof(newTextPostContent));

        bool okDisabled = std::string_view(newTextPostTitle).empty();
        if (okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            auto p = std::make_shared<post>();
            p->selfText = newTextPostContent;
            p->title = newTextPostTitle;
            p->subreddit = subredditName;
            p->postHint="self";
            submitNewPost(p);
            showingTextPostDialog = false;
            newTextPostTitle[0] = 0;
            newTextPostContent[0] = 0;
            ImGui::CloseCurrentPopup();
        }
        if (okDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            showingTextPostDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SubredditWindow::showWindowMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Statistics","Ctrl+S",false))
            {

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Refresh","F5",false))
            {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void SubredditWindow::showCommentsListener(const typename CommentsSignal::slot_type& slot)
{
    commentsSignal.connect(slot);
}
void SubredditWindow::setFocused()
{
    willBeFocused = true;
}
void SubredditWindow::submitNewPost(const post_ptr& p)
{
    auto createPostConnection = client->makeCreatePostClientConnection();
    createPostConnection->connectionCompleteHandler([self=shared_from_this()](const boost::system::error_code& ec,
                                const client_response<post_ptr>& response)
    {
       if(ec)
       {
           boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,self,ec.message()));
       }
       else
       {
           if(response.status >= 400)
           {
               boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,self,response.body));
           }
           else
           {
               if(!response.data)
               {
                    boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setErrorMessage,self,"Cannot create post"));
               }
               else
               {
                   self->commentsSignal(response.data->id,response.data->title);
               }
           }
       }
    });
    createPostConnection->createPost(p,true,token);
}
