#define DIRECTINPUT_VERSION 0x0500 // for joystick support
#include "InputCapture.h"
#include "CustomDLGs.h"
#include "Resource.h"
#include "logging.h"
#pragma comment(lib, "../../external/lib/dinput.lib")
#pragma comment(lib, "../../external/lib/dxguid.lib")

// clang-format off
struct ModifierKey InputCapture::modifierKeys[] = 
{
    {DIK_LCONTROL, 0x01},
    {DIK_RCONTROL, 0x02},
    {DIK_LSHIFT,   0x04},
    {DIK_RSHIFT,   0x08},
    {DIK_LMENU,    0x10},
    {DIK_RMENU,    0x20},
    {DIK_LWIN,     0x40},
    {DIK_RWIN,     0x80}
};


struct SingleInput InputCapture::SIList[] =
{
    {SINGLE_INPUT_DI_KEYBOARD, DIK_ESCAPE, L"Escape"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_1, L"1"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_2, L"2"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_3, L"3"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_4, L"4"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_5, L"5"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_6, L"6"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_7, L"7"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_8, L"8"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_9, L"9"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_0, L"0"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MINUS, L"-_"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_EQUALS, L"=+"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_BACK, L"Backspace"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_TAB, L"Tab"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_Q, L"Q"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_W, L"W"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_E, L"E"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_R, L"R"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_T, L"T"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_Y, L"Y"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_U, L"U"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_I, L"I"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_O, L"O"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_P, L"P"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LBRACKET, L"[{"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RBRACKET, L"]}"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RETURN, L"Enter"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LCONTROL, L"LControl"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_A, L"A"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_S, L"S"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_D, L"D"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F, L"F"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_G, L"G"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_H, L"H"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_J, L"J"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_K, L"K"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_L, L"L"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SEMICOLON, L"},:"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_APOSTROPHE, L"'\""},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_GRAVE, L"`~"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LSHIFT, L"LShift"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_BACKSLASH, L"\\|"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_Z, L"Z"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_X, L"X"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_C, L"C"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_V, L"V"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_B, L"B"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_N, L"N"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_M, L"M"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_COMMA, L",<"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_PERIOD, L".>"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SLASH, L"/?"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RSHIFT, L"RShift"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MULTIPLY, L"Numpad*"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LMENU, L"LAlt"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SPACE, L"Space"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_CAPITAL, L"CapsLock"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F1, L"F1"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F2, L"F2"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F3, L"F3"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F4, L"F4"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F5, L"F5"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F6, L"F6"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F7, L"F7"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F8, L"F8"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F9, L"F9"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F10, L"F10"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMLOCK, L"NumLock"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SCROLL, L"ScrollLock"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD7, L"Numpad7"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD8, L"Numpad8"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD9, L"Numpad9"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SUBTRACT, L"Numpad-"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD4, L"Numpad4"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD5, L"Numpad5"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD6, L"Numpad6"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_ADD, L"Numpad+"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD1, L"Numpad1"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD2, L"Numpad2"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD3, L"Numpad3"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD0, L"Numpad0"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_DECIMAL, L"Numpad."},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_OEM_102, L"<>\\|"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F11, L"F11"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F12, L"F12"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F13, L"F13"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F14, L"F14"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_F15, L"F15"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_KANA, L"Kana"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_ABNT_C1, L"/?"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_CONVERT, L"Convert"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NOCONVERT, L"NoConvert"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_YEN, L"Yen"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_ABNT_C2, L"Numpad."},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADEQUALS, L"Numpad="},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_PREVTRACK, L"Prevtrack"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_AT, L"@"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_COLON, L":"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_UNDERLINE, L"_"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_KANJI, L"Kanji"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_STOP, L"Stop"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_AX, L"AX"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_UNLABELED, L"Unlabeled"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NEXTTRACK, L"Nexttrack"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADENTER, L"NumpadEnter"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RCONTROL, L"RControl"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MUTE, L"Mute"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_CALCULATOR, L"Calculator"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_PLAYPAUSE, L"PlayPause"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MEDIASTOP, L"MediaStop"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_VOLUMEDOWN, L"VolumeDown"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_VOLUMEUP, L"VolumeUp"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBHOME, L"WebHome"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADCOMMA, L"Numpad,"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_DIVIDE, L"Numpad/"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SYSRQ, L"Sysrq"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RMENU, L"RAlt"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_PAUSE, L"Pause"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_HOME, L"Home"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_UP, L"Up"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_PRIOR, L"PageUp"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LEFT, L"Left"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RIGHT, L"Right"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_END, L"End"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_DOWN, L"Down"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_NEXT, L"PageDown"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_INSERT, L"Insert"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_DELETE, L"Delete"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_LWIN, L"LWin"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_RWIN, L"RWin"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_APPS, L"Apps"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_POWER, L"Power"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_SLEEP, L"Sleep"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WAKE, L"Wake"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBSEARCH, L"WebSearch"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBFAVORITES, L"WebFavorites"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBREFRESH, L"WebRefresh"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBSTOP, L"WebStop"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBFORWARD, L"WebForward"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_WEBBACK, L"WebBack"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MYCOMPUTER, L"MyComputer"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MAIL, L"Mail"},
    {SINGLE_INPUT_DI_KEYBOARD, DIK_MEDIASELECT, L"MediaSelect"},

    // Mouse
    {SINGLE_INPUT_DI_MOUSE, 0, L"Mouse Button 0"},
    {SINGLE_INPUT_DI_MOUSE, 1, L"Mouse Button 1"},
    {SINGLE_INPUT_DI_MOUSE, 2, L"Mouse Button 2"},
    {SINGLE_INPUT_DI_MOUSE, 3, L"Mouse Button 3"},

    // Xbox controller 1
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 0, L"Xbox Controller 1 - Dpad Up"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 1, L"Xbox Controller 1 - Dpad Down"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 2, L"Xbox Controller 1 - Dpad Left"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 3, L"Xbox Controller 1 - Dpad Right"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 4, L"Xbox Controller 1 - Start"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 5, L"Xbox Controller 1 - Back"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 6, L"Xbox Controller 1 - L3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 7, L"Xbox Controller 1 - R3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 8, L"Xbox Controller 1 - LB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 9, L"Xbox Controller 1 - RB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 12, L"Xbox Controller 1 - A"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 13, L"Xbox Controller 1 - B"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 14, L"Xbox Controller 1 - X"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (0 << 8) | 15, L"Xbox Controller 1 - Y"},

    // Xbox controller 2
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 0, L"Xbox Controller 2 - Dpad Up"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 1, L"Xbox Controller 2 - Dpad Down"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 2, L"Xbox Controller 2 - Dpad Left"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 3, L"Xbox Controller 2 - Dpad Right"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 4, L"Xbox Controller 2 - Start"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 5, L"Xbox Controller 2 - Back"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 6, L"Xbox Controller 2 - L3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 7, L"Xbox Controller 2 - R3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 8, L"Xbox Controller 2 - LB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 9, L"Xbox Controller 2 - RB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 12, L"Xbox Controller 2 - A"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 13, L"Xbox Controller 2 - B"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 14, L"Xbox Controller 2 - X"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (1 << 8) | 15, L"Xbox Controller 2 - Y"},

    // Xbox controller 3
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 0, L"Xbox Controller 3 - Dpad Up"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 1, L"Xbox Controller 3 - Dpad Down"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 2, L"Xbox Controller 3 - Dpad Left"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 3, L"Xbox Controller 3 - Dpad Right"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 4, L"Xbox Controller 3 - Start"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 5, L"Xbox Controller 3 - Back"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 6, L"Xbox Controller 3 - L3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 7, L"Xbox Controller 3 - R3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 8, L"Xbox Controller 3 - LB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 9, L"Xbox Controller 3 - RB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 12, L"Xbox Controller 3 - A"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 13, L"Xbox Controller 3 - B"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 14, L"Xbox Controller 3 - X"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (2 << 8) | 15, L"Xbox Controller 3 - Y"},

    // Xbox controller 4
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 0, L"Xbox Controller 4 - Dpad Up"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 1, L"Xbox Controller 4 - Dpad Down"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 2, L"Xbox Controller 4 - Dpad Left"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 3, L"Xbox Controller 4 - Dpad Right"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 4, L"Xbox Controller 4 - Start"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 5, L"Xbox Controller 4 - Back"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 6, L"Xbox Controller 4 - L3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 7, L"Xbox Controller 4 - R3"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 8, L"Xbox Controller 4 - LB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 9, L"Xbox Controller 4 - RB"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 12, L"Xbox Controller 4 - A"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 13, L"Xbox Controller 4 - B"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 14, L"Xbox Controller 4 - X"},
    {SINGLE_INPUT_XINPUT_JOYSTICK, (3 << 8) | 15, L"Xbox Controller 4 - Y"}

};

