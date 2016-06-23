/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(INPUTSETUP_C_INCL) && !defined(UNITY_BUILD)
#define INPUTSETUP_C_INCL

// inputsetup.cpp
// much of this comes from Gens
// ...at least the general way things work. there have of course been many changes

//#pragma optimize("",off)
//#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
// Windows Header Files:
#include <windows.h>
#include <mmsystem.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
//#include "stdafx.h"
#include "Resource.h"
#define MAX_LOADSTRING 100
#include <stdio.h>
#include <math.h>
#include <map>
#include <set>
#include <vector>
#include <aclapi.h>

#define stricmp _stricmp


#define DIRECTINPUT_VERSION 0x0500  // for joystick support
#include "../external/dinput.h"
#pragma comment(lib, "../external/lib/dinput.lib")
#pragma comment(lib, "../external/lib/dxguid.lib")

#include "../shared/ipc.h"
#include "../shared/logcat.h"

#include "ramwatch.h"

#include "inputsetup.h"

//int debugprintf(const char * fmt, ...); // temp, should add header
#include "logging.h"

#define KEYDOWN(key) (inputState.Keys[key] & 0x80) 
#define MAX_JOYS 8

struct InputState
{
	LPDIRECTINPUT lpDI;
	LPDIRECTINPUTDEVICE lpDIDKeyboard;
	LPDIRECTINPUTDEVICE lpDIDMouse;
	int Nb_Joys;
	IDirectInputDevice2* Joy_ID[MAX_JOYS];
	DIJOYSTATE Joy_State[MAX_JOYS];
	long MouseX, MouseY;
	unsigned char Keys[256];
};

InputState mainInputState = {};
InputState configInputState = {};
InputState tempInputState = {};

unsigned char StateSelectCfg = 5;
//bool BackgroundInput = true;
extern int inputFocusFlags;
extern int hotkeysFocusFlags;
#ifndef FOCUS_FLAGS_DEFINED
#define FOCUS_FLAGS_DEFINED
enum // TODO: consolidate enum duplicates
{
	FOCUS_FLAG_TASER=0x01,
	FOCUS_FLAG_TASEE=0x02,
	FOCUS_FLAG_OTHER=0x04,
};
#endif
//#define BackgroundInput (inputFocusFlags != 0)

extern bool started;


bool FastForwardKeyDown = false;
bool FrameAdvanceKeyDown1 = false;
bool FrameAdvanceKeyDown2 = false;
bool AutoFireKeyDown = false;
bool AutoHoldKeyDown = false;
bool AutoClearKeyDown = false;


#define MAX_PENDIND_UNSYNCED 128
int numPendingUnsyncedPresses = 0;
int pendingUnsyncedPresses [MAX_PENDIND_UNSYNCED];


#define MOD_NONE 0
#define VK_NONE 0
#define ID_NONE 0

// TODO: add a "category" field and a way to filter the hotkey list by category

struct InputButton
{
	int modifiers; // ex: MOD_ALT | MOD_CONTROL | MOD_SHIFT

	int virtKey; // ex: VK_ESCAPE or 'O'
	WORD eventID; // send message on press

	int diKey; // ex: DIK_ESCAPE
	bool* alias; // set value = held

	const char* description; // for user display... feel free to change it
	const char* saveIDString; // for config file... please do not ever change these names or you will break backward compatibility

	BOOL requiresFrameSync;

	bool heldNow;

	//bool ShouldUseAccelerator() {
	//	return eventID && (virtKey > 0x07) && !(modifiers & MOD_WIN);
	//}

	void CopyConfigurablePartsTo(InputButton& button) {
		button.modifiers = modifiers;
		button.virtKey = virtKey;
		button.diKey = diKey;
	}
	void SetAsDIK(int dik, int mods = 0) {
		modifiers = mods;
		virtKey = VK_NONE;
		diKey = dik;
	}
	void SetAsVirt(int virt, int mods = 0) {
		modifiers = mods;
		virtKey = virt;
		diKey = 0;
	}
};

unsigned char localGameInputKeys [256] = {};

static InputButton s_inputButtons [255] = {};
static void InitDefaultInputButtons()
{
	unsigned char vkOrder [256] = {
		VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,
		VK_TAB,
		VK_RETURN,
		VK_SPACE,
		VK_ESCAPE,
		VK_BACK,
		VK_PAUSE,
		VK_MENU,VK_LMENU,VK_RMENU,
		VK_SHIFT,VK_LSHIFT,VK_RSHIFT,
		VK_CONTROL,VK_LCONTROL,VK_RCONTROL,
		VK_NUMLOCK,VK_CAPITAL,VK_SCROLL,
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'1','2','3','4','5','6','7','8','9','0',
		VK_F1,VK_F2,VK_F3,VK_F4,VK_F5,VK_F6,VK_F7,VK_F8,VK_F9,VK_F10,VK_F11,VK_F12
	};
	int iNextEmptyOrder = 256;
	for(int i = 0; i < 256; i++)
	{
		if(!vkOrder[i])
		{
			iNextEmptyOrder = i;
			break;
		}
	}
	for(int i = 0; i < 256 && iNextEmptyOrder != 256; i++)
	{
		bool already = false;
		for(int j = 0; j < 256; j++)
		{
			if(vkOrder[j] == i)
			{
				already = true;
				break;
			}
		}
		if(already)
			continue;

		vkOrder[iNextEmptyOrder++] = i;
	}
	
	for(int i = 0; i < ARRAYSIZE(s_inputButtons); i++)
	{
		InputButton& button = s_inputButtons[i];
		int vk = vkOrder[i];
		button.SetAsVirt(vk);
		button.alias = (bool*)&localGameInputKeys[vk];
		button.description = _strdup(GetVirtualKeyName(button.virtKey));
		char strtemp [16];
		sprintf(strtemp, "Input%02X", vk);
		button.saveIDString = _strdup(strtemp);
		button.eventID = 0;
		button.requiresFrameSync = false;
		button.heldNow = false;
	}
}
static inline int GetNumInputs(void)
{
	return ARRAYSIZE(s_inputButtons);
}
static std::map<WORD,int> s_reverseEventInputLookup;
static InputButton s_defaultInputButtons [ARRAYSIZE(s_inputButtons)];
static InputButton s_initialInputButtons [ARRAYSIZE(s_inputButtons)];

static InputButton s_hotkeyButtons [] =
{
	{MOD_CONTROL,           'T',       ID_TOGGLE_MOVIE_READONLY,  0, NULL, "Toggle Movie Read-Only", "ToggleReadOnlyKey"},
	{MOD_CONTROL,           'O',       ID_FILES_OPENEXE,          0, NULL, "Open Executable", "OpenExeKey"},
	{MOD_CONTROL,           'M',       ID_FILES_OPENMOV,          0, NULL, "Open Movie", "OpenMovieKey"},
	{MOD_CONTROL,           'P',       ID_FILES_PLAYMOV,          0, NULL, "Run and Play", "PlayMovieKey"},
	{MOD_CONTROL,           'W',       ID_FILES_WATCHMOV,         0, NULL, "Watch from Beginning", "WatchMovieKey"},
	{MOD_CONTROL,           'R',       ID_FILES_RECORDMOV,        0, NULL, "Start Recording To", "RecordNewMovieKey"},
	{MOD_NONE,          VK_NONE,       ID_FILES_RESUMERECORDING,  0, NULL, "Resume Recording From Now", "ResumeRecordingKey"},
	{MOD_NONE,			VK_NONE,		ID_FILES_BACKUPMOVIE,		0, NULL, "Backup Movie", "BackupKey"}, // TODO: Change ResumeToKey into BackupKey
	//{MOD_NONE,          VK_NONE,       ID_FILES_RESUMEMOVAS,      0, NULL, "Resume Recording To", "ResumeToKey"},
	//{MOD_NONE,          VK_NONE,       ID_FILES_SPLICE,           0, NULL, "Splice Movie", "SpliceMovieKey"},
	{MOD_CONTROL|MOD_SHIFT, 'C',       ID_FILES_STOP_RELAY,       0, NULL, "Stop Running", "StopRunningKey"},
	{MOD_NONE,          VK_NONE,       ID_FILES_QUIT,             0, NULL, "Exit " /*"winTASer"*/ "Hourglass", "ExitKey"},
	{MOD_NONE,        VK_ESCAPE,       ID_TIME_TOGGLE_PAUSE,      0, NULL, "Pause/Unpause", "PauseKey0"},
	{MOD_NONE,        VK_DELETE,       ID_TIME_TOGGLE_PAUSE,      0, NULL, "Pause/Unpause (key 2)", "PauseKey"},
	{MOD_NONE,         VK_PAUSE,       ID_TIME_TOGGLE_PAUSE,      0, NULL, "Pause/Unpause (key 3)", "PauseKey2"},
	{MOD_NONE,          VK_NONE,       ID_TIME_TOGGLE_FASTFORWARD,0, NULL, "Toggle Fast-Forward", "ToggleFastForwardKey"},

//	{MOD_NONE,          VK_NONE,       ID_FILES_OPENRECENTROM0,  0, NULL, "Open Most Recent Executable", "OpenMostRecentROM"},
//	{MOD_NONE,          VK_NONE,       ID_LUA_OPENRECENTSCRIPT0, 0, NULL, "Open Most Recent Lua Script", "OpenMostRecentLua"},

	{MOD_NONE, VK_TAB,   ID_SWITCH_TO_TASEE_FROM_TASER, 0, &FastForwardKeyDown,  "Fast Forward", "FastForwardKey"},
	{MOD_NONE, VK_OEM_5, ID_SWITCH_TO_TASEE_FROM_TASER, 0, &FrameAdvanceKeyDown1,"Frame Advance", "SkipFrameKey"},
	{MOD_NONE, VK_SPACE, ID_SWITCH_TO_TASEE_FROM_TASER, 0, &FrameAdvanceKeyDown2,"Frame Advance (key 2)", "SkipFrameKey2"},
//	{MOD_NONE, VK_NONE,  ID_NONE, 0, &AutoFireKeyDown,     "Auto-Fire Modifier", "AutoFireKey"},
//	{MOD_NONE, VK_NONE,  ID_NONE, 0, &AutoHoldKeyDown,     "Auto-Hold Modifier", "AutoHoldKey"},
//	{MOD_NONE, VK_OEM_3, ID_NONE, 0, &AutoClearKeyDown,    "Clear Auto-Fire and Auto-Hold", "AutoClearKey"},

	{MOD_CONTROL,         VK_OEM_MINUS,ID_TIME_RATE_SLOWER,      0, NULL, "Decrease Speed", "SlowDownKey"},
	{MOD_CONTROL,         VK_OEM_PLUS, ID_TIME_RATE_FASTER,      0, NULL, "Increase Speed", "SpeedUpKey"},
	{MOD_NONE,            VK_NONE,     ID_TIME_RATE_100,         0, NULL, "Normal Speed", "NormalSpeedKey"},

	{MOD_NONE,          VK_NONE,       ID_SOUND_NOPLAYBUFFERS,   0, NULL, "Disable/Enable Sound", "DisableSoundKey"},

	{MOD_NONE,              '1',       ID_FILES_LOADSTATE_1,     0, NULL, "Load State 1", "Load1Key", TRUE},
	{MOD_NONE,              '2',       ID_FILES_LOADSTATE_2,     0, NULL, "Load State 2", "Load2Key", TRUE},
	{MOD_NONE,              '3',       ID_FILES_LOADSTATE_3,     0, NULL, "Load State 3", "Load3Key", TRUE},
	{MOD_NONE,              '4',       ID_FILES_LOADSTATE_4,     0, NULL, "Load State 4", "Load4Key", TRUE},
	{MOD_NONE,              '5',       ID_FILES_LOADSTATE_5,     0, NULL, "Load State 5", "Load5Key", TRUE},
	{MOD_NONE,              '6',       ID_FILES_LOADSTATE_6,     0, NULL, "Load State 6", "Load6Key", TRUE},
	{MOD_NONE,              '7',       ID_FILES_LOADSTATE_7,     0, NULL, "Load State 7", "Load7Key", TRUE},
	{MOD_NONE,              '8',       ID_FILES_LOADSTATE_8,     0, NULL, "Load State 8", "Load8Key", TRUE},
	{MOD_NONE,              '9',       ID_FILES_LOADSTATE_9,     0, NULL, "Load State 9", "Load9Key", TRUE},
	{MOD_NONE,              '0',       ID_FILES_LOADSTATE_10,    0, NULL, "Load State 10","Load10Key", TRUE},
	{MOD_NONE,            VK_F1,       ID_FILES_LOADSTATE_11,    0, NULL, "Load State 11","Load11Key", TRUE},
	{MOD_NONE,            VK_F2,       ID_FILES_LOADSTATE_12,    0, NULL, "Load State 12","Load12Key", TRUE},
	{MOD_NONE,            VK_F3,       ID_FILES_LOADSTATE_13,    0, NULL, "Load State 13","Load13Key", TRUE},
	{MOD_NONE,            VK_F4,       ID_FILES_LOADSTATE_14,    0, NULL, "Load State 14","Load14Key", TRUE},
	{MOD_NONE,            VK_F5,       ID_FILES_LOADSTATE_15,    0, NULL, "Load State 15","Load15Key", TRUE},
	{MOD_NONE,            VK_F6,       ID_FILES_LOADSTATE_16,    0, NULL, "Load State 16","Load16Key", TRUE},
	{MOD_NONE,            VK_F7,       ID_FILES_LOADSTATE_17,    0, NULL, "Load State 17","Load17Key", TRUE},
	{MOD_NONE,            VK_F8,       ID_FILES_LOADSTATE_18,    0, NULL, "Load State 18","Load18Key", TRUE},
	{MOD_NONE,            VK_F9,       ID_FILES_LOADSTATE_19,    0, NULL, "Load State 19","Load19Key", TRUE},
	{MOD_NONE,           VK_F10,       ID_FILES_LOADSTATE_20,    0, NULL, "Load State 20","Load20Key", TRUE},

	{MOD_SHIFT,             '1',       ID_FILES_SAVESTATE_1,     0, NULL, "Save State 1", "Save1Key", TRUE},
	{MOD_SHIFT,             '2',       ID_FILES_SAVESTATE_2,     0, NULL, "Save State 2", "Save2Key", TRUE},
	{MOD_SHIFT,             '3',       ID_FILES_SAVESTATE_3,     0, NULL, "Save State 3", "Save3Key", TRUE},
	{MOD_SHIFT,             '4',       ID_FILES_SAVESTATE_4,     0, NULL, "Save State 4", "Save4Key", TRUE},
	{MOD_SHIFT,             '5',       ID_FILES_SAVESTATE_5,     0, NULL, "Save State 5", "Save5Key", TRUE},
	{MOD_SHIFT,             '6',       ID_FILES_SAVESTATE_6,     0, NULL, "Save State 6", "Save6Key", TRUE},
	{MOD_SHIFT,             '7',       ID_FILES_SAVESTATE_7,     0, NULL, "Save State 7", "Save7Key", TRUE},
	{MOD_SHIFT,             '8',       ID_FILES_SAVESTATE_8,     0, NULL, "Save State 8", "Save8Key", TRUE},
	{MOD_SHIFT,             '9',       ID_FILES_SAVESTATE_9,     0, NULL, "Save State 9", "Save9Key", TRUE},
	{MOD_SHIFT,             '0',       ID_FILES_SAVESTATE_10,    0, NULL, "Save State 10","Save10Key", TRUE},
	{MOD_SHIFT,           VK_F1,       ID_FILES_SAVESTATE_11,    0, NULL, "Save State 11","Save11Key", TRUE},
	{MOD_SHIFT,           VK_F2,       ID_FILES_SAVESTATE_12,    0, NULL, "Save State 12","Save12Key", TRUE},
	{MOD_SHIFT,           VK_F3,       ID_FILES_SAVESTATE_13,    0, NULL, "Save State 13","Save13Key", TRUE},
	{MOD_SHIFT,           VK_F4,       ID_FILES_SAVESTATE_14,    0, NULL, "Save State 14","Save14Key", TRUE},
	{MOD_SHIFT,           VK_F5,       ID_FILES_SAVESTATE_15,    0, NULL, "Save State 15","Save15Key", TRUE},
	{MOD_SHIFT,           VK_F6,       ID_FILES_SAVESTATE_16,    0, NULL, "Save State 16","Save16Key", TRUE},
	{MOD_SHIFT,           VK_F7,       ID_FILES_SAVESTATE_17,    0, NULL, "Save State 17","Save17Key", TRUE},
	{MOD_SHIFT,           VK_F8,       ID_FILES_SAVESTATE_18,    0, NULL, "Save State 18","Save18Key", TRUE},
	{MOD_SHIFT,           VK_F9,       ID_FILES_SAVESTATE_19,    0, NULL, "Save State 19","Save19Key", TRUE},
	{MOD_SHIFT,          VK_F10,       ID_FILES_SAVESTATE_20,    0, NULL, "Save State 20","Save20Key", TRUE},

//	{MOD_NONE,           VK_NONE,     ID_FILES_NEXTSTATE,       0, NULL, "Set Next State", "SetNextKey"},
//	{MOD_NONE,           VK_NONE,     ID_FILES_PREVIOUSSTATE,   0, NULL, "Set Previous State", "SetPrevKey"},
//	{MOD_NONE,           VK_NONE,     ID_FILES_LOADSTATE,       0, NULL, "Load Current Savestate", "SaveCurrentKey"},
//	{MOD_NONE,           VK_NONE,     ID_FILES_SAVESTATE,       0, NULL, "Save Current Savestate", "LoadCurrentKey"},
//
//	{MOD_NONE,              VK_NONE,   IDC_NEW_LUA_SCRIPT,       0, NULL, "New Lua Script Window", "OpenNewLuaScriptWindow"},
//	{MOD_NONE,              VK_NONE,   IDC_CLOSE_LUA_SCRIPTS,    0, NULL, "Close Lua Script Windows", "CloseLuaScriptWindows"},
	{MOD_NONE,              VK_NONE,   ID_RAM_SEARCH,            0, NULL, "Ram Search", "RamSearchKey"},
	{MOD_NONE,              VK_NONE,   ID_RAM_WATCH,             0, NULL, "Ram Watch", "RamWatchKey"},

	{MOD_NONE,          VK_NONE,       ID_INPUT_HOTKEYS,0, NULL, "Configure Hotkeys", "HotkeysConfigKey"},
	{MOD_NONE,          VK_NONE,       ID_INPUT_INPUTS,0, NULL, "Configure Game Input", "InputsConfigKey"},
	{MOD_NONE,            VK_NONE,     ID_SWITCH_TO_TASEE,       0, NULL, "Switch to Game Window", "SwitchToTaseeKey"},
	{MOD_NONE,            VK_NONE,     ID_SWITCH_TO_TASER,       0, NULL, "Switch to Hourglass Window", "SwitchToTaserKey"},
};

