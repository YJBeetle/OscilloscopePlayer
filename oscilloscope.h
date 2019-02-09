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
    explicit Oscilloscope(QAudioDeviceInfo info, int sampleRate, int channelCount, int channelX, int channelY, int fps, QObject *parent = nullptr);
    ~Oscilloscope();

    void run();
    int stop(int time = 0);
    bool state();
    int isFormatSupported();

    QVector<Point> points;
    bool refresh = false;
signals:

public slots:

private:
    bool stopMe = false;
    bool stateStart = false;

    QAudioDeviceInfo audioDeviceInfo;
    QAudioFormat format;
    QAudioOutput* output = nullptr;
    QIODevice* device = nullptr;
    QByteArray buffer;
    int bufferDataSize = 0;
//    QByteArray bufferFrame;

    int sampleRate;
    int channelCount;
    int channelX;
    int channelY;
    int fps;
};

#endif // OSCILLOSCOPE_H
