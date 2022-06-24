#include "post.h"
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include "postgalleryitem.h"
#include "award.h"
#include "image.h"
#include "media.h"
#include "flairrichtext.h"
#include <tuple>
#include <algorithm>
#include <QtAlgorithms>

Post::Post(QObject *parent)
    : QObject{parent}
{

}
Post::Post(const QJsonObject& json, QObject *parent) : QObject{parent}
{
    title = json["title"].toString();
    id = json["id"].toString();
    name = json["name"].toString();
    if(json.contains("allow_live_comments") && json["allow_live_comments"].isBool())
    {
        allow_live_comments = json["allow_live_comments"].toBool();
    }
    if(json.contains("is_gallery") && json["is_gallery"].isBool())
    {
        isGallery = json["is_gallery"].toBool();
    }
    if(json.contains("gallery_data") && json["gallery_data"].isObject())
    {
        const auto& galleryDataJson = json["gallery_data"].toObject();
        if(galleryDataJson.contains("items") && galleryDataJson["items"].isArray())
        {
            for(const auto& itemValue : galleryDataJson["items"].toArray())
            {
                gallery.append(new PostGalleryItem(itemValue.toObject(),this));
            }
        }

        if(json.contains("media_metadata") && json["media_metadata"].isObject())
        {
            const auto& mediaMetadataJson = json["media_metadata"].toObject();
            for(auto&& galImage : gallery)
            {
                auto mediaId = galImage->getMediaId();
                if(mediaMetadataJson.contains(mediaId) &&
                   mediaMetadataJson[mediaId].isObject())
                {
                    const auto& mediaMetadataObjJson = mediaMetadataJson[mediaId].toObject();
                    if(mediaMetadataObjJson.contains("p") &&
                       mediaMetadataObjJson["p"].isArray())
                    {
                        const auto& lastPreviewItem = mediaMetadataObjJson["p"].toArray().last().toObject();
                        if(lastPreviewItem.contains("u") && lastPreviewItem["u"].isString())
                        {
                            galImage->setUrl(lastPreviewItem["u"].toString());
                        }
                    }
                }
            }
        }
    }
    if(json.contains("over_18") && json["over_18"].isBool())
    {
        over18 = json["over_18"].toBool();
    }
    if(json.contains("selftext") && json["selftext"].isString())
    {
        selfText = json["selftext"].toString();
    }
    if(json.contains("ups") && json["ups"].isDouble())
    {
        ups = json["ups"].toDouble();
    }
    if(json.contains("downs") && json["downs"].isDouble())
    {
        downs = json["downs"].toDouble();
    }
    if(json.contains("is_video") && json["is_video"].isBool())
    {
        isVideo = json["is_video"].toBool();
    }
    if(json.contains("is_self") && json["is_self"].isBool())
    {
        isSelf = json["is_self"].toBool();
    }
    if(json.contains("is_meta") && json["is_meta"].isBool())
    {
        isMeta = json["is_meta"].toBool();
    }
    if(json.contains("is_original_content") && json["is_original_content"].isBool())
    {
        isOriginalContent = json["is_original_content"].toBool();
    }
    if(json.contains("is_reddit_media_domain") && json["is_reddit_media_domain"].isBool())
    {
        isRedditMediaDomain = json["is_reddit_media_domain"].toBool();
    }
    if(json.contains("is_crosspostable") && json["is_crosspostable"].isBool())
    {
        isCrossPostable = json["is_crosspostable"].toBool();
    }
    if(json.contains("thumbnail") && json["thumbnail"].isString())
    {
        thumbnail = json["thumbnail"].toString();
    }
    if(json.contains("created_utc") && json["created_utc"].isDouble())
    {
        createdAt = json["created_utc"].toDouble();
        humanReadableTimeDifference = QString::fromStdString(Utils::getHumanReadableTimeAgo(createdAt));
    }
    if(json.contains("num_comments") && json["num_comments"].isDouble())
    {
        commentsCount = json["num_comments"].toDouble();
        //commentsText = fmt::format("{} {} comments",reinterpret_cast<const char*>(ICON_FA_COMMENTS), commentsCount);
        commentsText = QString("%1 comments").arg(commentsCount);
    }
    if(json.contains("subreddit_name_prefixed") && json["subreddit_name_prefixed"].isString())
    {
        subreddit = json["subreddit_name_prefixed"].toString();
    }
    if(json.contains("subreddit") && json["subreddit"].isString())
    {
        subredditName = json["subreddit"].toString();
    }
    if(json.contains("subreddit_id") && json["subreddit_id"].isString())
    {
        subredditId = json["subreddit_id"].toString();
    }
    if(json.contains("score") && json["score"].isDouble())
    {
        score = json["score"].toDouble();
        humanScore = QString::fromStdString(Utils::getHumanReadableNumber(score));
    }
    if(json.contains("url") && json["url"].isString())
    {
        url = json["url"].toString();
    }
    if(json.contains("url_overridden_by_dest") && json["url_overridden_by_dest"].isString())
    {
        urlOverridenByDest = json["url_overridden_by_dest"].toString();
    }
    if(json.contains("author_fullname") && json["author_fullname"].isString())
    {
        authorFullName = json["author_fullname"].toString();
    }
    if(json.contains("author") && json["author"].isString())
    {
        author = json["author"].toString();
    }
    if(json.contains("domain") && json["domain"].isString())
    {
        domain = json["domain"].toString();
    }
    if(json.contains("post_hint") && json["post_hint"].isString())
    {
        postHint = json["post_hint"].toString();
    }
    if(json.contains("preview") &&
            json["preview"].isObject() &&
            json["preview"]["images"].isArray())
    {
        for(const auto& imgValue : json["preview"]["images"].toArray())
        {
            previews.append(new ImagePreview(imgValue.toObject(),this));
        }
    }

    if(json.contains("media") && json["media"].isObject())
    {
        postMedia = new Media(json["media"].toObject(),this);
    }
    if(json.contains("secure_media") && json["secure_media"].isObject())
    {
        postMedia = new Media(json["secure_media"].toObject(),this);
    }

    if(!postMedia && json.contains("crosspost_parent_list") && json["crosspost_parent_list"].isArray())
    {
        for(const auto& parentValue : json["crosspost_parent_list"].toArray())
        {
            const auto& parent = parentValue.toObject();
            if(parent.contains("media") && parent["media"].isObject())
            {
                postMedia = new Media(parent["media"].toObject(),this);
            }
            if(parent.contains("secure_media") && parent["secure_media"].isObject())
            {
                postMedia = new Media(parent["secure_media"].toObject(),this);
            }
            if(postMedia) break;
        }
    }
    if(json.contains("locked") && json["locked"].isBool())
    {
        locked = json["locked"].toBool();
    }
    if(json.contains("clicked") && json["clicked"].isBool())
    {
        clicked = json["clicked"].toBool();
    }
    if(json.contains("visited") && json["visited"].isBool())
    {
        visited = json["visited"].toBool();
    }
    if(json.contains("likes") && !json["likes"].isNull() &&
            json["likes"].isBool())
    {
        auto likes = json["likes"].toBool();
        voted = likes ? Voted::UpVoted : Voted::DownVoted;
    }
    if(json.contains("gilded") && json["gilded"].isDouble())
    {
        gilded = json["gilded"].toDouble();
    }
    if(json.contains("total_awards_received") && json["total_awards_received"].isDouble())
    {
        totalAwardsReceived = json["total_awards_received"].toDouble();
    }
    if(json.contains("all_awardings") && json["all_awardings"].isArray())
    {
        for(const auto& postAward : json["all_awardings"].toArray())
        {
            allAwardings.append(new Award(postAward.toObject(),this));
        }
    }
    std::sort(allAwardings.begin(),allAwardings.end(),[](const Award* a1,const Award* a2){
        return std::forward_as_tuple(a1->getCoinPrice(),a1->getCount()) >
                std::forward_as_tuple(a2->getCoinPrice(),a2->getCount());
    });

    if(json.contains("gildings") && json["gildings"].isObject())
    {
        auto gilding = json["gildings"].toObject();
        for(const auto& key : gilding.keys())
        {
            gildings[key] = gilding.value(key).toDouble();
        }
    }
    if(json.contains("link_flair_type") && json["link_flair_type"].isString())
    {
        linkFlairType = json["link_flair_type"].toString();
    }
    if(json.contains("link_flair_text") && json["link_flair_text"].isString())
    {
        linkFlairText = json["link_flair_text"].toString();
    }
    if(json.contains("link_flair_template_id") && json["link_flair_template_id"].isString())
    {
        linkFlairTemplateId = json["link_flair_template_id"].toString();
    }
    if(json.contains("link_flair_richtext") && json["link_flair_richtext"].isArray())
    {
        for(const auto& linkFlair : json["link_flair_richtext"].toArray())
        {
            linkFlairsRichText.append(new FlairRichText( linkFlair.toObject(),this));
        }
    }
    if(json.contains("link_flair_background_color") && json["link_flair_background_color"].isString())
    {
        linkFlairBackgroundColor = json["link_flair_background_color"].toString();
    }
    if(json.contains("link_flair_css_class") && json["link_flair_css_class"].isString())
    {
        linkFlairCSSClass = json["link_flair_css_class"].toString();
    }
    if(json.contains("link_flair_text_color") && json["link_flair_text_color"].isString())
    {
        linkFlairTextColor = json["link_flair_text_color"].toString();
    }
}
Post::~Post()
{
    qDeleteAll(gallery);
    qDeleteAll(previews);
    qDeleteAll(allAwardings);
    qDeleteAll(linkFlairsRichText);
}