struct Event InputCapture::eventList[] =
{
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_T},              ID_TOGGLE_MOVIE_READONLY,  L"Toggle Movie Read-Only"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_O},              ID_FILES_OPENEXE,          L"Open Executable"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_M},              ID_FILES_OPENMOV,          L"Open Movie"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_P},              ID_FILES_PLAYMOV,          L"Run and Play"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_W},              ID_FILES_WATCHMOV,         L"Watch from Beginning"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_R},              ID_FILES_RECORDMOV,        L"Start Recording To"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_RESUMERECORDING,  L"Resume Recording From Now"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_BACKUPMOVIE,      L"Backup Movie"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL | DIK_LSHIFT*/ 0x500 | DIK_C}, ID_FILES_STOP_RELAY,       L"Stop Running"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_QUIT,             L"Exit " /*"winTASer"*/ L"Hourglass"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_ESCAPE},                                  ID_TIME_TOGGLE_PAUSE,      L"Pause/Unpause"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_DELETE},                                  ID_TIME_TOGGLE_PAUSE,      L"Pause/Unpause (key 2)"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_PAUSE},                                   ID_TIME_TOGGLE_PAUSE,      L"Pause/Unpause (key 3)"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_TIME_TOGGLE_FASTFORWARD,L"Toggle Fast-Forward"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_TAB},                                     ID_SWITCH_TO_TASEE_FROM_TASER, L"Fast Forward"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_OEM_102},                                 ID_TIME_FRAME_ADVANCE,     L"Frame Advance"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_SPACE},                                   ID_TIME_FRAME_ADVANCE,     L"Frame Advance (key 2)"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_MINUS},          ID_TIME_RATE_SLOWER,      L"Decrease Speed"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_EQUALS},         ID_TIME_RATE_FASTER,      L"Increase Speed"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_TIME_RATE_100,         L"Normal Speed"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SOUND_NOPLAYBUFFERS,   L"Disable/Enable Sound"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_1},                                       ID_FILES_LOADSTATE_1,     L"Load State 1"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_2},                                       ID_FILES_LOADSTATE_2,     L"Load State 2"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_3},                                       ID_FILES_LOADSTATE_3,     L"Load State 3"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_4},                                       ID_FILES_LOADSTATE_4,     L"Load State 4"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_5},                                       ID_FILES_LOADSTATE_5,     L"Load State 5"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_6},                                       ID_FILES_LOADSTATE_6,     L"Load State 6"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_7},                                       ID_FILES_LOADSTATE_7,     L"Load State 7"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_8},                                       ID_FILES_LOADSTATE_8,     L"Load State 8"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_9},                                       ID_FILES_LOADSTATE_9,     L"Load State 9"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_0},                                       ID_FILES_LOADSTATE_10,    L"Load State 10"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F1},                                      ID_FILES_LOADSTATE_11,    L"Load State 11"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F2},                                      ID_FILES_LOADSTATE_12,    L"Load State 12"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F3},                                      ID_FILES_LOADSTATE_13,    L"Load State 13"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F4},                                      ID_FILES_LOADSTATE_14,    L"Load State 14"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F5},                                      ID_FILES_LOADSTATE_15,    L"Load State 15"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F6},                                      ID_FILES_LOADSTATE_16,    L"Load State 16"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F7},                                      ID_FILES_LOADSTATE_17,    L"Load State 17"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F8},                                      ID_FILES_LOADSTATE_18,    L"Load State 18"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F9},                                      ID_FILES_LOADSTATE_19,    L"Load State 19"},
    {{SINGLE_INPUT_DI_KEYBOARD, DIK_F10},                                     ID_FILES_LOADSTATE_20,    L"Load State 20"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_1},                ID_FILES_SAVESTATE_1,     L"Save State 1"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_2},                ID_FILES_SAVESTATE_2,     L"Save State 2"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_3},                ID_FILES_SAVESTATE_3,     L"Save State 3"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_4},                ID_FILES_SAVESTATE_4,     L"Save State 4"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_5},                ID_FILES_SAVESTATE_5,     L"Save State 5"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_6},                ID_FILES_SAVESTATE_6,     L"Save State 6"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_7},                ID_FILES_SAVESTATE_7,     L"Save State 7"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_8},                ID_FILES_SAVESTATE_8,     L"Save State 8"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_9},                ID_FILES_SAVESTATE_9,     L"Save State 9"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_0},                ID_FILES_SAVESTATE_10,    L"Save State 10"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F1},               ID_FILES_SAVESTATE_11,    L"Save State 11"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F2},               ID_FILES_SAVESTATE_12,    L"Save State 12"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F3},               ID_FILES_SAVESTATE_13,    L"Save State 13"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F4},               ID_FILES_SAVESTATE_14,    L"Save State 14"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F5},               ID_FILES_SAVESTATE_15,    L"Save State 15"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F6},               ID_FILES_SAVESTATE_16,    L"Save State 16"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F7},               ID_FILES_SAVESTATE_17,    L"Save State 17"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F8},               ID_FILES_SAVESTATE_18,    L"Save State 18"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F9},               ID_FILES_SAVESTATE_19,    L"Save State 19"},
    {{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F10},              ID_FILES_SAVESTATE_20,    L"Save State 20"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_RAM_SEARCH,            L"Ram Search"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_RAM_WATCH,             L"Ram Watch"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_INPUT_HOTKEYS,         L"Configure Hotkeys"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_INPUT_INPUTS,          L"Configure Game Input"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SWITCH_TO_TASEE,       L"Switch to Game Window"},
    {{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SWITCH_TO_TASER,       L"Switch to Hourglass Window"}
};
// clang-format on

