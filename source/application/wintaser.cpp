/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

// main EXE cpp
//
// TODO: split up this file. at least the AVI recording part can be separated.

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0500
// Windows Header Files:
#include <Windows.h>
#pragma comment(lib,"Version.lib") // Needed for the new Windows version detection

#include <mmsystem.h>
// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <string.h>
#include <psapi.h>
#include <strsafe.h>
//#include "stdafx.h"
//#include "svnrev.h" // defines SRCVERSION number
#include "shared/version.h"
#include "Resource.h"
#include "trace/ExtendedTrace.h"
#include "InjectDLL.h"
#include "CustomDLGs.h"
#include "Movie.h"
//#include "crc32.h"
//#include "CRCMath.h"
//#include "inputsetup.h"
#include "MD5Checksum.h"
#include "MainMenu.h"
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
#include <type_traits>
#include <sstream>
#include <aclapi.h>
#include <assert.h>

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

#include "AVIDumping\AVIDumper.h"

#include "external/ddraw.h"
#include "shared/logcat.h"
#include "shared/ipc.h"
#include "shared/asm.h"
#include "shared/winutil.h"
#include "ramwatch.h"

#include "CPUinfo.h"
#include "DirLocks.h"
#include "ExeFileOperations.h"
#include "Utils/File.h"

#include "DbgHelp/DbgHelp.h"

#pragma warning(disable:4995)

#ifndef CONTEXT_ALL
#define CONTEXT_ALL (CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)
#endif

HINSTANCE hInst;
WCHAR title[MAX_LOADSTRING];
WCHAR window_class[MAX_LOADSTRING];

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
///* Reserves memory to a given object. If it fails to do so at the first time,
// *      it tries again later, until memory of the asked size is finally reserved.
// *      The goal is to avoid an error because of a lack of free space.
// * @see VirtualAllocEx from WinBase.h from windows.h
// */
//LPVOID WINAPI RetryingVirtualAllocEx(IN HANDLE hProcess, IN LPVOID lpAddress,
//    IN SIZE_T dwSize, IN DWORD flAllocationType, IN DWORD flProtect)
//{
//	LPVOID rv;
//	for(int i = 0; i < s_safeMaxIter; i++)
//	{
//		rv = VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
//		if(rv /*|| GetLastError() != ERROR_ACCESS_DENIED*/)
//			break;
//		Sleep(s_safeSleepPerIter);
//	}
//	return rv;
//}
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
//BOOL WINAPI RetryingWriteProcessMemory(IN HANDLE hProcess, IN LPVOID lpBaseAddress,
//    IN LPCVOID lpBuffer, IN SIZE_T nSize, OUT SIZE_T * lpNumberOfBytesWritten)
//{
//	BOOL rv;
//	for(int i = 0; i < s_safeMaxIter; i++)
//	{
//		rv = WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
//		if(rv /*|| GetLastError() != ERROR_ACCESS_DENIED*/)
//			break;
//		Sleep(s_safeSleepPerIter);
//	}
//	return rv;
//}

/* Detects the Windows version currently used. 
 * 
 * Detects the Windows version currently used. For now it's slower than the
 *      naive IsWindowsXP() and such that you could find below, but at the
 *      same time this one is more reliable.
 * @param *major int pointer to where the Windows major version number will be written
 * @param *minor int pointer to where the Windows minor version number will be written  
 */
void DetectWindowsVersion(int *major, int *minor)
{
    debugprintf(L"Detecting Windows version...\n");
    WCHAR filename[MAX_PATH + 1];
    UINT value = GetWindowsDirectoryW(filename, MAX_PATH);
    if (value != 0)
	{
		// Fix for Windows installations in root dirs, GetWindowsDirectory does not include the final slash EXCEPT for when Windows is installed in root.
        if (filename[value - 1] == L'\\')
        {
            filename[value - 1] = L'\0';
        }

        wcscat(filename, L"\\System32\\kernel32.dll");
        debugprintf(L"Using file '%s' for the detection.\n", filename);

        UINT size = 0;

		// Despite being a pointer this must NOT be deleted by us, it will be taken care of properly by the destructors of the other structures when we delete verInfo.
		VS_FIXEDFILEINFO *buffer = nullptr;
		DWORD info_size = GetFileVersionInfoSizeW(filename, nullptr);

        if (info_size != 0)
		{
			BYTE *ver_info = new BYTE[info_size];

			// On some Windows versions the return value of GetFileVersionInfo is not to be trusted.
			// It may return TRUE even if the function failed, but it always sets the right error code.
			// So instead of catching the return value of the function, we query GetLastError for the function's return status.
            GetFileVersionInfoW(filename, 0, info_size, ver_info);
            if (GetLastError() == ERROR_SUCCESS)
			{
				BOOL parse_success = VerQueryValueW(ver_info, L"\\", reinterpret_cast<LPVOID*>(&buffer), &size);
                if (parse_success != FALSE && size > 0)
				{
                    if (buffer->dwSignature == 0xFEEF04BD)
					{
						/* 
						   kernel32.dll versions and their Windows counter parts:
						   5.2 = XP
						   6.0 = Vista
						   6.1 = Win7
						   6.2 = Win8
						   6.3 = Win8.1
						   Now don't these remind a lot of the actual windows versions?
						*/
                        *major = HIWORD(buffer->dwFileVersionMS);
                        *minor = LOWORD(buffer->dwFileVersionMS);

                        delete[] ver_info; // We no longer need to hold on to this.
						
                        debugprintf(L"Detection succeeded, detected version %d.%d\n", *major, *minor);
                        return;
					}
				}
			}
			delete [] ver_info; // Destroy this if we failed, we cannot have a memory leak now can we?
		}
	}
    debugprintf(L"Failed to determinate Windows version, using old unsafe method...\n");

    OSVERSIONINFOW osvi = { sizeof(OSVERSIONINFOW) };
    GetVersionExW(&osvi);
    *major = osvi.dwMajorVersion;
    *minor = osvi.dwMinorVersion;
    return;
}

//OSVERSIONINFO osvi = {sizeof(OSVERSIONINFO)};
// warning: we can't trust these too much (version lies from compatibility mode shims are common)
// Note: these seems to be used in a DLL. DetectWindowsVersion is also much slower.
//      They are kept as is for now.
bool IsWindowsXP()    { return localTASflags.osVersionMajor == 5 && (localTASflags.osVersionMinor == 1 || localTASflags.osVersionMinor == 2 ); }
bool IsWindowsVista() { return localTASflags.osVersionMajor == 6 && localTASflags.osVersionMinor == 0; }
bool IsWindows7()     { return localTASflags.osVersionMajor == 6 && localTASflags.osVersionMinor == 1; }
// Not used for anything yet, but at least the function is here now, just in case.
bool IsWindows8()	  { return localTASflags.osVersionMajor == 6 && (localTASflags.osVersionMinor == 2 || localTASflags.osVersionMinor == 3 ); }


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

/** Normalizes the path to ensure it is usable for the program.
 *
 * @param path const widechar string with path to normalize
 * @return normalized path as a widechar string.
 */
std::wstring NormalizePath(const std::wstring& path)
{
    extern bool started;
    if (path.empty() || started)
    {
        return path;
    }

    DWORD len = 0;
    WCHAR normalized_path[MAX_PATH + 1];
    if (!started)
    {
        len = GetFullPathNameW(path.c_str(), MAX_PATH, normalized_path, nullptr);
        if (len == 0)
            PrintLastError(L"GetFullPathName", GetLastError());
    }
    if (len != 0 && len < MAX_PATH)
    {
        GetLongPathNameW(normalized_path, normalized_path, MAX_PATH); // GetFullPathName won't always convert short filenames to long filenames
    }
    return normalized_path;
}

// Following code needs functions from C header files
#define BUFSIZE 512

/** Translates a path containing the name of a device to a path
 *		containing a drive letter.
 *
 * GetProcessImageFileName and friends from the PSAPI.dll returns
 *      paths with devices instead of letters, thus the need to
 *      convert the paths we get through PSAPI to usable paths.
 * @param *pszFilename TCHAR string with the path to translate.
 * @return bool true in case of success, false elsewhere.
 */
static std::wstring TranslateDeviceName(const std::wstring& filename)
{
    // Translate path with device name to drive letters.
    WCHAR temp_str[BUFSIZE];
    memset(temp_str, L'\0', sizeof(temp_str));

    if (GetLogicalDriveStringsW(BUFSIZE - 1, temp_str) != 0)
    {
        // temp_str contains list of valid drives in the system.
        WCHAR name[MAX_PATH];
        WCHAR drive[3] = L" :";
        bool found = false;
        WCHAR* p = temp_str;

        // Try every valid device name until one matches the path, in that case the path is translated
        do {
            *drive = *p;
            if (QueryDosDeviceW(drive, name, MAX_PATH) != 0)
            {
                size_t name_len = wcslen(name);
                if (name_len < MAX_PATH)
                {
                    found = (_wcsnicmp(filename.c_str(), name, name_len) == 0);
                    if (found)
                    {
                        WCHAR temp_file[MAX_PATH];
                        int len = swprintf(temp_file, MAX_PATH, L"%s%s", drive, filename.c_str() + name_len);
                        if (len > sizeof(temp_file))
                        {
                            return L"";
                        }
                        wcsncpy(temp_str, temp_file, len + 1);
                        break;
                    }
                }
            } while (*p++);
        } while (!found && *p);
        return NormalizePath(temp_str);
    }
    return L"";
}

/** Retrieve the path of the executable that generated the given process.
 *      It is mostly GetProcessImageFileNameA() with a usable path.
 *
 * @see GetProcessImageFileName()
 * TODO hypothesis: this is to differenciate between a thread of the game
 *      and another thread.
 * @param hProcess HANDLE to a process.
 * @param filename char* where the path will be stored.
 */
std::wstring GetFileNameFromProcessHandle(HANDLE hProcess)
{
    WCHAR filename[MAX_PATH + 1];
    if (GetProcessImageFileNameW(hProcess, filename, MAX_PATH) != 0)
    {
        return TranslateDeviceName(filename);
    }
    return L"";
}
/** Retrieve the path of the executable that generated the given file handle.
 * TODO why?
 *
 * @param hFile HANDLE to a file.
 * @param pszFilename TCHAR* where the path will be stored.
 */
std::wstring GetFileNameFromFileHandle(HANDLE file) 
{
    // Creates a mapping of a given file, then tries to relate the mapping
    // to a possibly existing other mapping of the same file.
    // In case of success, the path we got is translated to be usable.
    bool success = false;
    WCHAR filename[MAX_PATH + 1];
    HANDLE mapped_file = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 1, NULL);
    if (mapped_file != nullptr)
    {
        LPVOID mem = MapViewOfFile(mapped_file, FILE_MAP_READ, 0, 0, 1);
        if (mem != nullptr)
        {
            if (GetMappedFileNameW(GetCurrentProcess(), mem, filename, MAX_PATH) != 0)
            {
                success = true;
            }
            UnmapViewOfFile(mem);
        }
        CloseHandle(mapped_file);
    }
    if (!success)
    {
        return L"";
    }
    return TranslateDeviceName(filename);
}

/** Get the thread suspend count.
 * This is needed to save the threads when saving the gamestate.
 *
 * @param hThread HANDLE to the thread you want to save the count.
 * @return count integer with the suspend count.
 */
int GetThreadSuspendCount(HANDLE hThread)
{
	int count = SuspendThread(hThread);
	ResumeThread(hThread);
	return count;
}
/** Set the thread suspend count.
 * This is needed to restore the threads when loading the gamestate.
 *
 * @param hThread HANDLE to the thread to initialize.
 * @param count integer with the desired count.
 */
void SetThreadSuspendCount(HANDLE hThread, int count)
{
	int suspendCount;
	do { suspendCount = SuspendThread(hThread);
	} while(suspendCount != -1 && suspendCount < count-1);
	do { suspendCount = ResumeThread(hThread);
	} while(suspendCount != -1 && suspendCount > count+1);
}





//std::map<LPVOID,HANDLE> dllBaseToHandle;
std::map<LPVOID, std::wstring> dll_base_to_filename;

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



std::wstring injected_dll_path;
std::wstring sub_exe_filename;



//int emuMode = EMUMODE_EMULATESOUND;

//int fastForwardFlags = FFMODE_FRONTSKIP | FFMODE_BACKSKIP | FFMODE_RAMSKIP | FFMODE_SLEEPSKIP;

