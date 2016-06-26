/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define DirectSoundCreate TrampDirectSoundCreate
TRAMPFUNC HRESULT WINAPI DirectSoundCreate(LPCGUID pcGuidDevice, struct IDirectSound* *ppDS, LPUNKNOWN pUnkOuter);
#define DirectSoundCreate8 TrampDirectSoundCreate8
TRAMPFUNC HRESULT WINAPI DirectSoundCreate8(LPCGUID pcGuidDevice, struct IDirectSound8* *ppDS, LPUNKNOWN pUnkOuter);

typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID, LPCWSTR, LPCWSTR, LPVOID);

#define DirectSoundEnumerateA TrampDirectSoundEnumerateA
TRAMPFUNC HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
#define DirectSoundEnumerateW TrampDirectSoundEnumerateW
TRAMPFUNC HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);
//#define DirectSoundCaptureCreate TrampDirectSoundCaptureCreate
//TRAMPFUNC HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID pcGuidDevice, struct IDirectSoundCapture * *ppDSC, LPUNKNOWN pUnkOuter);
#define DirectSoundCaptureEnumerateA TrampDirectSoundCaptureEnumerateA
TRAMPFUNC HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
#define DirectSoundCaptureEnumerateW TrampDirectSoundCaptureEnumerateW
TRAMPFUNC HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);
//#define DirectSoundCaptureCreate8 TrampDirectSoundCaptureCreate8
//TRAMPFUNC HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID pcGuidDevice, struct IDirectSoundCapture8 * *ppDSC8, LPUNKNOWN pUnkOuter);

#define waveOutGetPosition TrampwaveOutGetPosition
TRAMPFUNC MMRESULT WINAPI waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt);
#define waveOutReset TrampwaveOutReset
TRAMPFUNC MMRESULT WINAPI waveOutReset(HWAVEOUT hwo);
#define waveOutOpen TrampwaveOutOpen
TRAMPFUNC MMRESULT WINAPI waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
#define waveOutWrite TrampwaveOutWrite
TRAMPFUNC MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);

#define Beep TrampBeep
TRAMPFUNC BOOL WINAPI Beep(DWORD dwFreq, DWORD dwDuration);
#define MessageBeep TrampMessageBeep
TRAMPFUNC BOOL WINAPI MessageBeep(UINT uType);
#define PlaySoundA TrampPlaySoundA
TRAMPFUNC BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
#define PlaySoundW TrampPlaySoundW
TRAMPFUNC BOOL WINAPI PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);

#define mciSendCommandA TrampmciSendCommandA
TRAMPFUNC MCIERROR WINAPI mciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#define mciSendCommandW TrampmciSendCommandW
TRAMPFUNC MCIERROR WINAPI mciSendCommandW(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

// hack for soundmixing
struct CachedVolumeAndPan
{
	DWORD leftVolumeAsScale; // out of 65536
	DWORD rightVolumeAsScale; // out of 65536
};
