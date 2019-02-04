#ifndef DECODE_H
#define DECODE_H

#include <QThread>
#include <QQueue>
#include <QImage>
#include <QBuffer>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#define MAX_AUDIO_FRAME_SIZE    192000
}

class Decode : public QThread
{
    Q_OBJECT
public:
    explicit Decode(QObject *parent = nullptr);
    ~Decode();

    void run();
    int open(QString filename);
    AVRational fps();

    QQueue<QImage> video;
    QQueue<QBuffer> audio;
signals:

public slots:

private:
    AVFormatContext *fmt_ctx = nullptr; //源文件格式信息
    int video_stream_idx = -1, audio_stream_idx = -1;   //流类型
    AVStream *video_stream = nullptr, *audio_stream = nullptr;  //流
    AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx = nullptr;  //解码器上下文
    AVPacket pkt;
    AVFrame* frame = nullptr;
    /* 视频 */
    int vwidth, vheight;
    enum AVPixelFormat pix_fmt; //像素格式
    uint8_t *video_dst_data[4] = {nullptr}; //视频缓冲区
    int video_dst_bufsize;  //视频缓冲大小
    int      video_dst_linesize[4];
    int video_frame_count = 0;  //计数
    /* 音频 */
    SwrContext* audio_swr_ctx = nullptr;    //转码
    int audio_frame_count = 0;  //计数

    int decode_packet(int *got_frame);
};

#endif // DECODE_H
