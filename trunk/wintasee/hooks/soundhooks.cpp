/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(SOUNDHOOKS_INCL) && !defined(UNITY_BUILD)
#define SOUNDHOOKS_INCL

#include "../../external/dsound.h"
#include "../../external/dmusici.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <algorithm>
#include <vector>
#include <map>

// TODO: add MIDI support (for music in Eternal Daughter)
// probably involves hooking midiOutShortMsg and such
// first thing to try:
// play the MIDI with DirectMusic so it will mix into DirectSound for us for "free"
// IDirectMusic8::CreateMusicBuffer
// IDirectMusicBuffer8::PackUnstructured
// IDirectMusicPort::SetDirectSound
// IDirectMusicPort::PlayBuffer
// if for some reason that can't work:
// look at what timidity does. this is probably much harder...
// for instrument data, see LoadDLS()
// %SystemRoot%\SYSTEM32\drivers\GM.DLS // system default midi soundfont file
// %SystemRoot%\SYSTEM32\drivers\etc\GM.DLS
// or maybe 2GMGSMT.SF2 or better

DEFINE_LOCAL_GUID(IID_IDirectSoundBuffer, 0x279AFA85,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_LOCAL_GUID(IID_IDirectSoundBuffer8,0x6825A449,0x7524,0x4D82,0x92,0x0F,0x50,0xE3,0x6A,0xB3,0xAB,0x1E);
DEFINE_LOCAL_GUID(IID_IDirectSound3DBuffer,0x279AFA86,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_LOCAL_GUID(IID_IDirectSoundNotify,0xB0210783,0x89CD,0x11D0,0xAF,0x8,0x0,0xA0,0xC9,0x25,0xCD,0x16);


static CRITICAL_SECTION s_myMixingOutputBufferCS;
static IDirectSoundBuffer* s_myMixingOutputBuffer = NULL;
static IDirectSoundBuffer* s_primaryBuffer = NULL;
static bool s_primaryBufferFormatSet = false;
//static LONG s_mixSuspendVol = 0;

static unsigned char* contiguousMixOutBuf = NULL;
static DWORD contiguousMixOutBufSize = 0;
static DWORD contiguousMixOutBufAllocated = 0;
static DWORD contiguousMixOutBufOffset = 0;
static LPWAVEFORMATEX contiguousMixOutBufFormat = NULL;

LastFrameSoundInfo lastFrameSoundInfo;

static BOOL lockdown = FALSE;
static CRITICAL_SECTION s_soundBufferListCS;

static BOOL noDirectSoundOutputAvailable = FALSE;
static bool usingVirtualDirectSound = false;

void MixFromToInternal(DWORD pos1, DWORD pos2, DWORD outPos1, DWORD outPos2, bool pos2IsLastSample,
	DWORD outSamplesPerSec, WORD myBitsPerSample, WORD outBitsPerSample, WORD myChannels, WORD outChannels, WORD myBlockSize, WORD outBlockSize,
	unsigned char* buffer, unsigned char* contiguousMixOutBuf, CachedVolumeAndPan& volumes);


#include <map>
#include <math.h>


// new method: completely emulate DirectSound secondary buffers (do all the mixing manually).
// this should fix a whole bunch of problems with asynchronous sound output,
// and make it possible to capture it to AVI/WAV output.

class EmulatedDirectSoundBuffer : public IDirectSoundBuffer8, public IDirectSoundNotify, public IDirectSound3DBuffer
{
public:
	typedef EmulatedDirectSoundBuffer MyType;
	//typedef std::map<MyType*, DWORD> MyBufferMap;
	typedef std::vector<MyType*> MyBufferList;

	EmulatedDirectSoundBuffer(bool isFakePrimary=false) : m_isFakePrimary(isFakePrimary)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		EnterCriticalSection(&s_soundBufferListCS);
		//if(soundBuffers.find(this) == soundBuffers.end())
		//	soundBuffers.insert(std::make_pair(this, GetCurrentThreadId()));
		soundBuffers.push_back(this);
		LeaveCriticalSection(&s_soundBufferListCS);
		debuglog(LCF_DSOUND, "%d emulated sound buffers\n", soundBuffers.size());

		buffer = NULL;
		bufferSize = 0;
		allocated = 0;
		lockBuf = NULL;
		lockPtr1 = NULL;
		lockPtr2 = NULL;
		lockBytes1 = 0;
		lockBytes2 = 0;
		numRemainingUnlocksBeforeLeavingLockBufResident = 8;
		frequency = 22050;
		volume = 0;
		pan = 0;
		CalcVolumeScales();
		status = 0;
		playCursor = 0;
		writeCursor = 0;
		capsFlags = DSBCAPS_LOCSOFTWARE;
		waveformat = NULL;
		notifies = NULL;
		numNotifies = 0;
		InitializeCriticalSection(&m_bufferCS);
		InitializeCriticalSection(&m_lockBufferCS);

		refcount = 1;
	}
	~EmulatedDirectSoundBuffer()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		EnterCriticalSection(&s_soundBufferListCS);
		//soundBuffers.erase(this);
		MyBufferList::iterator found = std::find(soundBuffers.begin(), soundBuffers.end(), this);
		if(found != soundBuffers.end())
			soundBuffers.erase(found);
		LeaveCriticalSection(&s_soundBufferListCS);
		debuglog(LCF_DSOUND, "%d sound buffers\n", soundBuffers.size());
		free(buffer);
		free(lockBuf);
		free(waveformat);
		free(notifies);
		DeleteCriticalSection(&m_bufferCS);
		DeleteCriticalSection(&m_lockBufferCS);
	}

	static EmulatedDirectSoundBuffer* Duplicate(const EmulatedDirectSoundBuffer* original)
	{
		EmulatedDirectSoundBuffer* copy = new EmulatedDirectSoundBuffer();
		copy->bufferSize = original->bufferSize;
		copy->allocated = original->allocated;
		copy->buffer = (unsigned char*)malloc(copy->allocated);
		memcpy(copy->buffer, original->buffer, original->allocated);
		copy->frequency = original->frequency;
		copy->pan = original->pan;
		copy->volume = original->volume;
		copy->volumes.leftVolumeAsScale = original->volumes.leftVolumeAsScale;
		copy->volumes.rightVolumeAsScale = original->volumes.rightVolumeAsScale;
		copy->status = original->status & (DSBSTATUS_LOCHARDWARE | DSBSTATUS_LOCSOFTWARE);
		copy->capsFlags = original->capsFlags;

		int wfxsize = sizeof(WAVEFORMATEX);
		if(original->waveformat->wFormatTag != WAVE_FORMAT_PCM)
			wfxsize += original->waveformat->cbSize;
		copy->waveformat = (WAVEFORMATEX*)malloc(wfxsize);
		memcpy(copy->waveformat, original->waveformat, wfxsize);

		return copy;
	}

	// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
		if(!ppvObj)
			return E_POINTER;
		if(riid == IID_IDirectSoundBuffer)
			*ppvObj = (IDirectSoundBuffer*)this;
		else if(riid == IID_IDirectSoundBuffer8 && !m_isFakePrimary) // "IDirectSoundBuffer8 is not available for the primary buffer"
			*ppvObj = (IDirectSoundBuffer8*)this;
		else if(riid == IID_IDirectSound3DBuffer)
			*ppvObj = (IDirectSound3DBuffer*)this;
		else if(riid == IID_IDirectSoundNotify && s_myMixingOutputBuffer != (IDirectSoundBuffer*)this)
			*ppvObj = (IDirectSoundNotify*)this; // lesson in multiple inheritance: these casts are required, since they select which vtable to use
		else if(riid == IID_IUnknown)
			*ppvObj = (IUnknown*)(IDirectSoundBuffer*)this;
		else
		{
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		return ++refcount;
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		ULONG count = --refcount;
		if(0 == count)
			delete this;
		return count;
	}

    // IDirectSoundBuffer methods
    STDMETHOD(GetCaps)              (LPDSBCAPS pDSBufferCaps)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!pDSBufferCaps)
			return DSERR_INVALIDPARAM;

		pDSBufferCaps->dwFlags = capsFlags;
		pDSBufferCaps->dwBufferBytes = bufferSize;
		pDSBufferCaps->dwUnlockTransferRate = 1024*1024; // not sure... should be "very high" whatever that means
		pDSBufferCaps->dwPlayCpuOverhead = 0; // pretend there's no processing overhead (that's standard hardware buffer behavior)
		return DS_OK;
	}
    STDMETHOD(GetCurrentPosition)   (LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called (playCursor=%d, writeCursor=%d).\n", playCursor,writeCursor);
		if(pdwCurrentPlayCursor)
		{
			*pdwCurrentPlayCursor = playCursor;
			//*pdwCurrentPlayCursor = (playCursor + max(0, bufferSize-1024)) % bufferSize;
		}
		if(pdwCurrentWriteCursor)
		{
			*pdwCurrentWriteCursor = writeCursor;
			//*pdwCurrentWriteCursor = (writeCursor + 4096) % bufferSize;
		}
		return DS_OK;
	}
    STDMETHOD(GetFormat)            (LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!pwfxFormat)
		{
			debuglog(LCF_DSOUND, __FUNCTION__ " returned DSERR_INVALIDCALL.\n");
			return DSERR_INVALIDCALL;
		}
		int size = sizeof(WAVEFORMATEX) + (waveformat ? waveformat->cbSize : 0);
		if(waveformat)
			memcpy(pwfxFormat, waveformat, size);
		else
		{
			// might happen with fake primary buffer? not sure if this is the right thing to do,
			// but if we return a failure code, directmusic will notice and cause desyncs by not calling Sleep(10) from dmsynth (at least until a more general mechanism for distrusting dlls is added)
			pwfxFormat->nChannels = tasflags.audioChannels;
			pwfxFormat->nSamplesPerSec = tasflags.audioFrequency;
			pwfxFormat->wBitsPerSample = tasflags.audioBitsPerSecond;
			pwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
			pwfxFormat->nBlockAlign = pwfxFormat->wBitsPerSample * pwfxFormat->nChannels / 8;
			pwfxFormat->nAvgBytesPerSec = pwfxFormat->nSamplesPerSec * pwfxFormat->nBlockAlign;
			pwfxFormat->cbSize = 0;
		}
		if(pdwSizeWritten)
			*pdwSizeWritten = size;
		if(pwfxFormat)
			debuglog(LCF_DSOUND, __FUNCTION__ " returned format: %d %d %d\n", pwfxFormat->nSamplesPerSec, pwfxFormat->wBitsPerSample, pwfxFormat->nChannels);
		return DS_OK;
	}
    STDMETHOD(GetVolume)            (LPLONG plVolume)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(plVolume)
			*plVolume = volume;
		return DS_OK;
	}
    STDMETHOD(GetPan)               (LPLONG plPan)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(plPan)
			*plPan = pan;
		return DS_OK;
	}
    STDMETHOD(GetFrequency)         (LPDWORD pdwFrequency)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(pdwFrequency)
			*pdwFrequency = frequency;
		return DS_OK;
	}
    STDMETHOD(GetStatus)            (LPDWORD pdwStatus)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(pdwStatus)
			*pdwStatus = status;
		return DS_OK;
	}
    STDMETHOD(Initialize)           (LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");

		if(waveformat)
		{
			debuglog(LCF_DSOUND, __FUNCTION__ " returned DSERR_ALREADYINITIALIZED.\n");
			return DSERR_ALREADYINITIALIZED;
		}

		if(!pcDSBufferDesc)
		{
			debuglog(LCF_DSOUND|LCF_ERROR, __FUNCTION__ " returned DSERR_INVALIDPARAM.\n");
			return DSERR_INVALIDPARAM;
		}

		if(!pcDSBufferDesc->lpwfxFormat)
		{
			if(!m_isFakePrimary)
			{
				debuglog(LCF_DSOUND|LCF_ERROR, __FUNCTION__ " returned DSERR_INVALIDPARAM.\n");
				return DSERR_INVALIDPARAM;
			}
			WAVEFORMATEX wfmt;
			wfmt.nChannels = tasflags.audioChannels;
			wfmt.nSamplesPerSec = tasflags.audioFrequency;
			wfmt.wBitsPerSample = tasflags.audioBitsPerSecond;
			wfmt.wFormatTag = WAVE_FORMAT_PCM;
			wfmt.nBlockAlign = wfmt.wBitsPerSample * wfmt.nChannels / 8;
			wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;
			wfmt.cbSize = 0;
			SetWavFormat(&wfmt);
		}
		else
		{
			SetWavFormat(pcDSBufferDesc->lpwfxFormat);
		}

		bufferSize = pcDSBufferDesc->dwBufferBytes;
		EnterCriticalSection(&m_bufferCS);
		bufferSize = min(max(bufferSize, DSBSIZE_MIN), DSBSIZE_MAX);
		allocated = bufferSize;
		buffer = (unsigned char*)realloc(buffer, allocated);
		memset(buffer, (waveformat->wBitsPerSample <= 8) ? 0x80 : 0, allocated);
		LeaveCriticalSection(&m_bufferCS);

		capsFlags = pcDSBufferDesc->dwFlags;
		if(!(capsFlags & DSBCAPS_LOCHARDWARE))
			capsFlags |= DSBCAPS_LOCSOFTWARE;

		frequency = waveformat->nSamplesPerSec;

		return DS_OK;
	}

	void SetWavFormat(LPCWAVEFORMATEX pFormat)
	{
		int wfxsize = sizeof(WAVEFORMATEX);
		if(pFormat->wFormatTag != WAVE_FORMAT_PCM)
			wfxsize += pFormat->cbSize;
		waveformat = (WAVEFORMATEX*)realloc(waveformat, wfxsize);
		memcpy(waveformat, pFormat, wfxsize);
		if(pFormat->wFormatTag == WAVE_FORMAT_PCM)
			waveformat->cbSize = 0;
	}

    STDMETHOD(Lock)                 (DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X)(dwOffset=%d, dwBytes=%d, dwFlags=0x%X) called.\n", this,dwOffset,dwBytes,dwFlags);
		if(!buffer || !bufferSize)
			return DSERR_INVALIDCALL;
		EnterCriticalSection(&m_bufferCS);
		EnterCriticalSection(&m_lockBufferCS);

		if(!lockBuf)
			lockBuf = (unsigned char*)realloc(lockBuf, bufferSize);
		memcpy(lockBuf, buffer, bufferSize);

		if(dwFlags & DSBLOCK_FROMWRITECURSOR)
			dwOffset = writeCursor;
		else
			dwOffset %= bufferSize;

		if(dwFlags & DSBLOCK_ENTIREBUFFER)
			dwBytes = bufferSize;
		else
			dwBytes = min(bufferSize, dwBytes);

		lockPtr1 = NULL;
		lockPtr2 = NULL;
		lockBytes1 = 0;
		lockBytes2 = 0;

		if(ppvAudioPtr1)
			*ppvAudioPtr1 = lockPtr1 = lockBuf + dwOffset;

		if(dwOffset + dwBytes <= bufferSize)
		{
			debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) got (%d - %d).\n", this,dwOffset,dwOffset+dwBytes);
			if(pdwAudioBytes1)
				*pdwAudioBytes1 = lockBytes1 = dwBytes;
			if(ppvAudioPtr2)
				*ppvAudioPtr2 = NULL;
			if(pdwAudioBytes2)
				*pdwAudioBytes2 = 0;
		}
		else
		{
			debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) split (%d - %d) (%d - %d).\n", this,dwOffset,bufferSize,0,dwBytes+dwOffset-bufferSize);
			if(pdwAudioBytes1)
				*pdwAudioBytes1 = lockBytes1 = bufferSize - dwOffset;
			if(ppvAudioPtr2)
				*ppvAudioPtr2 = lockPtr2 = lockBuf;
			if(pdwAudioBytes2)
				*pdwAudioBytes2 = lockBytes2 = dwBytes + dwOffset - bufferSize;
		}
		LeaveCriticalSection(&m_bufferCS);

		return DS_OK;
	}

    STDMETHOD(Play)                 (DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(%p, %d) called.\n", this, dwFlags);
		status |= DSBSTATUS_PLAYING;
		if(dwFlags & DSBPLAY_LOOPING)
			status |= DSBSTATUS_LOOPING;
		else
			status &= ~DSBSTATUS_LOOPING;
		return DS_OK;
	}
    STDMETHOD(SetCurrentPosition)   (DWORD dwNewPosition)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(%p, %d) called.\n", this, dwNewPosition);
		playCursor = dwNewPosition % max(1,bufferSize);
		return DS_OK;
	}
    STDMETHOD(SetFormat)            (LPCWAVEFORMATEX pcfxFormat)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(m_isFakePrimary || !waveformat)
		{
			SetWavFormat(pcfxFormat);
			return DS_OK;
		}
		else
		{
			// this method is only for primary buffers,
			// but this class is only for secondary buffers.
			return DSERR_UNSUPPORTED;
		}
	}
    STDMETHOD(SetVolume)            (LONG lVolume)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!(capsFlags & DSBCAPS_CTRLVOLUME))
			return DSERR_CONTROLUNAVAIL;
		volume = min(max(lVolume, DSBVOLUME_MIN), DSBVOLUME_MAX);
		//verbosedebugprintf(__FUNCTION__": setting volume on 0x%X to %d\n", this, (int)volume);
		CalcVolumeScales();
		return DS_OK;
	}
    STDMETHOD(SetPan)               (LONG lPan)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!(capsFlags & DSBCAPS_CTRLPAN))
			return DSERR_CONTROLUNAVAIL;
		pan = min(max(lPan, DSBPAN_LEFT), DSBPAN_RIGHT);
		CalcVolumeScales();
		return DS_OK;
	}
    STDMETHOD(SetFrequency)         (DWORD dwFrequency)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!(capsFlags & DSBCAPS_CTRLFREQUENCY))
			return DSERR_CONTROLUNAVAIL;
		if(dwFrequency == DSBFREQUENCY_ORIGINAL)
			frequency = waveformat ? waveformat->nSamplesPerSec : frequency;
		else
			frequency = dwFrequency;
		return DS_OK;
	}
    STDMETHOD(Stop)                 ()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(%p) called.\n", this);
		status &= ~(DSBSTATUS_PLAYING | DSBSTATUS_LOOPING);
		// don't clear the play/write positions, because the docs say: "when the Play method is next called on the buffer, it will continue playing where it left off"
		// (except for primary buffers, which this class isn't intended to be used for)
		if(numNotifies)
			NotifyOfStop();
		return DS_OK;
	}
	STDMETHOD(Unlock)               (LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(!lockBuf)
			return DSERR_INVALIDCALL;
		EnterCriticalSection(&m_bufferCS);
		if(pvAudioPtr1 && pvAudioPtr1 == lockPtr1)
		{
			if(dwAudioBytes1 > lockBytes1)
				dwAudioBytes1 = lockBytes1;
			int dwOffset = (char*)pvAudioPtr1 - (char*)lockBuf;
			memcpy(buffer + dwOffset, lockBuf + dwOffset, dwAudioBytes1);
		}
		if(pvAudioPtr2 && pvAudioPtr2 == lockPtr2)
		{
			if(dwAudioBytes2 > lockBytes2)
				dwAudioBytes2 = lockBytes2;
			int dwOffset = (char*)pvAudioPtr2 - (char*)lockBuf;
			memcpy(buffer + dwOffset, lockBuf + dwOffset, dwAudioBytes2);
		}
		if(numRemainingUnlocksBeforeLeavingLockBufResident)
		{
			numRemainingUnlocksBeforeLeavingLockBufResident--;
			free(lockBuf);
			lockBuf = NULL;
		}
		ReplicateBufferIntoExtraAllocated();
		LeaveCriticalSection(&m_bufferCS);
		LeaveCriticalSection(&m_lockBufferCS);
		return DS_OK;
	}
    STDMETHOD(Restore)              ()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return DS_OK;
	}

    // IDirectSoundBuffer8 methods
    STDMETHOD(SetFX)                (DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
	{
		/*dsound*/debugprintf(__FUNCTION__ " called.\n");
		if(!(capsFlags & DSBCAPS_CTRLFX))
			return DSERR_CONTROLUNAVAIL;
		// NYI! chorus (GUID_DSFX_STANDARD_CHORUS), echo, flanger, gargle, distortion, etc.
		for(DWORD i = 0; i < dwEffectsCount; i++)
			pdwResultCodes[i] = DSFXR_FAILED;
		return DSERR_INVALIDPARAM;
	}
    STDMETHOD(AcquireResources)     (DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes)
	{
		/*dsound*/debugprintf(__FUNCTION__ " called.\n");
		// NYI
		return DSERR_CONTROLUNAVAIL;
	}
    STDMETHOD(GetObjectInPath)      (REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject)
	{
		/*dsound*/debugprintf(__FUNCTION__ " called.\n");
		// NYI
		return DSERR_CONTROLUNAVAIL;
	}



    // IDirectSoundNotify methods
    STDMETHOD(SetNotificationPositions) (DWORD dwPositionNotifies, LPCDSBPOSITIONNOTIFY pcPositionNotifies)
	{
		dsounddebugprintf(__FUNCTION__ "(%d) called.\n", dwPositionNotifies);
		if(dwPositionNotifies > DSBNOTIFICATIONS_MAX || pcPositionNotifies == NULL)
			return DSERR_INVALIDPARAM;

		free(notifies);
		numNotifies = 0;
		notifies = (DSBPOSITIONNOTIFY*)malloc(dwPositionNotifies * sizeof(DSBPOSITIONNOTIFY));
		if(!notifies)
			return DSERR_OUTOFMEMORY;
		
		memcpy(notifies, pcPositionNotifies, dwPositionNotifies * sizeof(DSBPOSITIONNOTIFY));
		numNotifies = dwPositionNotifies;
		return S_OK;
	}




    // IDirectSound3DBuffer methods
	// for now, these are just stubbed out (so that some games don't crash)
	STDMETHOD(GetAllParameters      )(LPDS3DBUFFER pDs3dBuffer) {DS3DBUFFER b={sizeof(DS3DBUFFER)};*pDs3dBuffer=b; return 0;}
    STDMETHOD(GetConeAngles         )(LPDWORD pdwInsideConeAngle, LPDWORD pdwOutsideConeAngle) {*pdwInsideConeAngle=0;*pdwOutsideConeAngle=0; return 0;}
	STDMETHOD(GetConeOrientation    )(D3DVECTOR* pvOrientation) {D3DVECTOR v={};*pvOrientation=v; return 0;}
    STDMETHOD(GetConeOutsideVolume  )(LPLONG plConeOutsideVolume) {*plConeOutsideVolume=0; return 0;}
    STDMETHOD(GetMaxDistance        )(D3DVALUE* pflMaxDistance) {*pflMaxDistance=0; return 0;}
    STDMETHOD(GetMinDistance        )(D3DVALUE* pflMinDistance) {*pflMinDistance=0; return 0;}
    STDMETHOD(GetMode               )(LPDWORD pdwMode) {*pdwMode=0; return 0;}
    STDMETHOD(GetPosition           )(D3DVECTOR* pvPosition) {D3DVECTOR v={};*pvPosition=v; return 0;}
    STDMETHOD(GetVelocity           )(D3DVECTOR* pvVelocity) {D3DVECTOR v={};*pvVelocity=v; return 0;}
    STDMETHOD(SetAllParameters      )(LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply) {return 0;}
    STDMETHOD(SetConeAngles         )(DWORD dwInsideConeAngle, DWORD dwOutsideConeAngle, DWORD dwApply) {return 0;}
    STDMETHOD(SetConeOrientation    )(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {return 0;}
    STDMETHOD(SetConeOutsideVolume  )(LONG lConeOutsideVolume, DWORD dwApply) {return 0;}
    STDMETHOD(SetMaxDistance        )(D3DVALUE flMaxDistance, DWORD dwApply) {return 0;}
    STDMETHOD(SetMinDistance        )(D3DVALUE flMinDistance, DWORD dwApply) {return 0;}
    STDMETHOD(SetMode               )(DWORD dwMode, DWORD dwApply) {return 0;}
    STDMETHOD(SetPosition           )(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {return 0;}
    STDMETHOD(SetVelocity           )(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwApply) {return 0;}






	// custom methods for emulated sound buffer

/*
	// for testing...
	void SaveToWavFile(const char* filename)
	{
		if(buffer && waveformat && bufferSize)
		{
			//char filename [256];
			//sprintf(filename, "C:/temp/wavs/%p.wav", this);
			FILE* file = fopen(filename, "wb");
			if(file)
			{
				int temp; short stemp;

				fputs("RIFF", file);
				temp = bufferSize+36; fwrite(&temp, 4, 1, file);
				fputs("WAVE", file);
				fputs("fmt ", file);
				temp = 16; fwrite(&temp, 4, 1, file);
				stemp = 1; fwrite(&stemp, 2, 1, file);
				stemp = waveformat->nChannels; fwrite(&stemp, 2, 1, file);
				temp = waveformat->nSamplesPerSec; fwrite(&temp, 4, 1, file);
				temp = waveformat->nAvgBytesPerSec; fwrite(&temp, 4, 1, file);
				stemp = waveformat->nBlockAlign; fwrite(&stemp, 2, 1, file);
				stemp = waveformat->wBitsPerSample; fwrite(&stemp, 2, 1, file);
				fputs("data", file);
				temp = bufferSize; fwrite(&temp, 4, 1, file);
				fwrite(buffer, 1, bufferSize, file);

				fclose(file);
			}
		}
	}
*/

	static MyBufferList soundBuffers;

	static void AdvanceTimeAndMixAll(DWORD ticks)
	{
//		g_soundMixedTicks += ticks;
		//debugprintf(__FUNCTION__ "g_soundMixedTicks=%d, g_videoFramesPrepared=%d, ratio=%g\n", g_soundMixedTicks, g_videoFramesPrepared, (float)g_soundMixedTicks/g_videoFramesPrepared);

		bool doMix = true;
		if((tasflags.emuMode & EMUMODE_NOPLAYBUFFERS) || noDirectSoundOutputAvailable)
			if(!(tasflags.aviMode & 2))
				doMix = false; // if muted and not capturing audio, there's no need to mix it

		if(doMix)
		{
			debuglog(LCF_DSOUND, __FUNCTION__ "(%d) called (at %d).\n", ticks, detTimer.GetTicks());

			EnterCriticalSection(&s_myMixingOutputBufferCS);

			static WAVEFORMATEX format; // must be static so wintaser can read it reliably...
			if(!(s_myMixingOutputBuffer && SUCCEEDED(s_myMixingOutputBuffer->GetFormat(&format, sizeof(WAVEFORMATEX), NULL))))
			{
				// well, maybe there's no DirectSound buffer,
				// but we can still mix it for AVI output, so we'll pick a waveformat
				format.nChannels = tasflags.audioChannels;
				format.nSamplesPerSec = tasflags.audioFrequency;
				format.wBitsPerSample = tasflags.audioBitsPerSecond;
				format.wFormatTag = WAVE_FORMAT_PCM;
				format.nBlockAlign = format.wBitsPerSample * format.nChannels / 8;
				format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
				format.cbSize = 0;
			}

			contiguousMixOutBufFormat = &format;
			DWORD outBytes = TicksToBytes(ticks, format.nSamplesPerSec, format);
			contiguousMixOutBufSize = outBytes;
			if(contiguousMixOutBufAllocated < contiguousMixOutBufSize+16)
			{
				contiguousMixOutBufAllocated = contiguousMixOutBufSize+16;
				contiguousMixOutBuf = (unsigned char*)realloc(contiguousMixOutBuf, contiguousMixOutBufAllocated);
			}
			if(contiguousMixOutBufSize)
				memset(contiguousMixOutBuf, (format.wBitsPerSample <= 8) ? 0x80 : 0, contiguousMixOutBufSize);

			lastFrameSoundInfo.buffer = contiguousMixOutBuf;
			lastFrameSoundInfo.size = contiguousMixOutBufSize;
			lastFrameSoundInfo.format = contiguousMixOutBufFormat;
		}

		EnterCriticalSection(&s_soundBufferListCS);
		for(MyBufferList::iterator iter = soundBuffers.begin(); iter != soundBuffers.end(); iter++)
			(*iter)->AdvanceTimeAndMix(ticks, doMix);
		LeaveCriticalSection(&s_soundBufferListCS);

		if(doMix && s_myMixingOutputBuffer)
		{
			void* ptr1=NULL; DWORD size1=0;
			void* ptr2=NULL; DWORD size2=0;

			if(!tasflags.fastForward)
			{
				// check to see if we're in a bad position for audio streaming.
				// if we don't correct it then DirectSound will keep reading bits of old data,
				// or we'll keep writing new data into a region DirectSound will mostly ignore,
				// which causes crackling and general crappiness of audio.
				DWORD play, write;
				s_myMixingOutputBuffer->GetCurrentPosition(&play, &write);

				// expand the "bad" zone back from the play cursor
				// because exactly at the play cursor is sometimes not enough room for us.
				if(write > play && play > write - play) // (don't bother when the math isn't easy)
					play -= write - play;

				// if we're in the bad region then wait
				if(contiguousMixOutBufOffset > play && contiguousMixOutBufOffset < write)
				{
					// TODO: figure out why contiguousMixOutBufSize < dwBufferBytes... in the meantime, use the latter here
					DSBCAPS caps = { sizeof(DSBCAPS) };
					s_myMixingOutputBuffer->GetCaps(&caps);
					
					// calculate an "ideal" position which is as far away from the bad region as possible.
					// (the actually-ideal position to be in depends on whether we're generating sound faster or slower than the sound buffer is being consumed,
					// but we use this average position to avoid needing to determine that.)
					DWORD idealPosition = ((play + write + caps.dwBufferBytes) >> 1);
					if(idealPosition > caps.dwBufferBytes)
						idealPosition -= caps.dwBufferBytes;
					int distFromIdeal = contiguousMixOutBufOffset - idealPosition;
					if(distFromIdeal < 0)
						distFromIdeal += caps.dwBufferBytes;

					// wait until the ideal position should have moved to our offset
					DWORD waitTime = distFromIdeal * 1000 / contiguousMixOutBufFormat->nAvgBytesPerSec;
					verbosedebugprintf("WAITING FOR SOUND TO REALIGN (waitTime=%d, offset=%d, play=%d, write=%d, ideal=%d)\n", waitTime, contiguousMixOutBufOffset, play, write, idealPosition);
					bool notFullSpeed = tasflags.frameAdvanceHeld || (tasflags.timescale != tasflags.timescaleDivisor) || (tasflags.aviMode & 1);
					DWORD maxWait = notFullSpeed ? 10 : 100;
					if(waitTime > maxWait)
						waitTime = maxWait;
					Sleep(waitTime);
				}
			}

			HRESULT hr = s_myMixingOutputBuffer->Lock(contiguousMixOutBufOffset, contiguousMixOutBufSize, &ptr1, &size1, &ptr2, &size2, 0);
			if(hr == DSBSTATUS_BUFFERLOST)
			{
				s_myMixingOutputBuffer->Restore();
				hr = s_myMixingOutputBuffer->Lock(contiguousMixOutBufOffset, contiguousMixOutBufSize, &ptr1, &size1, &ptr2, &size2, 0);
			}
			if(SUCCEEDED(hr))
			{
				unsigned char* mixPtr = contiguousMixOutBuf;
				memcpy(ptr1, mixPtr, size1);
				if(ptr2)
				{
					mixPtr += size1;
					memcpy(ptr2, mixPtr, size2);
				}
				s_myMixingOutputBuffer->Unlock(ptr1, size1, ptr2, size2);

				if(tasflags.fastForward)
					ForceAlign(true);

				contiguousMixOutBufOffset += contiguousMixOutBufSize;
				DSBCAPS caps = { sizeof(DSBCAPS) };
				s_myMixingOutputBuffer->GetCaps(&caps);
				contiguousMixOutBufOffset %= caps.dwBufferBytes;
			}
		}

		if(doMix)
			LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

	static DWORD TicksToBytes(DWORD ticks, DWORD frequency, WAVEFORMATEX& format)
	{
//		debugprintf("ticks=%d, freq=%d, samppersec=%d, blocksize=%d, bytes=%d\n", ticks, frequency, format.nSamplesPerSec, format.nBlockAlign, ticks * frequency / 1000 * format.nBlockAlign);
		//return ticks * format.nSamplesPerSec / 1000 * format.nBlockAlign;
		return ticks * frequency / 1000 * format.nBlockAlign;
	}

	void AdvanceTimeAndMix(DWORD ticks, bool doMix)
	{
		if(!buffer || !bufferSize || !waveformat)
			return;

		if(status & DSBSTATUS_PLAYING)
		{
			// order of things is VERY important in here:
			// update write cursor, notify, update play cursor, do mixing.
			// for test cases, listen to the music in Iji and Perfect Cherry Blossom.
			// they're both sensitive to this order in different ways.

			DWORD bytes = TicksToBytes(ticks, frequency, *waveformat);

			DWORD cursorbase = playCursor;
			DWORD unwrappedPlayCursor = playCursor + bytes;
			if(unwrappedPlayCursor < bufferSize)
			{
				// didn't reach the end of the buffer, everything's normal
				writeCursor = unwrappedPlayCursor;
				if(numNotifies)
					NotifyInRange(cursorbase, unwrappedPlayCursor);
				playCursor = unwrappedPlayCursor;
				if(doMix)
					MixFromTo(cursorbase, playCursor, 0, contiguousMixOutBufSize, playCursor >= ((status & DSBSTATUS_LOOPING) ? allocated : bufferSize) - waveformat->nBlockAlign);
			}
			else
			{
				// reached the end of the buffer!
				// what we do now depends on whether we're playing it as a looping sound.
				if(!(status & DSBSTATUS_LOOPING))
				{
					// we're not looping, so mix only until the end of this sound buffer
					writeCursor = 0;
					if(numNotifies)
						NotifyInRange(cursorbase, bufferSize);
					playCursor = 0;
					if(doMix)
						MixFromTo(cursorbase, bufferSize, 0, contiguousMixOutBufSize*(bufferSize-cursorbase)/bytes, true);

					// and stop this sound buffer from playing,
					// because of course non-looping sounds should stop automatically when they reach the end.
					status &= ~DSBSTATUS_PLAYING;
					if(numNotifies)
						NotifyOfStop();
				}
				else
				{
					DWORD wrappedPlayCursor = unwrappedPlayCursor % bufferSize;

					// make sure playCursor is in the valid range for next time
					writeCursor = wrappedPlayCursor;

					if(numNotifies)
					{
						NotifyInRange(cursorbase, bufferSize);
						NotifyInRange(0, (bytes >= bufferSize) ? cursorbase : wrappedPlayCursor);
					}

					playCursor = wrappedPlayCursor;

					if(doMix)
					{
						// we're looping... the math for doing this correctly can become quite terrible
						// especially because the sound can be so short that it loops many times at once.
						// so I'm going to "cheat" and simply replicate the sound buffer as many times as needed.
						if(unwrappedPlayCursor + waveformat->nBlockAlign > allocated)
						{
							EnterCriticalSection(&m_bufferCS);
							allocated = unwrappedPlayCursor + waveformat->nBlockAlign;
							buffer = (unsigned char*)realloc(buffer, allocated);
							if(!buffer) { buffer = (unsigned char*)realloc(buffer, allocated); }
							if(!buffer) { debuglog(LCF_ERROR, "FAILED TO ALLOCATE LOOP BUFFER\n"); }
							ReplicateBufferIntoExtraAllocated();
							LeaveCriticalSection(&m_bufferCS);
						}

						// now that we know the buffer's data is replicated far enough,
						// we can mix the sound in one go exactly like the "normal" case.
						MixFromTo(cursorbase, unwrappedPlayCursor, 0, contiguousMixOutBufSize, false);
					}
				}
			}
		}
	}

	void NotifyOfStop()
	{
		for(int i = 0; i < numNotifies; i++)
		{
			DSBPOSITIONNOTIFY& notify = notifies[i];
			if(notify.dwOffset == DSBPN_OFFSETSTOP)
			{
				//debugprintf("notify! stop\n");
				SetEvent(notify.hEventNotify);
			}
		}
	}

	void NotifyInRange(DWORD a, DWORD b)
	{
		for(int i = 0; i < numNotifies; i++)
		{
			DSBPOSITIONNOTIFY& notify = notifies[i];
			if(notify.dwOffset >= a && notify.dwOffset < b)
			{
				//debugprintf("notify! %d <= %d < %d\n", a,notify.dwOffset,b);
				DWORD prevPlayCursor = playCursor;
				playCursor = notify.dwOffset;
				SetEvent(notify.hEventNotify);
				playCursor = prevPlayCursor;
			}
		}
	}

	void ReplicateBufferIntoExtraAllocated()
	{
		// replicate the first bufferSize bytes up to the total amount allocated
		for(DWORD copied = bufferSize; copied < allocated; copied += bufferSize)
			memcpy(buffer + copied, buffer, min(bufferSize, allocated - copied));
	}

	void MixFromTo(DWORD pos1, DWORD pos2, DWORD outPos1, DWORD outPos2, bool pos2IsLastSample)
	{
		//DWORD mySamplesPerSec = waveformat->nSamplesPerSec;
		DWORD outSamplesPerSec = contiguousMixOutBufFormat->nSamplesPerSec;
		WORD myBitsPerSample = waveformat->wBitsPerSample;
		WORD outBitsPerSample = contiguousMixOutBufFormat->wBitsPerSample;
		WORD myChannels = (waveformat->nChannels >= 2) ? 2 : 1;
		WORD outChannels = (contiguousMixOutBufFormat->nChannels >= 2) ? 2 : 1;

		WORD myBlockSize = waveformat->nBlockAlign;
		WORD outBlockSize = contiguousMixOutBufFormat->nBlockAlign;

		MixFromToInternal(pos1, pos2, outPos1, outPos2, pos2IsLastSample,
			outSamplesPerSec, myBitsPerSample, outBitsPerSample, myChannels, outChannels, myBlockSize, outBlockSize,
			buffer, contiguousMixOutBuf, volumes);
	}

	void CalcVolumeScales()
	{
		volumes.leftVolumeAsScale = (DWORD)(pow(10.0, 0.0005 * (volume + min(0,-pan))) * 65536.0);
		volumes.rightVolumeAsScale = (DWORD)(pow(10.0, 0.0005 * (volume + min(0,pan))) * 65536.0);
		debuglog(LCF_DSOUND, "volume = %d -> (%g, %g)\n", volume, volumes.leftVolumeAsScale/65536.0f, volumes.rightVolumeAsScale/65536.0f);
	}

	static void PreSuspend()
	{
		EnterCriticalSection(&s_myMixingOutputBufferCS);
		//if(s_myMixingOutputBuffer)
		//{
		//	DWORD status = DSBSTATUS_PLAYING;
		//	while(SUCCEEDED(s_myMixingOutputBuffer->Stop()))
		//		if(FAILED(s_myMixingOutputBuffer->GetStatus(&status)) || !(status & DSBSTATUS_PLAYING))
		//			break;
		//}
		//if(s_primaryBuffer)
		//{
		//	DWORD status = DSBSTATUS_PLAYING;
		//	while(SUCCEEDED(s_primaryBuffer->Stop()))
		//		if(FAILED(s_primaryBuffer->GetStatus(&status)) || !(status & DSBSTATUS_PLAYING))
		//			break;
		//}
		if(s_myMixingOutputBuffer)
		{
			verbosedebugprintf(__FUNCTION__": setting volume on 0x%X to %d\n", s_myMixingOutputBuffer, (int)DSBVOLUME_MIN);

			//s_myMixingOutputBuffer->GetVolume(&s_mixSuspendVol);
			s_myMixingOutputBuffer->SetVolume(DSBVOLUME_MIN);
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}
	static void PostResume()
	{
		EnterCriticalSection(&s_myMixingOutputBufferCS);
		//if(s_primaryBuffer)
		//	s_primaryBuffer->Play(0,0,DSBPLAY_LOOPING);
		//if(s_myMixingOutputBuffer)
		//	s_myMixingOutputBuffer->Play(0,0,DSBPLAY_LOOPING);
		if(s_myMixingOutputBuffer)
		{
			verbosedebugprintf(__FUNCTION__": setting volume on 0x%X to %d\n", s_myMixingOutputBuffer, (int)DSBVOLUME_MAX);

			//s_myMixingOutputBuffer->SetVolume(s_mixSuspendVol);
			s_myMixingOutputBuffer->SetVolume(DSBVOLUME_MAX);
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

	static void DisableMixingBufferOutput(bool disable)
	{
		EnterCriticalSection(&s_myMixingOutputBufferCS);
		if(s_myMixingOutputBuffer)
		{
			if(disable)
				s_myMixingOutputBuffer->Stop();
			else
				s_myMixingOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

	// ForceAlign forces the mixing buffer playback position to slightly before the last place we wrote to.
	// if called frequently, this causes the sound to more accurately match up with the action,
	// but the downside is that it causes increased crackling to move the playback position so much.
	// thus, ForceAlign should not be called when running at normal speed,
	// but it's a good idea to call it every frame in either slow motion or fast-forward
	// (because error accumulates faster when not at 100% speed, to the point where
	//  correcting for that error sounds better even with the increased crackling.)
	static void ForceAlign(bool earlier=false)
	{
		EnterCriticalSection(&s_myMixingOutputBufferCS);
		if(s_myMixingOutputBuffer)
		{
			int offset = contiguousMixOutBufOffset;

			if(earlier)
			{
				DSBCAPS caps = { sizeof(DSBCAPS) };
				if(SUCCEEDED(s_myMixingOutputBuffer->GetCaps(&caps)))
				{
					DWORD play, write;
					if(SUCCEEDED(s_myMixingOutputBuffer->GetCurrentPosition(&play, &write)))
					{
						if(write < play)
							write += caps.dwBufferBytes;
						if(write >= play)
						{
							offset -= (write - play);
							offset += caps.dwBufferBytes;
							offset %= caps.dwBufferBytes;
						}
					}
				}
			}

			s_myMixingOutputBuffer->SetCurrentPosition(offset);
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

private:
	unsigned char* buffer;
	DWORD bufferSize;
	DWORD allocated;
	unsigned char* lockBuf;
	LPVOID lockPtr1; DWORD lockBytes1;
	LPVOID lockPtr2; DWORD lockBytes2;
	DWORD numRemainingUnlocksBeforeLeavingLockBufResident;
	DWORD frequency;
	LONG pan;
	LONG volume;
	CachedVolumeAndPan volumes;
	DWORD status;
	DWORD playCursor;
	DWORD writeCursor;
	DWORD capsFlags;
	LPWAVEFORMATEX waveformat;
	bool m_isFakePrimary;
	CRITICAL_SECTION m_bufferCS;
	CRITICAL_SECTION m_lockBufferCS;

	DSBPOSITIONNOTIFY* notifies;
	int numNotifies;

	ULONG refcount;

}; // EmulatedDirectSoundBuffer

EmulatedDirectSoundBuffer::MyBufferList EmulatedDirectSoundBuffer::soundBuffers;












// original method: simply wrap DirectSoundBuffer to keep track of it for savestates

template <typename IDirectSoundBufferN>
class MyDirectSoundBuffer : public IDirectSoundBufferN
{
public:
	typedef MyDirectSoundBuffer<IDirectSoundBufferN> MyType;
	typedef std::map<MyType*, DWORD> MyBufferMap;

	MyDirectSoundBuffer(IDirectSoundBufferN* dsb) : m_dsb(dsb)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		EnterCriticalSection(&s_soundBufferListCS);
		if(soundBuffers.find(this) == soundBuffers.end())
			soundBuffers.insert(std::make_pair(this, GetCurrentThreadId()));
		LeaveCriticalSection(&s_soundBufferListCS);
		debuglog(LCF_DSOUND, "%d sound buffers\n", soundBuffers.size());
		tempAudioPtr1 = 0;
		tempAudioSize1 = 0;
		tempAudioPtr2 = 0;
		tempAudioSize2 = 0;
		storedAudioBuf1 = 0;
		storedAudioSize1 = 0;
		storedAudioBuf2 = 0;
		storedAudioSize2 = 0;
	}
	~MyDirectSoundBuffer()
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		EnterCriticalSection(&s_soundBufferListCS);
		soundBuffers.erase(this);
		LeaveCriticalSection(&s_soundBufferListCS);
		debuglog(LCF_DSOUND, "%d sound buffers\n", soundBuffers.size());
		if(this == s_primaryBuffer)
		{
			s_primaryBuffer = NULL;
			s_primaryBufferFormatSet = false;
		}
		EnterCriticalSection(&s_myMixingOutputBufferCS);
		if(this == s_myMixingOutputBuffer)
		{
			s_myMixingOutputBuffer = NULL;
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

	// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		while(lockdown) Sleep(1); 
		/*dsound*/debugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
		HRESULT rv = m_dsb->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		return m_dsb->AddRef();
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		//bool dst = disableSelfTicking;
		//disableSelfTicking = true;
		tls.callerisuntrusted++;
		ULONG count = m_dsb->Release();
		tls.callerisuntrusted--;
		//disableSelfTicking = dst;
		if(0 == count)
			delete this;
		return count;
	}

    // IDirectSoundBuffer methods
    STDMETHOD(GetCaps)              (LPDSBCAPS pDSBufferCaps)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetCaps(pDSBufferCaps);
	}
    STDMETHOD(GetCurrentPosition)   (LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
	{
		while(lockdown) Sleep(1);
		HRESULT rv = m_dsb->GetCurrentPosition(pdwCurrentPlayCursor, pdwCurrentWriteCursor);
		debuglog(LCF_DSOUND, __FUNCTION__ " called (playCursor=%d, writeCursor=%d).\n", pdwCurrentPlayCursor?*pdwCurrentPlayCursor:0,pdwCurrentWriteCursor?*pdwCurrentWriteCursor:0);
////		debuglog(LCF_DSOUND, "pos: %d -> %d\n", detTimer.GetTicks(), *pdwCurrentPlayCursor);
//		*pdwCurrentPlayCursor = (detTimer.GetTicks() * 100) % 200000;
		return rv;
	}
    STDMETHOD(GetFormat)            (LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		//debuglog(LCF_DSOUND, __FUNCTION__ "this=0x%X, m_dsb=0x%X, dwSizeAllocated=%d.\n", this, m_dsb, dwSizeAllocated);
		HRESULT rv = m_dsb->GetFormat(pwfxFormat, dwSizeAllocated, pdwSizeWritten);
		debuglog(LCF_DSOUND, __FUNCTION__ " returned format: %d %d %d\n", pwfxFormat->nSamplesPerSec, pwfxFormat->wBitsPerSample, pwfxFormat->nChannels);
		return rv;
	}
    STDMETHOD(GetVolume)            (LPLONG plVolume)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetVolume(plVolume);
	}
    STDMETHOD(GetPan)               (LPLONG plPan)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetPan(plPan);
	}
    STDMETHOD(GetFrequency)         (LPDWORD pdwFrequency)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetFrequency(pdwFrequency);
	}
    STDMETHOD(GetStatus)            (LPDWORD pdwStatus)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetStatus(pdwStatus);
	}
    STDMETHOD(Initialize)           (LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->Initialize(pDirectSound, pcDSBufferDesc);
	}
    STDMETHOD(Lock)                 (DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
	{
		while(lockdown) Sleep(1); 
		HRESULT hr = m_dsb->Lock(dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags);
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		//debuglog(LCF_DSOUND, "THIS EQUALS: 0x%X.\n", this);
		//debuglog(LCF_DSOUND, "BASE EQUALS: 0x%X.\n", m_dsb);
		//debuglog(LCF_DSOUND, "SIZEOF() EQUALS: 0x%X.\n", sizeof(MyDirectSoundBuffer));
		//debuglog(LCF_DSOUND, "SIZEOF(BASE) EQUALS: 0x%X.\n", sizeof(IDirectSoundBufferN));
		//debuglog(LCF_DSOUND, "AUDIOPTR1 EQUALS: 0x%X.\n", ppvAudioPtr1);
		//debuglog(LCF_DSOUND, "AUDIOPTR2 EQUALS: 0x%X.\n", ppvAudioPtr2);
		//cmdprintf("NOLOAD: %d,%d.\n", *ppvAudioPtr1, *pdwAudioBytes1);
		//cmdprintf("NOLOAD: %d,%d.\n", *ppvAudioPtr2, *pdwAudioBytes2);
		return hr;
	}

	DWORD lastPlay1,lastPlay2,lastPlay3;

    STDMETHOD(Play)                 (DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
	{
		if(tasflags.emuMode & EMUMODE_NOPLAYBUFFERS)
			return DS_OK;
		while(lockdown) Sleep(1); 
		lastPlay1=dwReserved1;
		lastPlay2=dwPriority;
		lastPlay3=dwFlags;
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->Play(dwReserved1, dwPriority, dwFlags);
	}
    STDMETHOD(SetCurrentPosition)   (DWORD dwNewPosition)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->SetCurrentPosition(dwNewPosition);
	}
    STDMETHOD(SetFormat)            (LPCWAVEFORMATEX pcfxFormat)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(pcfxFormat)
			debuglog(LCF_DSOUND, "pcfxFormat = %d %d %d", pcfxFormat->nChannels, pcfxFormat->nSamplesPerSec, pcfxFormat->wBitsPerSample);
		//WAVEFORMATEX wfmt = {};
		//DWORD bla = 0;
		//this->GetFormat(&wfmt, sizeof(WAVEFORMATEX), &bla);

		if((tasflags.emuMode & EMUMODE_EMULATESOUND)
		&& s_primaryBufferFormatSet)
		{
			debuglog(LCF_DSOUND, __FUNCTION__ " denied setting format, in case it disagrees with the mixing buffer.");
			return DS_OK;
		}

		HRESULT rv = m_dsb->SetFormat(pcfxFormat);
		if(SUCCEEDED(rv))
			s_primaryBufferFormatSet = true;
		return rv;
	}
    STDMETHOD(SetVolume)            (LONG lVolume)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");

		verbosedebugprintf(__FUNCTION__": setting volume on 0x%X/0x%X to %d\n", this, m_dsb, (int)lVolume);

		return m_dsb->SetVolume(lVolume);
	}
    STDMETHOD(SetPan)               (LONG lPan)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->SetPan(lPan);
	}
    STDMETHOD(SetFrequency)         (DWORD dwFrequency)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->SetFrequency(dwFrequency);
	}
    STDMETHOD(Stop)                 ()
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->Stop();
	}
	STDMETHOD(Unlock)               (LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
	{
		while(lockdown) Sleep(1); 
		//storedAudioSize1 = dwAudioBytes1;
		//storedAudioSize2 = dwAudioBytes2;
		//if((storedAudioBuf1 = realloc(storedAudioBuf1, dwAudioBytes1)))
		//	memcpy(storedAudioBuf1, pvAudioPtr1, dwAudioBytes1);
		//if((storedAudioBuf2 = realloc(storedAudioBuf2, dwAudioBytes2)))
		//	memcpy(storedAudioBuf2, pvAudioPtr2, dwAudioBytes2);
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
	}
    STDMETHOD(Restore)              ()
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->Restore();
	}

    // IDirectSoundBuffer8 methods
    STDMETHOD(SetFX)                (DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->SetFX(dwEffectsCount, pDSFXDesc, pdwResultCodes);
	}
    STDMETHOD(AcquireResources)     (DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->AcquireResources(dwFlags, dwEffectsCount, pdwResultCodes);
	}
    STDMETHOD(GetObjectInPath)      (REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject)
	{
		while(lockdown) Sleep(1); 
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetObjectInPath(rguidObject, dwIndex, rguidInterface, ppObject);
	}


	// custom methods
	STDMETHOD(BackDoorAcquireLock)()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, 0x%X) called.\n", this, m_dsb);
		HRESULT hr = m_dsb->Lock(0, 0, &tempAudioPtr1, &tempAudioSize1, &tempAudioPtr2, &tempAudioSize2, DSBLOCK_FROMWRITECURSOR|DSBLOCK_ENTIREBUFFER);
		if(!FAILED(hr))
			hr = m_dsb->Unlock(tempAudioPtr1, tempAudioSize1, tempAudioPtr2, tempAudioSize2);
		return 0;
	}

	STDMETHOD(BackDoorGetStatus)(LPDWORD pdwStatus)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		return m_dsb->GetStatus(pdwStatus);
	}
	STDMETHOD(BackDoorSuspend)()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		m_dsb->GetStatus(&status);
		m_dsb->GetPan(&pan);
		m_dsb->GetFrequency(&frequency);
		m_dsb->GetVolume(&volume);
		m_dsb->Stop();
		m_dsb->GetCurrentPosition(&playCursor, &writeCursor);
		return 0;
	}

	STDMETHOD(BackDoorResume)()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		//HRESULT hr = m_dsb->Lock(0, 0, &tempAudioPtr1, &tempAudioSize1, &tempAudioPtr2, &tempAudioSize2, DSBLOCK_FROMWRITECURSOR|DSBLOCK_ENTIREBUFFER);
		//if(storedAudioBuf1)
		//	memcpy(tempAudioPtr1, storedAudioBuf1, min(storedAudioSize1,tempAudioSize1));
		//if(storedAudioBuf2)
		//	memcpy(tempAudioPtr2, storedAudioBuf2, min(storedAudioSize2,tempAudioSize2));
		//if(!FAILED(hr))
		//	hr = m_dsb->Unlock(tempAudioPtr1, tempAudioSize1, tempAudioPtr2, tempAudioSize2);

		m_dsb->Stop();
		if(this != s_primaryBuffer)
			m_dsb->SetCurrentPosition(playCursor); // can't remember why I had this disabled before, maybe it sounds bad in some game? it seems correct though
		if(status & DSBSTATUS_PLAYING)
			m_dsb->Play(lastPlay1,lastPlay2,lastPlay3);
		else
			m_dsb->Stop();
		m_dsb->SetPan(pan);
		m_dsb->SetFrequency(frequency);
		verbosedebugprintf(__FUNCTION__": setting volume on 0x%X/0x%X to %d\n", this, m_dsb, (int)volume);
		m_dsb->SetVolume(volume);
		return 0;
	}

	static void BackDoorLockAll()
	{
		MyBufferMap::iterator iter;
		for(iter = soundBuffers.begin(); iter != soundBuffers.end(); iter++)
		{
			MyType* dsb = iter->first;
			dsb->BackDoorAcquireLock(); // other threads must NOT be suspended when this is called!
		}
	}
	static void BackDoorStopAll()
	{
		MyBufferMap::iterator iter;
		for(iter = soundBuffers.begin(); iter != soundBuffers.end(); iter++)
		{
			MyType* dsb = iter->first;
			dsb->BackDoorSuspend();
			DWORD status = DSBSTATUS_PLAYING;
			while(status & DSBSTATUS_PLAYING)
			{
				dsb->BackDoorGetStatus(&status);
				if(status & DSBSTATUS_PLAYING)
					debugprintf("WAITING FOR DIRECTSOUND\n");
			}
		}
	}
	static void FrontDoorStopAll()
	{
		MyBufferMap::iterator iter;
		for(iter = soundBuffers.begin(); iter != soundBuffers.end(); iter++)
		{
			MyType* dsb = iter->first;
			dsb->Stop();
		}
	}
	static void BackDoorRestoreAll()
	{
		MyBufferMap::iterator iter;
		for(iter = soundBuffers.begin(); iter != soundBuffers.end(); iter++)
		{
			MyType* dsb = iter->first;
			dsb->BackDoorResume(); // other threads must NOT be suspended when this is called!
		}
	}

	//void Unhook(void** ppvObj)
	//{
	//	debuglog(LCF_DSOUND, "Unhooked a MyDirectSoundBuffer.\n");
	//	*ppvObj = (void*)m_dsb;
	//	m_dsb = NULL;
	//	delete this;
	//}

	static MyBufferMap soundBuffers;

	IDirectSoundBufferN* m_dsb;
