/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(WAITHOOKS_INCL) && !defined(UNITY_BUILD)
#define WAITHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../tls.h"
#include "../wintasee.h"
#include <map>

void ThreadHandleToExitHandle(HANDLE& pHandle);

HOOKFUNC VOID WINAPI MySleep(DWORD dwMilliseconds)
{
	BOOL isFrameThread = tls_IsPrimaryThread();//tls.isFrameThread;

	debuglog(LCF_SLEEP|((dwMilliseconds && isFrameThread)?LCF_NONE:LCF_FREQUENT), __FUNCTION__"(%d) called.\n", dwMilliseconds);

	//if(dwMilliseconds && s_frameThreadId == GetCurrentThreadId()) // main thread waiting?
	if(dwMilliseconds && isFrameThread)
	{
		// add a delay for framerate adjustment and to take large sleeps into account.
//		// we ignore <5 millisecond sleeps here because they can accumulate in our timer
//		// to make the game run at a lower framerate than it naturally does.
//		if(dwMilliseconds >= 5)
		if(VerifyIsTrustedCaller(!tls.callerisuntrusted))
			detTimer.AddDelay(dwMilliseconds, TRUE, TRUE);

		dwMilliseconds = 0;

		//verbosedebugprintf("DENIED: transferred to timer\n");
		debuglog(LCF_SLEEP, "sleep transferred to timer.\n");
		return;
	}

	//if(dwMilliseconds>0 && tasflags.fastForward) // fast-forward?
	//{
	//	dwMilliseconds = 0;
	//
	//	verbosedebugprintf("DENIED because of fast-forward\n");
	//}

	{
		Sleep(dwMilliseconds);
	}
}

HOOKFUNC VOID WINAPI MySleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
	debuglog(LCF_SLEEP|(dwMilliseconds?LCF_NONE:LCF_FREQUENT), __FUNCTION__"(%d, %d) called.\n", dwMilliseconds, bAlertable);

	BOOL isFrameThread = tls_IsPrimaryThread();//tls.isFrameThread;

	//if(dwMilliseconds && s_frameThreadId == GetCurrentThreadId()) // main thread waiting?
	if(dwMilliseconds && isFrameThread && !(bAlertable || !VerifyIsTrustedCaller(!tls.callerisuntrusted)))
	{
		// add a delay for framerate adjustment and to take large sleeps into account.
//		// we ignore <5 millisecond sleeps here because they can accumulate in our timer
//		// to make the game run at a lower framerate than it naturally does.
//		if(dwMilliseconds >= 5)
			detTimer.AddDelay(dwMilliseconds, TRUE, TRUE);

		dwMilliseconds = 0;

		//verbosedebugprintf("DENIED: transferred to timer\n");
		return;
	}

	//if(dwMilliseconds>0 && tasflags.fastForward) // fast-forward?
	//{
	//	dwMilliseconds = 0;
	//
	//	verbosedebugprintf("DENIED because of fast-forward\n");
	//}


	// not needed anymore now that joypad functions like joyGetPosEx are replaced.
	// might become a problem again later in which case this can be re-enabled.
	//// hack for rotategear intro in wrapped thread mode
	//// TODO (look into): happens because the joypad? thread sleeps for 60 seconds (why?)
	//// and the bug is that somehow that blocks the main thread for that full duration
	//if(!isFrameThread && dwMilliseconds > 1000 && framecount < 30)
	//{
	//	dwMilliseconds -= ((dwMilliseconds - 1000) / 32) * 31;
	//}


	{
		//debugprintf("sleepex: %d\n", dwMilliseconds);
		SleepEx(dwMilliseconds, bAlertable);
	}
}