std::map<SingleInput, SingleInput> InputCapture::inputMapping;
std::map<SingleInput, WORD> InputCapture::eventMapping;

extern HWND HotkeyHWnd;

InputCapture::InputCapture()
{
    lpDIDMouse = NULL;
    lpDIDKeyboard = NULL;
    lpDI = NULL;

    hotkeysbox = NULL;
    gameinputbox = NULL;

    // Since these are static, we want to only init them once, even though we allow more than once instance of the class.
    if (eventMapping.empty())
    {
        BuildDefaultEventMapping();
    }
    if (inputMapping.empty())
    {
        BuildDefaultInputMapping();
    }

    memset(oldKeys, 0, DI_KEY_NUMBER);
}

InputCapture::InputCapture(char* filename) // Construct by loading from file.
{
    lpDIDMouse = NULL;
    lpDIDKeyboard = NULL;
    lpDI = NULL;

    hotkeysbox = NULL;
    gameinputbox = NULL;

    if (filename != NULL && filename[0] != '\0')
    {
        LoadMapping(filename);
    }

    // In case the file only contains one map, or if filename was NULL, we fill out the maps with the defaults. Wise?
    if (eventMapping.empty())
    {
        BuildDefaultEventMapping();
    }
    if (inputMapping.empty())
    {
        BuildDefaultInputMapping();
    }

    memset(oldKeys, 0, DI_KEY_NUMBER);
}

bool InputCapture::IsModifier(SHORT key)
{
    for (int i = 0; i < 8; i++)
    {
        ModifierKey mkey = modifierKeys[i]; // Damn, no foreach structure in c++...
        if (mkey.DIK == key)
            return true;
    }
    return false;
}

char InputCapture::BuildModifier(unsigned char* keys)
{
    char modifiers = 0;
    for (int i = 0; i < 8; i++)
    {
        ModifierKey mkey = modifierKeys[i];
        if (keys[mkey.DIK] & DI_KEY_PRESSED_FLAG)
        {
            modifiers |= mkey.flag;
        }
    }
    return modifiers;
}

