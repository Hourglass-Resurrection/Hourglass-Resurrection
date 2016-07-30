/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "wintasee.h"
#include "tls.h"
#include "dettime.h"

using Log = DebugLog<LogCategory::DETTIMER>;

bool lastTimerIsNonDet = false;
int lastSetTickValue = 0;
NonDeterministicTimer nonDetTimer;
bool s_isSleepEnterFB = false;
bool s_isWaitEnterFB = false;
DeterministicTimer detTimer;

// used to adjust the timers if we switch between them,
// which isn't strictly necessary but it saves the user from
// having to wait for the new timer to catch up to the old one
// (which could take long enough that they'd think it froze)
extern bool lastTimerIsNonDet;
extern int lastSetTickValue;

DWORD NonDeterministicTimer::GetTicks(TimeCallType type)
{
    ENTER(type);
    //		verbosedebugprintf(__FUNCTION__ " called.\n");
    if (frameThreadId == 0)
        frameThreadId = GetCurrentThreadId();
    if (frameThreadId == GetCurrentThreadId()) // only one thread can perform this logic
    {
        if (!lastTimerIsNonDet)
        {
            LOG() << ": using other timer's value: " << ticks << " --> " << lastSetTickValue;
            ticks = lastSetTickValue; // prevent freezing when switching between timers
            lasttime = Hooks::timeGetTime();
            lastTimerIsNonDet = true;
        }

        int tc = Hooks::timeGetTime();
        int delta = tc - lasttime;
        if (tasflags.fastForward) // fast-forward
            delta *= 3; // arbitrary, but higher didn't work well for me with this timer
        int difference = lastExitTime - lastEnterTime;
        if (difference > 50) // this check prevent the normal frame boundary duration from throwing off the timer
        {
            delta -= difference; // this lets the timer slow down for pause and frame advance
            lastEnterTime = lastExitTime; // difference can only be used once per frame boundary
        }
        ticks += delta;
        //			debugprintf("%d: ticks += %d\n", __LINE__, delta);
        LOG() << ": ADDED: "<< delta << " ticks\n";
        lasttime = tc;
        lastSetTickValue = ticks;
    }
    return ticks;
}

void NonDeterministicTimer::ExitFrameBoundary()
{
    ENTER();
	lastExitTime = Hooks::timeGetTime();
}
void NonDeterministicTimer::EnterFrameBoundary(DWORD framesPerSecond)
{
	ENTER(framesPerSecond);

	lastEnterTime = Hooks::timeGetTime();

	GetTicks();
	Hooks::AdvanceTimeAndMixAll(ticks - lastEnterTicks);
	lastEnterTicks = ticks;
}

void NonDeterministicTimer::AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace)
{
    ENTER();
    //if(doSleep)
    {
        if (tasflags.fastForward && (tasflags.fastForwardFlags & (isSleep ? FFMODE_SLEEPSKIP : FFMODE_WAITSKIP)))
        {
            //if(tasflags.fastForwardFlags & FFMODE_SWITCHSKIP)
            //	return;
            delayTicks = 0;
        }
        Sleep(delayTicks);
    }
}


void NonDeterministicTimer::Initialize(DWORD startTicks)
{
	ENTER(startTicks);
	ticks = startTicks;
	lastEnterTicks = startTicks;
	lasttime = Hooks::timeGetTime();
	frameThreadId = 0;
	lastEnterTime = lasttime;
	lastExitTime = lasttime;
}