void TransferWait(DWORD& dwMilliseconds/*, bool rly=true*/)
{
#if 0
	if(tls.isFrameThread) // only apply this to the main thread
	{
		if(dwMilliseconds < 500 && tasflags.fastForward)
			dwMilliseconds = 0;
	}
	return;
#endif
	// basically, we have to make it so we're in control of the waits, not the game.
	// otherwise the game will run too slowly or (at best) won't be fast-forward capable.

	// don't enter a waiting state if the timeout is infinite or greater than some amount
	// (the amount chosen might affect sync) FIXME: amounts 500 or higher cause random desyncs... should clamp down better on what can call WaitForSingleObject/WaitForMultipleObjects
	if(dwMilliseconds != INFINITE && dwMilliseconds < 100)
	{
		// the only sync-safe thing we can do is assume the full time-out time always elapses.
		// luckily, most games that trigger this in the main thread will
		// pass in a time-out time that exactly corresponds with their desired framerate.
		if(dwMilliseconds > 0)
		{
		//	if(tls.isFrameThread && !tls.callerisuntrusted) // only apply this to the main thread
			if(tls_IsPrimaryThread() && VerifyIsTrustedCaller(!tls.callerisuntrusted)) // only apply this to the main thread
			{
				// add the delay logically but don't physically wait yet
//				if(rly){
				//detTimer.AddDelay(dwMilliseconds, FALSE, tasflags.waitSyncMode<2, TRUE); // attempted sync hack
				detTimer.AddDelay(dwMilliseconds, FALSE, TRUE, TRUE); // FIXME: might desync?
				//detTimer.AddDelay(dwMilliseconds, FALSE, FALSE, TRUE); // FIXME... sync hack
//				}
				// if in fast-forward mode, nullify the physical wait (if any remains)
//				if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP))
					dwMilliseconds = 0;
				// because of this, we have to be careful not to accidentally call this function twice in a row.
				// for example, if we hook both WaitForSingleObject and WaitForSingleObjectEx,
				// and WaitForSingleObject sometimes calls WaitForSingleObjectEx,
				// that would make fast-forward cause desyncs.
			}
		}
	}
}


HOOKFUNC DWORD WINAPI MyWaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable)
{
	debuglog(LCF_WAIT|LCF_FREQUENT|LCF_TODO, __FUNCTION__ "(%d, %d)\n", dwMilliseconds, bAlertable);

	{
//		TransferWait(dwMilliseconds);
//		return WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);

//		TransferWait(dwMilliseconds);
		DWORD rv;
		do{
			rv = WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);
		} while(rv == WAIT_TIMEOUT);
		return rv;


		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}

//	TransferWait(dwMilliseconds);
//	return WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);
	// careful not to do anything (not even a printf) after the wait function...
	// many games will crash from a race condition if we don't return as fast as possible.
}
HOOKFUNC DWORD WINAPI MyWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	debuglog(LCF_WAIT|LCF_FREQUENT/*|LCF_DESYNC*/, __FUNCTION__ "(%d) for 0x%X\n", dwMilliseconds, hHandle);

	{
		if(tasflags.threadMode == 1)
			ThreadHandleToExitHandle(hHandle);


		DWORD prevMs = dwMilliseconds;
		TransferWait(dwMilliseconds);
		if(!tls_IsPrimaryThread() || tasflags.waitSyncMode == 2
		//	|| (tasflags.waitSyncMode <= 1 && prevMs == 0) // hack for Rosenkreuzstilette Freudenstachel
		)
		{
			if(tasflags.fastForward && dwMilliseconds > 1 && dwMilliseconds < 100 && (tasflags.fastForwardFlags & FFMODE_SLEEPSKIP)) // hack: check sleepskip instead of waitskip because waitskip is intended for riskier wait skips
				dwMilliseconds = 1; // some games have a framerate regulator non-main thread that prevents fast-forward from working unless we do this. note that using 0 instead of 1 would give far worse results.
			return WaitForSingleObject(hHandle, dwMilliseconds);
		}
		//else if(tasflags.waitSyncMode <= 1 && prevMs == 0) // hack for Rosenkreuzstilette Freudenstachel
		//{
		//	return WaitForSingleObject(hHandle, 10);
		//}
		else if(tasflags.waitSyncMode == 0)
		{
			return WaitForSingleObject(hHandle, INFINITE);
		}
		else if(tasflags.threadMode == 0)
		{
			// for megamari with multithreading disabled
			DWORD rv = WaitForSingleObject(hHandle, 0);
			rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
			return rv;
		}
		else
		{
			// TODO: games with "loading threads" (such as Rosenkreuzstilette Freudenstachel)
			// have trouble with this case

			if((DWORD)dwMilliseconds < 100)
			{
				if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP) && dwMilliseconds)
					dwMilliseconds = 1;
				DWORD rv = WaitForSingleObject(hHandle, dwMilliseconds);
				//if(rv == WAIT_TIMEOUT)
				{
					rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
				}
				//if(prevMs == 0 && rv == WAIT_OBJECT_0)
				//{
				//	static int alt = 0;
				//	alt++;
				//	if(alt&1)
				//		rv = WAIT_TIMEOUT; // hack so Exit Fate can run with waitSyncMode == 1 ... but it makes Iji blackscreen?
				//}
				// careful, switching the rv from WAIT_OBJECT_0 to WAIT_TIMEOUT (even only sometimes) would break break iji startup
				return rv;
			}
			DWORD maxWaitTime = 1000; // TODO: make this number configurable in the UI and hopefully used in some other places if appropriate
			if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP))
				maxWaitTime = 1; // could cause problems but this option should be off by default

			DWORD rv = WaitForSingleObject(hHandle, maxWaitTime);
			if(rv == WAIT_TIMEOUT)
			{
				if(maxWaitTime)
					debuglog(LCF_WAIT|LCF_DESYNC|LCF_ERROR, __FUNCTION__": interrupted wait for 0x%X in case of deadlock.", hHandle);
			}
			rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
			return rv;
		}

