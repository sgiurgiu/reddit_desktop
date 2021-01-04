#include "entities.h"
#include <fmt/format.h>
#include "utils.h"
#include <iostream>
#include "fonts/IconsFontAwesome4.h"

std::string make_user_agent(const user& u)
{
    return fmt::format("{}:{}:v{} (by /u/{}",u.website,u.app_name,REDDITDESKTOP_VERSION,u.username);
}

namespace {
    std::optional<reddit_video> makeRedditMediaObject(const nlohmann::json& json)
    {
        auto redditMedia = std::make_optional<reddit_video>();
        if(json.contains("bitrate_kbps") && json["bitrate_kbps"].is_number())
        {
            redditMedia->bitrate = json["bitrate_kbps"].get<int>();
        }
        if(json.contains("height") && json["height"].is_number())
        {
            redditMedia->height = json["height"].get<int>();
        }
        if(json.contains("width") && json["width"].is_number())
        {
            redditMedia->width = json["width"].get<int>();
        }
        if(json.contains("duration") && json["duration"].is_number())
        {
            redditMedia->duration = json["duration"].get<int64_t>();
        }
        if(json.contains("fallback_url") && json["fallback_url"].is_string())
        {
            redditMedia->fallbackUrl = json["fallback_url"].get<std::string>();
        }
        if(json.contains("scrubber_media_url") && json["scrubber_media_url"].is_string())
        {
            redditMedia->scrubberMediaUrl = json["scrubber_media_url"].get<std::string>();
        }
        if(json.contains("dash_url") && json["dash_url"].is_string())
        {
            redditMedia->dashUrl = json["dash_url"].get<std::string>();
        }
        if(json.contains("hls_url") && json["hls_url"].is_string())
        {
            redditMedia->hlsUrl = json["hls_url"].get<std::string>();
        }
        if(json.contains("transcoding_status") && json["transcoding_status"].is_string())
        {
            redditMedia->transcodingStatus = json["transcoding_status"].get<std::string>();
        }
        if(json.contains("is_gif") && json["is_gif"].is_boolean())
        {
            redditMedia->isGif = json["is_gif"].get<bool>();
        }
        return redditMedia;
    }
    std::optional<oembed> makeOemEmbedObject(const nlohmann::json& json)
    {
        auto embed = std::make_optional<oembed>();
        if(json.contains("height") && json["height"].is_number())
        {
            embed->height = json["height"].get<int>();
        }
        if(json.contains("width") && json["width"].is_number())
        {
            embed->width = json["width"].get<int>();
        }
        if(json.contains("thumbnail_height") && json["thumbnail_height"].is_number())
        {
            embed->thumbnailHeight = json["thumbnail_height"].get<int>();
        }
        if(json.contains("thumbnail_width") && json["thumbnail_width"].is_number())
        {
            embed->thumbnailWidth = json["thumbnail_width"].get<int>();
        }
        if(json.contains("provider_url") && json["provider_url"].is_string())
        {
            embed->providerUrl = json["provider_url"].get<std::string>();
        }
        if(json.contains("html") && json["html"].is_string())
        {
            embed->html = json["html"].get<std::string>();
        }
        if(json.contains("thumbnail_url") && json["thumbnail_url"].is_string())
        {
            embed->thumbnailUrl = json["thumbnail_url"].get<std::string>();
        }
        if(json.contains("title") && json["title"].is_string())
        {
            embed->title = json["title"].get<std::string>();
        }
        if(json.contains("provider_name") && json["provider_name"].is_string())
        {
            embed->providerName = json["provider_name"].get<std::string>();
        }
        if(json.contains("type") && json["type"].is_string())
        {
            embed->type = json["type"].get<std::string>();
        }

        return embed;
    }
    std::optional<media> makeMediaObject(const nlohmann::json& json)
    {
        std::optional<media> m;
        if(json.is_null() || !json.is_object()) return m;
        m = media();
        if(json.contains("reddit_video"))
        {
            m->redditVideo = makeRedditMediaObject(json["reddit_video"]);
        }
        if(json.contains("oembed"))
        {
            m->oemEmbed = makeOemEmbedObject(json["oembed"]);
        }
        if(json.contains("type") && json["type"].is_string())
        {
            m->type = json["type"].get<std::string>();
        }
        return m;
    }
}

