#include "decode.h"

Decode::Decode(QObject *parent) : QThread(parent)
{

}

Decode::~Decode()
{
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_free(video_dst_data[0]);
}

void Decode::run()
{
    int ret = 0, got_frame;

    /* 从文件中读取帧 */
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        do {
            ret = decode_packet(&got_frame);
            if (ret < 0)
                break;
            pkt.data += ret;
            pkt.size -= ret;
        } while (pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }
    /* 刷新缓存的帧 */
    pkt.data = nullptr;
    pkt.size = 0;
    do {
        decode_packet(&got_frame);
    } while (got_frame);


}

int Decode::open(QString filename)
{
    /* 打开文件读取格式 */
    if (avformat_open_input(&fmt_ctx, filename.toLatin1().data(), nullptr, nullptr) < 0)
    {
        return 1;   //无法打开源文件
    }

    /* 显示输入文件信息（调试用） */
    av_dump_format(fmt_ctx, 0, filename.toLatin1().data(), 0);

    /* 检索流信息 */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
    {
        return 2;   //找不到流信息
    }

    /* 打开视频流 */
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx >= 0)
    {
        video_stream = fmt_ctx->streams[video_stream_idx];   //视频流
        /* 寻找视频流的解码器 */
        AVCodec* video_dec = nullptr;    //视频流的解码器
        video_dec = avcodec_find_decoder(video_stream->codecpar->codec_id);
        if (video_dec)
        {
            /* 为解码器分配编解码器上下文 */
            video_dec_ctx = avcodec_alloc_context3(video_dec);
            if (video_dec_ctx)
            {
                /* 将编解码器参数从输入流复制到输出编解码器上下文 */
                if (avcodec_parameters_to_context(video_dec_ctx, video_stream->codecpar) >= 0)
                {
                    /* 启动解码器 */
                    if (avcodec_open2(video_dec_ctx, video_dec, nullptr) >= 0)
                    {
                        /* 分配空间储存解码图像 */
                        vwidth = video_dec_ctx->width;
                        vheight = video_dec_ctx->height;
                        pix_fmt = video_dec_ctx->pix_fmt;
                        video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize, vwidth, vheight, pix_fmt, 1);
                        if (video_dst_bufsize < 0)
                            return 9;   //无法分配原始视频缓冲区
                    }
                    else
                        return 7;   //无法打开视频编解码器
                }
                else
                    return 6;   //无法将视频编解码器参数复制到解码器上下文
            }
            else
                return 5;   //无法分配编解码器上下文
        }
        else
            return 4;   //找不到视频流的解码器
    }
    else
        return 3;   //找不到视频流

    /* 打开音频流 */
    audio_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_idx >= 0)
    {
        audio_stream = fmt_ctx->streams[audio_stream_idx];   //音频流
        /* 寻找视频流的解码器 */
        AVCodec* audio_dec = nullptr;    //音频流的解码器
        audio_dec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
        if (audio_dec)
        {
            /* 为解码器分配编解码器上下文 */
            audio_dec_ctx = avcodec_alloc_context3(audio_dec);
            ;
            if (audio_dec_ctx)
            {
                /* 将编解码器参数从输入流复制到输出编解码器上下文 */
                if (avcodec_parameters_to_context(audio_dec_ctx, audio_stream->codecpar) >= 0)
                {
                    /* 启动解码器 */
                    if (avcodec_open2(audio_dec_ctx, audio_dec, nullptr) >= 0)
                    {
                        /* 音频重采样 */
                        audio_swr_ctx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, int64_t(audio_dec_ctx->channel_layout), audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate, 0, nullptr);    //采样为44100双声道16位整型
                        //audio_swr_ctx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, audio_stream->codec->channel_layout, audio_stream->codec->sample_fmt, audio_stream->codec->sample_rate, 0, NULL);    //采样为44100双声道16位整型
                        if(!audio_swr_ctx)
                        {
                            audio_stream_idx = -1;
                            return 15;  //无法设置重新采样上下文
                        }
                        swr_init(audio_swr_ctx);
                    }
                    else
                    {
                        audio_stream_idx = -1;
                        return 14;   //无法打开音频编解码器
                    }
                }
                else
                {
                    audio_stream_idx = -1;
                    return 13;   //无法将音频编解码器参数复制到解码器上下文
                }
            }
            else
            {
                audio_stream_idx = -1;
                return 12;   //无法分配编解码器上下文
            }
        }
        else
        {
            audio_stream_idx = -1;
            return 11;   //找不到音频流的解码器
        }
    }
    else
    {
        audio_stream_idx = -1;
        return 10;   //找不到音频流
    }

    frame = av_frame_alloc();
    if (!frame)
        return 16;  //无法分配帧

    /* 初始化数据包，data设置为nullptr，让demuxer填充它 */
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    return 0;
}

