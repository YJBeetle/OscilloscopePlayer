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









void MainWindow::decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        //printf("saving frame %3d\n", dec_ctx->frame_number);
        //fflush(stdout);
        /* the picture is allocated by the decoder. no need to
           free it */
        //snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);

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
}






void MainWindow::on_pushButtonOpen_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open"),
                                                "",
                                                tr("MPEG Video(*.mpg);;Allfile(*.*)"));
    if(!path.isEmpty()){
        qDebug() << path;

        const char *filename;
        const AVCodec *codec;
        AVCodecParserContext *parser;
        AVCodecContext *c= NULL;
        FILE *f;
        AVFrame *frame;
        uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
        uint8_t *data;
        size_t   data_size;
        int ret;
        AVPacket *pkt;

        filename = path.toLatin1().data();
        pkt = av_packet_alloc();
        if (!pkt)
            exit(1);
        /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
        memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
        /* find the MPEG-1 video decoder */
        codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
        if (!codec) {
            fprintf(stderr, "Codec not found\n");
            exit(1);
        }
        parser = av_parser_init(codec->id);
        if (!parser) {
            fprintf(stderr, "parser not found\n");
            exit(1);
        }
        c = avcodec_alloc_context3(codec);
        if (!c) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }
        /* For some codecs, such as msmpeg4 and mpeg4, width and height
           MUST be initialized there because this information is not
           available in the bitstream. */
        /* open it */
        if (avcodec_open2(c, codec, NULL) < 0) {
            fprintf(stderr, "Could not open codec\n");
            exit(1);
        }
        f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Could not open %s\n", filename);
            exit(1);
        }
        frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }
        while (!feof(f)) {
            /* read raw data from the input file */
            data_size = fread(inbuf, 1, INBUF_SIZE, f);
            if (!data_size)
                break;
            /* use the parser to split the data into frames */
            data = inbuf;
            while (data_size > 0) {
                ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                       data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
                if (ret < 0) {
                    fprintf(stderr, "Error while parsing\n");
                    exit(1);
                }
                data      += ret;
                data_size -= ret;
                if (pkt->size)
                    this->decode(c, frame, pkt);
            }
        }
        /* flush the decoder */
        this->decode(c, frame, NULL);
        fclose(f);
        av_parser_close(parser);
        avcodec_free_context(&c);
        av_frame_free(&frame);
        av_packet_free(&pkt);

    }
}
