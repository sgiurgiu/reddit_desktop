#ifndef HTMLPARSER_H
#define HTMLPARSER_H

#include <filesystem>
#include <string>

class HtmlParser
{
public:
    explicit HtmlParser(const std::filesystem::path& file);
    explicit HtmlParser(const std::string& contents);
    std::string getVideoUrl(const std::string& domain) const;
private:
    template<class Node>
    std::string lookupMetaOgVideoUrl(Node* node, const std::string& metaProperty) const;
    template<class Node>
    std::string lookupVideoSourceVideoUrl(Node* node) const;
    template<class Node>
    std::string lookupDivPlayerContainerVideoUrl(Node* node) const;

private:
    std::string contents;
};

#endif // HTMLPARSER_H
