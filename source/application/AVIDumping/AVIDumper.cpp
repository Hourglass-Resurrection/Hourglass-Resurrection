/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <windows.h>

#include "AVIDumper.h"
#include "../logging.h"
#include "../CustomDLGs.h"

#include <algorithm>

#include <malloc.h>
#include <stdio.h>

//#include "Resource.h"
#include "external/ddraw.h"
#include "shared/winutil.h"

#include <vfw.h>
#pragma comment(lib, "vfw32.lib")
#include <msacm.h>
#pragma comment(lib, "msacm32.lib")

#include "shared/ipc.h"
#include "../Config.h"

#include "WaveFormat.h"
#include "AudioConverterStream.h"
#include "WaitableBool.h"

extern bool tasFlagsDirty;

// TODO: Eliminate this
extern bool mainMenuNeedsRebuilding;

static int aviSplitCount = 0;
static int aviSplitDiscardCount = 0;
static int requestedAviSplitCount = 0;

static void* lastFrameSoundInfo = 0;
static void* paletteEntriesPointer = 0;
bool g_gammaRampEnabled = false;

static HANDLE captureProcess = NULL;

char avifilename[MAX_PATH + 1];

#define avidebugprintf verbosedebugprintf

#ifdef _DEBUG
#define _AVIDEBUG
#endif

#if defined(_AVIDEBUG) || 0 //1
	#define verbosedebugprintf debugprintf

	#include "../Movie.h"
	extern Movie movie;
#else
	#define verbosedebugprintf(...) ((void)0)
#endif

AudioConverterStream* audioConverterStream = NULL;

HRESULT SafeAVIStreamWrite(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten)
{
	HRESULT hr;

	__try
	{
		hr = AVIStreamWrite(pavi, lStart, lSamples, lpBuffer, cbBuffer, dwFlags, plSampWritten, plBytesWritten);
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hr = E_POINTER;
	}
	return hr;
}

//0x1FE -> -1
// 0xFF -> 0
// 0x7F -> 1
// 0x3F -> 2
static int leftShiftFromMask(unsigned int mask)
{
	int shift = 0;

	if (mask)
	{
		if (mask < 0xFF)
		{
			while ((mask << shift) < 128)
			{
				shift++;
			}
		}
		else
		{
			while ((mask >> -shift) > 255)
			{
				shift--;
			}
		}
	}

	return shift;
}

template<typename T>
static void ReserveBuffer(T*& buffer, int& bufferAllocated, int size)
{
	if (!buffer || bufferAllocated < size)
	{
		bufferAllocated = size;
		buffer = static_cast<T*>(realloc(buffer, size));
	}
}

// warning: gamma ramp is (unlike everything else) stored in a way that doesn't properly reset when loading a savestate.
// this is probably fine since it's currently only used for AVI capture.
DDGAMMARAMP g_gammaRamp;
void SetGammaRamp(void* gammaRampPointer, HANDLE process)
{
	if (gammaRampPointer)
	{
		SIZE_T bytesRead = 0;

		if (ReadProcessMemory(process, gammaRampPointer, &g_gammaRamp, sizeof(DDGAMMARAMP), &bytesRead))
		{
			g_gammaRampEnabled = true;
		}
	}
	else
	{
		g_gammaRampEnabled = false;
	}
}

