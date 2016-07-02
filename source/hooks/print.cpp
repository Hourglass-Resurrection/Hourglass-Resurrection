/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <windows.h>
#include <cstdio>
#include "print.h"
#include "global.h"

#include "shared/ipc.h"
#include "intercept.h"

#include "tramps/timetramps.h"

//#define TRAMPFUNC __declspec(noinline)
////#define GetAsyncKeyState TrampGetAsyncKeyState
////TRAMPFUNC SHORT WINAPI GetAsyncKeyState(int vKey) ;
//extern bool notramps;
//#define OutputDebugStringA (notramps ? OutputDebugStringA : TrampOutputDebugStringA)
//TRAMPFUNC VOID WINAPI TrampOutputDebugStringA(LPCSTR lpOutputString);

extern TasFlags tasflags;

//extern int getCurrentTimestamp2();
//extern int getCurrentTimestamp3();

#ifndef debugprintf
int debugprintf(const char* fmt, ...)
{
    if (tasflags.debugPrintMode == 0)
    {
        return 0;
    }

    char str[4096];
    memset(str, '\0', sizeof(str));

    int threadStamp = Hooks::getCurrentThreadstamp();
    if (threadStamp)
    {
        _snprintf(str, ARRAYSIZE(str) - 1, "MSG: %08X: (f=%d, t=%d) ", threadStamp, Hooks::getCurrentFramestamp(), Hooks::getCurrentTimestamp());
    }
    else
    {
        _snprintf(str, ARRAYSIZE(str) - 1, "MSG: MAIN: (f=%d, t=%d) ", Hooks::getCurrentFramestamp(), Hooks::getCurrentTimestamp());
    }

    va_list args;
    va_start(args, fmt);
    int headerlen = strlen(str);
    int rv = vsnprintf(str + headerlen, (ARRAYSIZE(str) - 1) - headerlen, fmt, args);
    va_end(args);

    OutputDebugStringA(str);
    return rv;
}
#endif

int cmdprintf(const char* fmt, ...)
{
    char str[4096];
    memset(str, '\0', sizeof(str));

    va_list args;
    va_start(args, fmt);
    int rv = vsnprintf(str, ARRAYSIZE(str) - 1, fmt, args);
    va_end(args);

    OutputDebugStringA(str);
    return rv;
}

#ifdef ENABLE_LOGGING
int logprintf_internal(LogCategoryFlag cat, const char* fmt, ...)
{
    if (tasflags.debugPrintMode == 0)
    {
        return 0;
    }

    char str[4096];
    memset(str, '\0', sizeof(str));

    int threadStamp = Hooks::getCurrentThreadstamp();
    if (threadStamp)
    {
        _snprintf(str, ARRAYSIZE(str) - 1, "LOG: %08X: (f=%d, t=%d, c=%08X) ", threadStamp, Hooks::getCurrentFramestamp(), Hooks::getCurrentTimestamp(), cat);
    }
    else
    {
        _snprintf(str, ARRAYSIZE(str) - 1, "LOG: MAIN: (f=%d, t=%d, c=%08X) ", Hooks::getCurrentFramestamp(), Hooks::getCurrentTimestamp(), cat);
    }

    va_list args;
    va_start(args, fmt);
    int headerlen = strlen(str);
    int rv = vsnprintf(str + headerlen, (ARRAYSIZE(str) - 1) - headerlen, fmt, args);
    va_end(args);

    OutputDebugStringA(str);
    return rv;
}
#endif
