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

struct AVFrame;
struct AVCodecContext;
struct AVStream;
struct AVFormatContext;
struct AVPacket;
struct SwsContext;

using dummy_response = client_response<void*>;
using media_body = boost::beast::http::file_body;
class MediaStreamingConnection : public RedditConnection<
        boost::beast::http::request<boost::beast::http::empty_body>,
        boost::beast::http::response<media_body>,
        boost::signals2::signal<void(const boost::system::error_code&,
                                     const dummy_response&)>
        >
{
public:
    MediaStreamingConnection(boost::asio::io_context& context,
                             boost::asio::ssl::context& ssl_context,
                             const std::string& userAgent);
    ~MediaStreamingConnection();
    void streamMedia(const std::string& url);
    using StreamingSignal = boost::signals2::signal<void(uint8_t *data,int width, int height,int linesize)>;
    using ErrorSignal = boost::signals2::signal<void(int errorCode,const std::string&)>;
    void framesAvailableHandler(const typename StreamingSignal::slot_type& slot);
    void errorHandler(const typename ErrorSignal::slot_type& slot);
    void clearAllSlots();
protected:
    virtual void responseReceivedComplete();
    virtual void onWrite(const boost::system::error_code& ec,std::size_t bytesTransferred) override;
    virtual void onRead(const boost::system::error_code& ec,std::size_t bytesTransferred) override;

private:
    void startStreaming(const std::string& url);
    int readFrame(AVPacket* pkt, AVCodecContext *video_dec_ctx,
                  AVFrame *frame, SwsContext * img_convert_ctx,
                  int height,AVFrame* frameRGB,int64_t* ts);

private:
    std::string userAgent;
    StreamingSignal streamingSignal;
    ErrorSignal errorSignal;
    std::atomic_bool cancel;
    boost::beast::http::response_parser<media_body> parser;
    std::string targetFile;
};

#endif // MEDIASTREAMINGCONNECTION_H