private:

	void* tempAudioPtr1;
	DWORD tempAudioSize1;
	void* tempAudioPtr2;
	DWORD tempAudioSize2;

	void* storedAudioBuf1;
	DWORD storedAudioSize1;
	void* storedAudioBuf2;
	DWORD storedAudioSize2;

	LONG pan;
	DWORD frequency;
	LONG volume;
	DWORD status;
	DWORD playCursor;
	DWORD writeCursor;
};

template<> HRESULT MyDirectSoundBuffer<IDirectSoundBuffer>::SetFX(DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectSoundBuffer<IDirectSoundBuffer>::AcquireResources(DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectSoundBuffer<IDirectSoundBuffer>::GetObjectInPath(REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID *ppObject) IMPOSSIBLE_IMPL

template<> MyDirectSoundBuffer<IDirectSoundBuffer>::MyBufferMap MyDirectSoundBuffer<IDirectSoundBuffer>::soundBuffers;
template<> MyDirectSoundBuffer<IDirectSoundBuffer8>::MyBufferMap MyDirectSoundBuffer<IDirectSoundBuffer8>::soundBuffers;


DEFINE_LOCAL_GUID(IID_IDirectSound, 0x279AFA83,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_LOCAL_GUID(IID_IDirectSound8,0xC50A7E93,0xF395,0x4834,0x9E,0xF6,0x7F,0xA9,0x9D,0xE5,0x09,0x66);

template<typename IDirectSoundN> struct IDirectSoundTraits {};
template<> struct IDirectSoundTraits<IDirectSound>   { enum{NUMBER = 1}; };
template<> struct IDirectSoundTraits<IDirectSound8>  { enum{NUMBER = 8}; };

static BOOL s_releasingDirectSound = FALSE;

template <typename IDirectSoundN>
class MyDirectSound : public IDirectSoundN
{
public:
	MyDirectSound(IDirectSoundN* ds) : m_ds(ds), m_myMixingOutputBuffer(NULL), m_fallbackRefcount(1)
	{
		debuglog(LCF_DSOUND, "MyDirectSound created.\n");
		noDirectSoundOutputAvailable = !m_ds;

		//if(m_freeDS)
		//{
		//	m_freeDS->Release();
		//	m_freeDS = NULL;
		//}
	}


	/*** IUnknown methods ***/
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		dsounddebugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
		if(ppvObj)
		{
			if(riid == IID_IUnknown) { *ppvObj = (IUnknown*)this; AddRef(); return S_OK; }
			if(riid == IID_IDirectSound) { *ppvObj = (IDirectSound*)this; AddRef(); return S_OK; }
			if(IDirectSoundTraits<IDirectSoundN>::NUMBER == 8 && riid == IID_IDirectSound8) { *ppvObj = (IDirectSound8*)this; AddRef(); return S_OK; }
		}
		if(!m_ds)
		{
			if(riid == IID_IDirectSoundSinkFactory)
			{
				if(!s_pEmulatedDirectSoundSinkFactory)
					s_pEmulatedDirectSoundSinkFactory = new EmulatedDirectSoundSinkFactory();
				if(s_pEmulatedDirectSoundSinkFactory)
				{
					*ppvObj = (IDirectSoundSinkFactory*)s_pEmulatedDirectSoundSinkFactory;
					s_pEmulatedDirectSoundSinkFactory->AddRef();
					return S_OK;
				}
			}
			return E_NOINTERFACE;
		}
		HRESULT rv = m_ds->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		ULONG rv = m_ds ? m_ds->AddRef() : ++m_fallbackRefcount;
		debuglog(LCF_DSOUND, __FUNCTION__ " called, returned %d.\n", rv);
		return rv;
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		s_releasingDirectSound = true;

		ULONG prevCount = m_fallbackRefcount;
		if(m_ds)
			prevCount = (m_ds->AddRef(), m_ds->Release());
		if(prevCount == 1)
		{
			// if we called CreateMyMixingOutputSoundBuffer,
			// we have to release that buffer BEFORE m_ds gets deleted.
			if(m_myMixingOutputBuffer)
			{
				//m_freeMixingOutputBuffer = m_myMixingOutputBuffer;
				m_myMixingOutputBuffer->Release(); // this should set s_myMixingOutputBuffer = NULL in the destructor
				m_myMixingOutputBuffer = NULL;
			}
		}
		ULONG count;
		if(m_ds)
		{
		// HACK: sometimes this deadlocks (e.g. Garden of Coloured Lights and Iji),
		// so we store the variable in m_freeDS and postpone Release until the next frame boundary, where it seems much less likely to deadlock.
		// I tried reusing the directsound instead of freeing it but that made directshow go crazy after a few reuses.
		// generally it's necessary to play a movie that goes through several levels of Iji before you can be reasonably sure the deadlock problem isn't there...
		// if the problem is there, symptoms often won't show up until partway through level 2.
		// there might be a different (unrelated) deadlock on startup but that one only happens sometimes and the rest is fine if you get past it.
		// this is surprisingly fragile stuff... we seem to be relying on the CLSID_FilterGraphNoThread and mciSendCommand hacks now too.
		// maybe it would be better to only create one real DirectSound in the debugger process and stream to it like we do when recording to AVI.
			if(prevCount > 1)
			{
				count = m_ds->Release();
			}
			else
			{
				count = 0;
				m_freeDS = m_ds;
				m_ds = NULL;
			}
			//count = 1;
		}
		else
		{
			count = --m_fallbackRefcount;
		}
		if(0 == count)
			delete this;
		s_releasingDirectSound = false;

		return count;
	}

    // IDirectSound methods
    STDMETHOD(CreateSoundBuffer)    (LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");

		if(!pcDSBufferDesc || !ppDSBuffer)
			return DSERR_INVALIDPARAM;

		debuglog(LCF_DSOUND, "flags=0x%X, bytes=0x%X\n", pcDSBufferDesc->dwFlags, pcDSBufferDesc->dwBufferBytes);

		if(!(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER) && ((tasflags.emuMode & EMUMODE_EMULATESOUND) || !m_ds))
		{
			if(!(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND) && !usingVirtualDirectSound)
			{
				EnterCriticalSection(&s_myMixingOutputBufferCS);
				if(!m_myMixingOutputBuffer)
					CreateMyMixingOutputSoundBuffer(/*pcDSBufferDesc*/);
				LeaveCriticalSection(&s_myMixingOutputBufferCS);
			}
			*ppDSBuffer = (LPDIRECTSOUNDBUFFER)(new EmulatedDirectSoundBuffer());
			(*ppDSBuffer)->Initialize(this, pcDSBufferDesc);
			return DS_OK;
		}
		else
		{
			if(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
				debuglog(LCF_DSOUND, "creating s_primaryBuffer flags=0x%X\n", pcDSBufferDesc->dwFlags);
			HRESULT hr = !m_ds ? -1 : m_ds->CreateSoundBuffer(pcDSBufferDesc, ppDSBuffer, pUnkOuter);
			if(SUCCEEDED(hr))
			{
				HookCOMInterface(IID_IDirectSoundBuffer, (LPVOID*)ppDSBuffer, true);

				if(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
				{
					s_primaryBuffer = *ppDSBuffer;
					debuglog(LCF_DSOUND, "s_primaryBuffer == 0x%X\n", s_primaryBuffer);

					if(tasflags.emuMode & EMUMODE_EMULATESOUND)
					{
						// let's make the primary buffer default to our mixing buffer format,
						// to minimize conversion loss.
						// this might make the sound unfaithfully-high-quality,
						// but that seems better than unfaithfully-low-quality
						// (which would happen because the EmulatedDirectSoundBuffer::Mix conversions
						// seem to be slightly lower quality than those of DirectSound)
						WAVEFORMATEX waveformat;
						waveformat.nChannels = tasflags.audioChannels;
						waveformat.nSamplesPerSec = tasflags.audioFrequency;
						waveformat.wBitsPerSample = tasflags.audioBitsPerSecond;
						waveformat.wFormatTag = WAVE_FORMAT_PCM;
						waveformat.nBlockAlign = waveformat.wBitsPerSample * waveformat.nChannels / 8;
						waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;
						waveformat.cbSize = 0;
						s_primaryBufferFormatSet = false;
						s_primaryBuffer->SetFormat(&waveformat);
						s_primaryBufferFormatSet = true;
					}
				}
			}
			else if(!m_ds)
			{
				// pretend to have a primary buffer
				*ppDSBuffer = (LPDIRECTSOUNDBUFFER)(new EmulatedDirectSoundBuffer(true));
				(*ppDSBuffer)->Initialize(this, pcDSBufferDesc);
				hr = DS_OK;
			}
			return hr;
		}
	}
    STDMETHOD(GetCaps)              (LPDSCAPS pDSCaps)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		enum{defaultCaps = DSCAPS_SECONDARYMONO|DSCAPS_SECONDARYSTEREO|DSCAPS_SECONDARY8BIT|DSCAPS_SECONDARY16BIT};
		if(/*(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)*/ !m_ds)
		{
			if(pDSCaps)
				pDSCaps->dwFlags = defaultCaps;
			else
				return DSERR_INVALIDCALL;
			return DS_OK;
		}
		//if(!m_ds) return DSERR_INVALIDCALL;
		HRESULT hr = m_ds->GetCaps(pDSCaps);
		if(SUCCEEDED(hr) && (tasflags.emuMode & EMUMODE_EMULATESOUND))
			pDSCaps->dwFlags |= defaultCaps;
		return hr;
	}
    STDMETHOD(DuplicateSoundBuffer) (LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		debuglog(LCF_DSOUND, __FUNCTION__ " pDSBufferOriginal=0x%X.\n", pDSBufferOriginal);

		if(is_of_type<LPDIRECTSOUNDBUFFER,EmulatedDirectSoundBuffer*>(pDSBufferOriginal))
		{
			debuglog(LCF_DSOUND, __FUNCTION__ " -> pDSBufferOriginal=0x%X, version=emulated.\n", pDSBufferOriginal);
			*ppDSBufferDuplicate = EmulatedDirectSoundBuffer::Duplicate((EmulatedDirectSoundBuffer*)pDSBufferOriginal);
			return DS_OK;
		}

		if(!m_ds) return DSERR_UNSUPPORTED;

		bool hookIt = true;
		if(is_of_type<LPDIRECTSOUNDBUFFER,MyDirectSoundBuffer<IDirectSoundBuffer8>*>(pDSBufferOriginal))
		{
			pDSBufferOriginal = ((MyDirectSoundBuffer<IDirectSoundBuffer8>*)pDSBufferOriginal)->m_dsb;
			debuglog(LCF_DSOUND, __FUNCTION__ " -> pDSBufferOriginal=0x%X, version=8.\n", pDSBufferOriginal);
		}
		else if(is_of_type<LPDIRECTSOUNDBUFFER,MyDirectSoundBuffer<IDirectSoundBuffer>*>(pDSBufferOriginal))
		{
			pDSBufferOriginal = ((MyDirectSoundBuffer<IDirectSoundBuffer>*)pDSBufferOriginal)->m_dsb;
			debuglog(LCF_DSOUND, __FUNCTION__ " -> pDSBufferOriginal=0x%X, version=1.\n", pDSBufferOriginal);
		}
		else 
		{
			debuglog(LCF_DSOUND, __FUNCTION__ " -> pDSBufferOriginal=0x%X, version=raw.\n");
			hookIt = false;
		}

		HRESULT hr = m_ds->DuplicateSoundBuffer(pDSBufferOriginal, ppDSBufferDuplicate);
		if(SUCCEEDED(hr) && hookIt)
			HookCOMInterface(IID_IDirectSoundBuffer, (LPVOID*)ppDSBufferDuplicate);
		return hr;
	}
    STDMETHOD(SetCooperativeLevel)  (HWND hwnd, DWORD dwLevel)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", dwLevel);
		if(!m_ds) return DS_OK;
		return m_ds->SetCooperativeLevel(hwnd, dwLevel);
	}
    STDMETHOD(Compact)              ()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!m_ds) return DS_OK;
		return m_ds->Compact();
	}
    STDMETHOD(GetSpeakerConfig)     (LPDWORD pdwSpeakerConfig)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!m_ds) return DSERR_UNSUPPORTED;
		return m_ds->GetSpeakerConfig(pdwSpeakerConfig);
	}
    STDMETHOD(SetSpeakerConfig)     (DWORD dwSpeakerConfig)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!m_ds) return DSERR_UNSUPPORTED;
		return m_ds->SetSpeakerConfig(dwSpeakerConfig);
	}
    STDMETHOD(Initialize)           (LPCGUID pcGuidDevice)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!m_ds) return DS_OK;
		if((tasflags.emuMode & EMUMODE_EMULATESOUND) && (s_primaryBuffer))
		{
			// Lyle fix, since that game initializes directsound multiple times
			// and expects that to stop all playing sounds each time.
			// doing this when the primary buffer gets reassigned would break la-mulana.
			// and doing this in SetCooperativeLevel would break iji.
			MyDirectSoundBuffer<IDirectSoundBuffer>::FrontDoorStopAll();
			MyDirectSoundBuffer<IDirectSoundBuffer8>::FrontDoorStopAll();
		}
		ThreadLocalStuff& curtls = tls;
		const char* oldName = curtls.curThreadCreateName;
		curtls.curThreadCreateName = "DirectSound";
		curtls.callerisuntrusted++;
		
		HRESULT rv = m_ds->Initialize(pcGuidDevice);
		
		curtls.callerisuntrusted--;
		curtls.curThreadCreateName = oldName;
		return rv;
	}

    // IDirectSound8 methods
    STDMETHOD(VerifyCertification)  (LPDWORD pdwCertified)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
		if(!m_ds) return DSERR_UNSUPPORTED;
		return m_ds->VerifyCertification(pdwCertified);
	}


	// custom methods
    void CreateMyMixingOutputSoundBuffer(/*LPCDSBUFFERDESC pcDSBufferDesc*/)
	{
		WAVEFORMATEX waveformat;
		// we could get the format of s_primaryBuffer here,
		// but we might actually need a higher-quality format than that
		// to keep our mixing results in, so let's always use these settings
		waveformat.nChannels = tasflags.audioChannels;
		waveformat.nSamplesPerSec = tasflags.audioFrequency;
		waveformat.wBitsPerSample = tasflags.audioBitsPerSecond;
		verbosedebugprintf("mixing format: %d %d %d\n", waveformat.nSamplesPerSec, waveformat.wBitsPerSample, waveformat.nChannels);

		// the rest of this applies no matter what settings are set above
		waveformat.wFormatTag = WAVE_FORMAT_PCM;
		waveformat.nBlockAlign = waveformat.wBitsPerSample * waveformat.nChannels / 8;
		waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;
		waveformat.cbSize = 0;

		DSBUFFERDESC desc = {sizeof(DSBUFFERDESC) };
		desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS /*| (tasflags.forceSoftware ? DSBCAPS_LOCSOFTWARE : 0)*/;
		desc.dwBufferBytes = waveformat.nAvgBytesPerSec / 6; // 0.167 second buffer
		desc.lpwfxFormat = &waveformat;

		debuglog(LCF_DSOUND, "creating m_myMixingOutputBuffer (m was 0x%X, s was 0x%X)\n", m_myMixingOutputBuffer, s_myMixingOutputBuffer);
		EnterCriticalSection(&s_myMixingOutputBufferCS);

		if(s_myMixingOutputBuffer)
		{
			// hack? fix for annoying audio in legend of princess
			s_myMixingOutputBuffer->Stop();
		}

		HRESULT hr;
		//if(m_freeMixingOutputBuffer)
		//{
		//	m_myMixingOutputBuffer = m_freeMixingOutputBuffer;
		//	m_freeMixingOutputBuffer = NULL;
		//	hr = -1;//S_OK;
		//	s_myMixingOutputBuffer = m_myMixingOutputBuffer;
		//}
		//else
		{
			hr = !m_ds ? -1 : m_ds->CreateSoundBuffer(&desc, &m_myMixingOutputBuffer, NULL);
		}
		if(SUCCEEDED(hr))
		{
			HookCOMInterface(IID_IDirectSoundBuffer, (LPVOID*)&m_myMixingOutputBuffer, true);
			s_myMixingOutputBuffer = m_myMixingOutputBuffer;
			debuglog(LCF_DSOUND, "s_myMixingOutputBuffer == 0x%X\n", s_myMixingOutputBuffer);

			// make sure buffer is initially empty (silence), just in case the documentation is telling the truth for once
			unsigned char* bufPtr1; DWORD bufSize1;
			if(SUCCEEDED(m_myMixingOutputBuffer->Lock(0, desc.dwBufferBytes, (LPVOID*)&bufPtr1, &bufSize1, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
			{
				if(bufPtr1)
					memset(bufPtr1, (waveformat.wBitsPerSample <= 8) ? 0x80 : 0, bufSize1);
				m_myMixingOutputBuffer->Unlock(bufPtr1, bufSize1, NULL, NULL);
			}

			// start playing it (we'll stream sound into it)
			m_myMixingOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
		}
		LeaveCriticalSection(&s_myMixingOutputBufferCS);
	}

	static IDirectSoundN* m_freeDS;
	//static IDirectSoundBuffer* m_freeMixingOutputBuffer;
private:
	IDirectSoundN* m_ds;
	int m_fallbackRefcount;

	// there's only one s_myMixingOutputBuffer at a time,
	// but due to creation order issues we have to temporarily keep track of one per DirectSound object
	IDirectSoundBuffer* m_myMixingOutputBuffer;
};

template<> HRESULT MyDirectSound<IDirectSound>::VerifyCertification(LPDWORD pdwCertified) IMPOSSIBLE_IMPL
template<> IDirectSound* MyDirectSound<IDirectSound>::m_freeDS = NULL;
template<> IDirectSound8* MyDirectSound<IDirectSound8>::m_freeDS = NULL;
//template<> IDirectSoundBuffer* MyDirectSound<IDirectSound>::m_freeMixingOutputBuffer = NULL;
//template<> IDirectSoundBuffer* MyDirectSound<IDirectSound8>::m_freeMixingOutputBuffer = NULL;



DEFINE_LOCAL_GUID(IID_IDirectMusic, 0x6536115A,0x7B2D,0x11D2,0xBA,0x18,0x00,0x00,0xF8,0x75,0xAC,0x12);
DEFINE_LOCAL_GUID(IID_IDirectMusic2,0x6FC2CAE1,0xBC78,0x11D2,0xAF,0xA6,0x00,0xAA,0x00,0x24,0xD8,0xB6);
DEFINE_LOCAL_GUID(IID_IDirectMusic8,0x2D3629F7,0x813D,0x4939,0x85,0x08,0xF0,0x5C,0x6B,0x75,0xFD,0x97);
DEFINE_LOCAL_GUID(IID_IDirectMusicPerformance, 0x07D43D03,0x6523,0x11D2,0x87,0x1D,0x00,0x60,0x08,0x93,0xB1,0xBD);
DEFINE_LOCAL_GUID(IID_IDirectMusicPerformance2, 0x6FC2CAE0, 0xBC78, 0x11D2, 0xAF, 0xA6, 0x0, 0xAA, 0x0, 0x24, 0xD8, 0xB6);
DEFINE_LOCAL_GUID(IID_IDirectMusicPerformance8, 0x679C4137,0xC62E,0x4147,0xB2,0xB4,0x9D,0x56,0x9A,0xCB,0x25,0x4C);

template <typename IDirectMusicPerformanceN> 
struct MyDirectMusicPerformance
{
	static BOOL Hook(IDirectMusicPerformanceN* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirectMusicPerformanceN, Init);
		rv |= VTHOOKFUNC(IDirectMusicPerformanceN, InitAudio);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectMusicPerformanceN* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirectMusicPerformanceN* pThis, REFIID riid, void** ppvObj)
	{
		dmusicdebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *Init)(IDirectMusicPerformanceN* pThis, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd);
    static HRESULT STDMETHODCALLTYPE MyInit(IDirectMusicPerformanceN* pThis, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd)
	{
		/*dmusic*/debugprintf(__FUNCTION__ " called.\n");
		const char* oldName = tls.curThreadCreateName;
		tls.curThreadCreateName = "DirectMusic";
		HRESULT rv = Init(pThis, ppDirectMusic, pDirectSound, hWnd);
		tls.curThreadCreateName = oldName;
		if(FAILED(rv) && tasflags.threadMode == 0)
			return S_OK;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *InitAudio)(IDirectMusicPerformanceN* pThis, IDirectMusic** ppDirectMusic, IDirectSound** ppDirectSound, HWND hWnd, DWORD dwDefaultPathType, DWORD dwPChannelCount, DWORD dwFlags, DMUS_AUDIOPARAMS *pParams);
    static HRESULT STDMETHODCALLTYPE MyInitAudio(IDirectMusicPerformanceN* pThis, IDirectMusic** ppDirectMusic, IDirectSound** ppDirectSound, HWND hWnd, DWORD dwDefaultPathType, DWORD dwPChannelCount, DWORD dwFlags, DMUS_AUDIOPARAMS *pParams)
	{
		/*dmusic*/debugprintf(__FUNCTION__ "(ppDirectMusic=0x%X, ppDirectSound=0x%X, *pDirectSound=0x%X, dwFlags=0x%X, pParams=0x%X) called.\n", ppDirectMusic, ppDirectSound, ppDirectSound?*ppDirectSound:0, dwFlags, pParams);
		const char* oldName = tls.curThreadCreateName;
		tls.curThreadCreateName = "DirectMusic";
		HRESULT rv = InitAudio(pThis, ppDirectMusic, ppDirectSound, hWnd, dwDefaultPathType, dwPChannelCount, dwFlags, pParams);
		tls.curThreadCreateName = oldName;
		if(FAILED(rv) && tasflags.threadMode == 0)
			return S_OK; // for Ninja Senki with threads disabled
		return rv;
	}
};

template<> BOOL MyDirectMusicPerformance<IDirectMusicPerformance>::Hook(IDirectMusicPerformance* obj)
{
	BOOL rv = FALSE;
	rv |= VTHOOKFUNC(IDirectMusicPerformance, Init);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}

#define DEF(x) template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusicPerformance<struct x>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusicPerformance<struct x>::Init)(x* pThis, IDirectMusic** ppDirectMusic, LPDIRECTSOUND pDirectSound, HWND hWnd) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusicPerformance<struct x>::InitAudio)(x* pThis, IDirectMusic** ppDirectMusic, IDirectSound** ppDirectSound, HWND hWnd, DWORD dwDefaultPathType, DWORD dwPChannelCount, DWORD dwFlags, DMUS_AUDIOPARAMS *pParams) = 0;
	DEF(IDirectMusicPerformance)
	DEF(IDirectMusicPerformance8)
#undef DEF




template <typename IDirectMusicN> 
struct MyDirectMusic
{
	static BOOL Hook(IDirectMusicN* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirectMusicN, GetMasterClock);
		rv |= VTHOOKFUNC(IDirectMusicN, SetMasterClock);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectMusicN* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirectMusicN* pThis, REFIID riid, void** ppvObj)
	{
		dmusicdebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetMasterClock)(IDirectMusicN* pThis, LPGUID pguidClock, IReferenceClock **ppReferenceClock);
	static HRESULT STDMETHODCALLTYPE MyGetMasterClock(IDirectMusicN* pThis, LPGUID pguidClock, IReferenceClock **ppReferenceClock)
	{
		HRESULT rv = GetMasterClock(pThis, pguidClock, ppReferenceClock);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IReferenceClock, (LPVOID*)ppReferenceClock);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *SetMasterClock)(IDirectMusicN* pThis, REFGUID rguidClock);
	static HRESULT STDMETHODCALLTYPE MySetMasterClock(IDirectMusicN* pThis, REFGUID rguidClock)
	{
		dmusicdebugprintf(__FUNCTION__ " called.\n");
		return DMUS_E_PORTS_OPEN; // disallow changing the clock
	}

};