static inline int GetNumHotkeys(void)
{
	return ARRAYSIZE(s_hotkeyButtons);
}

static std::map<WORD,int> s_reverseEventHotkeyLookup;

static InputButton s_defaultHotkeyButtons [ARRAYSIZE(s_hotkeyButtons)];
static bool defaultInputButtonsStored = false;
static void StoreDefaultInputButtons()
{
	if(!defaultInputButtonsStored)
	{
		InitDefaultInputButtons();
		memcpy(s_defaultHotkeyButtons, s_hotkeyButtons, sizeof(s_defaultHotkeyButtons));
		memcpy(s_defaultInputButtons, s_inputButtons, sizeof(s_defaultInputButtons));
		defaultInputButtonsStored = true;
	}
}
static InputButton s_initialHotkeyButtons [ARRAYSIZE(s_hotkeyButtons)];
static bool initialInputButtonsStored = false;
static void StoreInitialInputButtons()
{
	if(!initialInputButtonsStored)
	{
		memcpy(s_initialHotkeyButtons, s_hotkeyButtons, sizeof(s_initialHotkeyButtons));
		memcpy(s_initialInputButtons, s_inputButtons, sizeof(s_initialInputButtons));
		initialInputButtonsStored = true;
	}
}

void UpdateReverseLookup(bool isHotkeys)
{
	std::map<WORD,int>& reverseEventLookup = isHotkeys ? s_reverseEventHotkeyLookup : s_reverseEventInputLookup;
	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	reverseEventLookup.clear();
	int numInputButtons = GetNumButtons();
	for(int i=0; i<numInputButtons; i++)
		if(buttons[i].eventID && (buttons[i].diKey || buttons[i].virtKey || buttons[i].modifiers || !reverseEventLookup[buttons[i].eventID]))
			reverseEventLookup[buttons[i].eventID] = i+1;
}


static void SetButtonToOldDIKey(InputButton& button, int key)
{
	if(key)
	{
		if(button.diKey == DIK_PAUSE)
		{
			// special case this one since VK_PAUSE works much more reliably than DIK_PAUSE
			button.virtKey = VK_PAUSE;
			button.diKey = 0;
		}
		else
		{
			button.virtKey = VK_NONE;
			button.diKey = key;
		}
		button.modifiers = MOD_NONE;
	}
}

void SaveHotkeys(char* filename, bool isHotkeys)
{
	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	char Str_Tmp[1024];
	int numInputButtons = GetNumButtons();
	for(int i=0; i<numInputButtons; i++)
	{
		InputButton& button = buttons[i];
		sprintf(Str_Tmp, "%d,%d,%d", button.diKey, button.modifiers, button.virtKey); // it's important that diKey comes first, since older versions only had that one
		WritePrivateProfileStringA("Input", button.saveIDString, Str_Tmp, filename);
	}
}

void LoadHotkeys(char* filename, bool isHotkeys)
{
	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	StoreDefaultInputButtons();
	char Str_Tmp[1024];
	int numInputButtons = GetNumButtons();
	for(int i=0; i<numInputButtons; i++)
	{
		InputButton& button = buttons[i];

		GetPrivateProfileStringA("Input", button.saveIDString, "", Str_Tmp, 1024, filename);
		if(Str_Tmp[0] != 0)
		{
			// found
			InputButton temp;
			int read = sscanf(Str_Tmp, "%d, %d, %d", &temp.diKey, &temp.modifiers, &temp.virtKey);
			if(read == 3)
			{
				temp.CopyConfigurablePartsTo(button);
			}
			else if(read == 1)
			{
				SetButtonToOldDIKey(button, temp.diKey);
			}
		}
		else
		{
			// not found, if it's a savestate hotkey then make it honor the StateSelectCfg

			switch(button.eventID)
			{
			case ID_FILES_SAVESTATE_1:
			case ID_FILES_SAVESTATE_2:
			case ID_FILES_SAVESTATE_3:
			case ID_FILES_SAVESTATE_4:
			case ID_FILES_SAVESTATE_5:
			case ID_FILES_SAVESTATE_6:
			case ID_FILES_SAVESTATE_7:
			case ID_FILES_SAVESTATE_8:
			case ID_FILES_SAVESTATE_9:
			case ID_FILES_SAVESTATE_10:
			case ID_FILES_SAVESTATE_11:
			case ID_FILES_SAVESTATE_12:
			case ID_FILES_SAVESTATE_13:
			case ID_FILES_SAVESTATE_14:
			case ID_FILES_SAVESTATE_15:
			case ID_FILES_SAVESTATE_16:
			case ID_FILES_SAVESTATE_17:
			case ID_FILES_SAVESTATE_18:
			case ID_FILES_SAVESTATE_19:
			case ID_FILES_SAVESTATE_20:
				switch(StateSelectCfg)
				{
				case 0: case 5: button.modifiers = MOD_SHIFT; break;
				case 1: case 3: button.modifiers = MOD_NONE; break;
				case 2: case 4: button.modifiers = MOD_CONTROL; break;
				}
				break;

			case ID_FILES_LOADSTATE_1:
			case ID_FILES_LOADSTATE_2:
			case ID_FILES_LOADSTATE_3:
			case ID_FILES_LOADSTATE_4:
			case ID_FILES_LOADSTATE_5:
			case ID_FILES_LOADSTATE_6:
			case ID_FILES_LOADSTATE_7:
			case ID_FILES_LOADSTATE_8:
			case ID_FILES_LOADSTATE_9:
			case ID_FILES_LOADSTATE_10:
			case ID_FILES_LOADSTATE_11:
			case ID_FILES_LOADSTATE_12:
			case ID_FILES_LOADSTATE_13:
			case ID_FILES_LOADSTATE_14:
			case ID_FILES_LOADSTATE_15:
			case ID_FILES_LOADSTATE_16:
			case ID_FILES_LOADSTATE_17:
			case ID_FILES_LOADSTATE_18:
			case ID_FILES_LOADSTATE_19:
			case ID_FILES_LOADSTATE_20:
				switch(StateSelectCfg)
				{
				case 0: case 1: button.modifiers = MOD_CONTROL; break;
				case 2: case 3: button.modifiers = MOD_SHIFT; break;
				case 4: case 5: button.modifiers = MOD_NONE; break;
				}
				break;

			//case ID_FILES_SETSTATE_1:
			//case ID_FILES_SETSTATE_2:
			//case ID_FILES_SETSTATE_3:
			//case ID_FILES_SETSTATE_4:
			//case ID_FILES_SETSTATE_5:
			//case ID_FILES_SETSTATE_6:
			//case ID_FILES_SETSTATE_7:
			//case ID_FILES_SETSTATE_8:
			//case ID_FILES_SETSTATE_9:
			//case ID_FILES_SETSTATE_0:
			//	switch(StateSelectCfg)
			//	{
			//	case 0: case 2: button.modifiers = MOD_NONE; break;
			//	case 1: case 4: button.modifiers = MOD_SHIFT; break;
			//	case 3: case 5: button.modifiers = MOD_CONTROL; break;
			//	}
			//	break;
			}
		}
	}
	UpdateReverseLookup(isHotkeys);
}


static const char* alphabet = "A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z";
static const char* digits = "0" "\0" "1" "\0" "2" "\0" "3" "\0" "4" "\0" "5" "\0" "6" "\0" "7" "\0" "8" "\0" "9";

const char* GetVirtualKeyName(int key)
{
	if(key >= 'A' && key <= 'Z')
		return alphabet + 2 * (key - 'A');
	if(key >= '0' && key <= '9')
		return digits + 2 * (key - '0');

	switch(key)
	{
	case 0: return "None";
	case VK_LBUTTON: return "LeftClick";
	case VK_RBUTTON: return "RightClick";
	case VK_CANCEL: return "Cancel";
	case VK_MBUTTON: return "MiddleClick";
	case VK_BACK: return "Backspace";
	case VK_TAB: return "Tab";
	case VK_CLEAR: return "Clear";
	case VK_RETURN: return "Enter";
	case VK_SHIFT: return "Shift";
	case VK_CONTROL: return "Control";
	case VK_MENU: return "Alt";
	case VK_PAUSE: return "Pause/Break";
	case VK_CAPITAL: return "CapsLock";
	case VK_KANA: return "Kana/Hangul";
	case VK_JUNJA: return "Junja";
	case VK_FINAL: return "Final";
	case VK_HANJA: return "Hanja/Kanji";
	case VK_ESCAPE: return "Escape";
	case VK_CONVERT: return "Convert";
	case VK_NONCONVERT: return "NoConvert";
	case VK_ACCEPT: return "Accept";
	case VK_MODECHANGE: return "Modechange";
	case VK_SPACE: return "Space";
	case VK_PRIOR: return "PageUp";
	case VK_NEXT: return "PageDown";
	case VK_END: return "End";
	case VK_HOME: return "Home";
	case VK_LEFT: return "LeftArrow";
	case VK_UP: return "UpArrow";
	case VK_RIGHT: return "RightArrow";
	case VK_DOWN: return "DownArrow";
	case VK_SELECT: return "Select";
	case VK_PRINT: return "Print";
	case VK_EXECUTE: return "Execute";
	case VK_SNAPSHOT: return "PrintScreen";
	case VK_INSERT: return "Insert";
	case VK_DELETE: return "Delete";
	case VK_HELP: return "Help";
	case VK_LWIN: return "LWin";
	case VK_RWIN: return "RWin";
	case VK_APPS: return "Apps";
	case VK_SLEEP: return "Sleep";
	case VK_NUMPAD0: return "Numpad0";
	case VK_NUMPAD1: return "Numpad1";
	case VK_NUMPAD2: return "Numpad2";
	case VK_NUMPAD3: return "Numpad3";
	case VK_NUMPAD4: return "Numpad4";
	case VK_NUMPAD5: return "Numpad5";
	case VK_NUMPAD6: return "Numpad6";
	case VK_NUMPAD7: return "Numpad7";
	case VK_NUMPAD8: return "Numpad8";
	case VK_NUMPAD9: return "Numpad9";
	case VK_MULTIPLY: return "Numpad*";
	case VK_ADD: return "Numpad+";
	case VK_SEPARATOR: return "Separator";
	case VK_SUBTRACT: return "Numpad-";
	case VK_DECIMAL: return "Numpad.";
	case VK_DIVIDE: return "Numpad/";
	case VK_F1: return "F1";
	case VK_F2: return "F2";
	case VK_F3: return "F3";
	case VK_F4: return "F4";
	case VK_F5: return "F5";
	case VK_F6: return "F6";
	case VK_F7: return "F7";
	case VK_F8: return "F8";
	case VK_F9: return "F9";
	case VK_F10: return "F10";
	case VK_F11: return "F11";
	case VK_F12: return "F12";
	case VK_F13: return "F13";
	case VK_F14: return "F14";
	case VK_F15: return "F15";
	case VK_F16: return "F16";
	case VK_F17: return "F17";
	case VK_F18: return "F18";
	case VK_F19: return "F19";
	case VK_F20: return "F20";
	case VK_F21: return "F21";
	case VK_F22: return "F22";
	case VK_F23: return "F23";
	case VK_F24: return "F24";
	case VK_NUMLOCK: return "NumLock";
	case VK_SCROLL: return "ScrollLock";
	case VK_OEM_1: return ";:";
	case VK_OEM_PLUS: return "=+";
	case VK_OEM_COMMA: return ",<";
	case VK_OEM_MINUS: return "-_";
	case VK_OEM_PERIOD: return ".>";
	case VK_OEM_2: return "/?";
	case VK_OEM_3: return "`~";
	case VK_OEM_4: return "[{";
	case VK_OEM_5: return "\\|";
	case VK_OEM_6: return "]}";
	case VK_OEM_7: return "'\"";
	case VK_OEM_8: return "OEM_8";
	case VK_LSHIFT: return "LShift";
	case VK_RSHIFT: return "RShift";
	case VK_LCONTROL: return "LControl";
	case VK_RCONTROL: return "RControl";
	case VK_LMENU: return "LAlt";
	case VK_RMENU: return "RAlt";

	default:
		static char unk [8];
		sprintf(unk, "0x%X", key);
		return unk;
	}
}

