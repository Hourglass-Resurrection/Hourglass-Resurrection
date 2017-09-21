/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include <windows.h>

// no need to check this except in sensitive contexts (like LdrLoadDll)
// that could get called before DLL_PROCESS_ATTACH happens
extern bool tlsIsSafeToUse;

// _declspec(thread) is tempting, but ultimately too unreliable,
// so I use this for thread-local storage instead.
struct ThreadLocalStuff
{
    BOOL
        peekedMessage; // TODO: hopefully I can replace peekedMessage with something less hacky (for GDI hooking)
    BOOL isFrameThread;
    BOOL isFirstThread;
    BOOL createdFirstWindow;
    LONG crawlHackDisableCount; // testing
    LONG destroyWindowDepth;
    BOOL callingClientLoadLibrary;
    LONG
        treatDLLLoadsAsClient; // hack until callingClientLoadLibrary can be set in a more os-independent way
    LONG forceLocale;
#ifdef EMULATE_MESSAGE_QUEUES
    MessageQueue messageQueue;
#endif
    const char*
        curThreadCreateName; // threads created now will inherit this name. warning: can affect sync because named=known, unnamed=unknown, and denied unknown threads act differently than denied known threads

    __forceinline // this force inline gives me an extra few hundred FPS in cave story. most other inlining slows it down instead
        static ThreadLocalStuff&
        Get()
    {
        ThreadLocalStuff* stuff = (ThreadLocalStuff*) TlsGetValue(tlsIndex);
        if (!stuff)
            stuff = DllManage(DLL_THREAD_ATTACH);
        return *stuff;
    }

    __declspec(noinline) static ThreadLocalStuff* DllManage(DWORD fdwReason)
    {
        ThreadLocalStuff* stuff = NULL;
        switch (fdwReason)
        {
        case DLL_PROCESS_ATTACH:
            tlsIndex = TlsAlloc();
            tlsIsSafeToUse = true;
        // no break
        case DLL_THREAD_ATTACH:
            stuff = (ThreadLocalStuff*) LocalAlloc(LPTR, sizeof(ThreadLocalStuff));
            if (stuff)
            {
                stuff->Construct();
                TlsSetValue(tlsIndex, stuff);
            }
            break;
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            stuff = (ThreadLocalStuff*) TlsGetValue(tlsIndex);
            if (stuff)
                LocalFree((HLOCAL) stuff);
            if (fdwReason == DLL_PROCESS_DETACH)
                TlsFree(tlsIndex);
            break;
        }
        return stuff;
    }

    __forceinline static ThreadLocalStuff* GetIfAllocated()
    {
        ThreadLocalStuff* stuff =
            tlsIsSafeToUse ? (ThreadLocalStuff*) TlsGetValue(tlsIndex) : (ThreadLocalStuff*) 0;
        return stuff;
    }

private:
    static DWORD tlsIndex;
    void Construct()
    {
        peekedMessage = FALSE;
        isFrameThread = FALSE;
        isFirstThread = FALSE;
        createdFirstWindow = FALSE;
        crawlHackDisableCount = 0;
        destroyWindowDepth = 0;
        callingClientLoadLibrary = FALSE;
        treatDLLLoadsAsClient = 0;
        forceLocale = 0;
        curThreadCreateName = NULL;
#ifdef EMULATE_MESSAGE_QUEUES
        messageQueue.queueStatus = /*QS_NONE*/ 0;
        messageQueue.queueStatusAtLastGet = /*QS_NONE*/ 0;
        messageQueue.messages.clear();
        messageQueue.attachedWindows.clear();
        messageQueue.attachedWindows.push_back(
            (HWND) NULL); // so PostMessage with a NULL HWND knows to post to the current thread
        messageQueue.quit = false;
        messageQueue.timer = 0;
        messageQueue.key = 0;
        messageQueue.hotkey = 0;
        messageQueue.mousemove = 0;
        messageQueue.mousebutton = 0;
        messageQueue.rawinput = 0;
#endif
    }
};

// you can pretend tls is a global variable of type ThreadLocalStuff.
#define tls ThreadLocalStuff::Get()
// it is a system call, though,
// so if you're going to use it more than once in a function,
// it's best to cache the result like this:
// 	ThreadLocalStuff& curtls = tls;
