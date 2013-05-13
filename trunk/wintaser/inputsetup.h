/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef INPUTSETUP_H_INCL
#define INPUTSETUP_H_INCL

void Build_Main_Menu(HMENU& menu, HWND hWnd);
int Init_Input(HINSTANCE hInst, HWND hWnd, bool isConfigInput=false);
void End_Input(bool isConfigInput=false);
void Update_Input(HWND hWnd, bool frameSynced, bool allowExecute, bool isConfigInput, bool isHotkeys);
bool IsHotkeyPress(bool isConfigInput=false);
LRESULT CALLBACK HotkeysProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK InputsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
const char* GetVirtualKeyName(int key);

int Save_Config(const char* filename=0);
int Save_As_Config(HWND hWnd);
int Load_Config(const char* filename=0);
int Load_As_Config(HWND hWnd);

extern bool FastForwardKeyDown;
extern bool FrameAdvanceKeyDown1;
extern bool FrameAdvanceKeyDown2;
extern bool AutoFireKeyDown;
extern bool AutoHoldKeyDown;
extern bool AutoClearKeyDown;

#endif