double Decode::fps()
{
    if(video_stream && video_stream->avg_frame_rate.den && video_stream->avg_frame_rate.num)
        return double(video_stream->avg_frame_rate.num) / double(video_stream->avg_frame_rate.den);
    else
        return 30;
}

int Decode::decode_packet(int *got_frame)
{
    int ret = 0;
    int decoded = pkt.size;
    *got_frame = 0;
    if (pkt.stream_index == video_stream_idx) {
        /* decode video frame */
        ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
//            fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }
        if (*got_frame) {
            if (frame->width != vwidth || frame->height != vheight ||
                frame->format != pix_fmt) {
                /* To handle this change, one could call av_image_alloc again and
                 * decode the following frames into another rawvideo file. */
//                fprintf(stderr, "Error: Width, height and pixel format have to be "
//                        "constant in a rawvideo file, but the width, height or "
//                        "pixel format of the input video changed:\n"
//                        "old: vwidth = %d, vheight = %d, format = %s\n"
//                        "new: vwidth = %d, vheight = %d, format = %s\n",
//                        vwidth, vheight, av_get_pix_fmt_name(pix_fmt),
//                        frame->width, frame->height,
//                        av_get_pix_fmt_name(AVPixelFormat(frame->format)));
                return -1;
            }
//            printf("video_frame%s n:%d coded_n:%d\n",
//                   cached ? "(cached)" : "",
//                   video_frame_count++, frame->coded_picture_number);
            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize, const_cast<const uint8_t **>(frame->data), frame->linesize, pix_fmt, vwidth, vheight);
            /* write to rawvideo file */
//            fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);








//            auto image = new QImage(frame->width, frame->height, QImage::Format_RGB32);
//            for (int y = 0; y < frame->height; y++)
//            {
//                for(int x = 0; x < frame->width; x++)
//                {
//                    int s = y * frame->linesize[0] + x;
//                    QColor color(frame->data[0][s], frame->data[0][s+1], frame->data[0][s+2], frame->data[0][s+3]);
//                    image->setPixelColor(x, y, color);
//                }
//            }
//            if(this->ui->videoViewer->image) delete this->ui->videoViewer->image;
//            this->ui->videoViewer->image = image;

//            this->ui->videoViewer->update();
//    //        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//            QCoreApplication::processEvents();





        }
    } else if (pkt.stream_index == audio_stream_idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
//            fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, pkt.size);
        if (*got_frame) {
//            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
//            printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
//                   cached ? "(cached)" : "",
//                   audio_frame_count++, frame->nb_samples,
//                   av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));
            /* Write the raw audio data samples of the first plane. This works
             * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
             * most audio decoders output planar audio, which uses a separate
             * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
             * In other words, this code will write only the first audio channel
             * in these cases.
             * You should use libswresample or libavfilter to convert the frame
             * to packed data. */
//            fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);





//memset(play_buf, sizeof(uint8_t), out_size);
//            swr_convert(pSwrCtx, &play_buf , out_size, (const uint8_t**)frame->data, frame->nb_samples);
//            out->write((char*)play_buf, out_size);
//            QTest::qSleep( 20 );




        }
    }


    return decoded;
}
