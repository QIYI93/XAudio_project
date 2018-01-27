#include <Windows.h>
#include <stdio.h>  
#include <iostream>  
#include "XAudio2.h" 
#include "Objbase.h"

#pragma comment(lib,"Ole32.lib")

using namespace std;

//int main(int argc, char *argv[])
//{
//    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
//    if (FAILED(hr))
//        return -1;
//
//    IXAudio2 *pEngine = NULL;
//    hr = XAudio2Create(&pEngine);
//    if (FAILED(hr))
//        return -1;
//
//    UINT uPlayer = 0;
//    hr = pEngine->GetDeviceCount(&uPlayer);
//    if (FAILED(hr))
//        return -1;
//
//    XAUDIO2_DEVICE_DETAILS deviceInfo;
//    for (int i = 0; i < uPlayer; i++)
//    {
//        pEngine->GetDeviceDetails(i, &deviceInfo);
//    }
//
//    pEngine->Release();
//    CoUninitialize();
//
//    return 0;
//}

#define DEF_MaxBufferCount4
#define DEF_StreamingBufferSize40960

class VoiceCallBack;
class Speaker;





class VoiceCallBack :public IXAudio2VoiceCallback

{//回调类

public:HANDLE hBufferEndEvent;

       VoiceCallBack() :hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))

       {
           count = lastcontext = currentcontext = 0;
       }

       ~VoiceCallBack() { CloseHandle(hBufferEndEvent); }

       Speaker *speaker;

       XAUDIO2_VOICE_STATE state;

public:int count;

       int lastcontext;

       int currentcontext;

       //播放一个数据块前后触发事件

       void OnBufferEnd(void *pBufferContext);

       //不需要的方法只保留声明

       void OnBufferStart(void *pBufferContext) {}

       void OnVoiceProcessingPassEnd() {}

       void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}

       void OnStreamEnd() {}

       void OnLoopEnd(void *pBufferContext) {}

       void OnVoiceError(void *pBufferContext, HRESULT Error) {}

};



class Speaker

{
public:
    //XAudio2数据
    IXAudio2* pXAudio2;//XAudio2引擎
    IXAudio2MasteringVoice* pMasterVoice;//MsterVoice
    VoiceCallBack voiceCallBack;//回调处理
    XAUDIO2_BUFFER *pXAudio2Buffer;//封装的用来呈交的音频缓冲数据块
    WAVEFORMATEX WaveFormat;//音频格式
    IXAudio2SourceVoice *pSourceVoice;//源音处理接口
    int MaxBufferCount;//最大缓冲数据块数
    int StreamingBufferSize;//缓冲数据块大小
    unsigned char **ppAudioData;//音频缓冲数据
    bool IsPlaying;//是否在播放

    HRESULT State;//状态，0为正常，<0为错误，>0为信息或警告
    const wchar_t *pState;//状态内容详细描述

    //跟外部读取交换数据
    int ReadingBufferNumber;//正在读取的Buffer
    int WritingBufferNumber;//正在写入的Buffer
    int UnReadingBufferCount;//未呈交Buffer数量
    int UnProcessedBufferCount;//呈交但未处理的Buffer数量
    int WritingPosition;//当前块准备写入位置

    //控制函数部分

public:Speaker();//默认构造函数
public:~Speaker();//析构函数
private:void reset();//重置交换数据参数
private:HRESULT SetState(HRESULT state);//设置简单的状态信息
public:void StopPlaying();//停止播放
public:void Pause();//暂停
public:void StartPlaying();//开始播放
public:void SetFormat(short bits, short channels, int hz);//更改PCM格式
public:void SetBufferSize(int MaxBufferCount, int StreamingBufferSize);//分配音频数据缓冲区
public:static XAUDIO2_BUFFER MakeXAudio2Buffer(const BYTE *pBuffer, int BufferSize);//生成XAudio2缓冲区
public:static WAVEFORMATEX MakePCMSourceFormat(short bits, short channels, int hz);//生成PCM格式信息
public:void ReadData(unsigned char *pBuffer, int length);//从pBuffer指向的内存复制length字节的数据到缓冲区,不复制完不返回
public:int ReadDataFrom(unsigned char *pBuffer, int length);//从pBuffer指向的内存复制length字节的数据,立即返回实际复制的数据字节数
};



void VoiceCallBack::OnBufferEnd(void *pBufferContext)

