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
//#include "CRCMath.h"
//#include "inputsetup.h"
#include "MD5Checksum.h"
#include "Menu.h"
#include "Config.h"
using namespace Config;
#include "InputCapture.h"
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

#include "CPUinfo.h"
#include "DirLocks.h"
#include "ExeFileOperations.h"


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
/*
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
*/

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
	debugprintf("Detecting Windows version...\n");
	char filename[MAX_PATH+1];
	UINT value = GetWindowsDirectory(filename, MAX_PATH);
	if(value != 0)
	{
		// Fix for Windows installations in root dirs, GetWindowsDirectory does not include the final slash EXCEPT for when Windows is installed in root.
		if(filename[value-1] == '\\') filename[value-1] = '\0'; 

		strcat(filename, "\\System32\\kernel32.dll");
		debugprintf("Using file '%s' for the detection.\n", filename);

		UINT size = 0;

		// Despite being a pointer this must NOT be deleted by us, it will be taken care of properly by the destructors of the other structures when we delete verInfo.
		VS_FIXEDFILEINFO *buffer = NULL;
		DWORD infoSize = GetFileVersionInfoSize(filename, NULL);

		if(infoSize != 0)
		{
			BYTE *verInfo = new BYTE[(unsigned int)infoSize];

			// On some Windows versions the return value of GetFileVersionInfo is not to be trusted.
			// It may return TRUE even if the function failed, but it always sets the right error code.
			// So instead of catching the return value of the function, we query GetLastError for the function's return status.
			GetFileVersionInfo(filename, NULL, infoSize, verInfo);
			if(GetLastError() == ERROR_SUCCESS)
			{
				BOOL parseSuccess = VerQueryValueA(verInfo, "\\", (LPVOID*)&buffer, &size);
				if (parseSuccess != FALSE && size > 0)
				{
					if (buffer->dwSignature == 0xFEEF04BD)
					{
						/* 
						   kernel32.dll versions and their Windows counter parts:
						   5.2 = XP
						   6.0 = Vista
						   6.1 = Win7
						   6.2 = Win8
						   Now don't these remind a lot of the actual windows versions?
						*/
						*major = HIWORD(buffer->dwFileVersionMS);
						*minor = LOWORD(buffer->dwFileVersionMS);

						delete [] verInfo; // We no longer need to hold on to this.
						
						debugprintf("Detection succeeded, detected version %d.%d\n", *major, *minor);
						return;
					}
				}
			}
			delete [] verInfo; // Destroy this if we failed, we cannot have a memory leak now can we?
		}
	}
	debugprintf("Failed to determinate Windows version, using old unsafe method...\n");

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


/*static*/ Movie movie;

InputCapture inputC; // TODO, put the declaration somewhere else ?

