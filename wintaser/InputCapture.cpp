#include "InputCapture.h"
#include "Resource.h"

struct ModifierKey InputCapture::modifierKeys[] = 
{
	{DIK_LCONTROL, 0x1},
	{DIK_RCONTROL, 0x2},
	{DIK_LSHIFT,   0x4},
	{DIK_RSHIFT,   0x8},
	{DIK_LMENU,    0x10},
	{DIK_RMENU,    0x20},
	{DIK_LWIN,     0x40},
	{DIK_RWIN,     0x80}
};


struct SingleInput InputCapture::SIList[] =
{
	{SINGLE_INPUT_DI_KEYBOARD, DIK_ESCAPE, "Escape"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_1, "1"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_2, "2"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_3, "3"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_4, "4"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_5, "5"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_6, "6"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_7, "7"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_8, "8"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_9, "9"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_0, "0"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MINUS, "-_"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_EQUALS, "=+"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_BACK, "Backspace"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_TAB, "Tab"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_Q, "Q"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_W, "W"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_E, "E"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_R, "R"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_T, "T"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_Y, "Y"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_U, "U"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_I, "I"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_O, "O"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_P, "P"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LBRACKET, "[{"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RBRACKET, "]}"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RETURN, "Enter"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LCONTROL, "LControl"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_A, "A"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_S, "S"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_D, "D"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F, "F"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_G, "G"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_H, "H"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_J, "J"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_K, "K"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_L, "L"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SEMICOLON, "},:"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_APOSTROPHE, "'\""},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_GRAVE, "`~"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LSHIFT, "LShift"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_BACKSLASH, "\\|"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_Z, "Z"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_X, "X"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_C, "C"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_V, "V"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_B, "B"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_N, "N"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_M, "M"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_COMMA, ",<"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_PERIOD, ".>"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SLASH, "/?"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RSHIFT, "RShift"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MULTIPLY, "Numpad*"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LMENU, "LAlt"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SPACE, "Space"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_CAPITAL, "CapsLock"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F1, "F1"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F2, "F2"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F3, "F3"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F4, "F4"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F5, "F5"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F6, "F6"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F7, "F7"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F8, "F8"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F9, "F9"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F10, "F10"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMLOCK, "NumLock"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SCROLL, "ScrollLock"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD7, "Numpad7"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD8, "Numpad8"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD9, "Numpad9"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SUBTRACT, "Numpad-"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD4, "Numpad4"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD5, "Numpad5"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD6, "Numpad6"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_ADD, "Numpad+"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD1, "Numpad1"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD2, "Numpad2"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD3, "Numpad3"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPAD0, "Numpad0"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_DECIMAL, "Numpad."},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_OEM_102, "<>\\|"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F11, "F11"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F12, "F12"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F13, "F13"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F14, "F14"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_F15, "F15"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_KANA, "Kana"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_ABNT_C1, "/?"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_CONVERT, "Convert"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NOCONVERT, "NoConvert"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_YEN, "Yen"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_ABNT_C2, "Numpad."},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADEQUALS, "Numpad="},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_PREVTRACK, "Prevtrack"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_AT, "@"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_COLON, ":"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_UNDERLINE, "_"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_KANJI, "Kanji"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_STOP, "Stop"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_AX, "AX"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_UNLABELED, "Unlabeled"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NEXTTRACK, "Nexttrack"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADENTER, "NumpadEnter"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RCONTROL, "RControl"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MUTE, "Mute"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_CALCULATOR, "Calculator"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_PLAYPAUSE, "PlayPause"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MEDIASTOP, "MediaStop"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_VOLUMEDOWN, "VolumeDown"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_VOLUMEUP, "VolumeUp"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBHOME, "WebHome"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NUMPADCOMMA, "Numpad,"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_DIVIDE, "Numpad/"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SYSRQ, "Sysrq"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RMENU, "RAlt"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_PAUSE, "Pause"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_HOME, "Home"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_UP, "Up"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_PRIOR, "PageUp"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LEFT, "Left"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RIGHT, "Right"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_END, "End"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_DOWN, "Down"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_NEXT, "PageDown"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_INSERT, "Insert"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_DELETE, "Delete"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_LWIN, "LWin"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_RWIN, "RWin"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_APPS, "Apps"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_POWER, "Power"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_SLEEP, "Sleep"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WAKE, "Wake"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBSEARCH, "WebSearch"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBFAVORITES, "WebFavorites"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBREFRESH, "WebRefresh"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBSTOP, "WebStop"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBFORWARD, "WebForward"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_WEBBACK, "WebBack"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MYCOMPUTER, "MyComputer"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MAIL, "Mail"},
	{SINGLE_INPUT_DI_KEYBOARD, DIK_MEDIASELECT, "MediaSelect"}
};

