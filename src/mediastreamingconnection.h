#ifndef MEDIASTREAMINGCONNECTION_H
#define MEDIASTREAMINGCONNECTION_H

#include <boost/signals2.hpp>
#include <boost/asio/io_context.hpp>
#include <string>
#include <boost/system/error_code.hpp>
#include "entities.h"
#include <memory>
#include <atomic>
#include "redditconnection.h"
#include "htmlparser.h"

using dummy_response = client_response<void*>;
class MediaStreamingConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::empty_body>,
        boost::beast::http::response<boost::beast::http::string_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                     const dummy_response&)>
        >
{
public:
    MediaStreamingConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& userAgent);
    ~MediaStreamingConnection();
    void streamMedia(post* mediaPost);
    using StreamingSignal = boost::signals2::signal<void(HtmlParser::MediaLink)>;
    using ErrorSignal = boost::signals2::signal<void(int errorCode,const std::string&)>;
    void streamAvailableHandler(const typename StreamingSignal::slot_type& slot);
    void errorHandler(const typename ErrorSignal::slot_type& slot);
    void clearAllSlots();
protected:
    virtual void responseReceivedComplete() override;
    virtual void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred) override;
    virtual void onError(const boost::system::error_code& ec) override;

private:
    void startStreaming(const HtmlParser::MediaLink& link);
    void downloadUrl(const std::string& url);
private:
    std::string userAgent;
    StreamingSignal streamingSignal;
    ErrorSignal errorSignal;
    std::atomic_bool cancel;
    bool downloadingHtml = false;
    post* currentPost;
    std::string currentUrl;
};

#endif // MEDIASTREAMINGCONNECTION_H