// requires both buffers given to be at least MAX_PATH characters in size.
// it is allowed for both arguments to point at the same buffer.
char* NormalizePath(char* output, const char* path)
{
	extern bool started;
	if(/*movie.version >= 60 || */!started)
		while(*path == ' ')
			path++;
	DWORD len = 0;
	if(/*!(movie.version >= 40 && movie.version < 53) || */!started)
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

//bool paused = false;
//bool fastforward = false;
static bool temporaryUnpause = false;
static bool requestedCommandReenter = false;
static bool cannotSuspendForCommand = false;
static bool goingsuperfast = false;
//static bool frameAdvanceHeld = false;

static bool runningNow = false;
CRITICAL_SECTION g_processMemCS;

//bool started = false;
//bool playback = false;
//bool finished = false;
bool unsavedMovieData = false;
//bool nextLoadRecords = true;
//bool exeFileExists = false;
//bool movieFileExists = false;
//bool movieFileWritable = false;
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
//int runDllLast = 0;
//int waitSyncMode = 1;
//int aviMode = 0;
static int threadStuckCount = 0;
//static int aviSplitCount = 0;
//static int aviSplitDiscardCount = 0;
//static int requestedAviSplitCount = 0;
/*static*/ //int usedThreadMode = -1;
//static int windowActivateFlags = 0;
//int storeVideoMemoryInSavestates = 1;
//int storeGuardedPagesInSavestates = 1;
//int appLocale = 0;
//int tempAppLocale = 0; // Might still be needed
//int forceWindowed = 1;
//int truePause = 0;
//int onlyHookChildProcesses = 0;
//int advancePastNonVideoFrames = 0;
//bool advancePastNonVideoFramesConfigured = false;
bool runDllLastConfigured = false;
//bool crcVerifyEnabled = true;
//int audioFrequency = 44100;
//int audioBitsPerSecond = 16;
//int audioChannels = 2;
//static int stateLoaded = 0;
bool mainMenuNeedsRebuilding = false;
/*#ifndef FOCUS_FLAGS_DEFINED
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
*/

// these can be set from the UI now (runtime menu > debug logging > include/exclude)
/*
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
*/
//static bool traceLogs = false;
//static bool traceLogs = true;

/*static*/ bool tasFlagsDirty = false;



//char moviefilename [MAX_PATH+1];
//char exefilename [MAX_PATH+1];
//char commandline [160];
//char thisprocessPath [MAX_PATH+1];
char injectedDllPath [MAX_PATH+1];
char subexefilename [MAX_PATH+1];



//int emuMode = EMUMODE_EMULATESOUND;

//int fastForwardFlags = FFMODE_FRONTSKIP | FFMODE_BACKSKIP | FFMODE_RAMSKIP | FFMODE_SLEEPSKIP;

//int timescale = 1, timescaleDivisor = 1;

static bool warnedAboutFrameRateChange = false;

//bool recoveringStale = false;
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


//DWORD CalcExeCrc()
//{
//	return CalcFileCrcCached(exefilename);
//}

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



static void* remoteInputs = 0;

void ReceiveInputsPointer(__int64 pointerAsInt)
{
	remoteInputs = (void*)pointerAsInt;
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


void SaveMovie(char* filename)
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
		CalcFileMD5Cached(exefilename, movie.fmd5);
		movie.fsize = CalcExeFilesize();
		strcpy(movie.commandline, commandline);
		movie.headerBuilt = true;
	}

	if(SaveMovieToFile(movie, filename))
	{
		unsavedMovieData = false;
	}
	// TODO: Should be an else-block here with error handling, no?
}

