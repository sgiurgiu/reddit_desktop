#include "entities.h"
#include <fmt/format.h>
#include "utils.h"
#include <spdlog/spdlog.h>
#include "fonts/IconsFontAwesome4.h"

std::string make_user_agent(const user& u)
{
    return fmt::format("{}:{}:v{} (by /u/{})",u.website,u.app_name,REDDITDESKTOP_VERSION,u.username);
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
image_target::image_target(const nlohmann::json& json)
{
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("width") && json["width"].is_number())
    {
        width = json["width"].get<int>();
    }
    if(json.contains("height") && json["height"].is_number())
    {
        height = json["height"].get<int>();
    }
}
award::award(const nlohmann::json& json)
{
    id = json["id"].get<std::string>();
    name = json["name"].get<std::string>();
    icon.url = json["icon_url"].get<std::string>();
    icon.width = json["icon_width"].get<int>();
    icon.height = json["icon_height"].get<int>();
    staticIcon.url = json["static_icon_url"].get<std::string>();
    staticIcon.width = json["static_icon_width"].get<int>();
    staticIcon.height = json["static_icon_height"].get<int>();

    if(json.contains("is_new") && json["is_new"].is_boolean())
    {
        isNew = json["is_new"].get<bool>();
    }
    if(json.contains("days_of_premium") && json["days_of_premium"].is_number())
    {
        daysOfPremium = json["days_of_premium"].get<int>();
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description = json["description"].get<std::string>();
    }
    if(json.contains("count") && json["count"].is_number())
    {
        count = json["count"].get<int>();
    }
    if(json.contains("coin_price") && json["coin_price"].is_number())
    {
        coinPrice = json["coin_price"].get<int>();
    }
    if(json.contains("award_sub_type") && json["award_sub_type"].is_string())
    {
        awardSubType = json["award_sub_type"].get<std::string>();
    }
    if(json.contains("award_type") && json["award_type"].is_string())
    {
        awardType = json["award_type"].get<std::string>();
    }
    if(json.contains("resized_icons") && json["resized_icons"].is_array())
    {
        for(const auto& ri : json["resized_icons"])
        {
            resizedIcons.emplace_back(ri);
        }
    }
    if(json.contains("resized_static_icons") && json["resized_static_icons"].is_array())
    {
        for(const auto& ri : json["resized_static_icons"])
        {
            resizedStaticIcons.emplace_back(ri);
        }
    }
    const auto images_less = [](const image_target& i1,const image_target& i2){
        return std::tie(i1.width, i1.height) < std::tie(i2.width, i2.height);
    };
    std::sort(resizedIcons.begin(),resizedIcons.end(),images_less);
    std::sort(resizedStaticIcons.begin(),resizedStaticIcons.end(),images_less);
}