//		DWORD rv = WAIT_TIMEOUT;
//		while(rv == WAIT_TIMEOUT)
//		{
//			dwMilliseconds = 1;
//			TransferWait(dwMilliseconds);
//			rv = WaitForSingleObject(hHandle, dwMilliseconds);
//		}
//		return rv;
		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}
//	return MyWaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}


HOOKFUNC DWORD WINAPI MyWaitForMultipleObjectsEx(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable)
{
	debuglog(LCF_WAIT|LCF_FREQUENT|LCF_TODO, __FUNCTION__ "(%d, %d)\n", dwMilliseconds, bAlertable);

	{
	//	TransferWait(dwMilliseconds);
//		return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);

//		TransferWait(dwMilliseconds);
		DWORD rv;
		//do{
			rv = WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);
		//} while(rv == WAIT_TIMEOUT);
		return rv;


		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}
//	TransferWait(dwMilliseconds);
//	return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);
	// careful not to do anything (not even a printf) after the wait function...
	// many games will crash from a race condition if we don't return as fast as possible.
}
HOOKFUNC DWORD WINAPI MyWaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
	debuglog(LCF_WAIT|LCF_FREQUENT/*|LCF_DESYNC*/, __FUNCTION__ "(%d, count=%d, flag=%d)\n", dwMilliseconds, nCount, bWaitAll);
	for(int i = 0; i < (int)nCount; i++)
		debuglog(LCF_WAIT|LCF_FREQUENT/*|LCF_DESYNC*/, "  0x%X\n", lpHandles[i]);


	{
/*
		// TESTING
		// FIXME: this should work, but msctf.dll calls WaitForMultipleObjects at unpredictable times somehow, causing desyncs. (maybe filesystem or registry related? or multithreading...)
//		TransferWait(dwMilliseconds);
//		cmdprintf("SHORTTRACE: 3,120");
//		return WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);

		TransferWait(dwMilliseconds);
//		DWORD rv;
//		do{
//			rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
//		} while(rv == WAIT_TIMEOUT);
//		return rv;

		DWORD rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
		if(rv == WAIT_TIMEOUT && tasflags.threadMode == 1 && tls_IsPrimaryThread()) {
			rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, INFINITE);
		}
		return rv;
*/


		DWORD prevMs = dwMilliseconds;
		TransferWait(dwMilliseconds);
		if(!tls_IsPrimaryThread() || tasflags.waitSyncMode == 2)
		{
			if(tasflags.fastForward && dwMilliseconds > 1 && dwMilliseconds < 5000 && (tasflags.fastForwardFlags & FFMODE_SLEEPSKIP)) // hack: check sleepskip instead of waitskip because waitskip is intended for riskier wait skips
				dwMilliseconds = 1;
		}
		else if(tasflags.waitSyncMode == 0)
		{
			dwMilliseconds = INFINITE;
		}
		else if(tasflags.threadMode == 0)
		{
			// for eversion with multithreading disabled
			dwMilliseconds = 0;
		}
		else if(!tls.callerisuntrusted) // hack: if caller is untrusted already, it would have more trouble causing desync than otherwise, so in that case let it wait naturally to avoid frequent deadlocks in quartz.dll at Iji loading screens
		{
			//if(dwMilliseconds == 0)
			//{
			//	debugprintf("starting infinite wait\n");
			//	cmdprintf("SHORTTRACE: 3,50");
			//}
			// for iji sync... if this causes freezes in other games, experiment with using the following commented-out branch (that says it breaks iji startup) instead
			dwMilliseconds = INFINITE;
		}

		// hack for Lyle in Cube Sector in wrapped thread mode
		// wait for thread -> wait for event
		// avoids deadlock when a thread waits for a wrapped thread to exit
		// (actually this isn't that hacky, except it should be done to all WaitFor* cases instead of only this case)
		if(tasflags.threadMode == 1 && (int)nCount > 0)
		{
			HANDLE* handles = (HANDLE*)_alloca(nCount*sizeof(HANDLE));
			for(DWORD i = 0; i < nCount; i++)
			{
				HANDLE pHandle = lpHandles[i];
				ThreadHandleToExitHandle(pHandle);
				handles[i] = pHandle;
			}
		
//			if(dwMilliseconds != INFINITE)
			{
				DWORD rv = WaitForMultipleObjects(nCount, handles, bWaitAll, dwMilliseconds);
//	debuglog(LCF_WAIT|LCF_FREQUENT/*|LCF_DESYNC*/, __FUNCTION__ " done waiting 2 (0x%X).\n", rv);
				return rv;
			}
			//else
			//{
			//	DWORD rv;
			//	MSG msg;
			//	tls.callerisuntrusted++;
			//	do {
			//		PeekMessageA(&msg, gamehwnd, 0, 0, PM_NOREMOVE);
			//		rv = WaitForMultipleObjects(nCount, handles, bWaitAll, 10);
			//	} while(rv == WAIT_TIMEOUT);
			//	tls.callerisuntrusted--;
			//	return rv;
			//}
		}


		DWORD rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
//	debuglog(LCF_WAIT|LCF_FREQUENT/*|LCF_DESYNC*/, __FUNCTION__ " done waiting (0x%X).\n", rv);
		return rv;


		//else // breaks iji startup
		//{
		//	if((DWORD)dwMilliseconds < 500)
		//	{
		//		DWORD rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
		//		//if(rv == WAIT_TIMEOUT)
		//		//{
		//		//	rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
		//		//}
		//		{
		//			static int alt = 0;
		//			alt++;
		//			if((alt&1) || prevMs > 1)
		//				rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
		//			else
		//				rv = WAIT_TIMEOUT;
		//		}
		//		return rv;
		//	}
		//	//DWORD maxWaitTime = 5000; // TODO: make this number configurable in the UI and hopefully used in some other places if appropriate
		//	//if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP))
		//	//	maxWaitTime /= 10;
		//	//DWORD rv = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, maxWaitTime);
		//	//if(rv == WAIT_TIMEOUT)
		//	//{
		//	//	debuglog(LCF_WAIT|LCF_DESYNC|LCF_ERROR, __FUNCTION__": interrupted wait for 0x%X in case of deadlock.", lpHandles);
		//	//	rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
		//	//}
		//	//return rv;
		//	return WaitForMultipleObjects(nCount, lpHandles, bWaitAll, INFINITE);
		//}



		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}