#define DEF(x) template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusic<struct x>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusic<struct x>::GetMasterClock)(x* pThis, LPGUID pguidClock, IReferenceClock **ppReferenceClock) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirectMusic<struct x>::SetMasterClock)(x* pThis, REFGUID rguidClock) = 0;
	DEF(IDirectMusic)
	DEF(IDirectMusic2)
	DEF(IDirectMusic8)
#undef DEF





DEFINE_LOCAL_GUID(IID_IBaseFilter, 0x56a86895,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_LOCAL_GUID(IID_IMediaFilter,0x56a86899,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);
//DEFINE_LOCAL_GUID(IID_IAMOpenProgress,0x8e1c39a1,0xde53,0x11cf,0xaa,0x63,0x00,0x80,0xc7,0x44,0x52,0x8d);
//DEFINE_LOCAL_GUID(IID_IAMGraphStreams, 0x632105fa,0x072e,0x11d3,0x8a,0xf9,0x00,0xc0,0x4f,0xb6,0xbd,0x3d);


struct MyMediaFilter
{
	static BOOL Hook(IMediaFilter* obj)
	{
		//cmdprintf("SHORTTRACE: 3,50");

		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IMediaFilter, SetSyncSource);
		//rv |= VTHOOKFUNC(IMediaFilter, GetSyncSource); // seems unnecessary

		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IMediaFilter* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IMediaFilter* pThis, REFIID riid, void** ppvObj)
	{
		dmusicdebugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	//static HRESULT(STDMETHODCALLTYPE *GetSyncSource)             (IMediaFilter* pThis, IReferenceClock** pClock);
	//static HRESULT STDMETHODCALLTYPE MyGetSyncSource             (IMediaFilter* pThis, IReferenceClock** pClock)
	//{
	//	HRESULT rv = GetSyncSource(pThis, pClock);
	//	if(SUCCEEDED(rv) && pClock && *pClock)
	//		HookCOMInterface(IID_IReferenceClock, (LPVOID*)pClock);
	//	return rv;
	//}

	static HRESULT(STDMETHODCALLTYPE *SetSyncSource)             (IMediaFilter* pThis, IReferenceClock* pClock);
	static HRESULT STDMETHODCALLTYPE MySetSyncSource             (IMediaFilter* pThis, IReferenceClock* pClock)
	{
		dmusicdebugprintf(__FUNCTION__ "(0x%X) called.\n", pClock);
		if(pClock)
			HookCOMInterface(IID_IReferenceClock, (LPVOID*)&pClock);
		HRESULT rv = MyMediaFilter::SetSyncSource(pThis, pClock);
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyMediaFilter::QueryInterface)(IMediaFilter* pThis, REFIID riid, void** ppvObj) = 0;
//HRESULT (STDMETHODCALLTYPE* MyMediaFilter::GetSyncSource)(IMediaFilter* pThis, IReferenceClock** pClock) = 0;
HRESULT (STDMETHODCALLTYPE* MyMediaFilter::SetSyncSource)(IMediaFilter* pThis, IReferenceClock* pClock) = 0;


struct MyBaseFilter
{
	static BOOL Hook(IBaseFilter* obj)
	{
		return MyMediaFilter::Hook((IMediaFilter*)obj);
	}
};



//BOOL DebugVTable(void* iface, int entry)
//{
//	size_t* pVTable = (size_t*)*(size_t*)iface;
//	FARPROC oldPtr = (FARPROC)pVTable[entry];
//	debugprintf("DebugVTable: pVTable[%d] = 0x%X\n", entry, oldPtr);
//	cmdprintf("DEBUGTRACEADDRESS: %08X", oldPtr);
//	return TRUE;
//}

//struct MyDirectSoundSink
//{
//	static BOOL Hook(IUnknown* obj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", obj);
//		cmdprintf("SHORTTRACE: 3,50");
//		BOOL rv = FALSE;
//		//for(int i = 0; i < 1000; i++)
//		//	DebugVTable(obj,i);
//		return rv;
//	}
//};


//DEFINE_LOCAL_GUID(IID_IDirectSoundSink,???);
DEFINE_LOCAL_GUID(IID_IDirectSoundSinkFactory,0x2A8AF120,0xE9DE,0x4132,0xAA,0xA5,0x4B,0xDD,0xA5,0xF3,0x25,0xB8);
DEFINE_LOCAL_GUID(IID_IDirectSoundSinkSync,0x73A6A85A,0x493E,0x4C87,0xB4,0xA5,0xBE,0x53,0xEB,0x92,0x74,0x4B);
DEFINE_LOCAL_GUID(IID_IKsControl,0x28F54685,0x06FD,0x11D2,0xB2,0x7A,0x00,0xA0,0xC9,0x22,0x31,0x96);

#undef INTERFACE
#define INTERFACE IDirectSoundSinkFactory
DECLARE_INTERFACE_(IDirectSoundSinkFactory, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    // IDirectSoundSinkFactory? methods
    STDMETHOD(CreateSoundSink) (THIS_ LPWAVEFORMATEX lpWaveFormat, struct IDirectSoundSink** ppDSC) PURE;
};

#undef INTERFACE
#define INTERFACE IDirectSoundSink
DECLARE_INTERFACE_(IDirectSoundSink, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    // IDirectSoundSink? methods
	STDMETHOD(AddSource) (THIS_ struct IDirectSoundSource*) PURE;
	STDMETHOD(RemoveSource) (THIS_ struct IDirectSoundSource*) PURE;
	STDMETHOD(SetMasterClock) (THIS_ IReferenceClock*) PURE;
	STDMETHOD(CreateSoundBuffer) (THIS_ const LPDSBUFFERDESC pcDSBufferDesc, LPDWORD, DWORD, REFGUID rguid, IDirectSoundBuffer** ppDSBuffer) PURE;
	STDMETHOD(CreateSoundBufferFromConfig) (THIS_ IUnknown*, IDirectSoundBuffer**) PURE;
	STDMETHOD(GetSoundBuffer) (THIS_ DWORD, IDirectSoundBuffer**) PURE;
	STDMETHOD(GetBusCount) (THIS_ LPDWORD) PURE;
	STDMETHOD(GetBusIDs) (THIS_ LPDWORD, LPDWORD, DWORD) PURE;
	STDMETHOD(GetFunctionalID) (THIS_ DWORD, LPDWORD) PURE;
	STDMETHOD(GetSoundBufferBusIDs) (THIS_ IDirectSoundBuffer*, LPDWORD, LPDWORD, LPDWORD) PURE;
};

#undef INTERFACE
#define INTERFACE IDirectSoundSinkSync
DECLARE_INTERFACE_(IDirectSoundSinkSync, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    // IDirectSoundSinkSync? methods
	STDMETHOD(GetLatencyClock)(IReferenceClock** ppReferenceClock) PURE;
	STDMETHOD(Activate)(BOOL fEnable) PURE;
	STDMETHOD(SampleToRefTime)(LONGLONG llSampleTime, REFERENCE_TIME *prfTime) PURE;
	STDMETHOD(RefTimeToSample)(REFERENCE_TIME rfTime, LONGLONG *pllSampleTime) PURE;
	STDMETHOD(GetFormat)(LPWAVEFORMATEX lpwfxFormat, DWORD dwSizeAllocated, LPDWORD lpdwSizeWritten) PURE;
};

#undef INTERFACE
#define INTERFACE IDirectSoundSource
DECLARE_INTERFACE_(IDirectSoundSource, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    // IDirectSoundSource? methods
    STDMETHOD(SetSink)(IDirectSoundSink* pSink) PURE;
    STDMETHOD(GetFormat)(LPWAVEFORMATEX lpwfxFormat, DWORD dwSizeAllocated, LPDWORD lpdwSizeWritten) PURE;
    STDMETHOD(Seek)(ULONGLONG) PURE;
    STDMETHOD(Read)(LPVOID*,LPDWORD,LPDWORD,LPLONG,LPDWORD,DWORD,ULONGLONG*) PURE;
    STDMETHOD(ClearLoop)(DWORD) PURE;
    STDMETHOD(SetLoop)(DWORD,DWORD) PURE;
};

struct KSID { GUID Set; ULONG Id; ULONG Flags; };

#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    // IKsControl methods
    STDMETHOD(KsProperty) (THIS_ KSID* Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned) PURE;
    STDMETHOD(KsMethod) (THIS_ KSID* Method, ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned) PURE;
    STDMETHOD(KsEvent) (THIS_ KSID* Event, ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned) PURE;
};

// this undocumented abomination is needed for certain games (e.g. "I wanna be the GB")
// to bridge emulated directsound and real directmusic (when using VIRTUALDIRECTSOUND).
// it's not fully implemented and I'm not sure what everything in here is supposed to do.
// TODO: fully emulate directmusic instead? (maybe refer to wine there, dmime)
struct EmulatedDirectSoundSink : public IDirectSoundSink, public IDirectSoundSinkSync, public IKsControl
{
public:
	EmulatedDirectSoundSink(LPWAVEFORMATEX lpWaveFormat)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		
		refcount = 0;
		active = FALSE;
		pRefClock = NULL;
		nextBusID = 1;
		busCount = 0;

		int wfxsize = sizeof(WAVEFORMATEX);
		if(lpWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
			wfxsize += lpWaveFormat->cbSize;
		waveFormatAllocated = wfxsize;
		sinkWaveFormat = (WAVEFORMATEX*)malloc(wfxsize);
		memcpy(sinkWaveFormat, lpWaveFormat, wfxsize);
	}
	~EmulatedDirectSoundSink()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
	}

	// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
		if(!ppvObj)
			return E_POINTER;
		/*if(riid == IID_IDirectSoundSink)
			*ppvObj = (IDirectSoundSink*)this;
		else*/ if(riid == IID_IDirectSoundSinkSync)
			*ppvObj = (IDirectSoundSinkSync*)this;
		else if(riid == IID_IKsControl)
			*ppvObj = (IKsControl*)this;
		else if(riid == IID_IUnknown)
			*ppvObj = (IUnknown*)(IDirectSoundSink*)this;
		else
		{
			debugprintf(__FUNCTION__ " for unknown riid: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n", riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		return ++refcount;
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		ULONG count = --refcount;
		if(0 == count)
			delete this;
		return count;
	}

	std::vector<IDirectSoundSource*> sources;

	// IDirectSoundSink? methods
	STDMETHOD(AddSource) (IDirectSoundSource* pDSS)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, 0x%X) called.\n", this, pDSS);
		pDSS->SetSink(this);
		sources.push_back(pDSS);
		return DS_OK;
	}
	STDMETHOD(RemoveSource) (IDirectSoundSource* pDSS)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, 0x%X) called.\n", this, pDSS);
		sources.erase(std::remove(sources.begin(), sources.end(), pDSS), sources.end());
		return DS_OK;
	}
	STDMETHOD(SetMasterClock) (IReferenceClock* pReferenceClock)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, 0x%X) called.\n", this, pReferenceClock);
		if(pReferenceClock)
			HookCOMInterface(IID_IReferenceClock, (LPVOID*)&pReferenceClock);
		pRefClock = pReferenceClock;
		return DS_OK;
	}
	STDMETHOD(CreateSoundBuffer) (const LPDSBUFFERDESC pcDSBufferDesc, LPDWORD lpdwNotSure, DWORD dwNumSlotsOrSomething, REFGUID rguid, IDirectSoundBuffer** ppDSBuffer)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");

		if(!pcDSBufferDesc || !ppDSBuffer)
			return DSERR_INVALIDPARAM;

		debuglog(LCF_DSOUND, "flags=0x%X, bytes=0x%X\n", pcDSBufferDesc->dwFlags, pcDSBufferDesc->dwBufferBytes);

		if(!(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER))
		{
			*ppDSBuffer = (LPDIRECTSOUNDBUFFER)(new EmulatedDirectSoundBuffer());
		}
		else
		{
			if(pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
				debuglog(LCF_DSOUND, "creating s_primaryBuffer flags=0x%X\n", pcDSBufferDesc->dwFlags);
			// pretend to have a primary buffer
			*ppDSBuffer = (LPDIRECTSOUNDBUFFER)(new EmulatedDirectSoundBuffer(true));
		}
		(*ppDSBuffer)->Initialize(/*pEmulatedDirectSound*/NULL, pcDSBufferDesc);

		busToBuf[nextBusID] = *ppDSBuffer;
		bufToBus[*ppDSBuffer] = nextBusID;
		busToSlots[nextBusID] = dwNumSlotsOrSomething;
		nextBusID += (dwNumSlotsOrSomething>0) ? dwNumSlotsOrSomething : 1;
		busCount++;

		// TODO: use lpdwNotSure and rguid for somethingorother?

		return DS_OK;
	}
	STDMETHOD(CreateSoundBufferFromConfig) (IUnknown*, IDirectSoundBuffer** ppDSBuffer)
	{
		debuglog(LCF_DSOUND|LCF_TODO|LCF_ERROR, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI. but search for "DirectSound Buffer Configuration Form" if it's needed.
		return DSERR_UNSUPPORTED;
	}
	STDMETHOD(GetSoundBuffer) (DWORD dwBus, IDirectSoundBuffer** ppDSBuffer)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(!ppDSBuffer)
			return E_POINTER;
		*ppDSBuffer = busToBuf[dwBus];
		return DS_OK;
	}
	STDMETHOD(GetBusCount) (LPDWORD pBusOut)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(!pBusOut)
			return E_POINTER;
		*pBusOut = busCount;
		return DS_OK;
	}
	STDMETHOD(GetBusIDs) (LPDWORD a, LPDWORD b, DWORD c)
	{
		debuglog(LCF_DSOUND|LCF_TODO|LCF_ERROR, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI
		if(a)
			*a = 1;
		if(b)
			*b = 1;
		return DS_OK;
	}
	STDMETHOD(GetFunctionalID) (DWORD a, LPDWORD b)
	{
		debuglog(LCF_DSOUND|LCF_TODO|LCF_ERROR, __FUNCTION__ "(0x%X) called.\n", this);
		if(!b)
			return E_POINTER;
		// NYI
		*b = 1;
		return DS_OK;
	}
	STDMETHOD(GetSoundBufferBusIDs) (IDirectSoundBuffer* lpDSB, LPDWORD a, LPDWORD b, LPDWORD c)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, lpDSB=0x%X, a=0x%X, *a=0x%X, b=0x%X, *b=0x%X, c=0x%X, *c=0x%X) called.\n", this, lpDSB, a,a?*a:0, b,b?*b:0, c,c?*c:0);
		DWORD busID = bufToBus[lpDSB];
		if(a)
			*a = busID;
		if(b)
			*b = 0; // NYI (I've never seen b be nonzero)
		if(c)
			*c = busToSlots[nextBusID];
		return DS_OK;
	}

	// IDirectSoundSinkSync? methods
	STDMETHOD(GetLatencyClock)(IReferenceClock** ppReferenceClock)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(!ppReferenceClock)
		{
			debuglog(LCF_DSOUND|LCF_ERROR, __FUNCTION__ "returned LCF_DSOUND.\n");
			return E_POINTER;
		}
		debuglog(LCF_DSOUND, __FUNCTION__ " returned pRefClock=0x%X.\n", pRefClock);
		*ppReferenceClock = pRefClock;
		return DS_OK;
	}
	STDMETHOD(Activate)(BOOL fEnable)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X, fEnable=%d) called.\n", this, fEnable);
		active = fEnable;
		// NYI
		return DS_OK;
	}
	STDMETHOD(SampleToRefTime)(LONGLONG llSampleTime, REFERENCE_TIME *prfTime)
	{
		debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI
		if(!prfTime)
			return E_POINTER;
		*prfTime = llSampleTime;
		return DS_OK;
	}
	STDMETHOD(RefTimeToSample)(REFERENCE_TIME rfTime, LONGLONG *pllSampleTime)
	{
		debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI
		if(!pllSampleTime)
			return E_POINTER;
		*pllSampleTime = rfTime;
		return DS_OK;
	}
	STDMETHOD(GetFormat)(LPWAVEFORMATEX lpwfxFormat, DWORD dwSizeAllocated, LPDWORD lpdwSizeWritten)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		DWORD size = waveFormatAllocated;
		if(lpwfxFormat)
		{
			if(size > dwSizeAllocated)
				size = dwSizeAllocated;
			memcpy(lpwfxFormat, sinkWaveFormat, size);
		}
		if(lpdwSizeWritten)
			*lpdwSizeWritten = size;
		return DS_OK;
	}

	// IKsControl methods

    STDMETHOD(KsProperty) (KSID* Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned)
	{
		debuglog(LCF_DSOUND|LCF_TODO|LCF_ERROR, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI (I've seen IKsControl queried but never seen this function called.)
		return -1;
	}
    STDMETHOD(KsMethod) (KSID* Method, ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned)
	{
		return KsEvent(Method, MethodLength, MethodData, DataLength, BytesReturned);
	}
    STDMETHOD(KsEvent) (KSID* Event, ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned)
	{
		debuglog(LCF_DSOUND|LCF_TODO|LCF_ERROR, __FUNCTION__ "(0x%X) called.\n", this);
		// NYI (I've seen IKsControl queried but never seen this function called.)
		return -1;
	}

	LPWAVEFORMATEX sinkWaveFormat;
	DWORD waveFormatAllocated;
	BOOL active;

	DWORD nextBusID;
	std::map<DWORD,IDirectSoundBuffer*> busToBuf;
	std::map<IDirectSoundBuffer*,DWORD> bufToBus;
	std::map<DWORD,DWORD> busToSlots;
	DWORD busCount;

	IReferenceClock* pRefClock;

	DWORD refcount;
};





struct EmulatedDirectSoundSinkFactory* s_pEmulatedDirectSoundSinkFactory = NULL; // for now, we only need 1 of these

struct EmulatedDirectSoundSinkFactory : public IDirectSoundSinkFactory
{
public:
	EmulatedDirectSoundSinkFactory()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		s_pEmulatedDirectSoundSinkFactory = this;
		refcount = 0;
	}
	~EmulatedDirectSoundSinkFactory()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(s_pEmulatedDirectSoundSinkFactory == this)
			s_pEmulatedDirectSoundSinkFactory = NULL;
	}

	// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
		if(!ppvObj)
			return E_POINTER;
		if(riid == IID_IDirectSoundSinkFactory)
			*ppvObj = (IDirectSoundSinkFactory*)this;
		else if(riid == IID_IUnknown)
			*ppvObj = (IUnknown*)this;
		else
		{
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		return ++refcount;
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		ULONG count = --refcount;
		if(0 == count)
			delete this;
		return count;
	}

	// IDirectSoundSinkFactory? methods
	HRESULT STDMETHODCALLTYPE CreateSoundSink(LPWAVEFORMATEX lpWaveFormat, IDirectSoundSink** ppDSC)
	{
		debuglog(LCF_DSOUND, __FUNCTION__ "(0x%X) called.\n", this);
		if(!ppDSC)
			return E_POINTER;
		*ppDSC = (IDirectSoundSink*)(new EmulatedDirectSoundSink(lpWaveFormat));
		(*ppDSC)->AddRef();
		return S_OK;
	}

	DWORD refcount;
};