{

    if (pBufferContext == NULL)

    {

        speaker->pSourceVoice->GetState(&state);

        speaker->UnProcessedBufferCount = state.BuffersQueued;

    }

    else

    {

        currentcontext = *((int *)pBufferContext) + 1;

        if (currentcontext > lastcontext)

            count += currentcontext - lastcontext;

        else  if (currentcontext < lastcontext)

            count += speaker->MaxBufferCount + currentcontext - lastcontext;

        lastcontext = currentcontext;

        speaker->UnProcessedBufferCount -= count;

    }

    count = 0;

    while (speaker->UnReadingBufferCount)//提交

    {

        speaker->pSourceVoice->SubmitSourceBuffer(&speaker->pXAudio2Buffer[speaker->ReadingBufferNumber], NULL);

        speaker->ReadingBufferNumber = (1 + speaker->ReadingBufferNumber) % speaker->MaxBufferCount;

        speaker->UnProcessedBufferCount++;

        speaker->UnReadingBufferCount--;

    }

    SetEvent(hBufferEndEvent);

}



Speaker::Speaker()

{

    //初始化

    IsPlaying = false;

    MaxBufferCount = 0;

    StreamingBufferSize = 0;

    voiceCallBack.speaker = this;

    reset();

    //建立引擎

    if (FAILED(SetState(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))))goto END;

    //建立MasteringVoice

    if (FAILED(SetState(pXAudio2->CreateMasteringVoice(&pMasterVoice))))goto END;

    //分配缓冲内存并设置XAudio2Buffer

    SetBufferSize(DEF_MaxBufferCount, DEF_StreamingBufferSize);//设置默认缓冲

                                                               //设置pcm格式

    WaveFormat = Speaker::MakePCMSourceFormat(16, 2, 44100);

    //建立SourceVoice

    SetState(pXAudio2->CreateSourceVoice(&this->pSourceVoice, &this->WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &this->voiceCallBack, NULL, NULL));

END:;

};

Speaker::~Speaker()

{

    SetBufferSize(0, 0);//停止播放并释放缓冲区

    if (pSourceVoice != NULL)pSourceVoice->DestroyVoice();

    if (pMasterVoice != NULL)pMasterVoice->DestroyVoice();

    if (pXAudio2 != NULL)pXAudio2->StopEngine();

}

void Speaker::reset()

{

    SetState(0);

    ReadingBufferNumber = 0;

    UnReadingBufferCount = 0;

    UnProcessedBufferCount = 0;

    WritingBufferNumber = 0;

    WritingPosition = 0;

    voiceCallBack.count = 0;

    voiceCallBack.lastcontext = 0;

}

void Speaker::StopPlaying()

{

    if (pSourceVoice == NULL)return;

    XAUDIO2_VOICE_STATE state;

    if (IsPlaying)

    {

        IsPlaying = false;

        pSourceVoice->GetState(&state);

        while (state.BuffersQueued)

        {

            if (FAILED(SetState(pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW))))return;

            if (FAILED(SetState(pSourceVoice->FlushSourceBuffers())))return;

            Sleep(1);

            pSourceVoice->GetState(&state);

        }

    }

    reset();

}

void Speaker::Pause()

{

    if (pSourceVoice == NULL)return;

    if (IsPlaying)

    {

        SetState(pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW));

        if (FAILED(State))return;

        IsPlaying = false;

    }

}

void Speaker::StartPlaying()

{

    SetState(pSourceVoice->Start(0, XAUDIO2_COMMIT_NOW));

    if (FAILED(State))return;

    this->IsPlaying = true;

}

void  Speaker::SetFormat(short bits, short channels, int hz)

{

    if (WaveFormat.wBitsPerSample == bits && WaveFormat.nChannels == channels && WaveFormat.nSamplesPerSec == hz)

        return;

    StopPlaying();

    if (pSourceVoice)pSourceVoice->DestroyVoice();

    pSourceVoice = NULL;

    WaveFormat = MakePCMSourceFormat(bits, channels, hz);

    if (FAILED(SetState(pXAudio2->CreateSourceVoice(&pSourceVoice, &WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &this->voiceCallBack, NULL, NULL))))pSourceVoice = NULL;

}

void Speaker::SetBufferSize(int MaxBufferCount, int StreamingBufferSize)

{

    if (this->MaxBufferCount != MaxBufferCount || this->StreamingBufferSize != StreamingBufferSize)

    {

        this->StopPlaying();

        for (int i = 0; i < this->MaxBufferCount; ++i)

        {

            if (this->StreamingBufferSize)//删除块

            {

                delete[] this->ppAudioData[i];

                if (this->pXAudio2Buffer[i].pContext)

                    delete this->pXAudio2Buffer[i].pContext;

            }

        }

        if (this->MaxBufferCount)//删除所有

        {

            delete[] this->ppAudioData;

            delete[] this->pXAudio2Buffer;

        }

        this->MaxBufferCount = MaxBufferCount;//赋值

        this->StreamingBufferSize = StreamingBufferSize;

        voiceCallBack.lastcontext = 0;



        if (MaxBufferCount)//创建

        {

            reset();

            this->ppAudioData = new unsigned char *[MaxBufferCount];

            this->pXAudio2Buffer = new XAUDIO2_BUFFER[MaxBufferCount];

            if (StreamingBufferSize)

            {

                this->pXAudio2Buffer = new XAUDIO2_BUFFER[MaxBufferCount];

                for (int i = 0; i < MaxBufferCount; ++i)

                {

                    this->ppAudioData[i] = new unsigned char[StreamingBufferSize];

                    this->pXAudio2Buffer[i] = Speaker::MakeXAudio2Buffer(ppAudioData[i], StreamingBufferSize);

                    *((int*)this->pXAudio2Buffer[i].pContext) = i;

                }

            }

        }

    }

}

