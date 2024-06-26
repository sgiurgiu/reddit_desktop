#include "subredditwindow.h"
#include "cssparser.h"
#include "database.h"
#include "fonts/IconsFontAwesome4.h"
#include "macros.h"
#include "spinner/spinner.h"
#include "uri.h"
#include "utils.h"
#include <boost/asio/post.hpp>
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

namespace
{
constexpr auto NEWTEXT_POST_POPUP_TITLE = "New Text Post";
constexpr auto NEWLINK_POST_POPUP_TITLE = "New Link";
} // namespace
SubredditWindow::SubredditWindow(int id,
                                 const std::string& subreddit,
                                 const access_token& token,
                                 RedditClientProducer* client,
                                 const boost::asio::any_io_executor& executor)
: id(id)
, subredditTarget(subreddit)
, token(token)
, client(client)
, uiExecutor(executor)
, postsContentDestroyerTimer(uiExecutor)
, refreshTimer(uiExecutor)
, subredditStylesheet(
      std::make_shared<SubredditStylesheet>(token, client, uiExecutor))
{
    shouldBlurPictures = Database::getInstance()->getBlurNSFWPictures();
}
SubredditWindow::~SubredditWindow()
{
    clearExistingPostsData();
}
void SubredditWindow::clearExistingPostsData()
{
    std::vector<GLuint> textures;
    // std::vector<GLuint> fbos;
    textures.reserve(posts.size() * 3);
    // fbos.reserve(posts.size());
    for (auto&& p : posts)
    {
        if (p.thumbnailPicture && p.thumbnailPicture->textureId > 0)
        {
            textures.push_back(p.thumbnailPicture->textureId);
            p.thumbnailPicture->textureId = 0;
        }
        if (p.blurredThumbnailPicture && p.blurredThumbnailPicture->textureId > 0)
        {
            textures.push_back(p.blurredThumbnailPicture->textureId);
            p.blurredThumbnailPicture->textureId = 0;
        }
        if (p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia();
            p.postContentViewer.reset();
            // auto postContentViewerTextures =
            // p.postContentViewer->getAndResetTextures();
            // std::move(postContentViewerTextures.begin(),postContentViewerTextures.end(),std::back_inserter(textures));
            /*auto fbo = p.postContentViewer->getAndResetMediaFBO();
            if(fbo > 0)
            {
                fbos.push_back(fbo);
            }*/
        }
    }
    // glDeleteFramebuffers(fbos.size(), fbos.data());
    glDeleteTextures(textures.size(), textures.data());
}
void SubredditWindow::loadSubreddit()
{
    target = subredditTarget;
    if (subredditTarget.empty())
    {
        title = "Front Page";
        target = "/";
    }
    else
    {
        title = subredditTarget;
        if (!target.starts_with("r/") && !target.starts_with("/r/") &&
            (!target.starts_with("/user/") && target.find("/m/") == target.npos))
        {
            target = "/r/" + target;
        }
    }
    updateWindowsNames();
    if (!target.starts_with("/"))
        target = "/" + target;
    loadSubredditListings(target, token);
    lookAndDestroyPostsContents();
    // subredditStylesheet->LoadSubredditStylesheet(target);
}
void SubredditWindow::updateWindowsNames()
{
    std::string sortingText;
    auto sortingPostsIt =
        std::find_if(sortPosts.begin(), sortPosts.end(), [](const auto& s)
                     { return s.selected && !s.sort.empty(); });
    std::string windowTitle = title;
    std::string aboutWindowTitle = title;
    if (subredditAbout)
    {
        windowTitle = subredditAbout->title;
        aboutSubredditWindowName = subredditAbout->displayNamePrefixed;
    }
    if (sortingPostsIt != sortPosts.end())
    {
        windowTitle += " - " + sortingPostsIt->label;
    }

    windowName = fmt::format("{}###{}", windowTitle, id);
    aboutSubredditWindowName =
        fmt::format("About {}###{}_about", aboutWindowTitle, id);
}
void SubredditWindow::lookAndDestroyPostsContents()
{
    postsContentDestroyerTimer.expires_after(std::chrono::seconds(5));
    postsContentDestroyerTimer.async_wait(
        [weak = weak_from_this()](const boost::system::error_code& ec)
        {
            auto self = weak.lock();
            if (self && !ec)
            {
                for (auto&& p : self->posts)
                {
                    if (!p.showingContent && p.postContentViewer)
                    {
                        auto duration = std::chrono::steady_clock::now() -
                                        p.lastPostShowTime;
                        if (duration > std::chrono::seconds(20))
                        {
                            p.postContentViewer.reset();
                        }
                    }
                }

                self->lookAndDestroyPostsContents();
            }
        });
}

