/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

// main EXE cpp
//
// TODO: split up this file. at least the AVI recording part can be separated.

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0500
// Windows Header Files:
#include <windows.h>
#pragma comment(lib,"Version.lib") // Needed for the new Windows version detection

#include <mmsystem.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
//#include "stdafx.h"
//#include "svnrev.h" // defines SRCVERSION number
#include "../shared/version.h"
#include "Resource.h"
#include "trace/ExtendedTrace.h"
#include "InjectDLL.h"
#include "CustomDLGs.h"
#include "Movie.h"
//#include "crc32.h"
#include "CRCMath.h"
#include "inputsetup.h"
#include "ramsearch.h"
#define MAX_LOADSTRING 100
//#include <stdio.h>
#include "logging.h"
#include <math.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <aclapi.h>
#include <assert.h>

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

#include "AVIDumper.h"

#include "../external/ddraw.h"
#include "../shared/logcat.h"
#include "../shared/ipc.h"
#include "../shared/asm.h"
#include "../shared/winutil.h"
#include "ramwatch.h"


#pragma warning(disable:4995)

#ifndef CONTEXT_ALL
#define CONTEXT_ALL (CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)
#endif

#define stricmp _stricmp
#define unlink _unlink

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

BOOL InitInstance(HINSTANCE, int);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
//BOOL CALLBACK	ViewportDlgProc(HWND, UINT, WPARAM, LPARAM);
static void EnableDisablePlayRecordButtons(HWND hDlg);

// Globalized the TAS-flags struct, should reduce memory usage some.
TasFlags localTASflags =
{
	false, //playback
	60, //framerate
	8, //keylimit
	0, //forceSoftware
	0, //windowActivateFlags
	1, //threadMode
	1, //timersMode
	1, //messageSyncMode
	1, //waitSyncMode
	0, //aviMode
	EMUMODE_EMULATESOUND, //emuMode | (((recoveringStale||(fastForwardFlags&FFMODE_SOUNDSKIP))&&fastforward) ? EMUMODE_NOPLAYBUFFERS : 0) | ((threadMode==0||threadMode==4||threadMode==5) ? EMUMODE_VIRTUALDIRECTSOUND : 0),
	1, //forceWindowed
	false, //fastforward
	0, //forceSurfaceMemory
	44100, //audioFrequency
	16, //audioBitsPerSecond
	2, //audioChannels
	0, //stateLoaded
	FFMODE_FRONTSKIP | FFMODE_BACKSKIP | FFMODE_RAMSKIP | FFMODE_SLEEPSKIP, //fastForwardFlags,// | (recoveringStale ? (FFMODE_FRONTSKIP|FFMODE_BACKSKIP) ? 0),
	6000, // initialTime
	2, //debugPrintMode
	1, //timescale
	1, //timescaleDivisor
	false, //frameAdvanceHeld
	0, //allowLoadInstalledDlls
	0, //allowLoadUxtheme
	1, //storeVideoMemoryInSavestates
	0, //appLocale ? appLocale : tempAppLocale
	VERSION, //movie.version,
	0, //osvi.dwMajorVersion, // This will be filled in before the struct is used by anything else, look for the call to "DiscoverOS"
	0, //osvi.dwMinorVersion, // This will be filled in before the struct is used by anything else, look for the call to "DiscoverOS"
	LCF_NONE|LCF_NONE, //includeLogFlags|traceLogFlags,
	LCF_ERROR, //excludeLogFlags
};

//int debugPrintMode = 2;

#if defined(_DEBUG) && 0//1
	#define verbosedebugprintf debugprintf
#else
	#if _MSC_VER > 1310
		#define verbosedebugprintf(...) ((void)0)
	#else
		#define verbosedebugprintf() ((void)0)
		#pragma warning(disable:4002)
	#endif
#endif

//#define DEBUG_THREADS


#include "strsafe.h"

#undef ASSERT
#if 0
#define ASSERT(x)
#else
#define ASSERT(x) do{if(!(x))_asm{int 3}}while(0)
#endif


static const int s_safeMaxIter = 300;
static const int s_safeSleepPerIter = 10;

LPVOID WINAPI RetryingVirtualAllocEx(IN HANDLE hProcess, IN LPVOID lpAddress,
    IN SIZE_T dwSize, IN DWORD flAllocationType, IN DWORD flProtect)
{
	LPVOID rv;
	for(int i = 0; i < s_safeMaxIter; i++)
	{
		rv = VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
		if(rv /*|| GetLastError() != ERROR_ACCESS_DENIED*/)
			break;
		Sleep(s_safeSleepPerIter);
	}
	return rv;
}
//BOOL WINAPI RetryingVirtualProtectEx(IN  HANDLE hProcess, IN  LPVOID lpAddress,
//    IN  SIZE_T dwSize, IN  DWORD flNewProtect, OUT PDWORD lpflOldProtect)
//{
//	BOOL rv;
//	for(int i = 0; i < s_safeMaxIter; i++)
//	{
//		rv = VirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
//		if(rv /*|| GetLastError() != ERROR_ACCESS_DENIED*/)
//			break;
//		Sleep(s_safeSleepPerIter);
//	}
//	return rv;
//}
BOOL WINAPI RetryingWriteProcessMemory(IN HANDLE hProcess, IN LPVOID lpBaseAddress,
    IN LPCVOID lpBuffer, IN SIZE_T nSize, OUT SIZE_T * lpNumberOfBytesWritten)
{
	BOOL rv;
	for(int i = 0; i < s_safeMaxIter; i++)
	{
		rv = WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
		if(rv /*|| GetLastError() != ERROR_ACCESS_DENIED*/)
			break;
		Sleep(s_safeSleepPerIter);
	}
	return rv;
}

void DetectWindowsVersion(int *major, int *minor)
{
	debugprintf("Detecting Windows version...");
	char filename[MAX_PATH+1];
	UINT value = GetWindowsDirectory(filename, MAX_PATH);
	if(value != 0)
	{
		StringCchCat(filename, (size_t)value, _T("\\System32\\svchost.exe"));

		LPDWORD handle = NULL;
		UINT size = 0;
		LPBYTE buffer = NULL;
		DWORD infoSize = GetFileVersionInfoSize(filename, handle);

		if(infoSize != 0)
		{
			char *verInfo = new char[infoSize];

			BOOL verInfoSuccess = GetFileVersionInfo(filename, *handle, infoSize, verInfo);
			if(verInfoSuccess != FALSE)
			{
				BOOL parseSuccess = VerQueryValue(verInfo, "\\",(LPVOID*)&buffer, &size);
				if (parseSuccess != FALSE && size > 0)
				{
					if (((VS_FIXEDFILEINFO *)verInfo)->dwSignature == 0xFEEF04BD)
					{
						/* 
						   svchost.exe versions and their Windows counter parts:
						   5.2 = XP
						   6.0 = Vista
						   6.1 = Win7
						   6.2 = Win8
						   Now don't these remind a lot of the actual windows versions?
						*/
						*major = HIWORD(((VS_FIXEDFILEINFO *)verInfo)->dwFileVersionMS);
						*minor = LOWORD(((VS_FIXEDFILEINFO *)verInfo)->dwFileVersionMS);

						delete [] verInfo; // We no longer need to hold on to this.
						
						debugprintf("Detection succeeded, detected version %d.%d", *major, *minor);
						return;
					}
				}
			}
			delete [] verInfo; // Destroy this if we failed, we cannot have a memory leak now can we?
		}
	}
	debugprintf("Failed to determinate Windows version, using old unsafe method...");

	OSVERSIONINFO osvi = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&osvi);
	*major = osvi.dwMajorVersion;
	*minor = osvi.dwMinorVersion;
	return;
}

//OSVERSIONINFO osvi = {sizeof(OSVERSIONINFO)};
// warning: we can't trust these too much (version lies from compatibility mode shims are common)
bool IsWindowsXP()    { return localTASflags.osVersionMajor == 5 && localTASflags.osVersionMinor == 1; }
bool IsWindowsVista() { return localTASflags.osVersionMajor == 6 && localTASflags.osVersionMinor == 0; }
bool IsWindows7()     { return localTASflags.osVersionMajor == 6 && localTASflags.osVersionMinor == 1; }
// Not used for anything yet, but at least the function is here now, just in case.
bool IsWindows8()	  { return localTASflags.osVersionMajor == 6 && localTASflags.osVersionMinor == 2; }


/*static*/ bool terminateRequest = false;
static bool afterDebugThreadExit = false;


HWND hWnd = 0;
//HWND hExternalWnd = 0;
HWND RamSearchHWnd = NULL; // modeless dialog
HWND RamWatchHWnd = NULL; // modeless dialog
HWND HotkeyHWnd = NULL;
bool HotkeyHWndIsHotkeys = true;
//HWND SpliceHWnd = NULL; // modeless dialog


static Movie movie;



// requires both buffers given to be at least MAX_PATH characters in size.
// it is allowed for both arguments to point at the same buffer.
char* NormalizePath(char* output, const char* path)
{
	extern bool started;
	if(movie.version >= 60 || !started)
		while(*path == ' ')
			path++;
	DWORD len = 0;
	if(!(movie.version >= 40 && movie.version < 53) || !started)
		len = GetFullPathNameA(path, MAX_PATH, output, NULL);
	if(len && len < MAX_PATH)
		GetLongPathNameA(output, output, MAX_PATH); // GetFullPathName won't always convert short filenames to long filenames
	else if(path != output)
		strcpy(output, path);
	return output;
}


#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <psapi.h>
#include <strsafe.h>

#define BUFSIZE 512

static bool TranslateDeviceName(TCHAR* pszFilename)
{
	// Translate path with device name to drive letters.
	TCHAR szTemp[BUFSIZE];
	szTemp[0] = '\0';

	if(GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
	{
		TCHAR szName[MAX_PATH];
		TCHAR szDrive[3] = TEXT(" :");
		BOOL bFound = FALSE;
		TCHAR* p = szTemp;

		do {
			*szDrive = *p;
			if(QueryDosDevice(szDrive, szName, MAX_PATH))
			{
				UINT uNameLen = (UINT)_tcslen(szName);
				if(uNameLen < MAX_PATH) 
				{
					bFound = !_tcsnicmp(pszFilename, szName, uNameLen);
					if(bFound) 
					{
						TCHAR szTempFile[MAX_PATH];
						StringCchPrintf(szTempFile, MAX_PATH, TEXT("%s%s"), szDrive, pszFilename+uNameLen);
						StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, _tcslen(szTempFile));
					}
				}
			}
			while(*p++);
		} while(!bFound && *p);
		NormalizePath(pszFilename, pszFilename);
		return true;
	}
	return false;
}

bool GetFileNameFromProcessHandle(HANDLE hProcess, char* filename)
{
	if(GetProcessImageFileNameA(hProcess, filename, MAX_PATH))
		return TranslateDeviceName(filename);
	return false;
}

bool GetFileNameFromFileHandle(HANDLE hFile, TCHAR* pszFilename) 
{
	bool bSuccess = false;
	if(HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL)) 
	{
		if(void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1))
		{
			if(GetMappedFileName(GetCurrentProcess(), pMem, pszFilename, MAX_PATH)) 
				bSuccess = TranslateDeviceName(pszFilename);
			UnmapViewOfFile(pMem);
		}
		CloseHandle(hFileMap);
	}
	if(!bSuccess)
		strcpy(pszFilename, "(unknown)");
	return bSuccess;
}


int GetThreadSuspendCount(HANDLE hThread)
{
	int count = SuspendThread(hThread);
	ResumeThread(hThread);
	return count;
}
void SetThreadSuspendCount(HANDLE hThread, int count)
{
	int suspendCount;
	do { suspendCount = SuspendThread(hThread);
	} while(suspendCount != -1 && suspendCount < count-1);
	do { suspendCount = ResumeThread(hThread);
	} while(suspendCount != -1 && suspendCount > count+1);
}





//std::map<LPVOID,HANDLE> dllBaseToHandle;
std::map<LPVOID,std::string> dllBaseToFilename;

static int lastFrameAdvanceKeyUnheldTime = timeGetTime();
static int lastFrameAdvanceTime = timeGetTime();

bool paused = false;
//bool fastforward = false;
static bool temporaryUnpause = false;
static bool requestedCommandReenter = false;
static bool cannotSuspendForCommand = false;
static bool goingsuperfast = false;
//static bool frameAdvanceHeld = false;

static bool runningNow = false;
CRITICAL_SECTION g_processMemCS;

bool started = false;
//bool playback = false;
bool finished = false;
bool unsavedMovieData = false;
bool nextLoadRecords = true; // false if next load switches to playback, true if next load switches to recording... aka readonly/read-only toggle
bool exeFileExists = false;
bool movieFileExists = false;
bool movieFileWritable = false;
//static int framerate = 60; // TODO: should probably get this from some per-game database if possible. not critical since it's user-configurable and gets saved in the movie file
//static int keylimit = 8;

// FIXME: TODO: many of these variables are in the TasFlags struct as well, should use a global instance of that instead.

//static DWORD initialTime = 6000;
//int forceSoftware = 0;
//int forceSurfaceMemory = 0;
///*static*/ int threadMode = 1;
///*static*/ int timersMode = 1;
///*static*/ int messageSyncMode = 1;
//int allowLoadInstalledDlls = 0, allowLoadUxtheme = 0, 
int runDllLast = 0;
//int waitSyncMode = 1;
//int aviMode = 0;
static int threadStuckCount = 0;
//static int aviSplitCount = 0;
//static int aviSplitDiscardCount = 0;
//static int requestedAviSplitCount = 0;
/*static*/ int usedThreadMode = -1;
//static int windowActivateFlags = 0;
//int storeVideoMemoryInSavestates = 1;
int storeGuardedPagesInSavestates = 1;
//int appLocale = 0;
int tempAppLocale = 0; // Might still be needed
int forceWindowed = 1;
int truePause = 0;
int onlyHookChildProcesses = 0;
int advancePastNonVideoFrames = 0;
bool advancePastNonVideoFramesConfigured = false;
bool runDllLastConfigured = false;
bool crcVerifyEnabled = true;
//int audioFrequency = 44100;
//int audioBitsPerSecond = 16;
//int audioChannels = 2;
//static int stateLoaded = 0;
bool mainMenuNeedsRebuilding = false;
#ifndef FOCUS_FLAGS_DEFINED
#define FOCUS_FLAGS_DEFINED
enum
{
	FOCUS_FLAG_TASER=0x01,
	FOCUS_FLAG_TASEE=0x02,
	FOCUS_FLAG_OTHER=0x04,
};
#endif
int hotkeysFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_TASER; // allowbackgroundhotkeys
int inputFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_OTHER|FOCUS_FLAG_TASER; // allowbackgroundinput


// these can be set from the UI now (runtime menu > debug logging > include/exclude)
LogCategoryFlag includeLogFlags = LCF_ERROR
#ifdef _DEBUG
|LCF_UNTESTED
|LCF_DESYNC
//|~LCF_NONE
#endif
;
LogCategoryFlag traceLogFlags = LCF_NONE;
LogCategoryFlag excludeLogFlags = LCF_NONE
#ifndef _DEBUG
|LCF_FREQUENT
#endif
;

//static bool traceLogs = false;
//static bool traceLogs = true;

/*static*/ bool tasFlagsDirty = false;



char moviefilename [MAX_PATH+1];
char newmoviefilename [MAX_PATH+1];
char exefilename [MAX_PATH+1];
char commandline [160];
char thisprocessPath [MAX_PATH+1];
char injectedDllPath [MAX_PATH+1];
char subexefilename [MAX_PATH+1];



//int emuMode = EMUMODE_EMULATESOUND;

//int fastForwardFlags = FFMODE_FRONTSKIP | FFMODE_BACKSKIP | FFMODE_RAMSKIP | FFMODE_SLEEPSKIP;

//int timescale = 1, timescaleDivisor = 1;

static bool warnedAboutFrameRateChange = false;

bool recoveringStale = false;
static int recoveringStaleSlot = 0;
static int recoveringStaleFrame = 0;


static char localCommandSlot [256];




void UpdateFrameCountDisplay(int frameCount, int frequency);

void UpdateRerecordCountDisplay()
{
	char str [256];
	sprintf(str, "%d", movie.rerecordCount);
	char str2 [256];
	GetWindowText(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), str2, 256);
	if(strcmp(str,str2))
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), str);
}

int CalcFilesize(const char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file)
		return 0;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fclose(file);
	return size;
}

int CalcExeFilesize()
{
	return CalcFilesize(exefilename);
}


DWORD CalcExeCrc()
{
	return CalcFileCrcCached(exefilename);
}

const char* GetFilenameWithoutPath(const char* path)
{
	const char* slash = max(strrchr(path, '\\'), strrchr(path, '/'));
	return slash ? slash+1 : path;
}

const char* GetExeFilenameWithoutPath()
{
	return GetFilenameWithoutPath(exefilename);
}

struct SharedDataHandle
{
	unsigned char*const dataPtr;
	int userdata;

	void AddRef() { refCount++; }
	void Release()
	{
		if(--refCount == 0)
		{
			delete[] dataPtr;
			delete this;
		}
	}
	int GetRefCount() { return refCount; }
	SharedDataHandle(unsigned char* data) : refCount(1), dataPtr(data), userdata(0) {}
private:
	int refCount;
	SharedDataHandle(const SharedDataHandle& c);
};

template<typename T>
static void ClearAndDeallocateContainer(T& c)
{
	T temp;
	c.swap(temp);
}


// holds the state of a Windows process.
// the goal is for this to be so complete that
// the process can later be restored to exactly the same state,
// or at least pretty close to that for simple games during the same session.
struct SaveState
{
	struct MemoryRegion
	{
		MEMORY_BASIC_INFORMATION info;
		unsigned char* data;
		SharedDataHandle* dataHandle;
	};
	std::vector<MemoryRegion> memory;

	struct Thread
	{
		DWORD id;
		HANDLE handle;
		CONTEXT context;
		int suspendCount;
	};
	std::vector<Thread> threads;

	Movie movie;

	bool valid;
	bool stale;

	void Clear()
	{
		Deallocate();
		ClearAndDeallocateContainer(movie.frames);
		valid = false;
		stale = false;
	}
	SaveState() { Clear(); }

	void Deallocate()
	{
		stale = true;
		for(unsigned int i = 0; i < memory.size(); i++)
			memory[i].dataHandle->Release();
		ClearAndDeallocateContainer(memory);
		ClearAndDeallocateContainer(threads);
		// don't clear movie here, since stale states still need that info
	}
};

static const int maxNumSavestates = 21;
SaveState savestates [maxNumSavestates];


SharedDataHandle* FindMatchingDataBlock(SaveState::MemoryRegion& region)
{
	for(int i = 0; i < maxNumSavestates; i++)
		for(unsigned int j = 0; j < savestates[i].memory.size(); j++)
			savestates[i].memory[j].dataHandle->userdata = 0;

	for(int i = 0; i < maxNumSavestates; i++)
	{
		SaveState& savestate = savestates[i];
		for(unsigned int j = 0; j < savestate.memory.size(); j++)
		{
			SaveState::MemoryRegion& region2 = savestate.memory[j];
			if(region2.info.BaseAddress == region.info.BaseAddress
			&& region2.info.RegionSize >= region.info.RegionSize
			&& region2.dataHandle->userdata == 0)
			{
				if(0 == memcmp(region.data, region2.data, region.info.RegionSize))
				{
					// found a perfectly-matching region of memoroy
					return region2.dataHandle;
				}
				region2.dataHandle->userdata = 1; // avoids re-checking same data
			}
		}
	}
	return 0;
}






bool movienameCustomized = false;

CRITICAL_SECTION g_gameHWndsCS;

std::set<HWND> gameHWnds;
HANDLE hGameProcess = 0;

struct ThreadInfo
{
	//DWORD threadId; // not included because it's already the map index
	HANDLE handle;
	HANDLE hProcess;
	//DWORD waitingCount;
	//DWORD hopelessness;
	char name [64];

	operator HANDLE() { return handle; }
	ThreadInfo& operator= (const HANDLE& h) { 
								ASSERT(h);
		handle = h; /*waitingCount = 0; hopelessness = 0;*/ name[0] = 0; return *this; }
};

std::map<DWORD,ThreadInfo> hGameThreads;
std::vector<DWORD> gameThreadIdList;
int gameThreadIdIndex = 0;


//struct NoLoadRegion
//{
//	unsigned char* data;
//	void* remoteAddress;
//	int size;
//};
//std::map<void*,NoLoadRegion> noLoadRegions;
//
//void AddToNoLoadList(const char* info)
//{
//	void* remoteAddress;
//	int size;
//	sscanf(info, "%d,%d", &remoteAddress, &size);
//	if(!size || !remoteAddress) return;
//	NoLoadRegion region = {0};
//	region.size = size;
//	region.remoteAddress = remoteAddress;
//	noLoadRegions[region.remoteAddress] = region;
//}
//void ClearNoLoadList()
//{
//	noLoadRegions.clear();
//}


static void* remoteCommandSlot = 0;

void ReceiveCommandSlotPointer(__int64 pointerAsInt)
{
	remoteCommandSlot = (void*)pointerAsInt;
}

void SendCommand();
void ClearCommand();



static void* remoteKeyboardState = 0;

void ReceiveKeyStatusPointer(__int64 pointerAsInt)
{
	remoteKeyboardState = (void*)pointerAsInt;
}

static void* remoteTASflags = 0;

void SendTASFlags();

void ReceiveTASFlagsPointer(__int64 pointerAsInt)
{
	remoteTASflags = (void*)pointerAsInt;
	SendTASFlags();
}


static void* remoteDllLoadInfos = 0;

DllLoadInfos dllLoadInfos = {};
bool dllLoadInfosSent = false;

void ReceiveDllLoadInfosPointer(__int64 pointerAsInt)
{
	remoteDllLoadInfos = (void*)pointerAsInt;
	dllLoadInfos.numInfos = 0;
}


static void* remoteTrustedRangeInfos = 0;

void ReceiveTrustedRangeInfosPointer(__int64 pointerAsInt, HANDLE hProcess)
{
	remoteTrustedRangeInfos = (void*)pointerAsInt;
	void SendTrustedRangeInfos(HANDLE hProcess);
	SendTrustedRangeInfos(hProcess);
}


static void* remoteGeneralInfoFromDll = 0;

void ReceiveGeneralInfoPointer(__int64 pointerAsInt)
{
	remoteGeneralInfoFromDll = (void*)pointerAsInt;
}

void ReceiveSoundCaptureInfoPointer(__int64 pointerAsInt)
{
	SetLastFrameSoundInfo((void*)pointerAsInt);
}

//Since GammaRamp is only used for AVI capture there is really no need to 
void ReceiveGammaRampData(__int64 pointerAsInt, HANDLE hProcess)
{
	SetGammaRamp((void*)pointerAsInt, hProcess);
}



void ReceivePaletteEntriesPointer(__int64 pointerAsInt)
{
	SetPaletteEntriesPointer((void*)pointerAsInt);
}

//void ReceiveExtHWndBuf(__int64 pointerAsInt)
//{
//	SIZE_T bytesWritten = 0;
//	WriteProcessMemory(hGameProcess, (void*)pointerAsInt, &hExternalWnd, sizeof(HWND), &bytesWritten);
//}

void ReceiveKeyboardLayout(__int64 pointerAsInt, HANDLE hProcess)
{
	char name [KL_NAMELENGTH] = {0};
	SIZE_T bytesWritten = 0;
	// get the active layout
	ReadProcessMemory(hProcess, (void*)pointerAsInt, name, KL_NAMELENGTH, &bytesWritten);
	name[KL_NAMELENGTH-1] = 0;
	// replace the active layout with one stored in the movie, or vice-versa
	if(localTASflags.playback && *movie.keyboardLayoutName)
		strcpy(name, movie.keyboardLayoutName);
	else
		strcpy(movie.keyboardLayoutName, name);
	// set the possibly-new layout
	WriteProcessMemory(hProcess, (void*)pointerAsInt, name, KL_NAMELENGTH, &bytesWritten);
}


static bool gotSrcDllVersion = false;
void CheckSrcDllVersion(int version)
{
	if((DWORD)version != (DWORD)VERSION)
	{
		const char* str;
		if((DWORD)version > (DWORD)VERSION)
			str = "Wrong version detected: This hourglass.exe is too old compared to wintasee.dll.\nPlease make sure you have extracted Hourglass properly.\nThe files that came with hourglass.exe must stay together with it.";
		else
			str = "Wrong version detected: wintasee.dll is too old compared to this hourglass.exe.\nPlease make sure you have extracted Hourglass properly.\nThe files that came with hourglass.exe must stay together with it.";
		debugprintf("%s\n", str);
		CustomMessageBox(str, "Version Problem", MB_OK | MB_ICONWARNING);
	}
	gotSrcDllVersion = true;
}
void ProcessDllVersion(const char* version)
{
	if(!gotSrcDllVersion)
		CheckSrcDllVersion(0);
}


#ifdef _DEBUG
// TEMP
static char localCommandSlot_COPY [256];
#endif

void SuggestThreadName(DWORD threadId, HANDLE hProcess)
{
#ifdef _DEBUG
	// TEMP
	strcpy(localCommandSlot_COPY, localCommandSlot);
#endif
	strcpy(localCommandSlot, "unknown");
	SendCommand(); // FIXME: sometimes this can happen when requestedCommandReenter is true!
	requestedCommandReenter = false;

	// print the callstack of the exception thread
	std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(threadId);
	if(found != hGameThreads.end())
	{
		HANDLE hThread = found->second;
		THREADSTACKTRACE(hThread, hProcess); // FIXME
	}
}


//void debugprintallmemory(const char* msg)
//{
//	debugprintf("\n");
//	MEMORY_BASIC_INFORMATION mbi = {0};
//	SYSTEM_INFO si = {0};
//	GetSystemInfo(&si);
//	// walk process addresses
//	void* lpMem = si.lpMinimumApplicationAddress;
//	int totalSize = 0;
//	while (lpMem < si.lpMaximumApplicationAddress)
//	{
//		VirtualQueryEx(hGameProcess,lpMem,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
//		// increment lpMem to next region of memory
//		lpMem = (LPVOID)((unsigned char*)mbi.BaseAddress + (DWORD)mbi.RegionSize);
//
//		debugprintf(
//			"%s: BaseAddress=0x%X, AllocationBase=0x%X, AllocationProtect=0x%X, RegionSize=0x%X, State=0x%X, Protect=0x%X, Type=0x%X\n",
//			msg, mbi.BaseAddress,  mbi.AllocationBase,  mbi.AllocationProtect,  mbi.RegionSize,  mbi.State,  mbi.Protect,  mbi.Type
//		);
//	}
//}


void SaveMovie(const char* filename)
{
	// Update some variables, this should be harmless... (mostly)...
	// May however become really harmful if SaveMovie is called during the LoadMovie scenario!
	// Badly fucked up headers are to be expected from that case...

	// TODO: Find a way to NOT do this EVERY SaveMovie call, stuff like this should never change between calls to here.
	// Solved?
	if(movie.headerBuilt == false)
	{
		movie.fps = localTASflags.framerate;
		movie.it = localTASflags.initialTime;
		movie.crc = CalcExeCrc();
		movie.fsize = CalcExeFilesize();
		strcpy(movie.exefname, GetExeFilenameWithoutPath());
		strcpy(movie.commandline, commandline);
		movie.headerBuilt = true;
	}

	if(SaveMovieToFile(movie, filename))
	{
		unsavedMovieData = false;
	}
	// Should be an else-block here with error handling, no?
}

// returns 1 on success, 0 on failure, -1 on cancel
int LoadMovie(char* filename)
{
	if(unsavedMovieData)
	{
		int result = CustomMessageBox("The currently opened movie contains unsaved data.\nDo you want to save it before opening the new movie?\n(Click \"Yes\" to save, \"No\" to proceed without saving)", "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
		if(result == IDYES)
		{
			// TODO: Make this save to the old filename.
			// TODO done by introduction of 'newmoviefilename'?

			// IF SaveMovie fails here, it means that the movie file got deleted between the last save and the creation of the new movie
			// Should we really handle that scenario?
			SaveMovie(moviefilename);
			// transfer the new movie filename into the old movie filename.
			strcpy(moviefilename, filename);
		}
		else
		{
			unsavedMovieData = false;
		}
		movie.headerBuilt = false; // Otherwise we may retain the old header?
	}

	bool rv = LoadMovieFromFile(movie, filename);
	if(rv == false) return 0; // Check if LoadMovieFromFile failed, if it did we don't need to continue.

	if(localTASflags.playback)
	{
		if(movie.frames.size() == 0)
		{
			CustomMessageBox("This movie file doesn't contain any frames, playback is not possible.", "Error!", (MB_OK | MB_ICONERROR));
			return 0; // Even if opening the movie was successful, it cannot be played back, so it "failed".
		}
		if(movie.version != VERSION)
		{
			char str [1024];
			sprintf(str,
			"This movie was recorded using a different version of Hourglass.\n"
			"\n"
			"Movie's version: %d\n"
			"Program version: %d\n"
			"\n"
			"This may lead to the movie desyncing.\n"
			"If it would desync you might want to use the movies version of Hourglass.\n"
			"\n"
			"Do you want to try playing back the movie anyway?\n"
			"(Click \"Yes\" to continue, \"No\" to abort)\n",
			movie.version, (unsigned int)VERSION);

			int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1)); // MS_DEFBUTTON1 marks "Yes" as the default choice.
			if(result == IDNO)
			{
				return -1;
			}
			movie.version = VERSION;
		}
		if(movie.crc != CalcExeCrc())
		{
			char str[1024];
			sprintf(str, "This movie was recorded using a different exe.\n\nMovie's exe (size, crc): %s (%d, %X)\nCurrent exe (size, crc): %s (%d, %X)\n\nPlaying the movie with current exe may lead to the movie desyncing.\nDo you want to continue?\n(Click \"Yes\" to continue, \"No\" to abort)", movie.exefname, movie.fsize, movie.crc, GetExeFilenameWithoutPath(), CalcExeFilesize(), CalcExeCrc());
			int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2));
			if(result == IDNO)
			{
				return -1;
			}
		}
		if(movie.fps != localTASflags.framerate)
		{
			char str[1024];
			sprintf(str, "This movie was recorded using a different fps.\n\nMovie's fps: %d\nCurrent fps: %d\n\nPlaying the movie with current fps may lead to the movie desyncing.\nDo you want to use the movies fps instead?\n(Click \"Yes\" to use the movies fps, \"No\" to use current fps)", movie.fps, localTASflags.framerate);
			int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				localTASflags.framerate = movie.fps;
				// Also update the window text.
				char fpstext[256];
				sprintf(fpstext, "%d", localTASflags.framerate);
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), fpstext);
			}
		}
		if(movie.it != localTASflags.initialTime)
		{
			char str[1024];
			sprintf(str, "This movie was recorded using a different initial time.\n\nMovie's initial time: %d\nCurrent initial time: %d\n\nPlaying the movie with current initial time may lead to the movie desyncing.\nDo you want to use the movies initial time instead?\n(Click \"Yes\" to use the movies initial time, \"No\" to use current initial time)", movie.it, localTASflags.initialTime);
			int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				localTASflags.initialTime = movie.it;
				// Also update the window text.
				char ittext[256];
				sprintf(ittext, "%d", localTASflags.initialTime);
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), ittext);
			}
		}
		if(strcmp(movie.commandline, commandline) != 0)
		{
			char str[1024];
			sprintf(str, "This movie was recorded using a different command line.\n\nMovie's command line: %s\nCurrent command line: %s\n\nPlaying the movie with current command line may lead to the movie desyncing.\nDo you want to use the movies command line instead?\n(Click \"Yes\" to use the movies command line, \"No\" to use current command line)", movie.commandline, commandline);
			int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				strcpy(commandline, movie.commandline);
				// Also update the window text.
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline);
			}
		}
	}
	else // !localTASflags.playback ... i.e. record new movie.
	{
		if(movie.frames.size() > 0)
		{
			int rv = CustomMessageBox("This movie file contains frame data.\nAre you sure you want to overwrite it?\n(Click \"Yes\" to overwrite movie, \"No\" to abort)", "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2));
			if(rv == IDNO) return -1; // Abort
		}
	}

	movie.headerBuilt = true;

	UpdateFrameCountDisplay(movie.currentFrame, 1);
	UpdateRerecordCountDisplay();

	return 1; // We made it!
}

void SaveGameStatePhase2(int slot)
{
	AutoCritSect cs(&g_processMemCS);

	localTASflags.stateLoaded = false;
	SendTASFlags();

	if(finished && !recoveringStale)
	{
		debugprintf("DESYNC WARNING: tried to save state while movie was \"Finished\"\n");
		const char* str = "Warning: The movie is in \"Finished\" status.\n"
			"This means that any input you entered after the movie became \"Finished\" was lost.\n"
			"Any state saved now will contain a movie that immediately desyncs at that point.\n"
			"Are you sure you really want to save the state now?\n";
		int result = CustomMessageBox(str, "Desync Warning", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING);
		if(result == IDNO)
			return;
	}

	debugprintf("SAVED STATE %d\n", slot);

	slot %= maxNumSavestates;
	if(slot < 0)
	{
		slot += maxNumSavestates;
		slot %= maxNumSavestates;
	}

	SaveState& state = savestates[slot];
	state.Clear();

	// save all the threads
	{
		std::map<DWORD,ThreadInfo>::iterator iter;
		for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
		{
			SaveState::Thread thread;
			thread.id = iter->first;
			thread.handle = iter->second;
			if(thread.handle == NULL)
				continue;
			thread.suspendCount = GetThreadSuspendCount(thread.handle);
			if(thread.suspendCount == -1)
				debugprintf("ERROR: Failed to suspend thread?? hThread=0x%X, lastError=0x%X\n", thread.handle, GetLastError());
			else
			{
				thread.context.ContextFlags = CONTEXT_ALL;
				if(GetThreadContext(thread.handle, &thread.context))
					state.threads.push_back(thread);
				else
					debugprintf("ERROR: Failed to save thread?? hThread=0x%X, lastError=0x%X\n", thread.handle, GetLastError());
			}
		}
	}

//	debugprintallmemory("BEFORESAVE");

	// save all the memory
	{
		MEMORY_BASIC_INFORMATION mbi = {0};
		SYSTEM_INFO si = {0};
 		GetSystemInfo(&si);
		// walk process addresses
		void* lpMem = si.lpMinimumApplicationAddress;
//		debugprintf("MEM: START\n");
//		static void* lpMem2 = 0;
//		debugprintf("FOOOO: 0x%X,  0x%X\n", &lpMem, &lpMem2);
		int totalSize = 0;
		while (lpMem < si.lpMaximumApplicationAddress)
		{
			VirtualQueryEx(hGameProcess,lpMem,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
			// increment lpMem to next region of memory
			lpMem = (LPVOID)((unsigned char*)mbi.BaseAddress + (DWORD)mbi.RegionSize);

			// check if it's writable
			// if it's not writable (and committed) then assume/hope it doesn't need to be saved
			//if(((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READWRITE) || (mbi.Protect & PAGE_WRITECOPY) || (mbi.Protect & PAGE_EXECUTE_WRITECOPY))
			if(((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READWRITE))
			&& (!(mbi.Protect & PAGE_GUARD) || storeGuardedPagesInSavestates)
			&& (mbi.State & MEM_COMMIT)
			)
			{
				if(mbi.Protect & PAGE_GUARD)
				{
					DWORD dwOldProtect;
					VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.Protect & ~PAGE_GUARD, &dwOldProtect); 
				}
				SaveState::MemoryRegion region;
				region.info = mbi;
				region.data = new unsigned char[mbi.RegionSize];
				SIZE_T bytesRead = 0;

//				DWORD dwOldProtect;
//				BOOL protectResult = VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &dwOldProtect); 

				if(ReadProcessMemory(hGameProcess, mbi.BaseAddress, region.data, 1, &bytesRead) // save 1 byte so we can test it
				&& WriteProcessMemory(hGameProcess, mbi.BaseAddress, region.data, 1, &bytesRead) // (testing it here speeds up saves)
				&& ReadProcessMemory(hGameProcess, mbi.BaseAddress, region.data, mbi.RegionSize, &bytesRead))
				{
					region.dataHandle = FindMatchingDataBlock(region);
					if(region.dataHandle)
					{
						delete[] region.data;
						region.data = region.dataHandle->dataPtr;
						region.dataHandle->AddRef();
					}
					else
					{
						region.dataHandle = new SharedDataHandle(region.data);
					}

					state.memory.push_back(region);
				}
				else
				{
					verbosedebugprintf("WARNING: couldn't save memory: baseAddress=0x%X, regionSize=0x%X, lastError=0x%X\n",
						mbi.BaseAddress, mbi.RegionSize, GetLastError());

					//// print the callstack of all threads
					//std::map<DWORD,ThreadInfo>::iterator iter;
					//for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
					//{
					//	HANDLE hThread = iter->second;
					//	THREADSTACKTRACE(hThread);
					//}

					delete[] region.data;
				}

				if(mbi.Protect & PAGE_GUARD)
				{
					DWORD dwOldProtect;
					VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect); 
				}

				//debugprintf(
				//"MEM: BaseAddress=0x%X, AllocationBase=0x%X, AllocationProtect=0x%X, RegionSize=0x%X, State=0x%X, Protect=0x%X, Type=0x%X\n",
				//mbi.BaseAddress,  mbi.AllocationBase,  mbi.AllocationProtect,  mbi.RegionSize,  mbi.State,  mbi.Protect,  mbi.Type
				//);

				//// testing
				//SIZE_T bytesRead = 0;
				//unsigned char* buffer = new unsigned char[mbi.RegionSize];
				//ReadProcessMemory(hGameProcess, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead);
				//WriteProcessMemory(hGameProcess, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead);
				//delete [] buffer;

//				totalSize += mbi.RegionSize;
			}
		}
//		debugprintf("MEM: END (%d == %gkb == %gMB)\n", totalSize, totalSize/1024.0f, totalSize/1024.0f/1024.0f);
	}

	// save movie
	state.movie = movie;

	// TEMP: save movie file (temp because should be incremental, not only on savestate)
	if(unsavedMovieData)
	{
		SaveMovie(moviefilename);
	}

	// done
	state.valid = true;
	state.stale = false;


	// print some memory usage info
	{
		int numValidStates = 0;
		int totalBytes = 0;
		int totalUniqueBytes = 0;
		for(int i = 0; i < maxNumSavestates; i++)
			for(unsigned int j = 0; j < savestates[i].memory.size(); j++)
				savestates[i].memory[j].dataHandle->userdata = 0;
		for(int i = 0; i < maxNumSavestates; i++)
		{
			SaveState& savestate = savestates[i];
			if(savestate.valid)
				numValidStates++;
			for(unsigned int j = 0; j < savestate.memory.size(); j++)
			{
				SaveState::MemoryRegion& region = savestate.memory[j];
				SharedDataHandle& datahandle = *region.dataHandle;
				totalBytes += region.info.RegionSize;
				if(datahandle.userdata == 0)
					totalUniqueBytes += region.info.RegionSize;
				datahandle.userdata++;
			}
		}
		debugprintf("%d savestates, %g MB logical, %g MB actual\n", numValidStates, totalBytes/(float)(1024*1024), totalUniqueBytes/(float)(1024*1024));
	}

}

static bool MovieStatePreceeds(const Movie& shorter, const Movie& longer)
{
	if(shorter.currentFrame > longer.currentFrame)
		return false;
	if(memcmp(&shorter.frames[0], &longer.frames[0], shorter.currentFrame * sizeof(shorter.frames[0])))
		return false;
	return true;
}

void LoadGameStatePhase2(int slot);
static void RecoverStaleState(int slot)
{
	SaveState& state = savestates[slot];

	// let's see if we can avoid starting all over...
	// maybe an existing savestate or the current state
	// matches all input frames leading up to the target state.

	int bestStateToUse = -2; // -2 == restart, -1 == continue, 0+ == save slot number
	int bestStateLength = 0; // current movie length of the best option so far

	if(MovieStatePreceeds(movie, state.movie))
	{
		if(movie.currentFrame > bestStateLength)
		{
			// we can simply continue
			bestStateToUse = -1;
			bestStateLength = movie.currentFrame;
		}
	}

	for(int i = 0; i < maxNumSavestates; i++)
	{
		if(savestates[i].valid && !savestates[i].stale)
		{
			if(savestates[i].movie.currentFrame > bestStateLength)
			{
				if(MovieStatePreceeds(savestates[i].movie, state.movie))
				{
					// we can simply continue
					bestStateToUse = i;
					bestStateLength = savestates[i].movie.currentFrame;
				}
			}
		}
	}

	int emulateFramesRemaining = state.movie.currentFrame - bestStateLength;

	// now that we have the information required,
	// we'll ask the user if they really want to do it:
	char str [2048];
	if(bestStateToUse <= -2)
	{
		sprintf(str,
			"Warning: The savestate you are trying to load has gone stale.\n"
			"The only way to recover it is by playing the movie file within it.\n"
			"Attempt to recover the savestate by playing the movie from the beginning?\n"
			"Doing so will make all other savestates become stale (at least initially).\n"
			"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, emulateFramesRemaining);
	}
	else if(bestStateToUse == -1)
	{
		sprintf(str,
			"Warning: The savestate you are trying to load has gone stale.\n"
			"The only way to recover it is by playing the movie file within it.\n"
			"Attempt to recover the savestate by continuing the movie from the current state?\n"
			"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, emulateFramesRemaining);
	}
	else
	{
		sprintf(str,
			"Warning: The savestate you are trying to load has gone stale.\n"
			"The only way to recover it is by playing the movie file within it.\n"
			"Attempt to recover the savestate by continuing the movie from savestate %d?\n"
			"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, bestStateToUse, emulateFramesRemaining);
	}
	int result = CustomMessageBox(str, "Stale Savestate", MB_YESNO | MB_ICONQUESTION);
	if(result == IDNO)
		return;

	if(bestStateToUse >= 0)
	{
		// load another savestate in playback mode to continue
		bool prevNextLoadRecords = nextLoadRecords;
		nextLoadRecords = false;
		LoadGameStatePhase2(bestStateToUse);
		nextLoadRecords = prevNextLoadRecords;
		// I thought this was unnecessary but maybe it fixes a desync...
		movie.frames = state.movie.frames;
		localTASflags.playback = true;
	}
	if(bestStateToUse == -1)
	{
		// just continue in playback mode using the target state's movie
		movie.frames = state.movie.frames;
		localTASflags.playback = true;
	}

	recoveringStale = true;
	recoveringStaleSlot = slot;
	recoveringStaleFrame = state.movie.currentFrame;
	paused = false;
	tasFlagsDirty = true;
	mainMenuNeedsRebuilding = true;

	if(bestStateToUse <= -2)
	{
		// need to restart
		PostMessage(hWnd, WM_COMMAND, IDC_BUTTON_STOP, 42);
//			PostMessage(hWnd, WM_COMMAND, IDC_BUTTON_PLAY, 0);
	}
}


void LoadGameStatePhase2(int slot)
{
	AutoCritSect cs(&g_processMemCS);

	SaveState& state = savestates[slot];
	if(!state.valid)
	{
		debugprintf("NO STATE %d TO LOAD\n", slot);
		localTASflags.stateLoaded = -1;
		SendTASFlags();
		return;
	}

	localTASflags.stateLoaded = true;
	SendTASFlags();

	bool wasRecoveringStale = recoveringStale;
	if(state.stale)
	{
		RecoverStaleState(slot);
		return;
	}
	else
	{
		if(recoveringStale)
		{
			recoveringStale = false;
			tasFlagsDirty = true;
			mainMenuNeedsRebuilding = true;
		}
	}

	if(!wasRecoveringStale)
		debugprintf("LOADED STATE %d\n", slot);
	else
		debugprintf("RECOVERING STATE %d\n", slot);


	// OK, this is going to be way harder than saving the state,
	// since we have to reconcile differences in allocated threads and memory regions

	if(!wasRecoveringStale)
	{

		//// store the noload regions
		//{
		//	std::map<void*,NoLoadRegion>::iterator iter;
		//	for(iter = noLoadRegions.begin(); iter != noLoadRegions.end(); iter++)
		//	{
		//		NoLoadRegion region = iter->second;
		//		SIZE_T bytesRead = 0;
		//		region.data = new unsigned char[region.size];
		//		region.dataHandle.AddRef();
		//		if(!ReadProcessMemory(hGameProcess, region.remoteAddress, region.data, region.size, &bytesRead))
		//		{
		//			delete[] region.data;
		//			region.data = NULL;
		//		}
		//	}
		//}

		// load all the threads
		{
			//std::map<DWORD,ThreadInfo> threadsFoundOrMade;
			for(unsigned int i = 0; i < state.threads.size(); i++)
			{
				SaveState::Thread thread = state.threads[i];

				if(hGameThreads[thread.id] != thread.handle)
				{
					// this is obsolete anyway, but even if it weren't,
					// it should still be disabled because CreateRemoteThread is too suspicious a function to call.
					//// the thread doesn't exist anymore, have to create it first
					//thread.handle = RetryingCreateRemoteThread(
					//		hGameProcess,
					//		NULL,
					//		0,
					//		(LPTHREAD_START_ROUTINE)NULL, // is this allowed?
					//		NULL,
					//		NULL,
					//		&thread.id
					//);
					//// not sure if this matters considering the CREATE_THREAD_DEBUG_EVENT case should happen too, but it can't hurt
					//hGameThreads[thread.id] = thread.handle;

					//debugprintf("LOAD HAD TO CREATE THREAD: id=0x%X, handle=0x%X\n", thread.id, thread.handle);
					debugprintf("FAILED TO LOAD THREAD: id=0x%X, handle=0x%X\n", thread.id, thread.handle);
				}

				//if(thread.suspendCount)
				//	continue;

				verbosedebugprintf("Loaded thread: handle=0x%X, id=0x%X, suspendcount=0x%X\n", thread.handle, thread.id, thread.suspendCount);

				// now we can simply load into it
				SetThreadContext(thread.handle, &thread.context);
				SetThreadSuspendCount(thread.handle, thread.suspendCount);

				//threadsFoundOrMade[thread.id] = thread.handle;
			}

			//// now terminate any orphaned threads
			//// (or maybe it's safer to indefinitely suspend them?)
			//std::map<DWORD,ThreadInfo>::iterator iter;
			//for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
			//{
			//	DWORD id = iter->first;
			//	HANDLE hThread = iter->second;
			//	if(threadsFoundOrMade[id] != hThread)
			//	{
			//		debugprintf("LOAD HAD TO TERMINATE THREAD: id=0x%X, handle=0x%X\n", id, hThread);

			//		SuspendThread(hThread); // in case terminate somehow fails or is delayed
			//		TerminateThread(hThread, 0);
			//	}
			//}
		}

//		debugprintallmemory("BEFORELOAD");

		// load all the memory (instant brain transplant)
		{
			for(unsigned int i = 0; i < state.memory.size(); i++)
			{
				SaveState::MemoryRegion region = state.memory[i];

				MEMORY_BASIC_INFORMATION mbi = region.info;

				//if(!((DWORD)mbi.AllocationBase >= 0x1890000 && (DWORD)mbi.AllocationBase <= 0x19E0000))
				//	continue;
				//if((DWORD)mbi.AllocationBase != 0x400000)
				//	continue;
				//if((DWORD)mbi.BaseAddress >= 0x1890000)
				//	continue;

				verbosedebugprintf(
				"LMEM: BaseAddress=0x%X, AllocationBase=0x%X, AllocationProtect=0x%X, RegionSize=0x%X, State=0x%X, Protect=0x%X, Type=0x%X\n",
				mbi.BaseAddress,  mbi.AllocationBase,  mbi.AllocationProtect,  mbi.RegionSize,  mbi.State,  mbi.Protect,  mbi.Type
				);

				// we could check whether we need to allocate the region,
				// but VirtualAlloc allows for commit regions to overlap with existing regions
				// ("An attempt to commit a page that is already committed does not cause the function to fail.
				//   This means that you can commit pages without first determining the current commitment state of each page")
				// so we'll simply ask for it to be allocated in case it needs to be

				void* allocAddr = VirtualAllocEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.State, mbi.Protect & ~PAGE_GUARD);
				if(allocAddr != mbi.BaseAddress && mbi.State == MEM_COMMIT && GetLastError() != 0x5)
					allocAddr = VirtualAllocEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT | MEM_RESERVE, mbi.Protect & ~PAGE_GUARD);
				//if(allocAddr != mbi.BaseAddress && mbi.State == MEM_COMMIT && GetLastError() != 0x5)
				//{
				//	VirtualFreeEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, MEM_DECOMMIT);
				//	allocAddr = VirtualAllocEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT | MEM_RESERVE, mbi.Protect & ~PAGE_GUARD);
				//}
				//if(allocAddr != mbi.BaseAddress && mbi.State == MEM_COMMIT && GetLastError() != 0x5)
				//{
				//	VirtualFreeEx(hGameProcess, mbi.AllocationBase, 0, MEM_RELEASE);
				//	allocAddr = VirtualAllocEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT | MEM_RESERVE, mbi.Protect & ~PAGE_GUARD);
				//}
	//			void* allocAddr = VirtualAllocEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, (mbi.State == MEM_COMMIT) ? (MEM_COMMIT|MEM_RESERVE) : mbi.State, mbi.Protect);
				if(allocAddr != mbi.BaseAddress && !(mbi.Type & MEM_IMAGE))
					debugprintf("FAILED TO COMMIT MEMORY REGION: BaseAddress=0x%08X, AllocationBase=0x%08X, RegionSize=0x%X, ResultAddress=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.AllocationBase, mbi.RegionSize, allocAddr, GetLastError());
				DWORD dwOldProtect = 0;
				BOOL protectResult = VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.Protect & ~PAGE_GUARD, &dwOldProtect); 
				if(!protectResult )//&& !(mbi.Type & MEM_IMAGE))
					debugprintf("FAILED TO PROTECT MEMORY REGION: BaseAddress=0x%08X, RegionSize=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.RegionSize, GetLastError());

				SIZE_T bytesWritten = 0;
				BOOL writeResult = WriteProcessMemory(hGameProcess, mbi.BaseAddress, region.data, mbi.RegionSize, &bytesWritten);
				if(!writeResult )//&& !(mbi.Type & MEM_IMAGE))
					debugprintf("FAILED TO WRITE MEMORY REGION: BaseAddress=0x%08X, RegionSize=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.RegionSize, GetLastError());

				if(mbi.Protect & PAGE_GUARD)
					VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect); 
			}
		}

//		debugprintallmemory("AFTERLOAD");

		// make sure we didn't accidentally resurrect a command somehow
		ClearCommand();

		//// restore the noload regions
		//{
		//	std::map<void*,NoLoadRegion>::iterator iter;
		//	for(iter = noLoadRegions.begin(); iter != noLoadRegions.end(); iter++)
		//	{
		//		NoLoadRegion region = iter->second;
		//		if(!region.data)
		//			continue;
		//		SIZE_T bytesWritten = 0;
		//		if(!WriteProcessMemory(hGameProcess, region.remoteAddress, region.data, region.size, &bytesWritten))
		//			debugprintf("FAILED TO RESTORE NOLOAD REGION: BaseAddress=0x%08X, RegionSize=0x%X, LastError=0x%X\n", region.remoteAddress, region.size, GetLastError());
		//		delete[] region.data;
		//		region.data = NULL;
		//	}
		//}
	}

	// load movie
	bool wasFinished = finished;
	if(nextLoadRecords || wasRecoveringStale) // switch to recording / read+write
	{
		int rerecords = movie.rerecordCount;

		movie = state.movie;
		finished = (movie.currentFrame > (int)movie.frames.size());

		movie.rerecordCount = (unsavedMovieData || localTASflags.playback) ? (rerecords+1) : (rerecords);
		localTASflags.playback = false;
		UpdateRerecordCountDisplay();

		if(wasRecoveringStale)
		{
			//recoveringStaleFrame = movie.currentFrame;
			localTASflags.playback = !nextLoadRecords;
			paused = true;
			EnableDisablePlayRecordButtons(hWnd);
		}
	}
	else // switch to playback / read-only
	{
		if(state.movie.currentFrame > (int)movie.frames.size())
		{
			finished = true;
			debugprintf("MOVIE END WARNING: loaded movie frame %d comes after end of current movie length %d\n", state.movie.currentFrame, movie.frames.size());
			bool warned = false;
			if(!warned)
			{
				warned = true;
				char str [1024];
				sprintf(str, "Warning: Loaded state is at frame %d, but current movie is only %d frames long.\n"
					"You should load a different savestate before continuing, or switch to read+write and reload it."
					, state.movie.currentFrame, movie.frames.size());
				CustomMessageBox(str, "Movie End Warning", MB_OK | MB_ICONWARNING);
			}
		}
		else
		{
			localTASflags.playback = true;
			finished = false;
			movie.currentFrame = state.movie.currentFrame;
			bool warned = false;
			for(int i = 0; i < movie.currentFrame; i++)
			{
				if(memcmp(&movie.frames[i], &state.movie.frames[i], sizeof(MovieFrame)))
				{
					debugprintf("DESYNC WARNING: loaded movie has mismatch on frame %d\n", i);
					if(!warned)
					{
						warned = true;
						char str [1024];
						sprintf(str, "Warning: Loaded state's movie input does not match current movie input on frame %d.\n"
							"This can cause a desync. You should load a different savestate before continuing.", i);
						CustomMessageBox(str, "Desync Warning", MB_OK | MB_ICONWARNING);
					}
				}
			}
		}
	}
	//finished = movie.currentFrame > (int)movie.frames.size();
	SendTASFlags();

	UpdateFrameCountDisplay(movie.currentFrame, 1);

	Update_RAM_Search();

	if(unsavedMovieData)
	{
		SaveMovie(moviefilename);
	}

	// done... actually that wasn't so bad
}

bool CreateAVIFile()
{
	char filename[MAX_PATH+1];
	filename[0] = '\0'; // extra safety
	char title[32];
	if(localTASflags.aviMode & 1)
		strcpy(title, "Save AVI File");
	else
		strcpy(title, "Save AVI File (audio only)");
	OPENFILENAME ofn = {sizeof(OPENFILENAME)};
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = "AVI file\0*.avi\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = thisprocessPath;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "avi";
	if(!GetSaveFileName(&ofn))
		return false;

	// Attempt to inform the AVI dumper about the path to dump to
	bool success = SetAVIFilename(filename);
	if(!success) return false;
	// If hGameProcess has a value at this point and file creation was successful,
	// register hGameProcess with the AVI Dumper now so that we can start capture immediately
	if(hGameProcess) SetCaptureProcess(hGameProcess);
	return true;
}

void SendCommand()
{
	if(requestedCommandReenter) // a second SendCommand can overwrite the first, which is probably an error
	{
		static char tempCommandSlot [256];
		tempCommandSlot[0] = 0;
		SIZE_T bytesRead = 0;
		ReadProcessMemory(hGameProcess, remoteCommandSlot, tempCommandSlot, sizeof(tempCommandSlot), &bytesRead);
		debugprintf(__FUNCTION__ " ERROR!!! overwrote earlier command before it executed. replaced \"%s\" with \"%s\".\n", tempCommandSlot, localCommandSlot);
		if(IsDebuggerPresent())
		{
			_asm{int 3} // note: this is continuable
		}
	}

	SIZE_T bytesWritten = 0;
	WriteProcessMemory(hGameProcess, remoteCommandSlot, localCommandSlot, sizeof(localCommandSlot), &bytesWritten);
	requestedCommandReenter = true;
}
void ClearCommand()
{
	localCommandSlot[0] = 0;
	SIZE_T bytesWritten = 0;
	WriteProcessMemory(hGameProcess, remoteCommandSlot, localCommandSlot, 1, &bytesWritten);
}
inline void SetFastForward(bool set)
{
	//if(!remoteCommandSlot)
	//	return;
	//sprintf(localCommandSlot, "FAST: %d", set?1:0);
	//SendCommand();
	if(localTASflags.fastForward != set)
	{
		localTASflags.fastForward = set;
		tasFlagsDirty = true;
	}
}
void SaveGameStatePhase1(int slot)
{
	if(!remoteCommandSlot)
		return;
	sprintf(localCommandSlot, "SAVE: %d", slot);
	SendCommand();
	cannotSuspendForCommand = true;
}
void LoadGameStatePhase1(int slot)
{
	if(!remoteCommandSlot)
		return;
	sprintf(localCommandSlot, "LOAD: %d", slot);
	SendCommand();
	cannotSuspendForCommand = true;
}
void HandleRemotePauseEvents()
{
	if(truePause && !terminateRequest)
		return;
	if(!remoteCommandSlot)
		return;
	sprintf(localCommandSlot, "HANDLEEVENTS: %d", 1);
	SendCommand();
}

void SendTASFlags()
{
	if(!started)
		return;
	
	//Some updates that have to be done here
	localTASflags.emuMode = localTASflags.emuMode | (((recoveringStale||(localTASflags.fastForwardFlags&FFMODE_SOUNDSKIP))&&localTASflags.fastForward) ? EMUMODE_NOPLAYBUFFERS : 0) | ((localTASflags.threadMode==0||localTASflags.threadMode==4||localTASflags.threadMode==5) ? EMUMODE_VIRTUALDIRECTSOUND : 0);
	//localTASflags.fastForwardFlags = localTASflags.fastForwardFlags | (recoveringStale ? (FFMODE_FRONTSKIP|FFMODE_BACKSKIP) ? 0);
	localTASflags.appLocale = localTASflags.appLocale ? localTASflags.appLocale : tempAppLocale;
	localTASflags.includeLogFlags = includeLogFlags | traceLogFlags;
	localTASflags.excludeLogFlags = excludeLogFlags;

	tasFlagsDirty = false;

	SIZE_T bytesWritten = 0;
	WriteProcessMemory(hGameProcess, remoteTASflags, &localTASflags, sizeof(localTASflags), &bytesWritten);
}


bool InputHasFocus(bool isHotkeys)
{
	int flags = isHotkeys ? hotkeysFocusFlags : inputFocusFlags;
	
	if(flags == (FOCUS_FLAG_TASER|FOCUS_FLAG_TASEE|FOCUS_FLAG_OTHER))
		return true;

	HWND curHWnd = GetForegroundWindow();
	{
		AutoCritSect cs(&g_gameHWndsCS);
		bool isGameWindow = gameHWnds.find(curHWnd) != gameHWnds.end();
		
		if(((flags & FOCUS_FLAG_TASER) && (curHWnd == hWnd))
		|| ((flags & FOCUS_FLAG_TASEE) && (isGameWindow || (started && gameHWnds.empty())))
		|| ((flags & FOCUS_FLAG_OTHER) && (curHWnd != hWnd && !isGameWindow)))
		{
			return true;
		}
		return false;
	}
}



struct UncompressedMovieFrame
{
	unsigned char keys [256];
};


// InjectLocalInputs
void RecordLocalInputs()
{
	extern unsigned char localGameInputKeys [256];

	if(InputHasFocus(false))
	{
		Update_Input(NULL, true, false, false, false);
	}
	else
	{
		memset(localGameInputKeys, 0, sizeof(localGameInputKeys));
	}

#if 0 // TEMP HACK until proper autofire implemented
	bool ZXmode = (GetAsyncKeyState('Q') & 0x8000) != 0;
	if(ZXmode) {
		localGameInputKeys[(movie.currentFrame & 1) ? 'Z' : 'X'] = 1;
	}
#endif


	MovieFrame frame = {0};
	int frameByte = 0;

	// pack frame of input
	for(int i = 1; i < 256 && frameByte < sizeof(frame.heldKeyIDs); i++)
	{
		if(localGameInputKeys[i])
		{
			frame.heldKeyIDs[frameByte] = i;
			frameByte++;
			verbosedebugprintf("recording press of key 0x%X on frame %d\n", i, movie.currentFrame);
		}
	}

	if((int)movie.currentFrame < (int)movie.frames.size())
		movie.frames.erase(movie.frames.begin() + movie.currentFrame, movie.frames.end());
	if(movie.currentFrame == movie.frames.size())
	{
		movie.frames.push_back(frame);
		verbosedebugprintf("RECORD: wrote to movie frame %d\n", movie.currentFrame);
		unsavedMovieData = true;
	}
	else if(!finished)
	{
		finished = true;
		debugprintf("RECORDING STOPPED because %d != %d\n", movie.currentFrame, (int)movie.frames.size());
	}
}

#if 0
void MultitrackHack()
{
// TEMP HACK until configurable multitrack implemented
	if((GetAsyncKeyState('M') & 0x8000) == 0)
	{
		// not activated this frame
		return;
	}

	if((unsigned int)movie.currentFrame >= (unsigned int)movie.frames.size())
	{
		return;
	}


	// unpack frame of input
	UncompressedMovieFrame existingMovieKeyboardState = {0};
	{
		MovieFrame frame = movie.frames[movie.currentFrame];
		for(int i = 0; i < sizeof(frame.heldKeyIDs); i++)
		{
			int vk = frame.heldKeyIDs[i];
			if(!vk)
				continue;
			existingMovieKeyboardState.keys[vk] = true;
		}
	}


	extern unsigned char localGameInputKeys [256];

	if(InputHasFocus(false))
	{
		Update_Input(NULL, true, false, false, false);
	}
	else
	{
		memset(localGameInputKeys, 0, sizeof(localGameInputKeys));
	}


	unsigned char mergedGameInputKeys [256];
	memcpy(mergedGameInputKeys, existingMovieKeyboardState.keys, sizeof(mergedGameInputKeys));


	// TEMP HACK until configurable multitrack implemented
	{
		mergedGameInputKeys['A'] = localGameInputKeys['A'];
		mergedGameInputKeys['S'] = localGameInputKeys['S'];
		mergedGameInputKeys['X'] = localGameInputKeys['X'];
		if(localGameInputKeys['Z'])
			mergedGameInputKeys['Z'] = 1;
		if(!localGameInputKeys[VK_UP] || !localGameInputKeys[VK_DOWN])
		{
			mergedGameInputKeys[VK_UP] = localGameInputKeys[VK_UP];
			mergedGameInputKeys[VK_DOWN] = localGameInputKeys[VK_DOWN];
		}
	}

#if 1 // TEMP HACK until proper autofire implemented
	bool ZXmode = (GetAsyncKeyState('Q') & 0x8000) != 0;
	if(ZXmode && !(movie.currentFrame & 1)) {
		mergedGameInputKeys['X'] = 1;
	}
#endif


	MovieFrame frame = {0};
	int frameByte = 0;

	// pack frame of input
	for(int i = 1; i < 256 && frameByte < sizeof(frame.heldKeyIDs); i++)
	{
		if(mergedGameInputKeys[i])
		{
			frame.heldKeyIDs[frameByte] = i;
			frameByte++;
		}
	}

	// apply merged input back to movie
	//if((unsigned int)movie.currentFrame < (unsigned int)movie.frames.size())
	{
		movie.frames[movie.currentFrame] = frame;
		//unsaved = true;
	}
}
#endif

void InjectCurrentMovieFrame() // playback ... or recording, now
{
	if((unsigned int)movie.currentFrame >= (unsigned int)movie.frames.size())
	{
		if(!finished)
		{
			finished = true;
			debugprintf("MOVIE END (%d >= %d)\n", movie.currentFrame, (int)movie.frames.size());
			mainMenuNeedsRebuilding = true;
			// let's clear the key input when the movie is finished
			// to prevent potentially weird stuff from holding the last frame's buttons forever
			unsigned char keys [256] = {0};
			SIZE_T bytesWritten = 0;
			WriteProcessMemory(hGameProcess, remoteKeyboardState, &keys, 256, &bytesWritten);
		}
		return;
	}

//	movie.currentFrame++;
//	verbosedebugprintf("PLAY: movie.currentFrame = %d -> %d\n", movie.currentFrame-1,movie.currentFrame);

	MovieFrame frame = movie.frames[movie.currentFrame];
	verbosedebugprintf("injecting movie frame %d\n", movie.currentFrame);

	UncompressedMovieFrame localKeyboardState = {0};

	// unpack frame of input
	for(int i = 0; i < sizeof(frame.heldKeyIDs); i++)
	{
		int vk = frame.heldKeyIDs[i];
		if(!vk)
			continue;
		localKeyboardState.keys[vk] = true;
		verbosedebugprintf("injecting press of key 0x%X\n", vk);
	}

	SIZE_T bytesWritten = 0;
	if(!WriteProcessMemory(hGameProcess, remoteKeyboardState, &localKeyboardState, sizeof(UncompressedMovieFrame), &bytesWritten))
	{
		debugprintf("failed to write input!!\n");
	}

	// advance to next frame
	movie.currentFrame++;
}



// convert stale savestates to non-stale savestates when the current input matches exactly
// relies on SendCommand, so must be used inside the command loop
void RefreshSavestates(int frameCount)
{
	for(int i = 0; i < maxNumSavestates; i++)
	{
		if(savestates[i].stale)
		{
			if(savestates[i].movie.currentFrame == frameCount)
			{
				MovieFrame* frameArray = &savestates[i].movie.frames[0];
				MovieFrame* curFrameArray = &movie.frames[0];
				if(!memcmp(frameArray, curFrameArray, sizeof(MovieFrame)*(frameCount-1)))
				{
					SaveGameStatePhase1(i);
					return; // return because SaveGameStatePhase1 only works for 1 state at a time
				}
			}
		}
	}
}


void SuspendAllExcept(int ignoreThreadID);
void ResumeAllExcept(int ignoreThreadID);
void CheckDialogChanges(int frameCount);

//DWORD lastKeyThreadId = 0;


//struct DebugKeyStatus
//{
//	bool keys [256];
//};
//static DebugKeyStatus curinput = {0};
//static DebugKeyStatus previnput = {0};
//static bool KeyHeld(int vk)
//{
//	return curinput.keys[vk];
//}
//static bool KeyJustPressed(int vk)
//{
//	return curinput.keys[vk] && !previnput.keys[vk];
//}
//static bool KeyJustReleased(int vk)
//{
//	return previnput.keys[vk] && !curinput.keys[vk];
//}

void CheckHotkeys(int frameCount, bool frameSynced)
{
//#if 1 // TEMPORARY TESTING
//#ifdef _DEBUG
//	static int foo = 0;
//	foo++;
//	if(!paused || !(foo & 0xFF))
//		paused = !paused;
//#endif
//#endif

	bool recoveringNearTargetFrame = recoveringStale && (frameCount >= recoveringStaleFrame - 2);

	if(localTASflags.fastForward && !paused && !recoveringNearTargetFrame && goingsuperfast)
	{
		// fast-forward optimization
		// (this function actually becomes a bottleneck at such high speeds)
		static int checkskipcount = 0;
		checkskipcount++;
		if(checkskipcount & 31)
			return;
	}

	if(recoveringStale)
	{
		// fast-forward by default when recovering stale,
		// except a couple frames before reaching the destination
		// (to give the screen a chance to get drawn at the correct time)
		bool ff = !recoveringNearTargetFrame;
		//if(KeyHeld(VK_TAB) && !KeyHeld(VK_MENU))
		//	ff = !ff;
		SetFastForward(ff);

		if(frameCount == recoveringStaleFrame)
		{
//			SetFastForward(false);
			recoveringStale = false;
			mainMenuNeedsRebuilding = true;
			paused = true;
			// this is done in RefreshSavestates now
//			SaveGameStatePhase1(recoveringStaleSlot);
			localTASflags.playback = !nextLoadRecords;
			SendTASFlags();
		}
	}

	if(!InputHasFocus(true))
	{
		// ignore background input
		Update_Input(hWnd, frameSynced, false, false, true);
		return;
	}

	//if(KeyJustPressed('Y'))
	//	SuspendAllExcept(threadId);
	//if(KeyJustPressed('U'))
	//	ResumeAllExcept(threadId);
//	lastKeyThreadId = threadId;

	Update_Input(hWnd, frameSynced, true, false, true);

	// the rest of this should be replaced by Update_Input (done?)

	//previnput = curinput;
	//for(int i = 0; i < 256; i++)
	//	if(i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL)
	//		curinput.keys[i] = (GetKeyState(i) & 0x1) != 0;
	//	else
	//		curinput.keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;

	//for(int i = 0; i < 10; i++)
	//{
	//	bool fkeyp = KeyJustPressed(i+VK_F1);
	//	bool nkeyp = KeyJustPressed((i==9)?'0':(i+'1'));
	//	int iextra = fkeyp ? 11 : 1;
	//	if(fkeyp || nkeyp)
	//	{
	//		if(KeyHeld(VK_SHIFT))
	//			SaveGameStatePhase1(i+iextra);
	//		else
	//			//if(frameCount != -1)
	//				LoadGameStatePhase1(i+iextra);
	//			//else
	//			//	LoadGameStatePhase2(i+iextra);
	//		break;
	//	}
	//}

	static bool FastForwardKeyDown_prev = false;
	if(FastForwardKeyDown_prev != FastForwardKeyDown)
	{
		FastForwardKeyDown_prev = FastForwardKeyDown;
		SetFastForward(FastForwardKeyDown != 0);
	}

	//if(KeyHeld(VK_CONTROL) && !KeyHeld(VK_SHIFT) && KeyJustPressed('T'))
	//{
	//	nextLoadRecords = !nextLoadRecords;
	//	if(nextLoadRecords)
	//		debugprintf("switched to read+write\n");
	//	else
	//		debugprintf("switched to read-only\n");
	//}

	//if(KeyJustPressed(VK_DELETE) || KeyJustPressed(VK_PAUSE))
	//{
	//	paused = !paused;

	//	if(paused && fastforward)
	//		temporaryUnpause = true;
	//}

	static bool FrameAdvanceKeyDown1_prev = false;
	static bool FrameAdvanceKeyDown2_prev = false;
	bool FrameAdvanceKeyDown = FrameAdvanceKeyDown1 || FrameAdvanceKeyDown2;
	bool FrameAdvanceKeyDown_justPressed = false;
	bool FrameAdvanceKeyDown_justReleased = false;
	if(FrameAdvanceKeyDown1_prev != FrameAdvanceKeyDown1)
	{
		FrameAdvanceKeyDown1_prev = FrameAdvanceKeyDown1;
		FrameAdvanceKeyDown_justPressed = FrameAdvanceKeyDown1;
		FrameAdvanceKeyDown_justReleased = !FrameAdvanceKeyDown1;
	}
	if(FrameAdvanceKeyDown2_prev != FrameAdvanceKeyDown2)
	{
		FrameAdvanceKeyDown2_prev = FrameAdvanceKeyDown2;
		FrameAdvanceKeyDown_justPressed = FrameAdvanceKeyDown2;
		FrameAdvanceKeyDown_justReleased = !FrameAdvanceKeyDown2;
	}

	int time = timeGetTime();
	if(FrameAdvanceKeyDown && started)
	{
		if(!paused)
		{
			// if pressed while unpaused, frame advance transitions to paused
			paused = true;
			CheckDialogChanges(frameCount);

			if(localTASflags.fastForward)
				temporaryUnpause = true;
		}
		else
		{
			if(FrameAdvanceKeyDown_justPressed)
			{
				// if already paused, pressing frame advance temporarily unpauses
				temporaryUnpause = true;
				lastFrameAdvanceTime = time;
			}
			else
			{
				// while paused, holding frame advance temporarily unpauses periodically
				// after some initial delay
				const int frameAdvanceInitialWait = 500;
				const int frameAdvanceSubsequentWait = localTASflags.fastForward ? 20 : 50;
				if(time - lastFrameAdvanceKeyUnheldTime > frameAdvanceInitialWait && 
				   time - lastFrameAdvanceTime > frameAdvanceSubsequentWait)
				{
					temporaryUnpause = true;
					lastFrameAdvanceTime = time;
				}
			}
		}

		if(FrameAdvanceKeyDown_justPressed)
		{
			localTASflags.frameAdvanceHeld = true;
			SendTASFlags();
		}
	}
	else
	{
		lastFrameAdvanceKeyUnheldTime = time;

		if(FrameAdvanceKeyDown_justReleased)
		{
			localTASflags.frameAdvanceHeld = false;
			SendTASFlags();
		}
	}
	//if(paused && CAPTUREINFO_TYPE_NONE_SUBSEQUENT
}





// sends to UpdateLoadedOrUnloadedDllHooks
void AddAndSendDllInfo(const char* filename, bool loaded, HANDLE hProcess)
{
	if(!loaded)
		return; // because UnInterceptUnloadingAPIs is disabled now

	if(dllLoadInfos.numInfos < (int)ARRAYSIZE(dllLoadInfos.infos) + (filename ? 0 : 1))
	{
		SIZE_T bytesRead = 0;
		if(dllLoadInfosSent)
			ReadProcessMemory(hProcess, remoteDllLoadInfos, &dllLoadInfos.numInfos, sizeof(dllLoadInfos.numInfos), &bytesRead);

		if(filename)
		{
			const char* slash = max(strrchr(filename, '\\'), strrchr(filename, '/'));
			const char* dllname = slash ? slash+1 : filename;
			// insert at start, since dll consumes from end
			memmove(&dllLoadInfos.infos[1], dllLoadInfos.infos, dllLoadInfos.numInfos*sizeof(*dllLoadInfos.infos));
			DllLoadInfo& info = dllLoadInfos.infos[0];
			strncpy(info.dllname, dllname, sizeof(info.dllname));
			info.dllname[sizeof(info.dllname)-1] = 0;
			info.loaded = loaded;
			dllLoadInfos.numInfos++;
		}

		SIZE_T bytesWritten = 0;
		if(WriteProcessMemory(hProcess, remoteDllLoadInfos, &dllLoadInfos, sizeof(dllLoadInfos), &bytesWritten))
			dllLoadInfosSent = true;
	}
}




void UpdateGeneralInfoDisplay()
{
	if(remoteGeneralInfoFromDll)
	{
		InfoForDebugger localGeneralInfoFromDll;

		SIZE_T bytesRead = 0;
		if(ReadProcessMemory(hGameProcess, remoteGeneralInfoFromDll, &localGeneralInfoFromDll, sizeof(InfoForDebugger), &bytesRead))
		{
			char str [256];
			char str2 [256];
//#ifdef _DEBUG
//			sprintf(str, "frame %d  " /*"tick %d  "*/ "delay %d  " "new %d",
//				localGeneralInfoFromDll.frames,
//				//localGeneralInfoFromDll.ticks,
//				localGeneralInfoFromDll.addedDelay,
//				localGeneralInfoFromDll.lastNewTicks);
//			GetWindowText(GetDlgItem(hWnd, IDC_TIMEINFO_LABEL), str2, 256);
//			if(strcmp(str,str2))
//				SetWindowText(GetDlgItem(hWnd, IDC_TIMEINFO_LABEL), str);
//#endif
			sprintf(str, "%u", localGeneralInfoFromDll.ticks);
			GetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str2, 256);
			if(strcmp(str,str2))
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str);
#if 0
#if _DEBUG
			static FILE* file = NULL;
			if(!file)
			{
				char fname [16];
				for(int i = 0; i < 10000; i++)
				{
					sprintf(fname, "C:\\a%d.txt", i);
					file = fopen(fname, "rb");
					if(file)
						fclose(file);
					else
						break;
				}
				file = fopen(fname, "wb");
			}
			if(file)
			{
				static int lastTicks = -1;
				if(lastTicks != localGeneralInfoFromDll.ticks)
				{
					lastTicks = localGeneralInfoFromDll.ticks;
					fprintf(file, "%s\r\n", str);
					fflush(file);
				}
			}
#endif
#endif
		}
	}
}


static int displayedFrameCount = -1;
static int displayedMaxFrameCount = -1;
void UpdateFrameCountDisplay(int frameCount, int frequency)
{
	if(frequency <= 0)
		frequency = 25;
	if(frameCount % frequency == 0)
	{
		int maxcount = movie.frames.size();
		if(maxcount == frameCount-1 && !paused && !localTASflags.playback)
			maxcount = frameCount; // hack to fix off-by-one display when not paused
		if(frameCount != displayedFrameCount || maxcount != displayedMaxFrameCount)
		{
			if(frameCount != displayedFrameCount)
			{
				char str [256];
				sprintf(str, "%d", frameCount);
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_CURFRAME), str);
				displayedFrameCount = frameCount;
			}
			if(maxcount != displayedMaxFrameCount)
			{
				char str [256];
				sprintf(str, "%d", maxcount);
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_MAXFRAME), str);
				displayedMaxFrameCount = maxcount;
			}
			{
				if(localTASflags.framerate > 0)
				{
					float seconds = fmodf((float)frameCount / localTASflags.framerate, 60.0f);
					float maxseconds = fmodf((float)maxcount / localTASflags.framerate, 60.0f);
					int minutes = frameCount / (60 * localTASflags.framerate) % 60;
					int maxminutes = maxcount / (60 * localTASflags.framerate) % 60;
					int hours = frameCount / (60*60 * localTASflags.framerate);
					int maxhours = maxcount / (60*60 * localTASflags.framerate);

					char str [256];
					sprintf(str, "%dh %dm %02.2fs   /   %dh %dm %02.2fs",
						hours, minutes, seconds,  maxhours, maxminutes, maxseconds);
					SetWindowText(GetDlgItem(hWnd, IDC_STATIC_MOVIETIME), str);
				}
				else
				{
					SetWindowText(GetDlgItem(hWnd, IDC_STATIC_MOVIETIME), "");
				}
			}

//#ifndef _DEBUG
			UpdateGeneralInfoDisplay();
//#endif
		}
	}
//#ifdef _DEBUG
//	UpdateGeneralInfoDisplay();
//#endif
}


static bool displayed_checkdialog_inited = false;
static bool displayed_paused = false;
static bool displayed_fastforward = false;
static bool displayed_started = false;
static bool displayed_playback = false;
static bool displayed_finished = false;
static bool displayed_nextLoadRecords = false;
static int displayed_usedThreadMode = -1;
static int displayed_threadMode = -1;
static int displayed_waitSyncMode = -1;
static int displayed_muteMode = -1;
void CheckDialogChanges(int frameCount)
{
	if(!displayed_checkdialog_inited ||
		started != displayed_started ||
		localTASflags.playback != displayed_playback ||
		finished != displayed_finished)
	{
		displayed_started = started;
		displayed_playback = localTASflags.playback;
		displayed_finished = finished;
		const char* str;
		if(started)
			if(localTASflags.playback)
				if(finished)
					str = "Current Status: Finished";
				else
					str = "Current Status: Playing";
			else
				str = "Current Status: Recording";
		else
			str = "Current Status: Inactive";
		SetWindowText(GetDlgItem(hWnd, IDC_STATIC_MOVIESTATUS), str);
		InvalidateRect(GetDlgItem(hWnd, IDC_STATIC_FRAMESLASH), NULL, TRUE);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), EM_SETREADONLY, started, 0);
		EnableWindow(GetDlgItem(hWnd, IDC_CHECK_MOUSE), !started);
		EnableWindow(GetDlgItem(hWnd, IDC_CHECK_GAMEPAD), !started);
//		EnableWindow(GetDlgItem(hWnd, IDC_ALLOWHARDWAREACCEL), !started);
//		EnableWindow(GetDlgItem(hWnd, IDC_ALLOWFULLSCREEN), !started);
//		EnableWindow(GetDlgItem(hWnd, IDC_EMULATESOUND), !started);
		EnableWindow(GetDlgItem(hWnd, IDC_AVIAUDIO), !started || (localTASflags.emuMode & EMUMODE_EMULATESOUND));
		//EnableWindow(GetDlgItem(hWnd, IDC_EARLYINJECT), !started);
		if(!started)
		{
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_THREAD_DISABLE), !started);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_THREAD_WRAP), !started);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_THREAD_ALLOW), !started);
		}
		SendMessage(GetDlgItem(hWnd, IDC_TEXT_EXE), EM_SETREADONLY, started, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), EM_SETREADONLY, started, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_MOVIE), EM_SETREADONLY, started, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), EM_SETREADONLY, started, 0);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_MOVIEBROWSE), !started);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_GAMEBROWSE), !started);
		mainMenuNeedsRebuilding = true;
		if(frameCount >= 0)
			UpdateFrameCountDisplay(frameCount, 1);
		displayed_checkdialog_inited = 0;
	}
	if(!displayed_checkdialog_inited || displayed_nextLoadRecords != nextLoadRecords)
	{
		displayed_nextLoadRecords = nextLoadRecords;
		if(nextLoadRecords)
			CheckRadioButton(hWnd, IDC_RADIO_READONLY, IDC_RADIO_READWRITE, IDC_RADIO_READWRITE);
		else
			CheckRadioButton(hWnd, IDC_RADIO_READONLY, IDC_RADIO_READWRITE, IDC_RADIO_READONLY);
	}
	if(!displayed_checkdialog_inited || displayed_paused != paused)
	{
		displayed_paused = paused;
#if 0
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_CURFRAME), EM_SETREADONLY, !paused && started, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_MAXFRAME), EM_SETREADONLY, !paused && started, 0);
#else
		// FIXME: editing curframe/maxframe is NYI
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_CURFRAME), EM_SETREADONLY, TRUE, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_MAXFRAME), EM_SETREADONLY, TRUE, 0);
#endif
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_FPS), EM_SETREADONLY, !paused && started, 0);
		CheckDlgButton(hWnd, IDC_PAUSED, paused);
	}
	if(!displayed_checkdialog_inited || displayed_fastforward != localTASflags.fastForward)
	{
		displayed_fastforward = localTASflags.fastForward;
		CheckDlgButton(hWnd, IDC_FASTFORWARD, localTASflags.fastForward);
	}
	if(!displayed_checkdialog_inited || displayed_usedThreadMode != usedThreadMode && usedThreadMode >= 0)
	{
		displayed_usedThreadMode = usedThreadMode;
		if(usedThreadMode == 0 || usedThreadMode == 1)
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_THREAD_ALLOW), !started);
		if(usedThreadMode == 0 || usedThreadMode == 2)
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_THREAD_WRAP), !started);
	}
	if(!displayed_checkdialog_inited || displayed_threadMode != localTASflags.threadMode || displayed_waitSyncMode != localTASflags.waitSyncMode)
	{
		displayed_threadMode = localTASflags.threadMode;
		displayed_waitSyncMode = localTASflags.waitSyncMode;
		CheckDlgButton(hWnd, IDC_RADIO_THREAD_DISABLE, localTASflags.threadMode == 0 && localTASflags.waitSyncMode == 1);
		CheckDlgButton(hWnd, IDC_RADIO_THREAD_WRAP, localTASflags.threadMode == 1 && localTASflags.waitSyncMode == 1);
		CheckDlgButton(hWnd, IDC_RADIO_THREAD_ALLOW, localTASflags.threadMode == 2 && localTASflags.waitSyncMode == 2);
	}
	if(displayed_muteMode != ((localTASflags.emuMode & EMUMODE_NOPLAYBUFFERS) ? 1 : 0))
	{
		displayed_muteMode = ((localTASflags.emuMode & EMUMODE_NOPLAYBUFFERS) ? 1 : 0);
		CheckDlgButton(hWnd, IDC_CHECK_MUTE, displayed_muteMode);
	}
	displayed_checkdialog_inited = true;


	// now this is important
	if(tasFlagsDirty)
		SendTASFlags();
}

static int s_lastFrameCount = 0;

//template<typename T>
//void Swap(T& a, T& b)
//{
//	T t(a);
//	a = b;
//	b = t;
//}



void DoPow2Logic(int frameCount);

void SleepFrameBoundary(const char* frameInfo, int threadId)
{
	HandleAviSplitRequests();

	if(localTASflags.aviMode & 1)
		ProcessCaptureFrameInfo(NULL, CAPTUREINFO_TYPE_PREV);
	if(localTASflags.aviMode & 2)
		ProcessCaptureSoundInfo();
}

void FrameBoundary(const char* frameInfo, int threadId)
{
	int frameCount;
	void* frameCaptureInfoRemoteAddr;
	int frameCaptureInfoType;
	sscanf(frameInfo, " %d %p %d", &frameCount, &frameCaptureInfoRemoteAddr, &frameCaptureInfoType);

	if(!requestedCommandReenter)
	{
		// if it's actually a new frame instead of just paused

		s_lastFrameCount = frameCount;
		int frameCountUpdateFreq = localTASflags.framerate/2;
		if(localTASflags.fastForward && !(localTASflags.aviMode & 1))
			frameCountUpdateFreq = localTASflags.framerate*8;
		UpdateFrameCountDisplay(frameCount, frameCountUpdateFreq);

		// temp workaround for games that currently spray pixels onscreen when they boot up (cave story)
		if(frameCount == 1)
			InvalidateRect(hWnd, NULL, TRUE);

		HandleAviSplitRequests();

		if(frameCaptureInfoType != CAPTUREINFO_TYPE_NONE_SUBSEQUENT)
		{
			// handle AVI recording
			if(localTASflags.aviMode & 1)
				ProcessCaptureFrameInfo(frameCaptureInfoRemoteAddr, frameCaptureInfoType);
			if(localTASflags.aviMode & 2)
				ProcessCaptureSoundInfo();
		}

		// update ram search/watch windows
		Update_RAM_Search();

		// handle skipping lag frames if that option is enabled
		temporaryUnpause = (frameCaptureInfoType == CAPTUREINFO_TYPE_PREV) && advancePastNonVideoFrames;
	}
	else
	{
		temporaryUnpause = false;
	}

	if(requestedCommandReenter && !cannotSuspendForCommand)
		ResumeAllExcept(threadId);

	// handle hotkeys
	do
	{
		requestedCommandReenter = false;
		cannotSuspendForCommand = false;
		RefreshSavestates(frameCount);
		if(!requestedCommandReenter)
			CheckHotkeys(frameCount, true);
		CheckDialogChanges(frameCount);
		if(paused)
		{
			UpdateFrameCountDisplay(frameCount, 1);
			if(!requestedCommandReenter)
				Sleep(5);
			if(!temporaryUnpause && !requestedCommandReenter) // check to avoid breaking... everything
				HandleRemotePauseEvents();
		}

	} while(paused && !(temporaryUnpause || requestedCommandReenter));

	if(requestedCommandReenter && !cannotSuspendForCommand)
		SuspendAllExcept(threadId);

	// handle movie playback / recording
	if(!requestedCommandReenter)
	{
 		if(!localTASflags.playback)
			RecordLocalInputs();
		//else
		//	MultitrackHack();
		InjectCurrentMovieFrame();
		if(!(frameCount & (frameCount-1)))
			DoPow2Logic(frameCount);
	}
}

static int s_firstTimerDesyncWarnedFrame = 0x7FFFFFFF;

void DoPow2Logic(int frameCount)
{
	int which = 0;
	while((1 << which) < frameCount && which < 16) {
		which++;
	}
	if(which < 16 && !finished)
	{
		if(remoteGeneralInfoFromDll)
		{
			InfoForDebugger localGeneralInfoFromDll;
			SIZE_T bytesRead = 0;
			if(ReadProcessMemory(hGameProcess, remoteGeneralInfoFromDll, &localGeneralInfoFromDll, sizeof(InfoForDebugger), &bytesRead))
			{
				int movieTime = movie.desyncDetectionTimerValues[which];
				if(movieTime != localGeneralInfoFromDll.ticks)
				{
					if(localTASflags.playback && movieTime && s_firstTimerDesyncWarnedFrame >= frameCount)
					{
						s_firstTimerDesyncWarnedFrame = frameCount;
						char str [256];
						sprintf(str, "DESYNC DETECTED: on frame %d, your timer = %d but movie's timer = %d.\nThat means this playback of the movie desynced somewhere between frames %d and %d.", frameCount, localGeneralInfoFromDll.ticks, movieTime, frameCount, frameCount>>1);
						debugprintf("%s\n", str);
						CustomMessageBox(str, "Desync Warning", MB_OK | MB_ICONWARNING);
					}
					movie.desyncDetectionTimerValues[which] = localGeneralInfoFromDll.ticks;
				}
			}
		}
	}

}

static char dllLeaveAloneList [256][MAX_PATH+1];

//void SupplyDllList(int writeAddressInt)
//{
//	char* writeAddress = (char*)writeAddressInt;
//
//	char filename [MAX_PATH+1];
//
//	int i = 0;
//	std::map<LPVOID,HANDLE>::iterator iter; 
//	for(iter = dllBaseToHandle.begin(); iter != dllBaseToHandle.end(); iter++)
//	{
//		GetFileNameFromFileHandle(iter->second, filename);
//		const char* pfilename = strrchr(filename, '\\') + 1;
//		if(pfilename == (const char*)1)
//			pfilename = filename;
//
//		if(i < 256)
//			strcpy(dllLeaveAloneList[i++], pfilename);
//	}
//	if(i < 256)
//		strcpy(dllLeaveAloneList[i++], "SETUPAPI.DLL"); // the sound lags behind the action if this isn't in the list
//	if(i < 256)
//		strcpy(dllLeaveAloneList[i++], "uxtheme.dll"); // windows xp theme is disabled if this isn't in the list
//	while(i < 256)
//		*dllLeaveAloneList[i++] = 0;
//
//	SIZE_T bytesWritten = 0;
//	if(!WriteProcessMemory(hGameProcess, writeAddress, dllLeaveAloneList, sizeof(dllLeaveAloneList), &bytesWritten))
//	{
//		debugprintf("FAILED TO SUPPLY DLL LIST TO 0x%08X", writeAddressInt);
//	}
//}


void SuspendAllExcept(int ignoreThreadID)
{
	bool ok = false;
	std::map<DWORD,ThreadInfo>::iterator iter;
	for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
	{
		int id = iter->first;
		if(id == ignoreThreadID)
		{
			ok = true;
			continue;
		}
		HANDLE handle = iter->second;
		SuspendThread(handle);
	}
	if(!ok)
	{
		static int first = 1;
		if(first && ignoreThreadID)
			debugprintf("ERROR: suspendall failed to find requesting thread id=0x%X in list.\n", ignoreThreadID);
		first = 0;
	}
}
void ResumeAllExcept(int ignoreThreadID)
{
	bool ok = false;
	std::map<DWORD,ThreadInfo>::iterator iter;
	for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
	{
		int id = iter->first;
		if(id == ignoreThreadID)
		{
			ok = true;
			continue;
		}
		HANDLE handle = iter->second;
		ResumeThread(handle);
	}
	if(!ok)
	{
		static int first = 1;
		if(first)
			debugprintf("ERROR: resumeall failed to find requesting thread id=0x%X in list.\n", ignoreThreadID);
		first = 0;
	}
}

void ReceiveFrameRate(const char* fpsAsString)
{
	float fps = 0;
	float logicalfps = 0;
	sscanf(fpsAsString, "%g %g", &fps, &logicalfps);
	char str [256];
	sprintf(str, "Current FPS: %.1f / %.1f", fps, logicalfps);
	const char* pStr = str;
	if(strlen(str) > 24)
		pStr += 8; // omit "Current" if the string is getting too long
	SetWindowText(GetDlgItem(hWnd, IDC_STATIC_CURRENTFPS), pStr);

	goingsuperfast = (fps >= 300);
}

void ReceiveHWND(DWORD hwndAsInt)
{
	HWND hwnd = (HWND)hwndAsInt;
	{
		AutoCritSect cs(&g_gameHWndsCS);
		gameHWnds.insert(hwnd);
	}
}

#if 0 // code for PrintPrivileges, disabled since it was only for diagnostic purposes, maybe useful for debugging privilege problems if they happen on a new OS
void GetPaclMask(PACL pacl, DWORD& maskToMerge)
{
	if(pacl)
	{
		for(int i = 0; i < pacl->AceCount; i++)
		{
			ACE_HEADER* aceheader;
			if (!GetAce(pacl, i, (LPVOID*) &aceheader))
				continue;
			switch(aceheader->AceType)
			{
			case ACCESS_ALLOWED_ACE_TYPE:
			case ACCESS_DENIED_ACE_TYPE:
				{
					ACCESS_ALLOWED_ACE* ace = (ACCESS_ALLOWED_ACE*)aceheader;
					if(aceheader->AceType == ACCESS_ALLOWED_ACE_TYPE)
						maskToMerge |= ace->Mask;
					else
						maskToMerge &= ~ace->Mask;
				}
				break;
			}
		}
	}
}

void PrintAceMask(DWORD aceMask)
{
	debugprintf("%s: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s (0x%X)\n",
		/*aceheader->AceType == ACCESS_ALLOWED_ACE_TYPE ?*/ "ACCESS_ALLOWED" /*: "ACCESS_DENIED"*/,
		(aceMask & GENERIC_ALL) ? "ALL" : "",
		(aceMask & GENERIC_READ) ? "READ" : "",
		(aceMask & GENERIC_WRITE) ? "WRITE" : "",
		(aceMask & GENERIC_EXECUTE) ? "EXECUTE" : "",
		(aceMask & DELETE) ? "delete" : "",
		(aceMask & READ_CONTROL) ? "read_control" : "",
		(aceMask & WRITE_DAC) ? "write_dac" : "",
		(aceMask & WRITE_OWNER) ? "write_owner" : "",
		(aceMask & SYNCHRONIZE) ? "synchronize" : "",
		(aceMask & PROCESS_TERMINATE) ? "terminate" : "",
		(aceMask & PROCESS_CREATE_THREAD) ? "create_thread" : "",
		(aceMask & PROCESS_SET_SESSIONID) ? "set_sessionid" : "",
		(aceMask & PROCESS_VM_OPERATION ) ? "vm_operation" : "",
		(aceMask & PROCESS_VM_READ      ) ? "vm_read" : "",
		(aceMask & PROCESS_VM_WRITE     ) ? "vm_write" : "",
		(aceMask & PROCESS_DUP_HANDLE   ) ? "dup_handle" : "",
		(aceMask & PROCESS_CREATE_PROCESS) ? "create_process" : "",
		(aceMask & PROCESS_SET_QUOTA     ) ? "set_quota" : "",
		(aceMask & PROCESS_SET_INFORMATION  ) ? "set_information" : "",
		(aceMask & PROCESS_QUERY_INFORMATION) ? "query_information" : "",
		(aceMask & PROCESS_SUSPEND_RESUME   ) ? "suspend_resume" : "",
		aceMask);
}

void PrintPrivileges(HANDLE hProcess)
{
	debugprintf("privileges for process 0x%X:\n", hProcess);

	DWORD mask = 0;
	if(hProcess)
	{
		PACL pacl;
		PSECURITY_DESCRIPTOR psd;
		DWORD res = GetSecurityInfo(hProcess, SE_KERNEL_OBJECT,
			DACL_SECURITY_INFORMATION, NULL, NULL, &pacl, NULL, &psd);
		GetPaclMask(pacl, mask);
	}

	HANDLE htok;
	if(OpenProcessToken(hProcess, TOKEN_QUERY, &htok))
	{
		DWORD length = 0;
		GetTokenInformation(htok, TokenDefaultDacl, NULL, 0, &length);
		PTOKEN_DEFAULT_DACL pdacls = (PTOKEN_DEFAULT_DACL)alloca(length);
		if(GetTokenInformation(htok, TokenDefaultDacl, pdacls, length, &length))
		{
			TOKEN_DEFAULT_DACL& dacl = *pdacls;
			GetPaclMask(dacl.DefaultDacl, mask);
		}
	}

	PrintAceMask(mask);
}
#else
void PrintPrivileges(HANDLE hProcess) {}
#endif

static void AbsolutifyPath(char* buf)
{
	SearchPath(NULL, buf, NULL, MAX_PATH, buf, NULL);
	NormalizePath(buf, buf);
}

const char* ExceptionCodeToDescription(DWORD code, const char* defaultRv="(unknown exception code)")
{
	switch(code & (DWORD)0xCFFFFFFF)
	{
		// custom/nonstandard/undocumented exceptions
		case /*DELPHI_RUNTIME_ERROR*/(DWORD)0x0EEDFADE: return "Delphi runtime error.";
		case /*EXCEPTION_MSVC*/(DWORD)0xC06D7363: return "exception inserted by Visual C++, often used for SEH";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0524F54: return "EXCEPTION_COMPLUS (ROT)";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0434352: return "EXCEPTION_COMPLUS (CCR)";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0434F4D: return "EXCEPTION_COMPLUS (COM)";
		case /*EXCEPTION_HIJACK*/(DWORD)0xC0434F4E: return "Resuming thread after garbage collection.";
		case /*EXCEPTION_SOFTSO*/(DWORD)0xC053534F: return "Stack overflow in managed code.";
		case /*EXCEPTION_EXX*/(DWORD)0xC0455858: return "SetupThread PInvoke failure.";
		case (DWORD)0xC0440001: return "D runtime Win32 exception";
		default: return defaultRv;

		// standard status or exception codes
		case /*STATUS_PENDING*/(DWORD)0x00000103L: return "The operation that was requested is pending completion.";
		case /*STATUS_REPARSE*/(DWORD)0x00000104L: return "A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.";
		case /*STATUS_MORE_ENTRIES*/(DWORD)0x00000105L: return "Returned by enumeration APIs to indicate more information is available to successive calls.";
		case /*STATUS_NOT_ALL_ASSIGNED*/(DWORD)0x00000106L: return "Indicates not all privileges or groups referenced are assigned to the caller. This allows, for example, all privileges to be disabled without having to know exactly which privileges are assigned.";
		case /*STATUS_SOME_NOT_MAPPED*/(DWORD)0x00000107L: return "Some of the information to be translated has not been translated.";
		case /*STATUS_OPLOCK_BREAK_IN_PROGRESS*/(DWORD)0x00000108L: return "An open/create operation completed while an oplock break is underway.";
		case /*STATUS_VOLUME_MOUNTED*/(DWORD)0x00000109L: return "A new volume has been mounted by a file system.";
		case /*STATUS_RXACT_COMMITTED*/(DWORD)0x0000010AL: return "This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed.";
		case /*STATUS_NOTIFY_CLEANUP*/(DWORD)0x0000010BL: return "This indicates that a notify change request has been completed due to closing the handle which made the notify change request.";
		case /*STATUS_NOTIFY_ENUM_DIR*/(DWORD)0x0000010CL: return "This indicates that a notify change request is being completed and that the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.";
		case /*STATUS_NO_QUOTAS_FOR_ACCOUNT*/(DWORD)0x0000010DL: return "{No Quotas} No system quota limits are specifically set for this account.";
		case /*STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED*/(DWORD)0x0000010EL: return "{Connect Failure on Primary Transport} An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport.";
		case /*STATUS_PAGE_FAULT_TRANSITION*/(DWORD)0x00000110L: return "Page fault was a transition fault.";
		case /*STATUS_PAGE_FAULT_DEMAND_ZERO*/(DWORD)0x00000111L: return "Page fault was a demand zero fault.";
		case /*STATUS_PAGE_FAULT_COPY_ON_WRITE*/(DWORD)0x00000112L: return "Page fault was a demand zero fault.";
		case /*STATUS_PAGE_FAULT_GUARD_PAGE*/(DWORD)0x00000113L: return "Page fault was a demand zero fault.";
		case /*STATUS_PAGE_FAULT_PAGING_FILE*/(DWORD)0x00000114L: return "Page fault was satisfied by reading from a secondary storage device.";
		case /*STATUS_CACHE_PAGE_LOCKED*/(DWORD)0x00000115L: return "Cached page was locked during operation.";
		case /*STATUS_CRASH_DUMP*/(DWORD)0x00000116L: return "Crash dump exists in paging file.";
		case /*STATUS_BUFFER_ALL_ZEROS*/(DWORD)0x00000117L: return "Specified buffer contains all zeros.";
		case /*STATUS_REPARSE_OBJECT*/(DWORD)0x00000118L: return "A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.";
		case /*STATUS_RESOURCE_REQUIREMENTS_CHANGED*/(DWORD)0x00000119L: return "The device has succeeded a query-stop and its resource requirements have changed.";
		case /*STATUS_TRANSLATION_COMPLETE*/(DWORD)0x00000120L: return "The translator has translated these resources into the global space and no further translations should be performed.";
		case /*STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY*/(DWORD)0x00000121L: return "The directory service evaluated group memberships locally, as it was unable to contact a global catalog server.";
		case /*STATUS_NOTHING_TO_TERMINATE*/(DWORD)0x00000122L: return "A process being terminated has no threads to terminate.";
		case /*STATUS_PROCESS_NOT_IN_JOB*/(DWORD)0x00000123L: return "The specified process is not part of a job.";
		case /*STATUS_PROCESS_IN_JOB*/(DWORD)0x00000124L: return "The specified process is part of a job.";
		case /*STATUS_VOLSNAP_HIBERNATE_READY*/(DWORD)0x00000125L: return "{Volume Shadow Copy Service} The system is now ready for hibernation.";
		case /*STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY*/(DWORD)0x00000126L: return "A file system or file system filter driver has successfully completed an FsFilter operation.";
		case /*STATUS_INTERRUPT_VECTOR_ALREADY_CONNECTED*/(DWORD)0x00000127L: return "The specified interrupt vector was already connected.";
		case /*STATUS_INTERRUPT_STILL_CONNECTED*/(DWORD)0x00000128L: return "The specified interrupt vector is still connected.";
		case /*STATUS_PROCESS_CLONED*/(DWORD)0x00000129L: return "The current process is a cloned process.";
		case /*STATUS_FILE_LOCKED_WITH_ONLY_READERS*/(DWORD)0x0000012AL: return "The file was locked and all users of the file can only read.";
		case /*STATUS_FILE_LOCKED_WITH_WRITERS*/(DWORD)0x0000012BL: return "The file was locked and at least one user of the file can write.";
		case /*STATUS_RESOURCEMANAGER_READ_ONLY*/(DWORD)0x00000202L: return "The specified ResourceManager made no changes or updates to the resource under this transaction.";
		case /*DBG_EXCEPTION_HANDLED*/(DWORD)0x00010001L: return "Debugger handled exception";
		case /*DBG_CONTINUE*/(DWORD)0x00010002L: return "Debugger continued";
		case /*STATUS_FLT_IO_COMPLETE*/(DWORD)0x001C0001L: return "The IO was completed by a filter.";
		case /*STATUS_OBJECT_NAME_EXISTS*/(DWORD)0x40000000L: return "{Object Exists} An attempt was made to create an object and the object name already existed.";
		case /*STATUS_THREAD_WAS_SUSPENDED*/(DWORD)0x40000001L: return "{Thread Suspended} A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.";
		case /*STATUS_WORKING_SET_LIMIT_RANGE*/(DWORD)0x40000002L: return "{Working Set Range Error} An attempt was made to set the working set minimum or maximum to values which are outside of the allowable range.";
		case /*STATUS_IMAGE_NOT_AT_BASE*/(DWORD)0x40000003L: return "{Image Relocated} An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image.";
		case /*STATUS_RXACT_STATE_CREATED*/(DWORD)0x40000004L: return "This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created.";
		case /*STATUS_SEGMENT_NOTIFICATION*/(DWORD)0x40000005L: return "{Segment Load} A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments.";
		case /*STATUS_LOCAL_USER_SESSION_KEY*/(DWORD)0x40000006L: return "{Local Session Key} A user session key was requested for a local RPC connection. The session key returned is a constant value and not unique to this connection.";
		case /*STATUS_BAD_CURRENT_DIRECTORY*/(DWORD)0x40000007L: return "{Invalid Current Directory} The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit.";
		case /*STATUS_SERIAL_MORE_WRITES*/(DWORD)0x40000008L: return "{Serial IOCTL Complete} A serial I/O operation was completed by another write to a serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.)";
		case /*STATUS_REGISTRY_RECOVERED*/(DWORD)0x40000009L: return "{Registry Recovery} One of the files containing the system's Registry data had to be recovered by use of a log or alternate copy. The recovery was successful.";
		case /*STATUS_FT_READ_RECOVERY_FROM_BACKUP*/(DWORD)0x4000000AL: return "{Redundant Read} To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device.";
		case /*STATUS_FT_WRITE_RECOVERY*/(DWORD)0x4000000BL: return "{Redundant Write} To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device.";
		case /*STATUS_SERIAL_COUNTER_TIMEOUT*/(DWORD)0x4000000CL: return "{Serial IOCTL Timeout} A serial I/O operation completed because the time-out period expired. (The IOCTL_SERIAL_XOFF_COUNTER had not reached zero.)";
		case /*STATUS_NULL_LM_PASSWORD*/(DWORD)0x4000000DL: return "{Password Too Complex} The Windows password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.";
		case /*STATUS_IMAGE_MACHINE_TYPE_MISMATCH*/(DWORD)0x4000000EL: return "{Machine Type Mismatch} The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.";
		case /*STATUS_RECEIVE_PARTIAL*/(DWORD)0x4000000FL: return "{Partial Data Received} The network transport returned partial data to its client. The remaining data will be sent later.";
		case /*STATUS_RECEIVE_EXPEDITED*/(DWORD)0x40000010L: return "{Expedited Data Received} The network transport returned data to its client that was marked as expedited by the remote system.";
		case /*STATUS_RECEIVE_PARTIAL_EXPEDITED*/(DWORD)0x40000011L: return "{Partial Expedited Data Received} The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later.";
		case /*STATUS_EVENT_DONE*/(DWORD)0x40000012L: return "{TDI Event Done} The TDI indication has completed successfully.";
		case /*STATUS_EVENT_PENDING*/(DWORD)0x40000013L: return "{TDI Event Pending} The TDI indication has entered the pending state.";
		case /*STATUS_CHECKING_FILE_SYSTEM*/(DWORD)0x40000014L: return "Checking file system on %wZ";
		case /*STATUS_FATAL_APP_EXIT*/(DWORD)0x40000015L: return "{Fatal Application Exit} %hs";
		case /*STATUS_PREDEFINED_HANDLE*/(DWORD)0x40000016L: return "The specified registry key is referenced by a predefined handle.";
		case /*STATUS_WAS_UNLOCKED*/(DWORD)0x40000017L: return "{Page Unlocked} The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process.";
		case /*STATUS_SERVICE_NOTIFICATION*/(DWORD)0x40000018L: return "SERVICE_NOTIFICATION %hs";
		case /*STATUS_WAS_LOCKED*/(DWORD)0x40000019L: return "{Page Locked} One of the pages to lock was already locked.";
		case /*STATUS_LOG_HARD_ERROR*/(DWORD)0x4000001AL: return "Application popup:";
		case /*STATUS_ALREADY_WIN32*/(DWORD)0x4000001BL: return "ALREADY_WIN32";
		case /*STATUS_WX86_UNSIMULATE*/(DWORD)0x4000001CL: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_CONTINUE*/(DWORD)0x4000001DL: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_SINGLE_STEP*/(DWORD)0x4000001EL: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_BREAKPOINT*/(DWORD)0x4000001FL: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_EXCEPTION_CONTINUE*/(DWORD)0x40000020L: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_EXCEPTION_LASTCHANCE*/(DWORD)0x40000021L: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_EXCEPTION_CHAIN*/(DWORD)0x40000022L: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE*/(DWORD)0x40000023L: return "{Machine Type Mismatch} The image file %hs is valid, but is for a machine type other than the current machine.";
		case /*STATUS_NO_YIELD_PERFORMED*/(DWORD)0x40000024L: return "A yield execution was performed and no thread was available to run.";
		case /*STATUS_TIMER_RESUME_IGNORED*/(DWORD)0x40000025L: return "The resumable flag to a timer API was ignored.";
		case /*STATUS_ARBITRATION_UNHANDLED*/(DWORD)0x40000026L: return "The arbiter has deferred arbitration of these resources to its parent";
		case /*STATUS_CARDBUS_NOT_SUPPORTED*/(DWORD)0x40000027L: return "The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller.";
		case /*STATUS_WX86_CREATEWX86TIB*/(DWORD)0x40000028L: return "Exception status code used by Win32 x86 emulation subsystem.";
		case /*STATUS_MP_PROCESSOR_MISMATCH*/(DWORD)0x40000029L: return "The CPUs in this multiprocessor system are not all the same revision level.  To use all processors the operating system restricts itself to the features of the least capable processor in the system.  Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported.";
		case /*STATUS_HIBERNATED*/(DWORD)0x4000002AL: return "The system was put into hibernation.";
		case /*STATUS_RESUME_HIBERNATION*/(DWORD)0x4000002BL: return "The system was resumed from hibernation.";
		case /*STATUS_FIRMWARE_UPDATED*/(DWORD)0x4000002CL: return "Windows has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3].";
		case /*STATUS_DRIVERS_LEAKING_LOCKED_PAGES*/(DWORD)0x4000002DL: return "A device driver is leaking locked I/O pages causing system degradation.  The system has automatically enabled tracking code in order to try and catch the culprit.";
		case /*STATUS_MESSAGE_RETRIEVED*/(DWORD)0x4000002EL: return "The ALPC message being canceled has already been retrieved from the queue on the other side.";
		case /*STATUS_SYSTEM_POWERSTATE_TRANSITION*/(DWORD)0x4000002FL: return "The system powerstate is transitioning from %2 to %3.";
		case /*STATUS_ALPC_CHECK_COMPLETION_LIST*/(DWORD)0x40000030L: return "The receive operation was successful. Check the ALPC completion list for the received message.";
		case /*STATUS_SYSTEM_POWERSTATE_COMPLEX_TRANSITION*/(DWORD)0x40000031L: return "The system powerstate is transitioning from %2 to %3 but could enter %4.";
		case /*STATUS_ACCESS_AUDIT_BY_POLICY*/(DWORD)0x40000032L: return "Access to %1 is monitored by policy rule %2.";
		case /*STATUS_ABANDON_HIBERFILE*/(DWORD)0x40000033L: return "A valid hibernation file has been invalidated and should be abandoned.";
		case /*STATUS_BIZRULES_NOT_ENABLED*/(DWORD)0x40000034L: return "Business rule scripts are disabled for the calling application.";
		case /*DBG_REPLY_LATER*/(DWORD)0x40010001L: return "Debugger will reply later.";
		case /*DBG_UNABLE_TO_PROVIDE_HANDLE*/(DWORD)0x40010002L: return "Debugger cannot provide handle.";
		case /*DBG_TERMINATE_THREAD*/(DWORD)0x40010003L: return "Debugger terminated thread.";
		case /*DBG_TERMINATE_PROCESS*/(DWORD)0x40010004L: return "Debugger terminated process.";
		case /*DBG_CONTROL_C*/(DWORD)0x40010005L: return "Debugger got control C.";
		case /*DBG_PRINTEXCEPTION_C*/(DWORD)0x40010006L: return "Debugger printed exception on control C.";
		case /*DBG_RIPEXCEPTION*/(DWORD)0x40010007L: return "Debugger received RIP exception.";
		case /*DBG_CONTROL_BREAK*/(DWORD)0x40010008L: return "Debugger received control break.";
		case /*DBG_COMMAND_EXCEPTION*/(DWORD)0x40010009L: return "Debugger command communication exception.";
		case /*STATUS_FLT_BUFFER_TOO_SMALL*/(DWORD)0x801C0001L: return "{Buffer too small} The buffer is too small to contain the entry. No information has been written to the buffer.";
		case /*STATUS_GUARD_PAGE_VIOLATION*/(DWORD)0x80000001L: return "{GUARD_PAGE_VIOLATION} Guard Page Exception. A page of memory that marks the end of a data structure, such as a stack or an array, has been accessed.";
		case /*STATUS_DATATYPE_MISALIGNMENT*/(DWORD)0x80000002L: return "{DATATYPE_MISALIGNMENT} Alignment Fault. A datatype misalignment was detected in a load or store instruction.";
		case /*STATUS_BREAKPOINT*/(DWORD)0x80000003L: return "{BREAKPOINT} A breakpoint has been reached.";
		case /*STATUS_SINGLE_STEP*/(DWORD)0x80000004L: return "{SINGLE_STEP} A single step or trace operation has just been completed.";
		case /*STATUS_BUFFER_OVERFLOW*/(DWORD)0x80000005L: return "{Buffer Overflow} The data was too large to fit into the specified buffer.";
		case /*STATUS_NO_MORE_FILES*/(DWORD)0x80000006L: return "{No More Files} No more files were found which match the file specification.";
		case /*STATUS_WAKE_SYSTEM_DEBUGGER*/(DWORD)0x80000007L: return "{Kernel Debugger Awakened} the system debugger was awakened by an interrupt.";
		case /*STATUS_HANDLES_CLOSED*/(DWORD)0x8000000AL: return "{Handles Closed} Handles to objects have been automatically closed as a result of the requested operation.";
		case /*STATUS_NO_INHERITANCE*/(DWORD)0x8000000BL: return "{Non-Inheritable ACL} An access control list (ACL) contains no components that can be inherited.";
		case /*STATUS_GUID_SUBSTITUTION_MADE*/(DWORD)0x8000000CL: return "{GUID Substitution} During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended.";
		case /*STATUS_PARTIAL_COPY*/(DWORD)0x8000000DL: return "{Partial Copy} Due to protection conflicts not all the requested bytes could be copied.";
		case /*STATUS_DEVICE_PAPER_EMPTY*/(DWORD)0x8000000EL: return "{Out of Paper} The printer is out of paper.";
		case /*STATUS_DEVICE_POWERED_OFF*/(DWORD)0x8000000FL: return "{Device Power Is Off} The printer power has been turned off.";
		case /*STATUS_DEVICE_OFF_LINE*/(DWORD)0x80000010L: return "{Device Offline} The printer has been taken offline.";
		case /*STATUS_DEVICE_BUSY*/(DWORD)0x80000011L: return "{Device Busy} The device is currently busy.";
		case /*STATUS_NO_MORE_EAS*/(DWORD)0x80000012L: return "{No More EAs} No more extended attributes (EAs) were found for the file.";
		case /*STATUS_INVALID_EA_NAME*/(DWORD)0x80000013L: return "{Illegal EA} The specified extended attribute (EA) name contains at least one illegal character.";
		case /*STATUS_EA_LIST_INCONSISTENT*/(DWORD)0x80000014L: return "{Inconsistent EA List} The extended attribute (EA) list is inconsistent.";
		case /*STATUS_INVALID_EA_FLAG*/(DWORD)0x80000015L: return "{Invalid EA Flag} An invalid extended attribute (EA) flag was set.";
		case /*STATUS_VERIFY_REQUIRED*/(DWORD)0x80000016L: return "{Verifying Disk} The media has changed and a verify operation is in progress so no reads or writes may be performed to the device, except those used in the verify operation.";
		case /*STATUS_EXTRANEOUS_INFORMATION*/(DWORD)0x80000017L: return "{Too Much Information} The specified access control list (ACL) contained more information than was expected.";
		case /*STATUS_RXACT_COMMIT_NECESSARY*/(DWORD)0x80000018L: return "This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired).";
		case /*STATUS_NO_MORE_ENTRIES*/(DWORD)0x8000001AL: return "{No More Entries} No more entries are available from an enumeration operation.";
		case /*STATUS_FILEMARK_DETECTED*/(DWORD)0x8000001BL: return "{Filemark Found} A filemark was detected.";
		case /*STATUS_MEDIA_CHANGED*/(DWORD)0x8000001CL: return "{Media Changed} The media may have changed.";
		case /*STATUS_BUS_RESET*/(DWORD)0x8000001DL: return "{I/O Bus Reset} An I/O bus reset was detected.";
		case /*STATUS_END_OF_MEDIA*/(DWORD)0x8000001EL: return "{End of Media} The end of the media was encountered.";
		case /*STATUS_BEGINNING_OF_MEDIA*/(DWORD)0x8000001FL: return "Beginning of tape or partition has been detected.";
		case /*STATUS_MEDIA_CHECK*/(DWORD)0x80000020L: return "{Media Changed} The media may have changed.";
		case /*STATUS_SETMARK_DETECTED*/(DWORD)0x80000021L: return "A tape access reached a setmark.";
		case /*STATUS_NO_DATA_DETECTED*/(DWORD)0x80000022L: return "During a tape access, the end of the data written is reached.";
		case /*STATUS_REDIRECTOR_HAS_OPEN_HANDLES*/(DWORD)0x80000023L: return "The redirector is in use and cannot be unloaded.";
		case /*STATUS_SERVER_HAS_OPEN_HANDLES*/(DWORD)0x80000024L: return "The server is in use and cannot be unloaded.";
		case /*STATUS_ALREADY_DISCONNECTED*/(DWORD)0x80000025L: return "The specified connection has already been disconnected.";
		case /*STATUS_LONGJUMP*/(DWORD)0x80000026L: return "A long jump has been executed.";
		case /*STATUS_CLEANER_CARTRIDGE_INSTALLED*/(DWORD)0x80000027L: return "A cleaner cartridge is present in the tape library.";
		case /*STATUS_PLUGPLAY_QUERY_VETOED*/(DWORD)0x80000028L: return "The Plug and Play query operation was not successful.";
		case /*STATUS_UNWIND_CONSOLIDATE*/(DWORD)0x80000029L: return "A frame consolidation has been executed.";
		case /*STATUS_REGISTRY_HIVE_RECOVERED*/(DWORD)0x8000002AL: return "{Registry Hive Recovered} Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost.";
		case /*STATUS_DLL_MIGHT_BE_INSECURE*/(DWORD)0x8000002BL: return "The application is attempting to run executable code from the module %hs.  This may be insecure.  An alternative, %hs, is available.  Should the application use the secure module %hs?";
		case /*STATUS_DLL_MIGHT_BE_INCOMPATIBLE*/(DWORD)0x8000002CL: return "The application is loading executable code from the module %hs.  This is secure, but may be incompatible with previous releases of the operating system.  An alternative, %hs, is available.  Should the application use the secure module %hs?";
		case /*STATUS_STOPPED_ON_SYMLINK*/(DWORD)0x8000002DL: return "The create operation stopped after reaching a symbolic link.";
		case /*DBG_EXCEPTION_NOT_HANDLED*/(DWORD)0x80010001L: return "Debugger did not handle the exception.";
		case /*STATUS_CLUSTER_NODE_ALREADY_UP*/(DWORD)0x80130001L: return "The cluster node is already up.";
		case /*STATUS_CLUSTER_NODE_ALREADY_DOWN*/(DWORD)0x80130002L: return "The cluster node is already down.";
		case /*STATUS_CLUSTER_NETWORK_ALREADY_ONLINE*/(DWORD)0x80130003L: return "The cluster network is already online.";
		case /*STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE*/(DWORD)0x80130004L: return "The cluster network is already offline.";
		case /*STATUS_CLUSTER_NODE_ALREADY_MEMBER*/(DWORD)0x80130005L: return "The cluster node is already a member of the cluster.";
		case /*STATUS_FVE_PARTIAL_METADATA*/(DWORD)0x80210001L: return "Volume Metadata read or write is incomplete.";
		case /*STATUS_UNSUCCESSFUL*/(DWORD)0xC0000001L: return "{Operation Failed} The requested operation was unsuccessful.";
		case /*STATUS_NOT_IMPLEMENTED*/(DWORD)0xC0000002L: return "{Not Implemented} The requested operation is not implemented.";
		case /*STATUS_INVALID_INFO_CLASS*/(DWORD)0xC0000003L: return "{Invalid Parameter} The specified information class is not a valid information class for the specified object.";
		case /*STATUS_INFO_LENGTH_MISMATCH*/(DWORD)0xC0000004L: return "The specified information record length does not match the length required for the specified information class.";
		case /*STATUS_ACCESS_VIOLATION*/(DWORD)0xC0000005L: return "Memory Access Violation" /*"The instruction at 0x%08lx referenced memory at 0x%08lx. The memory could not be %s."*/;
		case /*STATUS_IN_PAGE_ERROR*/(DWORD)0xC0000006L: return "The instruction at 0x%08lx referenced memory at 0x%08lx. The required data was not placed into memory because of an I/O error status of 0x%08lx.";
		case /*STATUS_PAGEFILE_QUOTA*/(DWORD)0xC0000007L: return "The pagefile quota for the process has been exhausted.";
		case /*STATUS_INVALID_HANDLE*/(DWORD)0xC0000008L: return "An invalid HANDLE was specified.";
		case /*STATUS_BAD_INITIAL_STACK*/(DWORD)0xC0000009L: return "An invalid initial stack was specified in a call to NtCreateThread.";
		case /*STATUS_BAD_INITIAL_PC*/(DWORD)0xC000000AL: return "An invalid initial start address was specified in a call to NtCreateThread.";
		case /*STATUS_INVALID_CID*/(DWORD)0xC000000BL: return "An invalid Client ID was specified.";
		case /*STATUS_TIMER_NOT_CANCELED*/(DWORD)0xC000000CL: return "An attempt was made to cancel or set a timer that has an associated APC and the subject thread is not the thread that originally set the timer with an associated APC routine.";
		case /*STATUS_INVALID_PARAMETER*/(DWORD)0xC000000DL: return "An invalid parameter was passed to a service or function.";
		case /*STATUS_NO_SUCH_DEVICE*/(DWORD)0xC000000EL: return "A device which does not exist was specified.";
		case /*STATUS_NO_SUCH_FILE*/(DWORD)0xC000000FL: return "{File Not Found} The file %hs does not exist.";
		case /*STATUS_INVALID_DEVICE_REQUEST*/(DWORD)0xC0000010L: return "The specified request is not a valid operation for the target device.";
		case /*STATUS_END_OF_FILE*/(DWORD)0xC0000011L: return "The end-of-file marker has been reached. There is no valid data in the file beyond this marker.";
		case /*STATUS_WRONG_VOLUME*/(DWORD)0xC0000012L: return "{Wrong Volume} The wrong volume is in the drive. Please insert volume %hs into drive %hs.";
		case /*STATUS_NO_MEDIA_IN_DEVICE*/(DWORD)0xC0000013L: return "{No Disk} There is no disk in the drive. Please insert a disk into drive %hs.";
		case /*STATUS_UNRECOGNIZED_MEDIA*/(DWORD)0xC0000014L: return "{Unknown Disk Format} The disk in drive %hs is not formatted properly. Please check the disk, and reformat if necessary.";
		case /*STATUS_NONEXISTENT_SECTOR*/(DWORD)0xC0000015L: return "{Sector Not Found} The specified sector does not exist.";
		case /*STATUS_MORE_PROCESSING_REQUIRED*/(DWORD)0xC0000016L: return "{Still Busy} The specified I/O request packet (IRP) cannot be disposed of because the I/O operation is not complete.";
		case /*STATUS_NO_MEMORY*/(DWORD)0xC0000017L: return "{Not Enough Quota} Not enough virtual memory or paging file quota is available to complete the specified operation.";
		case /*STATUS_CONFLICTING_ADDRESSES*/(DWORD)0xC0000018L: return "{Conflicting Address Range} The specified address range conflicts with the address space.";
		case /*STATUS_NOT_MAPPED_VIEW*/(DWORD)0xC0000019L: return "Address range to unmap is not a mapped view.";
		case /*STATUS_UNABLE_TO_FREE_VM*/(DWORD)0xC000001AL: return "Virtual memory cannot be freed.";
		case /*STATUS_UNABLE_TO_DELETE_SECTION*/(DWORD)0xC000001BL: return "Specified section cannot be deleted.";
		case /*STATUS_INVALID_SYSTEM_SERVICE*/(DWORD)0xC000001CL: return "An invalid system service was specified in a system service call.";
		case /*STATUS_ILLEGAL_INSTRUCTION*/(DWORD)0xC000001DL: return "{ILLEGAL_INSTRUCTION} Illegal Instruction An attempt was made to execute an illegal instruction.";
		case /*STATUS_INVALID_LOCK_SEQUENCE*/(DWORD)0xC000001EL: return "{Invalid Lock Sequence} An attempt was made to execute an invalid lock sequence.";
		case /*STATUS_INVALID_VIEW_SIZE*/(DWORD)0xC000001FL: return "{Invalid Mapping} An attempt was made to create a view for a section which is bigger than the section.";
		case /*STATUS_INVALID_FILE_FOR_SECTION*/(DWORD)0xC0000020L: return "{Bad File} The attributes of the specified mapping file for a section of memory cannot be read.";
		case /*STATUS_ALREADY_COMMITTED*/(DWORD)0xC0000021L: return "{Already Committed} The specified address range is already committed.";
		case /*STATUS_ACCESS_DENIED*/(DWORD)0xC0000022L: return "{Access Denied} A process has requested access to an object, but has not been granted those access rights.";
		case /*STATUS_BUFFER_TOO_SMALL*/(DWORD)0xC0000023L: return "{Buffer Too Small} The buffer is too small to contain the entry. No information has been written to the buffer.";
		case /*STATUS_OBJECT_TYPE_MISMATCH*/(DWORD)0xC0000024L: return "{Wrong Type} There is a mismatch between the type of object required by the requested operation and the type of object that is specified in the request.";
		case /*STATUS_NONCONTINUABLE_EXCEPTION*/(DWORD)0xC0000025L: return "{NONCONTINUABLE_EXCEPTION} Cannot Continue Windows cannot continue from this exception.";
		case /*STATUS_INVALID_DISPOSITION*/(DWORD)0xC0000026L: return "An invalid exception disposition was returned by an exception handler.";
		case /*STATUS_UNWIND*/(DWORD)0xC0000027L: return "Unwind exception code.";
		case /*STATUS_BAD_STACK*/(DWORD)0xC0000028L: return "An invalid or unaligned stack was encountered during an unwind operation.";
		case /*STATUS_INVALID_UNWIND_TARGET*/(DWORD)0xC0000029L: return "An invalid unwind target was encountered during an unwind operation.";
		case /*STATUS_NOT_LOCKED*/(DWORD)0xC000002AL: return "An attempt was made to unlock a page of memory which was not locked.";
		case /*STATUS_PARITY_ERROR*/(DWORD)0xC000002BL: return "Device parity error on I/O operation.";
		case /*STATUS_UNABLE_TO_DECOMMIT_VM*/(DWORD)0xC000002CL: return "An attempt was made to decommit uncommitted virtual memory.";
		case /*STATUS_NOT_COMMITTED*/(DWORD)0xC000002DL: return "An attempt was made to change the attributes on memory that has not been committed.";
		case /*STATUS_INVALID_PORT_ATTRIBUTES*/(DWORD)0xC000002EL: return "Invalid Object Attributes specified to NtCreatePort or invalid Port Attributes specified to NtConnectPort";
		case /*STATUS_PORT_MESSAGE_TOO_LONG*/(DWORD)0xC000002FL: return "Length of message passed to NtRequestPort or NtRequestWaitReplyPort was longer than the maximum message allowed by the port.";
		case /*STATUS_INVALID_PARAMETER_MIX*/(DWORD)0xC0000030L: return "An invalid combination of parameters was specified.";
		case /*STATUS_INVALID_QUOTA_LOWER*/(DWORD)0xC0000031L: return "An attempt was made to lower a quota limit below the current usage.";
		case /*STATUS_DISK_CORRUPT_ERROR*/(DWORD)0xC0000032L: return "{Corrupt Disk} The file system structure on the disk is corrupt and unusable. Please run the Chkdsk utility on the volume %hs.";
		case /*STATUS_OBJECT_NAME_INVALID*/(DWORD)0xC0000033L: return "Object Name invalid.";
		case /*STATUS_OBJECT_NAME_NOT_FOUND*/(DWORD)0xC0000034L: return "Object Name not found.";
		case /*STATUS_OBJECT_NAME_COLLISION*/(DWORD)0xC0000035L: return "Object Name already exists.";
		case /*STATUS_PORT_DISCONNECTED*/(DWORD)0xC0000037L: return "Attempt to send a message to a disconnected communication port.";
		case /*STATUS_DEVICE_ALREADY_ATTACHED*/(DWORD)0xC0000038L: return "An attempt was made to attach to a device that was already attached to another device.";
		case /*STATUS_OBJECT_PATH_INVALID*/(DWORD)0xC0000039L: return "Object Path Component was not a directory object.";
		case /*STATUS_OBJECT_PATH_NOT_FOUND*/(DWORD)0xC000003AL: return "{Path Not Found} The path %hs does not exist.";
		case /*STATUS_OBJECT_PATH_SYNTAX_BAD*/(DWORD)0xC000003BL: return "Object Path Component was not a directory object.";
		case /*STATUS_DATA_OVERRUN*/(DWORD)0xC000003CL: return "{Data Overrun} A data overrun error occurred.";
		case /*STATUS_DATA_LATE_ERROR*/(DWORD)0xC000003DL: return "{Data Late} A data late error occurred.";
		case /*STATUS_DATA_ERROR*/(DWORD)0xC000003EL: return "{Data Error} An error in reading or writing data occurred.";
		case /*STATUS_CRC_ERROR*/(DWORD)0xC000003FL: return "{Bad CRC} A cyclic redundancy check (CRC) checksum error occurred.";
		case /*STATUS_SECTION_TOO_BIG*/(DWORD)0xC0000040L: return "{Section Too Large} The specified section is too big to map the file.";
		case /*STATUS_PORT_CONNECTION_REFUSED*/(DWORD)0xC0000041L: return "The NtConnectPort request is refused.";
		case /*STATUS_INVALID_PORT_HANDLE*/(DWORD)0xC0000042L: return "The type of port handle is invalid for the operation requested.";
		case /*STATUS_SHARING_VIOLATION*/(DWORD)0xC0000043L: return "A file cannot be opened because the share access flags are incompatible.";
		case /*STATUS_QUOTA_EXCEEDED*/(DWORD)0xC0000044L: return "Insufficient quota exists to complete the operation";
		case /*STATUS_INVALID_PAGE_PROTECTION*/(DWORD)0xC0000045L: return "The specified page protection was not valid.";
		case /*STATUS_MUTANT_NOT_OWNED*/(DWORD)0xC0000046L: return "An attempt to release a mutant object was made by a thread that was not the owner of the mutant object.";
		case /*STATUS_SEMAPHORE_LIMIT_EXCEEDED*/(DWORD)0xC0000047L: return "An attempt was made to release a semaphore such that its maximum count would have been exceeded.";
		case /*STATUS_PORT_ALREADY_SET*/(DWORD)0xC0000048L: return "An attempt to set a processes DebugPort or ExceptionPort was made, but a port already exists in the process or an attempt to set a file's CompletionPort made, but a port was already set in the file or an attempt to set an alpc port's associated completion port was made, but it is already set.";
		case /*STATUS_SECTION_NOT_IMAGE*/(DWORD)0xC0000049L: return "An attempt was made to query image information on a section which does not map an image.";
		case /*STATUS_SUSPEND_COUNT_EXCEEDED*/(DWORD)0xC000004AL: return "An attempt was made to suspend a thread whose suspend count was at its maximum.";
		case /*STATUS_THREAD_IS_TERMINATING*/(DWORD)0xC000004BL: return "An attempt was made to access a thread that has begun termination.";
		case /*STATUS_BAD_WORKING_SET_LIMIT*/(DWORD)0xC000004CL: return "An attempt was made to set the working set limit to an invalid value (minimum greater than maximum, etc).";
		case /*STATUS_INCOMPATIBLE_FILE_MAP*/(DWORD)0xC000004DL: return "A section was created to map a file which is not compatible to an already existing section which maps the same file.";
		case /*STATUS_SECTION_PROTECTION*/(DWORD)0xC000004EL: return "A view to a section specifies a protection which is incompatible with the initial view's protection.";
		case /*STATUS_EAS_NOT_SUPPORTED*/(DWORD)0xC000004FL: return "An operation involving EAs failed because the file system does not support EAs.";
		case /*STATUS_EA_TOO_LARGE*/(DWORD)0xC0000050L: return "An EA operation failed because EA set is too large.";
		case /*STATUS_NONEXISTENT_EA_ENTRY*/(DWORD)0xC0000051L: return "An EA operation failed because the name or EA index is invalid.";
		case /*STATUS_NO_EAS_ON_FILE*/(DWORD)0xC0000052L: return "The file for which EAs were requested has no EAs.";
		case /*STATUS_EA_CORRUPT_ERROR*/(DWORD)0xC0000053L: return "The EA is corrupt and non-readable.";
		case /*STATUS_FILE_LOCK_CONFLICT*/(DWORD)0xC0000054L: return "A requested read/write cannot be granted due to a conflicting file lock.";
		case /*STATUS_LOCK_NOT_GRANTED*/(DWORD)0xC0000055L: return "A requested file lock cannot be granted due to other existing locks.";
		case /*STATUS_DELETE_PENDING*/(DWORD)0xC0000056L: return "A non close operation has been requested of a file object with a delete pending.";
		case /*STATUS_CTL_FILE_NOT_SUPPORTED*/(DWORD)0xC0000057L: return "An attempt was made to set the control attribute on a file. This attribute is not supported in the target file system.";
		case /*STATUS_UNKNOWN_REVISION*/(DWORD)0xC0000058L: return "Indicates a revision number encountered or specified is not one known by the service. It may be a more recent revision than the service is aware of.";
		case /*STATUS_REVISION_MISMATCH*/(DWORD)0xC0000059L: return "Indicates two revision levels are incompatible.";
		case /*STATUS_INVALID_OWNER*/(DWORD)0xC000005AL: return "Indicates a particular Security ID may not be assigned as the owner of an object.";
		case /*STATUS_INVALID_PRIMARY_GROUP*/(DWORD)0xC000005BL: return "Indicates a particular Security ID may not be assigned as the primary group of an object.";
		case /*STATUS_NO_IMPERSONATION_TOKEN*/(DWORD)0xC000005CL: return "An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.";
		case /*STATUS_CANT_DISABLE_MANDATORY*/(DWORD)0xC000005DL: return "A mandatory group may not be disabled.";
		case /*STATUS_NO_LOGON_SERVERS*/(DWORD)0xC000005EL: return "There are currently no logon servers available to service the logon request.";
		case /*STATUS_NO_SUCH_LOGON_SESSION*/(DWORD)0xC000005FL: return "A specified logon session does not exist. It may already have been terminated.";
		case /*STATUS_NO_SUCH_PRIVILEGE*/(DWORD)0xC0000060L: return "A specified privilege does not exist.";
		case /*STATUS_PRIVILEGE_NOT_HELD*/(DWORD)0xC0000061L: return "A required privilege is not held by the client.";
		case /*STATUS_INVALID_ACCOUNT_NAME*/(DWORD)0xC0000062L: return "The name provided is not a properly formed account name.";
		case /*STATUS_USER_EXISTS*/(DWORD)0xC0000063L: return "The specified account already exists.";
		case /*STATUS_NO_SUCH_USER*/(DWORD)0xC0000064L: return "The specified account does not exist.";
		case /*STATUS_GROUP_EXISTS*/(DWORD)0xC0000065L: return "The specified group already exists.";
		case /*STATUS_NO_SUCH_GROUP*/(DWORD)0xC0000066L: return "The specified group does not exist.";
		case /*STATUS_MEMBER_IN_GROUP*/(DWORD)0xC0000067L: return "The specified user account is already in the specified group account. Also used to indicate a group cannot be deleted because it contains a member.";
		case /*STATUS_MEMBER_NOT_IN_GROUP*/(DWORD)0xC0000068L: return "The specified user account is not a member of the specified group account.";
		case /*STATUS_LAST_ADMIN*/(DWORD)0xC0000069L: return "Indicates the requested operation would disable or delete the last remaining administration account. This is not allowed to prevent creating a situation in which the system cannot be administrated.";
		case /*STATUS_WRONG_PASSWORD*/(DWORD)0xC000006AL: return "When trying to update a password, this return status indicates that the value provided as the current password is not correct.";
		case /*STATUS_ILL_FORMED_PASSWORD*/(DWORD)0xC000006BL: return "When trying to update a password, this return status indicates that the value provided for the new password contains values that are not allowed in passwords.";
		case /*STATUS_PASSWORD_RESTRICTION*/(DWORD)0xC000006CL: return "When trying to update a password, this status indicates that some password update rule has been violated. For example, the password may not meet length criteria.";
		case /*STATUS_LOGON_FAILURE*/(DWORD)0xC000006DL: return "The attempted logon is invalid. This is either due to a bad username or authentication information.";
		case /*STATUS_ACCOUNT_RESTRICTION*/(DWORD)0xC000006EL: return "Indicates a referenced user name and authentication information are valid, but some user account restriction has prevented successful authentication (such as time-of-day restrictions).";
		case /*STATUS_INVALID_LOGON_HOURS*/(DWORD)0xC000006FL: return "The user account has time restrictions and may not be logged onto at this time.";
		case /*STATUS_INVALID_WORKSTATION*/(DWORD)0xC0000070L: return "The user account is restricted such that it may not be used to log on from the source workstation.";
		case /*STATUS_PASSWORD_EXPIRED*/(DWORD)0xC0000071L: return "The user account's password has expired.";
		case /*STATUS_ACCOUNT_DISABLED*/(DWORD)0xC0000072L: return "The referenced account is currently disabled and may not be logged on to.";
		case /*STATUS_NONE_MAPPED*/(DWORD)0xC0000073L: return "None of the information to be translated has been translated.";
		case /*STATUS_TOO_MANY_LUIDS_REQUESTED*/(DWORD)0xC0000074L: return "The number of LUIDs requested may not be allocated with a single allocation.";
		case /*STATUS_LUIDS_EXHAUSTED*/(DWORD)0xC0000075L: return "Indicates there are no more LUIDs to allocate.";
		case /*STATUS_INVALID_SUB_AUTHORITY*/(DWORD)0xC0000076L: return "Indicates the sub-authority value is invalid for the particular use.";
		case /*STATUS_INVALID_ACL*/(DWORD)0xC0000077L: return "Indicates the ACL structure is not valid.";
		case /*STATUS_INVALID_SID*/(DWORD)0xC0000078L: return "Indicates the SID structure is not valid.";
		case /*STATUS_INVALID_SECURITY_DESCR*/(DWORD)0xC0000079L: return "Indicates the SECURITY_DESCRIPTOR structure is not valid.";
		case /*STATUS_PROCEDURE_NOT_FOUND*/(DWORD)0xC000007AL: return "Indicates the specified procedure address cannot be found in the DLL.";
		case /*STATUS_INVALID_IMAGE_FORMAT*/(DWORD)0xC000007BL: return "{Bad Image} %hs is either not designed to run on Windows or it contains an error. Try installing the program again using the original installation media or contact your system administrator or the software vendor for support.";
		case /*STATUS_NO_TOKEN*/(DWORD)0xC000007CL: return "An attempt was made to reference a token that doesn't exist. This is typically done by referencing the token associated with a thread when the thread is not impersonating a client.";
		case /*STATUS_BAD_INHERITANCE_ACL*/(DWORD)0xC000007DL: return "Indicates that an attempt to build either an inherited ACL or ACE was not successful. This can be caused by a number of things. One of the more probable causes is the replacement of a CreatorId with an SID that didn't fit into the ACE or ACL.";
		case /*STATUS_RANGE_NOT_LOCKED*/(DWORD)0xC000007EL: return "The range specified in NtUnlockFile was not locked.";
		case /*STATUS_DISK_FULL*/(DWORD)0xC000007FL: return "An operation failed because the disk was full.";
		case /*STATUS_SERVER_DISABLED*/(DWORD)0xC0000080L: return "The GUID allocation server is [already] disabled at the moment.";
		case /*STATUS_SERVER_NOT_DISABLED*/(DWORD)0xC0000081L: return "The GUID allocation server is [already] enabled at the moment.";
		case /*STATUS_TOO_MANY_GUIDS_REQUESTED*/(DWORD)0xC0000082L: return "Too many GUIDs were requested from the allocation server at once.";
		case /*STATUS_GUIDS_EXHAUSTED*/(DWORD)0xC0000083L: return "The GUIDs could not be allocated because the Authority Agent was exhausted.";
		case /*STATUS_INVALID_ID_AUTHORITY*/(DWORD)0xC0000084L: return "The value provided was an invalid value for an identifier authority.";
		case /*STATUS_AGENTS_EXHAUSTED*/(DWORD)0xC0000085L: return "There are no more authority agent values available for the given identifier authority value.";
		case /*STATUS_INVALID_VOLUME_LABEL*/(DWORD)0xC0000086L: return "An invalid volume label has been specified.";
		case /*STATUS_SECTION_NOT_EXTENDED*/(DWORD)0xC0000087L: return "A mapped section could not be extended.";
		case /*STATUS_NOT_MAPPED_DATA*/(DWORD)0xC0000088L: return "Specified section to flush does not map a data file.";
		case /*STATUS_RESOURCE_DATA_NOT_FOUND*/(DWORD)0xC0000089L: return "Indicates the specified image file did not contain a resource section.";
		case /*STATUS_RESOURCE_TYPE_NOT_FOUND*/(DWORD)0xC000008AL: return "Indicates the specified resource type cannot be found in the image file.";
		case /*STATUS_RESOURCE_NAME_NOT_FOUND*/(DWORD)0xC000008BL: return "Indicates the specified resource name cannot be found in the image file.";
		case /*STATUS_ARRAY_BOUNDS_EXCEEDED*/(DWORD)0xC000008CL: return "{ARRAY_BOUNDS_EXCEEDED} Array bounds exceeded.";
		case /*STATUS_FLOAT_DENORMAL_OPERAND*/(DWORD)0xC000008DL: return "{FLOAT_DENORMAL_OPERAND} Floating-point denormal operand.";
		case /*STATUS_FLOAT_DIVIDE_BY_ZERO*/(DWORD)0xC000008EL: return "{FLOAT_DIVIDE_BY_ZERO} Floating-point division by zero.";
		case /*STATUS_FLOAT_INEXACT_RESULT*/(DWORD)0xC000008FL: return "{FLOAT_INEXACT_RESULT} Floating-point inexact result.";
		case /*STATUS_FLOAT_INVALID_OPERATION*/(DWORD)0xC0000090L: return "{FLOAT_INVALID_OPERATION} Floating-point invalid operation.";
		case /*STATUS_FLOAT_OVERFLOW*/(DWORD)0xC0000091L: return "{FLOAT_OVERFLOW} Floating-point overflow.";
		case /*STATUS_FLOAT_STACK_CHECK*/(DWORD)0xC0000092L: return "{FLOAT_STACK_CHECK} Floating-point stack check.";
		case /*STATUS_FLOAT_UNDERFLOW*/(DWORD)0xC0000093L: return "{FLOAT_UNDERFLOW} Floating-point underflow.";
		case /*STATUS_INTEGER_DIVIDE_BY_ZERO*/(DWORD)0xC0000094L: return "{INTEGER_DIVIDE_BY_ZERO} Integer division by zero.";
		case /*STATUS_INTEGER_OVERFLOW*/(DWORD)0xC0000095L: return "{INTEGER_OVERFLOW} Integer overflow.";
		case /*STATUS_PRIVILEGED_INSTRUCTION*/(DWORD)0xC0000096L: return "{PRIVILEGED_INSTRUCTION} Privileged instruction.";
		case /*STATUS_TOO_MANY_PAGING_FILES*/(DWORD)0xC0000097L: return "An attempt was made to install more paging files than the system supports.";
		case /*STATUS_FILE_INVALID*/(DWORD)0xC0000098L: return "The volume for a file has been externally altered such that the opened file is no longer valid.";
		case /*STATUS_ALLOTTED_SPACE_EXCEEDED*/(DWORD)0xC0000099L: return "When a block of memory is allotted for future updates, such as the memory allocated to hold discretionary access control and primary group information, successive updates may exceed the amount of memory originally allotted. Since quota may already have been charged to several processes which have handles to the object, it is not reasonable to alter the size of the allocated memory. Instead, a request that requires more memory than has been allotted must fail and the STATUS_ALLOTED_SPACE_EXCEEDED error returned.";
		case /*STATUS_INSUFFICIENT_RESOURCES*/(DWORD)0xC000009AL: return "Insufficient system resources exist to complete the API.";
		case /*STATUS_DFS_EXIT_PATH_FOUND*/(DWORD)0xC000009BL: return "An attempt has been made to open a DFS exit path control file.";
		case /*STATUS_DEVICE_DATA_ERROR*/(DWORD)0xC000009CL: return "DEVICE_DATA_ERROR";
		case /*STATUS_DEVICE_NOT_CONNECTED*/(DWORD)0xC000009DL: return "DEVICE_NOT_CONNECTED";
		case /*STATUS_DEVICE_POWER_FAILURE*/(DWORD)0xC000009EL: return "DEVICE_POWER_FAILURE";
		case /*STATUS_FREE_VM_NOT_AT_BASE*/(DWORD)0xC000009FL: return "Virtual memory cannot be freed as base address is not the base of the region and a region size of zero was specified.";
		case /*STATUS_MEMORY_NOT_ALLOCATED*/(DWORD)0xC00000A0L: return "An attempt was made to free virtual memory which is not allocated.";
		case /*STATUS_WORKING_SET_QUOTA*/(DWORD)0xC00000A1L: return "The working set is not big enough to allow the requested pages to be locked.";
		case /*STATUS_MEDIA_WRITE_PROTECTED*/(DWORD)0xC00000A2L: return "{Write Protect Error} The disk cannot be written to because it is write protected. Please remove the write protection from the volume %hs in drive %hs.";
		case /*STATUS_DEVICE_NOT_READY*/(DWORD)0xC00000A3L: return "{Drive Not Ready} The drive is not ready for use; its door may be open. Please check drive %hs and make sure that a disk is inserted and that the drive door is closed.";
		case /*STATUS_INVALID_GROUP_ATTRIBUTES*/(DWORD)0xC00000A4L: return "The specified attributes are invalid, or incompatible with the attributes for the group as a whole.";
		case /*STATUS_BAD_IMPERSONATION_LEVEL*/(DWORD)0xC00000A5L: return "A specified impersonation level is invalid. Also used to indicate a required impersonation level was not provided.";
		case /*STATUS_CANT_OPEN_ANONYMOUS*/(DWORD)0xC00000A6L: return "An attempt was made to open an Anonymous level token. Anonymous tokens may not be opened.";
		case /*STATUS_BAD_VALIDATION_CLASS*/(DWORD)0xC00000A7L: return "The validation information class requested was invalid.";
		case /*STATUS_BAD_TOKEN_TYPE*/(DWORD)0xC00000A8L: return "The type of a token object is inappropriate for its attempted use.";
		case /*STATUS_BAD_MASTER_BOOT_RECORD*/(DWORD)0xC00000A9L: return "The type of a token object is inappropriate for its attempted use.";
		case /*STATUS_INSTRUCTION_MISALIGNMENT*/(DWORD)0xC00000AAL: return "An attempt was made to execute an instruction at an unaligned address and the host system does not support unaligned instruction references.";
		case /*STATUS_INSTANCE_NOT_AVAILABLE*/(DWORD)0xC00000ABL: return "The maximum named pipe instance count has been reached.";
		case /*STATUS_PIPE_NOT_AVAILABLE*/(DWORD)0xC00000ACL: return "An instance of a named pipe cannot be found in the listening state.";
		case /*STATUS_INVALID_PIPE_STATE*/(DWORD)0xC00000ADL: return "The named pipe is not in the connected or closing state.";
		case /*STATUS_PIPE_BUSY*/(DWORD)0xC00000AEL: return "The specified pipe is set to complete operations and there are current I/O operations queued so it cannot be changed to queue operations.";
		case /*STATUS_ILLEGAL_FUNCTION*/(DWORD)0xC00000AFL: return "The specified handle is not open to the server end of the named pipe.";
		case /*STATUS_PIPE_DISCONNECTED*/(DWORD)0xC00000B0L: return "The specified named pipe is in the disconnected state.";
		case /*STATUS_PIPE_CLOSING*/(DWORD)0xC00000B1L: return "The specified named pipe is in the closing state.";
		case /*STATUS_PIPE_CONNECTED*/(DWORD)0xC00000B2L: return "The specified named pipe is in the connected state.";
		case /*STATUS_PIPE_LISTENING*/(DWORD)0xC00000B3L: return "The specified named pipe is in the listening state.";
		case /*STATUS_INVALID_READ_MODE*/(DWORD)0xC00000B4L: return "The specified named pipe is not in message mode.";
		case /*STATUS_IO_TIMEOUT*/(DWORD)0xC00000B5L: return "{Device Timeout} The specified I/O operation on %hs was not completed before the time-out period expired.";
		case /*STATUS_FILE_FORCED_CLOSED*/(DWORD)0xC00000B6L: return "The specified file has been closed by another process.";
		case /*STATUS_PROFILING_NOT_STARTED*/(DWORD)0xC00000B7L: return "Profiling not started.";
		case /*STATUS_PROFILING_NOT_STOPPED*/(DWORD)0xC00000B8L: return "Profiling not stopped.";
		case /*STATUS_COULD_NOT_INTERPRET*/(DWORD)0xC00000B9L: return "The passed ACL did not contain the minimum required information.";
		case /*STATUS_FILE_IS_A_DIRECTORY*/(DWORD)0xC00000BAL: return "The file that was specified as a target is a directory and the caller specified that it could be anything but a directory.";
		case /*STATUS_NOT_SUPPORTED*/(DWORD)0xC00000BBL: return "Network specific errors. The request is not supported.";
		case /*STATUS_REMOTE_NOT_LISTENING*/(DWORD)0xC00000BCL: return "This remote computer is not listening.";
		case /*STATUS_DUPLICATE_NAME*/(DWORD)0xC00000BDL: return "A duplicate name exists on the network.";
		case /*STATUS_BAD_NETWORK_PATH*/(DWORD)0xC00000BEL: return "The network path cannot be located.";
		case /*STATUS_NETWORK_BUSY*/(DWORD)0xC00000BFL: return "The network is busy.";
		case /*STATUS_DEVICE_DOES_NOT_EXIST*/(DWORD)0xC00000C0L: return "This device does not exist.";
		case /*STATUS_TOO_MANY_COMMANDS*/(DWORD)0xC00000C1L: return "The network BIOS command limit has been reached.";
		case /*STATUS_ADAPTER_HARDWARE_ERROR*/(DWORD)0xC00000C2L: return "An I/O adapter hardware error has occurred.";
		case /*STATUS_INVALID_NETWORK_RESPONSE*/(DWORD)0xC00000C3L: return "The network responded incorrectly.";
		case /*STATUS_UNEXPECTED_NETWORK_ERROR*/(DWORD)0xC00000C4L: return "An unexpected network error occurred.";
		case /*STATUS_BAD_REMOTE_ADAPTER*/(DWORD)0xC00000C5L: return "The remote adapter is not compatible.";
		case /*STATUS_PRINT_QUEUE_FULL*/(DWORD)0xC00000C6L: return "The printer queue is full.";
		case /*STATUS_NO_SPOOL_SPACE*/(DWORD)0xC00000C7L: return "Space to store the file waiting to be printed is not available on the server.";
		case /*STATUS_PRINT_CANCELLED*/(DWORD)0xC00000C8L: return "The requested print file has been canceled.";
		case /*STATUS_NETWORK_NAME_DELETED*/(DWORD)0xC00000C9L: return "The network name was deleted.";
		case /*STATUS_NETWORK_ACCESS_DENIED*/(DWORD)0xC00000CAL: return "Network access is denied.";
		case /*STATUS_BAD_DEVICE_TYPE*/(DWORD)0xC00000CBL: return "{Incorrect Network Resource Type} The specified device type (LPT, for example) conflicts with the actual device type on the remote resource.";
		case /*STATUS_BAD_NETWORK_NAME*/(DWORD)0xC00000CCL: return "{Network Name Not Found} The specified share name cannot be found on the remote server.";
		case /*STATUS_TOO_MANY_NAMES*/(DWORD)0xC00000CDL: return "The name limit for the local computer network adapter card was exceeded.";
		case /*STATUS_TOO_MANY_SESSIONS*/(DWORD)0xC00000CEL: return "The network BIOS session limit was exceeded.";
		case /*STATUS_SHARING_PAUSED*/(DWORD)0xC00000CFL: return "File sharing has been temporarily paused.";
		case /*STATUS_REQUEST_NOT_ACCEPTED*/(DWORD)0xC00000D0L: return "No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.";
		case /*STATUS_REDIRECTOR_PAUSED*/(DWORD)0xC00000D1L: return "Print or disk redirection is temporarily paused.";
		case /*STATUS_NET_WRITE_FAULT*/(DWORD)0xC00000D2L: return "A network data fault occurred.";
		case /*STATUS_PROFILING_AT_LIMIT*/(DWORD)0xC00000D3L: return "The number of active profiling objects is at the maximum and no more may be started.";
		case /*STATUS_NOT_SAME_DEVICE*/(DWORD)0xC00000D4L: return "{Incorrect Volume} The target file of a rename request is located on a different device than the source of the rename request.";
		case /*STATUS_FILE_RENAMED*/(DWORD)0xC00000D5L: return "The file specified has been renamed and thus cannot be modified.";
		case /*STATUS_VIRTUAL_CIRCUIT_CLOSED*/(DWORD)0xC00000D6L: return "{Network Request Timeout} The session with a remote server has been disconnected because the time-out interval for a request has expired.";
		case /*STATUS_NO_SECURITY_ON_OBJECT*/(DWORD)0xC00000D7L: return "Indicates an attempt was made to operate on the security of an object that does not have security associated with it.";
		case /*STATUS_CANT_WAIT*/(DWORD)0xC00000D8L: return "Used to indicate that an operation cannot continue without blocking for I/O.";
		case /*STATUS_PIPE_EMPTY*/(DWORD)0xC00000D9L: return "Used to indicate that a read operation was done on an empty pipe.";
		case /*STATUS_CANT_ACCESS_DOMAIN_INFO*/(DWORD)0xC00000DAL: return "Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied.";
		case /*STATUS_CANT_TERMINATE_SELF*/(DWORD)0xC00000DBL: return "Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process.";
		case /*STATUS_INVALID_SERVER_STATE*/(DWORD)0xC00000DCL: return "Indicates the Sam Server was in the wrong state to perform the desired operation.";
		case /*STATUS_INVALID_DOMAIN_STATE*/(DWORD)0xC00000DDL: return "Indicates the Domain was in the wrong state to perform the desired operation.";
		case /*STATUS_INVALID_DOMAIN_ROLE*/(DWORD)0xC00000DEL: return "This operation is only allowed for the Primary Domain Controller of the domain.";
		case /*STATUS_NO_SUCH_DOMAIN*/(DWORD)0xC00000DFL: return "The specified Domain did not exist.";
		case /*STATUS_DOMAIN_EXISTS*/(DWORD)0xC00000E0L: return "The specified Domain already exists.";
		case /*STATUS_DOMAIN_LIMIT_EXCEEDED*/(DWORD)0xC00000E1L: return "An attempt was made to exceed the limit on the number of domains per server for this release.";
		case /*STATUS_OPLOCK_NOT_GRANTED*/(DWORD)0xC00000E2L: return "Error status returned when oplock request is denied.";
		case /*STATUS_INVALID_OPLOCK_PROTOCOL*/(DWORD)0xC00000E3L: return "Error status returned when an invalid oplock acknowledgment is received by a file system.";
		case /*STATUS_INTERNAL_DB_CORRUPTION*/(DWORD)0xC00000E4L: return "This error indicates that the requested operation cannot be completed due to a catastrophic media failure or on-disk data structure corruption.";
		case /*STATUS_INTERNAL_ERROR*/(DWORD)0xC00000E5L: return "An internal error occurred.";
		case /*STATUS_GENERIC_NOT_MAPPED*/(DWORD)0xC00000E6L: return "Indicates generic access types were contained in an access mask which should already be mapped to non-generic access types.";
		case /*STATUS_BAD_DESCRIPTOR_FORMAT*/(DWORD)0xC00000E7L: return "Indicates a security descriptor is not in the necessary format (absolute or self-relative).";
		case /*STATUS_INVALID_USER_BUFFER*/(DWORD)0xC00000E8L: return "Status codes raised by the Cache Manager which must be considered as \"expected\" by its callers. An access to a user buffer failed at an \"expected\" point in time. This code is defined since the caller does not want to accept STATUS_ACCESS_VIOLATION in its filter.";
		case /*STATUS_UNEXPECTED_IO_ERROR*/(DWORD)0xC00000E9L: return "If an I/O error is returned which is not defined in the standard FsRtl filter, it is converted to the following error which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.";
		case /*STATUS_UNEXPECTED_MM_CREATE_ERR*/(DWORD)0xC00000EAL: return "If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.";
		case /*STATUS_UNEXPECTED_MM_MAP_ERROR*/(DWORD)0xC00000EBL: return "If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.";
		case /*STATUS_UNEXPECTED_MM_EXTEND_ERR*/(DWORD)0xC00000ECL: return "If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.";
		case /*STATUS_NOT_LOGON_PROCESS*/(DWORD)0xC00000EDL: return "The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.";
		case /*STATUS_LOGON_SESSION_EXISTS*/(DWORD)0xC00000EEL: return "An attempt has been made to start a new session manager or LSA logon session with an ID that is already in use.";
		case /*STATUS_INVALID_PARAMETER_1*/(DWORD)0xC00000EFL: return "An invalid parameter was passed to a service or function as the first argument.";
		case /*STATUS_INVALID_PARAMETER_2*/(DWORD)0xC00000F0L: return "An invalid parameter was passed to a service or function as the second argument.";
		case /*STATUS_INVALID_PARAMETER_3*/(DWORD)0xC00000F1L: return "An invalid parameter was passed to a service or function as the third argument.";
		case /*STATUS_INVALID_PARAMETER_4*/(DWORD)0xC00000F2L: return "An invalid parameter was passed to a service or function as the fourth argument.";
		case /*STATUS_INVALID_PARAMETER_5*/(DWORD)0xC00000F3L: return "An invalid parameter was passed to a service or function as the fifth argument.";
		case /*STATUS_INVALID_PARAMETER_6*/(DWORD)0xC00000F4L: return "An invalid parameter was passed to a service or function as the sixth argument.";
		case /*STATUS_INVALID_PARAMETER_7*/(DWORD)0xC00000F5L: return "An invalid parameter was passed to a service or function as the seventh argument.";
		case /*STATUS_INVALID_PARAMETER_8*/(DWORD)0xC00000F6L: return "An invalid parameter was passed to a service or function as the eighth argument.";
		case /*STATUS_INVALID_PARAMETER_9*/(DWORD)0xC00000F7L: return "An invalid parameter was passed to a service or function as the ninth argument.";
		case /*STATUS_INVALID_PARAMETER_10*/(DWORD)0xC00000F8L: return "An invalid parameter was passed to a service or function as the tenth argument.";
		case /*STATUS_INVALID_PARAMETER_11*/(DWORD)0xC00000F9L: return "An invalid parameter was passed to a service or function as the eleventh argument.";
		case /*STATUS_INVALID_PARAMETER_12*/(DWORD)0xC00000FAL: return "An invalid parameter was passed to a service or function as the twelfth argument.";
		case /*STATUS_REDIRECTOR_NOT_STARTED*/(DWORD)0xC00000FBL: return "An attempt was made to access a network file, but the network software was not yet started.";
		case /*STATUS_REDIRECTOR_STARTED*/(DWORD)0xC00000FCL: return "An attempt was made to start the redirector, but the redirector has already been started.";
		case /*STATUS_STACK_OVERFLOW*/(DWORD)0xC00000FDL: return "A new guard page for the stack cannot be created.";
		case /*STATUS_NO_SUCH_PACKAGE*/(DWORD)0xC00000FEL: return "A specified authentication package is unknown.";
		case /*STATUS_BAD_FUNCTION_TABLE*/(DWORD)0xC00000FFL: return "A malformed function table was encountered during an unwind operation.";
		case /*STATUS_VARIABLE_NOT_FOUND*/(DWORD)0xC0000100L: return "Indicates the specified environment variable name was not found in the specified environment block.";
		case /*STATUS_DIRECTORY_NOT_EMPTY*/(DWORD)0xC0000101L: return "Indicates that the directory trying to be deleted is not empty.";
		case /*STATUS_FILE_CORRUPT_ERROR*/(DWORD)0xC0000102L: return "{Corrupt File} The file or directory %hs is corrupt and unreadable. Please run the Chkdsk utility.";
		case /*STATUS_NOT_A_DIRECTORY*/(DWORD)0xC0000103L: return "A requested opened file is not a directory.";
		case /*STATUS_BAD_LOGON_SESSION_STATE*/(DWORD)0xC0000104L: return "The logon session is not in a state that is consistent with the requested operation.";
		case /*STATUS_LOGON_SESSION_COLLISION*/(DWORD)0xC0000105L: return "An internal LSA error has occurred. An authentication package has requested the creation of a Logon Session but the ID of an already existing Logon Session has been specified.";
		case /*STATUS_NAME_TOO_LONG*/(DWORD)0xC0000106L: return "A specified name string is too long for its intended use.";
		case /*STATUS_FILES_OPEN*/(DWORD)0xC0000107L: return "The user attempted to force close the files on a redirected drive, but there were opened files on the drive, and the user did not specify a sufficient level of force.";
		case /*STATUS_CONNECTION_IN_USE*/(DWORD)0xC0000108L: return "The user attempted to force close the files on a redirected drive, but there were opened directories on the drive, and the user did not specify a sufficient level of force.";
		case /*STATUS_MESSAGE_NOT_FOUND*/(DWORD)0xC0000109L: return "RtlFindMessage could not locate the requested message ID in the message table resource.";
		case /*STATUS_PROCESS_IS_TERMINATING*/(DWORD)0xC000010AL: return "An attempt was made to access an exiting process.";
		case /*STATUS_INVALID_LOGON_TYPE*/(DWORD)0xC000010BL: return "Indicates an invalid value has been provided for the LogonType requested.";
		case /*STATUS_NO_GUID_TRANSLATION*/(DWORD)0xC000010CL: return "Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail.";
		case /*STATUS_CANNOT_IMPERSONATE*/(DWORD)0xC000010DL: return "Indicates that an attempt has been made to impersonate via a named pipe that has not yet been read from.";
		case /*STATUS_IMAGE_ALREADY_LOADED*/(DWORD)0xC000010EL: return "Indicates that the specified image is already loaded.";
		case /*STATUS_ABIOS_NOT_PRESENT*/(DWORD)0xC000010FL: return "ABIOS_NOT_PRESENT";
		case /*STATUS_ABIOS_LID_NOT_EXIST*/(DWORD)0xC0000110L: return "ABIOS_LID_NOT_EXIST";
		case /*STATUS_ABIOS_LID_ALREADY_OWNED*/(DWORD)0xC0000111L: return "ABIOS_LID_ALREADY_OWNED";
		case /*STATUS_ABIOS_NOT_LID_OWNER*/(DWORD)0xC0000112L: return "ABIOS_NOT_LID_OWNER";
		case /*STATUS_ABIOS_INVALID_COMMAND*/(DWORD)0xC0000113L: return "ABIOS_INVALID_COMMAND";
		case /*STATUS_ABIOS_INVALID_LID*/(DWORD)0xC0000114L: return "ABIOS_INVALID_LID";
		case /*STATUS_ABIOS_SELECTOR_NOT_AVAILABLE*/(DWORD)0xC0000115L: return "ABIOS_SELECTOR_NOT_AVAILABLE";
		case /*STATUS_ABIOS_INVALID_SELECTOR*/(DWORD)0xC0000116L: return "ABIOS_INVALID_SELECTOR";
		case /*STATUS_NO_LDT*/(DWORD)0xC0000117L: return "Indicates that an attempt was made to change the size of the LDT for a process that has no LDT.";
		case /*STATUS_INVALID_LDT_SIZE*/(DWORD)0xC0000118L: return "Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors.";
		case /*STATUS_INVALID_LDT_OFFSET*/(DWORD)0xC0000119L: return "Indicates that the starting value for the LDT information was not an integral multiple of the selector size.";
		case /*STATUS_INVALID_LDT_DESCRIPTOR*/(DWORD)0xC000011AL: return "Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors.";
		case /*STATUS_INVALID_IMAGE_NE_FORMAT*/(DWORD)0xC000011BL: return "The specified image file did not have the correct format. It appears to be NE format.";
		case /*STATUS_RXACT_INVALID_STATE*/(DWORD)0xC000011CL: return "Indicates that the transaction state of a registry sub-tree is incompatible with the requested operation. For example, a request has been made to start a new transaction with one already in progress, or a request has been made to apply a transaction when one is not currently in progress.";
		case /*STATUS_RXACT_COMMIT_FAILURE*/(DWORD)0xC000011DL: return "Indicates an error has occurred during a registry transaction commit. The database has been left in an unknown, but probably inconsistent, state. The state of the registry transaction is left as COMMITTING.";
		case /*STATUS_MAPPED_FILE_SIZE_ZERO*/(DWORD)0xC000011EL: return "An attempt was made to map a file of size zero with the maximum size specified as zero.";
		case /*STATUS_TOO_MANY_OPENED_FILES*/(DWORD)0xC000011FL: return "Too many files are opened on a remote server. This error should only be returned by the Windows redirector on a remote drive.";
		case /*STATUS_CANCELLED*/(DWORD)0xC0000120L: return "The I/O request was canceled.";
		case /*STATUS_CANNOT_DELETE*/(DWORD)0xC0000121L: return "An attempt has been made to remove a file or directory that cannot be deleted.";
		case /*STATUS_INVALID_COMPUTER_NAME*/(DWORD)0xC0000122L: return "Indicates a name specified as a remote computer name is syntactically invalid.";
		case /*STATUS_FILE_DELETED*/(DWORD)0xC0000123L: return "An I/O request other than close was performed on a file after it has been deleted, which can only happen to a request which did not complete before the last handle was closed via NtClose.";
		case /*STATUS_SPECIAL_ACCOUNT*/(DWORD)0xC0000124L: return "Indicates an operation has been attempted on a built-in (special) SAM account which is incompatible with built-in accounts. For example, built-in accounts cannot be deleted.";
		case /*STATUS_SPECIAL_GROUP*/(DWORD)0xC0000125L: return "The operation requested may not be performed on the specified group because it is a built-in special group.";
		case /*STATUS_SPECIAL_USER*/(DWORD)0xC0000126L: return "The operation requested may not be performed on the specified user because it is a built-in special user.";
		case /*STATUS_MEMBERS_PRIMARY_GROUP*/(DWORD)0xC0000127L: return "Indicates a member cannot be removed from a group because the group is currently the member's primary group.";
		case /*STATUS_FILE_CLOSED*/(DWORD)0xC0000128L: return "An I/O request other than close and several other special case operations was attempted using a file object that had already been closed.";
		case /*STATUS_TOO_MANY_THREADS*/(DWORD)0xC0000129L: return "Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads.";
		case /*STATUS_THREAD_NOT_IN_PROCESS*/(DWORD)0xC000012AL: return "An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.";
		case /*STATUS_TOKEN_ALREADY_IN_USE*/(DWORD)0xC000012BL: return "An attempt was made to establish a token for use as a primary token but the token is already in use. A token can only be the primary token of one process at a time.";
		case /*STATUS_PAGEFILE_QUOTA_EXCEEDED*/(DWORD)0xC000012CL: return "Page file quota was exceeded.";
		case /*STATUS_COMMITMENT_LIMIT*/(DWORD)0xC000012DL: return "{Out of Virtual Memory} Your system is low on virtual memory. To ensure that Windows runs properly, increase the size of your virtual memory paging file. For more information, see Help.";
		case /*STATUS_INVALID_IMAGE_LE_FORMAT*/(DWORD)0xC000012EL: return "The specified image file did not have the correct format, it appears to be LE format.";
		case /*STATUS_INVALID_IMAGE_NOT_MZ*/(DWORD)0xC000012FL: return "The specified image file did not have the correct format, it did not have an initial MZ.";
		case /*STATUS_INVALID_IMAGE_PROTECT*/(DWORD)0xC0000130L: return "The specified image file did not have the correct format, it did not have a proper e_lfarlc in the MZ header.";
		case /*STATUS_INVALID_IMAGE_WIN_16*/(DWORD)0xC0000131L: return "The specified image file did not have the correct format, it appears to be a 16-bit Windows image.";
		case /*STATUS_LOGON_SERVER_CONFLICT*/(DWORD)0xC0000132L: return "The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role.";
		case /*STATUS_TIME_DIFFERENCE_AT_DC*/(DWORD)0xC0000133L: return "The time at the Primary Domain Controller is different than the time at the Backup Domain Controller or member server by too large an amount.";
		case /*STATUS_SYNCHRONIZATION_REQUIRED*/(DWORD)0xC0000134L: return "The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required.";
		case /*STATUS_DLL_NOT_FOUND*/(DWORD)0xC0000135L: return "{Unable To Locate Component} This application has failed to start because %hs was not found. Re-installing the application may fix this problem.";
		case /*STATUS_OPEN_FAILED*/(DWORD)0xC0000136L: return "The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines.";
		case /*STATUS_IO_PRIVILEGE_FAILED*/(DWORD)0xC0000137L: return "{Privilege Failed} The I/O permissions for the process could not be changed.";
		case /*STATUS_ORDINAL_NOT_FOUND*/(DWORD)0xC0000138L: return "{Ordinal Not Found} The ordinal %ld could not be located in the dynamic link library %hs.";
		case /*STATUS_ENTRYPOINT_NOT_FOUND*/(DWORD)0xC0000139L: return "{Entry Point Not Found} The procedure entry point %hs could not be located in the dynamic link library %hs.";
		case /*STATUS_CONTROL_C_EXIT*/(DWORD)0xC000013AL: return "{Application Exit by CTRL+C} The application terminated as a result of a CTRL+C.";
		case /*STATUS_LOCAL_DISCONNECT*/(DWORD)0xC000013BL: return "{Virtual Circuit Closed} The network transport on your computer has closed a network connection. There may or may not be I/O requests outstanding.";
		case /*STATUS_REMOTE_DISCONNECT*/(DWORD)0xC000013CL: return "{Virtual Circuit Closed} The network transport on a remote computer has closed a network connection. There may or may not be I/O requests outstanding.";
		case /*STATUS_REMOTE_RESOURCES*/(DWORD)0xC000013DL: return "{Insufficient Resources on Remote Computer} The remote computer has insufficient resources to complete the network request. For instance, there may not be enough memory available on the remote computer to carry out the request at this time.";
		case /*STATUS_LINK_FAILED*/(DWORD)0xC000013EL: return "{Virtual Circuit Closed} An existing connection (virtual circuit) has been broken at the remote computer. There is probably something wrong with the network software protocol or the network hardware on the remote computer.";
		case /*STATUS_LINK_TIMEOUT*/(DWORD)0xC000013FL: return "{Virtual Circuit Closed} The network transport on your computer has closed a network connection because it had to wait too long for a response from the remote computer.";
		case /*STATUS_INVALID_CONNECTION*/(DWORD)0xC0000140L: return "The connection handle given to the transport was invalid.";
		case /*STATUS_INVALID_ADDRESS*/(DWORD)0xC0000141L: return "The address handle given to the transport was invalid.";
		case /*STATUS_DLL_INIT_FAILED*/(DWORD)0xC0000142L: return "{DLL Initialization Failed} Initialization of the dynamic link library %hs failed. The process is terminating abnormally.";
		case /*STATUS_MISSING_SYSTEMFILE*/(DWORD)0xC0000143L: return "{Missing System File} The required system file %hs is bad or missing.";
		case /*STATUS_UNHANDLED_EXCEPTION*/(DWORD)0xC0000144L: return "{Application Error} The exception %s (0x%08lx) occurred in the application at location 0x%08lx.";
		case /*STATUS_APP_INIT_FAILURE*/(DWORD)0xC0000145L: return "{Application Error} The application failed to initialize properly (0x%lx). Click OK to terminate the application.";
		case /*STATUS_PAGEFILE_CREATE_FAILED*/(DWORD)0xC0000146L: return "{Unable to Create Paging File} The creation of the paging file %hs failed (%lx). The requested size was %ld.";
		case /*STATUS_NO_PAGEFILE*/(DWORD)0xC0000147L: return "{No Paging File Specified} No paging file was specified in the system configuration.";
		case /*STATUS_INVALID_LEVEL*/(DWORD)0xC0000148L: return "{Incorrect System Call Level} An invalid level was passed into the specified system call.";
		case /*STATUS_WRONG_PASSWORD_CORE*/(DWORD)0xC0000149L: return "{Incorrect Password to LAN Manager Server} You specified an incorrect password to a LAN Manager 2.x or MS-NET server.";
		case /*STATUS_ILLEGAL_FLOAT_CONTEXT*/(DWORD)0xC000014AL: return "{ILLEGAL_FLOAT_CONTEXT} A real-mode application issued a floating-point instruction and floating-point hardware is not present.";
		case /*STATUS_PIPE_BROKEN*/(DWORD)0xC000014BL: return "The pipe operation has failed because the other end of the pipe has been closed.";
		case /*STATUS_REGISTRY_CORRUPT*/(DWORD)0xC000014CL: return "{The Registry Is Corrupt} The structure of one of the files that contains Registry data is corrupt, or the image of the file in memory is corrupt, or the file could not be recovered because the alternate copy or log was absent or corrupt.";
		case /*STATUS_REGISTRY_IO_FAILED*/(DWORD)0xC000014DL: return "An I/O operation initiated by the Registry failed unrecoverably. The Registry could not read in, or write out, or flush, one of the files that contain the system's image of the Registry.";
		case /*STATUS_NO_EVENT_PAIR*/(DWORD)0xC000014EL: return "An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread.";
		case /*STATUS_UNRECOGNIZED_VOLUME*/(DWORD)0xC000014FL: return "The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupt.";
		case /*STATUS_SERIAL_NO_DEVICE_INITED*/(DWORD)0xC0000150L: return "No serial device was successfully initialized. The serial driver will unload.";
		case /*STATUS_NO_SUCH_ALIAS*/(DWORD)0xC0000151L: return "The specified local group does not exist.";
		case /*STATUS_MEMBER_NOT_IN_ALIAS*/(DWORD)0xC0000152L: return "The specified account name is not a member of the group.";
		case /*STATUS_MEMBER_IN_ALIAS*/(DWORD)0xC0000153L: return "The specified account name is already a member of the group.";
		case /*STATUS_ALIAS_EXISTS*/(DWORD)0xC0000154L: return "The specified local group already exists.";
		case /*STATUS_LOGON_NOT_GRANTED*/(DWORD)0xC0000155L: return "A requested type of logon (e.g., Interactive, Network, Service) is not granted by the target system's local security policy. Please ask the system administrator to grant the necessary form of logon.";
		case /*STATUS_TOO_MANY_SECRETS*/(DWORD)0xC0000156L: return "The maximum number of secrets that may be stored in a single system has been exceeded. The length and number of secrets is limited to satisfy United States State Department export restrictions.";
		case /*STATUS_SECRET_TOO_LONG*/(DWORD)0xC0000157L: return "The length of a secret exceeds the maximum length allowed. The length and number of secrets is limited to satisfy United States State Department export restrictions.";
		case /*STATUS_INTERNAL_DB_ERROR*/(DWORD)0xC0000158L: return "The Local Security Authority (LSA) database contains an internal inconsistency.";
		case /*STATUS_FULLSCREEN_MODE*/(DWORD)0xC0000159L: return "The requested operation cannot be performed in fullscreen mode.";
		case /*STATUS_TOO_MANY_CONTEXT_IDS*/(DWORD)0xC000015AL: return "During a logon attempt, the user's security context accumulated too many security IDs. This is a very unusual situation. Remove the user from some global or local groups to reduce the number of security ids to incorporate into the security context.";
		case /*STATUS_LOGON_TYPE_NOT_GRANTED*/(DWORD)0xC000015BL: return "A user has requested a type of logon (e.g., interactive or network) that has not been granted. An administrator has control over who may logon interactively and through the network.";
		case /*STATUS_NOT_REGISTRY_FILE*/(DWORD)0xC000015CL: return "The system has attempted to load or restore a file into the registry, and the specified file is not in the format of a registry file.";
		case /*STATUS_NT_CROSS_ENCRYPTION_REQUIRED*/(DWORD)0xC000015DL: return "An attempt was made to change a user password in the security account manager without providing the necessary Windows cross-encrypted password.";
		case /*STATUS_DOMAIN_CTRLR_CONFIG_ERROR*/(DWORD)0xC000015EL: return "A Windows Server has an incorrect configuration.";
		case /*STATUS_FT_MISSING_MEMBER*/(DWORD)0xC000015FL: return "An attempt was made to explicitly access the secondary copy of information via a device control to the Fault Tolerance driver and the secondary copy is not present in the system.";
		case /*STATUS_ILL_FORMED_SERVICE_ENTRY*/(DWORD)0xC0000160L: return "A configuration registry node representing a driver service entry was ill-formed and did not contain required value entries.";
		case /*STATUS_ILLEGAL_CHARACTER*/(DWORD)0xC0000161L: return "An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE.";
		case /*STATUS_UNMAPPABLE_CHARACTER*/(DWORD)0xC0000162L: return "No mapping for the Unicode character exists in the target multi-byte code page.";
		case /*STATUS_UNDEFINED_CHARACTER*/(DWORD)0xC0000163L: return "The Unicode character is not defined in the Unicode character set installed on the system.";
		case /*STATUS_FLOPPY_VOLUME*/(DWORD)0xC0000164L: return "The paging file cannot be created on a floppy diskette.";
		case /*STATUS_FLOPPY_ID_MARK_NOT_FOUND*/(DWORD)0xC0000165L: return "{Floppy Disk Error} While accessing a floppy disk, an ID address mark was not found.";
		case /*STATUS_FLOPPY_WRONG_CYLINDER*/(DWORD)0xC0000166L: return "{Floppy Disk Error} While accessing a floppy disk, the track address from the sector ID field was found to be different than the track address maintained by the controller.";
		case /*STATUS_FLOPPY_UNKNOWN_ERROR*/(DWORD)0xC0000167L: return "{Floppy Disk Error} The floppy disk controller reported an error that is not recognized by the floppy disk driver.";
		case /*STATUS_FLOPPY_BAD_REGISTERS*/(DWORD)0xC0000168L: return "{Floppy Disk Error} While accessing a floppy-disk, the controller returned inconsistent results via its registers.";
		case /*STATUS_DISK_RECALIBRATE_FAILED*/(DWORD)0xC0000169L: return "{Hard Disk Error} While accessing the hard disk, a recalibrate operation failed, even after retries.";
		case /*STATUS_DISK_OPERATION_FAILED*/(DWORD)0xC000016AL: return "{Hard Disk Error} While accessing the hard disk, a disk operation failed even after retries.";
		case /*STATUS_DISK_RESET_FAILED*/(DWORD)0xC000016BL: return "{Hard Disk Error} While accessing the hard disk, a disk controller reset was needed, but even that failed.";
		case /*STATUS_SHARED_IRQ_BUSY*/(DWORD)0xC000016CL: return "An attempt was made to open a device that was sharing an IRQ with other devices. At least one other device that uses that IRQ was already opened. Two concurrent opens of devices that share an IRQ and only work via interrupts is not supported for the particular bus type that the devices use.";
		case /*STATUS_FT_ORPHANING*/(DWORD)0xC000016DL: return "{FT Orphaning} A disk that is part of a fault-tolerant volume can no longer be accessed.";
		case /*STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT*/(DWORD)0xC000016EL: return "The system bios failed to connect a system interrupt to the device or bus for which the device is connected.";
		case /*STATUS_PARTITION_FAILURE*/(DWORD)0xC0000172L: return "Tape could not be partitioned.";
		case /*STATUS_INVALID_BLOCK_LENGTH*/(DWORD)0xC0000173L: return "When accessing a new tape of a multivolume partition, the current blocksize is incorrect.";
		case /*STATUS_DEVICE_NOT_PARTITIONED*/(DWORD)0xC0000174L: return "Tape partition information could not be found when loading a tape.";
		case /*STATUS_UNABLE_TO_LOCK_MEDIA*/(DWORD)0xC0000175L: return "Attempt to lock the eject media mechanism fails.";
		case /*STATUS_UNABLE_TO_UNLOAD_MEDIA*/(DWORD)0xC0000176L: return "Unload media fails.";
		case /*STATUS_EOM_OVERFLOW*/(DWORD)0xC0000177L: return "Physical end of tape was detected.";
		case /*STATUS_NO_MEDIA*/(DWORD)0xC0000178L: return "{No Media} There is no media in the drive. Please insert media into drive %hs.";
		case /*STATUS_NO_SUCH_MEMBER*/(DWORD)0xC000017AL: return "A member could not be added to or removed from the local group because the member does not exist.";
		case /*STATUS_INVALID_MEMBER*/(DWORD)0xC000017BL: return "A new member could not be added to a local group because the member has the wrong account type.";
		case /*STATUS_KEY_DELETED*/(DWORD)0xC000017CL: return "Illegal operation attempted on a registry key which has been marked for deletion.";
		case /*STATUS_NO_LOG_SPACE*/(DWORD)0xC000017DL: return "System could not allocate required space in a registry log.";
		case /*STATUS_TOO_MANY_SIDS*/(DWORD)0xC000017EL: return "Too many Sids have been specified.";
		case /*STATUS_LM_CROSS_ENCRYPTION_REQUIRED*/(DWORD)0xC000017FL: return "An attempt was made to change a user password in the security account manager without providing the necessary LM cross-encrypted password.";
		case /*STATUS_KEY_HAS_CHILDREN*/(DWORD)0xC0000180L: return "An attempt was made to create a symbolic link in a registry key that already has subkeys or values.";
		case /*STATUS_CHILD_MUST_BE_VOLATILE*/(DWORD)0xC0000181L: return "An attempt was made to create a Stable subkey under a Volatile parent key.";
		case /*STATUS_DEVICE_CONFIGURATION_ERROR*/(DWORD)0xC0000182L: return "The I/O device is configured incorrectly or the configuration parameters to the driver are incorrect.";
		case /*STATUS_DRIVER_INTERNAL_ERROR*/(DWORD)0xC0000183L: return "An error was detected between two drivers or within an I/O driver.";
		case /*STATUS_INVALID_DEVICE_STATE*/(DWORD)0xC0000184L: return "The device is not in a valid state to perform this request.";
		case /*STATUS_IO_DEVICE_ERROR*/(DWORD)0xC0000185L: return "The I/O device reported an I/O error.";
		case /*STATUS_DEVICE_PROTOCOL_ERROR*/(DWORD)0xC0000186L: return "A protocol error was detected between the driver and the device.";
		case /*STATUS_BACKUP_CONTROLLER*/(DWORD)0xC0000187L: return "This operation is only allowed for the Primary Domain Controller of the domain.";
		case /*STATUS_LOG_FILE_FULL*/(DWORD)0xC0000188L: return "Log file space is insufficient to support this operation.";
		case /*STATUS_TOO_LATE*/(DWORD)0xC0000189L: return "A write operation was attempted to a volume after it was dismounted.";
		case /*STATUS_NO_TRUST_LSA_SECRET*/(DWORD)0xC000018AL: return "The workstation does not have a trust secret for the primary domain in the local LSA database.";
		case /*STATUS_NO_TRUST_SAM_ACCOUNT*/(DWORD)0xC000018BL: return "The SAM database on the Windows Server does not have a computer account for this workstation trust relationship.";
		case /*STATUS_TRUSTED_DOMAIN_FAILURE*/(DWORD)0xC000018CL: return "The logon request failed because the trust relationship between the primary domain and the trusted domain failed.";
		case /*STATUS_TRUSTED_RELATIONSHIP_FAILURE*/(DWORD)0xC000018DL: return "The logon request failed because the trust relationship between this workstation and the primary domain failed.";
		case /*STATUS_EVENTLOG_FILE_CORRUPT*/(DWORD)0xC000018EL: return "The Eventlog log file is corrupt.";
		case /*STATUS_EVENTLOG_CANT_START*/(DWORD)0xC000018FL: return "No Eventlog log file could be opened. The Eventlog service did not start.";
		case /*STATUS_TRUST_FAILURE*/(DWORD)0xC0000190L: return "The network logon failed. This may be because the validation authority can't be reached.";
		case /*STATUS_MUTANT_LIMIT_EXCEEDED*/(DWORD)0xC0000191L: return "An attempt was made to acquire a mutant such that its maximum count would have been exceeded.";
		case /*STATUS_NETLOGON_NOT_STARTED*/(DWORD)0xC0000192L: return "An attempt was made to logon, but the netlogon service was not started.";
		case /*STATUS_ACCOUNT_EXPIRED*/(DWORD)0xC0000193L: return "The user's account has expired.";
		case /*STATUS_POSSIBLE_DEADLOCK*/(DWORD)0xC0000194L: return "{POSSIBLE_DEADLOCK} Possible deadlock condition.";
		case /*STATUS_NETWORK_CREDENTIAL_CONFLICT*/(DWORD)0xC0000195L: return "Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again.";
		case /*STATUS_REMOTE_SESSION_LIMIT*/(DWORD)0xC0000196L: return "An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.";
		case /*STATUS_EVENTLOG_FILE_CHANGED*/(DWORD)0xC0000197L: return "The log file has changed between reads.";
		case /*STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT*/(DWORD)0xC0000198L: return "The account used is an Interdomain Trust account. Use your global user account or local user account to access this server.";
		case /*STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT*/(DWORD)0xC0000199L: return "The account used is a Computer Account. Use your global user account or local user account to access this server.";
		case /*STATUS_NOLOGON_SERVER_TRUST_ACCOUNT*/(DWORD)0xC000019AL: return "The account used is an Server Trust account. Use your global user account or local user account to access this server.";
		case /*STATUS_DOMAIN_TRUST_INCONSISTENT*/(DWORD)0xC000019BL: return "The name or SID of the domain specified is inconsistent with the trust information for that domain.";
		case /*STATUS_FS_DRIVER_REQUIRED*/(DWORD)0xC000019CL: return "A volume has been accessed for which a file system driver is required that has not yet been loaded.";
		case /*STATUS_IMAGE_ALREADY_LOADED_AS_DLL*/(DWORD)0xC000019DL: return "Indicates that the specified image is already loaded as a DLL.";
		case /*STATUS_NETWORK_OPEN_RESTRICTION*/(DWORD)0xC0000201L: return "A remote open failed because the network open restrictions were not satisfied.";
		case /*STATUS_NO_USER_SESSION_KEY*/(DWORD)0xC0000202L: return "There is no user session key for the specified logon session.";
		case /*STATUS_USER_SESSION_DELETED*/(DWORD)0xC0000203L: return "The remote user session has been deleted.";
		case /*STATUS_RESOURCE_LANG_NOT_FOUND*/(DWORD)0xC0000204L: return "Indicates the specified resource language ID cannot be found in the image file.";
		case /*STATUS_INSUFF_SERVER_RESOURCES*/(DWORD)0xC0000205L: return "Insufficient server resources exist to complete the request.";
		case /*STATUS_INVALID_BUFFER_SIZE*/(DWORD)0xC0000206L: return "The size of the buffer is invalid for the specified operation.";
		case /*STATUS_INVALID_ADDRESS_COMPONENT*/(DWORD)0xC0000207L: return "The transport rejected the network address specified as invalid.";
		case /*STATUS_INVALID_ADDRESS_WILDCARD*/(DWORD)0xC0000208L: return "The transport rejected the network address specified due to an invalid use of a wildcard.";
		case /*STATUS_TOO_MANY_ADDRESSES*/(DWORD)0xC0000209L: return "The transport address could not be opened because all the available addresses are in use.";
		case /*STATUS_ADDRESS_ALREADY_EXISTS*/(DWORD)0xC000020AL: return "The transport address could not be opened because it already exists.";
		case /*STATUS_ADDRESS_CLOSED*/(DWORD)0xC000020BL: return "The transport address is now closed.";
		case /*STATUS_CONNECTION_DISCONNECTED*/(DWORD)0xC000020CL: return "The transport connection is now disconnected.";
		case /*STATUS_CONNECTION_RESET*/(DWORD)0xC000020DL: return "The transport connection has been reset.";
		case /*STATUS_TOO_MANY_NODES*/(DWORD)0xC000020EL: return "The transport cannot dynamically acquire any more nodes.";
		case /*STATUS_TRANSACTION_ABORTED*/(DWORD)0xC000020FL: return "The transport aborted a pending transaction.";
		case /*STATUS_TRANSACTION_TIMED_OUT*/(DWORD)0xC0000210L: return "The transport timed out a request waiting for a response.";
		case /*STATUS_TRANSACTION_NO_RELEASE*/(DWORD)0xC0000211L: return "The transport did not receive a release for a pending response.";
		case /*STATUS_TRANSACTION_NO_MATCH*/(DWORD)0xC0000212L: return "The transport did not find a transaction matching the specific token.";
		case /*STATUS_TRANSACTION_RESPONDED*/(DWORD)0xC0000213L: return "The transport had previously responded to a transaction request.";
		case /*STATUS_TRANSACTION_INVALID_ID*/(DWORD)0xC0000214L: return "The transport does not recognized the transaction request identifier specified.";
		case /*STATUS_TRANSACTION_INVALID_TYPE*/(DWORD)0xC0000215L: return "The transport does not recognize the transaction request type specified.";
		case /*STATUS_NOT_SERVER_SESSION*/(DWORD)0xC0000216L: return "The transport can only process the specified request on the server side of a session.";
		case /*STATUS_NOT_CLIENT_SESSION*/(DWORD)0xC0000217L: return "The transport can only process the specified request on the client side of a session.";
		case /*STATUS_CANNOT_LOAD_REGISTRY_FILE*/(DWORD)0xC0000218L: return "{Registry File Failure} The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable.";
		case /*STATUS_DEBUG_ATTACH_FAILED*/(DWORD)0xC0000219L: return "{Unexpected Failure in DebugActiveProcess} An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error.";
		case /*STATUS_SYSTEM_PROCESS_TERMINATED*/(DWORD)0xC000021AL: return "{Fatal System Error} The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down.";
		case /*STATUS_DATA_NOT_ACCEPTED*/(DWORD)0xC000021BL: return "{Data Not Accepted} The TDI client could not handle the data received during an indication.";
		case /*STATUS_NO_BROWSER_SERVERS_FOUND*/(DWORD)0xC000021CL: return "{Unable to Retrieve Browser Server List} The list of servers for this workgroup is not currently available.";
		case /*STATUS_VDM_HARD_ERROR*/(DWORD)0xC000021DL: return "NTVDM encountered a hard error.";
		case /*STATUS_DRIVER_CANCEL_TIMEOUT*/(DWORD)0xC000021EL: return "{Cancel Timeout} The driver %hs failed to complete a cancelled I/O request in the allotted time.";
		case /*STATUS_REPLY_MESSAGE_MISMATCH*/(DWORD)0xC000021FL: return "{Reply Message Mismatch} An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message.";
		case /*STATUS_MAPPED_ALIGNMENT*/(DWORD)0xC0000220L: return "{Mapped View Alignment Incorrect} An attempt was made to map a view of a file, but either the specified base address or the offset into the file were not aligned on the proper allocation granularity.";
		case /*STATUS_IMAGE_CHECKSUM_MISMATCH*/(DWORD)0xC0000221L: return "{Bad Image Checksum} The image %hs is possibly corrupt. The header checksum does not match the computed checksum.";
		case /*STATUS_LOST_WRITEBEHIND_DATA*/(DWORD)0xC0000222L: return "{Delayed Write Failed} Windows was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.";
		case /*STATUS_CLIENT_SERVER_PARAMETERS_INVALID*/(DWORD)0xC0000223L: return "The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window.";
		case /*STATUS_PASSWORD_MUST_CHANGE*/(DWORD)0xC0000224L: return "The user's password must be changed before logging on the first time.";
		case /*STATUS_NOT_FOUND*/(DWORD)0xC0000225L: return "The object was not found.";
		case /*STATUS_NOT_TINY_STREAM*/(DWORD)0xC0000226L: return "The stream is not a tiny stream.";
		case /*STATUS_RECOVERY_FAILURE*/(DWORD)0xC0000227L: return "A transaction recover failed.";
		case /*STATUS_STACK_OVERFLOW_READ*/(DWORD)0xC0000228L: return "The request must be handled by the stack overflow code.";
		case /*STATUS_FAIL_CHECK*/(DWORD)0xC0000229L: return "A consistency check failed.";
		case /*STATUS_DUPLICATE_OBJECTID*/(DWORD)0xC000022AL: return "The attempt to insert the ID in the index failed because the ID is already in the index.";
		case /*STATUS_OBJECTID_EXISTS*/(DWORD)0xC000022BL: return "The attempt to set the object's ID failed because the object already has an ID.";
		case /*STATUS_CONVERT_TO_LARGE*/(DWORD)0xC000022CL: return "Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream.";
		case /*STATUS_RETRY*/(DWORD)0xC000022DL: return "The request needs to be retried.";
		case /*STATUS_FOUND_OUT_OF_SCOPE*/(DWORD)0xC000022EL: return "The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation.";
		case /*STATUS_ALLOCATE_BUCKET*/(DWORD)0xC000022FL: return "The bucket array must be grown. Retry transaction after doing so.";
		case /*STATUS_PROPSET_NOT_FOUND*/(DWORD)0xC0000230L: return "The property set specified does not exist on the object.";
		case /*STATUS_MARSHALL_OVERFLOW*/(DWORD)0xC0000231L: return "The user/kernel marshalling buffer has overflowed.";
		case /*STATUS_INVALID_VARIANT*/(DWORD)0xC0000232L: return "The supplied variant structure contains invalid data.";
		case /*STATUS_DOMAIN_CONTROLLER_NOT_FOUND*/(DWORD)0xC0000233L: return "Could not find a domain controller for this domain.";
		case /*STATUS_ACCOUNT_LOCKED_OUT*/(DWORD)0xC0000234L: return "The user account has been automatically locked because too many invalid logon attempts or password change attempts have been requested.";
		case /*STATUS_HANDLE_NOT_CLOSABLE*/(DWORD)0xC0000235L: return "NtClose was called on a handle that was protected from close via NtSetInformationObject.";
		case /*STATUS_CONNECTION_REFUSED*/(DWORD)0xC0000236L: return "The transport connection attempt was refused by the remote system.";
		case /*STATUS_GRACEFUL_DISCONNECT*/(DWORD)0xC0000237L: return "The transport connection was gracefully closed.";
		case /*STATUS_ADDRESS_ALREADY_ASSOCIATED*/(DWORD)0xC0000238L: return "The transport endpoint already has an address associated with it.";
		case /*STATUS_ADDRESS_NOT_ASSOCIATED*/(DWORD)0xC0000239L: return "An address has not yet been associated with the transport endpoint.";
		case /*STATUS_CONNECTION_INVALID*/(DWORD)0xC000023AL: return "An operation was attempted on a nonexistent transport connection.";
		case /*STATUS_CONNECTION_ACTIVE*/(DWORD)0xC000023BL: return "An invalid operation was attempted on an active transport connection.";
		case /*STATUS_NETWORK_UNREACHABLE*/(DWORD)0xC000023CL: return "The remote network is not reachable by the transport.";
		case /*STATUS_HOST_UNREACHABLE*/(DWORD)0xC000023DL: return "The remote system is not reachable by the transport.";
		case /*STATUS_PROTOCOL_UNREACHABLE*/(DWORD)0xC000023EL: return "The remote system does not support the transport protocol.";
		case /*STATUS_PORT_UNREACHABLE*/(DWORD)0xC000023FL: return "No service is operating at the destination port of the transport on the remote system.";
		case /*STATUS_REQUEST_ABORTED*/(DWORD)0xC0000240L: return "The request was aborted.";
		case /*STATUS_CONNECTION_ABORTED*/(DWORD)0xC0000241L: return "The transport connection was aborted by the local system.";
		case /*STATUS_BAD_COMPRESSION_BUFFER*/(DWORD)0xC0000242L: return "The specified buffer contains ill-formed data.";
		case /*STATUS_USER_MAPPED_FILE*/(DWORD)0xC0000243L: return "The requested operation cannot be performed on a file with a user mapped section open.";
		case /*STATUS_AUDIT_FAILED*/(DWORD)0xC0000244L: return "{Audit Failed} An attempt to generate a security audit failed.";
		case /*STATUS_TIMER_RESOLUTION_NOT_SET*/(DWORD)0xC0000245L: return "The timer resolution was not previously set by the current process.";
		case /*STATUS_CONNECTION_COUNT_LIMIT*/(DWORD)0xC0000246L: return "A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.";
		case /*STATUS_LOGIN_TIME_RESTRICTION*/(DWORD)0xC0000247L: return "Attempting to login during an unauthorized time of day for this account.";
		case /*STATUS_LOGIN_WKSTA_RESTRICTION*/(DWORD)0xC0000248L: return "The account is not authorized to login from this station.";
		case /*STATUS_IMAGE_MP_UP_MISMATCH*/(DWORD)0xC0000249L: return "{UP/MP Image Mismatch} The image %hs has been modified for use on a uniprocessor system, but you are running it on a multiprocessor machine. Please reinstall the image file.";
		case /*STATUS_INSUFFICIENT_LOGON_INFO*/(DWORD)0xC0000250L: return "There is insufficient account information to log you on.";
		case /*STATUS_BAD_DLL_ENTRYPOINT*/(DWORD)0xC0000251L: return "{Invalid DLL Entrypoint} The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly.";
		case /*STATUS_BAD_SERVICE_ENTRYPOINT*/(DWORD)0xC0000252L: return "{Invalid Service Callback Entrypoint} The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly.";
		case /*STATUS_LPC_REPLY_LOST*/(DWORD)0xC0000253L: return "The server received the messages but did not send a reply.";
		case /*STATUS_IP_ADDRESS_CONFLICT1*/(DWORD)0xC0000254L: return "There is an IP address conflict with another system on the network";
		case /*STATUS_IP_ADDRESS_CONFLICT2*/(DWORD)0xC0000255L: return "There is an IP address conflict with another system on the network";
		case /*STATUS_REGISTRY_QUOTA_LIMIT*/(DWORD)0xC0000256L: return "{Low On Registry Space} The system has reached the maximum size allowed for the system part of the registry.  Additional storage requests will be ignored.";
		case /*STATUS_PATH_NOT_COVERED*/(DWORD)0xC0000257L: return "The contacted server does not support the indicated part of the DFS namespace.";
		case /*STATUS_NO_CALLBACK_ACTIVE*/(DWORD)0xC0000258L: return "A callback return system service cannot be executed when no callback is active.";
		case /*STATUS_LICENSE_QUOTA_EXCEEDED*/(DWORD)0xC0000259L: return "The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept.";
		case /*STATUS_PWD_TOO_SHORT*/(DWORD)0xC000025AL: return "The password provided is too short to meet the policy of your user account. Please choose a longer password.";
		case /*STATUS_PWD_TOO_RECENT*/(DWORD)0xC000025BL: return "The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned.";
		case /*STATUS_PWD_HISTORY_CONFLICT*/(DWORD)0xC000025CL: return "You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used.";
		case /*STATUS_PLUGPLAY_NO_DEVICE*/(DWORD)0xC000025EL: return "You have attempted to load a legacy device driver while its device instance had been disabled.";
		case /*STATUS_UNSUPPORTED_COMPRESSION*/(DWORD)0xC000025FL: return "The specified compression format is unsupported.";
		case /*STATUS_INVALID_HW_PROFILE*/(DWORD)0xC0000260L: return "The specified hardware profile configuration is invalid.";
		case /*STATUS_INVALID_PLUGPLAY_DEVICE_PATH*/(DWORD)0xC0000261L: return "The specified Plug and Play registry device path is invalid.";
		case /*STATUS_DRIVER_ORDINAL_NOT_FOUND*/(DWORD)0xC0000262L: return "{Driver Entry Point Not Found} The %hs device driver could not locate the ordinal %ld in driver %hs.";
		case /*STATUS_DRIVER_ENTRYPOINT_NOT_FOUND*/(DWORD)0xC0000263L: return "{Driver Entry Point Not Found} The %hs device driver could not locate the entry point %hs in driver %hs.";
		case /*STATUS_RESOURCE_NOT_OWNED*/(DWORD)0xC0000264L: return "{Application Error} The application attempted to release a resource it did not own. Click OK to terminate the application.";
		case /*STATUS_TOO_MANY_LINKS*/(DWORD)0xC0000265L: return "An attempt was made to create more links on a file than the file system supports.";
		case /*STATUS_QUOTA_LIST_INCONSISTENT*/(DWORD)0xC0000266L: return "The specified quota list is internally inconsistent with its descriptor.";
		case /*STATUS_FILE_IS_OFFLINE*/(DWORD)0xC0000267L: return "The specified file has been relocated to offline storage.";
		case /*STATUS_EVALUATION_EXPIRATION*/(DWORD)0xC0000268L: return "{Windows Evaluation Notification} The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product.";
		case /*STATUS_ILLEGAL_DLL_RELOCATION*/(DWORD)0xC0000269L: return "{Illegal System DLL Relocation} The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for Windows system DLLs. The vendor supplying the DLL should be contacted for a new DLL.";
		case /*STATUS_LICENSE_VIOLATION*/(DWORD)0xC000026AL: return "{License Violation} The system has detected tampering with your registered product type. This is a violation of your software license. Tampering with product type is not permitted.";
		case /*STATUS_DLL_INIT_FAILED_LOGOFF*/(DWORD)0xC000026BL: return "{DLL Initialization Failed} The application failed to initialize because the window station is shutting down.";
		case /*STATUS_DRIVER_UNABLE_TO_LOAD*/(DWORD)0xC000026CL: return "{Unable to Load Device Driver} %hs device driver could not be loaded. Error Status was 0x%x";
		case /*STATUS_DFS_UNAVAILABLE*/(DWORD)0xC000026DL: return "DFS is unavailable on the contacted server.";
		case /*STATUS_VOLUME_DISMOUNTED*/(DWORD)0xC000026EL: return "An operation was attempted to a volume after it was dismounted.";
		case /*STATUS_WX86_INTERNAL_ERROR*/(DWORD)0xC000026FL: return "An internal error occurred in the Win32 x86 emulation subsystem.";
		case /*STATUS_WX86_FLOAT_STACK_CHECK*/(DWORD)0xC0000270L: return "Win32 x86 emulation subsystem Floating-point stack check.";
		case /*STATUS_VALIDATE_CONTINUE*/(DWORD)0xC0000271L: return "The validation process needs to continue on to the next step.";
		case /*STATUS_NO_MATCH*/(DWORD)0xC0000272L: return "There was no match for the specified key in the index.";
		case /*STATUS_NO_MORE_MATCHES*/(DWORD)0xC0000273L: return "There are no more matches for the current index enumeration.";
		case /*STATUS_NOT_A_REPARSE_POINT*/(DWORD)0xC0000275L: return "The NTFS file or directory is not a reparse point.";
		case /*STATUS_IO_REPARSE_TAG_INVALID*/(DWORD)0xC0000276L: return "The Windows I/O reparse tag passed for the NTFS reparse point is invalid.";
		case /*STATUS_IO_REPARSE_TAG_MISMATCH*/(DWORD)0xC0000277L: return "The Windows I/O reparse tag does not match the one present in the NTFS reparse point.";
		case /*STATUS_IO_REPARSE_DATA_INVALID*/(DWORD)0xC0000278L: return "The user data passed for the NTFS reparse point is invalid.";
		case /*STATUS_IO_REPARSE_TAG_NOT_HANDLED*/(DWORD)0xC0000279L: return "The layered file system driver for this IO tag did not handle it when needed.";
		case /*STATUS_REPARSE_POINT_NOT_RESOLVED*/(DWORD)0xC0000280L: return "The NTFS symbolic link could not be resolved even though the initial file name is valid.";
		case /*STATUS_DIRECTORY_IS_A_REPARSE_POINT*/(DWORD)0xC0000281L: return "The NTFS directory is a reparse point.";
		case /*STATUS_RANGE_LIST_CONFLICT*/(DWORD)0xC0000282L: return "The range could not be added to the range list because of a conflict.";
		case /*STATUS_SOURCE_ELEMENT_EMPTY*/(DWORD)0xC0000283L: return "The specified medium changer source element contains no media.";
		case /*STATUS_DESTINATION_ELEMENT_FULL*/(DWORD)0xC0000284L: return "The specified medium changer destination element already contains media.";
		case /*STATUS_ILLEGAL_ELEMENT_ADDRESS*/(DWORD)0xC0000285L: return "The specified medium changer element does not exist.";
		case /*STATUS_MAGAZINE_NOT_PRESENT*/(DWORD)0xC0000286L: return "The specified element is contained within a magazine that is no longer present.";
		case /*STATUS_REINITIALIZATION_NEEDED*/(DWORD)0xC0000287L: return "The device requires reinitialization due to hardware errors.";
		case /*STATUS_DEVICE_REQUIRES_CLEANING*/(DWORD)0x80000288L: return "The device has indicated that cleaning is necessary.";
		case /*STATUS_DEVICE_DOOR_OPEN*/(DWORD)0x80000289L: return "The device has indicated that it's door is open. Further operations require it closed and secured.";
		case /*STATUS_ENCRYPTION_FAILED*/(DWORD)0xC000028AL: return "The file encryption attempt failed.";
		case /*STATUS_DECRYPTION_FAILED*/(DWORD)0xC000028BL: return "The file decryption attempt failed.";
		case /*STATUS_RANGE_NOT_FOUND*/(DWORD)0xC000028CL: return "The specified range could not be found in the range list.";
		case /*STATUS_NO_RECOVERY_POLICY*/(DWORD)0xC000028DL: return "There is no encryption recovery policy configured for this system.";
		case /*STATUS_NO_EFS*/(DWORD)0xC000028EL: return "The required encryption driver is not loaded for this system.";
		case /*STATUS_WRONG_EFS*/(DWORD)0xC000028FL: return "The file was encrypted with a different encryption driver than is currently loaded.";
		case /*STATUS_NO_USER_KEYS*/(DWORD)0xC0000290L: return "There are no EFS keys defined for the user.";
		case /*STATUS_FILE_NOT_ENCRYPTED*/(DWORD)0xC0000291L: return "The specified file is not encrypted.";
		case /*STATUS_NOT_EXPORT_FORMAT*/(DWORD)0xC0000292L: return "The specified file is not in the defined EFS export format.";
		case /*STATUS_FILE_ENCRYPTED*/(DWORD)0xC0000293L: return "The specified file is encrypted and the user does not have the ability to decrypt it.";
		case /*STATUS_WAKE_SYSTEM*/(DWORD)0x40000294L: return "The system has awoken";
		case /*STATUS_WMI_GUID_NOT_FOUND*/(DWORD)0xC0000295L: return "The guid passed was not recognized as valid by a WMI data provider.";
		case /*STATUS_WMI_INSTANCE_NOT_FOUND*/(DWORD)0xC0000296L: return "The instance name passed was not recognized as valid by a WMI data provider.";
		case /*STATUS_WMI_ITEMID_NOT_FOUND*/(DWORD)0xC0000297L: return "The data item id passed was not recognized as valid by a WMI data provider.";
		case /*STATUS_WMI_TRY_AGAIN*/(DWORD)0xC0000298L: return "The WMI request could not be completed and should be retried.";
		case /*STATUS_SHARED_POLICY*/(DWORD)0xC0000299L: return "The policy object is shared and can only be modified at the root";
		case /*STATUS_POLICY_OBJECT_NOT_FOUND*/(DWORD)0xC000029AL: return "The policy object does not exist when it should";
		case /*STATUS_POLICY_ONLY_IN_DS*/(DWORD)0xC000029BL: return "The requested policy information only lives in the Ds";
		case /*STATUS_VOLUME_NOT_UPGRADED*/(DWORD)0xC000029CL: return "The volume must be upgraded to enable this feature";
		case /*STATUS_REMOTE_STORAGE_NOT_ACTIVE*/(DWORD)0xC000029DL: return "The remote storage service is not operational at this time.";
		case /*STATUS_REMOTE_STORAGE_MEDIA_ERROR*/(DWORD)0xC000029EL: return "The remote storage service encountered a media error.";
		case /*STATUS_NO_TRACKING_SERVICE*/(DWORD)0xC000029FL: return "The tracking (workstation) service is not running.";
		case /*STATUS_SERVER_SID_MISMATCH*/(DWORD)0xC00002A0L: return "The server process is running under a SID different than that required by client.";
		case /*STATUS_DS_NO_ATTRIBUTE_OR_VALUE*/(DWORD)0xC00002A1L: return "Directory Service specific Errors The specified directory service attribute or value does not exist.";
		case /*STATUS_DS_INVALID_ATTRIBUTE_SYNTAX*/(DWORD)0xC00002A2L: return "The attribute syntax specified to the directory service is invalid.";
		case /*STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED*/(DWORD)0xC00002A3L: return "The attribute type specified to the directory service is not defined.";
		case /*STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS*/(DWORD)0xC00002A4L: return "The specified directory service attribute or value already exists.";
		case /*STATUS_DS_BUSY*/(DWORD)0xC00002A5L: return "The directory service is busy.";
		case /*STATUS_DS_UNAVAILABLE*/(DWORD)0xC00002A6L: return "The directory service is not available.";
		case /*STATUS_DS_NO_RIDS_ALLOCATED*/(DWORD)0xC00002A7L: return "The directory service was unable to allocate a relative identifier.";
		case /*STATUS_DS_NO_MORE_RIDS*/(DWORD)0xC00002A8L: return "The directory service has exhausted the pool of relative identifiers.";
		case /*STATUS_DS_INCORRECT_ROLE_OWNER*/(DWORD)0xC00002A9L: return "The requested operation could not be performed because the directory service is not the master for that type of operation.";
		case /*STATUS_DS_RIDMGR_INIT_ERROR*/(DWORD)0xC00002AAL: return "The directory service was unable to initialize the subsystem that allocates relative identifiers.";
		case /*STATUS_DS_OBJ_CLASS_VIOLATION*/(DWORD)0xC00002ABL: return "The requested operation did not satisfy one or more constraints associated with the class of the object.";
		case /*STATUS_DS_CANT_ON_NON_LEAF*/(DWORD)0xC00002ACL: return "The directory service can perform the requested operation only on a leaf object.";
		case /*STATUS_DS_CANT_ON_RDN*/(DWORD)0xC00002ADL: return "The directory service cannot perform the requested operation on the Relatively Defined Name (RDN) attribute of an object.";
		case /*STATUS_DS_CANT_MOD_OBJ_CLASS*/(DWORD)0xC00002AEL: return "The directory service detected an attempt to modify the object class of an object.";
		case /*STATUS_DS_CROSS_DOM_MOVE_FAILED*/(DWORD)0xC00002AFL: return "An error occurred while performing a cross domain move operation.";
		case /*STATUS_DS_GC_NOT_AVAILABLE*/(DWORD)0xC00002B0L: return "Unable to Contact the Global Catalog Server.";
		case /*STATUS_DIRECTORY_SERVICE_REQUIRED*/(DWORD)0xC00002B1L: return "The requested operation requires a directory service, and none was available.";
		case /*STATUS_REPARSE_ATTRIBUTE_CONFLICT*/(DWORD)0xC00002B2L: return "The reparse attribute cannot be set as it is incompatible with an existing attribute.";
		case /*STATUS_CANT_ENABLE_DENY_ONLY*/(DWORD)0xC00002B3L: return "A group marked use for deny only cannot be enabled.";
		case /*STATUS_FLOAT_MULTIPLE_FAULTS*/(DWORD)0xC00002B4L: return "{FLOAT_MULTIPLE_FAULTS} Multiple floating point faults.";
		case /*STATUS_FLOAT_MULTIPLE_TRAPS*/(DWORD)0xC00002B5L: return "{FLOAT_MULTIPLE_TRAPS} Multiple floating point traps.";
		case /*STATUS_DEVICE_REMOVED*/(DWORD)0xC00002B6L: return "The device has been removed.";
		case /*STATUS_JOURNAL_DELETE_IN_PROGRESS*/(DWORD)0xC00002B7L: return "The volume change journal is being deleted.";
		case /*STATUS_JOURNAL_NOT_ACTIVE*/(DWORD)0xC00002B8L: return "The volume change journal is not active.";
		case /*STATUS_NOINTERFACE*/(DWORD)0xC00002B9L: return "The requested interface is not supported.";
		case /*STATUS_DS_ADMIN_LIMIT_EXCEEDED*/(DWORD)0xC00002C1L: return "A directory service resource limit has been exceeded.";
		case /*STATUS_DRIVER_FAILED_SLEEP*/(DWORD)0xC00002C2L: return "{System Standby Failed} The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode.";
		case /*STATUS_MUTUAL_AUTHENTICATION_FAILED*/(DWORD)0xC00002C3L: return "Mutual Authentication failed. The server's password is out of date at the domain controller.";
		case /*STATUS_CORRUPT_SYSTEM_FILE*/(DWORD)0xC00002C4L: return "The system file %1 has become corrupt and has been replaced.";
		case /*STATUS_DATATYPE_MISALIGNMENT_ERROR*/(DWORD)0xC00002C5L: return "{DATATYPE_MISALIGNMENT_ERROR} Alignment Error A datatype misalignment error was detected in a load or store instruction.";
		case /*STATUS_WMI_READ_ONLY*/(DWORD)0xC00002C6L: return "The WMI data item or data block is read only.";
		case /*STATUS_WMI_SET_FAILURE*/(DWORD)0xC00002C7L: return "The WMI data item or data block could not be changed.";
		case /*STATUS_COMMITMENT_MINIMUM*/(DWORD)0xC00002C8L: return "{Virtual Memory Minimum Too Low} Your system is low on virtual memory. Windows is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help.";
		case /*STATUS_REG_NAT_CONSUMPTION*/(DWORD)0xC00002C9L: return "{REG_NAT_CONSUMPTION} Register NaT consumption faults. A NaT value is consumed on a non speculative instruction.";
		case /*STATUS_TRANSPORT_FULL*/(DWORD)0xC00002CAL: return "The medium changer's transport element contains media, which is causing the operation to fail.";
		case /*STATUS_DS_SAM_INIT_FAILURE*/(DWORD)0xC00002CBL: return "Security Accounts Manager initialization failed because of the following error: %hs Error Status: Please click OK to shutdown this system and reboot into Directory Services Restore Mode, check the event log for more detailed information.";
		case /*STATUS_ONLY_IF_CONNECTED*/(DWORD)0xC00002CCL: return "This operation is supported only when you are connected to the server.";
		case /*STATUS_DS_SENSITIVE_GROUP_VIOLATION*/(DWORD)0xC00002CDL: return "Only an administrator can modify the membership list of an administrative group.";
		case /*STATUS_PNP_RESTART_ENUMERATION*/(DWORD)0xC00002CEL: return "A device was removed so enumeration must be restarted.";
		case /*STATUS_JOURNAL_ENTRY_DELETED*/(DWORD)0xC00002CFL: return "The journal entry has been deleted from the journal.";
		case /*STATUS_DS_CANT_MOD_PRIMARYGROUPID*/(DWORD)0xC00002D0L: return "Cannot change the primary group ID of a domain controller account.";
		case /*STATUS_SYSTEM_IMAGE_BAD_SIGNATURE*/(DWORD)0xC00002D1L: return "{Fatal System Error} The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down.";
		case /*STATUS_PNP_REBOOT_REQUIRED*/(DWORD)0xC00002D2L: return "Device will not start without a reboot.";
		case /*STATUS_POWER_STATE_INVALID*/(DWORD)0xC00002D3L: return "Current device power state cannot support this request.";
		case /*STATUS_DS_INVALID_GROUP_TYPE*/(DWORD)0xC00002D4L: return "The specified group type is invalid.";
		case /*STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN*/(DWORD)0xC00002D5L: return "In mixed domain no nesting of global group if group is security enabled.";
		case /*STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN*/(DWORD)0xC00002D6L: return "In mixed domain, cannot nest local groups with other local groups, if the group is security enabled.";
		case /*STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER*/(DWORD)0xC00002D7L: return "A global group cannot have a local group as a member.";
		case /*STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER*/(DWORD)0xC00002D8L: return "A global group cannot have a universal group as a member.";
		case /*STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER*/(DWORD)0xC00002D9L: return "A universal group cannot have a local group as a member.";
		case /*STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER*/(DWORD)0xC00002DAL: return "A global group cannot have a cross domain member.";
		case /*STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER*/(DWORD)0xC00002DBL: return "A local group cannot have another cross domain local group as a member.";
		case /*STATUS_DS_HAVE_PRIMARY_MEMBERS*/(DWORD)0xC00002DCL: return "Cannot change to security disabled group because of having primary members in this group.";
		case /*STATUS_WMI_NOT_SUPPORTED*/(DWORD)0xC00002DDL: return "The WMI operation is not supported by the data block or method.";
		case /*STATUS_INSUFFICIENT_POWER*/(DWORD)0xC00002DEL: return "There is not enough power to complete the requested operation.";
		case /*STATUS_SAM_NEED_BOOTKEY_PASSWORD*/(DWORD)0xC00002DFL: return "Security Account Manager needs to get the boot password.";
		case /*STATUS_SAM_NEED_BOOTKEY_FLOPPY*/(DWORD)0xC00002E0L: return "Security Account Manager needs to get the boot key from floppy disk.";
		case /*STATUS_DS_CANT_START*/(DWORD)0xC00002E1L: return "Directory Service cannot start.";
		case /*STATUS_DS_INIT_FAILURE*/(DWORD)0xC00002E2L: return "Directory Services could not start because of the following error: %hs Error Status: Please click OK to shutdown this system and reboot into Directory Services Restore Mode, check the event log for more detailed information.";
		case /*STATUS_SAM_INIT_FAILURE*/(DWORD)0xC00002E3L: return "Security Accounts Manager initialization failed because of the following error: %hs Error Status: Please click OK to shutdown this system and reboot into Safe Mode, check the event log for more detailed information.";
		case /*STATUS_DS_GC_REQUIRED*/(DWORD)0xC00002E4L: return "The requested operation can be performed only on a global catalog server.";
		case /*STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY*/(DWORD)0xC00002E5L: return "A local group can only be a member of other local groups in the same domain.";
		case /*STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS*/(DWORD)0xC00002E6L: return "Foreign security principals cannot be members of universal groups.";
		case /*STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED*/(DWORD)0xC00002E7L: return "Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.";
		case /*STATUS_MULTIPLE_FAULT_VIOLATION*/(DWORD)0xC00002E8L: return "MULTIPLE_FAULT_VIOLATION";
		case /*STATUS_CURRENT_DOMAIN_NOT_ALLOWED*/(DWORD)0xC00002E9L: return "This operation cannot be performed on the current domain.";
		case /*STATUS_CANNOT_MAKE*/(DWORD)0xC00002EAL: return "The directory or file cannot be created.";
		case /*STATUS_SYSTEM_SHUTDOWN*/(DWORD)0xC00002EBL: return "The system is in the process of shutting down.";
		case /*STATUS_DS_INIT_FAILURE_CONSOLE*/(DWORD)0xC00002ECL: return "Directory Services could not start because of the following error: %hs Error Status: Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.";
		case /*STATUS_DS_SAM_INIT_FAILURE_CONSOLE*/(DWORD)0xC00002EDL: return "Security Accounts Manager initialization failed because of the following error: %hs Error Status: Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.";
		case /*STATUS_UNFINISHED_CONTEXT_DELETED*/(DWORD)0xC00002EEL: return "A security context was deleted before the context was completed.  This is considered a logon failure.";
		case /*STATUS_NO_TGT_REPLY*/(DWORD)0xC00002EFL: return "The client is trying to negotiate a context and the server requires user-to-user but didn't send a TGT reply.";
		case /*STATUS_OBJECTID_NOT_FOUND*/(DWORD)0xC00002F0L: return "An object ID was not found in the file.";
		case /*STATUS_NO_IP_ADDRESSES*/(DWORD)0xC00002F1L: return "Unable to accomplish the requested task because the local machine does not have any IP addresses.";
		case /*STATUS_WRONG_CREDENTIAL_HANDLE*/(DWORD)0xC00002F2L: return "The supplied credential handle does not match the credential associated with the security context.";
		case /*STATUS_CRYPTO_SYSTEM_INVALID*/(DWORD)0xC00002F3L: return "The crypto system or checksum function is invalid because a required function is unavailable.";
		case /*STATUS_MAX_REFERRALS_EXCEEDED*/(DWORD)0xC00002F4L: return "The number of maximum ticket referrals has been exceeded.";
		case /*STATUS_MUST_BE_KDC*/(DWORD)0xC00002F5L: return "The local machine must be a Kerberos KDC (domain controller) and it is not.";
		case /*STATUS_STRONG_CRYPTO_NOT_SUPPORTED*/(DWORD)0xC00002F6L: return "The other end of the security negotiation is requires strong crypto but it is not supported on the local machine.";
		case /*STATUS_TOO_MANY_PRINCIPALS*/(DWORD)0xC00002F7L: return "The KDC reply contained more than one principal name.";
		case /*STATUS_NO_PA_DATA*/(DWORD)0xC00002F8L: return "Expected to find PA data for a hint of what etype to use, but it was not found.";
		case /*STATUS_PKINIT_NAME_MISMATCH*/(DWORD)0xC00002F9L: return "The client certificate does not contain a valid UPN, or does not match the client name in the logon request.  Please contact your administrator.";
		case /*STATUS_SMARTCARD_LOGON_REQUIRED*/(DWORD)0xC00002FAL: return "Smartcard logon is required and was not used.";
		case /*STATUS_KDC_INVALID_REQUEST*/(DWORD)0xC00002FBL: return "An invalid request was sent to the KDC.";
		case /*STATUS_KDC_UNABLE_TO_REFER*/(DWORD)0xC00002FCL: return "The KDC was unable to generate a referral for the service requested.";
		case /*STATUS_KDC_UNKNOWN_ETYPE*/(DWORD)0xC00002FDL: return "The encryption type requested is not supported by the KDC.";
		case /*STATUS_SHUTDOWN_IN_PROGRESS*/(DWORD)0xC00002FEL: return "A system shutdown is in progress.";
		case /*STATUS_SERVER_SHUTDOWN_IN_PROGRESS*/(DWORD)0xC00002FFL: return "The server machine is shutting down.";
		case /*STATUS_NOT_SUPPORTED_ON_SBS*/(DWORD)0xC0000300L: return "This operation is not supported on a computer running Windows Server 2003 for Small Business Server";
		case /*STATUS_WMI_GUID_DISCONNECTED*/(DWORD)0xC0000301L: return "The WMI GUID is no longer available";
		case /*STATUS_WMI_ALREADY_DISABLED*/(DWORD)0xC0000302L: return "Collection or events for the WMI GUID is already disabled.";
		case /*STATUS_WMI_ALREADY_ENABLED*/(DWORD)0xC0000303L: return "Collection or events for the WMI GUID is already enabled.";
		case /*STATUS_MFT_TOO_FRAGMENTED*/(DWORD)0xC0000304L: return "The Master File Table on the volume is too fragmented to complete this operation.";
		case /*STATUS_COPY_PROTECTION_FAILURE*/(DWORD)0xC0000305L: return "Copy protection failure.";
		case /*STATUS_CSS_AUTHENTICATION_FAILURE*/(DWORD)0xC0000306L: return "Copy protection error - DVD CSS Authentication failed.";
		case /*STATUS_CSS_KEY_NOT_PRESENT*/(DWORD)0xC0000307L: return "Copy protection error - The given sector does not contain a valid key.";
		case /*STATUS_CSS_KEY_NOT_ESTABLISHED*/(DWORD)0xC0000308L: return "Copy protection error - DVD session key not established.";
		case /*STATUS_CSS_SCRAMBLED_SECTOR*/(DWORD)0xC0000309L: return "Copy protection error - The read failed because the sector is encrypted.";
		case /*STATUS_CSS_REGION_MISMATCH*/(DWORD)0xC000030AL: return "Copy protection error - The given DVD's region does not correspond to the region setting of the drive.";
		case /*STATUS_CSS_RESETS_EXHAUSTED*/(DWORD)0xC000030BL: return "Copy protection error - The drive's region setting may be permanent.";
		case /*STATUS_PKINIT_FAILURE*/(DWORD)0xC0000320L: return "The kerberos protocol encountered an error while validating the KDC certificate during smartcard Logon.  There is more information in the system event log.";
		case /*STATUS_SMARTCARD_SUBSYSTEM_FAILURE*/(DWORD)0xC0000321L: return "The kerberos protocol encountered an error while attempting to utilize the smartcard subsystem.";
		case /*STATUS_NO_KERB_KEY*/(DWORD)0xC0000322L: return "The target server does not have acceptable kerberos credentials.";
		case /*STATUS_HOST_DOWN*/(DWORD)0xC0000350L: return "The transport determined that the remote system is down.";
		case /*STATUS_UNSUPPORTED_PREAUTH*/(DWORD)0xC0000351L: return "An unsupported preauthentication mechanism was presented to the kerberos package.";
		case /*STATUS_EFS_ALG_BLOB_TOO_BIG*/(DWORD)0xC0000352L: return "The encryption algorithm used on the source file needs a bigger key buffer than the one used on the destination file.";
		case /*STATUS_PORT_NOT_SET*/(DWORD)0xC0000353L: return "An attempt to remove a processes DebugPort was made, but a port was not already associated with the process.";
		case /*STATUS_DEBUGGER_INACTIVE*/(DWORD)0xC0000354L: return "An attempt to do an operation on a debug port failed because the port is in the process of being deleted.";
		case /*STATUS_DS_VERSION_CHECK_FAILURE*/(DWORD)0xC0000355L: return "This version of Windows is not compatible with the behavior version of directory forest, domain or domain controller.";
		case /*STATUS_AUDITING_DISABLED*/(DWORD)0xC0000356L: return "The specified event is currently not being audited.";
		case /*STATUS_PRENT4_MACHINE_ACCOUNT*/(DWORD)0xC0000357L: return "The machine account was created pre-NT4.  The account needs to be recreated.";
		case /*STATUS_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER*/(DWORD)0xC0000358L: return "A account group cannot have a universal group as a member.";
		case /*STATUS_INVALID_IMAGE_WIN_32*/(DWORD)0xC0000359L: return "The specified image file did not have the correct format, it appears to be a 32-bit Windows image.";
		case /*STATUS_INVALID_IMAGE_WIN_64*/(DWORD)0xC000035AL: return "The specified image file did not have the correct format, it appears to be a 64-bit Windows image.";
		case /*STATUS_BAD_BINDINGS*/(DWORD)0xC000035BL: return "Client's supplied SSPI channel bindings were incorrect.";
		case /*STATUS_NETWORK_SESSION_EXPIRED*/(DWORD)0xC000035CL: return "The client's session has expired, so the client must reauthenticate to continue accessing the remote resources.";
		case /*STATUS_APPHELP_BLOCK*/(DWORD)0xC000035DL: return "AppHelp dialog canceled thus preventing the application from starting.";
		case /*STATUS_ALL_SIDS_FILTERED*/(DWORD)0xC000035EL: return "The SID filtering operation removed all SIDs.";
		case /*STATUS_NOT_SAFE_MODE_DRIVER*/(DWORD)0xC000035FL: return "The driver was not loaded because the system is booting into safe mode.";
		case /*STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT*/(DWORD)0xC0000361L: return "Access to %1 has been restricted by your Administrator by the default software restriction policy level.";
		case /*STATUS_ACCESS_DISABLED_BY_POLICY_PATH*/(DWORD)0xC0000362L: return "Access to %1 has been restricted by your Administrator by location with policy rule %2 placed on path %3";
		case /*STATUS_ACCESS_DISABLED_BY_POLICY_PUBLISHER*/(DWORD)0xC0000363L: return "Access to %1 has been restricted by your Administrator by software publisher policy.";
		case /*STATUS_ACCESS_DISABLED_BY_POLICY_OTHER*/(DWORD)0xC0000364L: return "Access to %1 has been restricted by your Administrator by policy rule %2.";
		case /*STATUS_FAILED_DRIVER_ENTRY*/(DWORD)0xC0000365L: return "The driver was not loaded because it failed it's initialization call.";
		case /*STATUS_DEVICE_ENUMERATION_ERROR*/(DWORD)0xC0000366L: return "The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection.";
		case /*STATUS_WAIT_FOR_OPLOCK*/(DWORD)0x00000367L: return "An operation is blocked waiting for an oplock.";
		case /*STATUS_MOUNT_POINT_NOT_RESOLVED*/(DWORD)0xC0000368L: return "The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached.";
		case /*STATUS_INVALID_DEVICE_OBJECT_PARAMETER*/(DWORD)0xC0000369L: return "The device object parameter is either not a valid device object or is not attached to the volume specified by the file name.";
		case /*STATUS_MCA_OCCURED*/(DWORD)0xC000036AL: return "A Machine Check Error has occurred. Please check the system eventlog for additional information.";
		case /*STATUS_DRIVER_BLOCKED_CRITICAL*/(DWORD)0xC000036BL: return "Driver %2 has been blocked from loading.";
		case /*STATUS_DRIVER_BLOCKED*/(DWORD)0xC000036CL: return "Driver %2 has been blocked from loading.";
		case /*STATUS_DRIVER_DATABASE_ERROR*/(DWORD)0xC000036DL: return "There was error [%2] processing the driver database.";
		case /*STATUS_SYSTEM_HIVE_TOO_LARGE*/(DWORD)0xC000036EL: return "System hive size has exceeded its limit.";
		case /*STATUS_INVALID_IMPORT_OF_NON_DLL*/(DWORD)0xC000036FL: return "A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image.";
		case /*STATUS_DS_SHUTTING_DOWN*/(DWORD)0x40000370L: return "The Directory Service is shutting down.";
		case /*STATUS_NO_SECRETS*/(DWORD)0xC0000371L: return "The local account store does not contain secret material for the specified account.";
		case /*STATUS_ACCESS_DISABLED_NO_SAFER_UI_BY_POLICY*/(DWORD)0xC0000372L: return "Access to %1 has been restricted by your Administrator by policy rule %2.";
		case /*STATUS_FAILED_STACK_SWITCH*/(DWORD)0xC0000373L: return "The system was not able to allocate enough memory to perform a stack switch.";
		case /*STATUS_HEAP_CORRUPTION*/(DWORD)0xC0000374L: return "A heap has been corrupted.";
		case /*STATUS_SMARTCARD_WRONG_PIN*/(DWORD)0xC0000380L: return "An incorrect PIN was presented to the smart card";
		case /*STATUS_SMARTCARD_CARD_BLOCKED*/(DWORD)0xC0000381L: return "The smart card is blocked";
		case /*STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED*/(DWORD)0xC0000382L: return "No PIN was presented to the smart card";
		case /*STATUS_SMARTCARD_NO_CARD*/(DWORD)0xC0000383L: return "No smart card available";
		case /*STATUS_SMARTCARD_NO_KEY_CONTAINER*/(DWORD)0xC0000384L: return "The requested key container does not exist on the smart card";
		case /*STATUS_SMARTCARD_NO_CERTIFICATE*/(DWORD)0xC0000385L: return "The requested certificate does not exist on the smart card";
		case /*STATUS_SMARTCARD_NO_KEYSET*/(DWORD)0xC0000386L: return "The requested keyset does not exist";
		case /*STATUS_SMARTCARD_IO_ERROR*/(DWORD)0xC0000387L: return "A communication error with the smart card has been detected.";
		case /*STATUS_DOWNGRADE_DETECTED*/(DWORD)0xC0000388L: return "The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you.";
		case /*STATUS_SMARTCARD_CERT_REVOKED*/(DWORD)0xC0000389L: return "The smartcard certificate used for authentication has been revoked. Please contact your system administrator.  There may be additional information in the event log.";
		case /*STATUS_ISSUING_CA_UNTRUSTED*/(DWORD)0xC000038AL: return "An untrusted certificate authority was detected While processing the smartcard certificate used for authentication.  Please contact your system administrator.";
		case /*STATUS_REVOCATION_OFFLINE_C*/(DWORD)0xC000038BL: return "The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator.";
		case /*STATUS_PKINIT_CLIENT_FAILURE*/(DWORD)0xC000038CL: return "The smartcard certificate used for authentication was not trusted.  Please contact your system administrator.";
		case /*STATUS_SMARTCARD_CERT_EXPIRED*/(DWORD)0xC000038DL: return "The smartcard certificate used for authentication has expired.  Please contact your system administrator.";
		case /*STATUS_DRIVER_FAILED_PRIOR_UNLOAD*/(DWORD)0xC000038EL: return "The driver could not be loaded because a previous version of the driver is still in memory.";
		case /*STATUS_SMARTCARD_SILENT_CONTEXT*/(DWORD)0xC000038FL: return "The smartcard provider could not perform the action since the context was acquired as silent.";
		case /*STATUS_PER_USER_TRUST_QUOTA_EXCEEDED*/(DWORD)0xC0000401L: return "The current user's delegated trust creation quota has been exceeded.";
		case /*STATUS_ALL_USER_TRUST_QUOTA_EXCEEDED*/(DWORD)0xC0000402L: return "The total delegated trust creation quota has been exceeded.";
		case /*STATUS_USER_DELETE_TRUST_QUOTA_EXCEEDED*/(DWORD)0xC0000403L: return "The current user's delegated trust deletion quota has been exceeded.";
		case /*STATUS_DS_NAME_NOT_UNIQUE*/(DWORD)0xC0000404L: return "The requested name already exists as a unique identifier.";
		case /*STATUS_DS_DUPLICATE_ID_FOUND*/(DWORD)0xC0000405L: return "The requested object has a non-unique identifier and cannot be retrieved.";
		case /*STATUS_DS_GROUP_CONVERSION_ERROR*/(DWORD)0xC0000406L: return "The group cannot be converted due to attribute restrictions on the requested group type.";
		case /*STATUS_VOLSNAP_PREPARE_HIBERNATE*/(DWORD)0xC0000407L: return "{Volume Shadow Copy Service} Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation.";
		case /*STATUS_USER2USER_REQUIRED*/(DWORD)0xC0000408L: return "Kerberos sub-protocol User2User is required.";
		case /*STATUS_STACK_BUFFER_OVERRUN*/(DWORD)0xC0000409L: return "The system detected an overrun of a stack-based buffer in this application.  This overrun could potentially allow a malicious user to gain control of this application.";
		case /*STATUS_NO_S4U_PROT_SUPPORT*/(DWORD)0xC000040AL: return "The Kerberos subsystem encountered an error.  A service for user protocol request was made against a domain controller which does not support service for user.";
		case /*STATUS_CROSSREALM_DELEGATION_FAILURE*/(DWORD)0xC000040BL: return "An attempt was made by this server to make a Kerberos constrained delegation request for a target outside of the server's realm.  This is not supported, and indicates a misconfiguration on this server's allowed to delegate to list.  Please contact your administrator.";
		case /*STATUS_REVOCATION_OFFLINE_KDC*/(DWORD)0xC000040CL: return "The revocation status of the domain controller certificate used for smartcard authentication could not be determined.  There is additional information in the system event log. Please contact your system administrator.";
		case /*STATUS_ISSUING_CA_UNTRUSTED_KDC*/(DWORD)0xC000040DL: return "An untrusted certificate authority was detected while processing the domain controller certificate used for authentication.  There is additional information in the system event log.  Please contact your system administrator.";
		case /*STATUS_KDC_CERT_EXPIRED*/(DWORD)0xC000040EL: return "The domain controller certificate used for smartcard logon has expired. Please contact your system administrator with the contents of your system event log.";
		case /*STATUS_KDC_CERT_REVOKED*/(DWORD)0xC000040FL: return "The domain controller certificate used for smartcard logon has been revoked. Please contact your system administrator with the contents of your system event log.";
		case /*STATUS_PARAMETER_QUOTA_EXCEEDED*/(DWORD)0xC0000410L: return "Data present in one of the parameters is more than the function can operate on.";
		case /*STATUS_HIBERNATION_FAILURE*/(DWORD)0xC0000411L: return "The system has failed to hibernate (The error code is %hs).  Hibernation will be disabled until the system is restarted.";
		case /*STATUS_DELAY_LOAD_FAILED*/(DWORD)0xC0000412L: return "An attempt to delay-load a .dll or get a function address in a delay-loaded .dll failed.";
		case /*STATUS_AUTHENTICATION_FIREWALL_FAILED*/(DWORD)0xC0000413L: return "Logon Failure:";
		case /*STATUS_VDM_DISALLOWED*/(DWORD)0xC0000414L: return "%hs is a 16-bit application. You do not have permissions to execute 16-bit applications. Check your permissions with your system administrator.";
		case /*STATUS_HUNG_DISPLAY_DRIVER_THREAD*/(DWORD)0xC0000415L: return "{Display Driver Stopped Responding} The %hs display driver has stopped working normally.  Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft.";
		case /*STATUS_INSUFFICIENT_RESOURCE_FOR_SPECIFIED_SHARED_SECTION_SIZE*/(DWORD)0xC0000416L: return "The Desktop heap encountered an error while allocating session memory.  There is more information in the system event log.";
		case /*STATUS_INVALID_CRUNTIME_PARAMETER*/(DWORD)0xC0000417L: return "An invalid parameter was passed to a C runtime function.";
		case /*STATUS_NTLM_BLOCKED*/(DWORD)0xC0000418L: return "The authentication failed since NTLM was blocked.";
		case /*STATUS_ASSERTION_FAILURE*/(DWORD)0xC0000420L: return "An assertion failure has occurred.";
		case /*STATUS_VERIFIER_STOP*/(DWORD)0xC0000421L: return "Application verifier has found an error in the current process.";
		case /*STATUS_CALLBACK_POP_STACK*/(DWORD)0xC0000423L: return "An exception has occurred in a user mode callback and the kernel callback frame should be removed.";
		case /*STATUS_INCOMPATIBLE_DRIVER_BLOCKED*/(DWORD)0xC0000424L: return "%2 has been blocked from loading due to incompatibility with this system. Please contact your software vendor for a compatible version of the driver.";
		case /*STATUS_HIVE_UNLOADED*/(DWORD)0xC0000425L: return "Illegal operation attempted on a registry key which has already been unloaded.";
		case /*STATUS_COMPRESSION_DISABLED*/(DWORD)0xC0000426L: return "Compression is disabled for this volume.";
		case /*STATUS_FILE_SYSTEM_LIMITATION*/(DWORD)0xC0000427L: return "The requested operation could not be completed due to a file system limitation";
		case /*STATUS_INVALID_IMAGE_HASH*/(DWORD)0xC0000428L: return "Windows cannot verify the digital signature for this file. A recent hardware or software change might have installed a file that is signed incorrectly or damaged, or that might be malicious software from an unknown source.";
		case /*STATUS_NOT_CAPABLE*/(DWORD)0xC0000429L: return "The implementation is not capable of performing the request.";
		case /*STATUS_REQUEST_OUT_OF_SEQUENCE*/(DWORD)0xC000042AL: return "The requested operation is out of order with respect to other operations.";
		case /*STATUS_IMPLEMENTATION_LIMIT*/(DWORD)0xC000042BL: return "An operation attempted to exceed an implementation-defined limit.";
		case /*STATUS_ELEVATION_REQUIRED*/(DWORD)0xC000042CL: return "The requested operation requires elevation.";
		case /*STATUS_BEYOND_VDL*/(DWORD)0xC0000432L: return "The operation was attempted beyond the valid data length of the file.";
		case /*STATUS_ENCOUNTERED_WRITE_IN_PROGRESS*/(DWORD)0xC0000433L: return "The attempted write operation encountered a write already in progress for some portion of the range.";
		case /*STATUS_PTE_CHANGED*/(DWORD)0xC0000434L: return "The page fault mappings changed in the middle of processing a fault so the operation must be retried.";
		case /*STATUS_PURGE_FAILED*/(DWORD)0xC0000435L: return "The attempt to purge this file from memory failed to purge some or all the data from memory.";
		case /*STATUS_CRED_REQUIRES_CONFIRMATION*/(DWORD)0xC0000440L: return "The requested credential requires confirmation.";
		case /*STATUS_CS_ENCRYPTION_INVALID_SERVER_RESPONSE*/(DWORD)0xC0000441L: return "The remote server sent an invalid response for a file being opened with Client Side Encryption.";
		case /*STATUS_CS_ENCRYPTION_UNSUPPORTED_SERVER*/(DWORD)0xC0000442L: return "Client Side Encryption is not supported by the remote server even though it claims to support it.";
		case /*STATUS_CS_ENCRYPTION_EXISTING_ENCRYPTED_FILE*/(DWORD)0xC0000443L: return "File is encrypted and should be opened in Client Side Encryption mode.";
		case /*STATUS_CS_ENCRYPTION_NEW_ENCRYPTED_FILE*/(DWORD)0xC0000444L: return "A new encrypted file is being created and a $EFS needs to be provided.";
		case /*STATUS_CS_ENCRYPTION_FILE_NOT_CSE*/(DWORD)0xC0000445L: return "The SMB client requested a CSE FSCTL on a non-CSE file.";
		case /*STATUS_INVALID_LABEL*/(DWORD)0xC0000446L: return "Indicates a particular Security ID may not be assigned as the label of an object.";
		case /*STATUS_DRIVER_PROCESS_TERMINATED*/(DWORD)0xC0000450L: return "The process hosting the driver for this device has terminated.";
		case /*STATUS_AMBIGUOUS_SYSTEM_DEVICE*/(DWORD)0xC0000451L: return "The requested system device cannot be identified due to multiple indistinguishable devices potentially matching the identification criteria.";
		case /*STATUS_SYSTEM_DEVICE_NOT_FOUND*/(DWORD)0xC0000452L: return "The requested system device cannot be found.";
		case /*STATUS_RESTART_BOOT_APPLICATION*/(DWORD)0xC0000453L: return "This boot application must be restarted.";
		case /*STATUS_INVALID_TASK_NAME*/(DWORD)0xC0000500L: return "The specified task name is invalid.";
		case /*STATUS_INVALID_TASK_INDEX*/(DWORD)0xC0000501L: return "The specified task index is invalid.";
		case /*STATUS_THREAD_ALREADY_IN_TASK*/(DWORD)0xC0000502L: return "The specified thread is already joining a task.";
		case /*STATUS_CALLBACK_BYPASS*/(DWORD)0xC0000503L: return "A callback has requested to bypass native code.";
		case /*STATUS_PORT_CLOSED*/(DWORD)0xC0000700L: return "The ALPC port is closed.";
		case /*STATUS_MESSAGE_LOST*/(DWORD)0xC0000701L: return "The ALPC message requested is no longer available.";
		case /*STATUS_INVALID_MESSAGE*/(DWORD)0xC0000702L: return "The ALPC message supplied is invalid.";
		case /*STATUS_REQUEST_CANCELED*/(DWORD)0xC0000703L: return "The ALPC message has been canceled.";
		case /*STATUS_RECURSIVE_DISPATCH*/(DWORD)0xC0000704L: return "Invalid recursive dispatch attempt.";
		case /*STATUS_LPC_RECEIVE_BUFFER_EXPECTED*/(DWORD)0xC0000705L: return "No receive buffer has been supplied in a synchrounus request.";
		case /*STATUS_LPC_INVALID_CONNECTION_USAGE*/(DWORD)0xC0000706L: return "The connection port is used in an invalid context.";
		case /*STATUS_LPC_REQUESTS_NOT_ALLOWED*/(DWORD)0xC0000707L: return "The ALPC port does not accept new request messages.";
		case /*STATUS_RESOURCE_IN_USE*/(DWORD)0xC0000708L: return "The resource requested is already in use.";
		case /*STATUS_HARDWARE_MEMORY_ERROR*/(DWORD)0xC0000709L: return "The hardware has reported an uncorrectable memory error.";
		case /*STATUS_THREADPOOL_HANDLE_EXCEPTION*/(DWORD)0xC000070AL: return "Status 0x%08x was returned, waiting on handle 0x%x for wait 0x%p, in waiter 0x%p.";
		case /*STATUS_THREADPOOL_SET_EVENT_ON_COMPLETION_FAILED*/(DWORD)0xC000070BL: return "After a callback to 0x%p(0x%p), a completion call to SetEvent(0x%p) failed with status 0x%08x.";
		case /*STATUS_THREADPOOL_RELEASE_SEMAPHORE_ON_COMPLETION_FAILED*/(DWORD)0xC000070CL: return "After a callback to 0x%p(0x%p), a completion call to ReleaseSemaphore(0x%p, %d) failed with status 0x%08x.";
		case /*STATUS_THREADPOOL_RELEASE_MUTEX_ON_COMPLETION_FAILED*/(DWORD)0xC000070DL: return "After a callback to 0x%p(0x%p), a completion call to ReleaseMutex(%p) failed with status 0x%08x.";
		case /*STATUS_THREADPOOL_FREE_LIBRARY_ON_COMPLETION_FAILED*/(DWORD)0xC000070EL: return "After a callback to 0x%p(0x%p), an completion call to FreeLibrary(%p) failed with status 0x%08x.";
		case /*STATUS_THREADPOOL_RELEASED_DURING_OPERATION*/(DWORD)0xC000070FL: return "The threadpool 0x%p was released while a thread was posting a callback to 0x%p(0x%p) to it.";
		case /*STATUS_CALLBACK_RETURNED_WHILE_IMPERSONATING*/(DWORD)0xC0000710L: return "A threadpool worker thread is impersonating a client, after a callback to 0x%p(0x%p). This is unexpected, indicating that the callback is missing a call to revert the impersonation.";
		case /*STATUS_APC_RETURNED_WHILE_IMPERSONATING*/(DWORD)0xC0000711L: return "A threadpool worker thread is impersonating a client, after executing an APC. This is unexpected, indicating that the APC is missing a call to revert the impersonation.";
		case /*STATUS_PROCESS_IS_PROTECTED*/(DWORD)0xC0000712L: return "Either the target process, or the target thread's containing process, is a protected process.";
		case /*STATUS_MCA_EXCEPTION*/(DWORD)0xC0000713L: return "A Thread is getting dispatched with MCA EXCEPTION because of MCA.";
		case /*STATUS_CERTIFICATE_MAPPING_NOT_UNIQUE*/(DWORD)0xC0000714L: return "The client certificate account mapping is not unique.";
		case /*STATUS_SYMLINK_CLASS_DISABLED*/(DWORD)0xC0000715L: return "The symbolic link cannot be followed because its type is disabled.";
		case /*STATUS_INVALID_IDN_NORMALIZATION*/(DWORD)0xC0000716L: return "Indicates that the specified string is not valid for IDN normalization.";
		case /*STATUS_NO_UNICODE_TRANSLATION*/(DWORD)0xC0000717L: return "No mapping for the Unicode character exists in the target multi-byte code page.";
		case /*STATUS_ALREADY_REGISTERED*/(DWORD)0xC0000718L: return "The provided callback is already registered.";
		case /*STATUS_CONTEXT_MISMATCH*/(DWORD)0xC0000719L: return "The provided context did not match the target.";
		case /*STATUS_PORT_ALREADY_HAS_COMPLETION_LIST*/(DWORD)0xC000071AL: return "The specified port already has a completion list.";
		case /*STATUS_CALLBACK_RETURNED_THREAD_PRIORITY*/(DWORD)0xC000071BL: return "A threadpool worker thread enter a callback at thread base priority 0x%x and exited at priority 0x%x. This is unexpected, indicating that the callback missed restoring the priority.";
		case /*STATUS_INVALID_THREAD*/(DWORD)0xC000071CL: return "An invalid thread, handle %p, is specified for this operation.  Possibly, a threadpool worker thread was specified.";
		case /*STATUS_CALLBACK_RETURNED_TRANSACTION*/(DWORD)0xC000071DL: return "A threadpool worker thread enter a callback, which left transaction state. This is unexpected, indicating that the callback missed clearing the transaction.";
		case /*STATUS_CALLBACK_RETURNED_LDR_LOCK*/(DWORD)0xC000071EL: return "A threadpool worker thread enter a callback, which left the loader lock held. This is unexpected, indicating that the callback missed releasing the lock.";
		case /*STATUS_CALLBACK_RETURNED_LANG*/(DWORD)0xC000071FL: return "A threadpool worker thread enter a callback, which left with preferred languages set. This is unexpected, indicating that the callback missed clearing them.";
		case /*STATUS_CALLBACK_RETURNED_PRI_BACK*/(DWORD)0xC0000720L: return "A threadpool worker thread enter a callback, which left with background priorities set. This is unexpected, indicating that the callback missed restoring the original priorities.";
		case /*STATUS_CALLBACK_RETURNED_THREAD_AFFINITY*/(DWORD)0xC0000721L: return "A threadpool worker thread enter a callback at thread affinity %p and exited at affinity %p. This is unexpected, indicating that the callback missed restoring the priority.";
		case /*STATUS_DISK_REPAIR_DISABLED*/(DWORD)0xC0000800L: return "The attempted operation required self healing to be enabled.";
		case /*STATUS_DS_DOMAIN_RENAME_IN_PROGRESS*/(DWORD)0xC0000801L: return "The Directory Service cannot perform the requested operation because a domain rename operation is in progress.";
		case /*STATUS_DISK_QUOTA_EXCEEDED*/(DWORD)0xC0000802L: return "The requested file operation failed because the storage quota was exceeded. To free up disk space, move files to a different location or delete unnecessary files. For more information, contact your system administrator.";
		case /*STATUS_DATA_LOST_REPAIR*/(DWORD)0x80000803L: return "Windows discovered a corruption in the file %hs. This file has now been repaired. Please check if any data in the file was lost because of the corruption.";
		case /*STATUS_CONTENT_BLOCKED*/(DWORD)0xC0000804L: return "The requested file operation failed because the storage policy blocks that type of file. For more information, contact your system administrator.";
		case /*STATUS_BAD_CLUSTERS*/(DWORD)0xC0000805L: return "The operation could not be completed due to bad clusters on disk.";
		case /*STATUS_VOLUME_DIRTY*/(DWORD)0xC0000806L: return "The operation could not be completed because the volume is dirty.  Please run chkdsk and try again.";
		case /*STATUS_FILE_CHECKED_OUT*/(DWORD)0xC0000901L: return "This file is checked out or locked for editing by another user.";
		case /*STATUS_CHECKOUT_REQUIRED*/(DWORD)0xC0000902L: return "The file must be checked out before saving changes.";
		case /*STATUS_BAD_FILE_TYPE*/(DWORD)0xC0000903L: return "The file type being saved or retrieved has been blocked.";
		case /*STATUS_FILE_TOO_LARGE*/(DWORD)0xC0000904L: return "The file size exceeds the limit allowed and cannot be saved.";
		case /*STATUS_FORMS_AUTH_REQUIRED*/(DWORD)0xC0000905L: return "Access Denied.  Before opening files in this location, you must first browse to the web site and select the option to login automatically.";
		case /*STATUS_VIRUS_INFECTED*/(DWORD)0xC0000906L: return "Operation did not complete successfully because the file contains a virus.";
		case /*STATUS_VIRUS_DELETED*/(DWORD)0xC0000907L: return "This file contains a virus and cannot be opened. Due to the nature of this virus, the file has been removed from this location.";
		case /*STATUS_BAD_MCFG_TABLE*/(DWORD)0xC0000908L: return "The resources required for this device conflict with the MCFG table.";
		case /*STATUS_WOW_ASSERTION*/(DWORD)0xC0009898L: return "WOW Assertion Error.";
		case /*STATUS_INVALID_SIGNATURE*/(DWORD)0xC000A000L: return "The cryptographic signature is invalid.";
		case /*STATUS_HMAC_NOT_SUPPORTED*/(DWORD)0xC000A001L: return "The cryptographic provider does not support HMAC.";
		case /*STATUS_IPSEC_QUEUE_OVERFLOW*/(DWORD)0xC000A010L: return "The IPSEC queue overflowed.";
		case /*STATUS_ND_QUEUE_OVERFLOW*/(DWORD)0xC000A011L: return "The neighbor discovery queue overflowed.";
		case /*STATUS_HOPLIMIT_EXCEEDED*/(DWORD)0xC000A012L: return "An ICMP hop limit exceeded error was received.";
		case /*STATUS_PROTOCOL_NOT_SUPPORTED*/(DWORD)0xC000A013L: return "The protocol is not installed on the local machine.";
		case /*STATUS_LOST_WRITEBEHIND_DATA_NETWORK_DISCONNECTED*/(DWORD)0xC000A080L: return "{Delayed Write Failed} Windows was unable to save all the data for the file %hs; the data has been lost. This error may be caused by network connectivity issues. Please try to save this file elsewhere.";
		case /*STATUS_LOST_WRITEBEHIND_DATA_NETWORK_SERVER_ERROR*/(DWORD)0xC000A081L: return "{Delayed Write Failed} Windows was unable to save all the data for the file %hs; the data has been lost. This error was returned by the server on which the file exists. Please try to save this file elsewhere.";
		case /*STATUS_LOST_WRITEBEHIND_DATA_LOCAL_DISK_ERROR*/(DWORD)0xC000A082L: return "{Delayed Write Failed} Windows was unable to save all the data for the file %hs; the data has been lost. This error may be caused if the device has been removed or the media is write-protected.";
		case /*STATUS_XML_PARSE_ERROR*/(DWORD)0xC000A083L: return "Windows was unable to parse the requested XML data.";
		case /*STATUS_XMLDSIG_ERROR*/(DWORD)0xC000A084L: return "An error was encountered while processing an XML digital signature.";
		case /*STATUS_WRONG_COMPARTMENT*/(DWORD)0xC000A085L: return "Indicates that the caller made the connection request in the wrong routing compartment.";
		case /*STATUS_AUTHIP_FAILURE*/(DWORD)0xC000A086L: return "Indicates that there was an AuthIP failure when attempting to connect to the remote host.";
		case /*DBG_NO_STATE_CHANGE*/(DWORD)0xC0010001L: return "Debugger error values Debugger did not perform a state change.";
		case /*DBG_APP_NOT_IDLE*/(DWORD)0xC0010002L: return "Debugger has found the application is not idle.";
		case /*RPC_NT_INVALID_STRING_BINDING*/(DWORD)0xC0020001L: return "RPC error values The string binding is invalid.";
		case /*RPC_NT_WRONG_KIND_OF_BINDING*/(DWORD)0xC0020002L: return "The binding handle is not the correct type.";
		case /*RPC_NT_INVALID_BINDING*/(DWORD)0xC0020003L: return "The binding handle is invalid.";
		case /*RPC_NT_PROTSEQ_NOT_SUPPORTED*/(DWORD)0xC0020004L: return "The RPC protocol sequence is not supported.";
		case /*RPC_NT_INVALID_RPC_PROTSEQ*/(DWORD)0xC0020005L: return "The RPC protocol sequence is invalid.";
		case /*RPC_NT_INVALID_STRING_UUID*/(DWORD)0xC0020006L: return "The string UUID is invalid.";
		case /*RPC_NT_INVALID_ENDPOINT_FORMAT*/(DWORD)0xC0020007L: return "The endpoint format is invalid.";
		case /*RPC_NT_INVALID_NET_ADDR*/(DWORD)0xC0020008L: return "The network address is invalid.";
		case /*RPC_NT_NO_ENDPOINT_FOUND*/(DWORD)0xC0020009L: return "No endpoint was found.";
		case /*RPC_NT_INVALID_TIMEOUT*/(DWORD)0xC002000AL: return "The timeout value is invalid.";
		case /*RPC_NT_OBJECT_NOT_FOUND*/(DWORD)0xC002000BL: return "The object UUID was not found.";
		case /*RPC_NT_ALREADY_REGISTERED*/(DWORD)0xC002000CL: return "The object UUID has already been registered.";
		case /*RPC_NT_TYPE_ALREADY_REGISTERED*/(DWORD)0xC002000DL: return "The type UUID has already been registered.";
		case /*RPC_NT_ALREADY_LISTENING*/(DWORD)0xC002000EL: return "The RPC server is already listening.";
		case /*RPC_NT_NO_PROTSEQS_REGISTERED*/(DWORD)0xC002000FL: return "No protocol sequences have been registered.";
		case /*RPC_NT_NOT_LISTENING*/(DWORD)0xC0020010L: return "The RPC server is not listening.";
		case /*RPC_NT_UNKNOWN_MGR_TYPE*/(DWORD)0xC0020011L: return "The manager type is unknown.";
		case /*RPC_NT_UNKNOWN_IF*/(DWORD)0xC0020012L: return "The interface is unknown.";
		case /*RPC_NT_NO_BINDINGS*/(DWORD)0xC0020013L: return "There are no bindings.";
		case /*RPC_NT_NO_PROTSEQS*/(DWORD)0xC0020014L: return "There are no protocol sequences.";
		case /*RPC_NT_CANT_CREATE_ENDPOINT*/(DWORD)0xC0020015L: return "The endpoint cannot be created.";
		case /*RPC_NT_OUT_OF_RESOURCES*/(DWORD)0xC0020016L: return "Not enough resources are available to complete this operation.";
		case /*RPC_NT_SERVER_UNAVAILABLE*/(DWORD)0xC0020017L: return "The RPC server is unavailable.";
		case /*RPC_NT_SERVER_TOO_BUSY*/(DWORD)0xC0020018L: return "The RPC server is too busy to complete this operation.";
		case /*RPC_NT_INVALID_NETWORK_OPTIONS*/(DWORD)0xC0020019L: return "The network options are invalid.";
		case /*RPC_NT_NO_CALL_ACTIVE*/(DWORD)0xC002001AL: return "There are no remote procedure calls active on this thread.";
		case /*RPC_NT_CALL_FAILED*/(DWORD)0xC002001BL: return "The remote procedure call failed.";
		case /*RPC_NT_CALL_FAILED_DNE*/(DWORD)0xC002001CL: return "The remote procedure call failed and did not execute.";
		case /*RPC_NT_PROTOCOL_ERROR*/(DWORD)0xC002001DL: return "An RPC protocol error occurred.";
		case /*RPC_NT_UNSUPPORTED_TRANS_SYN*/(DWORD)0xC002001FL: return "The transfer syntax is not supported by the RPC server.";
		case /*RPC_NT_UNSUPPORTED_TYPE*/(DWORD)0xC0020021L: return "The type UUID is not supported.";
		case /*RPC_NT_INVALID_TAG*/(DWORD)0xC0020022L: return "The tag is invalid.";
		case /*RPC_NT_INVALID_BOUND*/(DWORD)0xC0020023L: return "The array bounds are invalid.";
		case /*RPC_NT_NO_ENTRY_NAME*/(DWORD)0xC0020024L: return "The binding does not contain an entry name.";
		case /*RPC_NT_INVALID_NAME_SYNTAX*/(DWORD)0xC0020025L: return "The name syntax is invalid.";
		case /*RPC_NT_UNSUPPORTED_NAME_SYNTAX*/(DWORD)0xC0020026L: return "The name syntax is not supported.";
		case /*RPC_NT_UUID_NO_ADDRESS*/(DWORD)0xC0020028L: return "No network address is available to use to construct a UUID.";
		case /*RPC_NT_DUPLICATE_ENDPOINT*/(DWORD)0xC0020029L: return "The endpoint is a duplicate.";
		case /*RPC_NT_UNKNOWN_AUTHN_TYPE*/(DWORD)0xC002002AL: return "The authentication type is unknown.";
		case /*RPC_NT_MAX_CALLS_TOO_SMALL*/(DWORD)0xC002002BL: return "The maximum number of calls is too small.";
		case /*RPC_NT_STRING_TOO_LONG*/(DWORD)0xC002002CL: return "The string is too long.";
		case /*RPC_NT_PROTSEQ_NOT_FOUND*/(DWORD)0xC002002DL: return "The RPC protocol sequence was not found.";
		case /*RPC_NT_PROCNUM_OUT_OF_RANGE*/(DWORD)0xC002002EL: return "The procedure number is out of range.";
		case /*RPC_NT_BINDING_HAS_NO_AUTH*/(DWORD)0xC002002FL: return "The binding does not contain any authentication information.";
		case /*RPC_NT_UNKNOWN_AUTHN_SERVICE*/(DWORD)0xC0020030L: return "The authentication service is unknown.";
		case /*RPC_NT_UNKNOWN_AUTHN_LEVEL*/(DWORD)0xC0020031L: return "The authentication level is unknown.";
		case /*RPC_NT_INVALID_AUTH_IDENTITY*/(DWORD)0xC0020032L: return "The security context is invalid.";
		case /*RPC_NT_UNKNOWN_AUTHZ_SERVICE*/(DWORD)0xC0020033L: return "The authorization service is unknown.";
		case /*EPT_NT_INVALID_ENTRY*/(DWORD)0xC0020034L: return "The entry is invalid.";
		case /*EPT_NT_CANT_PERFORM_OP*/(DWORD)0xC0020035L: return "The operation cannot be performed.";
		case /*EPT_NT_NOT_REGISTERED*/(DWORD)0xC0020036L: return "There are no more endpoints available from the endpoint mapper.";
		case /*RPC_NT_NOTHING_TO_EXPORT*/(DWORD)0xC0020037L: return "No interfaces have been exported.";
		case /*RPC_NT_INCOMPLETE_NAME*/(DWORD)0xC0020038L: return "The entry name is incomplete.";
		case /*RPC_NT_INVALID_VERS_OPTION*/(DWORD)0xC0020039L: return "The version option is invalid.";
		case /*RPC_NT_NO_MORE_MEMBERS*/(DWORD)0xC002003AL: return "There are no more members.";
		case /*RPC_NT_NOT_ALL_OBJS_UNEXPORTED*/(DWORD)0xC002003BL: return "There is nothing to unexport.";
		case /*RPC_NT_INTERFACE_NOT_FOUND*/(DWORD)0xC002003CL: return "The interface was not found.";
		case /*RPC_NT_ENTRY_ALREADY_EXISTS*/(DWORD)0xC002003DL: return "The entry already exists.";
		case /*RPC_NT_ENTRY_NOT_FOUND*/(DWORD)0xC002003EL: return "The entry is not found.";
		case /*RPC_NT_NAME_SERVICE_UNAVAILABLE*/(DWORD)0xC002003FL: return "The name service is unavailable.";
		case /*RPC_NT_INVALID_NAF_ID*/(DWORD)0xC0020040L: return "The network address family is invalid.";
		case /*RPC_NT_CANNOT_SUPPORT*/(DWORD)0xC0020041L: return "The requested operation is not supported.";
		case /*RPC_NT_NO_CONTEXT_AVAILABLE*/(DWORD)0xC0020042L: return "No security context is available to allow impersonation.";
		case /*RPC_NT_INTERNAL_ERROR*/(DWORD)0xC0020043L: return "An internal error occurred in RPC.";
		case /*RPC_NT_ZERO_DIVIDE*/(DWORD)0xC0020044L: return "The RPC server attempted an integer divide by zero.";
		case /*RPC_NT_ADDRESS_ERROR*/(DWORD)0xC0020045L: return "An addressing error occurred in the RPC server.";
		case /*RPC_NT_FP_DIV_ZERO*/(DWORD)0xC0020046L: return "A floating point operation at the RPC server caused a divide by zero.";
		case /*RPC_NT_FP_UNDERFLOW*/(DWORD)0xC0020047L: return "A floating point underflow occurred at the RPC server.";
		case /*RPC_NT_FP_OVERFLOW*/(DWORD)0xC0020048L: return "A floating point overflow occurred at the RPC server.";
		case /*RPC_NT_NO_MORE_ENTRIES*/(DWORD)0xC0030001L: return "The list of RPC servers available for auto-handle binding has been exhausted.";
		case /*RPC_NT_SS_CHAR_TRANS_OPEN_FAIL*/(DWORD)0xC0030002L: return "The file designated by DCERPCCHARTRANS cannot be opened.";
		case /*RPC_NT_SS_CHAR_TRANS_SHORT_FILE*/(DWORD)0xC0030003L: return "The file containing the character translation table has fewer than 512 bytes.";
		case /*RPC_NT_SS_IN_NULL_CONTEXT*/(DWORD)0xC0030004L: return "A null context handle is passed as an [in] parameter.";
		case /*RPC_NT_SS_CONTEXT_MISMATCH*/(DWORD)0xC0030005L: return "The context handle does not match any known context handles.";
		case /*RPC_NT_SS_CONTEXT_DAMAGED*/(DWORD)0xC0030006L: return "The context handle changed during a call.";
		case /*RPC_NT_SS_HANDLES_MISMATCH*/(DWORD)0xC0030007L: return "The binding handles passed to a remote procedure call do not match.";
		case /*RPC_NT_SS_CANNOT_GET_CALL_HANDLE*/(DWORD)0xC0030008L: return "The stub is unable to get the call handle.";
		case /*RPC_NT_NULL_REF_POINTER*/(DWORD)0xC0030009L: return "A null reference pointer was passed to the stub.";
		case /*RPC_NT_ENUM_VALUE_OUT_OF_RANGE*/(DWORD)0xC003000AL: return "The enumeration value is out of range.";
		case /*RPC_NT_BYTE_COUNT_TOO_SMALL*/(DWORD)0xC003000BL: return "The byte count is too small.";
		case /*RPC_NT_BAD_STUB_DATA*/(DWORD)0xC003000CL: return "The stub received bad data.";
		case /*RPC_NT_CALL_IN_PROGRESS*/(DWORD)0xC0020049L: return "A remote procedure call is already in progress for this thread.";
		case /*RPC_NT_NO_MORE_BINDINGS*/(DWORD)0xC002004AL: return "There are no more bindings.";
		case /*RPC_NT_GROUP_MEMBER_NOT_FOUND*/(DWORD)0xC002004BL: return "The group member was not found.";
		case /*EPT_NT_CANT_CREATE*/(DWORD)0xC002004CL: return "The endpoint mapper database entry could not be created.";
		case /*RPC_NT_INVALID_OBJECT*/(DWORD)0xC002004DL: return "The object UUID is the nil UUID.";
		case /*RPC_NT_NO_INTERFACES*/(DWORD)0xC002004FL: return "No interfaces have been registered.";
		case /*RPC_NT_CALL_CANCELLED*/(DWORD)0xC0020050L: return "The remote procedure call was cancelled.";
		case /*RPC_NT_BINDING_INCOMPLETE*/(DWORD)0xC0020051L: return "The binding handle does not contain all required information.";
		case /*RPC_NT_COMM_FAILURE*/(DWORD)0xC0020052L: return "A communications failure occurred during a remote procedure call.";
		case /*RPC_NT_UNSUPPORTED_AUTHN_LEVEL*/(DWORD)0xC0020053L: return "The requested authentication level is not supported.";
		case /*RPC_NT_NO_PRINC_NAME*/(DWORD)0xC0020054L: return "No principal name registered.";
		case /*RPC_NT_NOT_RPC_ERROR*/(DWORD)0xC0020055L: return "The error specified is not a valid Windows RPC error code.";
		case /*RPC_NT_UUID_LOCAL_ONLY*/(DWORD)0x40020056L: return "A UUID that is valid only on this computer has been allocated.";
		case /*RPC_NT_SEC_PKG_ERROR*/(DWORD)0xC0020057L: return "A security package specific error occurred.";
		case /*RPC_NT_NOT_CANCELLED*/(DWORD)0xC0020058L: return "Thread is not cancelled.";
		case /*RPC_NT_INVALID_ES_ACTION*/(DWORD)0xC0030059L: return "Invalid operation on the encoding/decoding handle.";
		case /*RPC_NT_WRONG_ES_VERSION*/(DWORD)0xC003005AL: return "Incompatible version of the serializing package.";
		case /*RPC_NT_WRONG_STUB_VERSION*/(DWORD)0xC003005BL: return "Incompatible version of the RPC stub.";
		case /*RPC_NT_INVALID_PIPE_OBJECT*/(DWORD)0xC003005CL: return "The RPC pipe object is invalid or corrupted.";
		case /*RPC_NT_INVALID_PIPE_OPERATION*/(DWORD)0xC003005DL: return "An invalid operation was attempted on an RPC pipe object.";
		case /*RPC_NT_WRONG_PIPE_VERSION*/(DWORD)0xC003005EL: return "Unsupported RPC pipe version.";
		case /*RPC_NT_PIPE_CLOSED*/(DWORD)0xC003005FL: return "The RPC pipe object has already been closed.";
		case /*RPC_NT_PIPE_DISCIPLINE_ERROR*/(DWORD)0xC0030060L: return "The RPC call completed before all pipes were processed.";
		case /*RPC_NT_PIPE_EMPTY*/(DWORD)0xC0030061L: return "No more data is available from the RPC pipe.";
		case /*RPC_NT_INVALID_ASYNC_HANDLE*/(DWORD)0xC0020062L: return "Invalid asynchronous remote procedure call handle.";
		case /*RPC_NT_INVALID_ASYNC_CALL*/(DWORD)0xC0020063L: return "Invalid asynchronous RPC call handle for this operation.";
		case /*RPC_NT_PROXY_ACCESS_DENIED*/(DWORD)0xC0020064L: return "Access to the HTTP proxy is denied.";
		case /*RPC_NT_SEND_INCOMPLETE*/(DWORD)0x400200AFL: return "Some data remains to be sent in the request buffer.";
		case /*STATUS_ACPI_INVALID_OPCODE*/(DWORD)0xC0140001L: return "ACPI error values An attempt was made to run an invalid AML opcode";
		case /*STATUS_ACPI_STACK_OVERFLOW*/(DWORD)0xC0140002L: return "The AML Interpreter Stack has overflowed";
		case /*STATUS_ACPI_ASSERT_FAILED*/(DWORD)0xC0140003L: return "An inconsistent state has occurred";
		case /*STATUS_ACPI_INVALID_INDEX*/(DWORD)0xC0140004L: return "An attempt was made to access an array outside of its bounds";
		case /*STATUS_ACPI_INVALID_ARGUMENT*/(DWORD)0xC0140005L: return "A required argument was not specified";
		case /*STATUS_ACPI_FATAL*/(DWORD)0xC0140006L: return "A fatal error has occurred";
		case /*STATUS_ACPI_INVALID_SUPERNAME*/(DWORD)0xC0140007L: return "An invalid SuperName was specified";
		case /*STATUS_ACPI_INVALID_ARGTYPE*/(DWORD)0xC0140008L: return "An argument with an incorrect type was specified";
		case /*STATUS_ACPI_INVALID_OBJTYPE*/(DWORD)0xC0140009L: return "An object with an incorrect type was specified";
		case /*STATUS_ACPI_INVALID_TARGETTYPE*/(DWORD)0xC014000AL: return "A target with an incorrect type was specified";
		case /*STATUS_ACPI_INCORRECT_ARGUMENT_COUNT*/(DWORD)0xC014000BL: return "An incorrect number of arguments were specified";
		case /*STATUS_ACPI_ADDRESS_NOT_MAPPED*/(DWORD)0xC014000CL: return "An address failed to translate";
		case /*STATUS_ACPI_INVALID_EVENTTYPE*/(DWORD)0xC014000DL: return "An incorrect event type was specified";
		case /*STATUS_ACPI_HANDLER_COLLISION*/(DWORD)0xC014000EL: return "A handler for the target already exists";
		case /*STATUS_ACPI_INVALID_DATA*/(DWORD)0xC014000FL: return "Invalid data for the target was specified";
		case /*STATUS_ACPI_INVALID_REGION*/(DWORD)0xC0140010L: return "An invalid region for the target was specified";
		case /*STATUS_ACPI_INVALID_ACCESS_SIZE*/(DWORD)0xC0140011L: return "An attempt was made to access a field outside of the defined range";
		case /*STATUS_ACPI_ACQUIRE_GLOBAL_LOCK*/(DWORD)0xC0140012L: return "The Global system lock could not be acquired";
		case /*STATUS_ACPI_ALREADY_INITIALIZED*/(DWORD)0xC0140013L: return "An attempt was made to reinitialize the ACPI subsystem";
		case /*STATUS_ACPI_NOT_INITIALIZED*/(DWORD)0xC0140014L: return "The ACPI subsystem has not been initialized";
		case /*STATUS_ACPI_INVALID_MUTEX_LEVEL*/(DWORD)0xC0140015L: return "An incorrect mutex was specified";
		case /*STATUS_ACPI_MUTEX_NOT_OWNED*/(DWORD)0xC0140016L: return "The mutex is not currently owned";
		case /*STATUS_ACPI_MUTEX_NOT_OWNER*/(DWORD)0xC0140017L: return "An attempt was made to access the mutex by a process that was not the owner";
		case /*STATUS_ACPI_RS_ACCESS*/(DWORD)0xC0140018L: return "An error occurred during an access to Region Space";
		case /*STATUS_ACPI_INVALID_TABLE*/(DWORD)0xC0140019L: return "An attempt was made to use an incorrect table";
		case /*STATUS_ACPI_REG_HANDLER_FAILED*/(DWORD)0xC0140020L: return "The registration of an ACPI event failed";
		case /*STATUS_ACPI_POWER_REQUEST_FAILED*/(DWORD)0xC0140021L: return "An ACPI Power Object failed to transition state";
		case /*STATUS_CTX_WINSTATION_NAME_INVALID*/(DWORD)0xC00A0001L: return "Terminal Server specific Errors Session name %1 is invalid.";
		case /*STATUS_CTX_INVALID_PD*/(DWORD)0xC00A0002L: return "The protocol driver %1 is invalid.";
		case /*STATUS_CTX_PD_NOT_FOUND*/(DWORD)0xC00A0003L: return "The protocol driver %1 was not found in the system path.";
		case /*STATUS_CTX_CDM_CONNECT*/(DWORD)0x400A0004L: return "The Client Drive Mapping Service Has Connected on Terminal Connection.";
		case /*STATUS_CTX_CDM_DISCONNECT*/(DWORD)0x400A0005L: return "The Client Drive Mapping Service Has Disconnected on Terminal Connection.";
		case /*STATUS_CTX_CLOSE_PENDING*/(DWORD)0xC00A0006L: return "A close operation is pending on the Terminal Connection.";
		case /*STATUS_CTX_NO_OUTBUF*/(DWORD)0xC00A0007L: return "There are no free output buffers available.";
		case /*STATUS_CTX_MODEM_INF_NOT_FOUND*/(DWORD)0xC00A0008L: return "The MODEM.INF file was not found.";
		case /*STATUS_CTX_INVALID_MODEMNAME*/(DWORD)0xC00A0009L: return "The modem (%1) was not found in MODEM.INF.";
		case /*STATUS_CTX_RESPONSE_ERROR*/(DWORD)0xC00A000AL: return "The modem did not accept the command sent to it. Verify the configured modem name matches the attached modem.";
		case /*STATUS_CTX_MODEM_RESPONSE_TIMEOUT*/(DWORD)0xC00A000BL: return "The modem did not respond to the command sent to it. Verify the modem is properly cabled and powered on.";
		case /*STATUS_CTX_MODEM_RESPONSE_NO_CARRIER*/(DWORD)0xC00A000CL: return "Carrier detect has failed or carrier has been dropped due to disconnect.";
		case /*STATUS_CTX_MODEM_RESPONSE_NO_DIALTONE*/(DWORD)0xC00A000DL: return "Dial tone not detected within required time. Verify phone cable is properly attached and functional.";
		case /*STATUS_CTX_MODEM_RESPONSE_BUSY*/(DWORD)0xC00A000EL: return "Busy signal detected at remote site on callback.";
		case /*STATUS_CTX_MODEM_RESPONSE_VOICE*/(DWORD)0xC00A000FL: return "Voice detected at remote site on callback.";
		case /*STATUS_CTX_TD_ERROR*/(DWORD)0xC00A0010L: return "Transport driver error";
		case /*STATUS_CTX_LICENSE_CLIENT_INVALID*/(DWORD)0xC00A0012L: return "The client you are using is not licensed to use this system. Your logon request is denied.";
		case /*STATUS_CTX_LICENSE_NOT_AVAILABLE*/(DWORD)0xC00A0013L: return "The system has reached its licensed logon limit. Please try again later.";
		case /*STATUS_CTX_LICENSE_EXPIRED*/(DWORD)0xC00A0014L: return "The system license has expired. Your logon request is denied.";
		case /*STATUS_CTX_WINSTATION_NOT_FOUND*/(DWORD)0xC00A0015L: return "The specified session cannot be found.";
		case /*STATUS_CTX_WINSTATION_NAME_COLLISION*/(DWORD)0xC00A0016L: return "The specified session name is already in use.";
		case /*STATUS_CTX_WINSTATION_BUSY*/(DWORD)0xC00A0017L: return "The requested operation cannot be completed because the Terminal Connection is currently busy processing a connect, disconnect, reset, or delete operation.";
		case /*STATUS_CTX_BAD_VIDEO_MODE*/(DWORD)0xC00A0018L: return "An attempt has been made to connect to a session whose video mode is not supported by the current client.";
		case /*STATUS_CTX_GRAPHICS_INVALID*/(DWORD)0xC00A0022L: return "The application attempted to enable DOS graphics mode. DOS graphics mode is not supported.";
		case /*STATUS_CTX_NOT_CONSOLE*/(DWORD)0xC00A0024L: return "The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access.";
		case /*STATUS_CTX_CLIENT_QUERY_TIMEOUT*/(DWORD)0xC00A0026L: return "The client failed to respond to the server connect message.";
		case /*STATUS_CTX_CONSOLE_DISCONNECT*/(DWORD)0xC00A0027L: return "Disconnecting the console session is not supported.";
		case /*STATUS_CTX_CONSOLE_CONNECT*/(DWORD)0xC00A0028L: return "Reconnecting a disconnected session to the console is not supported.";
		case /*STATUS_CTX_SHADOW_DENIED*/(DWORD)0xC00A002AL: return "The request to control another session remotely was denied.";
		case /*STATUS_CTX_WINSTATION_ACCESS_DENIED*/(DWORD)0xC00A002BL: return "A process has requested access to a session, but has not been granted those access rights.";
		case /*STATUS_CTX_INVALID_WD*/(DWORD)0xC00A002EL: return "The Terminal Connection driver %1 is invalid.";
		case /*STATUS_CTX_WD_NOT_FOUND*/(DWORD)0xC00A002FL: return "The Terminal Connection driver %1 was not found in the system path.";
		case /*STATUS_CTX_SHADOW_INVALID*/(DWORD)0xC00A0030L: return "The requested session cannot be controlled remotely. You cannot control your own session, a session that is trying to control your session, a session that has no user logged on, nor control other sessions from the console.";
		case /*STATUS_CTX_SHADOW_DISABLED*/(DWORD)0xC00A0031L: return "The requested session is not configured to allow remote control.";
		case /*STATUS_RDP_PROTOCOL_ERROR*/(DWORD)0xC00A0032L: return "The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client.";
		case /*STATUS_CTX_CLIENT_LICENSE_NOT_SET*/(DWORD)0xC00A0033L: return "Your request to connect to this Terminal server has been rejected. Your Terminal Server Client license number has not been entered for this copy of the Terminal Client. Please call your system administrator for help in entering a valid, unique license number for this Terminal Server Client. Click OK to continue.";
		case /*STATUS_CTX_CLIENT_LICENSE_IN_USE*/(DWORD)0xC00A0034L: return "Your request to connect to this Terminal server has been rejected. Your Terminal Server Client license number is currently being used by another user. Please call your system administrator to obtain a new copy of the Terminal Server Client with a valid, unique license number. Click OK to continue.";
		case /*STATUS_CTX_SHADOW_ENDED_BY_MODE_CHANGE*/(DWORD)0xC00A0035L: return "The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.";
		case /*STATUS_CTX_SHADOW_NOT_RUNNING*/(DWORD)0xC00A0036L: return "Remote control could not be terminated because the specified session is not currently being remotely controlled.";
		case /*STATUS_CTX_LOGON_DISABLED*/(DWORD)0xC00A0037L: return "Your interactive logon privilege has been disabled. Please contact your system administrator.";
		case /*STATUS_CTX_SECURITY_LAYER_ERROR*/(DWORD)0xC00A0038L: return "The Terminal Server security layer detected an error in the protocol stream and has disconnected the client.";
		case /*STATUS_TS_INCOMPATIBLE_SESSIONS*/(DWORD)0xC00A0039L: return "The target session is incompatible with the current session.";
		case /*STATUS_PNP_BAD_MPS_TABLE*/(DWORD)0xC0040035L: return "IO error values A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update.";
		case /*STATUS_PNP_TRANSLATION_FAILED*/(DWORD)0xC0040036L: return "A translator failed to translate resources.";
		case /*STATUS_PNP_IRQ_TRANSLATION_FAILED*/(DWORD)0xC0040037L: return "A IRQ translator failed to translate resources.";
		case /*STATUS_PNP_INVALID_ID*/(DWORD)0xC0040038L: return "Driver %2 returned invalid ID for a child device (%3).";
		case /*STATUS_IO_REISSUE_AS_CACHED*/(DWORD)0xC0040039L: return "Reissue the given operation as a cached IO operation";
		case /*STATUS_MUI_FILE_NOT_FOUND*/(DWORD)0xC00B0001L: return "MUI error values The resource loader failed to find MUI file.";
		case /*STATUS_MUI_INVALID_FILE*/(DWORD)0xC00B0002L: return "The resource loader failed to load MUI file because the file fail to pass validation.";
		case /*STATUS_MUI_INVALID_RC_CONFIG*/(DWORD)0xC00B0003L: return "The RC Manifest is corrupted with garbage data or unsupported version or missing required item.";
		case /*STATUS_MUI_INVALID_LOCALE_NAME*/(DWORD)0xC00B0004L: return "The RC Manifest has invalid culture name.";
		case /*STATUS_MUI_INVALID_ULTIMATEFALLBACK_NAME*/(DWORD)0xC00B0005L: return "The RC Manifest has invalid ultimatefallback name.";
		case /*STATUS_MUI_FILE_NOT_LOADED*/(DWORD)0xC00B0006L: return "The resource loader cache doesn't have loaded MUI entry.";
		case /*STATUS_RESOURCE_ENUM_USER_STOP*/(DWORD)0xC00B0007L: return "User stopped resource enumeration.";
		case /*STATUS_FLT_NO_HANDLER_DEFINED*/(DWORD)0xC01C0001L: return "A handler was not defined by the filter for this operation.";
		case /*STATUS_FLT_CONTEXT_ALREADY_DEFINED*/(DWORD)0xC01C0002L: return "A context is already defined for this object.";
		case /*STATUS_FLT_INVALID_ASYNCHRONOUS_REQUEST*/(DWORD)0xC01C0003L: return "Asynchronous requests are not valid for this operation.";
		case /*STATUS_FLT_DISALLOW_FAST_IO*/(DWORD)0xC01C0004L: return "Internal error code used by the filter manager to determine if a fastio operation should be forced down the IRP path.  Mini-filters should never return this value.";
		case /*STATUS_FLT_INVALID_NAME_REQUEST*/(DWORD)0xC01C0005L: return "An invalid name request was made.  The name requested cannot be retrieved at this time.";
		case /*STATUS_FLT_NOT_SAFE_TO_POST_OPERATION*/(DWORD)0xC01C0006L: return "Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.";
		case /*STATUS_FLT_NOT_INITIALIZED*/(DWORD)0xC01C0007L: return "The Filter Manager was not initialized when a filter tried to register.  Make sure that the Filter Manager is getting loaded as a driver.";
		case /*STATUS_FLT_FILTER_NOT_READY*/(DWORD)0xC01C0008L: return "The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).";
		case /*STATUS_FLT_POST_OPERATION_CLEANUP*/(DWORD)0xC01C0009L: return "The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.";
		case /*STATUS_FLT_INTERNAL_ERROR*/(DWORD)0xC01C000AL: return "The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed.  This is usually the result of a filter returning an invalid value from a pre-operation callback.";
		case /*STATUS_FLT_DELETING_OBJECT*/(DWORD)0xC01C000BL: return "The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.";
		case /*STATUS_FLT_MUST_BE_NONPAGED_POOL*/(DWORD)0xC01C000CL: return "Non-paged pool must be used for this type of context.";
		case /*STATUS_FLT_DUPLICATE_ENTRY*/(DWORD)0xC01C000DL: return "A duplicate handler definition has been provided for an operation.";
		case /*STATUS_FLT_CBDQ_DISABLED*/(DWORD)0xC01C000EL: return "The callback data queue has been disabled.";
		case /*STATUS_FLT_DO_NOT_ATTACH*/(DWORD)0xC01C000FL: return "Do not attach the filter to the volume at this time.";
		case /*STATUS_FLT_DO_NOT_DETACH*/(DWORD)0xC01C0010L: return "Do not detach the filter from the volume at this time.";
		case /*STATUS_FLT_INSTANCE_ALTITUDE_COLLISION*/(DWORD)0xC01C0011L: return "An instance already exists at this altitude on the volume specified.";
		case /*STATUS_FLT_INSTANCE_NAME_COLLISION*/(DWORD)0xC01C0012L: return "An instance already exists with this name on the volume specified.";
		case /*STATUS_FLT_FILTER_NOT_FOUND*/(DWORD)0xC01C0013L: return "The system could not find the filter specified.";
		case /*STATUS_FLT_VOLUME_NOT_FOUND*/(DWORD)0xC01C0014L: return "The system could not find the volume specified.";
		case /*STATUS_FLT_INSTANCE_NOT_FOUND*/(DWORD)0xC01C0015L: return "The system could not find the instance specified.";
		case /*STATUS_FLT_CONTEXT_ALLOCATION_NOT_FOUND*/(DWORD)0xC01C0016L: return "No registered context allocation definition was found for the given request.";
		case /*STATUS_FLT_INVALID_CONTEXT_REGISTRATION*/(DWORD)0xC01C0017L: return "An invalid parameter was specified during context registration.";
		case /*STATUS_FLT_NAME_CACHE_MISS*/(DWORD)0xC01C0018L: return "The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.";
		case /*STATUS_FLT_NO_DEVICE_OBJECT*/(DWORD)0xC01C0019L: return "The requested device object does not exist for the given volume.";
		case /*STATUS_FLT_VOLUME_ALREADY_MOUNTED*/(DWORD)0xC01C001AL: return "The specified volume is already mounted.";
		case /*STATUS_FLT_ALREADY_ENLISTED*/(DWORD)0xC01C001BL: return "The specified Transaction Context is already enlisted in a transaction";
		case /*STATUS_FLT_CONTEXT_ALREADY_LINKED*/(DWORD)0xC01C001CL: return "The specifiec context is already attached to another object";
		case /*STATUS_FLT_NO_WAITER_FOR_REPLY*/(DWORD)0xC01C0020L: return "No waiter is present for the filter's reply to this message.";
		case /*STATUS_SXS_SECTION_NOT_FOUND*/(DWORD)0xC0150001L: return "Side-by-side (SXS) error values The requested section is not present in the activation context.";
		case /*STATUS_SXS_CANT_GEN_ACTCTX*/(DWORD)0xC0150002L: return "Windows was not able to process the application binding information. Please refer to your System Event Log for further information.";
		case /*STATUS_SXS_INVALID_ACTCTXDATA_FORMAT*/(DWORD)0xC0150003L: return "The application binding data format is invalid.";
		case /*STATUS_SXS_ASSEMBLY_NOT_FOUND*/(DWORD)0xC0150004L: return "The referenced assembly is not installed on your system.";
		case /*STATUS_SXS_MANIFEST_FORMAT_ERROR*/(DWORD)0xC0150005L: return "The manifest file does not begin with the required tag and format information.";
		case /*STATUS_SXS_MANIFEST_PARSE_ERROR*/(DWORD)0xC0150006L: return "The manifest file contains one or more syntax errors.";
		case /*STATUS_SXS_ACTIVATION_CONTEXT_DISABLED*/(DWORD)0xC0150007L: return "The application attempted to activate a disabled activation context.";
		case /*STATUS_SXS_KEY_NOT_FOUND*/(DWORD)0xC0150008L: return "The requested lookup key was not found in any active activation context.";
		case /*STATUS_SXS_VERSION_CONFLICT*/(DWORD)0xC0150009L: return "A component version required by the application conflicts with another component version already active.";
		case /*STATUS_SXS_WRONG_SECTION_TYPE*/(DWORD)0xC015000AL: return "The type requested activation context section does not match the query API used.";
		case /*STATUS_SXS_THREAD_QUERIES_DISABLED*/(DWORD)0xC015000BL: return "Lack of system resources has required isolated activation to be disabled for the current thread of execution.";
		case /*STATUS_SXS_ASSEMBLY_MISSING*/(DWORD)0xC015000CL: return "The referenced assembly could not be found.";
		case /*STATUS_SXS_RELEASE_ACTIVATION_CONTEXT*/(DWORD)0x4015000DL: return "A kernel mode component is releasing a reference on an activation context.";
		case /*STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET*/(DWORD)0xC015000EL: return "An attempt to set the process default activation context failed because the process default activation context was already set.";
		case /*STATUS_SXS_EARLY_DEACTIVATION*/(DWORD)0xC015000FL: return "The activation context being deactivated is not the most recently activated one.";
		case /*STATUS_SXS_INVALID_DEACTIVATION*/(DWORD)0xC0150010L: return "The activation context being deactivated is not active for the current thread of execution.";
		case /*STATUS_SXS_MULTIPLE_DEACTIVATION*/(DWORD)0xC0150011L: return "The activation context being deactivated has already been deactivated.";
		case /*STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY*/(DWORD)0xC0150012L: return "The activation context of system default assembly could not be generated.";
		case /*STATUS_SXS_PROCESS_TERMINATION_REQUESTED*/(DWORD)0xC0150013L: return "A component used by the isolation facility has requested to terminate the process.";
		case /*STATUS_SXS_CORRUPT_ACTIVATION_STACK*/(DWORD)0xC0150014L: return "The activation context activation stack for the running thread of execution is corrupt.";
		case /*STATUS_SXS_CORRUPTION*/(DWORD)0xC0150015L: return "The application isolation metadata for this process or thread has become corrupt.";
		case /*STATUS_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE*/(DWORD)0xC0150016L: return "The value of an attribute in an identity is not within the legal range.";
		case /*STATUS_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME*/(DWORD)0xC0150017L: return "The name of an attribute in an identity is not within the legal range.";
		case /*STATUS_SXS_IDENTITY_DUPLICATE_ATTRIBUTE*/(DWORD)0xC0150018L: return "An identity contains two definitions for the same attribute.";
		case /*STATUS_SXS_IDENTITY_PARSE_ERROR*/(DWORD)0xC0150019L: return "The identity string is malformed.  This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.";
		case /*STATUS_SXS_COMPONENT_STORE_CORRUPT*/(DWORD)0xC015001AL: return "The component store has been corrupted.";
		case /*STATUS_SXS_FILE_HASH_MISMATCH*/(DWORD)0xC015001BL: return "A component's file does not match the verification information present in the component manifest.";
		case /*STATUS_SXS_MANIFEST_IDENTITY_SAME_BUT_CONTENTS_DIFFERENT*/(DWORD)0xC015001CL: return "The identities of the manifests are identical but their contents are different.";
		case /*STATUS_SXS_IDENTITIES_DIFFERENT*/(DWORD)0xC015001DL: return "The component identities are different.";
		case /*STATUS_SXS_ASSEMBLY_IS_NOT_A_DEPLOYMENT*/(DWORD)0xC015001EL: return "The assembly is not a deployment.";
		case /*STATUS_SXS_FILE_NOT_PART_OF_ASSEMBLY*/(DWORD)0xC015001FL: return "The file is not a part of the assembly.";
		case /*STATUS_ADVANCED_INSTALLER_FAILED*/(DWORD)0xC0150020L: return "An advanced installer failed during setup or servicing.";
		case /*STATUS_XML_ENCODING_MISMATCH*/(DWORD)0xC0150021L: return "The character encoding in the XML declaration did not match the encoding used in the document.";
		case /*STATUS_SXS_MANIFEST_TOO_BIG*/(DWORD)0xC0150022L: return "The size of the manifest exceeds the maximum allowed.";
		case /*STATUS_SXS_SETTING_NOT_REGISTERED*/(DWORD)0xC0150023L: return "The setting is not registered.";
		case /*STATUS_SXS_TRANSACTION_CLOSURE_INCOMPLETE*/(DWORD)0xC0150024L: return "One or more required members of the transaction are not present.";
		case /*STATUS_SMI_PRIMITIVE_INSTALLER_FAILED*/(DWORD)0xC0150025L: return "The SMI primitive installer failed during setup or servicing.";
		case /*STATUS_GENERIC_COMMAND_FAILED*/(DWORD)0xC0150026L: return "A generic command executable returned a result that indicates failure.";
		case /*STATUS_SXS_FILE_HASH_MISSING*/(DWORD)0xC0150027L: return "A component is missing file verification information in its manifest.";
		case /*STATUS_CLUSTER_INVALID_NODE*/(DWORD)0xC0130001L: return "Cluster error values The cluster node is not valid.";
		case /*STATUS_CLUSTER_NODE_EXISTS*/(DWORD)0xC0130002L: return "The cluster node already exists.";
		case /*STATUS_CLUSTER_JOIN_IN_PROGRESS*/(DWORD)0xC0130003L: return "A node is in the process of joining the cluster.";
		case /*STATUS_CLUSTER_NODE_NOT_FOUND*/(DWORD)0xC0130004L: return "The cluster node was not found.";
		case /*STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND*/(DWORD)0xC0130005L: return "The cluster local node information was not found.";
		case /*STATUS_CLUSTER_NETWORK_EXISTS*/(DWORD)0xC0130006L: return "The cluster network already exists.";
		case /*STATUS_CLUSTER_NETWORK_NOT_FOUND*/(DWORD)0xC0130007L: return "The cluster network was not found.";
		case /*STATUS_CLUSTER_NETINTERFACE_EXISTS*/(DWORD)0xC0130008L: return "The cluster network interface already exists.";
		case /*STATUS_CLUSTER_NETINTERFACE_NOT_FOUND*/(DWORD)0xC0130009L: return "The cluster network interface was not found.";
		case /*STATUS_CLUSTER_INVALID_REQUEST*/(DWORD)0xC013000AL: return "The cluster request is not valid for this object.";
		case /*STATUS_CLUSTER_INVALID_NETWORK_PROVIDER*/(DWORD)0xC013000BL: return "The cluster network provider is not valid.";
		case /*STATUS_CLUSTER_NODE_DOWN*/(DWORD)0xC013000CL: return "The cluster node is down.";
		case /*STATUS_CLUSTER_NODE_UNREACHABLE*/(DWORD)0xC013000DL: return "The cluster node is not reachable.";
		case /*STATUS_CLUSTER_NODE_NOT_MEMBER*/(DWORD)0xC013000EL: return "The cluster node is not a member of the cluster.";
		case /*STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS*/(DWORD)0xC013000FL: return "A cluster join operation is not in progress.";
		case /*STATUS_CLUSTER_INVALID_NETWORK*/(DWORD)0xC0130010L: return "The cluster network is not valid.";
		case /*STATUS_CLUSTER_NO_NET_ADAPTERS*/(DWORD)0xC0130011L: return "No network adapters are available.";
		case /*STATUS_CLUSTER_NODE_UP*/(DWORD)0xC0130012L: return "The cluster node is up.";
		case /*STATUS_CLUSTER_NODE_PAUSED*/(DWORD)0xC0130013L: return "The cluster node is paused.";
		case /*STATUS_CLUSTER_NODE_NOT_PAUSED*/(DWORD)0xC0130014L: return "The cluster node is not paused.";
		case /*STATUS_CLUSTER_NO_SECURITY_CONTEXT*/(DWORD)0xC0130015L: return "No cluster security context is available.";
		case /*STATUS_CLUSTER_NETWORK_NOT_INTERNAL*/(DWORD)0xC0130016L: return "The cluster network is not configured for internal cluster communication.";
		case /*STATUS_CLUSTER_POISONED*/(DWORD)0xC0130017L: return "The cluster node has been poisoned.";
		case /*STATUS_TRANSACTIONAL_CONFLICT*/(DWORD)0xC0190001L: return "Transaction Manager error values The function attempted to use a name that is reserved for use by another transaction.";
		case /*STATUS_INVALID_TRANSACTION*/(DWORD)0xC0190002L: return "The transaction handle associated with this operation is not valid.";
		case /*STATUS_TRANSACTION_NOT_ACTIVE*/(DWORD)0xC0190003L: return "The requested operation was made in the context of a transaction that is no longer active.";
		case /*STATUS_TM_INITIALIZATION_FAILED*/(DWORD)0xC0190004L: return "The Transaction Manager was unable to be successfully initialized.  Transacted operations are not supported.";
		case /*STATUS_RM_NOT_ACTIVE*/(DWORD)0xC0190005L: return "Transaction support within the specified file system resource manager is not started or was shutdown due to an error.";
		case /*STATUS_RM_METADATA_CORRUPT*/(DWORD)0xC0190006L: return "The metadata of the RM has been corrupted.  The RM will not function.";
		case /*STATUS_TRANSACTION_NOT_JOINED*/(DWORD)0xC0190007L: return "The resource manager has attempted to prepare a transaction that it has not successfully joined.";
		case /*STATUS_DIRECTORY_NOT_RM*/(DWORD)0xC0190008L: return "The specified directory does not contain a file system resource manager.";
		case /*STATUS_COULD_NOT_RESIZE_LOG*/(DWORD)0x80190009L: return "The log could not be set to the requested size.";
		case /*STATUS_TRANSACTIONS_UNSUPPORTED_REMOTE*/(DWORD)0xC019000AL: return "The remote server or share does not support transacted file operations.";
		case /*STATUS_LOG_RESIZE_INVALID_SIZE*/(DWORD)0xC019000BL: return "The requested log size for the file system resource manager is invalid.";
		case /*STATUS_REMOTE_FILE_VERSION_MISMATCH*/(DWORD)0xC019000CL: return "The remote server sent mismatching version number or Fid for a file opened with transactions.";
		case /*STATUS_CRM_PROTOCOL_ALREADY_EXISTS*/(DWORD)0xC019000FL: return "The RM tried to register a protocol that already exists.";
		case /*STATUS_TRANSACTION_PROPAGATION_FAILED*/(DWORD)0xC0190010L: return "The attempt to propagate the Transaction failed.";
		case /*STATUS_CRM_PROTOCOL_NOT_FOUND*/(DWORD)0xC0190011L: return "The requested propagation protocol was not registered as a CRM.";
		case /*STATUS_TRANSACTION_SUPERIOR_EXISTS*/(DWORD)0xC0190012L: return "The Transaction object already has a superior enlistment, and the caller attempted an operation that would have created a new superior.  Only a single superior enlistment is allowed.";
		case /*STATUS_TRANSACTION_REQUEST_NOT_VALID*/(DWORD)0xC0190013L: return "The requested operation is not valid on the Transaction object in its current state.";
		case /*STATUS_TRANSACTION_NOT_REQUESTED*/(DWORD)0xC0190014L: return "The caller has called a response API, but the response is not expected because the TM did not issue the corresponding request to the caller.";
		case /*STATUS_TRANSACTION_ALREADY_ABORTED*/(DWORD)0xC0190015L: return "It is too late to perform the requested operation, since the Transaction has already been aborted.";
		case /*STATUS_TRANSACTION_ALREADY_COMMITTED*/(DWORD)0xC0190016L: return "It is too late to perform the requested operation, since the Transaction has already been committed.";
		case /*STATUS_TRANSACTION_INVALID_MARSHALL_BUFFER*/(DWORD)0xC0190017L: return "The buffer passed in to NtPushTransaction or NtPullTransaction is not in a valid format.";
		case /*STATUS_CURRENT_TRANSACTION_NOT_VALID*/(DWORD)0xC0190018L: return "The current transaction context associated with the thread is not a valid handle to a transaction object.";
		case /*STATUS_LOG_GROWTH_FAILED*/(DWORD)0xC0190019L: return "An attempt to create space in the transactional resource manager's log failed.  The failure status has been recorded in the event log.";
		case /*STATUS_OBJECT_NO_LONGER_EXISTS*/(DWORD)0xC0190021L: return "The object (file, stream, link) corresponding to the handle has been deleted by a transaction savepoint rollback.";
		case /*STATUS_STREAM_MINIVERSION_NOT_FOUND*/(DWORD)0xC0190022L: return "The specified file miniversion was not found for this transacted file open.";
		case /*STATUS_STREAM_MINIVERSION_NOT_VALID*/(DWORD)0xC0190023L: return "The specified file miniversion was found but has been invalidated. Most likely cause is a transaction savepoint rollback.";
		case /*STATUS_MINIVERSION_INACCESSIBLE_FROM_SPECIFIED_TRANSACTION*/(DWORD)0xC0190024L: return "A miniversion may only be opened in the context of the transaction that created it.";
		case /*STATUS_CANT_OPEN_MINIVERSION_WITH_MODIFY_INTENT*/(DWORD)0xC0190025L: return "It is not possible to open a miniversion with modify access.";
		case /*STATUS_CANT_CREATE_MORE_STREAM_MINIVERSIONS*/(DWORD)0xC0190026L: return "It is not possible to create any more miniversions for this stream.";
		case /*STATUS_HANDLE_NO_LONGER_VALID*/(DWORD)0xC0190028L: return "The handle has been invalidated by a transaction. The most likely cause is the presence of memory mapping on a file or an open handle when the transaction ended or rolled back to savepoint.";
		case /*STATUS_NO_TXF_METADATA*/(DWORD)0x80190029L: return "There is no transaction metadata on the file.";
		case /*STATUS_LOG_CORRUPTION_DETECTED*/(DWORD)0xC0190030L: return "The log data is corrupt.";
		case /*STATUS_CANT_RECOVER_WITH_HANDLE_OPEN*/(DWORD)0x80190031L: return "The file can't be recovered because there is a handle still open on it.";
		case /*STATUS_RM_DISCONNECTED*/(DWORD)0xC0190032L: return "The transaction outcome is unavailable because the resource manager responsible for it has disconnected.";
		case /*STATUS_ENLISTMENT_NOT_SUPERIOR*/(DWORD)0xC0190033L: return "The request was rejected because the enlistment in question is not a superior enlistment.";
		case /*STATUS_RECOVERY_NOT_NEEDED*/(DWORD)0x40190034L: return "The transactional resource manager is already consistent.  Recovery is not needed.";
		case /*STATUS_RM_ALREADY_STARTED*/(DWORD)0x40190035L: return "The transactional resource manager has already been started.";
		case /*STATUS_FILE_IDENTITY_NOT_PERSISTENT*/(DWORD)0xC0190036L: return "The file cannot be opened transactionally, because its identity depends on the outcome of an unresolved transaction.";
		case /*STATUS_CANT_BREAK_TRANSACTIONAL_DEPENDENCY*/(DWORD)0xC0190037L: return "The operation cannot be performed because another transaction is depending on the fact that this property will not change.";
		case /*STATUS_CANT_CROSS_RM_BOUNDARY*/(DWORD)0xC0190038L: return "The operation would involve a single file with two transactional resource managers and is therefore not allowed.";
		case /*STATUS_TXF_DIR_NOT_EMPTY*/(DWORD)0xC0190039L: return "The $Txf directory must be empty for this operation to succeed.";
		case /*STATUS_INDOUBT_TRANSACTIONS_EXIST*/(DWORD)0xC019003AL: return "The operation would leave a transactional resource manager in an inconsistent state and is therefore not allowed.";
		case /*STATUS_TM_VOLATILE*/(DWORD)0xC019003BL: return "The operation could not be completed because the transaction manager does not have a log.";
		case /*STATUS_ROLLBACK_TIMER_EXPIRED*/(DWORD)0xC019003CL: return "A rollback could not be scheduled because a previously scheduled rollback has already executed or been queued for execution.";
		case /*STATUS_TXF_ATTRIBUTE_CORRUPT*/(DWORD)0xC019003DL: return "The transactional metadata attribute on the file or directory %hs is corrupt and unreadable.";
		case /*STATUS_EFS_NOT_ALLOWED_IN_TRANSACTION*/(DWORD)0xC019003EL: return "The encryption operation could not be completed because a transaction is active.";
		case /*STATUS_TRANSACTIONAL_OPEN_NOT_ALLOWED*/(DWORD)0xC019003FL: return "This object is not allowed to be opened in a transaction.";
		case /*STATUS_TRANSACTED_MAPPING_UNSUPPORTED_REMOTE*/(DWORD)0xC0190040L: return "Memory mapping (creating a mapped section) a remote file under a transaction is not supported.";
		case /*STATUS_TXF_METADATA_ALREADY_PRESENT*/(DWORD)0x80190041L: return "Transaction metadata is already present on this file and cannot be superseded.";
		case /*STATUS_TRANSACTION_SCOPE_CALLBACKS_NOT_SET*/(DWORD)0x80190042L: return "A transaction scope could not be entered because the scope handler has not been initialized.";
		case /*STATUS_TRANSACTION_REQUIRED_PROMOTION*/(DWORD)0xC0190043L: return "Promotion was required in order to allow the resource manager to enlist, but the transaction was set to disallow it.";
		case /*STATUS_CANNOT_EXECUTE_FILE_IN_TRANSACTION*/(DWORD)0xC0190044L: return "This file is open for modification in an unresolved transaction and may be opened for execute only by a transacted reader.";
		case /*STATUS_TRANSACTIONS_NOT_FROZEN*/(DWORD)0xC0190045L: return "The request to thaw frozen transactions was ignored because transactions had not previously been frozen.";
		case /*STATUS_TRANSACTION_FREEZE_IN_PROGRESS*/(DWORD)0xC0190046L: return "Transactions cannot be frozen because a freeze is already in progress.";
		case /*STATUS_NOT_SNAPSHOT_VOLUME*/(DWORD)0xC0190047L: return "The target volume is not a snapshot volume.  This operation is only valid on a volume mounted as a snapshot.";
		case /*STATUS_NO_SAVEPOINT_WITH_OPEN_FILES*/(DWORD)0xC0190048L: return "The savepoint operation failed because files are open on the transaction.  This is not permitted.";
		case /*STATUS_SPARSE_NOT_ALLOWED_IN_TRANSACTION*/(DWORD)0xC0190049L: return "The sparse operation could not be completed because a transaction is active on the file.";
		case /*STATUS_TM_IDENTITY_MISMATCH*/(DWORD)0xC019004AL: return "The call to create a TransactionManager object failed because the Tm Identity stored in the logfile does not match the Tm Identity that was passed in as an argument.";
		case /*STATUS_FLOATED_SECTION*/(DWORD)0xC019004BL: return "I/O was attempted on a section object that has been floated as a result of a transaction ending.  There is no valid data.";
		case /*STATUS_CANNOT_ACCEPT_TRANSACTED_WORK*/(DWORD)0xC019004CL: return "The transactional resource manager cannot currently accept transacted work due to a transient condition such as low resources.";
		case /*STATUS_CANNOT_ABORT_TRANSACTIONS*/(DWORD)0xC019004DL: return "The transactional resource manager had too many tranactions outstanding that could not be aborted.  The transactional resource manger has been shut down.";
		case /*STATUS_TRANSACTION_NOT_FOUND*/(DWORD)0xC019004EL: return "The specified Transaction was unable to be opened, because it was not found.";
		case /*STATUS_RESOURCEMANAGER_NOT_FOUND*/(DWORD)0xC019004FL: return "The specified ResourceManager was unable to be opened, because it was not found.";
		case /*STATUS_ENLISTMENT_NOT_FOUND*/(DWORD)0xC0190050L: return "The specified Enlistment was unable to be opened, because it was not found.";
		case /*STATUS_TRANSACTIONMANAGER_NOT_FOUND*/(DWORD)0xC0190051L: return "The specified TransactionManager was unable to be opened, because it was not found.";
		case /*STATUS_TRANSACTIONMANAGER_NOT_ONLINE*/(DWORD)0xC0190052L: return "The specified ResourceManager was unable to create an enlistment, because its associated TransactionManager is not online.";
		case /*STATUS_TRANSACTIONMANAGER_RECOVERY_NAME_COLLISION*/(DWORD)0xC0190053L: return "The specified TransactionManager was unable to create the objects contained in its logfile in the Ob namespace.  Therefore, the TransactionManager was unable to recover.";
		case /*STATUS_TRANSACTION_NOT_ROOT*/(DWORD)0xC0190054L: return "The call to create a superior Enlistment on this Transaction object could not be completed, because the Transaction object specified for the enlistment is a subordinate branch of the Transaction.  Only the root of the Transactoin can be enlisted on as a superior.";
		case /*STATUS_TRANSACTION_OBJECT_EXPIRED*/(DWORD)0xC0190055L: return "Because the associated transaction manager or resource manager has been closed, the handle is no longer valid.";
		case /*STATUS_COMPRESSION_NOT_ALLOWED_IN_TRANSACTION*/(DWORD)0xC0190056L: return "The compression operation could not be completed because a transaction is active on the file.";
		case /*STATUS_TRANSACTION_RESPONSE_NOT_ENLISTED*/(DWORD)0xC0190057L: return "The specified operation could not be performed on this Superior enlistment, because the enlistment was not created with the corresponding completion response in the NotificationMask.";
		case /*STATUS_TRANSACTION_RECORD_TOO_LONG*/(DWORD)0xC0190058L: return "The specified operation could not be performed, because the record that would be logged was too long.  This can occur because of two conditions:";
		case /*STATUS_NO_LINK_TRACKING_IN_TRANSACTION*/(DWORD)0xC0190059L: return "The link tracking operation could not be completed because a transaction is active.";
		case /*STATUS_OPERATION_NOT_SUPPORTED_IN_TRANSACTION*/(DWORD)0xC019005AL: return "This operation cannot be performed in a transaction.";
		case /*STATUS_TRANSACTION_INTEGRITY_VIOLATED*/(DWORD)0xC019005BL: return "The kernel transaction manager had to abort or forget the transaction because it blocked forward progress.";
		case /*STATUS_LOG_SECTOR_INVALID*/(DWORD)0xC01A0001L: return "CLFS (common log file system) error values Log service found an invalid log sector.";
		case /*STATUS_LOG_SECTOR_PARITY_INVALID*/(DWORD)0xC01A0002L: return "Log service encountered a log sector with invalid block parity.";
		case /*STATUS_LOG_SECTOR_REMAPPED*/(DWORD)0xC01A0003L: return "Log service encountered a remapped log sector.";
		case /*STATUS_LOG_BLOCK_INCOMPLETE*/(DWORD)0xC01A0004L: return "Log service encountered a partial or incomplete log block.";
		case /*STATUS_LOG_INVALID_RANGE*/(DWORD)0xC01A0005L: return "Log service encountered an attempt access data outside the active log range.";
		case /*STATUS_LOG_BLOCKS_EXHAUSTED*/(DWORD)0xC01A0006L: return "Log service user log marshalling buffers are exhausted.";
		case /*STATUS_LOG_READ_CONTEXT_INVALID*/(DWORD)0xC01A0007L: return "Log service encountered an attempt read from a marshalling area with an invalid read context.";
		case /*STATUS_LOG_RESTART_INVALID*/(DWORD)0xC01A0008L: return "Log service encountered an invalid log restart area.";
		case /*STATUS_LOG_BLOCK_VERSION*/(DWORD)0xC01A0009L: return "Log service encountered an invalid log block version.";
		case /*STATUS_LOG_BLOCK_INVALID*/(DWORD)0xC01A000AL: return "Log service encountered an invalid log block.";
		case /*STATUS_LOG_READ_MODE_INVALID*/(DWORD)0xC01A000BL: return "Log service encountered an attempt to read the log with an invalid read mode.";
		case /*STATUS_LOG_NO_RESTART*/(DWORD)0x401A000CL: return "Log service encountered a log stream with no restart area.";
		case /*STATUS_LOG_METADATA_CORRUPT*/(DWORD)0xC01A000DL: return "Log service encountered a corrupted metadata file.";
		case /*STATUS_LOG_METADATA_INVALID*/(DWORD)0xC01A000EL: return "Log service encountered a metadata file that could not be created by the log file system.";
		case /*STATUS_LOG_METADATA_INCONSISTENT*/(DWORD)0xC01A000FL: return "Log service encountered a metadata file with inconsistent data.";
		case /*STATUS_LOG_RESERVATION_INVALID*/(DWORD)0xC01A0010L: return "Log service encountered an attempt to erroneously allocate or dispose reservation space.";
		case /*STATUS_LOG_CANT_DELETE*/(DWORD)0xC01A0011L: return "Log service cannot delete log file or file system container.";
		case /*STATUS_LOG_CONTAINER_LIMIT_EXCEEDED*/(DWORD)0xC01A0012L: return "Log service has reached the maximum allowable containers allocated to a log file.";
		case /*STATUS_LOG_START_OF_LOG*/(DWORD)0xC01A0013L: return "Log service has attempted to read or write backwards past the start of the log.";
		case /*STATUS_LOG_POLICY_ALREADY_INSTALLED*/(DWORD)0xC01A0014L: return "Log policy could not be installed because a policy of the same type is already present.";
		case /*STATUS_LOG_POLICY_NOT_INSTALLED*/(DWORD)0xC01A0015L: return "Log policy in question was not installed at the time of the request.";
		case /*STATUS_LOG_POLICY_INVALID*/(DWORD)0xC01A0016L: return "The installed set of policies on the log is invalid.";
		case /*STATUS_LOG_POLICY_CONFLICT*/(DWORD)0xC01A0017L: return "A policy on the log in question prevented the operation from completing.";
		case /*STATUS_LOG_PINNED_ARCHIVE_TAIL*/(DWORD)0xC01A0018L: return "Log space cannot be reclaimed because the log is pinned by the archive tail.";
		case /*STATUS_LOG_RECORD_NONEXISTENT*/(DWORD)0xC01A0019L: return "Log record is not a record in the log file.";
		case /*STATUS_LOG_RECORDS_RESERVED_INVALID*/(DWORD)0xC01A001AL: return "Number of reserved log records or the adjustment of the number of reserved log records is invalid.";
		case /*STATUS_LOG_SPACE_RESERVED_INVALID*/(DWORD)0xC01A001BL: return "Reserved log space or the adjustment of the log space is invalid.";
		case /*STATUS_LOG_TAIL_INVALID*/(DWORD)0xC01A001CL: return "A new or existing archive tail or base of the active log is invalid.";
		case /*STATUS_LOG_FULL*/(DWORD)0xC01A001DL: return "Log space is exhausted.";
		case /*STATUS_LOG_MULTIPLEXED*/(DWORD)0xC01A001EL: return "Log is multiplexed, no direct writes to the physical log is allowed.";
		case /*STATUS_LOG_DEDICATED*/(DWORD)0xC01A001FL: return "The operation failed because the log is a dedicated log.";
		case /*STATUS_LOG_ARCHIVE_NOT_IN_PROGRESS*/(DWORD)0xC01A0020L: return "The operation requires an archive context.";
		case /*STATUS_LOG_ARCHIVE_IN_PROGRESS*/(DWORD)0xC01A0021L: return "Log archival is in progress.";
		case /*STATUS_LOG_EPHEMERAL*/(DWORD)0xC01A0022L: return "The operation requires a non-ephemeral log, but the log is ephemeral.";
		case /*STATUS_LOG_NOT_ENOUGH_CONTAINERS*/(DWORD)0xC01A0023L: return "The log must have at least two containers before it can be read from or written to.";
		case /*STATUS_LOG_CLIENT_ALREADY_REGISTERED*/(DWORD)0xC01A0024L: return "A log client has already registered on the stream.";
		case /*STATUS_LOG_CLIENT_NOT_REGISTERED*/(DWORD)0xC01A0025L: return "A log client has not been registered on the stream.";
		case /*STATUS_LOG_FULL_HANDLER_IN_PROGRESS*/(DWORD)0xC01A0026L: return "A request has already been made to handle the log full condition.";
		case /*STATUS_LOG_CONTAINER_READ_FAILED*/(DWORD)0xC01A0027L: return "Log service enountered an error when attempting to read from a log container.";
		case /*STATUS_LOG_CONTAINER_WRITE_FAILED*/(DWORD)0xC01A0028L: return "Log service enountered an error when attempting to write to a log container.";
		case /*STATUS_LOG_CONTAINER_OPEN_FAILED*/(DWORD)0xC01A0029L: return "Log service enountered an error when attempting open a log container.";
		case /*STATUS_LOG_CONTAINER_STATE_INVALID*/(DWORD)0xC01A002AL: return "Log service enountered an invalid container state when attempting a requested action.";
		case /*STATUS_LOG_STATE_INVALID*/(DWORD)0xC01A002BL: return "Log service is not in the correct state to perform a requested action.";
		case /*STATUS_LOG_PINNED*/(DWORD)0xC01A002CL: return "Log space cannot be reclaimed because the log is pinned.";
		case /*STATUS_LOG_METADATA_FLUSH_FAILED*/(DWORD)0xC01A002DL: return "Log metadata flush failed.";
		case /*STATUS_LOG_INCONSISTENT_SECURITY*/(DWORD)0xC01A002EL: return "Security on the log and its containers is inconsistent.";
		case /*STATUS_LOG_APPENDED_FLUSH_FAILED*/(DWORD)0xC01A002FL: return "Records were appended to the log or reservation changes were made, but the log could not be flushed.";
		case /*STATUS_LOG_PINNED_RESERVATION*/(DWORD)0xC01A0030L: return "The log is pinned due to reservation consuming most of the log space.  Free some reserved records to make space available.";
		case /*STATUS_VIDEO_HUNG_DISPLAY_DRIVER_THREAD*/(DWORD)0xC01B00EAL: return "XDDM Video Facility Error codes (videoprt.sys) {Display Driver Stopped Responding} The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to upload data about this failure to Microsoft.";
		case /*STATUS_VIDEO_HUNG_DISPLAY_DRIVER_THREAD_RECOVERED*/(DWORD)0x801B00EBL: return "{Display Driver Stopped Responding and recovered} The %hs display driver has stopped working normally. The recovery had been performed.";
		case /*STATUS_VIDEO_DRIVER_DEBUG_REPORT_REQUEST*/(DWORD)0x401B00ECL: return "{Display Driver Recovered From Failure} The %hs display driver has detected and recovered from a failure. Some graphical operations may have failed. The next time you reboot the machine a dialog will be displayed giving you a chance to upload data about this failure to Microsoft.";
		case /*STATUS_MONITOR_NO_DESCRIPTOR*/(DWORD)0xC01D0001L: return "Monitor Facility Error codes (monitor.sys) Monitor descriptor could not be obtained.";
		case /*STATUS_MONITOR_UNKNOWN_DESCRIPTOR_FORMAT*/(DWORD)0xC01D0002L: return "Format of the obtained monitor descriptor is not supported by this release.";
		case /*STATUS_MONITOR_INVALID_DESCRIPTOR_CHECKSUM*/(DWORD)0xC01D0003L: return "Checksum of the obtained monitor descriptor is invalid.";
		case /*STATUS_MONITOR_INVALID_STANDARD_TIMING_BLOCK*/(DWORD)0xC01D0004L: return "Monitor descriptor contains an invalid standard timing block.";
		case /*STATUS_MONITOR_WMI_DATABLOCK_REGISTRATION_FAILED*/(DWORD)0xC01D0005L: return "WMI data block registration failed for one of the MSMonitorClass WMI subclasses.";
		case /*STATUS_MONITOR_INVALID_SERIAL_NUMBER_MONDSC_BLOCK*/(DWORD)0xC01D0006L: return "Provided monitor descriptor block is either corrupted or does not contain monitor's detailed serial number.";
		case /*STATUS_MONITOR_INVALID_USER_FRIENDLY_MONDSC_BLOCK*/(DWORD)0xC01D0007L: return "Provided monitor descriptor block is either corrupted or does not contain monitor's user friendly name.";
		case /*STATUS_MONITOR_NO_MORE_DESCRIPTOR_DATA*/(DWORD)0xC01D0008L: return "There is no monitor descriptor data at the specified (offset, size) region.";
		case /*STATUS_MONITOR_INVALID_DETAILED_TIMING_BLOCK*/(DWORD)0xC01D0009L: return "Monitor descriptor contains an invalid detailed timing block.";
		case /*STATUS_GRAPHICS_NOT_EXCLUSIVE_MODE_OWNER*/(DWORD)0xC01E0000L: return "Graphics Facility Error codes (dxg.sys, dxgkrnl.sys)   Common Windows Graphics Kernel Subsystem status codes {0x0000..0x00ff} Exclusive mode ownership is needed to create unmanaged primary allocation.";
		case /*STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER*/(DWORD)0xC01E0001L: return "The driver needs more DMA buffer space in order to complete the requested operation.";
		case /*STATUS_GRAPHICS_INVALID_DISPLAY_ADAPTER*/(DWORD)0xC01E0002L: return "Specified display adapter handle is invalid.";
		case /*STATUS_GRAPHICS_ADAPTER_WAS_RESET*/(DWORD)0xC01E0003L: return "Specified display adapter and all of its state has been reset.";
		case /*STATUS_GRAPHICS_INVALID_DRIVER_MODEL*/(DWORD)0xC01E0004L: return "The driver stack doesn't match the expected driver model.";
		case /*STATUS_GRAPHICS_PRESENT_MODE_CHANGED*/(DWORD)0xC01E0005L: return "Present happened but ended up into the changed desktop mode";
		case /*STATUS_GRAPHICS_PRESENT_OCCLUDED*/(DWORD)0xC01E0006L: return "Nothing to present due to desktop occlusion";
		case /*STATUS_GRAPHICS_PRESENT_DENIED*/(DWORD)0xC01E0007L: return "Not able to present due to denial of desktop access";
		case /*STATUS_GRAPHICS_CANNOTCOLORCONVERT*/(DWORD)0xC01E0008L: return "Not able to present with color convertion";
		case /*STATUS_GRAPHICS_DRIVER_MISMATCH*/(DWORD)0xC01E0009L: return "The kernel driver detected a version mismatch between it and the user mode driver.";
		case /*STATUS_GRAPHICS_PARTIAL_DATA_POPULATED*/(DWORD)0x401E000AL: return "Specified buffer is not big enough to contain entire requested dataset. Partial data populated upto the size of the buffer. Caller needs to provide buffer of size as specified in the partially populated buffer's content (interface specific).";
		case /*STATUS_GRAPHICS_NO_VIDEO_MEMORY*/(DWORD)0xC01E0100L: return "Video Memory Manager (VidMM) specific status codes {0x0100..0x01ff} Not enough video memory available to complete the operation.";
		case /*STATUS_GRAPHICS_CANT_LOCK_MEMORY*/(DWORD)0xC01E0101L: return "Couldn't probe and lock the underlying memory of an allocation.";
		case /*STATUS_GRAPHICS_ALLOCATION_BUSY*/(DWORD)0xC01E0102L: return "The allocation is currently busy.";
		case /*STATUS_GRAPHICS_TOO_MANY_REFERENCES*/(DWORD)0xC01E0103L: return "An object being referenced has already reached the maximum reference count and can't be referenced any further.";
		case /*STATUS_GRAPHICS_TRY_AGAIN_LATER*/(DWORD)0xC01E0104L: return "A problem couldn't be solved due to some currently existing condition. The problem should be tried again later.";
		case /*STATUS_GRAPHICS_TRY_AGAIN_NOW*/(DWORD)0xC01E0105L: return "A problem couldn't be solved due to some currently existing condition. The problem should be tried again immediately.";
		case /*STATUS_GRAPHICS_ALLOCATION_INVALID*/(DWORD)0xC01E0106L: return "The allocation is invalid.";
		case /*STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNAVAILABLE*/(DWORD)0xC01E0107L: return "No more unswizzling aperture are currently available.";
		case /*STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNSUPPORTED*/(DWORD)0xC01E0108L: return "The current allocation can't be unswizzled by an aperture.";
		case /*STATUS_GRAPHICS_CANT_EVICT_PINNED_ALLOCATION*/(DWORD)0xC01E0109L: return "The request failed because a pinned allocation can't be evicted.";
		case /*STATUS_GRAPHICS_INVALID_ALLOCATION_USAGE*/(DWORD)0xC01E0110L: return "The allocation can't be used from it's current segment location for the specified operation.";
		case /*STATUS_GRAPHICS_CANT_RENDER_LOCKED_ALLOCATION*/(DWORD)0xC01E0111L: return "A locked allocation can't be used in the current command buffer.";
		case /*STATUS_GRAPHICS_ALLOCATION_CLOSED*/(DWORD)0xC01E0112L: return "The allocation being referenced has been closed permanently.";
		case /*STATUS_GRAPHICS_INVALID_ALLOCATION_INSTANCE*/(DWORD)0xC01E0113L: return "An invalid allocation instance is being referenced.";
		case /*STATUS_GRAPHICS_INVALID_ALLOCATION_HANDLE*/(DWORD)0xC01E0114L: return "An invalid allocation handle is being referenced.";
		case /*STATUS_GRAPHICS_WRONG_ALLOCATION_DEVICE*/(DWORD)0xC01E0115L: return "The allocation being referenced doesn't belong to the current device.";
		case /*STATUS_GRAPHICS_ALLOCATION_CONTENT_LOST*/(DWORD)0xC01E0116L: return "The specified allocation lost its content.";
		case /*STATUS_GRAPHICS_GPU_EXCEPTION_ON_DEVICE*/(DWORD)0xC01E0200L: return "Video GPU Scheduler (VidSch) specific status codes {0x0200..0x02ff} GPU exception is detected on the given device. The device is not able to be scheduled.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY*/(DWORD)0xC01E0300L: return "Video Present Network Management (VidPNMgr) specific status codes {0x0300..0x03ff} Specified VidPN topology is invalid.";
		case /*STATUS_GRAPHICS_VIDPN_TOPOLOGY_NOT_SUPPORTED*/(DWORD)0xC01E0301L: return "Specified VidPN topology is valid but is not supported by this model of the display adapter.";
		case /*STATUS_GRAPHICS_VIDPN_TOPOLOGY_CURRENTLY_NOT_SUPPORTED*/(DWORD)0xC01E0302L: return "Specified VidPN topology is valid but is not supported by the display adapter at this time, due to current allocation of its resources.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN*/(DWORD)0xC01E0303L: return "Specified VidPN handle is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE*/(DWORD)0xC01E0304L: return "Specified video present source is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET*/(DWORD)0xC01E0305L: return "Specified video present target is invalid.";
		case /*STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED*/(DWORD)0xC01E0306L: return "Specified VidPN modality is not supported (e.g. at least two of the pinned modes are not cofunctional).";
		case /*STATUS_GRAPHICS_MODE_NOT_PINNED*/(DWORD)0x401E0307L: return "No mode is pinned on the specified VidPN source/target.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_SOURCEMODESET*/(DWORD)0xC01E0308L: return "Specified VidPN source mode set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_TARGETMODESET*/(DWORD)0xC01E0309L: return "Specified VidPN target mode set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_FREQUENCY*/(DWORD)0xC01E030AL: return "Specified video signal frequency is invalid.";
		case /*STATUS_GRAPHICS_INVALID_ACTIVE_REGION*/(DWORD)0xC01E030BL: return "Specified video signal active region is invalid.";
		case /*STATUS_GRAPHICS_INVALID_TOTAL_REGION*/(DWORD)0xC01E030CL: return "Specified video signal total region is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE*/(DWORD)0xC01E0310L: return "Specified video present source mode is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET_MODE*/(DWORD)0xC01E0311L: return "Specified video present target mode is invalid.";
		case /*STATUS_GRAPHICS_PINNED_MODE_MUST_REMAIN_IN_SET*/(DWORD)0xC01E0312L: return "Pinned mode must remain in the set on VidPN's cofunctional modality enumeration.";
		case /*STATUS_GRAPHICS_PATH_ALREADY_IN_TOPOLOGY*/(DWORD)0xC01E0313L: return "Specified video present path is already in VidPN's topology.";
		case /*STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET*/(DWORD)0xC01E0314L: return "Specified mode is already in the mode set.";
		case /*STATUS_GRAPHICS_INVALID_VIDEOPRESENTSOURCESET*/(DWORD)0xC01E0315L: return "Specified video present source set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VIDEOPRESENTTARGETSET*/(DWORD)0xC01E0316L: return "Specified video present target set is invalid.";
		case /*STATUS_GRAPHICS_SOURCE_ALREADY_IN_SET*/(DWORD)0xC01E0317L: return "Specified video present source is already in the video present source set.";
		case /*STATUS_GRAPHICS_TARGET_ALREADY_IN_SET*/(DWORD)0xC01E0318L: return "Specified video present target is already in the video present target set.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_PRESENT_PATH*/(DWORD)0xC01E0319L: return "Specified VidPN present path is invalid.";
		case /*STATUS_GRAPHICS_NO_RECOMMENDED_VIDPN_TOPOLOGY*/(DWORD)0xC01E031AL: return "Miniport has no recommendation for augmentation of the specified VidPN's topology.";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_FREQUENCYRANGESET*/(DWORD)0xC01E031BL: return "Specified monitor frequency range set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_FREQUENCYRANGE*/(DWORD)0xC01E031CL: return "Specified monitor frequency range is invalid.";
		case /*STATUS_GRAPHICS_FREQUENCYRANGE_NOT_IN_SET*/(DWORD)0xC01E031DL: return "Specified frequency range is not in the specified monitor frequency range set.";
		case /*STATUS_GRAPHICS_NO_PREFERRED_MODE*/(DWORD)0x401E031EL: return "Specified mode set does not specify preference for one of its modes.";
		case /*STATUS_GRAPHICS_FREQUENCYRANGE_ALREADY_IN_SET*/(DWORD)0xC01E031FL: return "Specified frequency range is already in the specified monitor frequency range set.";
		case /*STATUS_GRAPHICS_STALE_MODESET*/(DWORD)0xC01E0320L: return "Specified mode set is stale. Please reacquire the new mode set.";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_SOURCEMODESET*/(DWORD)0xC01E0321L: return "Specified monitor source mode set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_SOURCE_MODE*/(DWORD)0xC01E0322L: return "Specified monitor source mode is invalid.";
		case /*STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN*/(DWORD)0xC01E0323L: return "Miniport does not have any recommendation regarding the request to provide a functional VidPN given the current display adapter configuration.";
		case /*STATUS_GRAPHICS_MODE_ID_MUST_BE_UNIQUE*/(DWORD)0xC01E0324L: return "ID of the specified mode is already used by another mode in the set.";
		case /*STATUS_GRAPHICS_EMPTY_ADAPTER_MONITOR_MODE_SUPPORT_INTERSECTION*/(DWORD)0xC01E0325L: return "System failed to determine a mode that is supported by both the display adapter and the monitor connected to it.";
		case /*STATUS_GRAPHICS_VIDEO_PRESENT_TARGETS_LESS_THAN_SOURCES*/(DWORD)0xC01E0326L: return "Number of video present targets must be greater than or equal to the number of video present sources.";
		case /*STATUS_GRAPHICS_PATH_NOT_IN_TOPOLOGY*/(DWORD)0xC01E0327L: return "Specified present path is not in VidPN's topology.";
		case /*STATUS_GRAPHICS_ADAPTER_MUST_HAVE_AT_LEAST_ONE_SOURCE*/(DWORD)0xC01E0328L: return "Display adapter must have at least one video present source.";
		case /*STATUS_GRAPHICS_ADAPTER_MUST_HAVE_AT_LEAST_ONE_TARGET*/(DWORD)0xC01E0329L: return "Display adapter must have at least one video present target.";
		case /*STATUS_GRAPHICS_INVALID_MONITORDESCRIPTORSET*/(DWORD)0xC01E032AL: return "Specified monitor descriptor set is invalid.";
		case /*STATUS_GRAPHICS_INVALID_MONITORDESCRIPTOR*/(DWORD)0xC01E032BL: return "Specified monitor descriptor is invalid.";
		case /*STATUS_GRAPHICS_MONITORDESCRIPTOR_NOT_IN_SET*/(DWORD)0xC01E032CL: return "Specified descriptor is not in the specified monitor descriptor set.";
		case /*STATUS_GRAPHICS_MONITORDESCRIPTOR_ALREADY_IN_SET*/(DWORD)0xC01E032DL: return "Specified descriptor is already in the specified monitor descriptor set.";
		case /*STATUS_GRAPHICS_MONITORDESCRIPTOR_ID_MUST_BE_UNIQUE*/(DWORD)0xC01E032EL: return "ID of the specified monitor descriptor is already used by another descriptor in the set.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_TARGET_SUBSET_TYPE*/(DWORD)0xC01E032FL: return "Specified video present target subset type is invalid.";
		case /*STATUS_GRAPHICS_RESOURCES_NOT_RELATED*/(DWORD)0xC01E0330L: return "Two or more of the specified resources are not related to each other, as defined by the interface semantics.";
		case /*STATUS_GRAPHICS_SOURCE_ID_MUST_BE_UNIQUE*/(DWORD)0xC01E0331L: return "ID of the specified video present source is already used by another source in the set.";
		case /*STATUS_GRAPHICS_TARGET_ID_MUST_BE_UNIQUE*/(DWORD)0xC01E0332L: return "ID of the specified video present target is already used by another target in the set.";
		case /*STATUS_GRAPHICS_NO_AVAILABLE_VIDPN_TARGET*/(DWORD)0xC01E0333L: return "Specified VidPN source cannot be used because there is no available VidPN target to connect it to.";
		case /*STATUS_GRAPHICS_MONITOR_COULD_NOT_BE_ASSOCIATED_WITH_ADAPTER*/(DWORD)0xC01E0334L: return "Newly arrived monitor could not be associated with a display adapter.";
		case /*STATUS_GRAPHICS_NO_VIDPNMGR*/(DWORD)0xC01E0335L: return "Display adapter in question does not have an associated VidPN manager.";
		case /*STATUS_GRAPHICS_NO_ACTIVE_VIDPN*/(DWORD)0xC01E0336L: return "VidPN manager of the display adapter in question does not have an active VidPN.";
		case /*STATUS_GRAPHICS_STALE_VIDPN_TOPOLOGY*/(DWORD)0xC01E0337L: return "Specified VidPN topology is stale. Please reacquire the new topology.";
		case /*STATUS_GRAPHICS_MONITOR_NOT_CONNECTED*/(DWORD)0xC01E0338L: return "There is no monitor connected on the specified video present target.";
		case /*STATUS_GRAPHICS_SOURCE_NOT_IN_TOPOLOGY*/(DWORD)0xC01E0339L: return "Specified source is not part of the specified VidPN's topology.";
		case /*STATUS_GRAPHICS_INVALID_PRIMARYSURFACE_SIZE*/(DWORD)0xC01E033AL: return "Specified primary surface size is invalid.";
		case /*STATUS_GRAPHICS_INVALID_VISIBLEREGION_SIZE*/(DWORD)0xC01E033BL: return "Specified visible region size is invalid.";
		case /*STATUS_GRAPHICS_INVALID_STRIDE*/(DWORD)0xC01E033CL: return "Specified stride is invalid.";
		case /*STATUS_GRAPHICS_INVALID_PIXELFORMAT*/(DWORD)0xC01E033DL: return "Specified pixel format is invalid.";
		case /*STATUS_GRAPHICS_INVALID_COLORBASIS*/(DWORD)0xC01E033EL: return "Specified color basis is invalid.";
		case /*STATUS_GRAPHICS_INVALID_PIXELVALUEACCESSMODE*/(DWORD)0xC01E033FL: return "Specified pixel value access mode is invalid.";
		case /*STATUS_GRAPHICS_TARGET_NOT_IN_TOPOLOGY*/(DWORD)0xC01E0340L: return "Specified target is not part of the specified VidPN's topology.";
		case /*STATUS_GRAPHICS_NO_DISPLAY_MODE_MANAGEMENT_SUPPORT*/(DWORD)0xC01E0341L: return "Failed to acquire display mode management interface.";
		case /*STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE*/(DWORD)0xC01E0342L: return "Specified VidPN source is already owned by a DMM client and cannot be used until that client releases it.";
		case /*STATUS_GRAPHICS_CANT_ACCESS_ACTIVE_VIDPN*/(DWORD)0xC01E0343L: return "Specified VidPN is active and cannot be accessed.";
		case /*STATUS_GRAPHICS_INVALID_PATH_IMPORTANCE_ORDINAL*/(DWORD)0xC01E0344L: return "Specified VidPN present path importance ordinal is invalid.";
		case /*STATUS_GRAPHICS_INVALID_PATH_CONTENT_GEOMETRY_TRANSFORMATION*/(DWORD)0xC01E0345L: return "Specified VidPN present path content geometry transformation is invalid.";
		case /*STATUS_GRAPHICS_PATH_CONTENT_GEOMETRY_TRANSFORMATION_NOT_SUPPORTED*/(DWORD)0xC01E0346L: return "Specified content geometry transformation is not supported on the respective VidPN present path.";
		case /*STATUS_GRAPHICS_INVALID_GAMMA_RAMP*/(DWORD)0xC01E0347L: return "Specified gamma ramp is invalid.";
		case /*STATUS_GRAPHICS_GAMMA_RAMP_NOT_SUPPORTED*/(DWORD)0xC01E0348L: return "Specified gamma ramp is not supported on the respective VidPN present path.";
		case /*STATUS_GRAPHICS_MULTISAMPLING_NOT_SUPPORTED*/(DWORD)0xC01E0349L: return "Multi-sampling is not supported on the respective VidPN present path.";
		case /*STATUS_GRAPHICS_MODE_NOT_IN_MODESET*/(DWORD)0xC01E034AL: return "Specified mode is not in the specified mode set.";
		case /*STATUS_GRAPHICS_DATASET_IS_EMPTY*/(DWORD)0x401E034BL: return "Specified data set (e.g. mode set, frequency range set, descriptor set, topology, etc.) is empty.";
		case /*STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET*/(DWORD)0x401E034CL: return "Specified data set (e.g. mode set, frequency range set, descriptor set, topology, etc.) does not contain any more elements.";
		case /*STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY_RECOMMENDATION_REASON*/(DWORD)0xC01E034DL: return "Specified VidPN topology recommendation reason is invalid.";
		case /*STATUS_GRAPHICS_INVALID_PATH_CONTENT_TYPE*/(DWORD)0xC01E034EL: return "Specified VidPN present path content type is invalid.";
		case /*STATUS_GRAPHICS_INVALID_COPYPROTECTION_TYPE*/(DWORD)0xC01E034FL: return "Specified VidPN present path copy protection type is invalid.";
		case /*STATUS_GRAPHICS_UNASSIGNED_MODESET_ALREADY_EXISTS*/(DWORD)0xC01E0350L: return "No more than one unassigned mode set can exist at any given time for a given VidPN source/target.";
		case /*STATUS_GRAPHICS_PATH_CONTENT_GEOMETRY_TRANSFORMATION_NOT_PINNED*/(DWORD)0x401E0351L: return "Specified content transformation is not pinned on the specified VidPN present path.";
		case /*STATUS_GRAPHICS_INVALID_SCANLINE_ORDERING*/(DWORD)0xC01E0352L: return "Specified scanline ordering type is invalid.";
		case /*STATUS_GRAPHICS_TOPOLOGY_CHANGES_NOT_ALLOWED*/(DWORD)0xC01E0353L: return "Topology changes are not allowed for the specified VidPN.";
		case /*STATUS_GRAPHICS_NO_AVAILABLE_IMPORTANCE_ORDINALS*/(DWORD)0xC01E0354L: return "All available importance ordinals are already used in specified topology.";
		case /*STATUS_GRAPHICS_INCOMPATIBLE_PRIVATE_FORMAT*/(DWORD)0xC01E0355L: return "Specified primary surface has a different private format attribute than the current primary surface";
		case /*STATUS_GRAPHICS_INVALID_MODE_PRUNING_ALGORITHM*/(DWORD)0xC01E0356L: return "Specified mode pruning algorithm is invalid";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_CAPABILITY_ORIGIN*/(DWORD)0xC01E0357L: return "Specified monitor capability origin is invalid.";
		case /*STATUS_GRAPHICS_INVALID_MONITOR_FREQUENCYRANGE_CONSTRAINT*/(DWORD)0xC01E0358L: return "Specified monitor frequency range constraint is invalid.";
		case /*STATUS_GRAPHICS_MAX_NUM_PATHS_REACHED*/(DWORD)0xC01E0359L: return "Maximum supported number of present paths has been reached.";
		case /*STATUS_GRAPHICS_CANCEL_VIDPN_TOPOLOGY_AUGMENTATION*/(DWORD)0xC01E035AL: return "Miniport requested that augmentation be cancelled for the specified source of the specified VidPN's topology.";
		case /*STATUS_GRAPHICS_INVALID_CLIENT_TYPE*/(DWORD)0xC01E035BL: return "Specified client type was not recognized.";
		case /*STATUS_GRAPHICS_CLIENTVIDPN_NOT_SET*/(DWORD)0xC01E035CL: return "Client VidPN is not set on this adapter (e.g. no user mode initiated mode changes took place on this adapter yet).";
		case /*STATUS_GRAPHICS_SPECIFIED_CHILD_ALREADY_CONNECTED*/(DWORD)0xC01E0400L: return "Port specific status codes {0x0400..0x04ff} Specified display adapter child device already has an external device connected to it.";
		case /*STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED*/(DWORD)0xC01E0401L: return "Specified display adapter child device does not support descriptor exposure.";
		case /*STATUS_GRAPHICS_UNKNOWN_CHILD_STATUS*/(DWORD)0x401E042FL: return "Child device presence was not reliably detected.";
		case /*STATUS_GRAPHICS_NOT_A_LINKED_ADAPTER*/(DWORD)0xC01E0430L: return "The display adapter is not linked to any other adapters.";
		case /*STATUS_GRAPHICS_LEADLINK_NOT_ENUMERATED*/(DWORD)0xC01E0431L: return "Lead adapter in a linked configuration was not enumerated yet.";
		case /*STATUS_GRAPHICS_CHAINLINKS_NOT_ENUMERATED*/(DWORD)0xC01E0432L: return "Some chain adapters in a linked configuration were not enumerated yet.";
		case /*STATUS_GRAPHICS_ADAPTER_CHAIN_NOT_READY*/(DWORD)0xC01E0433L: return "The chain of linked adapters is not ready to start because of an unknown failure.";
		case /*STATUS_GRAPHICS_CHAINLINKS_NOT_STARTED*/(DWORD)0xC01E0434L: return "An attempt was made to start a lead link display adapter when the chain links were not started yet.";
		case /*STATUS_GRAPHICS_CHAINLINKS_NOT_POWERED_ON*/(DWORD)0xC01E0435L: return "An attempt was made to power up a lead link display adapter when the chain links were powered down.";
		case /*STATUS_GRAPHICS_INCONSISTENT_DEVICE_LINK_STATE*/(DWORD)0xC01E0436L: return "The adapter link was found to be in an inconsistent state. Not all adapters are in an expected PNP/Power state.";
		case /*STATUS_GRAPHICS_LEADLINK_START_DEFERRED*/(DWORD)0x401E0437L: return "Starting the leadlink adapter has been deferred temporarily.";
		case /*STATUS_GRAPHICS_NOT_POST_DEVICE_DRIVER*/(DWORD)0xC01E0438L: return "The driver trying to start is not the same as the driver for the POSTed display adapter.";
		case /*STATUS_GRAPHICS_POLLING_TOO_FREQUENTLY*/(DWORD)0x401E0439L: return "The display adapter is being polled for children too frequently at the same polling level.";
		case /*STATUS_GRAPHICS_START_DEFERRED*/(DWORD)0x401E043AL: return "Starting the adapter has been deferred temporarily.";
		case /*STATUS_GRAPHICS_ADAPTER_ACCESS_NOT_EXCLUDED*/(DWORD)0xC01E043BL: return "An operation is being attempted that requires the display adapter to be in a quiescent state.";
		case /*STATUS_GRAPHICS_OPM_NOT_SUPPORTED*/(DWORD)0xC01E0500L: return "OPM, PVP and UAB status codes {0x0500..0x057F} The driver does not support OPM.";
		case /*STATUS_GRAPHICS_COPP_NOT_SUPPORTED*/(DWORD)0xC01E0501L: return "The driver does not support COPP.";
		case /*STATUS_GRAPHICS_UAB_NOT_SUPPORTED*/(DWORD)0xC01E0502L: return "The driver does not support UAB.";
		case /*STATUS_GRAPHICS_OPM_INVALID_ENCRYPTED_PARAMETERS*/(DWORD)0xC01E0503L: return "The specified encrypted parameters are invalid.";
		case /*STATUS_GRAPHICS_OPM_NO_PROTECTED_OUTPUTS_EXIST*/(DWORD)0xC01E0505L: return "The GDI display device passed to this function does not have any active protected outputs.";
		case /*STATUS_GRAPHICS_OPM_INTERNAL_ERROR*/(DWORD)0xC01E050BL: return "An internal error caused an operation to fail.";
		case /*STATUS_GRAPHICS_OPM_INVALID_HANDLE*/(DWORD)0xC01E050CL: return "The function failed because the caller passed in an invalid OPM user mode handle.";
		case /*STATUS_GRAPHICS_PVP_INVALID_CERTIFICATE_LENGTH*/(DWORD)0xC01E050EL: return "A certificate could not be returned because the certificate buffer passed to the function was too small.";
		case /*STATUS_GRAPHICS_OPM_SPANNING_MODE_ENABLED*/(DWORD)0xC01E050FL: return "The DxgkDdiOpmCreateProtectedOutput function could not create a protected output because the Video Present Target is in spanning mode.";
		case /*STATUS_GRAPHICS_OPM_THEATER_MODE_ENABLED*/(DWORD)0xC01E0510L: return "The DxgkDdiOpmCreateProtectedOutput function could not create a protected output because the Video Present Target is in theater mode.";
		case /*STATUS_GRAPHICS_PVP_HFS_FAILED*/(DWORD)0xC01E0511L: return "The function failed because the display adapter's Hardware Functionality Scan failed to validate the graphics hardware.  ";
		case /*STATUS_GRAPHICS_OPM_INVALID_SRM*/(DWORD)0xC01E0512L: return "The HDCP System Renewability Message passed to this function did not comply with section 5 of the HDCP 1.1 specification.";
		case /*STATUS_GRAPHICS_OPM_OUTPUT_DOES_NOT_SUPPORT_HDCP*/(DWORD)0xC01E0513L: return "The protected output cannot enable the High-bandwidth Digital Content Protection (HDCP) System because it does not support HDCP.";
		case /*STATUS_GRAPHICS_OPM_OUTPUT_DOES_NOT_SUPPORT_ACP*/(DWORD)0xC01E0514L: return "The protected output cannot enable Analogue Copy Protection (ACP) because it does not support ACP.";
		case /*STATUS_GRAPHICS_OPM_OUTPUT_DOES_NOT_SUPPORT_CGMSA*/(DWORD)0xC01E0515L: return "The protected output cannot enable the Content Generation Management System Analogue (CGMS-A) protection technology because it does not support CGMS-A.";
		case /*STATUS_GRAPHICS_OPM_HDCP_SRM_NEVER_SET*/(DWORD)0xC01E0516L: return "The DxgkDdiOPMGetInformation function cannot return the version of the SRM being used because the application never successfully passed an SRM to the protected output.  ";
		case /*STATUS_GRAPHICS_OPM_RESOLUTION_TOO_HIGH*/(DWORD)0xC01E0517L: return "The DxgkDdiOPMConfigureProtectedOutput function cannot enable the specified output protection technology because the output's screen resolution is too high.  ";
		case /*STATUS_GRAPHICS_OPM_ALL_HDCP_HARDWARE_ALREADY_IN_USE*/(DWORD)0xC01E0518L: return "The DxgkDdiOPMConfigureProtectedOutput function cannot enable HDCP because the display adapter's HDCP hardware is already being used by other physical outputs.";
		case /*STATUS_GRAPHICS_OPM_PROTECTED_OUTPUT_NO_LONGER_EXISTS*/(DWORD)0xC01E051AL: return "The operating system asynchronously destroyed this OPM protected output because the operating system's state changed.  This error typically occurs because the monitor PDO associated with this protected output was removed, the monitor PDO associated with this protected output was stopped, or the protected output's session became a non-console session.";
		case /*STATUS_GRAPHICS_OPM_PROTECTED_OUTPUT_DOES_NOT_HAVE_COPP_SEMANTICS*/(DWORD)0xC01E051CL: return "Either the DxgkDdiOPMGetCOPPCompatibleInformation, DxgkDdiOPMGetInformation, or DxgkDdiOPMConfigureProtectedOutput function failed.  This error is only returned if a protected output has OPM semantics.  DxgkDdiOPMGetCOPPCompatibleInformation always returns this error if a protected output has OPM semantics.  DxgkDdiOPMGetInformation returns this error code if the caller requested COPP specific information.  DxgkDdiOPMConfigureProtectedOutput returns this error when the caller tries to use a COPP specific command.  ";
		case /*STATUS_GRAPHICS_OPM_INVALID_INFORMATION_REQUEST*/(DWORD)0xC01E051DL: return "The DxgkDdiOPMGetInformation and DxgkDdiOPMGetCOPPCompatibleInformation functions return this error code if the passed in sequence number is not the expected sequence number or the passed in OMAC value is invalid.";
		case /*STATUS_GRAPHICS_OPM_DRIVER_INTERNAL_ERROR*/(DWORD)0xC01E051EL: return "The function failed because an unexpected error occurred inside of a display driver.";
		case /*STATUS_GRAPHICS_OPM_PROTECTED_OUTPUT_DOES_NOT_HAVE_OPM_SEMANTICS*/(DWORD)0xC01E051FL: return "Either the DxgkDdiOPMGetCOPPCompatibleInformation, DxgkDdiOPMGetInformation, or DxgkDdiOPMConfigureProtectedOutput function failed.  This error is only returned if a protected output has COPP semantics.  DxgkDdiOPMGetCOPPCompatibleInformation returns this error code if the caller requested OPM specific information.  DxgkDdiOPMGetInformation always returns this error if a protected output has COPP semantics.  DxgkDdiOPMConfigureProtectedOutput returns this error when the caller tries to use an OPM specific command.  ";
		case /*STATUS_GRAPHICS_OPM_SIGNALING_NOT_SUPPORTED*/(DWORD)0xC01E0520L: return "The DxgkDdiOPMGetCOPPCompatibleInformation and DxgkDdiOPMConfigureProtectedOutput functions return this error if the display driver does not support the DXGKMDT_OPM_GET_ACP_AND_CGMSA_SIGNALING and DXGKMDT_OPM_SET_ACP_AND_CGMSA_SIGNALING GUIDs.";
		case /*STATUS_GRAPHICS_OPM_INVALID_CONFIGURATION_REQUEST*/(DWORD)0xC01E0521L: return "The DxgkDdiOPMConfigureProtectedOutput function returns this error code if the passed in sequence number is not the expected sequence number or the passed in OMAC value is invalid.";
		case /*STATUS_GRAPHICS_I2C_NOT_SUPPORTED*/(DWORD)0xC01E0580L: return "Monitor Configuration API status codes {0x0580..0x05DF} The monitor connected to the specified video output does not have an I2C bus.";
		case /*STATUS_GRAPHICS_I2C_DEVICE_DOES_NOT_EXIST*/(DWORD)0xC01E0581L: return "No device on the I2C bus has the specified address.";
		case /*STATUS_GRAPHICS_I2C_ERROR_TRANSMITTING_DATA*/(DWORD)0xC01E0582L: return "An error occurred while transmitting data to the device on the I2C bus.";
		case /*STATUS_GRAPHICS_I2C_ERROR_RECEIVING_DATA*/(DWORD)0xC01E0583L: return "An error occurred while receiving data from the device on the I2C bus.";
		case /*STATUS_GRAPHICS_DDCCI_VCP_NOT_SUPPORTED*/(DWORD)0xC01E0584L: return "The monitor does not support the specified VCP code.";
		case /*STATUS_GRAPHICS_DDCCI_INVALID_DATA*/(DWORD)0xC01E0585L: return "The data received from the monitor is invalid.";
		case /*STATUS_GRAPHICS_DDCCI_MONITOR_RETURNED_INVALID_TIMING_STATUS_BYTE*/(DWORD)0xC01E0586L: return "The function failed because a monitor returned an invalid Timing Status byte when the operating system used the DDC/CI Get Timing Report & Timing Message command to get a timing report from a monitor.";
		case /*STATUS_GRAPHICS_DDCCI_INVALID_CAPABILITIES_STRING*/(DWORD)0xC01E0587L: return "A monitor returned a DDC/CI capabilities string which did not comply with the ACCESS.bus 3.0, DDC/CI 1.1, or MCCS 2 Revision 1 specification.";
		case /*STATUS_GRAPHICS_MCA_INTERNAL_ERROR*/(DWORD)0xC01E0588L: return "An internal error caused an operation to fail.";
		case /*STATUS_GRAPHICS_DDCCI_INVALID_MESSAGE_COMMAND*/(DWORD)0xC01E0589L: return "An operation failed because a DDC/CI message had an invalid value in its command field.";
		case /*STATUS_GRAPHICS_DDCCI_INVALID_MESSAGE_LENGTH*/(DWORD)0xC01E058AL: return "An error occurred because the field length of a DDC/CI message contained an invalid value.  ";
		case /*STATUS_GRAPHICS_DDCCI_INVALID_MESSAGE_CHECKSUM*/(DWORD)0xC01E058BL: return "An error occurred because the checksum field in a DDC/CI message did not match the message's computed checksum value.  This error implies that the data was corrupted while it was being transmitted from a monitor to a computer.";
		case /*STATUS_GRAPHICS_INVALID_PHYSICAL_MONITOR_HANDLE*/(DWORD)0xC01E058CL: return "This function failed because an invalid monitor handle was passed to it.";
		case /*STATUS_GRAPHICS_MONITOR_NO_LONGER_EXISTS*/(DWORD)0xC01E058DL: return "The operating system asynchronously destroyed the monitor which corresponds to this handle because the operating system's state changed.  This error typically occurs because the monitor PDO associated with this handle was removed, the monitor PDO associated with this handle was stopped, or a display mode change occurred.  A display mode change occurs when windows sends a WM_DISPLAYCHANGE windows message to applications.";
		case /*STATUS_GRAPHICS_ONLY_CONSOLE_SESSION_SUPPORTED*/(DWORD)0xC01E05E0L: return "OPM, UAB, PVP and DDC/CI shared status codes {0x25E0..0x25FF} This function can only be used if a program is running in the local console session.  It cannot be used if a program is running on a remote desktop session or on a terminal server session.";
		case /*STATUS_GRAPHICS_NO_DISPLAY_DEVICE_CORRESPONDS_TO_NAME*/(DWORD)0xC01E05E1L: return "This function cannot find an actual GDI display device which corresponds to the specified GDI display device name.";
		case /*STATUS_GRAPHICS_DISPLAY_DEVICE_NOT_ATTACHED_TO_DESKTOP*/(DWORD)0xC01E05E2L: return "The function failed because the specified GDI display device was not attached to the Windows desktop.";
		case /*STATUS_GRAPHICS_MIRRORING_DEVICES_NOT_SUPPORTED*/(DWORD)0xC01E05E3L: return "This function does not support GDI mirroring display devices because GDI mirroring display devices do not have any physical monitors associated with them.";
		case /*STATUS_GRAPHICS_INVALID_POINTER*/(DWORD)0xC01E05E4L: return "The function failed because an invalid pointer parameter was passed to it.  A pointer parameter is invalid if it is NULL, it points to an invalid address, it points to a kernel mode address or it is not correctly aligned.";
		case /*STATUS_GRAPHICS_NO_MONITORS_CORRESPOND_TO_DISPLAY_DEVICE*/(DWORD)0xC01E05E5L: return "This function failed because the GDI device passed to it did not have any monitors associated with it.";
		case /*STATUS_GRAPHICS_PARAMETER_ARRAY_TOO_SMALL*/(DWORD)0xC01E05E6L: return "An array passed to the function cannot hold all of the data that the function must copy into the array.";
		case /*STATUS_GRAPHICS_INTERNAL_ERROR*/(DWORD)0xC01E05E7L: return "An internal error caused an operation to fail.";
		case /*STATUS_GRAPHICS_SESSION_TYPE_CHANGE_IN_PROGRESS*/(DWORD)0xC01E05E8L: return "The function failed because the current session is changing its type.  This function cannot be called when the current session is changing its type.  There are currently three types of sessions:";
		case /*STATUS_FVE_LOCKED_VOLUME*/(DWORD)0xC0210000L: return "Full Volume Encryption Error codes (fvevol.sys) This volume is locked by BitLocker Drive Encryption.";
		case /*STATUS_FVE_NOT_ENCRYPTED*/(DWORD)0xC0210001L: return "The volume is not encrypted, no key is available.";
		case /*STATUS_FVE_BAD_INFORMATION*/(DWORD)0xC0210002L: return "The control block for the encrypted volume is not valid.";
		case /*STATUS_FVE_TOO_SMALL*/(DWORD)0xC0210003L: return "The volume cannot be encrypted because it does not have enough free space.";
		case /*STATUS_FVE_FAILED_WRONG_FS*/(DWORD)0xC0210004L: return "The volume cannot be encrypted because the file system is not supported.";
		case /*STATUS_FVE_FAILED_BAD_FS*/(DWORD)0xC0210005L: return "The file system is corrupt. Run CHKDSK.";
		case /*STATUS_FVE_FS_NOT_EXTENDED*/(DWORD)0xC0210006L: return "The file system does not extend to the end of the volume.";
		case /*STATUS_FVE_FS_MOUNTED*/(DWORD)0xC0210007L: return "This operation cannot be performed while a file system is mounted on the volume.";
		case /*STATUS_FVE_NO_LICENSE*/(DWORD)0xC0210008L: return "BitLocker Drive Encryption is not included with this version of Windows.";
		case /*STATUS_FVE_ACTION_NOT_ALLOWED*/(DWORD)0xC0210009L: return "Requested action not allowed in the current volume state.";
		case /*STATUS_FVE_BAD_DATA*/(DWORD)0xC021000AL: return "Data supplied is malformed.";
		case /*STATUS_FVE_VOLUME_NOT_BOUND*/(DWORD)0xC021000BL: return "The volume is not bound to the system.";
		case /*STATUS_FVE_NOT_DATA_VOLUME*/(DWORD)0xC021000CL: return "That volume is not a data volume.";
		case /*STATUS_FVE_CONV_READ_ERROR*/(DWORD)0xC021000DL: return "A read operation failed while converting the volume.";
		case /*STATUS_FVE_CONV_WRITE_ERROR*/(DWORD)0xC021000EL: return "A write operation failed while converting the volume.";
		case /*STATUS_FVE_OVERLAPPED_UPDATE*/(DWORD)0xC021000FL: return "The control block for the encrypted volume was updated by another thread. Try again.";
		case /*STATUS_FVE_FAILED_SECTOR_SIZE*/(DWORD)0xC0210010L: return "The encryption algorithm does not support the sector size of that volume.";
		case /*STATUS_FVE_FAILED_AUTHENTICATION*/(DWORD)0xC0210011L: return "BitLocker recovery authentication failed.";
		case /*STATUS_FVE_NOT_OS_VOLUME*/(DWORD)0xC0210012L: return "That volume is not the OS volume.";
		case /*STATUS_FVE_KEYFILE_NOT_FOUND*/(DWORD)0xC0210013L: return "The BitLocker startup key or recovery password could not be read from external media.";
		case /*STATUS_FVE_KEYFILE_INVALID*/(DWORD)0xC0210014L: return "The BitLocker startup key or recovery password file is corrupt or invalid.";
		case /*STATUS_FVE_KEYFILE_NO_VMK*/(DWORD)0xC0210015L: return "The BitLocker encryption key could not be obtained from the startup key or recovery password.";
		case /*STATUS_FVE_TPM_DISABLED*/(DWORD)0xC0210016L: return "The Trusted Platform Module (TPM) is disabled.";
		case /*STATUS_FVE_TPM_SRK_AUTH_NOT_ZERO*/(DWORD)0xC0210017L: return "The authorization data for the Storage Root Key (SRK) of the Trusted Platform Module (TPM) is not zero.";
		case /*STATUS_FVE_TPM_INVALID_PCR*/(DWORD)0xC0210018L: return "The system boot information changed or the Trusted Platform Module (TPM) locked out access to BitLocker encryption keys until the computer is restarted.";
		case /*STATUS_FVE_TPM_NO_VMK*/(DWORD)0xC0210019L: return "The BitLocker encryption key could not be obtained from the Trusted Platform Module (TPM).";
		case /*STATUS_FVE_PIN_INVALID*/(DWORD)0xC021001AL: return "The BitLocker encryption key could not be obtained from the Trusted Platform Module (TPM) and PIN.";
		case /*STATUS_FVE_AUTH_INVALID_APPLICATION*/(DWORD)0xC021001BL: return "A boot application hash does not match the hash computed when BitLocker was turned on.";
		case /*STATUS_FVE_AUTH_INVALID_CONFIG*/(DWORD)0xC021001CL: return "The Boot Configuration Data (BCD) settings are not supported or have changed since BitLocker was enabled.";
		case /*STATUS_FVE_DEBUGGER_ENABLED*/(DWORD)0xC021001DL: return "Boot debugging is enabled.  Run bcdedit to turn it off.";
		case /*STATUS_FVE_DRY_RUN_FAILED*/(DWORD)0xC021001EL: return "The BitLocker encryption key could not be obtained.";
		case /*STATUS_FVE_BAD_METADATA_POINTER*/(DWORD)0xC021001FL: return "The metadata disk region pointer is incorrect.";
		case /*STATUS_FVE_OLD_METADATA_COPY*/(DWORD)0xC0210020L: return "The backup copy of the metadata is out of date.";
		case /*STATUS_FVE_REBOOT_REQUIRED*/(DWORD)0xC0210021L: return "No action was taken as a system reboot is required.";
		case /*STATUS_FVE_RAW_ACCESS*/(DWORD)0xC0210022L: return "No action was taken as BitLocker Drive Encryption is in RAW access mode.";
		case /*STATUS_FVE_RAW_BLOCKED*/(DWORD)0xC0210023L: return "BitLocker Drive Encryption cannot enter raw access mode for this volume.";
		case /*STATUS_FWP_CALLOUT_NOT_FOUND*/(DWORD)0xC0220001L: return "FWP error codes (fwpkclnt.sys) The callout does not exist.";
		case /*STATUS_FWP_CONDITION_NOT_FOUND*/(DWORD)0xC0220002L: return "The filter condition does not exist.";
		case /*STATUS_FWP_FILTER_NOT_FOUND*/(DWORD)0xC0220003L: return "The filter does not exist.";
		case /*STATUS_FWP_LAYER_NOT_FOUND*/(DWORD)0xC0220004L: return "The layer does not exist.";
		case /*STATUS_FWP_PROVIDER_NOT_FOUND*/(DWORD)0xC0220005L: return "The provider does not exist.";
		case /*STATUS_FWP_PROVIDER_CONTEXT_NOT_FOUND*/(DWORD)0xC0220006L: return "The provider context does not exist.";
		case /*STATUS_FWP_SUBLAYER_NOT_FOUND*/(DWORD)0xC0220007L: return "The sublayer does not exist.";
		case /*STATUS_FWP_NOT_FOUND*/(DWORD)0xC0220008L: return "The object does not exist.";
		case /*STATUS_FWP_ALREADY_EXISTS*/(DWORD)0xC0220009L: return "An object with that GUID or LUID already exists.";
		case /*STATUS_FWP_IN_USE*/(DWORD)0xC022000AL: return "The object is referenced by other objects so cannot be deleted.";
		case /*STATUS_FWP_DYNAMIC_SESSION_IN_PROGRESS*/(DWORD)0xC022000BL: return "The call is not allowed from within a dynamic session.";
		case /*STATUS_FWP_WRONG_SESSION*/(DWORD)0xC022000CL: return "The call was made from the wrong session so cannot be completed.";
		case /*STATUS_FWP_NO_TXN_IN_PROGRESS*/(DWORD)0xC022000DL: return "The call must be made from within an explicit transaction.";
		case /*STATUS_FWP_TXN_IN_PROGRESS*/(DWORD)0xC022000EL: return "The call is not allowed from within an explicit transaction.";
		case /*STATUS_FWP_TXN_ABORTED*/(DWORD)0xC022000FL: return "The explicit transaction has been forcibly cancelled.";
		case /*STATUS_FWP_SESSION_ABORTED*/(DWORD)0xC0220010L: return "The session has been cancelled.";
		case /*STATUS_FWP_INCOMPATIBLE_TXN*/(DWORD)0xC0220011L: return "The call is not allowed from within a read-only transaction.";
		case /*STATUS_FWP_TIMEOUT*/(DWORD)0xC0220012L: return "The call timed out while waiting to acquire the transaction lock.";
		case /*STATUS_FWP_NET_EVENTS_DISABLED*/(DWORD)0xC0220013L: return "Collection of network diagnostic events is disabled.";
		case /*STATUS_FWP_INCOMPATIBLE_LAYER*/(DWORD)0xC0220014L: return "The operation is not supported by the specified layer.";
		case /*STATUS_FWP_KM_CLIENTS_ONLY*/(DWORD)0xC0220015L: return "The call is allowed for kernel-mode callers only.";
		case /*STATUS_FWP_LIFETIME_MISMATCH*/(DWORD)0xC0220016L: return "The call tried to associate two objects with incompatible lifetimes.";
		case /*STATUS_FWP_BUILTIN_OBJECT*/(DWORD)0xC0220017L: return "The object is built in so cannot be deleted.";
		case /*STATUS_FWP_TOO_MANY_CALLOUTS*/(DWORD)0xC0220018L: return "The maximum number of callouts has been reached.";
		case /*STATUS_FWP_NOTIFICATION_DROPPED*/(DWORD)0xC0220019L: return "A notification could not be delivered because a message queue is at its maximum capacity.";
		case /*STATUS_FWP_TRAFFIC_MISMATCH*/(DWORD)0xC022001AL: return "The traffic parameters do not match those for the security association context.";
		case /*STATUS_FWP_INCOMPATIBLE_SA_STATE*/(DWORD)0xC022001BL: return "The call is not allowed for the current security association state.";
		case /*STATUS_FWP_NULL_POINTER*/(DWORD)0xC022001CL: return "A required pointer is null.";
		case /*STATUS_FWP_INVALID_ENUMERATOR*/(DWORD)0xC022001DL: return "An enumerator is not valid.";
		case /*STATUS_FWP_INVALID_FLAGS*/(DWORD)0xC022001EL: return "The flags field contains an invalid value.";
		case /*STATUS_FWP_INVALID_NET_MASK*/(DWORD)0xC022001FL: return "A network mask is not valid.";
		case /*STATUS_FWP_INVALID_RANGE*/(DWORD)0xC0220020L: return "An FWP_RANGE is not valid.";
		case /*STATUS_FWP_INVALID_INTERVAL*/(DWORD)0xC0220021L: return "The time interval is not valid.";
		case /*STATUS_FWP_ZERO_LENGTH_ARRAY*/(DWORD)0xC0220022L: return "An array that must contain at least one element is zero length.";
		case /*STATUS_FWP_NULL_DISPLAY_NAME*/(DWORD)0xC0220023L: return "The displayData.name field cannot be null.";
		case /*STATUS_FWP_INVALID_ACTION_TYPE*/(DWORD)0xC0220024L: return "The action type is not one of the allowed action types for a filter.";
		case /*STATUS_FWP_INVALID_WEIGHT*/(DWORD)0xC0220025L: return "The filter weight is not valid.";
		case /*STATUS_FWP_MATCH_TYPE_MISMATCH*/(DWORD)0xC0220026L: return "A filter condition contains a match type that is not compatible with the operands.";
		case /*STATUS_FWP_TYPE_MISMATCH*/(DWORD)0xC0220027L: return "An FWP_VALUE or FWPM_CONDITION_VALUE is of the wrong type.";
		case /*STATUS_FWP_OUT_OF_BOUNDS*/(DWORD)0xC0220028L: return "An integer value is outside the allowed range.";
		case /*STATUS_FWP_RESERVED*/(DWORD)0xC0220029L: return "A reserved field is non-zero.";
		case /*STATUS_FWP_DUPLICATE_CONDITION*/(DWORD)0xC022002AL: return "A filter cannot contain multiple conditions operating on a single field.";
		case /*STATUS_FWP_DUPLICATE_KEYMOD*/(DWORD)0xC022002BL: return "A policy cannot contain the same keying module more than once.";
		case /*STATUS_FWP_ACTION_INCOMPATIBLE_WITH_LAYER*/(DWORD)0xC022002CL: return "The action type is not compatible with the layer.";
		case /*STATUS_FWP_ACTION_INCOMPATIBLE_WITH_SUBLAYER*/(DWORD)0xC022002DL: return "The action type is not compatible with the sublayer.";
		case /*STATUS_FWP_CONTEXT_INCOMPATIBLE_WITH_LAYER*/(DWORD)0xC022002EL: return "The raw context or the provider context is not compatible with the layer.";
		case /*STATUS_FWP_CONTEXT_INCOMPATIBLE_WITH_CALLOUT*/(DWORD)0xC022002FL: return "The raw context or the provider context is not compatible with the callout.";
		case /*STATUS_FWP_INCOMPATIBLE_AUTH_METHOD*/(DWORD)0xC0220030L: return "The authentication method is not compatible with the policy type.";
		case /*STATUS_FWP_INCOMPATIBLE_DH_GROUP*/(DWORD)0xC0220031L: return "The Diffie-Hellman group is not compatible with the policy type.";
		case /*STATUS_FWP_EM_NOT_SUPPORTED*/(DWORD)0xC0220032L: return "An IKE policy cannot contain an Extended Mode policy.";
		case /*STATUS_FWP_NEVER_MATCH*/(DWORD)0xC0220033L: return "The enumeration template or subscription will never match any objects.";
		case /*STATUS_FWP_PROVIDER_CONTEXT_MISMATCH*/(DWORD)0xC0220034L: return "The provider context is of the wrong type.";
		case /*STATUS_FWP_INVALID_PARAMETER*/(DWORD)0xC0220035L: return "The parameter is incorrect.";
		case /*STATUS_FWP_TOO_MANY_SUBLAYERS*/(DWORD)0xC0220036L: return "The maximum number of sublayers has been reached.";
		case /*STATUS_FWP_CALLOUT_NOTIFICATION_FAILED*/(DWORD)0xC0220037L: return "The notification function for a callout returned an error.";
		case /*STATUS_FWP_INVALID_AUTH_TRANSFORM*/(DWORD)0xC0220038L: return "The IPsec authentication transform is not valid.";
		case /*STATUS_FWP_INVALID_CIPHER_TRANSFORM*/(DWORD)0xC0220039L: return "The IPsec cipher transform is not valid.";
		case /*STATUS_FWP_TCPIP_NOT_READY*/(DWORD)0xC0220100L: return "The TCP/IP stack is not ready.";
		case /*STATUS_FWP_INJECT_HANDLE_CLOSING*/(DWORD)0xC0220101L: return "The injection handle is being closed by another thread.";
		case /*STATUS_FWP_INJECT_HANDLE_STALE*/(DWORD)0xC0220102L: return "The injection handle is stale.";
		case /*STATUS_FWP_CANNOT_PEND*/(DWORD)0xC0220103L: return "The classify cannot be pended.";
		case /*STATUS_NDIS_CLOSING*/(DWORD)0xC0230002L: return "NDIS error codes (ndis.sys) The binding to the network interface is being closed.";
		case /*STATUS_NDIS_BAD_VERSION*/(DWORD)0xC0230004L: return "An invalid version was specified.";
		case /*STATUS_NDIS_BAD_CHARACTERISTICS*/(DWORD)0xC0230005L: return "An invalid characteristics table was used.";
		case /*STATUS_NDIS_ADAPTER_NOT_FOUND*/(DWORD)0xC0230006L: return "Failed to find the network interface or network interface is not ready.";
		case /*STATUS_NDIS_OPEN_FAILED*/(DWORD)0xC0230007L: return "Failed to open the network interface.";
		case /*STATUS_NDIS_DEVICE_FAILED*/(DWORD)0xC0230008L: return "Network interface has encountered an internal unrecoverable failure.";
		case /*STATUS_NDIS_MULTICAST_FULL*/(DWORD)0xC0230009L: return "The multicast list on the network interface is full.";
		case /*STATUS_NDIS_MULTICAST_EXISTS*/(DWORD)0xC023000AL: return "An attempt was made to add a duplicate multicast address to the list.";
		case /*STATUS_NDIS_MULTICAST_NOT_FOUND*/(DWORD)0xC023000BL: return "At attempt was made to remove a multicast address that was never added.";
		case /*STATUS_NDIS_REQUEST_ABORTED*/(DWORD)0xC023000CL: return "Netowork interface aborted the request.";
		case /*STATUS_NDIS_RESET_IN_PROGRESS*/(DWORD)0xC023000DL: return "Network interface can not process the request because it is being reset.";
		case /*STATUS_NDIS_NOT_SUPPORTED*/(DWORD)0xC02300BBL: return "Netword interface does not support this request.";
		case /*STATUS_NDIS_INVALID_PACKET*/(DWORD)0xC023000FL: return "An attempt was made to send an invalid packet on a network interface.";
		case /*STATUS_NDIS_ADAPTER_NOT_READY*/(DWORD)0xC0230011L: return "Network interface is not ready to complete this operation.";
		case /*STATUS_NDIS_INVALID_LENGTH*/(DWORD)0xC0230014L: return "The length of the buffer submitted for this operation is not valid.";
		case /*STATUS_NDIS_INVALID_DATA*/(DWORD)0xC0230015L: return "The data used for this operation is not valid.";
		case /*STATUS_NDIS_BUFFER_TOO_SHORT*/(DWORD)0xC0230016L: return "The length of buffer submitted for this operation is too small.";
		case /*STATUS_NDIS_INVALID_OID*/(DWORD)0xC0230017L: return "Network interface does not support this OID (Object Identifier)";
		case /*STATUS_NDIS_ADAPTER_REMOVED*/(DWORD)0xC0230018L: return "The network interface has been removed.";
		case /*STATUS_NDIS_UNSUPPORTED_MEDIA*/(DWORD)0xC0230019L: return "Network interface does not support this media type.";
		case /*STATUS_NDIS_GROUP_ADDRESS_IN_USE*/(DWORD)0xC023001AL: return "An attempt was made to remove a token ring group address that is in use by other components.";
		case /*STATUS_NDIS_FILE_NOT_FOUND*/(DWORD)0xC023001BL: return "An attempt was made to map a file that can not be found.";
		case /*STATUS_NDIS_ERROR_READING_FILE*/(DWORD)0xC023001CL: return "An error occured while NDIS tried to map the file.";
		case /*STATUS_NDIS_ALREADY_MAPPED*/(DWORD)0xC023001DL: return "An attempt was made to map a file that is alreay mapped.";
		case /*STATUS_NDIS_RESOURCE_CONFLICT*/(DWORD)0xC023001EL: return "An attempt to allocate a hardware resource failed because the resource is used by another component.";
		case /*STATUS_NDIS_MEDIA_DISCONNECTED*/(DWORD)0xC023001FL: return "The I/O operation failed because network media is disconnected or wireless access point is out of range.";
		case /*STATUS_NDIS_INVALID_ADDRESS*/(DWORD)0xC0230022L: return "The network address used in the request is invalid.";
		case /*STATUS_NDIS_INVALID_DEVICE_REQUEST*/(DWORD)0xC0230010L: return "The specified request is not a valid operation for the target device.";
		case /*STATUS_NDIS_PAUSED*/(DWORD)0xC023002AL: return "The offload operation on the network interface has been paused.";
		case /*STATUS_NDIS_INTERFACE_NOT_FOUND*/(DWORD)0xC023002BL: return "Network interface was not found.";
		case /*STATUS_NDIS_UNSUPPORTED_REVISION*/(DWORD)0xC023002CL: return "The revision number specified in the structure is not supported.";
		case /*STATUS_NDIS_INVALID_PORT*/(DWORD)0xC023002DL: return "The specified port does not exist on this network interface.";
		case /*STATUS_NDIS_INVALID_PORT_STATE*/(DWORD)0xC023002EL: return "The current state of the specified port on this network interface does not support the requested operation.";
		case /*STATUS_NDIS_LOW_POWER_STATE*/(DWORD)0xC023002FL: return "The miniport adapter is in lower power state.";
		case /*STATUS_NDIS_DOT11_AUTO_CONFIG_ENABLED*/(DWORD)0xC0232000L: return "NDIS error codes (802.11 wireless LAN) The wireless local area network interface is in auto configuration mode and doesn't support the requested parameter change operation.";
		case /*STATUS_NDIS_DOT11_MEDIA_IN_USE*/(DWORD)0xC0232001L: return "The wireless local area network interface is busy and can not perform the requested operation.";
		case /*STATUS_NDIS_DOT11_POWER_STATE_INVALID*/(DWORD)0xC0232002L: return "The wireless local area network interface is power down and doesn't support the requested operation.";
		case /*STATUS_NDIS_INDICATION_REQUIRED*/(DWORD)0x40230001L: return "NDIS informational codes(ndis.sys) The request will be completed later by NDIS status indication.";
		case /*STATUS_IPSEC_BAD_SPI*/(DWORD)0xC0360001L: return "IPSEC error codes (tcpip.sys) The SPI in the packet does not match a valid IPsec SA.";
		case /*STATUS_IPSEC_SA_LIFETIME_EXPIRED*/(DWORD)0xC0360002L: return "Packet was received on an IPsec SA whose lifetime has expired.";
		case /*STATUS_IPSEC_WRONG_SA*/(DWORD)0xC0360003L: return "Packet was received on an IPsec SA that doesn't match the packet characteristics.";
		case /*STATUS_IPSEC_REPLAY_CHECK_FAILED*/(DWORD)0xC0360004L: return "Packet sequence number replay check failed.";
		case /*STATUS_IPSEC_INVALID_PACKET*/(DWORD)0xC0360005L: return "IPsec header and/or trailer in the packet is invalid.";
		case /*STATUS_IPSEC_INTEGRITY_CHECK_FAILED*/(DWORD)0xC0360006L: return "IPsec integrity check failed.";
		case /*STATUS_IPSEC_CLEAR_TEXT_DROP*/(DWORD)0xC0360007L: return "IPsec dropped a clear text packet.";
		// whew, that was probably overkill
	}
} // end of ExceptionCodeToDescription

static int CountSharedPrefixLength(const char* a, const char* b)
{
	int i = 0;
	while(a[i] && tolower(a[i]) == tolower(b[i]))
		i++;
	return i;
}
static int CountBackslashes(const char* str)
{
	int count = 0;
	while((str = strchr(str, '\\'))!=0)
		count++, str++;
	return count;
}

static int IsPathTrusted(const char* path)
{
	// we want to return 0 for all system or installed dlls,
	// 2 for our own dll, 1 for the game exe,
	// and 1 for any dlls or exes around the game's directory

	if(!_stricmp(path, injectedDllPath))
		return 2;

	int outCount = 2;
	if(movie.version >= 68)
	{
		if(CountBackslashes(exefilename + CountSharedPrefixLength(exefilename, path)) < outCount)
			return 1;
		if(CountBackslashes(subexefilename + CountSharedPrefixLength(subexefilename, path)) < outCount)
			return 1;
	}
	else // old version, which accidentally trusts some paths it clearly shouldn't:
	{
		const char* end = path + strlen(path);
		for(const char* newEnd = end; outCount && newEnd != path; newEnd--)
		{
			if(*newEnd == '\\')
			{
				end = newEnd + 1;
				outCount--;
			}
		}
		if(!_strnicmp(path, exefilename, end - path))
			return 1;
		if(!_strnicmp(path, subexefilename, end - path))
			return 1;
	}
	if(strlen(path) >= 4 && !_strnicmp(path + strlen(path) - 4, ".cox", 4)) // hack, we can generally assume the game outputted any dll that has this extension
		return 1;

	return 0;
}


struct MyMODULEINFO
{
	MODULEINFO mi;
	std::string path;
};
static std::vector<MyMODULEINFO> trustedModuleInfos;
static MyMODULEINFO injectedDllModuleInfo;

void SetTrustedRangeInfo(TrustedRangeInfo& to, const MyMODULEINFO& from)
{
	DWORD start = (DWORD)from.mi.lpBaseOfDll;
	to.end = start + from.mi.SizeOfImage;
	to.start = start;
}

void SendTrustedRangeInfos(HANDLE hProcess)
{
	TrustedRangeInfos infos = {};

	SetTrustedRangeInfo(infos.infos[0], injectedDllModuleInfo);
	infos.numInfos++;

	int count = trustedModuleInfos.size();
	for(int i = 0; i < count; i++)
	{
		if(i+1 < ARRAYSIZE(infos.infos))
		{
			SetTrustedRangeInfo(infos.infos[1+i], trustedModuleInfos[i]);
			infos.numInfos++;
		}
	}
	
	SIZE_T bytesWritten = 0;
	WriteProcessMemory(hProcess, remoteTrustedRangeInfos, &infos, sizeof(infos), &bytesWritten);
}

bool IsInRange(DWORD address, const MODULEINFO& range)
{
	return (DWORD)((DWORD)address - (DWORD)range.lpBaseOfDll) < (DWORD)(range.SizeOfImage);
}
bool IsInNonCurrentYetTrustedAddressSpace(DWORD address)
{
	int count = trustedModuleInfos.size();
	for(int i = 0; i < count; i++)
		if(IsInRange(address, trustedModuleInfos[i].mi))
			return true;
	return (address == 0);
}

DWORD CalculateModuleSize(LPVOID hModule, HANDLE hProcess)
{
	// as noted below, I can't use GetModuleInformation here,
	// and I don't want to rely on the debughelp dll for core functionality either.
	DWORD size = 0;
	MEMORY_BASIC_INFORMATION mbi = {};
	while(VirtualQueryEx(hProcess, (char*)hModule+size+0x1000, &mbi, sizeof(mbi))
	&& (DWORD)mbi.AllocationBase == (DWORD)hModule)
		size += mbi.RegionSize;
	return max(size, 0x10000);
}

void RegisterModuleInfo(LPVOID hModule, HANDLE hProcess, const char* path)
{
	int trusted = IsPathTrusted(path);
	if(!trusted)
		return; // we only want to keep track of the trusted ones

	MyMODULEINFO mmi;
//	if(!GetModuleInformation(hProcess, (HMODULE)hModule, &mmi.mi, sizeof(mmi.mi)))
	{
		// GetModuleInformation never works, and neither does EnumProcessModules.
		// I think it's because we're calling it too early.
		// (either that or some versions of PSAPI.DLL are simply busted)
		// so this is the best fallback I could come up with
		mmi.mi.lpBaseOfDll = hModule;
		mmi.mi.SizeOfImage = CalculateModuleSize(hModule, hProcess);
		mmi.mi.EntryPoint = hModule; // don't care
	}
	mmi.path = path;
	if(trusted == 2)
	{
		injectedDllModuleInfo = mmi;
		//SendTrustedRangeInfos(hProcess); // disabled because it's probably not ready to receive it yet.
	}
	else
	{
		for(unsigned int i = 0; i < trustedModuleInfos.size(); i++)
		{
			MyMODULEINFO& mmi2 = trustedModuleInfos[i];
			if((DWORD)mmi.mi.lpBaseOfDll >= (DWORD)mmi2.mi.lpBaseOfDll
			&& (DWORD)mmi.mi.lpBaseOfDll+mmi.mi.SizeOfImage <= (DWORD)mmi2.mi.lpBaseOfDll+mmi2.mi.SizeOfImage)
			{
				debugprintf("apparently already TRUSTED MODULE 0x%08X - 0x%08X (%s)\n", mmi.mi.lpBaseOfDll, (DWORD)mmi.mi.lpBaseOfDll+mmi.mi.SizeOfImage, path);
				return;
			}
		}
		trustedModuleInfos.push_back(mmi);
		SendTrustedRangeInfos(hProcess);
	}
	debugprintf("TRUSTED MODULE 0x%08X - 0x%08X (%s)\n", mmi.mi.lpBaseOfDll, (DWORD)mmi.mi.lpBaseOfDll+mmi.mi.SizeOfImage, path);
}
void UnregisterModuleInfo(LPVOID hModule, HANDLE hProcess, const char* path)
{
	int trusted = IsPathTrusted(path);
	if(!trusted)
		return; // we only want to keep track of the trusted ones

	MEMORY_BASIC_INFORMATION mbi = {};
	if(VirtualQueryEx(hProcess, (char*)hModule+0x1000, &mbi, sizeof(mbi)))
		hModule = (HMODULE)mbi.AllocationBase;

	if(trusted == 2)
	{
		injectedDllModuleInfo.path.clear();
		memset(&injectedDllModuleInfo.mi, 0, sizeof(injectedDllModuleInfo.mi));
		//SendTrustedRangeInfos(hProcess); // disabled because the thing we'd send it to is what just got unloaded
	}
	else
	{
		int count = trustedModuleInfos.size();
		for(int i = 0; i < count; i++)
		{
			if(trustedModuleInfos[i].mi.lpBaseOfDll == hModule)
			{
				trustedModuleInfos.erase(trustedModuleInfos.begin() + i);
				SendTrustedRangeInfos(hProcess);
				break;
			}
		}
	}
}



struct CustomBreakpoint
{
	DWORD address;
	DWORD threadId;
	unsigned char origByte;
};
static std::vector<CustomBreakpoint> customBreakpoints;

int GetBreakpointIndex(DWORD address, DWORD threadId)
{
	int count = customBreakpoints.size();
	for(int i = 0; i < count; i++)
	{
		CustomBreakpoint& bp = customBreakpoints[i];
		if(bp.address == address && bp.threadId == threadId)
			return i;
	}
	return -1;
}

void AddBreakpoint(DWORD address, DWORD threadId, HANDLE hProcess)
{
	if(GetBreakpointIndex(address, threadId) != -1)
		return;

	CustomBreakpoint bp;
	bp.address = address;
	bp.threadId = threadId;

	DWORD dwOldProtect = 0;
	VirtualProtectEx(hProcess, (void*)address, 1, PAGE_READWRITE, &dwOldProtect); 

	SIZE_T unused = 0;
	ReadProcessMemory(hProcess, (void*)address, &bp.origByte, 1, &unused);

	unsigned char newByte = 0xCC;
	WriteProcessMemory(hProcess, (void*)address, &newByte, 1, &unused);

	VirtualProtectEx(hProcess, (void*)address, 1, dwOldProtect, &dwOldProtect);

	customBreakpoints.push_back(bp);
}

void RemoveBreakpoint(DWORD address, DWORD threadId, HANDLE hProcess)
{
	int index = GetBreakpointIndex(address, threadId);
	if(index == -1)
		return;

	CustomBreakpoint& bp = customBreakpoints[index];

	DWORD dwOldProtect = 0;
	VirtualProtectEx(hProcess, (void*)address, 1, PAGE_READWRITE, &dwOldProtect); 

	SIZE_T unused = 0;
	WriteProcessMemory(hProcess, (void*)address, &bp.origByte, 1, &unused);

	VirtualProtectEx(hProcess, (void*)address, 1, dwOldProtect, &dwOldProtect);

	customBreakpoints.erase(customBreakpoints.begin() + index);
}

// SetPC
bool SetProgramCounter(HANDLE hThread, DWORD address)
{
	CONTEXT context = {CONTEXT_CONTROL};
	if(GetThreadContext(hThread, &context))
	{
		context.Eip = address;
		return SetThreadContext(hThread, &context) != 0;
	}
	return false;
}

// GetPC
DWORD GetProgramCounter(HANDLE hThread)
{
	CONTEXT context = {CONTEXT_CONTROL};
	if(GetThreadContext(hThread, &context))
		return context.Eip;
	return 0;
}




static std::vector<PROCESS_INFORMATION> allProcessInfos; // was oldProcessInfos


static HANDLE debuggerThread = NULL;

static void CloseThreadHandles(HANDLE threadHandleToNotClose, HANDLE hProcess)
{
	{
		std::map<DWORD,ThreadInfo>::iterator iter;
		for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
		{
			HANDLE handle = iter->second;
			if(handle == threadHandleToNotClose)
				continue;
			debugprintf("Closing thread handle 0x%X\n", handle);
#ifndef _DEBUG
			__try {
#endif
				CloseHandle(handle); // if we get a EXCEPTION_INVALID_HANDLE exception here that's really bad... maybe the LdrLoadDll hook is doing too much and so we got debugging events with wrong thread handles... or maybe we called CloseHandle on the wrong things in the debug event handlers here
#ifndef _DEBUG
			} __except(EXCEPTION_EXECUTE_HANDLER) {
			}
#endif
		}
		hGameThreads.clear();
		gameThreadIdList.clear();
	}
}

#if 0
// https://groups.google.com/forum/?fromgroups=#!topic/microsoft.public.vc.debugger/VWAYhm6tU7E
// Post speaks of awork around that fixes programs that don't spawn windows by adjusting tokens
// I have no idea what to adjust, I tried with the DEBUG token because it's quite harmless
// but it had no effect. I was hoping this would solve some freezing issues in games that 
// become "Zombie-fied" when launching them in Hourglass. But it had no effect.
// Leaving the code here in case someone wants to experiment further or know more about this.
// The call to revert the token changes needs to be moved, I don't know to where.
// -- Warepire

//When enable is true the function will assign extended privileges to the debugee
//When enable is false those privileges will be removed
//Returns true if the change was successful, and false otherwise
bool AdjustPrivileges(HANDLE hGameHandle, bool bEnable)
{
	debugprintf("Attempting adjustment of the game's privileges to: %s\n", bEnable ? "extended" : "normal");

    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

	//HANDLE currentProcess = GetCurrentProcess();

	BOOL opened = OpenProcessToken(hGameHandle, (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ), &hToken);
    if (opened == FALSE)
	{
		debugprintf("Failed to get the token for the game process...\n");
		return false;
	}

	BOOL lookedUp = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
    if (lookedUp == FALSE)
	{
		debugprintf("Failed to look up the privileges...\n");
		CloseHandle(hToken);
		return false;
	}

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	BOOL adjusted = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
	if(adjusted == FALSE)
	{
		debugprintf("Failed to complete the privilege adjustment...\n");
		CloseHandle(hToken);
		return false;
	}

	debugprintf("Adjustment completed successfully.\n");
	CloseHandle(hToken);
    return true;
}
#endif

static void DebuggerThreadFuncCleanup(HANDLE threadHandleToClose, HANDLE hProcess)
{
	if(threadHandleToClose != INVALID_HANDLE_VALUE)
	{
			// We have to take the privileges away or the system will permanently have them assigned to the game.
			// This should probably be performed somewhere else?
	//		SuspendThread(threadHandleToClose);
	//		AdjustPrivileges(threadHandleToClose, FALSE);
	//		ResumeThread(threadHandleToClose);

		debugprintf("Closing thread handle 0x%X\n", threadHandleToClose);
		CloseHandle(threadHandleToClose);
	}

	started = false;
	paused = false;
	mainMenuNeedsRebuilding = true;
	//playback = false; // can't clear this here or wasPlayback will get an incorrect value after a crash
	usedThreadMode = -1;
//	s_lastThreadSwitchedTo = 0;

	remoteCommandSlot = 0;
	remoteTASflags = 0;
	remoteKeyboardState = 0;
	//remoteLastFrameSoundInfo = 0;

	hGameProcess = 0;
	{
		AutoCritSect cs(&g_gameHWndsCS);
		gameHWnds.clear();
	}

	trustedModuleInfos.clear();
	injectedDllModuleInfo.path.clear();
	memset(&injectedDllModuleInfo.mi, 0, sizeof(injectedDllModuleInfo.mi));

	{
		//std::map<LPVOID,HANDLE>::iterator iter;
		//for(iter = dllBaseToHandle.begin(); iter != dllBaseToHandle.end(); iter++)
		//	CloseHandle(iter->second);
//		dllBaseToHandle.clear();
		dllBaseToFilename.clear();
	}

	//CloseThreadHandles(threadHandleToClose, hProcess);

	EXTENDEDTRACEUNINITIALIZE(hProcess);
	//for(unsigned int i = 0; i < allProcessInfos.size(); i++)
	//{
	//	HANDLE hOldProcess = allProcessInfos[i].hProcess;
	//	EXTENDEDTRACEUNINITIALIZE(hOldProcess);
	//	debugprintf("Closing process handle 0x%X\n", hOldProcess);
	//	CloseHandle(hOldProcess);
	//}
	allProcessInfos.clear();
	customBreakpoints.clear();

	terminateRequest = false;
	debuggerThread = 0; // should happen last
}



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

static DWORD danglingThreadNameId = -1;
static char danglingThreadName[64];

static HANDLE GetProcessHandle(PROCESS_INFORMATION& processInfo, const DEBUG_EVENT& de)
{
	HANDLE hProcess = processInfo.hProcess;
	if(processInfo.dwProcessId != de.dwProcessId)
		for(unsigned int i = 0; i < allProcessInfos.size(); i++)
			if(allProcessInfos[i].dwProcessId == de.dwProcessId)
				hProcess = allProcessInfos[i].hProcess;
	return hProcess;
}

static void HandleThreadExitEvent(DEBUG_EVENT& de, PROCESS_INFORMATION& processInfo)
{
	std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
	if(found != hGameThreads.end())
	{
		HANDLE hThread = found->second;
		debugprintf("STOPPED THREAD: id=0x%X, handle=0x%X, name=%s\n", de.dwThreadId, hThread, found->second.name);
		hGameThreads.erase(found);
	}
	else
	{
		debugprintf("STOPPED THREAD: id=0x%X, handle=unknown\n", de.dwThreadId);
	}

	std::vector<DWORD>::iterator found2 = std::find(gameThreadIdList.begin(), gameThreadIdList.end(), de.dwThreadId);
	if(found2 != gameThreadIdList.end())
		gameThreadIdList.erase(found2);

	if(de.dwThreadId == processInfo.dwThreadId)
		processInfo.hThread = INVALID_HANDLE_VALUE;
}

//static void SafeTerminateProcess(PROCESS_INFORMATION& processInfo)
//{
//	// print the status and callstack of all threads
//	debugprintf("final stack trace:\n");
//	std::map<DWORD,ThreadInfo>::iterator iter;
//	for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
//	{
//		DWORD threadId = iter->first;
//		ThreadInfo threadInfo = iter->second;
//		HANDLE hThread = threadInfo.handle;
//		char msg [16+72];
//		sprintf(msg, "(id=0x%X) (name=%s)", threadId, threadInfo.name);
//		THREADSTACKTRACEMSG(hThread, msg, /*threadInfo.hProcess*/hGameProcess);
//	}
//
//	// let's warn the game before forcefully shutting it down,
//	// mostly for the purpose of preventing sound stuttering.
//
//	// have to loop at least twice to eat the FRAME command the game might send first.
//	// should loop a few more times for safety (to give the game some extra time to cleanup).
//	// but can't be too many times because keep in mind we're on a 5000 millisecond timer to exit this thread.
//	DEBUG_EVENT de;
//	for (int i = 0; i < 2+8; i++)
//	{
//		sprintf(localCommandSlot, "PREPARETODIE: %d", i);
//		requestedCommandReenter = false;
//		SendCommand();
//		if(WaitForDebugEvent(&de, 50))
//		{
//			switch(de.dwDebugEventCode)
//			{
//			case EXIT_THREAD_DEBUG_EVENT:
//				HandleThreadExitEvent(de, processInfo);
//				break;
//			case EXIT_PROCESS_DEBUG_EVENT:
//				goto doneterminate;
//			}
//			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
//		}
//	}
//doneterminate:
//	DWORD exitcode;
//	enum {FORCED_EXITCODE = -4242 };
//	for(int i = 0; i < 10; i++) // we're still on a timer but another half second should be ok
//	{
//		if(GetExitCodeProcess(hGameProcess, &exitcode) && exitcode != STILL_ACTIVE)
//			break;
//
//		TerminateProcess(hGameProcess, FORCED_EXITCODE);
//
//		if(GetExitCodeProcess(hGameProcess, &exitcode) && exitcode != STILL_ACTIVE)
//			break;
//
//		Sleep(50);
//	}
//	if(GetExitCodeProcess(hGameProcess, &exitcode))
//	{
//		switch(exitcode)
//		{
//		case (DWORD)SUCCESSFUL_EXITCODE: debugprintf("game process exited (terminated successfully)\n"); break;
//		case (DWORD)FORCED_EXITCODE:     debugprintf("game process exited (terminated forcefully)\n"); break;
//		case STILL_ACTIVE:               debugprintf("game process exited? (terminated implicitly)\n"); break;
//		default: {
//			const char* desc = ExceptionCodeToDescription(exitcode, NULL);
//			if(desc)
//				debugprintf("game process exited (killed by: %s)\n", desc);
//			else
//				debugprintf("game process exited (with unknown exit code %d)\n", (int)exitcode);
//			} break;
//		}
//	}
//	else
//		debugprintf("game process exited? (no exit code)\n");
//	debugprintf("Closing process handle 0x%X\n", hGameProcess);
//	CloseHandle(hGameProcess);
//	hGameProcess = 0;
//}

void OnMovieStart()
{
	finished = false;

	if(movie.version >= 70)
	{
		// auto-detect app locale settings based on original movie author's keyboard layout in certain cases
		tempAppLocale = 0;
		if(!localTASflags.appLocale && movie.keyboardLayoutName[0] && movie.keyboardLayoutName[4])
		{
			DWORD keyboardLayout = 0;
			sscanf(movie.keyboardLayoutName, "%08X", &keyboardLayout);
			switch(keyboardLayout & 0xFFFF)
			{
			case 1041:
			case 2052:
			case 1042:
			//case 1033:
				tempAppLocale = (keyboardLayout & 0xFFFF);
				break;
			default:
				break;
			}
		}
	}
}



// debuggerthreadproc
static DWORD WINAPI DebuggerThreadFunc(LPVOID lpParam) 
{
	ClearCrcCache();

//restartgame:

	STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION processInfo = {0};
	allProcessInfos.clear();
	customBreakpoints.clear();

	s_lastFrameCount = 0;
	movie.currentFrame = 0;
	{
		int loadMovieResult = LoadMovie(moviefilename);
		if(loadMovieResult < 0 || (localTASflags.playback && loadMovieResult == 0))
			goto earlyAbort;
		OnMovieStart();
	}

	DeleteCriticalSection(&g_processMemCS);
	InitializeCriticalSection(&g_processMemCS);

	subexefilename[0] = 0;
	AbsolutifyPath(exefilename);
	char initialDirectory [MAX_PATH+1];
	strcpy(initialDirectory, exefilename);
	char* slash = max(strrchr(initialDirectory, '\\'), strrchr(initialDirectory, '/'));
	if(slash)
		*slash = 0;

	char* cmdline = NULL;
	if(movie.version >= 60 && movie.version <= 63)
		cmdline = commandline;
	else if(movie.version >= 64)
	{
		static char tempcmdline [ARRAYSIZE(commandline)+MAX_PATH+4];
		sprintf(tempcmdline, "\"%s\" %s", exefilename, commandline);
		cmdline = tempcmdline;
	}

	/*debugprintf("enabling full debug privileges...\n");
	if(!EnableDebugPrivilege())
	{
		debugprintf("failed to enable full debug privileges...\n");
	}*/

	debugprintf("creating process \"%s\"...\n", exefilename);
	//debugprintf("initial directory = \"%s\"...\n", initialDirectory);
	BOOL created = CreateProcess(exefilename, // application name
		cmdline, // commandline arguments
		NULL, // process attributes (e.g. security descriptor)
		NULL, // thread attributes (e.g. security descriptor)
		FALSE, // inherit handles
		CREATE_SUSPENDED | DEBUG_PROCESS, // creation flags
		///*INHERIT_PARENT_AFFINITY*/0x00010000 | CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS, // creation flags
		//0,//CREATE_SUSPENDED ,//| DEBUG_ONLY_THIS_PROCESS, // creation flags
		NULL, // environment
		initialDirectory, // initial directory
		&startupInfo,
		&processInfo);

	if(!created)
	{
		DWORD error = GetLastError();
		PrintLastError("CreateProcess", error);
		if(error == ERROR_ELEVATION_REQUIRED)
		{
			char str [1024];
			sprintf(str,
				"ERROR: Admin privileges are required to run \"%s\" on this system.\n"
				"Hourglass doesn't have high enough privileges to launch the game.\n"
				"Try closing Hourglass and then opening it with \"Run as administrator\".", exefilename);
			CustomMessageBox(str, "Permission Denied", MB_OK|MB_ICONERROR);
		}
	}

	hGameProcess = processInfo.hProcess;
	debugprintf("done creating process, got handle 0x%X, PID = %d.\n", hGameProcess, processInfo.dwProcessId);

	// If we're set to capture before the game is launched, hGameProcess needs to be registered ASAP with AVI Dumper after it's been created so that we can begin capture immediately.
	if(localTASflags.aviMode > 0)
	{
		SetCaptureProcess(hGameProcess);
	}

	ReopenRamWindows();
	reset_address_info();

		// doesn't work?
	//{
	//	char procfilename [MAX_PATH+1];
	//	if(GetFileNameFromFileHandle(hGameProcess, procfilename))
	//		debugprintf("created process from file \"%s\"\n", procfilename);
	//	else
	//		debugprintf("ERROR: unable to get process filename.\n");
	//}

	PrintPrivileges(hGameProcess);

//	AdjustPrivileges(hGameProcess, TRUE);

	if(!onlyHookChildProcesses)
	{
		EXTENDEDTRACEINITIALIZEEX( NULL, hGameProcess );
	}

	//debugprintf("INITIAL THREAD: 0x%X\n", processInfo.hThread);

	debugprintf("attempting injection...\n");

	char* dllpath = injectedDllPath;
	strcpy(dllpath, thisprocessPath);
	strcat(dllpath, "\\wintasee.dll");

	if(!onlyHookChildProcesses)
	{
		InjectDll(processInfo.hProcess, processInfo.dwProcessId, processInfo.hThread, processInfo.dwThreadId, dllpath, runDllLast!=0);

		debugprintf("done injection. starting game thread\n");
	}

	bool askedToRestartAboutBadThreadId = false;
	//bool hasMainThread = false;

	requestedCommandReenter = false;

	for(int i = 0; i < maxNumSavestates; i++)
		savestates[i].stale = savestates[i].valid;

	s_firstTimerDesyncWarnedFrame = 0x7FFFFFFF;
	gotSrcDllVersion = false;

	//g_gammaRampEnabled = false;
	dllLoadInfos.numInfos = 0;
	dllLoadInfosSent = false;

	EnterCriticalSection(&g_processMemCS);
	runningNow = true;

	started = true;
	CheckDialogChanges(0);
	EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP), true);
	mainMenuNeedsRebuilding = true;
	HANDLE hInitialThread = processInfo.hThread;
	ResumeThread(hInitialThread);

	//BOOL firstBreak = TRUE;
	bool postDllMainDone = false;
	bool mainThreadWaitingForPostDllMain = false;
	DWORD entrypoint = 0;
	int exceptionPrintingPaused = 0;
	//bool redalert = false;
	lastFrameAdvanceKeyUnheldTime = timeGetTime();

	debugprintf("entering debugger loop\n");
	DEBUG_EVENT de;

	bool terminated = false;
	bool terminateAttempted = false;
	while(!terminated)
	{
		if(terminateRequest && !terminateAttempted)
		{
			for(unsigned int i = 0; i < allProcessInfos.size(); i++)
				TerminateProcess(allProcessInfos[i].hProcess, 0);
			if(allProcessInfos.empty())
				TerminateProcess(hGameProcess, 0);
			terminateAttempted = true;
		}

		//if((GetAsyncKeyState('R') & 0x8000))
		//{
		//	TerminateProcess(processInfo.hProcess, 0);
		//	goto done;
		//}

//		if (WaitForDebugEvent(&de, INFINITE))
		if (WaitForDebugEvent(&de, /*redalert?1:*/30))
		{
			runningNow = false;

			//if(redalert)
			//{
			//	int numThreads = gameThreadIdList.size();
			//	for(int i = 0; i < numThreads; i++)
			//	{
			//		DWORD threadId = gameThreadIdList[i];
			//		ThreadInfo& info = hGameThreads[threadId];
			//		HANDLE hThread = info.handle;
			//		char msg [16+72];
			//		sprintf(msg, "(id=0x%X) (name=%s)", threadId, info.name);
			//		THREADSTACKTRACEMSG(hThread, msg, /*info.hProcess*/hGameProcess);
			//	}
			//	static int ralcount = 0;
			//	ralcount++;
			//	if(ralcount == 10000) {
			//		ralcount = 0;
			//		redalert = 0;
			//	}
			//}

			LeaveCriticalSection(&g_processMemCS);
			if(de.dwDebugEventCode != 8)
				verbosedebugprintf("WaitForDebugEvent. received debug event: %X\n", de.dwDebugEventCode);

			switch(de.dwDebugEventCode)
			{
			case OUTPUT_DEBUG_STRING_EVENT:
				{
#pragma region OUTPUT_DEBUG_STRING_EVENT
					HANDLE hProcess = hGameProcess;//GetProcessHandle(processInfo,de); 
					OUTPUT_DEBUG_STRING_INFO dsi = de.u.DebugString;
					int len = dsi.nDebugStringLength;
					char *str = new char[len]; // Taking all messages instead of ignoring extra large ones.
					//if(len+1 < sizeof(str)) // just ignore any strings bigger than our buffer size
					{
						SIZE_T bytesRead = 0;
						if(!ReadProcessMemory(hProcess, dsi.lpDebugStringData, (LPVOID)str, len, &bytesRead))
							sprintf(str, /*"SHORTTRACE: 0,50 "*/ "ERROR READING STRING of length %d\n", len);
//						if(str[strlen(str)-1] != '\n')
//							strcat(str, "\n");
//						str[sizeof(str)-1] = '\0';
#ifdef _DEBUG
						if(0!=strncmp(str, "MSG", 3) && 0!=strncmp(str, "LOG", 3) && (0!=strncmp(str, "FRAME", 5) || temporaryUnpause))
							debugprintf("debug string: 0x%X: %s", de.dwThreadId, str);
#endif
						const char* pstr = str;

#define MessagePrefixMatch(pre) (!strncmp(pstr, pre": ", sizeof(pre": ")-1) ? pstr += sizeof(pre": ")-1 : false)

						if(MessagePrefixMatch("FRAME"))
						{
							FrameBoundary(pstr, de.dwThreadId);

#ifdef _DEBUG
							if(s_lastFrameCount <= 1 && !requestedCommandReenter)
							{
								debugprintf("On Frame %d: \n", s_lastFrameCount);
								// print the callstack of all threads

								int numThreads = gameThreadIdList.size();
								for(int i = 0; i < numThreads; i++)
								{
									DWORD threadId = gameThreadIdList[i];
									ThreadInfo& info = hGameThreads[threadId];
									HANDLE hThread = info.handle;
									char msg [16+72];
									sprintf(msg, "(id=0x%X) (name=%s)", threadId, info.name);
									THREADSTACKTRACEMSG(hThread, msg, /*info.hProcess*/hGameProcess);
								}

								//std::map<DWORD,ThreadInfo>::iterator iter;
								//for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
								//{
								//	HANDLE hThread = iter->second;
								//	//THREADSTACKTRACE(hThread);
								//	char msg [16+72];
								//	sprintf(msg, "(id=0x%X) (name=%s)", iter->first, iter->second.name);
								//	THREADSTACKTRACEMSG(hThread, msg, (iter->second).hProcess);
								//}
							}
#endif

						}
						else if(MessagePrefixMatch("SLEEPFRAME"))
						{
							SleepFrameBoundary(pstr, de.dwThreadId);
#if 0
							// print the callstack of the thread
							std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
							if(found != hGameThreads.end())
							{
								HANDLE hThread = found->second;
								//THREADSTACKTRACE(hThread);
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
								THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
							}
#endif
						}
						else if(MessagePrefixMatch("MSG"))
							debugprintf("%s",pstr);
						else if(MessagePrefixMatch("LOG"))
						{
							int logFlags = 0;
							if(const char* cEquals = strstr(pstr+17, "c=")) {
								logFlags = strtol(cEquals+2, 0, 16);
							}
							if(includeLogFlags & logFlags)
							{
								debugprintf("%s",pstr);
							}
							if(traceLogFlags & logFlags)
							{
								int minDepth = 3, maxDepth = 64;
								// print the callstack of the thread
								std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
								if(found != hGameThreads.end())
								{
									HANDLE hThread = found->second;
									char msg [16+72];
									sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
									StackTraceOfDepth(hThread, msg, minDepth, maxDepth, /*(found->second).hProcess*/hGameProcess);
								}
							}
						}
						else if(MessagePrefixMatch("PAUSEEXCEPTIONPRINTING"))
							exceptionPrintingPaused++;
						else if(MessagePrefixMatch("RESUMEEXCEPTIONPRINTING"))
							exceptionPrintingPaused--;
						else if(MessagePrefixMatch("SAVE"))
							SaveGameStatePhase2(atoi(pstr));
						else if(MessagePrefixMatch("LOAD"))
							LoadGameStatePhase2(atoi(pstr));
						else if(MessagePrefixMatch("SUSPENDALL"))
							SuspendAllExcept(de.dwThreadId);
						else if(MessagePrefixMatch("RESUMEALL"))
							ResumeAllExcept(de.dwThreadId);
						else if(MessagePrefixMatch("FPS"))
							ReceiveFrameRate(pstr);
						else if(MessagePrefixMatch("HWND"))
							ReceiveHWND(atoi(pstr));
						else if(MessagePrefixMatch("WATCH")) 
						{
							AddressWatcher auto_watch;
							char comment[256];
							sscanf(pstr,"%08X,%c,%c,%s",&(auto_watch.Address),&(auto_watch.Size),&(auto_watch.Type),comment);
							auto_watch.WrongEndian=false;
							if(IsHardwareAddressValid(auto_watch.Address))
								InsertWatch(auto_watch,comment);							
						}
						else if(MessagePrefixMatch("UNWATCH")) 
						{
							AddressWatcher auto_unwatch;
							sscanf(pstr,"%08X, %c, %c",&(auto_unwatch.Address),&(auto_unwatch.Size),&(auto_unwatch.Type));
							auto_unwatch.WrongEndian=false;
							RemoveWatch(auto_unwatch);							
						}
						else if(MessagePrefixMatch("SHORTTRACE"))
						{
							int minDepth = 0, maxDepth = -1;
							sscanf(pstr, "%d, %d", &minDepth, &maxDepth);
							// print the callstack of the thread
							std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
							if(found != hGameThreads.end())
							{
								HANDLE hThread = found->second;
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
								StackTraceOfDepth(hThread, msg, minDepth, maxDepth, /*(found->second).hProcess*/hGameProcess);
							}
						}
						//else if(MessagePrefixMatch("NOLOAD"))
						//	AddToNoLoadList(pstr);
						//else if(MessagePrefixMatch("CLEARNOLOAD"))
						//	ClearNoLoadList();
						else if(MessagePrefixMatch("GIVEMEANAME"))
						{
							SuggestThreadName(de.dwThreadId, hProcess);
							// print the callstack of the thread
							std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
							if(found != hGameThreads.end())
							{
								HANDLE hThread = found->second;
								//THREADSTACKTRACE(hThread);
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
								THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
							}
						}
						else if(MessagePrefixMatch("HERESMYCARD"))
							ReceiveCommandSlotPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("DLLLOADINFOBUF")) //GETDLLLIST
							ReceiveDllLoadInfosPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("GIMMEDLLLOADINFOS"))
							AddAndSendDllInfo(NULL, true, hProcess);
						else if(MessagePrefixMatch("TRUSTEDRANGEINFOBUF"))
							ReceiveTrustedRangeInfosPointer(_atoi64(pstr), hProcess);
						else if(MessagePrefixMatch("KEYINPUTBUF"))
							ReceiveKeyStatusPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("TASFLAGSBUF"))
							ReceiveTASFlagsPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("GENERALINFO"))
							ReceiveGeneralInfoPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("SOUNDINFO"))
							ReceiveSoundCaptureInfoPointer(_atoi64(pstr));
						//else if(MessagePrefixMatch("EXTHWNDBUF"))
						//	ReceiveExtHWndBuf(_atoi64(pstr));
						else if(MessagePrefixMatch("GAMMARAMPDATA"))
							ReceiveGammaRampData(_atoi64(pstr), hProcess);
						else if(MessagePrefixMatch("PALETTEENTRIES"))
							ReceivePaletteEntriesPointer(_atoi64(pstr));
						else if(MessagePrefixMatch("KEYBLAYOUT"))
							ReceiveKeyboardLayout(_atoi64(pstr), hProcess);
						else if(MessagePrefixMatch("DENIEDTHREAD"))
							usedThreadMode = localTASflags.threadMode;
						else if(MessagePrefixMatch("POSTDLLMAINDONE"))
						{
							postDllMainDone = true;
							if(mainThreadWaitingForPostDllMain)
							{
								debugprintf("resuming main thread...\n");
								ResumeThread(processInfo.hThread);
								mainThreadWaitingForPostDllMain = false;
							}
						}
						else if(MessagePrefixMatch("SRCDLLVERSION"))
							CheckSrcDllVersion(atoi(pstr));
						else if(MessagePrefixMatch("DLLVERSION"))
							ProcessDllVersion(pstr);
						else if(MessagePrefixMatch("KILLME"))
							terminateRequest = true;
						else if(MessagePrefixMatch("DEBUGPAUSE"))
						{
							debugprintf("DEBUGPAUSE: %s",pstr);
							paused = true;
							temporaryUnpause = false;
							CheckDialogChanges(-1);
							while(paused && !temporaryUnpause && !terminateRequest)
							{
								CheckHotkeys(-1, false);
								Sleep(5);
							}
						}
						//else if(MessagePrefixMatch("DEBUGREDALERT"))
						//{
						//	redalert = true;
						//}
						//else if(MessagePrefixMatch("DEBUGTRACEADDRESS"))
						//{
						//	//debugprintf("DEBUGTRACEADDRESS: %s", pstr);
						//	DWORD addr = 0;
						//	sscanf(pstr, "%08X", &addr);
						//	void TraceSingleAddress(DWORD addr, HANDLE hProcess);
						//	TraceSingleAddress(addr, GetProcessHandle(processInfo,de));
						//}
						else
						{
#ifndef _DEBUG
							char first = pstr[0];
							if((first >= 'A' && first <= 'Z' || first >= 'a' && first <= 'z' || first >= '0' && first <= '9') && pstr[1] >= ' ')
#endif
							{
								debugprintf("UNKNOWN MESSAGE: %s",pstr); // unhandled message
							}
//#if defined(_DEBUG) && 0
							// print the callstack of the thread
							std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
							if(found != hGameThreads.end())
							{
								HANDLE hThread = found->second;
								//THREADSTACKTRACE(hThread);
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
								THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
							}
//#endif
						}

#undef MessagePrefixMatch
					}
				delete [] str; // destroying str since it's now a pointer, and we don't need it anymore.
#pragma endregion
				}
				break;

			case EXCEPTION_DEBUG_EVENT:
			{
#pragma region EXCEPTION_DEBUG_EVENT
				verbosedebugprintf("exception code: 0x%X\n", de.u.Exception.ExceptionRecord.ExceptionCode);

				if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				{
					DWORD address = (DWORD)de.u.Exception.ExceptionRecord.ExceptionAddress;
					int index = GetBreakpointIndex(address, de.dwThreadId);
					if(index != -1)
					{
						debugprintf("hit custom breakpoint at address = 0x%X\n", address);
						if(address == entrypoint)
						{
							RemoveBreakpoint(address, de.dwThreadId, /*GetProcessHandle(processInfo,de)*/hGameProcess);
							//debugprintf("pc was 0x%X\n", GetProgramCounter(processInfo.hThread));
							// in this case we can use processInfo.hThread because we know we're dealing with the main thread
							SetProgramCounter(processInfo.hThread, address);
							// suspend main thread until PostDllMain finishes
							if(!postDllMainDone)
							{
								debugprintf("suspending main thread until PostDllMain finishes...\n");
								SuspendThread(processInfo.hThread);
								mainThreadWaitingForPostDllMain = true;
							}
						}
					}
				}

				//if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				//{
				//	if(!firstBreak)
				//	{
				//		CustomMessageBox("_asm {int 3}", "Breakpoint??", MB_OK|MB_ICONWARNING);
				//	}
				//	firstBreak = false;
				//}
				//else
				{
				  if(de.u.Exception.ExceptionRecord.ExceptionCode != THREAD_NAME_EXCEPTION)
				  {
				    bool couldBeSerious = !de.u.Exception.dwFirstChance || (
					   (de.u.Exception.ExceptionRecord.ExceptionCode&0xCFFFFFFF) != /*EXCEPTION_MSVC*/0xC06D7363
					&& (de.u.Exception.ExceptionRecord.ExceptionCode&0xCFFFFFFF) != 0xC0440001
					);

#ifndef _DEBUG
					if(!exceptionPrintingPaused)
#endif
					{
						if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) // 0xC0000005
						{
							debugprintf("exception 0x%X (at 0x%08X): attempt to %s memory at 0x%08X\n",
								de.u.Exception.ExceptionRecord.ExceptionCode,
								de.u.Exception.ExceptionRecord.ExceptionAddress,
								de.u.Exception.ExceptionRecord.ExceptionInformation[0] == 0 ? "read" : (de.u.Exception.ExceptionRecord.ExceptionInformation[0] == 1 ? "write" : "hack"),
								de.u.Exception.ExceptionRecord.ExceptionInformation[1]);
						}
						else if(couldBeSerious)
						{
							debugprintf("exception 0x%X (at 0x%08X): %s\n",
								de.u.Exception.ExceptionRecord.ExceptionCode,
								de.u.Exception.ExceptionRecord.ExceptionAddress,
								ExceptionCodeToDescription(de.u.Exception.ExceptionRecord.ExceptionCode));
						}

						//HANDLE hProcess = GetProcessHandle(processInfo,de); 

						if(couldBeSerious)
						{
							// print the callstack of the exception thread
							std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
							if(found != hGameThreads.end())
							{
								HANDLE hThread = found->second;
								//THREADSTACKTRACE(hThread);
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
								THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
								//THREADSTACKTRACEMSG(hThread, msg, (found->second).hProcess);
							}
						}

						if(couldBeSerious && !de.u.Exception.dwFirstChance)
						{
							// print the callstack of all other threads

							int numThreads = gameThreadIdList.size();
							for(int i = 0; i < numThreads; i++)
							{
								DWORD threadId = gameThreadIdList[i];
								if(threadId == de.dwThreadId)
									continue;
								ThreadInfo& info = hGameThreads[threadId];
								HANDLE hThread = info.handle;
								char msg [16+72];
								sprintf(msg, "(id=0x%X) (name=%s)", threadId, info.name);
								THREADSTACKTRACEMSG(hThread, msg, /*info.hProcess*/hGameProcess);
							}

							//std::map<DWORD,ThreadInfo>::iterator iter;
							//for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
							//{
							//	if(iter->first == de.dwThreadId)
							//		continue;
							//	HANDLE hThread = iter->second;
							//	//THREADSTACKTRACE(hThread);
							//	char msg [16+72];
							//	sprintf(msg, "(id=0x%X) (name=%s)", iter->first, iter->second.name);
							//	THREADSTACKTRACEMSG(hThread, msg);
							//}
						}
					  }
				  }
				  

					//if(!(de.u.Exception.ExceptionRecord.ExceptionFlags & EXCEPTION_NONCONTINUABLE)
					////&& de.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_ACCESS_VIOLATION
					//&& de.u.Exception.dwFirstChance
					if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
					{
						// debug breakpoint, maybe user just hit F12 or we just attached
						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
					}
					else if(de.u.Exception.ExceptionRecord.ExceptionCode == THREAD_NAME_EXCEPTION)
					{
						// game is giving us information about a thread's purpose

						THREADNAME_INFO* nameInfo = (THREADNAME_INFO*)de.u.Exception.ExceptionRecord.ExceptionInformation;
						if(nameInfo && nameInfo->dwType == 0x1000)
						{
							DWORD threadId = nameInfo->dwThreadID;
							if(threadId == -1)
								threadId = de.dwThreadId;
							std::map<DWORD,ThreadInfo>::iterator threadInfoIter = hGameThreads.find(threadId);
							if(threadInfoIter != hGameThreads.end())
							{
								ThreadInfo threadInfo = threadInfoIter->second;
								char oldName[64];
								strcpy(oldName, threadInfo.name);
								SIZE_T bytesRead = 0;
								ReadProcessMemory(/*threadInfo.hProcess*/hGameProcess, (void*)nameInfo->szName, threadInfo.name, 64, &bytesRead);
								threadInfo.name[63] = '\0';
								ASSERT(threadInfo.handle);
								hGameThreads[threadId] = threadInfo;
								if(*oldName)
									debugprintf("got name for thread id=0x%X: \"%s\" (was \"%s\")\n", threadId, threadInfo.name, oldName);
								else
									debugprintf("got name for thread id=0x%X: \"%s\"\n", threadId, threadInfo.name);
//								if(strstr(threadInfo.name, "FrameThread"))
//									hasMainThread = true;
							}
							else
							{
								ThreadInfo threadInfo;
								SIZE_T bytesRead = 0;
								HANDLE hProcess = GetProcessHandle(processInfo,de);
								ReadProcessMemory(hProcess, (void*)nameInfo->szName, threadInfo.name, 64, &bytesRead);
								threadInfo.name[63] = '\0';
								debugprintf("got name for as-yet-invalid thread id=0x%X: \"%s\"\n", threadId, threadInfo.name);
								danglingThreadNameId = threadId;
								strcpy(danglingThreadName, threadInfo.name);
//								if(strstr(threadInfo.name, "FrameThread"))
//									hasMainThread = true;
							}
						}

						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
					}
					else if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE)
					{
						// invalid handle? (internal directsound error?)
						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
					}
					else if(de.u.Exception.dwFirstChance && de.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_NONCONTINUABLE_EXCEPTION)
					{
						// unknown, could be a crash but we'll wait to see if something else handles it
						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
					}
					else if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) // 0xC0000094
					{
						// divide by zero... let's wait for a more serious error (required for ikachan...) (actually, not anymore, must have fixed something else, might as well leave this error correction code here though)
						
						bool continuing = false;
						CONTEXT context = {0};
						context.ContextFlags = CONTEXT_ALL;
						std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
						if(found != hGameThreads.end())
						{
							if(GetThreadContext(found->second.handle, &context))
							{
								char bytes [32] = {0};
								SIZE_T bytesRead = 0;
								if(ReadProcessMemory(/*(found->second).hProcess*/hGameProcess, (void*)context.Eip, bytes, 32, &bytesRead))
								{
									int divInsLen = instructionLength((const unsigned char*)bytes);
									context.Eip += divInsLen;
									if(SetThreadContext(found->second.handle, &context))
									{
										continuing = true;
									}
								}
							}
						}



						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuing ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED);
						continue;
					}
					else // actual crash:
					{
						bool skipDialog = false;
						////if((!postDllMainDone || s_lastFrameCount < 4) && !runDllLast)
						//{
						//	// this is so RotateGear can run
						//	int result = IDYES;
						//	if(movie.version >= 66)
						//	{
						//		result = CustomMessageBox("The game failed to start up.\n"
						//			"\n"
						//			"Sometimes this can be fixed by changing your settings or using a different OS,\n"
						//			"but usually this means the game is not supported yet.\n"
						//			"\n"
						//			"In certain specific games (such as RotateGear),\n"
						//			"this is expected to happen because of DLL loading order problems.\n"
						//			"Do you want to try restarting the game with a different loading order?\n"
						//			"(If this is not one of those specific games, this can cause desyncs.)", "CRASH",
						//			MB_YESNO|MB_DEFBUTTON2|MB_ICONERROR);
						//		skipDialog = true;
						//	}
						//	if(result == IDYES)
						//	{
						//		debugprintf("the game crashed, but...\n");
						//		debugprintf("assuming crash is due to some bug in IATModifier::writeIAT. retrying with alternate method.\n");
						//		runDllLast = true;
						//
						//		//CloseHandle(hGameProcess);
						//		//hGameProcess = 0;
						//		//TerminateProcess(hGameProcess, -1);
						//
						//		LeaveCriticalSection(&g_processMemCS);
						//		//HANDLE hProcess = GetProcessHandle(processInfo,de); 
						//		//DebuggerThreadFuncCleanup(processInfo.hThread, hProcess);
						//
						//		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
						//		SafeTerminateProcess(processInfo);
						//
						//		goto restartgame;
						//	}
						//}


						//if(hasMainThread)
						//{
						//	std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
						//	if(found != hGameThreads.end())
						//	{
						//		ThreadInfo threadInfo = found->second;
						//		bool isSecondaryThread = false;
						//		if(!threadInfo.name || !strstr(threadInfo.name, "FrameThread"))
						//		{
						//			isSecondaryThread = true;
						//		}
						//		if(isSecondaryThread)
						//		{
						//			//debugprintf("The thread crashed... suspending %s\n", threadInfo.name);
						//			//SuspendThread(threadInfo.handle);
						//
						//			debugprintf("The thread crashed... %s\n", threadInfo.name);
						//			bool continuing = false;
						//			CONTEXT context = {0};
						//			context.ContextFlags = CONTEXT_ALL;
						//			if(GetThreadContext(threadInfo.handle, &context))
						//			{
						//				char bytes [32] = {0};
						//				SIZE_T bytesRead = 0;
						//				if(ReadProcessMemory(hGameProcess, (void*)context.Eip, bytes, 32, &bytesRead))
						//				{
						//					int divInsLen = instructionLength((const unsigned char*)bytes);
						//					context.Eip += divInsLen;
						//					if(SetThreadContext(threadInfo.handle, &context))
						//					{
						//						continuing = true;
						//					}
						//				}
						//			}
						//
						//			EnterCriticalSection(&g_processMemCS);
						//			runningNow = true;
						//			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuing ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED);
						//			continue;
						//		}
						//	}
						//}

						{
							static const char* mainmsg = "The game crashed...\n";
							static const char* waitskipmsg = "If this happens when fast-forwarding, try disabling \"Wait Skip\" in \"Misc > Fast-Forward Options\".\n";
							static const char* multidisablemsg = "If this happens sometimes randomly, try switching Multithreading to \"Disable\" in \"Misc > Multithreading Mode\".\n";
							static const char* dsounddisablemsg = "Or choose \"Sound > Disable DirectSound Creation\" if the game doesn't work without Multithreading on.\n";
							static const char* dsounddisablemsg2 = "If this happens sometimes randomly, try choosing \"Sound > Disable DirectSound Creation\".\n";
							char msg [8096];
							sprintf(msg, "%s%s%s%s%s", mainmsg,
								(localTASflags.fastForward && (localTASflags.fastForwardFlags & FFMODE_WAITSKIP)) ? waitskipmsg : "",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && !(localTASflags.aviMode & 2)) ? multidisablemsg : "",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && !(localTASflags.aviMode & 2) && !(localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)) ? dsounddisablemsg : "",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && (localTASflags.aviMode & 2) && !(localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)) ? dsounddisablemsg2 : ""
							);
							debugprintf(msg);
#ifndef _DEBUG
							if(!skipDialog)
								CustomMessageBox(msg, "CRASH", MB_OK|MB_ICONERROR);
#endif
							recoveringStale = false;
						}
						requestedCommandReenter = false; // maybe fixes something?
						if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
						{
							if(unsavedMovieData)
							{
								SaveMovie(moviefilename);
							}

							goto done;
						}

						EnterCriticalSection(&g_processMemCS);
						runningNow = true;
						ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
					}
					continue;
				}
#pragma endregion
			}
			break;

			// actually something a normal debugger might do:
			case LOAD_DLL_DEBUG_EVENT:
				{
#pragma region LOAD_DLL_DEBUG_EVENT
//					bool neverLoaded = dllBaseToHandle.find(de.u.LoadDll.lpBaseOfDll) == dllBaseToHandle.end();
					//bool neverLoaded = dllBaseToFilename.find(de.u.LoadDll.lpBaseOfDll) == dllBaseToFilename.end();
//					if(!dllBaseToHandle[de.u.LoadDll.lpBaseOfDll])
//					debugprintf("LOAD_DLL_DEBUG_EVENT: 0x%X\n", de.u.LoadDll.lpBaseOfDll);
					if(dllBaseToFilename[de.u.LoadDll.lpBaseOfDll].length() == 0)
					{
						char filename [MAX_PATH+1];
						GetFileNameFromFileHandle(de.u.LoadDll.hFile, filename);
						debugprintf("LOADED DLL: %s\n", filename);
						HANDLE hProcess = GetProcessHandle(processInfo,de);
						RegisterModuleInfo(de.u.LoadDll.lpBaseOfDll, hProcess, filename);
						AddAndSendDllInfo(filename, true, hProcess);

#if 0 // DLL LOAD CALLSTACK PRINTING

						int minDepth = 0, maxDepth = -1;
						// print the callstack of the thread
						std::map<DWORD,ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
						if(found != hGameThreads.end())
						{
							HANDLE hThread = found->second;
							char msg [16+72];
							sprintf(msg, "(id=0x%X) (name=%s)", found->first, found->second.name);
							StackTraceOfDepth(hThread, msg, minDepth, maxDepth, /*(found->second).hProcess*/hGameProcess);
						}
#endif

						//dllBaseToHandle[de.u.LoadDll.lpBaseOfDll] = de.u.LoadDll.hFile;
						dllBaseToFilename[de.u.LoadDll.lpBaseOfDll] = filename;
						//if(neverLoaded)
							LOADSYMBOLS2(hGameProcess, filename, de.u.LoadDll.hFile, de.u.LoadDll.lpBaseOfDll);

						// apparently we have to close it here.
						// waiting until UNLOAD_DLL_DEBUG_EVENT could make us close an invalid handle
						// which causes subtle bugs like EXCEPTION_INVALID_HANDLE exceptions with threads much later
						CloseHandle(de.u.LoadDll.hFile);
					}
#pragma endregion
				}
				break;
			case UNLOAD_DLL_DEBUG_EVENT:
				{
#pragma region UNLOAD_DLL_DEBUG_EVENT
					//char filename [MAX_PATH+1];
					//HANDLE hFile = dllBaseToHandle[de.u.UnloadDll.lpBaseOfDll];
					//GetFileNameFromFileHandle(hFile, filename);
					const char* filename = dllBaseToFilename[de.u.UnloadDll.lpBaseOfDll].c_str();
					debugprintf("UNLOADED DLL: %s\n", filename);
					HANDLE hProcess = GetProcessHandle(processInfo,de);
					UnregisterModuleInfo(de.u.UnloadDll.lpBaseOfDll, hProcess, filename);
					AddAndSendDllInfo(filename, false, hProcess);
					//dllBaseToHandle[de.u.LoadDll.lpBaseOfDll] = NULL;
					dllBaseToFilename[de.u.UnloadDll.lpBaseOfDll] = "";
					//debugprintf("CLOSE HANDLE( THREAD:?): fhandle=0x%X\n", hFile);
					//CloseHandle(hFile);
#pragma endregion
				}
				break;

			// keep track of which threads are running,
			// so we don't have to do some tricky thing to enumerate active threads
			// when it comes time to save or load them
			case CREATE_THREAD_DEBUG_EVENT:
				{
#pragma region CREATE_THREAD_DEBUG_EVENT
					debugprintf("STARTED THREAD: id=0x%X, handle=0x%X\n", de.dwThreadId, de.u.CreateThread.hThread);

			// print the status and callstack of all threads
			std::map<DWORD,ThreadInfo>::iterator iter;
			for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
			{
				DWORD threadId = iter->first;
				ThreadInfo threadInfo = iter->second;
				HANDLE hThread = threadInfo.handle;
				int suspendCount = GetThreadSuspendCount(threadInfo.handle);
				ASSERT(suspendCount == GetThreadSuspendCount(threadInfo.handle));
				//int waitCount = threadInfo.waitingCount;
				//debugprintf("thread status: id=0x%X, handle=0x%X, suspend=%d, wait=%d, name=%s\n", threadId, hThread, suspendCount, waitCount, threadInfo.name);
				debugprintf("thread status: id=0x%X, handle=0x%X, suspend=%d, name=%s\n", threadId, hThread, suspendCount, threadInfo.name);
				//THREADSTACKTRACE(hThread);
				char msg [16+72];
				sprintf(msg, "(id=0x%X) (name=%s)", threadId, threadInfo.name);
				THREADSTACKTRACEMSG(hThread, msg, /*threadInfo.hProcess*/hGameProcess);
			}

					//ThreadInfo newThreadInfo;
					//newThreadInfo.handle = de.u.CreateThread.hThread;
					//newThreadInfo.waitingCount = 0;
					hGameThreads[de.dwThreadId] = de.u.CreateThread.hThread;
					ASSERT(hGameThreads[de.dwThreadId].handle);
					HANDLE hProcess = GetProcessHandle(processInfo,de);
					hGameThreads[de.dwThreadId].hProcess = hProcess;
					gameThreadIdList.push_back(de.dwThreadId);
					usedThreadMode = localTASflags.threadMode;
					//if(threadMode == 3)
					//{
					//	SuspendThread(de.u.CreateThread.hThread);
					//}
					if(danglingThreadNameId == de.dwThreadId)
						strcpy(hGameThreads[de.dwThreadId].name, danglingThreadName);
					danglingThreadNameId = -1;
					ASSERT(hGameThreads[de.dwThreadId].handle);
#pragma endregion
				}
				break;
			case EXIT_THREAD_DEBUG_EVENT:
				{
#pragma region EXIT_THREAD_DEBUG_EVENT
					HandleThreadExitEvent(de, processInfo);
#pragma endregion
				}
				break;

			case CREATE_PROCESS_DEBUG_EVENT:
				{
#pragma region CREATE_PROCESS_DEBUG_EVENT
					char filename [MAX_PATH+1];

					// hFile is NULL sometimes...
					if((movie.version >= 0 && movie.version < 40) || !GetFileNameFromProcessHandle(de.u.CreateProcessInfo.hProcess, filename))
						GetFileNameFromFileHandle(de.u.CreateProcessInfo.hFile, filename);

//					debugprintf("CREATE_PROCESS_DEBUG_EVENT: 0x%X\n", de.u.CreateProcessInfo.lpBaseOfImage);
					debugprintf("CREATED PROCESS: %s\n", filename);
					strcpy(subexefilename, filename);
					RegisterModuleInfo(de.u.CreateProcessInfo.lpBaseOfImage, de.u.CreateProcessInfo.hProcess, filename);
					hGameThreads[de.dwThreadId] = de.u.CreateProcessInfo.hThread;
					ASSERT(hGameThreads[de.dwThreadId].handle);
					hGameThreads[de.dwThreadId].hProcess = de.u.CreateProcessInfo.hProcess;
					gameThreadIdList.push_back(de.dwThreadId);

					entrypoint = (int)de.u.CreateProcessInfo.lpStartAddress;
					if(entrypoint && movie.version >= 70)
					{
						AddBreakpoint(entrypoint, de.dwThreadId, de.u.CreateProcessInfo.hProcess);
						debugprintf("entrypoint = 0x%X\n", entrypoint);
					}

//					if(threadMode == 3)
//					{
//						debugprintf("MAIN THREAD: id=0x%X, handle=0x%X\n", de.dwThreadId, de.u.CreateProcessInfo.hThread);
//						if(!s_lastThreadSwitchedTo)
//							s_lastThreadSwitchedTo = de.dwThreadId;
////						SuspendThread(de.u.CreateProcessInfo.hThread);
//					}

					bool nullFile = !de.u.CreateProcessInfo.hFile && !(movie.version >= 0 && movie.version < 40);
					if(nullFile)
						de.u.CreateProcessInfo.hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

					if(de.dwProcessId == processInfo.dwProcessId)
					{
						// even if we don't have symbols for it,
						// this still lets us see the exe name
						// next to the addresses of functions it calls.
						LOADSYMBOLS2(hGameProcess, filename, de.u.CreateProcessInfo.hFile, de.u.CreateProcessInfo.lpBaseOfImage);
						//CloseHandle(de.u.CreateProcessInfo.hProcess);
					}
					else
					{
						// TODO:
						//// hook the game process' child process too
						//InjectDll(de.u.CreateProcessInfo.hProcess, dllpath);

						processInfo.hProcess = de.u.CreateProcessInfo.hProcess;
						processInfo.dwProcessId = de.dwProcessId;
						processInfo.dwThreadId = de.dwThreadId;
						processInfo.hThread = de.u.CreateProcessInfo.hThread;

						//EXTENDEDTRACEUNINITIALIZE(hProcess);
//						CloseHandle(hGameProcess);
						hGameProcess = processInfo.hProcess;
						debugprintf("switched to child process, handle 0x%X, PID = %d.\n", hGameProcess, processInfo.dwProcessId);
						PrintPrivileges(hGameProcess);
						EXTENDEDTRACEINITIALIZEEX( NULL, hGameProcess );
						LOADSYMBOLS2(hGameProcess, filename, de.u.CreateProcessInfo.hFile, de.u.CreateProcessInfo.lpBaseOfImage);
						debugprintf("attempting injection...\n");
						InjectDll(processInfo.hProcess, processInfo.dwProcessId, processInfo.hThread, processInfo.dwThreadId, dllpath, runDllLast!=0);
						debugprintf("done injection. continuing...\n");
					}

					allProcessInfos.push_back(processInfo);

					if(nullFile && de.u.CreateProcessInfo.hFile)
					{	// TODO: WHY?
						if(de.u.CreateProcessInfo.hFile != INVALID_HANDLE_VALUE)
							CloseHandle(de.u.CreateProcessInfo.hFile);
						de.u.CreateProcessInfo.hFile = NULL;
					}
#pragma endregion
				}
				CloseHandle(de.u.CreateProcessInfo.hFile);
				break;		
			case EXIT_PROCESS_DEBUG_EVENT:
				//debugprintf("got EXIT_PROCESS_DEBUG_EVENT \n");
				{
#pragma region EXIT_PROCESS_DEBUG_EVENT
					// TODO: handle closing child processes without closing the main one
					// for now we assume we're all done whenever any process or subprocess exits.

					if(unsavedMovieData)
					{
						SaveMovie(moviefilename);
					}

					//UnregisterModuleInfo(de.u.ExitProcess.lpBaseOfImage, hProcess, filename);

					//ContinueDebugEvent(de.dwProcessId, 0, DBG_CONTINUE); //DBG_TERMINATE_PROCESS
					//goto done;

					// keep track of which processes have exited, and set 'terminated' to true if all of them have.
					// we do this because, as a new "feature" of Vista and Windows 7,
					// the application will remain frozen if we don't call ContinueDebugProcess after its EXIT_PROCESS_DEBUG_EVENT,
					// so we can't exit the debugger loop until that has been done for each process that got created.
					for(unsigned int i = 0; i < allProcessInfos.size(); i++)
					{
						if(allProcessInfos[i].dwProcessId == de.dwProcessId)
						{
							allProcessInfos.erase(allProcessInfos.begin() + i);
							break;
						}
					}
					if(allProcessInfos.empty())
						terminated = true;
#pragma endregion
				}
				break;
			case RIP_EVENT: // I've never encountered a rip event, but I might as well log it in case it happens.
				debugprintf("RIP: err=0x%X, type=0x%X\n", de.u.RipInfo.dwError, de.u.RipInfo.dwType);
				break;
			}

			//// sometimes we fail to get a CREATE_THREAD_DEBUG_EVENT
			//// about a thread that keeps running and sending us messages.
			//// that can't be good, so ask the user if they't like to restart the game when it happens.
			//if(!askedToRestartAboutBadThreadId
			//&& de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT
			//&& de.dwDebugEventCode != EXIT_THREAD_DEBUG_EVENT
			//&& displayedFrameCount < 100)
			//{
			//	static int test = 1;
			//	if(hGameThreads.find(de.dwThreadId) == hGameThreads.end())
			//	{
			//		test = 0;
			//		askedToRestartAboutBadThreadId = false;
			//		debugprintf("WARNING: MISSED CREATION OF THREAD 0x%X, SHOULD RESTART GAME.\n", de.dwThreadId);
			//		int result = CustomMessageBox("The game failed to report some potentially important information.\nUsually this problem won't happen next time if you start the game again.\nRestart the game now?", "Problem", MB_YESNO | MB_ICONWARNING);
			//		if(result == IDYES)
			//		{
			//			SuspendAllExcept(de.dwThreadId);
			//			TerminateProcess(hGameProcess, -1);
			//			hGameThreads.clear();
			//			gameHWnds.clear();
			//			started = false;
			//			mainMenuNeedsRebuilding = true;
			//			paused = false;
			//			playback = false;
			//			EXTENDEDTRACEUNINITIALIZE();
			//			goto restartgame;
			//		}
			//	}
			//}

			EnterCriticalSection(&g_processMemCS);
			runningNow = true;
			//debugprintf("ContinueDebugEvent(0x%X, 0x%X, 0x%X)\n", de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
		}
		else
		{
//			// TODO: get rid of this stuckcount stuff, I don't think it's used anymore
//			threadStuckCount++;
//			if(threadStuckCount > 15000) // FIXME: should be never
//			{
//#ifdef DEBUG_THREADS
//				debugprintf("emergency(id=0x%X) ", s_lastThreadSwitchedTo);
//#endif
//			}
//#if defined(_DEBUG) //&& 0
//			// print the callstack of any should-be-running (but possibly-stuck) threads
//			if(threadStuckCount == 15000)
//			{
//				debugprintf("stuck?\n");
//				std::map<DWORD,ThreadInfo>::iterator iter;
//				for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
//				{
//					HANDLE hThread = iter->second;
//					if(GetThreadSuspendCount(hThread) != 0)
//						continue;
//					THREADSTACKTRACE(hThread, /*(iter->second).hProcess*/hGameProcess);
//				}
//			}
//#endif
			//Update_Input(hWnd, false, true);

			//debugprintf("WaitForDebugEvent. timed out. waiting...\n");

			CheckHotkeys(-1, false);
			Sleep(5);
		}
	}

done:
	//debugprintf("DONE\n");
	runningNow = false; 
	LeaveCriticalSection(&g_processMemCS);

//	if(hGameProcess)
//	{
//		SafeTerminateProcess(processInfo);
//		//debugprintf("Closing process handle 0x%X\n", hGameProcess);
//		//CloseHandle(hGameProcess);
//	}
	hGameProcess = 0;
earlyAbort:
	DebuggerThreadFuncCleanup(hInitialThread, processInfo.hProcess);
//	CloseHandle(processInfo.hProcess);

	// certain cleanup tasks need to happen after this thread has exited,
	// because either they might take too long, or they enable things in the UI
	// that aren't safe to click on until after this thread is really completely gone
	afterDebugThreadExit = true;

//	TerminateThread(GetCurrentThread(), 0); // hack for Vista (on Vista this thread just freezes if we try to return instead)
//	return 0; // this works fine on Windows XP but not on Vista?
	return 0;
}


HANDLE hAfterDebugThreadExitThread = NULL;

static DWORD WINAPI AfterDebugThreadExitThread(LPVOID lpParam)
{
	// clear out savestate memory (it's useless now anyway)
	for(int i = 0; i < maxNumSavestates; i++)
		savestates[i].Deallocate();

	// and clear out some other memory
	ClearAndDeallocateContainer(dllBaseToFilename);
	{
		AutoCritSect cs(&g_gameHWndsCS);
		ClearAndDeallocateContainer(gameHWnds);
	}
	ClearAndDeallocateContainer(hGameThreads);
	ClearAndDeallocateContainer(gameThreadIdList);
	ClearAndDeallocateContainer(trustedModuleInfos);
	ClearAndDeallocateContainer(injectedDllModuleInfo.path);
	ClearAndDeallocateContainer(allProcessInfos);

	DeallocateRamSearch();

	EnableDisablePlayRecordButtons(hWnd);
	CheckDialogChanges(0); // update dialog to reflect movie being closed

	hAfterDebugThreadExitThread = NULL;

	return 0;
}

void OnAfterDebugThreadExit()
{
	if(hAfterDebugThreadExitThread)
		return;

	afterDebugThreadExit = false;

	// let's run this cleanup code on a separate thread to keep the rest of the ui responsive
	// in case it happens to take a long time (it shouldn't, but who knows when it comes to deallocating memory)
	hAfterDebugThreadExitThread = CreateThread(NULL, 0, AfterDebugThreadExitThread, NULL, 0, NULL);
}


void TerminateDebuggerThread(DWORD maxWaitTime)
{
	if(!debuggerThread)
		return;
	HANDLE hDebuggerThread = debuggerThread;
	terminateRequest = true;
rewait:
	int prevTime = timeGetTime();
	while(debuggerThread && ((timeGetTime() - prevTime) < maxWaitTime))
	{
		Sleep(10);

		// we have to handle messages here otherwise we could get in a deadlock situation
		// where the debugger thread is waiting for us to respond to a message (like SetWindowText)
		// while we're waiting here for it to finish.
		MSG msg;
		int count = 0;
		while(count < 4 && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
			count++;
		}
	}
	if(debuggerThread)
	{
		int result = CustomMessageBox("The game-controlling thread is taking longer than expected to exit.\nClick Cancel to terminate it immediately (this might cause problems),\nor click Retry to check again and wait a few more seconds if needed.", "Stopping Game", MB_RETRYCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING);
		if(result == IDRETRY)
		{
			if(maxWaitTime < 4000)
				maxWaitTime = 4000;
			maxWaitTime += 1000;
			goto rewait;
		}
	}
	if(debuggerThread || WaitForSingleObject(hDebuggerThread, 100) == WAIT_TIMEOUT)
	{
		//debugprintf("WARNING: had to force terminate debugger thread after %d seconds\n", (timeGetTime() - prevTime)/1000);
		//if(IsDebuggerPresent())
		//{
		//	_asm{int 3} // please check to see where the DebuggerThreadFunc thread is frozen
		//}
		TerminateThread(hDebuggerThread, -1);
		DebuggerThreadFuncCleanup(INVALID_HANDLE_VALUE, hGameProcess);
		debuggerThread = 0;
		afterDebugThreadExit = true;
	}
}

void WaitForOtherThreadToTerminate(HANDLE& thread, DWORD maxWaitTime)
{
	if(!thread)
		return;
	HANDLE hThread = thread;
rewait:
	int prevTime = timeGetTime();
	while(thread && ((timeGetTime() - prevTime) < maxWaitTime))
	{
		Sleep(10);

		// we have to handle messages here otherwise we could get in a deadlock situation
		// where the other thread is waiting for us to respond to a message (like SetWindowText)
		// while we're waiting here for it to finish.
		MSG msg;
		int count = 0;
		while(count < 4 && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
			count++;
		}
	}
	if(thread)
	{
		int result = CustomMessageBox("A thread is taking longer than expected to exit.\nClick Cancel to terminate it immediately (this might cause problems),\nor click Retry to check again and wait a few more seconds if needed.", "Stopping", MB_RETRYCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING);
		if(result == IDRETRY)
		{
			if(maxWaitTime < 4000)
				maxWaitTime = 4000;
			maxWaitTime += 1000;
			goto rewait;
		}
	}
	if(thread || WaitForSingleObject(hThread, 100) == WAIT_TIMEOUT)
	{
		TerminateThread(hThread, -1);
		thread = NULL;
	}
}


void PrepareForExit()
{
	//unatexit(PrepareForExit);
	static bool already = false;
	if(already)
		return;
	already = true;

	if(localTASflags.aviMode) // hack, sometimes ~AviFrameQueue has trouble terminating the thread, no sense waiting for it if there's no avi and we're exiting anyway
	{
		CloseAVI();
	}
	if(unsavedMovieData)
	{
		SaveMovie(moviefilename);
	}
	Save_Config();
	TerminateDebuggerThread(6500);
	WaitForOtherThreadToTerminate(hAfterDebugThreadExitThread, 5000);
}


HMENU MainMenu = NULL;


// this is to get rid of dialog box beeps when pressing hotkeys,
// and also to make the edit boxes a little nicer to use
bool PreTranslateMessage(MSG& msg)
{
	if(IsHotkeyPress())
		return false;

	if(msg.message == WM_KEYDOWN)
	{
		char text[5];
		if(GetClassNameA(msg.hwnd, text, 5) && !_stricmp(text, "Edit"))
		{
			if(msg.wParam == 'A' && (GetKeyState(VK_CONTROL)&0x8000)) // select all
			{
				SendMessage(msg.hwnd,EM_SETSEL,0,-1);
				return false;
			}
			if(msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE)
			{
				SetFocus(NULL);
				return false;
			}
		}
	}

	return true;
}

//unsigned char unpackedData [256];
//unsigned char packedData [32];
//void bitPack()
//{
//	for(int i = 0; i < 32; i++)
//		packedData[i] = 0;
//	for(int i = 0; i < 256; i+=8)
//		for(int j = 0; j < 8; j++)
//			if(unpackedData[i+j])
//				packedData[i<<3] |= 1<<j;
//}
//void bitUnpack()
//{
//	for(int i = 0; i < 32; i++)
//		for(int j = 0; j < 8; j++)
//			unpackedData[(i<<3)+j] = packedData[i] & (1<<j);
//}

// GetErrorMode is missing from kernel32.dll on Windows XP and earlier,
// so we can't call it even if we're compiling this on Vista or newer.
// but we can implement it using SetErrorMode.
#define GetErrorMode() GetErrorModeXP()
DWORD GetErrorModeXP()
{
	DWORD prev = SetErrorMode(0);
	SetErrorMode(prev);
	return prev;
}


//HACCEL hAccelTable = NULL;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	GetCurrentDirectory(MAX_PATH, thisprocessPath);

	// this is so we don't start the target with the debug heap,
	// which would cause all sorts of complaining exceptions we'd have to ignore
	// and would potentially change program behavior.
	SetEnvironmentVariable("_NO_DEBUG_HEAP", "1");
	SetEnvironmentVariable("__MSVCRT_HEAP_SELECT", "__GLOBAL_HEAP_SELECTED,3");

	// disable annoying system error dialog boxes (including for any games we start).
	// otherwise certain error cases that we handle
	// would unnecessarily present the user with some error dialogs to click through.
	SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS);
	SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
	SetErrorMode(GetErrorMode() | SEM_NOALIGNMENTFAULTEXCEPT);
	SetErrorMode(GetErrorMode() | SEM_NOOPENFILEERRORBOX);

	//InitializeCriticalSection(&s_aviCS);
	//InitializeCriticalSection(&s_fqaCS);
	//InitializeCriticalSection(&s_fqvCS);
	InitAVICriticalSections();
	InitializeCriticalSection(&g_processMemCS);
	//InitializeCriticalSection(&g_debugPrintCS);
	InitDebugCriticalSection();
	InitializeCriticalSection(&g_gameHWndsCS);

	NormalizePath(thisprocessPath, thisprocessPath);

	// DetectOS function call goes here!!
	DetectWindowsVersion(&(localTASflags.osVersionMajor), &(localTASflags.osVersionMinor));

//	GetVersionEx(&osvi);
	//if(!IsWindowsXP() && !IsWindows7())
	//{
	//	// HACK: disable a feature on systems not supported by the current implementation of MyKiUserCallbackDispatcher
	//	allowLoadInstalledDlls = true;
	//	allowLoadUxtheme = true;
	//}

	InitRamSearch();

	Load_Config();

	MSG msg;
	if (!InitInstance (hInstance, nCmdShow)) // this calls our DlgProc with WM_SHOWWINDOW
		return FALSE;
	//hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_WINTASER);

	Init_Input(hInst, hWnd);

	Build_Main_Menu(MainMenu, hWnd);

	PrintPrivileges(GetCurrentProcess());

	CheckDialogChanges(0);

//	atexit(PrepareForExit);

	// Main message loop:
	//BOOL bRet;
	//while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	while(true)
	{
		//if(bRet == -1)
		//{
		//	debugprintf("main loop GetMessage error! 0x%X.\n", GetLastError());
		//	break;
		//}

		//DWORD ret = MsgWaitForMultipleObjects(0, NULL, FALSE, started ? 1000 : 30, QS_ALLINPUT);

		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//if(!IsDialogMessage(hWnd, &msg))
			//&& !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
			{
				if(PreTranslateMessage(msg))
					TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if(msg.message == WM_QUIT)
				break;
		}
		else
		{
			//Sleep(started ? 1000 : 30);
			MsgWaitForMultipleObjects(0, NULL, FALSE, started ? 1000 : 30, QS_ALLINPUT);
		}

		//if(!debuggerThread && (GetAsyncKeyState('G') & 0x8000))
		//{
		//	debuggerThread = CreateThread(NULL, 0, DebuggerThreadFunc, NULL, 0, NULL);
		//}

		if(!started/* && GetActiveWindow() == hWnd*/)
		{
			//Update_Input(hWnd, false, true);
			CheckHotkeys(-1, /*false*/true); // let's count "before any frame" as framesynced as far as hotkeys
		}

		//// temp hack
		//if((GetAsyncKeyState('F') & 0x8000))
		//{
		//	UpdateGeneralInfoDisplay();
		//}
#if 0 // TODO: move to a hotkey. disabled for now.
		// temp hack so I can get the callstack for diagnosing game freezes or slowdown
		if((GetAsyncKeyState('T') & 0x8000))
		{
			bool entered = false;
			for(int i = 0; i < 100 && !entered; i++)
			{
				if(TryEnterCriticalSection(&g_processMemCS))
					entered = true;
				else
					Sleep(5);
			}
			// if we didn't enter, go ahead anyway (that's a lot better than freezing)
			if(!entered)
				debugprintf("warning: unable to acquire critical section, stack trace may be slightly off.\n");

			// print the status and callstack of all threads
			std::map<DWORD,ThreadInfo>::iterator iter;
			for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
			{
				DWORD threadId = iter->first;
				ThreadInfo threadInfo = iter->second;
				HANDLE hThread = threadInfo.handle;
				int suspendCount = GetThreadSuspendCount(threadInfo.handle);
				ASSERT(suspendCount == GetThreadSuspendCount(threadInfo.handle));
				//int waitCount = threadInfo.waitingCount;
				//debugprintf("thread status: id=0x%X, handle=0x%X, suspend=%d, wait=%d, name=%s\n", threadId, hThread, suspendCount, waitCount, threadInfo.name);
				debugprintf("thread status: id=0x%X, handle=0x%X, suspend=%d, name=%s\n", threadId, hThread, suspendCount, threadInfo.name);
				//THREADSTACKTRACE(hThread);
				char msg [16+72];
				sprintf(msg, "(id=0x%X) (name=%s)", threadId, threadInfo.name);
				THREADSTACKTRACEMSG(hThread, msg, /*threadInfo.hProcess*/hGameProcess);
			}
			// print the status again so it's all in a row
			for(iter = hGameThreads.begin(); iter != hGameThreads.end(); iter++)
			{
				DWORD threadId = iter->first;
				ThreadInfo threadInfo = iter->second;
				HANDLE hThread = threadInfo.handle;
				int suspendCount = GetThreadSuspendCount(threadInfo.handle);
				ASSERT(suspendCount == GetThreadSuspendCount(threadInfo.handle));
				//int waitCount = threadInfo.waitingCount;
				//debugprintf(" thread status: id=0x%X, handle=0x%X, suspend=%d, wait=%d, name=%s\n", threadId, hThread, suspendCount, waitCount, threadInfo.name);
				debugprintf(" thread status: id=0x%X, handle=0x%X, suspend=%d, name=%s\n", threadId, hThread, suspendCount, threadInfo.name);
			}
			if(entered)
				LeaveCriticalSection(&g_processMemCS);
		}
#endif
		//// temp hack for testing sync stability in the face of unpredictable windows messages
		//if((GetAsyncKeyState('Y') & 0x8000))
		//{
		//	static int msgNum = 0;
		//	msgNum++;
		//	if(msgNum > 0x220)
		//		msgNum = 0;
		//	while(msgNum == WM_CLOSE || msgNum == WM_QUIT || msgNum == WM_SETREDRAW)
		//		msgNum++;
		//	debugprintf("TESTING MSG: 0x%X\n", msgNum);
		//	if(!gameHWnds.empty())
		//	{
		//		std::set<HWND>::iterator iter;
		//		for(iter = gameHWnds.begin(); iter != gameHWnds.end();++iter)
		//		{
		//			HWND gamehwnd = *iter;
		//			PostMessage(gamehwnd, msgNum, rand(), rand());
		//		}
		//	}
		//}

		if(mainMenuNeedsRebuilding)
		{
			Build_Main_Menu(MainMenu, hWnd);
			mainMenuNeedsRebuilding = false;
		}

		if(afterDebugThreadExit)
			OnAfterDebugThreadExit();
	}

	PrepareForExit();

	return (int) msg.wParam;
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);

	if(!hWnd)
		return FALSE;

	SendMessage(hWnd, WM_SETICON, WPARAM(ICON_SMALL), LPARAM(LoadImage(hInst, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
	SendMessage(hWnd, WM_SETICON, WPARAM(ICON_BIG),   LPARAM(LoadImage(hInst, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR)));

	//hExternalWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	//   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	SetForegroundWindow(hWnd);
	UpdateWindow(hWnd);

	//#if 0
	//   // remote viewport, for now I only use this for debugging eversion savestates
	//   hExternalWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)ViewportDlgProc);
	//   SetWindowPos(hExternalWnd, NULL, 0,340, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	//   ShowWindow(hExternalWnd, nCmdShow);
	//   UpdateWindow(hExternalWnd);
	//#endif

	return TRUE;
}

static void EnableDisablePlayRecordButtons(HWND hDlg)
{
	bool writable = true;
	bool exists = true;
	bool exeexists = true;
	FILE* moviefile = *moviefilename ? fopen(moviefilename, "r+b") : NULL;
	if(moviefile)
		fclose(moviefile);
	else
	{
		moviefile = *moviefilename ? fopen(moviefilename, "rb") : NULL;
		if(moviefile)
			fclose(moviefile);
		else
			exists = false;

		moviefile = *moviefilename ? fopen(moviefilename, "ab") : NULL;
		if(moviefile)
		{
			fclose(moviefile);
			if(!exists)
				unlink(moviefilename);
		}
		else
			writable = false;
	}

	FILE* exefile = *exefilename ? fopen(exefilename, "rb") : NULL;
	if(exefile)
		fclose(exefile);
	else
		exeexists = false;

	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PLAY), exists && exeexists && !started);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RECORD), writable && exeexists && !started);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), started);

	if(exeFileExists != exeexists
	|| movieFileExists != exists
	|| movieFileWritable != writable)
	{
		exeFileExists = exeexists;
		movieFileExists = exists;
		movieFileWritable = writable;
		mainMenuNeedsRebuilding = true;
	}

	// preview data from movie
	if(exists && !started)
	{
		LoadMovie(moviefilename);
		
		UpdateFrameCountDisplay(movie.currentFrame, 1);

		char str [256];
		sprintf(str, "%u", localTASflags.initialTime);
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str);
		sprintf(str, "%d", localTASflags.framerate);
		SetWindowText(GetDlgItem(hDlg, IDC_EDIT_FPS), str);

		tasFlagsDirty = true;
	}
}

void BringGameWindowToForeground()
{
	if(!started)
		return;
	std::set<HWND>::iterator iter;
	EnterCriticalSection(&g_gameHWndsCS);
	for(iter = gameHWnds.begin(); iter != gameHWnds.end();)
	{
		HWND gamehwnd = *iter;
		DWORD style = (DWORD)GetWindowLong(gamehwnd, GWL_STYLE);
		iter++;
		if((style & (WS_VISIBLE|WS_CAPTION)) == (WS_VISIBLE|WS_CAPTION) || iter == gameHWnds.end())
		{
			RECT rect;
			GetClientRect(gamehwnd, &rect);
			if(rect.bottom - rect.top > 10)
			{
				LeaveCriticalSection(&g_gameHWndsCS);
				SetForegroundWindow(gamehwnd);
				return;
			}
		}
	}
	LeaveCriticalSection(&g_gameHWndsCS);
}

//BOOL CALLBACK ViewportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    BOOL rv = TRUE;  
//    switch(message)  
//    {  
//        case WM_INITDIALOG:  
//            break;
//
//		case WM_SHOWWINDOW:
//			break;
//
//		case WM_CLOSE:
//			{
//				SIZE_T bytesWritten = 0;
//				WriteProcessMemory(hGameProcess, (void*)0, &hWnd, sizeof(HWND), &bytesWritten);
//				EndDialog(hDlg, 0);
//				return true;
//			}
//            break;  
//
//		default:  
//            rv = FALSE;  
//            break;  
//    }  
// 
//    return rv;
//}

/*void DoSplice(const char* sourceFilename, unsigned int srcStartFrame, unsigned int srcEndFrame, unsigned int dstStartFrame, unsigned int dstEndFrame)
{
	Movie source = Movie();
	if(LoadMovieFromFile(source, sourceFilename, false, true) <= 0)
	{
		CustomMessageBox("Failed to load source movie file.\nNo changes were made.", "Splice Error", MB_OK | MB_ICONERROR, SpliceHWnd);
		return;
	}

	unsigned int numDstFrames = movie.frames.size();
	unsigned int numSrcFrames = source.frames.size();

	if(srcStartFrame >= numSrcFrames)
		srcStartFrame = 0;
	if(srcEndFrame > numSrcFrames)
		srcEndFrame = numSrcFrames;
	if(dstStartFrame > numDstFrames)
		dstStartFrame = 0;
	if(dstEndFrame > numDstFrames)
		dstEndFrame = numDstFrames;

	unsigned int src = srcStartFrame;
	unsigned int dst = dstStartFrame;
	while(src != srcEndFrame && dst != dstEndFrame)
	{
		//debugprintf("copying %d to %d\n", src, dst);
		movie.frames[dst++] = source.frames[src++];
	}
	if(src != srcEndFrame)
	{
		//debugprintf("copying %d-%d to %d\n", src, srcEndFrame, dst);
		movie.frames.insert(movie.frames.begin() + dst, source.frames.begin() + src, source.frames.begin() + srcEndFrame);
	}
	else if(dst != dstEndFrame)
	{
		//debugprintf("deleting %d-%d\n", dst, dstEndFrame);
		movie.frames.erase(movie.frames.begin() + dst, movie.frames.begin() + dstEndFrame);
	}

	unsaved = true;
}*/

void SplitToValidPath(const char* initialPath, const char* defaultDir, char* filename, char* directory);
BOOL SetWindowTextAndScrollRight(HWND hEdit, LPCSTR lpString);

/*LRESULT CALLBACK SpliceProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG: {

			RECT r;
			GetWindowRect(hWnd, &r);
			int dx1 = (r.right - r.left) / 2;
			int dy1 = (r.bottom - r.top) / 2;

			RECT r2;
			GetWindowRect(hDlg, &r2);
			int dx2 = (r2.right - r2.left) / 2;
			int dy2 = (r2.bottom - r2.top) / 2;

			// push it away from the main window if we can
			const int width = (r.right-r.left);
			const int height = (r.bottom - r.top);
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}
			//-------------------------------------------------------------------------------------
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			
			DragAcceptFiles(hDlg, TRUE);

			return true;
			//break;
		}
		
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_BUTTON_MOVIEBROWSE:
				{
					char filename [MAX_PATH+1] = {0};
					char directory [MAX_PATH+1] = {0};
					SplitToValidPath(moviefilename, thisprocessPath, filename, directory);
					NormalizePath(moviefilename, moviefilename);
					OPENFILENAME ofn = {sizeof(OPENFILENAME)};
					ofn.hwndOwner = hWnd;
					ofn.hInstance = hInst;
					ofn.lpstrFilter = "Windows TAS Files\0*.wtf;*.wtf2;*.wtf3;*.wtf4;*.wtf5;*.wtf6;*.wtf7;*.wtf8;*.wtf9;*.wtf*;*.hgm\0All Files\0*.*\0\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFile = filename;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrInitialDir = directory;
					ofn.lpstrTitle = "Choose Source Movie File";
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "wtf";
					if(GetOpenFileName(&ofn))
						SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), filename);
				}	return true;
				case IDOK:
					if(paused)
					{
						char filename [MAX_PATH+1] = {0};
						GetWindowText(GetDlgItem(hDlg, IDC_EDIT_MOVIE), filename, MAX_PATH);
						NormalizePath(filename, filename);

						char sourceStartFrameText [64];
						GetWindowText(GetDlgItem(hDlg, IDC_EDIT_SPLICESOURCESTART), sourceStartFrameText, sizeof(sourceStartFrameText));
						int sourceStartFrame = atoi(sourceStartFrameText);

						DoSplice(filename, sourceStartFrame-1, -1, movie.currentFrame, -1);
					}
					else
					{
						CustomMessageBox("The movie must already be paused on the frame you want to splice into.\nNo changes were made.", "Splice Error", MB_OK | MB_ICONERROR, SpliceHWnd);
					}
					//SpliceHWnd = NULL;
					//DragAcceptFiles(hDlg, FALSE);
					//EndDialog(hDlg, true);
					return true;
				case IDCANCEL:
					SpliceHWnd = NULL;
					DragAcceptFiles(hDlg, FALSE);
					EndDialog(hDlg, true);
					return true;
			}
			break;
		
		case WM_CLOSE:
			SpliceHWnd = NULL;
			DragAcceptFiles(hDlg, FALSE);
			EndDialog(hDlg, true);
			return true;

		case WM_DROPFILES:
		{
			char filename [MAX_PATH+1] = {0};
			HDROP hDrop = (HDROP)wParam;
			DragQueryFile(hDrop, 0, filename, MAX_PATH);
			DragFinish(hDrop);

			char* slash = max(strrchr(filename, '\\'), strrchr(filename, '/'));
			char* dot = strrchr(filename, '.');
			if(slash<dot)
				if(!_strnicmp(dot, ".wtf", 4) || !_strnicmp(dot, ".hgm", 4)) // windows TAS file (input movie)
					SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), filename);
		}	break;
	}

	return false;
}*/

BOOL DirectoryExists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}
void SplitToValidPath(const char* initialPath, const char* defaultDir, char* filename, char* directory)
{
	filename[0] = '\0';
	directory[0] = '\0';

	if(strlen(initialPath) > MAX_PATH)
		return;

	if(initialPath)
	{
		strcpy(filename, initialPath);
		strcpy(directory, initialPath);
	}

	while(!DirectoryExists(directory))
	{
		char* slash = max(strrchr(directory, '\\'), strrchr(directory, '/'));
		if(slash)
			*slash = '\0';
		else
		{
			if(defaultDir && strlen(defaultDir) <= MAX_PATH)
				strcpy(directory, defaultDir);
			if(!DirectoryExists(directory))
				*directory = '\0';
			break;
		}
	}

	if(!strncmp(filename, directory, strlen(directory)))
		memmove(filename, filename+strlen(directory), 1+strlen(filename)-strlen(directory));
	char* slash = max(strrchr(filename, '\\'), strrchr(filename, '/'));
	if(slash++)
		memmove(filename, slash, 1+strlen(slash));
}

BOOL SetWindowTextAndScrollRight(HWND hEdit, LPCSTR lpString)
{
	BOOL rv = SetWindowText(hEdit, lpString);
	SendMessage(hEdit, EM_SETSEL, -2, -1);
	return rv;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL rv = TRUE;  
    switch(message)  
    {  
        case WM_INITDIALOG:
			if(VERSION >= 0)
			{
				char title [256];
				sprintf(title, "Hourglass v%d", VERSION);
#ifdef _DEBUG
				strcat(title, "? (debug)");
#endif
				SetWindowTextA(hDlg, title);
			}
#ifdef _DEBUG
			else
			{
				SetWindowTextA(hDlg, "Hourglass (debug)");
			}
#endif
            break;

		case WM_SHOWWINDOW:
 			{
				char str [256];
				sprintf(str, "%d", localTASflags.framerate);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_FPS), str);

				localTASflags.initialTime = /*timeGetTime()*/6000;
				sprintf(str, "%d", localTASflags.initialTime);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SYSTEMCLOCK), str);

				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_CURFRAME), "0");
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_MAXFRAME), "0");
				SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MOVIETIME), "");

//				CheckDlgButton(hDlg, IDC_ALLOWHARDWAREACCEL, forceSoftware ? 0 : 1);
//				CheckDlgButton(hDlg, IDC_ALLOWFULLSCREEN, forceWindowed ? 0 : 1);
				//CheckDlgButton(hDlg, IDC_ALLOWDEACTIVATE, windowActivateFlags & 1);
				CheckDlgButton(hDlg, IDC_MAKETOPMOST, localTASflags.windowActivateFlags & 2);
				//CheckDlgButton(hDlg, IDC_EARLYINJECT, iatInjectionEnabled);
				//CheckDlgButton(hDlg, IDC_AVIVIDEO, aviMode & 1);
				//CheckDlgButton(hDlg, IDC_AVIAUDIO, aviMode & 2);
//				CheckDlgButton(hDlg, IDC_EMULATESOUND, emuMode & EMUMODE_EMULATESOUND);

				CheckDlgButton(hDlg, IDC_RADIO_THREAD_DISABLE, localTASflags.threadMode == 0);
				CheckDlgButton(hDlg, IDC_RADIO_THREAD_WRAP, localTASflags.threadMode == 1);
				CheckDlgButton(hDlg, IDC_RADIO_THREAD_ALLOW, localTASflags.threadMode == 2);

				char path [MAX_PATH+1];
				if(exefilename[0] == '\0')
				{
					path[0] = 0;
					//strcpy(path, thisprocessPath);
					//path[MAX_PATH-23] = 0;
 					//strcat(path, "\\..\\game\\Doukutsu.exe");
				}
				else
				{
					strcpy(path, exefilename);
				}
				AbsolutifyPath(path);
				//char temp_moviefilename [MAX_PATH+1];
				//strcpy(temp_moviefilename, moviefilename);
				movienameCustomized = false;
				SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), path);
				SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), strcmp(newmoviefilename, moviefilename) ? newmoviefilename : moviefilename);//temp_moviefilename);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_COMMANDLINE), commandline);
				movienameCustomized = false;
				SetFocus(GetDlgItem(hDlg, IDC_BUTTON_RECORD));

				EnableDisablePlayRecordButtons(hDlg);
			}
			break;

		case WM_ACTIVATE:
			UpdateFrameCountDisplay(s_lastFrameCount, 1);
			SetFocus(NULL);
			//EnableDisablePlayRecordButtons(hDlg); // disabled because it breaks being able to set fps or system clock on existing movie
			break;

        case WM_CLOSE:
			SendMessage(hDlg, WM_COMMAND, ID_FILES_STOP, 0);
			PrepareForExit();
			PostQuitMessage(0);
            break;

		case WM_DESTROY:
			hWnd = NULL;
			PrepareForExit();
			PostQuitMessage(0);
            break;


		//case WM_LBUTTONUP: // auto-switch to game window on left click on unimportant place
		//	if(started && !finished)
		//	{
		//		HWND curHWnd = GetForegroundWindow();
		//		if(curHWnd == hWnd)
		//		{
		//			//if(gameHWnds.find(curHWnd) == gameHWnds.end())
		//			{
		//				if(!gameHWnds.empty())
		//				{
		//					std::set<HWND>::iterator iter;
		//					for(iter = gameHWnds.begin(); iter != gameHWnds.end();)
		//					{
		//						HWND gamehwnd = *iter;
		//						DWORD style = (DWORD)GetWindowLong(gamehwnd, GWL_STYLE);
		//						iter++;
		//						if((style & (WS_VISIBLE|WS_CAPTION)) == (WS_VISIBLE|WS_CAPTION) || iter == gameHWnds.end())
		//						{
		//							// check to see if the windows are overlapping.
		//							// if they are, then auto-focusing the game window would be too annoying
		//							// unless it's already set as topmost.
		//							RECT rect1, rect2, rectIntersect;
		//							GetWindowRect(hWnd, &rect1);
		//							GetWindowRect(gamehwnd, &rect2);
		//							if(!IntersectRect(&rectIntersect, &rect1, &rect2) || (windowActivateFlags & 2))
		//							{
		//								SetForegroundWindow(gamehwnd);
		//							}
		//							break;
		//						}
		//					}
		//				}
		//			}
		//		}
		//	}
		//	break;

		//case WM_KEYDOWN:
		//case WM_KEYUP:
		//case WM_CHAR:
		//	return 0;

		case WM_COMMAND:
			{
				DWORD command = LOWORD(wParam);
				switch(command)
				{
				case ID_FILES_QUIT:
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					return 0;

				case ID_RAM_SEARCH:
					if(!RamSearchHWnd)
					{
						LRESULT CALLBACK RamSearchProc(HWND, UINT, WPARAM, LPARAM);
						RamSearchHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_RAMSEARCH), hWnd, (DLGPROC) RamSearchProc);
					}
					else
						SetForegroundWindow(RamSearchHWnd);
					break;

				case ID_RAM_WATCH:
					if(!RamWatchHWnd)
					{
						LRESULT CALLBACK RamWatchProc(HWND, UINT, WPARAM, LPARAM);
						RamWatchHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_RAMWATCH), hWnd, (DLGPROC) RamWatchProc);
					}
					else
						SetForegroundWindow(RamWatchHWnd);
					break;

				case ID_TOGGLE_MOVIE_READONLY:
					{
						nextLoadRecords = !nextLoadRecords;
						CheckDialogChanges(-1);
						//if(nextLoadRecords)
						//	debugprintf("switched to read+write\n");
						//else
						//	debugprintf("switched to read-only\n");
					}
					break;
				case IDC_RADIO_READONLY:
				case IDC_RADIO_READWRITE:
					{
						bool nonreadonly = IsDlgButtonChecked(hDlg, IDC_RADIO_READWRITE) != 0;
						if(nonreadonly != nextLoadRecords)
						{
							nextLoadRecords = nonreadonly;
							//if(nextLoadRecords)
							//	debugprintf("switched to read+write\n");
							//else
							//	debugprintf("switched to read-only\n");
						}
					}
					break;

				case ID_TIME_TOGGLE_PAUSE:
					if(started || paused || lParam != 777)
					{
						if(paused && started)
							SendMessage(hDlg, WM_COMMAND, ID_SWITCH_TO_TASEE_FROM_TASER, 0);
						paused = !paused;
						if(paused && localTASflags.fastForward)
							temporaryUnpause = true; // fixes something I can't remember
						CheckDialogChanges(-1);
					}
					break;
				case IDC_PAUSED:
					paused = IsDlgButtonChecked(hDlg, IDC_PAUSED) != 0;
					break;

				case ID_TIME_TOGGLE_FASTFORWARD:
					localTASflags.fastForward = !localTASflags.fastForward;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					mainMenuNeedsRebuilding = true;
					break;
				case IDC_FASTFORWARD:
					localTASflags.fastForward = IsDlgButtonChecked(hDlg, IDC_FASTFORWARD) != 0;
					tasFlagsDirty = true;
					break;

				//case IDC_EARLYINJECT:
				//	iatInjectionEnabled = IsDlgButtonChecked(hDlg, IDC_EARLYINJECT) != 0;
				//	break;
				//case IDC_ALLOWHARDWAREACCEL:
				//	forceSoftware = IsDlgButtonChecked(hDlg, IDC_ALLOWHARDWAREACCEL) == 0;
				//	tasFlagsDirty = true;
				//	break;
				case ID_GRAPHICS_FORCESOFTWARE:
					localTASflags.forceSoftware = !localTASflags.forceSoftware;
					tasFlagsDirty = true;
					break;
				//case IDC_ALLOWFULLSCREEN:
				//	forceWindowed = IsDlgButtonChecked(hDlg, IDC_ALLOWFULLSCREEN) == 0;
				//	tasFlagsDirty = true;
				//	break;
				case ID_GRAPHICS_ALLOWFULLSCREEN:
					localTASflags.forceWindowed = !localTASflags.forceWindowed;
					tasFlagsDirty = true;
					break;
				case ID_GRAPHICS_MEM_DONTCARE:
					localTASflags.forceSurfaceMemory = 0;
					tasFlagsDirty = true;
					break;
				case ID_GRAPHICS_MEM_SYSTEM:
					localTASflags.forceSurfaceMemory = 1;
					tasFlagsDirty = true;
					break;
				case ID_GRAPHICS_MEM_NONLOCAL:
					localTASflags.forceSurfaceMemory = 2;
					tasFlagsDirty = true;
					break;
				case ID_GRAPHICS_MEM_LOCAL:
					localTASflags.forceSurfaceMemory = 3;
					tasFlagsDirty = true;
					break;
				//case IDC_EMULATESOUND:
				//	emuMode = (emuMode & ~EMUMODE_EMULATESOUND) | (IsDlgButtonChecked(hDlg, IDC_EMULATESOUND) ? EMUMODE_EMULATESOUND : 0);
				//	if(!(emuMode & EMUMODE_EMULATESOUND))
				//	{
				//		aviMode &= ~2;
				//		CheckDlgButton(hDlg, IDC_AVIAUDIO, aviMode & 2);
				//	}
				//	tasFlagsDirty = true;
				//	break;
				case ID_SOUND_SOFTWAREMIX:
					localTASflags.emuMode ^= EMUMODE_EMULATESOUND;
					tasFlagsDirty = true;
					break;
				case IDC_CHECK_MUTE:
				case ID_SOUND_NOPLAYBUFFERS:
					localTASflags.emuMode ^= EMUMODE_NOPLAYBUFFERS;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					mainMenuNeedsRebuilding = true;
					break;
				case ID_SOUND_VIRTUALDIRECTSOUND:
					localTASflags.emuMode ^= EMUMODE_VIRTUALDIRECTSOUND;
					tasFlagsDirty = true;
					break;

				case ID_SOUND_RATE_8000:  localTASflags.audioFrequency = 8000;  tasFlagsDirty = true; break;
				case ID_SOUND_RATE_11025: localTASflags.audioFrequency = 11025; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_12000: localTASflags.audioFrequency = 12000; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_16000: localTASflags.audioFrequency = 16000; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_22050: localTASflags.audioFrequency = 22050; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_24000: localTASflags.audioFrequency = 24000; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_32000: localTASflags.audioFrequency = 32000; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_44100: localTASflags.audioFrequency = 44100; tasFlagsDirty = true; break;
				case ID_SOUND_RATE_48000: localTASflags.audioFrequency = 48000; tasFlagsDirty = true; break;
				case ID_SOUND_BITS_8:  localTASflags.audioBitsPerSecond = 8;  tasFlagsDirty = true; break;
				case ID_SOUND_BITS_16: localTASflags.audioBitsPerSecond = 16; tasFlagsDirty = true; break;
				case ID_SOUND_CHANNELS_1: localTASflags.audioChannels = 1; tasFlagsDirty = true; break;
				case ID_SOUND_CHANNELS_2: localTASflags.audioChannels = 2; tasFlagsDirty = true; break;

				//case IDC_ALLOWDEACTIVATE:
				//	windowActivateFlags = (windowActivateFlags & ~1) | (IsDlgButtonChecked(hDlg, IDC_ALLOWDEACTIVATE) ? 1 : 0);
				//	tasFlagsDirty = true;
				//	break;
				case IDC_MAKETOPMOST:
					localTASflags.windowActivateFlags = (localTASflags.windowActivateFlags & ~2) | (IsDlgButtonChecked(hDlg, IDC_MAKETOPMOST) ? 2 : 0);
					tasFlagsDirty = true;
					break;

				case ID_TIME_FF_FRONTSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_FRONTSKIP;
					tasFlagsDirty = true;
					break;
				case ID_TIME_FF_BACKSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_BACKSKIP;
					tasFlagsDirty = true;
					break;
				case ID_TIME_FF_SOUNDSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_SOUNDSKIP;
					tasFlagsDirty = true;
					break;
				case ID_TIME_FF_RAMSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_RAMSKIP;
					tasFlagsDirty = true;
					break;
				case ID_TIME_FF_SLEEPSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_SLEEPSKIP;
					tasFlagsDirty = true;
					break;
				case ID_TIME_FF_WAITSKIP:
					localTASflags.fastForwardFlags ^= FFMODE_WAITSKIP;
					tasFlagsDirty = true;
					break;

				case ID_TIME_RATE_100:
					localTASflags.timescale = 1;
					localTASflags.timescaleDivisor = 1;
					tasFlagsDirty = true;
					break;
				case ID_TIME_RATE_75:
					localTASflags.timescale = 3;
					localTASflags.timescaleDivisor = 4;
					tasFlagsDirty = true;
					break;
				case ID_TIME_RATE_50:
					localTASflags.timescale = 1;
					localTASflags.timescaleDivisor = 2;
					tasFlagsDirty = true;
					break;
				case ID_TIME_RATE_25:
					localTASflags.timescale = 1;
					localTASflags.timescaleDivisor = 4;
					tasFlagsDirty = true;
					break;
				case ID_TIME_RATE_12:
					localTASflags.timescale = 1;
					localTASflags.timescaleDivisor = 8;
					tasFlagsDirty = true;
					break;
				case ID_TIME_RATE_SLOWER:
					if(localTASflags.timescale == localTASflags.timescaleDivisor) {localTASflags.timescale = 3; localTASflags.timescaleDivisor = 4; tasFlagsDirty = true;}
					else if(localTASflags.timescale*4 == localTASflags.timescaleDivisor*3) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 2; tasFlagsDirty = true;}
					else if(localTASflags.timescale*2 == localTASflags.timescaleDivisor) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 4; tasFlagsDirty = true;}
					else if(localTASflags.timescale*4 == localTASflags.timescaleDivisor) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 8; tasFlagsDirty = true;}
					break;
				case ID_TIME_RATE_FASTER:
					if(localTASflags.timescale*4 == localTASflags.timescaleDivisor*3) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 1; tasFlagsDirty = true;}
					else if(localTASflags.timescale*2 == localTASflags.timescaleDivisor) {localTASflags.timescale = 3; localTASflags.timescaleDivisor = 4; tasFlagsDirty = true;}
					else if(localTASflags.timescale*4 == localTASflags.timescaleDivisor) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 2; tasFlagsDirty = true;}
					else if(localTASflags.timescale*8 == localTASflags.timescaleDivisor) {localTASflags.timescale = 1; localTASflags.timescaleDivisor = 4; tasFlagsDirty = true;}
					break;


				//case ID_EXEC_SUSPEND:
				//	SuspendAllExcept(lastKeyThreadId);
				//	break;
				//case ID_EXEC_RESUME:
				//	ResumeAllExcept(lastKeyThreadId);
				//	break;

				case ID_EXEC_USETRUEPAUSE:
					truePause = !truePause;
					break;
				case ID_EXEC_ONLYHOOKCHILDPROC:
					onlyHookChildProcesses = !onlyHookChildProcesses;
					break;

				case ID_EXEC_THREADS_DISABLE:
					localTASflags.threadMode = 0;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_THREADS_WRAP:
					localTASflags.threadMode = 1;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_THREADS_ALLOW:
					localTASflags.threadMode = 2;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_THREADS_KNOWN:
					localTASflags.threadMode = 3;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_THREADS_UNKNOWN:
					localTASflags.threadMode = 4;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_THREADS_TRUSTED:
					localTASflags.threadMode = 5;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;

				case ID_EXEC_TIMERS_DISABLE:
					localTASflags.timersMode = 0;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_TIMERS_SYNC:
					localTASflags.timersMode = 1;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_TIMERS_ASYNC:
					localTASflags.timersMode = 2;
					tasFlagsDirty = true;
					break;

				case ID_EXEC_MESSAGES_SYNC:
					localTASflags.messageSyncMode = 0;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_MESSAGES_SEMISYNC:
					localTASflags.messageSyncMode = 1;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_MESSAGES_ASYNC:
					localTASflags.messageSyncMode = 2;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_MESSAGES_DESYNC:
					localTASflags.messageSyncMode = 3;
					tasFlagsDirty = true;
					break;

				case ID_EXEC_WAITSYNC_SYNCWAIT:
					localTASflags.waitSyncMode = 0;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_WAITSYNC_SYNCSKIP:
					localTASflags.waitSyncMode = 1;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				case ID_EXEC_WAITSYNC_ASYNC:
					localTASflags.waitSyncMode = 2;
					tasFlagsDirty = true;
					CheckDialogChanges(-1);
					break;
				
				case ID_EXEC_DLLS_INSTALLED:
					localTASflags.allowLoadInstalledDlls = !localTASflags.allowLoadInstalledDlls;
					tasFlagsDirty = true;
					break;
				case ID_EXEC_DLLS_UXTHEME:
					localTASflags.allowLoadUxtheme = !localTASflags.allowLoadUxtheme;
					tasFlagsDirty = true;
					break;

				case ID_EXEC_DLLS_RUNLAST:
					runDllLast = !runDllLast;
					runDllLastConfigured = true;
					mainMenuNeedsRebuilding = true;
					break;


				case ID_EXEC_LOCALE_SYSTEM: localTASflags.appLocale = 0; tasFlagsDirty = true; break;
				case ID_EXEC_LOCALE_JAPANESE: localTASflags.appLocale = 1041; tasFlagsDirty = true; break;
				case ID_EXEC_LOCALE_CHINESE: localTASflags.appLocale = 2052; tasFlagsDirty = true; break;
				case ID_EXEC_LOCALE_KOREAN: localTASflags.appLocale = 1042; tasFlagsDirty = true; break;
				case ID_EXEC_LOCALE_ENGLISH: localTASflags.appLocale = 1033; tasFlagsDirty = true; break;


				case ID_INPUT_SKIPLAGFRAMES:
					advancePastNonVideoFrames = !advancePastNonVideoFrames;
					advancePastNonVideoFramesConfigured = true;
					mainMenuNeedsRebuilding = true;
					break;

				case ID_INPUT_BACKGROUNDHOTKEY_TASER:
					hotkeysFocusFlags ^= FOCUS_FLAG_TASER;
					mainMenuNeedsRebuilding = true;
					break;
				case ID_INPUT_BACKGROUNDHOTKEY_TASEE:
					hotkeysFocusFlags ^= FOCUS_FLAG_TASEE;
					mainMenuNeedsRebuilding = true;
					break;
				case ID_INPUT_BACKGROUNDHOTKEY_OTHER:
					hotkeysFocusFlags ^= FOCUS_FLAG_OTHER;
					mainMenuNeedsRebuilding = true;
					break;
				case ID_INPUT_BACKGROUNDINPUTS_TASER:
					inputFocusFlags ^= FOCUS_FLAG_TASER;
					mainMenuNeedsRebuilding = true;
					break;
				case ID_INPUT_BACKGROUNDINPUTS_TASEE:
					inputFocusFlags ^= FOCUS_FLAG_TASEE;
					mainMenuNeedsRebuilding = true;
					break;
				case ID_INPUT_BACKGROUNDINPUTS_OTHER:
					inputFocusFlags ^= FOCUS_FLAG_OTHER;
					mainMenuNeedsRebuilding = true;
					break;

				case ID_SWITCH_TO_TASEE:
					BringGameWindowToForeground();
					break;
				case ID_SWITCH_TO_TASER:
					SetForegroundWindow(hDlg);
					break;
				case ID_SWITCH_TO_TASEE_FROM_TASER:
					if(GetForegroundWindow() == hDlg)
						BringGameWindowToForeground();
					break;

				case ID_INPUT_HOTKEYS: {
					RECT rect = {};
					if(HotkeyHWnd && !HotkeyHWndIsHotkeys)
					{
						GetWindowRect(HotkeyHWnd, &rect);
						SendMessage(HotkeyHWnd, WM_CLOSE, 0, 0); // we don't currently support having both input dialogs open
					}
					if(!HotkeyHWnd)
						HotkeyHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_HOTKEYS), hWnd, (DLGPROC) HotkeysProc);
					else
						SetForegroundWindow(HotkeyHWnd);
					if(rect.right > rect.left && rect.bottom > rect.top)
						SetWindowPos(HotkeyHWnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOSIZE|SWP_NOZORDER);
					HotkeyHWndIsHotkeys = true;
				}	break;

				case ID_INPUT_INPUTS: {
					RECT rect = {};
					if(HotkeyHWnd && HotkeyHWndIsHotkeys)
					{
						GetWindowRect(HotkeyHWnd, &rect);
						SendMessage(HotkeyHWnd, WM_CLOSE, 0, 0); // we don't currently support having both input dialogs open
					}
					if(!HotkeyHWnd)
						HotkeyHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_HOTKEYS), hWnd, (DLGPROC) InputsProc);
					else
						SetForegroundWindow(HotkeyHWnd);
					if(rect.right > rect.left && rect.bottom > rect.top)
						SetWindowPos(HotkeyHWnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOSIZE|SWP_NOZORDER);
					HotkeyHWndIsHotkeys = false;
				}	break;


				case ID_INCLUDE_LCF_NONE: includeLogFlags = 0; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_ALL: includeLogFlags = ~LCF_FREQUENT; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_UNTESTED: includeLogFlags ^= LCF_UNTESTED; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_DESYNC: includeLogFlags ^= LCF_DESYNC; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_FREQUENT: includeLogFlags ^= LCF_FREQUENT; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_ERROR: includeLogFlags ^= LCF_ERROR; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_TODO: includeLogFlags ^= LCF_TODO; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_FRAME: includeLogFlags ^= LCF_FRAME; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_HOOK: includeLogFlags ^= LCF_HOOK; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_TIMEFUNC: includeLogFlags ^= LCF_TIMEFUNC; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_TIMESET: includeLogFlags ^= LCF_TIMESET; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_TIMEGET: includeLogFlags ^= LCF_TIMEGET; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_SYNCOBJ: includeLogFlags ^= LCF_SYNCOBJ; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_WAIT: includeLogFlags ^= LCF_WAIT; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_SLEEP: includeLogFlags ^= LCF_SLEEP; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_DDRAW: includeLogFlags ^= LCF_DDRAW; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_D3D: includeLogFlags ^= LCF_D3D; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_OGL: includeLogFlags ^= LCF_OGL; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_GDI: includeLogFlags ^= LCF_GDI; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_SDL: includeLogFlags ^= LCF_SDL; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_DINPUT: includeLogFlags ^= LCF_DINPUT; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_KEYBOARD: includeLogFlags ^= LCF_KEYBOARD; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_MOUSE: includeLogFlags ^= LCF_MOUSE; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_JOYPAD: includeLogFlags ^= LCF_JOYPAD; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_DSOUND: includeLogFlags ^= LCF_DSOUND; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_WSOUND: includeLogFlags ^= LCF_WSOUND; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_PROCESS: includeLogFlags ^= LCF_PROCESS; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_MODULE: includeLogFlags ^= LCF_MODULE; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_MESSAGES: includeLogFlags ^= LCF_MESSAGES; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_WINDOW: includeLogFlags ^= LCF_WINDOW; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_FILEIO: includeLogFlags ^= LCF_FILEIO; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_REGISTRY: includeLogFlags ^= LCF_REGISTRY; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_THREAD: includeLogFlags ^= LCF_THREAD; tasFlagsDirty = true; break;
				case ID_INCLUDE_LCF_TIMERS: includeLogFlags ^= LCF_TIMERS; tasFlagsDirty = true; break;

				case ID_EXCLUDE_LCF_NONE: excludeLogFlags = 0; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_ALL: excludeLogFlags = ~0; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_UNTESTED: excludeLogFlags ^= LCF_UNTESTED; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_DESYNC: excludeLogFlags ^= LCF_DESYNC; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_FREQUENT: excludeLogFlags ^= LCF_FREQUENT; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_ERROR: excludeLogFlags ^= LCF_ERROR; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_TODO: excludeLogFlags ^= LCF_TODO; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_FRAME: excludeLogFlags ^= LCF_FRAME; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_HOOK: excludeLogFlags ^= LCF_HOOK; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_TIMEFUNC: excludeLogFlags ^= LCF_TIMEFUNC; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_TIMESET: excludeLogFlags ^= LCF_TIMESET; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_TIMEGET: excludeLogFlags ^= LCF_TIMEGET; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_SYNCOBJ: excludeLogFlags ^= LCF_SYNCOBJ; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_WAIT: excludeLogFlags ^= LCF_WAIT; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_SLEEP: excludeLogFlags ^= LCF_SLEEP; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_DDRAW: excludeLogFlags ^= LCF_DDRAW; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_D3D: excludeLogFlags ^= LCF_D3D; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_OGL: excludeLogFlags ^= LCF_OGL; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_GDI: excludeLogFlags ^= LCF_GDI; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_SDL: excludeLogFlags ^= LCF_SDL; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_DINPUT: excludeLogFlags ^= LCF_DINPUT; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_KEYBOARD: excludeLogFlags ^= LCF_KEYBOARD; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_MOUSE: excludeLogFlags ^= LCF_MOUSE; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_JOYPAD: excludeLogFlags ^= LCF_JOYPAD; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_DSOUND: excludeLogFlags ^= LCF_DSOUND; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_WSOUND: excludeLogFlags ^= LCF_WSOUND; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_PROCESS: excludeLogFlags ^= LCF_PROCESS; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_MODULE: excludeLogFlags ^= LCF_MODULE; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_MESSAGES: excludeLogFlags ^= LCF_MESSAGES; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_WINDOW: excludeLogFlags ^= LCF_WINDOW; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_FILEIO: excludeLogFlags ^= LCF_FILEIO; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_REGISTRY: excludeLogFlags ^= LCF_REGISTRY; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_THREAD: excludeLogFlags ^= LCF_THREAD; tasFlagsDirty = true; break;
				case ID_EXCLUDE_LCF_TIMERS: excludeLogFlags ^= LCF_TIMERS; tasFlagsDirty = true; break;

				case ID_TRACE_LCF_NONE: traceLogFlags = 0; tasFlagsDirty = true; break;
				case ID_TRACE_LCF_ALL: traceLogFlags = ~LCF_FREQUENT; traceEnabled = true; tasFlagsDirty = true; break;
				case ID_TRACE_LCF_UNTESTED: traceLogFlags ^= LCF_UNTESTED; if(traceLogFlags & LCF_UNTESTED){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_DESYNC: traceLogFlags ^= LCF_DESYNC; if(traceLogFlags & LCF_DESYNC){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_ERROR: traceLogFlags ^= LCF_ERROR; if(traceLogFlags & LCF_ERROR){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_TODO: traceLogFlags ^= LCF_TODO; if(traceLogFlags & LCF_TODO){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_FRAME: traceLogFlags ^= LCF_FRAME; if(traceLogFlags & LCF_FRAME){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_HOOK: traceLogFlags ^= LCF_HOOK; if(traceLogFlags & LCF_HOOK){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_TIMEFUNC: traceLogFlags ^= LCF_TIMEFUNC; if(traceLogFlags & LCF_TIMEFUNC){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_TIMESET: traceLogFlags ^= LCF_TIMESET; if(traceLogFlags & LCF_TIMESET){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_TIMEGET: traceLogFlags ^= LCF_TIMEGET; if(traceLogFlags & LCF_TIMEGET){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_SYNCOBJ: traceLogFlags ^= LCF_SYNCOBJ; if(traceLogFlags & LCF_SYNCOBJ){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_WAIT: traceLogFlags ^= LCF_WAIT; if(traceLogFlags & LCF_WAIT){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_SLEEP: traceLogFlags ^= LCF_SLEEP; if(traceLogFlags & LCF_SLEEP){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_DDRAW: traceLogFlags ^= LCF_DDRAW; if(traceLogFlags & LCF_DDRAW){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_D3D: traceLogFlags ^= LCF_D3D; if(traceLogFlags & LCF_D3D){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_OGL: traceLogFlags ^= LCF_OGL; if(traceLogFlags & LCF_OGL){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_GDI: traceLogFlags ^= LCF_GDI; if(traceLogFlags & LCF_GDI){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_SDL: traceLogFlags ^= LCF_SDL; if(traceLogFlags & LCF_SDL){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_DINPUT: traceLogFlags ^= LCF_DINPUT; if(traceLogFlags & LCF_DINPUT){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_KEYBOARD: traceLogFlags ^= LCF_KEYBOARD; if(traceLogFlags & LCF_KEYBOARD){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_MOUSE: traceLogFlags ^= LCF_MOUSE; if(traceLogFlags & LCF_MOUSE){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_JOYPAD: traceLogFlags ^= LCF_JOYPAD; if(traceLogFlags & LCF_JOYPAD){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_DSOUND: traceLogFlags ^= LCF_DSOUND; if(traceLogFlags & LCF_DSOUND){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_WSOUND: traceLogFlags ^= LCF_WSOUND; if(traceLogFlags & LCF_WSOUND){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_PROCESS: traceLogFlags ^= LCF_PROCESS; if(traceLogFlags & LCF_PROCESS){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_MODULE: traceLogFlags ^= LCF_MODULE; if(traceLogFlags & LCF_MODULE){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_MESSAGES: traceLogFlags ^= LCF_MESSAGES; if(traceLogFlags & LCF_MESSAGES){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_WINDOW: traceLogFlags ^= LCF_WINDOW; if(traceLogFlags & LCF_WINDOW){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_FILEIO: traceLogFlags ^= LCF_FILEIO; if(traceLogFlags & LCF_FILEIO){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_REGISTRY: traceLogFlags ^= LCF_REGISTRY; if(traceLogFlags & LCF_REGISTRY){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_THREAD: traceLogFlags ^= LCF_THREAD; if(traceLogFlags & LCF_THREAD){traceEnabled = true;} tasFlagsDirty = true; break;
				case ID_TRACE_LCF_TIMERS: traceLogFlags ^= LCF_TIMERS; if(traceLogFlags & LCF_TIMERS){traceEnabled = true;} tasFlagsDirty = true; break;


				case ID_DEBUGLOG_DISABLED: localTASflags.debugPrintMode = 0; tasFlagsDirty = true; break;
				case ID_DEBUGLOG_DEBUGGER: localTASflags.debugPrintMode = 1; tasFlagsDirty = true; break;
				case ID_DEBUGLOG_LOGFILE: localTASflags.debugPrintMode = 2; tasFlagsDirty = true; break;
				
				case ID_DEBUGLOG_TOGGLETRACEENABLE: traceEnabled = !traceEnabled; mainMenuNeedsRebuilding = true; break;
				case ID_DEBUGLOG_TOGGLECRCVERIFY: crcVerifyEnabled = !crcVerifyEnabled; mainMenuNeedsRebuilding = true; break;
				case ID_PERFORMANCE_TOGGLESAVEVIDMEM: localTASflags.storeVideoMemoryInSavestates = !localTASflags.storeVideoMemoryInSavestates; tasFlagsDirty = true; break;
				case ID_PERFORMANCE_TOGGLESAVEGUARDED: storeGuardedPagesInSavestates = !storeGuardedPagesInSavestates; mainMenuNeedsRebuilding = true; break;
				case ID_PERFORMANCE_DEALLOCSTATES:
					for(int i = 0; i < maxNumSavestates; i++)
						savestates[i].Deallocate();
					break;
				case ID_PERFORMANCE_DELETESTATES:
					for(int i = 0; i < maxNumSavestates; i++)
						savestates[i].Clear();
					break;

				case ID_FILES_LOADSTATE_1:
				case ID_FILES_LOADSTATE_2:
				case ID_FILES_LOADSTATE_3:
				case ID_FILES_LOADSTATE_4:
				case ID_FILES_LOADSTATE_5:
				case ID_FILES_LOADSTATE_6:
				case ID_FILES_LOADSTATE_7:
				case ID_FILES_LOADSTATE_8:
				case ID_FILES_LOADSTATE_9:
				case ID_FILES_LOADSTATE_10:
				case ID_FILES_LOADSTATE_11:
				case ID_FILES_LOADSTATE_12:
				case ID_FILES_LOADSTATE_13:
				case ID_FILES_LOADSTATE_14:
				case ID_FILES_LOADSTATE_15:
				case ID_FILES_LOADSTATE_16:
				case ID_FILES_LOADSTATE_17:
				case ID_FILES_LOADSTATE_18:
				case ID_FILES_LOADSTATE_19:
				case ID_FILES_LOADSTATE_20:
					if(started)
						LoadGameStatePhase1(1 + (command - ID_FILES_LOADSTATE_1));
					break;

				case ID_FILES_SAVESTATE_1:
				case ID_FILES_SAVESTATE_2:
				case ID_FILES_SAVESTATE_3:
				case ID_FILES_SAVESTATE_4:
				case ID_FILES_SAVESTATE_5:
				case ID_FILES_SAVESTATE_6:
				case ID_FILES_SAVESTATE_7:
				case ID_FILES_SAVESTATE_8:
				case ID_FILES_SAVESTATE_9:
				case ID_FILES_SAVESTATE_10:
				case ID_FILES_SAVESTATE_11:
				case ID_FILES_SAVESTATE_12:
				case ID_FILES_SAVESTATE_13:
				case ID_FILES_SAVESTATE_14:
				case ID_FILES_SAVESTATE_15:
				case ID_FILES_SAVESTATE_16:
				case ID_FILES_SAVESTATE_17:
				case ID_FILES_SAVESTATE_18:
				case ID_FILES_SAVESTATE_19:
				case ID_FILES_SAVESTATE_20:
					if(started)
						SaveGameStatePhase1(1 + (command - ID_FILES_SAVESTATE_1));
					break;

				//case ID_FILES_SAVECONFIG:
				//	Save_Config();
				//	break;
				case ID_FILES_SAVECONFIGAS:
					Save_As_Config(hDlg);
					break;
				case ID_FILES_LOADCONFIGFROM:
					Load_As_Config(hDlg);
					break;

				//case IDC_AVIVIDEO:
				//	aviMode = (aviMode & ~1) | (IsDlgButtonChecked(hDlg, IDC_AVIVIDEO) ? 1 : 0);
				//	curAviWidth = 0;
				//	curAviHeight = 0;
				//	curAviFps = 0;
				//	tasFlagsDirty = true;
				//	if(!(aviMode))
				//		CloseAVI();
				//	break;
				//case IDC_AVIAUDIO:
				//	aviMode = (aviMode & ~2) | (IsDlgButtonChecked(hDlg, IDC_AVIAUDIO) ? 2 : 0);
				//	if(!(emuMode & EMUMODE_EMULATESOUND))
				//	{
				//		emuMode |= EMUMODE_EMULATESOUND;
				//		//CheckDlgButton(hDlg, IDC_EMULATESOUND, emuMode & EMUMODE_EMULATESOUND);
				//	}
				//	tasFlagsDirty = true;
				//	if(!(aviMode))
				//		CloseAVI();
				//	break;
				case ID_AVI_VIDEO:
					localTASflags.aviMode = 1;
					if(!CreateAVIFile())
					{
						localTASflags.aviMode = 0;
						CustomMessageBox("The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", "File creation error!", MB_OK | MB_ICONERROR);
					}
					tasFlagsDirty = true;
					break;
				case ID_AVI_AUDIO:
					localTASflags.aviMode = 2;
					if(!(localTASflags.emuMode & EMUMODE_EMULATESOUND))
						localTASflags.emuMode |= EMUMODE_EMULATESOUND;
					if(!CreateAVIFile())
					{
						localTASflags.aviMode = 0;
						CustomMessageBox("The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", "File creation error!", MB_OK | MB_ICONERROR);
					}
					tasFlagsDirty = true;
					break;
				case ID_AVI_BOTH:
					localTASflags.aviMode = (1|2);
					if(!(localTASflags.emuMode & EMUMODE_EMULATESOUND))
						localTASflags.emuMode |= EMUMODE_EMULATESOUND;
					if(!CreateAVIFile())
					{
						localTASflags.aviMode = 0;
						CustomMessageBox("The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", "File creation error!", MB_OK | MB_ICONERROR);
					}
					tasFlagsDirty = true;
					break;
				case ID_AVI_NONE:
					localTASflags.aviMode = 0;
					CloseAVI();
					tasFlagsDirty = true;
					break;
				case ID_AVI_SPLITNOW:
					//requestedAviSplitCount = aviSplitCount + 1;
					SplitAVINow();
					break;
				case IDC_RADIO_THREAD_DISABLE:
					localTASflags.threadMode = 0;
					localTASflags.waitSyncMode = 1; // hack, should rename these radio buttons
					tasFlagsDirty = true;
					break;
				case IDC_RADIO_THREAD_WRAP:
					localTASflags.threadMode = 1;
					localTASflags.waitSyncMode = 1; // hack, should rename these radio buttons
					tasFlagsDirty = true;
					break;
				case IDC_RADIO_THREAD_ALLOW:
					localTASflags.threadMode = 2;
					localTASflags.waitSyncMode = 2; // hack, should rename these radio buttons
					tasFlagsDirty = true;
					break;
				case IDC_EDIT_FPS:
					{
						if(HIWORD(wParam) == EN_CHANGE)
						{
							char str [256];
							if(started && !warnedAboutFrameRateChange)
							{
								GetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str, 255);
								if(localTASflags.framerate != strtol(str,0,0))
								{
									warnedAboutFrameRateChange = true;
									int result = CustomMessageBox("If you are in the middle of recording or playing a movie,\nchanging the game's framerate might cause desync in some games.\nBe careful.", "Warning", MB_OKCANCEL | MB_ICONWARNING);
									if(result == IDCANCEL)
									{
										sprintf(str, "%d", localTASflags.framerate);
										SetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str);
										warnedAboutFrameRateChange = false; // must be true above this to avoid infinite warnings
										break;
									}
								}
							}

							GetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str, 255);
							localTASflags.framerate = strtol(str,0,0);
							tasFlagsDirty = true;
						}
					}
					break;
				case IDC_EDIT_SYSTEMCLOCK:
					if(!started && !unsavedMovieData)
					{
						if(HIWORD(wParam) == EN_CHANGE)
						{
							static bool selfEdit = false;
							if(!selfEdit)
							{
								selfEdit = true;
								char str [256];
								GetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str, 255);
								localTASflags.initialTime = strtoul(str,0,0);
								if(!*str) localTASflags.initialTime = /*timeGetTime()*/6000;
								//char str2 [256];
								//sprintf(str2, "%u", initialTime);
								//if(strcmp(str,str2))
								//	SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str2);
								tasFlagsDirty = true;
								selfEdit = false;
							}
						}
					}
					break;
				case ID_FILES_STOP_RELAY:
					PostMessage(hDlg, WM_COMMAND, ID_FILES_STOP, 0);
					break;
				case ID_FILES_STOP:
				case IDC_BUTTON_STOP:
					if(started)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), false);
						CloseAVI();
						if(unsavedMovieData)
						{
							SaveMovie(moviefilename);
						}
						Save_Config();
						//CheckDlgButton(hDlg, IDC_AVIVIDEO, aviMode & 1);
						//CheckDlgButton(hDlg, IDC_AVIAUDIO, aviMode & 2);
						bool wasPlayback = localTASflags.playback;
						TerminateDebuggerThread(12000);
						if(unsavedMovieData)
						{
							SaveMovie(moviefilename);
						}
						terminateRequest = false;
						if(afterDebugThreadExit)
							OnAfterDebugThreadExit();
						requestedCommandReenter = false;
						if(lParam == 42)
							goto case_IDC_BUTTON_PLAY;
						else
							recoveringStale = false;
					}
					break;
				case_IDC_BUTTON_RECORD:
				case IDC_BUTTON_RECORD:
					if(IsWindowEnabled(GetDlgItem(hDlg, IDC_BUTTON_RECORD)))
					{
						Save_Config();
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RECORD), false);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PLAY), false);
						movie = Movie();
						localTASflags.playback = false;
						nextLoadRecords = true;
						localTASflags.fastForward = false;
						started = true; CheckDialogChanges(-1); started = false;
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), false);
						GetWindowText(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
						debuggerThread = CreateThread(NULL, 0, DebuggerThreadFunc, NULL, 0, NULL);
					}
					break;

				case ID_FILES_PLAYMOV:
				case IDC_BUTTON_PLAY:
					if(IsWindowEnabled(GetDlgItem(hDlg, IDC_BUTTON_PLAY)))
					{
				case_IDC_BUTTON_PLAY:
						WaitForOtherThreadToTerminate(hAfterDebugThreadExitThread, 5000);
						Save_Config();
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_RECORD), false);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PLAY), false);
						localTASflags.playback = true;
						nextLoadRecords = false;
						started = true; CheckDialogChanges(-1); started = false;
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), false);
						GetWindowText(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
						debuggerThread = CreateThread(NULL, 0, DebuggerThreadFunc, NULL, 0, NULL);
					}
					break;

				case ID_FILES_WATCHMOV:
					if(started)
					{
						Save_Config();
						PostMessage(hDlg, WM_COMMAND, IDC_BUTTON_STOP, 42);
					}
					break;

				case ID_FILES_OPENEXE:
				case IDC_BUTTON_GAMEBROWSE:
					{
						char filename [MAX_PATH+1] = {0};
						char directory [MAX_PATH+1] = {0};
						SplitToValidPath(exefilename, thisprocessPath, filename, directory);

						OPENFILENAME ofn = {sizeof(OPENFILENAME)};
						ofn.hwndOwner = hWnd;
						ofn.hInstance = hInst;
						ofn.lpstrFilter = "Executable Program\0*.exe\0All Files\0*.*\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = filename;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrInitialDir = directory;
						ofn.lpstrTitle = "Choose Executable";
						ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
						ofn.lpstrDefExt = "exe";
						if(GetOpenFileName(&ofn))
						{
							SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), filename);
						}
						Save_Config();
					}
					break;
				case ID_FILES_RECORDMOV:
				case ID_FILES_OPENMOV:
				case IDC_BUTTON_MOVIEBROWSE:
					if(!started)
					{
				//case ID_FILES_RESUMEMOVAS: // TODO: Eliminate this case
						Save_Config();
						char filename [MAX_PATH+1] = {0};
						char directory [MAX_PATH+1] = {0};
						SplitToValidPath(moviefilename, thisprocessPath, filename, directory);
						NormalizePath(moviefilename, moviefilename);

						bool isSave = (command == ID_FILES_RECORDMOV/* || command == ID_FILES_RESUMEMOVAS*/);

						bool wasPaused = paused;
						if(!paused)
						{
							paused = true;
							tasFlagsDirty = true;
						}

						OPENFILENAME ofn = {sizeof(OPENFILENAME)};
						ofn.hwndOwner = hWnd;
						ofn.hInstance = hInst;
						ofn.lpstrFilter = "Windows TAS Files\0*.wtf;*.wtf2;*.wtf3;*.wtf4;*.wtf5;*.wtf6;*.wtf7;*.wtf8;*.wtf9;*.wtf*;*.hgm\0All Files\0*.*\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = filename;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrInitialDir = directory;
						ofn.lpstrTitle = isSave ? "Create Movie File" : "Choose Movie File";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | (isSave ? (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN) : 0);
						ofn.lpstrDefExt = "wtf";
						if(isSave ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
						{
							/*if(started && (!localTASflags.playback || nextLoadRecords))
							{
								SaveMovie();
							}*/

							SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), filename);

							if(movieFileWritable && exeFileExists)
							{
								if(command == ID_FILES_RECORDMOV)
								{
									if(paused != wasPaused)
									{
										paused = wasPaused;
										tasFlagsDirty = true;
									}
									goto case_IDC_BUTTON_RECORD;
								}
								/*if(command == ID_FILES_RESUMEMOVAS)
								{
									localTASflags.playback = false;
									SaveMovie();
								}*/
							}
						}
						if(paused != wasPaused)
						{
							paused = wasPaused;
							tasFlagsDirty = true;
						}
					}
					break;
				case ID_FILES_RESUMERECORDING:
					if(started && localTASflags.playback && !finished)
					{
						nextLoadRecords = true;
						movie.rerecordCount++;
						localTASflags.playback = false;
						UpdateRerecordCountDisplay();
						UpdateFrameCountDisplay(movie.currentFrame, 1);
					}
					break;
				case ID_FILES_BACKUPMOVIE:
					{
						bool wasPaused = paused;
						if(!paused) //If we're not paused, pause
						{
							paused = true;
							tasFlagsDirty = true;
						}

						// Always start with the name of the movie currently recording.
						// And in the same directory as the movie currently recording if possible
						// otherwise we start with the default dir.
						char filename[MAX_PATH+1] = {'\0'};
						char directory[MAX_PATH+1] = {'\0'};
						SplitToValidPath(moviefilename, thisprocessPath, filename, directory);
						//strcpy(filename, GetFilenameWithoutPath(moviefilename));
						//strcpy(directory, thisprocessPath);

						OPENFILENAME ofn = {sizeof(OPENFILENAME)};
						ofn.hwndOwner = hWnd;
						ofn.hInstance = hInst;
						// What the hell is up with all these naming variations of .wtf?
						ofn.lpstrFilter = "Windows TAS Files\0*.wtf;*.wtf2;*.wtf3;*.wtf4;*.wtf5;*.wtf6;*.wtf7;*.wtf8;*.wtf9;*.wtf*;*.hgm\0All Files\0*.*\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = filename;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrInitialDir = directory;
						ofn.lpstrTitle = "Save Backup Movie";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN);
						ofn.lpstrDefExt = "wtf";
						if(GetSaveFileName(&ofn))
						{
							// This is really fucking ugly, needs improving
							//char oldmoviefilename[MAX_PATH+1];
							//strcpy(oldmoviefilename, moviefilename);
							//strcpy(moviefilename, filename);
							SaveMovie(filename);
							//strcpy(moviefilename, oldmoviefilename);
						}
						if(paused != wasPaused) // If we had to pause earlier, unpause now.
						{
							paused = wasPaused;
							tasFlagsDirty = true;
						}
					}
					break;
//				case ID_FILES_SPLICE:
//					if(!SpliceHWnd)
//						SpliceHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SPLICE), hWnd, (DLGPROC) SpliceProc);
//					else
//						SetForegroundWindow(SpliceHWnd);
				case IDC_EDIT_COMMANDLINE:
					if(HIWORD(wParam) == EN_CHANGE)
					{
						GetWindowText(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
					}
					break;
				case IDC_TEXT_EXE:
					if(HIWORD(wParam) == EN_CHANGE || !movieFileExists || !*moviefilename)
					{
						static bool recursing = false;
						if(recursing)
							break;
						recursing = true;

						GetWindowText(GetDlgItem(hDlg, IDC_TEXT_EXE), exefilename, MAX_PATH);
						NormalizePath(exefilename, exefilename);
						EnableDisablePlayRecordButtons(hDlg); // updates exeFileExists

						if(!exeFileExists) // Extra check, do we need it?
						{
							char str[1024];
							sprintf(str, "The exe file \'%s\' does not exist, please check that it has not been renamed or moved.", exefilename);
							CustomMessageBox(str, "Error!", (MB_OK | MB_ICONERROR));
							break;
						}


						//FILE* file = *exefilename ? fopen(exefilename, "rb") : NULL;
						//if(file)
						//{
						//	fclose(file);
							if((!movienameCustomized && HIWORD(wParam) == EN_CHANGE) || !movieFileExists || !*moviefilename)
							{
								// a little hack to help people out when first selecting an exe for recording,
								// until perhaps there's a proper ini file for such defaults
								int newFramerate;
								BOOL newLagskip;
								int newRunDllLast = 0;
								const char* fname = GetExeFilenameWithoutPath();
								if(!stricmp(fname, "Doukutsu.exe")
								|| !stricmp(fname, "dxIka.exe")
								|| !stricmp(fname, "nothing.exe")
								|| !_strnicmp(fname, "iwbtgbeta", sizeof("iwbtgbeta")-1)
								|| !stricmp(fname, "i_wanna_be_the_GB.exe")
								)
								{
									newFramerate = 50; newLagskip = false;
								}
								else if(!stricmp(fname, "Lyle in Cube Sector.exe")
								|| !stricmp(fname, "Eternal Daughter.exe")
								|| !stricmp(fname, "Geoffrey The Fly.exe")
								)
								{
									newFramerate = 50; newLagskip = true;
								}
								else if(!stricmp(fname, "herocore.exe"))
								{
									newFramerate = 40; newLagskip = false;
								}
								else if(!stricmp(fname, "iji.exe"))
								{
									newFramerate = 30; newLagskip = false;
								}
								else if(!stricmp(fname, "lamulana.exe"))
								{
									newFramerate = 60; newLagskip = true;
								}
								else if(!stricmp(fname, "NinjaSenki.exe"))
								{
									newFramerate = 60; newLagskip = false;

									// extra temp: window activation is broken in this game
									if(!(localTASflags.windowActivateFlags & 2))
									{
										localTASflags.windowActivateFlags |= 2;
										CheckDlgButton(hDlg, IDC_MAKETOPMOST, localTASflags.windowActivateFlags & 2);
										tasFlagsDirty = true;
									}
								}
								else if(!stricmp(fname, "RotateGear.exe"))
								{
									newFramerate = 60; newLagskip = false; newRunDllLast = true;
								}
								else // default to 60 for everything else
								{
									newFramerate = 60; newLagskip = false;
								}
								if(localTASflags.framerate != newFramerate && !movieFileExists)
								{
									tasFlagsDirty = true;
									char str [32];
									sprintf(str, "%d", newFramerate);
									SetWindowText(GetDlgItem(hDlg, IDC_EDIT_FPS), str);
								}
								if(newLagskip != advancePastNonVideoFrames && (!advancePastNonVideoFramesConfigured || !advancePastNonVideoFrames))
								{
									advancePastNonVideoFrames = newLagskip;
									advancePastNonVideoFramesConfigured = false;
									mainMenuNeedsRebuilding = true;
								}
								if(runDllLast != newRunDllLast && !runDllLastConfigured)
								{
									runDllLast = newRunDllLast;
									mainMenuNeedsRebuilding = true;
								}
							}
							if(!movieFileExists || (moviefilename[0] == '\0') || !movienameCustomized) // set default movie name based on exe
							{
								bool setMovieFile = true;
								if(movieFileExists)
								{
									const char* exefname = GetExeFilenameWithoutPath();
									if(!strncmp(exefname, movie.exefname, 48))
										setMovieFile = false; // exe didn't actually change
								}

								if(setMovieFile)
								{
									char filename [MAX_PATH+1];
									strcpy(filename, exefilename);
									char* slash = max(strrchr(filename, '\\'), strrchr(filename, '/'));
									char* dot = strrchr(filename, '.');
									if(slash<dot)
									{
										char path [MAX_PATH+1];
										strcpy(path, thisprocessPath);
										strcat(path, "\\");
										strcpy(dot, ".wtf");
										const char* moviename = slash ? slash+1 : filename;
										strcat(path, moviename);
										if(0 != strcmp(path, newmoviefilename))
										{
											SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), path);
											movienameCustomized = false;
										}
									}
								}
							}
						//}

						recursing = false;
					}
					break;
				case IDC_EDIT_MOVIE:
					if(HIWORD(wParam) == EN_CHANGE)
					{
						char* tmp_movie = strcmp(newmoviefilename, moviefilename) ? newmoviefilename : moviefilename;
						GetWindowText(GetDlgItem(hDlg, IDC_EDIT_MOVIE), tmp_movie, MAX_PATH);
						NormalizePath(newmoviefilename, tmp_movie);
						EnableDisablePlayRecordButtons(hDlg);
						movienameCustomized = tmp_movie[0] != '\0';
					}
					break;
				}
			}
			if(tasFlagsDirty)
				mainMenuNeedsRebuilding = true;
			break;

		case WM_DROPFILES:
			if(!started)
			{
				HDROP hDrop = (HDROP)wParam;
				int count = (int)DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				for(int i = 0; i < count; i++)
				{
					char filename [MAX_PATH+1];
					if(DragQueryFile(hDrop, i, filename, MAX_PATH))
					{
						//debugprintf("Got File: %s\n", filename);

						char* slash = max(strrchr(filename, '\\'), strrchr(filename, '/'));
						char* dot = strrchr(filename, '.');
						if(slash<dot)
						{
							if(!stricmp(dot, ".exe")) // executable (game)
							{
								SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), filename);
							}
							else if(!_strnicmp(dot, ".wtf", 4) || !_strnicmp(dot, ".hgm", 4)) // windows TAS file (input movie)
							{
								SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_EDIT_MOVIE), filename);
							}
							else if(!stricmp(dot, ".cfg"))
							{
								Load_Config(filename);
							}
						}
					}
				}
				DragFinish(hDrop);
			}
			break;

		case WM_CTLCOLORSTATIC:
			if(((HWND)lParam == GetDlgItem(hDlg, IDC_STATIC_MOVIESTATUS)
			|| (HWND)lParam == GetDlgItem(hDlg, IDC_STATIC_FRAMESLASH))
			&& started) 
			{
				if(finished) // turn text red when movie is finished
				{
					HDC hdcStatic = (HDC)wParam;
					SetTextColor(hdcStatic, RGB(255, 0, 0));
					SetBkMode(hdcStatic, TRANSPARENT);
					return (LONG)GetSysColorBrush(COLOR_BTNFACE);
				}
				else if(localTASflags.playback) // turn text blue when movie is playing
				{
					HDC hdcStatic = (HDC)wParam;
					SetTextColor(hdcStatic, RGB(0, 0, 255));
					SetBkMode(hdcStatic, TRANSPARENT);
					return (LONG)GetSysColorBrush(COLOR_BTNFACE);
				}
			}
			rv = 0;
			break;

        default:  
            rv = FALSE;  
            break;  
    }  
 
    return rv;
}




#ifdef UNITY_BUILD
#undef UNITY_BUILD
#include "ramwatch.cpp"
#include "ramsearch.cpp"
#include "inputsetup.cpp"
#include "trace/extendedtrace.cpp"
#include "inject/process.cpp"
#include "inject/iatmodifier.cpp"
#define UNITY_BUILD
#endif