post::post(const nlohmann::json& json)
{
    title = json["title"].get<std::string>();
    id = json["id"].get<std::string>();
    name = json["name"].get<std::string>();
    if(json.contains("is_gallery") && json["is_gallery"].is_boolean())
    {
        isGallery = json["is_gallery"].get<bool>();
    }
    if(json.contains("gallery_data") && json["gallery_data"].is_object())
    {
        const auto& galleryDataJson = json["gallery_data"];
        if(galleryDataJson.contains("items") && galleryDataJson["items"].is_array())
        {
            for(const auto& item : galleryDataJson["items"])
            {
                post_gallery_item pi;
                if(item.contains("id") && item["id"].is_number_unsigned())
                {
                    pi.id = item["id"].get<uint64_t>();
                }
                if(item.contains("media_id") && item["media_id"].is_string())
                {
                    pi.mediaId = item["media_id"].get<std::string>();
                }
                gallery.push_back(pi);
            }
        }

        if(json.contains("media_metadata") && json["media_metadata"].is_object())
        {
            const auto& mediaMetadataJson = json["media_metadata"];
            for(auto&& galImage : gallery)
            {
                if(mediaMetadataJson.contains(galImage.mediaId) &&
                   mediaMetadataJson[galImage.mediaId].is_object())
                {
                    const auto& mediaMetadataObjJson = mediaMetadataJson[galImage.mediaId];
                    if(mediaMetadataObjJson.contains("p") &&
                       mediaMetadataObjJson["p"].is_array() &&
                       !mediaMetadataObjJson["p"].empty())
                    {
                        const auto& lastPreviewItem = mediaMetadataObjJson["p"].back();
                        if(lastPreviewItem.contains("u") && lastPreviewItem["u"].is_string())
                        {
                            galImage.url = lastPreviewItem["u"].get<std::string>();
                        }
                    }
                }
            }
        }
    }
    if(json.contains("over_18") && json["over_18"].is_boolean())
    {
        over18 = json["over_18"].get<bool>();
    }
    if(json.contains("selftext") && json["selftext"].is_string())
    {
        selfText = json["selftext"].get<std::string>();
    }
    if(json.contains("ups") && json["ups"].is_number())
    {
        ups = json["ups"].get<int>();
    }
    if(json.contains("downs") && json["downs"].is_number())
    {
        downs = json["downs"].get<int>();
    }
    if(json.contains("is_video") && json["is_video"].is_boolean())
    {
        isVideo = json["is_video"].get<bool>();
    }
    if(json.contains("is_self") && json["is_self"].is_boolean())
    {
        isSelf = json["is_self"].get<bool>();
    }
    if(json.contains("is_meta") && json["is_meta"].is_boolean())
    {
        isMeta = json["is_meta"].get<bool>();
    }
    if(json.contains("is_original_content") && json["is_original_content"].is_boolean())
    {
        isOriginalContent = json["is_original_content"].get<bool>();
    }
    if(json.contains("is_reddit_media_domain") && json["is_reddit_media_domain"].is_boolean())
    {
        isRedditMediaDomain = json["is_reddit_media_domain"].get<bool>();
    }
    if(json.contains("is_crosspostable") && json["is_crosspostable"].is_boolean())
    {
        isCrossPostable = json["is_crosspostable"].get<bool>();
    }
    if(json.contains("thumbnail") && json["thumbnail"].is_string())
    {
        thumbnail = json["thumbnail"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json.contains("num_comments") && json["num_comments"].is_number())
    {
        commentsCount = json["num_comments"].get<int>();
        commentsText = fmt::format(reinterpret_cast<const char*>(ICON_FA_COMMENTS " {} comments##{}"),commentsCount,id);
    }
    if(json.contains("subreddit_name_prefixed") && json["subreddit_name_prefixed"].is_string())
    {
        subreddit = json["subreddit_name_prefixed"].get<std::string>();
    }
    if(json.contains("subreddit") && json["subreddit"].is_string())
    {
        subredditName = json["subreddit"].get<std::string>();
    }
    if(json.contains("subreddit_id") && json["subreddit_id"].is_string())
    {
        subredditId = json["subreddit_id"].get<std::string>();
    }
    if(json.contains("score") && json["score"].is_number())
    {
        score = json["score"].get<int>();
        humanScore = Utils::getHumanReadableNumber(score);
    }
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("url_overridden_by_dest") && json["url_overridden_by_dest"].is_string())
    {
        urlOverridenByDest = json["url_overridden_by_dest"].get<std::string>();
    }
    if(json.contains("author_fullname") && json["author_fullname"].is_string())
    {
        authorFullName = json["author_fullname"].get<std::string>();
    }
    if(json.contains("author") && json["author"].is_string())
    {
        author = json["author"].get<std::string>();
    }
    if(json.contains("domain") && json["domain"].is_string())
    {
        domain = json["domain"].get<std::string>();
    }
    if(json.contains("post_hint") && json["post_hint"].is_string())
    {
        postHint = json["post_hint"].get<std::string>();
    }
    if(json.contains("preview") &&
            json["preview"].is_object() &&
            json["preview"]["images"].is_array())
    {
        for(const auto& img : json["preview"]["images"])
        {
            images_preview preview;
            if(img["source"].is_object())
            {
                preview.source.url = img["source"]["url"].get<std::string>();
                preview.source.width = img["source"]["width"].get<int>();
                preview.source.height = img["source"]["height"].get<int>();
            }
            if(img["resolutions"].is_array())
            {
                for(const auto& res : img["resolutions"]) {
                    image_target img_target;
                    img_target.url = res["url"].get<std::string>();
                    img_target.width = res["width"].get<int>();
                    img_target.height = res["height"].get<int>();
                    preview.resolutions.push_back(img_target);
                }
            }

            previews.push_back(preview);
        }
    }

    if(json.contains("media"))
    {
        postMedia = makeMediaObject(json["media"]);
    }
    if(json.contains("secure_media"))
    {
        postMedia = makeMediaObject(json["secure_media"]);
    }

    if(!postMedia && json.contains("crosspost_parent_list"))
    {
        for(const auto& parent : json["crosspost_parent_list"])
        {
            if(parent.contains("media"))
            {
                postMedia = makeMediaObject(parent["media"]);
            }
            if(parent.contains("secure_media"))
            {
                postMedia = makeMediaObject(parent["secure_media"]);
            }
            if(postMedia) break;
        }
    }

    //if(isVideo || (postHint != "self" && postHint!="link" && !postHint.empty()))
    {
        std::cout << title <<", is_video:"<<isVideo<<", hint:"<<postHint<<", url:"<<url<<std::endl;
    }
}

