/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

//#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
//#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define DIRECTINPUT_VERSION 0x0800
#include "../shared/input.h"
#include "global.h"
#include "../shared/ipc.h"

void FrameBoundary(void* captureInfo=0, int captureInfoType=CAPTUREINFO_TYPE_NONE);
void MakeWindowWindowed(HWND hwnd, DWORD width, DWORD height);
bool IsWindowFakeFullscreen(HWND hwnd);
bool RedrawScreen();
BOOL tls_IsPrimaryThread();
BOOL tls_IsPrimaryThread2(struct ThreadLocalStuff* pCurTls);

bool VerifyIsTrustedCaller(bool trusted);

bool ShouldSkipDrawing(bool destIsFrontBuffer, bool destIsBackBuffer);

extern HWND gamehwnd;
extern BOOL fakeDisplayValid;
extern int fakeDisplayWidth;
extern int fakeDisplayHeight;
extern int fakePixelFormatBPP;
extern int fakeDisplayRefresh;
extern bool usingSDLOrDD;
extern PALETTEENTRY activePalette [256];
//extern bool inPauseHandler;
extern bool redrawingScreen;
extern bool pauseHandlerSuspendedSound;
extern int framecount;
extern DWORD threadCounter;
extern unsigned char asynckeybit [256];
extern unsigned char synckeybit [256];
extern int framecountModSkipFreq;

extern CurrentInput previnput;
extern CurrentInput curinput;


#include "dettime.h"
