#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}
























int MainWindow::decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    int decoded = pkt.size;
    *got_frame = 0;
    if (pkt.stream_index == video_stream_idx) {
        /* decode video frame */
        ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }
        if (*got_frame) {
            if (frame->width != vwidth || frame->height != vheight ||
                frame->format != pix_fmt) {
                /* To handle this change, one could call av_image_alloc again and
                 * decode the following frames into another rawvideo file. */
                fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a rawvideo file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: vwidth = %d, vheight = %d, format = %s\n"
                        "new: vwidth = %d, vheight = %d, format = %s\n",
                        vwidth, vheight, av_get_pix_fmt_name(pix_fmt),
                        frame->width, frame->height,
                        av_get_pix_fmt_name(AVPixelFormat(frame->format)));
                return -1;
            }
            printf("video_frame%s n:%d coded_n:%d\n",
                   cached ? "(cached)" : "",
                   video_frame_count++, frame->coded_picture_number);
            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize,
                          (const uint8_t **)(frame->data), frame->linesize,
                          pix_fmt, vwidth, vheight);
            /* write to rawvideo file */
//            fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);








            auto image = new QImage(frame->width, frame->height, QImage::Format_RGB32);
            for (int y = 0; y < frame->height; y++)
            {
                for(int x = 0; x < frame->width; x++)
                {
                    int s = y * frame->linesize[0] + x;
                    QColor color(frame->data[0][s], frame->data[0][s+1], frame->data[0][s+2], frame->data[0][s+3]);
                    image->setPixelColor(x, y, color);
                }
            }
            if(this->ui->videoViewer->image) delete this->ui->videoViewer->image;
            this->ui->videoViewer->image = image;

            this->ui->videoViewer->update();
    //        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            QCoreApplication::processEvents();





        }
    } else if (pkt.stream_index == audio_stream_idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, pkt.size);
        if (*got_frame) {
            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
            printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
                   cached ? "(cached)" : "",
                   audio_frame_count++, frame->nb_samples,
                   av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));
            /* Write the raw audio data samples of the first plane. This works
             * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
             * most audio decoders output planar audio, which uses a separate
             * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
             * In other words, this code will write only the first audio channel
             * in these cases.
             * You should use libswresample or libavfilter to convert the frame
             * to packed data. */
//            fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);


memset(play_buf, sizeof(uint8_t), out_size);

            swr_convert(pSwrCtx, &play_buf , out_size, (const uint8_t**)frame->data, frame->nb_samples);


            out->write((char*)play_buf, out_size);



            QTest::qSleep( 30 );




        }
    }
    /* If we use frame reference counting, we own the data and need
     * to de-reference it when we don't use it anymore */
    if (*got_frame && refcount)
        av_frame_unref(frame);
    return decoded;
}

int MainWindow::open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }
        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }
    return 0;
}
static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;
    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }
    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}































void MainWindow::on_pushButtonOpen_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open"),
                                                "",
                                                tr("MPEG Video(*.mp4 *.mov *.mpg *.m4v *.avi *.flv *.rm *.rmvb);;Allfile(*.*)"));
    if(!path.isEmpty()){
        qDebug() << path;








        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setCodec("audio/pcm");
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleSize(16);
        format.setByteOrder(QAudioFormat::LittleEndian);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(format)) {
            qDebug() << "Raw audio format not supported by backend, cannot play audio.";
            return;
        }


        audio = new QAudioOutput(format, this);
        out = audio->start();

        play_buf = (uint8_t*)av_malloc(out_size);
















        int ret = 0, got_frame;
        src_filename = path.toLatin1().data();
//        video_dst_filename = argv[2];
//        audio_dst_filename = argv[3];
        /* open input file, and allocate format context */
        if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
            fprintf(stderr, "Could not open source file %s\n", src_filename);
            exit(1);
        }
        /* retrieve stream information */
        if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
            fprintf(stderr, "Could not find stream information\n");
            exit(1);
        }
        if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
            video_stream = fmt_ctx->streams[video_stream_idx];