void InputCapture::InputToDescription(SingleInput& si) // TODO: Make this better!
{
    si.description[0] = L'\0'; // Make sure description is cleared.
    if (si.device == SINGLE_INPUT_DI_KEYBOARD)
    {
        char modifier = (si.key >> 8);

        if (modifier)
        {
            bool RControl = false;
            bool LControl = false;
            bool LShift = false;
            bool RShift = false;
            bool LAlt = false;
            bool RAlt = false;
            bool LWin = false;
            bool RWin = false;

            if (modifier & modifierKeys[0].flag)
                LControl = true;
            if (modifier & modifierKeys[1].flag)
                RControl = true;
            if (modifier & modifierKeys[2].flag)
                LShift = true;
            if (modifier & modifierKeys[3].flag)
                RShift = true;
            if (modifier & modifierKeys[4].flag)
                LAlt = true;
            if (modifier & modifierKeys[5].flag)
                RAlt = true;
            if (modifier & modifierKeys[6].flag)
                LWin = true;
            if (modifier & modifierKeys[7].flag)
                RWin = true;

            if (LControl && RControl)
                wcscat(si.description, L"Ctrl+");
            else if (LControl)
                wcscat(si.description, L"LCtrl+");
            else if (RControl)
                wcscat(si.description, L"RCtrl+");
            if (LShift && RShift)
                wcscat(si.description, L"Shift+");
            else if (LShift)
                wcscat(si.description, L"LShift+");
            else if (RShift)
                wcscat(si.description, L"RShift+");
            if (LAlt && RAlt)
                wcscat(si.description, L"Alt+");
            else if (LAlt)
                wcscat(si.description, L"LAlt+");
            else if (RAlt)
                wcscat(si.description, L"RAlt+");
            if (LWin && RWin)
                wcscat(si.description, L"WinKey+");
            else if (LWin)
                wcscat(si.description, L"LWinKey+");
            else if (RWin)
                wcscat(si.description, L"RWinKey+");
        }

        for (int i = 0; i < SICount; i++)
        {
            if (SIList[i].key == (si.key & 0x00FF))
            {
                wcscat(si.description, SIList[i].description);
                return;
            }
        }
    }
    // TODO: The other devices
    return;
}

