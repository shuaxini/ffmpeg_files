#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>

#ifdef __cplusplus
}
#endif


using namespace std;

int main(int argc, char* argv[])
{
    AVOutputFormat *ofmt = NULL;

    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int videoindex = -1;
    int frame_index = 0;

    //in_filename = "rtmp://58.200.131.2:1935/livetv/hunantv";
    in_filename = "rtmp://localhost:1935/rtmplive";
    out_filename = "receive.flv";

    av_register_all();
    //init network
    avformat_network_init();
    //input
    if(ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0) < 0) {
        std::cerr << "Could not open file " << in_filename << std::endl;
        goto end;
    }
    if(ret = avformat_find_stream_info(ifmt_ctx, NULL) < 0) {
        std::cerr << "Failed to retrieve input stream information" << std::endl;
        goto end;
    }

    for(i=0; i<ifmt_ctx->nb_streams; ++i) {
        if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }

    std::cout << "Output stream infrrmation" << std::endl;
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    
    //output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if(!ofmt_ctx) {
        std::cerr << "Could not create output context." << std::endl;
        goto end;
    }

    ofmt = ofmt_ctx->oformat;
    for(i = 0; i<ifmt_ctx->nb_streams; ++i) {
        //Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodec *codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, codec);

        if(!out_stream) {
            std::cerr << "Failed allocating output stream." << std::endl;
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        AVCodecContext *p_codec_ctx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(p_codec_ctx, in_stream->codecpar);

        //copy the setting of AVCodecContext
        if(ret < 0) {
            std::cerr << "Failed to cpoy context from input to output stream codec context." << std::endl;
            goto end;
        }
        p_codec_ctx->codec_tag = 0;
        if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            p_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        ret = avcodec_parameters_from_context(out_stream->codecpar, p_codec_ctx);
        if(ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "eno:[%d] error to paramters codec paramter.", ret);
        }

    }

    //dump format
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //open output URL
    if(!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if(ret < 0) {
            std::cerr << "Could not open output URL %s." << out_filename << std::endl;
            goto end;
        }
    }

    //write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if(ret < 0){
        std::cerr << "Error occured when opening output URL." << std::endl;
        goto end;
    }

    //pull stream
    while (1) {
        AVStream *in_stream, *out_stram;

        //Get an packet
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0) break;

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stram = ofmt_ctx->streams[pkt.stream_index];

        //copy packet , convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stram->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stram->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stram->time_base);
        pkt.pos = -1;

        //print to screen
        if(pkt.stream_index == videoindex) {
            std::cout << "Receive " << frame_index << " video frame from input URL" << std::endl;
            frame_index++;
        }
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if(ret < 0) {
            std::cerr << "Error muxing packet" << std::endl;
            break;
        }
        av_packet_unref(&pkt);

    }

    //write file trailer
    av_write_trailer(ofmt_ctx);

end:
    avformat_close_input(&ifmt_ctx);
    if(ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if(ret < 0 && ret != AVERROR_EOF) {
        std::cerr << "Error occurred!" << std::endl;
        return -1;
    }

    return 0;
}
