#include "htmlparser.h"
#include <gumbo.h>

#include <fstream>
#include <cstring>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "json.hpp"
#include "uri.h"

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_uint.hpp>

#include <spdlog/spdlog.h>
#include "redditclientproducer.h"
#include <future>
#include "uri.h"
#include <unordered_set>

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

std::string getClientId(const std::string& jsContents)
{
    //from our look at the code, it seems that we have
    // self.AMPLITUDE_KEY assigned in the block of code that
    // has the client ID
    // THIS IS EXTREMELY HACKY AND FRAGILE AND WILL CEASE TO FUNCTION
    // AS SOON AS IMGUR UPDATES THEIR CODE
    // TODO: Experiment with V8 and maybe have more luck in more safely determine
    // the client ID. Elk is not quite that powerful.
    auto amplitudeIndex = jsContents.find("self.AMPLITUDE_KEY");
    if(amplitudeIndex != std::string::npos)
    {
        //walk back from there to `var`
        auto varIndex = jsContents.rfind("var",amplitudeIndex);
        if(varIndex != std::string::npos)
        {
            auto assignmentText = jsContents.substr(varIndex,amplitudeIndex - varIndex);
            std::vector<std::string> v;
            boost::algorithm::split(v, assignmentText, boost::algorithm::is_from_range(',',','));
            // we take the second to last item
            if(v.size() > 2)
            {
                auto interestingItem = *(v.rbegin() + 1);
                v.clear();
                boost::algorithm::split(v, interestingItem, boost::algorithm::is_from_range('"','"'));
                if(v.size() > 1)
                {
                    return *(v.rbegin() + 1);
                }
            }
        }
    }
    return "";
}

const std::unordered_set<std::string> stream_video_source_domains = {
    "streamja.com", "streamvi.com", "streamwo.com", "streamye.com",
    "streamgg.com","streamff.com"
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

HtmlParser::HtmlParser(const std::string& contents, RedditClientProducer* client):contents(contents),
    client(client)
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
                            Uri urlParts(tmpUrl);
                            auto params = urlParts.query();
                            /*
                             * TODO: Fix this with UriParser library
                             * for(const auto& p : params)
                            {
                                if(p.encoded_key() == "url")
                                {
                                    auto url = p.encoded_value().to_string();
                                    return HtmlParser::unescape(url);
                                }
                            }*/
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

template<class Node>
std::string HtmlParser::getImgurPostDataJson(Node* node) const
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
            if(text.find("window.postDataJSON") == text.npos) continue;
            auto jsonStartIndex = text.find("=");
            if(jsonStartIndex == text.npos) continue;
            ++jsonStartIndex;
            auto jsonSubstr = text.substr(jsonStartIndex);
            boost::algorithm::replace_all(jsonSubstr,"\\\"","\"");
            boost::algorithm::replace_all(jsonSubstr,"\"{","{");
            boost::algorithm::replace_all(jsonSubstr,"}\"","}");
            boost::algorithm::replace_all(jsonSubstr,"\\'","'");
            return jsonSubstr;
        }
    }


    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        auto json = getImgurPostDataJson(static_cast<GumboNode*>(children->data[i]));
        if(!json.empty()) return json;
    }
    return "";
}

