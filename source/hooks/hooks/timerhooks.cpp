/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../wintasee.h"
#include "../msgqueue.h"
#include <algorithm>
#include <vector>
#include <map>
#include <set>

using Log = DebugLog<LogCategory::TIMERS>;

namespace Hooks
{
    void TickMultiMediaTimers(DWORD time = 0); // extern? (I mean, move to header)

    struct SetTimerData
    {
        HWND hWnd;
        UINT_PTR nIDEvent;
        DWORD targetTime;
        TIMERPROC lpTimerFunc;

        // Stuff below required?
        bool killRequested; // The place where this is used (MySetTimerTimerThread) is not used anywhere
        bool operator < (const SetTimerData& other) const
        {
            LARGE_INTEGER a = { (DWORD)hWnd, (LONG)nIDEvent };
            LARGE_INTEGER b = { (DWORD)other.hWnd, (LONG)other.nIDEvent };
            return a.QuadPart < b.QuadPart;
        }
    };
    struct SetTimerDataCompare // Struct with functor that helps us sort the elements in the set.
    {
        bool operator() (const SetTimerData& a, const SetTimerData& b) const
        {
            return a.nIDEvent < b.nIDEvent;
        }
    };
    // Ordered set, we're sorting on TimerID as that is the most important detail in the struct.
    static std::set<SetTimerData, SetTimerDataCompare> s_pendingSetTimers;
    static CRITICAL_SECTION s_pendingSetTimerCS;

    // Creates a guaranteed unique ID that is as low as possible 
    UINT_PTR CreateNewTimerID()
    {
        UINT_PTR rv = 1;
        for (std::set<SetTimerData, SetTimerDataCompare>::iterator it = s_pendingSetTimers.begin(); it != s_pendingSetTimers.end(); it++)
        {
            if (rv == it->nIDEvent)
            {
                rv++;
            }
            else
            {
                break;
            }
        }

        return rv;
    }

    //static bool inProcessTimers = false;
    void ProcessTimers()
    {
        //	if(inProcessTimers)
        //		return; // disabled, some games need to recurse
        //	inProcessTimers = true;

        {
            DWORD time = detTimer.GetTicks();
            //	DWORD earliestTriggerTime = (DWORD)(time + 0x7FFFFFFF);

            std::vector<SetTimerData> triggeredTimers;
            //	bool triedAgain = false;
            //tryAgain:
            EnterCriticalSection(&s_pendingSetTimerCS);
            std::set<SetTimerData, SetTimerDataCompare>::iterator iter;
            for (iter = s_pendingSetTimers.begin(); iter != s_pendingSetTimers.end();)
            {
                ////		debugprintf("HOO: %d, %d\n", value.targetTime, time);
                //		if((int)(earliestTriggerTime - value.targetTime) > 0)
                //			earliestTriggerTime = value.targetTime;
                if ((int)(time - iter->targetTime) >= 0)
                {
                    triggeredTimers.push_back(*iter);
                    s_pendingSetTimers.erase(iter++);
                }
                else
                {
                    iter++;
                }
            }
            LeaveCriticalSection(&s_pendingSetTimerCS);

            //	if(!s_frameThreadId && triggeredTimers.empty() && !triedAgain)
            //	{
            ////		debugprintf("HAA: %d, %d\n", earliestTriggerTime, time);
            //		if((int)(earliestTriggerTime - time) > 0)
            //		{
            ////			debugprintf("HA: %d\n", earliestTriggerTime - time);
            //			detTimer.AddDelay(earliestTriggerTime - time, FALSE, TRUE, TRUE);
            //			triedAgain = true;
            //			goto tryAgain;
            //		}
            //	}

            if (!(tasflags.timersMode == 0))
            {
                for (unsigned int i = 0; i < triggeredTimers.size(); i++)
                {
                    SetTimerData data = triggeredTimers[i];

                    LOG() << "timer triggered: hWnd=" << data.hWnd << " nIDEvent=" << data.nIDEvent
                          << " targetTime=" << data.targetTime << " lpTimerFunc=" << data.lpTimerFunc;

                    if (data.lpTimerFunc)
                    {
                        data.lpTimerFunc(data.hWnd, WM_TIMER, data.nIDEvent, time);
                    }
                    else
                    {
                        // posting it doesn't work for some reason (iji hangs on startup)
                        //PostMessageInternal(key.hWnd, WM_TIMER, key.nIDEvent, (LPARAM)value.lpTimerFunc);
                        DispatchMessageInternal(data.hWnd, WM_TIMER, data.nIDEvent, (LPARAM)data.lpTimerFunc, true, MAF_PASSTHROUGH | MAF_RETURN_OS);
                    }
                }
            }
            triggeredTimers.clear();
        }

        TickMultiMediaTimers();
        //inProcessTimers = false;
    }

