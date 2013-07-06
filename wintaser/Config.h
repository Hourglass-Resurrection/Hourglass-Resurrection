#include "../shared/logcat.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SetPrivateProfileIntA(lpAppName, lpKeyName, nValue, lpFileName) \
	sprintf(Str_Tmp, "%d", nValue); \
	WritePrivateProfileStringA(lpAppName, lpKeyName, Str_Tmp, lpFileName);

#ifndef FOCUS_FLAGS_DEFINED
#define FOCUS_FLAGS_DEFINED
enum
{
	FOCUS_FLAG_TASER=0x01,
	FOCUS_FLAG_TASEE=0x02,
	FOCUS_FLAG_OTHER=0x04,
};
#endif

namespace Config{ // A namespace is maybe not the best solution...

	// TODO: Comment *everything* !
	extern int audioFrequency;
	extern int audioBitsPerSecond;
	extern int audioChannels;
	extern bool paused;
	extern bool fastforward;
	extern bool started;
	extern bool playback;
	extern bool finished;
	extern bool nextLoadRecords; // false if next load switches to playback, true if next load switches to recording... aka readonly/read-only toggle
	extern bool recoveringStale;
	extern bool exeFileExists;
	extern bool movieFileExists;
	extern bool movieFileWritable;
	extern int forceWindowed;
	extern int truePause;
	extern int onlyHookChildProcesses;
	extern int forceSurfaceMemory;
	extern int forceSoftware;
	extern int aviMode;
	extern int emuMode;
	extern int fastForwardFlags;
	extern int timescale, timescaleDivisor;
	extern int allowLoadInstalledDlls, allowLoadUxtheme;
	extern int runDllLast;
	extern int advancePastNonVideoFrames;
	extern bool advancePastNonVideoFramesConfigured;
	extern int threadMode;
	extern int usedThreadMode;
	extern int timersMode;
	extern int messageSyncMode;
	extern int waitSyncMode;
	extern int aviFrameCount;
	extern int aviSoundFrameCount;
	extern bool traceEnabled;
	extern bool crcVerifyEnabled;
	extern int storeVideoMemoryInSavestates;
	extern int storeGuardedPagesInSavestates;
	extern int appLocale;
	extern int tempAppLocale;
	extern int debugPrintMode;
	extern LogCategoryFlag includeLogFlags;
	extern LogCategoryFlag traceLogFlags;
	extern LogCategoryFlag excludeLogFlags;
	extern int inputFocusFlags;
	extern int hotkeysFocusFlags;

	extern char moviefilename [MAX_PATH+1];
	extern char exefilename [MAX_PATH+1];
	extern char commandline [160];
	extern char thisprocessPath [MAX_PATH+1];

	static const char* defaultConfigFilename = "hourglass.cfg";

	int Save_Config(const char* filename=0);

	int Save_As_Config(HWND hWnd, HINSTANCE hInst);

	int Load_Config(const char* filename=0);

	int Load_As_Config(HWND hWnd, HINSTANCE hInst);

}
