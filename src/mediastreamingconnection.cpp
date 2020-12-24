#include "mediastreamingconnection.h"
#include <boost/asio.hpp>
#include <boost/url.hpp>
#include <fstream>
#include <filesystem>

#include "htmlparser.h"
#include "macros.h"

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
    struct avparser_deleter
    {
        void operator()(AVCodecParserContext* parser)
        {
            av_parser_close(parser);
        }
    };
}

MediaStreamingConnection::MediaStreamingConnection(boost::asio::io_context& context,
                                                   boost::asio::ssl::context& ssl_context,
                                                   const std::string& userAgent):
    RedditConnection(context,ssl_context,"",""),userAgent(userAgent),cancel(false)
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
    boost::beast::http::read_header(stream.value(),buffer,responseParser.value(),error);
    if(error)
    {
        onError(error);
        return;
    }

    std::string contentType;
    std::string location;
    for(const auto& h : responseParser.value().get())
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
           status == boost::beast::http::status::found)
        {
            stream.emplace(boost::asio::make_strand(context), ssl_context);
            downloadUrl(location);
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
    boost::beast::http::async_read(stream.value(), buffer, responseParser.value(), readMethod);
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
        auto videoUrl = htmlParser.getVideoUrl(currentPost->domain);
        boost::system::error_code ec;
        stream->shutdown(ec);
        if(videoUrl.empty())
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
        //startStreaming(targetFile);
        BOOST_ASSERT(false);
    }
}

void MediaStreamingConnection::streamMedia(post* mediaPost)
{
    currentPost = mediaPost;
    auto url = currentPost->url;
    if(currentPost->postMedia && currentPost->postMedia->redditVideo)
    {
        url = currentPost->postMedia->redditVideo->dashUrl;
        boost::asio::post(context.get_executor(),std::bind(
                               &MediaStreamingConnection::startStreaming,
                               this->shared_from_base<MediaStreamingConnection>(),url));
    }
    else
    {
        //'https://www.youtube.com/watch?v=BaW_jenozKc&gl=US&hl=en&has_verified=1&bpctr=9999999999'

        downloadUrl(url);
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
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target(target);
    request.set(boost::beast::http::field::connection, "close");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.prepare_payload();
    response.clear();
    buffer.consume(buffer.size());
    buffer.clear();
    responseParser.emplace();
    responseParser->body_limit(std::numeric_limits<uint64_t>::max());
    resolveHost();
}

void MediaStreamingConnection::startStreaming(const std::string& file)
{   
    int video_stream_idx = -1;
    AVStream *video_stream = nullptr;
    AVPacket pkt;
    AVCodec *dec = nullptr;
    AVDictionary *general_options = nullptr;

    std::unique_ptr<AVFormatContext,avformat_context_deleter> fmt_ctx{nullptr,avformat_context_deleter{}};
    {
        AVFormatContext *fmt = avformat_alloc_context();
        int ret = avformat_open_input(&fmt,file.c_str(), nullptr, &general_options);
        if (ret < 0)
        {
            char buf[256];
            //av_strerror(ret,buf,sizeof(buf));
            errorSignal(ret,buf);
            return;
        }
        fmt_ctx.reset(fmt);
    }
    int ret = avformat_find_stream_info(fmt_ctx.get(), nullptr);
    if (ret < 0)
    {
        char buf[256];
        //av_strerror(ret,buf,sizeof(buf));
        errorSignal(ret,buf);
        return;
    }

#ifdef REDDIT_DESKTOP_DEBUG
    av_dump_format(fmt_ctx.get(), 0, file.c_str(), 0);
#endif

    if((video_stream_idx = av_find_best_stream(fmt_ctx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) < 0)
    {
        errorSignal(-1,"Media does not contain video stream");
        return;
    }

    video_stream = fmt_ctx->streams[video_stream_idx];
    //video_stream->codecpar->extradata;

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
    AVDictionary *opts = NULL;
    if (video_dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO || video_dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        //av_dict_set(&opts, "refcounted_frames", "1", 0);
    }

    ret = avcodec_open2(video_dec_ctx.get(), dec, nullptr);
    if(ret < 0)
    {
        char buf[256];
       // av_strerror(ret,buf,sizeof(buf));
        errorSignal(ret,buf);
        return;
    }

    std::unique_ptr<AVFrame,avframe_deleter> frame(av_frame_alloc(),avframe_deleter());
    std::unique_ptr<AVFrame,avframe_deleter> frameRGB(av_frame_alloc(),avframe_deleter());

    AVPixelFormat  pFormat = AV_PIX_FMT_RGBA;

    //av_image_alloc();
    //auto lineSize = av_image_get_linesize(pFormat, width, height);
    int numBytes = av_image_get_buffer_size(pFormat, width, height,
                                        1 /*https://stackoverflow.com/questions/35678041/what-is-linesize-alignment-meaning*/);
    std::unique_ptr<uint8_t,avbuffer_deleter> buffer ((uint8_t *) av_malloc(numBytes*sizeof(uint8_t)),avbuffer_deleter());
    av_image_fill_arrays(frameRGB->data,frameRGB->linesize,buffer.get(),pFormat,
                         width, height,1);
    frameRGB->width = width;
    frameRGB->height = height;

    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    std::unique_ptr<SwsContext,swscontext_deleter> img_convert_ctx(
                sws_getContext(width, height, pix_fmt, width,
                                     height, pFormat, SWS_BILINEAR,
                                     nullptr, nullptr,nullptr),swscontext_deleter());
    int counter = 0;
    int64_t lastTs = 0;
    int64_t ts = 0;

    //av_parser_parse2(parser.get(),video_dec_ctx.get(),&pkt.data,&pkt.size,data,data_size,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
    while (!cancel && av_read_frame(fmt_ctx.get(), &pkt) >= 0)
    {
        if (pkt.stream_index == video_stream_idx)
        {
            ret  = readFrame(&pkt,video_dec_ctx.get(),frame.get(),img_convert_ctx.get(),height,frameRGB.get(),&ts);
            auto waitTime = (ts-lastTs)*av_q2d(video_stream->time_base)*1000;
            std::this_thread::sleep_for(std::chrono::milliseconds((long)waitTime));
            lastTs = ts;
            counter++;
        }
        av_packet_unref(&pkt);
    }
    //flush
    readFrame(nullptr,video_dec_ctx.get(),frame.get(),img_convert_ctx.get(),height,frameRGB.get(),&ts);
    av_dict_free(&opts);
}

int MediaStreamingConnection::readFrame(AVPacket* pkt,
                                    AVCodecContext *video_dec_ctx, AVFrame *frame,
                                    SwsContext * img_convert_ctx,int height,AVFrame* frameRGB,int64_t* ts)
{
    int ret = 0;
    ret = avcodec_send_packet(video_dec_ctx,pkt);
    if(ret < 0) return ret;
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
        if(!cancel && !streamingSignal.empty()) {
            streamingSignal(frameRGB->data[0],frameRGB->width,frameRGB->height,frameRGB->linesize[0]);
        } else {
            return ret;
        }
        *ts = frame->best_effort_timestamp;
        //video_dec_ctx->
        //frame->pkt_duration;
    }
    if(ret == AVERROR_EOF)
    {
        avcodec_flush_buffers(video_dec_ctx);
        return ret;
    }

    return ret;
}

void MediaStreamingConnection::framesAvailableHandler(const typename StreamingSignal::slot_type& slot)
{
    streamingSignal.connect(slot);
}
void MediaStreamingConnection::errorHandler(const typename ErrorSignal::slot_type& slot)
{
    errorSignal.connect(slot);
}