//int timescale = 1, timescaleDivisor = 1;

static bool warnedAboutFrameRateChange = false;

//bool recoveringStale = false;
static int recoveringStaleSlot = 0;
static int recoveringStaleFrame = 0;


static char localCommandSlot[256];




void UpdateFrameCountDisplay(int frameCount, int frequency);

/** Refresh the rerecord count displayed */
void UpdateRerecordCountDisplay()
{
    WCHAR rr_count[256];
    swprintf(rr_count, L"%u", movie.rerecordCount);
    WCHAR old_rr_count[256];
    GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), old_rr_count, 256);
    if (wcscmp(rr_count, old_rr_count))
    {
        SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), rr_count);
    }
}

/** Calculates the size taken by the given file. 
 * @see CalcExeFilesize()
 * @param path char* with the path to the file.
 * @return size integer with the number of bytes of given file, 
 *			    or 0 if the file has not been found.
 */
LONGLONG CalcFilesize(const std::wstring& path)
{
    BOOL rv;
    LARGE_INTEGER file_size;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    memset(&file_size, 0, sizeof(file_size));
    rv = GetFileSizeEx(file, &file_size);
    CloseHandle(file);
    if (rv == FALSE)
    {
        return 0;
    }
    return file_size.QuadPart;
}
/** Calculats the size of the currently used .exe game.
 * @return size integer with the number of bytes of given file, 
 *			    or 0 if the file has not been found.
 */
LONGLONG CalcExeFilesize()
{
	return CalcFilesize(exe_filename.c_str());
}


//DWORD CalcExeCrc()
//{
//	return CalcFileCrcCached(exefilename);
//}

/** Get the name of the file without the rest of the path.
 * @see GetExeFilenameWithoutPath()
 * @param path char* with the filepath
 * @return path char* with the filename alone
 */
std::wstring GetFilenameWithoutPath(const std::wstring& path)
{
    size_t last = path.find_last_of(L"\\/");
    if (last == std::wstring::npos)
    {
        return path;
    }
    return path.substr(last + 1);
}

/** Get the name of the .exe file without the rest of the path.
 * @see GetExeFilenameWithoutPath() FIXME why is this here? Maybe it was supposed to be @see GetGilenameWithoutPath?
 * @return path char* with the filename alone
 */
std::wstring GetExeFilenameWithoutPath()
{
	return GetFilenameWithoutPath(exe_filename);
}

/** TODO clarifying.
 * Seems to be a HANDLE for data that could be shared between savestates.
 * @see SaveState, FindMarchingDataBlock()
 */
struct SharedDataHandle
{
    LPBYTE const dataPtr;
	int userdata; // used in functions as "I checked this place already"

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
	SharedDataHandle(LPBYTE data) : refCount(1), dataPtr(data), userdata(0) {}
private:
	int refCount;
	SharedDataHandle(const SharedDataHandle& c);
};

/** See it as a shortcut to clean up the memory for any kind of container
 * @param c reference to a container, any type of container.
 */
template<typename T>
static void ClearAndDeallocateContainer(T& c)
{
	T temp;
	c.swap(temp);
}


/** Structure for managing savestates.
 * nitsuja's commentary: 
 *		holds the state of a Windows process. the goal is for this to be so complete
 *		that the process can later be restored to exactly the same state,
 *		or at least pretty close to that for simple games during the samesession.
 */
struct SaveState
{
	struct MemoryRegion
	{
		MEMORY_BASIC_INFORMATION info;
		LPBYTE data;
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

	/** TODO : why Deallocate doesn't completely clear? */
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

/** Find in the savestates the block of memory corresponding (NOTE: containing?)
 *		the given MemoryRegion. In case of failure, 0 is returned.
 * @param &region a SaveState::MemoryRegion reference
 * @return ___ the pointer to the SharedDataHandle structure, or 0 if not found.
 */
SharedDataHandle* FindMatchingDataBlock(SaveState::MemoryRegion& region)
{
	// Initializes the savestate's memories's datahandles as unchecked for the next loop
	for(int i = 0; i < maxNumSavestates; i++)
		for(unsigned int j = 0; j < savestates[i].memory.size(); j++)
			savestates[i].memory[j].dataHandle->userdata = 0;

	for(int i = 0; i < maxNumSavestates; i++)
	{
		SaveState& savestate = savestates[i];
		for(unsigned int j = 0; j < savestate.memory.size(); j++)
		{
			SaveState::MemoryRegion& region2 = savestate.memory[j];
			// if for a given MemoryRegion, compared to the parameter: 
			//		their MEMORY_BASIC_INFORMATION.BaseAddress are the same
			//		the MemoryRegion region size is at least as big as param's region size
			//		and the MemoryRegion hasn't been checked already
			//			(note: a MemoryRegion could be shared between two savestates)
			// then we compare the memory of current MemoryRegion and the region param
			if(region2.info.BaseAddress == region.info.BaseAddress
			&& region2.info.RegionSize >= region.info.RegionSize
			&& region2.dataHandle->userdata == 0)
			{
				// Check if region2's data contains region's data
				if(0 == memcmp(region.data, region2.data, region.info.RegionSize))
				{
					// found a perfectly-matching region of memory
					return region2.dataHandle;
				}
				// avoids re-checking same data in case it is in another savestate
				region2.dataHandle->userdata = 1;
			}
		}
	}
	return 0; // no match found
}




bool movienameCustomized = false;

CRITICAL_SECTION g_gameHWndsCS;

std::set<HWND> gameHWnds;
HANDLE hGameProcess = 0;
/**
 * TODO
 */
struct ThreadInfo
{
	//DWORD threadId; // not included because it's already the map index
	HANDLE handle;
	HANDLE hProcess;
	//DWORD waitingCount;
	//DWORD hopelessness;
    char name[64];

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

/** TODO why */
static void* remoteCommandSlot = 0;

/* 
 * Shortcut to set where remoteCommandSlot points.
 */
void ReceiveCommandSlotPointer(LPVOID command_slot_pointer)
{
	remoteCommandSlot = command_slot_pointer;
}

void SendCommand();
void ClearCommand();


/** TODO why */
static void* remoteInputs = 0;

/*
 * Shortcut to set where remoteInputs points.
 */
void ReceiveInputsPointer(LPVOID remote_inputs_pointer)
{
	remoteInputs = remote_inputs_pointer;
}

/** TODO why */
static void* remoteTASflags = 0;

void SendTASFlags();

/** Shortcut to set where remoteTASflags points.
 *      It thus upates the local TAS flags.
 * @param pointerAsInt the int64 value of a pointer.
 */
void ReceiveTASFlagsPointer(LPVOID tas_flags_pointer)
{
	remoteTASflags = tas_flags_pointer;
	SendTASFlags();
}


static void* remoteDllLoadInfos = 0;

DllLoadInfos dllLoadInfos = {};
bool dllLoadInfosSent = false;

void ReceiveDllLoadInfosPointer(LPVOID dll_load_info_pointer)
{
	remoteDllLoadInfos = dll_load_info_pointer;
	dllLoadInfos.numInfos = 0;
}


static void* remoteTrustedRangeInfos = 0;

void ReceiveTrustedRangeInfosPointer(LPVOID trusted_range_infos_pointer, HANDLE process)
{
	remoteTrustedRangeInfos = trusted_range_infos_pointer;
	void SendTrustedRangeInfos(HANDLE hProcess);
	SendTrustedRangeInfos(process);
}


static void* remoteGeneralInfoFromDll = 0;

void ReceiveGeneralInfoPointer(LPVOID general_info_pointer)
{
    remoteGeneralInfoFromDll = general_info_pointer;
}

void ReceiveSoundCaptureInfoPointer(LPVOID sound_capture_info_pointer)
{
	SetLastFrameSoundInfo(sound_capture_info_pointer);
}

//Since GammaRamp is only used for AVI capture there is really no need to 
void ReceiveGammaRampData(LPVOID gamma_ramp_pointer, HANDLE process)
{
	SetGammaRamp(gamma_ramp_pointer, process);
}



void ReceivePaletteEntriesPointer(LPVOID palette_entries_pointer)
{
	SetPaletteEntriesPointer(palette_entries_pointer);
}

//void ReceiveExtHWndBuf(__int64 pointerAsInt)
//{
//	SIZE_T bytesWritten = 0;
//	WriteProcessMemory(hGameProcess, (void*)pointerAsInt, &hExternalWnd, sizeof(HWND), &bytesWritten);
//}

void ReceiveKeyboardLayout(LPVOID keyboard_layout_pointer, HANDLE hProcess)
{
	char name [KL_NAMELENGTH] = {0};
	SIZE_T bytesWritten = 0;
	// get the active layout
	ReadProcessMemory(hProcess, keyboard_layout_pointer, name, KL_NAMELENGTH, &bytesWritten);
	name[KL_NAMELENGTH-1] = 0;
	// replace the active layout with one stored in the movie, or vice-versa
	if(localTASflags.playback && *movie.keyboardLayoutName)
		strcpy(name, movie.keyboardLayoutName);
	else
		strcpy(movie.keyboardLayoutName, name);
	// set the possibly-new layout
	// NOTE: maybe move that under the if? Only needs to be called if name was updated.
	WriteProcessMemory(hProcess, keyboard_layout_pointer, name, KL_NAMELENGTH, &bytesWritten);
}


void CheckSrcDllVersion(DWORD version)
{
	if(version != VERSION)
	{
		LPCWSTR str;
		if(version > VERSION)
			str = L"Wrong version detected: This hourglass.exe is too old compared to hooks.dll.\nPlease make sure you have extracted Hourglass properly.\nThe files that came with hourglass.exe must stay together with it.";
		else
			str = L"Wrong version detected: hooks.dll is too old compared to this hourglass.exe.\nPlease make sure you have extracted Hourglass properly.\nThe files that came with hourglass.exe must stay together with it.";
		debugprintf(L"%s\n", str);
		CustomMessageBox(str, L"Version Problem", MB_OK | MB_ICONWARNING);
	}
}


#ifdef _DEBUG
// TEMP
static char localCommandSlot_COPY[256];
#endif

void SuggestThreadName(DWORD threadId, HANDLE hProcess, IPC::SuggestThreadName& thread_name)
{
    thread_name.SetThreadName("unknown");
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


void SaveMovie(const std::wstring& filename)
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
		CalcFileMD5Cached(exe_filename.c_str(), movie.fmd5);
		movie.fsize = CalcExeFilesize();
		wcscpy(movie.commandline, command_line.c_str());
		movie.headerBuilt = true;
	}

