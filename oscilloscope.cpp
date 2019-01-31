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

int Oscilloscope::test()
{

    points[0x00].x=0x0000;
    points[0x01].x=0x1000;
    points[0x02].x=0x2000;
    points[0x03].x=0x3000;
    points[0x04].x=0x4000;
    points[0x05].x=0x5000;
    points[0x06].x=0x6000;
    points[0x07].x=0x7000;
    points[0x08].x=0x7fff;
    points[0x09].x=0x7000;
    points[0x0a].x=0x6000;
    points[0x0b].x=0x5000;
    points[0x0c].x=0x4000;
    points[0x0d].x=0x3000;
    points[0x0e].x=0x2000;
    points[0x0f].x=0x1000;
    points[0x10].x=-0x0000;
    points[0x11].x=-0x1000;
    points[0x12].x=-0x2000;
    points[0x13].x=-0x3000;
    points[0x14].x=-0x4000;
    points[0x15].x=-0x5000;
    points[0x16].x=-0x6000;
    points[0x17].x=-0x7000;
    points[0x18].x=-0x8000;
    points[0x19].x=-0x7000;
    points[0x1a].x=-0x6000;
    points[0x1b].x=-0x5000;
    points[0x1c].x=-0x4000;
    points[0x1d].x=-0x3000;
    points[0x1e].x=-0x2000;
    points[0x1f].x=-0x1000;

    points[0x00].y=0x7fff;
    points[0x01].y=0x7000;
    points[0x02].y=0x6000;
    points[0x03].y=0x5000;
    points[0x04].y=0x4000;
    points[0x05].y=0x3000;
    points[0x06].y=0x2000;
    points[0x07].y=0x1000;
    points[0x08].y=-0x0000;
    points[0x09].y=-0x1000;
    points[0x0a].y=-0x2000;
    points[0x0b].y=-0x3000;
    points[0x0c].y=-0x4000;
    points[0x0d].y=-0x5000;
    points[0x0e].y=-0x6000;
    points[0x0f].y=-0x7000;
    points[0x10].y=-0x8000;
    points[0x11].y=-0x7000;
    points[0x12].y=-0x6000;
    points[0x13].y=-0x5000;
    points[0x14].y=-0x4000;
    points[0x15].y=-0x3000;
    points[0x16].y=-0x2000;
    points[0x17].y=-0x1000;
    points[0x18].y=0x0000;
    points[0x19].y=0x1000;
    points[0x1a].y=0x2000;
    points[0x1b].y=0x3000;
    points[0x1c].y=0x4000;
    points[0x1d].y=0x5000;
    points[0x1e].y=0x6000;
    points[0x1f].y=0x7000;

    pointsDataSize = 0x20;
    refresh = true;

    this->start();

    return 0;
}
