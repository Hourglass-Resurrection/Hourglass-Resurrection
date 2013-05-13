/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(PRINT_C_INCL) && !defined(UNITY_BUILD)
#define PRINT_C_INCL

#include <windows.h>
#include <stdio.h>
#include "print.h"
#include "global.h"

#include "../shared/ipc.h"
#include "intercept.h"

//#define TRAMPFUNC __declspec(noinline)
////#define GetAsyncKeyState TrampGetAsyncKeyState
////TRAMPFUNC SHORT WINAPI GetAsyncKeyState(int vKey) ;
//extern bool notramps;
//#define OutputDebugStringA (notramps ? OutputDebugStringA : TrampOutputDebugStringA)
//TRAMPFUNC VOID WINAPI TrampOutputDebugStringA(LPCSTR lpOutputString);

extern TasFlags tasflags;

extern int getCurrentThreadstamp();
extern int getCurrentFramestamp();
extern int getCurrentTimestamp();
//extern int getCurrentTimestamp2();
//extern int getCurrentTimestamp3();
TRAMPFUNC DWORD WINAPI TramptimeGetTime(void);

#ifndef debugprintf
int debugprintf(const char * fmt, ...)
{
	//if(!notramps && GetAsyncKeyState('A') & 0x8000)
	//	return 0;
	if(tasflags.debugPrintMode == 0)
		return 0;
	char str[4096];
	//str[sizeof(str)-1] = 0x61;
	va_list args;
	va_start (args, fmt);
	strcpy(str, "MSG: ");
	int threadStamp = getCurrentThreadstamp();
	// TODO: passing in extra argument (notramps?0:TramptimeGetTime()) makes cave story run significantly faster in fast-forward, weird, but that's why I've left it here for now
	if(threadStamp)
		sprintf(str+(sizeof("MSG: ")-1), "%08X: (f=%d, t=%d) ", getCurrentThreadstamp(), getCurrentFramestamp(), getCurrentTimestamp(), notramps?0:TramptimeGetTime());
	else
		sprintf(str+(sizeof("MSG: ")-1), "MAIN: (f=%d, t=%d) ", getCurrentFramestamp(), getCurrentTimestamp(), notramps?0:TramptimeGetTime());
	int headerlen = strlen(str);
	//int rv = vsprintf (str+5+10, fmt, args);
	int rv = vsprintf(str+headerlen, fmt, args);
	va_end (args);
	OutputDebugStringA(str);
	//if(str[sizeof(str)-1] != 0x61) { _asm{int 3} } // buffer overrun alert
	return rv;
}
#endif
int cmdprintf(const char * fmt, ...)
{
	char str[4096];
	//str[sizeof(str)-1] = 0x67;
	va_list args;
	va_start (args, fmt);
	int rv = vsprintf(str, fmt, args);
	va_end (args);
	OutputDebugStringA(str);
	//if(str[sizeof(str)-1] != 0x67) { _asm{int 3} } // buffer overrun alert
	return rv;
}

#ifdef ENABLE_LOGGING
int logprintf_internal(LogCategoryFlag cat, const char * fmt, ...)
{
	if(tasflags.debugPrintMode == 0)
		return 0;

	char str[4096];
	//str[sizeof(str)-1] = 0x69;
	va_list args;
	va_start (args, fmt);
	strcpy(str, "LOG: ");
	int threadStamp = getCurrentThreadstamp();
	if(threadStamp)
		sprintf(str+(sizeof("LOG: ")-1), "%08X: (f=%d, t=%d, c=%08X) ", getCurrentThreadstamp(), getCurrentFramestamp(), getCurrentTimestamp(), cat, notramps?0:TramptimeGetTime());
	else
		sprintf(str+(sizeof("LOG: ")-1), "MAIN: (f=%d, t=%d, c=%08X) ", getCurrentFramestamp(), getCurrentTimestamp(), cat, notramps?0:TramptimeGetTime());
	int headerlen = strlen(str);
	//int rv = vsprintf(str+5+10, fmt, args);
	int rv = vsprintf(str+headerlen, fmt, args);
	va_end (args);
	OutputDebugStringA(str);
	//if(str[sizeof(str)-1] != 0x69) { _asm{int 3} } // buffer overrun alert
	return rv;
}
#endif

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
