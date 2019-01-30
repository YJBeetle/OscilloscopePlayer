#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <QThread>
#include <QCoreApplication>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

class Oscilloscope : public QThread
{
    Q_OBJECT
public:
    explicit Oscilloscope(QObject *parent = nullptr);
    ~Oscilloscope();

    void run();
    int stop(int time = 0);
    bool state();

    int set(QAudioDeviceInfo info, int sampleRate, int sampleSize, int channelCount, int channelX, int channelY, int fps);
    int setPointData(char* pointData, int size);
    int test();

signals:

public slots:

private:
    bool stopMe = false;
    bool stateStart = false;

    QAudioFormat format;
    QAudioOutput* output = nullptr;
    QIODevice* device = nullptr;
    char* buffer = nullptr;
    int bufferSize = 0;
    int bufferDataSize = 0;
    char* pointData = nullptr;
    int pointDataSize = 0;
    bool refresh = false;

    int channelX;
    int channelY;
    int fps;
};

#endif // OSCILLOSCOPE_H
