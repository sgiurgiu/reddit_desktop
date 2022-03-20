#ifndef HTMLPARSER_H
#define HTMLPARSER_H

#include <filesystem>
#include <string>
#include <vector>

class HtmlParser
{
public:
    enum class MediaType
    {
        Unknown = 0,
        Video,
        Image,
        Gif,
        Gallery
    };
    struct MediaLink
    {
        std::vector<std::string> urls;
        MediaType type;
        bool useLink = false;
    };

    explicit HtmlParser(const std::filesystem::path& file);
    explicit HtmlParser(const std::string& contents);
    MediaLink getMediaLink(const std::string& domain) const;
    static std::string unescape(const std::string &input);
    static std::string escape(const std::string &input);
private:
    template<class Node>
    std::string lookupMetaOgVideoUrl(Node* node, const std::string& metaProperty) const;
    template<class Node>
    std::string lookupVideoSourceVideoUrl(Node* node) const;
    template<class Node>
    std::string lookupDivPlayerContainerVideoUrl(Node* node) const;
    template<class Node>
    std::string lookupYoutubeVideoUrl(Node* node) const;


private:
    std::string contents;
};

#endif // HTMLPARSER_H
