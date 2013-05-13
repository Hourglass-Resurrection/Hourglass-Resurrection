/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef DETTIME_H_INCL
#define DETTIME_H_INCL

#include "tls.h"

enum TimeCallType
{
	TIMETYPE_UNTRACKED=-1,
	TIMETYPE_QUERYPERFCOUNT=0,
	TIMETYPE_NTQUERYSYSTIME,
	TIMETYPE_GETTICKCOUNT,
	TIMETYPE_TIMEGETSYSTIME,
	TIMETYPE_TIMEGETTIME,
	TIMETYPE_SETTIMER,
	TIMETYPE_CRAWLHACK,
	TIMETYPE_NUMTRACKEDTYPES
};

// used to adjust the timers if we switch between them,
// which isn't strictly necessary but it saves the user from
// having to wait for the new timer to catch up to the old one
// (which could take long enough that they'd think it froze)
extern bool lastTimerIsNonDet;
extern int lastSetTickValue;

// a simple timer that directly uses the system timer,
// but with support for being stored in savestates
// and being somewhat affected by fast-forward and frame advance.
// it is not deterministic, but can be a good reference for
// comparing against what the deterministic timer does.
// (i.e. run the game first with this to see what the framerate is.
//  setting the framerate to 0 or (blank) will default to this timer.)
//
// if the game runs differently with this timer, then
// probably this one is doing the more correct behavior,
// especially if the chosen frame rate is wrong.
// but many games will be unrecordable while using this timer.
class NonDeterministicTimer //: public Timer
{
public:
	DWORD GetTicks(TimeCallType type=TIMETYPE_UNTRACKED)
	{
		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", type);
//		verbosedebugprintf(__FUNCTION__ " called.\n");
		if(frameThreadId == 0)
			frameThreadId = GetCurrentThreadId();
		if(frameThreadId == GetCurrentThreadId()) // only one thread can perform this logic
		{
			if(!lastTimerIsNonDet)
			{
				debuglog(LCF_TIMESET|LCF_TIMEGET, __FUNCTION__ ": using other timer's value: %d --> %d\n", ticks, lastSetTickValue);
				ticks = lastSetTickValue; // prevent freezing when switching between timers
				lasttime = timeGetTime();
				lastTimerIsNonDet = true;
			}

			int tc = timeGetTime();
			int delta = tc - lasttime;
			if(tasflags.fastForward) // fast-forward
				delta *= 3; // arbitrary, but higher didn't work well for me with this timer
			int difference = lastExitTime - lastEnterTime;
			if(difference > 50) // this check prevent the normal frame boundary duration from throwing off the timer
			{
				delta -= difference; // this lets the timer slow down for pause and frame advance
				lastEnterTime = lastExitTime; // difference can only be used once per frame boundary
			}
			ticks += delta;
//			debugprintf("%d: ticks += %d\n", __LINE__, delta);
			debuglog(LCF_TIMESET|LCF_FREQUENT, __FUNCTION__ ": ADDED: %d ticks\n", delta);
			lasttime = tc;
			lastSetTickValue = ticks;
		}
		return ticks;
	}
	void ExitFrameBoundary();
	void EnterFrameBoundary(DWORD framesPerSecond);
	void AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace=FALSE)
	{
		debuglog(LCF_SLEEP|LCF_FREQUENT, __FUNCTION__ " called.\n");
		//if(doSleep)
		{
			if(tasflags.fastForward && (tasflags.fastForwardFlags & (isSleep ? FFMODE_SLEEPSKIP : FFMODE_WAITSKIP)))
			{
				//if(tasflags.fastForwardFlags & FFMODE_SWITCHSKIP)
				//	return;
				delayTicks = 0;
			}
			Sleep(delayTicks);
		}
	} 
	void Initialize(DWORD startTicks=0);

	FILETIME GetFileTime()
	{
//		timedebugprintf(__FUNCTION__ " called.\n");
		ULARGE_INTEGER uli;
		uli.QuadPart = (ULONGLONG)GetTicks() * 10000;

		// some games crash if the time is too bogus
		// (e.g. BMD crashes when you press S to take a screenshot),
		// so let's try making it slightly-less-bogus.
		uli.QuadPart |= ULONGLONG(0x100000000000000); // I guess this is January 18th or something?

		FILETIME filetime;
		filetime.dwLowDateTime = uli.LowPart;
		filetime.dwHighDateTime = uli.HighPart;
		return filetime;
	}
	SYSTEMTIME GetSystemTime()
	{
//		timedebugprintf(__FUNCTION__ " called.\n");
		FILETIME fileTime = GetFileTime();
		SYSTEMTIME systemTime;
		FileTimeToSystemTime(&fileTime, &systemTime);
		return systemTime;
	}