struct Event InputCapture::eventList[] =
{
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_T},              ID_TOGGLE_MOVIE_READONLY,  "Toggle Movie Read-Only"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_O},              ID_FILES_OPENEXE,          "Open Executable"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_M},              ID_FILES_OPENMOV,          "Open Movie"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_P},              ID_FILES_PLAYMOV,          "Run and Play"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_W},              ID_FILES_WATCHMOV,         "Watch from Beginning"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_R},              ID_FILES_RECORDMOV,        "Start Recording To"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_RESUMERECORDING,  "Resume Recording From Now"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_BACKUPMOVIE,      "Backup Movie"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL | DIK_LSHIFT*/ 0x500 | DIK_C}, ID_FILES_STOP_RELAY,       "Stop Running"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_FILES_QUIT,             "Exit " /*"winTASer"*/ "Hourglass"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_ESCAPE},                                  ID_TIME_TOGGLE_PAUSE,      "Pause/Unpause"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_DELETE},                                  ID_TIME_TOGGLE_PAUSE,      "Pause/Unpause (key 2)"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_PAUSE},                                   ID_TIME_TOGGLE_PAUSE,      "Pause/Unpause (key 3)"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_TIME_TOGGLE_FASTFORWARD,"Toggle Fast-Forward"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_TAB},                                     ID_SWITCH_TO_TASEE_FROM_TASER, "Fast Forward"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_OEM_102},                                 ID_SWITCH_TO_TASEE_FROM_TASER, "Frame Advance"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_SPACE},                                   ID_SWITCH_TO_TASEE_FROM_TASER, "Frame Advance (key 2)"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_MINUS},          ID_TIME_RATE_SLOWER,      "Decrease Speed"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LCONTROL*/ 0x100 | DIK_EQUALS},         ID_TIME_RATE_FASTER,      "Increase Speed"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_TIME_RATE_100,         "Normal Speed"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SOUND_NOPLAYBUFFERS,   "Disable/Enable Sound"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_1},                                       ID_FILES_LOADSTATE_1,     "Load State 1"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_2},                                       ID_FILES_LOADSTATE_2,     "Load State 2"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_3},                                       ID_FILES_LOADSTATE_3,     "Load State 3"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_4},                                       ID_FILES_LOADSTATE_4,     "Load State 4"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_5},                                       ID_FILES_LOADSTATE_5,     "Load State 5"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_6},                                       ID_FILES_LOADSTATE_6,     "Load State 6"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_7},                                       ID_FILES_LOADSTATE_7,     "Load State 7"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_8},                                       ID_FILES_LOADSTATE_8,     "Load State 8"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_9},                                       ID_FILES_LOADSTATE_9,     "Load State 9"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_0},                                       ID_FILES_LOADSTATE_10,    "Load State 10"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F1},                                      ID_FILES_LOADSTATE_11,    "Load State 11"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F2},                                      ID_FILES_LOADSTATE_12,    "Load State 12"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F3},                                      ID_FILES_LOADSTATE_13,    "Load State 13"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F4},                                      ID_FILES_LOADSTATE_14,    "Load State 14"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F5},                                      ID_FILES_LOADSTATE_15,    "Load State 15"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F6},                                      ID_FILES_LOADSTATE_16,    "Load State 16"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F7},                                      ID_FILES_LOADSTATE_17,    "Load State 17"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F8},                                      ID_FILES_LOADSTATE_18,    "Load State 18"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F9},                                      ID_FILES_LOADSTATE_19,    "Load State 19"},
	{{SINGLE_INPUT_DI_KEYBOARD, DIK_F10},                                     ID_FILES_LOADSTATE_20,    "Load State 20"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_1},                ID_FILES_SAVESTATE_1,     "Save State 1"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_2},                ID_FILES_SAVESTATE_2,     "Save State 2"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_3},                ID_FILES_SAVESTATE_3,     "Save State 3"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_4},                ID_FILES_SAVESTATE_4,     "Save State 4"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_5},                ID_FILES_SAVESTATE_5,     "Save State 5"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_6},                ID_FILES_SAVESTATE_6,     "Save State 6"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_7},                ID_FILES_SAVESTATE_7,     "Save State 7"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_8},                ID_FILES_SAVESTATE_8,     "Save State 8"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_9},                ID_FILES_SAVESTATE_9,     "Save State 9"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_0},                ID_FILES_SAVESTATE_10,    "Save State 10"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F1},               ID_FILES_SAVESTATE_11,    "Save State 11"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F2},               ID_FILES_SAVESTATE_12,    "Save State 12"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F3},               ID_FILES_SAVESTATE_13,    "Save State 13"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F4},               ID_FILES_SAVESTATE_14,    "Save State 14"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F5},               ID_FILES_SAVESTATE_15,    "Save State 15"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F6},               ID_FILES_SAVESTATE_16,    "Save State 16"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F7},               ID_FILES_SAVESTATE_17,    "Save State 17"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F8},               ID_FILES_SAVESTATE_18,    "Save State 18"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F9},               ID_FILES_SAVESTATE_19,    "Save State 19"},
	{{SINGLE_INPUT_DI_KEYBOARD, /*DIK_LSHIFT*/ 0x400 | DIK_F10},              ID_FILES_SAVESTATE_20,    "Save State 20"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_RAM_SEARCH,            "Ram Search"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_RAM_WATCH,             "Ram Watch"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_INPUT_HOTKEYS,         "Configure Hotkeys"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_INPUT_INPUTS,          "Configure Game Input"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SWITCH_TO_TASEE,       "Switch to Game Window"},
	{{SINGLE_INPUT_DI_KEYBOARD, 0},                                           ID_SWITCH_TO_TASER,       "Switch to Hourglass Window"}
};