comment::comment(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("body") && json["body"].is_string())
    {
        body =json["body"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json["score"].is_number())
    {
        score = json["score"].get<int>();
        humanScore = Utils::getHumanReadableNumber(score);
    }
    if(json.contains("author_fullname") && json["author_fullname"].is_string())
    {
        authorFullName = json["author_fullname"].get<std::string>();
    }
    if(json["author"].is_string())
    {
        author = json["author"].get<std::string>();
    }
    if(json["ups"].is_number())
    {
        ups = json["ups"].get<int>();
    }
    if(json["downs"].is_number())
    {
        downs = json["downs"].get<int>();
    }

    if(json.contains("replies") && json["replies"].is_object())
    {
        const auto& replies_json = json["replies"];
        if(!replies_json.contains("kind") || replies_json["kind"].get<std::string>() != "Listing")
        {
            return;
        }
        if(!replies_json.contains("data") || !replies_json["data"].contains("children"))
        {
            return;
        }
        const auto& children = replies_json["data"]["children"];
        if(!children.is_array()) return;
        for(const auto& child: children)
        {
            if(child.contains("kind") && child["kind"].get<std::string>() == "more")
            {
                hasMoreReplies = true;
            }
            else if(child.contains("kind") && child["kind"].get<std::string>() == "t1" &&
                    child.contains("data") && child["data"].is_object())
            {
                replies.emplace_back(child["data"]);
            }
        }
    }
}

