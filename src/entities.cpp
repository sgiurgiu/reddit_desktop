#include "entities.h"
#include <fmt/format.h>
#include "utils.h"
#include <iostream>

std::string make_user_agent(const user& u)
{
    return fmt::format("{}:{}:v{} (by /u/{}",u.website,u.app_name,REDDITDESKTOP_VERSION,u.username);
}
gl_image::~gl_image()
{
    if(textureId > 0)
    {
        glDeleteTextures(1,&textureId);
    }
}
post::post(const nlohmann::json& json)
{
    title = json["title"].get<std::string>();
    id = json["id"].get<std::string>();
    name = json["name"].get<std::string>();
    if(json["selftext"].is_string())
    {
        selfText = json["selftext"].get<std::string>();
    }
    if(json["ups"].is_number())
    {
        ups = json["ups"].get<int>();
    }
    if(json["downs"].is_number())
    {
        downs = json["downs"].get<int>();
    }
    if(json["is_video"].is_boolean())
    {
        isVideo = json["is_video"].get<bool>();
    }
    if(json["is_self"].is_boolean())
    {
        isSelf = json["is_self"].get<bool>();
    }
    if(json["is_meta"].is_boolean())
    {
        isMeta = json["is_meta"].get<bool>();
    }
    if(json["is_original_content"].is_boolean())
    {
        isOriginalContent = json["is_original_content"].get<bool>();
    }
    if(json["is_reddit_media_domain"].is_boolean())
    {
        isRedditMediaDomain = json["is_reddit_media_domain"].get<bool>();
    }
    if(json["is_crosspostable"].is_boolean())
    {
        isCrossPostable = json["is_crosspostable"].get<bool>();
    }
    if(json["thumbnail"].is_string())
    {
        thumbnail = json["thumbnail"].get<std::string>();
    }
    if(json["created_utc"].is_number())
    {
        createdAt = json["created_utc"].get<uint64_t>();
        humanReadableTimeDifference = Utils::getHumanReadableTimeAgo(createdAt);
    }
    if(json["num_comments"].is_number())
    {
        commentsCount = json["num_comments"].get<int>();
    }
    if(json["subreddit_name_prefixed"].is_string())
    {
        subreddit = json["subreddit_name_prefixed"].get<std::string>();
    }
    if(json["score"].is_number())
    {
        score = json["score"].get<int>();
        humanScore = Utils::getHumanReadableNumber(score);
    }
    if(json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("url_overridden_by_dest") && json["url_overridden_by_dest"].is_string())
    {
        urlOverridenByDest = json["url_overridden_by_dest"].get<std::string>();
    }
    if(json["author_fullname"].is_string())
    {
        authorFullName = json["author_fullname"].get<std::string>();
    }
    if(json["author"].is_string())
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

    std::cout << title <<", is_video:"<<isVideo<<", hint:"<<postHint<<std::endl;
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
                replies.emplace_back(std::make_shared<comment>(child["data"]));
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