//	return MyWaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, FALSE);
}


HOOKFUNC DWORD WINAPI MySignalObjectAndWait(HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable)
{
	debuglog(LCF_WAIT|LCF_SYNCOBJ|LCF_FREQUENT|LCF_DESYNC, __FUNCTION__ "(%d, %d)\n", dwMilliseconds, bAlertable);

	{
	//	TransferWait(dwMilliseconds);
//		return SignalObjectAndWait(hObjectToSignal, hObjectToWaitOn, dwMilliseconds, bAlertable);

		TransferWait(dwMilliseconds);
		DWORD rv = SignalObjectAndWait(hObjectToSignal, hObjectToWaitOn, dwMilliseconds, bAlertable);
		if(rv == WAIT_TIMEOUT && tasflags.waitSyncMode < 2 && tls_IsPrimaryThread()) {
			if(tasflags.waitSyncMode == 0)
				rv = WaitForSingleObjectEx(hObjectToWaitOn, INFINITE, 0);
			else
				rv = WAIT_OBJECT_0;
		}
		return rv;

		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}
}



HOOKFUNC DWORD WINAPI MyMsgWaitForMultipleObjects(DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask)
{
	debuglog(LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(%d, 0x%X)\n", dwMilliseconds, dwWakeMask);


	{
		// FIXME super desync prone especially in Iji (enemy randomness).
		// this function (MyMsgWaitForMultipleObjects) and some of its siblings
		// were getting called an unpredictable number of times in the main thread,
		// which is a problem because some games use this as their main way to pass time,
		// and any slight nondeterminism in incrementing the timer can easily cause desyncs.
		// it's fixed for now here and elsewhere by upgrading most timeouts to INFINITE,
		// and in cases where that wasn't possible, pretending that it didn't time out even if it did
		// in order to give it a consistent return value no matter what the system timing is.
		// also, by removing the ability to get interrupted by message queue contents since I don't fully control that yet.
		//
		// but the way this was done probably causes some games to simply freeze
		// (waiting forever for something)
		// so more thought on this is probably necessary.
		//
		// ok, I think it's fixed now. need to clean this stuff up...

//DWORD rv = MyWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds); // desyncs too... not anymore, but
//return rv;
		//return MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
		
		DWORD prevMs = dwMilliseconds;

		TransferWait(dwMilliseconds);

		HANDLE* handles = (HANDLE*)_alloca(nCount*sizeof(HANDLE));
		if(tasflags.threadMode == 1 && (int)nCount > 0)
		{
			// try to convert thread handles to thread exit event handles
			for(DWORD i = 0; i < nCount; i++)
			{
				HANDLE pHandle = pHandles[i];
				ThreadHandleToExitHandle(pHandle);
				handles[i] = pHandle;
			}
			pHandles = handles;
		}

		if(tasflags.waitSyncMode == 2 /*|| (!tls_IsPrimaryThread())*/)
		{
			return MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
		}
		else if(tasflags.waitSyncMode == 1 && (dwWakeMask & QS_ALLINPUT) != 0 && dwMilliseconds >= 500 && tls_IsPrimaryThread())
		{
			// hack to fix freeze in Temporal
			HRESULT rv = WaitForMultipleObjects(nCount, pHandles, bWaitAll, /*500*/0);
			if(rv == WAIT_TIMEOUT)
				rv = WAIT_OBJECT_0 + nCount;
			return rv;
		}
		else if(!tls_IsPrimaryThread())
		{
			HRESULT rv;
			// not sure
			//return MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
			dwWakeMask &= ~(QS_PAINT|QS_MOUSE|QS_KEY|QS_HOTKEY|/*QS_RAWINPUT*/0x0400);
			rv = MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
			//// hack to fix freeze in Garden of Coloured Lights
			//// problem: breaks music in Perfect Cherry Blossom if the wait is too small, doesn't work well in garden of coloured lights if too large
			//if(dwWakeMask & (QS_MOUSE|QS_KEY|QS_HOTKEY|/*QS_RAWINPUT*/0x0400))
			//{
			//	dwMilliseconds = 500;
			//	rv = WaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds);
			//	if(rv == WAIT_TIMEOUT)
			//		rv = WAIT_OBJECT_0 + nCount;
			//}
			//else
			//	rv = MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
			return rv;
		}
		else if(tasflags.waitSyncMode == 0)
		{
			return WaitForMultipleObjects(nCount, pHandles, bWaitAll, INFINITE);
		}
		else
		{
			//if((DWORD)dwMilliseconds < 100)
			{
				if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP))
					dwMilliseconds = 0;
				DWORD maxWaitTime = 1000; // TODO: make this number configurable in the UI and hopefully used in some other places if appropriate
				if(dwMilliseconds > maxWaitTime)
					dwMilliseconds = maxWaitTime;
#ifdef EMULATE_MESSAGE_QUEUES
#pragma message("FIXMEEE") // should probably rewrite this function to use MessageQueue somehow
#endif
				//DWORD rv = WaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds);
				DWORD rv = MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
//				if(rv == WAIT_TIMEOUT)
				{
					// this is sync-critical code, but the variable is automatically part of savestates, so it's OK.
					// however, changing this code at all will break any existing movies for any games that call this function.
					static int alt = 0;
					alt++;
					if((alt&1) || prevMs > 1)
					{
						//if(rv == WAIT_TIMEOUT)
						//	if(!(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP)))
						//		MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, 100, dwWakeMask); // in case
						rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
						// rotategear has no input if it never gets WAIT_OBJECT_0
					}
					else
					{
						//rv = WAIT_TIMEOUT; // some games (tumiki fighters) need short waits to time out sometimes
						rv = WAIT_OBJECT_0 + nCount;
					}
				}
				return rv;
			}
			//DWORD maxWaitTime = 1000; // TODO: make this number configurable in the UI and hopefully used in some other places if appropriate
			//if(tasflags.fastForward && (tasflags.fastForwardFlags & FFMODE_WAITSKIP))
			//	maxWaitTime = 0;
			//DWORD rv = WaitForMultipleObjects(nCount, pHandles, bWaitAll, maxWaitTime);
			//if(rv == WAIT_TIMEOUT)
			//{
			//	if(maxWaitTime)
			//		debuglog(LCF_WAIT|LCF_DESYNC|LCF_ERROR, __FUNCTION__": interrupted wait for 0x%X in case of deadlock.", pHandles);
			//	//rv = WAIT_OBJECT_0; // "might crash", but that's better than "probably desync"
			//	rv = WAIT_OBJECT_0 + nCount;
			//}
			//return rv;
		}


		//QS_ALLPOSTMESSAGE
		//debugprintf("MsgWaitForMultipleObjects. (nCount = 0x%X, pHandles = 0x%X, bWaitAll = 0x%X, dwMilliseconds = 0x%X, dwWakeMask = 0x%X)\n", nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
//return 0; // doesn't desync, but no music
//		TransferWait(dwMilliseconds);

/*
		TransferWait(dwMilliseconds);
		DWORD rv;
		do{
			//rv = MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
			rv = WaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds);
		} while(rv == WAIT_TIMEOUT);
		return rv;
*/

//		return MsgWaitForMultipleObjects(nCount, pHandles, bWaitAll, dwMilliseconds, dwWakeMask);
		// careful not to do anything (not even a printf) after the wait function...
		// many games will crash from a race condition if we don't return as fast as possible.
	}
}