static const char* GetDirectInputKeyName(int key)
{
	switch(key)
	{
	case 0: return "None";
	case DIK_ESCAPE: return "Escape";
	case DIK_1: return "1";
	case DIK_2: return "2";
	case DIK_3: return "3";
	case DIK_4: return "4";
	case DIK_5: return "5";
	case DIK_6: return "6";
	case DIK_7: return "7";
	case DIK_8: return "8";
	case DIK_9: return "9";
	case DIK_0: return "0";
	case DIK_MINUS: return "-_";
	case DIK_EQUALS: return "=+";
	case DIK_BACK: return "Backspace";
	case DIK_TAB: return "Tab";
	case DIK_Q: return "Q";
	case DIK_W: return "W";
	case DIK_E: return "E";
	case DIK_R: return "R";
	case DIK_T: return "T";
	case DIK_Y: return "Y";
	case DIK_U: return "U";
	case DIK_I: return "I";
	case DIK_O: return "O";
	case DIK_P: return "P";
	case DIK_LBRACKET: return "[{";
	case DIK_RBRACKET: return "]}";
	case DIK_RETURN: return "Enter";
	case DIK_LCONTROL: return "LControl";
	case DIK_A: return "A";
	case DIK_S: return "S";
	case DIK_D: return "D";
	case DIK_F: return "F";
	case DIK_G: return "G";
	case DIK_H: return "H";
	case DIK_J: return "J";
	case DIK_K: return "K";
	case DIK_L: return "L";
	case DIK_SEMICOLON: return ";:";
	case DIK_APOSTROPHE: return "'\"";
	case DIK_GRAVE: return "`~";
	case DIK_LSHIFT: return "LShift";
	case DIK_BACKSLASH: return "\\|";
	case DIK_Z: return "Z";
	case DIK_X: return "X";
	case DIK_C: return "C";
	case DIK_V: return "V";
	case DIK_B: return "B";
	case DIK_N: return "N";
	case DIK_M: return "M";
	case DIK_COMMA: return ",<";
	case DIK_PERIOD: return ".>";
	case DIK_SLASH: return "/?";
	case DIK_RSHIFT: return "RShift";
	case DIK_MULTIPLY: return "Numpad*";
	case DIK_LMENU: return "LAlt";
	case DIK_SPACE: return "Space";
	case DIK_CAPITAL: return "CapsLock";
	case DIK_F1: return "F1";
	case DIK_F2: return "F2";
	case DIK_F3: return "F3";
	case DIK_F4: return "F4";
	case DIK_F5: return "F5";
	case DIK_F6: return "F6";
	case DIK_F7: return "F7";
	case DIK_F8: return "F8";
	case DIK_F9: return "F9";
	case DIK_F10: return "F10";
	case DIK_NUMLOCK: return "NumLock";
	case DIK_SCROLL: return "ScrollLock";
	case DIK_NUMPAD7: return "Numpad7";
	case DIK_NUMPAD8: return "Numpad8";
	case DIK_NUMPAD9: return "Numpad9";
	case DIK_SUBTRACT: return "Numpad-";
	case DIK_NUMPAD4: return "Numpad4";
	case DIK_NUMPAD5: return "Numpad5";
	case DIK_NUMPAD6: return "Numpad6";
	case DIK_ADD: return "Numpad+";
	case DIK_NUMPAD1: return "Numpad1";
	case DIK_NUMPAD2: return "Numpad2";
	case DIK_NUMPAD3: return "Numpad3";
	case DIK_NUMPAD0: return "Numpad0";
	case DIK_DECIMAL: return "Numpad.";
	case DIK_OEM_102: return "<>\\|";
	case DIK_F11: return "F11";
	case DIK_F12: return "F12";
	case DIK_F13: return "F13";
	case DIK_F14: return "F14";
	case DIK_F15: return "F15";
	case DIK_KANA: return "Kana";
	case DIK_ABNT_C1: return "/?";
	case DIK_CONVERT: return "Convert";
	case DIK_NOCONVERT: return "NoConvert";
	case DIK_YEN: return "Yen";
	case DIK_ABNT_C2: return "Numpad.";
	case DIK_NUMPADEQUALS: return "Numpad=";
	case DIK_PREVTRACK: return "Prevtrack";
	case DIK_AT: return "@";
	case DIK_COLON: return ":";
	case DIK_UNDERLINE: return "_";
	case DIK_KANJI: return "Kanji";
	case DIK_STOP: return "Stop";
	case DIK_AX: return "AX";
	case DIK_UNLABELED: return "Unlabeled";
	case DIK_NEXTTRACK: return "Nexttrack";
	case DIK_NUMPADENTER: return "NumpadEnter";
	case DIK_RCONTROL: return "RControl";
	case DIK_MUTE: return "Mute";
	case DIK_CALCULATOR: return "Calculator";
	case DIK_PLAYPAUSE: return "PlayPause";
	case DIK_MEDIASTOP: return "MediaStop";
	case DIK_VOLUMEDOWN: return "VolumeDown";
	case DIK_VOLUMEUP: return "VolumeUp";
	case DIK_WEBHOME: return "WebHome";
	case DIK_NUMPADCOMMA: return "Numpad,";
	case DIK_DIVIDE: return "Numpad/";
	case DIK_SYSRQ: return "Sysrq";
	case DIK_RMENU: return "RAlt";
	case DIK_PAUSE: return "Pause";
	case DIK_HOME: return "Home";
	case DIK_UP: return "Up";
	case DIK_PRIOR: return "PageUp";
	case DIK_LEFT: return "Left";
	case DIK_RIGHT: return "Right";
	case DIK_END: return "End";
	case DIK_DOWN: return "Down";
	case DIK_NEXT: return "PageDown";
	case DIK_INSERT: return "Insert";
	case DIK_DELETE: return "Delete";
	case DIK_LWIN: return "LWin";
	case DIK_RWIN: return "RWin";
	case DIK_APPS: return "Apps";
	case DIK_POWER: return "Power";
	case DIK_SLEEP: return "Sleep";
	case DIK_WAKE: return "Wake";
	case DIK_WEBSEARCH: return "WebSearch";
	case DIK_WEBFAVORITES: return "WebFavorites";
	case DIK_WEBREFRESH: return "WebRefresh";
	case DIK_WEBSTOP: return "WebStop";
	case DIK_WEBFORWARD: return "WebForward";
	case DIK_WEBBACK: return "WebBack";
	case DIK_MYCOMPUTER: return "MyComputer";
	case DIK_MAIL: return "Mail";
	case DIK_MEDIASELECT: return "MediaSelect";

	default:
		if (key > 0x100)
		{
			static char joy[64];
			sprintf(joy,"Pad%d",((key >> 8) & 0xF) + 1);
			char Key[32];
			if (key & 0x80)
			{
				switch (key & 0xF)
				{
					case 1:
						sprintf(Key,"Povhat%dUp",((key >> 4) & 0x3) + 1);
						break;
					case 2:
						sprintf(Key,"Povhat%dRight",((key >> 4) & 0x3) + 1);
						break;
					case 3:
						sprintf(Key,"Povhat%dDown",((key >> 4) & 0x3) + 1);
						break;
					case 4:
						sprintf(Key,"Povhat%dLeft",((key >> 4) & 0x3) + 1);
						break;
					default:
						sprintf(Key,"Povhat%dunknown 0x%X",((key >> 4) & 0x3) + 1, key & 0xF);
						break;
				}
			}
			else if (key & 0x70)
				sprintf(Key,"Button%d",(key & 0xFF) - 0xF);
			else 
			{
				switch (key & 0xF)
				{
					case 1:
						sprintf(Key,"Up");
						break;
					case 2:
						sprintf(Key,"Down");
						break;
					case 3:
						sprintf(Key,"Left");
						break;
					case 4:
						sprintf(Key,"Right");
						break;
					case 5:
						sprintf(Key,"RLeft");
						break;
					case 6:
						sprintf(Key,"RRight");
						break;
					case 7:
						sprintf(Key,"RUp");
						break;
					case 8:
						sprintf(Key,"RDown");
						break;
					case 9:
						sprintf(Key,"ZRight");
						break;
					case 0xA:
						sprintf(Key,"ZLeft");
						break;

					default:
						sprintf(Key,"undefined 0x%X", key & 0xF);
				}
			}
			strcat(joy,Key);
			return joy;
		}
		static char unk [8];
		sprintf(unk, "0x%X", key);
		return unk;
	}
}

void AddHotkeySuffix(char* str, InputButton& button)
{
	if(!button.modifiers && !button.virtKey && !button.diKey)
		return;

	strcat(str, "\t");

//#define MODIFIER_SEPARATOR "+"
#define MODIFIER_SEPARATOR " "

	if(button.modifiers & MOD_CONTROL)
		strcat(str, "Ctrl" MODIFIER_SEPARATOR);
	if(button.modifiers & MOD_SHIFT)
		strcat(str, "Shift" MODIFIER_SEPARATOR);
	if(button.modifiers & MOD_ALT)
		strcat(str, "Alt" MODIFIER_SEPARATOR);
	if(button.modifiers & MOD_WIN)
		strcat(str, "Win" MODIFIER_SEPARATOR);

	if(button.virtKey)
		strcat(str, GetVirtualKeyName(button.virtKey));
	else if(button.diKey)
		strcat(str, GetDirectInputKeyName(button.diKey));
}


void AddHotkeySuffix(char* str, int id, const char* defaultSuffix, bool isHotkeys)
{
	if(id & 0xFFFF0000)
		return; // ignore non-WORD menu IDs

	std::map<WORD,int>& reverseEventLookup = isHotkeys ? s_reverseEventHotkeyLookup : s_reverseEventInputLookup;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	int index = reverseEventLookup[id]-1;
	if(index < 0)
	{
		strcat(str, "\t");
		if(defaultSuffix)
			strcat(str, defaultSuffix);
		return;
	}

	AddHotkeySuffix(str, buttons[index]);
}


extern HINSTANCE hInst;

void Get_Key_2(InputButton& button, bool allowVirtual, HWND hwnd, bool isHotkeys);
static void SetKey (char* message, InputButton& button, HWND hset, bool isHotkeys)
{
	InputState& inputState = configInputState;
	if(!inputState.lpDI && !Init_Input(hInst, hset, true))
		MessageBox(NULL,"I failed to initialize the input.","Notice",MB_OK);

	for (int i = 0; i < 256; i++)
		inputState.Keys[i] &= ~0x80;

	Get_Key_2(button, true, NULL, isHotkeys);

	MSG m;
	while (PeekMessage(&m, hset, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
	while (PeekMessage(&m, hset, WM_LBUTTONDOWN, WM_MBUTTONDBLCLK, PM_REMOVE));
}



void End_Input(bool isConfigInput)
{
	InputState* pInputState = isConfigInput ? &configInputState : &mainInputState;
	InputState& inputState = *pInputState;

	int i;
	
	if(inputState.lpDI)
	{
		if(inputState.lpDIDMouse)
		{
			inputState.lpDIDMouse->Release();
			inputState.lpDIDMouse = NULL;
		}

		if(inputState.lpDIDKeyboard)
		{
			inputState.lpDIDKeyboard->Release();
			inputState.lpDIDKeyboard = NULL;
		}

		for(i = 0; i < MAX_JOYS; i++)
		{
			if(inputState.Joy_ID[i])
			{
				inputState.Joy_ID[i]->Unacquire();
				inputState.Joy_ID[i]->Release();
			}
		}

		inputState.Nb_Joys = 0;
		inputState.lpDI->Release();
		inputState.lpDI = NULL;
	}
	Save_Config();
}

BOOL CALLBACK InitJoystick(LPCDIDEVICEINSTANCE lpDIIJoy, LPVOID pvRef)
{
	InputState* pInputState = (InputState*)pvRef;
	InputState& inputState = *pInputState;

	//HWND HWnd = (HWND)pvRef;
	extern HWND hWnd;
	HWND HWnd = hWnd;//(HWND)pvRef;

	HRESULT rval;
	LPDIRECTINPUTDEVICE	lpDIJoy;
	DIPROPRANGE diprg;
	int i;
 
	if (inputState.Nb_Joys >= MAX_JOYS) return(DIENUM_STOP);
		
	inputState.Joy_ID[inputState.Nb_Joys] = NULL;

	rval = inputState.lpDI->CreateDevice(lpDIIJoy->guidInstance, &lpDIJoy, NULL);
	if(rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInput::CreateDevice FAILED", "erreur joystick", MB_OK);
		return(DIENUM_CONTINUE);
	}

	rval = lpDIJoy->QueryInterface(IID_IDirectInputDevice2, (void **)&inputState.Joy_ID[inputState.Nb_Joys]);
	lpDIJoy->Release();
	if(rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInputDevice2::QueryInterface FAILED", "erreur joystick", MB_OK);
	    inputState.Joy_ID[inputState.Nb_Joys] = NULL;
	    return(DIENUM_CONTINUE);
	}

	rval = inputState.Joy_ID[inputState.Nb_Joys]->SetDataFormat(&c_dfDIJoystick);
	if(rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInputDevice::SetDataFormat FAILED", "erreur joystick", MB_OK);
		inputState.Joy_ID[inputState.Nb_Joys]->Release();
		inputState.Joy_ID[inputState.Nb_Joys] = NULL;
		return(DIENUM_CONTINUE);
	}

	rval = inputState.Joy_ID[inputState.Nb_Joys]->SetCooperativeLevel(HWnd, DISCL_NONEXCLUSIVE | (/*BackgroundInput*/true?DISCL_BACKGROUND:DISCL_FOREGROUND));

	if(rval != DI_OK)
	{ 
		MessageBox(HWnd, "IDirectInputDevice::SetCooperativeLevel FAILED", "erreur joystick", MB_OK);
		inputState.Joy_ID[inputState.Nb_Joys]->Release();
		inputState.Joy_ID[inputState.Nb_Joys] = NULL;
		return(DIENUM_CONTINUE);
	}
 
	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_X;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = inputState.Joy_ID[inputState.Nb_Joys]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT)) 
		MessageBox(HWnd, "IDirectInputDevice::SetProperty() (X-Axis) FAILED", "erreur joystick", MB_OK);

	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_Y;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = inputState.Joy_ID[inputState.Nb_Joys]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT)) 
		MessageBox(HWnd, "IDirectInputDevice::SetProperty() (Y-Axis) FAILED", "erreur joystick", MB_OK);

	for(i = 0; i < 10; i++)
	{
		rval = inputState.Joy_ID[inputState.Nb_Joys]->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	inputState.Nb_Joys++;

	return(DIENUM_CONTINUE);
}


