#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int channelCount, int channelX, int channelY, int fps, QObject *parent) : QThread(parent), audioDeviceInfo(audioDeviceInfo), sampleRate(sampleRate), channelCount(channelCount), channelX(channelX), channelY(channelY), fps(fps)
{
    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);

    if(this->fps == 0) this->fps = 1;
    points.resize(this->sampleRate / this->fps);    //最大的一次的数据量是 采样率/贞率
    buffer.resize(this->sampleRate / this->fps * this->channelCount * 2);
    output = new QAudioOutput(this->audioDeviceInfo, format, this);
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
        //退出检测
        if(stopMe || output->state() == QAudio::StoppedState)
        {
            stopMe = false;
            stateStart = false;
            return;
        }

        //如果需要刷新，先刷新
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

        //输出
        if(device && output && bufferDataSize)
        {
            if((output->bufferSize() - output->bytesFree()) * 10 / 2 / channelCount * fps / sampleRate < 10 && //如果剩余数据可播放的时间小于fps的倒数(*10是为了不想用浮点数)
                    output->bytesFree() > bufferDataSize)    //且剩余缓冲大于即将写入的数据大小 (一般情况下不会出现这个情况，这里只是以防万一)
                device->write(buffer, bufferDataSize);
            else    //否则说明缓冲区时间一定大于fps的倒数，所以可以休息一会
                QTest::qSleep(1000 / fps / 2);  //这里/2为了更保守一些
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