bool InputCapture::IsModifier(char key){
	for (int i=0; i<DI_KEY_NUMBER; i++){
		ModifierKey mkey = modifierKeys[i]; // Damn, no foreach structure in c++...
		if (mkey.DIK == key)
			return true;
	}
	return false;
}

char InputCapture::BuildModifier(char* keys){
	char modifiers = 0;
	for (int i=0; i<DI_KEY_NUMBER; i++){
		ModifierKey mkey = modifierKeys[i];
		if (keys[mkey.DIK] & DI_KEY_PRESSED_FLAG){
			modifiers |= mkey.flag;
		}
	}
	return modifiers;
}

void InputCapture::GetKeyboardState(char* keys){
	HRESULT rval = lpDIDKeyboard->GetDeviceState(256, keys);

	if((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED)){
		lpDIDKeyboard->Acquire();
		rval = lpDIDKeyboard->GetDeviceState(256, keys);
		if((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
			// We couldn't get the state of the keyboard. Let's just say nothing was pressed.
			memset(keys, 0, sizeof(keys));
	}
}

bool InputCapture::InitInputs(HINSTANCE hInst, HWND hWnd){

	// Init the main DI interface.
	HRESULT rval = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpDI, NULL);
	if(rval != DI_OK)
	{
		MessageBox(hWnd, "DirectInput failed... You must have at least DirectX 5", "Error", MB_OK);
		return false;
	}

	// Try to init a keyboard.
	rval = InitDIKeyboard(hWnd);

	// Is a keyboard mandatory ?
	if(rval != DI_OK)
	{
		MessageBox(hWnd, "I couldn't find any keyboard here", "Error", MB_OK);
		return false;
	}

	// Try to init a mouse.
	//rval = InitDIMouse(hWnd);

	return true;
}

HRESULT InputCapture::InitDIKeyboard(HWND hWnd){
	HRESULT rval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDKeyboard, NULL);
	if(rval != DI_OK) return rval;

	rval = lpDIDKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | (/*BackgroundInput*/true?DISCL_BACKGROUND:DISCL_FOREGROUND));
	if(rval != DI_OK) return rval;

	rval = lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if(rval != DI_OK) return rval;

	// Copy from original code. Why is there a loop here ???
	for(int i = 0; i < 10; i++)
	{
		rval = lpDIDKeyboard->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	return rval;
}