// returns 1 on success, 0 on failure, -1 on cancel
// TODO: Dependency on parameter can probably be removed.
int LoadMovie(char* filename)
{
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
			"This movie was recorded using a different main version of Hourglass Resurrection.\n"
			"\n"
			"Movie's version: %d\n"
			"Program version: %d\n"
			"\n"
			"This may lead to the movie desyncing.\n"
			"If it would desync you might want to use the movies main version of Hourglass Resurrection.\n"
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
		unsigned int temp_md5[4];
		CalcFileMD5Cached(exefilename, temp_md5);
		if(memcmp(movie.fmd5, temp_md5, 4*4) == 0)
		{
			char str[1024];
			sprintf(str, "This movie was probably recorded using a different exe.\n\n"
						 "Movie's exe's md5: %X%X%X%X, size: %d\n"
						 "Current exe's md5: %X%X%X%X, size: %d\n\n"
						 "Playing the movie with current exe may lead to the movie desyncing.\n"
						 "Do you want to continue?\n(Click \"Yes\" to continue, \"No\" to abort)",
						 movie.fmd5[0], movie.fmd5[1], movie.fmd5[2], movie.fmd5[3], movie.fsize, 
						 temp_md5[0], temp_md5[1], temp_md5[2], temp_md5[3], CalcExeFilesize());
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


/*
struct UncompressedMovieFrame
{
	unsigned char keys [256];
};
*/

// InjectLocalInputs
void RecordLocalInputs()
{
	MovieFrame frame;
	if(InputHasFocus(false))
	{
		inputC.ProcessInputs(&frame.inputs, NULL);
		//Update_Input(NULL, true, false, false, false);
	}
	else
	{
		frame.inputs.clear();
		//memset(localGameInputKeys, 0, sizeof(localGameInputKeys));
	}
	
	// Convert pointer screen coordinates into window coordinates.
	// We already have the list of windows that were created by the game.
	// We choose the first window that is visible and whose height is higher than 10, or the last one.
	// TODO: Might take a while, try not to test on every frame.

	std::set<HWND>::iterator iter;
	EnterCriticalSection(&g_gameHWndsCS);
	for(iter = gameHWnds.begin(); iter != gameHWnds.end();)
	{
		HWND gamehwnd = *iter;
		DWORD style = (DWORD)GetWindowLong(gamehwnd, GWL_STYLE);
		iter++;
		if((style & WS_VISIBLE) != 0 || iter == gameHWnds.end())
		{
			RECT rect;
			GetClientRect(gamehwnd, &rect);
			if(rect.bottom - rect.top > 10)
			{
				ScreenToClient(gamehwnd, &frame.inputs.mouse.coords);
				break;
			}
		}
	}
	LeaveCriticalSection(&g_gameHWndsCS);

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
			CurrentInput ci;
			ci.clear();
			SIZE_T bytesWritten = 0;
			WriteProcessMemory(hGameProcess, remoteInputs, &ci, sizeof(CurrentInput), &bytesWritten);
		}
		return;
	}

	MovieFrame frame = movie.frames[movie.currentFrame];
	verbosedebugprintf("injecting movie frame %d\n", movie.currentFrame);

	SIZE_T bytesWritten = 0;
	if(!WriteProcessMemory(hGameProcess, remoteInputs, &frame.inputs, sizeof(CurrentInput), &bytesWritten))
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
		// FIXME: it looks like this call is doing pretty much nothing except for the frameSynced stuff.
		// I really don't know what this is, but if/when we encounter problems, we will see...

		// ignore background input
		//Update_Input(hWnd, frameSynced, false, false, true);
		return;
	}

	CurrentInput ci; // Useless.
	inputC.ProcessInputs(&ci, hWnd); // Only process events.

	//Update_Input(hWnd, frameSynced, true, false, true);


	// FIXME: FastFoward trigger does not use an message, but an extra virtual key.
	// This is not supported by our new InputCapture class!

	/*
	static bool FastForwardKeyDown_prev = false;
	if(FastForwardKeyDown_prev != FastForwardKeyDown)
	{
		FastForwardKeyDown_prev = FastForwardKeyDown;
		SetFastForward(FastForwardKeyDown != 0);
	}
	*/

	// FIXME: Same for FrameAdvance stuff.
	// This really looks like a hack, we should find another way to support it.
	// Use messages? I guess the problem is that we want to support holding keys (for fast-forward mostly).
	/*
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
	*/
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
		//SendMessage(GetDlgItem(hWnd, IDC_TEXT_EXE), EM_SETREADONLY, started, 0);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), EM_SETREADONLY, started, 0);
		//SendMessage(GetDlgItem(hWnd, IDC_TEXT_MOVIE), EM_SETREADONLY, started, 0);
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

const char* ExceptionCodeToDescription(DWORD code)
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
		default:
			LPVOID lpMessageBuffer;
			HMODULE Hand = LoadLibrary("NTDLL.DLL");
   
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_FROM_HMODULE,
				Hand,
				code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMessageBuffer,
				0,
				NULL );

			FreeLibrary(Hand);

			return (char*)lpMessageBuffer;
	}
}

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
	//if(movie.version >= 68)
	//{
		if(CountBackslashes(exefilename + CountSharedPrefixLength(exefilename, path)) < outCount)
			return 1;
		if(CountBackslashes(subexefilename + CountSharedPrefixLength(subexefilename, path)) < outCount)
			return 1;
	//}
	/*else // old version, which accidentally trusts some paths it clearly shouldn't:
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
	}*/
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
	remoteInputs = 0;
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

	//if(movie.version >= 70)
	//{
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
	//}
}



