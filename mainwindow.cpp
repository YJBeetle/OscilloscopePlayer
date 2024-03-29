#include "mainwindow.h"
#include "ui_mainwindow.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaDevices>
#endif
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //列出音频设备
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    audioDeviceList = QMediaDevices::audioOutputs();
#else
    audioDeviceList = QAudioDevice::availableDevices(QAudio::AudioOutput);
#endif
    int i = 0;
    foreach (const QAudioDevice &audioDevice, audioDeviceList)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        this->ui->comboBoxList->addItem(audioDevice.description());
#else
        this->ui->comboBoxList->addItem(audioDevice.deviceName());
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if(audioDevice == QMediaDevices::defaultAudioOutput())
#else
        if(audioDevice == QAudioDevice::defaultOutputDevice())
#endif
            this->ui->comboBoxList->setCurrentIndex(i);
        i++;
    }

    //设置日志最大行数
    ui->textEditInfo->document()->setMaximumBlockCount(100);

    //解码器初始设置
    decode.set( ui->horizontalSliderScaleX->value() <= 1000 ? ui->horizontalSliderScaleX->value() / 10 : ui->horizontalSliderScaleX->value() - 900,
                ui->horizontalSliderScaleY->value() <= 1000 ? ui->horizontalSliderScaleY->value() / 10 : ui->horizontalSliderScaleY->value() - 900,
                ui->horizontalSliderMoveX->value(),
                ui->horizontalSliderMoveY->value(),
                ui->horizontalSliderEdge->value());

    //示波器初始设置
    if (!oscilloscope.set(audioDeviceList[ui->comboBoxList->currentIndex()],
                        ui->comboBoxRate->currentText().toInt(),
                        ui->spinBoxChannel->value(),
                        ui->spinBoxChannelX->value(),
                        ui->spinBoxChannelY->value(),
                        ui->comboBoxFPS->currentText().toInt()))
    {
        QMessageBox msgBox;
        msgBox.setText("音频输出设备不支持当前设置。");
        msgBox.exec();
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(const QString text)
{
    ui->textEditInfo->append(text);
}

void MainWindow::on_pushButtonOpen_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open"),
                                                "",
                                                tr("MPEG Video(*.mp4 *.mov *.mpg *.m4v *.avi *.flv *.rm *.rmvb);;Allfile(*.*)"));
    if(!path.isEmpty())
    {
        log("打开: " + path);

        switch(decode.open(path))
        {
        case 0:
            break;
        case 1:
            QMessageBox::warning(this, "打开失败", "无法打开源文件。");
            return;
        case 2:
            QMessageBox::warning(this, "打开失败", "找不到流信息。");
            return;
        default:
            QMessageBox::warning(this, "打开失败", "未知原因打开失败。");
            return;
        }

        //显示文件信息
        auto fps = decode.fps();
        log("FPS: " + QString::number(double(fps.num) / double(fps.den), 'f', 2));
        auto width = decode.width();
        auto height = decode.height();
        log("Size: " + QString::number(width) + " x " + QString::number(height));

        //设置UI上的FPS
        ui->comboBoxFPS->setCurrentText(QString::number(double(fps.num) / double(fps.den), 'f', 0));    //示波器输出的fps与视频的不同，因为如果一个场景点数过多，则需要更低fps（实际就算点数过多，也会完成一次刷新，只是会丢帧，但是其实也没关系，所以ui上的fps设置主要是是为了预留更合适的音频缓冲区而设定）

        //设置缩放
        int value = width > height ? 256 * 100 / width : 256 * 100 / height;
        value = value <= 100 ? value * 10 : value + 900;
        ScaleXY = true;
        ui->horizontalSliderScaleX->setValue(value);

        //设置状态
        state = Ready;
    }
}