// Init a DI mouse if possible
HRESULT InputCapture::InitDIMouse(HWND hWnd){
	HRESULT rval = lpDI->CreateDevice(GUID_SysMouse, &lpDIDMouse, NULL);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->SetDataFormat(&c_dfDIMouse);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->Acquire();
	return rval;
}

void InputCapture::ReleaseInputs(){

	if(lpDI){
		if(lpDIDMouse){
			lpDIDMouse->Release();
			lpDIDMouse = NULL;
		}

		if(lpDIDKeyboard){
			lpDIDKeyboard->Release();
			lpDIDKeyboard = NULL;
		}

		lpDI->Release();
		lpDI = NULL;
	}
}

void InputCapture::AddKeyToKeyMapping(short modifiedKey, char destinationKey){
	SingleInput mapFrom = {SINGLE_INPUT_DI_KEYBOARD, modifiedKey};
	SingleInput mapTo = {SINGLE_INPUT_DI_KEYBOARD, (short)destinationKey};
	inputMapping[mapFrom] = mapTo; // Cool syntax!
}

// Add a map from a key+modifiers to an event.
void InputCapture::AddKeyToEventMapping(short modifiedKey, WORD eventId){
	SingleInput mapFrom = {SINGLE_INPUT_DI_KEYBOARD, modifiedKey};
	eventMapping[mapFrom] = eventId;
}

// Clear all input mappings
void InputCapture::EmptyAllInputMappings(){
	inputMapping.clear();
}

// Clear all event mappings
void InputCapture::EmptyAllEventMappings(){
	eventMapping.clear();
}

void InputCapture::ProcessInputs(CurrentInput* currentI, HWND hWnd){

	// We first clear the CurrentInput object.
	currentI->clear();

	// Get the current keyboard state.
	char keys[256];
	GetKeyboardState(keys);

	// Bulding the modifier from the array of pressed keys.
	char modifier = BuildModifier(keys);

	// We want now to convert the initial inputs to their mapping.
	// There are two mappings: inputs and events.
	// We only need to consider the inputs that are actually mapped to something.
	// So it's better to iterate through the map than through the keys array (I guess).

	// Let's start with the input map first.
	for(std::map<SingleInput,SingleInput>::iterator iter = inputMapping.begin(); iter != inputMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		SingleInput toInput = iter->second;
		// We want to map fromInput -> toInput

		switch (fromInput.device){

		case SINGLE_INPUT_DI_KEYBOARD:{

			// Mapping source is a keyboard, we need to check if the modifiers are identical, and if the key is pressed.
			char fromModifier = (char)(fromInput.key >> 8);
			char fromKey = (char)(fromInput.key & 0xFF);
			if((fromModifier == modifier) && DI_KEY_PRESSED(keys[fromKey])){
				// We have a match! Now we must insert the corresponding input into the CurrentInput object.

				if (! currentI->isSourceCompatible(toInput.device)){
					// This is a problem, we already have stored an input from another device.
					// Currently, we do not allow two inputs from two different devices to be passed.
					// To resolve this, I'm just gonna pass.
					break;
				}

				// If the device type is not yet set, set it.
				currentI->setTypeBasedOnSingleInput(toInput.device);

				// Store the input.
				currentI->directInput.keys[toInput.key] = 1;
			}
									  }
		case SINGLE_INPUT_DI_MOUSE:{
			// TODO
								   }

		case SINGLE_INPUT_DI_JOYSTICK:{
			// TODO
									  }

		case SINGLE_INPUT_XINPUT_JOYSTICK:{
			// TODO
										  }
		}
	}



	// Now doing the event map.
	// The code is almost the same, couldn't we factorise it? By using a single map?
	for(std::map<SingleInput,WORD>::iterator iter = eventMapping.begin(); iter != eventMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		WORD eventId = iter->second;
		// We want to map fromInput -> eventId

		switch (fromInput.device){

		case SINGLE_INPUT_DI_KEYBOARD:{

			// Mapping source is a keyboard, we need to check if the modifiers are identical, and if the key is pressed.
			char fromModifier = (char)(fromInput.key >> 8);
			char fromKey = (char)(fromInput.key & 0xFF);
			if((fromModifier == modifier) && DI_KEY_PRESSED(keys[fromKey])){
				// We have a match! Now we just have to send the corresponding message.

				SendMessage(hWnd, WM_COMMAND, eventId, 777);
			}
									  }
		case SINGLE_INPUT_DI_MOUSE:{
			// TODO
								   }

		case SINGLE_INPUT_DI_JOYSTICK:{
			// TODO
									  }

		case SINGLE_INPUT_XINPUT_JOYSTICK:{
			// TODO
										  }
		}
	}
}


