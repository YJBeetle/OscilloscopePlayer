#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <QThread>
#include <QCoreApplication>
#include <QVector>
#include <QByteArray>
#include <QtEndian>
#include <QTest>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

struct Point {
    quint8 x = 0;
    quint8 y = 0;
};

class Oscilloscope : public QThread
{
    Q_OBJECT
public:
    explicit Oscilloscope(QObject *parent = nullptr);
    ~Oscilloscope();

    int set(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int channelCount, int channelX, int channelY, int fps);
    int setAudioDeviceInfo(const QAudioDeviceInfo audioDeviceInfo);
    int setSampleRate(int sampleRate);
    int setChannelCount(int channelCount);
    void setChannelX(int channelX);
    void setChannelY(int channelY);
    void setFPS(int fps);
    void setPoints(const QVector<Point> points);
    void run();
    int stop(int time = 0);
    bool state();

signals:

public slots:

private:
    void setBuffer();  //根据参数重新分配buffer内存
    int isFormatSupported();

    bool stopMe = false;
    bool stateStart = false;

    QAudioDeviceInfo audioDeviceInfo;
    int sampleRate;
    int channelCount;
    int channelX;
    int channelY;
    int fps;
    QAudioFormat format;

    int bufferMaxSize = 0;
    char* buffer = nullptr;
    int bufferLen = 0;
    char* bufferRefresh = nullptr;
    int bufferRefreshLen = 0;
    QAtomicInteger<bool> refresh = false;       //当 refresh == true 时候，线程将会用 bufferRefresh 的 bufferRefreshLen 长度内容复制到 buffer 中去
    //QByteArray bufferFrame;   //框

};

#endif // OSCILLOSCOPE_H
