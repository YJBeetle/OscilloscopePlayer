#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    audioDeviceInfoList = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    //列出音频设备
    int i = 0;
    foreach (const QAudioDeviceInfo &audioDeviceInfo, audioDeviceInfoList)
    {
        this->ui->comboBoxList->addItem(audioDeviceInfo.deviceName());
        if(audioDeviceInfo == QAudioDeviceInfo::defaultOutputDevice())
            this->ui->comboBoxList->setCurrentIndex(i);
        i++;
    }

    //设置最大行数
    ui->textEditInfo->document()->setMaximumBlockCount(100);
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

        decode = new Decode();
        switch(decode->open(path))
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
        auto fps = decode->fps();
        log("FPS: " + QString::number(double(fps.num) / double(fps.den), 'f', 2));

        //设置UI上的FPS
        ui->comboBoxFPS->setCurrentText(QString::number(double(fps.num) / double(fps.den), 'f', 0));    //示波器输出的fps与视频的不同，因为如果一个场景点数过多，则需要更低fps（实际就算点数过多，也会完成一次刷新，只是会丢帧，但是其实也没关系，所以ui上的fps设置主要是是为了预留更合适的音频缓冲区而设定）
    }
}

void MainWindow::on_pushButtonPlay_clicked()
{
    if(oscilloscope)
    {
        ui->pushButtonPlay->setText("播放");
        return;
    }

    if(!decode)
    {
        QMessageBox::warning(this, "播放失败", "请先打开文件。");
        return;
    }

    //设置音频输出
    QAudioFormat audioFormat;
    //audioFormat = QAudioDeviceInfo::defaultOutputDevice().preferredFormat();
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(2);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setSampleSize(16);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(audioFormat)) {
        QMessageBox::warning(this, "播放失败", "不支持的音频设置。");
        return;
    }
    QAudioOutput audioOutput(audioFormat);

    //根据帧率设置音频缓冲区
    auto fps = decode->fps();
    QIODevice* audioDevice = audioOutput.start();

    int out_size = MAX_AUDIO_FRAME_SIZE*2;
    uint8_t *play_buf = nullptr;
    play_buf = reinterpret_cast<uint8_t*>(av_malloc(size_t(out_size)));

    //解码器启动
    decode->start();

    //示波器
    oscilloscope = new Oscilloscope(audioDeviceInfoList[ui->comboBoxList->currentIndex()],
                        ui->comboBoxRate->currentText().toInt(),
                        ui->spinBoxChannel->value(),
                        ui->spinBoxChannelX->value(),
                        ui->spinBoxChannelY->value(),
                        ui->comboBoxFPS->currentText().toInt());
    //检查示波器输出声卡兼容性
    if (!oscilloscope->isFormatSupported())
    {
        QMessageBox msgBox;
        msgBox.setText("所选输出设备不支持此设置。");
        msgBox.exec();
        return;
    }
    //示波器输出启动
    oscilloscope->start();


    //计时器
    QTime time;
    time.start();
    int i = 0;

    //按钮字样
    ui->pushButtonPlay->setText("暂停");

    while(1)
    {
        if(1000 * double(i) * double(fps.den) / double(fps.num) < time.elapsed())
        {
            //qDebug() << double(time.elapsed()) / 1000;    //显示时间
            if(!decode->video.isEmpty())
            {
                //刷新视频图像
                ui->videoViewer->image = decode->video.dequeue();   //设置视频新图像
                ui->videoViewer->update();  //刷新视频图像

                //输出音频

                //刷新示波器输出
                if(!decode->points.isEmpty())
                {
                    oscilloscope->points = decode->points.dequeue();
                    oscilloscope->pointsDataSize = 4;
                    oscilloscope->refresh = true;

                    //屏幕绘制示波器的模拟

                }
                else
                    log("示波器丢帧");

                i++;
            }
            else
                log("丢帧");
        }
        //QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents();
        QTest::qSleep(1000 * double(fps.den) / double(fps.num) / 10);   //休息 每帧时间/10 ms
    }
}