void SubredditWindow::loadSubredditListings(const std::string& target,
                                            const access_token& token)
{
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
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setErrorMessage,
                                            self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setErrorMessage,
                                            self, std::move(response.body)));
            }
            else
            {
                boost::asio::post(
                    self->uiExecutor,
                    std::bind(&SubredditWindow::loadListingsFromConnection,
                              self, std::move(response.data)));
            }
        });
    listingConnection->targetChangedHandler(
        [weak = weak_from_this()](std::string newTarget, std::any)
        {
            auto self = weak.lock();
            if (!self)
                return;

            boost::asio::post(self->uiExecutor,
                              std::bind(&SubredditWindow::changeSubreddit, self,
                                        std::move(newTarget)));
        });
    std::string sort = "";
    auto sortPostsIt = std::find_if(sortPosts.begin(), sortPosts.end(),
                                    [](const auto& s) { return s.selected; });
    if (sortPostsIt != sortPosts.end())
    {
        sort = sortPostsIt->sort;
    }
    listingConnection->list(target + sort, token);
    downloadSubredditAbout();
}
void SubredditWindow::downloadSubredditAbout()
{
    spdlog::debug("Loading about data of : {} ", target);
    if (target != "/r/random" && target != "/r/randnsfw" && target != "/" &&
        target != "/r/all" && target.find("/m/") == target.npos &&
        target.find("/user/") == target.npos)
    {
        auto aboutConnection = client->makeListingClientConnection();
        aboutConnection->connectionCompleteHandler(
            [weak = weak_from_this()](const boost::system::error_code& ec,
                                      client_response<listing> response)
            {
                auto self = weak.lock();
                if (!self)
                    return;
                spdlog::debug("Loaded about data : {} ", response.body);
                if (!ec)
                {
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&SubredditWindow::loadAbout,
                                                self, std::move(response.data)));
                }
            });
        aboutConnection->list(target + "/about", token);
    }
}
RedditClientProducer::RedditSRSubscriptionClientConnection
SubredditWindow::makeSubscriptionConnection()
{
    auto srSubscriptionConnection =
        client->makeRedditRedditSRSubscriptionClientConnection();
    srSubscriptionConnection->connectionCompleteHandler(
        [weak = weak_from_this()](const boost::system::error_code& ec,
                                  client_response<bool> response)
        {
            auto self = weak.lock();
            if (!self)
                return;
            if (!ec && response.data)
            {
                auto aboutConnection =
                    self->client->makeListingClientConnection();
                aboutConnection->connectionCompleteHandler(
                    [weak = self->weak_from_this()](
                        const boost::system::error_code& ec,
                        client_response<listing> response)
                    {
                        auto self = weak.lock();
                        if (!self)
                            return;
                        spdlog::debug("Loaded about data : {} ", response.body);
                        if (!ec)
                        {
                            boost::asio::post(
                                self->uiExecutor,
                                std::bind(&SubredditWindow::loadAbout, self,
                                          std::move(response.data)));
                        }
                    });
                aboutConnection->list(self->target + "/about", self->token);
                self->subscriptionChangedSignal();
            }
        });
    return srSubscriptionConnection;
}
void SubredditWindow::changeSubreddit(std::string newSubreddit)
{
    const std::string jsonStr{ "/.json" };
    if (newSubreddit.ends_with(jsonStr))
    {
        newSubreddit.erase(newSubreddit.length() - jsonStr.length(),
                           jsonStr.length());
    }
    if (newSubreddit.starts_with("/"))
    {
        newSubreddit.erase(newSubreddit.begin());
    }

    this->subredditTarget = std::move(newSubreddit);
    loadSubreddit();
}
void SubredditWindow::setErrorMessage(std::string errorMessage)
{
    listingErrorMessage = std::move(errorMessage);
}
void SubredditWindow::loadAbout(listing aboutData)
{
    if (!aboutData.json.contains("kind") || !aboutData.json["kind"].is_string() ||
        aboutData.json["kind"].get<std::string>() != "t5" ||
        !aboutData.json.contains("data") || !aboutData.json["data"].is_object())
    {
        return;
    }

    subredditAbout = std::make_optional<subreddit>(aboutData.json["data"]);
    updateWindowsNames();
}
void SubredditWindow::loadListingsFromConnection(listing listingResponse)
{
    posts_list tmpPosts;

    for (auto& child : listingResponse.json["data"]["children"])
    {
        auto kind = child["kind"].get<std::string>();
        if (kind == "t3")
        {
            tmpPosts.emplace_back(std::make_shared<post>(child["data"]), token,
                                  client, uiExecutor);
        }
    }
    setListings(std::move(tmpPosts),
                std::move(listingResponse.json["data"]["before"]),
                std::move(listingResponse.json["data"]["after"]));
}
RedditClientProducer::RedditResourceClientConnection
SubredditWindow::makeResourceConnection()
{
    auto resourceConnection = client->makeResourceClientConnection();
    resourceConnection->connectionCompleteHandler(
        [weak = weak_from_this()](const boost::system::error_code& ec,
                                  resource_response response)
        {
            auto self = weak.lock();
            if (!self)
                return;
            std::string postName = "";
            if (response.userData.has_value() &&
                response.userData.type() == typeid(std::string))
            {
                postName = std::any_cast<std::string>(response.userData);
            }
            if (ec)
            {
                auto message = "Cannot load thumbnail:" + ec.message();
                boost::asio::post(
                    self->uiExecutor,
                    std::bind(&SubredditWindow::setPostErrorMessage, self,
                              std::move(postName), std::move(message)));
            }
            else if (response.status >= 400)
            {
                auto message = "Cannot load thumbnail:" + response.body;
                boost::asio::post(
                    self->uiExecutor,
                    std::bind(&SubredditWindow::setPostErrorMessage, self,
                              std::move(postName), std::move(message)));
            }
            else if (response.status == 200)
            {

                int width, height, channels;
                auto data = Utils::decodeImageData(response.data.data(),
                                                   response.data.size(), &width,
                                                   &height, &channels);
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setPostThumbnail,
                                            self, std::move(postName),
                                            std::move(data), width, height,
                                            channels));
            }
        });
    return resourceConnection;
}
void SubredditWindow::setListings(posts_list receivedPosts,
                                  nlohmann::json beforeJson,
                                  nlohmann::json afterJson)
{
    before.reset();
    after.reset();
    if (!beforeJson.is_null() && beforeJson.is_string())
    {
        before = beforeJson.get<std::string>();
    }
    if (!afterJson.is_null() && afterJson.is_string())
    {
        after = afterJson.get<std::string>();
    }
    listingErrorMessage.clear();
    clearExistingPostsData();
    posts = std::move(receivedPosts);
    scrollToTop = true;
    for (auto& p : posts)
    {
        if (subredditName.empty())
        {
            subredditName = p.post->subredditName;
        }
        p.upvoteButtonText = fmt::format(
            "{}##_up{}", reinterpret_cast<const char*>(ICON_FA_ARROW_UP),
            p.post->name);
        p.downvoteButtonText = fmt::format(
            "{}##_down{}", reinterpret_cast<const char*>(ICON_FA_ARROW_DOWN),
            p.post->name);
        p.openLinkButtonText =
            fmt::format("{}##openLink{}",
                        reinterpret_cast<const char*>(
                            ICON_FA_EXTERNAL_LINK_SQUARE " Open Link"),
                        p.post->name);
        p.votesChildText = fmt::format("##{}_votes_child", p.post->name);
        p.subredditLinkText = fmt::format("##{}_subredditlink", p.post->name);
        p.updateShowContentText();
        if (!p.post->thumbnail.empty())
        {
            p.standardThumbnail = Utils::GetRedditThumbnail(p.post->thumbnail);
            if (!p.standardThumbnail)
            {
                makeResourceConnection()->getResource(p.post->thumbnail,
                                                      p.post->name);
            }
        }
        p.showContentThumbnailId = fmt::format("##{}_thumbnail", p.post->name);
    }
}
void SubredditWindow::setPostErrorMessage(std::string postName, std::string msg)
{
    auto it = std::find_if(posts.begin(), posts.end(),
                           [name = std::move(postName)](const auto& p)
                           { return p.post->name == name; });
    if (it != posts.end())
    {
        it->errorMessageText = std::move(msg);
    }
}
void SubredditWindow::PostDisplay::updateShowContentText()
{
    showContentButtonText = fmt::format(
        "{}##showContent{}",
        reinterpret_cast<const char*>(showingContent ? ICON_FA_MINUS_SQUARE_O
                                                     : ICON_FA_PLUS_SQUARE_O),
        post->name);
}
void SubredditWindow::setPostThumbnail(std::string postName,
                                       Utils::STBImagePtr data,
                                       int width,
                                       int height,
                                       int channels)
{
    ((void)channels);
    auto it = std::find_if(posts.begin(), posts.end(),
                           [name = std::move(postName)](const auto& p)
                           { return p.post->name == name; });
    if (it != posts.end())
    {
        it->thumbnailPicture =
            Utils::loadImage(data.get(), width, height, STBI_rgb_alpha);
        if (it->post->over18 && shouldBlurPictures)
        {
            it->blurredThumbnailPicture = Utils::loadBlurredImage(
                data.get(), width, height, STBI_rgb_alpha);
        }
    }
}
void SubredditWindow::votePost(post_ptr p, Voted voted)
{
    listingErrorMessage.clear();
    std::pair<std::string, Voted> pair = std::make_pair<>(p->name, voted);
    auto voteConnection = client->makeRedditVoteClientConnection();
    voteConnection->connectionCompleteHandler(
        [weak = weak_from_this()](const boost::system::error_code& ec,
                                  client_response<std::string> response)
        {
            auto self = weak.lock();
            if (!self)
                return;

            if (ec)
            {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setErrorMessage,
                                            self, ec.message()));
            }
            else if (response.status >= 400)
            {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setErrorMessage,
                                            self, std::move(response.body)));
            }
            else if (response.userData.has_value() &&
                     response.userData.type() ==
                         typeid(std::pair<std::string, Voted>))
            {
                auto p = std::any_cast<std::pair<std::string, Voted>>(
                    response.userData);
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::updatePostVote,
                                            self, std::move(p.first), p.second));
            }
        });
    voteConnection->vote(p->name, token, voted, std::move(pair));
}
void SubredditWindow::updatePostVote(std::string postName, Voted voted)
{
    auto it = std::find_if(posts.begin(), posts.end(),
                           [name = std::move(postName)](const auto& p)
                           { return p.post->name == name; });
    if (it != posts.end())
    {
        auto oldVoted = it->post->voted;
        it->post->voted = voted;
        it->post->humanScore =
            Utils::CalculateScore(it->post->score, oldVoted, voted);
    }
}
void SubredditWindow::pauseAllMedia()
{
    for (auto&& p : posts)
    {
        if (p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(true);
        }
    }
}
void SubredditWindow::refreshPosts()
{
    shouldBlurPictures = Database::getInstance()->getBlurNSFWPictures();
    listingErrorMessage.clear();
    clearExistingPostsData();
    posts.clear();
    loadSubredditListings(target, token);
    rearmRefreshTimer();
}
void SubredditWindow::rearmRefreshTimer()
{
    if (autoRefreshEnabled)
    {
        refreshTimer.expires_after(std::chrono::seconds(
            Database::getInstance()->getAutoRefreshTimeout()));
        refreshTimer.async_wait(
            [weak = weak_from_this()](const boost::system::error_code& ec)
            {
                auto self = weak.lock();
                if (self && !ec)
                {
                    self->refreshPosts();
                }
            });
    }
    else
    {
        refreshTimer.cancel();
    }
}
void SubredditWindow::renderPostVotes(PostDisplay& p)
{
    ImGui::BeginChild(
        p.votesChildText.c_str(),
        ImVec2(maxScoreWidth,
               ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.x * 2));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::Dummy(ImVec2(upvotesButtonsIdent / 2.f, 0.f));
    ImGui::SameLine();

    if (p.post->voted == Voted::UpVoted)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Utils::GetUpVoteColor());
    }

    if (ImGui::Button(p.upvoteButtonText.c_str()))
    {
        votePost(p.post, p.post->voted == Voted::UpVoted ? Voted::NotVoted
                                                         : Voted::UpVoted);
    }

    if (p.post->voted == Voted::UpVoted)
    {
        ImGui::PopStyleColor(1);
    }

    auto scoreIdent =
        (maxScoreWidth - ImGui::CalcTextSize(p.post->humanScore.c_str()).x) / 2.f;
    // ImGui::Dummy(ImVec2(scoreIdent,0.f));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::SameLine(scoreIdent);
    ImGui::Text("%s", p.post->humanScore.c_str());
    if (ImGui::GetItemRectSize().x > maxScoreWidth)
        maxScoreWidth = ImGui::GetItemRectSize().x;
    ImGui::Dummy(ImVec2(upvotesButtonsIdent / 2.f, 0.f));
    ImGui::SameLine();

    if (p.post->voted == Voted::DownVoted)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Utils::GetDownVoteColor());
    }

    if (ImGui::Button(p.downvoteButtonText.c_str()))
    {
        votePost(p.post, p.post->voted == Voted::DownVoted ? Voted::NotVoted
                                                           : Voted::DownVoted);
    }

    if (p.post->voted == Voted::DownVoted)
    {
        ImGui::PopStyleColor(1);
    }

    ImGui::PopStyleColor(1);
    ImGui::EndChild();
}
float SubredditWindow::renderPostThumbnail(PostDisplay& p)
{
    auto height = ImGui::GetCursorPosY();

    GLuint textureId = -1;
    ImVec2 size;
    ImVec2 uv0(0, 0);
    ImVec2 uv1(1, 1);
    if (p.post->over18 && shouldBlurPictures && p.blurredThumbnailPicture &&
        !p.shouldShowUnblurredImage)
    {
        textureId = p.blurredThumbnailPicture->textureId;
        size = ImVec2(p.blurredThumbnailPicture->width,
                      p.blurredThumbnailPicture->height);
    }
    else if (p.thumbnailPicture)
    {
        textureId = p.thumbnailPicture->textureId;
        size = ImVec2(p.thumbnailPicture->width, p.thumbnailPicture->height);
    }
    else if (p.standardThumbnail)
    {
        auto redditSprites = Utils::GetRedditDefaultSprites();
        textureId = redditSprites->textureId;
        size = p.standardThumbnail->size;
        uv0 = p.standardThumbnail->uv0;
        uv1 = p.standardThumbnail->uv1;
    }

    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    bool clicked = ImGui::ImageButton(p.showContentThumbnailId.c_str(),
                                      (void*)(intptr_t)textureId, size, uv0, uv1);
    ImGui::PopStyleColor(3);

    height = std::fmax(height, ImGui::GetCursorPosY());
    auto rectMin = ImGui::GetItemRectMin();
    auto rectMax = ImGui::GetItemRectMax();

    if (p.post->over18 && shouldBlurPictures && p.blurredThumbnailPicture &&
        !p.shouldShowUnblurredImage && ImGui::IsWindowFocused() &&
        ImGui::IsMouseHoveringRect(rectMin, rectMax))
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Hold down Ctrl to unblur");
        ImGui::EndTooltip();
    }
    // if Ctrl is pressed, next render it will show the unblurred version
    p.shouldShowUnblurredImage =
        ImGui::IsWindowFocused() &&
        ImGui::IsMouseHoveringRect(rectMin, rectMax) &&
        (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl)) ||
         ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RightCtrl)));
    if (ImGui::IsWindowFocused() &&
        ImGui::IsMouseHoveringRect(rectMin, rectMax) && clicked)
    {
        p.showingContent = !p.showingContent;
        p.updateShowContentText();
        if (!p.showingContent && p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(true);
        }
        else if (p.showingContent && p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(false);
        }
    }
    ImGui::SameLine();
    return height;
}
void SubredditWindow::renderPostShowContentButton(PostDisplay& p)
{
    if (ImGui::Button(p.showContentButtonText.c_str()))
    {
        p.showingContent = !p.showingContent;
        p.updateShowContentText();
        if (!p.showingContent && p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(true);
        }
        else if (p.showingContent && p.postContentViewer)
        {
            p.postContentViewer->stopPlayingMedia(false);
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(p.showingContent ? "Hide post content"
                                                : "Show post content");
        ImGui::EndTooltip();
    }
}
void SubredditWindow::renderPostCommentsButton(PostDisplay& p)
{
    ImGui::PushID(("comments" + p.post->id).c_str());
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::Text("%s", p.post->commentsText.c_str());
    auto rectMax = ImGui::GetItemRectMax();
    auto rectMin = ImGui::GetItemRectMin();
    rectMin.y = rectMax.y;
    ImGui::SetCursorScreenPos(p0);
    auto subredditTextSize = ImGui::GetItemRectSize();
    if (ImGui::InvisibleButton("comments", subredditTextSize))
    {
        commentsSignal(p.post->id, p.post->title);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::GetWindowDrawList()->AddLine(rectMin, rectMax,
                                            ImGui::GetColorU32(ImGuiCol_Text));
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Show Comments");
        ImGui::EndTooltip();
    }
    ImGui::PopID();
}
void SubredditWindow::renderPostOpenLinkButton(PostDisplay& p)
{
    if (!p.post->url.empty())
    {
        ImGui::SameLine();
        ImGui::PushID(("openlink" + p.post->id).c_str());
        ImGui::AlignTextToFramePadding();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::Text("%s", reinterpret_cast<const char*>(
                              "Open Link " ICON_FA_EXTERNAL_LINK_SQUARE));
        auto rectMax = ImGui::GetItemRectMax();
        auto rectMin = ImGui::GetItemRectMin();
        rectMin.y = rectMax.y;
        ImGui::SetCursorScreenPos(p0);
        auto subredditTextSize = ImGui::GetItemRectSize();
        if (ImGui::InvisibleButton(p.openLinkButtonText.c_str(),
                                   subredditTextSize))
        {
            try
            {
                Uri urlParts(p.post->url);
                auto host = urlParts.host();
                if (host.find("reddit.com") == std::string::npos ||
                    ImGui::GetIO().KeyCtrl)
                {
                    Utils::openInBrowser(p.post->url);
                }
                else
                {
                    auto target = urlParts.fullPath();
                    if (target.find("/comments/") != std::string::npos)
                    {
                        commentsSignal(p.post->id, p.post->title);
                    }
                    else
                    {
                        subredditSignal(target);
                    }
                }
            }
            catch (const std::exception& ex)
            {
                p.errorMessageText = ex.what();
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::GetWindowDrawList()->AddLine(
                rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Text));
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(p.post->url.c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
}
void SubredditWindow::showWindow(int appFrameWidth, int appFrameHeight)
{
    ImGui::SetNextWindowSize(ImVec2(appFrameWidth * 0.6, appFrameHeight * 0.8),
                             ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(windowName.c_str(), &windowOpen,
                      ImGuiWindowFlags_HorizontalScrollbar))
    {
        pauseAllMedia();
        ImGui::End();
        return;
    }
    if (!windowOpen)
    {
        pauseAllMedia();
        ImGui::End();
        return;
    }
    if (willBeFocused)
    {
        ImGui::SetWindowFocus();
        willBeFocused = false;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)) &&
        ImGui::GetIO().KeyCtrl && ImGui::IsWindowFocused())
    {
        pauseAllMedia();
        windowOpen = false;
    }
    if (windowPositionAndSizeSet)
    {
        windowPositionAndSizeSet = false;
        ImGui::SetWindowPos(windowPos);
        ImGui::SetWindowSize(windowSize);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)) &&
        ImGui::IsWindowFocused())
    {
        refreshPosts();
    }

    if (scrollToTop)
    {
        ImGui::SetScrollHereY(0.0f);
        scrollToTop = false;
    }

    // subredditStylesheet->ShowHeader();

    // ImGuiStyle& style = ImGui::GetStyle();
    // auto textItemWidth = ImGui::GetItemRectSize().x + style.ItemSpacing.x +
    // style.ItemInnerSpacing.x;
    // auto availableWidth = ImGui::GetContentRegionAvailWidth();

    if (!listingErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s",
                           listingErrorMessage.c_str());
    }

    if (subredditAbout)
    {
        if (subredditAbout->userIsBanned)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                               "You are banned here");
        }
        else
        {
            if (ImGui::Button("Submit New Post"))
            {
                newTextPostDialog = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Submit New Link"))
            {
                newLinkPostDialog = true;
            }
            ImGui::SameLine();
        }
    }

    if (ImGui::Checkbox("Auto Refresh", &autoRefreshEnabled))
    {
        rearmRefreshTimer();
    }
    if (autoRefreshEnabled)
    {
        ImGui::SameLine();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            refreshTimer.expiry() - std::chrono::steady_clock::now());
        std::string text = Utils::formatDuration(diff);
        ImGui::PushFont(
            ImGui::GetIO()
                .Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopFont();
    }
    {
        ImGui::SameLine();
        if (sortPostsLabelWidth < 0.f)
        {
            for (const auto& s : sortPosts)
            {
                auto size = ImGui::CalcTextSize(s.label.c_str()).x;
                if (size > sortPostsLabelWidth)
                {
                    sortPostsLabelWidth = size;
                }
            }
        }
        ImGui::SetNextItemWidth(sortPostsLabelWidth + ImGui::GetFrameHeight() +
                                ImGui::GetStyle().FramePadding.x * 2.0f);
        if (ImGui::BeginCombo("Sort", currentlySelectedSortPostLabel.c_str()))
        {
            for (auto&& s : sortPosts)
            {
                if (ImGui::Selectable(s.label.c_str(), s.selected))
                {
                    if (!s.selected)
                    {
                        for (auto&& ss : sortPosts)
                            ss.selected = false;
                        s.selected = true;
                        currentlySelectedSortPostLabel = s.label;
                        refreshPosts();
                        updateWindowsNames();
                    }
                }
            }
            ImGui::EndCombo();
        }
    }
    {
        if (ImGui::BeginPopupContextItem("aboutsubredditpopup"))
        {
            bool refreshButtonDisabled = posts.empty();
            if (ImGui::Selectable(reinterpret_cast<const char*>(
                    ICON_FA_REFRESH " Refresh (F5)")) &&
                !refreshButtonDisabled)
            {
                refreshPosts();
            }
            if (subredditAbout && !subredditAbout->userIsSubscriber &&
                ImGui::Selectable("Subscribe"))
            {
                makeSubscriptionConnection()->updateSrSubscription(
                    { subredditAbout->name },
                    RedditSRSubscriptionConnection::SubscriptionAction::Subscribe,
                    token);
            }
            if (subredditAbout && subredditAbout->userIsSubscriber &&
                ImGui::Selectable("Unsubscribe"))
            {
                makeSubscriptionConnection()->updateSrSubscription(
                    { subredditAbout->name },
                    RedditSRSubscriptionConnection::SubscriptionAction::Unsubscribe,
                    token);
            }
            if (subredditAbout && ImGui::Selectable("About"))
            {
                aboutSubredditWindowOpen = true;
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x -
                        (ImGui::GetStyle().FramePadding.x) -
                        ImGui::GetFontSize());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
        if (ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ELLIPSIS_V)))
        {
            ImGui::OpenPopup("aboutsubredditpopup");
        }
        ImGui::PopStyleColor();
    }

    if (maxScoreWidth == 0.f)
    {
        maxScoreWidth = ImGui::CalcTextSize("XXXXXX").x;
    }
    if (upvotesButtonsIdent == 0.f)
    {
        std::string btnText =
            posts.empty() ? reinterpret_cast<const char*>(ICON_FA_ARROW_UP)
                          : posts.front().upvoteButtonText;
        auto upvotesButtonsWidth = ImGui::CalcTextSize(btnText.c_str()).x +
                                   (ImGui::GetStyle().FramePadding.x);
        upvotesButtonsIdent = (maxScoreWidth - upvotesButtonsWidth) / 2.f -
                              (ImGui::GetStyle().FramePadding.x);
    }

    for (auto& p : posts)
    {
        if (!p.errorMessageText.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s",
                               p.errorMessageText.c_str());
        }

        renderPostVotes(p);

        ImGui::SameLine();

        auto height = renderPostThumbnail(p);

        ImGui::BeginGroup();
        {
            ImGui::PushFont(
                ImGui::GetIO()
                    .Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Bold)]);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::Text("%s", p.post->subreddit.c_str());
            ImGui::PopFont();
            auto rectMax = ImGui::GetItemRectMax();
            auto rectMin = ImGui::GetItemRectMin();
            rectMin.y = rectMax.y;
            ImGui::SetCursorScreenPos(p0);
            auto subredditTextSize = ImGui::GetItemRectSize();
            if (ImGui::InvisibleButton(p.subredditLinkText.c_str(),
                                       subredditTextSize) &&
                p.post->subreddit != subredditTarget)
            {
                subredditSignal(p.post->subreddit);
            }
            if (ImGui::IsItemHovered() && p.post->subreddit != subredditTarget)
            {
                ImGui::GetWindowDrawList()->AddLine(
                    rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Text));
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Open Subreddit");
                ImGui::EndTooltip();
            }
        }

        ImGui::SameLine();
        ImGui::PushFont(
            ImGui::GetIO()
                .Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("Posted by %s %s", p.post->author.c_str(),
                    p.post->humanReadableTimeDifference.c_str());

        // Awards
        ImGui::SameLine();
        p.awardsRenderer->Render();

        if (p.post->over18)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "NSFW");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Not Safe For Work");
                ImGui::EndTooltip();
            }
        }
        ImGui::PopFont();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(
            Utils::Fonts::Noto_Medium_Big)]);
        if (!p.post->title.empty())
        {
            // ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x-(ImGui::GetStyle().FramePadding.x*2.0f)
            // - ImGui::GetStyle().ScrollbarSize);
            // ImGui::SetNextItemWidth(sortPostsLabelWidth +
            // ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x*2.0f);
            ImGui::TextWrapped("%s", p.post->title.c_str());
        }
        else
        {
            ImGui::TextWrapped("<No Title>");
        }
        ImGui::PopFont();

        p.flairRenderer->Render();

        ImGui::PushFont(
            ImGui::GetIO()
                .Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Light)]);
        ImGui::Text("(%s)", p.post->domain.c_str());
        ImGui::PopFont();

        auto normalPositionY = ImGui::GetCursorPosY();
        auto desiredPositionY = height - ImGui::GetFrameHeightWithSpacing();
        if (normalPositionY < desiredPositionY)
            ImGui::SetCursorPosY(desiredPositionY);

        renderPostShowContentButton(p);

        ImGui::SameLine();

        renderPostCommentsButton(p);

        renderPostOpenLinkButton(p);

        if (p.showingContent)
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() / 2.f));
            if (!p.postContentViewer)
            {
                p.postContentViewer = std::make_shared<PostContentViewer>(
                    client, token, uiExecutor);
            }
            if (!p.postContentViewer->isCurrentPostSet())
            {
                p.postContentViewer->loadContent(p.post);
            }
            p.postContentViewer->showPostContent();
            p.lastPostShowTime = std::chrono::steady_clock::now();
        }

        ImGui::EndGroup();
        ImGui::Separator();
    }

    if (!posts.empty())
    {
        bool previousEnabled = before.has_value();
        bool nextEnabled = after.has_value();
        if (!previousEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                ImGui::GetStyle().Alpha * 0.5f);
        }
        if ((ImGui::Button(reinterpret_cast<const char*>(ICON_FA_ARROW_LEFT
                                                         " Previous")) ||
             (ImGui::IsMouseClicked(3) && ImGui::IsWindowFocused())) &&
            previousEnabled)
        {
            auto oldPostsSize = posts.size();
            clearExistingPostsData();
            posts.clear();
            loadSubredditListings(target + "?before=" + before.value() +
                                      "&count=" + std::to_string(currentCount),
                                  token);
            currentCount -= oldPostsSize;
            before.reset();
            after.reset();
        }
        if (ImGui::IsItemHovered() && ImGui::IsWindowFocused())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Previous page (mouse back button)");
            ImGui::EndTooltip();
        }
        if (!previousEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
        auto width = ImGui::GetWindowContentRegionMax().x;
        auto previousSize = ImGui::GetItemRectSize();
        ImGui::SameLine(width - previousSize.x);
        if (!nextEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                ImGui::GetStyle().Alpha * 0.5f);
        }
        if ((ImGui::Button(
                 reinterpret_cast<const char*>("Next " ICON_FA_ARROW_RIGHT),
                 previousSize) ||
             (ImGui::IsMouseClicked(4) && ImGui::IsWindowFocused())) &&
            nextEnabled)
        {
            currentCount += posts.size();
            clearExistingPostsData();
            posts.clear();
            loadSubredditListings(target + "?after=" + after.value() +
                                      "&count=" + std::to_string(currentCount),
                                  token);
            after.reset();
            before.reset();
        }
        if (ImGui::IsItemHovered() && ImGui::IsWindowFocused())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Next page (mouse forward button)");
            ImGui::EndTooltip();
        }
        if (!nextEnabled)
        {
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
    }

    showNewTextPostDialog();
    showNewLinkPostDialog();

    ImGui::End();

    showAboutWindow();
}

