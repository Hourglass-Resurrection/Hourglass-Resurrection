/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(TIMEHOOKS_INCL) && !defined(UNITY_BUILD)
#define TIMEHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../wintasee.h"
#include "../tls.h"

void AdvanceTimeAndMixAll(DWORD ticks); // extern
HOOKFUNC MMRESULT WINAPI MytimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent); // ectern
HOOKFUNC MMRESULT WINAPI MytimeKillEvent(UINT uTimerID); // extern




static int s_tlsRecursionDetector = 0;

int getCurrentThreadstamp()
{
	BOOL isFrameThread = false;
	if(!s_tlsRecursionDetector) // we can (maybe) get called as a result of accessing tls
	{
		s_tlsRecursionDetector++;
		isFrameThread = tls.isFrameThread;
		s_tlsRecursionDetector--;
		if(s_tlsRecursionDetector < 0) s_tlsRecursionDetector = 0;
	}
	if(isFrameThread)
		return 0;
	return GetCurrentThreadId();
}
int getCurrentFramestamp() { return framecount; }
int getCurrentFramestampLogical();
int getCurrentTimestamp() { return detTimer.GetInternalTickCountForDebugging(); }
int getCurrentTimestamp2() { return detTimer.GetInternalTickCount2ForDebugging(); }
int getCurrentTimestamp3() { return detTimer.GetInternalTickCount3ForDebugging(); }


DEFINE_LOCAL_GUID(IID_IReferenceClock,0x56a86897,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);

//template<typename IDirectMusicN> struct IDirectMusicTraits {};
//template<> struct IDirectMusicTraits<IDirectMusic>  { typedef IDirectMusic  DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); };
//template<> struct IDirectMusicTraits<IDirectMusic2> { typedef IDirectMusic2 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); };
//template<> struct IDirectMusicTraits<IDirectMusic3> { typedef IDirectMusic3 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); };
//template<> struct IDirectMusicTraits<IDirectMusic4> { typedef IDirectMusic4 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef LPRECT UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC2, LPVOID); };
//template<> struct IDirectMusicTraits<IDirectMusic7> { typedef IDirectMusic7 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef LPRECT UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2, LPVOID); };

//struct MyReferenceClock : public IReferenceClock
//{
//	MyReferenceClock()
//	{
//		ddrawdebugprintf(__FUNCTION__ " called.\n");
//	}
//	~MyReferenceClock()
//	{
//		ddrawdebugprintf(__FUNCTION__ " called.\n");
//	}
//
//	/*** IUnknown methods ***/
//    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
//	{
//		ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
//		HRESULT rv = m_dd->QueryInterface(riid, ppvObj);
//		if(SUCCEEDED(rv))
//			HookCOMInterface(riid, ppvObj);
//		return rv;
//	}
//
//    ULONG STDMETHODCALLTYPE AddRef()
//	{
//		ddrawdebugprintf(__FUNCTION__ " called.\n");
//		return m_dd->AddRef();
//	}
//
//    ULONG STDMETHODCALLTYPE Release()
//	{
//		ddrawdebugprintf(__FUNCTION__ " called.\n");
//		ULONG count = m_dd->Release();
//		if(0 == count)
//			delete this;
//
//		return count;
//	}
//
//    /*** IReferenceClock methods ***/
//
//};

