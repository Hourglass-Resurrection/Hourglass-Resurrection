/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(TIMERHOOKS_INCL) && !defined(UNITY_BUILD)
#define TIMERHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../wintasee.h"
#include "../msgqueue.h"
#include <vector>
#include <map>

void TickMultiMediaTimers(DWORD time=0); // extern? (I mean, move to header)
LRESULT DispatchMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/); // extern? (I mean, move to header)
void PostMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, struct MessageQueue* pmq/*=NULL*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/); // extern? (I mean, move to header)
HOOKFUNC VOID WINAPI MySleep(DWORD dwMilliseconds); // extern? (I mean, move to header)

struct SetTimerKey
{
	HWND hWnd;
	UINT_PTR nIDEvent;
	bool operator < (const SetTimerKey& other) const
	{
		LARGE_INTEGER a = {(DWORD)hWnd, (LONG)nIDEvent};
		LARGE_INTEGER b = {(DWORD)other.hWnd, (LONG)other.nIDEvent};
		return a.QuadPart < b.QuadPart;
	}
};
struct SetTimerValue
{
	DWORD targetTime;
	TIMERPROC lpTimerFunc;
	bool killRequested;
};
static std::map<SetTimerKey,SetTimerValue> s_pendingSetTimers; 
static CRITICAL_SECTION s_pendingSetTimerCS;

//static bool inProcessTimers = false;
void ProcessTimers()
{
//	if(inProcessTimers)
//		return; // disabled, some games need to recurse
//	inProcessTimers = true;

	{
		DWORD time = detTimer.GetTicks();
	//	DWORD earliestTriggerTime = (DWORD)(time + 0x7FFFFFFF);

		std::vector<std::pair<SetTimerKey,SetTimerValue> > triggeredTimers;
	//	bool triedAgain = false;
	//tryAgain:
		EnterCriticalSection(&s_pendingSetTimerCS);
		std::map<SetTimerKey,SetTimerValue>::iterator iter;
		for(iter = s_pendingSetTimers.begin(); iter != s_pendingSetTimers.end();)
		{
			SetTimerValue& value = iter->second;
	////		debugprintf("HOO: %d, %d\n", value.targetTime, time);
	//		if((int)(earliestTriggerTime - value.targetTime) > 0)
	//			earliestTriggerTime = value.targetTime;
			if((int)(time - value.targetTime) >= 0)
			{
				const SetTimerKey& key = iter->first;
				triggeredTimers.push_back(std::make_pair(key,value));
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

		if(!(tasflags.timersMode == 0))
		{
			for(unsigned int i = 0; i < triggeredTimers.size(); i++)
			{
				SetTimerKey& key = triggeredTimers[i].first;
				SetTimerValue& value = triggeredTimers[i].second;
				
				debuglog(LCF_TIMERS, "timer triggered: 0x%X, 0x%X, %d, 0x%X\n", key.hWnd, key.nIDEvent, value.targetTime, value.lpTimerFunc);

				if(value.lpTimerFunc)
					value.lpTimerFunc(key.hWnd, WM_TIMER, key.nIDEvent, time);
				else
				{
					// posting it doesn't work for some reason (iji hangs on startup)
					//PostMessageInternal(key.hWnd, WM_TIMER, key.nIDEvent, (LPARAM)value.lpTimerFunc);
					DispatchMessageInternal(key.hWnd, WM_TIMER, key.nIDEvent, (LPARAM)value.lpTimerFunc, true, MAF_PASSTHROUGH|MAF_RETURN_OS);
				}
			}
		}
		triggeredTimers.clear();
	}

	TickMultiMediaTimers();
	//inProcessTimers = false;
}


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


UINT_PTR AddSetTimerTimer(HWND hWnd, UINT_PTR nIDEvent, DWORD uElapse, TIMERPROC lpTimerFunc)
{
	uElapse = max(10, min(2147483647, uElapse)); // USER_TIMER_MINIMUM, USER_TIMER_MAXIMUM
	DWORD targetTime = detTimer.GetTicks() + uElapse;

	SetTimerKey key = {hWnd, nIDEvent};
	SetTimerValue value = {targetTime, lpTimerFunc, false};

	EnterCriticalSection(&s_pendingSetTimerCS);
	if(!nIDEvent)
	{
		// generate a unique timer ID
		while(s_pendingSetTimers.find(key) != s_pendingSetTimers.end())
			key.nIDEvent++;
	}

	bool threadAlreadyExisted = false;

	s_pendingSetTimers[key] = value;
	LeaveCriticalSection(&s_pendingSetTimerCS);

	return key.nIDEvent;
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
} timerHead(0,0,0,0,0,0);
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
	verbosedebugprintf(__FUNCTION__ " called.\n");
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
	verbosedebugprintf(__FUNCTION__ " called.\n");
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
	verbosedebugprintf(__FUNCTION__ " called.\n");

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
	if(!time)
		time = detTimer.GetTicks();

	TimerThreadInfo* info = ttiHead->next;
	while(info)
	{
		if(info->killRequest)
		{
			debuglog(LCF_TIMERS, __FUNCTION__ ": killing...\n");
			TimerThreadInfo* next = info->next;
			info->prev->next = next;
			if(info->next)
				info->next->prev = info->prev;
			if(info == ttiTail)
				ttiTail = info->prev;
			delete info;
			info = next;
		}
		else
		{
			if((int)(time - info->prevTime) >= (int)info->delay - info->overshot)
			{
				if(info->event & TIME_CALLBACK_EVENT_SET)
				{
					debuglog(LCF_TIMERS, __FUNCTION__ ": setting 0x%X\n", info->callback);
					SetEvent((HANDLE)info->callback);
				}
				else if(info->event & TIME_CALLBACK_EVENT_PULSE)
				{
					debuglog(LCF_TIMERS, __FUNCTION__ ": pulsing 0x%X\n", info->callback);
					PulseEvent((HANDLE)info->callback);
				}
				else
				{
					debuglog(LCF_TIMERS, __FUNCTION__ ": calling 0x%X\n", info->callback);
					info->callback(info->uid, 0, info->user, 0, 0);
				}

				// correct for error that can accumulate as a result of
				// the timer not being particularly high-resolution.
				// if we don't do this, music will tend to play too slowly.
				// NOTE: if this appears to make the music play too quickly,
				// that probably means the DeterministicTimer is advancing too fast.
				// zeroing this out or subtracting from it is not the answer.
				info->overshot += (int)(time) - (int)(info->prevTime + info->delay);

	#if 0
				DWORD currentTime = timeGetTime();
				timesTriggered++;
				float ticksPerTrigger = (float)(currentTime - initialTime) / (float)timesTriggered;
				debugprintf("mmtimer delay=%d, triggered=%d, overshot=%d, rate=%g\n", info->delay, timesTriggered, overshot, ticksPerTrigger);
	#endif

				if(!(info->event & TIME_PERIODIC))
				{
					debuglog(LCF_TIMERS, __FUNCTION__ ": one-shot finished, killrequest=true.\n");
					info->killRequest = true;
				}

				info->prevTime = time;
			}

			info = info->next;
		}
	}
}
HOOKFUNC MMRESULT WINAPI MytimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
	if(tasflags.timersMode == 0)
	{
		debuglog(LCF_TIMERS, __FUNCTION__ " called (and suppressed).\n");
		return 11 * ++timerUID;
	}
	debuglog(LCF_TIMERS, __FUNCTION__ "(%d, %d, 0x%X, 0x%X, 0x%X) called.\n", uDelay, uResolution, (DWORD)lpTimeProc, (DWORD)dwUser, fuEvent);
	if(tasflags.timersMode == 2)
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
	debuglog(LCF_TIMERS, __FUNCTION__ " created TimerThreadInfo with uid 0x%X.\n", threadInfo->uid);
	return threadInfo->uid;
}
static MMRESULT filterKillTimerResult(MMRESULT res)
{
	if(tasflags.timersMode == 0)
		return TIMERR_NOERROR;
	return res;
}
HOOKFUNC MMRESULT WINAPI MytimeKillEvent(UINT uTimerID)
{
	if(tasflags.timersMode == 2)
		return filterKillTimerResult(timeKillEvent(uTimerID));
	debuglog(LCF_TIMERS, __FUNCTION__ "(0x%X) called.\n", uTimerID);
	TimerThreadInfo* info = ttiHead;
	do{ info = info->next; }
	while( info && info->uid != uTimerID );
	//do{ info = info->next; debugprintf("0x%X\n", info); }
	//while( info && (debugprintf("0x%X != 0x%X?\n",info->uid,uTimerID), info->uid != uTimerID) );
	if(!info)
		return filterKillTimerResult(MMSYSERR_INVALPARAM);

	info->killRequest = true;
	return TIMERR_NOERROR;
}


HOOKFUNC UINT_PTR WINAPI MySetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
	debuglog(LCF_TIMERS, __FUNCTION__ "(0x%X, 0x%X, %d, 0x%X) called.\n", hWnd, nIDEvent, uElapse, lpTimerFunc);
	if(tasflags.timersMode == 2)
		return SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
	UINT_PTR rv = AddSetTimerTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
	return rv;
}
HOOKFUNC BOOL WINAPI MyKillTimer(HWND hWnd, UINT_PTR nIDEvent)
{
	debuglog(LCF_TIMERS, __FUNCTION__ "(0x%X, 0x%X) called.\n", hWnd, nIDEvent);
	if(tasflags.timersMode == 2)
		return KillTimer(hWnd, nIDEvent);

	BOOL rv = FALSE;
	SetTimerKey key = {hWnd, nIDEvent};
	EnterCriticalSection(&s_pendingSetTimerCS);
	std::map<SetTimerKey,SetTimerValue>::iterator iter = s_pendingSetTimers.find(key);
	if(iter != s_pendingSetTimers.end())
	{
		rv = TRUE;
		if(tasflags.threadMode == 2)
			s_pendingSetTimers[key].killRequested = true;
		else
			s_pendingSetTimers.erase(key);
	}
	LeaveCriticalSection(&s_pendingSetTimerCS);

	return rv;
}

HOOKFUNC DWORD WINAPI MyQueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData)
{
	DWORD rv = QueueUserAPC(pfnAPC, hThread, dwData);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned %d.\n", rv);
	return rv;
}

HOOKFUNC BOOL WINAPI MyCreateTimerQueueTimer(
	PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback,
	PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags)
{
	debuglog(LCF_TIMERS|LCF_TODO|LCF_DESYNC, __FUNCTION__ " called.\n");
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
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, WINMM, timeSetEvent),
		MAKE_INTERCEPT(1, WINMM, timeKillEvent),
		MAKE_INTERCEPT(1, USER32, SetTimer),
		MAKE_INTERCEPT(1, USER32, KillTimer),
		MAKE_INTERCEPT(1, KERNEL32, CreateTimerQueueTimer),
		//MAKE_INTERCEPT(1, KERNEL32, CreateTimerQueue ),
		MAKE_INTERCEPT(1, KERNEL32, QueueUserAPC),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
