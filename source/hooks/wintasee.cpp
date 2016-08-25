/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

// main DLL cpp

#define DEFINE_TRAMPS // main file definition, must define at top of exactly one c or cpp file that includes alltramps.h

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <Ntsecapi.h>
#include <mmsystem.h>
#include <strmif.h>

#include <stdio.h>


#include <vector>
#include <algorithm>

//#include "svnrev.h" // defines SRCVERSION number
#include "shared/version.h"


//#define whitelistMaskFilter(x) ((tasflags.messageSyncMode != 3) ? (x) : 0)


//#define DIRECT3D_VERSION 0x0800
//#include <d3d8.h>


//#include "wintasee.h"

//#pragma check_stack (off) // not sure if it's needed... doesn't seem to help, but I'll leave it here anyway

#include "wintasee.h"
#include "print.h"

#include "shared/msg.h"

#include "intercept.h"
#include "tls.h"
#include "msgqueue.h"
#include "locale.h"

#include <map>


// FIXME: should get things working with EMULATE_MESSAGE_QUEUES enabled, currently it breaks Iji arrow key input


//#include <Psapi.h>
//#pragma comment(lib, "psapi.lib")









// if the debugger absolutely needs to send us a message
// it can put it here and we'll take a look
// at the end of every frame boundary
char commandSlot[256*8] = {0};








/*static*/ HWND gamehwnd = 0;




