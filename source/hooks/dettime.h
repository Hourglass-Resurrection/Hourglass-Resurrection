/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "tls.h"

enum TimeCallType
{
    TIMETYPE_UNTRACKED = -1,
    TIMETYPE_QUERYPERFCOUNT = 0,
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
    DWORD GetTicks(TimeCallType type = TIMETYPE_UNTRACKED);

    void ExitFrameBoundary();
    void EnterFrameBoundary(DWORD framesPerSecond);
    void AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace = FALSE);
    void Initialize(DWORD startTicks = 0);

    FILETIME GetFileTime();
    SYSTEMTIME GetSystemTime();

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
    DWORD GetTicks(TimeCallType type = TIMETYPE_UNTRACKED);
    void ExitFrameBoundary();
    void EnterFrameBoundary(DWORD framesPerSecond);
    void AddDelay(DWORD delayTicks, BOOL isSleep, BOOL timed, BOOL replace = FALSE);

    DeterministicTimer();
    void InitializeWithoutDependencies(DWORD startTicks);
    void Initialize(DWORD startTicks = 0);
    void OnSystemTimerRecalibrated();

    FILETIME GetFileTime();
    SYSTEMTIME GetSystemTime();

    int GetInternalTickCountForDebugging()
    {
        return ticks;
    }
    int GetInternalTickCount2ForDebugging()
    {
        return addedDelay;
    }
    int GetInternalTickCount3ForDebugging()
    {
        return lastNewTicks;
    }

private:
    DWORD ticks;
    DWORD ticksMinorAccum;
    DWORD getTimes;
    DWORD lastEnterTime;
    DWORD lastEnterTicks;
    DWORD lastEnterStartTicks; // TODO: DELETE THIS (unused)
    DWORD lastOvershot;
    DWORD frameThreadId;
    DWORD altGetTimes
        [TIMETYPE_NUMTRACKEDTYPES]; // limit for each time-getting method before time auto-advances to avoid a freeze
    DWORD altGetTimeLimits[TIMETYPE_NUMTRACKEDTYPES];
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