int Init_Input(HINSTANCE hInst, HWND hWnd, bool isConfigInput)
{
	InputState* pInputState = isConfigInput ? &configInputState : &mainInputState;
	InputState& inputState = *pInputState;

	int i;
	HRESULT rval;

	End_Input(isConfigInput);
	
	StoreDefaultInputButtons();

	rval = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &inputState.lpDI, NULL);
	if(rval != DI_OK)
	{
		MessageBox(hWnd, "DirectInput failed... You must have at least DirectX 5", "Error", MB_OK);
		return 0;
	}
	
	inputState.Nb_Joys = 0;

	for(i = 0; i < MAX_JOYS; i++) inputState.Joy_ID[i] = NULL;

	rval = inputState.lpDI->EnumDevices(DIDEVTYPE_JOYSTICK, &InitJoystick, pInputState, DIEDFL_ATTACHEDONLY);
	if(rval != DI_OK) return 0;

//	rval = inputState.lpDI->CreateDevice(GUID_SysMouse, &inputState.lpDIDMouse, NULL);
	rval = inputState.lpDI->CreateDevice(GUID_SysKeyboard, &inputState.lpDIDKeyboard, NULL);
	if(rval != DI_OK) return 0;

//	rval = inputState.lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	rval = inputState.lpDIDKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | (/*BackgroundInput*/true?DISCL_BACKGROUND:DISCL_FOREGROUND));
	if(rval != DI_OK) return 0;

//	rval = inputState.lpDIDMouse->SetDataFormat(&c_dfDIMouse);
	rval = inputState.lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if(rval != DI_OK) return 0;

//	rval = inputState.lpDIDMouse->Acquire();
	for(i = 0; i < 10; i++)
	{
		rval = inputState.lpDIDKeyboard->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	UpdateReverseLookup(true);
	UpdateReverseLookup(false);

	return 1;
}


void Restore_Input(InputState& inputState)
{
//	inputState.lpDIDMouse->Acquire();
	inputState.lpDIDKeyboard->Acquire();
}

bool Check_Key_Pressed(unsigned int key, InputState& inputState);

void Update_Input(HWND HWnd, bool frameSynced, bool allowExecute, bool isConfigInput, bool isHotkeys)
{
	InputState* pInputState = isConfigInput ? &configInputState : &mainInputState;
	InputState& inputState = *pInputState;

//	DIMOUSESTATE MouseState;
	HRESULT rval = inputState.lpDIDKeyboard->GetDeviceState(256, &inputState.Keys);

	if((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
	{
		Restore_Input(inputState);
		rval = inputState.lpDIDKeyboard->GetDeviceState(256, &inputState.Keys);
		if((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
			memset(inputState.Keys, 0, sizeof(inputState.Keys));
	}

	for(int i = 0; i < inputState.Nb_Joys; i++)
	{
		if(inputState.Joy_ID[i])
		{
			inputState.Joy_ID[i]->Poll();
			rval = inputState.Joy_ID[i]->GetDeviceState(sizeof(inputState.Joy_State[i]), &inputState.Joy_State[i]);
			if(rval != DI_OK) inputState.Joy_ID[i]->Acquire();
		}
	}

//	rval = lpDIDMouse->GetDeviceState(sizeof(MouseState), &MouseState);
	
//	if ((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
//		Restore_Input(inputState);

//  MouseX = MouseState.lX;
//  MouseY = MouseState.lY;

	if(numPendingUnsyncedPresses && frameSynced && HWnd)
	{
		InputButton& button = s_hotkeyButtons[pendingUnsyncedPresses[0]];
		SendMessage(HWnd, WM_COMMAND, button.eventID, 0);
		if(--numPendingUnsyncedPresses)
		{
			memmove(&pendingUnsyncedPresses[0], &pendingUnsyncedPresses[1], numPendingUnsyncedPresses);
			pendingUnsyncedPresses[numPendingUnsyncedPresses] = 0;
		}
		frameSynced = false;
	}

	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	int numButtons = GetNumButtons();
	for(int i=0; i<numButtons; i++)
	{
		InputButton& button = buttons[i];

		bool pressed = button.diKey ? Check_Key_Pressed(button.diKey, inputState) : false;

		if(button.virtKey || button.modifiers)
		{
			bool pressed2 = button.virtKey ? !!(GetAsyncKeyState(button.virtKey) & 0x8000) : true;

			if(pressed2)
			{
				if(isHotkeys)
				{
					if(!button.alias)
					{
						// require exactly the same modifiers
						pressed2 &= !(button.modifiers & MOD_CONTROL) == !(GetAsyncKeyState(VK_CONTROL) & 0x8000);
						pressed2 &= !(button.modifiers & MOD_SHIFT) == !(GetAsyncKeyState(VK_SHIFT) & 0x8000);
						pressed2 &= !(button.modifiers & MOD_ALT) == !(GetAsyncKeyState(VK_MENU) & 0x8000);
						pressed2 &= !(button.modifiers & MOD_WIN) == !((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN)) & 0x8000);
					}
					else // consider hotkeys with an alias game-button-like
					{
						if((button.modifiers & MOD_CONTROL) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) pressed2 = false;
						if((button.modifiers & MOD_SHIFT) && !(GetAsyncKeyState(MOD_SHIFT) & 0x8000)) pressed2 = false;
						pressed2 &= !(button.modifiers & MOD_ALT) == !(GetAsyncKeyState(VK_MENU) & 0x8000); // is mainly special for the alt-tab case
						if((button.modifiers & MOD_WIN) && !(GetAsyncKeyState(MOD_WIN) & 0x8000)) pressed2 = false;
					}
				}
				else
				{
					// require the button's modifiers but allow extra ones if also held (in case the extra modifier is being used as a separate button/action)
					if((button.modifiers & MOD_CONTROL) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) pressed2 = false;
					if((button.modifiers & MOD_SHIFT) && !(GetAsyncKeyState(MOD_SHIFT) & 0x8000)) pressed2 = false;
					if((button.modifiers & MOD_ALT) && !(GetAsyncKeyState(MOD_ALT) & 0x8000)) pressed2 = false;
					if((button.modifiers & MOD_WIN) && !(GetAsyncKeyState(MOD_WIN) & 0x8000)) pressed2 = false;
				}
			}

			if(!button.diKey)
				pressed = true;

			if(!pressed2)
				pressed = false;
		}

		if(button.alias)
			*button.alias = pressed;

		bool oldPressed = button.heldNow;
		button.heldNow = pressed;

		if(pressed && !oldPressed)
		{
			//if(pressed && !oldPressed && button.eventID && ((BackgroundInput && !(GetActiveWindow()==HWnd)) || !button.ShouldUseAccelerator()))
			if(button.eventID && allowExecute && HWnd)
			{
				if(!(button.modifiers & (MOD_CONTROL|MOD_SHIFT|MOD_WIN)))
				{
					HWND focus = GetFocus();
					if(focus)
					{
						char text[5];
						GetClassNameA(focus, text, 5);
						if(!_stricmp(text, "Edit"))
						{
							// if typing into an edit field, don't interpret it as hotkey presses
							continue;
						}
					}
				}

				if(button.requiresFrameSync)
				{
					if(frameSynced)
					{
						frameSynced = false;
					}
					else
					{
						if(started)
							if(numPendingUnsyncedPresses < MAX_PENDIND_UNSYNCED)
								pendingUnsyncedPresses[numPendingUnsyncedPresses++] = i;
						continue;
					}
				}
				SendMessage(HWnd, WM_COMMAND, button.eventID, 777);
			}
		}
	}
}


bool IsHotkeyPress(bool isConfigInput)
{
	InputState* pInputState = isConfigInput ? &configInputState : &mainInputState;
	InputState& inputState = *pInputState;

	HRESULT rval = inputState.lpDIDKeyboard->GetDeviceState(256, &tempInputState.Keys);
	if((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
	{
		Restore_Input(inputState);
		rval = inputState.lpDIDKeyboard->GetDeviceState(256, &tempInputState.Keys);
		if((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
			memset(tempInputState.Keys, 0, sizeof(tempInputState.Keys));
	}

	for(int i = 0; i < inputState.Nb_Joys; i++)
	{
		if(inputState.Joy_ID[i])
		{
			inputState.Joy_ID[i]->Poll();
			rval = inputState.Joy_ID[i]->GetDeviceState(sizeof(inputState.Joy_State[i]), &inputState.Joy_State[i]);
			if(rval != DI_OK) inputState.Joy_ID[i]->Acquire();
		}
	}

	int numButtons = GetNumHotkeys();
	for(int i=0; i<numButtons; i++)
	{
		InputButton& button = s_hotkeyButtons[i];

		bool pressed = button.diKey ? Check_Key_Pressed(button.diKey, tempInputState) : false;

		if(button.virtKey || button.modifiers)
		{
			bool pressed2 = button.virtKey ? !!(GetAsyncKeyState(button.virtKey) & 0x8000) : true;

			if(pressed2)
			{
				if(!button.alias)
				{
					// require exactly the same modifiers
					pressed2 &= !(button.modifiers & MOD_CONTROL) == !(GetAsyncKeyState(VK_CONTROL) & 0x8000);
					pressed2 &= !(button.modifiers & MOD_SHIFT) == !(GetAsyncKeyState(VK_SHIFT) & 0x8000);
					pressed2 &= !(button.modifiers & MOD_ALT) == !(GetAsyncKeyState(VK_MENU) & 0x8000);
					pressed2 &= !(button.modifiers & MOD_WIN) == !((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN)) & 0x8000);
				}
				else // consider hotkeys with an alias game-button-like
				{
					if((button.modifiers & MOD_CONTROL) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) pressed2 = false;
					if((button.modifiers & MOD_SHIFT) && !(GetAsyncKeyState(MOD_SHIFT) & 0x8000)) pressed2 = false;
					pressed2 &= !(button.modifiers & MOD_ALT) == !(GetAsyncKeyState(VK_MENU) & 0x8000); // is mainly special for the alt-tab case
					if((button.modifiers & MOD_WIN) && !(GetAsyncKeyState(MOD_WIN) & 0x8000)) pressed2 = false;
				}
			}

			if(!button.diKey)
				pressed = true;

			if(!pressed2)
				pressed = false;
		}

		//bool oldPressed = button.heldNow;
		if(pressed /*&& !oldPressed*/)
		{
			if(button.eventID)
			{
				if(!(button.modifiers & (MOD_CONTROL|MOD_SHIFT|MOD_WIN)))
				{
					HWND focus = GetFocus();
					if(focus)
					{
						char text[5];
						GetClassNameA(focus, text, 5);
						if(!_stricmp(text, "Edit"))
						{
							// if typing into an edit field, don't interpret it as hotkey presses
							continue;
						}
					}
				}
			}

			return true;
		}
	}
	return false;
}


bool Check_Key_Pressed(unsigned int key, InputState& inputState)
{
	int Num_Joy;

	if (key < 0x100)
	{
		if KEYDOWN(key)
			return(1);
	}
	else
	{
		Num_Joy = ((key >> 8) & 0xF);

		if (inputState.Joy_ID[Num_Joy])
		{
			if (key & 0x80)			// Test POV Joys
			{
				int value = inputState.Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3];
				if (value == -1) return (0);
				switch(key & 0xF)
				{
					case 1:
						if ((value >= 29250) || (value <=  6750))
							return(1);
						break;
					case 2:
						if ((value >=  2250) && (value <= 15750))
							return(1);
						break;
					case 3:
						if ((value >= 11250) && (value <= 24750))
							return(1);
						break;
					case 4:
						if ((value >= 20250) && (value <= 33750))
							return(1);
						break;
				}

			}
			else if (key & 0x70)		// Test Button Joys
			{
				if (inputState.Joy_State[Num_Joy].rgbButtons[(key & 0xFF) - 0x10])
					return(1);
			}
			else
			{
				switch(key & 0xF)
				{
					case 1:
						if (inputState.Joy_State[Num_Joy].lY < -500)
							return(1);
						break;

					case 2:
						if (inputState.Joy_State[Num_Joy].lY > +500)
							return(1);
						break;

					case 3:
						if (inputState.Joy_State[Num_Joy].lX < -500)
							return(1);
						break;

					case 4:
						if (inputState.Joy_State[Num_Joy].lX > +500)
							return(1);
						break;

					case 5:
						if (inputState.Joy_State[Num_Joy].lRx < 0x3FFF)
							return(1);
						break;
					case 6:
						if (inputState.Joy_State[Num_Joy].lRx > 0xBFFF)
							return(1);
						break;
					case 7:
						if (inputState.Joy_State[Num_Joy].lRy < 0x3FFF)
							return(1);
						break;
					case 8:
						if (inputState.Joy_State[Num_Joy].lRy > 0xBFFF)
							return(1);
						break;
					case 9:
						if (inputState.Joy_State[Num_Joy].lZ < 0x3FFF)
							return(1);
						break;
					case 0xA:
						if (inputState.Joy_State[Num_Joy].lZ > 0xBFFF)
							return(1);
						break;
				}
			}
		}
	}

	return 0;
}


void Get_Key_2(InputButton& button, bool allowVirtual, HWND hWnd, bool isHotkeys)
{
	InputState& inputState = configInputState;
	int i, j, joyIndex;

	bool prevReady = false;
	int curAlt,curCtrl,curSft,curWin = curSft = curCtrl = curAlt = 0;
	int prevAlt,prevCtrl,prevSft,prevWin = prevSft = prevCtrl = prevAlt = 0;

	int prevMod;
	BOOL prevDiKeys[256];
	BOOL prevVirtKeys[256];
	BOOL prevJoyKeys[256];

	int curMod;
	BOOL curDiKeys[256];
	BOOL curVirtKeys[256];
	BOOL curJoyKeys[256];

	while(1)
	{
		// compute the current state of all buttons
		{
			Update_Input(NULL, true, false, true, isHotkeys);
			Sleep(1);

			// current state of virtual windows keys
			for(i = 0; i < 256; i++)
				curVirtKeys[i] = (GetAsyncKeyState(i) & 0x8000);

			// current state of direct input keys
			for(i = 0; i < 256; i++)
				curDiKeys[i] = KEYDOWN(i);

			// current state of modifier keys
			curMod = 0;
			prevCtrl = curCtrl;
			prevSft = curSft;
			prevAlt = curAlt;
			prevWin = curWin;
			if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
				curMod |= MOD_CONTROL, curCtrl = (curDiKeys[DIK_LCONTROL]?1:0) | (curDiKeys[DIK_RCONTROL]?2:0);
			if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
				curMod |= MOD_SHIFT, curSft = (curDiKeys[DIK_LSHIFT]?1:0) | (curDiKeys[DIK_RSHIFT]?2:0);
			if(GetAsyncKeyState(VK_MENU) & 0x8000)
				curMod |= MOD_ALT, curAlt = (curDiKeys[DIK_LMENU]?1:0) | (curDiKeys[DIK_RMENU]?2:0);
			if((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN)) & 0x8000)
				curMod |= MOD_WIN, curWin =  (curDiKeys[DIK_LWIN]?1:0) | (curDiKeys[DIK_RWIN]?2:0);

			// current state of recognized buttons on joypad
			joyIndex = 0;
			for(i = 0; i < inputState.Nb_Joys; i++)
			{
				if (inputState.Joy_ID[i])
				{
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lY < -500);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lY > +500);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lX < -500);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lX > +500);
					for (j = 0; j < 4; j++) {
						curJoyKeys[joyIndex++] = (inputState.Joy_State[i].rgdwPOV[j] == 0);
						curJoyKeys[joyIndex++] = (inputState.Joy_State[i].rgdwPOV[j] == 9000);
						curJoyKeys[joyIndex++] = (inputState.Joy_State[i].rgdwPOV[j] == 18000);
						curJoyKeys[joyIndex++] = (inputState.Joy_State[i].rgdwPOV[j] == 27000);
					}
					for (j = 0; j < 32; j++)
						curJoyKeys[joyIndex++] = (inputState.Joy_State[i].rgbButtons[j]);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lRx < 0x3FFF);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lRx > 0xBFFF);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lRy < 0x3FFF);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lRy > 0xBFFF);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lZ < 0x3FFF);
					curJoyKeys[joyIndex++] = (inputState.Joy_State[i].lZ > 0xBFFF);
				}
			}
		}

		// compare buttons against the previous state
		// to determine what is now pressed that wasn't already pressed before
		if(prevReady)
		{
			// check for new virtual key presses
			for(i = 1; i < 255; i++)
			{
				if(curVirtKeys[i] && !prevVirtKeys[i])
				{
					if(allowVirtual)
					{
						if(i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON)
						{
							extern HWND HotkeyHWnd;
							if(HotkeyHWnd)
							{
								POINT p;
								GetCursorPos(&p);
								RECT r;
								GetClientRect(HotkeyHWnd, &r);
								ClientToScreen(HotkeyHWnd, (LPPOINT)&r.left);
								ClientToScreen(HotkeyHWnd, (LPPOINT)&r.right);
								if(!PtInRect(&r,p))
									return; // clicked outside of config window, cancel assignment instead of assigning a click
							}
						}
						if(i == VK_CONTROL || i == VK_SHIFT || i == VK_MENU || i == VK_LWIN || i == VK_RWIN || i == VK_LSHIFT || i == VK_RSHIFT || i == VK_LCONTROL || i == VK_RCONTROL || i == VK_LMENU || i == VK_RMENU)
							continue;
						button.SetAsVirt(i, curMod);
						return;
					}
					else if(i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON || i == VK_CANCEL)
					{
						button.SetAsDIK(0);
						return;
					}
				}
			}

			// check for new direct input key presses
			for(i = 1; i < 255; i++)
			{
				if(curDiKeys[i] && !prevDiKeys[i])
				{
					if(allowVirtual && (i == DIK_LWIN || i == DIK_RWIN || i == DIK_LSHIFT || i == DIK_RSHIFT || i == DIK_LCONTROL || i == DIK_RCONTROL || i == DIK_LMENU || i == DIK_RMENU))
						continue;
					button.SetAsDIK(i, curMod);
					return;
				}
			}

			// check for modifier key releases
			// this allows a modifier key to be used as a hotkey on its own, as some people like to do
			if(!curMod && prevMod)
			{
				switch (prevMod)
				{
					case MOD_ALT:
						button.SetAsDIK((prevAlt == 2)? DIK_RMENU : DIK_LMENU, curMod);	//for some reason, it won't read these if set as VirtKeys.
						break;
					case MOD_CONTROL:
						button.SetAsDIK((prevCtrl == 2)? DIK_RCONTROL : DIK_LCONTROL, curMod);
						break;
					case MOD_SHIFT:
						button.SetAsDIK((prevSft == 2)? DIK_RSHIFT : DIK_LSHIFT, curMod);
						break;
					case MOD_WIN:
						button.SetAsDIK((prevWin == 2)? DIK_RWIN : DIK_LWIN, curMod);
						break;
				}
				return;
			}
			if(curSft == 3) // both LSHIFT + RSHIFT held = assign them both to the hotkey
				{ button.SetAsVirt(0, MOD_SHIFT); return; }
			if(curAlt == 3)
				{ button.SetAsVirt(0, MOD_ALT); return; }
			if(curCtrl == 3)
				{ button.SetAsVirt(0, MOD_CONTROL); return; }
			if(curWin == 3)
				{ button.SetAsVirt(0, MOD_WIN); return; }

			// check for new recognized joypad button presses
			for(int index = 0; index < joyIndex; index++)
			{
				if(curJoyKeys[index] && !prevJoyKeys[index])
				{
					int joyIndex2 = 0;
					for(i = 0; i < inputState.Nb_Joys; i++)
					{
						if (inputState.Joy_ID[i])
						{
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x1, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x2, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x3, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x4, curMod); return; }
							for (j = 0; j < 4; j++) {
								if(index == joyIndex2++) { button.SetAsDIK(0x1080 + (0x100 * i) + (0x10 * j) + 0x1, curMod); return; }
								if(index == joyIndex2++) { button.SetAsDIK(0x1080 + (0x100 * i) + (0x10 * j) + 0x2, curMod); return; }
								if(index == joyIndex2++) { button.SetAsDIK(0x1080 + (0x100 * i) + (0x10 * j) + 0x3, curMod); return; }
								if(index == joyIndex2++) { button.SetAsDIK(0x1080 + (0x100 * i) + (0x10 * j) + 0x4, curMod); return; }
							}
							for (j = 0; j < 32; j++)
								if(index == joyIndex2++) { button.SetAsDIK(0x1010 + (0x100 * i) + j, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x5, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x6, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x7, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x8, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0x9, curMod); return; }
							if(index == joyIndex2++) { button.SetAsDIK(0x1000 + (0x100 * i) + 0xA, curMod); return; }
						}
					}
				}
			}
		}

		// update previous state
		memcpy(prevVirtKeys, curVirtKeys, sizeof(prevVirtKeys));
		memcpy(prevDiKeys, curDiKeys, sizeof(curDiKeys));
		memcpy(prevJoyKeys, curJoyKeys, sizeof(curJoyKeys));
		prevMod = curMod;
		prevReady = true;
	}
}