// disabled because it's only used for debugging
//struct MyDirectSoundSink
//{
//	static BOOL Hook(IDirectSoundSink* obj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", obj);
//		BOOL rv = FALSE;
//		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
//		rv |= VTHOOKFUNC(IDirectSoundSink, GetSoundBufferBusIDs);
//		rv |= VTHOOKFUNC(IDirectSoundSink, CreateSoundBuffer);
//		return rv;
//	}
//	
//	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectSoundSink* pThis, REFIID riid, void** ppvObj);
//	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirectSoundSink* pThis, REFIID riid, void** ppvObj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
//		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
//		if(SUCCEEDED(rv))
//			HookCOMInterface(riid, ppvObj);
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *GetSoundBufferBusIDs)(IDirectSoundSink* pThis, IDirectSoundBuffer* lpDSB, LPDWORD a, LPDWORD b, LPDWORD c);
//	static HRESULT STDMETHODCALLTYPE MyGetSoundBufferBusIDs(IDirectSoundSink* pThis, IDirectSoundBuffer* lpDSB, LPDWORD a, LPDWORD b, LPDWORD c)
//	{
//		debugprintf(__FUNCTION__ "(0x%X, lpDSB=0x%X, a=0x%X, *a=0x%X, b=0x%X, *b=0x%X, c=0x%X, *c=0x%X) called.\n", pThis, lpDSB, a,a?*a:0,b,b?*b:0, c,c?*c:0);
//		HRESULT rv = GetSoundBufferBusIDs(pThis, lpDSB, a, b, c);
//		debugprintf(__FUNCTION__ "(0x%X, lpDSB=0x%X, a=0x%X, *a=0x%X, b=0x%X, *b=0x%X, c=0x%X, *c=0x%X) called.\n", pThis, lpDSB, a,a?*a:0,b,b?*b:0, c,c?*c:0);
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *CreateSoundBuffer)(IDirectSoundSink* pThis, const LPDSBUFFERDESC pcDSBufferDesc, LPDWORD lpdwNotSure, DWORD dwNumSlotsOrSomething, REFGUID rguid, IDirectSoundBuffer** ppDSBuffer);
//	static HRESULT STDMETHODCALLTYPE MyCreateSoundBuffer(IDirectSoundSink* pThis, const LPDSBUFFERDESC pcDSBufferDesc, LPDWORD lpdwNotSure, DWORD dwNumSlotsOrSomething, REFGUID rguid, IDirectSoundBuffer** ppDSBuffer)
//	{
//		debugprintf(__FUNCTION__ "(0x%X, pcDSBufferDesc=0x%X, lpdwNotSure=0x%X, *lpdwNotSure=0x%X, dwNumSlotsOrSomething=0x%X) called.\n", pThis, pcDSBufferDesc, lpdwNotSure,lpdwNotSure?*lpdwNotSure:0,dwNumSlotsOrSomething);
//		HRESULT rv = CreateSoundBuffer(pThis, pcDSBufferDesc, lpdwNotSure, dwNumSlotsOrSomething, rguid, ppDSBuffer);
//		debugprintf(__FUNCTION__ "(0x%X, pcDSBufferDesc=0x%X, lpdwNotSure=0x%X, *lpdwNotSure=0x%X, dwNumSlotsOrSomething=0x%X) called.\n", pThis, pcDSBufferDesc, lpdwNotSure,lpdwNotSure?*lpdwNotSure:0,dwNumSlotsOrSomething);
//		return rv;
//	}
//
//};
//HRESULT (STDMETHODCALLTYPE* MyDirectSoundSink::QueryInterface)(IDirectSoundSink* pThis, REFIID riid, void** ppvObj) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirectSoundSink::GetSoundBufferBusIDs)(IDirectSoundSink* pThis, IDirectSoundBuffer* lpDSB, LPDWORD a, LPDWORD b, LPDWORD c) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirectSoundSink::CreateSoundBuffer)(IDirectSoundSink* pThis, const LPDSBUFFERDESC pcDSBufferDesc, LPDWORD lpdwNotSure, DWORD dwNumSlotsOrSomething, REFGUID rguid, IDirectSoundBuffer** ppDSBuffer) = 0;
//
//struct MyDirectSoundSinkFactory
//{
//	static BOOL Hook(IDirectSoundSinkFactory* obj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", obj);
//		BOOL rv = FALSE;
//		rv |= VTHOOKFUNC(IDirectSoundSinkFactory, CreateSoundSink);
//		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectSoundSinkFactory* pThis, REFIID riid, void** ppvObj);
//	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirectSoundSinkFactory* pThis, REFIID riid, void** ppvObj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
//		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
//		if(SUCCEEDED(rv))
//			HookCOMInterface(riid, ppvObj);
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *CreateSoundSink)(IDirectSoundSinkFactory* pThis, LPWAVEFORMATEX lpWaveFormat, IDirectSoundSink** ppDSC);
//	static HRESULT STDMETHODCALLTYPE MyCreateSoundSink(IDirectSoundSinkFactory* pThis, LPWAVEFORMATEX lpWaveFormat, IDirectSoundSink** ppDSC)
//	{
//		debuglog(LCF_DSOUND|LCF_TODO|LCF_UNTESTED,__FUNCTION__ "(0x%X) called.\n", pThis);
//		cmdprintf("SHORTTRACE: 3,50");
//		HRESULT rv = CreateSoundSink(pThis, lpWaveFormat, ppDSC);
//		if(SUCCEEDED(rv))
//			MyDirectSoundSink::Hook(*ppDSC);
//		return rv;
//	}
//};
//HRESULT (STDMETHODCALLTYPE* MyDirectSoundSinkFactory::QueryInterface)(IDirectSoundSinkFactory* pThis, REFIID riid, void** ppvObj) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirectSoundSinkFactory::CreateSoundSink)(IDirectSoundSinkFactory* pThis, LPWAVEFORMATEX lpWaveFormat, IDirectSoundSink** ppDSC) = 0;