    // Is this even used anywhere?
    /*
    DWORD WINAPI MySetTimerTimerThread(LPVOID lpParam)
    {
        debuglog(LCF_TIMERS|LCF_THREAD, __FUNCTION__ " called.\n");
        SetTimerKey& key = *(SetTimerKey*)lpParam;
        SetTimerValue value;
        bool triggered = false;
        bool killRequest = false;
    int realstarttime = timeGetTime();
    int fakestarttime = detTimer.GetTicks();
        while(!triggered)
        {
            MySleep(100);
    //		DWORD time = detTimer.GetTicks(TIMETYPE_SETTIMER);
            DWORD time = detTimer.GetTicks();

            std::map<SetTimerKey,SetTimerValue>::iterator iter;
            EnterCriticalSection(&s_pendingSetTimerCS);
            iter = s_pendingSetTimers.find(key);
            if(iter != s_pendingSetTimers.end())
            {
                value = iter->second;
                if(value.killRequested)
                    killRequest = true;
                else
                {
                    int timeOver = (int)(time - value.targetTime);
                    debuglog(LCF_TIMERS, "timeOver = %d, realElapsed = %d, fakeElapsed = %d\n", timeOver, timeGetTime() - realstarttime, detTimer.GetTicks() - fakestarttime);
                    if(timeOver >= 0)
                    {
                        s_pendingSetTimers.erase(iter);
                        triggered = true;
                    }
                    else
                    {
    //					MySleep(-timeOver/2);
                    }
                }
            }
            else
            {
                killRequest = true;
            }
            LeaveCriticalSection(&s_pendingSetTimerCS);
            if(killRequest)
                break;
            if(triggered && !(tasflags.timersMode == 0))
            {
                debuglog(LCF_TIMERS, "timer triggered: 0x%X, 0x%X, %d, 0x%X\n", key.hWnd, key.nIDEvent, value.targetTime, value.lpTimerFunc);
                MySleep(0);

                if(value.lpTimerFunc)
                    value.lpTimerFunc(key.hWnd, WM_TIMER, key.nIDEvent, time);
                else
                {
                    //SendMessageA(key.hWnd, toggleWhitelistMessage(WM_TIMER), key.nIDEvent, (LPARAM)value.lpTimerFunc);
    //#ifdef EMULATE_MESSAGE_QUEUES
                    //MyPostMessageA(key.hWnd, toggleWhitelistMessage(WM_TIMER), key.nIDEvent, (LPARAM)value.lpTimerFunc);
                    PostMessageInternal(key.hWnd, WM_TIMER, key.nIDEvent, (LPARAM)value.lpTimerFunc, true, NULL, MAF_PASSTHROUGH|MAF_RETURN_OS);
    //#else
    //				  PostMessageA(key.hWnd, toggleWhitelistMessage(WM_TIMER), key.nIDEvent, (LPARAM)value.lpTimerFunc);
    //#endif
                }

                debuglog(LCF_TIMERS, "timer done triggering: 0x%X, 0x%X, %d, 0x%X\n", key.hWnd, key.nIDEvent, value.targetTime, value.lpTimerFunc);
            }
        }
        if(killRequest)
            debuglog(LCF_TIMERS, "timer got kill request. 0x%X, 0x%X, %d, 0x%X\n", key.hWnd, key.nIDEvent, value.targetTime, value.lpTimerFunc);
        delete &key;
        return 0;
    }
    */


