#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QObject *parent) : QThread(parent)
{
    //设置边框数据
    //bufferFrame.resize(32 * this->channelCount);
    //for(int i = 0; i < 8; i++)
    //{
    //    bufferFrame[i * channelCount + channelX] = char(i * 32);
    //    bufferFrame[i * channelCount + channelY] = char(0);
    //    bufferFrame[(8 + i) * channelCount + channelX] = char(255);
    //    bufferFrame[(8 + i) * channelCount + channelY] = char(i * 32);
    //    bufferFrame[(16 + i) * channelCount + channelX] = char((7 - i) * 32);
    //    bufferFrame[(16 + i) * channelCount + channelY] = char(255);
    //    bufferFrame[(24 + i) * channelCount + channelX] = char(0);
    //    bufferFrame[(24 + i) * channelCount + channelY] = char((7 - i) * 32);
    //}
    //bufferFrame.resize(256 * 4 * this->channelCount);
    //for(int i = 0; i < 256; i++)
    //{
    //    bufferFrame[i * channelCount + channelX] = char(i);
    //    bufferFrame[i * channelCount + channelY] = char(0);
    //    bufferFrame[(256 + i) * channelCount + channelX] = char(255);
    //    bufferFrame[(256 + i) * channelCount + channelY] = char(i);
    //    bufferFrame[(256 * 2 + i) * channelCount + channelX] = char(255 - i);
    //    bufferFrame[(256 * 2 + i) * channelCount + channelY] = char(255);
    //    bufferFrame[(256 * 3 + i) * channelCount + channelX] = char(0);
    //    bufferFrame[(256 * 3 + i) * channelCount + channelY] = char(255 - i);
    //}

    //设置输出格式
    format.setCodec("audio/pcm");
    format.setSampleSize(8);
    format.setSampleType(QAudioFormat::UnSignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
}

Oscilloscope::~Oscilloscope()
{
    stop();
}

int Oscilloscope::set(QAudioDeviceInfo audioDeviceInfo, int sampleRate, int channelCount, int channelX, int channelY, int fps)
{
    this->audioDeviceInfo = audioDeviceInfo;
    this->sampleRate = sampleRate;
    format.setSampleRate(sampleRate);
    this->channelCount = channelCount;
    format.setChannelCount(channelCount);
    this->channelX = channelX;
    this->channelY = channelY;
    if(fps == 0)
        this->fps = 1;
    else
        this->fps = fps;
    setBuffer();
    return isFormatSupported();
}

int Oscilloscope::setAudioDeviceInfo(const QAudioDeviceInfo audioDeviceInfo)
{
    this->audioDeviceInfo = audioDeviceInfo;
    return isFormatSupported();
}

int Oscilloscope::setSampleRate(int sampleRate)
{
    this->sampleRate = sampleRate;
    format.setSampleRate(sampleRate);
    setBuffer();
    return isFormatSupported();
}

int Oscilloscope::setChannelCount(int channelCount)
{
    this->channelCount = channelCount;
    format.setChannelCount(channelCount);
    setBuffer();
    return isFormatSupported();
}

void Oscilloscope::setChannelX(int channelX)
{
    this->channelX = channelX;
}

void Oscilloscope::setChannelY(int channelY)
{
    this->channelY = channelY;
}

void Oscilloscope::setFPS(int fps)
{
    if(fps == 0)
        this->fps = 1;
    else
        this->fps = fps;
    setBuffer();
}

void Oscilloscope::setBuffer()  //根据参数重新分配buffer内存
{
    bufferMaxSize = this->sampleRate / this->fps * this->channelCount;  //最大值等于一个帧的时间的按照采样率的数据量
    if(buffer)
        delete buffer;
    buffer = new char[bufferMaxSize];
    if(bufferRefresh)
        delete buffer;
    bufferRefresh = new char[bufferMaxSize];
}

void Oscilloscope::setPoints(const QVector<Point> points)   //根据点数据计算buffer，暂存在bufferRefresh等另一个线程下一次刷新时候将会复制走
{
    for(int i = 0; i < points.length() && i * channelCount < bufferMaxSize; i++)
    {
        bufferRefresh[i * channelCount + channelX] = char(points.at(i).x);
        bufferRefresh[i * channelCount + channelY] = char(-1 - points.at(i).y);
    }

    bufferRefreshLen = points.length() * channelCount;
    if(bufferRefreshLen > bufferMaxSize) bufferRefreshLen = bufferMaxSize;

    refresh = true; //设置需要刷新标记，等待线程刷新
}

int Oscilloscope::isFormatSupported()
{
    return audioDeviceInfo.isFormatSupported(format);
}

void Oscilloscope::run()
{
    auto output = new QAudioOutput(this->audioDeviceInfo, format);
    if(output->bufferSize() < bufferMaxSize * 2) output->setBufferSize(bufferMaxSize * 2);  //如果音频缓冲区小于最大buffer的两倍则扩大之。
    auto device = output->start();

    stateStart = true;
    while(1)
    {
        //退出检测
        if(stopMe)
        {
            output->stop();
            delete output;
            stopMe = false;
            stateStart = false;
            return;
        }

        //如果需要刷新，先刷新
        if(refresh)
        {
            refresh = false;
            memcpy(buffer, bufferRefresh, size_t(bufferRefreshLen));
            bufferLen = bufferRefreshLen;
        }

        //输出
        if(device && output && bufferLen)
        {
            if((output->bufferSize() - output->bytesFree()) * 10 / channelCount * fps / sampleRate < 10 && //如果剩余数据可播放的时间小于fps的倒数(*10是为了不想用浮点数)
                    output->bytesFree() > bufferLen)    //且剩余缓冲大于即将写入的数据大小 (一般情况下不会出现这个情况，这里只是以防万一)
            {
                //device->write(bufferFrame, bufferFrame.length()); //输出边框
                device->write(buffer, bufferLen);
            }
            else    //否则说明缓冲区时间一定大于fps的倒数，所以可以休息一会
                QTest::qSleep(1000 / fps / 10);  //休息十分之一的帧的时间
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