void InputCapture::NextInput(SingleInput* si){

	char previousKeys[DI_KEY_NUMBER];
	char currentKeys[DI_KEY_NUMBER];
	bool somethingPressed = false;

	// Get the previous keyboard state.
	GetKeyboardState(previousKeys);
	// TODO: get also mouse/joystick states.

	Sleep(1);

	while(true){ // Set a timeout?

		// Get the current keyboard state.
		GetKeyboardState(currentKeys);
		// TODO: get also mouse/joystick states.

		// Try to find a non-modifier key that was just pressed.
		for (int i=0; i<DI_KEY_NUMBER; i++){

			if (IsModifier(i))
				continue;

			if (!DI_KEY_PRESSED(previousKeys[i]) && DI_KEY_PRESSED(currentKeys[i])){
				// We found a just-pressed key. We need to compute the modifiers and return the SingleInput
				char modifier = BuildModifier(currentKeys);
				si->device = SINGLE_INPUT_DI_KEYBOARD;
				si->key = (((short)modifier) << 8) | i;
				// TODO: Build description!!
				return;
			}
		}

		// TODO: Try to find a mouse button that was just pressed.
		// TODO: Try to find a joystick button that was just pressed.

		memcpy(previousKeys, currentKeys, DI_KEY_NUMBER);
		Sleep(1);
	}
}


void InputCapture::RemoveValueFromInputMap(SingleInput* si){
	for(std::map<SingleInput,SingleInput>::iterator iter = inputMapping.begin(); iter != inputMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		SingleInput toInput = iter->second;
		if(!(*si < toInput) && !(toInput < *si)){ // if (*si == toInput)
			inputMapping.erase(fromInput);
			return;
		}
	}
}

void InputCapture::RemoveValueFromEventMap(WORD eventId){
	for(std::map<SingleInput,WORD>::iterator iter = eventMapping.begin(); iter != eventMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		WORD toEventId = iter->second;
		if(eventId == toEventId){
			eventMapping.erase(fromInput);
			return;
		}
	}
}


void InputCapture::ReassignInput(int SIListIndex){

	SingleInput fromSI;
	SingleInput toSI = SIList[SIListIndex];

	// Gather the next input.
	NextInput(&fromSI);

	// Remove the previous mapping if any.
	RemoveValueFromInputMap(&toSI);

	// Map the retreived input pressed to the element of the SIList array.
	inputMapping[fromSI] = toSI;
}

void InputCapture::DefaultInput(int SIListIndex){
	SingleInput si = SIList[SIListIndex];
	RemoveValueFromInputMap(&si);
	inputMapping[si] = si;
}

void InputCapture::DisableInput(int SIListIndex){
	SingleInput si = SIList[SIListIndex];
	RemoveValueFromInputMap(&si);
}

void InputCapture::ReassignEvent(int eventListIndex){
	SingleInput fromSI;
	Event ev = eventList[eventListIndex];

	// Gather the next input.
	NextInput(&fromSI);

	// Remove the previous mapping if any.
	RemoveValueFromEventMap(ev.id);

	// Map the retreived input pressed to the element of the eventList array.
	eventMapping[fromSI] = ev.id;
}

void InputCapture::DefaultEvent(int eventListIndex){
	Event ev = eventList[eventListIndex];
	RemoveValueFromEventMap(ev.id);
	eventMapping[ev.defaultInput] = ev.id;
}

