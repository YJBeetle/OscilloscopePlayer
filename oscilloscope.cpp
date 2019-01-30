#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QObject *parent) : QThread(parent)
{

}

Oscilloscope::~Oscilloscope()
{
    stop();
    if(output){
        output->stop();
        free(output);
    }
    if(buffer) free(buffer);
}

void Oscilloscope::run()
{
    stateStart = true;
    while(1)
    {
        if(device && output && bufferSize)
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
            stopMe = false;
            stateStart = false;
            return;
        }
    }
}

int Oscilloscope::stop(int time)
{
    if(stateStart && (!stopMe))
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

int Oscilloscope::set(QAudioDeviceInfo info, int sampleRate, int sampleSize, int channelCount, int channelX, int channelY, int fps)
{
    //如果正在运行先停止
    stop();

    if(output)
    {
        output->stop();
        device = nullptr;
        free(output);
        output = nullptr;
    }
    if(buffer){
        free(buffer);
        buffer = nullptr;
    }
    bufferSize = 0;

    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(sampleRate);
    format.setSampleSize(sampleSize);
    format.setChannelCount(channelCount);

    this->channelX = channelX;
    this->channelY = channelY;
    this->fps = fps;

    if (!info.isFormatSupported(format)) {
        return 1;
    }

    output = new QAudioOutput(info, format);
    device = output->start();

    bufferSize = sampleRate / fps + 1;
    buffer = new char[bufferSize];

    return 0;
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
    if((!device) && (!output)) return 1;
    if(!bufferSize) return 2;

    buffer[0x00]=0x00;
    buffer[0x02]=0x10;
    buffer[0x04]=0x20;
    buffer[0x06]=0x30;
    buffer[0x08]=0x40;
    buffer[0x0a]=0x50;
    buffer[0x0c]=0x60;
    buffer[0x0e]=0x70;
    buffer[0x10]=0x79;
    buffer[0x12]=0x70;
    buffer[0x14]=0x60;
    buffer[0x16]=0x50;
    buffer[0x18]=0x40;
    buffer[0x1a]=0x30;
    buffer[0x1c]=0x20;
    buffer[0x1e]=0x10;
    buffer[0x20]=-0x00;
    buffer[0x22]=-0x10;
    buffer[0x24]=-0x20;
    buffer[0x26]=-0x30;
    buffer[0x28]=-0x40;
    buffer[0x2a]=-0x50;
    buffer[0x2c]=-0x60;
    buffer[0x2e]=-0x70;
    buffer[0x30]=-0x80;
    buffer[0x32]=-0x70;
    buffer[0x34]=-0x60;
    buffer[0x36]=-0x50;
    buffer[0x38]=-0x40;
    buffer[0x3a]=-0x30;
    buffer[0x3c]=-0x20;
    buffer[0x3e]=-0x10;

    buffer[0x01]=0x79;
    buffer[0x03]=0x70;
    buffer[0x05]=0x60;
    buffer[0x07]=0x50;
    buffer[0x09]=0x40;
    buffer[0x0b]=0x30;
    buffer[0x0d]=0x20;
    buffer[0x0f]=0x10;
    buffer[0x11]=-0x00;
    buffer[0x13]=-0x10;
    buffer[0x15]=-0x20;
    buffer[0x17]=-0x30;
    buffer[0x19]=-0x40;
    buffer[0x1b]=-0x50;
    buffer[0x1d]=-0x60;
    buffer[0x1f]=-0x70;
    buffer[0x21]=-0x80;
    buffer[0x23]=-0x70;
    buffer[0x25]=-0x60;
    buffer[0x27]=-0x50;
    buffer[0x29]=-0x40;
    buffer[0x2b]=-0x30;
    buffer[0x2d]=-0x20;
    buffer[0x2f]=-0x10;
    buffer[0x31]=0x00;
    buffer[0x33]=0x10;
    buffer[0x35]=0x20;
    buffer[0x37]=0x30;
    buffer[0x39]=0x40;
    buffer[0x3b]=0x50;
    buffer[0x3d]=0x60;
    buffer[0x3f]=0x70;

    bufferDataSize = 64;

    this->start();

    return 0;
}
