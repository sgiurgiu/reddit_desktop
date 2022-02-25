#ifndef ENTITIES_H
#define ENTITIES_H

#include <string>
#include "json.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <any>
#include <queue>

struct user
{
    user(){}
    user(const std::string& username,const std::string& password,
         const std::string& client_id,const std::string& secret,
         const std::string& website,const std::string& app_name,
         bool lastLoggedIn):
        username(username),password(password),
        client_id(client_id),secret(secret),
        website(website),app_name(app_name),
        lastLoggedIn(lastLoggedIn)
    {}
    std::string username;
    std::string password;
    std::string client_id;
    std::string secret;
    std::string website;
    std::string app_name;
    bool lastLoggedIn;
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

template <typename T, typename U = std::any>
struct client_response
{
    typedef U   user_type;
    T data;
    int status;
    int64_t contentLength;
    std::string contentType;
    std::string body;
    U userData;
};
struct image_target
{
    image_target(){}
    image_target(const nlohmann::json& json);
    std::string url;
    int width = 0;
    int height = 0;
};
struct preview_variant
{
    std::string kind;
    image_target source;
    std::vector<image_target> resolutions;
}; 
struct images_preview
{
    image_target source;
    std::vector<image_target> resolutions;
    std::vector<preview_variant> variants;
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
struct award
{
    award(){}
    award(const nlohmann::json& json);
    std::string id;
    std::string name;
    std::string subredditId;
    bool isNew = false;
    image_target icon;
    image_target staticIcon;
    int daysOfPremium;
    std::vector<image_target> resizedIcons;
    std::string description;
    int count = 0;
    std::vector<image_target> resizedStaticIcons;
    std::string awardSubType;
    std::string awardType;
    int coinPrice = 0;
};

struct flair_richtext
{
    flair_richtext(){}
    flair_richtext(const nlohmann::json& json);
    std::string e; // this looks to be the type
    std::string t; //this looks to be the text
    std::string a; // looks to be the emoji id
    std::string u; //url for the emoji
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
    int gilded = 0;
    int totalAwardsReceived = 0;
    std::vector<award> allAwardings;
    std::map<std::string,int> gildings;
    std::string linkFlairType;
    std::string linkFlairText;
    std::string linkFlairTemplateId;
    std::vector<flair_richtext> linkFlairsRichText;
    std::string linkFlairBackgroundColor;
    std::string linkFlairCSSClass;
    std::string linkFlairTextColor;
    bool allow_live_comments = false;
    //this is an object
   // std::string link_rich_text;

};
using post_ptr = std::shared_ptr<post>;

struct unloaded_children
{
    unloaded_children(){}
    unloaded_children(const nlohmann::json& json);
    std::string id;
    std::string name;
    std::string parentId;
    int count = 0;
    int depth = 0;
    std::vector<std::string> children;
};

struct comment;
using comments_list = std::vector<comment>;
struct comment
{
    comment(){}
    comment(const nlohmann::json& json, const user& currentUser);
    std::string id;
    std::string name;
    std::string linkId;
    std::string body;
    std::string parentId;
    std::optional<unloaded_children> unloadedChildren;
    int ups = 0;
    int downs = 0;
    int score = 0;
    Voted voted = Voted::NotVoted;
    std::string humanScore;
    std::string authorFullName;
    std::string author;
    uint64_t createdAt = 0;
    std::string humanReadableTimeDifference;
    comments_list replies;
    bool isSubmitter = false;
    bool edited = false;
    bool locked = false;
    bool removed = false;
    // set to true if this comment is the current logged in user's comment
    bool isUsersComment = false;
    int gilded = 0;
    int totalAwardsReceived = 0;
    std::vector<award> allAwardings;
    std::map<std::string,int> gildings;
    std::string authorFlairType;
    std::string authorFlairText;
    std::string authorFlairTemplateId;
    std::vector<flair_richtext> authorFlairsRichText;
    std::string authorFlairBackgroundColor;
    std::string authorFlairCSSClass;
    std::string authorFlairTextColor;
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
    std::string submitText;
    std::string url;
    bool over18 = false;
    int64_t subscribers = 0;
    std::string title;
    bool userIsBanned = false;
    bool userIsModerator = false;
    bool userIsSubscriber = false;
    bool userIsMuted = false;
    bool quarantine = false;
    std::string iconImage;
    std::string headerImage;
    std::pair<int,int> iconSize;
    std::pair<int,int> headerSize;
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
    message(const nlohmann::json& json, const std::string& kind);
    std::string kind;
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
    Voted voted = Voted::NotVoted;
    int score = 0;
    std::string humanScore;
};

struct subreddit_about
{
    subreddit_about(){}
    subreddit_about(const nlohmann::json& json);

    std::string bannerBackgroundColor;
    std::string bannerBackgroundImage;
    std::string bannerImage;
    std::pair<int,int> bannerSize;
    std::string communityIcon;
    std::string headerImage;
    std::pair<int,int> headerSize;
    std::string headerTitle;
    std::string iconImage;
    std::pair<int,int> iconSize;
    std::string keyColor;
    std::string mobileBannerImage;
    std::string primaryColor;
    std::string publicDescription;
    std::string description;
};

struct stylesheet_image
{
    stylesheet_image(){}
    stylesheet_image(const nlohmann::json& json);
    std::string url;
    std::string link;
    std::string name;
};

struct subreddit_stylesheet
{
    subreddit_stylesheet(){}
    subreddit_stylesheet(const nlohmann::json& json);
    std::string subredditId;
    std::string stylesheet;
    std::vector<stylesheet_image> images;
};

struct live_update_event_about
{
    live_update_event_about(){}
    live_update_event_about(const nlohmann::json& json);

    std::string announcement_url;
    std::string button_cta;
    float created;
    float created_utc;
    std::string description;
    std::string icon;
    std::string id;
    bool is_announcement = false;
    std::string name;
    bool nsfw = false;
    int num_times_dismissable = 0;
    std::string resources;
    std::string state;
    std::string title;
    int total_views = 0;
    int viewer_count = 0;
    bool viewer_count_fuzzed = false;
    std::string websocket_url;
};

struct live_update_event_embed
{
    live_update_event_embed(){}
    live_update_event_embed(const nlohmann::json& json);
    std::string provider_url;
    std::string description;
    std::string original_url;
    std::string url;
    std::string title;
    std::string thumbnail_url;
    int thumbnail_width = 0;
    int thumbnail_height = 0;
    int width = 0;
    std::string author_name;
    std::string version;
    std::string provider_name;
    int64_t cache_age = 0;
    std::string type;
    std::string author_url;
};

struct live_update_event
{
    live_update_event(){}
    live_update_event(const nlohmann::json& json);
    std::string body;
    std::string name;
    std::string author;
    int64_t created;
    int64_t created_utc;
    bool stricken = false;
    std::string id;
    std::vector<live_update_event_embed> embeds;
};

#endif // ENTITIES_H