user_info::user_info(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
    }
    if(json.contains("oauth_client_id") && json["oauth_client_id"].is_string())
    {
        oauthClientId =json["oauth_client_id"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json.contains("inbox_count") && json["inbox_count"].is_number())
    {
        inboxCount = json["inbox_count"].get<int>();
    }
    if(json.contains("comment_karma") && json["comment_karma"].is_number())
    {
        commentKarma = json["comment_karma"].get<int64_t>();
        humanCommentKarma = Utils::getHumanReadableNumber(commentKarma);
    }
    if(json.contains("total_karma") && json["total_karma"].is_number())
    {
        totalKarma = json["total_karma"].get<int64_t>();
        humanTotalKarma = Utils::getHumanReadableNumber(totalKarma);
    }
    if(json.contains("link_karma") && json["link_karma"].is_number())
    {
        linkKarma = json["link_karma"].get<int64_t>();
        humanLinkKarma = Utils::getHumanReadableNumber(linkKarma);
    }
    if(json.contains("has_mail") && json["has_mail"].is_boolean())
    {
        hasMail = json["has_mail"].get<bool>();
    }
    if(json.contains("has_mod_mail") && json["has_mod_mail"].is_boolean())
    {
        hasModMail = json["has_mod_mail"].get<bool>();
    }
    if(json.contains("gold_creddits") && json["gold_creddits"].is_number())
    {
        goldCreddits = json["gold_creddits"].get<int64_t>();
    }
    if(json.contains("is_suspended") && json["is_suspended"].is_boolean())
    {
        isSuspended = json["is_suspended"].get<bool>();
    }
    if(json.contains("is_mod") && json["is_mod"].is_boolean())
    {
        isMod = json["is_mod"].get<bool>();
    }
    if(json.contains("is_gold") && json["is_gold"].is_boolean())
    {
        isGold = json["is_gold"].get<bool>();
    }
    if(json.contains("coins") && json["coins"].is_number())
    {
        coins = json["coins"].get<int64_t>();
    }

}
subreddit::subreddit(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description =json["description"].get<std::string>();
    }
    if(json.contains("display_name") && json["display_name"].is_string())
    {
        displayName =json["display_name"].get<std::string>();
    }
    if(json.contains("display_name_prefixed") && json["display_name_prefixed"].is_string())
    {
        displayNamePrefixed =json["display_name_prefixed"].get<std::string>();
    }
    if(json.contains("title") && json["title"].is_string())
    {
        title =json["title"].get<std::string>();
    }
    if(json.contains("subscribers") && json["subscribers"].is_number())
    {
        subscribers = json["subscribers"].get<int64_t>();
    }
    if(json.contains("over18") && json["over18"].is_boolean())
    {
        over18 = json["over18"].get<bool>();
    }
    if(json.contains("user_is_banned") && json["user_is_banned"].is_boolean())
    {
        userIsBanned = json["user_is_banned"].get<bool>();
    }
    if(json.contains("user_is_moderator") && json["user_is_moderator"].is_boolean())
    {
        userIsModerator = json["user_is_moderator"].get<bool>();
    }
    if(json.contains("user_is_muted") && json["user_is_muted"].is_boolean())
    {
        userIsMuted = json["user_is_muted"].get<bool>();
    }
    if(json.contains("user_is_subscriber") && json["user_is_subscriber"].is_boolean())
    {
        userIsSubscriber = json["user_is_subscriber"].get<bool>();
    }

}

multireddit::multireddit(const nlohmann::json& json)
{
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
    }
    if(json.contains("owner") && json["owner"].is_string())
    {
        owner =json["owner"].get<std::string>();
    }
    if(json.contains("owner_id") && json["owner_id"].is_string())
    {
        ownerId =json["owner_id"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json.contains("num_subscribers") && json["num_subscribers"].is_number())
    {
        subscribers = json["num_subscribers"].get<int64_t>();
    }
    if(json.contains("description_md") && json["description_md"].is_string())
    {
        description =json["description_md"].get<std::string>();
    }
    if(json.contains("display_name") && json["display_name"].is_string())
    {
        displayName =json["display_name"].get<std::string>();
    }
    if(json.contains("over18") && json["over18"].is_boolean())
    {
        over18 = json["over18"].get<bool>();
    }
    if(json.contains("can_edit") && json["can_edit"].is_boolean())
    {
        canEdit = json["can_edit"].get<bool>();
    }
    if(json.contains("path") && json["path"].is_string())
    {
        path =json["path"].get<std::string>();
    }
}

message::message(const nlohmann::json& json)
{
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
    }
    if(json.contains("body") && json["body"].is_string())
    {
        body =json["body"].get<std::string>();
    }
    if(json.contains("was_comment") && json["was_comment"].is_boolean())
    {
        wasComment = json["was_comment"].get<bool>();
    }
    if(json.contains("first_message") && json["first_message"].is_string())
    {
        firstMessage =json["first_message"].get<std::string>();
    }
    if(json.contains("first_message_name") && json["first_message_name"].is_string())
    {
        firstMessageName =json["first_message_name"].get<std::string>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json.contains("author") && json["author"].is_string())
    {
        author =json["author"].get<std::string>();
    }
    if(json.contains("subreddit") && json["subreddit"].is_string())
    {
        subreddit =json["subreddit"].get<std::string>();
    }
    if(json.contains("parent_id") && json["parent_id"].is_string())
    {
        parentId =json["parent_id"].get<std::string>();
    }
    if(json.contains("context") && json["context"].is_string())
    {
        context =json["context"].get<std::string>();
    }
    if(json.contains("replies") && json["replies"].is_string())
    {
        replies =json["replies"].get<std::string>();
    }
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("new") && json["new"].is_boolean())
    {
        isNew = json["new"].get<bool>();
    }
    if(json.contains("distinguished") && json["distinguished"].is_string())
    {
        distinguished =json["distinguished"].get<std::string>();
    }
    if(json.contains("subject") && json["subject"].is_string())
    {
        subject =json["subject"].get<std::string>();
    }
}

