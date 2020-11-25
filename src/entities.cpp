#include "entities.h"
#include <fmt/format.h>
#include "utils.h"

std::string make_user_agent(const user& u)
{
    return fmt::format("{}:{}:v1.0 (by /u/{}",u.website,u.app_name,u.username);
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
}

comment::comment(const nlohmann::json& json)
{
    if(json.contains("body") && json["body"].is_string())
    {
        body =json["body"].get<std::string>();
    }
}