void InputCapture::DisableEvent(int eventListIndex){
	Event ev = eventList[eventListIndex];
	RemoveValueFromEventMap(ev.id);
}

// Build (or restore to) a default input mapping.
void InputCapture::BuildDefaultInputMapping(){
	inputMapping.clear();
	for (int i=0; i<SICount; i++){
		inputMapping[SIList[i]] = SIList[i]; // By default, an input is mapped to itself.
	}
}

// Build (or restore to) a default event mapping.
void InputCapture::BuildDefaultEventMapping(){
	eventMapping.clear();
	for (int i=0; i<eventCount; i++){
		eventMapping[eventList[i].defaultInput] = eventList[i].id; // The default mapping in stored inside the eventList.
	}
}

void InputCapture::FormatInputMapping(int index, char* from, char* to){
	char localFrom[1024] = "";
	char localTo[1024] = "";

	SingleInput* si = &SIList[index];
	strcpy(localFrom, si->description);
	from = localFrom;

	for(std::map<SingleInput,SingleInput>::iterator iter = inputMapping.begin(); iter != inputMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		SingleInput toInput = iter->second;
		if(!(*si < toInput) && !(toInput < *si)){ // if (*si == toInput)
			strcat(localTo, fromInput.description);
			break;
		}
	}

	to = localTo;
}

void InputCapture::FormatEventMapping(int index, char* from, char* to){
	char localFrom[1024] = "";
	char localTo[1024] = "";

	Event* ev = &eventList[index];
	strcpy(localFrom, ev->description);
	from = localFrom;

	for(std::map<SingleInput,WORD>::iterator iter = eventMapping.begin(); iter != eventMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		WORD toEventId = iter->second;
		if(ev->id == toEventId){
			strcat(localTo, fromInput.description);
			break;
		}
	}

	to = localTo;
}


// Save current mapping into a config file
void InputCapture::SaveMapping(char* filename){
	//TODO. Use WritePrivateProfileStringA
}

// Load mapping from a config file
void InputCapture::LoadMapping(char* filename){
	// TODO. Use GetPrivateProfileSectionA function?
}


InputCapture* inputC;
extern HINSTANCE hInst; // I don't like extern. Can it be removed?


LRESULT CALLBACK HotkeysOrInputsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isHotkeys)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static HWND Tex0 = NULL;
	extern HWND hWnd; // Same comment!

	switch(uMsg)
	{
		case WM_INITDIALOG: {
			GetWindowRect(hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			Tex0 = GetDlgItem(hDlg, IDC_STATIC_TEXT0);

			// We create the input interface for the dialog.
			inputC = new InputCapture();
			if (!inputC->InitInputs(hInst, hDlg)) return false;

			EnableWindow(GetDlgItem(hDlg, IDC_REASSIGNKEY), FALSE);
			//EnableWindow(GetDlgItem(hDlg, IDC_REVERTKEY), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_USEDEFAULTKEY), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_DISABLEKEY), FALSE);

			HWND listbox = GetDlgItem(hDlg, IDC_HOTKEYLIST);

			int stops [4] = {25*4, 25*4+4, 25*4+8, 25*4+16};
			SendMessage(listbox, LB_SETTABSTOPS, 4, (LONG)(LPSTR)stops);

			PopulateHotkeyListbox(listbox, isHotkeys);

			if(isHotkeys)
				SetWindowText(hDlg, "Hotkey Configuration");
			else
				SetWindowText(hDlg, "Input Mapping Configuration");

			return true;
		}	break;

		case WM_COMMAND:
		{
			int controlID = LOWORD(wParam);
			int messageID = HIWORD(wParam);

			if (messageID == LBN_SELCHANGE && controlID == IDC_HOTKEYLIST)
			{
				int selCount = SendDlgItemMessage(hDlg, IDC_HOTKEYLIST, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
				EnableWindow(GetDlgItem(hDlg, IDC_REASSIGNKEY), (selCount == 1) ? TRUE : FALSE);
				//EnableWindow(GetDlgItem(hDlg, IDC_REVERTKEY), (selCount >= 1) ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_USEDEFAULTKEY), (selCount >= 1) ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISABLEKEY), (selCount >= 1) ? TRUE : FALSE);
			}

			switch(controlID)
			{
				case IDOK:
					inputC->ReleaseInputs();
					delete inputC;
					EndDialog(hDlg, true);
					return true;
					break;

				case IDC_REASSIGNKEY:
				//case IDC_REVERTKEY:
				case IDC_USEDEFAULTKEY:
				case IDC_DISABLEKEY:
					ModifyHotkeyFromListbox(GetDlgItem(hDlg, IDC_HOTKEYLIST), controlID, Tex0, hDlg, isHotkeys);
					break;
			}
		}	break;
		case WM_CLOSE:
			inputC->ReleaseInputs();
			delete inputC;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}

LRESULT CALLBACK HotkeysProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return HotkeysOrInputsProc(hDlg, uMsg, wParam, lParam, true);
}
LRESULT CALLBACK InputsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return HotkeysOrInputsProc(hDlg, uMsg, wParam, lParam, false);
}