//            video_dst_file = fopen(video_dst_filename, "wb");
//            if (!video_dst_file) {
//                fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
//                ret = 1;
//                goto end;
//            }
            /* allocate image where the decoded image will be put */
            vwidth = video_dec_ctx->width;
            vheight = video_dec_ctx->height;
            pix_fmt = video_dec_ctx->pix_fmt;
            ret = av_image_alloc(video_dst_data, video_dst_linesize,
                                 vwidth, vheight, pix_fmt, 1);
            if (ret < 0) {
                fprintf(stderr, "Could not allocate raw video buffer\n");
                goto end;
            }
            video_dst_bufsize = ret;
        }
        if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
            audio_stream = fmt_ctx->streams[audio_stream_idx];






            //音频
            pSwrCtx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 48000, audio_stream->codec->channel_layout, audio_stream->codec->sample_fmt, audio_stream->codec->sample_rate, 0, NULL);
            if(!pSwrCtx)
            {
                fprintf(stderr,"Could not set options for resample context.\n");
                return;
            }
            swr_init(pSwrCtx);






//            audio_dst_file = fopen(audio_dst_filename, "wb");
//            if (!audio_dst_file) {
//                fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
//                ret = 1;
//                goto end;
//            }
        }

        /* dump input information to stderr */
        av_dump_format(fmt_ctx, 0, src_filename, 0);
        if (!audio_stream && !video_stream) {
            fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
            ret = 1;
            goto end;
        }
        frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "Could not allocate frame\n");
            ret = AVERROR(ENOMEM);
            goto end;
        }
        /* initialize packet, set data to NULL, let the demuxer fill it */
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
//        if (video_stream)
//            printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
//        if (audio_stream)
//            printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);
        /* read frames from the file */
        while (av_read_frame(fmt_ctx, &pkt) >= 0) {
            AVPacket orig_pkt = pkt;
            do {
                ret = decode_packet(&got_frame, 0);
                if (ret < 0)
                    break;
                pkt.data += ret;
                pkt.size -= ret;
            } while (pkt.size > 0);
            av_packet_unref(&orig_pkt);
        }
        /* flush cached frames */
        pkt.data = NULL;
        pkt.size = 0;
        do {
            decode_packet(&got_frame, 1);
        } while (got_frame);
        printf("Demuxing succeeded.\n");
//        if (video_stream) {
//            printf("Play the output video file with the command:\n"
//                   "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
//                   av_get_pix_fmt_name(pix_fmt), vwidth, vheight,
//                   video_dst_filename);
//        }
//        if (audio_stream) {
//            enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
//            int n_channels = audio_dec_ctx->channels;
//            const char *fmt;
//            if (av_sample_fmt_is_planar(sfmt)) {
//                const char *packed = av_get_sample_fmt_name(sfmt);
//                printf("Warning: the sample format the decoder produced is planar "
//                       "(%s). This example will output the first channel only.\n",
//                       packed ? packed : "?");
//                sfmt = av_get_packed_sample_fmt(sfmt);
//                n_channels = 1;
//            }
//            if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
//                goto end;
//            printf("Play the output audio file with the command:\n"
//                   "ffplay -f %s -ac %d -ar %d %s\n",
//                   fmt, n_channels, audio_dec_ctx->sample_rate,
//                   audio_dst_filename);
//        }
    end:
        avcodec_free_context(&video_dec_ctx);
        avcodec_free_context(&audio_dec_ctx);
        avformat_close_input(&fmt_ctx);
//        if (video_dst_file)
//            fclose(video_dst_file);
//        if (audio_dst_file)
//            fclose(audio_dst_file);
        av_frame_free(&frame);
        av_free(video_dst_data[0]);
//        return ret < 0;











    }
}