	if(SaveMovieToFile(movie, filename.c_str()))
	{
		unsavedMovieData = false;
	}
	// TODO: Should be an else-block here with error handling, no?
}

// returns 1 on success, 0 on failure, -1 on cancel
// TODO: Dependency on parameter can probably be removed.
int LoadMovie(const std::wstring& filename)
{
	// NOTE: if ( !LoadMovieFromFile(movie, filename) ) return 0; Maybe do it like that? Cleaner.
	bool rv = LoadMovieFromFile(movie, filename.c_str());
	if(rv == false) return 0; // Check if LoadMovieFromFile failed, if it did we don't need to continue.

	if(localTASflags.playback)
	{
		if(movie.frames.size() == 0)
		{
			CustomMessageBox(L"This movie file doesn't contain any frames, playback is not possible.", L"Error!", (MB_OK | MB_ICONERROR));
			return 0; // Even if opening the movie was successful, it cannot be played back, so it "failed".
		}
		if(movie.version != VERSION)
		{
            WCHAR str[1024];
			swprintf(str,
			L"This movie was recorded using a different main version of Hourglass Resurrection.\n"
			L"\n"
			L"Movie's version: %u\n"
			L"Program version: %u\n"
			L"\n"
			L"This may lead to the movie desyncing.\n"
			L"If it would desync you might want to use the movies main version of Hourglass Resurrection.\n"
			L"\n"
			L"Do you want to try playing back the movie anyway?\n"
			L"(Click \"Yes\" to continue, \"No\" to abort)\n",
			movie.version, (unsigned int)VERSION);

			int result = CustomMessageBox(str, L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1)); // MS_DEFBUTTON1 marks "Yes" as the default choice.
			if(result == IDNO)
			{
				return -1;
			}
			movie.version = VERSION;
		}
		unsigned int temp_md5[4];
		CalcFileMD5Cached(exe_filename.c_str(), temp_md5);
		if(memcmp(movie.fmd5, temp_md5, 4*4) != 0)
		{
			WCHAR str[1024];
			swprintf(str, L"This movie was probably recorded using a different exe.\n\n"
						 L"Movie's exe's md5: %X%X%X%X, size: %d\n"
						 L"Current exe's md5: %X%X%X%X, size: %lld\n\n"
						 L"Playing the movie with current exe may lead to the movie desyncing.\n"
						 L"Do you want to continue?\n(Click \"Yes\" to continue, \"No\" to abort)",
						 movie.fmd5[0], movie.fmd5[1], movie.fmd5[2], movie.fmd5[3], static_cast<int>(movie.fsize), 
						 temp_md5[0], temp_md5[1], temp_md5[2], temp_md5[3], CalcExeFilesize());
			int result = CustomMessageBox(str, L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2));
			if(result == IDNO)
			{
				return -1;
			}
		}
		if(movie.fps != localTASflags.framerate)
		{
			WCHAR str[1024];
			swprintf(str, L"This movie was recorded using a different fps.\n\n"
						 L"Movie's fps: %d\nCurrent fps: %d\n\n"
						 L"Playing the movie with current fps may lead to the movie desyncing.\n"
						 L"Do you want to use the movies fps instead?\n"
						 L"(Click \"Yes\" to use the movies fps, \"No\" to use current fps)",
						 static_cast<int>(movie.fps), localTASflags.framerate);
			int result = CustomMessageBox(str, L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				localTASflags.framerate = movie.fps;
				// Also update the window text.
				WCHAR fpstext[256];
				swprintf(fpstext, L"%d", localTASflags.framerate);
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_FPS), fpstext);
			}
		}
		if(movie.it != localTASflags.initialTime)
		{
			WCHAR str[1024];
			swprintf(str, L"This movie was recorded using a different initial time.\n\n"
						 L"Movie's initial time: %d\n"
						 L"Current initial time: %d\n\n"
						 L"Playing the movie with current initial time may lead to the movie desyncing.\n"
						 L"Do you want to use the movies initial time instead?\n"
						 L"(Click \"Yes\" to use the movies initial time, \"No\" to use current initial time)",
						 static_cast<int>(movie.it), localTASflags.initialTime);
			int result = CustomMessageBox(str, L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				localTASflags.initialTime = movie.it;
				// Also update the window text.
				WCHAR ittext[256];
				swprintf(ittext, L"%d", localTASflags.initialTime);
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), ittext);
			}
		}
		if(wcscmp(movie.commandline, command_line.c_str()) != 0)
		{
			WCHAR str[1024];
			swprintf(str, L"This movie was recorded using a different command line.\n\nMovie's command line: %s\nCurrent command line: %s\n\nPlaying the movie with current command line may lead to the movie desyncing.\nDo you want to use the movies command line instead?\n(Click \"Yes\" to use the movies command line, \"No\" to use current command line)", movie.commandline, command_line.c_str());
			int result = CustomMessageBox(str, L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
			if(result == IDYES)
			{
				command_line = movie.commandline;
				// Also update the window text.
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), command_line.c_str());
			}
		}
	}
	else // !localTASflags.playback ... i.e. record new movie.
	{
		if(movie.frames.size() > 0)
		{
			int rv = CustomMessageBox(L"This movie file contains frame data.\nAre you sure you want to overwrite it?\n(Click \"Yes\" to overwrite movie, \"No\" to abort)", L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2));
			if(rv == IDNO) return -1; // Abort
		}
	}

	movie.headerBuilt = true;

	UpdateFrameCountDisplay(movie.currentFrame, 1);
	UpdateRerecordCountDisplay();

	return 1; // We made it!
}

/**
* Saves game's threads and (writable) memory into the savestate
*     of the given slot.
*
* @param slot The slot of the savestate to save to.
*/
void SaveGameStatePhase2(int slot)
{
	AutoCritSect cs(&g_processMemCS);

	localTASflags.stateLoaded = false;
	SendTASFlags();

	if(finished && !recoveringStale)
	{
		debugprintf(L"DESYNC WARNING: tried to save state while movie was \"Finished\"\n");
		LPCWSTR str = L"Warning: The movie is in \"Finished\" status.\n"
			L"This means that any input you entered after the movie became \"Finished\" was lost.\n"
			L"Any state saved now will contain a movie that immediately desyncs at that point.\n"
			L"Are you sure you really want to save the state now?\n";
		int result = CustomMessageBox(str, L"Desync Warning", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING);
		if(result == IDNO)
			return;
	}

	debugprintf(L"SAVED STATE %d\n", slot);

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
				debugprintf(L"ERROR: Failed to suspend thread?? hThread=0x%X, lastError=0x%X\n", thread.handle, GetLastError());
			else
			{
				thread.context.ContextFlags = CONTEXT_ALL;
				if(GetThreadContext(thread.handle, &thread.context))
					state.threads.push_back(thread);
				else
					debugprintf(L"ERROR: Failed to save thread?? hThread=0x%X, lastError=0x%X\n", thread.handle, GetLastError());
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
					// If we stored this memory before
					region.dataHandle = FindMatchingDataBlock(region);
					if(region.dataHandle)
					{
						// Then don't store it again to reduce memory usage.
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
					verbosedebugprintf(L"WARNING: couldn't save memory: baseAddress=0x%X, regionSize=0x%X, lastError=0x%X\n",
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
		SaveMovie(movie_filename);
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
		debugprintf(L"%d savestates, %g MB logical, %g MB actual\n", numValidStates, totalBytes/(float)(1024*1024), totalUniqueBytes/(float)(1024*1024));
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
	WCHAR str [2048];
	if(bestStateToUse <= -2)
	{
		swprintf(str,
			L"Warning: The savestate you are trying to load has gone stale.\n"
			L"The only way to recover it is by playing the movie file within it.\n"
			L"Attempt to recover the savestate by playing the movie from the beginning?\n"
			L"Doing so will make all other savestates become stale (at least initially).\n"
			L"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, emulateFramesRemaining);
	}
	else if(bestStateToUse == -1)
	{
		swprintf(str,
			L"Warning: The savestate you are trying to load has gone stale.\n"
			L"The only way to recover it is by playing the movie file within it.\n"
			L"Attempt to recover the savestate by continuing the movie from the current state?\n"
			L"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, emulateFramesRemaining);
	}
	else
	{
		swprintf(str,
			L"Warning: The savestate you are trying to load has gone stale.\n"
			L"The only way to recover it is by playing the movie file within it.\n"
			L"Attempt to recover the savestate by continuing the movie from savestate %d?\n"
			L"(It will emulate %d frames in fast-forward before automatically pausing.)"
			, bestStateToUse, emulateFramesRemaining);
	}
	int result = CustomMessageBox(str, L"Stale Savestate", MB_YESNO | MB_ICONQUESTION);
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
		PostMessageW(hWnd, WM_COMMAND, IDC_BUTTON_STOP, 42);
//			PostMessage(hWnd, WM_COMMAND, IDC_BUTTON_PLAY, 0);
	}
}

/**
* Loads game's threads and (writable) memory from the savestate
*     of the given slot.
*
* @param slot The slot of the savestate to load from.
*/
void LoadGameStatePhase2(int slot)
{
	AutoCritSect cs(&g_processMemCS);

	SaveState& state = savestates[slot];
	if(!state.valid)
	{
		debugprintf(L"NO STATE %d TO LOAD\n", slot);
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
		debugprintf(L"LOADED STATE %d\n", slot);
	else
		debugprintf(L"RECOVERING STATE %d\n", slot);


	// OK, this is going to be way harder than saving the state,
	// since we have to reconcile the differences in allocated threads and memory regions

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
					debugprintf(L"FAILED TO LOAD THREAD: id=0x%X, handle=0x%X\n", thread.id, thread.handle);
				}

				//if(thread.suspendCount)
				//	continue;

				verbosedebugprintf(L"Loaded thread: handle=0x%X, id=0x%X, suspendcount=0x%X\n", thread.handle, thread.id, thread.suspendCount);

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
				if(allocAddr != mbi.BaseAddress && mbi.State == MEM_COMMIT && GetLastError() != 0x5) // NOTE: Replace with ERROR_ACCESS_DENIED for the sake of cleaner code?
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
					debugprintf(L"FAILED TO COMMIT MEMORY REGION: BaseAddress=0x%08X, AllocationBase=0x%08X, RegionSize=0x%X, ResultAddress=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.AllocationBase, mbi.RegionSize, allocAddr, GetLastError());
				DWORD dwOldProtect = 0;
				BOOL protectResult = VirtualProtectEx(hGameProcess, mbi.BaseAddress, mbi.RegionSize, mbi.Protect & ~PAGE_GUARD, &dwOldProtect); 
				if(!protectResult )//&& !(mbi.Type & MEM_IMAGE))
					debugprintf(L"FAILED TO PROTECT MEMORY REGION: BaseAddress=0x%08X, RegionSize=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.RegionSize, GetLastError());

				SIZE_T bytesWritten = 0;
				BOOL writeResult = WriteProcessMemory(hGameProcess, mbi.BaseAddress, region.data, mbi.RegionSize, &bytesWritten);
				if(!writeResult )//&& !(mbi.Type & MEM_IMAGE))
					debugprintf(L"FAILED TO WRITE MEMORY REGION: BaseAddress=0x%08X, RegionSize=0x%X, LastError=0x%X\n", mbi.BaseAddress, mbi.RegionSize, GetLastError());

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
			debugprintf(L"MOVIE END WARNING: loaded movie frame %d comes after end of current movie length %d\n", state.movie.currentFrame, movie.frames.size());
			bool warned = false;
			if(!warned)
			{
				warned = true;
				WCHAR str [1024];
				swprintf(str, L"Warning: Loaded state is at frame %d, but current movie is only %d frames long.\n"
					L"You should load a different savestate before continuing, or switch to read+write and reload it."
					, state.movie.currentFrame, static_cast<int>(movie.frames.size()));
				CustomMessageBox(str, L"Movie End Warning", MB_OK | MB_ICONWARNING);
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
					debugprintf(L"DESYNC WARNING: loaded movie has mismatch on frame %d\n", i);
					if(!warned)
					{
						warned = true;
						WCHAR str [1024];
						swprintf(str, L"Warning: Loaded state's movie input does not match current movie input on frame %d.\n"
							L"This can cause a desync. You should load a different savestate before continuing.", i);
						CustomMessageBox(str, L"Desync Warning", MB_OK | MB_ICONWARNING);
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
		SaveMovie(movie_filename);
	}

	// done... actually that wasn't so bad
}

bool CreateAVIFile()
{
    std::wstring avi_file_name = Utils::File::GetFileNameSave(this_process_path, {
        Utils::File::FileFilter::AVI,
        Utils::File::FileFilter::AllFiles
    });

    if (avi_file_name.empty())
    {
        return false;
    }

    // Attempt to inform the AVI dumper about the path to dump to
    bool success = SetAVIFilename(avi_file_name.c_str());

    if (!success)
    {
        return false;
    }

    // If hGameProcess has a value at this point and file creation was successful,
    // register hGameProcess with the AVI Dumper now so that we can start capture immediately
    if (hGameProcess)
    {
        SetCaptureProcess(hGameProcess);
    }

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
		debugprintf(__FUNCTIONW__ L" ERROR!!! overwrote earlier command before it executed. replaced \"%S\" with \"%S\".\n", tempCommandSlot, localCommandSlot);
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
	if(localTASflags.fastForward != static_cast<int>(set))
	{
		localTASflags.fastForward = set;
		tasFlagsDirty = true;
	}
}
void SaveGameStatePhase1(int slot)
{
	if(!remoteCommandSlot)
		return;
	::sprintf(localCommandSlot, "SAVE: %d", slot);
	SendCommand();
	cannotSuspendForCommand = true;
}
void LoadGameStatePhase1(int slot)
{
	if(!remoteCommandSlot)
		return;
	::sprintf(localCommandSlot, "LOAD: %d", slot);
	SendCommand();
	cannotSuspendForCommand = true;
}
void HandleRemotePauseEvents()
{
	if(truePause && !terminateRequest)
		return;
	if(!remoteCommandSlot)
		return;
	::sprintf(localCommandSlot, "HANDLEEVENTS: %d", 1);
	SendCommand();
}

/** Updates the flags of the current TAS. */
void SendTASFlags()
{
	if(!started)
		return;
	
	//Some updates that have to be done here
	localTASflags.emuMode = localTASflags.emuMode
	    | (((recoveringStale||(localTASflags.fastForwardFlags&FFMODE_SOUNDSKIP))      
	         && localTASflags.fastForward)
           ? EMUMODE_NOPLAYBUFFERS : 0)
        | ((localTASflags.threadMode==0 
                || localTASflags.threadMode==4
                ||localTASflags.threadMode==5)
           ? EMUMODE_VIRTUALDIRECTSOUND : 0);
	//localTASflags.fastForwardFlags = localTASflags.fastForwardFlags | (recoveringStale ? (FFMODE_FRONTSKIP|FFMODE_BACKSKIP) ? 0);
	localTASflags.appLocale = localTASflags.appLocale ? localTASflags.appLocale : tempAppLocale;
	//localTASflags.includeLogFlags = includeLogFlags | traceLogFlags;
	//localTASflags.excludeLogFlags = excludeLogFlags;

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
		verbosedebugprintf(L"RECORD: wrote to movie frame %d\n", movie.currentFrame);
		unsavedMovieData = true;
	}
	else if(!finished)
	{
		finished = true;
		debugprintf(L"RECORDING STOPPED because %d != %d\n", movie.currentFrame, (int)movie.frames.size());
	}
}


void InjectCurrentMovieFrame() // playback ... or recording, now
{
	if((unsigned int)movie.currentFrame >= (unsigned int)movie.frames.size())
	{
		if(!finished)
		{
			finished = true;
			debugprintf(L"MOVIE END (%d >= %d)\n", movie.currentFrame, (int)movie.frames.size());
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
		debugprintf(L"failed to write input!!\n");
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
void AddAndSendDllInfo(LPCWSTR filename, bool loaded, HANDLE hProcess)
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
			LPCWSTR slash = std::max(wcsrchr(filename, L'\\'), wcsrchr(filename, L'/'));
			LPCWSTR dllname = slash ? slash+1 : filename;
			// insert at start, since dll consumes from end
			memmove(&dllLoadInfos.infos[1], dllLoadInfos.infos, dllLoadInfos.numInfos*sizeof(*dllLoadInfos.infos));
			DllLoadInfo& info = dllLoadInfos.infos[0];
			snprintf(info.dllname, sizeof(info.dllname), "%S", dllname);
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
			WCHAR str [256];
			WCHAR str2 [256];
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
			swprintf(str, L"%d", localGeneralInfoFromDll.ticks);
			GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str2, 256);
			if(wcscmp(str,str2))
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str);
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
				WCHAR str [256];
				swprintf(str, L"%d", frameCount);
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_CURFRAME), str);
				displayedFrameCount = frameCount;
			}
			if(maxcount != displayedMaxFrameCount)
			{
				WCHAR str [256];
				swprintf(str, L"%d", maxcount);
				SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_MAXFRAME), str);
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

					WCHAR str [256];
					swprintf(str, L"%dh %dm %02.2fs   /   %dh %dm %02.2fs",
						hours, minutes, seconds,  maxhours, maxminutes, maxseconds);
					SetWindowTextW(GetDlgItem(hWnd, IDC_STATIC_MOVIETIME), str);
				}
				else
				{
					SetWindowTextW(GetDlgItem(hWnd, IDC_STATIC_MOVIETIME), L"");
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
		localTASflags.playback != static_cast<int>(displayed_playback) ||
		finished != displayed_finished)
	{
		displayed_started = started;
		displayed_playback = (localTASflags.playback != 0);
		displayed_finished = finished;
		LPCWSTR str;
		if(started)
			if(localTASflags.playback)
				if(finished)
					str = L"Current Status: Finished";
				else
					str = L"Current Status: Playing";
			else
				str = L"Current Status: Recording";
		else
			str = L"Current Status: Inactive";
		SetWindowTextW(GetDlgItem(hWnd, IDC_STATIC_MOVIESTATUS), str);
		InvalidateRect(GetDlgItem(hWnd, IDC_STATIC_FRAMESLASH), NULL, TRUE);
		SendMessageW(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), EM_SETREADONLY, started, 0);
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
		SendMessageW(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), EM_SETREADONLY, started, 0);
		//SendMessage(GetDlgItem(hWnd, IDC_TEXT_MOVIE), EM_SETREADONLY, started, 0);
		SendMessageW(GetDlgItem(hWnd, IDC_EDIT_RERECORDS), EM_SETREADONLY, started, 0);
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
	if(!displayed_checkdialog_inited || static_cast<int>(displayed_fastforward) != localTASflags.fastForward)
	{
		displayed_fastforward = (localTASflags.fastForward != 0);
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

void FrameBoundary(IPC::FrameBoundaryInfo& frame_info, int threadId)
{
    if (!requestedCommandReenter)
    {
        // if it's actually a new frame instead of just paused
        s_lastFrameCount = frame_info.GetTotalRanFrames();
        int frameCountUpdateFreq = localTASflags.framerate / 2;
        if (localTASflags.fastForward && !(localTASflags.aviMode & 1))
            frameCountUpdateFreq = localTASflags.framerate * 8;
        UpdateFrameCountDisplay(frame_info.GetTotalRanFrames(), frameCountUpdateFreq);

        // temp workaround for games that currently spray pixels onscreen when they boot up (cave story)
        if (frame_info.GetTotalRanFrames() == 1)
        {
            InvalidateRect(hWnd, NULL, TRUE);
        }
        HandleAviSplitRequests();

        if (frame_info.GetCaptureInfoType() != CAPTUREINFO_TYPE_NONE_SUBSEQUENT)
        {
            // handle AVI recording
            if (localTASflags.aviMode & 1)
            {
                ProcessCaptureFrameInfo(frame_info.GetCaptureInfo(), frame_info.GetCaptureInfoType());
            }
            if (localTASflags.aviMode & 2)
            {
                ProcessCaptureSoundInfo();
            }
        }

        // update ram search/watch windows
        Update_RAM_Search();

        // handle skipping lag frames if that option is enabled
        temporaryUnpause = (frame_info.GetCaptureInfoType() == CAPTUREINFO_TYPE_PREV) && advancePastNonVideoFrames;
    }
    else
    {
        temporaryUnpause = false;
    }

    if (requestedCommandReenter && !cannotSuspendForCommand)
    {
        ResumeAllExcept(threadId);
    }

    // handle hotkeys
    do
    {
        requestedCommandReenter = false;
        cannotSuspendForCommand = false;
        RefreshSavestates(frame_info.GetTotalRanFrames());
        if (!requestedCommandReenter)
        {
            CheckHotkeys(frame_info.GetTotalRanFrames(), true);
        }
        CheckDialogChanges(frame_info.GetTotalRanFrames());
        if (paused)
        {
            UpdateFrameCountDisplay(frame_info.GetTotalRanFrames(), 1);
            if (!requestedCommandReenter)
            {
                Sleep(5);
            }
            if (!temporaryUnpause && !requestedCommandReenter) // check to avoid breaking... everything
            {
                HandleRemotePauseEvents();
            }
        }
    } while (paused && !(temporaryUnpause || requestedCommandReenter));

    if (requestedCommandReenter && !cannotSuspendForCommand)
    {
        SuspendAllExcept(threadId);
    }

    // handle movie playback / recording
    if (!requestedCommandReenter)
    {
        if (!localTASflags.playback)
        {
            RecordLocalInputs();
        }
        InjectCurrentMovieFrame();
        if (!(frame_info.GetTotalRanFrames() & (frame_info.GetTotalRanFrames() - 1)))
        {
            DoPow2Logic(frame_info.GetTotalRanFrames());
        }
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
						WCHAR str [256];
						swprintf(str, L"DESYNC DETECTED: on frame %d, your timer = %d but movie's timer = %d.\nThat means this playback of the movie desynced somewhere between frames %d and %d.", frameCount, localGeneralInfoFromDll.ticks, movieTime, frameCount, frameCount>>1);
						debugprintf(L"%s\n", str);
						CustomMessageBox(str, L"Desync Warning", MB_OK | MB_ICONWARNING);
					}
					movie.desyncDetectionTimerValues[which] = localGeneralInfoFromDll.ticks;
				}
			}
		}
	}

}