    UINT_PTR AddSetTimerTimer(HWND hWnd, UINT_PTR nIDEvent, DWORD uElapse, TIMERPROC lpTimerFunc)
    {
        EnterCriticalSection(&s_pendingSetTimerCS);

        uElapse = std::max<DWORD>(USER_TIMER_MINIMUM, std::min<DWORD>(USER_TIMER_MAXIMUM, uElapse));
        DWORD targetTime = detTimer.GetTicks() + uElapse;

        SetTimerData data = { hWnd, nIDEvent, targetTime, lpTimerFunc, false };

        if (nIDEvent != 0)
        {
            bool found = false;
            for (std::set<SetTimerData, SetTimerDataCompare>::iterator it = s_pendingSetTimers.begin(); it != s_pendingSetTimers.end(); it++)
            {
                // Find the right timer
                if (it->nIDEvent == data.nIDEvent)
                {
                    if (data.hWnd == NULL) // Does it replace the most recent timer with this ID? Or is it some other heriarchy? Assuming first created.
                    {
                        found = true;
                        data.hWnd = it->hWnd;
                        (SetTimerData)(*it) = data;
                        break;
                    }
                    else if (data.hWnd == it->hWnd)
                    {
                        found = true;
                        (SetTimerData)(*it) = data;
                        break;
                    }
                }
            }

            if (found == false) // New timer.
            {
                if (data.hWnd == NULL) // If hWnd is NOT NULL and timer isn't found, then we use the provided ID for the new timer.
                {
                    data.hWnd = gamehwnd; // Fix hWnd like this?
                    data.nIDEvent = CreateNewTimerID();
                }

                s_pendingSetTimers.insert(data);
            }
        }
        else // No ID provided, assuming new timer creation ... TODO: Error if hWnd is not NULL?
        {
            data.nIDEvent = CreateNewTimerID();

            if (data.hWnd == NULL) // NULL hWnds are bad?
                data.hWnd = gamehwnd;

            // Necessary? Seems not, nothing changes here so... Remove out-comment if everything broke
            // bool threadAlreadyExisted = false;

            s_pendingSetTimers.insert(data);
        }

        LeaveCriticalSection(&s_pendingSetTimerCS);

        return data.nIDEvent;
    }

    struct TimerThreadInfo
    {
        UINT delay;
        UINT res;
        UINT event;
        LPTIMECALLBACK callback;
        DWORD_PTR user;
        UINT uid;
        bool killRequest;
        bool dead;
        HANDLE handle;
        int overshot;
        DWORD prevTime;
        TimerThreadInfo* prev;
        TimerThreadInfo* next;
        TimerThreadInfo(UINT uDelay, UINT uResolution, UINT fuEvent, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT uTimerID)
            : delay(uDelay), res(uResolution), event(fuEvent), callback(lpTimeProc), user(dwUser), uid(uTimerID),
            killRequest(false), dead(false), handle(NULL), overshot(0), prevTime(0), prev(NULL), next(NULL)
        {
        }
    } timerHead(0, 0, 0, 0, 0, 0);
    // usually I would use STL, but things are inexplicably fragile here that I'll stick with a simple linked list
    static TimerThreadInfo* ttiHead = &timerHead;
    static TimerThreadInfo* ttiTail = &timerHead;
    static int timerUID = 1;
    static int timerListSize = 0;

    //void PrintTimerResolutionInfo()
    //{
    //	typedef DWORD (NTAPI *ntqueryfunc)(ULONG*, ULONG*, ULONG*); 
    //	ntqueryfunc querytimer = (ntqueryfunc)GetProcAddress(GetModuleHandle("NTDLL.DLL"), "NtQueryTimerResolution");
    //	if(querytimer)
    //	{
    //		ULONG timermin=-1,timermax=-1,timercur=-1;
    //		querytimer(&timermin, &timermax, &timercur);
    //		debugprintf("timermin=%d, timermax=%d, timercur=%d", timermin, timermax, timercur);
    //	}
    //}

