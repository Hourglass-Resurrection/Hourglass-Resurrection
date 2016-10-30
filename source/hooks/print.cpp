/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <windows.h>
#include <cstdio>
#include "print.h"
//#include "global.h"

#include "shared/ipc.h"
#include "ipc.h"
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

VerboseLog::VerboseLog()
{
#ifdef VERBOSE_DEBUG
    int threadStamp = Hooks::getCurrentThreadstamp();
    if (threadStamp)
    {
        m_print_message << threadStamp << ": ";
    }
    else
    {
        m_print_message << "MAIN: ";
    }
    m_print_message << "(f=" << Hooks::getCurrentFramestamp()
        << ", t=" << Hooks::getCurrentTimestamp() << ") ";
#endif
}

VerboseLog::~VerboseLog()
{
#ifdef VERBOSE_DEBUG
    IPC::SendIPCMessage(IPC::Command::CMD_PRINT_MESSAGE, &m_print_message, sizeof(m_print_message));
#endif
}

/*
* TODO: Enable constexpr when C++14 extended constexpr is supported
* -- Warepire
*/
/*constexpr*/ const char* LogCategoryToString(LogCategory category)
{
    switch (category)
    {
    case LogCategory::ANY:
        return "";
    case LogCategory::HOOK:
        return "[Hook]";
    case LogCategory::TIME:
        return "[Time]";
    case LogCategory::DETTIMER:
        return "[Det. Timer]";
    case LogCategory::SYNC:
        return "[Synchronization]";
    case LogCategory::DDRAW:
        return "[DDraw]";
    case LogCategory::D3D:
        return "[D3D]";
    case LogCategory::OGL:
        return "[OpenGL:TODO:Stop wrapping D3D!]";
    case LogCategory::GDI:
        return "[GDI]";
    case LogCategory::SDL:
        return "[SDL:TODO:Remove!]";
    case LogCategory::DINPUT:
        return "[DInput]";
    case LogCategory::WINPUT:
        return "[WinInput]";
    case LogCategory::XINPUT:
        return "[XInput]";
    case LogCategory::DSOUND:
        return "[DSound";
    case LogCategory::WSOUND:
        return "[WSound]";
    case LogCategory::PROCESS:
        return "[Process]";
    case LogCategory::MODULE:
        return "[Module]";
    case LogCategory::MESSAGES:
        return "[Messages]";
    case LogCategory::WINDOW:
        return "[Window]";
    case LogCategory::FILEIO:
        return "[File I/O]";
    case LogCategory::REGISTRY:
        return "[Registry]";
    case LogCategory::THREAD:
        return "[Thread]";
    case LogCategory::TIMERS:
        return "[Timers]";
    default:
        return "[Unknown]";
    }
    return "";
}