void InputCapture::GetKeyboardState(unsigned char* keys)
{
    HRESULT rval = lpDIDKeyboard->GetDeviceState(DI_KEY_NUMBER, keys);

    if ((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
    {
        lpDIDKeyboard->Acquire();
        rval = lpDIDKeyboard->GetDeviceState(256, keys);
        if ((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
            // We couldn't get the state of the keyboard. Let's just say nothing was pressed.
            memset(keys, 0, DI_KEY_NUMBER);
    }
}

void InputCapture::GetMouseState(DIMOUSESTATE* mouse)
{
    HRESULT rval = lpDIDMouse->GetDeviceState(sizeof(DIMOUSESTATE), mouse);

    if ((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
    {
        lpDIDMouse->Acquire();
        rval = lpDIDMouse->GetDeviceState(sizeof(DIMOUSESTATE), mouse);
        if ((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
            // We couldn't get the state of the mouse. Let's just say nothing was pressed.
            memset(mouse, 0, sizeof(DIMOUSESTATE));
    }
}

bool InputCapture::InitInputs(HINSTANCE hInst, HWND hWnd)
{
    // Init the main DI interface.
    HRESULT rval = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpDI, NULL);
    if (rval != DI_OK)
    {
        MessageBoxW(hWnd,
                    L"DirectInput failed... You must have at least DirectX 5",
                    L"Error",
                    MB_OK);
        return false;
    }

    // Try to init a keyboard.
    rval = InitDIKeyboard(hWnd);

    // Is a keyboard mandatory ?
    if (rval != DI_OK)
    {
        MessageBoxW(hWnd, L"I couldn't find any keyboard here", L"Error", MB_OK);
        return false;
    }

    // Try to init a mouse.
    //rval = InitDIMouse(hWnd);

    return true;
}

HRESULT InputCapture::InitDIKeyboard(HWND hWnd)
{
    HRESULT rval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDKeyboard, NULL);
    if (rval != DI_OK)
        return rval;

    rval = lpDIDKeyboard->SetCooperativeLevel(hWnd,
                                              DISCL_NONEXCLUSIVE
                                                  | (/*BackgroundInput*/ true ? DISCL_BACKGROUND
                                                                              : DISCL_FOREGROUND));
    if (rval != DI_OK)
        return rval;

    rval = lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
    if (rval != DI_OK)
        return rval;

    // Copy from original code. Why is there a loop here ???
    for (int i = 0; i < 10; i++)
    {
        rval = lpDIDKeyboard->Acquire();
        if (rval == DI_OK)
            break;
        Sleep(10);
    }

    return rval;
}

// Init a DI mouse if possible
HRESULT InputCapture::InitDIMouse(HWND hWnd, bool exclusive)
{
    HRESULT rval = lpDI->CreateDevice(GUID_SysMouse, &lpDIDMouse, NULL);
    if (rval != DI_OK)
        return rval;

    // FIXME: This is not good!
    if (exclusive)
        //rval = lpDIDMouse->SetCooperativeLevel(NULL, DISCL_NONEXCLUSIVE|DISCL_BACKGROUND);
        rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    else
        rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    if (rval != DI_OK)
        return rval;

    rval = lpDIDMouse->SetDataFormat(&c_dfDIMouse);
    if (rval != DI_OK)
        return rval;

    rval = lpDIDMouse->Acquire();
    return rval;
}

void InputCapture::ReleaseInputs()
{
    if (lpDI)
    {
        if (lpDIDMouse)
        {
            lpDIDMouse->Release();
            lpDIDMouse = NULL;
        }

        if (lpDIDKeyboard)
        {
            lpDIDKeyboard->Release();
            lpDIDKeyboard = NULL;
        }

        lpDI->Release();
        lpDI = NULL;
    }
}

// TODO: Put this somewhere else ?
unsigned char convertDIKToVK(unsigned char DIK, HKL keyboardLayout)
{
    unsigned char VK = MapVirtualKeyEx(DIK, /*MAPVK_VSC_TO_VK_EX*/ 3, keyboardLayout) & 0xFF;

    // unfortunately MapVirtualKeyEx is slightly broken, so patch up the results ourselves...
    // (note that some of the left/right modifier keys get lost too despite MAPVK_VSC_TO_VK_EX)
    switch (DIK)
    {
    case DIK_LEFT:
        VK = VK_LEFT;
        break;
    case DIK_RIGHT:
        VK = VK_RIGHT;
        break;
    case DIK_UP:
        VK = VK_UP;
        break;
    case DIK_DOWN:
        VK = VK_DOWN;
        break;
    case DIK_PRIOR:
        VK = VK_PRIOR;
        break;
    case DIK_NEXT:
        VK = VK_NEXT;
        break;
    case DIK_HOME:
        VK = VK_HOME;
        break;
    case DIK_END:
        VK = VK_END;
        break;
    case DIK_INSERT:
        VK = VK_INSERT;
        break;
    case DIK_DELETE:
        VK = VK_DELETE;
        break;
    case DIK_DIVIDE:
        VK = VK_DIVIDE;
        break;
    case DIK_NUMLOCK:
        VK = VK_NUMLOCK;
        break;
    case DIK_LWIN:
        VK = VK_LWIN;
        break;
    case DIK_RWIN:
        VK = VK_RWIN;
        break;
    case DIK_RMENU:
        VK = VK_RMENU;
        break;
    case DIK_RCONTROL:
        VK = VK_RCONTROL;
        break;
    // these work for me, but are here in case other layouts need them
    case DIK_RSHIFT:
        VK = VK_RSHIFT;
        break;
    case DIK_LMENU:
        VK = VK_LMENU;
        break;
    case DIK_LCONTROL:
        VK = VK_LCONTROL;
        break;
    case DIK_LSHIFT:
        VK = VK_LSHIFT;
        break;
    }
    return VK;
}

unsigned char convertDIKToVK(unsigned char DIK)
{
    HKL keyboardLayout = GetKeyboardLayout(0);
    return convertDIKToVK(DIK, keyboardLayout);
}

void InputCapture::ProcessInputs(CurrentInput* currentI, HWND hWnd)
{
    // We first clear the CurrentInput object.
    currentI->clear();

    // Get the current keyboard state.
    unsigned char keys[DI_KEY_NUMBER];
    GetKeyboardState(keys);

    DIMOUSESTATE mouseState;
    // If a mouse is attached, get it's state.
    if (lpDIDMouse != NULL)
    {
        if (hWnd == NULL)
        { // Very bad hack. We must not gather mouse inputs if we only deal with events
            GetMouseState(&mouseState);

            // We can directly copy the axis states as it is not mappable.
            currentI->mouse.di.lX = mouseState.lX;
            currentI->mouse.di.lY = mouseState.lY;
            currentI->mouse.di.lZ = mouseState.lZ;

            // Get the absolute coords as well.
            GetCursorPos(&currentI->mouse.coords);
        }
    }
    else
    {
        memset(&mouseState, 0, sizeof(DIMOUSESTATE));
    }

    // Bulding the modifier from the array of pressed keys.
    unsigned short modifier = BuildModifier(keys);

    // We want now to convert the initial inputs to their mapping.
    // There are two mappings: inputs and events.

    /** Keyboard **/
    for (SHORT k = 1; k < DI_KEY_NUMBER; k++)
    {
        // If k is not pressed, we skip to the next key.
        if (!DI_KEY_PRESSED(keys[k]))
            continue;

        // Now we build the SingleInput, and check if it's mapped to something.
        SingleInput siPressed = {SINGLE_INPUT_DI_KEYBOARD, k, L""};

        /* Input mapping */
        std::map<SingleInput, SingleInput>::iterator iterI = inputMapping.find(siPressed);
        if (iterI != inputMapping.end())
        {
            // We found a mapping. We need to flag the key in the CurrentInput struct.
            // As wintasee is dealing with a VK-indexed array, we are doing the conversion here.
            SingleInput siMapped = iterI->second;
            if (siMapped.device == SINGLE_INPUT_DI_KEYBOARD)
                currentI->keys[convertDIKToVK(static_cast<unsigned char>(siMapped.key))] = 1;
            if (siMapped.device == SINGLE_INPUT_DI_MOUSE)
                currentI->mouse.di.rgbButtons[siMapped.key] |= 0x80;
            if (siMapped.device == SINGLE_INPUT_XINPUT_JOYSTICK)
            {
                currentI->gamepad[siMapped.key >> 8].wButtons |= 1 << (siMapped.key & 0xFF);
                DebugLog() << "currentI->gamepad[" << (siMapped.key >> 8)
                           << "].wButtons |= " << (1 << (siMapped.key & 0xFF));
            }
        }

        if (!hWnd)
            continue; // I don't want to process events!

        /* Event mapping */

        // We also have to check that the key was just pressed.
        if (DI_KEY_PRESSED(oldKeys[k]))
            continue;

        // If k is a modifier, we don't process it.
        if (IsModifier(k))
            continue;

        // We build the SingleInput with modifiers this time, and check if it's mapped to something.
        SingleInput siPressedMod = {SINGLE_INPUT_DI_KEYBOARD, (modifier << 8) | k, L""};

        std::map<SingleInput, WORD>::iterator iterE = eventMapping.find(siPressedMod);
        if (iterE != eventMapping.end())
        { // There is something.
            WORD eventId = iterE->second;
            // Now we just have to send the corresponding message.
            SendMessage(hWnd, WM_COMMAND, eventId, 777); // TODO: Doc what is 777.
        }
    }

    // Computing the previousKeys for the next call.
    // We assume that this function can be called multiple times within a single frame.
    // But only one call per frame is processing the events.
    if (hWnd)
    {
        memmove(oldKeys, keys, DI_KEY_NUMBER);
    }

    /** Mouse **/
    for (int i = 0; i < 4; i++)
    {
        // If mouse button i is not pressed, we skip to the next button.
        if (!DI_KEY_PRESSED(mouseState.rgbButtons[i]))
            continue;

        // Now we build the SingleInput, and check if it's mapped to something.
        SingleInput siPressed = {SINGLE_INPUT_DI_MOUSE, static_cast<SHORT>(i), L""};

        //TODO: Duplicate code !!!

        /* Input mapping */
        std::map<SingleInput, SingleInput>::iterator iterI = inputMapping.find(siPressed);
        if (iterI != inputMapping.end())
        {
            // We found a mapping. We need to flag the key in the CurrentInput struct.
            // As wintasee is dealing with a VK-indexed array, we are doing the conversion here.
            SingleInput siMapped = iterI->second;
            if (siMapped.device == SINGLE_INPUT_DI_KEYBOARD)
                currentI->keys[convertDIKToVK((unsigned char) siMapped.key)] = 1;
            if (siMapped.device == SINGLE_INPUT_DI_MOUSE)
                currentI->mouse.di.rgbButtons[siMapped.key] |= 0x80;
            if (siMapped.device == SINGLE_INPUT_XINPUT_JOYSTICK)
                currentI->gamepad[siMapped.key >> 8].wButtons |= siMapped.key & 0xFF;
        }
    }

    // TODO: Joystick, etc.
}

void InputCapture::NextInput(SingleInput* si, bool allowModifiers)
{
    unsigned char previousKeys[DI_KEY_NUMBER];
    unsigned char currentKeys[DI_KEY_NUMBER];

    DIMOUSESTATE previousMouse;
    DIMOUSESTATE currentMouse;

    // Get the previous keyboard state.
    GetKeyboardState(previousKeys);

    // Get the previous mouse state.
    if (lpDIDMouse != NULL)
        GetMouseState(&previousMouse);

    Sleep(1);

    while (true)
    { // Set a timeout?

        // Get the current keyboard and mouse state.
        GetKeyboardState(currentKeys);
        if (lpDIDMouse != NULL)
            GetMouseState(&currentMouse);

        char modifier = BuildModifier(currentKeys);

        // Try to find a key that was just pressed.
        for (int i = 0; i < DI_KEY_NUMBER; i++)
        {
            if (allowModifiers && IsModifier(i))
                continue;

            if (!DI_KEY_PRESSED(previousKeys[i]) && DI_KEY_PRESSED(currentKeys[i]))
            {
                // We found a just-pressed key. We need to return the SingleInput
                si->device = SINGLE_INPUT_DI_KEYBOARD;
                if (allowModifiers)
                {
                    si->key = (((short) modifier) << 8) | i;
                }
                else
                {
                    si->key = i;
                }
                InputToDescription(*si);
                return;
            }
        }

        // Try to find a mouse button that was just pressed. We can only map mouse buttons, not axes.
        for (int i = 0; i < 4; i++)
        {
            if (!DI_KEY_PRESSED(previousMouse.rgbButtons[i])
                && DI_KEY_PRESSED(currentMouse.rgbButtons[i]))
            {
                // We found a just-pressed mouse button. We need to return the SingleInput
                si->device = SINGLE_INPUT_DI_MOUSE;
                si->key = i;
                swprintf(si->description, ARRAYSIZE(si->description), L"Mouse button %d", i);
                return;
            }
        }

        // TODO: Try to find a joystick button that was just pressed.

        memcpy(previousKeys, currentKeys, DI_KEY_NUMBER);
        memcpy(&previousMouse, &currentMouse, sizeof(DIMOUSESTATE));
        Sleep(1);
    }
}

void InputCapture::RemoveValueFromInputMap(const SingleInput* si)
{
    for (std::map<SingleInput, SingleInput>::iterator iter = inputMapping.begin();
         iter != inputMapping.end();
         ++iter)
    {
        SingleInput fromInput = iter->first;
        SingleInput toInput = iter->second;
        if (!(*si < toInput) && !(toInput < *si))
        { // if (*si == toInput)
            inputMapping.erase(fromInput);
            return;
        }
    }
}

void InputCapture::RemoveValueFromEventMap(const WORD eventId)
{
    for (std::map<SingleInput, WORD>::iterator iter = eventMapping.begin();
         iter != eventMapping.end();
         ++iter)
    {
        SingleInput fromInput = iter->first;
        WORD toEventId = iter->second;
        if (eventId == toEventId)
        {
            eventMapping.erase(fromInput);
            return;
        }
    }
}

void InputCapture::ReassignInput(int SIListIndex)
{
    SingleInput fromSI;
    SingleInput toSI = SIList[SIListIndex];

    // Gather the next input.
    NextInput(&fromSI, false);

    // Remove the previous mapping if any.
    RemoveValueFromInputMap(&toSI);

    // Map the retreived input pressed to the element of the SIList array.
    inputMapping[fromSI] = toSI;
}

void InputCapture::DefaultInput(int SIListIndex)
{
    SingleInput si = SIList[SIListIndex];
    RemoveValueFromInputMap(&si);
    inputMapping[si] = si;
}

void InputCapture::DisableInput(int SIListIndex)
{
    SingleInput si = SIList[SIListIndex];
    RemoveValueFromInputMap(&si);
}

void InputCapture::ReassignEvent(int eventListIndex)
{
    SingleInput fromSI;
    Event ev = eventList[eventListIndex];

    // Gather the next input.
    NextInput(&fromSI, true);

    // Remove the previous mapping if any.
    RemoveValueFromEventMap(ev.id);

    // Map the retreived input pressed to the element of the eventList array.
    eventMapping[fromSI] = ev.id;
}

void InputCapture::DefaultEvent(int eventListIndex)
{
    Event ev = eventList[eventListIndex];
    RemoveValueFromEventMap(ev.id);
    InputToDescription(ev.defaultInput); // Building here the description, quite strange...
    eventMapping[ev.defaultInput] = ev.id;
}

void InputCapture::DisableEvent(int eventListIndex)
{
    Event ev = eventList[eventListIndex];
    RemoveValueFromEventMap(ev.id);
}

// Build (or restore to) a default input mapping.
void InputCapture::BuildDefaultInputMapping()
{
    inputMapping.clear();
    for (int i = 0; i < SICount; i++)
    {
        inputMapping[SIList[i]] = SIList[i]; // By default, an input is mapped to itself.
    }
}

// Build (or restore to) a default event mapping.
void InputCapture::BuildDefaultEventMapping()
{
    eventMapping.clear();
    for (int i = 0; i < eventCount; i++)
    {
        InputToDescription(
            eventList[i].defaultInput); // Building here the description, quite strange...
        eventMapping[eventList[i].defaultInput] =
            eventList[i].id; // The default mapping in stored inside the eventList.
    }
}

std::wstring InputCapture::FormatInputMapping(int index)
{
    std::wstring line;

    SingleInput* si = &SIList[index];
    line = si->description;
    line += L'\t';

    for (const auto& input : inputMapping)
    {
        const SingleInput& fromInput = input.first;
        const SingleInput& toInput = input.second;

        if (!(*si < toInput) && !(toInput < *si))
        { // if (*si == toInput)
            line += fromInput.description;
            break;
        }
    }

    return line;
}

std::wstring InputCapture::FormatEventMapping(int index)
{
    std::wstring line;

    Event* ev = &eventList[index];
    line = ev->description;
    line += L'\t';

    for (const auto& event : eventMapping)
    {
        const SingleInput& fromInput = event.first;
        WORD toEventId = event.second;

        if (ev->id == toEventId)
        {
            line += fromInput.description;
            break;
        }
    }

    return line;
}

// Save current mapping into a config file
void InputCapture::SaveMapping(char* filename)
{
    //TODO. Use WritePrivateProfileStringA
}

// Load mapping from a config file
void InputCapture::LoadMapping(char* filename)
{
    // TODO. Use GetPrivateProfileSectionA function?
}

LRESULT CALLBACK InputCapture::ConfigureInput(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //RECT r;
    //RECT r2;
    //int dx1, dy1, dx2, dy2;
    static InputCapture* inputC = NULL;
    static bool unsavedChanges = false;
    static bool hotkeysNoLongerDefault = false;
    static bool gameinputNoLongerDefault = false;
    //static HWND Tex0 = NULL;
    //extern HWND hWnd; // Same comment!

    EnableWindow(GetDlgItem(hDlg, IDC_CONF_SAVE), unsavedChanges ? TRUE : FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_CONF_RESTOREDEFHKS), hotkeysNoLongerDefault ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_CONF_RESTOREDEFGIS), gameinputNoLongerDefault ? TRUE : FALSE);

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        //GetWindowRect(hWnd, &r);
        //dx1 = (r.right - r.left) / 2;
        //dy1 = (r.bottom - r.top) / 2;

        //GetWindowRect(hDlg, &r2);
        //dx2 = (r2.right - r2.left) / 2;
        //dy2 = (r2.bottom - r2.top) / 2;

        //SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        //Tex0 = GetDlgItem(hDlg, IDC_STATIC_TEXT0);

        // We create the input interface for the dialog.
        inputC = new InputCapture();
        // GetModuleHandle returns the HMODULE of the calling process (Hourglass.exe), for this case the HMODULE is the same as the HINSTANCE.
        // IF this fails we can probably use GetWindowLong instead.
        HINSTANCE hInstProcess = (HINSTANCE) GetModuleHandleW(NULL);
        if (hInstProcess == NULL)
        {
            PrintLastError(L"InputConfig: GetModuleHandle", GetLastError());
            return FALSE;
        }
        if (!(inputC->InitInputs(hInstProcess, hDlg)))
        {
            return FALSE;
        }
        inputC->InitDIMouse(hDlg, false);

        /*HWND hotkeys*/ inputC->hotkeysbox = GetDlgItem(hDlg, IDC_HOTKEYBOX);
        /*HWND gameinput*/ inputC->gameinputbox = GetDlgItem(hDlg, IDC_GAMEINPUTBOX);

        int stops[4] = {25 * 4, 25 * 4 + 4, 25 * 4 + 8, 25 * 4 + 16};
        SendMessageW(inputC->hotkeysbox, LB_SETTABSTOPS, 4, reinterpret_cast<LPARAM>(stops));
        SendMessageW(inputC->gameinputbox, LB_SETTABSTOPS, 4, reinterpret_cast<LPARAM>(stops));

        inputC->PopulateListbox(inputC->hotkeysbox);
        inputC->PopulateListbox(inputC->gameinputbox);

        return TRUE;
    }
    break;

    case WM_COMMAND:
    {
        int controlID = LOWORD(wParam);
        int messageID = HIWORD(wParam);

        if (messageID == LBN_SELCHANGE)
        {
            int selCount =
                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
            // Nothing marked in the HOTKEY box? Let's check the GAMEINPUT box.
            selCount = selCount == 0 ? SendDlgItemMessageW(hDlg,
                                                           IDC_GAMEINPUTBOX,
                                                           LB_GETSELCOUNT,
                                                           (WPARAM) 0,
                                                           (LPARAM) 0)
                                     : selCount;

            EnableWindow(GetDlgItem(hDlg, IDC_CONF_ASSIGNKEY), (selCount == 1) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CONF_USEDEFAULT), (selCount >= 1) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CONF_DISABLE), (selCount >= 1) ? TRUE : FALSE);
        }

        switch (controlID)
        {
        case IDC_CONF_RESTOREDEFHKS:
        {
            HRESULT rv =
                CustomMessageBox(L"Restoring the HotKeys to default will not be undoable by "
                                 L"closing the Config window.\n\nDo you wish to continue?",
                                 L"Warning!",
                                 MB_YESNO | MB_ICONWARNING);
            if (rv == IDYES)
            {
                inputC->BuildDefaultEventMapping();
                unsavedChanges = false;
                hotkeysNoLongerDefault = false;
                inputC->PopulateListbox(inputC->hotkeysbox);
            }
        }
        break;
        case IDC_CONF_RESTOREDEFGIS:
        {
            HRESULT rv =
                CustomMessageBox(L"Restoring the Game Input keys to default will not be undoable "
                                 L"by closing the Config window.\n\nDo you wish to continue?",
                                 L"Warning!",
                                 MB_YESNO | MB_ICONWARNING);
            if (rv == IDYES)
            {
                inputC->BuildDefaultInputMapping();
                unsavedChanges = false;
                gameinputNoLongerDefault = false;
                inputC->PopulateListbox(inputC->gameinputbox);
            }
        }
        break;
        case IDC_CONF_ASSIGNKEY: // TODO: Turn some of the code below into function calls.
        {
            int buf[1]; // Necessary, function crashes if you send a pointer to a simple varialbe.

            //SingleInput si; // FIXME: Why is this unused?

            // Check if the selection happened in HotKeys
            if (SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_GETSELITEMS, 1, (LPARAM) buf))
            {
                inputC->ReassignEvent(buf[0]);

                std::wstring line = inputC->FormatEventMapping(buf[0]);

                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_DELETESTRING, buf[0], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_HOTKEYBOX,
                                    LB_INSERTSTRING,
                                    buf[0],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                hotkeysNoLongerDefault = true;
            }
            else if (SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_GETSELITEMS, 1, (LPARAM) buf))
            {
                inputC->ReassignInput(buf[0]);

                std::wstring line = inputC->FormatInputMapping(buf[0]);

                SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_DELETESTRING, buf[0], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_GAMEINPUTBOX,
                                    LB_INSERTSTRING,
                                    buf[0],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                gameinputNoLongerDefault = true;
            }
            //else: Focus lost, that is bad. TODO: Make sure selection is locked until key has been set.

            unsavedChanges = true;
        }
        break;
        case IDC_CONF_USEDEFAULT: // TODO: Check if restoring the defaults for selection actually gets you the defaults back
        {
            int buf[256];
            int returned =
                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_GETSELITEMS, 256, (LPARAM) buf);
            for (int i = 0; i < returned; i++)
            {
                inputC->DefaultEvent(buf[i]);

                std::wstring line = inputC->FormatEventMapping(buf[i]);

                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_DELETESTRING, buf[i], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_HOTKEYBOX,
                                    LB_INSERTSTRING,
                                    buf[i],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                hotkeysNoLongerDefault = true;
            }
            returned =
                SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_GETSELITEMS, 256, (LPARAM) buf);
            for (int i = 0; i < returned; i++)
            {
                inputC->DefaultInput(buf[i]);

                std::wstring line = inputC->FormatInputMapping(buf[i]);

                SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_DELETESTRING, buf[i], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_GAMEINPUTBOX,
                                    LB_INSERTSTRING,
                                    buf[i],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                gameinputNoLongerDefault = true;
            }
            unsavedChanges = true;
        }
        break;
        case IDC_CONF_DISABLE:
        {
            int buf[256];
            int returned =
                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_GETSELITEMS, 256, (LPARAM) buf);
            for (int i = 0; i < returned; i++)
            {
                inputC->DisableEvent(buf[i]);

                std::wstring line = inputC->FormatEventMapping(buf[i]);

                SendDlgItemMessageW(hDlg, IDC_HOTKEYBOX, LB_DELETESTRING, buf[i], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_HOTKEYBOX,
                                    LB_INSERTSTRING,
                                    buf[i],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                hotkeysNoLongerDefault = true;
            }
            returned =
                SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_GETSELITEMS, 256, (LPARAM) buf);
            for (int i = 0; i < returned; i++)
            {
                inputC->DisableInput(buf[i]);

                std::wstring line = inputC->FormatInputMapping(buf[i]);

                SendDlgItemMessageW(hDlg, IDC_GAMEINPUTBOX, LB_DELETESTRING, buf[i], NULL);
                SendDlgItemMessageW(hDlg,
                                    IDC_GAMEINPUTBOX,
                                    LB_INSERTSTRING,
                                    buf[i],
                                    reinterpret_cast<LPARAM>(line.c_str()));

                gameinputNoLongerDefault = true;
            }
            unsavedChanges = true;
        }
        break;
        case IDC_CONF_SAVE:
        {
            //inputC->SaveMapping(filename);
            unsavedChanges = false;
        }
        break;
        case IDC_CONF_CLOSE:
        {
            // Go to the WM_CLOSE case instead.
            SendMessage(hDlg, WM_CLOSE, NULL, NULL);
        }
        break;
        }
    }
    break;
    case WM_CLOSE:
    {
        inputC->ReleaseInputs();
        delete inputC;
        inputC = NULL;
        EndDialog(hDlg, true);
        HotkeyHWnd = NULL;
        return TRUE;
    }
    break;
    }

    return FALSE;
}

void InputCapture::PopulateListbox(HWND listbox)
{
    // Make sure the listbox is empty before we populate it.
    SendMessageW(listbox, LB_RESETCONTENT, NULL, NULL);

    int inputNumber = 0;
    if (listbox == hotkeysbox)
        inputNumber = eventCount;
    else
        inputNumber = SICount;

    for (int i = 0; i < inputNumber; i++)
    {
        std::wstring line;

        // Get the map strings.
        if (listbox == hotkeysbox)
        {
            line = FormatEventMapping(i);
        }
        else
        {
            line = FormatInputMapping(i);
        }

        // Send it.
        SendMessageW((HWND) listbox,
                     (UINT) LB_ADDSTRING,
                     (WPARAM) 0,
                     reinterpret_cast<LPARAM>(line.c_str()));
    }
}