DEFINE_LOCAL_GUID(IID_IUnknown,0x00000000,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46); // needed to avoid linker error
DEFINE_LOCAL_GUID(IID_IClassFactory,0x00000001,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

//DEFINE_LOCAL_GUID(GUID_SysKeyboard,0x6F1D2B61,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
//DEFINE_LOCAL_GUID(CLSID_DirectDraw7,0x3c305196,0x50db,0x11d3,0x9c,0xfe,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );





//static bool disableSelfTicking = false; // maybe setting to true fixes some desyncs?
//static bool disableSelfTicking = true; // FIXME this must be set to false!!! (for herocore and other games to work) but I'm testing something temporarily


bool usingSDLOrDD = false;

bool redrawingScreen = false;


bool RedrawScreen()
{
    VERBOSE_LOG() << "called";
	redrawingScreen = true;
	if(Hooks::RedrawScreenD3D8())
	{}
	else if(Hooks::RedrawScreenD3D9())
	{}
	else if(Hooks::RedrawScreenDDraw()) // moved to after d3d8 to fix redraw in tumiki fighters
	{}
	else if(Hooks::RedrawScreenGDI())
	{}
	//// NYI
	//else if(gamehwnd)
	//{
	//	InvalidateRect(gamehwnd, NULL, TRUE);
	//	UpdateWindow(gamehwnd);
	//}
	else
	{
		redrawingScreen = false;
		return false;
	}
	redrawingScreen = false;
	return true;
}







// function hook trampolines follow.
// or not. (moved to files included by alltramps.h)

// example:
//   #define StretchBlt TrampStretchBlt
//   TRAMPFUNC BOOL WINAPI StretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop) TRAMPOLINE_DEF
// the reason I define them like this is that I might want to call the originals myself
// and this makes it easy to do so (because code can call it using its original name)
// this is partly a safety thing as well (when I add a hook, my own code is less likely to be affected)


// was checking something. rand shouldn't need to be hooked.
//#define srand Trampsrand
//TRAMPFUNC void __cdecl srand(unsigned int _Seed) TRAMPOLINE_DEF_VOID
//#define rand Tramprand
//TRAMPFUNC int __cdecl rand(void) TRAMPOLINE_DEF





static InfoForDebugger infoForDebugger = {};

static TrustedRangeInfos trustedRangeInfos = {};

TasFlags tasflags = {};

PALETTEENTRY activePalette [256];

//HACK for externs
//LogCategoryFlag& g_includeLogFlags = tasflags.includeLogFlags;
//LogCategoryFlag& g_excludeLogFlags = tasflags.excludeLogFlags;


CurrentInput previnput = {0};
CurrentInput curinput = {0};

unsigned char asynckeybit [256] = {0}; // since-last-call bit for GetAsyncKeystate
unsigned char synckeybit [256] = {0}; // toggle status for GetKeyState







int framecount = 0;
static DWORD s_frameThreadId = 0;

static bool dllInitializationDone = false;

bool tlsIsSafeToUse = false;
DWORD ThreadLocalStuff::tlsIndex = 0;

BOOL tls_IsPrimaryThread()
{
	ThreadLocalStuff& t = tls;
	return s_frameThreadId ? t.isFrameThread : (gamehwnd ? t.createdFirstWindow : t.isFirstThread);
}

BOOL tls_IsPrimaryThread2(ThreadLocalStuff* pCurTls)
{
	ThreadLocalStuff& t = *pCurTls;
	return s_frameThreadId ? t.isFrameThread : (gamehwnd ? t.createdFirstWindow : t.isFirstThread);
}

bool IsInRange(DWORD address, const TrustedRangeInfo& range)
{
	return (DWORD)((DWORD)address - range.start) < (DWORD)(range.end - range.start);
}

bool IsInCurrentDllAddressSpace(DWORD address)
{
	return IsInRange(address, trustedRangeInfos.infos[0]);
}
bool IsInNonCurrentYetTrustedAddressSpace(DWORD address)
{
	int count = trustedRangeInfos.numInfos;
	for(int i = 1; i < std::min(count, trustedRangeInfos.numInfos); i++)
		if(IsInRange(address, trustedRangeInfos.infos[i]))
			return true;
	return (address == 0);
}
bool IsInAnyTrustedAddressSpace(DWORD address)
{
	int count = trustedRangeInfos.numInfos;
	for(int i = 0; i < std::min(count, trustedRangeInfos.numInfos); i++)
		if(IsInRange(address, trustedRangeInfos.infos[i]))
			return true;
	return (address == 0);
}

__declspec(noinline) bool IsNearStackTop(DWORD address)
{
	DWORD top = (DWORD)&top;
	DWORD under = address - top;
	return under < 0x1000;
}
bool IsNear(DWORD address, DWORD address2)
{
	int diff = address - address2;
	return diff < 0x10000 && diff > -0x10000;
}

//void debugsplatmem(DWORD address, const char* name)
//{
//	char str [2048];
//	char* pstr = str;
//	pstr += sprintf(pstr, "%s=0x%08X", name, address);
//	if(IsNearStackTop(address))
//	{
//		for(int i = 0; i <= 128; i += 4)
//		{
//			DWORD value = *((DWORD*)(address + i));
//			if(value != address)
////			if(IsNearStackTop(value) || IsInAnyTrustedAddressSpace(value))
//			{
//				pstr += sprintf(pstr, ", %s[%d]=0x%08X", name, i, value);
//				if(IsNearStackTop(value))
//				{
//					DWORD subvalue = *((DWORD*)(value));
//					if(IsInAnyTrustedAddressSpace(subvalue))
//						pstr += sprintf(pstr, ", *%s[%d]=0x%08X", name, i, subvalue);
//				}
//			}
//			if(!i) continue;
//			value = *((DWORD*)(address - i));
//			if(value != address)
////			if(IsNearStackTop(value) || IsInAnyTrustedAddressSpace(value))
//			{
//				pstr += sprintf(pstr, ", %s[%d]=0x%08X", name, -i, value);
//				if(IsNearStackTop(value))
//				{
//					DWORD subvalue = *((DWORD*)(value));
//					if(IsInAnyTrustedAddressSpace(subvalue))
//						pstr += sprintf(pstr, ", *%s[%d]=0x%08X", name, -i, subvalue);
//				}
//			}
//		}
//	}
//	debugprintf(str);
//}

// VerifyIsTrustedCaller
//
// this is our last defense against unwanted code causing time to advance.
// in some cases we're on the main thread and tls.untrusted is 0,
// but the thing calling us is still some "random" OS-specific function in some DLL,
// so we use this function to detect whether it's really the game's code calling us or not.
//
// for one example where this makes a difference:
// when La-Mulana starts up, if threads are enabled and DirectMusic initializes successfully,
// the directmusic synth init will call Sleep(10) some number of times,
// which affects the timer enough to change the random enemy behavior in that game.
// this is undesirable because:
// - I can't be sure that DLL always does the same Sleep calls across all platforms. (it doesn't come with the game, so everyone could have a different version of it)
// - even if it's deterministic, it still breaks sync between different thread options unnecessarily.
// - I could hook the synth init code and set tls.untrusted, but that solution doesn't scale.
// which is why this catch-all guard against such problems exists.
// as hacky as it is, it probably fixes desync problems in a bunch of games.
bool VerifyIsTrustedCaller(bool trusted)
{
	if(!trusted)
		return false; // if we already know it's untrusted then there's no need to run the test.

//	debugprintf("VerifyIsTrustedCaller wants to do its own stack trace!");
//	cmdprintf("SHORTTRACE: 3,50");


	DWORD myEBP;
	//DWORD myESP;
	//DWORD myEIP;
    __asm
    {
    //Label:
      mov [myEBP], ebp;
      //mov [myESP], esp;
      //mov eax, [Label];
      //mov [myEIP], eax;
    }

	//debugprintf("myEBP=0x%X, myESP=0x%X, myEIP=0x%X\n", myEBP, myESP, myEIP);

//	debugsplatmem(myESP, "esp");
//	debugsplatmem(myEIP, "eip");

	DWORD frame=myEBP;
	DWORD oldFrame=0;
	DWORD addr;//=myEIP;
	//__asm { mov frame, ebp }
	for(int dr=129; --dr;) // can be infinite with seemingly no issues, but I limited the loop just in case
	{
//		debugprintf("VerifyIsTrustedCaller: frame = 0x%08x, &frame=0x%X\n", frame, &frame);

//		debugsplatmem(frame, "ebp");

		if(!IsNearStackTop(frame))
		{
			// there was no stack frame pointer...
			// we should explicitly use the /Oy- compiler option to avoid this
			// (using "Omit Frame Pointers"="No" or #pragma optimize("y",off) isn't enough!)
			// but it sometimes happens regardless (maybe the trampoline code's fault?)
			// so anyway
			// we try to find the next stack frame pointer and continue from there.
			// I don't want to rely on code I'm not in control of to do the stack walk,
			// (because determinism across different computers and OS versions is important,
			// and because I want this function to run very quickly)
			// but this is still probably not the most robust way of doing it...

			frame = oldFrame + 4; // roll back
			for(int i=17; --i;)
			{
				frame += 4;
				//debugprintf("trying frame 0x%X", frame);
				//if(IsNearStackTop(frame))
				//	debugprintf("= 0x%X", *(DWORD*)frame);
				//if(IsNearStackTop(frame) && IsNear(*(DWORD*)frame, addr))
				if(IsNearStackTop(frame) && IsInAnyTrustedAddressSpace(*(DWORD*)frame))
					break;
				//debugprintf("failed");
			}
			frame -= 4;

			if(!IsNearStackTop(frame))
			{
				// TODO: load debughelp StackWalk in this situation?
				// it seems to be better at getting valid addresses without stack frame pointers.
				// then again if it's not going to work the same on everyone's computer, it's out.
				// for now we just give up and assume it's trusted. (assuming the reverse would break much more stuff)
				static int errcount = 1024; // let's spew out a few errors, but not endlessly
				if(errcount)
				{
					errcount--;
					DEBUG_LOG() << "ERROR in VerifyIsTrustedCaller. make sure the DLL was compiled with the /Oy- option! frame=" << frame;
					//cmdprintf("SHORTTRACE: 3,50");
				}
				return true;
			}

//			debugprintf("AFTER CORRECTION:");
//			debugsplatmem(frame, "ebp");
//	cmdprintf("SHORTTRACE: 3,50");
		}

		addr = ((DWORD*)frame)[1]; 

		////debugprintf("ebp=0x%X\n", frame);
		////debugprintf("ebp[0]=0x%X, ebp[4]=0x%X, ebp[8]=0x%X, ebp[12]=0x%X, ebp[-4]=0x%X, ebp[-8]=0x%X, ebp[-12]=0x%X\n", ((DWORD*)frame)[0], ((DWORD*)frame)[1], ((DWORD*)frame)[2], ((DWORD*)frame)[3], ((DWORD*)frame)[-1], ((DWORD*)frame)[-2], ((DWORD*)frame)[-3]);
		////debugprintf("esp[0]=0x%X, esp[4]=0x%X, esp[8]=0x%X, esp[12]=0x%X, esp[-4]=0x%X, esp[-8]=0x%X, esp[-12]=0x%X\n", ((DWORD*)myESP)[0], ((DWORD*)myESP)[1], ((DWORD*)myESP)[2], ((DWORD*)myESP)[3], ((DWORD*)myESP)[-1], ((DWORD*)myESP)[-2], ((DWORD*)myESP)[-3]);
//		debugprintf("VerifyIsTrustedCaller: addr = 0x%08x, &addr=0x%X\n", addr, &addr);

		if(IsInNonCurrentYetTrustedAddressSpace(addr))
		{
			//debugprintf("TRUSTED: 0x%08x\n", addr);
			//cmdprintf("SHORTTRACE: 3,50");
			return true;
		}
		else if(!IsInCurrentDllAddressSpace(addr))
		{
			//debugprintf("UNTRUSTED: 0x%08x\n", addr); 
			//cmdprintf("SHORTTRACE: 3,50");
			return false;
		}

		oldFrame = frame;
		frame = *((DWORD*)frame);

	}

	return true;
}


//void ManualBacktrace()
//{
//	DWORD myEBP;
//	//DWORD myESP;
//	//DWORD myEIP;
//    __asm
//    {
//    //Label:
//      mov [myEBP], ebp;
//      //mov [myESP], esp;
//      //mov eax, [Label];
//      //mov [myEIP], eax;
//    }
//
//	//debugprintf("myEBP=0x%X, myESP=0x%X, myEIP=0x%X\n", myEBP, myESP, myEIP);
//
////	debugsplatmem(myESP, "esp");
////	debugsplatmem(myEIP, "eip");
//
//	DWORD frame=myEBP;
//	DWORD oldFrame=0;
//	DWORD addr;//=myEIP;
//	//__asm { mov frame, ebp }
//	for(int dr=129; --dr;) // can be infinite with seemingly no issues, but I limited the loop just in case
//	{
////		debugprintf("VerifyIsTrustedCaller: frame = 0x%08x, &frame=0x%X\n", frame, &frame);
//
////		debugsplatmem(frame, "ebp");
//
//		if(!IsNearStackTop(frame))
//		{
//			// there was no stack frame pointer...
//			// we should explicitly use the /Oy- compiler option to avoid this
//			// (using "Omit Frame Pointers"="No" or #pragma optimize("y",off) isn't enough!)
//			// but it sometimes happens regardless (maybe the trampoline code's fault?)
//			// so anyway
//			// we try to find the next stack frame pointer and continue from there.
//			// I don't want to rely on code I'm not in control of to do the stack walk,
//			// (because determinism across different computers and OS versions is important,
//			// and because I want this function to run very quickly)
//			// but this is still probably not the most robust way of doing it...
//
//			frame = oldFrame + 4; // roll back
//			for(int i=17; --i;)
//			{
//				frame += 4;
//				//debugprintf("trying frame 0x%X", frame);
//				//if(IsNearStackTop(frame))
//				//	debugprintf("= 0x%X", *(DWORD*)frame);
//				//if(IsNearStackTop(frame) && IsNear(*(DWORD*)frame, addr))
//				if(IsNearStackTop(frame) && IsInAnyTrustedAddressSpace(*(DWORD*)frame))
//					break;
//				//debugprintf("failed");
//			}
//			frame -= 4;
//
//			if(!IsNearStackTop(frame))
//			{
//				return;
//			}
//
////			debugprintf("AFTER CORRECTION:");
////			debugsplatmem(frame, "ebp");
////	cmdprintf("SHORTTRACE: 3,50");
//		}
//
//		addr = ((DWORD*)frame)[1]; 
//
//		////debugprintf("ebp=0x%X\n", frame);
//		////debugprintf("ebp[0]=0x%X, ebp[4]=0x%X, ebp[8]=0x%X, ebp[12]=0x%X, ebp[-4]=0x%X, ebp[-8]=0x%X, ebp[-12]=0x%X\n", ((DWORD*)frame)[0], ((DWORD*)frame)[1], ((DWORD*)frame)[2], ((DWORD*)frame)[3], ((DWORD*)frame)[-1], ((DWORD*)frame)[-2], ((DWORD*)frame)[-3]);
//		////debugprintf("esp[0]=0x%X, esp[4]=0x%X, esp[8]=0x%X, esp[12]=0x%X, esp[-4]=0x%X, esp[-8]=0x%X, esp[-12]=0x%X\n", ((DWORD*)myESP)[0], ((DWORD*)myESP)[1], ((DWORD*)myESP)[2], ((DWORD*)myESP)[3], ((DWORD*)myESP)[-1], ((DWORD*)myESP)[-2], ((DWORD*)myESP)[-3]);
////		debugprintf("VerifyIsTrustedCaller: addr = 0x%08x, &addr=0x%X\n", addr, &addr);
//
//		cmdprintf("DEBUGTRACEADDRESS: %08X", addr);
//
//		oldFrame = frame;
//		frame = *((DWORD*)frame);
//
//	}
//}





int framecountModSkipFreq = 0;
bool ShouldSkipDrawing(bool destIsFrontBuffer, bool destIsBackBuffer)
{
	// frameskip
	return 
	( framecountModSkipFreq &&
	   ( (destIsBackBuffer && (tasflags.fastForwardFlags & FFMODE_BACKSKIP) && !(tasflags.aviMode & 1))
	   ||(destIsFrontBuffer && (tasflags.fastForwardFlags & FFMODE_FRONTSKIP)) 
	   )
	);
}




HWND extHWnd = 0;

char keyboardLayoutName [KL_NAMELENGTH*2];
HKL g_hklOverride = 0;

static DWORD g_videoFramesPrepared = 0;
//static DWORD g_soundMixedTicks = 0;


#include <map>
static std::map<HWND, BOOL> hwndSizeLocked;



void MakeWindowWindowed(HWND hwnd, DWORD width, DWORD height)
{
//	if(hwndSizeLocked.find(hwnd) == hwndSizeLocked.end())
//		return;
	RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
	//SetWindowPos(hwnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, /*SWP_NOMOVE | */SWP_SHOWWINDOW);
	SetWindowPos(hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, /*SWP_NOMOVE | */SWP_SHOWWINDOW);
	hwndSizeLocked[hwnd] = TRUE;
}

bool IsWindowFakeFullscreen(HWND hwnd)
{
	return hwndSizeLocked.find(hwnd) != hwndSizeLocked.end();
}




int getCurrentFramestampLogical();


void UpdateInfoForDebugger()
{
	infoForDebugger.frames = getCurrentFramestampLogical();
	infoForDebugger.ticks = Hooks::getCurrentTimestamp();
	infoForDebugger.addedDelay = Hooks::getCurrentTimestamp2();
	infoForDebugger.lastNewTicks = Hooks::getCurrentTimestamp3();
}






static HANDLE hCurrentProcess = 0;


static int totalRanFrames = 0;
static int totalSleepFrames = 0;
static int measureFrameCount = 0;
static int measureFrameTime = 0;
static int ranFramesThisFPSInterval = 0;
int getCurrentFramestampLogical() { return totalRanFrames - totalSleepFrames; }

static int measureTickCount = 0;

bool inPauseHandler = false;
int pauseHandlerContiguousCallCount = 0;



bool pauseHandlerSuspendedSound = false;



//void SetFastForward(bool set)
//{
//	tasflags.fastForward = set;
//}

void SaveOrLoad(int slot, bool save)
{
    VERBOSE_LOG() << "called.";
	tls.callerisuntrusted++;
	if(save && tasflags.storeVideoMemoryInSavestates)
	{
		Hooks::BackupVideoMemoryOfAllDDrawSurfaces();
		Hooks::BackupVideoMemoryOfAllD3D8Surfaces();
		Hooks::BackupVideoMemoryOfAllD3D9Surfaces();
	}
	Hooks::DirectSound::StopAllSounds();
	if(save)
	{
        IPC::SendIPCMessage(IPC::Command::CMD_SAVE_STATE, &slot, sizeof(&slot));
		save = !tasflags.stateLoaded; // otherwise "save" will be wrong after loading
	}
	else
	{
        IPC::SendIPCMessage(IPC::Command::CMD_LOAD_STATE, &slot, sizeof(&slot));
		// note: any code placed here will never run! execution continues in the above branch.
	}
	if(tasflags.stateLoaded > 0 && tasflags.storeVideoMemoryInSavestates)
	{
		Hooks::RestoreVideoMemoryOfAllDDrawSurfaces();
		Hooks::RestoreVideoMemoryOfAllD3D8Surfaces();
		Hooks::RestoreVideoMemoryOfAllD3D9Surfaces();
		RedrawScreen();
	}
	Hooks::DirectSound::ResumePlayingSounds();
	pauseHandlerContiguousCallCount = 0;
	if(pauseHandlerSuspendedSound)
	{
		Hooks::DirectSound::PostResumeSound();
		pauseHandlerSuspendedSound = false;
	}
	tls.callerisuntrusted--;
}


void GetFrameInput()
{
    VERBOSE_LOG() << "called.";
	Hooks::WinInput::ProcessFrameInput();
}

// pausehelper
void HandlePausedEvents()
{
	//VERBOSE_ENTER(pauseHandlerContiguousCallCount, pauseHandlerSuspendedSound);
	if(inPauseHandler)
		return;
	inPauseHandler = true;
	tls.callerisuntrusted += 2;

	framecountModSkipFreq = 0; // disable frameskip when fast-forward and frame advance are used simultaneously

	if(tasflags.emuMode & EMUMODE_EMULATESOUND)
	{
		pauseHandlerContiguousCallCount++;
		if(!pauseHandlerSuspendedSound)
		{
			if(pauseHandlerContiguousCallCount > 30)
			{
				VERBOSE_LOG() << "suspended sound for pause";
				Hooks::DirectSound::PreSuspendSound();
				pauseHandlerSuspendedSound = true;
			}
			else if(pauseHandlerContiguousCallCount == 1)
			{
				if(!tasflags.fastForward)
					Hooks::DirectSound::ForceAlignSound(false);
			}
		}
	}

	// TODO: is this completely desync-proof? the "disable pause helper" option is there in case it isn't, but...

	// only processing certain messages necessary for interaction with the paused game window,
	// and don't let them reach game code
	struct MsgFilterArgs {
		UINT wMsgFilterMin;
		UINT wMsgFilterMax;
		UINT wRemoveMsg;
	} msgFilterArgs [] = {
		{0,0,PM_QS_PAINT},
		{0,0,((/*QS_SENDMESSAGE |*/ QS_MOUSEMOVE) << 16)},
		{WM_NCCALCSIZE,WM_NCACTIVATE,0},
		{WM_NCMOUSEMOVE,WM_NCMBUTTONDBLCLK,0},
	};

	MSG msg;

	// using gamehwnd isn't reliable enough
	std::map<HWND, WNDPROC>::iterator iter;
	bool sentToAny = false;
	for(iter = Hooks::hwndToOrigHandler.begin(); iter != Hooks::hwndToOrigHandler.end();)
	{
		HWND hwnd = iter->first;
		iter++;
		DWORD style = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
		if(IsWindow(hwnd)
		&& ((hwnd==gamehwnd && (style&WS_VISIBLE))
		 || ((style & (WS_VISIBLE|WS_CAPTION)) == (WS_VISIBLE|WS_CAPTION))
		 || ((style & (WS_VISIBLE|WS_POPUP)) == (WS_VISIBLE|WS_POPUP)))
		 )
		{
			for(int i = 0; i < ARRAYSIZE(msgFilterArgs); i++)
			{
				if(PeekMessageA(&msg, hwnd, msgFilterArgs[i].wMsgFilterMin,msgFilterArgs[i].wMsgFilterMax, PM_REMOVE|PM_NOYIELD | msgFilterArgs[i].wRemoveMsg))
				{
					//DispatchMessageInternal(hwnd, msg.message, msg.wParam, msg.lParam, true, MAF_PASSTHROUGH|MAF_RETURN_OS);
					Hooks::MyWndProcA(hwnd, msg.message, msg.wParam, msg.lParam);
				}
			}
		}
	}

	tls.callerisuntrusted -= 2;
	inPauseHandler = false;
}

void HandleShutdown()
{
	Hooks::DirectSound::PreSuspendSound();
	Hooks::DirectSound::BackDoorStopAll();
	fflush(NULL);
	// ExitProcess doesn't work for me. sets wrong exit code or completely fails to exit.
	TerminateProcess(GetCurrentProcess(), SUCCESSFUL_EXITCODE); // but this does work.
	ExitProcess(SUCCESSFUL_EXITCODE); // but if it doesn't then maybe this will?
	// old version, left here in case TerminateProcess and ExitProcess somehow noop
	//// just send anything so the WaitForDebugEvent call returns
	OutputDebugStringW(L"ok");
	//// it might ask more than once
	while(true) { OutputDebugStringW(L"ok ok"); Sleep(10); TerminateProcess(GetCurrentProcess(), SUCCESSFUL_EXITCODE); }
}








static bool inFrameBoundary = false;

DWORD threadCounter = 0;



//char realTimeLogs[4096];
//char* realTimeLogsPtr = realTimeLogs;
//
//void LogRealTime(const char* prefix)
//{
//	return;
//	static int lastTime = timeGetTime();
//	int time = timeGetTime();
//	//debugprintf("%s: TIME += %d\n", prefix, time - lastTime);
//	realTimeLogsPtr += sprintf(realTimeLogsPtr, "%s: TIME += %d\n", prefix, time - lastTime);
//	if(realTimeLogsPtr > realTimeLogs + sizeof(realTimeLogs) - 256)
//	{
//		debugprintf(realTimeLogs);
//		realTimeLogsPtr = realTimeLogs;
//	}
//	lastTime = timeGetTime();//time;
//}







static int lastSentFPS = 0;
static int s_skipFreq = 8;


void FrameBoundary(void* captureInfo, int captureInfoType)
{
	int localFrameCount = framecount;
    DEBUG_LOG() << "called. Frame " << localFrameCount << " -> " << localFrameCount + 1;
	//cmdprintf("SUSPENDALL: ");

	int framerate = tasflags.framerate;
	//bool wasInFrameBoundary = inFrameBoundary;

	if(!dllInitializationDone)
		return;

//	debugprintf("gamehwnd = 0x%X\n", gamehwnd);

	if(!s_frameThreadId && captureInfoType != CAPTUREINFO_TYPE_PREV)
	{
		char name[64];
		sprintf(name, "%u_" "FrameThread" "_at_%u", threadCounter++, detTimer.GetTicks());
        Hooks::SetThreadName(-1, name);

		s_frameThreadId = GetCurrentThreadId();
		tls.isFrameThread = TRUE;
	}

#ifndef EMULATE_MESSAGE_QUEUES
	if(Hooks::hasPostedMessages)
		Hooks::HandlePostedMessages();
#endif

	Hooks::ProcessTimers();


	// NOTE: the above code might recurse. we're not really in a frame boundary just yet.
	// for this reason, localFrameCount should NOT be used except for things like debugging printouts.
	if(inFrameBoundary)
	{
		DEBUG_LOG() << "warning: attempting to handle recursion in FrameBoundary...";
		detTimer.ExitFrameBoundary();
	}
	detTimer.EnterFrameBoundary(framerate);
	inFrameBoundary = true;


	//if(!wasInFrameBoundary)
	//{
	//	debuglog(LCF_FRAME|LCF_ERROR, __FUNCTION__ " skipping recursion...\n");
	//	_asm{int 3}
	//	return;
	//}
	////inFrameBoundary = true;



	//LogRealTime("beforeEnterFrame");
//	detTimer.EnterFrameBoundary(framerate);
	//LogRealTime("afterEnterFrame");

	framecount++;


	DEBUG_LOG() << "New frame!";
//	cmdprintf("DEBUGPAUSE: %d", getCurrentFramestamp());
	//debuglog(LCF_FREQUENT|LCF_DESYNC|LCF_TODO|-1, "frameLOG: f=%d, t=%d\n", getCurrentFramestamp(), getCurrentTimestamp());
	//cmdprintf("SHORTTRACE: 3,50");

	if(tasflags.fastForward)
	{
		framecountModSkipFreq++;
		BOOL recordingAVIVideo = (tasflags.aviMode & 1);
		int skipFreq = (tasflags.aviMode & 1) ? 5 : s_skipFreq;
		if(framecountModSkipFreq >= skipFreq)
			framecountModSkipFreq = 0;
	}
	else
	{
		framecountModSkipFreq = 0;
	}

	if(++ranFramesThisFPSInterval >= (tasflags.fastForward ? 600 : 300))
	{
		int time = Hooks::timeGetTime();

		ranFramesThisFPSInterval = 0;
		float fps = (float)((totalRanFrames-totalSleepFrames) - measureFrameCount) * 1000.0f / (time - measureFrameTime);

		int totalRanTicks = detTimer.GetTicks();
		float logicalfps = (float)((totalRanFrames-totalSleepFrames) - measureFrameCount) * 1000.0f / (totalRanTicks - measureTickCount); // this shows the logical (timer) frames-per-second instead of actual FPS
		measureTickCount = totalRanTicks;

        IPC::FPSInfo fps_info(fps, logicalfps);
        IPC::SendIPCMessage(IPC::Command::CMD_FPS_UPDATE, &fps_info, sizeof(fps_info));
		measureFrameTime = time;
		measureFrameCount = totalRanFrames-totalSleepFrames;
		lastSentFPS = (int)fps;

		// this looks like a stupid way of calculating it but it's better to have
		// bands of the same skip frequency instead of continous variation
		// and these are emperically determined values that aren't very even at the low end
		if(lastSentFPS > 4000)
			s_skipFreq = 270; // warp speed!
		else if(lastSentFPS > 3000)
			s_skipFreq = 200;
		else if(lastSentFPS > 2000)
			s_skipFreq = 128;
		else if(lastSentFPS > 1400)
			s_skipFreq = 96;
		else if(lastSentFPS > 900)
			s_skipFreq = 64;
		else if(lastSentFPS > 200)
			s_skipFreq = 32;
		else if(lastSentFPS > 100)
			s_skipFreq = 16;
		else
			s_skipFreq = 8;
	}
	totalRanFrames++;
	if(captureInfoType == CAPTUREINFO_TYPE_PREV)
		totalSleepFrames++;


//	static int x = 0;
//	x++;

	//if(x % 100 == 0)
	//{
	//	debugprintf(__FUNCTION__ " called %d times\n", x);
	//}

	BOOL prevWindowActivateFlags = tasflags.windowActivateFlags;

	previnput = curinput;

	g_videoFramesPrepared++;
	//debugprintf(__FUNCTION__ ": g_soundMixedTicks=%d, g_videoFramesPrepared=%d, ratio=%g\n", g_soundMixedTicks, g_videoFramesPrepared, (float)g_soundMixedTicks/g_videoFramesPrepared);
	bool ranCommand;
	do
	{
		UpdateInfoForDebugger();

        IPC::FrameBoundaryInfo frame_boundary_info(totalRanFrames, captureInfo, captureInfoType);
        IPC::SendIPCMessage(IPC::Command::CMD_FRAME_BOUNDARY, &frame_boundary_info, sizeof(frame_boundary_info));

		ranCommand = false;

		captureInfo = 0;
		captureInfoType = CAPTUREINFO_TYPE_NONE_SUBSEQUENT;

		// now check for commands
		if(*commandSlot)
		{
#define MessagePrefixMatch(pre) (!strncmp(pstr, pre": ", sizeof(pre": ")-1) ? pstr += sizeof(pre": ")-1 : false)
			const char* pstr = commandSlot;
			if(MessagePrefixMatch("SAVE"))
				SaveOrLoad(atoi(pstr), true), (ranCommand=true);
			else if(MessagePrefixMatch("LOAD"))
				SaveOrLoad(atoi(pstr), false), (ranCommand=true);
			//else if(MessagePrefixMatch("FAST"))
			//	SetFastForward(atoi(pstr)!=0), (ranCommand=true);
			else if(MessagePrefixMatch("HANDLEEVENTS"))
				HandlePausedEvents(), (ranCommand=true);
			else if(MessagePrefixMatch("PREPARETODIE"))
				HandleShutdown(), (ranCommand=true);
			*commandSlot = 0;
#undef MessagePrefixMatch
		}
	} while(ranCommand);

	pauseHandlerContiguousCallCount = 0;

	Hooks::DirectSound::DoFrameBoundarySoundChecks();


	if(framecount > 1)
	{
//		if((prevWindowActivateFlags&1) != (tasflags.windowActivateFlags&1))
		{
//			hwndDeniedDeactivate.clear();
		}

		if((prevWindowActivateFlags&2) != (tasflags.windowActivateFlags&2))
		{
		// handle toggling "always on top" status when allow deactivate checkbox changes
			std::map<HWND, WNDPROC>::iterator iter;
			for(iter = Hooks::hwndToOrigHandler.begin(); iter != Hooks::hwndToOrigHandler.end(); iter++)
			{
				HWND hwnd = iter->first;
				if(IsWindow(hwnd))
				{
					if(!(tasflags.windowActivateFlags & 2))
						SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
					else
					{
						SetForegroundWindow(hwnd);
						//SetActiveWindow(hwnd);
						//SetFocus(hwnd);
						SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
					}
				}
			}
		}
	}


	GetFrameInput();


	// using gamehwnd isn't reliable enough
    using Log = DebugLog<LogCategory::WINPUT>;
	std::map<HWND, WNDPROC>::iterator iter;
	bool sentToAny = false;
	for(iter = Hooks::hwndToOrigHandler.begin(); iter != Hooks::hwndToOrigHandler.end();)
	{
		HWND hwnd = iter->first;
		iter++;
        DWORD style = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
        if (IsWindow(hwnd)
            && ((hwnd == gamehwnd && (style&WS_VISIBLE))
                || ((style & (WS_VISIBLE | WS_CAPTION)) == (WS_VISIBLE | WS_CAPTION))
                || ((style & (WS_VISIBLE | WS_POPUP)) == (WS_VISIBLE | WS_POPUP))
                || (!sentToAny && iter == Hooks::hwndToOrigHandler.end()))
            )
		{
			sentToAny = true;

			/* Send keyboard messages */
			HKL keyboardLayout = GetKeyboardLayout(GetCurrentThreadId());

			BYTE keyboardState[256];
			Hooks::WinInput::MyGetKeyboardState(keyboardState);

			for(int i = 0; i < 256; i++)
			{
				int cur = curinput.keys[i];
				int prev = previnput.keys[i];
				if(cur && !prev)
				{
					LOG() << "DOWN: " << i;
#ifdef EMULATE_MESSAGE_QUEUES
					PostMessageInternal(hwnd, WM_KEYDOWN, i, 0);
#else
					//SendMessage(hwnd, toggleWhitelistMessage(WM_KEYDOWN), i, 0);
					Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(WM_KEYDOWN), i, 0);
#endif

					// also send a WM_CHAR event in case some games need it (HACK, should do this in TranslateMessage)
					CHAR outbuf [4];
					//int DIK = MapVirtualKeyEx(i, /*MAPVK_VK_TO_VSC*/0, keyboardLayout) & 0xFF;
					int results = ToAsciiEx(i, 0, keyboardState, (WORD*)outbuf, 0, keyboardLayout);
					for(int res = 0; res < results; res++)
					{
						int c = outbuf[res];
						LOG() << "CHAR: " << c;
#ifdef EMULATE_MESSAGE_QUEUES
						PostMessageInternal(hwnd, WM_CHAR, c, 0);
#else
						//SendMessage(hwnd, toggleWhitelistMessage(WM_CHAR), c, 0);
						Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(WM_CHAR), c, 0);
#endif
					}
				}
				else if(!cur && prev)
				{
                    LOG() << "UP: " << i;
#ifdef EMULATE_MESSAGE_QUEUES
					PostMessageInternal(hwnd, WM_KEYUP, i, 0);
#else
					//SendMessage(hwnd, toggleWhitelistMessage(WM_KEYUP), i, 0);
					Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(WM_KEYUP), i, 0);
#endif
				}
			}

			/* Send mouse messages */

			// Build the wParam (contains other buttons status + keyboard modifiers).
			WPARAM flag = 0;
			WPARAM mouseFlags[3] = {MK_LBUTTON, MK_RBUTTON, MK_MBUTTON};
			for (int i = 0; i < 3; i++){
				if (curinput.mouse.di.rgbButtons[i])
					flag |= mouseFlags[i];
			}
			if (curinput.keys[VK_CONTROL])
				flag |= MK_CONTROL;
			if (curinput.keys[VK_SHIFT])
				flag |= MK_SHIFT;

			// Build the lParam (contains cursor coordinates relative to the window).
			LPARAM coords = (curinput.mouse.coords.y << 16) | (curinput.mouse.coords.x); // Not sure it's good when coords are negative...

			// Send the MOUSEMOVE message if needed.
            if ((curinput.mouse.di.lX != 0) || (curinput.mouse.di.lY != 0))
            {
				LOG() << "MOUSE MOVE";
				//debugprintf("My MOUSE message is 0x%X with wParam = %d and lParam = %d\n",WM_MOUSEMOVE, flag, coords);
				//debugprintf("Coords are x=%d and y=%d\n",GET_X_LPARAM(coords), GET_Y_LPARAM(coords));
#ifdef EMULATE_MESSAGE_QUEUES
				PostMessageInternal(hwnd, WM_MOUSEMOVE, flag, coords);
#else
				Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(WM_MOUSEMOVE), flag, coords); // TODO: fill lParam and wParam
#endif
			}

			// Send the BUTTONUP/DOWN messages.
			WORD mouseButtonsUp[3] = {WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP};
			WORD mouseButtonsDown[3] = {WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN};

            for (int i = 0; i < 3; i++)
            {
				int cur = curinput.mouse.di.rgbButtons[i];
				int prev = previnput.mouse.di.rgbButtons[i];
				if(cur && !prev)
				{
					LOG() << "MOUSE BUTTON DOWN: " << i;
#ifdef EMULATE_MESSAGE_QUEUES
					PostMessageInternal(hwnd, mouseButtonsDown[i], flag, coords);
#else
					Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(mouseButtonsDown[i]), flag, coords);
#endif
				}
				else if(!cur && prev)
				{
					LOG() << "MOUSE BUTTON UP: " << i;
#ifdef EMULATE_MESSAGE_QUEUES
					PostMessageInternal(hwnd, mouseButtonsUp[i], flag, coords);
#else
					Hooks::MyWndProcA(hwnd, toggleWhitelistMessage(mouseButtonsUp[i]), flag, coords);
#endif
				}
			}
		}
	}

