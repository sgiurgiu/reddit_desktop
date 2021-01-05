#ifndef ENTITIES_H
#define ENTITIES_H

#include <string>
#include "json.hpp"
#include <vector>
#include <memory>
#include <chrono>

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
    std::optional<reddit_video> redditVideo;
    std::optional<oembed> oemEmbed;
};


struct post_gallery_item
{
    uint64_t id = 0;
    std::string mediaId;
    std::string url;
};

enum class Voted
{
    DownVoted = -1,
    NotVoted = 0,
    UpVoted = 1
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
    std::string subredditName;
    std::string subredditId;
    int score = 0;
    std::string humanScore;
    std::string url;
    std::string urlOverridenByDest;
    std::vector<images_preview> previews;
    std::string authorFullName;
    std::string author;
    std::string domain;
    std::string postHint;    
    std::optional<media> postMedia;
    bool isGallery = false;
    std::vector<post_gallery_item> gallery;
    bool over18 = false;
    bool locked = false;
    bool clicked = false;
    bool visited = false;
    Voted voted = Voted::NotVoted;
};
using post_ptr = std::shared_ptr<post>;
struct comment;
//using comment_ptr = std::shared_ptr<comment>;
using comments_list = std::vector<comment>;
struct comment
{
    comment(){}
    comment(const nlohmann::json& json);
    std::string id;
    std::string name;
    std::string body;
    bool hasMoreReplies = false;
    int ups = 0;
    int downs = 0;
    int score = 0;
    Voted voted = Voted::NotVoted;
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
//using user_info_ptr = std::shared_ptr<user_info>;
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
struct multireddit
{
    multireddit(){}
    multireddit(const nlohmann::json& json);
    std::string name;
    std::string owner;
    std::string ownerId;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    std::string description;
    std::string displayName;
    bool over18 = false;
    bool canEdit = false;
    int64_t subscribers = 0;
    std::string path;
};
using subreddit_list = std::vector<subreddit>;
using multireddit_list = std::vector<multireddit>;

struct message
{
    message(){}
    message(const nlohmann::json& json);
    std::string body;
    bool wasComment = false;
    std::optional<std::string> firstMessage;
    std::string name;
    std::optional<std::string> firstMessageName;
    uint64_t createdAt;
    std::string humanReadableTimeDifference;
    std::string author;
    std::string subreddit;
    std::string parentId;
    std::string context;
    std::string replies;
    std::string id;
    bool isNew = false;
    std::string distinguished;
    std::string subject;
};

#endif // ENTITIES_H
