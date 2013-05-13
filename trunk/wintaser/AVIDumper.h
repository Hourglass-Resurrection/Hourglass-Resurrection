#pragma once

void ProcessCaptureFrameInfo(void* frameCaptureInfoRemoteAddr, int frameCaptureInfoType);
void ProcessCaptureSoundInfo();

// Must be called once and ONCE only, recommended to make the call from the initialization code of the whole program!
void InitAVICriticalSections();

bool SetAVIFilename(char* filename);
void SetLastFrameSoundInfo(void* soundInfoPointer);
void SetGammaRamp(void* gammaRampPointer, HANDLE process);
void SetPaletteEntriesPointer(void* pointer);
void SetCaptureProcess(HANDLE process);

void CloseAVI();
//bool OpenAVIFile(int width, int height, int bpp, int fps);
//void WriteAVIFrame(void* remotePixels, int width, int height, int pitch, int bpp, int rmask, int gmask, int bmask);
//void RewriteAVIFrame();
//bool ChooseAudioCodec(const LPWAVEFORMATEX defaultFormat);
//int OpenAVIAudioStream();
//void WriteAVIAudio(HANDLE process, void* lastAudioFrameSoundInfo);
void SplitAVINow();
/*static*/ void HandleAviSplitRequests();