//	LogRealTime("beforeExitFrame");
	detTimer.ExitFrameBoundary();
	inFrameBoundary = false;
//	LogRealTime("afterExitFrame");
//	debugprintf("\n");
	//cmdprintf("RESUMEALL: ");

	Hooks::g_midFrameAsyncKeyRequests = 0;

	DEBUG_LOG() << "returned. (" << localFrameCount << " -> " << framecount << ")";
}


// NOTE: don't forget to put WINAPI everywhere it belongs or you'll get weird crashes sometimes





































struct MyClassFactory : IClassFactory
{
	//static BOOL Hook(IClassFactory* obj)
	//{
	//	BOOL rv = FALSE;
	//	rv |= VTHOOKFUNC(IClassFactory, CreateInstance);
	//	//rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	//	return rv;
	//}
	//
	//static HRESULT(STDMETHODCALLTYPE *CreateInstance)(IClassFactory* pThis, IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
	//static HRESULT STDMETHODCALLTYPE MyCreateInstance(IClassFactory* pThis, IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
	//{
	//	debuglog(LCF_HOOK|LCF_MODULE, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
	//	HRESULT rv = CreateInstance(pThis, pUnkOuter, riid, ppvObject);
	//	if(SUCCEEDED(rv))
	//		HookCOMInterface(riid, ppvObject, true);
	//	return rv;
	//}


