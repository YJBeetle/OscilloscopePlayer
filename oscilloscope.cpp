#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int sampleSize, int channelCount, int channelX, int channelY, int fps, QObject *parent) : QThread(parent)
{
    this->audioDeviceInfo = audioDeviceInfo;

    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(sampleRate);
    format.setSampleSize(sampleSize);
    format.setChannelCount(channelCount);

    this->channelX = channelX;
    this->channelY = channelY;
    this->fps = fps;
    bufferSize = sampleRate / fps + 1;
}

Oscilloscope::~Oscilloscope()
{
    stop();
}

void Oscilloscope::run()
{
    output = new QAudioOutput(audioDeviceInfo, format);
    device = output->start();
    buffer = new char[bufferSize];

    stateStart = true;
    while(1)
    {
        if(device && output && bufferSize && bufferDataSize)
        {
            device->write(buffer, bufferDataSize);
        }
        if(refresh)
        {
            memcpy(buffer, pointData, size_t(pointDataSize));
            bufferDataSize = pointDataSize;
            refresh = false;
        }
        if(stopMe)
        {
            output->stop();
            free(output);
            free(buffer);
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

int Oscilloscope::setPointData(char* pointData, int size)
{
    if(size > bufferSize)
        return 1;

    this->pointData = pointData;
    this->pointDataSize = size;
    refresh = true;

    return 0;
}

int Oscilloscope::test()
{
    pointData = new char[64];

    pointData[0x00]=0x00;
    pointData[0x02]=0x10;
    pointData[0x04]=0x20;
    pointData[0x06]=0x30;
    pointData[0x08]=0x40;
    pointData[0x0a]=0x50;
    pointData[0x0c]=0x60;
    pointData[0x0e]=0x70;
    pointData[0x10]=0x79;
    pointData[0x12]=0x70;
    pointData[0x14]=0x60;
    pointData[0x16]=0x50;
    pointData[0x18]=0x40;
    pointData[0x1a]=0x30;
    pointData[0x1c]=0x20;
    pointData[0x1e]=0x10;
    pointData[0x20]=-0x00;
    pointData[0x22]=-0x10;
    pointData[0x24]=-0x20;
    pointData[0x26]=-0x30;
    pointData[0x28]=-0x40;
    pointData[0x2a]=-0x50;
    pointData[0x2c]=-0x60;
    pointData[0x2e]=-0x70;
    pointData[0x30]=-0x80;
    pointData[0x32]=-0x70;
    pointData[0x34]=-0x60;
    pointData[0x36]=-0x50;
    pointData[0x38]=-0x40;
    pointData[0x3a]=-0x30;
    pointData[0x3c]=-0x20;
    pointData[0x3e]=-0x10;

    pointData[0x01]=0x79;
    pointData[0x03]=0x70;
    pointData[0x05]=0x60;
    pointData[0x07]=0x50;
    pointData[0x09]=0x40;
    pointData[0x0b]=0x30;
    pointData[0x0d]=0x20;
    pointData[0x0f]=0x10;
    pointData[0x11]=-0x00;
    pointData[0x13]=-0x10;
    pointData[0x15]=-0x20;
    pointData[0x17]=-0x30;
    pointData[0x19]=-0x40;
    pointData[0x1b]=-0x50;
    pointData[0x1d]=-0x60;
    pointData[0x1f]=-0x70;
    pointData[0x21]=-0x80;
    pointData[0x23]=-0x70;
    pointData[0x25]=-0x60;
    pointData[0x27]=-0x50;
    pointData[0x29]=-0x40;
    pointData[0x2b]=-0x30;
    pointData[0x2d]=-0x20;
    pointData[0x2f]=-0x10;
    pointData[0x31]=0x00;
    pointData[0x33]=0x10;
    pointData[0x35]=0x20;
    pointData[0x37]=0x30;
    pointData[0x39]=0x40;
    pointData[0x3b]=0x50;
    pointData[0x3d]=0x60;
    pointData[0x3f]=0x70;

    pointDataSize = 64;
    refresh = true;

    this->start();

    return 0;
}
