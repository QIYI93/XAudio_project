#include "XAudio2.h" 
#include <stdio.h>  
#include <iostream>  

using namespace std;

int main(int argc, char *argv[])
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

    IXAudio2 *pEngine = NULL;
    hr = XAudio2Create(&pEngine); //create
    if (FAILED(hr))
        return -1;

    UINT uPlayer = 0;
    hr = pEngine->GetDeviceCount(&uPlayer);//��ȡ��Ƶ����豸����  
    if (FAILED(hr))
        return -1;

    cout << uPlayer << endl;

    pEngine->Release();//�ͷ���Դ  
    CoUninitialize();//�ͷ���Դ  

    return 0;
}