template<int numSlots>
struct AviFrameQueue
{
	AviFrameQueue() : m_nextWrite(0), m_nextRead(0), m_nextWriteAudio(0), m_nextReadAudio(0)
	{
		m_disableFills = true;

		for (int i = 0; i < numSlots; i++)
		{
			m_slots[i].slotNum = i;
		}

		threadDone = false;
		thread = CreateThread(NULL, 0, AviFrameQueue<numSlots>::OutputVideoThreadFunc, (void*)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
		threadAudio = CreateThread(NULL, 0, AviFrameQueue<numSlots>::OutputAudioThreadFunc, (void*)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority(threadAudio, THREAD_PRIORITY_ABOVE_NORMAL);
		m_disableFills = false;
		ResumeThread(thread);
		ResumeThread(threadAudio);
	}

	~AviFrameQueue()
	{
		m_disableFills = true;
		threadDone = true;

		for (int i = 0; i < numSlots; i++)
		{
			m_slots[i].hasProcessedVideo = true; // wake up thread to stop it
			m_slots[i].hasProcessedAudio = true;
		}

		DWORD exitCode = STILL_ACTIVE;

		for (int i = 0; exitCode == STILL_ACTIVE && i < 500; i++)
		{
			if (!GetExitCodeThread(thread, &exitCode))
			{
				break;
			}
			else
			{
				Sleep(10);
			}
		}

		if (exitCode == STILL_ACTIVE)
		{
			debugprintf(L"WARNING: had to force terminate AVI video thread\n");
			if(IsDebuggerPresent())
			{
				_asm{int 3}
			}

			TerminateThread(thread, -1);
		}

		CloseHandle(thread);
		exitCode = STILL_ACTIVE;

		for (int i = 0; exitCode == STILL_ACTIVE && i < 500; i++)
		{
			if (!GetExitCodeThread(threadAudio, &exitCode))
			{
				break;
			}
			else
			{
				Sleep(10);
			}
		}

		if (exitCode == STILL_ACTIVE)
		{
			debugprintf(L"WARNING: had to force terminate AVI audio thread\n");

			if (IsDebuggerPresent())
			{
				_asm{int 3}
			}

			TerminateThread(threadAudio, -1);
		}

		CloseHandle(threadAudio);
	}

	void Flush()
	{
		for (int i = 0; i < numSlots; i++)
		{
			m_slots[i].hasProcessedVideo.WaitUntilFalse();
			m_slots[i].hasProcessedAudio.WaitUntilFalse();
		}
	}

	// send a new frame to AVI
	void FillFrame(void* remotePixels, int width, int height, int pitch, int bpp,
				   int rmask, int gmask, int bmask)
	{
		AutoCritSect cs(&s_fqvCS);
		if (m_disableFills)
		{
			return;
		}

		Slot& slot = m_slots[m_nextWrite];
		avidebugprintf(L"FillFrame: slot %d, movie frame %d\n", m_nextWrite, movie.currentFrame);
		m_nextWrite++;
		m_nextWrite %= numSlots;

		const int remotePixelsSize = (pitch > 0 ? pitch : -pitch) * height;

		slot.hasProcessedVideo.WaitUntilFalse();

		static unsigned char* inPixels = NULL;
		static int inPixelsAllocated = 0;
		ReserveBuffer(inPixels, inPixelsAllocated, remotePixelsSize);

#ifdef _DEBUG
		DWORD time1 = timeGetTime();
#endif

		unsigned char* curInPixels;
		if (pitch >= 0)
		{
			// we got a pointer to the start of the first row of pixel data
			ReadProcessMemory(captureProcess, remotePixels, inPixels, remotePixelsSize, NULL);
			curInPixels = inPixels;
		}
		else
		{
			// we got a pointer to the start of the last row of pixel data (see FrameBoundaryDIBitsToAVI)
			ReadProcessMemory(captureProcess, (char*)remotePixels + pitch*(height-1), inPixels, remotePixelsSize, NULL);
			curInPixels = inPixels - pitch*(height-1);
		}

#ifdef _DEBUG
		DWORD time2 = timeGetTime();
		debugprintf(L"AVI: reading pixel data took %d ticks\n", (int)(time2-time1));
#endif

#ifdef _AVIDEBUG
		slot.framecount = movie.currentFrame;
#endif

		const int aviPixelsSize = width * height * (24 / 8) + 8;
		ReserveBuffer(slot.aviPixels, slot.aviPixelsAllocated, aviPixelsSize);

		unsigned char* aviPix = slot.aviPixels;

		const int bytesPerInPixel = bpp >> 3;
		const int bytesPerAVIPixel = 24 >> 3;

		int rShiftLeft = leftShiftFromMask(rmask);
		int gShiftLeft = leftShiftFromMask(gmask);
		int bShiftLeft = leftShiftFromMask(bmask);
		const int rShiftRight = std::max(0, -rShiftLeft);
		const int gShiftRight = std::max(0, -gShiftLeft);
		const int bShiftRight = std::max(0, -bShiftLeft);
		rShiftLeft = std::max(0, rShiftLeft);
		gShiftLeft = std::max(0, gShiftLeft);
		bShiftLeft = std::max(0, bShiftLeft);

		if (bytesPerInPixel == 4 && rmask == 0x00FF0000 && gmask == 0x0000FF00 && bmask == 0x000000FF)
		{
			// "fast" case
			// TODO: use the MMX-optimized blit32to24loop from kkapture for this case
			for (int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);

				for (int xRem = width + 1; --xRem; inPix += 4, aviPix += bytesPerAVIPixel)
				{
					*(unsigned int*)aviPix = *(unsigned int*)inPix;
				}
			}
		}
		else if (bytesPerInPixel == 1 && !rmask && !gmask && !bmask)
		{
			// palettized case
			static PALETTEENTRY activePalette [256];
			ReadProcessMemory(captureProcess, paletteEntriesPointer, &activePalette, sizeof(activePalette), NULL);
			for (int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);
				for (int xDst = 0; xDst < width; xDst++, inPix++, aviPix += bytesPerAVIPixel)
				{
					PALETTEENTRY entry = activePalette[*(unsigned char*)inPix];
					aviPix[0] = entry.peBlue;
					aviPix[1] = entry.peGreen;
					aviPix[2] = entry.peRed;
				}
			}
		}
		else
		{
			// general case
			for (int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);
				for (int xDst = 0; xDst < width; xDst++, inPix += bytesPerInPixel, aviPix += bytesPerAVIPixel)
				{
					int pixel = *(int*)inPix;
					unsigned char r = (pixel & rmask) << rShiftLeft >> rShiftRight;
					unsigned char g = (pixel & gmask) << gShiftLeft >> gShiftRight;
					unsigned char b = (pixel & bmask) << bShiftLeft >> bShiftRight;
					aviPix[0] = b;
					aviPix[1] = g;
					aviPix[2] = r;
				}
			}
		}


