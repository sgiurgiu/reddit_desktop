#ifndef ENTITIES_H
#define ENTITIES_H

#include <string>
#include "json.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <glad/glad.h>

struct user
{
    user(){}
    user(const std::string& username,const std::string& password,
         const std::string& client_id,const std::string& secret,
         const std::string& website,const std::string& app_name):
        username(username),password(password),
        client_id(client_id),secret(secret),
        website(website),app_name(app_name)
    {}
    std::string username;
    std::string password;
    std::string client_id;
    std::string secret;
    std::string website;
    std::string app_name;
};

std::string make_user_agent(const user& u);

struct access_token
{
    std::string token;
    std::string tokenType;
    int expires;
    std::string scope;
};

struct listing
{
    nlohmann::json json;
};

template <typename T>
struct client_response
{
    T data;
    int status;
    int64_t contentLength;
    std::string contentType;
    std::string body;
};

struct image_target
{
    std::string url;
    int width;
    int height;
};
struct images_preview
{
    image_target source;
    std::vector<image_target> resolutions;
};
struct gl_image
{
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    ~gl_image();
};
using gl_image_ptr = std::shared_ptr<gl_image>;
struct oembed
{
    int height;
    int width;
    int thumbnailHeight;
    int thumbnailWidth;
    std::string providerUrl;
    std::string thumbnailUrl;
    std::string title;
    std::string html;
    std::string providerName;
    std::string type;
};

struct reddit_video
{
    int bitrate;
    int height;
    int width;
    int64_t duration;
    std::string fallbackUrl;
    std::string scrubberMediaUrl;
    std::string dashUrl;
    std::string hlsUrl;
    bool isGif;
    std::string transcodingStatus;
};

struct media
{
    std::string type;
    std::unique_ptr<reddit_video> redditVideo;
    std::unique_ptr<oembed> oemEmbed;
};

struct gif_image
{
    gif_image(){}
    gif_image(gl_image_ptr img, int delay):
        img(std::move(img)),delay(delay)
    {}
    gl_image_ptr img;
    int delay;
    std::chrono::steady_clock::time_point lastDisplay;
    bool displayed = false;
};

struct post_gif
{
    std::vector<std::unique_ptr<gif_image>> images;
    int currentImage = 0;
};

struct post
{
    post(){}
    post(const nlohmann::json& json);
    std::string name;
    std::string id;
    std::string title;
    std::string selfText;
    int ups = 0;
    int downs = 0;
    bool isVideo = false;
    bool isSelf = false;
    bool isMeta = false;
    bool isOriginalContent = false;
    bool isRedditMediaDomain = false;
    bool isCrossPostable = false;
    std::string thumbnail;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    int commentsCount = 0;
    std::string commentsText;
    std::string subreddit;
    int score = 0;
    std::string humanScore;
    std::string url;
    std::string urlOverridenByDest;
    std::vector<images_preview> previews;
    gl_image_ptr thumbnail_picture;
    std::string authorFullName;
    std::string author;
    std::string domain;
    std::string postHint;
    gl_image_ptr post_picture;
    std::unique_ptr<media> postMedia;
    std::unique_ptr<post_gif> gif;
};
using post_ptr = std::shared_ptr<post>;
using posts_list = std::vector<post_ptr>;
struct comment;
using comment_ptr = std::shared_ptr<comment>;
using comments_list = std::vector<comment_ptr>;
struct comment
{
    comment(){}
    comment(const nlohmann::json& json);
    std::string id;
    std::string body;
    bool hasMoreReplies = false;
    int ups = 0;
    int downs = 0;
    int score = 0;
    std::string humanScore;
    std::string authorFullName;
    std::string author;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    comments_list replies;
};

struct user_info
{
    user_info(){}
    user_info(const nlohmann::json& json);
    std::string name;
    std::string id;
    std::string oauthClientId;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    int inboxCount = 0;
    int64_t commentKarma = 0;
    int64_t totalKarma = 0;
    int64_t linkKarma = 0;
    std::string humanCommentKarma;
    std::string humanTotalKarma;
    std::string humanLinkKarma;
    bool hasMail = false;
    bool hasModMail = false;
    int64_t goldCreddits = 0;
    bool isSuspended = false;
    bool isMod = false;
    bool isGold = false;
    int64_t coins = 0;
};
using user_info_ptr = std::shared_ptr<user_info>;
struct subreddit
{
    subreddit(){}
    subreddit(const nlohmann::json& json);
    std::string name;
    std::string id;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    std::string description;
    std::string displayName;
    std::string displayNamePrefixed;
    bool over18 = false;
    int64_t subscribers = 0;
    std::string title;
    bool userIsBanned = false;
    bool userIsModerator = false;
    bool userIsSubscriber = false;
    bool userIsMuted = false;
};
using subreddit_ptr = std::shared_ptr<subreddit>;
using subreddit_list = std::vector<subreddit_ptr>;
#endif // ENTITIES_H