// debuggerthreadproc
static DWORD WINAPI DebuggerThreadFunc(LPVOID lpParam) 
{
	ClearMD5Cache();

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
	/*if(movie.version >= 60 && movie.version <= 63)
		cmdline = commandline;
	else if(movie.version >= 64)
	{*/
		// TODO: Expand the size of allowed commandline
		static char tempcmdline [ARRAYSIZE(commandline)+MAX_PATH+4];
		sprintf(tempcmdline, "\"%s\" %s", exefilename, commandline);
		cmdline = tempcmdline;
	//}

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

	// Older games developed before multi-core CPUs were a thing may still use threads, and due to insufficient (or even no) syncing this causes problems when
	// the game is being played on a multi-core CPU. Since disabling this for every game may cause newer games to experience problems instead or just be unnecessarily slow
	// making it a configurable setting is the only sensible way to go about this.
	// There is also the possible scenario that a game is optimized for Hyper-Threading but will fail on multi-core, so we will give the option to allow Hyper-Threading
	// in case the game doesn't run properly without it.
	// At least 2 games are affected by Multi-Core incompatibility:
	// -No One Lives Forever (audio issues): this game is a nightmare, some events are timed by audio playback, and audio can fail on multi-core systems
	//                                       so that the sounds aren't played in full or not at all, causing the game to generate events way too fast,
	//                                       and sometimes even skipping them. Examples are weapons not having cool-downs between shots and conversations cutting out.
	// -XIII (speed issues): the GOG release of XIII seems to have fixed the speed issues however, but that version might not be the best one for TASing.

	if(limitGameToOnePhysicalCore) // We want to limit the game to one physical core to avoid issues
	{
		int logicalCores;
		int physicalCores;
		//bool hyperThreading;

		CPUInfo(&logicalCores, &physicalCores, /*&hyperThreading*/ NULL);

		// Only actually do something if there is more than one core to choose from.
		// The code will still generate a valid mask for a single-core CPU, it's just unnecessary since the result will be the exact same mask as the default, currently applied, one.
		if(logicalCores > 1)
		{
			unsigned int numHyperThreadCores = logicalCores / physicalCores; // Get how many logical cores each physical CPU core consists of.

			DWORD_PTR mask;

			// Set the number of logical cores that we will use on the physical core.
			if(numHyperThreadCores > 1 && !disableHyperThreading)
			{
				for (unsigned int i = 0; i < numHyperThreadCores; i++)
				{
					mask |= (0x01 << i);
				}
			}
			else // the CPU doesn't have any purely logical cores, or we want to disable Hyper-Threading
			{
				mask = 0x01;
			}

			// Affinity is defined as follows for all CPUs:
			// 0x01 logical core 1
			// 0x02 logical core 2
			// 0x04 logical core 3
			// 0x08 logical core 4
			// ... and so on.
			// It's also defined that logical cores are grouped. Which means for a dual-core CPU with 2 logical cores for each physical:
			// 0x01 logical core 1 cpu core 1
			// 0x02 logical core 2 cpu core 1
			// 0x04 logical core 1 cpu core 2
			// 0x08 logical core 2 cpu core 2
			//
			// Using this information, we can use this code to pick physical cores:

			if(physicalCores > 1) // We have more than one physical CPU core
			{
				mask = mask << numHyperThreadCores; // Use the 2nd physical core.
			}
			// Else: There is only one physical core, which means we cannot manipulate the mask further.


			if(!SetProcessAffinityMask(hGameProcess, mask))
			{
				PrintLastError("SetProcessAffinityMask", GetLastError()); // We failed, write the reason to the error log.
			}
		}
	}

	// I have no idea what this would be good for, as far as I know Hyper-Threading never caused any problems on it's own, except for making games run slower overall,
	// however that cannot be fixed like this since that needs proper disabling of Hyper-Threading from the BIOS given that the CPU allows it.
	// By design Hyper-Threads are purely logical and therefore deterministic since the CPU will divide the execution time evenly across the Hyper-Threads.
	// Since Hyper-Threading cannot be turned off at a software level, all we can do is to force the game to run on only one Hyper-Thread core of each physical core,
	// this alone will most likely never fix any of the Hyper-Threading related slowdowns games can experience, and is probably entirely useless.
	// But since computers and software always finds new ways to deviate from logic and make me speechless, let's include this option.
	// -- Warepire
	else if(disableHyperThreading)
	{
		int logicalCores;
		int physicalCores;
		bool hyperThreading;

		CPUInfo(&logicalCores, &physicalCores, &hyperThreading);

		// It's a Hyper-Threading CPU.
		// Even though a valid mask would be generated even for CPUs without Hyper-Threading it is completely unnecessary to do these
		// calculations as the resulting mask would be the same as the default, currently applied, one.
		if(hyperThreading)
		{
			unsigned int numHyperThreadCores = logicalCores / physicalCores; // Number of Hyper-Thread cores per physical core.

			DWORD_PTR mask = 0;

			// Only let the game use one of the Hyper-Thread cores of each physical core.
			// Refer to the comments in the code above for forcing the game to run on only one core for how Affinity masks work.
			for(int i = 0; i < physicalCores; i++)
			{
				mask = mask << numHyperThreadCores;
				mask |= 0x01;
			}

			if(!SetProcessAffinityMask(hGameProcess, mask))
			{
				PrintLastError("SetProcessAffinityMask", GetLastError()); // We failed, write the reason to the error log.
			}
		}
		// Else: No Hyper-Threading to disable.
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
						else if(MessagePrefixMatch("MOUSEREG"))
						{
							//inputC.InitDIMouse((HWND)(atoi(pstr)), true);
							inputC.InitDIMouse(hWnd, true);
						}
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
						else if(MessagePrefixMatch("INPUTSBUF"))
							ReceiveInputsPointer(_atoi64(pstr));
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
					if(/*(movie.version >= 0 && movie.version < 40) || */!GetFileNameFromProcessHandle(de.u.CreateProcessInfo.hProcess, filename))
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
					if(entrypoint/* && movie.version >= 70*/)
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

					bool nullFile = !de.u.CreateProcessInfo.hFile/* && !(movie.version >= 0 && movie.version < 40)*/;
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
	// FIXME: Implement this.
	/*
	if(IsHotkeyPress())
		return false;
	*/

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

	//Init_Input(hInst, hWnd);
	inputC.InitInputs(hInst, hWnd);

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
				sprintf(title, "Hourglass-Resurrection v%d.%d", VERSION, MINORVERSION);
#ifdef _DEBUG
				strcat(title, " (debug build)");
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
				// As they start blank, we have no interest in updating these if the filenames
				// are empty. This prevents messages about file '' not existing.
				if (path[0] != 0)
					SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), path);
				if (moviefilename[0] != 0)
					SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), moviefilename);
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

				case ID_TIME_FRAME_ADVANCE:
					if(started)
					{
						if(!paused)
						{
							// if pressed while unpaused, frame advance transitions to paused
							paused = true;

							if(localTASflags.fastForward)
								temporaryUnpause = true;
						}
						else
						{
							// if already paused, pressing frame advance temporarily unpauses
							temporaryUnpause = true;
						}
					}
					break;

				//case ID_EXEC_SUSPEND:
				//	SuspendAllExcept(lastKeyThreadId);
				//	break;
				//case ID_EXEC_RESUME:
				//	ResumeAllExcept(lastKeyThreadId);
				//	break;

				case ID_EXEC_LIMITONEPHYSICALCORE:
					limitGameToOnePhysicalCore = !limitGameToOnePhysicalCore;
					break;
				case ID_EXEC_DISABLEHYPERTHREADS:
					disableHyperThreading = !disableHyperThreading;
					break;

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

				// We now have a unique dialog box for both input types. I'm using the ID_INPUT_INPUTS id for it.
				/*
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
				*/

				case ID_INPUT_INPUTS: {
					if(HotkeyHWnd == NULL)
					{
						int dialogPosX = 0;
						int dialogPosY = 0;
						
						static unsigned int dialogSizeX = 0;
						static unsigned int dialogSizeY = 0;

						HotkeyHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONTROLCONF), hWnd, (DLGPROC) &InputCapture::ConfigureInput);

						// Get the coordinates of the Hourglass main window, and the desktop.
						RECT desktopRect = {};
						RECT hourglassRect = {};
						GetWindowRect(hWnd, &hourglassRect);
						GetWindowRect(GetDesktopWindow(), &desktopRect);

						// A little bit of a hack: get the size of the dialog box in pixels.
						if(dialogSizeX == 0 || dialogSizeY == 0)
						{
							RECT configRect = {};
							GetWindowRect(HotkeyHWnd, &configRect);
							dialogSizeY = (configRect.bottom - configRect.top);
							dialogSizeX = (configRect.right - configRect.left);
						}

						// Attempt creating the window in a cascaded position without using CascadeWindows();
						dialogPosY = hourglassRect.top + 15;
						dialogPosX = hourglassRect.left + 15;
						// Correct positioning so that we don't spawn (halfly) off-screen if Hourglass' window is in a corner or something.
						if(dialogPosY < desktopRect.top) dialogPosY = desktopRect.top;
						if(dialogPosX < desktopRect.left) dialogPosX = desktopRect.left;
						if((dialogPosY + dialogSizeY) > desktopRect.bottom) dialogPosY = desktopRect.bottom - dialogSizeY;
						if((dialogPosX + dialogSizeX) > desktopRect.right) dialogPosX = desktopRect.right - dialogSizeX;

						SetWindowPos(HotkeyHWnd, HWND_TOP, dialogPosX, dialogPosY, dialogSizeX, dialogSizeY, SWP_NOZORDER | SWP_SHOWWINDOW);
					}
					else
						SetForegroundWindow(HotkeyHWnd);
					//if(rect.right > rect.left && rect.bottom > rect.top)
					//	SetWindowPos(HotkeyHWnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOSIZE|SWP_NOZORDER);
					// HotkeyHWndIsHotkeys = false;
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
					Save_As_Config(hDlg, hInst);
					break;
				case ID_FILES_LOADCONFIGFROM:
					Load_As_Config(hDlg, hInst);
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
						SaveMovie(moviefilename); // Save the new movie.
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
						ofn.lpstrFilter = "Executable Program (*.exe)\0*.exe\0All Files\0*.*\0\0";
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
						ofn.lpstrFilter = "Windows TAS Files (*.hgr)\0*.hgr\0All Files\0*.*\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = filename;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrInitialDir = directory;
						ofn.lpstrTitle = isSave ? "Create Movie File" : "Choose Movie File";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | (isSave ? (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN) : 0);
						ofn.lpstrDefExt = "hgr";
						if(isSave ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
						{
							/*if(started && (!localTASflags.playback || nextLoadRecords))
							{
								SaveMovie();
							}*/

							SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), filename);

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
						ofn.lpstrFilter = "Windows TAS Files (*.hgr)\0*.hgr\0All Files\0*.*\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = filename;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrInitialDir = directory;
						ofn.lpstrTitle = "Save Backup Movie";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
						ofn.lpstrDefExt = "hgr";
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
							recursing = false;
							break;
						}

						debugprintf("Attempting to determinate default thread stack size for the game...\n");
						localTASflags.threadStackSize = GetWin32ExeDefaultStackSize(exefilename);
						if(localTASflags.threadStackSize != 0)
						{
							debugprintf("Detecting the default stack size succeeded, size is: %u bytes\n", localTASflags.threadStackSize);
						}
						else
						{
							char str[1024];
							sprintf(str, "Determinating the default thread stack size failed!\nVerify that '%s' a valid Win32 executable.", exefilename);
							CustomMessageBox(str, "Error!", (MB_OK | MB_ICONERROR));
							recursing = false;
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
									if(!strncmp(exefname, moviefilename, strlen(exefname)-4)) // Compare names, without the file extension.
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
										strcpy(dot, ".hgr");
										const char* moviename = slash ? slash+1 : filename;
										strcat(path, moviename);
										if(0 != strcmp(path, moviefilename))
										{
											SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), path);
											movienameCustomized = false;
										}
									}
								}
							}
						//}

						recursing = false;
					}
					break;
				case IDC_TEXT_MOVIE: // Whenever IDC_TEXT_MOVIE changes, a new movie has been chosen for whatever reason.
					if(HIWORD(wParam) == EN_CHANGE)
					{

						// TODO: We can probably skip this alltogether and just make the check in OPENMOVIE.
						// Will require implementing ability to run games without recording movies.
						// This should do for now.
						char tmp_movie[MAX_PATH+1];
						GetWindowText(GetDlgItem(hDlg, IDC_TEXT_MOVIE), tmp_movie, MAX_PATH);
						NormalizePath(tmp_movie, tmp_movie);
						
						// HACK... TODO: Fix this properly, by completing TODO above!
						if(tmp_movie[0] == '\0') break; // No movie loaded, don't go forward with anything.

						EnableDisablePlayRecordButtons(hDlg);
						movienameCustomized = tmp_movie[0] != '\0';

						if(unsavedMovieData) // Check if we have some unsaved changes in an already loaded movie.
						{
							int result = CustomMessageBox("The currently opened movie contains unsaved data.\nDo you want to save it before opening the new movie?\n(Click \"Yes\" to save, \"No\" to proceed without saving)", "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
							if(result == IDYES)
							{
								// TODO:
								// IF SaveMovie fails here, it means that the movie file got deleted between the last save and the creation of the new movie
								// Should we really handle that scenario?
								SaveMovie(moviefilename);
							}
							unsavedMovieData = false;
							movie.headerBuilt = false; // Otherwise we may retain the old header? // TODO: Find this out
						}

						if(moviefilename[0] != '\0') // We have currently locked the directory that we're saving the old movie to.
						{
							UnlockDirectory(MOVIE);
						}

						char movieDirectory[MAX_PATH+1];
						// TODO: max will call strrchr 3 times in total, perhaps that can be reduced to 2 times?
						char* dirEnd = max(strrchr(tmp_movie, '\\'), strrchr(tmp_movie, '/'));
						unsigned int strLen = dirEnd - tmp_movie + 1; // Pointers-as-values math, yay!
						strncpy(movieDirectory, tmp_movie, (size_t)strLen);
						movieDirectory[strLen] = '\0'; // properly null-terminate the string.

						// Attempt to lock the new directory.
						if(LockDirectory(movieDirectory, MOVIE) == false)
						{
							moviefilename[0] = '\0'; // Let's not keep the old movie file name, or there could be some non-desired overwriting taking place.
							break; // Break early so that we don't allow the movie to be loaded.
						}

						// If we got here we can finally transfer the new movie filename into the old movie filename.
						strcpy(moviefilename, tmp_movie);
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
							else if(!_strnicmp(dot, ".hgr", 4)) // windows TAS file (input movie)
							{
								SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), filename);
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
//#include "inputsetup.cpp"
#include "trace/extendedtrace.cpp"
#include "inject/process.cpp"
#include "inject/iatmodifier.cpp"
#define UNITY_BUILD
#endif