//unsigned int Get_Key(HWND hWnd, bool isHotkeys)
//{
//	InputButton tempButton;
//	tempButton.diKey = 0;
//	Get_Key_2(tempButton, false, hWnd, isHotkeys);
//	return tempButton.diKey;
//}


LRESULT CALLBACK HotkeysOrInputsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isHotkeys)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static HWND Tex0 = NULL;
	extern HWND hWnd;
	extern HWND HotkeyHWnd;

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

			if (!Init_Input(hInst, hDlg, true)) return false;

			EnableWindow(GetDlgItem(hDlg, IDC_REASSIGNKEY), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_REVERTKEY), FALSE);
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
				EnableWindow(GetDlgItem(hDlg, IDC_REVERTKEY), (selCount >= 1) ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_USEDEFAULTKEY), (selCount >= 1) ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISABLEKEY), (selCount >= 1) ? TRUE : FALSE);
			}

			switch(controlID)
			{
				case IDOK:
					End_Input(true);
					HotkeyHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
					break;

				case IDC_REASSIGNKEY:
				case IDC_REVERTKEY:
				case IDC_USEDEFAULTKEY:
				case IDC_DISABLEKEY:
					ModifyHotkeyFromListbox(GetDlgItem(hDlg, IDC_HOTKEYLIST), controlID, Tex0, hDlg, isHotkeys);
					break;
			}
		}	break;
		case WM_CLOSE:
			End_Input(true);
			HotkeyHWnd = NULL;
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

void ModifyHotkeyFromListbox(HWND listbox, WORD command, HWND statusText, HWND parentWindow, bool isHotkeys)
{
	std::map<WORD,int>& reverseEventLookup = isHotkeys ? s_reverseEventHotkeyLookup : s_reverseEventInputLookup;
	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	StoreInitialInputButtons();

//	bool rebuildAccelerators = false;

	int numHotkeys = GetNumButtons();
	for(int i=0; i<numHotkeys; i++)
	{
		int selected = SendMessage((HWND) listbox, (UINT) LB_GETSEL, (WPARAM) i, (LPARAM) 0);
		if(selected <= 0)
			continue;

		InputButton& button = buttons[i];

		switch(command)
		{
			case IDC_REASSIGNKEY:
				{
					char str [256];
					sprintf(str, "SETTING KEY: %s", button.description);
					SetWindowText(statusText, str);
					SetKey(str, button, parentWindow, isHotkeys);
					SetWindowText(statusText, "");

					// for convenience, set all similar savestate buttons together when the first one is set to certain keys
					if(button.virtKey == '1' || button.virtKey == VK_F1)
					{
						if(button.eventID == ID_FILES_SAVESTATE_1
						|| button.eventID == ID_FILES_SAVESTATE_11
						|| button.eventID == ID_FILES_LOADSTATE_1
						|| button.eventID == ID_FILES_LOADSTATE_11
						|| button.eventID == ID_FILES_SETSTATE_1)
						{
							for(int j=1;j<=9;j++)
							{
								int index2 = reverseEventLookup[button.eventID+j]-1;
								if(index2 >= 0)
								{
									InputButton& otherButton = buttons[index2];

									otherButton.diKey = 0;
									otherButton.modifiers = button.modifiers;
									int vk = button.virtKey + j; if(vk == '1' + 9) vk = '0';
									otherButton.virtKey = vk;

									char str [1024];
									strcpy(str, otherButton.description);
									AddHotkeySuffix(str, otherButton);

									SendMessage(listbox, LB_DELETESTRING, index2, 0);  
									SendMessage(listbox, LB_INSERTSTRING, index2, (LPARAM) str); 
								}
							}
						}
					}
				}
				break;
			case IDC_REVERTKEY:
				(isHotkeys ? s_initialHotkeyButtons : s_initialInputButtons)[i].CopyConfigurablePartsTo(button);
				break;
			case IDC_USEDEFAULTKEY:
				(isHotkeys ? s_defaultHotkeyButtons : s_defaultInputButtons)[i].CopyConfigurablePartsTo(button);
				break;
			case IDC_DISABLEKEY:
				button.modifiers = MOD_NONE;
				button.virtKey = VK_NONE;
				button.diKey = 0;
				break;
		}
		SetFocus(listbox);

		char str [1024];
		strcpy(str, button.description);
		AddHotkeySuffix(str, button);

		SendMessage(listbox, LB_DELETESTRING, i, 0);  
		SendMessage(listbox, LB_INSERTSTRING, i, (LPARAM) str); 
		SendMessage(listbox, LB_SETSEL, (WPARAM) TRUE, (LPARAM) i); 
	}

	UpdateReverseLookup(isHotkeys);

	extern bool mainMenuNeedsRebuilding;
	mainMenuNeedsRebuilding = true;
}

void PopulateHotkeyListbox(HWND listbox, bool isHotkeys)
{
	int(*GetNumButtons)() = isHotkeys ? GetNumHotkeys : GetNumInputs;
	InputButton* buttons = isHotkeys ? s_hotkeyButtons : s_inputButtons;

	StoreDefaultInputButtons();
	int numInputButtons = GetNumButtons();
	for(int i=0; i<numInputButtons; i++)
	{
		InputButton& button = buttons[i];

		char str [1024];
		strcpy(str, button.description);
		AddHotkeySuffix(str, button);

		SendMessage((HWND) listbox, (UINT) LB_ADDSTRING, (WPARAM) 0, (LPARAM) str);
	}
}




char Str_Tmp [1024] = {0};




