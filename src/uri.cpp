#include "uri.h"
#include <uriparser/Uri.h>
#define MAKE_URI_TEXT_RANGE_STRING(TextRange) std::string(TextRange.first,TextRange.afterLast)

Uri::Uri(const std::string& uri):uri_(uri)
{
    UriUriA uriUri;
    auto uriResult = uriParseSingleUriA(&uriUri, uri.c_str(), nullptr);
    isValid_ = uriResult == URI_SUCCESS;
    if(!isValid_) return;
    scheme_ = MAKE_URI_TEXT_RANGE_STRING(uriUri.scheme);
    host_ = MAKE_URI_TEXT_RANGE_STRING(uriUri.hostText);
    query_ = MAKE_URI_TEXT_RANGE_STRING(uriUri.query);
    port_ = MAKE_URI_TEXT_RANGE_STRING(uriUri.portText);
    fragment_ = MAKE_URI_TEXT_RANGE_STRING(uriUri.fragment);
    auto path = uriUri.pathHead;
    while(path)
    {
        path_.push_back(MAKE_URI_TEXT_RANGE_STRING(path->text));
        path = path->next;
    }

    uriFreeUriMembersA(&uriUri);
}

std::string Uri::fullPath(const std::string& delim) const
{
    std::string fullPath;
    for(const auto& p : path_)
    {
        fullPath.append(delim+p);
    }
    return fullPath;
}
