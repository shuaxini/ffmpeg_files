#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavutil/samplefmt.h"
#include "libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/types_c.h"


void AVFrame2Img(AVFrame *pFrame, cv::Mat& img);
void Yuv420p2Rgb32(const uchar *yuvBuffer_in, const uchar *rgbBuffer_out, int width, int heigth);

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	AVFormatContext *ifmt_cxt = NULL;
	AVPacket pkt;
	AVFrame *pFrame = NULL;
	int ret, i;
	int videoindex = -1;

	AVCodecContext *pCodecCtx;
	AVCodec        *pCodec;


	const char *in_filename = "rtmp://localhost:1935/rtmplive";
	const char *out_filename = "test.h264";

	//Register
	av_register_all();

	//NetWork
	avformat_network_init();

	//input
	if(ret = avformat_open_input(&ifmt_cxt, in_filename, 0, 0) < 0) {
		cerr << "Count not open input file: " << in_filename << endl;
		return -1;
	}

	if(ret = avformat_find_stream_info(ifmt_cxt, 0) < 0) {
		cerr << "Failed to retrive input stream information." << endl;
		return -1;
	}

	//stream information
	av_dump_format(ifmt_cxt, 0, in_filename, 0);

	for(i=0; i<ifmt_cxt->nb_streams; ++i) {
		if(ifmt_cxt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			videoindex = i;
	}

	//Find H.264 Decoder
	pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!pCodec) {
		cerr << "Could't find Coec." << endl;
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if(!pCodecCtx) {
		cerr << "Could not allocate video codec context" << endl;
		return -1;
	}

	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		cerr << "Could not open codec." << endl;
		return -1;
	}

	pFrame = av_frame_alloc();
	if(!pFrame) {
		cerr << "Could not allocate video frame." << endl;
		return -1;
	}

	FILE *fp_video = fopen(out_filename, "wb+");

	cv::Mat image_test;

	AVBitStreamFilterContext *h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

	while(av_read_frame(ifmt_cxt, &pkt) >= 0) {

		if(pkt.stream_index == videoindex) {
			av_bitstream_filter_filter(h264bsfc, ifmt_cxt->streams[videoindex]->codec, NULL, 
										&pkt.data, &pkt.size, pkt.data, pkt.size, 0);

			cout << "Write video packet. size:" << pkt.size << " pts:" << pkt.pts << endl;

			fwrite(pkt.data, 1, pkt.size, fp_video);

			if(pkt.size) {
				ret = avcodec_send_packet(pCodecCtx, &pkt);
				if(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					cout << "avcodec_send_packet: " << ret << std::endl;
					continue;
				}

				//get frame
				ret = avcodec_receive_frame(pCodecCtx, pFrame);
				if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					cout << "avcodec_receive_frame: " << ret << endl;
					continue;
				}

				AVFrame2Img(pFrame, image_test);
			}
		}

		av_packet_unref(&pkt);
	}


	//close filter
	av_bitstream_filter_close(h264bsfc);
	fclose(fp_video);
	avformat_close_input(&ifmt_cxt);
	if(ret < 0 && ret != AVERROR_EOF) {
		cerr << "Error occurred." << endl;
		return -1;
	}

	return 0;

}


void Yuv420p2Rgb32(const uchar *yuvBuffer_in, const uchar *rgbBuffer_out, int width, int height)
{
	uchar *yuvBuffer = (uchar *)yuvBuffer_in;
	uchar *rgb32Buffer = (uchar *)rgbBuffer_out;

	int channels = 3;

	for(int y=0; y<height; ++y) {
		for(int x=0; x<width; ++x) {
            int index = y * width + x;

            int indexY = y * width + x;
            int indexU = width * height + y / 2 * width / 2 + x / 2;
            int indexV = width * height + width * height / 4 + y / 2 * width / 2 + x / 2;

            uchar Y = yuvBuffer[indexY];
            uchar U = yuvBuffer[indexU];
            uchar V = yuvBuffer[indexV];

            int R = Y + 1.402 * (V - 128);
            int G = Y - 0.34413 * (U - 128) - 0.71414*(V - 128);
            int B = Y + 1.772*(U - 128);
            R = (R < 0) ? 0 : R;
            G = (G < 0) ? 0 : G;
            B = (B < 0) ? 0 : B;
            R = (R > 255) ? 255 : R;
            G = (G > 255) ? 255 : G;
            B = (B > 255) ? 255 : B;

            rgb32Buffer[(y*width + x)*channels + 2] = uchar(R);
            rgb32Buffer[(y*width + x)*channels + 1] = uchar(G);
            rgb32Buffer[(y*width + x)*channels + 0] = uchar(B);
		}
	}

}


void AVFrame2Img(AVFrame *pFrame, cv::Mat& img)
{
	int frameHeight = pFrame->height;
    int frameWidth = pFrame->width;
    int channels = 3;
    //输出图像分配内存
    img = cv::Mat::zeros(frameHeight, frameWidth, CV_8UC3);
    Mat output = cv::Mat::zeros(frameHeight, frameWidth,CV_8U);

    //创建保存yuv数据的buffer
    uchar* pDecodedBuffer = (uchar*)malloc(frameHeight*frameWidth * sizeof(uchar)*channels);

    //从AVFrame中获取yuv420p数据，并保存到buffer
    int i, j, k;
    //拷贝y分量
    for (i = 0; i < frameHeight; i++)
    {
        memcpy(pDecodedBuffer + frameWidth*i,
               pFrame->data[0] + pFrame->linesize[0] * i,
               frameWidth);
    }
    //拷贝u分量
    for (j = 0; j < frameHeight / 2; j++)
    {
        memcpy(pDecodedBuffer + frameWidth*i + frameWidth / 2 * j,
               pFrame->data[1] + pFrame->linesize[1] * j,
               frameWidth / 2);
    }
    //拷贝v分量
    for (k = 0; k < frameHeight / 2; k++)
    {
        memcpy(pDecodedBuffer + frameWidth*i + frameWidth / 2 * j + frameWidth / 2 * k,
               pFrame->data[2] + pFrame->linesize[2] * k,
               frameWidth / 2);
    }

    //将buffer中的yuv420p数据转换为RGB;
    Yuv420p2Rgb32(pDecodedBuffer, img.data, frameWidth, frameHeight);

    //简单处理，这里用了canny来进行二值化
    cvtColor(img, output, CV_RGB2GRAY);
    waitKey(2);
    Canny(img, output, 50, 50*2);
    waitKey(2);
    imshow("test",output);
    waitKey(10);
    
    // 测试函数
    // imwrite("test.jpg",img);
    //释放buffer
    free(pDecodedBuffer);
    img.release();
    output.release();

}
