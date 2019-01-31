#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int channelCount, int channelX, int channelY, int fps, QObject *parent) : QThread(parent)
{
    this->audioDeviceInfo = audioDeviceInfo;

    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);

    this->channelCount = channelCount;
    this->channelX = channelX;
    this->channelY = channelY;
    this->fps = fps;
    points.resize(sampleRate / fps);
    buffer.resize(sampleRate / fps * channelCount * 2);
    output = new QAudioOutput(audioDeviceInfo, format, this);
    device = output->start();
}

Oscilloscope::~Oscilloscope()
{
    stop();
    output->stop();
}

void Oscilloscope::run()
{
    stateStart = true;
    while(1)
    {
        if(device && output && bufferDataSize)
        {
            device->write(buffer, bufferDataSize);
        }
        if(refresh)
        {
            if(pointsDataSize > points.length()) pointsDataSize = points.length();

            unsigned char* bufferPtr = reinterpret_cast<unsigned char *>(buffer.data());
            for(int i = 0; i < pointsDataSize; i++)
            {
                qToLittleEndian<qint16>(points.at(i).x, bufferPtr + (i * channelCount + channelX) * 2);
                qToLittleEndian<qint16>(points.at(i).y, bufferPtr + (i * channelCount + channelY) * 2);
            }

            bufferDataSize = pointsDataSize * channelCount * 2;
            if(bufferDataSize > buffer.length()) bufferDataSize = buffer.length();
            refresh = false;
        }
        if(stopMe)
        {
            stopMe = false;
            stateStart = false;
            return;
        }
    }
}

int Oscilloscope::stop(int time)
{
    if(stateStart)
    {
        stopMe = true;

        int i = 0;
        while(stateStart)
        {
            QCoreApplication::processEvents();
            i++;
            if(time && (i > time)) return 1;
        }
    }
    return 0;
}

bool Oscilloscope::state()
{
    return stateStart;
}

int Oscilloscope::isFormatSupported()
{
    return audioDeviceInfo.isFormatSupported(format);
}