// I should really put some of this crap in a header...
extern int audioFrequency;
extern int audioBitsPerSecond;
extern int audioChannels;
extern bool paused;
extern bool fastforward;
extern bool started;
extern bool playback;
extern bool finished;
extern bool nextLoadRecords;
extern bool recoveringStale;
extern bool exeFileExists;
extern bool movieFileExists;
extern bool movieFileWritable;
extern int forceWindowed;
extern int truePause;
extern int onlyHookChildProcesses;
extern int forceSurfaceMemory;
extern int forceSoftware;
extern int aviMode;
extern int emuMode;
extern int fastForwardFlags;
extern int timescale, timescaleDivisor;
extern int allowLoadInstalledDlls, allowLoadUxtheme, runDllLast;
extern int advancePastNonVideoFrames;
extern bool advancePastNonVideoFramesConfigured;
extern int threadMode;
extern int usedThreadMode;
extern int timersMode;
extern int messageSyncMode;
extern int waitSyncMode;
extern int aviFrameCount;
extern int aviSoundFrameCount;
extern bool traceEnabled;
extern bool crcVerifyEnabled;
extern int storeVideoMemoryInSavestates;
extern int storeGuardedPagesInSavestates;
extern int appLocale;
extern int tempAppLocale;
extern int debugPrintMode;
extern LogCategoryFlag includeLogFlags;
extern LogCategoryFlag traceLogFlags;
extern LogCategoryFlag excludeLogFlags;

extern char moviefilename [MAX_PATH+1];
extern char exefilename [MAX_PATH+1];
extern char commandline [160];
extern char thisprocessPath [MAX_PATH+1];

static const char* defaultConfigFilename = "hourglass.cfg";

#define SetPrivateProfileIntA(lpAppName, lpKeyName, nValue, lpFileName) \
	sprintf(Str_Tmp, "%d", nValue); \
	WritePrivateProfileStringA(lpAppName, lpKeyName, Str_Tmp, lpFileName);

int Save_Config(const char* filename)
{
	if(!filename)
		filename = defaultConfigFilename;
	char Conf_File[1024];

	//SetCurrentDirectoryA(thisprocessPath);
	//strcpy(Conf_File, filename);
	if(*filename && filename[1] == ':')
		strcpy(Conf_File, filename);
	else
		sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

	WritePrivateProfileStringA("General", "Exe path", exefilename, Conf_File);
	WritePrivateProfileStringA("General", "Movie path", moviefilename, Conf_File);
	WritePrivateProfileStringA("General", "Command line", commandline, Conf_File);

	//for(int i = 0; i < MAX_RECENT_ROMS; i++)
	//{
	//	char str[256];
	//	sprintf(str, "Rom %d", i+1);
	//	WritePrivateProfileStringA("General", str, Recent_Rom[i], Conf_File);
	//}

	//for(int i = 0; i < MAX_RECENT_SCRIPTS; i++)
	//{
	//	char str[256];
	//	sprintf(str, "Script %d", i+1);
	//	WritePrivateProfileStringA("General", str, Recent_Scripts[i], Conf_File);
	//}

	SetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
	SetPrivateProfileIntA("Graphics", "Force Windowed", forceWindowed, Conf_File);
	SetPrivateProfileIntA("Tools", "Fast Forward Flags", fastForwardFlags, Conf_File);
	if(advancePastNonVideoFramesConfigured)
		SetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
	SetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
	SetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

	SetPrivateProfileIntA("Debug", "Debug Logging Mode", debugPrintMode, Conf_File);
	SetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
	SetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

	wsprintf(Str_Tmp, "%d", AutoRWLoad);
	WritePrivateProfileString("Watches", "AutoLoadWatches", Str_Tmp, Conf_File);
	
	wsprintf(Str_Tmp, "%d", RWSaveWindowPos);
	WritePrivateProfileString("Watches", "SaveWindowPosition", Str_Tmp, Conf_File);
	
	if (RWSaveWindowPos)
	{
	SetPrivateProfileIntA("Watches", "Ramwatch_X", ramw_x, Conf_File);
	SetPrivateProfileIntA("Watches", "Ramwatch_Y", ramw_y, Conf_File);
	}

	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		WritePrivateProfileStringA("Watches", str, &rw_recent_files[i][0], Conf_File);	
	}

	SaveHotkeys(Conf_File, true);
	SaveHotkeys(Conf_File, false);

	return 1;
}


int Save_As_Config(HWND hWnd)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectoryA(thisprocessPath);

	strcpy(&Name[0], defaultConfigFilename);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = thisprocessPath;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if(GetSaveFileName(&ofn))
	{
		Save_Config(Name);
		//strcpy(Str_Tmp, "config saved in ");
		//strcat(Str_Tmp, Name);
		//Put_Info(Str_Tmp, 2000);
		debugprintf("config saved in \"%s\".\n", Name);
		return 1;
	}
	else return 0;
}


int Load_Config(const char* filename)
{
	if(!filename)
		filename = defaultConfigFilename;
	char Conf_File[1024];

	//SetCurrentDirectoryA(thisprocessPath);
	//strcpy(Conf_File, filename);
	if(*filename && filename[1] == ':')
		strcpy(Conf_File, filename);
	else
		sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

	GetPrivateProfileStringA("General", "Exe path", exefilename, exefilename, MAX_PATH, Conf_File);
	GetPrivateProfileStringA("General", "Movie path", moviefilename, moviefilename, MAX_PATH, Conf_File);
	GetPrivateProfileStringA("General", "Command line", commandline, commandline, ARRAYSIZE(commandline), Conf_File);

	//for(int i = 0; i < MAX_RECENT_ROMS; i++)
	//{
	//	char str[256];
	//	sprintf(str, "Rom %d", i+1);
	//	GetPrivateProfileStringA("General", str, "", &Recent_Rom[i][0], 1024, Conf_File);
	//}

	//for(int i = 0; i < MAX_RECENT_SCRIPTS; i++)
	//{
	//	char str[256];
	//	sprintf(str, "Script %d", i+1);
	//	GetPrivateProfileStringA("General", str, "", &Recent_Scripts[i][0], 1024, Conf_File);
	//}

	nextLoadRecords = 0!=GetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
	forceWindowed = GetPrivateProfileIntA("Graphics", "Force Windowed", forceWindowed, Conf_File);
	fastForwardFlags = GetPrivateProfileIntA("Tools", "Fast Forward Flags", fastForwardFlags, Conf_File);
	advancePastNonVideoFrames = GetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
	advancePastNonVideoFramesConfigured = 0!=GetPrivateProfileIntA("Input", "Skip Lag Frames", 0, Conf_File);
	inputFocusFlags = GetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
	hotkeysFocusFlags = GetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

	debugPrintMode = GetPrivateProfileIntA("Debug", "Debug Logging Mode", debugPrintMode, Conf_File);
	traceEnabled = 0!=GetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
	crcVerifyEnabled = 0!=GetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

	if (RWSaveWindowPos)
	{
	ramw_x = GetPrivateProfileIntA ("Watches", "Ramwatch_X", 0, Conf_File);
	ramw_y = GetPrivateProfileIntA ("Watches", "Ramwatch_Y", 0, Conf_File);
	}

	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		GetPrivateProfileStringA("Watches", str, "", &rw_recent_files[i][0], 1024, Conf_File);
	}

	LoadHotkeys(Conf_File, true);
	LoadHotkeys(Conf_File, false);

	// done loading

	UpdateReverseLookup(true);
	UpdateReverseLookup(false);
	
	static bool first = true;
	if(first)
		first = false;
	else
		debugprintf("loaded config from \"%s\".\n", Conf_File);

	return 1;
}


int Load_As_Config(HWND hWnd)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectoryA(thisprocessPath);

	strcpy(&Name[0], defaultConfigFilename);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = thisprocessPath;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		Load_Config(Name);
		//strcpy(Str_Tmp, "config loaded from ");
		//strcat(Str_Tmp, Name);
		//Put_Info(Str_Tmp, 2000);
		return 1;
	}
	else return 0;
}








#define MENU_L(smenu, pos, flags, id, suffixe, str, disableReason)	do{									\
/*GetPrivateProfileStringA(language_name[Language], (str), (def), Str_Tmp, 1024, Language_Path);*/	\
if((disableReason) && ((flags) & (MF_DISABLED|MF_GRAYED))) \
	sprintf(Str_Tmp, "%s (%s)", str, disableReason); \
else \
	strcpy(Str_Tmp, str); \
/*strcat(Str_Tmp, (suffixe));*/ AddHotkeySuffix(Str_Tmp, id, suffixe, true);							\
InsertMenu((smenu), (pos), (flags), (id), Str_Tmp); } while(0)


void Build_Main_Menu(HMENU& MainMenu, HWND hWnd)
{
	unsigned int Flags;
	int i=0, j=0;
	char str [1024];

	if(MainMenu)
		DestroyMenu(MainMenu);

	MainMenu = CreateMenu();

	HMENU Files = CreatePopupMenu();
	HMENU Graphics = CreatePopupMenu();
	HMENU CPU = CreatePopupMenu();
	HMENU Sound = CreatePopupMenu();
	HMENU Input = CreatePopupMenu();
	HMENU Exec = CreatePopupMenu();
	HMENU Options = CreatePopupMenu();
	HMENU TAS_Tools = CreatePopupMenu();
	//HMENU Time = CreatePopupMenu();
	HMENU Time = TAS_Tools;
	HMENU Avi = CreatePopupMenu();
	HMENU Help = CreatePopupMenu();
	//HMENU FilesChangeState = CreatePopupMenu();
	//HMENU FilesSaveState = CreatePopupMenu();
	//HMENU FilesLoadState = CreatePopupMenu();
	//HMENU FilesHistory = CreatePopupMenu();
	//HMENU GraphicsRender = CreatePopupMenu();
	HMENU GraphicsMemory = CreatePopupMenu();
	//HMENU GraphicsLayers = CreatePopupMenu();
	//HMENU WindowOptions = CreatePopupMenu();
	//HMENU CPUSlowDownSpeed = CreatePopupMenu();
	HMENU SoundFormat = CreatePopupMenu();
	HMENU TimeFastForward = CreatePopupMenu();
	HMENU TimeRate = CreatePopupMenu();
	HMENU ExecMultithreading = CreatePopupMenu();
	HMENU ExecTimers = CreatePopupMenu();
	HMENU ExecMessageSync = CreatePopupMenu();
	HMENU ExecWaitSync = CreatePopupMenu();
	HMENU ExecDlls = CreatePopupMenu();
	HMENU Locale = CreatePopupMenu();
	HMENU DebugLogging = CreatePopupMenu();
	HMENU DebugLoggingInclude = CreatePopupMenu();
	HMENU DebugLoggingTrace = CreatePopupMenu();
	HMENU DebugLoggingExclude = CreatePopupMenu();
	HMENU Performance = CreatePopupMenu();
	//HMENU Tools_Movies = CreatePopupMenu();
	//HMENU Tools_AVI = CreatePopupMenu();
	//HMENU Lua_Script = CreatePopupMenu();
	HMENU InputHotkeyFocus = CreatePopupMenu();
	HMENU InputInputFocus = CreatePopupMenu();



	const char* exefilenameonly = max(strrchr(exefilename,'\\'),strrchr(exefilename,'/'));
	if(exefilenameonly) ++exefilenameonly;
	if(!exeFileExists) exefilenameonly = NULL;
	const char* moviefilenameonly = max(strrchr(moviefilename,'\\'),strrchr(moviefilename,'/'));
	if(moviefilenameonly) ++moviefilenameonly;
	if(!movieFileExists) moviefilenameonly = NULL;
	bool on = true;

	Flags = MF_BYPOSITION | MF_POPUP | MF_STRING;

	i = 0;
	MENU_L(MainMenu, i++, Flags, (UINT)Files, "", "&File", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)Graphics, "", "&Graphics", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)Sound, "", "&Sound", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)Exec, "", "&Runtime", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)Input, "", "&Input", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)TAS_Tools, "", "&Tools", 0);
	MENU_L(MainMenu, i++, Flags, (UINT)Avi, "", (aviMode&&started&&(aviFrameCount||aviSoundFrameCount)) ? ((aviFrameCount&&aviSoundFrameCount)?"&AVI!!":"&AVI!") : "&AVI", 0);
//	MENU_L(MainMenu, i++, Flags, (UINT)Help, "", "&Help", 0);


	// File Menu
	Flags = MF_BYPOSITION | MF_STRING;
	i = 0;

	on = exefilenameonly && !started;
	sprintf(str, "&Open Executable... %s%s%s", on?"(now open: \"":"", on?exefilenameonly:"", on?"\")":"");
	MENU_L(Files, i++, Flags | (started?MF_GRAYED:0), ID_FILES_OPENEXE, "\tCtrl+O", str, "can't change while running");
	on = moviefilenameonly && !started;
	sprintf(str, "&Open Movie... %s%s%s", on?"(now open: \"":"", on?moviefilenameonly:"", on?"\")":"");
	MENU_L(Files, i++, Flags | (started?MF_GRAYED:0), ID_FILES_OPENMOV, "\tCtrl+M", str, "can't change while running");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