void MainWindow::on_pushButtonPlay_clicked()
{
    if(!decode.isReady())
    {
        QMessageBox::warning(this, "播放失败", "请先打开文件。");
        return;
    }

    switch (state) {
    case Inited:
        QMessageBox::warning(this, "播放失败", "请先打开文件。");
        return;
    case Pause:
        state = Play;
        ui->pushButtonPlay->setText("暂停");
        return;
    case Play:
        state = Pause;
        ui->pushButtonPlay->setText("播放");
        return;
    case Stop:
        return;
    case Ready:
        break;
    }

    //根据帧率设置音频缓冲区
    auto fps = decode.fps();

    int out_size = MAX_AUDIO_FRAME_SIZE*2;
    uint8_t *play_buf = nullptr;
    play_buf = reinterpret_cast<uint8_t*>(av_malloc(size_t(out_size)));

    //解码器启动
    decode.start();

    //示波器输出启动
    oscilloscope.start();

    //计时器
    QElapsedTimer time;
    time.start();
    int i = 0;

    //状态设置
    state = Play;
    ui->pushButtonPlay->setText("暂停");

    while(1)
    {
        if(state == Stop)   //停止检测
        {
//            decode.stop();
            oscilloscope.stop();
            return;
        }

        if(1000 * double(i) * double(fps.den) / double(fps.num) < time.elapsed())
        {
            //qDebug() << double(time.elapsed()) / 1000;    //显示时间
            if(state == Play)  //在播放模式
            {
                if((!decode.video.isEmpty()) && (!decode.videoEdge.isEmpty()) && (!decode.points.isEmpty()))
                {
                    //刷新视频图像
                    ui->videoViewer->image = decode.video.dequeue();   //设置视频新图像
                    ui->videoViewer->update();  //刷新视频图像
                    ui->videoViewerEdge->image = decode.videoEdge.dequeue();   //设置视频新图像
                    ui->videoViewerEdge->update();  //刷新视频图像

                    //输出音频

                    //刷新示波器输出
                    oscilloscope.setPoints(decode.points.dequeue());
                }
                else
                    log("丢帧");
            }

            i++;
        }
        //QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents();
        QThread::msleep(1000 * double(fps.den) / double(fps.num) / 10);   //休息 每帧时间/10 ms
    }
}

void MainWindow::on_pushButtonTest_clicked()
{
    if(!ui->pushButtonTest->isChecked())
    {
        oscilloscope.stop();
        ui->pushButtonTest->setText("测试输出");
        return;
    }

    //示波器输出启动
    oscilloscope.start();

    //设置波
    QVector<Point> points(0x20);
    points[0x00].x=0x00;
    points[0x01].x=0x10;
    points[0x02].x=0x20;
    points[0x03].x=0x30;
    points[0x04].x=0x40;
    points[0x05].x=0x50;
    points[0x06].x=0x60;
    points[0x07].x=0x70;
    points[0x08].x=0x80;
    points[0x09].x=0x90;
    points[0x0a].x=0xa0;
    points[0x0b].x=0xb0;
    points[0x0c].x=0xc0;
    points[0x0d].x=0xd0;
    points[0x0e].x=0xe0;
    points[0x0f].x=0xf0;
    points[0x10].x=0xff;
    points[0x11].x=0xf0;
    points[0x12].x=0xe0;
    points[0x13].x=0xd0;
    points[0x14].x=0xc0;
    points[0x15].x=0xb0;
    points[0x16].x=0xa0;
    points[0x17].x=0x90;
    points[0x18].x=0x80;
    points[0x19].x=0x70;
    points[0x1a].x=0x60;
    points[0x1b].x=0x50;
    points[0x1c].x=0x40;
    points[0x1d].x=0x30;
    points[0x1e].x=0x20;
    points[0x1f].x=0x10;

    points[0x00].y=0x80;
    points[0x01].y=0x90;
    points[0x02].y=0xa0;
    points[0x03].y=0xb0;
    points[0x04].y=0xc0;
    points[0x05].y=0xd0;
    points[0x06].y=0xe0;
    points[0x07].y=0xf0;
    points[0x08].y=0xff;
    points[0x09].y=0xf0;
    points[0x0a].y=0xe0;
    points[0x0b].y=0xd0;
    points[0x0c].y=0xc0;
    points[0x0d].y=0xb0;
    points[0x0e].y=0xa0;
    points[0x0f].y=0x90;
    points[0x10].y=0x80;
    points[0x11].y=0x70;
    points[0x12].y=0x60;
    points[0x13].y=0x50;
    points[0x14].y=0x40;
    points[0x15].y=0x30;
    points[0x16].y=0x20;
    points[0x17].y=0x10;
    points[0x18].y=0x00;
    points[0x19].y=0x10;
    points[0x1a].y=0x20;
    points[0x1b].y=0x30;
    points[0x1c].y=0x40;
    points[0x1d].y=0x50;
    points[0x1e].y=0x60;
    points[0x1f].y=0x70;

    oscilloscope.setPoints(points);

    ui->pushButtonTest->setText("停止测试");
}