private:
	DWORD ticks;
	DWORD lasttime;
	DWORD frameThreadId;
	DWORD lastEnterTime;
	DWORD lastExitTime;
	DWORD lastEnterTicks;
};

extern NonDeterministicTimer nonDetTimer;


#define MAX_NONFRAME_GETTIMES 4000

// hack to avoid adding more parameters through to EnterFrameBoundary
extern bool s_isSleepEnterFB;
extern bool s_isWaitEnterFB;


// a timer that gives deterministic values, at least in the main thread.
// 
// deterministic means that calling FrameBoundary() and querying this timer
// in the same order will produce the same stream of results,
// independently of anything else like the system clock or CPU speed.
//
// the main thread is defined as the last thread that called FrameBoundary(),
// or if that has never been called, the first thread to query this timer. <-- TODO: that could be a problem
// (other threads will get similar values but they may not be deterministic,
// due to the fact that threads run asynchronously from each other.)
//
// the trick to making this timer deterministic and still run at a normal speed is:
// we define a frame rate beforehand and always tell the game it is running that fast
// from frame to frame. Then we do the waiting ourselves for each frame (using system timer).
// this also lets us support "fast forward" without changing the values the game sees.
class DeterministicTimer //: public Timer
{
public:
	DWORD GetTicks(TimeCallType type=TIMETYPE_UNTRACKED)
	{
		//if(!fullyInitialized)
		//{
		//	debugprintf("WARNING: GetTicks called before initialization! ticks=%d\n", ticks);
		//	cmdprintf("SHORTTRACE: 3,120");
		//}

		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", type);
		if(tasflags.framerate <= 0 /*|| (!frameThreadId && getTimes > 4000)*/)
		{
			//debugprintf("A (%d)\n", getTimes);
			DWORD rv = nonDetTimer.GetTicks();
//			debugprintf("OHNOWARNING1!!%d\n", rv);
			//timedebugprintf("Non"__FUNCTION__ " called (%d) (%d) (%d).\n", rv, timeGetTime(), GetTickCount());
			return rv; // 0 framerate means disable deterministic timer
		}

//BOOL tls_IsPrimaryThread();
//		BOOL isFrameThread = tls_IsPrimaryThread(); //tls.isFrameThread;
//		LONG untrustedCaller = tls.callerisuntrusted;

		BOOL isFrameThread;
		LONG untrustedCaller;
		if(ThreadLocalStuff* pCurtls = ThreadLocalStuff::GetIfAllocated())
		{
			isFrameThread = tls_IsPrimaryThread2(pCurtls);
			untrustedCaller = pCurtls->callerisuntrusted;
			if(type != TIMETYPE_UNTRACKED)
				untrustedCaller = !VerifyIsTrustedCaller(!untrustedCaller);
		}
		else
		{
			isFrameThread = FALSE;
			untrustedCaller = TRUE;
		}

		//verbosedebugprintf(__FUNCTION__ " called (%d).\n", ticks);

//		debugprintf("thread = 0x%X, framethread = 0x%X\n", frameThreadId, GetCurrentThreadId());
		//if(frameThreadId == 0)
		//	frameThreadId = GetCurrentThreadId();
		//if(frameThreadId == GetCurrentThreadId() || !frameThreadId)
		if(!isFrameThread)
		{
			if(!untrustedCaller && type != TIMETYPE_UNTRACKED)
			{
				//debugprintf("A %d\n", getTimes);
				if(getTimes >= MAX_NONFRAME_GETTIMES)
				{
					if(getTimes == MAX_NONFRAME_GETTIMES)
						debuglog(LCF_TIMEGET|LCF_TIMESET|LCF_DESYNC, "temporarily assuming main thread\n");
					isFrameThread = true;
				}
				getTimes++;
			}
			else
			{
				//debugprintf("B %d %d %d\n", getTimes, untrustedCaller, type);
				//if(untrustedCaller)
				//	cmdprintf("SHORTTRACE: 3,120");
			}
		}
		//else
		//{
		//	if(getTimes >= MAX_NONFRAME_GETTIMES)
		//		debuglog(LCF_TIMEGET|LCF_TIMESET|LCF_DESYNC, "found main thread %d\n", getTimes);
		//	debugprintf("C %d\n", getTimes);
		//	getTimes = 0;
		//}

//			//if(s_frameThreadId)
//			//{
//			//	static int offset = ticks - timeGetTime();
//			//	return timeGetTime() + offset;
////				return lastEnterTicks + timeGetTime() - lastEnterTime;
//				//return ticks - lastOvershot;
//			//}
//
//			//debugprintf("NOT MAIN THREAD (%d,%d,%d,%d)\n", tls.isFirstThread, tls.createdFirstWindow, tls.isFrameThread, tls.callerisuntrusted);
//			if(/*frameThreadId == 0 &&*/ !untrustedCaller && type != TIMETYPE_UNTRACKED
//				)
//			{
//				if(getTimes == MAX_NONFRAME_GETTIMES)
//				{
//					//debugprintf("WARNING! temporarily switched to non-deterministic timer (%d)\n", ticks);
//					// switch temporarily to non-deterministic timer
//
//					debuglog(LCF_TIMESET|LCF_TIMEGET|LCF_DESYNC, "WARNING! temporarily switched timer to fallback mode (%d)\n", ticks);
//					lastSetTickValue = ticks;
//				}
//				getTimes++;
//				if(getTimes > MAX_NONFRAME_GETTIMES && getTimes % 4 == 0)
//				{
//					ticks++;
////					debugprintf("OHNOWARNING2!!%d\n", ticks);
//					debuglog(LCF_TIMESET|LCF_FREQUENT|LCF_DESYNC, __FUNCTION__ ": ADDED: 1 tick (!)\n");
//				}
//			}
//		}
//		else
		if(isFrameThread)
		{
			// only do this in the main thread so as to not dirty the timer with nondeterministic values
			// (not to mention it would be extremely multithreading-unsafe without using sync primitives otherwise)

			DWORD ticksExtra = 0;

			//if(getTimes > MAX_NONFRAME_GETTIMES && !untrustedCaller)
			//{
			//	getTimes = 0; // prevents some games from entering this block of code twice (changes sync)
			//
			//	debuglog(LCF_TIMESET|LCF_DESYNC, "switched timer to normal mode (%d --> %d)\n", ticks, (ticks - lastSetTickValue <= 3000) ? (lastSetTickValue + 3000) : ticks);
			//
			//	{
			//		// have to update ticks to prevent freezing when switching between timers
			//		if(ticks - lastSetTickValue <= 3000)
			//			ticks = lastSetTickValue + 3000; // leave a window of 3000 ticks where the destination is deterministic (this number affects sync!)
			//		else
			//			debuglog(LCF_TIMEGET|LCF_TIMESET|LCF_DESYNC|LCF_ERROR, "WARNING! fallback mode overran time allotment, may get non-deterministic results\n");
			//	}
			//}
#if 0
//			debugprintf("A\n");
			if(lastTimerIsNonDet)
			{
				// have to update ticks to prevent freezing when switching between timers
				if(lastSetTickValue - ticks < 6000)
					ticks += 6000; // leave a window of 6000 ticks where the destination is deterministic
				else
					ticks = lastSetTickValue;

				debugprintf("WARNING! switched back from non-deterministic timer (%d)\n", ticks);
				lastEnterTime = timeGetTime();
				lastTimerIsNonDet = false;
			}
#endif
//if(0) // FIXME FIXME
			if(/*!disableSelfTicking &&*/ !untrustedCaller)
			{
//				ticks++;
#if 1
//				if(type != TIMETYPE_UNTRACKED && (/*!s_frameThreadId ||*/ isFrameThread && !untrustedCaller))
				if(type != TIMETYPE_UNTRACKED && ((/*(type == TIMETYPE_CRAWLHACK && !s_frameThreadId) ||*/ /*isFrameThread*/1) && !untrustedCaller))
				{
					debuglog(LCF_TIMESET|LCF_FREQUENT, "subticks[%d]++, %d -> %d, / %d\n", type, altGetTimes[type], altGetTimes[type]+1, altGetTimeLimits[type]+1);
//#pragma message("FIXMEEE")
//		cmdprintf("SHORTTRACE: 3,120");

//					debuglog(LCF_TIMERS, "!!! %d (type %d)\n", altGetTimes[type], type);
					altGetTimes[type]++;
					//if(altGetTimes[type] > altGetTimeLimits[type] * (s_frameThreadId ? 100 : 1)
					if(altGetTimes[type] > altGetTimeLimits[type]
					/*&& (GetAsyncKeyState('A') & 0x8000)*/)
					{
						int tickDelta = 1;
						//int tickDelta = 1000/tasflags.framerate;
						//altGetTimes[type] = 0;

						debuglog(LCF_TIMESET|/*LCF_DESYNC|*/LCF_FREQUENT, "WARNING! force-advancing time (%d) (type %d) by %d\n", altGetTimes[type]-altGetTimeLimits[type], type, tickDelta);

//		cmdprintf("SHORTTRACE: 3,120");
						//AddDelay(1,0,1);
//if(type==6) // fixme
//						cmdprintf("SHORTTRACE: 3,120");

						ticksExtra += tickDelta;
						//if(!s_frameThreadId)
						//	debugprintf("OHNOWARNING3!!%d (%d)\n", ticks, tickDelta);
						//forceAdvancedTicks += tickDelta;

						//altGetTimes[type] -= 5; // todo
						for(int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
							altGetTimes[i] = 0;

//						if(s_frameThreadId)
//							altGetTimes[type] -= 10;

						//void ProcessTimers();
						//ProcessTimers();
						
						//if(type == TIMETYPE_QUERYPERFCOUNT)
						//{
						//	ticksExtra+=4;
						//	forceAdvancedTicks+=4;
						//}
					}
				}
#elif 1
				if(getTimes == MAX_NONFRAME_GETTIMES && !frameThreadId)
					debugprintf("WARNING! temporarily switched to non-deterministic timer (%d)\n", ticks);
				// switch temporarily to non-deterministic timer
				getTimes++;
#else
//			debugprintf("C (%d)\n", getTimes);
				if(!(++getTimes & 0xFFF))
				{
//			debugprintf("D (%d)\n", getTimes);
					if(warningcountdown >= 0)
					{
						if(warningcountdown > 0)
							debugprintf("WARNING! getTimes = %d (0x%X)\n", getTimes, getTimes);
						else
							debugprintf("WARNING! GAVE UP! getTimes = %d (0x%X)\n", getTimes, getTimes);
						warningcountdown--;
					}
					if(warningcountdown <= 0)
					{
//			debugprintf("E (%d)\n", ticks);
						ticksExtra++; // do this because games tend to expect time to increase over time (avoids freezing)
						getTimes = 0;
					}
				}
#endif
			}

			if(ticksExtra) // will be 0 if untrustedCaller
				AddDelay(ticksExtra, FALSE, TRUE, TRUE);
			lastSetTickValue = ticks;
		}
//#else
//		// FIXME!!! TEMP, very bad for movie sync
////				if(!(++getTimes & 0x3))
////				{
//						//ticks+=15; // do this because games tend to expect time to increase over time (avoids freezing)
//						//getTimes = 0;
////				}

		

		return ticks;
	}
	void ExitFrameBoundary();
	void EnterFrameBoundary(DWORD framesPerSecond);
	// TODO: some of these arguments (like "replace") are unused now
	void AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace=FALSE)
	{
		// disabled because we should never call this function if callerisuntrusted
//		if(tls.callerisuntrusted)
//			return;

		//if(timed)
		//{
		//	__asm{int 3}
		//}

//#pragma message("FIXMEEE")
		debuglog(LCF_TIMESET|LCF_SLEEP, __FUNCTION__ "(%d, %d, %d, %d) called.\n", delayTicks, isSleep, timed, replace);
		//cmdprintf("SHORTTRACE: 3,120");
		//cmdprintf("SHORTTRACE: 3,10");


//		cmdprintf("SHORTTRACE: 3,5");
		if(tasflags.framerate <= 0) // 0 framerate means disable deterministic timer
			return nonDetTimer.AddDelay(delayTicks, isSleep, timed);


//		if(replace)
//		{
//			//DWORD maxReserve = 1000 / tasflags.framerate;
//			//if(replaceReserveUsed < maxReserve)
//			//{
//			//	replaceReserveUsed += delayTicks;
//			//	if(replaceReserveUsed >= maxReserve)
//			//		delayTicks = replaceReserveUsed - maxReserve;
//			//	else
//			//		delayTicks = 0;
//			//}
//			//forceAdvancedTicks += delayTicks;
//			//lastOvershot += delayTicks;
//			replacedDelay += delayTicks;
//		}
//
//		if(!timed) // sync hack
//		{
//			if(!isSleep) // both FALSE indicates async delay add, so don't actually send any command or do anything but add to a counter otherwise really weird things will happen like inaccurate thread creation reports to the debugger
//				queuedAsyncAddDelay += delayTicks;
//			else
//				SleepAndAccumulateOutput(delayTicks, isSleep);
//			return;
//		}

		// deferring as much of the delay as possible
		// until the place where the regular per-frame delay is applied
		// gives the smoothest results.
		// however, there must be a limit,
		// otherwise it could easily build up and make us freeze (in some games)
		DWORD maxDeferredDelay = lastNewTicks * 6;
////		DWORD maxDeferredDelay = lastNewTicks + 1;

		// deferred delay add
//		if(addedDelay < maxDeferredDelay /*&& !replace*/)
//		{
//			int postponedDelayTicks = delayTicks;
//			if(addedDelay + postponedDelayTicks > maxDeferredDelay)
//				postponedDelayTicks = maxDeferredDelay - addedDelay;
//			delayTicks -= postponedDelayTicks;
//
//			ticks += postponedDelayTicks;
//
////			debugprintf("%d: ticks += %d\n", __LINE__, postponedDelayTicks);
//			timedebugprintf("ADDED: %d ticks\n", postponedDelayTicks);
//			lastEnterTicks += postponedDelayTicks;
//			addedDelay += postponedDelayTicks;
//
//			//// correct for the slight difference from using Sleep(1) instead of one big Sleep
//			//static int extraCounter = 0;
//			//extraCounter++;
//			//postponedDelayTicks += extraCounter%3 ? 1 : 0;
//
//			lastEnterTime += postponedDelayTicks; // defer sleep to next frame boundary
//		}

		// FIXME: TESTING
		// anyway this setup accumulates wait/sleep time toward the frame count which is an important change from previous versions
		addedDelay += delayTicks;
		ticks += delayTicks;
		forceAdvancedTicks += delayTicks;

		if(!tasflags.fastForward || !(tasflags.fastForwardFlags & FFMODE_WAITSKIP))
		{
			// because the caller would have yielded at least a little
			//Sleep(0);
			SwitchToThread();
		}

		while(addedDelay > maxDeferredDelay)
//		while(addedDelay >= lastNewTicks)
//		while(addedDelay > lastNewTicks + 1) // note: the threshold here affects sync. but the specific value is not very important as long as it's >= newTicks. I think (lastNewTicks + 1) is always >= newTicks which is why I chose it.
		{
			s_isSleepEnterFB = !!isSleep;
			s_isWaitEnterFB = !isSleep;

//			int prevtu = ticks;

			// note: this decrements addedDelay by (basically) how much it advances ticks
			FrameBoundary(NULL, CAPTUREINFO_TYPE_PREV);

//			ticksAddedSinceLastFrame += ticks - prevtu;
		}




//		// non-deferred delay add
//		if(delayTicks)
//		{
//			lastEnterTicks += delayTicks;
//			// allow timer-driven events (e.g. music) to keep updating during long sleep sequences
//			// also send the multiple frames for AVI output
//			while(delayTicks > 10)
//			{
//				SleepAndAccumulateOutput(10, isSleep);
//				ticks += 10;
////			debugprintf("%d: ticks += %d\n", __LINE__, 10);
//				timedebugprintf("ADDED: %d ticks\n", 10);
//				delayTicks -= 10;
//				void ProcessTimers();
//				ProcessTimers();
//			}
//			SleepAndAccumulateOutput(delayTicks, isSleep);
//			ticks += delayTicks;
////			debugprintf("%d: ticks += %d\n", __LINE__, delayTicks);
//			timedebugprintf("ADDED: %d ticks\n", delayTicks);
//		}
	}
	DeterministicTimer();
	void InitializeWithoutDependencies(DWORD startTicks);
	void Initialize(DWORD startTicks=0);
	void OnSystemTimerRecalibrated();

