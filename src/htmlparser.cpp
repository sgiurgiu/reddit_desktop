#include "htmlparser.h"
#include "gumbo.h"

#include <fstream>
#include <cstring>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "json.hpp"
#include <boost/url.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_uint.hpp>

/**
 * This class looks for video links in HTML
 */

namespace bsq = boost::spirit::qi;
namespace bk = boost::spirit::karma;

namespace {
bsq::int_parser<unsigned char, 16, 2, 2> hex_byte;
template <typename InputIterator>
struct unescaped_string
    : bsq::grammar<InputIterator, std::string(char const *)> {
  unescaped_string() : unescaped_string::base_type(unesc_str) {
    unesc_char.add("+", ' ');

    unesc_str = *(unesc_char | "%" >> hex_byte | bsq::char_);
  }

  bsq::rule<InputIterator, std::string(char const *)> unesc_str;
  bsq::symbols<char const, char const> unesc_char;
};

template <typename OutputIterator>
struct escaped_string : bk::grammar<OutputIterator, std::string(char const *)> {
  escaped_string() : escaped_string::base_type(esc_str) {

    esc_str = *(bk::char_("a-zA-Z0-9_.~-") | "%" << bk::right_align(2,0)[bk::hex]);
  }
  bk::rule<OutputIterator, std::string(char const *)> esc_str;
};
}

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
                            if(!f.contains("url") && !f.contains("signatureCipher")) return true;
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
                        if(desiredFormat.contains("url"))
                        {
                            return desiredFormat["url"].get<std::string>();
                        }
                        else if (desiredFormat.contains("signatureCipher"))
                        {
                            auto sigCihper = desiredFormat["signatureCipher"].get<std::string>();
                            std::string tmpUrl("http://example.com/?"+sigCihper);
                            boost::url_view urlParts(tmpUrl);
                            auto params = urlParts.params();
                            for(const auto& p : params)
                            {
                                if(p.encoded_key() == "url")
                                {
                                    auto url = p.encoded_value().to_string();
                                    return HtmlParser::unescape(url);
                                }
                            }
                        }
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

HtmlParser::MediaLink HtmlParser::getMediaLink(const std::string& domain) const
{
    auto output = gumbo_parse_with_options(&kGumboDefaultOptions,contents.c_str(),contents.size());

    MediaLink link;
    link.type = MediaType::Video;
    if(domain.find("youtube") != domain.npos || domain == "youtu.be")
    {
        link.url = this->template lookupYoutubeVideoUrl<GumboNode>(output->root);
    }
    else if(domain == "streamable.com")
    {
        link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video:url");
    }
    else if (domain == "streamja.com" || domain == "streamvi.com" || domain == "streamwo.com")
    {
        link.url = this->template lookupVideoSourceVideoUrl<GumboNode>(output->root);
    }
    else if (domain == "clippituser.tv" || domain == "www.clippituser.tv")
    {
        link.url = this->template lookupDivPlayerContainerVideoUrl<GumboNode>(output->root);
    }
    else if(domain.find("gfycat") != domain.npos ||
            domain.find("redgifs") != domain.npos)
    {
        link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video");
        if(link.url.empty())
        {
            link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:url");
        }
        if(link.url.empty())
        {
            link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:image");
            link.type = MediaType::Image;
        }
    }
    else if(domain.find("imgur") != domain.npos)
    {
        link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video");

        if(link.url.empty())
        {
            link.url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:image");
            link.type = MediaType::Image;
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions,output);
    if(link.url.empty())
    {
        link.type = MediaType::Unknown;
    }
    return link;
}

std::string HtmlParser::unescape(const std::string &input)
{
  std::string retVal;
  retVal.reserve(input.size());
  typedef std::string::const_iterator iterator_type;

  char const *start = "";
  iterator_type beg = input.begin();
  iterator_type end = input.end();
  unescaped_string<iterator_type> p;

  if (!bsq::parse(beg, end, p(start), retVal))
    retVal = input;
  return retVal;
}

std::string HtmlParser::escape(const std::string &input)
{
  typedef std::back_insert_iterator<std::string> sink_type;
  std::string retVal;
  retVal.reserve(input.size() * 3);
  sink_type sink(retVal);
  char const *start = "";

  escaped_string<sink_type> g;
  if (!bk::generate(sink, g(start), input))
    retVal = input;
  return retVal;
}
