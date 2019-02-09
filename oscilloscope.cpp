#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int channelCount, int channelX, int channelY, int fps, QObject *parent) : QThread(parent), audioDeviceInfo(audioDeviceInfo), sampleRate(sampleRate), channelCount(channelCount), channelX(channelX), channelY(channelY), fps(fps)
{
    format.setCodec("audio/pcm");
    format.setSampleSize(8);
    format.setSampleType(QAudioFormat::UnSignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);

    if(this->fps == 0) this->fps = 1;
//    points.resize(this->sampleRate / this->fps);    //最大的一次的数据量是 采样率/贞率
    buffer.resize(this->sampleRate / this->fps * this->channelCount);
//    bufferFrame.resize(32 * this->channelCount);
//    for(int i = 0; i < 8; i++)
//    {
//        bufferFrame[i * channelCount + channelX] = char(i * 32);
//        bufferFrame[i * channelCount + channelY] = char(0);
//        bufferFrame[(8 + i) * channelCount + channelX] = char(255);
//        bufferFrame[(8 + i) * channelCount + channelY] = char(i * 32);
//        bufferFrame[(16 + i) * channelCount + channelX] = char((7 - i) * 32);
//        bufferFrame[(16 + i) * channelCount + channelY] = char(255);
//        bufferFrame[(24 + i) * channelCount + channelX] = char(0);
//        bufferFrame[(24 + i) * channelCount + channelY] = char((7 - i) * 32);
//    }
//    bufferFrame.resize(256 * 4 * this->channelCount);
//    for(int i = 0; i < 256; i++)
//    {
//        bufferFrame[i * channelCount + channelX] = char(i);
//        bufferFrame[i * channelCount + channelY] = char(0);
//        bufferFrame[(256 + i) * channelCount + channelX] = char(255);
//        bufferFrame[(256 + i) * channelCount + channelY] = char(i);
//        bufferFrame[(256 * 2 + i) * channelCount + channelX] = char(255 - i);
//        bufferFrame[(256 * 2 + i) * channelCount + channelY] = char(255);
//        bufferFrame[(256 * 3 + i) * channelCount + channelX] = char(0);
//        bufferFrame[(256 * 3 + i) * channelCount + channelY] = char(255 - i);
//    }
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
            for(int i = 0; i < points.length() && i * channelCount < buffer.length(); i++)
            {
                buffer[i * channelCount + channelX] = char(points.at(i).x);
                buffer[i * channelCount + channelY] = char(-1 - points.at(i).y);
            }

            bufferDataSize = points.length() * channelCount;
            if(bufferDataSize > buffer.length()) bufferDataSize = buffer.length();
            refresh = false;
        }

        //输出4个边角防止抖动
        //输出
        if(device && output && bufferDataSize)
        {
            if((output->bufferSize() - output->bytesFree()) * 10 / channelCount * fps / sampleRate < 10 && //如果剩余数据可播放的时间小于fps的倒数(*10是为了不想用浮点数)
                    output->bytesFree() > bufferDataSize)    //且剩余缓冲大于即将写入的数据大小 (一般情况下不会出现这个情况，这里只是以防万一)
            {
//                device->write(bufferFrame, bufferFrame.length());
                device->write(buffer, bufferDataSize);
            }
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