FILETIME NonDeterministicTimer::GetFileTime()
{
    ENTER();
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

SYSTEMTIME NonDeterministicTimer::GetSystemTime()
{
    ENTER();
    FILETIME fileTime = GetFileTime();
    SYSTEMTIME systemTime;
    FileTimeToSystemTime(&fileTime, &systemTime);
    return systemTime;
}



DWORD DeterministicTimer::GetTicks(TimeCallType type)
{
    //if(!fullyInitialized)
    //{
    //	debugprintf("WARNING: GetTicks called before initialization! ticks=%d\n", ticks);
    //	cmdprintf("SHORTTRACE: 3,120");
    //}

    ENTER(type);
    if (tasflags.framerate <= 0 /*|| (!frameThreadId && getTimes > 4000)*/)
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
    if (ThreadLocalStuff* pCurtls = ThreadLocalStuff::GetIfAllocated())
    {
        isFrameThread = tls_IsPrimaryThread2(pCurtls);
        untrustedCaller = pCurtls->callerisuntrusted;
        if (type != TIMETYPE_UNTRACKED)
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
    if (!isFrameThread)
    {
        if (!untrustedCaller && type != TIMETYPE_UNTRACKED)
        {
            //debugprintf("A %d\n", getTimes);
            if (getTimes >= MAX_NONFRAME_GETTIMES)
            {
                if (getTimes == MAX_NONFRAME_GETTIMES)
                    LOG() << "temporarily assuming main thread";
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
    if (isFrameThread)
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
        if (lastTimerIsNonDet)
        {
            // have to update ticks to prevent freezing when switching between timers
            if (lastSetTickValue - ticks < 6000)
                ticks += 6000; // leave a window of 6000 ticks where the destination is deterministic
            else
                ticks = lastSetTickValue;

            debugprintf("WARNING! switched back from non-deterministic timer (%d)\n", ticks);
            lastEnterTime = timeGetTime();
            lastTimerIsNonDet = false;
        }
#endif
        //if(0) // FIXME FIXME
        if (/*!disableSelfTicking &&*/ !untrustedCaller)
        {
            //				ticks++;
#if 1
            //				if(type != TIMETYPE_UNTRACKED && (/*!s_frameThreadId ||*/ isFrameThread && !untrustedCaller))
            if (type != TIMETYPE_UNTRACKED && ((/*(type == TIMETYPE_CRAWLHACK && !s_frameThreadId) ||*/ /*isFrameThread*/1) && !untrustedCaller))
            {
                LOG() << "subticks[" << type << "]++, " << altGetTimes[type] << " -> "
                      << altGetTimes[type] + 1 << ", / " << altGetTimeLimits[type] + 1;
                //#pragma message("FIXMEEE")
                //		cmdprintf("SHORTTRACE: 3,120");

                //					debuglog(LCF_TIMERS, "!!! %d (type %d)\n", altGetTimes[type], type);
                altGetTimes[type]++;
                //if(altGetTimes[type] > altGetTimeLimits[type] * (s_frameThreadId ? 100 : 1)
                if (altGetTimes[type] > altGetTimeLimits[type]
                    /*&& (GetAsyncKeyState('A') & 0x8000)*/)
                {
                    int tickDelta = 1;
                    //int tickDelta = 1000/tasflags.framerate;
                    //altGetTimes[type] = 0;

                    LOG() << "WARNING! force-advancing time (" << altGetTimes[type] - altGetTimeLimits[type]
                          << ") (type " << type << ") by " << tickDelta;

                    //		cmdprintf("SHORTTRACE: 3,120");
                    //AddDelay(1,0,1);
                    //if(type==6) // fixme
                    //						cmdprintf("SHORTTRACE: 3,120");

                    ticksExtra += tickDelta;
                    //if(!s_frameThreadId)
                    //	debugprintf("OHNOWARNING3!!%d (%d)\n", ticks, tickDelta);
                    //forceAdvancedTicks += tickDelta;

                    //altGetTimes[type] -= 5; // todo
                    for (int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
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
            if (getTimes == MAX_NONFRAME_GETTIMES && !frameThreadId)
                debugprintf("WARNING! temporarily switched to non-deterministic timer (%d)\n", ticks);
            // switch temporarily to non-deterministic timer
            getTimes++;
#else
            //			debugprintf("C (%d)\n", getTimes);
            if (!(++getTimes & 0xFFF))
            {
                //			debugprintf("D (%d)\n", getTimes);
                if (warningcountdown >= 0)
                {
                    if (warningcountdown > 0)
                        debugprintf("WARNING! getTimes = %d (0x%X)\n", getTimes, getTimes);
                    else
                        debugprintf("WARNING! GAVE UP! getTimes = %d (0x%X)\n", getTimes, getTimes);
                    warningcountdown--;
                }
                if (warningcountdown <= 0)
                {
                    //			debugprintf("E (%d)\n", ticks);
                    ticksExtra++; // do this because games tend to expect time to increase over time (avoids freezing)
                    getTimes = 0;
                }
            }
#endif
        }

        if (ticksExtra) // will be 0 if untrustedCaller
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


void DeterministicTimer::ExitFrameBoundary()
{
    ENTER();
	for(int i = 0; i < TIMETYPE_NUMTRACKEDTYPES; i++)
		altGetTimes[i] = 0;
	//forceAdvancedTicks = 0;
	ticksAddedSinceLastFrame = 0;
	replaceReserveUsed = 0;

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
	ENTER(framesPerSecond);
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
		LOG() << ": ADDED: " << newTicks - takenTicks << " ticks ("
              << newTicks << " - " << takenTicks << ")";
//		cmdprintf("SHORTTRACE: 3,6");
	}

	// get the current actual time
	DWORD time = Hooks::timeGetTime();

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
	Hooks::AdvanceTimeAndMixAll(newTicks);

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
            DEBUG_LOG() << "WARNING: " << (float)(int)(desiredTime - time) / 1000.0f << "second wait... time=" << time
                        << " desiredTime=" << desiredTime << " desiredDeltaTime=" << desiredDeltaTime
                        << " timeScaleDiv=" << timeScaleDiv << " timeScale=" << timeScale
                        << " lastEnterTime=" << lastEnterTime << " newTicks=" << newTicks
                        << " lastOvershot=" << lastOvershot;
			//debugprintf("desiredTime = %d, time = %d\n", desiredTime, time);
			//debugprintf("DWORD %d = %d + (%d - %d)*%d/%d\n", desiredTime,lastEnterTime,newTicks,lastOvershot,tasflags.timescaleDivisor,tasflags.timescale);
			//cmdprintf("SHORTTRACE: 3,50");
		}

		// wait for the amount of time it should have taken (THROTTLE)
		while((int)(desiredTime - time) > 0)
		{
			Sleep(1);
			time = Hooks::timeGetTime();
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
	lastOvershot = Hooks::timeGetTime() - desiredTime;
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
//		TIME_ENTER(sleepTicks, doSleep);
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

// TODO: some of these arguments (like "replace") are unused now
void DeterministicTimer::AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace)
{
    // disabled because we should never call this function if callerisuntrusted
    //		if(tls.callerisuntrusted)
    //			return;

    //if(timed)
    //{
    //	__asm{int 3}
    //}

    //#pragma message("FIXMEEE")
    ENTER(delayTicks, isSleep, timed, replace);
    //cmdprintf("SHORTTRACE: 3,120");
    //cmdprintf("SHORTTRACE: 3,10");


    //		cmdprintf("SHORTTRACE: 3,5");
    if (tasflags.framerate <= 0) // 0 framerate means disable deterministic timer
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

    if (!tasflags.fastForward || !(tasflags.fastForwardFlags & FFMODE_WAITSKIP))
    {
        // because the caller would have yielded at least a little
        //Sleep(0);
        SwitchToThread();
    }

    while (addedDelay > maxDeferredDelay)
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
	nonPostponedAddedDelay = 0;
//		disableSelfTicking = 0;
	lastFrameIsFromDraw = 1;
}
void DeterministicTimer::Initialize(DWORD startTicks)
{
	ENTER();
	InitializeWithoutDependencies(startTicks);
	lastEnterTime = Hooks::timeGetTime();
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

FILETIME DeterministicTimer::GetFileTime()
{
    ENTER();
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
SYSTEMTIME DeterministicTimer::GetSystemTime()
{
    ENTER();
    FILETIME fileTime = GetFileTime();
    SYSTEMTIME systemTime;
    FileTimeToSystemTime(&fileTime, &systemTime);
    return systemTime;
}