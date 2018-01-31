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

    void OnBufferEnd(void *pBufferContext);//播放一个数据块前后触发事件

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

    static XAUDIO2_BUFFER makeXAudio2Buffer(const BYTE *pBuffer, int BufferSize);//生成XAudio2缓冲区

private:
    IXAudio2* m_XAudio2;                        //XAudio2引擎
    IXAudio2MasteringVoice* m_masterVoice;      //MsterVoice
    VoiceCallBack m_voiceCallBack;              //回调处理
    XAUDIO2_BUFFER *m_audio2Buffer;             //封装的用来呈交的音频缓冲数据块
    WAVEFORMATEX m_waveFormat;                  //音频格式
    IXAudio2SourceVoice *m_sourceVoice;         //源音处理接口

    int m_maxBufferCount;                       //最大缓冲数据块数
    int m_streamingBufferSize;                  //缓冲数据块大小
    unsigned char ** m_audioData;               //音频缓冲数据

    bool m_isPlaying;                           //是否在播放
    bool m_isValid = false;

    int m_readingBufferNumber = 0;//正在读取的Buffer
    int m_writingBufferNumber = 0;//正在写入的Buffer
    int m_unReadingBufferCount;//未呈交Buffer数量


    int m_unProcessedBufferCount;//呈交但未处理的Buffer数量
    int m_writingPosition;//当前块准备写入位置
};


#endif