HOOKFUNC MMRESULT WINAPI MywaveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
{
	// TODO: audio capture for AVI when DirectSound isn't used...
	debuglog(LCF_WSOUND|LCF_TODO, __FUNCTION__ " called.\n");
	MMRESULT rv = waveOutWrite(hwo, pwh, cbwh);
	return rv;
}

HOOKFUNC BOOL WINAPI MyBeep(DWORD dwFreq, DWORD dwDuration)
{
	//BOOL rv = Beep(dwFreq, dwDuration;
	debuglog(LCF_WSOUND, __FUNCTION__ " called (and suppressed).\n");
	BOOL rv = TRUE; // no beeping allowed
	return rv;
}
HOOKFUNC BOOL WINAPI MyMessageBeep(UINT uType)
{
	//BOOL rv = MessageBeep(uType;
	debuglog(LCF_WSOUND, __FUNCTION__ " called (and suppressed).\n");
	BOOL rv = TRUE; // no beeping allowed
	return rv;
}
HOOKFUNC BOOL WINAPI MyPlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
	//BOOL rv = PlaySoundA(pszSound, hmod, fdwSound);
	debuglog(LCF_WSOUND|LCF_TODO, __FUNCTION__ " called (and suppressed).\n");
	BOOL rv = TRUE;
	return rv;
}
HOOKFUNC BOOL WINAPI MyPlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
	//BOOL rv = PlaySoundW(pszSound, hmod, fdwSound);
	debuglog(LCF_WSOUND|LCF_TODO, __FUNCTION__ " called (and suppressed).\n");
	BOOL rv = TRUE;
	return rv;
}

