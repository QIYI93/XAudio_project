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

{//�ص���

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

       //����һ�����ݿ�ǰ�󴥷��¼�

       void OnBufferEnd(void *pBufferContext);

       //����Ҫ�ķ���ֻ��������

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
    //XAudio2����
    IXAudio2* pXAudio2;//XAudio2����
    IXAudio2MasteringVoice* pMasterVoice;//MsterVoice
    VoiceCallBack voiceCallBack;//�ص�����
    XAUDIO2_BUFFER *pXAudio2Buffer;//��װ�������ʽ�����Ƶ�������ݿ�
    WAVEFORMATEX WaveFormat;//��Ƶ��ʽ
    IXAudio2SourceVoice *pSourceVoice;//Դ������ӿ�
    int MaxBufferCount;//��󻺳����ݿ���
    int StreamingBufferSize;//�������ݿ��С
    unsigned char **ppAudioData;//��Ƶ��������
    bool IsPlaying;//�Ƿ��ڲ���

    HRESULT State;//״̬��0Ϊ������<0Ϊ����>0Ϊ��Ϣ�򾯸�
    const wchar_t *pState;//״̬������ϸ����

    //���ⲿ��ȡ��������
    int ReadingBufferNumber;//���ڶ�ȡ��Buffer
    int WritingBufferNumber;//����д���Buffer
    int UnReadingBufferCount;//δ�ʽ�Buffer����
    int UnProcessedBufferCount;//�ʽ���δ�����Buffer����
    int WritingPosition;//��ǰ��׼��д��λ��

    //���ƺ�������

public:Speaker();//Ĭ�Ϲ��캯��
public:~Speaker();//��������
private:void reset();//���ý������ݲ���
private:HRESULT SetState(HRESULT state);//���ü򵥵�״̬��Ϣ
public:void StopPlaying();//ֹͣ����
public:void Pause();//��ͣ
public:void StartPlaying();//��ʼ����
public:void SetFormat(short bits, short channels, int hz);//����PCM��ʽ
public:void SetBufferSize(int MaxBufferCount, int StreamingBufferSize);//������Ƶ���ݻ�����
public:static XAUDIO2_BUFFER MakeXAudio2Buffer(const BYTE *pBuffer, int BufferSize);//����XAudio2������
public:static WAVEFORMATEX MakePCMSourceFormat(short bits, short channels, int hz);//����PCM��ʽ��Ϣ
public:void ReadData(unsigned char *pBuffer, int length);//��pBufferָ����ڴ渴��length�ֽڵ����ݵ�������,�������겻����
public:int ReadDataFrom(unsigned char *pBuffer, int length);//��pBufferָ����ڴ渴��length�ֽڵ�����,��������ʵ�ʸ��Ƶ������ֽ���
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

    while (speaker->UnReadingBufferCount)//�ύ

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

    //��ʼ��

    IsPlaying = false;

    MaxBufferCount = 0;

    StreamingBufferSize = 0;

    voiceCallBack.speaker = this;

    reset();

    //��������

    if (FAILED(SetState(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))))goto END;

    //����MasteringVoice

    if (FAILED(SetState(pXAudio2->CreateMasteringVoice(&pMasterVoice))))goto END;

    //���仺���ڴ沢����XAudio2Buffer

    SetBufferSize(DEF_MaxBufferCount, DEF_StreamingBufferSize);//����Ĭ�ϻ���

                                                               //����pcm��ʽ

    WaveFormat = Speaker::MakePCMSourceFormat(16, 2, 44100);

    //����SourceVoice

    SetState(pXAudio2->CreateSourceVoice(&this->pSourceVoice, &this->WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &this->voiceCallBack, NULL, NULL));

END:;

};

Speaker::~Speaker()

{

    SetBufferSize(0, 0);//ֹͣ���Ų��ͷŻ�����

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

            if (this->StreamingBufferSize)//ɾ����

            {

                delete[] this->ppAudioData[i];

                if (this->pXAudio2Buffer[i].pContext)

                    delete this->pXAudio2Buffer[i].pContext;

            }

        }

        if (this->MaxBufferCount)//ɾ������

        {

            delete[] this->ppAudioData;

            delete[] this->pXAudio2Buffer;

        }

        this->MaxBufferCount = MaxBufferCount;//��ֵ

        this->StreamingBufferSize = StreamingBufferSize;

        voiceCallBack.lastcontext = 0;



        if (MaxBufferCount)//����

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

    format.wFormatTag = WAVE_FORMAT_PCM;//PCM��ʽ

    format.wBitsPerSample = bits;//λ��

    format.nChannels = channels;//������

    format.nSamplesPerSec = hz;//������

    format.nBlockAlign = bits*channels / 8;//���ݿ����

    format.nAvgBytesPerSec = format.nBlockAlign*hz;//ƽ����������

    format.cbSize = 0;//������Ϣ

    return format;

}

void Speaker::ReadData(unsigned char *pBuffer, int length)

{

    int i = 0;

    while (length > i)//�����겻�˳�ѭ��

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

int Speaker::ReadDataFrom(unsigned char *pBuffer, int length)//��pBufferָ����ڴ渴��length�ֽڵ�����,��������ʵ�ʸ��Ƶ������ֽ���

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

    voiceCallBack.OnBufferEnd(NULL);//����XAudio2������

    return i;

}

HRESULT Speaker::SetState(HRESULT state)

{

    this->State = state;

    switch (state)

    {

    case 0:pState = L"����."; break;

    case XAUDIO2_E_INVALID_CALL:pState = L"������Ч����"; break;

    case XAUDIO2_E_XMA_DECODER_ERROR:pState = L"XMA�豸��"; break;

    case XAUDIO2_E_XAPO_CREATION_FAILED:pState = L"XAPOЧ����ʼ��ʧ��"; break;

    case XAUDIO2_E_DEVICE_INVALIDATED:pState = L"��Ƶ�豸������"; break;

    default:if (state > 0)pState = L"δ֪��Ϣ";

            else pState = L"δ֪����";

    }

    return state;

}