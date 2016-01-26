#include <windows.h>

#include "AVIDumper.h"
#include "logging.h"
#include "CustomDLGs.h"

#include <malloc.h>
#include <stdio.h>

//#include "Resource.h"
#include "../external/ddraw.h"
#include "../shared/winutil.h"

#include <vfw.h>
#pragma comment(lib, "vfw32.lib")
#include <msacm.h>
#pragma comment(lib, "msacm32.lib")

#include "../shared/ipc.h"
#include "Config.h"

//extern TasFlags localTASflags;
extern bool tasFlagsDirty;
extern bool mainMenuNeedsRebuilding; // TODO: Elimitate this from here <----

static int aviSplitCount = 0;
static int aviSplitDiscardCount = 0;
static int requestedAviSplitCount = 0;

static void* lastFrameSoundInfo = 0;
static void* paletteEntriesPointer = 0;
bool g_gammaRampEnabled = false;

static HANDLE captureProcess = NULL;

char avifilename [MAX_PATH+1];

#define avidebugprintf verbosedebugprintf

#ifdef _DEBUG
#define _AVIDEBUG
#endif

#if defined(_AVIDEBUG) || 0 //1
	#define verbosedebugprintf debugprintf

	#include "Movie.h"
	extern Movie movie;
#else
	#if _MSC_VER > 1310
		#define verbosedebugprintf(...) ((void)0)
	#else
		#define verbosedebugprintf() ((void)0)
		#pragma warning(disable:4002)
	#endif
#endif

// wrapper for LPWAVEFORMATEX that lets me pass it around like a value type
// without worrying about the original pointer needing to stay valid
struct WaveFormat
{
	WaveFormat(const LPWAVEFORMATEX format=NULL) : m_format(NULL) { Init(format); }
	WaveFormat(const WAVEFORMATEX& format) : m_format(NULL) { Init((LPWAVEFORMATEX)&format); }
	WaveFormat(const WaveFormat& format) : m_format(NULL) { Init(format.m_format); }
	WaveFormat& operator= (const WaveFormat& format) { Init(format.m_format); return *this; }
	~WaveFormat() { free(m_format); }
	operator LPWAVEFORMATEX () { return m_format; }
	operator const LPWAVEFORMATEX () const { return m_format; }
	size_t GetSize() { return m_format ? sizeof(WAVEFORMATEX) + m_format->cbSize : 0; }
	size_t GetMaxSize() { if(!m_format) return 0; size_t size = GetSize(); acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &size); return size; }
	LPWAVEFORMATEX m_format;
private:
	void Init(const LPWAVEFORMATEX format)
	{
		if(format == m_format)
			return;
		if(m_format)
		{
			free(m_format);
			m_format = NULL;
		}
		if(format)
		{
			size_t size = sizeof(WAVEFORMATEX) + format->cbSize;
			acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &size); // to simplify in-place format changing
			m_format = (LPWAVEFORMATEX)malloc(size);
			memcpy(m_format, format, size);
		}
	}
};


