/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include <../../external/Xinput.h>

#define XInputGetState TrampXInputGetState
TRAMPFUNC DWORD WINAPI XInputGetState(
    DWORD         dwUserIndex, // [in] Index of the user's controller.
    XINPUT_STATE* pState       // [out] Receives the current state of the controller.
) TRAMPOLINE_DEF

#define XInputGetStateEx TrampXInputGetStateEx
TRAMPFUNC DWORD WINAPI XInputGetStateEx(
    DWORD         dwUserIndex, // [in] Index of the user's controller.
    XINPUT_STATE* pState       // [out] Receives the current state of the controller.
) TRAMPOLINE_DEF

#define XInputSetState TrampXInputSetState
TRAMPFUNC DWORD WINAPI XInputSetState(
    DWORD             dwUserIndex, // [in] Index of the user's controller.
    XINPUT_VIBRATION* pVibration   // [in, out] The vibration information to send to the controller.
) TRAMPOLINE_DEF

#define XInputGetCapabilities TrampXInputGetCapabilities
TRAMPFUNC DWORD WINAPI XInputGetCapabilities(
    DWORD                dwUserIndex,  // [in] Index of the gamer associated with the device.
    DWORD                dwFlags,      // [in] Input flags that identify the device type.
    XINPUT_CAPABILITIES* pCapabilities // [out] Receives the capabilities.
) TRAMPOLINE_DEF

#define XInputGetDSoundAudioDeviceGuids TrampXInputGetDSoundAudioDeviceGuids
TRAMPFUNC DWORD WINAPI XInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,       // [in] Index of the gamer associated with the device.
    GUID* pDSoundRenderGuid, // [out] DSound device ID for render.
    GUID* pDSoundCaptureGuid // [out] DSound device ID for capture.
) TRAMPOLINE_DEF

#define XInputGetKeystroke TrampXInputGetKeystroke
TRAMPFUNC DWORD WINAPI XInputGetKeystroke(
    DWORD             dwUserIndex, // [in] Index of the gamer associated with the device.
    DWORD             dwReserved,  // [in] Reserved.
    XINPUT_KEYSTROKE* pKeystroke   // [out] Receives the keystroke.
) TRAMPOLINE_DEF

#define XInputEnable TrampXInputEnable
TRAMPFUNC void WINAPI XInputEnable(
    BOOL enable // [in] Indicates whether XInput is enabled or disabled.
) TRAMPOLINE_DEF_VOID

#define XInputGetBatteryInformation TrampXInputGetBatteryInformation
TRAMPFUNC DWORD WINAPI XInputGetBatteryInformation(
    DWORD                       dwUserIndex,        // [in] Index of the gamer associated with the device.
    BYTE                        devType,            // [in] Which device on this user index.
    XINPUT_BATTERY_INFORMATION* pBatteryInformation // [out] Contains the level and types of batteries.
) TRAMPOLINE_DEF

#define XInputGetAudioDeviceIds TrampXInputGetAudioDeviceIds
TRAMPFUNC DWORD XInputGetAudioDeviceIds(
    DWORD  dwUserIndex,      // [in] Index of the gamer associated with the device.
    LPWSTR pRenderDeviceId,  // [out, optional] Windows Core Audio device ID string for render (speakers).
    UINT*  pRenderCount,     // [in, out, optional] Size, in wide-chars, of the render device ID string buffer.
    LPWSTR pCaptureDeviceId, // [out, optional] Windows Core Audio device ID string for capture (microphone).
    UINT*  pCaptureCount     // [in, out, optional] Size, in wide-chars, of capture device ID string buffer.
) TRAMPOLINE_DEF