	MyClassFactory(IClassFactory* cf) : m_cf(cf)
	{
//		ENTER();
//		cmdprintf("SHORTTRACE: 3,50");
	}


	// IUnknown methods

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		//ENTER(riid.Data1);
		HRESULT rv = m_cf->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		//ENTER();
		return m_cf->AddRef();
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		//ENTER();
		ULONG count = m_cf->Release();
		if(0 == count)
			delete this;

		return count;
	}

    // IClassFactory methods

	STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
	{
        using Log = DebugLog<LogCategory::MODULE>;
		ENTER(riid.Data1);
		HRESULT rv = m_cf->CreateInstance(pUnkOuter, riid, ppvObject);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObject, true);
		return rv;
	}

	STDMETHOD(LockServer)(BOOL fLock)
	{
		return m_cf->LockServer(fLock);
	}

private:

	IClassFactory* m_cf;
};
//HRESULT (STDMETHODCALLTYPE* MyClassFactory::CreateInstance)(IClassFactory* pThis, IUnknown* pUnkOuter, REFIID riid, void** ppvObject) = 0;
////HRESULT (STDMETHODCALLTYPE* MyClassFactory::QueryInterface)(IClassFactory* pThis, REFIID riid, void** ppvObj) = 0;




void HookCOMInterface(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
    using Log = DebugLog<LogCategory::HOOK>;
	if(!ppvOut)
		return;

	ENTER(riid.Data1);

	switch(riid.Data1)
	{
		//VTHOOKRIID3(IClassFactory,MyClassFactory);
		HOOKRIID3(IClassFactory,MyClassFactory);

		default:
			if(!Hooks::HookCOMInterfaceDDraw(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D7(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D8(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D9(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::DirectSound::HookCOMInterfaceSound(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::DirectInput::HookCOMInterfaceInput(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceTime(riid, ppvOut, uncheckedFastNew))
                LOG() << "unknown riid: " << riid.Data1 << "-" << riid.Data2 << "-" << riid.Data3
                      << "-" << riid.Data4[0] << riid.Data4[1] << riid.Data4[2] << riid.Data4[3]
                      << riid.Data4[4] << riid.Data4[5] << riid.Data4[6] << riid.Data4[7];
			break;
	}
}

void HookCOMInterfaceEx(REFIID riid, LPVOID* ppvOut, REFGUID parameter, bool uncheckedFastNew)
{
    using Log = DebugLog<LogCategory::HOOK>;
	if(!ppvOut)
		return;

	ENTER(riid.Data1);

	switch(riid.Data1)
	{
		//VTHOOKRIID3(IClassFactory,MyClassFactory);
		HOOKRIID3(IClassFactory,MyClassFactory);

		default:
			if(!Hooks::HookCOMInterfaceDDraw(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D7(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D8(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceD3D9(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::DirectSound::HookCOMInterfaceSound(riid, ppvOut, uncheckedFastNew)
			&& !Hooks::DirectInput::HookCOMInterfaceInputEx(riid, ppvOut, parameter, uncheckedFastNew)
			&& !Hooks::HookCOMInterfaceTime(riid, ppvOut, uncheckedFastNew))
                LOG() << "unknown riid: " << riid.Data1 << "-" << riid.Data2 << "-" << riid.Data3
                      << "-" << riid.Data4[0] << riid.Data4[1] << riid.Data4[2] << riid.Data4[3]
                      << riid.Data4[4] << riid.Data4[5] << riid.Data4[6] << riid.Data4[7];
			break;
	}
}


















//HOOKFUNC void __cdecl Mysrand(unsigned int _Seed)
//{
//	debugprintf(__FUNCTION__ "(%d == 0x%X, time=0x%X) called.\n", _Seed, _Seed, detTimer.GetTicks());
//	Trampsrand(_Seed);
//}
//HOOKFUNC int __cdecl Myrand(void)
//{
//	int rv = Tramprand();
//	debugprintf(__FUNCTION__ "(%d == 0x%X, time=0x%X) called.\n", rv, rv, detTimer.GetTicks());
//	return rv;
//}





















//HOOKFUNC BOOL WINAPI MyIsDebuggerPresent(VOID) // already handled by clearDebuggerFlag()
//{
//	// we're not really attempting to be "stealthy", but
//	// maybe this helps prevent some games from showing debug crap
//	return FALSE;
//}
//HOOKFUNC VOID WINAPI MyOutputDebugStringA(LPCSTR lpOutputString)
//{
//	debugprintf("game called " __FUNCTION__ "(\"%s\")", lpOutputString);
//}
//HOOKFUNC VOID WINAPI MyOutputDebugStringW(LPCWSTR lpOutputString)
//{
//	debugprintf("game called" __FUNCTION__ "(\"%S\")", lpOutputString);
//}







//#pragma optimize( "", on )



// look into at some point: for any APIs that I'm completely replacing,
// it might help to use DONT_RESOLVE_DLL_REFERENCES to prevent their DllMain from being called.
// but this is dangerous unless I'm sure I've fully replaced everything in the dll.
// opengl might be a good candidate for this.




//static InterceptDescriptor intercepts [] = 
//{
//	// note: some of these cannot be disabled by commenting them out.
//	// generally the first parameter should be used instead.
//
//	// time
//
//	// timer
//
//
//	//MAKE_INTERCEPT(1, KERNEL32, IsDebuggerPresent),
//	//MAKE_INTERCEPT(1, KERNEL32, OutputDebugStringA),
//	//MAKE_INTERCEPT(1, KERNEL32, OutputDebugStringW),
//	//MAKE_INTERCEPT(1, NTDLL, NtQueryInformationProcess),
//
//
//	// input
//
//	// graphics
//
//
//	// execution
//
//	// modules/init/code
//
//	// synchronization
//	
//
//	// window
//
//
//
//
//	// messages
//
//
//
//
//	// files
//
//	//// testing
//	//MAKE_INTERCEPT(1, MSVCRT, rand),
//	//MAKE_INTERCEPT(1, MSVCRT, srand),
//};

//typedef UINT (WINAPI *SetCPGlobalType)(UINT acp);

// warning: we can't trust these too much (version lies from compatibility mode shims are common)
bool IsWindowsXP()    { return tasflags.osVersionMajor == 5 && tasflags.osVersionMinor == 1; }
bool IsWindowsVista() { return tasflags.osVersionMajor == 6 && tasflags.osVersionMinor == 0; }
bool IsWindows7()     { return tasflags.osVersionMajor == 6 && tasflags.osVersionMinor == 1; }


// order of execution when the game starts up:
// 1: A very small number of essential DLLs get loaded, such as kernel32.dll.
// 2: DllMain runs, because this DLL (wintasee.dll) has been inserted into the IAT.
// 3: Any statically-linked DLLs used by the game get loaded.
// 4: WinMain runs, but pauses at a breakpoint the debugger process inserted at the entrypoint.
// 5: PostDllMain runs (because we created a thread to run it in DllMain).
// 6: WinMain resumes, and the game's code starts executing.


// DllMain is the entry point of this DLL, but the things we can do there are extremely limited.
// For example, DllMain is not allowed to call any function that might cause any DLL to load.
// PostDllMain is the code we run after DllMain to finish up initialization without restrictions.
DWORD WINAPI PostDllMain(LPVOID lpParam)
{
    DEBUG_LOG() << " (lpParam=" << lpParam << ") called.";
	dllInitializationDone = true;

	detTimer.OnSystemTimerRecalibrated();

	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++; // avoid advancing timer here

	// see MyKiUserCallbackDispatcher... to avoid hardcoding OS-specific constants, we take the chance to measure one of them here.
	Hooks::watchForCLLApiNum = true; // a few functions like GetSystemMetrics and LoadKeyboardLayout are very likely to call ClientLoadLibrary
	Hooks::cllApiNum = -1;
	curtls.treatDLLLoadsAsClient++;
	GetSystemMetrics(42);
	curtls.treatDLLLoadsAsClient--; // disable here because we'd prefer LoadKeyboardLayout to actually succeed.  
	curtls.callingClientLoadLibrary = FALSE;

	// moved from DllMain since it was causing a loader lock problem
	//LoadKeyboardLayoutA(keyboardLayoutName, KLF_ACTIVATE | KLF_REORDER | KLF_SETFORPROCESS);
	// activate disabled because it interferes with directinput in other apps (e.g. hourglass hotkeys)
	HKL loadLayoutRv = LoadKeyboardLayoutA(keyboardLayoutName, 0);
	g_hklOverride = loadLayoutRv;
	//if(!loadLayoutRv) // because LoadKeyboardLayout sometimes lies about succeeding and returns the default layout, let's always go into the fallback branch to ensure consistency
	{
		sscanf(keyboardLayoutName, "%08X", reinterpret_cast<int*>(&g_hklOverride));
		if(!((DWORD)g_hklOverride & 0xFFFF0000))
			(DWORD&)g_hklOverride |= ((DWORD)g_hklOverride << 16);
	}
	DEBUG_LOG() << "keyboardLayout = " << keyboardLayoutName << ", hkl = " << loadLayoutRv << " -> " << g_hklOverride;
	curtls.callingClientLoadLibrary = FALSE;

	if(tasflags.appLocale)
	{
		SetThreadLocale(tasflags.appLocale);
		SetThreadUILanguage(tasflags.appLocale);
	}

	if(Hooks::watchForCLLApiNum || Hooks::cllApiNum == -1)
	{
		// didn't find it, somehow
		Hooks::watchForCLLApiNum = false;
		Hooks::cllApiNum = (IsWindows7() ? 65 : 66);
        DEBUG_LOG() << "using ClientLoadLibrary ApiNumber = " << Hooks::cllApiNum << ". OS = " << tasflags.osVersionMajor << "." << tasflags.osVersionMinor;
	}
	else
	{
        DEBUG_LOG() << "found ClientLoadLibrary ApiNumber = " << Hooks::cllApiNum << ". OS = " << tasflags.osVersionMajor << "." << tasflags.osVersionMinor;
	}
	curtls.callingClientLoadLibrary = FALSE;

	curtls.callerisuntrusted--;

	curtls.isFirstThread = true;

    IPC::SendIPCMessage(IPC::Command::CMD_POST_DLL_MAIN_DONE, nullptr, 0);
	DEBUG_LOG() << "returned.";

	return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule, 
                       DWORD  fdwReason, 
                       LPVOID lpReserved
					 )
{
	ThreadLocalStuff::DllManage(fdwReason);

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
        /*
         * HACK until new DLL injection is done.
         * TODO: Remove.
         * -- Warepire
         */
        IPC::SendIPCBufferAddress();
		tasflags.timescale = 1;
		tasflags.timescaleDivisor = 1;
        tasflags.log_categories = { true };

		DEBUG_LOG() << "DllMain started, injection must have worked.";
		hCurrentProcess = GetCurrentProcess();
		DisableThreadLibraryCalls(hModule);

		if(!s_frameThreadId)
            Hooks::SetThreadName(-1, "DllMain");

		Hooks::SyncDllMainInit();
		Hooks::TimerDllMainInit();
		Hooks::MessageDllMainInit();
		Hooks::SoundDllMainInit();
		Hooks::ModuleDllMainInit();

        IPC::SendIPCMessage(IPC::Command::CMD_DLL_VERSION, &VERSION, sizeof(VERSION));

        // tell it where to put commands
        IPC::SendIPCMessage(IPC::Command::CMD_COMMAND_BUF, commandSlot, sizeof(&commandSlot[0]));

        // tell it where to read/write full inputs status
        IPC::SendIPCMessage(IPC::Command::CMD_INPUT_BUF, &curinput, sizeof(&curinput));

        // tell it where to write dll load/unload info
        // must happen before we call Apply*Intercepts functions
        IPC::SendIPCMessage(IPC::Command::CMD_DLL_LOAD_INFO_BUF, &Hooks::dllLoadInfos, sizeof(&Hooks::dllLoadInfos));

        // tell it where to write trusted address range info
        IPC::SendIPCMessage(IPC::Command::CMD_TRUSTED_RANGE_INFO_BUF, &trustedRangeInfos, sizeof(&trustedRangeInfos));

        // tell it where to write other flags (current only movie playback flag)
        IPC::SendIPCMessage(IPC::Command::CMD_TAS_FLAGS_BUF, &tasflags, sizeof(&tasflags));

        // tell it where we put sound capture information
        IPC::SendIPCMessage(IPC::Command::CMD_SOUND_INFO, &Hooks::lastFrameSoundInfo, sizeof(&Hooks::lastFrameSoundInfo));

        // tell it where we put other information (statistics for the debugger, etc)
        IPC::SendIPCMessage(IPC::Command::CMD_GENERAL_INFO, &infoForDebugger, sizeof(&infoForDebugger));
        IPC::SendIPCMessage(IPC::Command::CMD_PALETTE_ENTRIES, &activePalette, sizeof(&activePalette));

		// for the external viewport (a test/debugging thing that's probably currently disabled)
		//cmdprintf("EXTHWNDBUF: %Iu", &extHWnd);


		//cmdprintf("GETDLLLIST: %Iu", dllLeaveAloneList);

		//SetPriorityClass(hCurrentProcess, HIGH_PRIORITY_CLASS);

		//{
		//	GetModuleFileNameA((HMODULE)hModule, currentModuleFilename, MAX_PATH);
		//	strcpy(dlltempDir, currentModuleFilename);
		//	char* dot = strrchr(dlltempDir, '.');
		//	char* slash = strrchr(dlltempDir, '\\');
		//	if(slash<dot)
		//		*dot = 0;
		//	strcat(dlltempDir, "\\dlltemp");
		//	CreateDirectoryA(dlltempDir, NULL);
		//	debugprintf(dlltempDir);
		//}

		//ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
        Hooks::ApplyTimeIntercepts();
        Hooks::ApplyTimerIntercepts();
        Hooks::ApplyModuleIntercepts();
        Hooks::ApplyThreadIntercepts();
        Hooks::ApplyMessageIntercepts();
        Hooks::ApplyWindowIntercepts();
        Hooks::DirectInput::ApplyDirectInputIntercepts();
        Hooks::WinInput::ApplyWinInputIntercepts();
        Hooks::ApplyGDIIntercepts();
        Hooks::ApplyDDrawIntercepts();
        Hooks::ApplyD3DIntercepts();
        Hooks::ApplyD3D8Intercepts();
        Hooks::ApplyD3D9Intercepts();
        Hooks::ApplyOGLIntercepts();
        Hooks::ApplySDLIntercepts();
        Hooks::DirectSound::ApplyDirectSoundIntercepts();
        Hooks::WinSound::ApplyWinSoundIntercepts();
        Hooks::ApplyWaitIntercepts();
        Hooks::ApplySyncIntercepts();
        Hooks::ApplyFileIntercepts();
        Hooks::ApplyRegistryIntercepts();
        Hooks::ApplyXinputIntercepts();

        IPC::SendIPCMessage(IPC::Command::CMD_GIMME_DLL_LOAD_INFOS, nullptr, 0);
		Hooks::UpdateLoadedOrUnloadedDllHooks();

		notramps = false;

		// tell it which keyboard layout is active,
		// let it replace it with one stored in a movie if it wants,
		// then load the possibly-new keyboard layout
		::GetKeyboardLayoutNameA(keyboardLayoutName);
        IPC::SendIPCMessage(IPC::Command::CMD_KEYBOARD_LAYOUT_NAME, keyboardLayoutName, sizeof(&keyboardLayoutName[0]));
		// moved to PostDllMain since it was causing a loader lock problem
		//LoadKeyboardLayout(keyboardLayoutName, KLF_ACTIVATE | KLF_REORDER | KLF_SETFORPROCESS);

		if(tasflags.appLocale)
		{
			SetConsoleCP(LocaleToCodePage(tasflags.appLocale));
			SetConsoleOutputCP(LocaleToCodePage(tasflags.appLocale));
			SetThreadLocale(tasflags.appLocale);
			SetThreadUILanguage(tasflags.appLocale);
			// disabled because it breaks fonts in pcb, undocumented anyway
			//if(HMODULE kernel32 = LoadLibrary("kernel32.dll"))
			//	if(SetCPGlobalType SetCPGlobal = (SetCPGlobalType)GetProcAddress(kernel32, "SetCPGlobal"))
			//		SetCPGlobal(LocaleToCodePage(tasflags.appLocale));
		}

        DEBUG_LOG() << "version = " << VERSION << ", movie version = " << tasflags.movieVersion << ", OS = " << tasflags.osVersionMajor << "." << tasflags.osVersionMinor;

		// in case PostDllMain doesn't get called right away (although we really need it to...)
		if(tasflags.osVersionMajor <= 6)
		{
			Hooks::cllApiNum = (IsWindows7() ? 65 : 66);
		}

		detTimer.Initialize(tasflags.initialTime);
		nonDetTimer.Initialize(tasflags.initialTime);

		{
			DWORD threadId = 0;
			HANDLE hThread = CreateThread(NULL, 0, PostDllMain, NULL, 0, &threadId); // note that Windows won't let this thread start executing until after DllMain returns
			SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL); // in case suspending the main thread doesn't work
			SetThreadPriorityBoost(hThread, TRUE);
            Hooks::SetThreadName(threadId, "PostDllMain");
		}
		if(!s_frameThreadId)
            Hooks::SetThreadName(-1, "Main");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

    DEBUG_LOG() << "DllMain returned. (fdwReason = " << fdwReason << ")";

    return TRUE;
}


// we have to export at least one thing or the game will fail to load our DLL
// if we're using IAT patching.
__declspec(dllexport) int fnwintased(void)
{
	return 42;
}