void MainWindow::on_comboBoxList_activated(int index)
{
    //示波器
    if (!oscilloscope.setAudioDevice(audioDeviceList[index]))
    {
        QMessageBox msgBox;
        msgBox.setText("音频输出设备不支持当前设置。");
        msgBox.exec();
        return;
    }
}

void MainWindow::on_comboBoxRate_currentTextChanged(const QString &arg1)
{
    int rate = arg1.toInt();
    if(rate <= 0)
    {
        rate = 1;
    }
    //示波器
    if (!oscilloscope.setSampleRate(rate))
    {
        QMessageBox msgBox;
        msgBox.setText("音频输出设备不支持当前设置。");
        msgBox.exec();
        return;
    }
}

void MainWindow::on_spinBoxChannel_valueChanged(int arg1)
{
    ui->spinBoxChannelX->setMaximum(arg1 - 1);
    ui->spinBoxChannelY->setMaximum(arg1 - 1);

    //示波器
    if (!oscilloscope.setChannelCount(arg1))
    {
        QMessageBox msgBox;
        msgBox.setText("音频输出设备不支持当前设置。");
        msgBox.exec();
        return;
    }
}

void MainWindow::on_spinBoxChannelX_valueChanged(int arg1)
{
    //示波器
    oscilloscope.setChannelX(arg1);
}

void MainWindow::on_spinBoxChannelY_valueChanged(int arg1)
{
    //示波器
    oscilloscope.setChannelY(arg1);
}

void MainWindow::on_comboBoxFPS_currentTextChanged(const QString &arg1)
{
    int fps = arg1.toInt();
    if(fps <= 0)
    {
        fps = 1;
    }
    //示波器
    oscilloscope.setFPS(fps);
}

void MainWindow::on_horizontalSliderScaleX_valueChanged(int value)
{
    if(ScaleXY) ui->horizontalSliderScaleY->setValue(value);    //等比缩放
    value = value <= 1000 ? value / 10 : value - 900;
    decode.setScaleX(value);
    ui->labelScaleX->setText("缩放X：" + QString::number(value) + " %");
}

void MainWindow::on_horizontalSliderScaleY_valueChanged(int value)
{
    value = value <= 1000 ? value / 10 : value - 900;
    decode.setScaleY(value);
    ui->labelScaleY->setText("缩放Y：" + QString::number(value) + " %");
}

void MainWindow::on_horizontalSliderMoveX_valueChanged(int value)
{
    decode.setMoveX(value);
    ui->labelMoveX->setText("偏移X：" + QString::number(value));
}

void MainWindow::on_horizontalSliderMoveY_valueChanged(int value)
{
    decode.setMoveY(value);
    ui->labelMoveY->setText("偏移Y：" + QString::number(value));
}

void MainWindow::on_horizontalSliderEdge_valueChanged(int value)
{
    decode.setEdge(value);
    ui->labelEdge->setText("边缘阈值：" + QString::number(value));
}

void MainWindow::on_horizontalSliderScaleY_sliderReleased()
{
    ScaleXY = (ui->horizontalSliderScaleX->value() == ui->horizontalSliderScaleY->value());
}