static WCHAR dllLeaveAloneList [256][MAX_PATH+1];

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
			debugprintf(L"ERROR: suspendall failed to find requesting thread id=0x%X in list.\n", ignoreThreadID);
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
			debugprintf(L"ERROR: resumeall failed to find requesting thread id=0x%X in list.\n", ignoreThreadID);
		first = 0;
	}
}

void ReceiveFrameRate(IPC::FPSInfo& fps_info)
{
    WCHAR str[128];
	swprintf(str, L"Current FPS: %.1f / %.1f", fps_info.GetFPS(), fps_info.GetLogicalFPS());
	LPCWSTR pStr = str;
	if(wcslen(str) > 24)
		pStr += 8; // omit "Current" if the string is getting too long
	SetWindowTextW(GetDlgItem(hWnd, IDC_STATIC_CURRENTFPS), pStr);

	goingsuperfast = (fps_info.GetFPS() >= 300.0f);
}

void ReceiveHWND(HWND hwnd)
{
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

static std::wstring AbsolutifyPath(const std::wstring& path)
{
    WCHAR buffer[MAX_PATH + 1];
	SearchPathW(nullptr, path.c_str(), nullptr, MAX_PATH, buffer, nullptr);
	return NormalizePath(buffer);
}

LPCWSTR ExceptionCodeToDescription(DWORD code)
{
	switch(code & (DWORD)0xCFFFFFFF)
	{
		// custom/nonstandard/undocumented exceptions
		case /*DELPHI_RUNTIME_ERROR*/(DWORD)0x0EEDFADE: return L"Delphi runtime error.";
		case /*EXCEPTION_MSVC*/(DWORD)0xC06D7363: return L"exception inserted by Visual C++, often used for SEH";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0524F54: return L"EXCEPTION_COMPLUS (ROT)";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0434352: return L"EXCEPTION_COMPLUS (CCR)";
		case /*EXCEPTION_COMPLUS*/(DWORD)0xC0434F4D: return L"EXCEPTION_COMPLUS (COM)";
		case /*EXCEPTION_HIJACK*/(DWORD)0xC0434F4E: return L"Resuming thread after garbage collection.";
		case /*EXCEPTION_SOFTSO*/(DWORD)0xC053534F: return L"Stack overflow in managed code.";
		case /*EXCEPTION_EXX*/(DWORD)0xC0455858: return L"SetupThread PInvoke failure.";
		case (DWORD)0xC0440001: return L"D runtime Win32 exception";
		default:
			LPVOID lpMessageBuffer;
			HMODULE Hand = LoadLibraryW(L"NTDLL.DLL");
   
			FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_FROM_HMODULE,
				Hand,
				code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPWSTR) &lpMessageBuffer,
				0,
				NULL );

			FreeLibrary(Hand);

            /*
             * TODO: Memleak
             * -- Warepire
             */
			return (LPWSTR)lpMessageBuffer;
	}
}

static int CountSharedPrefixLength(const std::wstring& a, const std::wstring& b)
{
	int i = 0;
	while(a[i] && tolower(a[i]) == tolower(b[i]))
		i++;
	return i;
}
static int CountBackslashes(const std::wstring& str)
{
    return std::count(str.begin(), str.end(), L'\\');
}

static int IsPathTrusted(const std::wstring& path)
{
	// we want to return 0 for all system or installed dlls,
	// 2 for our own dll, 1 for the game exe,
	// and 1 for any dlls or exes around the game's directory

    if (!wcsicmp(path.c_str(), injected_dll_path.c_str()))
    {
        return 2;
    }

    int outCount = 2;

	if(CountBackslashes(exe_filename.c_str() + CountSharedPrefixLength(exe_filename, path)) < outCount)
		return 1;
	if(CountBackslashes(sub_exe_filename.c_str() + CountSharedPrefixLength(sub_exe_filename, path)) < outCount)
		return 1;

    if(path.length() >= 4 && !wcsnicmp(path.c_str() + path.length() - 4, L".cox", 4)) // hack, we can generally assume the game outputted any dll that has this extension
		return 1;

	return 0;
}


struct MyMODULEINFO
{
	MODULEINFO mi;
	std::wstring path;
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
	return std::max<DWORD>(size, 0x10000);
}

