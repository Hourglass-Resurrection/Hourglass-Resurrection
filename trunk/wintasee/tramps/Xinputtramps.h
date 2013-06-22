/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef XINPUTTRAMPS_H_INCL
#define XINPUTTRAMPS_H_INCL

#include <Xinput.h>

#define XInputGetState TrampXInputGetState
TRAMPFUNC DWORD WINAPI XInputGetState(
    DWORD         dwUserIndex,
	XINPUT_STATE* pState
) TRAMPOLINE_DEF

#define XInputSetState TrampXInputSetState
TRAMPFUNC DWORD WINAPI XInputSetState(
    DWORD             dwUserIndex,
    XINPUT_VIBRATION* pVibration
) TRAMPOLINE_DEF

#define XInputGetCapabilities TrampXInputGetCapabilities
TRAMPFUNC DWORD WINAPI XInputGetCapabilities(
    DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
    DWORD                dwFlags,       // [in] Input flags that identify the device type
    XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
) TRAMPOLINE_DEF

#define XInputGetDSoundAudioDeviceGuids TrampXInputGetDSoundAudioDeviceGuids
DWORD WINAPI XInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,          // [in] Index of the gamer associated with the device
    GUID* pDSoundRenderGuid,    // [out] DSound device ID for render
    GUID* pDSoundCaptureGuid    // [out] DSound device ID for capture
) TRAMPOLINE_DEF

#endif