post::post(const nlohmann::json& json)
{
    title = json["title"].get<std::string>();
    id = json["id"].get<std::string>();
    name = json["name"].get<std::string>();
    if(json.contains("allow_live_comments") && json["allow_live_comments"].is_boolean())
    {
        allow_live_comments = json["allow_live_comments"].get<bool>();
    }
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
        commentsText = fmt::format("{} {} comments",reinterpret_cast<const char*>(ICON_FA_COMMENTS), commentsCount);
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
                preview.source = image_target(img["source"]);
            }
            if(img["resolutions"].is_array())
            {
                for(const auto& res : img["resolutions"]) {
                    preview.resolutions.emplace_back(res);
                }
            }
            if (img.contains("variants") && img["variants"].is_object())
            {
                for (auto it = img["variants"].begin(); it != img["variants"].end();++it)
                {
                    preview_variant variant;
                    variant.kind = it.key();
                    if (it.value().contains("source") && it.value()["source"].is_object())
                    {
                        variant.source = image_target(it.value()["source"]);
                    }
                    if (it.value().contains("resolutions") && it.value()["resolutions"].is_array())
                    {
                        for (const auto& res : it.value()["resolutions"]) {
                            variant.resolutions.emplace_back(res);
                        }
                    }
                    preview.variants.push_back(variant);
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
    if(json.contains("locked") && json["locked"].is_boolean())
    {
        locked = json["locked"].get<bool>();
    }
    if(json.contains("clicked") && json["clicked"].is_boolean())
    {
        clicked = json["clicked"].get<bool>();
    }
    if(json.contains("visited") && json["visited"].is_boolean())
    {
        visited = json["visited"].get<bool>();
    }
    if(json.contains("likes") && !json["likes"].is_null() &&
            json["likes"].is_boolean())
    {
        auto likes = json["likes"].get<bool>();
        voted = likes ? Voted::UpVoted : Voted::DownVoted;
    }
    if(json.contains("gilded") && json["gilded"].is_number())
    {
        gilded = json["gilded"].get<int>();
    }
    if(json.contains("total_awards_received") && json["total_awards_received"].is_number())
    {
        totalAwardsReceived = json["total_awards_received"].get<int>();
    }
    if(json.contains("all_awardings") && json["all_awardings"].is_array())
    {
        for(const auto& postAward : json["all_awardings"])
        {
            allAwardings.emplace_back(postAward);
        }
    }
    std::sort(allAwardings.begin(),allAwardings.end(),[](const award& a1,const award& a2){
        return std::tie(a1.coinPrice,a1.count) > std::tie(a2.coinPrice,a2.count);
    });
    if(json.contains("gildings"))
    {
        for(const auto& gilding : json["gildings"].items())
        {
            gildings[gilding.key()] = gilding.value().get<int>();
        }
    }
    if(json.contains("link_flair_type") && json["link_flair_type"].is_string())
    {
        linkFlairType = json["link_flair_type"].get<std::string>();
    }
    if(json.contains("link_flair_text") && json["link_flair_text"].is_string())
    {
        linkFlairText = json["link_flair_text"].get<std::string>();
    }
    if(json.contains("link_flair_template_id") && json["link_flair_template_id"].is_string())
    {
        linkFlairTemplateId = json["link_flair_template_id"].get<std::string>();
    }
    if(json.contains("link_flair_richtext") && json["link_flair_richtext"].is_array())
    {
        for(const auto& linkFlair : json["link_flair_richtext"])
        {
            linkFlairsRichText.emplace_back(linkFlair);
        }
    }
    if(json.contains("link_flair_background_color") && json["link_flair_background_color"].is_string())
    {
        linkFlairBackgroundColor = json["link_flair_background_color"].get<std::string>();
    }
    if(json.contains("link_flair_css_class") && json["link_flair_css_class"].is_string())
    {
        linkFlairCSSClass = json["link_flair_css_class"].get<std::string>();
    }
    if(json.contains("link_flair_text_color") && json["link_flair_text_color"].is_string())
    {
        linkFlairTextColor = json["link_flair_text_color"].get<std::string>();
    }

    //if(isVideo || (postHint != "self" && postHint!="link" && !postHint.empty()))
    {
        spdlog::debug("{}, is_video: {}, hint: {}, url: {}",title,isVideo,postHint,url);
    }
}

flair_richtext::flair_richtext(const nlohmann::json& json)
{
    if(json.contains("a") && json["a"].is_string())
    {
        a = json["a"].get<std::string>();
    }
    if(json.contains("e") && json["e"].is_string())
    {
        e = json["e"].get<std::string>();
    }
    if(json.contains("t") && json["t"].is_string())
    {
        t = json["t"].get<std::string>();
    }
    if(json.contains("u") && json["u"].is_string())
    {
        u = json["u"].get<std::string>();
    }

}

comment::comment(const nlohmann::json& json, const user& currentUser)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
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
    if(json.contains("author") && json["author"].is_string())
    {
        author = json["author"].get<std::string>();
        isUsersComment = (author == currentUser.username);
    }
    if(json.contains("link_id") && json["link_id"].is_string())
    {
        linkId = json["link_id"].get<std::string>();
    }
    if(json["ups"].is_number())
    {
        ups = json["ups"].get<int>();
    }
    if(json["downs"].is_number())
    {
        downs = json["downs"].get<int>();
    }
    if(json.contains("parent_id") && json["parent_id"].is_string())
    {
        parentId =json["parent_id"].get<std::string>();
    }

    if(json.contains("likes") && !json["likes"].is_null() &&
            json["likes"].is_boolean())
    {
        auto likes = json["likes"].get<bool>();
        voted = likes ? Voted::UpVoted : Voted::DownVoted;
    }
    if(json.contains("is_submitter") && json["is_submitter"].is_boolean())
    {
        isSubmitter = json["is_submitter"].get<bool>();
    }
    if (json.contains("edited") && json["edited"].is_boolean())
    {
        edited = json["edited"].get<bool>();
    }
    if (json.contains("locked") && json["locked"].is_boolean())
    {
        locked = json["locked"].get<bool>();
    }
    if (json.contains("removed") && json["removed"].is_boolean())
    {
        removed = json["removed"].get<bool>();
    }
    if(json.contains("gilded") && json["gilded"].is_number())
    {
        gilded = json["gilded"].get<int>();
    }
    if(json.contains("total_awards_received") && json["total_awards_received"].is_number())
    {
        totalAwardsReceived = json["total_awards_received"].get<int>();
    }
    if(json.contains("all_awardings") && json["all_awardings"].is_array())
    {
        for(const auto& postAward : json["all_awardings"])
        {
            allAwardings.emplace_back(postAward);
        }
    }
    std::sort(allAwardings.begin(),allAwardings.end(),[](const award& a1,const award& a2){
        return std::tie(a1.coinPrice,a1.count) > std::tie(a2.coinPrice,a2.count);
    });
    if(json.contains("gildings"))
    {
        for(const auto& gilding : json["gildings"].items())
        {
            gildings[gilding.key()] = gilding.value().get<int>();
        }
    }

    if(json.contains("author_flair_type") && json["author_flair_type"].is_string())
    {
        authorFlairType = json["author_flair_type"].get<std::string>();
    }
    if(json.contains("author_flair_text") && json["author_flair_text"].is_string())
    {
        authorFlairText = json["author_flair_text"].get<std::string>();
    }
    if(json.contains("author_flair_template_id") && json["author_flair_template_id"].is_string())
    {
        authorFlairTemplateId = json["author_flair_template_id"].get<std::string>();
    }
    if(json.contains("author_flair_richtext") && json["author_flair_richtext"].is_array())
    {
        for(const auto& linkFlair : json["author_flair_richtext"])
        {
            authorFlairsRichText.emplace_back(linkFlair);
        }
    }
    if(json.contains("author_flair_background_color") && json["author_flair_background_color"].is_string())
    {
        authorFlairBackgroundColor = json["author_flair_background_color"].get<std::string>();
    }
    if(json.contains("author_flair_css_class") && json["author_flair_css_class"].is_string())
    {
        authorFlairCSSClass = json["author_flair_css_class"].get<std::string>();
    }
    if(json.contains("author_flair_text_color") && json["author_flair_text_color"].is_string())
    {
        authorFlairTextColor = json["author_flair_text_color"].get<std::string>();
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
            if(child.contains("kind") && child["kind"].get<std::string>() == "more" &&
               child.contains("data") && child["data"].is_object())
            {
                unloadedChildren = std::make_optional<unloaded_children>(child["data"]);
                if(unloadedChildren->count <= 0)
                {
                    unloadedChildren.reset();
                }
            }
            else if(child.contains("kind") && child["kind"].get<std::string>() == "t1" &&
                    child.contains("data") && child["data"].is_object())
            {
                replies.emplace_back(child["data"], currentUser);
            }
        }
    }
}