static DWORD waveOutBaseTime = 0;
static WAVEFORMATEX waveOutWaveFormat = {};

HOOKFUNC MMRESULT WINAPI MywaveOutReset(HWAVEOUT hwo)
{
	MMRESULT rv = waveOutReset(hwo);
	waveOutBaseTime = detTimer.GetTicks();
	return rv;
}
HOOKFUNC MMRESULT WINAPI MywaveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	MMRESULT rv = waveOutOpen(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
	waveOutBaseTime = detTimer.GetTicks();
	if(pwfx)
		waveOutWaveFormat = *pwfx;
	return rv;
}
HOOKFUNC MMRESULT WINAPI MywaveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt)
{
	// FIXME
	MMRESULT rv = waveOutGetPosition(hwo, pmmt, cbmmt);
	debuglog(LCF_WSOUND/*|LCF_UNTESTED*/, "MyWaveOutGetPosition, type=0x%X, value=0x%X\n", pmmt->wType, pmmt->u.cb);

	DWORD timeDiff = detTimer.GetTicks(TIMETYPE_GETTICKCOUNT) - waveOutBaseTime;
	pmmt->wType = TIME_BYTES;
	pmmt->u.cb = (timeDiff * waveOutWaveFormat.nSamplesPerSec / 1000) * waveOutWaveFormat.nBlockAlign;

	return rv;
}