void MainWindow::on_pushButtonTest_clicked()
{
    if(oscilloscopeTest)
    {
        oscilloscopeTest->stop();
        free(oscilloscopeTest);
        oscilloscopeTest = nullptr;
        ui->pushButtonTest->setText("测试输出");
        return;
    }

    oscilloscopeTest = new Oscilloscope(audioDeviceInfoList[this->ui->comboBoxList->currentIndex()],
                        this->ui->comboBoxRate->currentText().toInt(),
                        this->ui->spinBoxChannel->value(),
                        this->ui->spinBoxChannelX->value(),
                        this->ui->spinBoxChannelY->value(),
                        this->ui->comboBoxFPS->currentText().toInt());

    if (!oscilloscopeTest->isFormatSupported())
    {
        QMessageBox msgBox;
        msgBox.setText("所选输出设备不支持此设置。");
        msgBox.exec();
        return;
    }

    oscilloscopeTest->start();

    oscilloscopeTest->points[0x00].x=0x0000;
    oscilloscopeTest->points[0x01].x=0x1000;
    oscilloscopeTest->points[0x02].x=0x2000;
    oscilloscopeTest->points[0x03].x=0x3000;
    oscilloscopeTest->points[0x04].x=0x4000;
    oscilloscopeTest->points[0x05].x=0x5000;
    oscilloscopeTest->points[0x06].x=0x6000;
    oscilloscopeTest->points[0x07].x=0x7000;
    oscilloscopeTest->points[0x08].x=0x7fff;
    oscilloscopeTest->points[0x09].x=0x7000;
    oscilloscopeTest->points[0x0a].x=0x6000;
    oscilloscopeTest->points[0x0b].x=0x5000;
    oscilloscopeTest->points[0x0c].x=0x4000;
    oscilloscopeTest->points[0x0d].x=0x3000;
    oscilloscopeTest->points[0x0e].x=0x2000;
    oscilloscopeTest->points[0x0f].x=0x1000;
    oscilloscopeTest->points[0x10].x=-0x0000;
    oscilloscopeTest->points[0x11].x=-0x1000;
    oscilloscopeTest->points[0x12].x=-0x2000;
    oscilloscopeTest->points[0x13].x=-0x3000;
    oscilloscopeTest->points[0x14].x=-0x4000;
    oscilloscopeTest->points[0x15].x=-0x5000;
    oscilloscopeTest->points[0x16].x=-0x6000;
    oscilloscopeTest->points[0x17].x=-0x7000;
    oscilloscopeTest->points[0x18].x=-0x8000;
    oscilloscopeTest->points[0x19].x=-0x7000;
    oscilloscopeTest->points[0x1a].x=-0x6000;
    oscilloscopeTest->points[0x1b].x=-0x5000;
    oscilloscopeTest->points[0x1c].x=-0x4000;
    oscilloscopeTest->points[0x1d].x=-0x3000;
    oscilloscopeTest->points[0x1e].x=-0x2000;
    oscilloscopeTest->points[0x1f].x=-0x1000;

    oscilloscopeTest->points[0x00].y=0x7fff;
    oscilloscopeTest->points[0x01].y=0x7000;
    oscilloscopeTest->points[0x02].y=0x6000;
    oscilloscopeTest->points[0x03].y=0x5000;
    oscilloscopeTest->points[0x04].y=0x4000;
    oscilloscopeTest->points[0x05].y=0x3000;
    oscilloscopeTest->points[0x06].y=0x2000;
    oscilloscopeTest->points[0x07].y=0x1000;
    oscilloscopeTest->points[0x08].y=-0x0000;
    oscilloscopeTest->points[0x09].y=-0x1000;
    oscilloscopeTest->points[0x0a].y=-0x2000;
    oscilloscopeTest->points[0x0b].y=-0x3000;
    oscilloscopeTest->points[0x0c].y=-0x4000;
    oscilloscopeTest->points[0x0d].y=-0x5000;
    oscilloscopeTest->points[0x0e].y=-0x6000;
    oscilloscopeTest->points[0x0f].y=-0x7000;
    oscilloscopeTest->points[0x10].y=-0x8000;
    oscilloscopeTest->points[0x11].y=-0x7000;
    oscilloscopeTest->points[0x12].y=-0x6000;
    oscilloscopeTest->points[0x13].y=-0x5000;
    oscilloscopeTest->points[0x14].y=-0x4000;
    oscilloscopeTest->points[0x15].y=-0x3000;
    oscilloscopeTest->points[0x16].y=-0x2000;
    oscilloscopeTest->points[0x17].y=-0x1000;
    oscilloscopeTest->points[0x18].y=0x0000;
    oscilloscopeTest->points[0x19].y=0x1000;
    oscilloscopeTest->points[0x1a].y=0x2000;
    oscilloscopeTest->points[0x1b].y=0x3000;
    oscilloscopeTest->points[0x1c].y=0x4000;
    oscilloscopeTest->points[0x1d].y=0x5000;
    oscilloscopeTest->points[0x1e].y=0x6000;
    oscilloscopeTest->points[0x1f].y=0x7000;

    oscilloscopeTest->pointsDataSize = 0x20;
    oscilloscopeTest->refresh = true;

    ui->pushButtonTest->setText("停止测试");
}

void MainWindow::on_spinBoxChannel_valueChanged(int arg1)
{
    ui->spinBoxChannelX->setMaximum(arg1 - 1);
    ui->spinBoxChannelY->setMaximum(arg1 - 1);
}
