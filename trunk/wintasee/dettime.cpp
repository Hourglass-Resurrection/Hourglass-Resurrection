/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(DETTIME_C_INCL) && !defined(UNITY_BUILD)
#define DETTIME_C_INCL

#include "global.h"
#include "wintasee.h"
#include "tls.h"
#include "dettime.h"

bool lastTimerIsNonDet = false;
int lastSetTickValue = 0;
NonDeterministicTimer nonDetTimer;
bool s_isSleepEnterFB = false;
bool s_isWaitEnterFB = false;
DeterministicTimer detTimer;

void AdvanceTimeAndMixAll(DWORD ticks);

// used to adjust the timers if we switch between them,
// which isn't strictly necessary but it saves the user from
// having to wait for the new timer to catch up to the old one
// (which could take long enough that they'd think it froze)
extern bool lastTimerIsNonDet;
extern int lastSetTickValue;

	void NonDeterministicTimer::ExitFrameBoundary()
	{
		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
		//timedebugprintf(__FUNCTION__ " called.\n");
		lastExitTime = timeGetTime();
	}
	void NonDeterministicTimer::EnterFrameBoundary(DWORD framesPerSecond)
	{
		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(%d) called.\n", framesPerSecond);

		//timedebugprintf(__FUNCTION__ " called.\n");
		lastEnterTime = timeGetTime();

		GetTicks();
		AdvanceTimeAndMixAll(ticks - lastEnterTicks);
		lastEnterTicks = ticks;
	}
	void NonDeterministicTimer::Initialize(DWORD startTicks)
	{
		//timedebugprintf(__FUNCTION__ " called.\n");
		ticks = startTicks;
		lastEnterTicks = startTicks;
		lasttime = timeGetTime();
		frameThreadId = 0;
		lastEnterTime = lasttime;
		lastExitTime = lasttime;
	}

	void DeterministicTimer::ExitFrameBoundary()
	{
		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
		for(int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
			altGetTimes[i] = 0;
		//forceAdvancedTicks = 0;
		ticksAddedSinceLastFrame = 0;
		replaceReserveUsed = 0;

		//timedebugprintf(__FUNCTION__ " called.\n");
		if(tasflags.framerate <= 0)
			return nonDetTimer.ExitFrameBoundary(); // 0 framerate means disable deterministic timer

//		disableSelfTicking = 0;

//		debugprintf("thread = 0x%X, framethread = 0x%X\n", frameThreadId, GetCurrentThreadId());
//		debugprintf("getTimes = %d (0x%X)", getTimes, getTimes);
		getTimes = 0;
		if(warningcountdown > 0)
			warningcountdown = 100;

		// FIXME: TESTING
		DWORD addedTicks = lastNewTicks;//(ticks - lastEnterStartTicks);
		if(addedDelay > addedTicks)
			addedDelay -= addedTicks;
		else
			addedDelay = 0;
	}

	void DeterministicTimer::EnterFrameBoundary(DWORD framesPerSecond)
	{
		debuglog(LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(%d) called.\n", framesPerSecond);
		//timedebugprintf(__FUNCTION__ " called.\n");
		if(tasflags.framerate <= 0)
			return nonDetTimer.EnterFrameBoundary(framesPerSecond); // 0 framerate means disable deterministic timer

		lastEnterStartTicks = ticks;

//		disableSelfTicking = 0;

//		debugprintf("framethread 0x%X -> 0x%X\n", frameThreadId, GetCurrentThreadId());
		frameThreadId = GetCurrentThreadId();

		DWORD newTicks = 1000/framesPerSecond;

		// make the increment alternate correctly when
		// 1000 is not evenly divisible by framesPerSecond
		DWORD remainder = 1000%framesPerSecond;
		ticksMinorAccum += remainder;
		int ticksOver = ticksMinorAccum/framesPerSecond;
		if(ticksOver > 0)
		{
			int ticksContrib = ticksOver;
			newTicks += ticksContrib;
			ticksMinorAccum -= ticksContrib*framesPerSecond;
		}

		lastNewTicks = newTicks;

//		debugprintf("%d\n",newTicks);


//		// subtract out ticks that were made when calling GetTicks()
//		DWORD takenTicks = (ticks - lastEnterTicks) + ((s_isSleepEnterFB || s_isWaitEnterFB) ? 0 : ticksAddedSinceLastFrame) /*+ forceAdvancedTicks*/;
//		DWORD takenTicks = (ticks - lastEnterTicks) /*+ forceAdvancedTicks*/;
		DWORD takenTicks = (ticks - lastEnterTicks) + forceAdvancedTicks;
//		DWORD takenTicks = (ticks - lastEnterTicks) + ((s_isSleepEnterFB || s_isWaitEnterFB) ? 0 : forceAdvancedTicks); // TODO: use this version after all, with a smaller sleep increment that adds more directly to ticks
 
int getCurrentFramestampLogical();
//		if(s_isSleepEnterFB || s_isWaitEnterFB)
//			debugprintf("f %d, sleep %d ticks\n", getCurrentFramestampLogical(), max(0, (int)newTicks - (int)takenTicks));
//		else
//			debugprintf("f %d, frame %d ticks\n", getCurrentFramestampLogical(), max(0, (int)newTicks - (int)takenTicks));
		if(!(s_isSleepEnterFB || s_isWaitEnterFB))
			forceAdvancedTicks = 0;
		//if(takenTicks >= newTicks && newTicks > 0) // FIXME TEMP TESTING
		//	takenTicks = newTicks - 1; // FIXME TEMP TESTING
		if(newTicks > takenTicks)
		{
			ticks += newTicks - takenTicks;
//			debugprintf("%d: ticks += %d\n", __LINE__, newTicks - takenTicks);
//			/*time*/debugprintf("ADDED: %d ticks (%d - %d)\n", newTicks - takenTicks, newTicks, takenTicks);
			debuglog(LCF_TIMESET/*|LCF_FREQUENT*/, __FUNCTION__ ": ADDED: %d ticks (%d - %d)\n", newTicks - takenTicks, newTicks, takenTicks);
//		cmdprintf("SHORTTRACE: 3,6");
		}

		// get the current actual time
		DWORD time = timeGetTime();

		// calculate the target time we wanted to be at now
		DWORD timeScale = tasflags.timescale;
		DWORD timeScaleDiv = tasflags.timescaleDivisor;
		int desiredDeltaTime = ((int)(newTicks - lastOvershot)) * (int)timeScaleDiv;
		if(timeScale > 1)
			desiredDeltaTime /= (int)timeScale;
		DWORD desiredTime = lastEnterTime + desiredDeltaTime;
//MAIN: (f=453, t=21173) WARNING: 1.43166e+006 second wait... time=71729, desiredTime=1431727455, timeScaleDiv=4, timeScale=3, lastEnterTime=71691, newTicks=33, lastOvershot=34

		// safety net disabled because it could lead to movies that lie about their time (e.g. years pass by in a few minutes of watching) (note that there's aleady a warning below too)
		//// safety net: don't wait more than 1 full second per frame
		//if(!disableSelfTicking)
		//{
		//	if((int)(desiredTime - time) > 1000)
		//	{
		//		debugprintf("WARNING: %g second wait capped at 1 second.\n", (float)(int)(desiredTime - time)/1000.0f, 1000);
		//		desiredTime = time + 1000;
		//	}
		//}

//		debugprintf("time = %d, desired = %d, diff = %d\n", time, desiredTime, desiredTime-time);

		// add postponed delay frames (if any extra ones) to AVI
		//if(addedDelay > newTicks)
		//	SleepAndAccumulateOutput(addedDelay - newTicks, FALSE);

		// FIXME TESTING (this block was enabled, is disabled for testing)
		//if(replacedDelay)
		//{
		//	if(replacedDelay > newTicks)
		//		replacedDelay = newTicks;
		//	if(addedDelay > replacedDelay)
		//		addedDelay -= replacedDelay;
		//	else
		//		addedDelay = 0;
		//}
		//if(addedDelay)
		//	SleepAndAccumulateOutput(addedDelay, FALSE);
		//addedDelay = 0;
		//replacedDelay = 0;

		// must happen after any calls to SleepAndAccumulateOutput in this function,
		// or AVIs will get skipping audio in games that sleep
		AdvanceTimeAndMixAll(newTicks);

		BOOL skip = tasflags.fastForward;
		if(skip)
		{
			if(s_isSleepEnterFB && !(tasflags.fastForwardFlags & FFMODE_SLEEPSKIP))
				skip = false;
			if(s_isWaitEnterFB && !(tasflags.fastForwardFlags & FFMODE_WAITSKIP))
				skip = false;
		}
		BOOL thisFrameIsFromDraw = !(s_isSleepEnterFB || s_isWaitEnterFB);
// FIXME
		//if(thisFrameIsFromDraw && !lastFrameIsFromDraw)
		//	skip = true;
		lastFrameIsFromDraw = thisFrameIsFromDraw;
		s_isSleepEnterFB = false;
		s_isWaitEnterFB = false;

		if(!skip && lastEnterValid) // if not fast-forwarding and not the first frame
		{
			if((int)(desiredTime - time) > 1000)
			{
				debugprintf("WARNING: %g second wait... time=%d, desiredTime=%d, desiredDeltaTime=%d, timeScaleDiv=%d, timeScale=%d, lastEnterTime=%d, newTicks=%d, lastOvershot=%d\n", (float)(int)(desiredTime - time)/1000.0f, time, desiredTime, desiredDeltaTime, timeScaleDiv, timeScale, lastEnterTime, newTicks, lastOvershot);
				//debugprintf("desiredTime = %d, time = %d\n", desiredTime, time);
				//debugprintf("DWORD %d = %d + (%d - %d)*%d/%d\n", desiredTime,lastEnterTime,newTicks,lastOvershot,tasflags.timescaleDivisor,tasflags.timescale);
				//cmdprintf("SHORTTRACE: 3,50");
			}

			// wait for the amount of time it should have taken (THROTTLE)
			while((int)(desiredTime - time) > 0)
			{
				Sleep(1);
				time = timeGetTime();
				if(tasflags.fastForward || timeScaleDiv != tasflags.timescaleDivisor)
					break;
			}
		}

		//if(queuedAsyncAddDelay)
		//{
		//	SleepAndAccumulateOutput(queuedAsyncAddDelay, FALSE);
		//	queuedAsyncAddDelay = 0;
		//}

		// if we accidentally waited too long,
		// remember that so we can compensate for it the next time through.
		lastOvershot = timeGetTime() - desiredTime;
		if((int)lastOvershot < 0) lastOvershot = 0;
		if((int)lastOvershot > newTicks) lastOvershot = newTicks;
//		debugprintf("overshot: %d\n", lastOvershot);

//		debugprintf("time, newticks, taken = %d, %d, %d\n", time, newTicks, takenTicks);

		lastEnterTime = time; // TODO: can't remember if it's intentional that this is affected by the throttle loop
		lastEnterTicks = ticks;
		lastEnterValid = TRUE;
	}
//	void SleepAndAccumulateOutput(DWORD sleepTicks, BOOL doSleep) // sync hack, usually not used (except when waitSyncMode is 2)
//	{
//		timedebugprintf(__FUNCTION__ " (%d, %d) called.", sleepTicks, doSleep);
//	
//		// add sleep time
//		if(doSleep && !tasflags.fastForward)
//			Sleep(sleepTicks);
//		sleepAccumTicks += sleepTicks;
//	
//		while(true)
//		{
//			// calculate threshold
//			int framesPerSecond = tasflags.framerate;
//			DWORD sleepFrameTickThreshold = 1000/framesPerSecond;
//			DWORD remainder = 1000%framesPerSecond;
//			int ticksOver = sleepTicksMinorAccum/framesPerSecond;
//			if(ticksOver < 0) ticksOver = 0;
//			sleepFrameTickThreshold += ticksOver;
//	
//			// if sleep time exceeds threshold, output a sleep frame (AVI)
//			if((int)sleepAccumTicks >= (int)sleepFrameTickThreshold)
//			{
//				//FrameBoundary(NULL, CAPTUREINFO_TYPE_PREV);
//				timedebugprintf("%d >= %d, sleeping\n", sleepAccumTicks, sleepFrameTickThreshold);
//				totalSleepFrames++;
//				AdvanceTimeAndMixAll(sleepFrameTickThreshold);
//				cmdprintf("SLEEPFRAME: %d", totalSleepFrames);
//				g_videoFramesPrepared++;
//				//debugprintf(__FUNCTION__ ": g_soundMixedTicks=%d, g_videoFramesPrepared=%d, ratio=%g\n", g_soundMixedTicks, g_videoFramesPrepared, (float)g_soundMixedTicks/g_videoFramesPrepared);
//				sleepAccumTicks -= sleepFrameTickThreshold;
//				sleepTicksMinorAccum += remainder - ticksOver*framesPerSecond;
//			}
//			else
//			{
//				break;
//			}
//		}
//	}

	DeterministicTimer::DeterministicTimer()
	{
		InitializeWithoutDependencies(/*tasflags.initialTime ? tasflags.initialTime :*/ 6000);
	}
	void DeterministicTimer::InitializeWithoutDependencies(DWORD startTicks)
	{
//		fullyInitialized = false;
		getTimes = 0;
		ticks = startTicks;
		ticksMinorAccum = 0;
		lastOvershot = 0;
		lastEnterTime = /*timeGetTime()*/0;
		lastEnterTicks = ticks;
		lastEnterStartTicks = ticks;
		frameThreadId = 0;
		for(int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
			altGetTimes[i] = 0;
		for(int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
			altGetTimeLimits[i] = 10;
		altGetTimeLimits[TIMETYPE_GETTICKCOUNT] = 150;
		altGetTimeLimits[TIMETYPE_CRAWLHACK] = 5;
		//altGetTimeLimits[TIMETYPE_QUERYPERFCOUNT] = 50;
		forceAdvancedTicks = 0;
		ticksAddedSinceLastFrame = 0;
		warningcountdown = 100;
		lastNewTicks = 0;
		addedDelay = 0;
		replacedDelay = 0;
		lastEnterValid = 0;
		sleepAccumTicks = 0;
		sleepTicksMinorAccum = 0;
		totalSleepFrames = 0;
		//queuedAsyncAddDelay = 0;
		replaceReserveUsed = 0;
//		disableSelfTicking = 0;
		lastFrameIsFromDraw = 1;
	}
	void DeterministicTimer::Initialize(DWORD startTicks)
	{
		timedebugprintf(__FUNCTION__ " called.\n");
		InitializeWithoutDependencies(startTicks);
		lastEnterTime = timeGetTime();
		OnSystemTimerRecalibrated(); // in case
//		fullyInitialized = true;
//		debugprintf("thread = 0x%X, framethread = 0x%X\n", frameThreadId, GetCurrentThreadId());
	}
	void DeterministicTimer::OnSystemTimerRecalibrated()
	{
		// hack, _KiUserCallbackDispatcher > _LdrpCallInitRoutine > _DllProcessAttach > _InitDevices > _TimeInit > _CalibrateTimer (winmm) calls GetTickCount...
		// so the system time can actually jump around before all loaded dlls have been initialized
		// and opengl's dll initialization actually does things we could interpret as rendering a frame...
		// if lastEnterTime got set before the system time jumped, we're hosed, so let's invalidate it
		lastEnterValid = FALSE;
		// also TODO: hook time source maybe used by perfect cherry blossom: google for: KUSER_SHARED_DATA 0x7ffe0000 time
		// see "getting-os-information-the-kuser_shared_data-structure/"
		// maybe should use that (InterruptTime) if we want to get the real time, instead of calling timeGetTime.
		// then we wouldn't have recalibration issues.
		// but we haven't to worry about what happens if the game tries to do that too.
	}

#endif
