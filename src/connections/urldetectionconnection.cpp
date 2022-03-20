#include "urldetectionconnection.h"
#include <boost/asio.hpp>
#include <fstream>
#include <filesystem>
#include "uri.h"
#include "htmlparser.h"
#include "macros.h"

UrlDetectionConnection::UrlDetectionConnection(const boost::asio::any_io_executor& executor,
                                                   boost::asio::ssl::context& ssl_context,
                                                   const std::string& userAgent):
    RedditConnection(executor,ssl_context,"",""),userAgent(userAgent),cancel(false)
{
    responseParser->body_limit(10*1024*1024);//10 MB html file limit should be plenty
}

UrlDetectionConnection::~UrlDetectionConnection()
{
    clearAllSlots();
    cancel = true;
}
void UrlDetectionConnection::clearAllSlots()
{
    streamingSignal.disconnect_all_slots();
    errorSignal.disconnect_all_slots();
}

void UrlDetectionConnection::onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if(ec)
    {
        onError(ec);
        return;
    }
    responseParser->body_limit(10*1024*1024);
    boost::system::error_code error;
    if(isSsl)
    {
        boost::beast::http::read_header(stream.value(),readBuffer,responseParser.value(),error);
    }
    else
    {
        boost::beast::http::read_header(stream.value().next_layer(),readBuffer,responseParser.value(),error);
    }
    if(error)
    {
        onError(error);
        return;
    }
    auto status = responseParser->get().result_int();
    if(status >= 400)
    {
        errorSignal(-1,"Not found");
        return;
    }
    std::string contentType;
    std::string location;
    for(const auto& h : responseParser->get())
    {
        if(h.name() == boost::beast::http::field::content_type)
        {
            contentType = h.value().to_string();
        }
        if(h.name() == boost::beast::http::field::location)
        {
            location = h.value().to_string();
        }
    }

    if(!location.empty())
    {
        auto status = responseParser->get().result();

        if(status == boost::beast::http::status::moved_permanently ||
           status == boost::beast::http::status::temporary_redirect ||
           status == boost::beast::http::status::permanent_redirect ||
           status == boost::beast::http::status::see_other ||
           status == boost::beast::http::status::found)
        {
            connected = false;
            stream.emplace(strand, ssl_context);
            currentUrl = location;
            downloadUrl(currentUrl);
            return;
        }
    }

    if(contentType.empty())
    {
        //dunno what we're reading here
    }
    else
    {
        if(contentType.find("html") != contentType.npos)
        {
            downloadingHtml = true;
        }
    }

    if(error)
    {
        onError(error);
        return;
    }

    using namespace std::placeholders;
    auto readMethod = std::bind(&UrlDetectionConnection::onRead,this->shared_from_base<UrlDetectionConnection>(),_1,_2);
    if(isSsl)
    {
        boost::beast::http::async_read(stream.value(), readBuffer, responseParser.value(), readMethod);
    }
    else
    {
        boost::beast::http::async_read(stream.value().next_layer(), readBuffer, responseParser.value(), readMethod);
    }
}

void UrlDetectionConnection::onError(const boost::system::error_code& ec)
{
    errorSignal(ec.value(),ec.message());
}

void UrlDetectionConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    if(status >= 400)
    {
        errorSignal(-1,"Cannot retrieve media");
        return;
    }
    if(downloadingHtml)
    {
        HtmlParser htmlParser(responseParser->get().body());
        auto videoUrl = htmlParser.getMediaLink(currentPost->domain);
        //boost::system::error_code ec;
        //stream->shutdown(ec);
//        if(videoUrl.type == HtmlParser::MediaType::Unknown)
//        {
//            errorSignal(-1,"No video URL found in post");
//            return;
//        }
        downloadingHtml = false;
        //std::filesystem::remove(targetPath);
        //stream.emplace(boost::asio::make_strand(context), ssl_context);
        urlDetected(videoUrl);
    }
    else
    {
        urlDetected({{currentUrl},HtmlParser::MediaType::Unknown});
        //BOOST_ASSERT(false);
    }
}

void UrlDetectionConnection::detectMediaUrl(post* mediaPost)
{
    currentPost = mediaPost;
    currentUrl = currentPost->url;
    if(currentPost->postMedia && currentPost->postMedia->redditVideo)
    {
        currentUrl = currentPost->postMedia->redditVideo->dashUrl;
        HtmlParser::MediaLink link{{currentUrl},HtmlParser::MediaType::Video, true};
        boost::asio::post(strand,std::bind(
                               &UrlDetectionConnection::urlDetected,
                               this->shared_from_base<UrlDetectionConnection>(),link));
    }
    else
    {
        downloadUrl(currentUrl);
    }
}

void UrlDetectionConnection::downloadUrl(const std::string& url)
{
    Uri urlParts(url);
    if(!urlParts.port().empty() || !urlParts.scheme().empty())
    {
        service = urlParts.port().empty() ? urlParts.scheme() : urlParts.port();
    }
    if(!urlParts.host().empty())
    {
        host = urlParts.host();
    }
    auto target = urlParts.fullPath();
    auto query = urlParts.query();
    if(!query.empty())
    {
        target+="?"+urlParts.query();
    }
    if(host.find("youtube") != host.npos ||  host == "youtu.be")
    {
        if(!query.empty()) target+="&gl=US&hl=en&has_verified=1&bpctr=9999999999";
        else target+="?gl=US&hl=en&has_verified=1&bpctr=9999999999";
    }
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.prepare_payload();
    readBuffer.consume(readBuffer.size());
    readBuffer.clear();
    responseParser.emplace();
    responseParser->body_limit(std::numeric_limits<uint64_t>::max());
    performRequest(std::move(request));
}

void UrlDetectionConnection::urlDetected(const HtmlParser::MediaLink& link)
{   
    streamingSignal(link);
}