HOOKFUNC MCIERROR WINAPI MymciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	// MymciSendCommandA seems to call MymciSendCommandW at least once internally so maybe this isn't necessary,
	// but I'll keep it in case some other OS version skips that step.

	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++; // helps Eternal Daughter sync (especially when fast-forwarding with sleep skip on) and allows us to help prevent Iji deadlock in MyWaitForMultipleObjects when threads are enabled

	MCIERROR rv = mciSendCommandA(mciId, uMsg, dwParam1, dwParam2);

	curtls.callerisuntrusted--;
	return rv;
}

HOOKFUNC MCIERROR WINAPI MymciSendCommandW(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;

	const char* oldName = tls.curThreadCreateName;
	if(!oldName) // in this case, only suggest a name if the caller hasn't already
		tls.curThreadCreateName = "MediaControl";

	MCIERROR rv = mciSendCommandW(mciId, uMsg, dwParam1, dwParam2);

	if(!oldName)
		tls.curThreadCreateName = oldName;

	curtls.callerisuntrusted--;
	return rv;
}

HOOKFUNC HRESULT WINAPI MyDirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
{
	debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
	
//			if(ppDS)
//			{
//				*ppDS = new MyDirectSound<IDirectSound>(NULL);
//				return DS_OK;
//			}

	//if(MyDirectSound<IDirectSound>::m_freeDS)
	//{
	//	*ppDS = MyDirectSound<IDirectSound>::m_freeDS;
	//	MyDirectSound<IDirectSound>::m_freeDS = NULL;
	//	return S_OK;
	//}
	//else
	{
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;

		const char* oldName = curtls.curThreadCreateName;
		curtls.curThreadCreateName = "DirectSound";
		HRESULT rv = E_FAIL;
		if(!(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND) && !usingVirtualDirectSound)
		{
			rv = DirectSoundCreate(pcGuidDevice, ppDS, pUnkOuter);

			if(SUCCEEDED(rv))
				HookCOMInterface(IID_IDirectSound, (LPVOID*)ppDS);
		}
		if(FAILED(rv))
		{
			if(!(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND))
				debuglog(LCF_DSOUND|LCF_ERROR, "DirectSoundCreate FAILED%s.\n", tasflags.threadMode?", all on its own":", because multithreading is disabled");
			// let's give it a dummy directsound for sync purposes
			if(ppDS)
			{
				usingVirtualDirectSound = true;
				*ppDS = new MyDirectSound<IDirectSound>(NULL);
				rv = DS_OK;
			}
		}
		curtls.callerisuntrusted--;
		curtls.curThreadCreateName = oldName;
		return rv;
	}
}
HOOKFUNC HRESULT WINAPI MyDirectSoundCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS, LPUNKNOWN pUnkOuter)
{
	debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");

//			if(ppDS)
//			{
//				*ppDS = new MyDirectSound<IDirectSound8>(NULL);
//				return DS_OK;
//			}

	//if(MyDirectSound<IDirectSound8>::m_freeDS)
	//{
	//	*ppDS = MyDirectSound<IDirectSound8>::m_freeDS;
	//	MyDirectSound<IDirectSound8>::m_freeDS = NULL;
	//	return S_OK;
	//}
	//else
	{
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;

		const char* oldName = curtls.curThreadCreateName;
		curtls.curThreadCreateName = "DirectSound8";
		HRESULT rv = E_FAIL;
		if(!(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND) && !usingVirtualDirectSound)
		{
			rv = DirectSoundCreate8(pcGuidDevice, ppDS, pUnkOuter);

			if(SUCCEEDED(rv))
				HookCOMInterface(IID_IDirectSound8, (LPVOID*)ppDS);
		}
		if(FAILED(rv))
		{
			if(!(tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND))
				debuglog(LCF_DSOUND|LCF_ERROR, "DirectSoundCreate8 FAILED%s.\n", tasflags.threadMode?", all on its own":", because multithreading is disabled");
			// let's give it a dummy directsound for sync purposes
			if(ppDS)
			{
				usingVirtualDirectSound = true;
				*ppDS = new MyDirectSound<IDirectSound8>(NULL);
				rv = DS_OK;
			}
		}

		curtls.curThreadCreateName = oldName;
		curtls.callerisuntrusted--;
		return rv;
	}
}