//	MENU_L(Files, i++, Flags, ID_FILES_PLAYMOV, "\tCtrl+P", "&Play Movie...", 0);
//	MENU_L(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_WATCHMOV, "", "&Watch From Beginning", "must be running");
	sprintf(str, started ? "&Watch From Beginning" : "&Play Movie %s%s%s", moviefilenameonly?"\"":"", moviefilenameonly?moviefilenameonly:"", moviefilenameonly?"\"":"");
	MENU_L(Files, i++, Flags | (!(movieFileExists&&exeFileExists)?MF_GRAYED:0), started ? ID_FILES_WATCHMOV : ID_FILES_PLAYMOV, "\tCtrl+P", str, movieFileExists ? "must open an executable first" : "must open a movie first");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Files, i++, Flags | ((started||!exeFileExists)?MF_GRAYED:0), ID_FILES_RECORDMOV, "\tCtrl+R", "&Record New Movie...", started ? "must stop running first" : "must open an executable first");
	MENU_L(Files, i++, Flags | ((!started||playback)?MF_GRAYED:0), ID_FILES_BACKUPMOVIE, "", "Backup Movie to File...", "Must be recording");
	//MENU_L(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_RESUMEMOVAS, "", (playback||!started) ? "Resume Recording to &Different File..." : "Continue Recording to &Different File...", "must be running");
	MENU_L(Files, i++, Flags | ((!started||!playback||finished)?MF_GRAYED:0), ID_FILES_RESUMERECORDING, "", "Resume Recording from Now", "movie must be playing");
	//MENU_L(Files, i++, Flags | ((!started||finished)?MF_GRAYED:0), ID_FILES_SPLICE, "", "Splice...", "movie must be playing or recording");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	//MENU_L(Files, i++, Flags, ID_FILES_SAVECONFIG, "", "Save Config", 0);
	MENU_L(Files, i++, Flags, ID_FILES_SAVECONFIGAS, "", "Save Config As...", 0);
	MENU_L(Files, i++, Flags, ID_FILES_LOADCONFIGFROM, "", "Load Config From...", 0);

	//InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	//if (strcmp(Recent_Rom[0], ""))
	//{
	//	MENU_L(Files, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)FilesHistory, "", "&Recent EXEs", 0);
	//	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	//}

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_STOP_RELAY, "", "Stop Running", "already stopped");
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Files, i++, Flags, ID_FILES_QUIT, "Alt F4", "E&xit", 0);

	//// Menu FilesChangeState
	//
	//MENU_L(FilesChangeState, i++, Flags, ID_FILES_PREVIOUSSTATE, "Previous State", "", "Previous State");
	//MENU_L(FilesChangeState, i++, Flags, ID_FILES_NEXTSTATE, "Next State", "", "Next State");
	//InsertMenu(FilesChangeState, i++, MF_SEPARATOR, NULL, NULL);
	//for(j = 0; j < 10; j++)
	//{
	//	sprintf(Str_Tmp ,"Set &%d", (j+1)%10);
	//	MENU_L(FilesChangeState, i++, Flags | (Current_State == ((j+1)%10) ? MF_CHECKED : MF_UNCHECKED), ID_FILES_SETSTATE_1 + j, Str_Tmp, "", Str_Tmp);
	//}

	//MENU_L(FilesSaveState, i++, Flags, ID_FILES_SAVESTATE, "Save State", "\tF5", "Quick &Save");
	//MENU_L(FilesSaveState, i++, Flags, ID_FILES_SAVESTATEAS, "Save State as", "\tShift+F5", "&Save State as...");
	//InsertMenu(FilesSaveState, i++, MF_SEPARATOR, NULL, NULL);
	//for(j = 0; j < 10; j++)
	//{
	//	sprintf(Str_Tmp ,"Save &%d", (j+1)%10);
	//	MENU_L(FilesSaveState, i++, Flags, ID_FILES_SAVESTATE_1 + j, Str_Tmp, "", Str_Tmp);
	//}

	//MENU_L(FilesLoadState, i++, Flags, ID_FILES_LOADSTATE, "Load State", "\tF8", "Quick &Load");
	//MENU_L(FilesLoadState, i++, Flags, ID_FILES_LOADSTATEAS, "Load State as", "\tShift+F8", "&Load State...");
	//InsertMenu(FilesLoadState, i++, MF_SEPARATOR, NULL, NULL);
	//for(j = 0; j < 10; j++)
	//{
	//	sprintf(Str_Tmp ,"Load &%d", (j+1)%10);
	//	MENU_L(FilesLoadState, i++, Flags, ID_FILES_LOADSTATE_1 + j, Str_Tmp, "", Str_Tmp);
	//}

	//// Menu FilesHistory
	//
	//for(i = 0; i < MAX_RECENT_ROMS; i++)
	//{
	//	if (strcmp(Recent_Rom[i], ""))
	//	{
	//		char tmp[1024];
	//		switch (Detect_Format(Recent_Rom[i]) >> 1)
	//		{
	//			default:
	//				strcpy(tmp, "[---]    - "); // does not exist anymore
	//				break;
	//			case 1:
	//				strcpy(tmp, "[MD]   - ");
	//				break;
	//		}
	//		Get_Name_From_Path(Recent_Rom[i], Str_Tmp);
	//		strcat(tmp, Str_Tmp);
	//		// & is an escape sequence in windows menu names, so replace & with &&
	//		int len = strlen(tmp);
	//		for(int j = 0; j < len && len < 1023; j++)
	//			if(tmp[j] == '&')
	//				memmove(tmp+j+1, tmp+j, strlen(tmp+j)+1), ++len, ++j;
	//		MENU_L(FilesHistory, i, Flags, ID_FILES_OPENRECENTROM0 + i, tmp, "", tmp);
	//	}
	//	else break;
	//}

	
	// Graphics Menu

	Flags = MF_BYPOSITION | MF_STRING;
	
	i = 0;

	MENU_L(Graphics, i++, Flags | (!forceWindowed ? MF_CHECKED : MF_UNCHECKED) | (started ? MF_GRAYED : 0), ID_GRAPHICS_ALLOWFULLSCREEN, "", "Allow &Fullscreen / Display Mode Changes", "can't change while running");
	InsertMenu(Graphics, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Graphics, i++, Flags | (!forceSoftware ? MF_CHECKED : MF_UNCHECKED) | (started ? MF_GRAYED : 0), ID_GRAPHICS_FORCESOFTWARE, "", "&Allow Hardware Acceleration", "can't change while running");
	MENU_L(Graphics, i++, Flags | MF_POPUP | ((forceSoftware||started) ? MF_GRAYED : 0), (UINT)GraphicsMemory, "", "Surface &Memory", started ? "can't change while running" : "hardware acceleration must be enabled");

	// GraphicsMemory Menu
	i = 0;
	MENU_L(GraphicsMemory, i++, Flags | ((forceSurfaceMemory == 0) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_DONTCARE, "", "&Let Game Choose", 0);
	MENU_L(GraphicsMemory, i++, Flags | ((forceSurfaceMemory == 1) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_SYSTEM, "", "&System Memory               (Slow Draw, Fast Read)", 0);
	MENU_L(GraphicsMemory, i++, Flags | ((forceSurfaceMemory == 2) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_NONLOCAL, "", "&Non-Local Video Memory (Varies)", 0);
	MENU_L(GraphicsMemory, i++, Flags | ((forceSurfaceMemory == 3) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_LOCAL, "", "Local &Video Memory        (Fast Draw, Slow Read)", 0);


	// Execution Menu
	i = 0;
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)Locale, "", "App &Locale", 0);
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)ExecMultithreading, "", "&Multithreading Mode", 0);
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)ExecTimers, "", "Multimedia &Timer Mode", 0);
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)ExecMessageSync, "", "Message &Sync Mode", 0);	
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)ExecWaitSync, "", "&Wait Sync Mode", 0);	
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)ExecDlls, "", "&DLL Loading", 0);
	MENU_L(Exec, i++, Flags | ((truePause)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_USETRUEPAUSE, "", "Disable Pause Helper", 0);
	MENU_L(Exec, i++, Flags | ((onlyHookChildProcesses)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_EXEC_ONLYHOOKCHILDPROC, "", "Wait until sub-process creation" /*" (might help IWBTG)"*/, "can't change while running");
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)Performance, "", "&Performance", 0);
	MENU_L(Exec, i++, Flags | MF_POPUP, (UINT)DebugLogging, "", "&Debug Logging", 0);
