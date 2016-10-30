/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <map>
#include <string>

#include "../wintasee.h"
#include "../tls.h"
//#include <setjmp.h>

extern char commandSlot[256 * 8];

using Log = DebugLog<LogCategory::THREAD>;

namespace Hooks
{
    void CloseHandles(DWORD threadId); // extern

#define THREAD_NAME_EXCEPTION 0x406D1388
#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    void SetThreadName(DWORD dwThreadID, char* threadName)
    {
        // note: we can only call unhooked functions here, since SetThreadName can get called before anything is hooked
        if (notramps)
            UntrampedSleep(10);
        else
            Sleep(10);
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;

        __try
        {
            RaiseException(THREAD_NAME_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }


    struct ThreadWrapperInfo
    {
        struct Arguments
        {
            LPSECURITY_ATTRIBUTES lpThreadAttributes;
            SIZE_T dwStackSize;
            LPTHREAD_START_ROUTINE lpStartAddress;
            LPVOID lpParameter;
            DWORD dwCreationFlags;
        } args;
        HANDLE handle;
        HANDLE privateHandle;
        HANDLE exitEvent;
        DWORD threadId;
        bool idling;
        bool comatose;
        volatile DWORD exitcode;
        CONTEXT beforecallcontext;
        //jmp_buf beforecallbuf;
        bool beforecallvalid;
        ThreadWrapperInfo()
        {
            idling = false;
            comatose = false;
            exitcode = 0;
            handle = 0;
            privateHandle = 0;
            exitEvent = 0;
            threadId = 0;
            beforecallvalid = false;
        }
    };

    std::map<DWORD, ThreadWrapperInfo*> threadWrappers;
    std::map<HANDLE, DWORD> threadWrappersOriginalHandleToId;

    DWORD WINAPI MyThreadWrapperThread(LPVOID lpParam)
    {
        if (tasflags.appLocale)
        {
            SetThreadLocale(tasflags.appLocale);
            SetThreadUILanguage(tasflags.appLocale);
        }
        ENTER();
        DWORD threadId = GetCurrentThreadId();
        ThreadWrapperInfo& info = *(ThreadWrapperInfo*)lpParam;
        while (true)
        {
            info.exitcode = STILL_ACTIVE;
            info.beforecallcontext.ContextFlags = CONTEXT_FULL;
            GetThreadContext(GetCurrentThread(), &info.beforecallcontext);
            //setjmp(info.beforecallbuf);
    //		if(info.args.dwCreationFlags & CREATE_SUSPENDED)
    //			SuspendThread(GetCurrentThread());
    //		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
            info.beforecallvalid = true;
            if (info.exitcode == STILL_ACTIVE)
                info.exitcode = info.args.lpStartAddress(info.args.lpParameter);
            info.beforecallvalid = false;
            //		return info.exitcode;
            info.idling = true;
            LOG() << "Pretended to exit. handle=" << info.handle << ", id=" << GetCurrentThreadId();
            CloseHandles(threadId);
            //CloseHandle(info.handle);
            SetEvent(info.exitEvent);
            while (info.idling)
                Sleep(1);
            LOG() << "Reused thread. handle=" << info.handle << ", id=" << GetCurrentThreadId();
        }
    }

    DWORD WINAPI MyEmptyThreadFunction(LPVOID lpParam)
    {
        Sleep(100); // no reason besides making debug output a little better
        return 0;
    }

    // brief explanation of the threadMode options as far as thread creation goes:
    // 0 -> disable threads. either return failure or create a do-nothing thread and return success.
    // 1 -> wrapped threads. create a thread that can be reused for a future thread creation in some circumstances. the main purpose for this is to make savestate code less likely to fail due to fewer actual threads being created over time.
    // 2 -> allow threads. create a thread normally, as if we hadn't hooked CreateThread.
    // 3 -> micromanaged "synchronous" threads. not currently working. abandoned and disabled, actually.

    HOOK_FUNCTION(HANDLE, WINAPI, CreateThread,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId
    );
    HOOKFUNC HANDLE WINAPI MyCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId
    )
    {
        ENTER(lpStartAddress, tls.curThreadCreateName);
        //cmdprintf("SHORTTRACE: 3,50");

        if (tasflags.threadMode == 0 || tasflags.threadMode == 3 && !tls.curThreadCreateName || tasflags.threadMode == 4 && tls.curThreadCreateName || (tasflags.threadMode == 5 && !VerifyIsTrustedCaller(!tls.callerisuntrusted)))
        {
            const char* threadTypeName = tls.curThreadCreateName;
            LOG() << "thread creation denied. name=" << (threadTypeName ? threadTypeName : "unknown_thread");
            IPC::SendIPCMessage(IPC::Command::CMD_DENIED_THREAD, &lpStartAddress, sizeof(&lpStartAddress));

            // FIXME: it's a terrible hack to choose between these two methods depending on whether we have a thread name,
            // but it gets herocore working with threads disabled, and I can't think of a better solution at the moment.
            // the reason it helps there is that herocore.exe crashes if it can't create a thread but works correctly if it creates a do-nothing thread,
            // whereas things like DirectSound (which I've named their threads) correctly handle the creation failure case but freeze if a do-nothing thread is created.
            if (threadTypeName)
            {
                SetLastError(ERROR_MAX_THRDS_REACHED);
                return 0;
            }
            else
            {
                DWORD threadId;
                if (!lpThreadId)
                    lpThreadId = &threadId;
                HANDLE rv = CreateThread(lpThreadAttributes, dwStackSize, MyEmptyThreadFunction, lpParameter, dwCreationFlags, lpThreadId);
                char name[64];
                if (!threadTypeName)
                {
                    IPC::SuggestThreadName suggested_name;
                    IPC::SendIPCMessage(IPC::Command::CMD_SUGGEST_THREAD_NAME, &suggested_name, sizeof(suggested_name));
                    threadTypeName = suggested_name.GetThreadName();
                }
                sprintf(name, "%u_FAKE_%s_at_%u", threadCounter++, threadTypeName, detTimer.GetTicks());
                SetThreadName(*lpThreadId, name);
                return rv;
            }
        }
        else if (tasflags.threadMode >= 2)
        {
            //return CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);

            // create and set a default thread name
            DWORD threadId;
            if (!lpThreadId)
                lpThreadId = &threadId;
            HANDLE rv = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
            char name[64];
            const char* threadTypeName = tls.curThreadCreateName;
            if (!threadTypeName)
            {
                IPC::SuggestThreadName suggested_name;
                IPC::SendIPCMessage(IPC::Command::CMD_SUGGEST_THREAD_NAME, &suggested_name, sizeof(suggested_name));
                threadTypeName = suggested_name.GetThreadName();
            }
            sprintf(name, "%u_%s_at_%u", threadCounter++, threadTypeName, detTimer.GetTicks());
            SetThreadName(*lpThreadId, name);
            LOG() << "created real thread and named it: " << name;
            return rv;
        }

        //	return 0;
        //	_asm{int 3};
        //	return 0;
            //static int time = 0;
            //time++;
            //if(time <= 6)
            //	return 0;

        //	debuglog(LCF_THREAD, "!!!CreateThread(0x%X)\n", lpStartAddress);

        //	DWORD threadid = 0;
        //	HANDLE rv = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress,
        //		lpParameter,dwCreationFlags,&threadid);
        //	if(lpThreadId)
        //		*lpThreadId = threadid;
        //////	HANDLE copy = OpenThread(THREAD_ALL_ACCESS, FALSE, threadid);
        ////	CONTEXT context;
        ////	context.ContextFlags = CONTEXT_ALL;
        ////	GetThreadContext(rv, &context);
        //////	TerminateThread(rv, 0);
        ////	BOOL worked = SetThreadContext(rv, &context);
        ////	debugprintf("worked: %d\n", worked);
        //	return rv;

            // try to reuse a thread, to avoid leaking lots of thread handles:
        if (dwStackSize == 0) dwStackSize = tasflags.threadStackSize; // In the case of 0 (default) assign it the default stack size.
        std::map<DWORD, ThreadWrapperInfo*>::iterator iter;
        for (iter = threadWrappers.begin(); iter != threadWrappers.end(); iter++)
        {
            ThreadWrapperInfo* twi = iter->second;
            //debugprintf("!!! 0x%X\n",twi);
            if (twi && twi->idling && !twi->comatose &&
                twi->args.dwStackSize == dwStackSize &&
                twi->args.lpThreadAttributes == lpThreadAttributes)
            {
                twi->args.lpStartAddress = lpStartAddress;
                twi->args.lpParameter = lpParameter;
                twi->args.dwCreationFlags = dwCreationFlags;
                DWORD resumeResult;
                while (true)
                {
                    resumeResult = ResumeThread(twi->privateHandle);
                    if (resumeResult <= 1)
                        break;
                    if (resumeResult == (DWORD)-1)
                        break;
                }
                if (resumeResult == (DWORD)-1)
                {
                    LOG() << "abandoning thread (handle=" << twi->handle << " (ph=" << twi->privateHandle
                          << "), id=" << twi->threadId << "! error code " << GetLastError();
                    twi->comatose = true;
                    continue; // private handle became invalid somehow?... give it up and keep searching for something else that's valid to reuse
                }
                if (dwCreationFlags & CREATE_SUSPENDED)
                    SuspendThread(twi->privateHandle);
                if (lpThreadId)
                    *lpThreadId = twi->threadId;
                SetThreadPriority(twi->privateHandle, THREAD_PRIORITY_NORMAL);

                // generate a new public handle (because the game might have closed the old one with CloseHandle)
                HANDLE oldHandle = twi->handle;
                threadWrappersOriginalHandleToId[twi->handle] = NULL;
                DuplicateHandle(GetCurrentProcess(), twi->privateHandle, GetCurrentProcess(), &twi->handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
                threadWrappersOriginalHandleToId[twi->handle] = twi->threadId;

                ResetEvent(twi->exitEvent);

                // (re)name wrapper thread
                {
                    char name[64];
                    const char* threadTypeName = tls.curThreadCreateName;
                    if (!threadTypeName)
                    {
                        IPC::SuggestThreadName suggested_name;
                        IPC::SendIPCMessage(IPC::Command::CMD_SUGGEST_THREAD_NAME, &suggested_name, sizeof(suggested_name));
                        threadTypeName = suggested_name.GetThreadName();
                    }
                    sprintf(name, "%u_%s_at_%u", threadCounter++, threadTypeName, detTimer.GetTicks());
                    SetThreadName(twi->threadId, name);
                    LOG() << "reused wrapper thread and renamed it: " << name << ". handle=" << oldHandle
                          << "->" << twi->handle << " (ph=" << twi->privateHandle << "), id=" << twi->threadId;
                }

                twi->idling = false;
                return twi->handle;
            }
        }

        // make sure we'll get a thread ID
        DWORD threadId;
        if (!lpThreadId)
            lpThreadId = &threadId;

        // actually make a new thread:
        ThreadWrapperInfo* twi = new ThreadWrapperInfo();
        twi->args.lpThreadAttributes = lpThreadAttributes;
        twi->args.dwStackSize = dwStackSize;
        twi->args.lpStartAddress = lpStartAddress;
        twi->args.lpParameter = lpParameter;
        twi->args.dwCreationFlags = dwCreationFlags;
        HANDLE handle = CreateThread(
            lpThreadAttributes,
            // It's important to add our head to the stack size so that games that create threads with optimized stack sizes
            // doesn't run out of stack because of us.
            (dwStackSize + sizeof(ThreadWrapperInfo)),
            MyThreadWrapperThread,
            twi,
            dwCreationFlags,
            lpThreadId
        );
        //#pragma message("FIXMEEE")
        //	if(!tls.curThreadCreateName){
        //		debugprintf("OMFG!!\n");
        //cmdprintf("SHORTTRACE: 3,50");
        //		_asm{int 3}
        //		SuspendThread(handle);SuspendThread(handle);} //
        twi->handle = handle;
        DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &twi->privateHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
        threadId = *lpThreadId;
        threadWrappers[threadId] = twi;
        threadWrappersOriginalHandleToId[handle] = threadId;
        twi->threadId = threadId;

        twi->exitEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

        // name wrapper threads too
        {
            char name[64];
            const char* threadTypeName = tls.curThreadCreateName;
            if (!threadTypeName)
            {
                IPC::SuggestThreadName suggested_name;
                IPC::SendIPCMessage(IPC::Command::CMD_SUGGEST_THREAD_NAME, &suggested_name, sizeof(suggested_name));
                threadTypeName = suggested_name.GetThreadName();
            }
            sprintf(name, "%u_%s_at_%u", threadCounter++, threadTypeName, detTimer.GetTicks());
            SetThreadName(twi->threadId, name);
            LOG() << "created real (wrapper) thread and named it: " << name << ". handle=" << twi->handle
                  << " (ph=" << twi->privateHandle << "), id=" << twi->threadId;
        }

        return handle;
    }
    HOOK_FUNCTION(VOID, WINAPI, ExitThread, DWORD dwExitCode);
    HOOKFUNC VOID WINAPI MyExitThread(DWORD dwExitCode)
    {
        ENTER(dwExitCode);
        DWORD threadId = GetCurrentThreadId();
        LOG() << "called for thread: " << threadId;
        ThreadWrapperInfo* twi = threadWrappers[threadId];
        if (twi)
        {
            if (twi->idling)
                return;

            HANDLE hThread = GetCurrentThread();
            twi->exitcode = dwExitCode;
            twi->comatose = true;
            //		SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);

            CloseHandles(threadId);
            //CloseHandle(twi->handle);
            SetEvent(twi->exitEvent);
            // ^maybe that addresses the following, but I'm not sure yet:
            // FIXME: this doesn't actually release all of the thread's resources,
            // so it could cause a deadlock...
            // maybe the only way to wrap threads completely properly is to
            // spoof all thread IDs the game can receive and then
            // make the thread saving/loading code a lot more complicated.
    //		if(twi->beforecallvalid) 
    //		{
    ////			SetThreadContext(hThread, &twi->beforecallcontext);
    //			longjmp(twi->beforecallbuf, dwExitCode);
    //		}
            twi->idling = true;
            while (true)
                SuspendThread(hThread);
        }
        tls.callerisuntrusted++;
        ExitThread(dwExitCode);
    }

    HOOK_FUNCTION(BOOL, WINAPI, TerminateThread, HANDLE hThread, DWORD dwExitCode);
    HOOKFUNC BOOL WINAPI MyTerminateThread(HANDLE hThread, DWORD dwExitCode)
    {
        ENTER(dwExitCode);

        //DWORD threadId = GetThreadId(hThread); // function doesn't exist on windows 2000...
        DWORD threadId = (hThread == GetCurrentThread()) ? GetCurrentThreadId() : threadWrappersOriginalHandleToId[hThread];
        ThreadWrapperInfo* twi = threadWrappers[threadId];
        if (twi)
        {
            if (twi->idling || twi->comatose)
                return TRUE;
            CloseHandles(threadId);
            //CloseHandle(twi->handle);
            SetEvent(twi->exitEvent);
            twi->exitcode = dwExitCode;
            twi->comatose = true;
            for (int i = 0; i < 10; i++)
                SuspendThread(hThread);
            return TRUE;
        }
        return TerminateThread(hThread, dwExitCode);
    }
    HOOK_FUNCTION(BOOL, WINAPI, GetExitCodeThread, HANDLE hThread, LPDWORD lpExitCode);
    HOOKFUNC BOOL WINAPI MyGetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode)
    {
        ENTER(hThread, lpExitCode);
        DWORD threadId = (hThread == GetCurrentThread()) ? GetCurrentThreadId() : threadWrappersOriginalHandleToId[hThread];
        ThreadWrapperInfo* twi = threadWrappers[threadId];
        if (twi && (twi->comatose || twi->idling))
        {
            if (lpExitCode)
            {
                *lpExitCode = twi->exitcode;
                LOG() << "wrapper thread faked exit code " << *lpExitCode;
                return TRUE;
            }
            return FALSE;
        }
        BOOL rv = GetExitCodeThread(hThread, lpExitCode);
        LOG() << "returned " << rv;
        if (rv)
            LOG() << " thread gave exit code " << *lpExitCode;
        return rv;
    }

    HOOK_FUNCTION(NTSTATUS, NTAPI, NtSetInformationThread, HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
    HOOKFUNC NTSTATUS NTAPI MyNtSetInformationThread(HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength)
    {
        if (ThreadInformationClass == 0x11/*ThreadHideFromDebugger*/)
        {
            LOG() << "denied setting ThreadHideFromDebugger";
            return 0; // STATUS_SUCCESS
        }
        NTSTATUS rv = NtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
        return rv;
    }

    struct _tiddata {
        unsigned long   _tid;       /* thread ID */
        uintptr_t _thandle;         /* thread handle */
        int     _terrno;            /* errno value */
        unsigned long   _tdoserrno; /* _doserrno value */
        unsigned int    _fpds;      /* Floating Point data segment */
        unsigned long   _holdrand;  /* rand() seed value */
                                    //There's more than this in a full _tiddata struct, but this is probably everything we're interested in
    };

    typedef struct _tiddata * _ptiddata;
    BOOL FlsRecursing = FALSE;
    std::map<DWORD, DWORD *> fseeds;
    HOOK_FUNCTION(BOOL, WINAPI, FlsSetValue, DWORD dwFlsIndex, LPVOID lpFlsData);
    HOOKFUNC BOOL WINAPI MyFlsSetValue(DWORD dwFlsIndex, LPVOID lpFlsData) {
        BOOL rv = FlsSetValue(dwFlsIndex, lpFlsData);
        if ((!FlsRecursing) && (lpFlsData != NULL)) {
            FlsRecursing = TRUE;
            if (fseeds.find(dwFlsIndex) == fseeds.end()) {
                _ptiddata ptd = (_ptiddata)FlsGetValue(dwFlsIndex);
                LOG() << "FlsSetValue(" << dwFlsIndex << ",lpFlsData), set _tiddata structure at " << ptd;
                IPC::AutoWatch auto_watch(&(ptd->_holdrand), 'd', 'u', (std::string("AutoRandSeed_Fiber_") + std::to_string(dwFlsIndex)).c_str());
                IPC::SendIPCMessage(IPC::Command::CMD_WATCH_ADDRESS, &auto_watch, sizeof(auto_watch));
                fseeds[dwFlsIndex] = &(ptd->_holdrand);
            }
            FlsRecursing = FALSE;
        }
        return rv;
    }
    //BOOL TlsRecursing = FALSE;
    //std::map<DWORD, DWORD *> tseeds;
    //HOOK_FUNCTION(BOOL, WINAPI, TlsSetValue, DWORD dwTlsIndex, LPVOID lpTlsValue);
    //HOOKFUNC BOOL WINAPI MyTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) {
    //    BOOL rv = TlsSetValue(dwTlsIndex, lpTlsValue);
    //    if ((!TlsRecursing) && (lpTlsValue != NULL)) {
    //        TlsRecursing = TRUE;
    //        if (tseeds.find(dwTlsIndex) == tseeds.end()) {
    //            _ptiddata ptd = (_ptiddata)TlsGetValue(dwTlsIndex);
    //            LOG() << "TlsSetValue(" << dwTlsIndex << ",lpTlsValue), set _tiddata structure at " << ptd;
    //            IPC::AutoWatch auto_watch(&(ptd->_holdrand), 'd', 'u', (std::string("AutoRandSeed_Thread_") + std::to_string(dwTlsIndex)).c_str());
    //            IPC::SendIPCMessage(IPC::Command::CMD_WATCH_ADDRESS, &auto_watch, sizeof(auto_watch));
    //            tseeds[dwTlsIndex] = &(ptd->_holdrand);
    //        }
    //        TlsRecursing = FALSE;
    //    }
    //    return rv;
    //}

    // not really hooked, I just needed their trampolines
    //HOOK_FUNCTION(LPVOID, WINAPI, TlsGetValue, DWORD dwTlsIndex);
    //HOOKFUNC LPVOID WINAPI MyTlsGetValue(DWORD dwTlsIndex) IMPOSSIBLE_IMPL
    HOOK_FUNCTION(PVOID, WINAPI, FlsGetValue, DWORD dwFlsIndex);
    HOOKFUNC PVOID WINAPI MyFlsGetValue(DWORD dwFlsIndex) IMPOSSIBLE_IMPL

    void ThreadHandleToExitHandle(HANDLE& hHandle)
    {
        std::map<HANDLE, DWORD>::iterator found = threadWrappersOriginalHandleToId.find(hHandle);
        if (found != threadWrappersOriginalHandleToId.end())
        {
            std::map<DWORD, ThreadWrapperInfo*>::iterator found2 = threadWrappers.find(found->second);
            if (found2 != threadWrappers.end())
            {
                ThreadWrapperInfo* twi = found2->second;
                hHandle = twi->exitEvent;
            }
        }
    }

    void ApplyThreadIntercepts()
    {
        static const InterceptDescriptor intercepts[] =
        {
            MAKE_INTERCEPT(1, KERNEL32, CreateThread),
            MAKE_INTERCEPT(1, KERNEL32, ExitThread),
            MAKE_INTERCEPT(1, KERNEL32, TerminateThread),
            MAKE_INTERCEPT(1, KERNEL32, GetExitCodeThread),
            MAKE_INTERCEPT(1, NTDLL, NtSetInformationThread),

            MAKE_INTERCEPT(0, KERNEL32, FlsGetValue), // get trampoline only
            //MAKE_INTERCEPT(0, KERNEL32, TlsGetValue), // get trampoline only
            MAKE_INTERCEPT(1, KERNEL32, FlsSetValue),
            //MAKE_INTERCEPT(1, KERNEL32, TlsSetValue),
        };
        ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
    }
}
