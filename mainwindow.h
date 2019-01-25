#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDebug>

extern "C"
{
#include "libavcodec/avcodec.h"
#define INBUF_SIZE 4096
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

private:
    Ui::MainWindow *ui;

    void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt);
};

#endif // MAINWINDOW_H