template<class Node>
std::string HtmlParser::getImgurMainJsURL(Node* node) const
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return "";
    }
    if (node->v.element.tag == GUMBO_TAG_SCRIPT)
    {
        GumboVector* attributes = &node->v.element.attributes;
        for (unsigned int i = 0; i < attributes->length; ++i)
        {
            auto attribute = static_cast<GumboAttribute*>(attributes->data[i]);
            if(std::string(attribute->name) != "src") continue;
            std::string src(attribute->value);
            if(src.find("https://s.imgur.com/desktop-assets/js/main") == std::string::npos) continue;
            return src;
        }
    }


    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        auto url = getImgurMainJsURL(static_cast<GumboNode*>(children->data[i]));
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
        link.urls.emplace_back(this->template lookupYoutubeVideoUrl<GumboNode>(output->root));
    }
    else if(domain == "streamable.com")
    {
        link.urls.emplace_back(this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video:url"));
    }
    else if (stream_video_source_domains.contains(domain))
    {
        link.urls.emplace_back(this->template lookupVideoSourceVideoUrl<GumboNode>(output->root));
    }
    else if (domain == "clippituser.tv" || domain == "www.clippituser.tv")
    {
        link.urls.emplace_back(this->template lookupDivPlayerContainerVideoUrl<GumboNode>(output->root));
    }
    else if(domain.find("gfycat") != domain.npos ||
            domain.find("redgifs") != domain.npos ||
            domain.find("giphy") != domain.npos)
    {
        link.urls.emplace_back(this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video"));
        if(link.urls.empty() || link.urls.begin()->empty())
        {
            link.urls.emplace_back(this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:url"));
        }
        if(link.urls.empty() || link.urls.begin()->empty())
        {
            link.urls.emplace_back(this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:image"));
            link.type = MediaType::Image;
        }
    }
    else if(domain.find("imgur") != domain.npos)
    {

        auto videoUrl = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:video");
        if(!videoUrl.empty())
        {
            link.urls.emplace_back(std::move(videoUrl));
        }

        if(link.urls.empty() || link.urls.begin()->empty())
        {
            auto url = this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:url");
            Uri uri(url);
            auto uriPath = uri.path();
            auto jsonStr = getImgurPostDataJson<GumboNode>(output->root);
            try{
                nlohmann::json json = nlohmann::json::parse(jsonStr);
                if(json.contains("media") && json["media"].is_array())
                {
                    for(const auto& m : json["media"])
                    {
                        if(m.contains("url") && m["url"].is_string())
                        {
                            auto url = m["url"].get<std::string>();
                            if(!url.empty())
                            {
                                link.urls.emplace_back(std::move(url));
                            }
                        }
                    }
                }
            }catch(const std::exception& ex){
                spdlog::error("Error parsing galery json:{}",ex.what());
            }

            if(link.urls.empty())
            {
                auto mainJsUrl = getImgurMainJsURL<GumboNode>(output->root);
                std::promise<std::string> jsContentsPromise;
                auto future = jsContentsPromise.get_future();
                {
                    auto resourceConnection = client->makeResourceClientConnection();
                    resourceConnection->connectionCompleteHandler([&jsContentsPromise](auto ec, auto response){
                        if(ec)
                        {
                            jsContentsPromise.set_exception(std::make_exception_ptr(std::runtime_error(ec.message())));
                        }
                        else if(response.status >= 400)
                        {
                            jsContentsPromise.set_exception(std::make_exception_ptr(std::runtime_error("Cant find url")));
                        }
                        else if(response.status == 200 )
                        {
                            jsContentsPromise.set_value(std::string(reinterpret_cast<const char*>(response.data.data()),response.data.size()));
                        }
                    });
                    resourceConnection->getResource(mainJsUrl);
                }
                try
                {
                    auto mainJsUrlContents = future.get();
                    auto clientId = getClientId(mainJsUrlContents);
                    if(!clientId.empty())
                    {
                        std::promise<std::string> mainJsonContentsPromise;
                        auto mainJsonContentsFuture = mainJsonContentsPromise.get_future();
                        auto resourceConnection = client->makeResourceClientConnection();
                        resourceConnection->connectionCompleteHandler([&mainJsonContentsPromise](auto ec, auto response){
                            if(ec)
                            {
                                mainJsonContentsPromise.set_exception(std::make_exception_ptr(std::runtime_error(ec.message())));
                            }
                            else if(response.status >= 400)
                            {
                                mainJsonContentsPromise.set_exception(std::make_exception_ptr(std::runtime_error("Cant find url")));
                            }
                            else if(response.status == 200 )
                            {
                                mainJsonContentsPromise.set_value(std::string(reinterpret_cast<const char*>(response.data.data()),response.data.size()));
                            }
                        });
                        //the ID of the album is the last item in the uri path
                        auto albumID = uriPath.back();
                        resourceConnection->getResource("https://api.imgur.com/post/v1/albums/"+albumID+"?client_id="+clientId+"&include=media");
                        auto mainJsonContents = mainJsonContentsFuture.get();
                        auto json = nlohmann::json::parse(mainJsonContents);
                        if(json.contains("media") && json["media"].is_array())
                        {
                            for(const auto& m : json["media"])
                            {
                                if(m.contains("url") && m["url"].is_string() && m.contains("type") && m["type"].get<std::string>() == "image")
                                {
                                    auto url = m["url"].get<std::string>();
                                    if(!url.empty())
                                    {
                                        link.urls.emplace_back(std::move(url));
                                    }
                                }
                            }
                        }
                    }
                }
                catch(const std::exception& ex)
                {
                    spdlog::error("Error getting mainjs:{}",ex.what());
                }
            }

            if(link.urls.size() > 1)
            {
                link.type = MediaType::Gallery;                
            }
            else
            {                
                link.type = MediaType::Image;
            }
            if(link.urls.empty())
            {
                link.urls.emplace_back(this->template lookupMetaOgVideoUrl<GumboNode>(output->root,"og:image"));
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions,output);
    if(link.urls.empty())
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