// Ex versions disabled as a precaution, see the note in TransferWait about how hooking both might cause desyncs.
HOOKFUNC DWORD WINAPI MyMsgWaitForMultipleObjectsEx(DWORD nCount, const HANDLE *pHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags)
{
	debuglog(LCF_WAIT|LCF_FREQUENT|LCF_TODO, __FUNCTION__ "(%d, 0x%X)\n", dwMilliseconds, dwWakeMask);
//	TransferWait(dwMilliseconds);
	return MsgWaitForMultipleObjectsEx(nCount, pHandles, dwMilliseconds, dwWakeMask, dwFlags);
	// careful not to do anything (not even a printf) after the wait function...
	// many games will crash from a race condition if we don't return as fast as possible.
}

HOOKFUNC NTSTATUS NTAPI MyNtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
	DWORD dwMilliseconds = Timeout ? Timeout->LowPart : ~0;
	debuglog(LCF_WAIT|LCF_FREQUENT|LCF_DESYNC, __FUNCTION__ "(%d)\n", dwMilliseconds);

	{
		return NtWaitForSingleObject(Handle, Alertable, Timeout);
	}
}
HOOKFUNC NTSTATUS NTAPI MyNtWaitForMultipleObjects(ULONG ObjectCount, PHANDLE ObjectsArray, DWORD WaitType, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
	DWORD dwMilliseconds = Timeout ? Timeout->LowPart : ~0;
	debuglog(LCF_WAIT|LCF_FREQUENT|LCF_DESYNC, __FUNCTION__ "(%d)\n", dwMilliseconds);

	{
		return NtWaitForMultipleObjects(ObjectCount, ObjectsArray, WaitType, Alertable, Timeout);
	}
}

