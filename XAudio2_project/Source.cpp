#include <Windows.h>
#include <iostream>
#include "dxsdk/XAudio2.h" 
#include "xaudioplay.h"

int main(int argc, char *argv[])
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

    FILE *PCMFile = nullptr;
    fopen_s(&PCMFile, "NocturneNo2inEflat_44.1k_s16le.pcm", "rb+");
    fseek(PCMFile, 0L, SEEK_END);
    int size = ftell(PCMFile);
    fseek(PCMFile, 0, 0);
    unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char)*size);
    fread_s(data, size, size, 1, PCMFile);
    fclose(PCMFile);
    
    XAudioPlay audioPlay;
    if (!audioPlay.isValid())
        return -1;
    audioPlay.setFormat(16, 2, 44100);
    audioPlay.startPlaying();
    audioPlay.readData(data, size);
    CoUninitialize();

    return 0;
}
