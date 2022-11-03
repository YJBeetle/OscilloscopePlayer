#ifndef DECODE_H
#define DECODE_H

#include <QThread>
#include <QQueue>
#include <QImage>
#include <QBuffer>
#include <atomic>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#define MAX_AUDIO_FRAME_SIZE    192000
}

#include "oscilloscope.h"

class Decode : public QThread
{
    Q_OBJECT
public:
    explicit Decode(QObject *parent = nullptr);
    ~Decode();

    void set(int scaleX, int scaleY, int moveX, int moveY, int edge);
    void setScaleX(int scaleX);
    void setScaleY(int scaleY);
    void setMoveX(int moveX);
    void setMoveY(int moveY);
    void setEdge(int edge);
    void run();
    int open(QString filename);
    bool isReady();
    AVRational fps();
    int width();
    int height();

    QQueue<QImage> video;
    QQueue<QImage> videoEdge;
    QQueue<QBuffer> audio;
    QQueue<QVector<Point>> points;
signals:

public slots:

private:
    bool ready = false;

    int scaleX = 100;
    int scaleY = 100;
    int moveX = 0;
    int moveY = 0;
    int edge = 64;
    int scaleXrefresh = 100;
    int scaleYrefresh = 100;
    int moveXrefresh = 0;
    int moveYrefresh = 0;
    int edgeRefresh = 64;
    QAtomicInteger<bool> refresh = false;

    AVFormatContext *fmt_ctx = nullptr; //源文件格式信息
    int video_stream_idx = -1, audio_stream_idx = -1;   //流类型
    AVStream *video_stream = nullptr, *audio_stream = nullptr;  //流
    AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx = nullptr;  //解码器上下文
    AVPacket pkt;
    AVFrame* frame = nullptr;
    /* 视频 */
    int video_width, video_height;
    AVPixelFormat video_pix_fmt; //像素格式
//    SwsContext* video_convert_ctx = nullptr;    //转码
//    AVFrame* video_convert_frame = nullptr;
    quint8* video_edge = nullptr;  //储存边缘位图
    int video_frame_count = 0;  //视频计数
    /* 音频 */
    SwrContext* audio_convert_ctx = nullptr;    //转码
    int audio_frame_count = 0;  //音频计数

    int decode_packet(int *got_frame);
};

#endif // DECODE_H
