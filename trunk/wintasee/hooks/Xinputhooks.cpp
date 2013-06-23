/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(XINPUTHOOKS_INCL) && !defined(UNITY_BUILD)
#define XINPUTHOOKS_INCL

#include "../global.h"
//#include <Xinput.h>
#include <../../external/Xinput.h>

HOOKFUNC DWORD WINAPI MyXInputGetState(
    DWORD         dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_STATE* pState        // [out] Receives the current state
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	return XInputGetState(dwUserIndex, pState);
}

HOOKFUNC DWORD WINAPI MyXInputSetState(
    DWORD             dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_VIBRATION* pVibration    // [in, out] The vibration information to send to the controller
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	return XInputSetState(dwUserIndex, pVibration);
}

HOOKFUNC DWORD WINAPI MyXInputGetCapabilities(
    DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
    DWORD                dwFlags,       // [in] Input flags that identify the device type
    XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d, dwFlags=%d) called.\n", dwUserIndex, dwFlags);
	return XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
}

HOOKFUNC DWORD WINAPI MyXInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,          // [in] Index of the gamer associated with the device
    GUID* pDSoundRenderGuid,    // [out] DSound device ID for render
    GUID* pDSoundCaptureGuid    // [out] DSound device ID for capture
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	return XInputGetDSoundAudioDeviceGuids(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
}


void ApplyXinputIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
	MAKE_INTERCEPT(1, XInput9_1_0, XInputGetState),
	MAKE_INTERCEPT(1, XInput9_1_0, XInputSetState),
	MAKE_INTERCEPT(1, XInput9_1_0, XInputGetCapabilities),
	MAKE_INTERCEPT(1, XInput9_1_0, XInputGetDSoundAudioDeviceGuids),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