struct MyReferenceClock : IReferenceClock
{
	static BOOL Hook(IReferenceClock* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IReferenceClock, GetTime);
		rv |= VTHOOKFUNC(IReferenceClock, AdviseTime);
		rv |= VTHOOKFUNC(IReferenceClock, AdvisePeriodic);
		rv |= VTHOOKFUNC(IReferenceClock, Unadvise);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IReferenceClock* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IReferenceClock* pThis, REFIID riid, void** ppvObj)
	{
		verbosedebugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	inline static LONGLONG GetTimeInternal()
	{
		return (LONGLONG)((ULONGLONG)(detTimer.GetTicks() /*+ 10*/) * 10000);
	}

	static HRESULT(STDMETHODCALLTYPE *GetTime)              (IReferenceClock* pThis, REFERENCE_TIME *pTime);
	static HRESULT STDMETHODCALLTYPE MyGetTime              (IReferenceClock* pThis, REFERENCE_TIME *pTime)
	{
		//return GetTime(pThis, pTime);
		if(!pTime)
			return E_POINTER;
		*pTime = GetTimeInternal();
		debuglog(LCF_TIMERS|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called (%d).\n", (DWORD)*pTime);
		return S_OK;
	}

	static void CALLBACK AdvisePeriodicCallback (UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		HANDLE hSemaphore = (HANDLE)dwUser;
		UINT startTime = *((UINT*)hSemaphore);
		if((LONGLONG)startTime * 10000 - GetTimeInternal() < 0)
			return; // not yet
		ReleaseSemaphore(hSemaphore, 1, NULL);
	}

    static HRESULT(STDMETHODCALLTYPE *AdviseTime)           (IReferenceClock* pThis, REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime, HANDLE hEvent, LPDWORD pdwAdviseCookie);
    static HRESULT STDMETHODCALLTYPE MyAdviseTime           (IReferenceClock* pThis, REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime, HANDLE hEvent, LPDWORD pdwAdviseCookie)
	{
		debuglog(LCF_TIMERS, __FUNCTION__ "(rtBaseTime=%d, rtStreamTime=%d) called.\n", (DWORD)(rtBaseTime/10000), (DWORD)(rtStreamTime/10000));
		//return AdviseTime(pThis, rtBaseTime, rtStreamTime, hEvent, pdwAdviseCookie);
		REFERENCE_TIME time = rtBaseTime + rtStreamTime;
//		if((ULONGLONG)time >= 86400000L)
//			return E_INVALIDARG;
		if(!pdwAdviseCookie)
			return E_POINTER;
		time -= GetTimeInternal();
		time /= 10000;
		*pdwAdviseCookie = (DWORD)MytimeSetEvent((UINT)time, 0, (LPTIMECALLBACK)hEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
		if(!*pdwAdviseCookie)
			return E_OUTOFMEMORY;
		return S_OK;
	}

    static HRESULT(STDMETHODCALLTYPE *AdvisePeriodic)       (IReferenceClock* pThis, REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime, HANDLE hSemaphore, LPDWORD pdwAdviseCookie);
    static HRESULT STDMETHODCALLTYPE MyAdvisePeriodic       (IReferenceClock* pThis, REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime, HANDLE hSemaphore, LPDWORD pdwAdviseCookie)
	{
		debuglog(LCF_TIMERS|LCF_DESYNC|LCF_UNTESTED, __FUNCTION__ "(rtStartTime=%d, rtPeriodTime=%d) called.\n", (DWORD)(rtStartTime/10000), (DWORD)(rtPeriodTime/10000));
		return AdvisePeriodic(pThis, rtStartTime, rtPeriodTime, hSemaphore, pdwAdviseCookie);
		// following is NYI (or at least, not tested so it's probably broken)
//		if((ULONGLONG)rtStartTime >= 86400000L)
//			return E_INVALIDARG;
		if(!pdwAdviseCookie || !hSemaphore)
			return E_POINTER;
		rtStartTime -= GetTimeInternal();
		rtStartTime /= 10000;
		rtPeriodTime /= 10000;
		*((UINT*)hSemaphore) = (UINT)rtStartTime; // kind of evil... stuff the start time in the "int unused" field of the HANDLE struct, since I can't dynamically allocate the data since I don't know what to hook to clean up when Unadvice doesn't get called.
		*pdwAdviseCookie = (DWORD)MytimeSetEvent((UINT)rtPeriodTime, 0, (LPTIMECALLBACK)AdvisePeriodicCallback, (DWORD_PTR)hSemaphore, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
		if(!*pdwAdviseCookie)
			return E_OUTOFMEMORY;
		return S_OK;
	}
    static HRESULT(STDMETHODCALLTYPE *Unadvise)             (IReferenceClock* pThis, DWORD dwAdviseCookie);
    static HRESULT STDMETHODCALLTYPE MyUnadvise             (IReferenceClock* pThis, DWORD dwAdviseCookie)
	{
		debuglog(LCF_TIMERS, __FUNCTION__ " called.\n");
		//return Unadvise(pThis, dwAdviseCookie);
		if(TIMERR_NOERROR == MytimeKillEvent((UINT)dwAdviseCookie))
			return S_OK;
		//return S_FALSE;
		return Unadvise(pThis, dwAdviseCookie); // because AdvisePeriodic still does passthrough
	}
};

#define DEF(x) HRESULT (STDMETHODCALLTYPE* MyReferenceClock::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyReferenceClock::GetTime)(x* pThis, REFERENCE_TIME *pTime) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyReferenceClock::AdviseTime)(x* pThis, REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime, HANDLE hEvent, LPDWORD pdwAdviseCookie) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyReferenceClock::AdvisePeriodic)(x* pThis, REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime, HANDLE hSemaphore, LPDWORD pdwAdviseCookie) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyReferenceClock::Unadvise)(x* pThis, DWORD dwAdviseCookie) = 0;
	DEF(IReferenceClock)
#undef DEF


//#include "../external/strmif.h"

//const char* riidToName(REFIID riid);





//struct MyAMOpenProgress : IAMOpenProgress
//{
//	static BOOL Hook(IAMOpenProgress* obj)
//	{
//		BOOL rv = FALSE;
//		rv |= VTHOOKFUNC(IAMOpenProgress, QueryProgress);
//
//		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IAMOpenProgress* pThis, REFIID riid, void** ppvObj);
//	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IAMOpenProgress* pThis, REFIID riid, void** ppvObj)
//	{
//		debugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
//		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
//		if(SUCCEEDED(rv))
//			HookCOMInterface(riid, ppvObj);
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *QueryProgress)             (IAMOpenProgress* pThis, LONGLONG *pllTotal,LONGLONG *pllCurrent);
//	static HRESULT STDMETHODCALLTYPE MyQueryProgress             (IAMOpenProgress* pThis, LONGLONG *pllTotal,LONGLONG *pllCurrent)
//	{
//		HRESULT rv = QueryProgress(pThis, pllTotal, pllCurrent);
//		debuglog(LCF_TIMERS|LCF_UNTESTED, __FUNCTION__ " called (total=%d, current=%d).\n", (DWORD)(pllTotal?*pllTotal:0), (DWORD)(pllCurrent?*pllCurrent:0));
//		return rv;
//	}
//};
//
//HRESULT (STDMETHODCALLTYPE* MyAMOpenProgress::QueryInterface)(IAMOpenProgress* pThis, REFIID riid, void** ppvObj) = 0;
//HRESULT (STDMETHODCALLTYPE* MyAMOpenProgress::QueryProgress)(IAMOpenProgress* pThis, LONGLONG *pllTotal,LONGLONG *pllCurrent) = 0;




//struct MyAMGraphStreams : IAMGraphStreams
//{
//	static BOOL Hook(IAMGraphStreams* obj)
//	{
//		BOOL rv = FALSE;
//		rv |= VTHOOKFUNC(IAMGraphStreams, SyncUsingStreamOffset);
//		rv |= VTHOOKFUNC(IAMGraphStreams, SetMaxGraphLatency);
//
//		//rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
//
//		obj->SyncUsingStreamOffset(TRUE);
//		obj->SetMaxGraphLatency((REFERENCE_TIME)100);
//
//		return rv;
//	}
//
//	//static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IAMGraphStreams* pThis, REFIID riid, void** ppvObj);
//	//static HRESULT STDMETHODCALLTYPE MyQueryInterface(IAMGraphStreams* pThis, REFIID riid, void** ppvObj)
//	//{
//	//	const char* name = riidToName(riid);
//	//	debugprintf(__FUNCTION__ "(0x%X (%s)) called.\n", riid.Data1, name?name:"?");
//	//	HRESULT rv = QueryInterface(pThis, riid, ppvObj);
//	//	if(SUCCEEDED(rv))
//	//		HookCOMInterface(riid, ppvObj);
//	//	return rv;
//	//}
//
//	static HRESULT(STDMETHODCALLTYPE *SyncUsingStreamOffset)             (IAMGraphStreams* pThis, BOOL bUseStreamOffset);
//	static HRESULT STDMETHODCALLTYPE MySyncUsingStreamOffset             (IAMGraphStreams* pThis, BOOL bUseStreamOffset)
//	{
//		debuglog(LCF_TIMERS|LCF_UNTESTED, __FUNCTION__ "(%d) called.\n", (DWORD)bUseStreamOffset);
//		HRESULT rv = SyncUsingStreamOffset(pThis, bUseStreamOffset);
//		return rv;
//	}
//
//	static HRESULT(STDMETHODCALLTYPE *SetMaxGraphLatency)             (IAMGraphStreams* pThis, REFERENCE_TIME rtMaxGraphLatency);
//	static HRESULT STDMETHODCALLTYPE MySetMaxGraphLatency             (IAMGraphStreams* pThis, REFERENCE_TIME rtMaxGraphLatency)
//	{
//		debuglog(LCF_TIMERS|LCF_UNTESTED, __FUNCTION__ "(%d) called.\n", (DWORD)(rtMaxGraphLatency/10000));
//		HRESULT rv = SetMaxGraphLatency(pThis, rtMaxGraphLatency);
//		return rv;
//	}
//};
//
////HRESULT (STDMETHODCALLTYPE* MyAMGraphStreams::QueryInterface)(IAMGraphStreams* pThis, REFIID riid, void** ppvObj) = 0;
//HRESULT (STDMETHODCALLTYPE* MyAMGraphStreams::SyncUsingStreamOffset)(BOOL bUseStreamOffset) = 0;
//HRESULT (STDMETHODCALLTYPE* MyAMGraphStreams::SetMaxGraphLatency)(REFERENCE_TIME rtMaxGraphLatency) = 0;

// WARNING: don't call this directly unless you don't mind it advancing time.
// calling detTimer.GetTicks() with no arguments is safer.
HOOKFUNC DWORD WINAPI MytimeGetTime(void)
{
//	return timeGetTime();
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
	DWORD rv = detTimer.GetTicks(TIMETYPE_TIMEGETTIME);
//	debugprintf(__FUNCTION__ " called (%d).\n", rv);
	return rv;
}

HOOKFUNC VOID WINAPI MyGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
//#if defined(_MSC_VER) && _MSC_VER >= 1400 && _MSC_VER < 1500
//	static bool already = 0;
//	if(!already)
//	{
//		LoadLibrary(0);
//		already = true;
//	}
//#endif

//	GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
//	debugprintf("  ACTUAL: low = 0x%X (%d), high = 0x%X (%d)\n", lpSystemTimeAsFileTime->dwLowDateTime, lpSystemTimeAsFileTime->dwLowDateTime, lpSystemTimeAsFileTime->dwHighDateTime, lpSystemTimeAsFileTime->dwHighDateTime);
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", lpSystemTimeAsFileTime);
	*lpSystemTimeAsFileTime = detTimer.GetFileTime();
//	debugprintf("RETURNED: low = 0x%X (%d), high = 0x%X (%d)\n", lpSystemTimeAsFileTime->dwLowDateTime, lpSystemTimeAsFileTime->dwLowDateTime, lpSystemTimeAsFileTime->dwHighDateTime, lpSystemTimeAsFileTime->dwHighDateTime);
	//lpSystemTimeAsFileTime->dwLowDateTime = 0xC39A9DC8;
	//lpSystemTimeAsFileTime->dwHighDateTime = 0x1C9FDDC;
}
HOOKFUNC VOID WINAPI MyGetSystemTime(LPSYSTEMTIME lpSystemTime)
{
//	GetSystemTime(lpSystemTime);
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", lpSystemTime);
	*lpSystemTime = detTimer.GetSystemTime();
}
HOOKFUNC VOID WINAPI MyGetLocalTime(LPSYSTEMTIME lpLocalTime)
{
//	GetLocalTime(lpLocalTime);
//	GetSystemTime(lpLocalTime);
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", lpLocalTime);
	*lpLocalTime = detTimer.GetSystemTime();
	//lpLocalTime->wYear += 172; // TEMP HACK (some games don't like running in 1827 or whenever)
}

HOOKFUNC MMRESULT WINAPI MytimeGetSystemTime(LPMMTIME pmmt, UINT cbmmt)
{
//	return timeGetSystemTime(pmmt, cbmmt);
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
	pmmt->wType = TIME_MS;
	pmmt->u.ms = detTimer.GetTicks(TIMETYPE_TIMEGETSYSTIME);
	return TIMERR_NOERROR;
}

//HOOKFUNC MMRESULT WINAPI MytimeBeginPeriod(DWORD res)
//{
//	debuglog(LCF_TIMEFUNC, __FUNCTION__ " called.\n");
//	return timeBeginPeriod(res);
//}
//HOOKFUNC MMRESULT WINAPI MytimeEndPeriod(DWORD res)
//{
//	debuglog(LCF_TIMEFUNC, __FUNCTION__ " called.\n");
//	return timeEndPeriod(res);
//}
//HOOKFUNC MMRESULT WINAPI MytimeGetDevCaps(LPTIMECAPS ptc, UINT cbtc)
//{
//	MMRESULT rv = timeGetDevCaps(ptc, cbtc);
//	if(!rv)
//		debuglog(LCF_TIMEFUNC|-1, __FUNCTION__ " called, returned %d,%d.\n", ptc->wPeriodMin, ptc->wPeriodMax);
//	return rv;
//}

HOOKFUNC DWORD WINAPI MyGetTickCount(void)
{
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
//	DWORD a = GetTickCount();
	DWORD b = detTimer.GetTicks(TIMETYPE_GETTICKCOUNT);
//	debugprintf(__FUNCTION__ " called (%d -> %d).\n", a, b);
//	_asm{int 3}
	return b;
}

HOOKFUNC BOOL WINAPI MyGetSystemTimes(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
	debuglog(LCF_TIMEFUNC|LCF_TODO|LCF_UNTESTED|LCF_DESYNC, __FUNCTION__ " called.\n");
	return GetSystemTimes(lpIdleTime, lpKernelTime, lpUserTime);
}

HOOKFUNC NTSTATUS NTAPI MyNtQuerySystemTime(PLARGE_INTEGER SystemTime)
{
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
//	return NtQuerySystemTime(SystemTime);
	if(SystemTime)
	{
		// FIXME: gets called at unreliable times, need to track down why, until then I've disabled TIMETYPE_NTQUERYSYSTIME
		SystemTime->QuadPart = detTimer.GetTicks(/*TIMETYPE_NTQUERYSYSTIME*/);
		return 0; // STATUS_SUCCESS
	}
	return STATUS_ACCESS_VIOLATION;
}
// note: hooking Zw versions of Nt functions is NOT necessary.
// they both get hooked at once due to actually being the same function

HOOKFUNC BOOL WINAPI MyQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
//	return QueryPerformanceCounter(lpPerformanceCount);
	debuglog(LCF_TIMEFUNC|LCF_TIMEGET|LCF_FREQUENT, __FUNCTION__ " called.\n");
	if(lpPerformanceCount)
	{
		lpPerformanceCount->QuadPart = (LONGLONG)detTimer.GetTicks(TIMETYPE_QUERYPERFCOUNT) * (LONGLONG)715909 / (LONGLONG)(1000 / 5);
//		timedebugprintf(__FUNCTION__ " returned %I64d.\n", lpPerformanceCount->QuadPart);
		return TRUE;
	}
	return FALSE;
}
HOOKFUNC BOOL WINAPI MyQueryPerformanceFrequency(LARGE_INTEGER* lpPerformanceFrequency)
{
//	return QueryPerformanceFrequency(lpPerformanceFrequency);
	debuglog(LCF_TIMEFUNC|LCF_FREQUENT, __FUNCTION__ " called.\n");
	if(lpPerformanceFrequency)
	{
		lpPerformanceFrequency->QuadPart = (LONGLONG)715909 * (LONGLONG)5;
//		timedebugprintf(__FUNCTION__ " returned %I64d.\n", lpPerformanceFrequency->QuadPart);
		return TRUE;
	}
	return FALSE;
}