class AudioConverterStream
{
public:
	AudioConverterStream(WaveFormat sourceFormat, WaveFormat destFormat, WaveFormat** prevSourceFormats=NULL, int numPrevSourceFormats=0)
		: outBuffer(NULL), outSize(0), outBufferAllocated(0), stream(NULL), isProxy(false), failed(false), subConverter(NULL), startOffset(0)
	{
		if(sourceFormat.GetSize() == destFormat.GetSize() && !memcmp((LPWAVEFORMATEX)sourceFormat, (LPWAVEFORMATEX)destFormat, sourceFormat.GetSize()))
		{
			// source and destination formats are identical, so we'll just pass the input through to the output
			isProxy = true;
			return;
		}

		MMRESULT mm = acmStreamOpen(&stream, NULL, sourceFormat, destFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);
		if(mm != MMSYSERR_NOERROR)
		{
			// not supported directly, so try letting ACM suggest an intermediate format
			// so we can perform the conversion in multiple stages
			WaveFormat intermediateFormat;
			// this weird looping structure is because we must try all combinations of four flags,
			// and even when a given combination yields a valid suggestion,
			// we must be able to backtrack and continue looping (tryMoreSuggestions)
			// if the conversion based on that suggestion later fails or dead-ends.
			bool foundSuggest = false;
			int chan, samp, bits, form, done;
			for(done = 0; done < 1 && !foundSuggest; foundSuggest ? 0 : ++done)
			{
				for(chan = 1; chan >= 0 && !foundSuggest; foundSuggest ? 0 : --chan)
				{
					for(samp = 1; samp >= 0 && !foundSuggest; foundSuggest ? 0 : --samp)
					{
						for(bits = 0; bits <= 1 && !foundSuggest; foundSuggest ? 0 : ++bits)
						{
							for(form = 0; form <= 1 && !foundSuggest; foundSuggest ? 0 : ++form)
							{
								int flags = 0;
								if(chan) flags |= ACM_FORMATSUGGESTF_NCHANNELS;
								if(samp) flags |= ACM_FORMATSUGGESTF_NSAMPLESPERSEC;
								if(bits) flags |= ACM_FORMATSUGGESTF_WBITSPERSAMPLE;
								if(form) flags |= ACM_FORMATSUGGESTF_WFORMATTAG;

								intermediateFormat = destFormat;
								MMRESULT mmSuggest = acmFormatSuggest(NULL, sourceFormat, intermediateFormat, intermediateFormat.GetSize(), flags);
								if(mmSuggest == MMSYSERR_NOERROR)
								{
									// got a possibly-valid suggestion,
									// but it might be a suggestion to do absolutely nothing (which would be bad)
									// so first make sure there's some sort of change involved:
									if(sourceFormat.m_format->nChannels != intermediateFormat.m_format->nChannels
									|| sourceFormat.m_format->nSamplesPerSec != intermediateFormat.m_format->nSamplesPerSec
									|| sourceFormat.m_format->wBitsPerSample != intermediateFormat.m_format->wBitsPerSample
									|| sourceFormat.m_format->wFormatTag != intermediateFormat.m_format->wFormatTag)
									{
										// got a suggestion
										foundSuggest = true;

										// now check to see if it's identical to a previous conversion state.
										// if it is, then we'll revert foundSuggest to false
										// to prevent endless conversion cycles.
										for(int prev = 0; prev < numPrevSourceFormats && prevSourceFormats && foundSuggest; prev++)
										{
											WaveFormat& oldFormat = *prevSourceFormats[prev];
											if(oldFormat.m_format->nChannels == intermediateFormat.m_format->nChannels
											&& oldFormat.m_format->nSamplesPerSec == intermediateFormat.m_format->nSamplesPerSec
											&& oldFormat.m_format->wBitsPerSample == intermediateFormat.m_format->wBitsPerSample
											&& oldFormat.m_format->wFormatTag == intermediateFormat.m_format->wFormatTag)
											{
												// already went through this exact format
												foundSuggest = false;
											}
										}
									}
								}
tryMoreSuggestions:
								continue;
							}
						}
					}
				}
			}
			if(!foundSuggest)
			{
				failed = true;
				return;
			}

			// we'll handle conversion to the intermediate format
			mm = acmStreamOpen(&stream, NULL, sourceFormat, intermediateFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);
			if(mm != MMSYSERR_NOERROR)
			{
				if(!done)
				{
					foundSuggest = false;
					goto tryMoreSuggestions; // continue the search
				}

				// reached dead end
				failed = true;
				return;
			}

			// create temporary updated conversion history for cycle prevention
			WaveFormat** prevFormats = (WaveFormat**)alloca(sizeof(WaveFormat*) * (numPrevSourceFormats + 1));
			if(prevSourceFormats)
				memcpy(prevFormats, prevSourceFormats, sizeof(WaveFormat*) * numPrevSourceFormats);
			prevFormats[numPrevSourceFormats] = &sourceFormat;

			// delegate the rest of the conversion to a new converter (recursive construction)
			subConverter = new AudioConverterStream(intermediateFormat, destFormat, prevFormats, numPrevSourceFormats + 1);
			if(subConverter->failed)
			{
				delete subConverter;
				subConverter = NULL;

				if(!done)
				{
					foundSuggest = false;
					goto tryMoreSuggestions; // continue the search
				}

				// reached dead end
				failed = true;
				return;
			}
		}

		// prepare the stream header
		memset(&header, 0, sizeof(ACMSTREAMHEADER));
		header.cbStruct = sizeof(ACMSTREAMHEADER);
		header.pbSrc = inWorkBuffer;
		header.cbSrcLength = sizeof(inWorkBuffer);
		header.pbDst = outWorkBuffer;
		header.cbDstLength = sizeof(outWorkBuffer);
		mm = acmStreamPrepareHeader(stream, &header, 0);

		if(mm != MMSYSERR_NOERROR)
			failed = true;
	}
	~AudioConverterStream()
	{
		if(isProxy)
			return;
		delete subConverter;
		acmStreamUnprepareHeader(stream, &header, 0);
		acmStreamClose(stream, 0);
		free(outBuffer);
	}
	struct ConvertOutput
	{
		BYTE* buffer;
		int size;
	};
	ConvertOutput Convert(const BYTE* inBuffer, int inSize)
	{
		if(isProxy)
		{
			// source and destination formats are identical, so just pass the input through to the output
			ConvertOutput rv = {(BYTE*)inBuffer, inSize};
			return rv;
		}

		outSize = 0;
		int prevSrcLength = header.cbSrcLength;
		int usedInSize = 0;
		while(inSize + startOffset)
		{
			int curInSize = min(inSize, (int)sizeof(inWorkBuffer) - startOffset);
			memcpy(inWorkBuffer + startOffset, inBuffer + usedInSize, curInSize);
			usedInSize += curInSize;
			inSize -= curInSize;
			header.cbSrcLength = curInSize + startOffset;
			MMRESULT mm = acmStreamConvert(stream, &header, ACM_STREAMCONVERTF_BLOCKALIGN);
			if(mm != MMSYSERR_NOERROR)
			{
				failed = true;
				break;
			}

			// append to output
			int prevOutSize = outSize;
			outSize += header.cbDstLengthUsed;
			ReserveOutBuffer(outSize);
			memcpy(outBuffer + prevOutSize, outWorkBuffer, header.cbDstLengthUsed);

			// recycle unused input
			startOffset = header.cbSrcLength - header.cbSrcLengthUsed;
			if(startOffset > 0 && header.cbSrcLengthUsed) // startOffset = number of leftover bytes
				memmove(inWorkBuffer, inWorkBuffer + header.cbSrcLengthUsed, startOffset);

			if(header.cbDstLengthUsed == 0 && header.cbSrcLengthUsed == 0)
				break;
		}
		header.cbSrcLength = prevSrcLength;

		if(header.cbDstLengthUsed == 0 && header.cbSrcLengthUsed == 0 && startOffset == sizeof(inWorkBuffer))
		{
			failed = true;
		}

		if(subConverter)
			return subConverter->Convert(outBuffer, outSize);

		ConvertOutput rv = {outBuffer, outSize};
		return rv;
	}