unloaded_children::unloaded_children(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id =json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name =json["name"].get<std::string>();
    }
    if(json.contains("parent_id") && json["parent_id"].is_string())
    {
        parentId =json["parent_id"].get<std::string>();
    }
    if(json.contains("count") && json["count"].is_number())
    {
        count = json["count"].get<int>();
    }
    if(json.contains("depth") && json["depth"].is_number())
    {
        depth = json["depth"].get<int>();
    }
    if(json.contains("children") && json["children"].is_array())
    {
        for(const auto& child:json["children"])
        {
            children.emplace_back(child.get<std::string>());
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
    if(json.contains("submit_text") && json["submit_text"].is_string())
    {
        submitText = json["submit_text"].get<std::string>();
    }
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("icon_img") && json["icon_img"].is_string())
    {
        iconImage = json["icon_img"].get<std::string>();
    }
    if(json.contains("header_img") && json["header_img"].is_string())
    {
        headerImage = json["header_img"].get<std::string>();
    }
    if(json.contains("subscribers") && json["subscribers"].is_number())
    {
        subscribers = json["subscribers"].get<int64_t>();
    }
    if(json.contains("over18") && json["over18"].is_boolean())
    {
        over18 = json["over18"].get<bool>();
    }
    if(json.contains("quarantine") && json["quarantine"].is_boolean())
    {
        quarantine = json["quarantine"].get<bool>();
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
    if(json.contains("header_size") && json["header_size"].is_array() &&
           json["header_size"].size() == 2 )
    {
        headerSize.first = json["header_size"][0].get<int>();
        headerSize.second = json["header_size"][1].get<int>();
    }
    if(json.contains("icon_size") && json["icon_size"].is_array() &&
           json["icon_size"].size() == 2 )
    {
        iconSize.first = json["icon_size"][0].get<int>();
        iconSize.second = json["icon_size"][1].get<int>();
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

message::message(const nlohmann::json& json, const std::string& kind):kind(kind)
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
    if(json.contains("score") && json["score"].is_number())
    {
        score = json["score"].get<int>();
        humanScore = Utils::getHumanReadableNumber(score);
    }
}

subreddit_about::subreddit_about(const nlohmann::json& json)
{

    if(json.contains("banner_background_color") && json["banner_background_color"].is_string())
    {
        bannerBackgroundColor = json["banner_background_color"].get<std::string>();
    }
    if(json.contains("banner_background_image") && json["banner_background_image"].is_string())
    {
        bannerBackgroundImage = json["banner_background_image"].get<std::string>();
    }
    if(json.contains("banner_img") && json["banner_img"].is_string())
    {
        bannerImage = json["banner_img"].get<std::string>();
    }
    if(json.contains("banner_size") && json["banner_size"].is_array())
    {
        bannerSize.first = json["banner_size"][0].get<int>();
        bannerSize.second = json["banner_size"][1].get<int>();
    }
    if(json.contains("community_icon") && json["community_icon"].is_string())
    {
        communityIcon = json["community_icon"].get<std::string>();
    }
    if(json.contains("header_img") && json["header_img"].is_string())
    {
        headerImage = json["header_img"].get<std::string>();
    }
    if(json.contains("header_size") && json["header_size"].is_array())
    {
        headerSize.first = json["header_size"][0].get<int>();
        headerSize.second = json["header_size"][1].get<int>();
    }
    if(json.contains("header_title") && json["header_title"].is_string())
    {
        headerTitle = json["header_title"].get<std::string>();
    }
    if(json.contains("icon_img") && json["icon_img"].is_string())
    {
        iconImage = json["icon_img"].get<std::string>();
    }
    if(json.contains("icon_size") && json["icon_size"].is_array())
    {
        iconSize.first = json["icon_size"][0].get<int>();
        iconSize.second = json["icon_size"][1].get<int>();
    }
    if(json.contains("key_color") && json["key_color"].is_string())
    {
        keyColor = json["key_color"].get<std::string>();
    }
    if(json.contains("mobile_banner_image") && json["mobile_banner_image"].is_string())
    {
        mobileBannerImage = json["mobile_banner_image"].get<std::string>();
    }
    if(json.contains("primary_color") && json["primary_color"].is_string())
    {
        primaryColor = json["primary_color"].get<std::string>();
    }
    if(json.contains("public_description") && json["public_description"].is_string())
    {
        publicDescription = json["public_description"].get<std::string>();
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description = json["description"].get<std::string>();
    }
}
subreddit_stylesheet::subreddit_stylesheet(const nlohmann::json& json)
{
    if(json.contains("subreddit_id") && json["subreddit_id"].is_string())
    {
        subredditId = json["subreddit_id"].get<std::string>();
    }
    if(json.contains("stylesheet") && json["stylesheet"].is_string())
    {
        stylesheet = json["stylesheet"].get<std::string>();
    }
    if(json.contains("images") && json["images"].is_array())
    {
        images.reserve(json["images"].size());
        std::ranges::transform(json["images"],std::back_inserter(images),
                [](const auto& jsonImg){
            return stylesheet_image(jsonImg);
        });
    }
}
stylesheet_image::stylesheet_image(const nlohmann::json& json)
{
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("link") && json["link"].is_string())
    {
        link = json["link"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name = json["name"].get<std::string>();
    }
}
live_update_event_about::live_update_event_about(const nlohmann::json& json)
{
    if(json.contains("websocket_url") && json["websocket_url"].is_string())
    {
        websocket_url = json["websocket_url"].get<std::string>();
    }
    if(json.contains("announcement_url") && json["announcement_url"].is_string())
    {
        announcement_url = json["announcement_url"].get<std::string>();
    }
    if(json.contains("button_cta") && json["button_cta"].is_string())
    {
        button_cta = json["button_cta"].get<std::string>();
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description = json["description"].get<std::string>();
    }
    if(json.contains("icon") && json["icon"].is_string())
    {
        icon = json["icon"].get<std::string>();
    }
    if(json.contains("id") && json["id"].is_string())
    {
        id = json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name = json["name"].get<std::string>();
    }
    if(json.contains("resources") && json["resources"].is_string())
    {
        resources = json["resources"].get<std::string>();
    }
    if(json.contains("state") && json["state"].is_string())
    {
        state = json["state"].get<std::string>();
    }
    if(json.contains("title") && json["title"].is_string())
    {
        title = json["title"].get<std::string>();
    }
    if(json.contains("created") && json["created"].is_number_float())
    {
        created = json["created"].get<double>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number_float())
    {
        created_utc = json["created_utc"].get<double>();
    }
    if(json.contains("is_announcement") && json["is_announcement"].is_boolean())
    {
        is_announcement = json["is_announcement"].get<bool>();
    }
    if(json.contains("nsfw") && json["nsfw"].is_boolean())
    {
        nsfw = json["nsfw"].get<bool>();
    }
    if(json.contains("viewer_count_fuzzed") && json["viewer_count_fuzzed"].is_boolean())
    {
        viewer_count_fuzzed = json["viewer_count_fuzzed"].get<bool>();
    }
    if(json.contains("num_times_dismissable") && json["num_times_dismissable"].is_number())
    {
        num_times_dismissable = json["num_times_dismissable"].get<int>();
    }
    if(json.contains("total_views") && json["total_views"].is_number())
    {
        total_views = json["total_views"].get<int>();
    }
    if(json.contains("viewer_count") && json["viewer_count"].is_number())
    {
        viewer_count = json["viewer_count"].get<int>();
    }
}

live_update_event::live_update_event(const nlohmann::json& json)
{
    if(json.contains("body") && json["body"].is_string())
    {
        body = json["body"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name = json["name"].get<std::string>();
    }
    if(json.contains("author") && json["author"].is_string())
    {
        author = json["author"].get<std::string>();
    }
    if(json.contains("id") && json["id"].is_string())
    {
        id = json["id"].get<std::string>();
    }

    if(json.contains("created") && json["created"].is_number())
    {
        created = json["created"].get<int64_t>();
    }
    if(json.contains("created_utc") && json["created_utc"].is_number())
    {
        created_utc = json["created_utc"].get<int64_t>();
    }
    if(json.contains("stricken") && json["stricken"].is_boolean())
    {
        stricken = json["stricken"].get<bool>();
    }
    if(json.contains("mobile_embeds") && json["mobile_embeds"].is_array())
    {
        for(const auto& embed:json["mobile_embeds"])
        {
            embeds.emplace_back(embed);
        }
    }
}
live_update_event_embed::live_update_event_embed(const nlohmann::json& json)
{
    if(json.contains("provider_url") && json["provider_url"].is_string())
    {
        provider_url = json["provider_url"].get<std::string>();
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description = json["description"].get<std::string>();
    }
    if(json.contains("original_url") && json["original_url"].is_string())
    {
        original_url = json["original_url"].get<std::string>();
    }
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("title") && json["title"].is_string())
    {
        title = json["title"].get<std::string>();
    }
    if(json.contains("thumbnail_url") && json["thumbnail_url"].is_string())
    {
        thumbnail_url = json["thumbnail_url"].get<std::string>();
    }
    if(json.contains("author_name") && json["author_name"].is_string())
    {
        author_name = json["author_name"].get<std::string>();
    }
    if(json.contains("version") && json["version"].is_string())
    {
        version = json["version"].get<std::string>();
    }
    if(json.contains("provider_name") && json["provider_name"].is_string())
    {
        provider_name = json["provider_name"].get<std::string>();
    }
    if(json.contains("type") && json["type"].is_string())
    {
        type = json["type"].get<std::string>();
    }
    if(json.contains("author_url") && json["author_url"].is_string())
    {
        author_url = json["author_url"].get<std::string>();
    }
    if(json.contains("cache_age") && json["cache_age"].is_number())
    {
        cache_age = json["cache_age"].get<int64_t>();
    }
    if(json.contains("thumbnail_width") && json["thumbnail_width"].is_number())
    {
        thumbnail_width = json["thumbnail_width"].get<int>();
    }
    if(json.contains("thumbnail_height") && json["thumbnail_height"].is_number())
    {
        thumbnail_height = json["thumbnail_height"].get<int>();
    }
    if(json.contains("width") && json["width"].is_number())
    {
        width = json["width"].get<int>();
    }
}
tweet_public_metrics::tweet_public_metrics(const nlohmann::json& json)
{
    if(json.contains("retweet_count") && json["retweet_count"].is_number())
    {
        retweet_count = json["retweet_count"].get<int>();
    }
    if(json.contains("reply_count") && json["reply_count"].is_number())
    {
        reply_count = json["reply_count"].get<int>();
    }
    if(json.contains("like_count") && json["like_count"].is_number())
    {
        like_count = json["like_count"].get<int>();
    }
    if(json.contains("quote_count") && json["quote_count"].is_number())
    {
        quote_count = json["quote_count"].get<int>();
    }
}
tweet_attachments::tweet_attachments(const nlohmann::json& json)
{
    if(json.contains("media_keys") && json["media_keys"].is_array())
    {
        for(const auto& key : json["media_keys"])
        {
            media_keys.emplace_back(key.get<std::string>());
        }
    }
}
tweet_entity::tweet_entity(const nlohmann::json& json)
{
    if(json.contains("start") && json["start"].is_number())
    {
        start = json["start"].get<int>();
    }
    if(json.contains("end") && json["end"].is_number())
    {
        end = json["end"].get<int>();
    }
}
tweet_entity_url::tweet_entity_url(const nlohmann::json& json):tweet_entity(json)
{
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("expanded_url") && json["expanded_url"].is_string())
    {
        expanded_url = json["expanded_url"].get<std::string>();
    }
    if(json.contains("display_url") && json["display_url"].is_string())
    {
        display_url = json["display_url"].get<std::string>();
    }
    if(json.contains("images") && json["images"].is_array())
    {
        for(const auto& image:json["images"])
        {
            images.emplace_back(image);
        }
    }
    if(json.contains("status") && json["status"].is_number())
    {
        status = json["status"].get<int>();
    }
    if(json.contains("title") && json["title"].is_string())
    {
        title = json["title"].get<std::string>();
    }
    if(json.contains("description") && json["description"].is_string())
    {
        description = json["description"].get<std::string>();
    }
    if(json.contains("unwound_url") && json["unwound_url"].is_string())
    {
        unwound_url = json["unwound_url"].get<std::string>();
    }
}
std::string tweet_entity_url::GetMarkdown() const
{
    std::string display = display_url;
    if(display.empty())
    {
        display = expanded_url;
    }
    if(display.empty())
    {
        display = url;
    }
    return "["+display+"]("+(expanded_url.empty() ? url : expanded_url)+")";
}
tweet_entity_annotation::tweet_entity_annotation(const nlohmann::json& json):tweet_entity(json)
{
    if(json.contains("probability") && json["probability"].is_number())
    {
        probability = json["probability"].get<double>();
    }
    if(json.contains("type") && json["type"].is_string())
    {
        type = json["type"].get<std::string>();
    }
    if(json.contains("normalized_text") && json["normalized_text"].is_string())
    {
        normalized_text = json["normalized_text"].get<std::string>();
    }
}
std::string tweet_entity_annotation::GetMarkdown() const
{
    return normalized_text;
}
tweet_entity_hashtag::tweet_entity_hashtag(const nlohmann::json& json):tweet_entity(json)
{
    if(json.contains("tag") && json["tag"].is_string())
    {
        tag = json["tag"].get<std::string>();
    }
}
std::string tweet_entity_hashtag::GetMarkdown() const
{
    return "[#"+tag+"](https://twitter.com/hashtag/"+tag+")";
}

tweet_entities::tweet_entities(const nlohmann::json& json)
{
    if(json.contains("urls") && json["urls"].is_array())
    {
        for(const auto& url:json["urls"])
        {
            urls.emplace_back(url);
        }
    }
    if(json.contains("annotations") && json["annotations"].is_array())
    {
        for(const auto& annotation:json["annotations"])
        {
            annotations.emplace_back(annotation);
        }
    }
    if(json.contains("hashtags") && json["hashtags"].is_array())
    {
        for(const auto& hashtag:json["hashtags"])
        {
            hashtags.emplace_back(hashtag);
        }
    }
}
tweet_media::tweet_media(const nlohmann::json& json)
{
    if(json.contains("width") && json["width"].is_number())
    {
        width = json["width"].get<int>();
    }
    if(json.contains("height") && json["height"].is_number())
    {
        height = json["height"].get<int>();
    }
    if(json.contains("type") && json["type"].is_string())
    {
        type = json["type"].get<std::string>();
    }
    if(json.contains("preview_image_url") && json["preview_image_url"].is_string())
    {
        preview_image_url = json["preview_image_url"].get<std::string>();
    }
    if(json.contains("url") && json["url"].is_string())
    {
        url = json["url"].get<std::string>();
    }
    if(json.contains("media_key") && json["media_key"].is_string())
    {
        media_key = json["media_key"].get<std::string>();
    }
}
tweet_user::tweet_user(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id = json["id"].get<std::string>();
    }
    if(json.contains("name") && json["name"].is_string())
    {
        name = json["name"].get<std::string>();
    }
    if(json.contains("username") && json["username"].is_string())
    {
        username = json["username"].get<std::string>();
    }
    if(json.contains("verified") && json["verified"].is_boolean())
    {
        verified = json["verified"].get<bool>();
    }
}