void SubredditWindow::showAboutWindow()
{
    if (!subredditAbout || !aboutSubredditWindowOpen)
        return;
    if (!subredditAboutDisplay)
    {
        subredditAboutDisplay =
            std::make_optional<AboutDisplay>(subredditAbout.value());
    }

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(aboutSubredditWindowName.c_str(),
                     &aboutSubredditWindowOpen, ImGuiWindowFlags_None))
    {
        ImGui::Text("%s", subredditAbout->title.c_str());
        subredditAboutDisplay->description.RenderMarkdown();
        subredditAboutDisplay->submit.RenderMarkdown();
    }
    ImGui::End();
}

void SubredditWindow::showNewLinkPostDialog()
{
    auto windowWidth = ImGui::GetWindowWidth();
    if (newLinkPostDialog)
    {
        ImGui::OpenPopup(NEWLINK_POST_POPUP_TITLE);
        newLinkPostDialog = false;
        showingLinkPostDialog = true;
    }
    if (showingLinkPostDialog &&
        ImGui::BeginPopupModal(NEWLINK_POST_POPUP_TITLE, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("*Title:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        {
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SetNextItemWidth(windowWidth * 0.5f);
        ImGui::InputText("##newLinkPostTitle", &newTextPostTitle);
        ImGui::Text("*Link:");
        ImGui::SetNextItemWidth(windowWidth * 0.5f);
        ImGui::InputText("##newLinkPostText", &newLinkPost);

        bool okDisabled = newTextPostTitle.empty() || newLinkPost.empty();
        if (okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            auto p = std::make_shared<post>();
            p->url = newLinkPost;
            p->title = newTextPostTitle;
            p->subreddit = subredditName;
            p->postHint = "link";
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
    if (newTextPostDialog)
    {
        ImGui::OpenPopup(NEWTEXT_POST_POPUP_TITLE);
        newTextPostDialog = false;
        showingTextPostDialog = true;
    }
    if (showingTextPostDialog &&
        ImGui::BeginPopupModal(NEWTEXT_POST_POPUP_TITLE, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("*Title:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        {
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SetNextItemWidth(windowWidth * 0.75f);
        ImGui::InputText("##newTextPostTitle", &newTextPostTitle);
        ImGui::Text("Text:");
        ImGui::SetNextItemWidth(windowWidth * 0.75f);
        ImGui::InputTextMultiline("##newTextPostText", &newTextPostContent);

        bool okDisabled = newTextPostTitle.empty();
        if (okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            auto p = std::make_shared<post>();
            p->selfText = newTextPostContent;
            p->title = newTextPostTitle;
            p->subreddit = subredditName;
            p->postHint = "self";
            submitNewPost(p);
            showingTextPostDialog = false;
            newTextPostTitle.clear();
            newTextPostContent.clear();
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
            if (ImGui::MenuItem("Save Statistics", "Ctrl+S", false))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Refresh", "F5", false))
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
    createPostConnection->connectionCompleteHandler(
        [weak = weak_from_this()](const boost::system::error_code& ec,
                                  client_response<post_ptr> response)
        {
            auto self = weak.lock();
            if (!self)
                return;
            if (ec)
            {
                boost::asio::post(self->uiExecutor,
                                  std::bind(&SubredditWindow::setErrorMessage,
                                            self, ec.message()));
            }
            else
            {
                if (response.status >= 400)
                {
                    boost::asio::post(self->uiExecutor,
                                      std::bind(&SubredditWindow::setErrorMessage,
                                                self, response.body));
                }
                else
                {
                    if (!response.data)
                    {
                        boost::asio::post(
                            self->uiExecutor,
                            std::bind(&SubredditWindow::setErrorMessage, self,
                                      "Cannot create post"));
                    }
                    else
                    {
                        self->commentsSignal(response.data->id,
                                             response.data->title);
                    }
                }
            }
        });
    createPostConnection->createPost(p, true, token);
}
