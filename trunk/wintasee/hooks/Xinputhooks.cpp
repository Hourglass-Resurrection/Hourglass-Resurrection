/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(XINPUTHOOKS_INCL) && !defined(UNITY_BUILD)
#define XINPUTHOOKS_INCL

//#include "../global.h"
#include "../wintasee.h"
//#include <Xinput.h>
#include <../../external/Xinput.h>

DWORD myDwPacketNumber = 0;

HOOKFUNC DWORD WINAPI MyXInputGetState(
    DWORD         dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_STATE* pState        // [out] Receives the current state
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	//return XInputGetState(dwUserIndex, pState);

	pState->Gamepad = curinput.gamepad[dwUserIndex];

	// We have to see if the state is the same as the previous state
	// We can use memcmp because the structs have been initialised with memset.
	if (memcmp(&curinput.gamepad[dwUserIndex], &previnput.gamepad[dwUserIndex], sizeof(XINPUT_GAMEPAD)) == 0)
		pState->dwPacketNumber = myDwPacketNumber;
	else
		pState->dwPacketNumber = ++myDwPacketNumber;

	return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputSetState(
    DWORD             dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_VIBRATION* pVibration    // [in, out] The vibration information to send to the controller
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	//return XInputSetState(dwUserIndex, pVibration);

	return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputGetCapabilities(
    DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
    DWORD                dwFlags,       // [in] Input flags that identify the device type
    XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d, dwFlags=%d) called.\n", dwUserIndex, dwFlags);
	//return XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);

	// We always return that 4 standard controllers are plugged in.

	pCapabilities->Type = XINPUT_DEVTYPE_GAMEPAD;
	pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
	pCapabilities->Flags = 0;

	XINPUT_GAMEPAD fullySupportedGamepad;
	fullySupportedGamepad.wButtons = 0xF3FF;
	/* Doc says: For proportional controls, such as thumbsticks,
	 *           the value indicates the resolution for that control.
	 *           Some number of the least significant bits may not be set,
	 *           indicating that the control does not provide resolution to that level.
	 */
	fullySupportedGamepad.bLeftTrigger = 0xFF;
	fullySupportedGamepad.bRightTrigger = 0xFF;
	fullySupportedGamepad.sThumbLX = 0xFFFF;
	fullySupportedGamepad.sThumbLY = 0xFFFF;
	fullySupportedGamepad.sThumbRX = 0xFFFF;
	fullySupportedGamepad.sThumbRY = 0xFFFF;
	pCapabilities->Gamepad = fullySupportedGamepad;

	XINPUT_VIBRATION noVibration;
	noVibration.wLeftMotorSpeed = 0;
	noVibration.wRightMotorSpeed = 0;
	pCapabilities->Vibration = noVibration;

	return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,          // [in] Index of the gamer associated with the device
    GUID* pDSoundRenderGuid,    // [out] DSound device ID for render
    GUID* pDSoundCaptureGuid    // [out] DSound device ID for capture
)
{
	debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);
	//return XInputGetDSoundAudioDeviceGuids(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);

	// No headset plugged in.
	*pDSoundRenderGuid = GUID_NULL;
	*pDSoundCaptureGuid = GUID_NULL;
	return ERROR_SUCCESS;
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
