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

    this->sampleRate = sampleRate;
    this->channelCount = channelCount;
    this->channelX = channelX;
    this->channelY = channelY;
    this->fps = fps;
    points.resize(sampleRate / fps);
    buffer.resize(sampleRate / fps * channelCount * 2);
    output = new QAudioOutput(audioDeviceInfo, format, this);
    if(output->bufferSize() < buffer.length() * 2) output->setBufferSize(buffer.length() * 2);  //如果音频缓冲区小于最大buffer的两倍则扩大之。
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
        if(stopMe || output->state() == QAudio::StoppedState)
        {
            stopMe = false;
            stateStart = false;
            return;
        }

        if(device && output && bufferDataSize)
        {
            if((output->bufferSize() - output->bytesFree()) * 10 / 2 / channelCount * fps / sampleRate < 10 && //如果剩余数据可播放的时间小于fps的倒数(*10是为了不想用浮点数)
                    output->bytesFree() > bufferDataSize)    //如果剩余缓冲大于即将写入的数据大小
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
