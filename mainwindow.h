#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

#include "decode.h"
#include "oscilloscope.h"

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
    void on_pushButtonPlay_clicked();
    void on_pushButtonTest_clicked();
    void on_comboBoxList_activated(int index);
    void on_comboBoxRate_currentTextChanged(const QString &arg1);
    void on_spinBoxChannel_valueChanged(int arg1);
    void on_spinBoxChannelX_valueChanged(int arg1);
    void on_spinBoxChannelY_valueChanged(int arg1);
    void on_comboBoxFPS_currentTextChanged(const QString &arg1);
    void on_horizontalSliderScaleX_valueChanged(int value);
    void on_horizontalSliderScaleY_valueChanged(int value);
    void on_horizontalSliderMoveX_valueChanged(int value);
    void on_horizontalSliderMoveY_valueChanged(int value);
    void on_horizontalSliderEdge_valueChanged(int value);
    void on_horizontalSliderScaleY_sliderReleased();

private:
    Ui::MainWindow *ui;

    QList<QAudioDeviceInfo> audioDeviceInfoList;
    Oscilloscope oscilloscope;
    Decode decode;

    bool ScaleXY = true;    //等比缩放开关
    enum State{Inited, Ready, Play, Pause, Stop} state;

    void log(const QString text);
};

#endif // MAINWINDOW_H
