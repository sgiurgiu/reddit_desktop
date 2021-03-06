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

class RedditClientProducer;

using dummy_response = client_response<void*>;
class UrlDetectionConnection : public RedditGetConnection<dummy_response>
{
public:
    UrlDetectionConnection(const boost::asio::any_io_executor& executor,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& userAgent,
                           RedditClientProducer* client);
    ~UrlDetectionConnection();
    void detectMediaUrl(post* mediaPost);
    using StreamingSignal = boost::signals2::signal<void(HtmlParser::MediaLink)>;
    using ErrorSignal = boost::signals2::signal<void(int errorCode,const std::string&)>;
    template<typename S>
    void streamAvailableHandler(S slot)
    {
        streamingSignal.connect(std::move(slot));
    }
    template<typename S>
    void errorHandler(S slot)
    {
        errorSignal.connect(std::move(slot));
    }
    void clearAllSlots();
protected:
    virtual void responseReceivedComplete() override;
    virtual void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred) override;
    virtual void onError(const boost::system::error_code& ec) override;

private:
    void urlDetected(const HtmlParser::MediaLink& link);
    void downloadUrl(const std::string& url);
private:
    std::string userAgent;
    StreamingSignal streamingSignal;
    ErrorSignal errorSignal;
    std::atomic_bool cancel;
    bool downloadingHtml = false;
    post* currentPost;
    std::string currentUrl;
    RedditClientProducer* client;
};

#endif // MEDIASTREAMINGCONNECTION_H
