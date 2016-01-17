/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef XINPUTTRAMPS_H_INCL
#define XINPUTTRAMPS_H_INCL

//#include <Xinput.h>
#include <../../external/Xinput.h>

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
TRAMPFUNC DWORD WINAPI XInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,          // [in] Index of the gamer associated with the device
    GUID* pDSoundRenderGuid,    // [out] DSound device ID for render
    GUID* pDSoundCaptureGuid    // [out] DSound device ID for capture
) TRAMPOLINE_DEF

#define XInputGetKeystroke TrampXInputGetKeystroke
TRAMPFUNC DWORD WINAPI XInputGetKeystroke(
    DWORD dwUserIndex,  // [in] Index of the gamer associated with the device
    DWORD dwReserved,   // [in] Reserved
    XINPUT_KEYSTROKE* pKeystroke    // [out] Receives the keystroke
) TRAMPOLINE_DEF

#define XInputEnable TrampXInputEnable
TRAMPFUNC void WINAPI XInputEnable(
    BOOL enable // [in] Indicates whether xinput is enabled or disabled.
) TRAMPOLINE_DEF_VOID

#define XInputGetBatteryInformation TrampXInputGetBatteryInformation
TRAMPFUNC DWORD WINAPI XInputGetBatteryInformation(
    DWORD dwUserIndex,  // [in] Index of the gamer associated with the device
    BYTE devType,       // [in] Which device on this user index
    XINPUT_BATTERY_INFORMATION* pBatteryInformation  // [out] Contains the level and types of batteries
) TRAMPOLINE_DEF

#endif
