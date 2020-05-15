# ffmpeg_files

ffmpeg_test: 测试 ffmpeg 是否安装成功

rtsp_stream: ffmpeg + rtmp 拉流, 接收视频流并转成flv格式保存下来, 采用的协议是rtmp, 停止运行按ctrl+c即可

h264_decodec_RGB: rtmp拉流 -> FLV -> H.264 -> YUV -> RGB -> OPENCV处理
