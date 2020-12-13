#include "htmlparser.h"
#include "gumbo.h"

#include <fstream>
#include <cstring>
#include <boost/algorithm/string.hpp>

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

std::string HtmlParser::getVideoUrl(const std::string& domain) const
{
    auto output = gumbo_parse_with_options(&kGumboDefaultOptions,contents.c_str(),contents.size());

    std::string url;
    if(domain == "streamable.com")
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