XAUDIO2_BUFFER Speaker::MakeXAudio2Buffer(const BYTE *pBuffer, int BufferSize)

{

    XAUDIO2_BUFFER XAudio2Buffer;

    XAudio2Buffer.AudioBytes = BufferSize;

    XAudio2Buffer.Flags = 0;

    XAudio2Buffer.LoopBegin = 0;

    XAudio2Buffer.LoopCount = 0;

    XAudio2Buffer.LoopLength = 0;

    XAudio2Buffer.pAudioData = pBuffer;

    XAudio2Buffer.pContext = (void*)new int[1];

    XAudio2Buffer.PlayBegin = 0;

    XAudio2Buffer.PlayLength = 0;

    return XAudio2Buffer;

}

WAVEFORMATEX Speaker::MakePCMSourceFormat(short bits, short channels, int hz)

{

    WAVEFORMATEX format;

    format.wFormatTag = WAVE_FORMAT_PCM;//PCM格式

    format.wBitsPerSample = bits;//位数

    format.nChannels = channels;//声道数

    format.nSamplesPerSec = hz;//采样率

    format.nBlockAlign = bits*channels / 8;//数据块调整

    format.nAvgBytesPerSec = format.nBlockAlign*hz;//平均传输速率

    format.cbSize = 0;//附加信息

    return format;

}

void Speaker::ReadData(unsigned char *pBuffer, int length)

{

    int i = 0;

    while (length > i)//不读完不退出循环

    {

        while (UnProcessedBufferCount + UnReadingBufferCount

            >= MaxBufferCount - 1)

        {

            if (!IsPlaying)

            {

                StartPlaying();

                Sleep(0);

                continue;

            }

            voiceCallBack.OnBufferEnd(NULL);

            Sleep(10);



        }

        if (StreamingBufferSize > length - i + WritingPosition)

        {

            while (i < length)

            {

                this->ppAudioData[WritingBufferNumber][WritingPosition] = pBuffer[i];

                ++WritingPosition;

                ++i;

            }

            break;

        }

        while (StreamingBufferSize > WritingPosition)

        {

            this->ppAudioData[WritingBufferNumber][WritingPosition] = pBuffer[i];

            ++WritingPosition;

            ++i;

        }

        WritingPosition = 0;

        WritingBufferNumber = (1 + WritingBufferNumber) % MaxBufferCount;

        ++UnReadingBufferCount;

    }

}

int Speaker::ReadDataFrom(unsigned char *pBuffer, int length)//从pBuffer指向的内存复制length字节的数据,立即返回实际复制的数据字节数

{

    int i = 0;

    //XAUDIO2_VOICE_STATE state;

    //pSourceVoice->GetState(&state);

    if (pBuffer == nullptr || !length)

        return 0;

    while (UnProcessedBufferCount + UnReadingBufferCount < this->MaxBufferCount)

    {

        if (this->StreamingBufferSize > length - i + WritingPosition)

        {

            while (i < length)

            {

                this->ppAudioData[WritingBufferNumber][WritingPosition] = pBuffer[i];

                ++WritingPosition;

                ++i;

            }

            return length;

        }

        while (WritingPosition < StreamingBufferSize)

        {

            this->ppAudioData[WritingBufferNumber][WritingPosition] = pBuffer[i];

            ++WritingPosition;

            ++i;

        }

        WritingPosition = 0;

        WritingBufferNumber = (1 + WritingBufferNumber) % MaxBufferCount;

        ++UnReadingBufferCount;

        ++UnProcessedBufferCount;

    }

    voiceCallBack.OnBufferEnd(NULL);//督促XAudio2快点放完

    return i;

}

HRESULT Speaker::SetState(HRESULT state)

{

    this->State = state;

    switch (state)

    {

    case 0:pState = L"正常."; break;

    case XAUDIO2_E_INVALID_CALL:pState = L"函数无效调用"; break;

    case XAUDIO2_E_XMA_DECODER_ERROR:pState = L"XMA设备损坏"; break;

    case XAUDIO2_E_XAPO_CREATION_FAILED:pState = L"XAPO效果初始化失败"; break;

    case XAUDIO2_E_DEVICE_INVALIDATED:pState = L"音频设备不可用"; break;

    default:if (state > 0)pState = L"未知信息";

            else pState = L"未知错误";

    }

    return state;

}