/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"
#include "shared/ipc.h"

namespace Hooks
{
    extern LastFrameSoundInfo lastFrameSoundInfo;

    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID pcGuidDevice, struct IDirectSound* *ppDS, LPUNKNOWN pUnkOuter);
    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID pcGuidDevice, struct IDirectSound8* *ppDS, LPUNKNOWN pUnkOuter);

    typedef BOOL(CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);
    typedef BOOL(CALLBACK *LPDSENUMCALLBACKW)(LPGUID, LPCWSTR, LPCWSTR, LPVOID);

    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundEnumerateA, LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundEnumerateW, LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);
    //HOOK_DECLARE(HRESULT WINAPI DirectSoundCaptureCreate(LPCGUID pcGuidDevice, struct IDirectSoundCapture * *ppDSC, LPUNKNOWN pUnkOuter);
    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundCaptureEnumerateA, LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
    HOOK_DECLARE(HRESULT, WINAPI, DirectSoundCaptureEnumerateW, LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);
    //HOOK_DECLARE(HRESULT WINAPI DirectSoundCaptureCreate8(LPCGUID pcGuidDevice, struct IDirectSoundCapture8 * *ppDSC8, LPUNKNOWN pUnkOuter);

    HOOK_DECLARE(MMRESULT, WINAPI, waveOutGetPosition, HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt);
    HOOK_DECLARE(MMRESULT, WINAPI, waveOutReset, HWAVEOUT hwo);
    HOOK_DECLARE(MMRESULT, WINAPI, waveOutOpen, LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
    HOOK_DECLARE(MMRESULT, WINAPI, waveOutWrite, HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);

    HOOK_DECLARE(BOOL, WINAPI, Beep, DWORD dwFreq, DWORD dwDuration);
    HOOK_DECLARE(BOOL, WINAPI, MessageBeep, UINT uType);
    HOOK_DECLARE(BOOL, WINAPI, PlaySoundA, LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
    HOOK_DECLARE(BOOL, WINAPI, PlaySoundW, LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);

    HOOK_DECLARE(MCIERROR, WINAPI, mciSendCommandA, MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    HOOK_DECLARE(MCIERROR, WINAPI, mciSendCommandW, MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    // hack for soundmixing
    struct CachedVolumeAndPan
    {
        DWORD leftVolumeAsScale; // out of 65536
        DWORD rightVolumeAsScale; // out of 65536
    };

    void AdvanceTimeAndMixAll(DWORD ticks);
    void PreSuspendSound();
    void PostResumeSound();
    void ForceAlignSound(bool earlier);
    void BackDoorStopAll();
    void StopAllSounds();
    void ResumePlayingSounds();
    void DoFrameBoundarySoundChecks();

    void ApplySoundIntercepts();

    bool HookCOMInterfaceSound(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);

    void SoundDllMainInit();
}