void MainWindow::on_pushButtonTest_clicked()
{

    oscilloscopeFormat.setSampleRate(96000);
    oscilloscopeFormat.setChannelCount(2);
    oscilloscopeFormat.setCodec("audio/pcm");
    oscilloscopeFormat.setSampleType(QAudioFormat::SignedInt);
    oscilloscopeFormat.setSampleSize(8);
    oscilloscopeFormat.setByteOrder(QAudioFormat::LittleEndian);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

    int i = 0;
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if(i == 4)
        info = deviceInfo;
        i++;
        qDebug() << "Device name: " << deviceInfo.deviceName();
    }

    if (!info.isFormatSupported(oscilloscopeFormat)) {
        QMessageBox msgBox;
        msgBox.setText("所选输出设备不支持此设置。");
        msgBox.exec();
        return;
    }

    oscilloscopeOutput = new QAudioOutput(info, oscilloscopeFormat, this);
    oscilloscopeDevice = oscilloscopeOutput->start();

    oscilloscopeBufSize = 96000 / 20;

    oscilloscopeBuf = new char[oscilloscopeBufSize];
    memset(oscilloscopeBuf, 0, oscilloscopeBufSize);

    while(1){

        oscilloscopeBuf[0x00]=0x00;
        oscilloscopeBuf[0x02]=0x10;
        oscilloscopeBuf[0x04]=0x20;
        oscilloscopeBuf[0x06]=0x30;
        oscilloscopeBuf[0x08]=0x40;
        oscilloscopeBuf[0x0a]=0x50;
        oscilloscopeBuf[0x0c]=0x60;
        oscilloscopeBuf[0x0e]=0x70;
        oscilloscopeBuf[0x10]=0x79;
        oscilloscopeBuf[0x12]=0x70;
        oscilloscopeBuf[0x14]=0x60;
        oscilloscopeBuf[0x16]=0x50;
        oscilloscopeBuf[0x18]=0x40;
        oscilloscopeBuf[0x1a]=0x30;
        oscilloscopeBuf[0x1c]=0x20;
        oscilloscopeBuf[0x1e]=0x10;
        oscilloscopeBuf[0x20]=-0x00;
        oscilloscopeBuf[0x22]=-0x10;
        oscilloscopeBuf[0x24]=-0x20;
        oscilloscopeBuf[0x26]=-0x30;
        oscilloscopeBuf[0x28]=-0x40;
        oscilloscopeBuf[0x2a]=-0x50;
        oscilloscopeBuf[0x2c]=-0x60;
        oscilloscopeBuf[0x2e]=-0x70;
        oscilloscopeBuf[0x30]=-0x80;
        oscilloscopeBuf[0x32]=-0x70;
        oscilloscopeBuf[0x34]=-0x60;
        oscilloscopeBuf[0x36]=-0x50;
        oscilloscopeBuf[0x38]=-0x40;
        oscilloscopeBuf[0x3a]=-0x30;
        oscilloscopeBuf[0x3c]=-0x20;
        oscilloscopeBuf[0x3e]=-0x10;

        oscilloscopeBuf[0x01]=0x79;
        oscilloscopeBuf[0x03]=0x70;
        oscilloscopeBuf[0x05]=0x60;
        oscilloscopeBuf[0x07]=0x50;
        oscilloscopeBuf[0x09]=0x40;
        oscilloscopeBuf[0x0b]=0x30;
        oscilloscopeBuf[0x0d]=0x20;
        oscilloscopeBuf[0x0f]=0x10;
        oscilloscopeBuf[0x11]=-0x00;
        oscilloscopeBuf[0x13]=-0x10;
        oscilloscopeBuf[0x15]=-0x20;
        oscilloscopeBuf[0x17]=-0x30;
        oscilloscopeBuf[0x19]=-0x40;
        oscilloscopeBuf[0x1b]=-0x50;
        oscilloscopeBuf[0x1d]=-0x60;
        oscilloscopeBuf[0x1f]=-0x70;
        oscilloscopeBuf[0x21]=-0x80;
        oscilloscopeBuf[0x23]=-0x70;
        oscilloscopeBuf[0x25]=-0x60;
        oscilloscopeBuf[0x27]=-0x50;
        oscilloscopeBuf[0x29]=-0x40;
        oscilloscopeBuf[0x2b]=-0x30;
        oscilloscopeBuf[0x2d]=-0x20;
        oscilloscopeBuf[0x2f]=-0x10;
        oscilloscopeBuf[0x31]=0x00;
        oscilloscopeBuf[0x33]=0x10;
        oscilloscopeBuf[0x35]=0x20;
        oscilloscopeBuf[0x37]=0x30;
        oscilloscopeBuf[0x39]=0x40;
        oscilloscopeBuf[0x3b]=0x50;
        oscilloscopeBuf[0x3d]=0x60;
        oscilloscopeBuf[0x3f]=0x70;

        oscilloscopeDevice->write(oscilloscopeBuf, 64);

        QCoreApplication::processEvents();

    }



}