	FILETIME GetFileTime()
	{
//		timedebugprintf(__FUNCTION__ " called.\n");
		ULARGE_INTEGER uli;
		uli.QuadPart = (ULONGLONG)GetTicks() * 10000;

		// some games crash if the time is too bogus
		// (e.g. BMD crashes when you press S to take a screenshot),
		// so let's try making it slightly-less-bogus.
		uli.QuadPart |= ULONGLONG(0x100000000000000); // I guess this is January 18th or something?

		FILETIME filetime;
		filetime.dwLowDateTime = uli.LowPart;
		filetime.dwHighDateTime = uli.HighPart;
		return filetime;
	}
	SYSTEMTIME GetSystemTime()
	{
//		timedebugprintf(__FUNCTION__ " called.\n");
		FILETIME fileTime = GetFileTime();
		SYSTEMTIME systemTime;
		FileTimeToSystemTime(&fileTime, &systemTime);
		return systemTime;
	}


	int GetInternalTickCountForDebugging() { return ticks; }
	int GetInternalTickCount2ForDebugging() { return addedDelay; }
	int GetInternalTickCount3ForDebugging() { return lastNewTicks; }

private:
	DWORD ticks;
	DWORD ticksMinorAccum;
	DWORD getTimes;
	DWORD lastEnterTime;
	DWORD lastEnterTicks;
	DWORD lastEnterStartTicks; // TODO: DELETE THIS (unused)
	DWORD lastOvershot;
	DWORD frameThreadId;
	DWORD altGetTimes [TIMETYPE_NUMTRACKEDTYPES]; // limit for each time-getting method before time auto-advances to avoid a freeze
	DWORD altGetTimeLimits [TIMETYPE_NUMTRACKEDTYPES];
	DWORD lastNewTicks;
	DWORD addedDelay;
	DWORD replacedDelay;
	DWORD replaceReserveUsed;
	DWORD nonPostponedAddedDelay;
	DWORD sleepAccumTicks;
	DWORD sleepTicksMinorAccum;
	DWORD totalSleepFrames;
	DWORD ticksAddedSinceLastFrame;
	BOOL lastFrameIsFromDraw;
	DWORD forceAdvancedTicks;
	//DWORD queuedAsyncAddDelay;
	BOOL lastEnterValid;
	int warningcountdown;
	//BOOL fullyInitialized;
};


extern DeterministicTimer detTimer;

#endif
