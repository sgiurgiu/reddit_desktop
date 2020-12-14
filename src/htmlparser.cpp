#include "htmlparser.h"
#include "gumbo.h"

#include <fstream>
#include <cstring>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "json.hpp"

HtmlParser::HtmlParser(const std::filesystem::path& file)
{
    std::ifstream ifs(file.generic_string(), std::ios::binary|std::ios::ate);

    if(!ifs)
        throw std::runtime_error(file.generic_string() + ": " + std::strerror(errno));

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    if(size == 0) // avoid undefined behavior
        return;

    contents.resize(size);

    if(!ifs.read(&contents[0], contents.size()))
        throw std::runtime_error(file.generic_string() + ": " + std::strerror(errno));

}

HtmlParser::HtmlParser(const std::string& contents):contents(contents)
{
}

template<class Node>
std::string HtmlParser::lookupMetaOgVideoUrl(Node* node,const std::string& metaProperty) const
{
    std::string url;
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return url;
    }
    GumboAttribute* property;
    GumboAttribute* content;
    if (node->v.element.tag == GUMBO_TAG_META &&
      (property = gumbo_get_attribute(&node->v.element.attributes, "property")))
    {
        std::string prop_value(property->value);
        boost::algorithm::to_lower(prop_value);
        if(prop_value == metaProperty &&
          (content = gumbo_get_attribute(&node->v.element.attributes, "content")))
        {
            url = content->value;
        }
    }

    if(url.empty())
    {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            url = lookupMetaOgVideoUrl(static_cast<GumboNode*>(children->data[i]),metaProperty);
            if(!url.empty()) break;
        }
    }

    return url;
}

template<class Node>
std::string HtmlParser::lookupVideoSourceVideoUrl(Node* node) const
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return "";
    }

    if (node->v.element.tag == GUMBO_TAG_VIDEO)
    {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            auto child = static_cast<GumboNode*>(children->data[i]);
            if(child->type != GUMBO_NODE_ELEMENT) continue;
            GumboAttribute* src;
            GumboAttribute* type;
            if (child->v.element.tag == GUMBO_TAG_SOURCE &&
               (src = gumbo_get_attribute(&child->v.element.attributes, "src")))
            {
                type = gumbo_get_attribute(&child->v.element.attributes, "type");
                if(type)
                {
                    std::string type_value(type->value);
                    boost::algorithm::to_lower(type_value);
                    if(type_value.find("video") != type_value.npos)
                    {
                        return src->value;
                    }
                }
                else
                {
                    //welp, no type, whatever
                    return src->value;
                }
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        auto url = lookupVideoSourceVideoUrl(static_cast<GumboNode*>(children->data[i]));
        if(!url.empty()) return url;
    }
    return "";
}

template<class Node>
std::string HtmlParser::lookupDivPlayerContainerVideoUrl(Node* node) const
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return "";
    }

    GumboAttribute* id;
    if (node->v.element.tag == GUMBO_TAG_DIV &&
        (id = gumbo_get_attribute(&node->v.element.attributes, "id")))
    {
        std::string id_value(id->value);
        boost::algorithm::to_lower(id_value);
        if(id_value == "player-container")
        {
            GumboAttribute* hd_url = gumbo_get_attribute(&node->v.element.attributes, "data-hd-file");
            GumboAttribute* sd_url = gumbo_get_attribute(&node->v.element.attributes, "data-sd-file");
            if(hd_url)
            {
                return hd_url->value;
            }
            if(sd_url)
            {
                return sd_url->value;
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        auto url = lookupDivPlayerContainerVideoUrl(static_cast<GumboNode*>(children->data[i]));
        if(!url.empty()) return url;
    }

    return "";
}

template<class Node>
std::string HtmlParser::lookupYoutubeVideoUrl(Node* node) const
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return "";
    }

    if (node->v.element.tag == GUMBO_TAG_SCRIPT)
    {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            auto child = static_cast<GumboNode*>(children->data[i]);
            if(child->type != GUMBO_NODE_TEXT) continue;
            std::string text(child->v.text.text);
            if(text.find("ytInitialPlayerResponse") == text.npos) continue;
            //we found our script, get the json from there
            auto startBracket = text.find_first_of('{');
            auto endBracket = text.find_last_of('}');
            if(startBracket == text.npos || endBracket==text.npos) continue;
            auto ourJson = text.substr(startBracket,endBracket-startBracket+1);
            try
            {
                auto jsonObject = nlohmann::json::parse(ourJson);
                if(jsonObject.contains("streamingData") && jsonObject["streamingData"].contains("formats"))
                {
                    auto formats = jsonObject["streamingData"]["formats"];
                    if(formats.is_array() && formats.size() > 0)
                    {
                        auto removeIt = std::remove_if(formats.begin(),formats.end(),[](const auto& f){
                            if(!f.contains("url")) return true;
                            if(!f.contains("mimeType")) return true;
                            auto mime = f["mimeType"].template get<std::string>();
                            if(mime.find("video") == mime.npos) return true;
                            return false;
                        });
                        formats.erase(removeIt,formats.end());
                        if(formats.empty()) continue;
                        std::stable_sort(formats.begin(),formats.end(),[](const auto& f1,const auto& f2) {
                            int f1height = -1;
                            int f2height = -1;
                            if(f1.contains("height"))
                            {
                                f1height = f1["height"].template get<int>();
                            }
                            if(f2.contains("height"))
                            {
                                f2height = f2["height"].template get<int>();
                            }
                            return f1height < f2height;
                        });
                        auto& desiredFormat =  formats[formats.size()/2];
                        return desiredFormat["url"].get<std::string>();
                    }
                }


            }
            catch(std::exception& ex)
            {
                std::cerr << "Cannot parse youtube json "<<ex.what()<<std::endl;
            }
        }
    }


    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        auto url = lookupYoutubeVideoUrl(static_cast<GumboNode*>(children->data[i]));
        if(!url.empty()) return url;
    }
    return "";
}

std::string HtmlParser::getVideoUrl(const std::string& domain) const
{
    auto output = gumbo_parse_with_options(&kGumboDefaultOptions,contents.c_str(),contents.size());

    std::string url;
    if(domain == "youtube.com" || domain == "youtu.be")
    {
        url = this->template lookupYoutubeVideoUrl<GumboNode>(output->root);
    }
    else if(domain == "streamable.com")
    {
        url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video:url");
    }
    else if (domain == "streamja.com")
    {
        url = this->template lookupVideoSourceVideoUrl<GumboNode>(output->root);
    }
    else if (domain == "clippituser.tv" || domain == "www.clippituser.tv")
    {
        url = this->template lookupDivPlayerContainerVideoUrl<GumboNode>(output->root);
    }
    else if(domain.find("imgur") != domain.npos || domain.find("gfycat") != domain.npos)
    {
        url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video");
        if(url.empty())
        {
            url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:url");
        }
        if(url.empty())
        {
            url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:image");
        }
    }


    gumbo_destroy_output(&kGumboDefaultOptions,output);
    return url;
}
