#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTest>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>



#define MAX_AUDIO_FRAME_SIZE    192000
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonOpen_clicked();

    void on_pushButtonTest_clicked();

private:
    Ui::MainWindow *ui;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx = nullptr;
    int vwidth, vheight;
    enum AVPixelFormat pix_fmt;
    AVStream *video_stream = nullptr, *audio_stream = nullptr;
    const char *src_filename = nullptr;
    //const char *video_dst_filename = NULL;
    //const char *audio_dst_filename = NULL;
    //FILE *video_dst_file = NULL;
    //FILE *audio_dst_file = NULL;
    uint8_t *video_dst_data[4] = {nullptr};
    int      video_dst_linesize[4];
    int video_dst_bufsize;
    int video_stream_idx = -1, audio_stream_idx = -1;
    AVFrame *frame = nullptr;
    AVPacket pkt;
    int video_frame_count = 0;
    int audio_frame_count = 0;
    /* Enable or disable frame reference counting. You are not supposed to support
     * both paths in your application but pick the one most appropriate to your
     * needs. Look for the use of refcount in this example to see what are the
     * differences of API usage between them. */
    int refcount = 0;

    QAudioFormat format;
    QAudioOutput *audio = nullptr;
    QIODevice *out = nullptr;
    SwrContext *pSwrCtx = nullptr;
    int out_size = MAX_AUDIO_FRAME_SIZE*2;
    uint8_t *play_buf = nullptr;

    int decode_packet(int *got_frame, int cached);
    int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);






    QAudioFormat oscilloscopeFormat;
    QAudioOutput* oscilloscopeOutput = nullptr;
    QIODevice* oscilloscopeDevice = nullptr;
    unsigned int oscilloscopeBufSize = 0;
    char* oscilloscopeBuf = nullptr;


};

#endif // MAINWINDOW_H
