#include "mediastreamingconnection.h"
#include <boost/asio.hpp>
#include <boost/url.hpp>
#include <fstream>
#include <filesystem>

#include "htmlparser.h"
#include "macros.h"

MediaStreamingConnection::MediaStreamingConnection(const boost::asio::any_io_executor& executor,
                                                   boost::asio::ssl::context& ssl_context,
                                                   const std::string& userAgent):
    RedditConnection(executor,ssl_context,"",""),userAgent(userAgent),cancel(false)
{
    responseParser->body_limit(10*1024*1024);//10 MB html file limit should be plenty
}

MediaStreamingConnection::~MediaStreamingConnection()
{
    clearAllSlots();
    cancel = true;
}
void MediaStreamingConnection::clearAllSlots()
{
    streamingSignal.disconnect_all_slots();
    errorSignal.disconnect_all_slots();
}

void MediaStreamingConnection::onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if(ec)
    {
        onError(ec);
        return;
    }
    boost::system::error_code error;
    if(isSsl)
    {
        boost::beast::http::read_header(stream.value(),buffer,responseParser.value(),error);
    }
    else
    {
        boost::beast::http::read_header(stream.value().next_layer(),buffer,responseParser.value(),error);
    }
    if(error)
    {
        onError(error);
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
    auto readMethod = std::bind(&MediaStreamingConnection::onRead,this->shared_from_base<MediaStreamingConnection>(),_1,_2);
    if(isSsl)
    {
        boost::beast::http::async_read(stream.value(), buffer, responseParser.value(), readMethod);
    }
    else
    {
        boost::beast::http::async_read(stream.value().next_layer(), buffer, responseParser.value(), readMethod);
    }
}

void MediaStreamingConnection::onError(const boost::system::error_code& ec)
{
    errorSignal(ec.value(),ec.message());
}

void MediaStreamingConnection::responseReceivedComplete()
{
    if(downloadingHtml)
    {
        HtmlParser htmlParser(responseParser->get().body());
        auto videoUrl = htmlParser.getMediaLink(currentPost->domain);
        //boost::system::error_code ec;
        //stream->shutdown(ec);
        if(videoUrl.type == HtmlParser::MediaType::Unknown)
        {
            errorSignal(-1,"No video URL found in post");
            return;
        }
        downloadingHtml = false;
        //std::filesystem::remove(targetPath);
        //stream.emplace(boost::asio::make_strand(context), ssl_context);
        startStreaming(videoUrl);
    }
    else
    {
        startStreaming({currentUrl,HtmlParser::MediaType::Unknown});
        //BOOST_ASSERT(false);
    }
}

void MediaStreamingConnection::streamMedia(post* mediaPost)
{
    currentPost = mediaPost;
    currentUrl = currentPost->url;
    if(currentPost->postMedia && currentPost->postMedia->redditVideo)
    {
        currentUrl = currentPost->postMedia->redditVideo->dashUrl;
        HtmlParser::MediaLink link{currentUrl,HtmlParser::MediaType::Video};
        boost::asio::post(strand,std::bind(
                               &MediaStreamingConnection::startStreaming,
                               this->shared_from_base<MediaStreamingConnection>(),link));
    }
    else
    {
        //'https://www.youtube.com/watch?v=BaW_jenozKc&gl=US&hl=en&has_verified=1&bpctr=9999999999'

        downloadUrl(currentUrl);
    }
}

void MediaStreamingConnection::downloadUrl(const std::string& url)
{
    boost::url_view urlParts(url);
    if(!urlParts.port().empty() || !urlParts.scheme().empty())
    {
        service = urlParts.port().empty() ? urlParts.scheme().to_string() : urlParts.port().to_string();
    }
    if(!urlParts.host().empty())
    {
        host = urlParts.encoded_host().to_string();
    }
    auto target = urlParts.encoded_path().to_string();
    auto query = urlParts.encoded_query().to_string();
    if(!query.empty())
    {
        target+="?"+urlParts.encoded_query().to_string();
    }
    if(host == "youtube.com" || host == "www.youtube.com" ||  host == "youtu.be")
    {
        if(!query.empty()) target+="&gl=US&hl=en&has_verified=1&bpctr=9999999999";
        else target+="?gl=US&hl=en&has_verified=1&bpctr=9999999999";
    }
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.prepare_payload();
    buffer.consume(buffer.size());
    buffer.clear();
    responseParser.emplace();
    responseParser->body_limit(std::numeric_limits<uint64_t>::max());
    if(connected)
    {
        sendRequest();
    }
    else
    {
        resolveHost();
    }
}

void MediaStreamingConnection::startStreaming(const HtmlParser::MediaLink& link)
{   
    streamingSignal(link);
}

void MediaStreamingConnection::streamAvailableHandler(const typename StreamingSignal::slot_type& slot)
{
    streamingSignal.connect(slot);
}
void MediaStreamingConnection::errorHandler(const typename ErrorSignal::slot_type& slot)
{
    errorSignal.connect(slot);
}