	bool failed;

private:
	BYTE* outBuffer;
	int outSize;
	ACMSTREAMHEADER header;
	BYTE inWorkBuffer [16*1024];
	BYTE outWorkBuffer [4*1024];
	int outBufferAllocated;
	int startOffset;
	HACMSTREAM stream;
	bool isProxy;
	AudioConverterStream* subConverter;
	void ReserveOutBuffer(int size)
	{
		if(outBufferAllocated < size)
		{
			outBufferAllocated = size;
			outBuffer = (BYTE*)realloc(outBuffer, outBufferAllocated);
		}
	}
};
AudioConverterStream* audioConverterStream = NULL;

//struct AviFrameArgs
//{
//	int width; int height; int pitch; int bpp; int rmask; int gmask; int bmask;
//};


class WaitableBool
{
public:
	WaitableBool(bool initialState = false) : m_value(initialState)
	{
		m_event = CreateEvent(NULL, TRUE, initialState, NULL);
		m_eventInverse = CreateEvent(NULL, TRUE, !initialState, NULL);
	}
	~WaitableBool()
	{
		CloseHandle(m_event);
		CloseHandle(m_eventInverse);
	}
	void WaitUntilTrue() const
	{
		if(!m_value)
			WaitForSingleObject(m_event, INFINITE);
	}
	void WaitUntilFalse() const
	{
		if(m_value)
			WaitForSingleObject(m_eventInverse, INFINITE);
	}
	void operator= (bool set)
	{
		m_value = set;
		if(set)
		{
			SetEvent(m_event);
			ResetEvent(m_eventInverse);
		}
		else
		{
			ResetEvent(m_event);
			SetEvent(m_eventInverse);
		}
	}
	operator bool () const
	{
		return m_value;
	}
private:
	HANDLE m_event;
	HANDLE m_eventInverse;
	bool m_value;
};