//	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
//	MENU_L(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_SUSPEND, "", "Suspend Secondary Threads", "must be running");
//	MENU_L(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_RESUME, "", "Resume Secondary Threads", "must be running");
//	MENU_L(Exec, i++, Flags | ((truePause)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_USETRUEPAUSE, "", "Don't handle any messages while paused", 0);
	


	// Time Menu
	i = 0;
	MENU_L(Time, i++, Flags | (paused?MF_CHECKED:MF_UNCHECKED), ID_TIME_TOGGLE_PAUSE, "", "Pause", 0);
	InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Time, i++, Flags | MF_POPUP, (UINT)TimeRate, "", "&Slow Motion", 0);
	InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Time, i++, Flags | (fastforward?MF_CHECKED:MF_UNCHECKED), ID_TIME_TOGGLE_FASTFORWARD, "", "Fast-Forward", 0);
	MENU_L(Time, i++, Flags | MF_POPUP, (UINT)TimeFastForward, "", "&Fast-Forward Options", 0);
	if(Time == TAS_Tools) InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);


	// Input Menu
	i = 0;
	MENU_L(Input, i++, Flags, ID_INPUT_HOTKEYS, "", "Configure &Hotkeys...", 0);	
	MENU_L(Input, i++, Flags | MF_POPUP, (UINT)InputHotkeyFocus, "", "Enable Hotkeys When", 0);
	InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Input, i++, Flags, ID_INPUT_INPUTS, "", "Configure Game &Input...", 0);	
	MENU_L(Input, i++, Flags | MF_POPUP, (UINT)InputInputFocus, "", "Enable Game Input When", 0);
	InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Input, i++, Flags | (advancePastNonVideoFrames?MF_CHECKED:MF_UNCHECKED), ID_INPUT_SKIPLAGFRAMES, "", "Frame Advance Skips \"&Lag\" Frames", 0);

	i = 0;
	MENU_L(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_TASER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_TASER, "", "Hourglass has Focus", 0);
	MENU_L(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_TASEE)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_TASEE, "", "Game Window has Focus", 0);
	MENU_L(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_OTHER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_OTHER, "", "Other Programs have Focus", 0);

	i = 0;
	MENU_L(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_TASER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_TASER, "", "Hourglass has Focus", 0);
	MENU_L(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_TASEE)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_TASEE, "", "Game Window has Focus", 0);
	MENU_L(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_OTHER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_OTHER, "", "Other Programs have Focus", 0);


	// Fast-Forward Submenu
	i = 0;
	MENU_L(TimeFastForward, i++, Flags | ((fastForwardFlags&FFMODE_FRONTSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_FRONTSKIP, "", "Frontbuffer Frameskip", 0);
	MENU_L(TimeFastForward, i++, Flags | ((fastForwardFlags&FFMODE_BACKSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_BACKSKIP, "", "Backbuffer Frameskip", 0);
	MENU_L(TimeFastForward, i++, Flags | ((recoveringStale||(fastForwardFlags&FFMODE_SOUNDSKIP))?MF_CHECKED:MF_UNCHECKED) | (recoveringStale ? MF_GRAYED : 0), ID_TIME_FF_SOUNDSKIP, "", "Soundskip", "always on while recovering stale");
	MENU_L(TimeFastForward, i++, Flags | ((fastForwardFlags&FFMODE_RAMSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_RAMSKIP, "", "RAM Search/Watch Skip", 0);
	MENU_L(TimeFastForward, i++, Flags | ((fastForwardFlags&FFMODE_SLEEPSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_SLEEPSKIP, "", "Sleep Skip", 0);
	MENU_L(TimeFastForward, i++, Flags | ((fastForwardFlags&FFMODE_WAITSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_WAITSKIP, "", "Wait Skip", 0);

	// Time Rate Submenu
	i = 0;
	MENU_L(TimeRate, i++, Flags | ((timescale==timescaleDivisor)?MF_GRAYED:0), ID_TIME_RATE_FASTER, "", "Speed &Up", "already at 100%");
	MENU_L(TimeRate, i++, Flags | ((timescale*8==timescaleDivisor)?MF_GRAYED:0), ID_TIME_RATE_SLOWER, "", "Slow &Down", "already at slowest option");
	InsertMenu(TimeRate, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(TimeRate, i++, Flags | ((timescale==timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_100, "", "&100% (normal speed)", 0);
	MENU_L(TimeRate, i++, Flags | ((timescale*4==timescaleDivisor*3)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_75, "", "75%", 0);
	MENU_L(TimeRate, i++, Flags | ((timescale*2==timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_50, "", "50%", 0);
	MENU_L(TimeRate, i++, Flags | ((timescale*4==timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_25, "", "25%", 0);
	MENU_L(TimeRate, i++, Flags | ((timescale*8==timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_12, "", "12%", 0);

	// Multithreading Submenu
	i = 0;
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_THREADS_DISABLE, "", "&Disable (prevent thread creation)", 0);
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 1)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode>=2?MF_GRAYED:0), ID_EXEC_THREADS_WRAP, "", "&Wrap (recycle threads)", "can't set while running after normal threads created");
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 2)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_ALLOW, "", "&Allow (normal thread creation)", "can't set while running after wrapped threads created");
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 3)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_KNOWN, "", "Allow &known threads only", "can't set while running after wrapped threads created");
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 4)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_UNKNOWN, "", "Allow &unknown threads only", "can't set while running after wrapped threads created");
	MENU_L(ExecMultithreading, i++, Flags | ((threadMode == 5)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_TRUSTED, "", "Allow &trusted threads only", "can't set while running after wrapped threads created");
	

	// Timers Submenu
	i = 0;
	MENU_L(ExecTimers, i++, Flags | ((timersMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_DISABLE, "", "&Disable (prevent timer creation)", 0);
	MENU_L(ExecTimers, i++, Flags | ((timersMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_SYNC, "", "&Synchronous (run timers at frame boundaries)", 0);
	MENU_L(ExecTimers, i++, Flags | ((timersMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_ASYNC, "", "&Asynchronous (run timers in separate threads)", 0);

	// Message Sync Submenu
	i = 0;
	MENU_L(ExecMessageSync, i++, Flags | ((messageSyncMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_SYNC, "", "&Synchronous (no timeouts)", 0);
	MENU_L(ExecMessageSync, i++, Flags | ((messageSyncMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_SEMISYNC, "", "Semi-S&ynchronous (some timeouts)", 0);
	MENU_L(ExecMessageSync, i++, Flags | ((messageSyncMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_ASYNC, "", "&Asynchronous (any timeouts)", 0);
	MENU_L(ExecMessageSync, i++, Flags | ((messageSyncMode == 3)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_DESYNC, "", "&Unchecked (native messaging)", 0);

	// Wait Sync Submenu
	i = 0;
	MENU_L(ExecWaitSync, i++, Flags | ((waitSyncMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_SYNCWAIT, "", "Synchronous &Wait (infinite timeouts)", 0);
	MENU_L(ExecWaitSync, i++, Flags | ((waitSyncMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_SYNCSKIP, "", "Synchronous &Skip (limited timeouts)", 0);
	MENU_L(ExecWaitSync, i++, Flags | ((waitSyncMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_ASYNC, "", "&Asynchronous (unchecked waits)", 0);

	// Dll Loading Submenu	
	i = 0;
	MENU_L(ExecDlls, i++, Flags | ((allowLoadInstalledDlls)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_INSTALLED, "", "Allow loading any custom/installed DLLs (e.g. Fraps) (can affect sync)", 0);
	MENU_L(ExecDlls, i++, Flags | ((allowLoadUxtheme)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_UXTHEME, "", "Allow loading uxtheme.dll (for non-classic window styles on XP)", 0);
	InsertMenu(ExecDlls, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(ExecDlls, i++, Flags | ((runDllLast)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_RUNLAST, "", "Allow other DLLs to run first (this is a hack currently required for RotateGear)", 0);

	// Locale Submenu	
	i = 0;
	int curAppLocale = appLocale ? appLocale : tempAppLocale;
	MENU_L(Locale, i++, Flags | ((curAppLocale==0)?MF_CHECKED:MF_UNCHECKED) | ((tempAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_SYSTEM, "", "Use system locale", "movie has forced non-system locale");
	MENU_L(Locale, i++, Flags | ((curAppLocale==1041)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_JAPANESE, "", "Force &Japanese locale", "can't enable while running");	
	MENU_L(Locale, i++, Flags | ((curAppLocale==2052)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_CHINESE, "", "Force &Chinese (Simplified) locale", "can't enable while running");	
	MENU_L(Locale, i++, Flags | ((curAppLocale==1042)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_KOREAN, "", "Force &Korean locale", "can't enable while running");	
	MENU_L(Locale, i++, Flags | ((curAppLocale==1033)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_ENGLISH, "", "Force &English locale", "can't enable while running");

	// Performance Submenu
	i = 0;
	MENU_L(Performance, i++, Flags | ((traceEnabled)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_DEBUGLOG_TOGGLETRACEENABLE, "", "Load All Symbols and dbghelp.dll", "can't change while running");
	MENU_L(Performance, i++, Flags | ((crcVerifyEnabled)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_DEBUGLOG_TOGGLECRCVERIFY, "", "Check CRCs on movie playback", "can't change while running");
	InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Performance, i++, Flags | ((storeVideoMemoryInSavestates)?MF_CHECKED:MF_UNCHECKED), ID_PERFORMANCE_TOGGLESAVEVIDMEM, "", "Store Video Memory in Savestates", 0);
	MENU_L(Performance, i++, Flags | ((storeGuardedPagesInSavestates)?MF_CHECKED:MF_UNCHECKED), ID_PERFORMANCE_TOGGLESAVEGUARDED, "", "Store Guarded Memory Pages in Savestates", 0);
	InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Performance, i++, Flags | (!started?MF_GRAYED:0), ID_PERFORMANCE_DEALLOCSTATES, "", "Discard All Savestates Now", "must be running");
	MENU_L(Performance, i++, Flags, ID_PERFORMANCE_DELETESTATES, "", "Delete All Savestates Now", 0);


	// Debug Log Submenu
	i = 0;
	MENU_L(DebugLogging, i++, Flags | ((debugPrintMode==0)?MF_CHECKED:MF_UNCHECKED), ID_DEBUGLOG_DISABLED, "", "Disabled", 0);
	if(debugPrintMode==1 || IsDebuggerPresent())
		MENU_L(DebugLogging, i++, Flags | ((debugPrintMode==1)?MF_CHECKED:MF_UNCHECKED), ID_DEBUGLOG_DEBUGGER, "", "Send to Debugger", 0);
	MENU_L(DebugLogging, i++, Flags | ((debugPrintMode==2)?MF_CHECKED:MF_UNCHECKED), ID_DEBUGLOG_LOGFILE, "", "Write to Log File", 0);
	InsertMenu(DebugLogging, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(DebugLogging, i++, Flags | MF_POPUP, (UINT)DebugLoggingInclude, "", "&Print Categories", 0);
	MENU_L(DebugLogging, i++, Flags | MF_POPUP, (UINT)DebugLoggingTrace, "", "&Trace Categories", 0);
	MENU_L(DebugLogging, i++, Flags | MF_POPUP, (UINT)DebugLoggingExclude, "", "&Exclude Categories", 0);

	i = 0;
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_ERROR, "", "error", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_UNTESTED, "", "untested", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TODO, "", "todo", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DESYNC, "", "desync", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_FRAME, "", "frame", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_HOOK, "", "hook", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMEFUNC, "", "timefunc", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMESET, "", "timeset", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMEGET, "", "timeget", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SYNCOBJ, "", "syncobj", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WAIT, "", "wait", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SLEEP, "", "sleep", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DDRAW, "", "directdraw", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_D3D, "", "direct3d", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_OGL, "", "opengl", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_GDI, "", "gdi", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SDL, "", "sdl", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DINPUT, "", "directinput", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_KEYBOARD, "", "keyboard", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MOUSE, "", "mouse", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_JOYPAD, "", "joypad", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DSOUND, "", "directsound", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WSOUND, "", "othersound", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_PROCESS, "", "process", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MODULE, "", "module", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MESSAGES, "", "messages", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WINDOW, "", "window", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_FILEIO, "", "file", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_REGISTRY, "", "registry", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_THREAD, "", "thread", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMERS, "", "timers", 0);
	InsertMenu(DebugLoggingInclude, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_ALL, "", "all", 0);
	MENU_L(DebugLoggingInclude, i++, Flags | ((includeLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_NONE, "", "none (uncategorized only)", 0);

	i = 0;
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_ERROR, "", "error", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_UNTESTED, "", "untested", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TODO, "", "todo", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DESYNC, "", "desync", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_FRAME, "", "frame", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_HOOK, "", "hook", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMEFUNC, "", "timefunc", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMESET, "", "timeset", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMEGET, "", "timeget", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SYNCOBJ, "", "syncobj", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WAIT, "", "wait", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SLEEP, "", "sleep", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DDRAW, "", "directdraw", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_D3D, "", "direct3d", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_OGL, "", "opengl", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_GDI, "", "gdi", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SDL, "", "sdl", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DINPUT, "", "directinput", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_KEYBOARD, "", "keyboard", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MOUSE, "", "mouse", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_JOYPAD, "", "joypad", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DSOUND, "", "directsound", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WSOUND, "", "othersound", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_PROCESS, "", "process", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MODULE, "", "module", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MESSAGES, "", "messages", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WINDOW, "", "window", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_FILEIO, "", "file", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_REGISTRY, "", "registry", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_THREAD, "", "thread", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMERS, "", "timers", 0);
	InsertMenu(DebugLoggingTrace, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_ALL, "", "all (categorized only)", 0);
	MENU_L(DebugLoggingTrace, i++, Flags | ((traceLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_NONE, "", "none", 0);

	i = 0;
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FREQUENT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FREQUENT, "", "frequent", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_ERROR, "", "error", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_UNTESTED, "", "untested", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TODO, "", "todo", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DESYNC, "", "desync", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FRAME, "", "frame", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_HOOK, "", "hook", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMEFUNC, "", "timefunc", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMESET, "", "timeset", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMEGET, "", "timeget", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SYNCOBJ, "", "syncobj", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WAIT, "", "wait", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SLEEP, "", "sleep", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DDRAW, "", "directdraw", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_D3D, "", "direct3d", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_OGL, "", "opengl", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_GDI, "", "gdi", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SDL, "", "sdl", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DINPUT, "", "directinput", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_KEYBOARD, "", "keyboard", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MOUSE, "", "mouse", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_JOYPAD, "", "joypad", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DSOUND, "", "directsound", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WSOUND, "", "othersound", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_PROCESS, "", "process", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MODULE, "", "module", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MESSAGES, "", "messages", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WINDOW, "", "window", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FILEIO, "", "file", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_REGISTRY, "", "registry", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_THREAD, "", "thread", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMERS, "", "timers", 0);
	InsertMenu(DebugLoggingExclude, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_ALL, "", "all (categorized only)", 0);
	MENU_L(DebugLoggingExclude, i++, Flags | ((excludeLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_NONE, "", "none", 0);


	// Sound Menu

	i = 0;

	MENU_L(Sound, i++, Flags | ((emuMode & EMUMODE_EMULATESOUND) ? MF_CHECKED : MF_UNCHECKED) | (started?MF_GRAYED:0), ID_SOUND_SOFTWAREMIX, "", "&Use Software Mixing", "can't change while running");
	MENU_L(Sound, i++, Flags | MF_POPUP | (started||!(emuMode&EMUMODE_EMULATESOUND) ? MF_GRAYED : 0), (UINT)SoundFormat, "", "&Format", started ? "can't change while running" : "software mixing must be enabled");
	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Sound, i++, Flags | ((emuMode & EMUMODE_NOPLAYBUFFERS) ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_NOPLAYBUFFERS, "", (emuMode&EMUMODE_EMULATESOUND) ? ((aviMode&2) ? "&Mute Sound" : "&Mute Sound (Skip Mixing)") : "&Mute Sound (Skip Playing)", 0);
	MENU_L(Sound, i++, Flags | ((emuMode & EMUMODE_VIRTUALDIRECTSOUND) ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_VIRTUALDIRECTSOUND, "", /*(emuMode&EMUMODE_EMULATESOUND) ? "&Virtual DirectSound" :*/ "&Disable DirectSound Creation", 0);



	// Sound Format Menu
	i = 0;
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 8000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_8000, "8000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 11025 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_11025, "&11025 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 12000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_12000, "12000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 16000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_16000, "16000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 22050 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_22050, "&22050 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 24000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_24000, "24000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 32000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_32000, "32000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 44100 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_44100, "&44100 Hz (default)");
	InsertMenu(SoundFormat, i++, Flags | (audioFrequency == 48000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_48000, "48000 Hz");
	InsertMenu(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
	InsertMenu(SoundFormat, i++, Flags | (audioBitsPerSecond == 8 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_BITS_8, "8 bit");
	InsertMenu(SoundFormat, i++, Flags | (audioBitsPerSecond == 16 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_BITS_16, "16 bit (default)");
	InsertMenu(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
	InsertMenu(SoundFormat, i++, Flags | (audioChannels == 1 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_CHANNELS_1, "&Mono");
	InsertMenu(SoundFormat, i++, Flags | (audioChannels == 2 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_CHANNELS_2, "&Stereo (default)");

	// TAS Tools Menu
	MENU_L(TAS_Tools,i++,Flags,ID_RAM_WATCH,"","RAM &Watch", 0);

	bool ramSearchAvailable = true;
	const char* ramSearchString = "&RAM Search (not done)";
#if defined(_MSC_VER) && (_MSC_VER <= 1310)
	ramSearchAvailable = false;
#endif
	MENU_L(TAS_Tools,i++,Flags|(ramSearchAvailable ? MF_ENABLED : MF_DISABLED | MF_GRAYED),ID_RAM_SEARCH,"",ramSearchString,"compiler too old");

	//i = 0;
	//MENU_L(Lua_Script,i++,Flags,IDC_NEW_LUA_SCRIPT,"New Lua Script Window...","","&New Lua Script Window...");
	//MENU_L(Lua_Script,i++,Flags | (!LuaScriptHWnds.empty() ? MF_ENABLED : MF_DISABLED|MF_GRAYED),IDC_CLOSE_LUA_SCRIPTS,"Close All Lua Windows","","&Close All Lua Windows");
	//if(!LuaScriptHWnds.empty())
	//{
	//	InsertMenu(Lua_Script, i++, MF_SEPARATOR, NULL, NULL);
	//	for(unsigned int j=0; j<LuaScriptHWnds.size(); j++)
	//	{
	//		GetWindowText(LuaScriptHWnds[j], Str_Tmp, 1024);
	//		MENU_L(Lua_Script,i++,Flags,IDC_LUA_SCRIPT_0+j,Str_Tmp,"",Str_Tmp);
	//	}
	//}

	//{
	//	int dividerI = i;
	//	for(unsigned int j=0; j<MAX_RECENT_SCRIPTS; j++)
	//	{
	//		const char* pathPtr = Recent_Scripts[j];
	//		if(!*pathPtr)
	//			continue;

	//		HWND IsScriptFileOpen(const char* Path);
	//		if(IsScriptFileOpen(pathPtr))
	//			continue;

	//		// only show some of the path
	//		const char* pathPtrSearch;
	//		int slashesLeft = 2;
	//		for(pathPtrSearch = pathPtr + strlen(pathPtr) - 1; 
	//			pathPtrSearch != pathPtr && slashesLeft >= 0;
	//			pathPtrSearch--)
	//		{
	//			char c = *pathPtrSearch;
	//			if(c == '\\' || c == '/')
	//				slashesLeft--;
	//		}
	//		if(slashesLeft < 0)
	//			pathPtr = pathPtrSearch + 2;
	//		strcpy(Str_Tmp, pathPtr);

	//		if(i == dividerI)
	//			InsertMenu(Lua_Script, i++, MF_SEPARATOR, NULL, NULL);

	//		MENU_L(Lua_Script,i++,Flags,ID_LUA_OPENRECENTSCRIPT0+j,Str_Tmp,"",Str_Tmp);
	//	}
	//}


	// AVI Menu
//	InsertMenu(Avi, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Avi,i++,Flags | ((aviMode&(1|2))==(1|2)?MF_CHECKED:MF_UNCHECKED) | ((started&&!(emuMode&EMUMODE_EMULATESOUND))?MF_GRAYED:0),ID_AVI_BOTH,"","Capture Video and Audio", "software sound mixing must be enabled");
	MENU_L(Avi,i++,Flags | ((aviMode&(1|2))==(1  )?MF_CHECKED:MF_UNCHECKED),ID_AVI_VIDEO,"","Capture Video Only", 0);
	MENU_L(Avi,i++,Flags | ((aviMode&(1|2))==(  2)?MF_CHECKED:MF_UNCHECKED) | ((started&&!(emuMode&EMUMODE_EMULATESOUND))?MF_GRAYED:0),ID_AVI_AUDIO,"","Capture Audio Only", "software sound mixing must be enabled");
	MENU_L(Avi,i++,Flags | ((aviMode&(1|2))==( 0 )?MF_CHECKED:MF_UNCHECKED),ID_AVI_NONE,"",aviMode ? "Stop / Disable" : "AVI Capture Disabled", 0);
	InsertMenu(Avi, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Avi,i++,Flags | ((!aviMode||!started||(!aviFrameCount&&!aviSoundFrameCount))?MF_GRAYED:0),ID_AVI_SPLITNOW,"","Split AVI now", "must be capturing");
	//MENU_L(Avi,i++,Flags,ID_AVI_SETSPLIT,"","Split AVI after...", 0);


	//// Help Menu

	//i = 0;

	//MENU_L(Help, i++, Flags, ID_HELP_ABOUT, "About" ,"", "&About");
	//InsertMenu(Help, i++, MF_SEPARATOR, NULL, NULL);


	HWND tlhwnd = GetDlgItem(hWnd, IDC_TOPLEFTCONTROL);
	RECT oldrect={}, newrect={};
	GetWindowRect(tlhwnd, &oldrect);



	SetMenu(hWnd, MainMenu);


	// if setting the menu pushed the window contents down,
	// extend the bottom of the window by the same amount.
	// (less painful than trying to directly measure the menubar height or width.)
	GetWindowRect(tlhwnd, &newrect);
	int change = newrect.top - oldrect.top;
	if(change)
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);
		SetWindowPos(hWnd, NULL, 0, 0, rect.right-rect.left, change+rect.bottom-rect.top, SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	}
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