    /*
    // basically I have to reimplement the multimedia timer callback manager
    // since I can't find anything else to hook into that changes its behavior
    // MULTI-THREADED APPROACH
    DWORD WINAPI MyTimerThread(LPVOID lpParam)
    {
        VERBOSE_ENTER();
        TimerThreadInfo* info = (TimerThreadInfo*)lpParam;

        int isSetEvent = info->event & TIME_CALLBACK_EVENT_SET;
        int isPulseEvent = info->event & TIME_CALLBACK_EVENT_PULSE;
        int isSynchronous = info->event & TIME_KILL_SYNCHRONOUS;

    #if 0
        DWORD initialTime = timeGetTime();
        int timesTriggered = 0;
    #endif

        int overshot = 0;

        timeBeginPeriod(info->res);

        //PrintTimerResolutionInfo();

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        DWORD prevTime = detTimer.GetTicks();
        while(!info->killRequest)
        {
            DWORD time = detTimer.GetTicks();
            if((int)(time - prevTime) >= (int)info->delay - overshot)
            {
                if(isSynchronous && info->killRequest)
                    continue;
                else if(isSetEvent)
                    SetEvent((HANDLE)info->callback);
                else if(isPulseEvent)
                    PulseEvent((HANDLE)info->callback);
                else
                    info->callback(info->uid, 0, info->user, 0, 0);

                // correct for error that can accumulate as a result of
                // the timer not being particularly high-resolution.
                // if we don't do this, music will tend to play too slowly.
                // NOTE: if this appears to make the music play too quickly,
                // that probably means the DeterministicTimer is advancing too fast.
                // zeroing this out or subtracting from it is not the answer.
                overshot += (int)(time) - (int)(prevTime + info->delay);

    #if 0
                DWORD currentTime = timeGetTime();
                timesTriggered++;
                float ticksPerTrigger = (float)(currentTime - initialTime) / (float)timesTriggered;
                debugprintf("mmtimer delay=%d, triggered=%d, overshot=%d, rate=%g\n", info->delay, timesTriggered, overshot, ticksPerTrigger);
    #endif

                if(!(info->event & TIME_PERIODIC))
                    break;

                prevTime = time;
            }
            Sleep(1);
        }
        timeEndPeriod(info->res);

        info->dead = true;

        return 0;
    }

    HOOKFUNC MMRESULT WINAPI MytimeSetEvent(UINT uDelay, UINT uResolution,
    LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
    {
    //	return timeSetEvent(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
        VERBOSE_ENTER();
        TimerThreadInfo* threadInfo = new TimerThreadInfo(uDelay, uResolution, fuEvent, lpTimeProc, dwUser, 11 * ++timerUID);
        threadInfo->prev = ttiTail;
        ttiTail->next = threadInfo;
        ttiTail = threadInfo;
        timerListSize++;
        threadInfo->handle = MyCreateThread(NULL, 0, MyTimerThread, threadInfo, 0, NULL);
        SetThreadPriority(threadInfo->handle, THREAD_PRIORITY_BELOW_NORMAL);
        if(!threadInfo->handle)
        {
            threadInfo->prev->next = NULL;
            delete threadInfo;
            return NULL;
        }
        verbosedebugprintf(__FUNCTION__ "(0x%X)\n", threadInfo->uid);
        return threadInfo->uid;
    }
    HOOKFUNC MMRESULT WINAPI MytimeKillEvent(UINT uTimerID)
    {
    //	return timeKillEvent(uTimerID);
        VERBOSE_ENTER();

        verbosedebugprintf(__FUNCTION__ "(0x%X)\n", uTimerID);
        TimerThreadInfo* info = ttiHead;
        do{ info = info->next; }
        while( info && info->uid != uTimerID );
        //do{ info = info->next; debugprintf("0x%X\n", info); }
        //while( info && (debugprintf("0x%X != 0x%X?\n",info->uid,uTimerID), info->uid != uTimerID) );
        if(!info)
            return MMSYSERR_INVALPARAM;

        info->killRequest = true;
        while(!info->dead)
        {
            Sleep(1);
        }
        info->prev->next = info->next;
        if(info->next)
            info->next->prev = info->prev;
        if(info == ttiTail)
            ttiTail = info->prev;
        delete info;
        --timerListSize;
        //static int x = 0;
        //x++;
        //	debugprintf(__FUNCTION__ " called %d times\n", x);

        //return timeKillEvent(uTimerID);
        return TIMERR_NOERROR;
    }
    */
    // basically I have to reimplement the multimedia timer callback manager
    // since I can't find anything else to hook into that changes its behavior
    // SINGLE-THREADED APPROACH
    void TickMultiMediaTimers(DWORD time)
    {
        if (!time)
            time = detTimer.GetTicks();

        TimerThreadInfo* info = ttiHead->next;
        while (info)
        {
            if (info->killRequest)
            {
                LOG() << "killing " << info->uid << "...";
                TimerThreadInfo* next = info->next;
                info->prev->next = next;
                if (info->next)
                    info->next->prev = info->prev;
                if (info == ttiTail)
                    ttiTail = info->prev;
                delete info;
                info = next;
            }
            else
            {
                if ((int)(time - info->prevTime) >= (int)info->delay - info->overshot)
                {
                    if (info->event & TIME_CALLBACK_EVENT_SET)
                    {
                        LOG() << "setting " << info->callback;
                        SetEvent((HANDLE)info->callback);
                    }
                    else if (info->event & TIME_CALLBACK_EVENT_PULSE)
                    {
                        LOG() << "pulsing " << info->callback;
                        PulseEvent((HANDLE)info->callback);
                    }
                    else
                    {
                        LOG() << "calling 0x%X\n" << info->callback;
                        info->callback(info->uid, 0, info->user, 0, 0);
                    }

                    // correct for error that can accumulate as a result of
                    // the timer not being particularly high-resolution.
                    // if we don't do this, music will tend to play too slowly.
                    // NOTE: if this appears to make the music play too quickly,
                    // that probably means the DeterministicTimer is advancing too fast.
                    // zeroing this out or subtracting from it is not the answer.
                    info->overshot += (int)(time)-(int)(info->prevTime + info->delay);

#if 0
                    DWORD currentTime = timeGetTime();
                    timesTriggered++;
                    float ticksPerTrigger = (float)(currentTime - initialTime) / (float)timesTriggered;
                    debugprintf("mmtimer delay=%d, triggered=%d, overshot=%d, rate=%g\n", info->delay, timesTriggered, overshot, ticksPerTrigger);
#endif

                    if (!(info->event & TIME_PERIODIC))
                    {
                        LOG() << "one-shot finished, killrequest=true.";
                        info->killRequest = true;
                    }

                    info->prevTime = time;
                }

                info = info->next;
            }
        }
    }
    HOOK_FUNCTION(MMRESULT, WINAPI, timeSetEvent, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent);
    HOOKFUNC MMRESULT WINAPI MytimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
    {
        if (tasflags.timersMode == 0)
        {
            LOG() << "called (and suppressed).";
            return 11 * ++timerUID;
        }
        ENTER(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
        if (tasflags.timersMode == 2)
            return timeSetEvent(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
        TimerThreadInfo* threadInfo = new TimerThreadInfo(uDelay, uResolution, fuEvent, lpTimeProc, dwUser, 11 * ++timerUID);
        threadInfo->prevTime = detTimer.GetTicks();
        threadInfo->prev = ttiTail;
        ttiTail->next = threadInfo;
        ttiTail = threadInfo;
        timerListSize++;
        //threadInfo->handle = MyCreateThread(NULL, 0, MyTimerThread, threadInfo, 0, NULL);
        //SetThreadPriority(threadInfo->handle, THREAD_PRIORITY_BELOW_NORMAL);
        //if(!threadInfo->handle)
        //{
        //	threadInfo->prev->next = NULL;
        //	delete threadInfo;
        //	return NULL;
        //}
        LOG() << "created TimerThreadInfo with uid " << threadInfo->uid;
        return threadInfo->uid;
    }
    static MMRESULT filterKillTimerResult(MMRESULT res)
    {
        if (tasflags.timersMode == 0)
            return TIMERR_NOERROR;
        return res;
    }
    HOOK_FUNCTION(MMRESULT, WINAPI, timeKillEvent, UINT uTimerID);
    HOOKFUNC MMRESULT WINAPI MytimeKillEvent(UINT uTimerID)
    {
        if (tasflags.timersMode == 2)
            return filterKillTimerResult(timeKillEvent(uTimerID));
        ENTER(uTimerID);
        TimerThreadInfo* info = ttiHead;
        do { info = info->next; } while (info && info->uid != uTimerID);
        //do{ info = info->next; debugprintf("0x%X\n", info); }
        //while( info && (debugprintf("0x%X != 0x%X?\n",info->uid,uTimerID), info->uid != uTimerID) );
        if (!info)
            return filterKillTimerResult(MMSYSERR_INVALPARAM);

        info->killRequest = true;
        return TIMERR_NOERROR;
    }


    HOOK_FUNCTION(UINT_PTR, WINAPI, SetTimer, HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
    HOOKFUNC UINT_PTR WINAPI MySetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
    {
        ENTER(hWnd, nIDEvent, uElapse, lpTimerFunc);
        if (tasflags.timersMode == 2)
            return SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
        UINT_PTR rv = AddSetTimerTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, KillTimer, HWND hWnd, UINT_PTR nIDEvent);
    HOOKFUNC BOOL WINAPI MyKillTimer(HWND hWnd, UINT_PTR nIDEvent)
    {
        ENTER(hWnd, nIDEvent);
        if (tasflags.timersMode == 2)
            return KillTimer(hWnd, nIDEvent);

        BOOL rv = FALSE;
        EnterCriticalSection(&s_pendingSetTimerCS);

        for (std::set<SetTimerData, SetTimerDataCompare>::iterator iter = s_pendingSetTimers.begin(); iter != s_pendingSetTimers.end(); iter++)
        {
            if ((iter->hWnd == hWnd) && (iter->nIDEvent == nIDEvent))
            {
                rv = TRUE;
                if (tasflags.threadMode == 2)
                {
                    // Can't modify a value of a set element directly
                    SetTimerData data = *iter;
                    s_pendingSetTimers.erase(iter);
                    data.killRequested = true;
                    s_pendingSetTimers.insert(data);
                }
                else
                {
                    s_pendingSetTimers.erase(iter);
                }
            }
        }

        LeaveCriticalSection(&s_pendingSetTimerCS);

        return rv;
    }

    HOOK_FUNCTION(BOOL, WINAPI, CreateTimerQueueTimer,
        PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback,
        PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags);
    HOOKFUNC BOOL WINAPI MyCreateTimerQueueTimer(PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback,
        PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags)
    {
        ENTER();
        DEBUG_LOG() << "Not yet implemented!";
        BOOL rv = CreateTimerQueueTimer(phNewTimer, TimerQueue, Callback,
            Parameter, DueTime, Period, Flags);
        return rv;
    }

    void TimerDllMainInit()
    {
        InitializeCriticalSection(&s_pendingSetTimerCS);
    }

    void ApplyTimerIntercepts()
    {
        static const InterceptDescriptor intercepts[] =
        {
            MAKE_INTERCEPT(1, WINMM, timeSetEvent),
            MAKE_INTERCEPT(1, WINMM, timeKillEvent),
            MAKE_INTERCEPT(1, USER32, SetTimer),
            MAKE_INTERCEPT(1, USER32, KillTimer),
            MAKE_INTERCEPT(1, KERNEL32, CreateTimerQueueTimer),
            //MAKE_INTERCEPT(1, KERNEL32, CreateTimerQueue ),
        };
        ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
    }
}