//LPDSENUMCALLBACKA gameDSEnumCallbackA;
//BOOL CALLBACK MyDSEnumCallbackA(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
//{
//	BOOL rv = gameDSEnumCallbackA(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
//	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ "(0x%X, \"%s\", \"%s\", 0x%X) called. returned %d\n", lpGuid?lpGuid->Data1:0, lpcstrDescription, lpcstrModule, lpContext, rv);
//	return rv;
//}
//LPDSENUMCALLBACKW gameDSEnumCallbackW;
//BOOL CALLBACK MyDSEnumCallbackW(LPGUID lpGuid, LPCWSTR lpcstrDescription, LPCWSTR lpcstrModule, LPVOID lpContext)
//{
//	BOOL rv = gameDSEnumCallbackW(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
//	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ "(0x%X, \"%S\", \"%S\", 0x%X) called. returned %d\n", lpGuid?lpGuid->Data1:0, lpcstrDescription, lpcstrModule, lpContext, rv);
//	return rv;
//}

HOOKFUNC HRESULT WINAPI MyDirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext)
{
	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
	//HRESULT rv = DirectSoundEnumerateA(pDSEnumCallback, pContext);
	//gameDSEnumCallbackA = pDSEnumCallback;
	//HRESULT rv = DirectSoundEnumerateA(MyDSEnumCallbackA, pContext);
	//return rv;
	if(!pDSEnumCallback)
		return DSERR_INVALIDPARAM;
	pDSEnumCallback(0, "Primary Sound Driver", "", pContext);
	return DS_OK;
}
HOOKFUNC HRESULT WINAPI MyDirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext)
{
	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
	//HRESULT rv = DirectSoundEnumerateW(pDSEnumCallback, pContext);
	//gameDSEnumCallbackW = pDSEnumCallback;
	//HRESULT rv = DirectSoundEnumerateW(MyDSEnumCallbackW, pContext);
	//return rv;
	if(!pDSEnumCallback)
		return DSERR_INVALIDPARAM;
	pDSEnumCallback(0, L"Primary Sound Driver", L"", pContext);
	return DS_OK;
}
//HOOKFUNC HRESULT WINAPI MyDirectSoundCaptureCreate(LPCGUID pcGuidDevice, struct IDirectSoundCapture * *ppDSC, LPUNKNOWN pUnkOuter)
//{
//	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
//	HRESULT rv = DirectSoundCaptureCreate(pcGuidDevice, ppDSC, pUnkOuter);
//	return rv;
//}
HOOKFUNC HRESULT WINAPI MyDirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext)
{
	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
	//HRESULT rv = DirectSoundCaptureEnumerateA(pDSEnumCallback, pContext);
	//return rv;
	return DS_OK;
}
HOOKFUNC HRESULT WINAPI MyDirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext)
{
	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
	//HRESULT rv = DirectSoundCaptureEnumerateW(pDSEnumCallback, pContext);
	//return rv;
	return DS_OK;
}
//HOOKFUNC HRESULT WINAPI MyDirectSoundCaptureCreate8(LPCGUID pcGuidDevice, struct IDirectSoundCapture8 * *ppDSC8, LPUNKNOWN pUnkOuter)
//{
//	debuglog(LCF_DSOUND|LCF_TODO, __FUNCTION__ " called.\n");
//	HRESULT rv = DirectSoundCaptureCreate8(pcGuidDevice, ppDSC8, pUnkOuter);
//	return rv;
//}








void AdvanceTimeAndMixAll(DWORD ticks)
{
	EmulatedDirectSoundBuffer::AdvanceTimeAndMixAll(ticks);
}
void PreSuspendSound()
{
	EmulatedDirectSoundBuffer::PreSuspend();
}
void PostResumeSound()
{
	EmulatedDirectSoundBuffer::PostResume();
}
void ForceAlignSound(bool earlier)
{
	EmulatedDirectSoundBuffer::ForceAlign(earlier);
}
void BackDoorStopAll()
{
	MyDirectSoundBuffer<IDirectSoundBuffer>::BackDoorStopAll();
	MyDirectSoundBuffer<IDirectSoundBuffer8>::BackDoorStopAll();
}

void StopAllSounds()
{
	debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
	// this is really tricky to make reliable...
	// PROBLEM:
	// - for some reason, if any directsound sound/music is active, then loading a savestate
	//   will cause a freeze/deadlock the next time one of the interrupted sounds plays again
	// SOLUTION:
	// - stop all directsound sounds before making a savestate
	// PROBLEM with that solution:
	// - another thread is feeding sounds in while we're trying to stop them, causing freezes/crashes
	// SOLUTION for that problem with that solution:
	// - suspend all the threads except this one before stopping the sounds
	// PROBLEM with the solution for that problem with that solution:
	// - the other threads might have a lock on the sound buffers, so we can't just suspend them at any time
	// SOLUTION for that problem with the solution for that problem with that solution:
	// - disable all directsound buffer calls and lock all the sound buffers ourselves through a backdoor function

	lockdown = true; // this prevents new sounds from playing while we're stopping them.
	EnterCriticalSection(&s_soundBufferListCS);

	//if(tasflags.emuMode & EMUMODE_EMULATESOUND)
	//{
	//	if(!pauseHandlerSuspendedSound)
	//		EmulatedDirectSoundBuffer::PreSuspend();
	//	//pauseHandlerSuspendedSound = false;
	//}
	//else
	//if(!pauseHandlerSuspendedSound)
	{
		// loop through all sounds and lock their buffers
		// (other threads must NOT be suspended when this is called!)
		MyDirectSoundBuffer<IDirectSoundBuffer>::BackDoorLockAll();
		MyDirectSoundBuffer<IDirectSoundBuffer8>::BackDoorLockAll();

		MyDirectSoundBuffer<IDirectSoundBuffer>::BackDoorStopAll();
		MyDirectSoundBuffer<IDirectSoundBuffer8>::BackDoorStopAll();
	}

	cmdprintf("SUSPENDALL: "); // suspend all other threads in this process
	lockdown = false; // now it's safe to reenable DirectSoundBuffer operations
}

// must call this once after StopAllSounds()
void ResumePlayingSounds()
{
	debuglog(LCF_DSOUND, __FUNCTION__ " called.\n");
	lockdown = true;
	cmdprintf("RESUMEALL: ");

	//if(tasflags.emuMode & EMUMODE_EMULATESOUND)
	//{
	//	if(!pauseHandlerSuspendedSound)
	//	{
	//		EmulatedDirectSoundBuffer::PostResume();
	//	}
	//	//pauseHandlerSuspendedSound = false;
	//}
	//else
	//if(!pauseHandlerSuspendedSound)
	{
		MyDirectSoundBuffer<IDirectSoundBuffer>::BackDoorLockAll();
		MyDirectSoundBuffer<IDirectSoundBuffer8>::BackDoorLockAll();

		// loop through and restore some of their state
		MyDirectSoundBuffer<IDirectSoundBuffer>::BackDoorRestoreAll();
		MyDirectSoundBuffer<IDirectSoundBuffer8>::BackDoorRestoreAll();
	}
	LeaveCriticalSection(&s_soundBufferListCS);
	lockdown = false;
}

void DoFrameBoundarySoundChecks()
{
	if(pauseHandlerSuspendedSound)
	{
		PostResumeSound();
		pauseHandlerSuspendedSound = false;
	}

	static bool noPlayBuffersOn = false;
	bool noPlayBuffersOnNow = (tasflags.emuMode & EMUMODE_NOPLAYBUFFERS) != 0;
	if(noPlayBuffersOn != noPlayBuffersOnNow)
	{
		noPlayBuffersOn = noPlayBuffersOnNow;
		EmulatedDirectSoundBuffer::DisableMixingBufferOutput(noPlayBuffersOnNow);
	}


	if(MyDirectSound<IDirectSound>::m_freeDS)
	{
		// disabled as a temporary hack fix for games like Eternal Daughter still freezing
		//MyDirectSound<IDirectSound>::m_freeDS->Release();
		MyDirectSound<IDirectSound>::m_freeDS = NULL;
	}
	if(MyDirectSound<IDirectSound8>::m_freeDS)
	{
		MyDirectSound<IDirectSound8>::m_freeDS->Release();
		MyDirectSound<IDirectSound8>::m_freeDS = NULL;
	}
}

void SoundDllMainInit()
{
	InitializeCriticalSection(&s_soundBufferListCS);
	InitializeCriticalSection(&s_myMixingOutputBufferCS);
}

bool TrySoundCoCreateInstance(REFIID riid, LPVOID *ppv)
{
	bool applicable = ((tasflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND) || usingVirtualDirectSound);
	if(applicable && riid == IID_IDirectSound)
	{
		*ppv = (IDirectSound*)(new MyDirectSound<IDirectSound>(NULL));
		return true;
	}
	else if(applicable && riid == IID_IDirectSound8)
	{
		*ppv = (IDirectSound8*)(new MyDirectSound<IDirectSound8>(NULL));
		return true;
	}
	return false;
}

bool HookCOMInterfaceSound(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		HOOKRIID(DirectSound,);
		HOOKRIID(DirectSound,8);

		HOOKRIID(DirectSoundBuffer,);
		HOOKRIID(DirectSoundBuffer,8);

		VTHOOKRIID(DirectMusic,);
		VTHOOKRIID2(DirectMusic,2,);
		VTHOOKRIID(DirectMusic,8);
		VTHOOKRIID(DirectMusicPerformance,);
		VTHOOKRIID2(DirectMusicPerformance,2,);
		VTHOOKRIID(DirectMusicPerformance,8);

		VTHOOKRIID3(IBaseFilter,MyBaseFilter);
		VTHOOKRIID3(IMediaFilter,MyMediaFilter);
		//VTHOOKRIID3(IAMGraphStreams,MyAMGraphStreams);
		//VTHOOKRIID3(IAMOpenProgress,MyAMOpenProgress);

//		VTHOOKRIID3(IDirectSoundSinkFactory,MyDirectSoundSinkFactory); // disabled because it's only used for debugging

		default: return false;
	}
	return true;
}

void ApplySoundIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, DSOUND, DirectSoundCreate),
		MAKE_INTERCEPT(1, DSOUND, DirectSoundCreate8),
		MAKE_INTERCEPT(1, WINMM, waveOutWrite),
		MAKE_INTERCEPT(1, WINMM, waveOutGetPosition),
		MAKE_INTERCEPT(1, WINMM, waveOutReset),
		MAKE_INTERCEPT(1, WINMM, waveOutOpen),
		MAKE_INTERCEPT(1, KERNEL32, Beep),
		MAKE_INTERCEPT(1, USER32, MessageBeep),
		MAKE_INTERCEPT(1, WINMM, PlaySoundA),
		MAKE_INTERCEPT(1, WINMM, PlaySoundW),
		MAKE_INTERCEPT(1, WINMM, mciSendCommandA),
		MAKE_INTERCEPT(1, WINMM, mciSendCommandW),
		MAKE_INTERCEPT(1, DSOUND, DirectSoundEnumerateA),
		MAKE_INTERCEPT(1, DSOUND, DirectSoundEnumerateW),
		//MAKE_INTERCEPT(1, DSOUND, DirectSoundCaptureCreate),
		MAKE_INTERCEPT(1, DSOUND, DirectSoundCaptureEnumerateA),
		MAKE_INTERCEPT(1, DSOUND, DirectSoundCaptureEnumerateW),
		//MAKE_INTERCEPT(1, DSOUND, DirectSoundCaptureCreate8),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
