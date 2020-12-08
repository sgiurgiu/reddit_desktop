#ifndef MEDIASTREAMINGCONNECTION_H
#define MEDIASTREAMINGCONNECTION_H

#include <boost/signals2.hpp>
#include <boost/asio/io_context.hpp>
#include <string>
#include <boost/system/error_code.hpp>
#include "entities.h"
#include <memory>
#include <atomic>

struct AVFrame;
struct AVCodecContext;
struct AVStream;
struct AVFormatContext;
struct AVPacket;
struct SwsContext;

class MediaStreamingConnection : public std::enable_shared_from_this<MediaStreamingConnection>
{
public:
    MediaStreamingConnection(boost::asio::io_context& context,
                             const std::string& userAgent);
    ~MediaStreamingConnection();
    void streamMedia(const std::string& url);
    using Signal = boost::signals2::signal<void(uint8_t *data,int width, int height,int linesize)>;
    using ErrorSignal = boost::signals2::signal<void(int errorCode,const std::string&)>;
    void framesAvailableHandler(const typename Signal::slot_type& slot);
    void errorHandler(const typename ErrorSignal::slot_type& slot);
    void clearAllSlots();
private:
    void startStreaming(const std::string& url);
    int readFrame(AVPacket* pkt, AVCodecContext *video_dec_ctx,
                  AVFrame *frame, SwsContext * img_convert_ctx,
                  int height,AVFrame* frameRGB,int64_t* ts);

private:
    boost::asio::io_context& context;
    std::string userAgent;
    Signal signal;
    ErrorSignal errorSignal;
    std::atomic_bool cancel;
};

#endif // MEDIASTREAMINGCONNECTION_H