tweet_includes::tweet_includes(const nlohmann::json& json)
{
    if(json.contains("media") && json["media"].is_array())
    {
        for(const auto& m:json["media"])
        {
            media.emplace_back(m);
        }
    }
    if(json.contains("users") && json["users"].is_array())
    {
        for(const auto& m:json["users"])
        {
            users.emplace_back(m);
        }
    }
    if(json.contains("tweets") && json["tweets"].is_array())
    {
        for(const auto& t:json["tweets"])
        {
            auto tw = std::make_shared<tweet>();
            tw->load(t);
            tweets.push_back(std::move(tw));
        }
    }
}
tweet_referenced_tweet::tweet_referenced_tweet(const nlohmann::json& json)
{
    if(json.contains("id") && json["id"].is_string())
    {
        id = json["id"].get<std::string>();
    }
    if(json.contains("type") && json["type"].is_string())
    {
        type = json["type"].get<std::string>();
    }
}
void tweet::load(const nlohmann::json& data)
{
    if(data.contains("possibly_sensitive") && data["possibly_sensitive"].is_boolean())
    {
        possibly_sensitive = data["possibly_sensitive"].get<bool>();
    }
    if(data.contains("id") && data["id"].is_string())
    {
        id = data["id"].get<std::string>();
    }
    if(data.contains("created_at") && data["created_at"].is_string())
    {
        created_at = data["created_at"].get<std::string>();
    }
    if(data.contains("text") && data["text"].is_string())
    {
        text = data["text"].get<std::string>();
    }
    if(data.contains("author_id") && data["author_id"].is_string())
    {
        author_id = data["author_id"].get<std::string>();
    }
    if(data.contains("lang") && data["lang"].is_string())
    {
        lang = data["lang"].get<std::string>();
    }
    if(data.contains("public_metrics") && data["public_metrics"].is_object())
    {
        public_metrics = tweet_public_metrics{data["public_metrics"]};
    }
    if(data.contains("attachments") && data["attachments"].is_object())
    {
        attachments = tweet_attachments{data["attachments"]};
    }
    if(data.contains("entities") && data["entities"].is_object())
    {
        entities = tweet_entities{data["entities"]};
    }
    if(data.contains("referenced_tweets") && data["referenced_tweets"].is_array())
    {
        for(const auto& t : data["referenced_tweets"])
        {
            referenced_tweets.emplace_back(t);
        }
    }
}
tweet::tweet(const nlohmann::json& json)
{
    if(json.contains("data") && json["data"].is_object())
    {
        load(json["data"]);
    }
    if(json.contains("includes") && json["includes"].is_object())
    {
        includes = tweet_includes{json["includes"]};
    }
}
