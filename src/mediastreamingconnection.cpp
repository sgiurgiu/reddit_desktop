#include "mediastreamingconnection.h"
#include <boost/asio.hpp>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/imgutils.h>

}

namespace
{
    struct avio_context_deleter
    {
        void operator()(AVIOContext *avio_ctx)
        {
            av_free(avio_ctx->buffer);
#if LIBAVFORMAT_BUILD >= AV_VERSION_INT(57,80,100)
            avio_context_free(&avio_ctx);
#else
            av_free(avio_ctx);
#endif
        }
    };
    struct avformat_context_deleter
    {
        void operator()(AVFormatContext* ctx)
        {
            avformat_close_input(&ctx);
        }
    };
    struct avcodec_context_deleter
    {
        void operator()(AVCodecContext* ctx)
        {
            avcodec_free_context(&ctx);
        }
    };
    struct avframe_deleter
    {
        void operator()(AVFrame* frame)
        {
            av_frame_free(&frame);
        }
    };
    struct swscontext_deleter
    {
        void operator()(SwsContext* ctx)
        {
            sws_freeContext(ctx);
        }
    };
    struct avbuffer_deleter
    {
        void operator()(uint8_t* buf)
        {
            av_free(buf);
        }
    };
}

MediaStreamingConnection::MediaStreamingConnection(boost::asio::io_context& context,
                                                   const std::string& userAgent):
    context(context),userAgent(userAgent),cancel(false)
{
}

MediaStreamingConnection::~MediaStreamingConnection()
{
    clearAllSlots();
    cancel = true;
}
void MediaStreamingConnection::clearAllSlots()
{
    signal.disconnect_all_slots();
    errorSignal.disconnect_all_slots();
}
void MediaStreamingConnection::streamMedia(const std::string& url)
{
    boost::asio::post(context.get_executor(),std::bind(&MediaStreamingConnection::startStreaming,shared_from_this(),url));
}

void MediaStreamingConnection::startStreaming(const std::string& url)
{
    int video_stream_idx = -1;
    AVStream *video_stream = nullptr;

    AVPacket pkt;
    AVCodec *dec = nullptr;
    AVDictionary *general_options = nullptr;

    std::unique_ptr<AVFormatContext,avformat_context_deleter> fmt_ctx{nullptr,avformat_context_deleter{}};
    {
        AVFormatContext *fmt = nullptr;
        int ret = avformat_open_input(&fmt,"/home/sergiu/uDsJLCP.mp4"/*url.c_str()*/, nullptr, &general_options);
        if (ret < 0)
        {
            char buf[256];
            av_strerror(ret,buf,sizeof(buf));
            errorSignal(ret,buf);
            return;
        }
        fmt_ctx.reset(fmt);
    }
    int ret = avformat_find_stream_info(fmt_ctx.get(), nullptr);
    if (ret < 0)
    {
        char buf[256];
        av_strerror(ret,buf,sizeof(buf));
        errorSignal(ret,buf);
        return;
    }

    if((video_stream_idx = av_find_best_stream(fmt_ctx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) < 0)
    {
        errorSignal(-1,"Media does not contain video stream");
        return;
    }

    video_stream = fmt_ctx->streams[video_stream_idx];
    dec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if(!dec)
    {
        errorSignal(-1,"Cannot find decoder for video stream");
        return;
    }

    std::unique_ptr<AVCodecContext,avcodec_context_deleter> video_dec_ctx(avcodec_alloc_context3(dec),
                                                                          avcodec_context_deleter());
    if (!video_dec_ctx)
    {
        errorSignal(-1,"Cannot allocate decoder for video stream");
        return;
    }

    avcodec_parameters_to_context(video_dec_ctx.get(),video_stream->codecpar);

    int width = video_dec_ctx->width;
    int height = video_dec_ctx->height;
    AVPixelFormat pix_fmt = video_dec_ctx->pix_fmt;
    ret = avcodec_open2(video_dec_ctx.get(), dec, nullptr);
    if(ret < 0)
    {
        char buf[256];
        av_strerror(ret,buf,sizeof(buf));
        errorSignal(ret,buf);
        return;
    }

    std::unique_ptr<AVFrame,avframe_deleter> frame(av_frame_alloc(),avframe_deleter());
    std::unique_ptr<AVFrame,avframe_deleter> frameRGB(av_frame_alloc(),avframe_deleter());

    AVPixelFormat  pFormat = AV_PIX_FMT_RGB24;

    //av_image_alloc();
    int numBytes = av_image_get_buffer_size(pFormat, width, height,
                                        1 /*https://stackoverflow.com/questions/35678041/what-is-linesize-alignment-meaning*/);
    std::unique_ptr<uint8_t,avbuffer_deleter> buffer ((uint8_t *) av_malloc(numBytes*sizeof(uint8_t)),avbuffer_deleter());
    av_image_fill_arrays(frameRGB->data,frameRGB->linesize,buffer.get(),pFormat,
                         width, height,32);
    frameRGB->width = width;
    frameRGB->height = height;

    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    std::unique_ptr<SwsContext,swscontext_deleter> img_convert_ctx(
                sws_getContext(width, height, pix_fmt, width,
                                     height, pFormat, SWS_BICUBIC,
                                     nullptr, nullptr,nullptr),swscontext_deleter());
    int counter = 0;
    int64_t lastTs = 0;
    int64_t ts = 0;
    while (!cancel && av_read_frame(fmt_ctx.get(), &pkt) >= 0)
    {                
        ret  = readFrame(&pkt,video_dec_ctx.get(),frame.get(),img_convert_ctx.get(),height,frameRGB.get(),&ts);
        if(cancel) return;
        auto waitTime = (ts-lastTs)*av_q2d(video_stream->time_base)*1000;
        std::this_thread::sleep_for(std::chrono::milliseconds((long)waitTime));
        lastTs = ts;
        counter++;
    }
    //flush
    readFrame(nullptr,video_dec_ctx.get(),frame.get(),img_convert_ctx.get(),height,frameRGB.get(),&ts);
}

int MediaStreamingConnection::readFrame(AVPacket* pkt,
                                    AVCodecContext *video_dec_ctx, AVFrame *frame,
                                    SwsContext * img_convert_ctx,int height,AVFrame* frameRGB,int64_t* ts)
{
    AVPacket* orig_pkt = pkt;
    int ret = avcodec_send_packet(video_dec_ctx,pkt);
    if(ret < 0)
    {
        return ret;
    }
    while(!cancel && ret >=0)
    {
        ret = avcodec_receive_frame(video_dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            break;
        }
        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,
                                      height, frameRGB->data, frameRGB->linesize);
        if(!cancel && !signal.empty()) {
            signal(frameRGB->data[0],frameRGB->width,frameRGB->height,frameRGB->linesize[0]);
        } else {
            return ret;
        }
        *ts = frame->best_effort_timestamp;
        //video_dec_ctx->
        //frame->pkt_duration;

        /*cv::Mat img(frameRGB->height,frameRGB->width,CV_8UC3,
                    frameRGB->data[0],frameRGB->linesize[0]); //dst->data[0]);
        frames->push_back(img.clone());*/
    }
    if(orig_pkt)
    {
        av_packet_unref(orig_pkt);
    }
    return ret;
}

void MediaStreamingConnection::framesAvailableHandler(const typename Signal::slot_type& slot)
{
    signal.connect(slot);
}
void MediaStreamingConnection::errorHandler(const typename ErrorSignal::slot_type& slot)
{
    errorSignal.connect(slot);
}