HOOKFUNC BOOL NTAPI MyRtlTryEnterCriticalSection(RTL_CRITICAL_SECTION* crit)
{
	debuglog(LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(0x%x) called.", (DWORD)crit);
	return RtlTryEnterCriticalSection(crit);
}

HOOKFUNC NTSTATUS NTAPI MyRtlEnterCriticalSection(RTL_CRITICAL_SECTION *crit)
{

	NTSTATUS rv = RtlEnterCriticalSection(crit);

	return rv;
}

HOOKFUNC BOOL WINAPI MyWaitMessage()
{
	debuglog(LCF_MESSAGES, __FUNCTION__ " called.\n");
#ifdef EMULATE_MESSAGE_QUEUES
	bool firstWait = true;
	MessageQueue& mq = tls.messageQueue;
	while(true)
	{
		InternalizeMessageQueue();
		if(mq.queueStatus & (~mq.queueStatusAtLastGet) & QS_ALLEVENTS)
			break;
//		Sleep(5); // TEMP (should probably wait on a custom event instead ... or not)
		if(firstWait)
		{
			firstWait = false;
			debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " started waiting.\n");
		}
	
		// hack
		if(tasflags.messageSyncMode != 0)
			detTimer.GetTicks(TIMETYPE_CRAWLHACK); // potentially desync prone (but some games will need it) ... moving it here (on no-result) helped sync a bit though... and the problem that happens here is usually caused by GetMessageActionFlags being incomplete
	}
	if(!firstWait)
		debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " finished waiting.\n");
	TrackMessageQueueStatusChange(mq, WM_NULL, 0);
	return TRUE;