		// apply gamma ramp
		// temp? processing the pixels in the game without causing too much slowdown
		// was getting to be a pain, so for now gamma ramp effects only shows up in AVIs.
		if (g_gammaRampEnabled)
		{
			aviPix = slot.aviPixels;
			for (int count = aviPixelsSize / bytesPerAVIPixel + 1; --count; aviPix += bytesPerAVIPixel)
			{
				unsigned char b = aviPix[0];
				unsigned char g = aviPix[1];
				unsigned char r = aviPix[2];
				aviPix[0] = g_gammaRamp.blue[b] >> 8;
				aviPix[1] = g_gammaRamp.green[g] >> 8;
				aviPix[2] = g_gammaRamp.red[r] >> 8;
			}
		}

		slot.hasProcessedVideo = true;
	}

	// output the last frame again to AVI
	void RefillFrame()
	{
		AutoCritSect cs(&s_fqvCS);
		if (m_disableFills)
		{
			return;
		}

		Slot& prevSlot = m_slots[(numSlots+m_nextWrite-1)%numSlots];
		Slot& slot = m_slots[m_nextWrite];
		avidebugprintf(L"RefillFrame: slot %d, movie frame %d\n", m_nextWrite, movie.currentFrame);
		m_nextWrite++;
		m_nextWrite %= numSlots;

		slot.hasProcessedVideo.WaitUntilFalse();

#ifdef _AVIDEBUG
		slot.framecount = movie.currentFrame;
#endif

		const int aviPixelsSize = prevSlot.aviPixelsAllocated;
		ReserveBuffer(slot.aviPixels, slot.aviPixelsAllocated, aviPixelsSize);
		if (!slot.aviPixels)
		{
			const int aviPixelsSize = curAviWidth * curAviHeight * (24 / 8) + 8;
			ReserveBuffer(slot.aviPixels, slot.aviPixelsAllocated, aviPixelsSize);
		}

		if (prevSlot.aviPixels)
		{
			memcpy(slot.aviPixels, prevSlot.aviPixels, aviPixelsSize);
		}
		else
		{
			memset(slot.aviPixels, 0, aviPixelsSize);
		}

		slot.hasProcessedVideo = true;
	}

	void FillAudioFrame()
	{
		AutoCritSect cs(&s_fqaCS);
		if (m_disableFills)
		{
			return;
		}

		LastFrameSoundInfo soundInfo;
		if (!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
		{
			return;
		}

		if (!soundInfo.format)
		{
			return;
		}

		WAVEFORMATEX format;
		if (!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
		{
			return;
		}


		Slot& slot = m_slots[m_nextWriteAudio];
		avidebugprintf(L"FillAudioFrame: slot %d, movie frame %d\n", m_nextWriteAudio, movie.currentFrame);
		m_nextWriteAudio++;
		m_nextWriteAudio %= numSlots;

		slot.hasProcessedAudio.WaitUntilFalse();

		slot.audioFrameSamples = soundInfo.size / format.nBlockAlign;

		ReserveBuffer(slot.audioBuffer, slot.audioBufferAllocated, soundInfo.size);
		ReadProcessMemory(captureProcess, soundInfo.buffer, slot.audioBuffer, soundInfo.size, NULL);
		slot.inAudioSize = soundInfo.size;

		slot.inAudioSeconds = (double)soundInfo.size / format.nAvgBytesPerSec;

		slot.hasProcessedAudio = true;
	}

	void FillEmptyAudioFrame()
	{
		AutoCritSect cs(&s_fqaCS);
		if (m_disableFills)
		{
			return;
		}

		LastFrameSoundInfo soundInfo;
		if (!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
		{
			return;
		}

		if (!soundInfo.format)
		{
			return;
		}

		WAVEFORMATEX format;
		if (!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
		{
			return;
		}

		Slot& slot = m_slots[m_nextWriteAudio];
		avidebugprintf(L"FillEmptyAudioFrame: slot %d, movie frame %d\n", m_nextWriteAudio, movie.currentFrame);
		m_nextWriteAudio++;
		m_nextWriteAudio %= numSlots;

		slot.hasProcessedAudio.WaitUntilFalse();

		slot.audioFrameSamples = soundInfo.size / format.nBlockAlign;

		ReserveBuffer(slot.audioBuffer, slot.audioBufferAllocated, soundInfo.size);
		if (format.wBitsPerSample == 8)
		{
			memset(slot.audioBuffer, 0x80, soundInfo.size);
		}
		else
		{
			memset(slot.audioBuffer, 0, soundInfo.size);
		}

		slot.inAudioSize = soundInfo.size;
		slot.inAudioSeconds = (double)soundInfo.size / format.nAvgBytesPerSec;

		slot.hasProcessedAudio = true;
	}

	struct Slot
	{
		WaitableBool hasProcessedVideo;
		WaitableBool hasProcessedAudio;
		unsigned char* aviPixels;
		int aviPixelsAllocated;
		unsigned char* audioBuffer;
		int audioBufferAllocated;
		int inAudioSize, audioFrameSamples;
		double inAudioSeconds; // hack
		int framecount;
		int slotNum; // for debugging
		Slot() : hasProcessedVideo(false), hasProcessedAudio(false),
			aviPixels(NULL), aviPixelsAllocated(0),
			audioBuffer(NULL), audioBufferAllocated(0),
			audioFrameSamples(0), inAudioSize(0), inAudioSeconds(0)
		{
			framecount = -1;
			slotNum = 0;
		}

		~Slot()
		{
			Deallocate();
		}

		void Deallocate()
		{
			free(aviPixels);
			aviPixels = NULL;
			free(audioBuffer);
			audioBuffer = NULL;
			aviPixelsAllocated = 0;
			audioBufferAllocated = 0;
		}

		private:

			void OutputVideoFrame()
			{
				if (Config::localTASflags.aviMode & 1)
				{
					int videoFrameSize = curAviWidth * curAviHeight * (24 / 8);
					LONG bytesWritten = 0;

					{
						AutoCritSect cs(&s_aviCS);

						if (!aviCompressedStream)
						{
							return;
						}

#ifdef _DEBUG
						DWORD time1 = timeGetTime();
#endif

						HRESULT hr = SafeAVIStreamWrite(aviCompressedStream, aviFrameCount, 1, aviPixels, videoFrameSize, 0, NULL, &bytesWritten);

#ifdef _DEBUG
						DWORD time2 = timeGetTime();
						debugprintf(L"AVI: video codec took %d ticks\n", (int)(time2-time1));
#endif
						if (FAILED(hr))
						{ 
							debugprintf(L"AVIStreamWrite failed! (0x%X)\n", hr);
							NormalMessageBox("AVIStreamWrite failed! (Sorry... try restarting this program and/or choosing a different codec.)\n", "Error", MB_OK|MB_ICONERROR);
							CloseAVI();
							Config::localTASflags.aviMode = 0;
							tasFlagsDirty = true;
						}
					}

					aviFrameCount++;
					if (!bytesWritten)
					{
						if (aviEmptyFrameCount == 0 && (Config::localTASflags.aviMode & 2) && aviFrameCount < 300 && !aviSplitCount)
						{
							CustomMessageBox("The video encoder you chose is outputting some null frames.\nThis may confuse video players into adding delays and letting the sound stream get out of sync.", "Warning", MB_OK | MB_ICONWARNING);
						}

						aviEmptyFrameCount++;;
					}
					aviFilesize += bytesWritten;

					if (aviFrameCount == 1)
					{
						mainMenuNeedsRebuilding = true;
					}

					if ((aviFilesize >> 10) > ((unsigned int)1953 << 10)) // roughly 2 GB
					{
						requestedAviSplitCount = aviSplitCount + 1;
					}

					avidebugprintf(L"Output:    slot %d, movie frame %d, video frame %d\n", slotNum, framecount, aviFrameCount);
				}
			}

			void OutputAudioFrame()
			{
				if (Config::localTASflags.aviMode & 2)
				{
					if (!audioConverterStream)
					{
						return;
					}

					double nextAviSoundSecondsCount;
					if (Config::localTASflags.aviMode & 1)
					{
						// try to correct for if the game sends us more audio than video data
						nextAviSoundSecondsCount = aviSoundSecondsCount + inAudioSeconds;
					}
 
					ConvertOutput output = audioConverterStream->Convert((const BYTE*)audioBuffer, inAudioSize);
					if (audioConverterStream->m_failed)
					{
						return;
					}

					int audioFrameSize = output.m_size;
					LONG bytesWritten = audioFrameSize;

					{
						AutoCritSect cs(&s_aviCS);

						if (!aviSoundStream)
						{
							return;
						}

						HRESULT hr = SafeAVIStreamWrite(aviSoundStream, aviSoundSampleCount, audioFrameSamples, output.m_buffer, audioFrameSize, 0, nullptr, &bytesWritten);
						if (FAILED(hr))
						{ 
							debugprintf(L"AVIStreamWrite(audio) failed! (0x%X)\n", hr);
							NormalMessageBox("AVIStreamWrite(audio) failed!\n", "Error", MB_OK|MB_ICONERROR);
							CloseAVI();
							Config::localTASflags.aviMode = 0;
							tasFlagsDirty = true;
						}
					}

					aviSoundSampleCount += audioFrameSamples;
					if (Config::localTASflags.aviMode & 1)
					{
						aviSoundSecondsCount = nextAviSoundSecondsCount;
						aviSoundFrameCount = (int)(aviSoundSecondsCount * curAviFps);
					}
					else
					{
						aviSoundFrameCount++;
					}

					aviFilesize += bytesWritten;

					if (aviSoundFrameCount == 1)
					{
						mainMenuNeedsRebuilding = true;
					}

					if ((aviFilesize >> 10) > ((unsigned int)1953 << 10)) // roughly 2 GB
					{
						requestedAviSplitCount = aviSplitCount + 1;
					}

					avidebugprintf(L"Output:    slot %d, movie frame %d, audio frame %d\n", slotNum, framecount, aviSoundFrameCount);
				}
			}

			friend AviFrameQueue<numSlots>;
	};

	HANDLE thread;
	HANDLE threadAudio;
	bool threadDone;

	static DWORD WINAPI OutputVideoThreadFunc(LPVOID lpParam)
	{
		static_cast<AviFrameQueue<numSlots>*>(lpParam)->OutputVideo();
		return 0;
	}

	static DWORD WINAPI OutputAudioThreadFunc(LPVOID lpParam)
	{
		static_cast<AviFrameQueue<numSlots>*>(lpParam)->OutputAudio();
		return 0;
	}

	void OutputVideo()
	{
		while (!threadDone)
		{
			Slot& slot = m_slots[m_nextRead];

			slot.hasProcessedVideo.WaitUntilTrue();

			if (!threadDone)
			{
				slot.OutputVideoFrame();
			}

			slot.hasProcessedVideo = false;

			m_nextRead++;
			m_nextRead %= numSlots;
		}
	}
	void OutputAudio()
	{
		while (!threadDone)
		{
			Slot& slot = m_slots[m_nextReadAudio];

			slot.hasProcessedAudio.WaitUntilTrue();
			if (!threadDone)
			{
				slot.OutputAudioFrame();
			}
			slot.hasProcessedAudio = false;

			m_nextReadAudio++;
			m_nextReadAudio %= numSlots;
		}
	}

private:
	Slot m_slots[numSlots];
	int m_nextWrite;
	int m_nextWriteAudio;
	int m_nextRead;
	int m_nextReadAudio;
	bool m_disableFills;
};

// the number here decides how many buffers we keep
// (1 means single-buffered = bad for threading, 2 means double-buffered = slightly better...)
AviFrameQueue<4>* aviFrameQueue = new AviFrameQueue<4>();

static bool aviLibraryOpened = false;
static PAVIFILE aviFile = NULL;
static PAVISTREAM aviStream = NULL;
static PAVISTREAM aviSoundStream = NULL;
static PAVISTREAM aviCompressedStream = NULL;
static int curAviWidth=0, curAviHeight=0, curAviFps=0;
int aviFrameCount = 0, aviEmptyFrameCount = 0;
static bool oldIsBasicallyEmpty = false;
static int aviSoundSampleCount = 0;
int aviSoundFrameCount = 0;
static double aviSoundSecondsCount = 0;
static int aviFilesize = 0;
static CRITICAL_SECTION s_aviCS;
static CRITICAL_SECTION s_fqaCS;
static CRITICAL_SECTION s_fqvCS;

void InitAVICriticalSections()
{
	InitializeCriticalSection(&s_aviCS);
	InitializeCriticalSection(&s_fqaCS);
	InitializeCriticalSection(&s_fqvCS);
}

void CloseAVI()
{
	if (aviStream)
	{
		oldIsBasicallyEmpty |= (aviFrameCount - aviEmptyFrameCount < 5 && aviSoundFrameCount < 15);
	}

	if (aviFrameQueue)
	{
		aviFrameQueue->Flush();
	}

	AutoCritSect cs1(&s_fqaCS);
	AutoCritSect cs2(&s_fqvCS);
	delete aviFrameQueue;
	aviFrameQueue = NULL;

	Config::localTASflags.aviMode = 0;
	aviSplitCount = 0;
	aviSplitDiscardCount = 0;

	aviFrameCount = 0;
	aviEmptyFrameCount = 0;
	aviSoundFrameCount = 0;
	aviSoundSampleCount = 0;
	aviSoundSecondsCount = 0;
	aviFilesize = 0;

	AutoCritSect cs(&s_aviCS);

	if (aviStream)
	{
		AVIStreamClose(aviStream);
	}
	aviStream = NULL;
	
	if (aviCompressedStream)
	{
		AVIStreamClose(aviCompressedStream);
	}
	aviCompressedStream = NULL;

	if (aviSoundStream)
	{
		AVIStreamClose(aviSoundStream);
	}
	aviSoundStream = NULL;

	if (aviFile)
	{
		AVIFileClose(aviFile);
	}
	aviFile = NULL;

	delete audioConverterStream;
	audioConverterStream = NULL;

	if (aviLibraryOpened)
	{
		AVIFileExit();
	}
	aviLibraryOpened = false;

	aviFrameQueue = new AviFrameQueue<4>();

	tasFlagsDirty = true;
	mainMenuNeedsRebuilding = true;
}

bool SetAVIFilename(char* filename)
{
	if (strlen(filename) > MAX_PATH + 1) // filename too long
	{
		return false;
	}

	strcpy(avifilename, filename);
	return true;
}

void SetCaptureProcess(HANDLE process)
{
	captureProcess = process;
}

void SetLastFrameSoundInfo(void* soundInfoPointer)
{
	lastFrameSoundInfo = soundInfoPointer;
}

void SetPaletteEntriesPointer(void* pointer)
{
	paletteEntriesPointer = pointer;
}

bool OpenAVIFile(int width, int height, int bpp, int fps)
{
	int oldAviMode = Config::localTASflags.aviMode;
	int oldAviSplitCount = aviSplitCount;
	int oldAviSplitDiscardCount = aviSplitDiscardCount;
	CloseAVI();
	Config::localTASflags.aviMode = oldAviMode;
	aviSplitCount = oldAviSplitCount;
	aviSplitDiscardCount = oldAviSplitDiscardCount;

	const char* filename = avifilename;

	if (aviSplitCount > 0)
	{
		if (oldIsBasicallyEmpty)
		{
			// discard empty or almost-empty AVI segments
			aviSplitDiscardCount++;
		}

		static char avifilename2 [MAX_PATH+1];
		strcpy(avifilename2, avifilename);
		char* dot = strrchr(avifilename2, '.');
		if (dot)
		{
			*dot = 0;
		}
		else
		{
			dot = avifilename2 + strlen(avifilename2);
		}

		int segNum = aviSplitCount + 1 - aviSplitDiscardCount;
		if (segNum > 1)
		{
			_snprintf(dot, MAX_PATH + 1 - strlen(avifilename2), "%03d%s", segNum, dot - avifilename2 + avifilename);
			filename = avifilename2;
		}
	}

	oldIsBasicallyEmpty = false;

	debugprintf(__FUNCTIONW__ L"(filename=\"%S\", width=%d, height=%d, bpp=%d, fps=%d)\n", filename, width, height, bpp, fps);

	AutoCritSect cs(&s_aviCS);

	if (!aviLibraryOpened)
	{
		AVIFileInit();
		aviLibraryOpened = true;
	}

	_unlink(filename);
	HRESULT hr = AVIFileOpen(&aviFile, filename, OF_WRITE | OF_CREATE, NULL);
	if (FAILED(hr))
	{
		char str[MAX_PATH + 64];
		sprintf(str, "AVIFileOpen(\"%s\") failed!\n", filename);
		debugprintf(L"%S", str);
		NormalMessageBox(str, "Error", MB_OK|MB_ICONERROR);
		return false;
	}

	if (width && height && fps)
	{
		BITMAPINFO bmpInfo = 
		{
			// BITMAPINFOHEADER
			{ 
				sizeof(BITMAPINFO), 
				width, height, static_cast<WORD>(1),
				static_cast<WORD>(bpp), BI_RGB, 
				static_cast<DWORD>(width) * static_cast<DWORD>(height) * static_cast<DWORD>(bpp) / static_cast<DWORD>(8),
			}
		};  

		AVISTREAMINFO streamInfo = { streamtypeVIDEO };
		streamInfo.dwRate = fps;
		streamInfo.dwScale = 1;
		streamInfo.dwQuality = -1;
		streamInfo.rcFrame.right = width;
		streamInfo.rcFrame.bottom = height;

		hr = AVIFileCreateStream(aviFile, &aviStream, &streamInfo);
		if (FAILED(hr))
		{ 
			debugprintf(L"AVIFileCreateStream failed!\n");
			NormalMessageBox("AVIFileCreateStream failed!\n", "Error", MB_OK|MB_ICONERROR);
			CloseAVI();
			return false;
		}

		int chooseIter = 0;

chooseAnotherFormat:
		chooseIter++;
		static AVICOMPRESSOPTIONS options = {0};
		AVICOMPRESSOPTIONS* pOptions = &options;
		if (!aviSplitCount || chooseIter > 1)
		{
			if (!AVISaveOptions(/*hWnd*/NULL, ICMF_CHOOSE_KEYFRAME|ICMF_CHOOSE_DATARATE, 1, &aviStream, &pOptions))
			{
				CloseAVI();
				return false;
			}
		}

		hr = AVIMakeCompressedStream(&aviCompressedStream, aviStream, pOptions, NULL);
		if (FAILED(hr))
		{ 
			debugprintf(L"AVIMakeCompressedStream failed! (0x%X)\n", hr);
			NormalMessageBox("AVIMakeCompressedStream failed!\n", "Error", MB_OK|MB_ICONERROR);
			goto chooseAnotherFormat;
		}

		if (bmpInfo.bmiHeader.biWidth & 1) 
		{ 
			bmpInfo.bmiHeader.biWidth--; 
		}

		if (bmpInfo.bmiHeader.biHeight & 1)
		{ 
			bmpInfo.bmiHeader.biHeight--; 
		}

		hr = AVIStreamSetFormat(aviCompressedStream, 0, &bmpInfo, sizeof(bmpInfo));
		if (FAILED(hr))
		{ 
			debugprintf(L"AVIStreamSetFormat failed! (0x%X)\n", hr);
			NormalMessageBox("AVIStreamSetFormat failed!\n", "Error", MB_OK|MB_ICONERROR);
			AVIStreamClose(aviCompressedStream);
			aviCompressedStream = NULL;
			goto chooseAnotherFormat;
		}
	}

	aviFrameCount = 0;
	aviEmptyFrameCount = 0;

	curAviWidth = width;
	curAviHeight = height;
	curAviFps = fps;
	return true;
}

void WriteAVIFrame(void* remotePixels, int width, int height, int pitch, int bpp, int rmask, int gmask, int bmask)
{
	if (!(Config::localTASflags.aviMode & 1))
	{
		// double-check...
		return;
	}

	int fps = Config::localTASflags.framerate;
	if (fps <= 0)
	{
		fps = 60;
	}

	if (!aviFile || !aviCompressedStream || curAviWidth != width || curAviHeight != height || curAviFps != fps)
	{
		if (aviFile)
		{
			aviSplitCount++;
		}

		if (!OpenAVIFile(width, height, 24, fps))
		{
			Config::localTASflags.aviMode = 0;
			tasFlagsDirty = true;
			return;
		}
	}

	// producer
	aviFrameQueue->FillFrame(remotePixels, width, height, pitch, bpp, rmask, gmask, bmask);
}

void RewriteAVIFrame()
{
	if ((Config::localTASflags.aviMode & 1) && aviCompressedStream)
	{
		aviFrameQueue->RefillFrame();
	}
}

WaveFormat chooseFormat;

bool ChooseAudioCodec(const LPWAVEFORMATEX defaultFormat)
{
	if (Config::localTASflags.framerate <= 0)
	{
		int result = CustomMessageBox("This is a no-framerate movie,\nso audio capture is unlikely to work well.", "Warning", MB_OKCANCEL | MB_ICONWARNING);
		
		if (result == IDCANCEL)
		{
			return false;
		}
	}

	chooseFormat = defaultFormat;

	ACMFORMATCHOOSE choose = { sizeof(ACMFORMATCHOOSE) };
	choose.fdwStyle  = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
	choose.hwndOwner = NULL; //hWnd;
	choose.pwfx      = chooseFormat;
	choose.cbwfx     = chooseFormat.GetMaxSize();
	choose.pszTitle  = "Choose Audio Codec";
	choose.fdwEnum   = ACM_FORMATENUMF_OUTPUT;
	// don't use ACM_FORMATENUMF_CONVERT because that's too conservative
	// and would hide many of the conversions we're able to handle.
	// ACM_FORMATENUMF_SUGGEST is even worse.

	return acmFormatChoose(&choose) == MMSYSERR_NOERROR;
}   

int OpenAVIAudioStream()
{
	if (!lastFrameSoundInfo)
	{
		return 0;
	}

	{
		AutoCritSect cs(&s_aviCS);

		LastFrameSoundInfo soundInfo;
		if (!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
		{
			return -1;
		}

		if (!soundInfo.format)
		{
			return 0;
		}

		WAVEFORMATEX format;
		if (!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
		{
			return -1;
		}

		// open the output file if it's not already open
		if (!aviFile)
		{
			if (!(Config::localTASflags.aviMode & 1))
			{
				curAviWidth = 0;
				curAviHeight = 0;
				curAviFps = 0;
			}
			else
			{
				if (!curAviWidth && !curAviHeight && !curAviFps)
				{
					// if video recording is on, wait until first frame of video
					return 0;
				}
			}

			if (!OpenAVIFile(curAviWidth, curAviHeight, 24, curAviFps))
			{
				Config::localTASflags.aviMode = 0;
				tasFlagsDirty = true;
				return -1;
			}
		}

		LPWAVEFORMATEX outputFormat = &format;
		int outputFormatSize = sizeof(WAVEFORMATEX) + outputFormat->cbSize;

		for (int chooseIter = 1;; chooseIter++)
		{
			if ((aviSplitCount && chooseIter <= 1) || ChooseAudioCodec(&format))
			{
				// user chose a format
				outputFormat = chooseFormat;
				outputFormatSize = sizeof(WAVEFORMATEX) + outputFormat->cbSize;
			}
			else
			{
				// user cancelled
				Config::localTASflags.aviMode &= ~2;
				tasFlagsDirty = true;
				return -1;
			}

			// figure out how to convert to the chosen format
			audioConverterStream = new AudioConverterStream(format, chooseFormat);
			if (audioConverterStream->m_failed)
			{
				// try again
				debugprintf(L"AudioConverterStream() failed!\n");
				NormalMessageBox("Couldn't find a valid conversion sequence.\nTry a different audio codec.\n", "Error", MB_OK|MB_ICONERROR);
				delete audioConverterStream;
				audioConverterStream = NULL;
			}
			else
			{
				// done choosing format
				break;
			}
		}

		AVISTREAMINFO streamInfo = { streamtypeAUDIO };
		streamInfo.dwRate = outputFormat->nAvgBytesPerSec;
		streamInfo.dwScale = outputFormat->nBlockAlign;
		streamInfo.dwSampleSize = outputFormat->nBlockAlign;
		streamInfo.dwQuality = -1;
		streamInfo.dwInitialFrames = 0;

		HRESULT hr = AVIFileCreateStream(aviFile, &aviSoundStream, &streamInfo);
		if (FAILED(hr))
		{ 
			debugprintf(L"AVIFileCreateStream(audio) failed!\n");
			NormalMessageBox("AVIFileCreateStream(audio) failed!\nCapture will continue without audio\n", "Error", MB_OK|MB_ICONERROR);
			Config::localTASflags.aviMode &= ~2;
			tasFlagsDirty = true;
			return -1;
		}

		hr = AVIStreamSetFormat(aviSoundStream, 0, outputFormat, outputFormatSize);
		if (FAILED(hr))
		{ 
			debugprintf(L"AVIStreamSetFormat(audio) failed!\n");
			NormalMessageBox("AVIStreamSetFormat(audio) failed!\nCapture will continue without audio\n", "Error", MB_OK|MB_ICONERROR);
			Config::localTASflags.aviMode &= ~2;
			tasFlagsDirty = true;
			return -1;
		}
	}

	aviSoundSampleCount = 0;
	aviSoundSecondsCount = 0;
	aviSoundFrameCount = 0;

	return 1;
}

void WriteAVIAudio()
{
	if (!aviSoundStream)
	{
		int res = OpenAVIAudioStream();
		if (!res)
		{
			return;
		}

		if (res < 0)
		{
			Config::localTASflags.aviMode &= ~2;
			tasFlagsDirty = true;
			return;
		}
	}

	if (aviCompressedStream && (aviSoundFrameCount < 30 || aviSoundFrameCount+8 < aviFrameCount) && !aviSplitCount)
	{
		if (aviSoundFrameCount < aviFrameCount)
		{
			AutoCritSect cs(&s_fqaCS); // critical section and check again in case we got here while CloseAVI is running
			if (aviCompressedStream && (aviSoundFrameCount < 30 || aviSoundFrameCount + 8 < aviFrameCount) && !aviSplitCount)
			{
				while (aviSoundFrameCount < aviFrameCount)
				{
					// in case video started before audio
					aviFrameQueue->FillEmptyAudioFrame();
				}
			}
		}
	}

	aviFrameQueue->FillAudioFrame();
}

void SplitAVINow()
{
	requestedAviSplitCount = aviSplitCount + 1;
}

void HandleAviSplitRequests()
{
	if (requestedAviSplitCount > aviSplitCount)
	{
		int oldAviMode = Config::localTASflags.aviMode;
		int oldAviSplitDiscardCount = aviSplitDiscardCount;
		int oldRequestedAviSplitCount = requestedAviSplitCount;
		Sleep(200); // this isn't a fix for anything, the proper waits are in place. but it can't hurt...
		CloseAVI();
		Config::localTASflags.aviMode = oldAviMode;
		aviSplitDiscardCount = oldAviSplitDiscardCount;
		requestedAviSplitCount = oldRequestedAviSplitCount;
		aviSplitCount = requestedAviSplitCount;
		requestedAviSplitCount = 0;
		tasFlagsDirty = true;
	}
}

void ProcessCaptureFrameInfo(LPCVOID frameCaptureInfoRemoteAddr, int frameCaptureInfoType)
{
	switch (frameCaptureInfoType)
	{
	case CAPTUREINFO_TYPE_NONE:
	case CAPTUREINFO_TYPE_NONE_SUBSEQUENT:
		break;

	case CAPTUREINFO_TYPE_PREV:
		RewriteAVIFrame();
		break;

	case CAPTUREINFO_TYPE_DDSD:
		if ((Config::localTASflags.aviMode & 1) && frameCaptureInfoRemoteAddr)
		{
			DDSURFACEDESC desc = {0};
			ReadProcessMemory(captureProcess, frameCaptureInfoRemoteAddr, &desc, sizeof(desc), NULL);
			WriteAVIFrame(desc.lpSurface, desc.dwWidth, desc.dwHeight, desc.lPitch, desc.ddpfPixelFormat.dwRGBBitCount,
				desc.ddpfPixelFormat.dwRBitMask, desc.ddpfPixelFormat.dwGBitMask, desc.ddpfPixelFormat.dwBBitMask);
		}
		break;
	}
}

void ProcessCaptureSoundInfo()
{
	WriteAVIAudio();
}