void PopulateHotkeyListbox(HWND listbox, bool isHotkeys)
{
	int inputNumber = 0;
	if (isHotkeys)
		inputNumber = inputC->eventCount;
	else
		inputNumber = inputC->SICount;

	// TODO: Should load from file. For now, building the default mapping.
	if (isHotkeys)
		inputC->BuildDefaultEventMapping();
	else
		inputC->BuildDefaultInputMapping();

	char* key;
	char* map;
	char line[1024];

	for(int i=0; i<inputNumber; i++){

		// Get the map strings.
		if (isHotkeys)
			inputC->FormatEventMapping(i, key, map);
		else
			inputC->FormatInputMapping(i, key, map);

		// Build the line.
		strcpy(line, key);
		strcat(line, "\t");
		strcpy(line, map);

		// Send it.
		SendMessage((HWND) listbox, (UINT) LB_ADDSTRING, (WPARAM) 0, (LPARAM) line);
	}
}


void ModifyHotkeyFromListbox(HWND listbox, WORD command, HWND statusText, HWND parentWindow, bool isHotkeys)
{
	char* key;
	char* map;

	int inputNumber = 0;
	if (isHotkeys)
		inputNumber = inputC->eventCount;
	else
		inputNumber = inputC->SICount;

	for(int i=0; i<inputNumber; i++)
	{
		int selected = SendMessage((HWND) listbox, (UINT) LB_GETSEL, (WPARAM) i, (LPARAM) 0);
		if(selected <= 0)
			continue;

		//InputButton& button = buttons[i];

		switch(command)
		{
			case IDC_REASSIGNKEY:
				{
					char str [256];


					if (isHotkeys){
						inputC->FormatEventMapping(i, key, map);
						sprintf(str, "SETTING KEY: %s", key);
						SetWindowText(statusText, str);
						inputC->ReassignEvent(i);
					}
					else{
						inputC->FormatInputMapping(i, key, map);
						sprintf(str, "SETTING KEY: %s", key);
						SetWindowText(statusText, str);
						inputC->ReassignInput(i);
					}
					SetWindowText(statusText, "");
				}
				break;
			//case IDC_REVERTKEY:
				//(isHotkeys ? s_initialHotkeyButtons : s_initialInputButtons)[i].CopyConfigurablePartsTo(button);
				//break;
			case IDC_USEDEFAULTKEY:
				if (isHotkeys)
					inputC->DefaultEvent(i);
				else
					inputC->DefaultInput(i);
				break;
			case IDC_DISABLEKEY:
				if (isHotkeys)
					inputC->DisableEvent(i);
				else
					inputC->DisableInput(i);
				break;
		}
		SetFocus(listbox);

		if (isHotkeys){
			inputC->FormatEventMapping(i, key, map);
		}
		else{
			inputC->FormatInputMapping(i, key, map);
		}

		char line[1024];
		strcpy(line, key);
		strcat(line, "\t");
		strcpy(line, map);

		SendMessage(listbox, LB_DELETESTRING, i, 0);  
		SendMessage(listbox, LB_INSERTSTRING, i, (LPARAM) line); 
		SendMessage(listbox, LB_SETSEL, (WPARAM) TRUE, (LPARAM) i); 
	}
}
