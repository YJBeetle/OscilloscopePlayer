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
//    av_frame_free(&video_convert_frame);
    if(video_edge) delete video_edge;
}

void Decode::set(int scaleX, int scaleY, int moveX, int moveY, int edge)
{
    this->scaleX = scaleX;
    this->scaleY = scaleY;
    this->moveX = moveX;
    this->moveY = moveY;
    this->edge = edge;
    refresh = true;
}

void Decode::setScaleX(int scaleX)
{
    this->scaleXrefresh = scaleX;
    refresh = true;
}

void Decode::setScaleY(int scaleY)
{
    this->scaleYrefresh = scaleY;
    refresh = true;
}

void Decode::setMoveX(int moveX)
{
    this->moveXrefresh = moveX;
    refresh = true;
}

void Decode::setMoveY(int moveY)
{
    this->moveYrefresh = moveY;
    refresh = true;
}

void Decode::setEdge(int edge)
{
    this->edgeRefresh = edge;
    refresh = true;
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

bool Decode::isReady()
{
    return ready;
}

int Decode::open(QString filename)
{
    /* 打开文件读取格式 */
    if (avformat_open_input(&fmt_ctx, filename.toLatin1().data(), nullptr, nullptr) < 0)
    {
        return 1;   //无法打开源文件
    }

    /* 显示输入文件信息（调试用） */
    //    av_dump_format(fmt_ctx, 0, filename.toLatin1().data(), 0);

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
        const AVCodec* video_dec = avcodec_find_decoder(video_stream->codecpar->codec_id); //视频流的解码器
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
                        /* 读取一些信息，宽高像素格式 */
                        video_width = video_dec_ctx->width;
                        video_height = video_dec_ctx->height;
                        video_pix_fmt = video_dec_ctx->pix_fmt;
                        //转换色彩
//                        video_convert_frame = av_frame_alloc(); //帧
//                        if (!video_convert_frame)
//                            return 8;  //无法分配帧
//                        unsigned char* out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_BGRA, video_width, video_height, 1));
//                        if (!out_buffer)
//                            return 8;  //无法分配
//                        av_image_fill_arrays(video_convert_frame->data, video_convert_frame->linesize, out_buffer, AV_PIX_FMT_BGRA, video_width, video_height, 1);
//                        video_convert_ctx = sws_getContext(video_width, video_height, video_pix_fmt,
//                                                           video_width, video_height, AV_PIX_FMT_BGRA,
//                                                           SWS_BICUBIC, NULL, NULL, NULL);
//                        if(!video_convert_ctx)
//                        {
//                            audio_stream_idx = -1;
//                            return 8;  //无法设置视频色彩转换上下文
//                        }
                        //边缘检测
                        video_edge = new quint8[256 * 256];
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
        const AVCodec* audio_dec = avcodec_find_decoder(audio_stream->codecpar->codec_id); //音频流的解码器
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
                        /* 音频重采样 */    //采样为44100双声道16位整型
                        audio_convert_ctx = swr_alloc_set_opts(nullptr,
                                                               AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_U8, 44100,
                                                               int64_t(audio_dec_ctx->channel_layout), audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate,
                                                               0, nullptr);
                        //audio_convert_ctx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, audio_stream->codec->channel_layout, audio_stream->codec->sample_fmt, audio_stream->codec->sample_rate, 0, NULL);    //采样为44100双声道16位整型
                        if(!audio_convert_ctx)
                        {
                            audio_stream_idx = -1;
                            return 15;  //无法设置重新采样上下文
                        }
                        swr_init(audio_convert_ctx);
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

    ready = true;   //已准备好状态

    return 0;
}

AVRational Decode::fps()
{
    if(video_stream && video_stream->avg_frame_rate.num && video_stream->avg_frame_rate.den)
        return video_stream->avg_frame_rate;
    else
    {
        AVRational ret;
        ret.den = 1;
        ret.num = 30;
        return ret;
    }
}

int Decode::width()
{
    return video_width;
}

int Decode::height()
{
    return video_height;
}

int Decode::decode_packet(int *got_frame)
{
    int ret = 0;
    int decoded = pkt.size;
    *got_frame = 0;

    //检查队列长度
    auto fps = this->fps();
    if(video.size() > 4)    //缓存4f，如果队列中已有的超过4f则休息一帧的时间
        QThread::msleep(1000 * fps.den / fps.num);
    if (pkt.stream_index == video_stream_idx)   //包是视频包
    {
        /* 解码视频帧 */
        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0)
        {
            fprintf(stderr, "发送数据包进行解码时出错\n");
            return ret;
        }
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(video_dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return ret;
            else if (ret < 0) {
                fprintf(stderr, "解码时出错\n");
                return ret;
            }

            //输出帧信息
            //printf("video_frame n:%d coded_n:%d\n",
            //       video_frame_count++, frame->coded_picture_number);
            //转换色彩
            //sws_scale(video_convert_ctx,
            //          frame->data, frame->linesize, 0, video_height,
            //          video_convert_frame->data, video_convert_frame->linesize);

            //检查刷新
            if(refresh)
            {
                refresh = false;
                scaleX = scaleXrefresh;
                scaleY = scaleYrefresh;
                moveX = moveXrefresh;
                moveY = moveYrefresh;
                edge = edgeRefresh;
            }

            //边缘检测
            float temp1[9]={1,0,-1,1,0,-1,1,0,-1};  //模板数组
            float temp2[9]={-1,-1,-1,0,0,0,1,1,1};
            float result1;  //用于暂存模板值
            float result2;
            int count = 0;  //总点数计数
            memset(video_edge, 0, 256 * 256);
            {
                int y = (1 - moveY - video_height / 2) * scaleY / 100 + 256 / 2;
                if(y < 0)
                    y = 0;
                int yMax = (video_height - 1 - moveY - video_height / 2) * scaleY / 100 + 256 / 2;
                if(yMax > 256)
                    yMax = 256;
                for (; y < yMax; y++)
                {
                    int yy = video_height / 2 + (-256 / 2 + y) * 100 / scaleY + moveY;
                    {
                        int x = (1 - moveX - video_width / 2) * scaleX / 100 + 256 / 2;
                        if(x < 0)
                            x = 0;
                        int xMax = (video_width - 1 - moveX - video_width / 2) * scaleX / 100 + 256 / 2;
                        if(xMax > 256)
                            xMax = 256;
                        for(; x < xMax; x++)
                        {
                            int xx = video_width / 2 + (-256 / 2 + x) * 100 / scaleX + moveX;
                            result1 = 0;
                            result2 = 0;
                            for (int ty = 0; ty < 3; ty++)
                            {
                                for (int tx = 0; tx < 3; tx++)
                                {
                                    int z = frame->data[0][(yy - 1 + ty) * frame->linesize[0] + xx - 1 + tx]; //这里我们假装视频是YUV，这里只用Y就是灰度了
                                    result1 += z * temp1[ ty * 3 + tx];
                                    result2 += z * temp2[ ty * 3 + tx];
                                }
                            }
                            result1 = abs(int(result1));
                            result2 = abs(int(result2));
                            if(result1 < result2)
                                result1 = result2;
                            if((result1 > edge))  //超过阈值则为有效点
                            {
                                video_edge[y * 256 + x] = 255;
                                count++;
                            }

                        }
                    }
                }
            }

            //生成QImage
            //QImage image(video_width, video_height, QImage::Format_ARGB32);
            //for (int y = 0; y < video_height; y++)
            //    memcpy(image.scanLine(y), video_convert_frame->data[0] + y * video_convert_frame->linesize[0], video_width * 4);
            QImage image(256, 256, QImage::Format_Grayscale8);
            for (int y = 0; y < 256; y++)
            {
                int yy = video_height / 2 + (-256 / 2 + y) * 100 / scaleY + moveY;
                if(yy < 0 || yy >= video_height)
                    memset(image.scanLine(y), 0, 256);
                else
                    for(int x = 0; x < 256; x++)
                    {
                        int xx = video_width / 2 + (-256 / 2 + x) * 100 / scaleX + moveX;
                        if(xx < 0 || xx >= video_width)
                            image.scanLine(y)[x] = 0;
                        else
                            image.scanLine(y)[x] = frame->data[0][yy * frame->linesize[0] + xx]; //这里我们假装视频是YUV，这里只用Y就是灰度了
                    }
            }
            video.enqueue(image);   //添加到队列尾部
            QImage imageEdge(256, 256, QImage::Format_Grayscale8);
            for (int y = 0; y < 256; y++)
                memcpy(imageEdge.scanLine(y), video_edge + y * 256, 256);
            videoEdge.enqueue(imageEdge);   //添加到队列尾部

            //计算路径点
            QVector<Point> points(count);
            int x = 0;
            int y = 0;
            for (int i = 0; i < count; i++)
            {
                int rMax;
                rMax = (x > y) ? x : y;
                rMax = (rMax > 256 - x) ? rMax : 256 - x;
                rMax = (rMax > 256 - y) ? rMax : 256 - y;

                for (int r = 0; r < rMax; r++)
                {
                    int xMin = x - r;
                    int xMax = x + r;
                    int yMin = y - r;
                    int yMax = y + r;
                    bool xMinOut = false;
                    bool xMaxOut = false;
                    bool yMinOut = false;
                    bool yMaxOut = false;
                    if(xMin < 0)
                    {
                        xMin = 0;
                        xMinOut = true;
                    }
                    if(xMax >= 256)
                    {
                        xMax = 255;
                        xMaxOut = true;
                    }
                    if(yMin < 0)
                    {
                        yMin = 0;
                        yMinOut = true;
                    }
                    if(yMax >= 256)
                    {
                        yMax = 255;
                        yMaxOut = true;
                    }
                    //上边缘
                    if(!yMinOut)
                    {
                        for(int xx = xMin; xx < xMax; xx++)
                        {
                            if(video_edge[yMin * 256 + xx] == 255)
                            {
                                points[i].x = x = xx;
                                points[i].y = y = yMin;
                                video_edge[yMin * 256 + xx] = 254; //标记为已使用过了
                                goto find;
                            }
                        }
                    }
                    //下边缘
                    if(!yMaxOut)
                    {
                        for(int xx = xMin; xx < xMax; xx++)
                        {
                            if(video_edge[yMax * 256 + xx] == 255)
                            {
                                points[i].x = x = xx;
                                points[i].y = y = yMax;
                                video_edge[yMax * 256 + xx] = 254; //标记为已使用过了
                                goto find;
                            }
                        }
                    }
                    //左边缘
                    if(!xMinOut)
                    {
                        for(int yy = yMin; yy < yMax; yy++)
                        {
                            if(video_edge[yy * 256 + xMin] == 255)
                            {
                                points[i].x = x = xMin;
                                points[i].y = y = yy;
                                video_edge[yy * 256 + xMin] = 254; //标记为已使用过了
                                goto find;
                            }
                        }
                    }
                    //右边缘
                    if(!xMaxOut)
                    {
                        for(int yy = yMin; yy < yMax; yy++)
                        {
                            if(video_edge[yy * 256 + xMax] == 255)
                            {
                                points[i].x = x = xMax;
                                points[i].y = y = yy;
                                video_edge[yy * 256 + xMax] = 254; //标记为已使用过了
                                goto find;
                            }
                        }
                    }
                }
find:
                ;;
            }
            this->points.enqueue(points);
        }
    }
    else if (pkt.stream_index == audio_stream_idx)  //是音频包
    {
        /* 解码音频帧 */
        ret = avcodec_send_packet(audio_dec_ctx, &pkt);
        if (ret < 0)
        {
            fprintf(stderr, "将数据包提交给解码器时出错\n");
            return ret;
        }
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(audio_dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return ret;
            else if (ret < 0) {
                fprintf(stderr, "解码时出错\n");
                return ret;
            }
            //输出帧信息
            //printf("audio_frame n:%d nb_samples:%d pts:%s\n",
            //       audio_frame_count++, frame->nb_samples,
            //       av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

            int data_size = av_get_bytes_per_sample(audio_dec_ctx->sample_fmt);
            if (data_size < 0) {
                /* This should not occur, checking just for paranoia */
                fprintf(stderr, "无法计算数据大小\n");
                return -1;
            }

            //            for (i = 0; i < frame->nb_samples; i++)
            //                for (ch = 0; ch < audio_dec_ctx->channels; ch++)
            //                    fwrite(frame->data[ch] + data_size*i, 1, data_size, audio_dst_file);


            //            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
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
            //            QThread::msleep( 20 );




        }
    }


    return decoded;
}