#else
	//return WaitMessage();
	return TRUE; // I'm not sure what else I can do that's deterministic... (TODO: if the windows message queue is emulated then this can be made more accurate)
#endif
}

void ApplyWaitIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, KERNEL32, Sleep),
		MAKE_INTERCEPT(1, KERNEL32, SleepEx),
		MAKE_INTERCEPT(1, KERNEL32, WaitForSingleObject),
		MAKE_INTERCEPT(0, KERNEL32, WaitForSingleObjectEx),
		MAKE_INTERCEPT(1, KERNEL32, WaitForMultipleObjects),
		MAKE_INTERCEPT(0, KERNEL32, WaitForMultipleObjectsEx),
		MAKE_INTERCEPT(1, KERNEL32, SignalObjectAndWait),
		MAKE_INTERCEPT(1, USER32, MsgWaitForMultipleObjects),
		MAKE_INTERCEPT(0, USER32, MsgWaitForMultipleObjectsEx),
		MAKE_INTERCEPT(0, NTDLL, NtWaitForSingleObject),
		MAKE_INTERCEPT(0, NTDLL, NtWaitForMultipleObjects),
		MAKE_INTERCEPT(/*1*/0, NTDLL, RtlEnterCriticalSection),
		MAKE_INTERCEPT(0, NTDLL, RtlTryEnterCriticalSection),
		MAKE_INTERCEPT(1, USER32, WaitMessage),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