void RegisterModuleInfo(LPVOID hModule, HANDLE hProcess, LPCWSTR path)
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
				debugprintf(L"apparently already TRUSTED MODULE 0x%08X - 0x%08X (%s)\n", mmi.mi.lpBaseOfDll, (DWORD)mmi.mi.lpBaseOfDll+mmi.mi.SizeOfImage, path);
				return;
			}
		}
		trustedModuleInfos.push_back(mmi);
		SendTrustedRangeInfos(hProcess);
	}
	debugprintf(L"TRUSTED MODULE 0x%08X - 0x%08X (%s)\n", mmi.mi.lpBaseOfDll, (DWORD)mmi.mi.lpBaseOfDll+mmi.mi.SizeOfImage, path);
}
void UnregisterModuleInfo(LPVOID hModule, HANDLE hProcess, LPCWSTR path)
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
	BYTE origByte;
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

	static constexpr BYTE newByte = 0xCC;
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
			debugprintf(L"Closing thread handle 0x%X\n", handle);
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

		debugprintf(L"Closing thread handle 0x%X\n", threadHandleToClose);
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
		dll_base_to_filename.clear();
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
		debugprintf(L"STOPPED THREAD: id=0x%X, handle=0x%X, name=%S\n", de.dwThreadId, hThread, found->second.name);
		hGameThreads.erase(found);
	}
	else
	{
		debugprintf(L"STOPPED THREAD: id=0x%X, handle=unknown\n", de.dwThreadId);
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
    HANDLE hInitialThread = INVALID_HANDLE_VALUE;

    do {
	allProcessInfos.clear();
	customBreakpoints.clear();

    
    s_lastFrameCount = 0;
	movie.currentFrame = 0;
	{
		int loadMovieResult = LoadMovie(movie_filename);
		if (loadMovieResult < 0 || (localTASflags.playback && loadMovieResult == 0))
		{
			// Replacing this goto with copy-pasted code.
			// goto earlyAbort;

			HANDLE hInitialThread = processInfo.hThread;
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
		OnMovieStart();
	}

	DeleteCriticalSection(&g_processMemCS);
	InitializeCriticalSection(&g_processMemCS);

    sub_exe_filename.clear();
    exe_filename = AbsolutifyPath(exe_filename);
    std::wstring initial_directory = exe_filename.substr(0, exe_filename.find_last_of(L"\\/"));

    std::vector<wchar_t> cmdline;
    cmdline.push_back(L'\"');
    cmdline.insert(cmdline.end(), exe_filename.begin(), exe_filename.end());
    cmdline.push_back(L'\"');
    cmdline.push_back(L' ');
    cmdline.insert(cmdline.end(), command_line.begin(), command_line.end());

	/*debugprintf("enabling full debug privileges...\n");
	if(!EnableDebugPrivilege())
	{
		debugprintf("failed to enable full debug privileges...\n");
	}*/

	debugprintf(L"creating process \"%s\"...\n", exe_filename.c_str());
	//debugprintf("initial directory = \"%s\"...\n", initialDirectory);
	BOOL created = CreateProcessW(exe_filename.c_str(), // application name
		cmdline.data(), // commandline arguments
		NULL, // process attributes (e.g. security descriptor)
		NULL, // thread attributes (e.g. security descriptor)
		FALSE, // inherit handles
		CREATE_SUSPENDED | DEBUG_PROCESS, // creation flags
		///*INHERIT_PARENT_AFFINITY*/0x00010000 | CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS, // creation flags
		//0,//CREATE_SUSPENDED ,//| DEBUG_ONLY_THIS_PROCESS, // creation flags
		NULL, // environment
		initial_directory.c_str(), // initial directory
		&startupInfo,
		&processInfo);

	if(!created)
	{
		DWORD error = GetLastError();
		PrintLastError(L"CreateProcess", error);
		if(error == ERROR_ELEVATION_REQUIRED)
		{
			WCHAR str [1024];
			swprintf(str,
				L"ERROR: Admin privileges are required to run \"%s\" on this system.\n"
				L"Hourglass doesn't have high enough privileges to launch the game.\n"
				L"Try closing Hourglass and then opening it with \"Run as administrator\".", exe_filename.c_str());
			CustomMessageBox(str, L"Permission Denied", MB_OK|MB_ICONERROR);
		}
	}

	hGameProcess = processInfo.hProcess;
	debugprintf(L"done creating process, got handle 0x%X, PID = %d.\n", hGameProcess, processInfo.dwProcessId);

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
				PrintLastError(L"SetProcessAffinityMask", GetLastError()); // We failed, write the reason to the error log.
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
				PrintLastError(L"SetProcessAffinityMask", GetLastError()); // We failed, write the reason to the error log.
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

	debugprintf(L"attempting injection...\n");

	injected_dll_path = this_process_path + L"\\hooks.dll";

	if(!onlyHookChildProcesses)
	{
		InjectDll(processInfo.hProcess, processInfo.dwProcessId, processInfo.hThread, processInfo.dwThreadId, injected_dll_path.c_str(), runDllLast!=0);

		debugprintf(L"done injection. starting game thread\n");
	}

	bool askedToRestartAboutBadThreadId = false;
	//bool hasMainThread = false;

	requestedCommandReenter = false;

	for(int i = 0; i < maxNumSavestates; i++)
		savestates[i].stale = savestates[i].valid;

	s_firstTimerDesyncWarnedFrame = 0x7FFFFFFF;
	//gotSrcDllVersion = false;

	//g_gammaRampEnabled = false;
	dllLoadInfos.numInfos = 0;
	dllLoadInfosSent = false;

	EnterCriticalSection(&g_processMemCS);
	runningNow = true;

	started = true;
	CheckDialogChanges(0);
	EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP), true);
	mainMenuNeedsRebuilding = true;
	hInitialThread = processInfo.hThread;
	ResumeThread(hInitialThread);

	//BOOL firstBreak = TRUE;
	bool postDllMainDone = false;
	bool mainThreadWaitingForPostDllMain = false;
	DWORD entrypoint = 0;
	int exceptionPrintingPaused = 0;
	//bool redalert = false;
	lastFrameAdvanceKeyUnheldTime = timeGetTime();

	debugprintf(L"entering debugger loop\n");
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

            static LPVOID ipc_address_in_process = nullptr;

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
							::sprintf(str, /*"SHORTTRACE: 0,50 "*/ "ERROR READING STRING of length %d\n", len);

						const char* pstr = str;

#define MessagePrefixMatch(pre) (!strncmp(pstr, pre": ", sizeof(pre": ")-1) ? pstr += sizeof(pre": ")-1 : false)
                        if (MessagePrefixMatch("IPCBUFFER"))
                        {
                            ipc_address_in_process = reinterpret_cast<LPVOID>(strtol(pstr, nullptr, 16));
                        }
						else if(MessagePrefixMatch("PAUSEEXCEPTIONPRINTING"))
							exceptionPrintingPaused++;
						else if(MessagePrefixMatch("RESUMEEXCEPTIONPRINTING"))
							exceptionPrintingPaused--;
						else if(MessagePrefixMatch("DEBUGPAUSE"))
						{
							debugprintf(L"DEBUGPAUSE: %S",pstr);
							paused = true;
							temporaryUnpause = false;
							CheckDialogChanges(-1);
							while(paused && !temporaryUnpause && !terminateRequest)
							{
								CheckHotkeys(-1, false);
								Sleep(5);
							}
						}
						else
						{
							debugprintf(L"UNKNOWN MESSAGE: %S\n",pstr); // unhandled message
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
				verbosedebugprintf(L"exception code: 0x%X\n", de.u.Exception.ExceptionRecord.ExceptionCode);

				if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				{
					DWORD address = (DWORD)de.u.Exception.ExceptionRecord.ExceptionAddress;
					int index = GetBreakpointIndex(address, de.dwThreadId);
					if(index != -1)
					{
						debugprintf(L"hit custom breakpoint at address = 0x%X\n", address);
						if(address == entrypoint)
						{
							RemoveBreakpoint(address, de.dwThreadId, /*GetProcessHandle(processInfo,de)*/hGameProcess);
							//debugprintf("pc was 0x%X\n", GetProgramCounter(processInfo.hThread));
							// in this case we can use processInfo.hThread because we know we're dealing with the main thread
							SetProgramCounter(processInfo.hThread, address);
							// suspend main thread until PostDllMain finishes
							if(!postDllMainDone)
							{
								debugprintf(L"suspending main thread until PostDllMain finishes...\n");
								SuspendThread(processInfo.hThread);
								mainThreadWaitingForPostDllMain = true;
							}
						}
					}
                    else if (ipc_address_in_process != nullptr)
                    {
                        IPC::CommandFrame ipc_frame;
                        SIZE_T read_bytes;
                        /*
                         * HACK: Workaround for limitation in extendedtrace.
                         * TODO: Write new wrapper for dbghelp.dll then check the break point came from SendIPCMessage
                         * -- Warepire
                         */
                        BOOL rv = ReadProcessMemory(hGameProcess, ipc_address_in_process, &ipc_frame, sizeof(ipc_frame), &read_bytes);
                        if (rv && read_bytes == sizeof(ipc_frame))
                        {
                            if (ipc_frame.m_command > IPC::Command::CMD_DUMMY_ENTRY_MIN_OPCODE &&
                                static_cast<std::underlying_type<IPC::Command>::type>(ipc_frame.m_command) % 2 == 0)
                            {
                                std::vector<BYTE> buf;
                                read_bytes = 0;
                                if (ipc_frame.m_command_data_size != 0)
                                {
                                    buf.resize(ipc_frame.m_command_data_size);
                                    rv = ReadProcessMemory(hGameProcess, ipc_frame.m_command_data, buf.data(), buf.capacity(), &read_bytes);
                                }
                                if (rv && read_bytes == ipc_frame.m_command_data_size)
                                {
                                    switch (ipc_frame.m_command)
                                    {
                                    case IPC::Command::CMD_DEBUG_MESSAGE:
                                        debugprintf(L"%s", reinterpret_cast<IPC::DebugMessage*>(buf.data())->GetMessage());
                                        {
                                            std::map<DWORD, ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
                                            if (found != hGameThreads.end())
                                            {
                                                auto cb = [](IDbgHelpStackWalkCallback& data)
                                                    { 
                                                        std::wostringstream oss(L"STACK FRAME: ");
                                                        oss.setf(std::ios_base::showbase);
                                                        oss.setf(std::ios_base::hex, std::ios_base::basefield);

                                                        /*
                                                         * Add the module and function name.
                                                         */
                                                        oss << data.GetModuleName() << L" : " << data.GetFunctionName() << L'(';

                                                        size_t arg_number = 1;
                                                        for (const auto& parameter : data.GetParameters())
                                                        {
                                                            if (arg_number > 1)
                                                            {
                                                                oss << L", ";
                                                            }

                                                            /*
                                                             * Add the parameter type and name.
                                                             */
                                                            oss << parameter.m_type.GetName() << L' ' << parameter.m_name << L" = ";

                                                            /*
                                                             * Add the parameter value.
                                                             */
                                                            if (parameter.m_value.has_value())
                                                            {
#pragma message(__FILE__ ": TODO: change to constexpr-if lambdas when they are supported (VS2017 Preview 3)")
                                                                class visitor
                                                                {
                                                                    std::wostringstream& m_oss;

                                                                public:
                                                                    visitor(std::wostringstream& oss) : m_oss(oss) {}

                                                                    /*
                                                                     * Print chars.
                                                                     */
                                                                    void operator()(char value)
                                                                    {
                                                                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                                                                    }

                                                                    void operator()(wchar_t value)
                                                                    {
                                                                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                                                                    }

                                                                    void operator()(char16_t value)
                                                                    {
                                                                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                                                                    }

                                                                    void operator()(char32_t value)
                                                                    {
                                                                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                                                                    }

                                                                    /*
                                                                     * Pointers ignore showbase.
                                                                     */
                                                                    void operator()(void* value)
                                                                    {
                                                                        m_oss << L"0x" << value;
                                                                    }

                                                                    template<typename T>
                                                                    void operator()(T value)
                                                                    {
                                                                        m_oss << value;
                                                                    }
                                                                };

                                                                std::visit(visitor(oss), parameter.m_value.value());
                                                            }

                                                            ++arg_number;
                                                        }

                                                        oss << L')';

                                                        /*
                                                         * Add the unsure status display.
                                                         */
                                                        if (data.GetUnsureStatus() > 0)
                                                        {
                                                            oss << L'?';
                                                        }

                                                        oss << L'\n';

                                                        debugprintf(L"%s", oss.str().c_str());

                                                        return IDbgHelpStackWalkCallback::Action::CONTINUE;
                                                    };
                                                DbgHelp::StackWalk(de.dwProcessId, found->second.handle, cb);
                                                std::map<DWORD, ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
                                                if (found != hGameThreads.end())
                                                {
                                                    HANDLE hThread = found->second;
                                                    WCHAR msg[16 + 72];
                                                    swprintf(msg, L"(id=0x%X) (name=%S)", found->first, found->second.name);
                                                    THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
                                                }

                                                debugprintf(L"Hello!\n");
                                            }
                                        }
                                        break;
                                    case IPC::Command::CMD_DLL_VERSION:
                                        CheckSrcDllVersion(*reinterpret_cast<DWORD*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_COMMAND_BUF:
                                        ReceiveCommandSlotPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_INPUT_BUF:
                                        ReceiveInputsPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_DLL_LOAD_INFO_BUF:
                                        ReceiveDllLoadInfosPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_TRUSTED_RANGE_INFO_BUF:
                                        ReceiveTrustedRangeInfosPointer(*reinterpret_cast<LPVOID*>(buf.data()), hGameProcess);
                                        break;
                                    case IPC::Command::CMD_TAS_FLAGS_BUF:
                                        ReceiveTASFlagsPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_SOUND_INFO:
                                        ReceiveSoundCaptureInfoPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_GENERAL_INFO:
                                        ReceiveGeneralInfoPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_PALETTE_ENTRIES:
                                        ReceivePaletteEntriesPointer(*reinterpret_cast<LPVOID*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_GIMME_DLL_LOAD_INFOS:
                                        AddAndSendDllInfo(NULL, true, hGameProcess);
                                        break;
                                    case IPC::Command::CMD_KEYBOARD_LAYOUT_NAME:
                                        ReceiveKeyboardLayout(*reinterpret_cast<LPVOID*>(buf.data()), hGameProcess);
                                        break;
                                    case IPC::Command::CMD_SUGGEST_THREAD_NAME:
                                        SuggestThreadName(de.dwThreadId, hGameProcess, *reinterpret_cast<IPC::SuggestThreadName*>(buf.data()));
                                        {
                                            DWORD bytes_written;
                                            WriteProcessMemory(hGameProcess, const_cast<LPVOID>(ipc_frame.m_command_data), buf.data(), ipc_frame.m_command_data_size, &bytes_written);
                                            // print the callstack of the thread
                                            std::map<DWORD, ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
                                            if (found != hGameThreads.end())
                                            {
                                                HANDLE hThread = found->second;
                                                //THREADSTACKTRACE(hThread);
                                                WCHAR msg[16 + 72];
                                                swprintf(msg, L"(id=0x%X) (name=%S)", found->first, found->second.name);
                                                THREADSTACKTRACEMSG(hThread, msg, /*(found->second).hProcess*/hGameProcess);
                                            }
                                        }
                                        break;
                                    case IPC::Command::CMD_POST_DLL_MAIN_DONE:
                                        postDllMainDone = true;
                                        if (mainThreadWaitingForPostDllMain)
                                        {
                                            debugprintf(L"resuming main thread...\n");
                                            ResumeThread(processInfo.hThread);
                                            mainThreadWaitingForPostDllMain = false;
                                        }
                                        break;
                                    case IPC::Command::CMD_KILL_ME:
                                        terminateRequest = true;
                                        break;
                                    case IPC::Command::CMD_FPS_UPDATE:
                                        ReceiveFrameRate(*reinterpret_cast<IPC::FPSInfo*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_DENIED_THREAD:
                                        usedThreadMode = localTASflags.threadMode;
                                        break;
                                    case IPC::Command::CMD_GAMMA_RAMP_BUF:
                                        ReceiveGammaRampData(*reinterpret_cast<LPVOID*>(buf.data()), hGameProcess);
                                        break;
                                    case IPC::Command::CMD_STACK_TRACE:
                                        {
                                            // print the callstack of the thread
                                            std::map<DWORD, ThreadInfo>::iterator found = hGameThreads.find(de.dwThreadId);
                                            IPC::StackTrace* stack_trace_info = reinterpret_cast<IPC::StackTrace*>(buf.data());
                                            if (found != hGameThreads.end())
                                            {
                                                HANDLE hThread = found->second;
                                                WCHAR msg[16 + 72];
                                                swprintf(msg, L"(id=0x%X) (name=%S)", found->first, found->second.name);
                                                StackTraceOfDepth(hThread, msg, stack_trace_info->GetMinDepth(), stack_trace_info->GetMaxDepth(), /*(found->second).hProcess*/hGameProcess);
                                            }
                                        }
                                        break;
                                    case IPC::Command::CMD_SAVE_STATE:
                                        SaveGameStatePhase2(*reinterpret_cast<int*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_LOAD_STATE:
                                        LoadGameStatePhase2(*reinterpret_cast<int*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_SUSPEND_ALL_THREADS:
                                        SuspendAllExcept(de.dwThreadId);
                                        break;
                                    case IPC::Command::CMD_RESUME_ALL_THREADS:
                                        ResumeAllExcept(de.dwThreadId);
                                        break;
                                    case IPC::Command::CMD_HWND:
                                        ReceiveHWND(*reinterpret_cast<HWND*>(buf.data()));
                                        break;
                                    case IPC::Command::CMD_WATCH_ADDRESS:
                                        {
                                            IPC::AutoWatch* auto_watch = reinterpret_cast<IPC::AutoWatch*>(buf.data());
                                            AddressWatcher watch = { auto_watch->GetAddress(),
                                                                     auto_watch->GetSize(),
                                                                     auto_watch->GetType() };
                                            watch.WrongEndian = false;
                                            if (IsHardwareAddressValid(watch.Address))
                                            {
                                                InsertWatch(watch, auto_watch->GetComment());
                                            }
                                        }
                                        break;
                                    case IPC::Command::CMD_UNWATCH_ADDRESS:
                                        {
                                            IPC::AutoWatch* auto_watch = reinterpret_cast<IPC::AutoWatch*>(buf.data());
                                            AddressWatcher watch = { auto_watch->GetAddress(),
                                                                     auto_watch->GetSize(),
                                                                     auto_watch->GetType() };
                                            watch.WrongEndian = false;
                                            RemoveWatch(watch);
                                        }
                                        break;
                                    case IPC::Command::CMD_FRAME_BOUNDARY:
                                        {
                                            FrameBoundary(*reinterpret_cast<IPC::FrameBoundaryInfo*>(buf.data()), de.dwThreadId);
#ifdef _DEBUG
                                            if (s_lastFrameCount <= 1 && !requestedCommandReenter)
                                            {
                                                debugprintf(L"On Frame %d: \n", s_lastFrameCount);
                                                // print the callstack of all threads

                                                int numThreads = gameThreadIdList.size();
                                                for (int i = 0; i < numThreads; i++)
                                                {
                                                    DWORD threadId = gameThreadIdList[i];
                                                    ThreadInfo& info = hGameThreads[threadId];
                                                    HANDLE hThread = info.handle;
                                                    WCHAR msg[16 + 72];
                                                    swprintf(msg, L"(id=0x%X) (name=%S)", threadId, info.name);
                                                    THREADSTACKTRACEMSG(hThread, msg, /*info.hProcess*/hGameProcess);
                                                }
                                            }
#endif
                                        }
                                        break;
                                    case IPC::Command::CMD_MOUSE_REG:
                                        inputC.InitDIMouse(hWnd, true);
                                        break;
                                    default:
                                        break;
                                    }

                                    /*
                                     * Hacky...
                                     * -- Warepire
                                     */
                                    ipc_frame.m_command = static_cast<IPC::Command>(static_cast<std::underlying_type<IPC::Command>::type>(ipc_frame.m_command) + 1);
                                    DWORD bytes_written;
                                    WriteProcessMemory(hGameProcess, ipc_address_in_process, &ipc_frame, sizeof(ipc_frame), &bytes_written);
                                    break;
                                }
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
							debugprintf(L"exception 0x%X (at 0x%08X): attempt to %s memory at 0x%08X\n",
								de.u.Exception.ExceptionRecord.ExceptionCode,
								de.u.Exception.ExceptionRecord.ExceptionAddress,
								de.u.Exception.ExceptionRecord.ExceptionInformation[0] == 0 ? L"read" : (de.u.Exception.ExceptionRecord.ExceptionInformation[0] == 1 ? L"write" : L"hack"),
								de.u.Exception.ExceptionRecord.ExceptionInformation[1]);
						}
						else if(couldBeSerious)
						{
							debugprintf(L"exception 0x%X (at 0x%08X): %s\n",
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
								WCHAR msg [16+72];
								swprintf(msg, L"(id=0x%X) (name=%S)", found->first, found->second.name);
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
								WCHAR msg [16+72];
								swprintf(msg, L"(id=0x%X) (name=%S)", threadId, info.name);
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
									debugprintf(L"got name for thread id=0x%X: \"%S\" (was \"%S\")\n", threadId, threadInfo.name, oldName);
								else
									debugprintf(L"got name for thread id=0x%X: \"%S\"\n", threadId, threadInfo.name);
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
								debugprintf(L"got name for as-yet-invalid thread id=0x%X: \"%S\"\n", threadId, threadInfo.name);
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
							static LPCWSTR mainmsg = L"The game crashed...\n";
							static LPCWSTR waitskipmsg = L"If this happens when fast-forwarding, try disabling \"Wait Skip\" in \"Misc > Fast-Forward Options\".\n";
							static LPCWSTR multidisablemsg = L"If this happens sometimes randomly, try switching Multithreading to \"Disable\" in \"Misc > Multithreading Mode\".\n";
							static LPCWSTR dsounddisablemsg = L"Or choose \"Sound > Disable DirectSound Creation\" if the game doesn't work without Multithreading on.\n";
							static LPCWSTR dsounddisablemsg2 = L"If this happens sometimes randomly, try choosing \"Sound > Disable DirectSound Creation\".\n";
							WCHAR msg [8096];
							swprintf(msg, L"%s%s%s%s%s", mainmsg,
								(localTASflags.fastForward && (localTASflags.fastForwardFlags & FFMODE_WAITSKIP)) ? waitskipmsg : L"",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && !(localTASflags.aviMode & 2)) ? multidisablemsg : L"",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && !(localTASflags.aviMode & 2) && !(localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)) ? dsounddisablemsg : L"",
								(s_lastFrameCount > 30 && localTASflags.threadMode != 0 && (localTASflags.aviMode & 2) && !(localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)) ? dsounddisablemsg2 : L""
							);
							debugprintf(L"%s", msg);
#ifndef _DEBUG
							if(!skipDialog)
								CustomMessageBox(msg, L"CRASH", MB_OK|MB_ICONERROR);
#endif
							recoveringStale = false;
						}
						requestedCommandReenter = false; // maybe fixes something?
						if(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
						{
							if(unsavedMovieData)
							{
								SaveMovie(movie_filename);
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
					if(dll_base_to_filename[de.u.LoadDll.lpBaseOfDll].length() == 0)
					{
						std::wstring filename = GetFileNameFromFileHandle(de.u.LoadDll.hFile);
						debugprintf(L"LOADED DLL: %s\n", filename.c_str());
						HANDLE hProcess = GetProcessHandle(processInfo,de);
						RegisterModuleInfo(de.u.LoadDll.lpBaseOfDll, hProcess, filename.c_str());
						AddAndSendDllInfo(filename.c_str(), true, hProcess);

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
						dll_base_to_filename[de.u.LoadDll.lpBaseOfDll] = filename;
                        DbgHelp::LoadSymbols(de.dwProcessId, de.u.LoadDll.hFile, filename.c_str(), reinterpret_cast<DWORD64>(de.u.LoadDll.lpBaseOfDll));
                        LOADSYMBOLS2(hGameProcess, filename.c_str(), de.u.LoadDll.hFile, de.u.LoadDll.lpBaseOfDll);

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
					LPCWSTR filename = dll_base_to_filename[de.u.UnloadDll.lpBaseOfDll].c_str();
					debugprintf(L"UNLOADED DLL: %s\n", filename);
					HANDLE hProcess = GetProcessHandle(processInfo,de);
					UnregisterModuleInfo(de.u.UnloadDll.lpBaseOfDll, hProcess, filename);
					AddAndSendDllInfo(filename, false, hProcess);
					//dllBaseToHandle[de.u.LoadDll.lpBaseOfDll] = NULL;
					dll_base_to_filename[de.u.UnloadDll.lpBaseOfDll] = L"";
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
					debugprintf(L"STARTED THREAD: id=0x%X, handle=0x%X\n", de.dwThreadId, de.u.CreateThread.hThread);

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
				debugprintf(L"thread status: id=0x%X, handle=0x%X, suspend=%d, name=%S\n", threadId, hThread, suspendCount, threadInfo.name);
				//THREADSTACKTRACE(hThread);
				WCHAR msg [16+72];
				swprintf(msg, L"(id=0x%X) (name=%S)", threadId, threadInfo.name);
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
                    std::wstring filename = GetFileNameFromProcessHandle(de.u.CreateProcessInfo.hProcess);

					// hFile is NULL sometimes...
					if(filename == L"")
						filename = GetFileNameFromFileHandle(de.u.CreateProcessInfo.hFile);

//					debugprintf("CREATE_PROCESS_DEBUG_EVENT: 0x%X\n", de.u.CreateProcessInfo.lpBaseOfImage);
					debugprintf(L"CREATED PROCESS: %s\n", filename.c_str());
                    sub_exe_filename = filename;
                    DbgHelp::AddProcess(de.u.CreateProcessInfo.hProcess, de.dwProcessId);
					RegisterModuleInfo(de.u.CreateProcessInfo.lpBaseOfImage, de.u.CreateProcessInfo.hProcess, filename.c_str());
					hGameThreads[de.dwThreadId] = de.u.CreateProcessInfo.hThread;
					ASSERT(hGameThreads[de.dwThreadId].handle);
					hGameThreads[de.dwThreadId].hProcess = de.u.CreateProcessInfo.hProcess;
					gameThreadIdList.push_back(de.dwThreadId);

					entrypoint = (int)de.u.CreateProcessInfo.lpStartAddress;
					if(entrypoint/* && movie.version >= 70*/)
					{
						AddBreakpoint(entrypoint, de.dwThreadId, de.u.CreateProcessInfo.hProcess);
						debugprintf(L"entrypoint = 0x%X\n", entrypoint);
					}

//					if(threadMode == 3)
//					{
//						debugprintf("MAIN THREAD: id=0x%X, handle=0x%X\n", de.dwThreadId, de.u.CreateProcessInfo.hThread);
//						if(!s_lastThreadSwitchedTo)
//							s_lastThreadSwitchedTo = de.dwThreadId;
////						SuspendThread(de.u.CreateProcessInfo.hThread);
//					}

					bool nullFile = !de.u.CreateProcessInfo.hFile;
					if(nullFile)
						de.u.CreateProcessInfo.hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    DbgHelp::LoadSymbols(de.dwProcessId, de.u.CreateProcessInfo.hFile, filename.c_str(), reinterpret_cast<DWORD64>(de.u.CreateProcessInfo.lpBaseOfImage));

					if(de.dwProcessId == processInfo.dwProcessId)
					{
						// even if we don't have symbols for it,
						// this still lets us see the exe name
						// next to the addresses of functions it calls.
                        hGameProcess = de.u.CreateProcessInfo.hProcess;
						LOADSYMBOLS2(hGameProcess, filename.c_str(), de.u.CreateProcessInfo.hFile, de.u.CreateProcessInfo.lpBaseOfImage);
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
						debugprintf(L"switched to child process, handle 0x%X, PID = %d.\n", hGameProcess, processInfo.dwProcessId);
						PrintPrivileges(hGameProcess);
						EXTENDEDTRACEINITIALIZEEX( NULL, hGameProcess );
						LOADSYMBOLS2(hGameProcess, filename.c_str(), de.u.CreateProcessInfo.hFile, de.u.CreateProcessInfo.lpBaseOfImage);
						debugprintf(L"attempting injection...\n");
						InjectDll(processInfo.hProcess, processInfo.dwProcessId, processInfo.hThread, processInfo.dwThreadId, injected_dll_path.c_str(), runDllLast!=0);
						debugprintf(L"done injection. continuing...\n");
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
						SaveMovie(movie_filename);
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
				debugprintf(L"RIP: err=0x%X, type=0x%X\n", de.u.RipInfo.dwError, de.u.RipInfo.dwType);
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

    } while (false);

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
	ClearAndDeallocateContainer(dll_base_to_filename);
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
		while(count < 4 && PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			DispatchMessageW(&msg);
			count++;
		}
	}
	if(debuggerThread)
	{
		int result = CustomMessageBox(L"The game-controlling thread is taking longer than expected to exit.\nClick Cancel to terminate it immediately (this might cause problems),\nor click Retry to check again and wait a few more seconds if needed.", L"Stopping Game", MB_RETRYCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING);
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
		while(count < 4 && PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			DispatchMessageW(&msg);
			count++;
		}
	}
	if(thread)
	{
		int result = CustomMessageBox(L"A thread is taking longer than expected to exit.\nClick Cancel to terminate it immediately (this might cause problems),\nor click Retry to check again and wait a few more seconds if needed.", L"Stopping", MB_RETRYCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING);
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
		SaveMovie(movie_filename);
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

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR    lpCmdLine,
                      int       nCmdShow)
{
    {
        WCHAR path[MAX_PATH + 1];
        GetCurrentDirectoryW(MAX_PATH, path);
        this_process_path = path;
    }

	// this is so we don't start the target with the debug heap,
	// which would cause all sorts of complaining exceptions we'd have to ignore
	// and would potentially change program behavior.
	SetEnvironmentVariableW(L"_NO_DEBUG_HEAP", L"1");
	SetEnvironmentVariableW(L"__MSVCRT_HEAP_SELECT", L"__GLOBAL_HEAP_SELECTED,3");

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

	this_process_path = NormalizePath(this_process_path);

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

    DbgHelp::Init();

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

		if(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//if(!IsDialogMessage(hWnd, &msg))
			//&& !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
			{
				if(PreTranslateMessage(msg))
					TranslateMessage(&msg);
				DispatchMessageW(&msg);
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

	hWnd = CreateDialogParamW(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc, 0);

	if(!hWnd)
		return FALSE;

	SendMessageW(hWnd, WM_SETICON, WPARAM(ICON_SMALL), LPARAM(LoadImageW(hInst, MAKEINTRESOURCEW(IDI_MYICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
	SendMessageW(hWnd, WM_SETICON, WPARAM(ICON_BIG),   LPARAM(LoadImageW(hInst, MAKEINTRESOURCEW(IDI_MYICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR)));

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
	FILE* moviefile = !movie_filename.empty() ? _wfopen(movie_filename.c_str(), L"r+b") : NULL;
	if(moviefile)
		fclose(moviefile);
	else
	{
		moviefile = !movie_filename.empty() ? _wfopen(movie_filename.c_str(), L"rb") : NULL;
		if(moviefile)
			fclose(moviefile);
		else
			exists = false;

		moviefile = !movie_filename.empty() ? _wfopen(movie_filename.c_str(), L"ab") : NULL;
		if(moviefile)
		{
			fclose(moviefile);
			if(!exists)
				DeleteFileW(movie_filename.c_str());
		}
		else
			writable = false;
	}

	FILE* exefile = !exe_filename.empty() ? _wfopen(exe_filename.c_str(), L"rb") : NULL;
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
		LoadMovie(movie_filename);
		
		UpdateFrameCountDisplay(movie.currentFrame, 1);

		WCHAR str [256];
		swprintf(str, L"%u", localTASflags.initialTime);
		SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str);
		swprintf(str, L"%d", localTASflags.framerate);
		SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_FPS), str);

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
		DWORD style = (DWORD)GetWindowLongW(gamehwnd, GWL_STYLE);
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

BOOL DirectoryExists(LPCWSTR path)
{
	DWORD attr = GetFileAttributesW(path);
	return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}
void SplitToValidPath(LPCWSTR initialPath, LPCWSTR defaultDir, LPWSTR filename, LPWSTR directory)
{
	filename[0] = '\0';
	directory[0] = '\0';

	if(initialPath && wcslen(initialPath) > MAX_PATH)
		return;

	if(initialPath)
	{
		wcscpy(filename, initialPath);
		wcscpy(directory, initialPath);
	}

	while(!DirectoryExists(directory))
	{
		LPWSTR slash = std::max(wcsrchr(directory, L'\\'), wcsrchr(directory, L'/'));
		if(slash)
			*slash = '\0';
		else
		{
			if(defaultDir && wcslen(defaultDir) <= MAX_PATH)
				wcscpy(directory, defaultDir);
			if(!DirectoryExists(directory))
				*directory = '\0';
			break;
		}
	}

	if(!wcsncmp(filename, directory, wcslen(directory)))
        memmove(filename, filename + wcslen(directory), (1 + wcslen(filename) - wcslen(directory)) * 2);
	LPWSTR slash = std::max(wcsrchr(filename, '\\'), wcsrchr(filename, '/'));
	if(slash++)
        memmove(filename, slash, (1 + wcslen(slash)) * 2);
}

BOOL SetWindowTextAndScrollRight(HWND hEdit, LPCWSTR lpString)
{
	BOOL rv = SetWindowTextW(hEdit, lpString);
	SendMessageW(hEdit, EM_SETSEL, -2, -1);
	return rv;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL rv = TRUE;  
    switch(message)  
    {  
        case WM_INITDIALOG:
        {
            WCHAR title[256];
			swprintf(title, L"Hourglass-Resurrection v%d.%d", VERSION, MINORVERSION);
#ifdef _DEBUG
			wcscat(title, L" (debug build)");
#endif
			SetWindowTextW(hDlg, title);
		}
        break;

		case WM_SHOWWINDOW:
 			{
				WCHAR str [256];
				swprintf(str, L"%d", localTASflags.framerate);
				SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_FPS), str);

				localTASflags.initialTime = /*timeGetTime()*/6000;
				swprintf(str, L"%d", localTASflags.initialTime);
				SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_SYSTEMCLOCK), str);

				SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_CURFRAME), L"0");
				SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_MAXFRAME), L"0");
				SetWindowTextW(GetDlgItem(hDlg, IDC_STATIC_MOVIETIME), L"");

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

				std::wstring path = exe_filename;
				path = AbsolutifyPath(path);
				//char temp_moviefilename [MAX_PATH+1];
				//strcpy(temp_moviefilename, moviefilename);
				movienameCustomized = false;
				// As they start blank, we have no interest in updating these if the filenames
				// are empty. This prevents messages about file '' not existing.
				if (!path.empty())
					SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), path.c_str());
				if (!movie_filename.empty())
					SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), movie_filename.c_str());
				SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_COMMANDLINE), command_line.c_str());
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
			SendMessageW(hDlg, WM_COMMAND, ID_FILES_STOP, 0);
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
						if(dialogPosY + static_cast<int>(dialogSizeY) > desktopRect.bottom) dialogPosY = desktopRect.bottom - dialogSizeY;
						if(dialogPosX + static_cast<int>(dialogSizeX) > desktopRect.right) dialogPosX = desktopRect.right - dialogSizeX;

						SetWindowPos(HotkeyHWnd, HWND_TOP, dialogPosX, dialogPosY, dialogSizeX, dialogSizeY, SWP_NOZORDER | SWP_SHOWWINDOW);
					}
					else
						SetForegroundWindow(HotkeyHWnd);
					//if(rect.right > rect.left && rect.bottom > rect.top)
					//	SetWindowPos(HotkeyHWnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOSIZE|SWP_NOZORDER);
					// HotkeyHWndIsHotkeys = false;
				}	break;


                case ID_INCLUDE_LCF_NONE:
                    localTASflags.log_categories.fill(false);
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::ANY)] = true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_ALL:
                    localTASflags.log_categories.fill(true);
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_ANY:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::ANY)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_HOOK:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::HOOK)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_TIME:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::TIME)] ^= true; 
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_DETTIMER:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DETTIMER)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_SYNC:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::SYNC)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_DDRAW:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DDRAW)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_D3D:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::D3D)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_OGL:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::OGL)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_GDI:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::GDI)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_SDL:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::SDL)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_DINPUT:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DINPUT)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_WINPUT:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WINPUT)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_XINPUT:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::XINPUT)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_DSOUND:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DSOUND)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_WSOUND:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WSOUND)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_PROCESS:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::PROCESS)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_MODULE:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::MODULE)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_MESSAGES:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::MESSAGES)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_WINDOW:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WINDOW)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_FILEIO:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::FILEIO)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_REGISTRY:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::REGISTRY)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_THREAD:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::THREAD)] ^= true;
                    tasFlagsDirty = true;
                    break;
                case ID_INCLUDE_LCF_TIMERS:
                    localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::TIMERS)] ^= true;
                    tasFlagsDirty = true;
                    break;

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
					Save_As_Config();
					break;
				case ID_FILES_LOADCONFIGFROM:
					Load_As_Config();
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
						CustomMessageBox(L"The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", L"File creation error!", MB_OK | MB_ICONERROR);
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
						CustomMessageBox(L"The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", L"File creation error!", MB_OK | MB_ICONERROR);
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
						CustomMessageBox(L"The file cannot be created here.\nProbable causes are that the location is write protected\nor the file name exceeds the maximum allowed characters by Windows (260 characters, with extension)\n", L"File creation error!", MB_OK | MB_ICONERROR);
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
							WCHAR str [256];
							if(started && !warnedAboutFrameRateChange)
							{
								GetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str, 255);
								if(localTASflags.framerate != wcstol(str,0,0))
								{
									warnedAboutFrameRateChange = true;
									int result = CustomMessageBox(L"If you are in the middle of recording or playing a movie,\nchanging the game's framerate might cause desync in some games.\nBe careful.", L"Warning", MB_OKCANCEL | MB_ICONWARNING);
									if(result == IDCANCEL)
									{
										swprintf(str, L"%d", localTASflags.framerate);
										SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_FPS), str);
										warnedAboutFrameRateChange = false; // must be true above this to avoid infinite warnings
										break;
									}
								}
							}

							GetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str, 255);
							localTASflags.framerate = wcstol(str,0,0);
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
								WCHAR str [256];
								GetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str, 255);
								localTASflags.initialTime = wcstoul(str,0,0);
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
							SaveMovie(movie_filename);
						}
						Save_Config();
						//CheckDlgButton(hDlg, IDC_AVIVIDEO, aviMode & 1);
						//CheckDlgButton(hDlg, IDC_AVIAUDIO, aviMode & 2);
						bool wasPlayback = (localTASflags.playback != 0);
						TerminateDebuggerThread(12000);
						if(unsavedMovieData)
						{
							SaveMovie(movie_filename);
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
						SaveMovie(movie_filename); // Save the new movie.
						localTASflags.playback = false;
						nextLoadRecords = true;
						localTASflags.fastForward = false;
						started = true; CheckDialogChanges(-1); started = false;
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), false);
                        {
                            WCHAR commandline[1024];
                            GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
                            command_line = commandline;
                        }
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
                        {
                            WCHAR commandline[1024];
                            GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
                            command_line = commandline;
                        }
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
                        std::wstring exe_file_name = Utils::File::GetFileNameOpen(this_process_path, {
                            Utils::File::FileFilter::Executable,
                            Utils::File::FileFilter::AllFiles
                        });

                        if (!exe_file_name.empty())
                        {
                            HWND dialog_item = GetDlgItem(hDlg, IDC_TEXT_EXE);
                            SetWindowTextAndScrollRight(dialog_item, exe_file_name.c_str());
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

						bool isSave = (command == ID_FILES_RECORDMOV/* || command == ID_FILES_RESUMEMOVAS*/);

						bool wasPaused = paused;
						if(!paused)
						{
							paused = true;
							tasFlagsDirty = true;
						}

                        std::wstring movie_file_name;

                        if (isSave)
                        {
                            movie_file_name = Utils::File::GetFileNameOpen(this_process_path, {
                                Utils::File::FileFilter::HourglassMovie,
                                Utils::File::FileFilter::AllFiles
                            });
                        }
                        else
                        {
                            movie_file_name = Utils::File::GetFileNameSave(this_process_path, {
                                Utils::File::FileFilter::HourglassMovie,
                                Utils::File::FileFilter::AllFiles
                            });
                        }

                        if (!movie_file_name.empty())
                        {
                            /*if(started && (!localTASflags.playback || nextLoadRecords))
                            {
                                SaveMovie();
                            }*/

                            HWND dialog_item = GetDlgItem(hDlg, IDC_TEXT_MOVIE);
                            SetWindowTextAndScrollRight(dialog_item, movie_file_name.c_str());

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

                        // If we're not paused, pause
                        if (!paused)
                        {
                            paused = true;
                            tasFlagsDirty = true;
                        }

                        // Always start with the name of the movie currently recording.
                        std::wstring movie_file_name = Utils::File::GetFileNameSave(movie_filename, {
                            Utils::File::FileFilter::HourglassMovie,
                            Utils::File::FileFilter::AllFiles
                        });

                        if (!movie_file_name.empty())
                        {
                            // As SaveMovie actually modifies the contents of the passed string, we
                            // can't just pass the const .c_str() result
                            WCHAR c_file_name[FILENAME_MAX];
                            movie_file_name.copy(c_file_name, movie_file_name.length());
                            SaveMovie(c_file_name);
                        }

                        // If we had to pause earlier, unpause now.
                        if (paused != wasPaused)
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
                        WCHAR commandline[1024];
                        GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline, ARRAYSIZE(commandline));
                        command_line = commandline;
					}
					break;
				case IDC_TEXT_EXE:
					if(HIWORD(wParam) == EN_CHANGE || !movieFileExists || movie_filename.empty())
					{
						static bool recursing = false;
						if(recursing)
							break;
						recursing = true;

                        {
                            WCHAR buffer[MAX_PATH + 1];
                            GetWindowTextW(GetDlgItem(hDlg, IDC_TEXT_EXE), buffer, MAX_PATH);
                            exe_filename = NormalizePath(buffer);
                        }
						EnableDisablePlayRecordButtons(hDlg); // updates exeFileExists

						if(!exeFileExists) // Extra check, do we need it?
						{
							WCHAR str[1024];
							swprintf(str, L"The exe file \'%s\' does not exist, please check that it has not been renamed or moved.", exe_filename.c_str());
							CustomMessageBox(str, L"Error!", (MB_OK | MB_ICONERROR));
							recursing = false;
							break;
						}

						debugprintf(L"Attempting to determinate default thread stack size for the game...\n");
						localTASflags.threadStackSize = GetWin32ExeDefaultStackSize(exe_filename.c_str());
						if(localTASflags.threadStackSize != 0)
						{
							debugprintf(L"Detecting the default stack size succeeded, size is: %u bytes\n", localTASflags.threadStackSize);
						}
						else
						{
							WCHAR str[1024];
							swprintf(str, L"Determinating the default thread stack size failed!\nVerify that '%s' a valid Win32 executable.", exe_filename.c_str());
							CustomMessageBox(str, L"Error!", (MB_OK | MB_ICONERROR));
							recursing = false;
							break;
						}


						//FILE* file = *exefilename ? fopen(exefilename, "rb") : NULL;
						//if(file)
						//{
						//	fclose(file);
							if((!movienameCustomized && HIWORD(wParam) == EN_CHANGE) || !movieFileExists || movie_filename.empty())
							{
								// a little hack to help people out when first selecting an exe for recording,
								// until perhaps there's a proper ini file for such defaults
								int newFramerate;
								BOOL newLagskip;
								int newRunDllLast = 0;
								std::wstring fname = GetExeFilenameWithoutPath();
								if(!wcsicmp(fname.c_str(), L"Doukutsu.exe")
								|| !wcsicmp(fname.c_str(), L"dxIka.exe")
								|| !wcsicmp(fname.c_str(), L"nothing.exe")
								|| !wcsnicmp(fname.c_str(), L"iwbtgbeta", wcslen(L"iwbtgbeta")-1)
								|| !wcsicmp(fname.c_str(), L"i_wanna_be_the_GB.exe")
								)
								{
									newFramerate = 50; newLagskip = false;
								}
								else if(!wcsicmp(fname.c_str(), L"Lyle in Cube Sector.exe")
								|| !wcsicmp(fname.c_str(), L"Eternal Daughter.exe")
								|| !wcsicmp(fname.c_str(), L"Geoffrey The Fly.exe")
								)
								{
									newFramerate = 50; newLagskip = true;
								}
								else if(!wcsicmp(fname.c_str(), L"herocore.exe"))
								{
									newFramerate = 40; newLagskip = false;
								}
								else if(!wcsicmp(fname.c_str(), L"iji.exe"))
								{
									newFramerate = 30; newLagskip = false;
								}
								else if(!wcsicmp(fname.c_str(), L"lamulana.exe"))
								{
									newFramerate = 60; newLagskip = true;
								}
								else if(!wcsicmp(fname.c_str(), L"NinjaSenki.exe"))
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
								else if(!wcsicmp(fname.c_str(), L"RotateGear.exe"))
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
									WCHAR str [32];
									swprintf(str, L"%d", newFramerate);
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
							if(!movieFileExists || movie_filename.empty() || !movienameCustomized) // set default movie name based on exe
							{
								bool setMovieFile = true;
								if(movieFileExists)
								{
									std::wstring exefname = GetExeFilenameWithoutPath();
									if(!wcsncmp(exefname.c_str(), movie_filename.c_str(), exefname.length()-4)) // Compare names, without the file extension.
										setMovieFile = false; // exe didn't actually change
								}

								if(setMovieFile)
								{
									WCHAR filename [MAX_PATH+1];
									wcscpy(filename, exe_filename.c_str());
									LPWSTR slash = std::max(wcsrchr(filename, '\\'), wcsrchr(filename, '/'));
									LPWSTR dot = wcsrchr(filename, '.');
									if(slash<dot)
									{
										WCHAR path [MAX_PATH+1];
										wcscpy(path, this_process_path.c_str());
										wcscat(path, L"\\");
										wcscpy(dot, L".hgr");
										LPCWSTR moviename = slash ? slash+1 : filename;
										wcscat(path, moviename);
										if(0 != wcscmp(path, movie_filename.c_str()))
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
                        std::wstring tmp_movie;
                        {
                            WCHAR buffer[MAX_PATH + 1];
                            GetWindowTextW(GetDlgItem(hDlg, IDC_TEXT_MOVIE), buffer, MAX_PATH);
                            tmp_movie = buffer;
                        }
						tmp_movie = NormalizePath(tmp_movie);
						
						// HACK... TODO: Fix this properly, by completing TODO above!
						if(tmp_movie.empty()) break; // No movie loaded, don't go forward with anything.

						EnableDisablePlayRecordButtons(hDlg);
						movienameCustomized = tmp_movie[0] != '\0';

						if(unsavedMovieData) // Check if we have some unsaved changes in an already loaded movie.
						{
							int result = CustomMessageBox(L"The currently opened movie contains unsaved data.\nDo you want to save it before opening the new movie?\n(Click \"Yes\" to save, \"No\" to proceed without saving)", L"Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1));
							if(result == IDYES)
							{
								// TODO:
								// IF SaveMovie fails here, it means that the movie file got deleted between the last save and the creation of the new movie
								// Should we really handle that scenario?
								SaveMovie(movie_filename);
							}
							unsavedMovieData = false;
							movie.headerBuilt = false; // Otherwise we may retain the old header? // TODO: Find this out
						}

						if(!movie_filename.empty()) // We have currently locked the directory that we're saving the old movie to.
						{
							UnlockDirectory(MOVIE);
						}

						std::wstring movie_directory;
						size_t slash = tmp_movie.find_last_of(L"\\/");
                        movie_directory = tmp_movie.substr(0, slash);

						// Attempt to lock the new directory.
						if(LockDirectory(movie_directory.c_str(), MOVIE) == false)
						{
							movie_filename.clear(); // Let's not keep the old movie file name, or there could be some non-desired overwriting taking place.
							break; // Break early so that we don't allow the movie to be loaded.
						}

						// If we got here we can finally transfer the new movie filename into the old movie filename.
                        movie_filename = tmp_movie;
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
					WCHAR filename [MAX_PATH+1];
					if(DragQueryFile(hDrop, i, filename, MAX_PATH))
					{
						//debugprintf("Got File: %s\n", filename);

						LPWSTR slash = std::max(wcsrchr(filename, '\\'), wcsrchr(filename, '/'));
						LPWSTR dot = wcsrchr(filename, '.');
						if(slash<dot)
						{
							if(!wcsicmp(dot, L".exe")) // executable (game)
							{
								SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_EXE), filename);
							}
							else if(!wcsnicmp(dot, L".hgr", 4)) // windows TAS file (input movie)
							{
								SetWindowTextAndScrollRight(GetDlgItem(hDlg, IDC_TEXT_MOVIE), filename);
							}
							else if(!wcsicmp(dot, L".cfg"))
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
