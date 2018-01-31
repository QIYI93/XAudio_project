#ifndef XAUDIOPLAY_H
#define XAUDIOPLAY_H

#include "dxsdk/XAudio2.h"

#define DEF_MaxBufferCount 4
#define DEF_StreamingBufferSize 40960

class XAudioPlay;
class VoiceCallBack :public IXAudio2VoiceCallback
{
public:
    VoiceCallBack();
    VoiceCallBack(VoiceCallBack &) = delete;
    ~VoiceCallBack();

    void OnBufferEnd(void *pBufferContext);//����һ�����ݿ�ǰ�󴥷��¼�

    void OnBufferStart(void *pBufferContext) {}
    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnStreamEnd() {}
    void OnLoopEnd(void *pBufferContext) {}
    void OnVoiceError(void *pBufferContext, HRESULT Error) {}

public:
    int m_count;
    int m_lastContext;
    int m_currentContext;
    HANDLE m_hBufferEndEvent;
    XAudioPlay *m_audioPlay;
    XAUDIO2_VOICE_STATE m_state;


};

class XAudioPlay
{

public:
    friend class VoiceCallBack;
    XAudioPlay();
    XAudioPlay(XAudioPlay&) = delete;
    ~XAudioPlay();
    bool isValid() { return m_isValid; }
    void setFormat(int bits, int channels, int freq);
    void pause();
    void startPlaying();
    void stopPlaying();
    void readData(unsigned char *buffer, int length); //blocking

private:
    void reset();
    void deleteBuffer();
    void setBufferSize(int maxBufferCount, int streamingBufferSize);

    static XAUDIO2_BUFFER makeXAudio2Buffer(const BYTE *pBuffer, int BufferSize);//����XAudio2������

private:
    IXAudio2* m_XAudio2;                        //XAudio2����
    IXAudio2MasteringVoice* m_masterVoice;      //MsterVoice
    VoiceCallBack m_voiceCallBack;              //�ص�����
    XAUDIO2_BUFFER *m_audio2Buffer;             //��װ�������ʽ�����Ƶ�������ݿ�
    WAVEFORMATEX m_waveFormat;                  //��Ƶ��ʽ
    IXAudio2SourceVoice *m_sourceVoice;         //Դ������ӿ�

    int m_maxBufferCount;                       //��󻺳����ݿ���
    int m_streamingBufferSize;                  //�������ݿ��С
    unsigned char ** m_audioData;               //��Ƶ��������

    bool m_isPlaying;                           //�Ƿ��ڲ���
    bool m_isValid = false;

    int m_readingBufferNumber = 0;//���ڶ�ȡ��Buffer
    int m_writingBufferNumber = 0;//����д���Buffer
    int m_unReadingBufferCount;//δ�ʽ�Buffer����


    int m_unProcessedBufferCount;//�ʽ���δ�����Buffer����
    int m_writingPosition;//��ǰ��׼��д��λ��
};


#endif
