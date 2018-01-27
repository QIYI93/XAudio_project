#include "xaudioplay.h"

VoiceCallBack::VoiceCallBack()
    :m_hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    ,m_count(NULL)
    ,m_currentContext(NULL)
    ,m_lastContext(NULL)
    ,m_audioPlay(nullptr)
{
}

VoiceCallBack::~VoiceCallBack()
{
    CloseHandle(m_hBufferEndEvent);
}

XAudioPlay::XAudioPlay()
{

    m_isPlaying = false;
    m_maxBufferCount = 0;
    m_streamingBufferSize = 0;
    m_voiceCallBack.m_audioPlay = this;
    reset();

    //create engine
    HRESULT lRet;
    lRet = XAudio2Create(&m_XAudio2, NULL, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(lRet))
        return;

    lRet = m_XAudio2->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(lRet))
        return;

    setBufferSize(DEF_MaxBufferCount, DEF_StreamingBufferSize);

    lRet = m_XAudio2->CreateSourceVoice(&m_sourceVoice, 
        &m_waveFormat,
        NULL, 
        XAUDIO2_DEFAULT_FREQ_RATIO,
        &m_voiceCallBack,
        nullptr, 
        nullptr);

    if (FAILED(lRet))
        return;

    m_isValid = true;
}


XAudioPlay::~XAudioPlay()
{
}

void XAudioPlay::setBufferSize(int maxBufferCount, int streamingBufferSize)
{
    if (m_maxBufferCount != maxBufferCount || m_streamingBufferSize != streamingBufferSize)
    {
        stopPlaying();
        for (int i = 0; i < m_maxBufferCount; ++i)
        {
            delete[] m_audioData[i];
            if (m_audio2Buffer[i].pContext)
                delete m_audio2Buffer[i].pContext;
        }
        delete[] m_audioData;
        delete[] m_audio2Buffer;

        m_maxBufferCount = maxBufferCount;
        m_streamingBufferSize = streamingBufferSize;
        m_voiceCallBack.m_lastContext = 0;
        reset();

        m_audioData = new unsigned char *[maxBufferCount];
        m_audio2Buffer = new XAUDIO2_BUFFER[maxBufferCount];
        for (int i = 0; i < maxBufferCount; ++i)
        {
            m_audioData[i] = new unsigned char[streamingBufferSize];
            m_audio2Buffer[i] = XAudioPlay::makeXAudio2Buffer(m_audioData[i], streamingBufferSize);
            *((int*)this->m_audio2Buffer[i].pContext) = i;

        }
    }
}


void XAudioPlay::reset()
{
    m_readingBufferNumber = 0;
    m_writingBufferNumber = 0;
    m_unReadingBufferCount = 0;
    m_unProcessedBufferCount = 0;
    m_writingPosition = 0;
    m_voiceCallBack.m_count = 0;
    m_voiceCallBack.m_lastContext = 0;
}

void XAudioPlay::stopPlaying()
{
    if (m_sourceVoice == nullptr)
        return;

    XAUDIO2_VOICE_STATE state;

    if (m_isPlaying)
    {
        m_isPlaying = false;
        m_sourceVoice->GetState(&state);
        while (state.BuffersQueued)
        {
            m_sourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
            m_sourceVoice->FlushSourceBuffers();
            Sleep(1);
            m_sourceVoice->GetState(&state);
        }
    }
    reset();
}

void XAudioPlay::pause()
{
    if (m_sourceVoice == nullptr)
        return;

    if (!m_isPlaying)
        return;

    m_sourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
    m_isPlaying = false;
}

void XAudioPlay::startPlaying()
{
    m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    m_isPlaying = true;
}

XAUDIO2_BUFFER XAudioPlay::makeXAudio2Buffer(const BYTE *pBuffer, int BufferSize)
{
    XAUDIO2_BUFFER xAudio2Buffer;
    xAudio2Buffer.AudioBytes = BufferSize;
    xAudio2Buffer.Flags = 0;
    xAudio2Buffer.LoopBegin = 0;
    xAudio2Buffer.LoopCount = 0;
    xAudio2Buffer.LoopLength = 0;
    xAudio2Buffer.pAudioData = pBuffer;
    xAudio2Buffer.pContext = (void*)new int[1];
    xAudio2Buffer.PlayBegin = 0;
    xAudio2Buffer.PlayLength = 0;
    return xAudio2Buffer;
}

void XAudioPlay::setFormat(int bitsPerSample, int channels, int freq)
{
    if (m_waveFormat.wBitsPerSample == bitsPerSample 
        && m_waveFormat.nChannels == channels 
        && m_waveFormat.nSamplesPerSec == freq)
        return;

    stopPlaying();

    if (m_sourceVoice)
        m_sourceVoice->DestroyVoice();

    m_sourceVoice = nullptr;

    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.wBitsPerSample = bitsPerSample;
    m_waveFormat.nChannels = channels;
    m_waveFormat.nSamplesPerSec = freq;
    m_waveFormat.nBlockAlign = bitsPerSample * channels;
    m_waveFormat.nAvgBytesPerSec = m_waveFormat.nBlockAlign * freq;
    m_waveFormat.cbSize = 0;

    HRESULT lRet;
    lRet = m_XAudio2->CreateSourceVoice(&m_sourceVoice,
        &m_waveFormat, 
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        &m_voiceCallBack,
        nullptr,
        nullptr);

    if (FAILED(lRet))
        return;
}