TRAMPFUNC NTSTATUS NTAPI MyNtQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount, LARGE_INTEGER* lpPerformanceFrequency)
{
	debuglog(LCF_TIMEFUNC|LCF_FREQUENT, __FUNCTION__ " called.\n");

	if(lpPerformanceCount)
	{
		// let's not track it (no arg to GetTicks), I don't trust anyone that calls this function directly
		lpPerformanceCount->QuadPart = (LONGLONG)detTimer.GetTicks() * (LONGLONG)715909 / (LONGLONG)(1000 / 5);
		if(lpPerformanceFrequency)
			lpPerformanceFrequency->QuadPart = (LONGLONG)715909 * (LONGLONG)5;
		return 0; // STATUS_SUCCESS
	}
	return STATUS_ACCESS_VIOLATION;
}

bool HookCOMInterfaceTime(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID3(IReferenceClock,MyReferenceClock);

		default: return false;
	}
	return true;
}

void ApplyTimeIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(2, WINMM, timeGetTime),
		MAKE_INTERCEPT(1, WINMM, timeGetSystemTime),
		MAKE_INTERCEPT(1, KERNEL32, QueryPerformanceCounter),
		MAKE_INTERCEPT(1, KERNEL32, QueryPerformanceFrequency),
		MAKE_INTERCEPT(1, NTDLL, NtQueryPerformanceCounter),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemTime),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemTimeAsFileTime),
		MAKE_INTERCEPT(1, KERNEL32, GetLocalTime),
		MAKE_INTERCEPT(1, KERNEL32, GetTickCount),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemTimes),
		MAKE_INTERCEPT(1, NTDLL, NtQuerySystemTime),
		//MAKE_INTERCEPT(1, WINMM, timeBeginPeriod),
		//MAKE_INTERCEPT(1, WINMM, timeEndPeriod),
		//MAKE_INTERCEPT(1, WINMM, timeGetDevCaps),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
