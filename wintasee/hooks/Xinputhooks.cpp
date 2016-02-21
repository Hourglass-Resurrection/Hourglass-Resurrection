/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <cassert>

#include <wintasee.h>
#include <external/Xinput.h>

static bool gs_xinput_enabled = true;

HOOKFUNC DWORD WINAPI MyXInputGetState(
    DWORD         dwUserIndex, // [in] Index of the user's controller.
    XINPUT_STATE* pState       // [out] Receives the current state of the controller.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called; gs_xinput_enabled = %d.\n", dwUserIndex, static_cast<int>(gs_xinput_enabled));

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    /*
     * Increment packet number if the input has changed
     * from the last time the function was called.
     *
     * I only have one gamepad so I'm unable to test whether
     * the packet number is the same for all gamepads or
     * if it's increased separately for each, so
     * assuming it is the same. Shouldn't cause any issues.
     * -- YaLTeR
     */
    static XINPUT_GAMEPAD s_last_state[4];
    static DWORD s_packet_number = 0;

    XINPUT_GAMEPAD new_state = gs_xinput_enabled ? curinput.gamepad[dwUserIndex] : XINPUT_GAMEPAD();
    pState->Gamepad = new_state;

    if (s_last_state[dwUserIndex].wButtons != new_state.wButtons ||
        s_last_state[dwUserIndex].bLeftTrigger != new_state.bLeftTrigger ||
        s_last_state[dwUserIndex].bRightTrigger != new_state.bRightTrigger ||
        s_last_state[dwUserIndex].sThumbLX != new_state.sThumbLX ||
        s_last_state[dwUserIndex].sThumbLY != new_state.sThumbLY ||
        s_last_state[dwUserIndex].sThumbRX != new_state.sThumbRX ||
        s_last_state[dwUserIndex].sThumbRY != new_state.sThumbRY)
    {
        pState->dwPacketNumber = ++s_packet_number;
        s_last_state[dwUserIndex] = new_state;
    }
    else
    {
        pState->dwPacketNumber = s_packet_number;
    }

    return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputGetStateEx(
    DWORD         dwUserIndex, // [in] Index of the user's controller.
    XINPUT_STATE* pState       // [out] Receives the current state of the controller.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called; gs_xinput_enabled = %d.\n", dwUserIndex, static_cast<int>(gs_xinput_enabled));

    /*
     * This is an undocumented function of XInput 1.3 and above
     * that is said to also return the state of the guide button
     * at the 0x400 bit position. We don't handle this button (yet?)
     * so just call normal XInputGetState here.
     * -- YaLTeR
     */
    return MyXInputGetState(dwUserIndex, pState);
}

HOOKFUNC DWORD WINAPI MyXInputSetState(
    DWORD             dwUserIndex, // [in] Index of the user's controller.
    XINPUT_VIBRATION* pVibration   // [in, out] The vibration information to send to the controller.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputGetCapabilities(
    DWORD                dwUserIndex,  // [in] Index of the gamer associated with the device.
    DWORD                dwFlags,      // [in] Input flags that identify the device type.
    XINPUT_CAPABILITIES* pCapabilities // [out] Receives the capabilities.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d, dwFlags=%d) called.\n", dwUserIndex, dwFlags);

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    /*
     * We always return that 4 standard controllers are plugged in.
     */
    pCapabilities->Type = XINPUT_DEVTYPE_GAMEPAD;
    pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    pCapabilities->Flags = 0;

    XINPUT_GAMEPAD supported_gamepad;
    supported_gamepad.wButtons = 0xF3FF;
    /*
     * Doc says: For proportional controls, such as thumbsticks,
     *           the value indicates the resolution for that control.
     *           Some number of the least significant bits may not be set,
     *           indicating that the control does not provide resolution to that level.
     *
     * We set all bits to indicate maximal possible precision.
     */
    supported_gamepad.bLeftTrigger = ~0;
    supported_gamepad.bRightTrigger = ~0;
    supported_gamepad.sThumbLX = ~0;
    supported_gamepad.sThumbLY = ~0;
    supported_gamepad.sThumbRX = ~0;
    supported_gamepad.sThumbRY = ~0;
    pCapabilities->Gamepad = supported_gamepad;

    XINPUT_VIBRATION no_vibration;
    no_vibration.wLeftMotorSpeed = 0;
    no_vibration.wRightMotorSpeed = 0;
    pCapabilities->Vibration = no_vibration;

    return ERROR_SUCCESS;
}

HOOKFUNC DWORD WINAPI MyXInputGetDSoundAudioDeviceGuids(
    DWORD dwUserIndex,       // [in] Index of the gamer associated with the device.
    GUID* pDSoundRenderGuid, // [out] DSound device ID for render.
    GUID* pDSoundCaptureGuid // [out] DSound device ID for capture.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    /*
     * No headset plugged in.
     */
    *pDSoundRenderGuid = GUID_NULL;
    *pDSoundCaptureGuid = GUID_NULL;
    return ERROR_SUCCESS;
}

enum class ThumbState
{
    NONE,
    UP,
    UP_RIGHT,
    RIGHT,
    DOWN_RIGHT,
    DOWN,
    DOWN_LEFT,
    LEFT,
    UP_LEFT
};

/*
 * From my tests, thumbs get triggered
 * when the respective axis is > 20000 (or < -20000),
 * and triggers get triggered when their value is > 30.
 *
 * Thumb axes: Up and Right is positive, Down and Left is negative.
 * -- YaLTeR
 */

static ThumbState XYToThumbState(SHORT x, SHORT y)
{
    enum
    {
        THUMB_THRESHOLD = 20000
    };

    if (x > THUMB_THRESHOLD)
    {
        if (y > THUMB_THRESHOLD)
        {
            return ThumbState::UP_RIGHT;
        }
        else if (y < -THUMB_THRESHOLD)
        {
            return ThumbState::DOWN_RIGHT;
        }
        else
        {
            return ThumbState::RIGHT;
        }
    }
    else if (x < -THUMB_THRESHOLD)
    {
        if (y > THUMB_THRESHOLD)
        {
            return ThumbState::UP_LEFT;
        }
        else if (y < -THUMB_THRESHOLD)
        {
            return ThumbState::DOWN_LEFT;
        }
        else
        {
            return ThumbState::LEFT;
        }
    }
    else
    {
        if (y > THUMB_THRESHOLD)
        {
            return ThumbState::UP;
        }
        else if (y < -THUMB_THRESHOLD)
        {
            return ThumbState::DOWN;
        }
        else
        {
            return ThumbState::NONE;
        }
    }
}

enum class ThumbStick
{
    LEFT,
    RIGHT
};

static WORD ThumbStateToVirtualKey(ThumbState state, ThumbStick which_stick)
{
    switch (state)
    {
    case ThumbState::UP:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_UP : VK_PAD_RTHUMB_UP;
    case ThumbState::UP_RIGHT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_UPRIGHT : VK_PAD_RTHUMB_UPRIGHT;
    case ThumbState::RIGHT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_RIGHT : VK_PAD_RTHUMB_RIGHT;
    case ThumbState::DOWN_RIGHT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_DOWNRIGHT : VK_PAD_RTHUMB_DOWNRIGHT;
    case ThumbState::DOWN:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_DOWN : VK_PAD_RTHUMB_DOWN;
    case ThumbState::DOWN_LEFT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_DOWNLEFT : VK_PAD_RTHUMB_DOWNLEFT;
    case ThumbState::LEFT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_LEFT : VK_PAD_RTHUMB_LEFT;
    case ThumbState::UP_LEFT:
        return (which_stick == ThumbStick::LEFT) ? VK_PAD_LTHUMB_UPLEFT : VK_PAD_RTHUMB_UPLEFT;
    default:
        /*
         * TODO: replace with DLL_ASSERT when it's upstream.
         */
        assert(false);
    }

    /*
     * Unreachable.
     */
    return 0;
}

struct KeystrokeState
{
    WORD m_buttons = 0;
    bool m_left_trigger = false;
    bool m_right_trigger = false;
    ThumbState m_left_thumb = ThumbState::NONE;
    ThumbState m_right_thumb = ThumbState::NONE;

    KeystrokeState()
    {
    }

    enum
    {
        TRIGGER_THRESHOLD = 30
    };
    KeystrokeState(const XINPUT_GAMEPAD& gamepad)
        : m_buttons(gamepad.wButtons)
        , m_left_trigger(gamepad.bLeftTrigger > TRIGGER_THRESHOLD)
        , m_right_trigger(gamepad.bRightTrigger > TRIGGER_THRESHOLD)
        , m_left_thumb(XYToThumbState(gamepad.sThumbLX, gamepad.sThumbLY))
        , m_right_thumb(XYToThumbState(gamepad.sThumbRX, gamepad.sThumbRY))
    {
    }

    bool CheckOneDifference(const KeystrokeState& new_state, XINPUT_KEYSTROKE* keystroke)
    {
        #define CHECK_BUTTON(gamepad_name, vk_name) \
            if ((m_buttons & XINPUT_GAMEPAD_ ## gamepad_name) != (new_state.m_buttons & XINPUT_GAMEPAD_ ## gamepad_name)) \
            { \
                keystroke->VirtualKey = VK_PAD_ ## vk_name; \
                if ((new_state.m_buttons & XINPUT_GAMEPAD_ ## gamepad_name) > 0) \
                { \
                    keystroke->Flags = XINPUT_KEYSTROKE_KEYDOWN; \
                    m_buttons |= XINPUT_GAMEPAD_ ## gamepad_name; \
                } \
                else \
                { \
                    keystroke->Flags = XINPUT_KEYSTROKE_KEYUP; \
                    m_buttons &= ~XINPUT_GAMEPAD_ ## gamepad_name; \
                } \
                return true; \
            }

        CHECK_BUTTON(DPAD_UP, DPAD_UP);
        CHECK_BUTTON(DPAD_DOWN, DPAD_DOWN);
        CHECK_BUTTON(DPAD_LEFT, DPAD_LEFT);
        CHECK_BUTTON(DPAD_RIGHT, DPAD_RIGHT);
        CHECK_BUTTON(START, START);
        CHECK_BUTTON(BACK, BACK);
        CHECK_BUTTON(LEFT_THUMB, LTHUMB_PRESS);
        CHECK_BUTTON(RIGHT_THUMB, RTHUMB_PRESS);
        CHECK_BUTTON(LEFT_SHOULDER, LSHOULDER);
        CHECK_BUTTON(RIGHT_SHOULDER, RSHOULDER);
        CHECK_BUTTON(A, A);
        CHECK_BUTTON(B, B);
        CHECK_BUTTON(X, X);
        CHECK_BUTTON(Y, Y);
        #undef CHECK_BUTTON

        if (m_left_trigger != new_state.m_left_trigger)
        {
            keystroke->VirtualKey = VK_PAD_LTRIGGER;
            keystroke->Flags = new_state.m_left_trigger ? XINPUT_KEYSTROKE_KEYDOWN : XINPUT_KEYSTROKE_KEYUP;
            m_left_trigger = new_state.m_left_trigger;
            return true;
        }
        if (m_right_trigger != new_state.m_right_trigger)
        {
            keystroke->VirtualKey = VK_PAD_RTRIGGER;
            keystroke->Flags = new_state.m_right_trigger ? XINPUT_KEYSTROKE_KEYDOWN : XINPUT_KEYSTROKE_KEYUP;
            m_right_trigger = new_state.m_right_trigger;
            return true;
        }

        /*
         * If a thumb went from a non-None state to another non-None state,
         * we first send a KEYUP and set the state to None, and then
         * on a subsequent call we send a KEYDOWN and set the sate to the new state.
         * -- YaLTeR
         */
        if (m_left_thumb != new_state.m_left_thumb)
        {
            if (m_left_thumb == ThumbState::NONE)
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(new_state.m_left_thumb, ThumbStick::LEFT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYDOWN;
                m_left_thumb = new_state.m_left_thumb;
                return true;
            }
            else if (new_state.m_left_thumb == ThumbState::NONE)
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(m_left_thumb, ThumbStick::LEFT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
                m_left_thumb = new_state.m_left_thumb;
                return true;
            }
            else
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(m_left_thumb, ThumbStick::LEFT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
                m_left_thumb = ThumbState::NONE;
                return true;
            }
        }
        if (m_right_thumb != new_state.m_right_thumb)
        {
            if (m_right_thumb == ThumbState::NONE)
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(new_state.m_right_thumb, ThumbStick::RIGHT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYDOWN;
                m_right_thumb = new_state.m_right_thumb;
                return true;
            }
            else if (new_state.m_right_thumb == ThumbState::NONE)
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(m_right_thumb, ThumbStick::RIGHT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
                m_right_thumb = new_state.m_right_thumb;
                return true;
            }
            else
            {
                keystroke->VirtualKey = ThumbStateToVirtualKey(m_right_thumb, ThumbStick::RIGHT);
                keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
                m_right_thumb = ThumbState::NONE;
                return true;
            }
        }

        /*
         * Nothing changed.
         */
        return false;
    }
};

HOOKFUNC DWORD WINAPI MyXInputGetKeystroke(
    DWORD             dwUserIndex, // [in] Index of the gamer associated with the device.
    DWORD             dwReserved,  // [in] Reserved.
    XINPUT_KEYSTROKE* pKeystroke   // [out] Receives the keystroke.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called; gs_xinput_enabled = %d.\n", dwUserIndex, static_cast<int>(gs_xinput_enabled));

    if (dwUserIndex > 3 && dwUserIndex != XUSER_INDEX_ANY)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    static KeystrokeState s_last_state[4];

    if (dwUserIndex == XUSER_INDEX_ANY)
    {
        /*
         * Check every gamepad for changes.
         */
        bool something_changed = false;
        for (INT i = 0; i < 4; ++i)
        {
            XINPUT_GAMEPAD new_state = gs_xinput_enabled ? curinput.gamepad[i] : XINPUT_GAMEPAD();
            if (s_last_state[i].CheckOneDifference(new_state, pKeystroke))
            {
                pKeystroke->UserIndex = i;
                something_changed = true;
                break;
            }
        }

        if (!something_changed)
        {
            return ERROR_EMPTY;
        }
    }
    else
    {
        pKeystroke->UserIndex = static_cast<BYTE>(dwUserIndex);
        XINPUT_GAMEPAD new_state = gs_xinput_enabled ? curinput.gamepad[dwUserIndex] : XINPUT_GAMEPAD();
        if (!s_last_state[dwUserIndex].CheckOneDifference(curinput.gamepad[dwUserIndex], pKeystroke))
        {
            /*
             * Nothing changed.
             */
            return ERROR_EMPTY;
        }
    }

    pKeystroke->Unicode = L'\0';
    pKeystroke->HidCode = 0;
    return ERROR_SUCCESS;
}

HOOKFUNC void WINAPI MyXInputEnable(
    BOOL enable // [in] Indicates whether XInput is enabled or disabled.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(enable = %d) called.\n", enable);

    gs_xinput_enabled = (enable == FALSE) ? false : true;
}

HOOKFUNC DWORD WINAPI MyXInputGetBatteryInformation(
    DWORD                       dwUserIndex,        // [in] Index of the gamer associated with the device.
    BYTE                        devType,            // [in] Which device on this user index.
    XINPUT_BATTERY_INFORMATION* pBatteryInformation // [out] Contains the level and types of batteries.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    /*
     * A wired controller doesn't have a battery.
     * BatteryLevel is undefined.
     */
    pBatteryInformation->BatteryType = BATTERY_TYPE_WIRED;

    return ERROR_SUCCESS;
}

HOOKFUNC DWORD MyXInputGetAudioDeviceIds(
    DWORD  dwUserIndex,      // [in] Index of the gamer associated with the device.
    LPWSTR pRenderDeviceId,  // [out, optional] Windows Core Audio device ID string for render (speakers).
    UINT*  pRenderCount,     // [in, out, optional] Size, in wide-chars, of the render device ID string buffer.
    LPWSTR pCaptureDeviceId, // [out, optional] Windows Core Audio device ID string for capture (microphone).
    UINT*  pCaptureCount     // [in, out, optional] Size, in wide-chars, of capture device ID string buffer.
)
{
    debuglog(LCF_JOYPAD, __FUNCTION__ "(dwUserIndex=%d) called.\n", dwUserIndex);

    if (dwUserIndex > 3)
    {
        return ERROR_BAD_ARGUMENTS;
    }

    /*
     * No audio devices connected.
     */
    if (pRenderCount)
    {
        if (*pRenderCount > 0 && pRenderDeviceId)
        {
            pRenderDeviceId[0] = L'\0';
        }

        *pRenderCount = 0;
    }

    if (pCaptureCount)
    {
        if (*pCaptureCount > 0 && pCaptureDeviceId)
        {
            pCaptureDeviceId[0] = L'\0';
        }

        *pCaptureCount = 0;
    }

    return ERROR_SUCCESS;
}

void ApplyXinputIntercepts()
{
    static const InterceptDescriptor intercepts[] =
    {
        MAKE_INTERCEPT(1, XInput9_1_0, XInputGetState),
        MAKE_INTERCEPT(1, XInput9_1_0, XInputSetState),
        MAKE_INTERCEPT(1, XInput9_1_0, XInputGetCapabilities),
        MAKE_INTERCEPT(1, XInput9_1_0, XInputGetDSoundAudioDeviceGuids),
        MAKE_INTERCEPT(1, XInput9_1_0, XInputEnable),
        MAKE_INTERCEPT(1, XInput1_3, XInputGetState),
        MAKE_INTERCEPT(1, XInput1_3, XInputSetState),
        MAKE_INTERCEPT(1, XInput1_3, XInputGetCapabilities),
        MAKE_INTERCEPT(1, XInput1_3, XInputGetDSoundAudioDeviceGuids),
        MAKE_INTERCEPT(1, XInput1_3, XInputGetKeystroke),
        MAKE_INTERCEPT(1, XInput1_3, XInputEnable),
        MAKE_INTERCEPT(1, XInput1_3, XInputGetBatteryInformation),
        MAKE_INTERCEPT_ORD(1, XInput1_3, XInputGetStateEx, 100),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetState),
        MAKE_INTERCEPT(1, XInput1_4, XInputSetState),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetCapabilities),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetDSoundAudioDeviceGuids),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetKeystroke),
        MAKE_INTERCEPT(1, XInput1_4, XInputEnable),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetBatteryInformation),
        MAKE_INTERCEPT(1, XInput1_4, XInputGetAudioDeviceIds),
        MAKE_INTERCEPT_ORD(1, XInput1_4, XInputGetStateEx, 100),
    };
    ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}
