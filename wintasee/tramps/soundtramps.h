/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef SOUNDTRAMPS_H_INCL
#define SOUNDTRAMPS_H_INCL

#define DirectSoundCreate TrampDirectSoundCreate
TRAMPFUNC HRESULT WINAPI DirectSoundCreate(LPCGUID pcGuidDevice, struct IDirectSound* *ppDS, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF
#define DirectSoundCreate8 TrampDirectSoundCreate8
TRAMPFUNC HRESULT WINAPI DirectSoundCreate8(LPCGUID pcGuidDevice, struct IDirectSound8* *ppDS, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF

typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID, LPCWSTR, LPCWSTR, LPVOID);

#define DirectSoundEnumerateA TrampDirectSoundEnumerateA
TRAMPFUNC HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext) TRAMPOLINE_DEF
#define DirectSoundEnumerateW TrampDirectSoundEnumerateW
TRAMPFUNC HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext) TRAMPOLINE_DEF
//#define DirectSoundCaptureCreate TrampDirectSoundCaptureCreate
//TRAMPFUNC HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID pcGuidDevice, struct IDirectSoundCapture * *ppDSC, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF
#define DirectSoundCaptureEnumerateA TrampDirectSoundCaptureEnumerateA
TRAMPFUNC HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext) TRAMPOLINE_DEF
#define DirectSoundCaptureEnumerateW TrampDirectSoundCaptureEnumerateW
TRAMPFUNC HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext) TRAMPOLINE_DEF
//#define DirectSoundCaptureCreate8 TrampDirectSoundCaptureCreate8
//TRAMPFUNC HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID pcGuidDevice, struct IDirectSoundCapture8 * *ppDSC8, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF

#define waveOutGetPosition TrampwaveOutGetPosition
TRAMPFUNC MMRESULT WINAPI waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt) TRAMPOLINE_DEF
#define waveOutReset TrampwaveOutReset
TRAMPFUNC MMRESULT WINAPI waveOutReset(HWAVEOUT hwo) TRAMPOLINE_DEF
#define waveOutOpen TrampwaveOutOpen
TRAMPFUNC MMRESULT WINAPI waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) TRAMPOLINE_DEF
#define waveOutWrite TrampwaveOutWrite
TRAMPFUNC MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) TRAMPOLINE_DEF

#define Beep TrampBeep
TRAMPFUNC BOOL WINAPI Beep(DWORD dwFreq, DWORD dwDuration) TRAMPOLINE_DEF
#define MessageBeep TrampMessageBeep
TRAMPFUNC BOOL WINAPI MessageBeep(UINT uType) TRAMPOLINE_DEF
#define PlaySoundA TrampPlaySoundA
TRAMPFUNC BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound) TRAMPOLINE_DEF
#define PlaySoundW TrampPlaySoundW
TRAMPFUNC BOOL WINAPI PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound) TRAMPOLINE_DEF

#define mciSendCommandA TrampmciSendCommandA
TRAMPFUNC MCIERROR WINAPI mciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) TRAMPOLINE_DEF
#define mciSendCommandW TrampmciSendCommandW
TRAMPFUNC MCIERROR WINAPI mciSendCommandW(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) TRAMPOLINE_DEF

// hack for soundmixing
struct CachedVolumeAndPan
{
	DWORD leftVolumeAsScale; // out of 65536
	DWORD rightVolumeAsScale; // out of 65536
};

#endif
