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
    //std::vector<GLuint> fbos;
    textures.reserve(posts.size()*3);
    //fbos.reserve(posts.size());
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
            //auto postContentViewerTextures = p.postContentViewer->getAndResetTextures();
            //std::move(postContentViewerTextures.begin(),postContentViewerTextures.end(),std::back_inserter(textures));
            /*auto fbo = p.postContentViewer->getAndResetMediaFBO();
            if(fbo > 0)
            {
                fbos.push_back(fbo);
            }*/
        }
    }
    //glDeleteFramebuffers(fbos.size(), fbos.data());
    glDeleteTextures(textures.size(),textures.data());
}
void SubredditWindow::loadSubreddit()
{
    target = subreddit;
    if(subreddit.empty())
    {
        title = "Front Page";
        target = "/";
    }
    else
    {
        title = subreddit;
        if(!target.starts_with("r/") && !target.starts_with("/r/") &&
            (!target.starts_with("/user/") && target.find("/m/") == target.npos))
        {
            target = "/r/" + target;
        }
    }
    windowName = fmt::format("{}##{}",title,id);

    if(!target.starts_with("/")) target = "/" + target;
    loadSubredditListings(target,token);
}
void SubredditWindow::loadSubredditListings(const std::string& target,const access_token& token)
{

    if(!listingConnection)
    {
        listingConnection = client->makeListingClientConnection();
        listingConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                    client_response<listing> response)
        {
            auto self = weak.lock();
            if(!self) return;
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
    }

    if(!resourceConnection)
    {
        resourceConnection = client->makeResourceClientConnection();
        resourceConnection->connectionCompleteHandler(
                    [weak=weak_from_this()](const boost::system::error_code& ec,
                         resource_response response)
        {
            auto self = weak.lock();
            if(!self) return;
            PostDisplay* post = static_cast<PostDisplay*>(response.userData);
            if(!post) return;
            if(ec)
            {
                auto message = "Cannot load thumbnail:" + ec.message();
                boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setPostErrorMessage,self,post,std::move(message)));
            }
            else if(response.status >= 400)
            {
                auto message = "Cannot load thumbnail:" + response.body;
                boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setPostErrorMessage,self,post,std::move(message)));
            }
            else if(response.status == 200)
            {
                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),response.data.size(),&width,&height,&channels);
                boost::asio::post(self->uiExecutor,std::bind(&SubredditWindow::setPostThumbnail,self,post,data,width,height,channels));
            }
        });
    }
    if (!voteConnection)
    {
        voteConnection = client->makeRedditVoteClientConnection();
        voteConnection->connectionCompleteHandler(
            [weak = weak_from_this()](const boost::system::error_code& ec,
                client_response<std::string> response)
        {
            auto self = weak.lock();
            if (!self) return;
            std::pair<post_ptr,Voted>* p = static_cast<std::pair<post_ptr, Voted>*>(response.userData);
            if (!p) return;

            if (ec)
            {
                boost::asio::post(self->uiExecutor, std::bind(&SubredditWindow::setErrorMessage, self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor, std::bind(&SubredditWindow::setErrorMessage, self, std::move(response.body)));
            }
            else
            {
                boost::asio::post(self->uiExecutor, std::bind(&SubredditWindow::updatePostVote, self, p->first.get(), p->second));
            }
            delete p;
        });
    }

    listingConnection->list(target,token);
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
    for(auto& p : posts)
    {
        if(subredditName.empty())
        {
            subredditName = p.post->subredditName;
        }
        p.upvoteButtonText = fmt::format("{}##_up{}",reinterpret_cast<const char*>(ICON_FA_ARROW_UP),p.post->name);
        p.downvoteButtonText = fmt::format("{}##_down{}",reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),p.post->name);
        p.openLinkButtonText = fmt::format("{}##openLink{}",
                                           reinterpret_cast<const char*>(ICON_FA_EXTERNAL_LINK_SQUARE " Open Link"),
                                           p.post->name);
        p.votesChildText = fmt::format("##{}_votes_child",p.post->name).c_str();
        p.updateShowContentText();
        if(!p.post->thumbnail.empty())
        {
            p.thumbnailPicture = Utils::GetRedditThumbnail(p.post->thumbnail);
            if(!p.thumbnailPicture)
            {
                resourceConnection->getResource(p.post->thumbnail,&p);
            }
        }
    }
}
void SubredditWindow::setPostErrorMessage(PostDisplay* post,std::string msg)
{
    post->errorMessageText = std::move(msg);
}
void SubredditWindow::PostDisplay::updateShowContentText()
{
    showContentButtonText = fmt::format("{}##showContent{}",
                                          reinterpret_cast<const char*>(
                                              showingContent ? ICON_FA_MINUS_SQUARE_O:ICON_FA_PLUS_SQUARE_O),
                                          post->name);
}
void SubredditWindow::setPostThumbnail(PostDisplay* p,unsigned char* data, int width, int height, int channels)
{
    ((void)channels);
    p->thumbnailPicture = Utils::loadImage(data,width,height,STBI_rgb_alpha);
    if(p->post->over18 && shouldBlurPictures)
    {
        p->blurredThumbnailPicture = Utils::loadBlurredImage(data,width,height,STBI_rgb_alpha);
    }
    stbi_image_free(data);    
}
void SubredditWindow::votePost(post_ptr p,Voted voted)
{
    listingErrorMessage.clear();
    std::pair<post_ptr, Voted>* pair = new std::pair<post_ptr, Voted>(p,voted);
    voteConnection->vote(p->name,token,voted,pair);
}
void SubredditWindow::updatePostVote(post* p,Voted voted)
{
    auto oldVoted = p->voted;
    p->voted = voted;
    p->humanScore = Utils::CalculateScore(p->score,oldVoted,voted);
}
void SubredditWindow::pauseAllPosts()
{
    for(auto&& p : posts)
    {
        if(p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(true);
        }
    }
}
void SubredditWindow::showWindow(int appFrameWidth,int appFrameHeight)
{

    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);

    if(!ImGui::Begin(windowName.c_str(),&windowOpen,ImGuiWindowFlags_None))
    {
        pauseAllPosts();
        ImGui::End();
        return;
    }
    if(!windowOpen)
    {
        pauseAllPosts();
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
        pauseAllPosts();
        windowOpen = false;
    }
    if(windowPositionAndSizeSet)
    {
        windowPositionAndSizeSet = false;
        ImGui::SetWindowPos(windowPos);
        ImGui::SetWindowSize(windowSize);
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
    {
        bool refreshDisabled = posts.empty();
        if(refreshDisabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x-(ImGui::GetStyle().FramePadding.x)-ImGui::GetFontSize());
        if(ImGui::Button(reinterpret_cast<const char*>(ICON_FA_REFRESH)))
        {
            listingErrorMessage.clear();
            clearExistingPostsData();
            posts.clear();
            loadSubredditListings(target,token);
        }
        if(refreshDisabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
    }

    if(maxScoreWidth == 0.f)
    {
        maxScoreWidth = ImGui::CalcTextSize("XXXXXX").x;
    }
    if(upvotesButtonsIdent == 0.f)
    {
        std::string btnText = posts.empty() ? reinterpret_cast<const char*>(ICON_FA_ARROW_UP) : posts.front().upvoteButtonText;
        auto upvotesButtonsWidth = ImGui::CalcTextSize(btnText.c_str()).x  + (ImGui::GetStyle().FramePadding.x);
        upvotesButtonsIdent = (maxScoreWidth - upvotesButtonsWidth) / 2.f - (ImGui::GetStyle().FramePadding.x);
    }

    for(auto& p : posts)
    {                
        if(!p.errorMessageText.empty())
        {
            ImGui::TextColored(ImVec4(1.0f,0.0f,0.0f,1.0f), "%s",p.errorMessageText.c_str());
        }

        ImGui::BeginChild(p.votesChildText.c_str(),
                          ImVec2(maxScoreWidth,ImGui::GetFontSize()*4+ImGui::GetStyle().FramePadding.x*2));
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.0f,1.0f,1.0f,0.0f));
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();

        if(p.post->voted == Voted::UpVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetUpVoteColor());
        }

        if(ImGui::Button(p.upvoteButtonText.c_str()))
        {
            votePost(p.post,p.post->voted == Voted::UpVoted ? Voted::NotVoted : Voted::UpVoted);
        }

        if(p.post->voted == Voted::UpVoted)
        {
            ImGui::PopStyleColor(1);
        }

        auto scoreIdent = (maxScoreWidth - ImGui::CalcTextSize(p.post->humanScore.c_str()).x)/2.f;
        //ImGui::Dummy(ImVec2(scoreIdent,0.f));
        ImGui::Dummy(ImVec2(0,0));
        ImGui::SameLine(scoreIdent);
        ImGui::Text("%s",p.post->humanScore.c_str());
        if(ImGui::GetItemRectSize().x > maxScoreWidth) maxScoreWidth = ImGui::GetItemRectSize().x;
        ImGui::Dummy(ImVec2(upvotesButtonsIdent/2.f,0.f));ImGui::SameLine();

        if(p.post->voted == Voted::DownVoted)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,Utils::GetDownVoteColor());
        }

        if(ImGui::Button(p.downvoteButtonText.c_str()))
        {
            votePost(p.post,p.post->voted == Voted::DownVoted ? Voted::NotVoted : Voted::DownVoted);
        }

        if(p.post->voted == Voted::DownVoted)
        {
            ImGui::PopStyleColor(1);
        }

        ImGui::PopStyleColor(1);
        ImGui::EndChild();

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
            ImGui::TextColored(ImVec4(1.f,0.f,0.f,1.f),"NSFW");
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

        if(ImGui::Button(p.showContentButtonText.c_str()))
        {
            p.showingContent = !p.showingContent;
            p.updateShowContentText();
            if(!p.showingContent && p.postContentViewer)
            {
                p.postContentViewer->stopPlayingMedia(true);
            }
            else if (p.showingContent && p.postContentViewer)
            {
                p.postContentViewer->stopPlayingMedia(false);
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

            if(ImGui::Button(p.openLinkButtonText.c_str()))
            {
                Utils::openInBrowser(p.post->url);
            }
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(p.post->url.c_str());
                ImGui::EndTooltip();
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
        if( (ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT " Previous")) ||
             (ImGui::IsMouseClicked(3) && ImGui::IsWindowFocused())) && previousEnabled)
        {
            auto oldPostsSize = posts.size();
            clearExistingPostsData();
            posts.clear();
            loadSubredditListings(target+"?before="+before.value()+"&count="+std::to_string(currentCount),token);
            currentCount-=oldPostsSize;
            before.reset();
            after.reset();
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
        if((ImGui::Button(reinterpret_cast<const char*>("Next " ICON_FA_ARROW_RIGHT),previousSize) ||
           (ImGui::IsMouseClicked(4) && ImGui::IsWindowFocused())) && nextEnabled)
        {
            currentCount+=posts.size();
            clearExistingPostsData();
            posts.clear();
            loadSubredditListings(target+"?after="+after.value()+"&count="+std::to_string(currentCount),token);
            after.reset();
            before.reset();
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
void SubredditWindow::setFocused()
{
    willBeFocused = true;
}
void SubredditWindow::submitNewPost(const post_ptr& p)
{
    auto createPostConnection = client->makeCreatePostClientConnection();
    createPostConnection->connectionCompleteHandler([weak=weak_from_this()](const boost::system::error_code& ec,
                                client_response<post_ptr> response)
    {
       auto self = weak.lock();
       if(!self) return;
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
