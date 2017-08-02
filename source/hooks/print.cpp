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