HRESULT SafeAVIStreamWrite(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten)
{
	HRESULT hr;
	__try {
		hr = AVIStreamWrite(pavi, lStart, lSamples, lpBuffer, cbBuffer, dwFlags, plSampWritten, plBytesWritten);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
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
	if(mask)
	{
		if(mask < 0xFF)
		{
			while((mask << shift) < 128)
				shift++;
		}
		else
		{
			while((mask >> -shift) > 255)
				shift--;
		}
	}
	return shift;
}

template<typename T>
static void ReserveBuffer(T*& buffer, int& bufferAllocated, int size)
{
	if(!buffer || bufferAllocated < size)
	{
		bufferAllocated = size;
		buffer = (T*)realloc(buffer, size);
	}
}

// warning: gamma ramp is (unlike everything else) stored in a way that doesn't properly reset when loading a savestate.
// this is probably fine since it's currently only used for AVI capture.
DDGAMMARAMP g_gammaRamp;
void SetGammaRamp(void* gammaRampPointer, HANDLE process)
{
	if(gammaRampPointer)
	{
		SIZE_T bytesRead = 0;
		if(ReadProcessMemory(process, gammaRampPointer, &g_gammaRamp, sizeof(DDGAMMARAMP), &bytesRead))
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
		for(int i = 0; i < numSlots; i++)
			m_slots[i].slotNum = i;
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
		for(int i = 0; i < numSlots; i++)
		{
			m_slots[i].hasProcessedVideo = true; // wake up thread to stop it
			m_slots[i].hasProcessedAudio = true;
		}
		DWORD exitCode = STILL_ACTIVE;
		for(int i = 0; exitCode == STILL_ACTIVE && i < 500; i++)
			if(!GetExitCodeThread(thread, &exitCode))
				break;
			else
				Sleep(10);
		if(exitCode == STILL_ACTIVE)
		{
			debugprintf("WARNING: had to force terminate AVI video thread\n");
			if(IsDebuggerPresent())
			{
				_asm{int 3}
			}
			TerminateThread(thread, -1);
		}
		CloseHandle(thread);
		exitCode = STILL_ACTIVE;
		for(int i = 0; exitCode == STILL_ACTIVE && i < 500; i++)
			if(!GetExitCodeThread(threadAudio, &exitCode))
				break;
			else
				Sleep(10);
		if(exitCode == STILL_ACTIVE)
		{
			debugprintf("WARNING: had to force terminate AVI audio thread\n");
			if(IsDebuggerPresent())
			{
				_asm{int 3}
			}
			TerminateThread(threadAudio, -1);
		}
		CloseHandle(threadAudio);
	}

	void Flush()
	{
		for(int i = 0; i < numSlots; i++)
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
		if(m_disableFills)
			return;

		Slot& slot = m_slots[m_nextWrite];
		avidebugprintf("FillFrame: slot %d, movie frame %d\n", m_nextWrite, movie.currentFrame);
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
		if(pitch >= 0)
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
		debugprintf("AVI: reading pixel data took %d ticks\n", (int)(time2-time1));
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
		const int rShiftRight = max(0, -rShiftLeft);
		const int gShiftRight = max(0, -gShiftLeft);
		const int bShiftRight = max(0, -bShiftLeft);
		rShiftLeft = max(0, rShiftLeft);
		gShiftLeft = max(0, gShiftLeft);
		bShiftLeft = max(0, bShiftLeft);

		if(bytesPerInPixel == 4 && rmask == 0x00FF0000 && gmask == 0x0000FF00 && bmask == 0x000000FF)
		{
			// "fast" case
			// TODO: use the MMX-optimized blit32to24loop from kkapture for this case
			for(int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);
				for(int xRem = width+1; --xRem; inPix += 4, aviPix += bytesPerAVIPixel)
					*(unsigned int*)aviPix = *(unsigned int*)inPix;
			}
		}
		else if(bytesPerInPixel == 1 && !rmask && !gmask && !bmask)
		{
			// palettized case
			static PALETTEENTRY activePalette [256];
			ReadProcessMemory(captureProcess, paletteEntriesPointer, &activePalette, sizeof(activePalette), NULL);
			for(int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);
				for(int xDst = 0; xDst < width; xDst++, inPix++, aviPix += bytesPerAVIPixel)
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
			for(int yDst = height-1; yDst >= 0; yDst--)
			{
				unsigned char* inPix = curInPixels + (yDst * pitch);
				for(int xDst = 0; xDst < width; xDst++, inPix += bytesPerInPixel, aviPix += bytesPerAVIPixel)
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
		if(g_gammaRampEnabled)
		{
			aviPix = slot.aviPixels;
			for(int count = aviPixelsSize/bytesPerAVIPixel+1; --count; aviPix += bytesPerAVIPixel)
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
		if(m_disableFills)
			return;

		Slot& prevSlot = m_slots[(numSlots+m_nextWrite-1)%numSlots];
		Slot& slot = m_slots[m_nextWrite];
		avidebugprintf("RefillFrame: slot %d, movie frame %d\n", m_nextWrite, movie.currentFrame);
		m_nextWrite++;
		m_nextWrite %= numSlots;

		slot.hasProcessedVideo.WaitUntilFalse();

#ifdef _AVIDEBUG
		slot.framecount = movie.currentFrame;
#endif

		const int aviPixelsSize = prevSlot.aviPixelsAllocated;
		ReserveBuffer(slot.aviPixels, slot.aviPixelsAllocated, aviPixelsSize);
		if(!slot.aviPixels)
		{
			const int aviPixelsSize = curAviWidth * curAviHeight * (24 / 8) + 8;
			ReserveBuffer(slot.aviPixels, slot.aviPixelsAllocated, aviPixelsSize);
		}
		if(prevSlot.aviPixels)
			memcpy(slot.aviPixels, prevSlot.aviPixels, aviPixelsSize);
		else
			memset(slot.aviPixels, 0, aviPixelsSize);

		slot.hasProcessedVideo = true;
	}

	void FillAudioFrame()
	{
		AutoCritSect cs(&s_fqaCS);
		if(m_disableFills)
			return;

		LastFrameSoundInfo soundInfo;
		if(!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
			return;

		if(!soundInfo.format)
			return;

		WAVEFORMATEX format;
		if(!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
			return;


		Slot& slot = m_slots[m_nextWriteAudio];
		avidebugprintf("FillAudioFrame: slot %d, movie frame %d\n", m_nextWriteAudio, movie.currentFrame);
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
		if(m_disableFills)
			return;

		LastFrameSoundInfo soundInfo;
		if(!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
			return;

		if(!soundInfo.format)
			return;

		WAVEFORMATEX format;
		if(!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
			return;

		Slot& slot = m_slots[m_nextWriteAudio];
		avidebugprintf("FillEmptyAudioFrame: slot %d, movie frame %d\n", m_nextWriteAudio, movie.currentFrame);
		m_nextWriteAudio++;
		m_nextWriteAudio %= numSlots;

		slot.hasProcessedAudio.WaitUntilFalse();

		slot.audioFrameSamples = soundInfo.size / format.nBlockAlign;

		ReserveBuffer(slot.audioBuffer, slot.audioBufferAllocated, soundInfo.size);
		if(format.wBitsPerSample == 8)
			memset(slot.audioBuffer, 0x80, soundInfo.size);
		else
			memset(slot.audioBuffer, 0, soundInfo.size);
		slot.inAudioSize = soundInfo.size;
		slot.inAudioSeconds = (double)soundInfo.size / format.nAvgBytesPerSec;

		slot.hasProcessedAudio = true;
	}

	//void ReclaimMemory()
	//{
	//	m_disableFills = true;
	//	for(int i = 0; i < numSlots; i++)
	//		m_slots[i].Deallocate();
	//	m_disableFills = false;
	//}

	struct Slot
	{
		WaitableBool hasProcessedVideo;
		WaitableBool hasProcessedAudio;
//		unsigned char* inPixels;
//		int inPixelsAllocated;
		unsigned char* aviPixels;
		int aviPixelsAllocated;
		unsigned char* audioBuffer;
		int audioBufferAllocated;
		int inAudioSize, audioFrameSamples;
		double inAudioSeconds; // hack
		int framecount; int slotNum; // for debugging
		Slot() : hasProcessedVideo(false), hasProcessedAudio(false),
			aviPixels(NULL), aviPixelsAllocated(0),
//			inPixels(NULL), inPixelsAllocated(0),
			audioBuffer(NULL), audioBufferAllocated(0),
			audioFrameSamples(0), inAudioSize(0), inAudioSeconds(0)
		{
			framecount = -1;
		}
		~Slot()
		{
			Deallocate();
		}

		void Deallocate()
		{
			//hasProcessedAudio.WaitUntilFalse();
			//hasProcessedVideo.WaitUntilFalse();
//			free(inPixels);
//			inPixels = NULL;
			free(aviPixels);
			aviPixels = NULL;
			free(audioBuffer);
			audioBuffer = NULL;
//			inPixelsAllocated = 0;
			aviPixelsAllocated = 0;
			audioBufferAllocated = 0;
		}

		private:

			void OutputVideoFrame()
			{
				if(Config::localTASflags.aviMode & 1)
				{
			//		AutoCritSect cs(&s_aviCS);

					int videoFrameSize = curAviWidth * curAviHeight * (24 / 8);
					LONG bytesWritten = 0;

					{
						AutoCritSect cs(&s_aviCS);

						if(!aviCompressedStream)
							return;

#ifdef _DEBUG
						DWORD time1 = timeGetTime();
#endif

						HRESULT hr = SafeAVIStreamWrite(aviCompressedStream, aviFrameCount, 1, aviPixels, videoFrameSize, 0, NULL, &bytesWritten);
						//debugprintf("0x%X = SafeAVIStreamWrite(0x%X, %d, 1, 0x%X, %d, 0, NULL, %d)\n", hr, aviCompressedStream, aviFrameCount, aviPixels, videoFrameSize, bytesWritten);

#ifdef _DEBUG
						DWORD time2 = timeGetTime();
						debugprintf("AVI: video codec took %d ticks\n", (int)(time2-time1));
#endif
						if(FAILED(hr))
						{ 
							debugprintf("AVIStreamWrite failed! (0x%X)\n", hr);
							NormalMessageBox("AVIStreamWrite failed! (Sorry... try restarting this program and/or choosing a different codec.)\n", "Error", MB_OK|MB_ICONERROR);
							CloseAVI();
							//CheckDlgButton(hWnd, IDC_AVIVIDEO, 0);
							//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
							Config::localTASflags.aviMode = 0;
							tasFlagsDirty = true;
						}
					}

					aviFrameCount++;
					if(!bytesWritten)
					{
						if(aviEmptyFrameCount == 0 && (Config::localTASflags.aviMode & 2) && aviFrameCount < 300 && !aviSplitCount)
							CustomMessageBox("The video encoder you chose is outputting some null frames.\nThis may confuse video players into adding delays and letting the sound stream get out of sync.", "Warning", MB_OK | MB_ICONWARNING);
						aviEmptyFrameCount++;;
					}
					aviFilesize += bytesWritten;

					if(aviFrameCount == 1)
						mainMenuNeedsRebuilding = true;

					if((aviFilesize>>10) > ((unsigned int)1953 << 10)) // roughly 2 GB
						requestedAviSplitCount = aviSplitCount+1;

					avidebugprintf("Output:    slot %d, movie frame %d, video frame %d\n", slotNum, framecount, aviFrameCount);

//					aviFrameCount++;
				}
			}

			void OutputAudioFrame()
			{
				if(Config::localTASflags.aviMode & 2)
				{
					if(!audioConverterStream)
						return;

					double nextAviSoundSecondsCount;
					if((Config::localTASflags.aviMode & 1) /*&& (framerate <= 0)*/)
					{
					//	// try to correct for if the game sends us more audio than video data
						nextAviSoundSecondsCount = aviSoundSecondsCount + inAudioSeconds;
						//double aviVideoSecondsCount = (aviFrameCount-aviEmptyFrameCount+1) / (double)curAviFps;
						//double subtract = nextAviSoundSecondsCount - (aviVideoSecondsCount + 0.1);
						//if(subtract > 0 && aviVideoSecondsCount > 0.5 - 0.1)
						//{
						//	subtract += 0.1;
						//	double factor = (nextAviSoundSecondsCount - subtract) / nextAviSoundSecondsCount;
						//	int blockAlign = inAudioSize / audioFrameSamples;
						//	int prevAudioFrameSamples = audioFrameSamples;
						//	audioFrameSamples = (int)(audioFrameSamples * factor);
						//	if(audioFrameSamples < 1 && prevAudioFrameSamples >= 1)
						//		audioFrameSamples = 1;
						//	inAudioSize = audioFrameSamples * blockAlign;
						//	nextAviSoundSecondsCount -= subtract;
						//}
					}
 
					AudioConverterStream::ConvertOutput output = audioConverterStream->Convert((const BYTE*)audioBuffer, inAudioSize);
					if(audioConverterStream->failed)
						return;

					int audioFrameSize = output.size;
					LONG bytesWritten = audioFrameSize;

					{
						AutoCritSect cs(&s_aviCS);

						if(!aviSoundStream)
							return;

						HRESULT hr = SafeAVIStreamWrite(aviSoundStream, aviSoundSampleCount, audioFrameSamples, output.buffer, audioFrameSize, 0, NULL, &bytesWritten);
						if(FAILED(hr))
						{ 
							debugprintf("AVIStreamWrite(audio) failed! (0x%X)\n", hr);
							NormalMessageBox("AVIStreamWrite(audio) failed!\n", "Error", MB_OK|MB_ICONERROR);
							CloseAVI();
							//CheckDlgButton(hWnd, IDC_AVIVIDEO, 0);
							//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
							Config::localTASflags.aviMode = 0;
							tasFlagsDirty = true;
						}
					}

					aviSoundSampleCount += audioFrameSamples;
					if((Config::localTASflags.aviMode & 1) /*&& (framerate <= 0)*/)
					{
						aviSoundSecondsCount = nextAviSoundSecondsCount;
						//int prevAviSoundFrameCount = aviSoundFrameCount;
						aviSoundFrameCount = (int)(aviSoundSecondsCount * curAviFps);
						//if(aviSoundFrameCount <= prevAviSoundFrameCount)
						//	aviSoundFrameCount = prevAviSoundFrameCount + 1;
					}
					else
					{
						aviSoundFrameCount++;
					}
					aviFilesize += bytesWritten;

					if(aviSoundFrameCount == 1)
						mainMenuNeedsRebuilding = true;

					if((aviFilesize>>10) > ((unsigned int)1953 << 10)) // roughly 2 GB
						requestedAviSplitCount = aviSplitCount+1;

					avidebugprintf("Output:    slot %d, movie frame %d, audio frame %d\n", slotNum, framecount, aviSoundFrameCount);
				}
			}

			friend AviFrameQueue<numSlots>;
	};

	HANDLE thread;
	HANDLE threadAudio;
	bool threadDone;
	static DWORD WINAPI OutputVideoThreadFunc(LPVOID lpParam)
	{
		((AviFrameQueue<numSlots>*)lpParam)->OutputVideo();
		return 0;
	}
	static DWORD WINAPI OutputAudioThreadFunc(LPVOID lpParam)
	{
		((AviFrameQueue<numSlots>*)lpParam)->OutputAudio();
		return 0;
	}

	void OutputVideo()
	{
		while(!threadDone)
		{
			Slot& slot = m_slots[m_nextRead];

			slot.hasProcessedVideo.WaitUntilTrue();
			if(!threadDone)
				slot.OutputVideoFrame();
			slot.hasProcessedVideo = false;

			//// leave last video slot occupied until next new frame so RefillFrame can reuse its data
			//m_slots[(numSlots+m_nextRead-1)%numSlots].hasProcessedVideo = false;

			m_nextRead++;
			m_nextRead %= numSlots;
		}
	}
	void OutputAudio()
	{
		while(!threadDone)
		{
			Slot& slot = m_slots[m_nextReadAudio];

			slot.hasProcessedAudio.WaitUntilTrue();
			if(!threadDone)
				slot.OutputAudioFrame();
			slot.hasProcessedAudio = false;

			m_nextReadAudio++;
			m_nextReadAudio %= numSlots;
		}
	}

private:
	Slot m_slots [numSlots];
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
//static PAVISTREAM aviCompressedSoundStream = NULL;
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
	if(aviStream)
		oldIsBasicallyEmpty |= (aviFrameCount-aviEmptyFrameCount < 5 && aviSoundFrameCount < 15);

	if(aviFrameQueue)
		aviFrameQueue->Flush();
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

	// This is probably not the right thing to do here?
	//lastFrameSoundInfo = 0;
	//g_gammaRampEnabled = false;
	//paletteEntriesPointer = 0;
	//captureProcess = NULL;

	AutoCritSect cs(&s_aviCS);

	if(aviStream)
		AVIStreamClose(aviStream);
	aviStream = NULL;
	
	if(aviCompressedStream)
		AVIStreamClose(aviCompressedStream);
	aviCompressedStream = NULL;

	if(aviSoundStream)
		AVIStreamClose(aviSoundStream);
	aviSoundStream = NULL;

	if(aviFile)
		AVIFileClose(aviFile);
	aviFile = NULL;

	delete audioConverterStream;
	audioConverterStream = NULL;

	if(aviLibraryOpened)
		AVIFileExit();
	aviLibraryOpened = false;

	aviFrameQueue = new AviFrameQueue<4>();

	tasFlagsDirty = true;
	mainMenuNeedsRebuilding = true;
}

bool SetAVIFilename(char* filename)
{
	if(strlen(filename) > MAX_PATH+1) // filename too long
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

/*	if(aviSplitCount == 0)
	{
		*avifilename = '\0';
		char title [256];
		if(width || height || fps)
			sprintf(title, "Save AVI File (%d x %d) (%d FPS)", width, height, fps);
		else
			strcpy(title, "Save AVI File (audio only)");
		OPENFILENAME ofn = {sizeof(OPENFILENAME)};
		ofn.hwndOwner = hWnd;
		ofn.hInstance = hInst;
		ofn.lpstrFilter = "AVI file\0*.avi\0All Files\0*.*\0\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = avifilename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = saveDir;
		ofn.lpstrTitle = title;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "avi";
		if(!GetSaveFileName(&ofn))
			return false;
	}
	else*/
	if(aviSplitCount > 0)
	{
		if(oldIsBasicallyEmpty)
			aviSplitDiscardCount++; // discard empty or almost-empty AVI segments

		static char avifilename2 [MAX_PATH+1];
		strcpy(avifilename2, avifilename);
		char* dot = strrchr(avifilename2, '.');
		if(dot)
			*dot = 0;
		else
			dot = avifilename2 + strlen(avifilename2);
		int segNum = aviSplitCount+1-aviSplitDiscardCount;
		if(segNum > 1)
		{
			_snprintf(dot, MAX_PATH+1 - strlen(avifilename2), "%03d%s", segNum, dot - avifilename2 + avifilename);
			filename = avifilename2;
		}
	}

	oldIsBasicallyEmpty = false;

	debugprintf(__FUNCTION__ "(filename=\"%s\", width=%d, height=%d, bpp=%d, fps=%d)\n", filename, width, height, bpp, fps);

	AutoCritSect cs(&s_aviCS);

	if(!aviLibraryOpened)
	{
		AVIFileInit();
		aviLibraryOpened = true;
	}

	_unlink(filename);
	HRESULT hr = AVIFileOpen(&aviFile, filename, OF_WRITE|OF_CREATE, NULL);
	if(FAILED(hr))
	{
		char str [MAX_PATH + 64];
		sprintf(str, "AVIFileOpen(\"%s\") failed!\n", filename);
		debugprintf(str);
		NormalMessageBox(str, "Error", MB_OK|MB_ICONERROR);
		return false;
	}

	if(width && height && fps)
	{
		BITMAPINFO bmpInfo = 
		{
			// BITMAPINFOHEADER
			{ 
				sizeof(BITMAPINFO), 
				width, height, static_cast<WORD>(1),
				static_cast<WORD>(bpp), BI_RGB, 
				static_cast<DWORD>(static_cast<unsigned>(width) * height * bpp / 8), 
			}
		};  

		AVISTREAMINFO streamInfo = { streamtypeVIDEO };
		streamInfo.dwRate = fps;
		streamInfo.dwScale = 1;
		streamInfo.dwQuality = -1;
		streamInfo.rcFrame.right = width;
		streamInfo.rcFrame.bottom = height;

		hr = AVIFileCreateStream(aviFile, &aviStream, &streamInfo);
		if(FAILED(hr))
		{ 
			debugprintf("AVIFileCreateStream failed!\n");
			NormalMessageBox("AVIFileCreateStream failed!\n", "Error", MB_OK|MB_ICONERROR);
			CloseAVI();
			return false;
		}

		int chooseIter = 0;
chooseAnotherFormat:
		chooseIter++;
		static AVICOMPRESSOPTIONS options = {0};
		AVICOMPRESSOPTIONS* pOptions = &options;
		if(!aviSplitCount || chooseIter > 1)
		{
			if(!AVISaveOptions(/*hWnd*/NULL, ICMF_CHOOSE_KEYFRAME|ICMF_CHOOSE_DATARATE, 1, &aviStream, &pOptions))
			{
				CloseAVI();
				return false;
			}
		}

		hr = AVIMakeCompressedStream(&aviCompressedStream, aviStream, pOptions, NULL);
		if(FAILED(hr))
		{ 
			debugprintf("AVIMakeCompressedStream failed! (0x%X)\n", hr);
			NormalMessageBox("AVIMakeCompressedStream failed!\n", "Error", MB_OK|MB_ICONERROR);
			//CloseAVI();
			//return false;
			goto chooseAnotherFormat;
		}

		if(bmpInfo.bmiHeader.biWidth & 1) bmpInfo.bmiHeader.biWidth--;
		if(bmpInfo.bmiHeader.biHeight & 1) bmpInfo.bmiHeader.biHeight--;
		hr = AVIStreamSetFormat(aviCompressedStream, 0, &bmpInfo, sizeof(bmpInfo));
		if(FAILED(hr))
		{ 
			debugprintf("AVIStreamSetFormat failed! (0x%X)\n", hr);
			NormalMessageBox("AVIStreamSetFormat failed!\n", "Error", MB_OK|MB_ICONERROR);
			//CloseAVI();
			//return false;
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
	if(!(Config::localTASflags.aviMode & 1))
		return; // double-check...

	int fps = Config::localTASflags.framerate;
	if(fps <= 0)
		fps = 60;

	if(!aviFile || !aviCompressedStream || curAviWidth != width || curAviHeight != height || curAviFps != fps)
	{
		if(aviFile)
			aviSplitCount++;
		if(!OpenAVIFile(width, height, 24, fps))
		{
			//CheckDlgButton(hWnd, IDC_AVIVIDEO, 0);
			//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
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
	if((Config::localTASflags.aviMode & 1) && aviCompressedStream)
	{
		aviFrameQueue->RefillFrame();
	}
}

WaveFormat chooseFormat;

bool ChooseAudioCodec(const LPWAVEFORMATEX defaultFormat)
{
	if(Config::localTASflags.framerate <= 0)
	{
		int result = CustomMessageBox("This is a no-framerate movie,\nso audio capture is unlikely to work well.", "Warning", MB_OKCANCEL | MB_ICONWARNING);
		if(result == IDCANCEL)
			return false;
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
	if(!lastFrameSoundInfo)
		return 0;

	{
		AutoCritSect cs(&s_aviCS);

		LastFrameSoundInfo soundInfo;
		if(!ReadProcessMemory(captureProcess, lastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
			return -1;

		if(!soundInfo.format)
			return 0;

		WAVEFORMATEX format;
		if(!ReadProcessMemory(captureProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
			return -1;

		// open the output file if it's not already open
		if(!aviFile)
		{
			if(!(Config::localTASflags.aviMode & 1))
			{
				curAviWidth = 0;
				curAviHeight = 0;
				curAviFps = 0;
			}
			else
			{
				if(!curAviWidth && !curAviHeight && !curAviFps)
					return 0; // if video recording is on, wait until first frame of video
			}

			if(!OpenAVIFile(curAviWidth, curAviHeight, 24, curAviFps))
			{
				//CheckDlgButton(hWnd, IDC_AVIVIDEO, 0);
				//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
				Config::localTASflags.aviMode = 0;
				tasFlagsDirty = true;
				return -1;
			}
		}

		LPWAVEFORMATEX outputFormat = &format;
		int outputFormatSize = sizeof(WAVEFORMATEX) + outputFormat->cbSize;

		for(int chooseIter = 1;; chooseIter++)
		{
			if((aviSplitCount && chooseIter <= 1) || ChooseAudioCodec(&format))
			{
				// user chose a format
				outputFormat = chooseFormat;
				outputFormatSize = sizeof(WAVEFORMATEX) + outputFormat->cbSize;
			}
			else
			{
				// user cancelled
				//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
				Config::localTASflags.aviMode &= ~2;
				tasFlagsDirty = true;
				return -1;
			}

			// figure out how to convert to the chosen format
			audioConverterStream = new AudioConverterStream(format, chooseFormat);
			if(audioConverterStream->failed)
			{
				// try again
				debugprintf("AudioConverterStream() failed!\n");
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
		//streamInfo.dwInitialFrames = aviCompressedStream ? aviFrameCount : 0;
		streamInfo.dwInitialFrames = 0;

		HRESULT hr = AVIFileCreateStream(aviFile, &aviSoundStream, &streamInfo);
		if(FAILED(hr))
		{ 
			debugprintf("AVIFileCreateStream(audio) failed!\n");
			NormalMessageBox("AVIFileCreateStream(audio) failed!\nCapture will continue without audio\n", "Error", MB_OK|MB_ICONERROR);
			//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
			Config::localTASflags.aviMode &= ~2;
			tasFlagsDirty = true;
			return -1;
		}

		hr = AVIStreamSetFormat(aviSoundStream, 0, outputFormat, outputFormatSize);
		if(FAILED(hr))
		{ 
			debugprintf("AVIStreamSetFormat(audio) failed!\n");
			NormalMessageBox("AVIStreamSetFormat(audio) failed!\nCapture will continue without audio\n", "Error", MB_OK|MB_ICONERROR);
			//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
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
	//	AutoCritSect cs(&s_aviCS);

	if(!aviSoundStream)
	{
		int res = OpenAVIAudioStream();
		if(!res)
			return;
		if(res < 0)
		{
			//CheckDlgButton(hWnd, IDC_AVIAUDIO, 0);
			Config::localTASflags.aviMode &= ~2;
			tasFlagsDirty = true;
			return;
		}
	}

	if(aviCompressedStream && (aviSoundFrameCount < 30 || aviSoundFrameCount+8 < aviFrameCount) && !aviSplitCount)
	{
		if(aviSoundFrameCount < aviFrameCount/*-aviEmptyFrameCount*/)
		{
			AutoCritSect cs(&s_fqaCS); // critical section and check again in case we got here while CloseAVI is running
			if(aviCompressedStream && (aviSoundFrameCount < 30 || aviSoundFrameCount+8 < aviFrameCount) && !aviSplitCount)
				while(aviSoundFrameCount < aviFrameCount/*-aviEmptyFrameCount*/)
					aviFrameQueue->FillEmptyAudioFrame(); // in case video started before audio
		}
	}

	aviFrameQueue->FillAudioFrame();

	//LastFrameSoundInfo soundInfo;
	//if(!ReadProcessMemory(hGameProcess, remoteLastFrameSoundInfo, &soundInfo, sizeof(LastFrameSoundInfo), NULL))
	//	return;

	//if(!soundInfo.format)
	//	return;

	//WAVEFORMATEX format;
	//if(!ReadProcessMemory(hGameProcess, soundInfo.format, &format, sizeof(WAVEFORMATEX), NULL))
	//	return;

	//int numSamples = soundInfo.size / format.nBlockAlign;

	//static void* buffer = NULL;
	//static int bufferAllocated = 0;
	//if(bufferAllocated < (int)soundInfo.size)
	//{
	//	bufferAllocated = soundInfo.size;
	//	buffer = realloc(buffer, bufferAllocated);
	//}
	//if(!ReadProcessMemory(hGameProcess, soundInfo.buffer, buffer, soundInfo.size, NULL))
	//	return;

	//if(!audioConverterStream)
	//	return;
	//AudioConverterStream::ConvertOutput output = audioConverterStream->Convert((const BYTE*)buffer, soundInfo.size);
	//if(audioConverterStream->failed)
	//	return;

	//HRESULT hr = AVIStreamWrite(aviSoundStream, aviSoundSampleCount, numSamples, output.buffer, output.size, 0, NULL, NULL);
	//if(FAILED(hr))
	//	return;

	//aviSoundSampleCount += numSamples;
	//aviSoundFrameCount++;
	//aviFilesize += output.size;
}

void SplitAVINow()
{
	requestedAviSplitCount = aviSplitCount + 1;
}

void HandleAviSplitRequests()
{
	if(requestedAviSplitCount > aviSplitCount)
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

void ProcessCaptureFrameInfo(void* frameCaptureInfoRemoteAddr, int frameCaptureInfoType)
{
	//if(frameCaptureInfoType != CAPTUREINFO_TYPE_NONE && frameCaptureInfoType != CAPTUREINFO_TYPE_NONE_SUBSEQUENT)
	//	if(aviSoundStream && (aviMode & 1) && aviCompressedStream && aviFrameCount < 30)
	//		while(aviFrameCount+2 < aviSoundFrameCount)
	//			aviFrameQueue->RefillFrame(); // in case audio started before video

	switch(frameCaptureInfoType)
	{
	case CAPTUREINFO_TYPE_NONE:
	case CAPTUREINFO_TYPE_NONE_SUBSEQUENT:
		break;
	case CAPTUREINFO_TYPE_PREV:
		RewriteAVIFrame();
		break;
	case CAPTUREINFO_TYPE_DDSD:
		if((Config::localTASflags.aviMode & 1) && frameCaptureInfoRemoteAddr)
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