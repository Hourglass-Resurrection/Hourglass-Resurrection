#include "../shared/logcat.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SetPrivateProfileIntA(lpAppName, lpKeyName, nValue, lpFileName) \
	sprintf(Str_Tmp, "%d", nValue); \
	WritePrivateProfileStringA(lpAppName, lpKeyName, Str_Tmp, lpFileName);


class Config{
public:

	// TODO: Comment *everything* !
	int audioFrequency;
	int audioBitsPerSecond;
	int audioChannels;
	bool paused;
	bool fastforward;
	bool started;
	bool playback;
	bool finished;
	bool nextLoadRecords;
	bool recoveringStale;
	bool exeFileExists;
	bool movieFileExists;
	bool movieFileWritable;
	int forceWindowed;
	int truePause;
	int onlyHookChildProcesses;
	int forceSurfaceMemory;
	int forceSoftware;
	int aviMode;
	int emuMode;
	int fastForwardFlags;
	int timescale, timescaleDivisor;
	int allowLoadInstalledDlls, allowLoadUxtheme, runDllLast;
	int advancePastNonVideoFrames;
	bool advancePastNonVideoFramesConfigured;
	int threadMode;
	int usedThreadMode;
	int timersMode;
	int messageSyncMode;
	int waitSyncMode;
	int aviFrameCount;
	int aviSoundFrameCount;
	bool traceEnabled;
	bool crcVerifyEnabled;
	int storeVideoMemoryInSavestates;
	int storeGuardedPagesInSavestates;
	int appLocale;
	int tempAppLocale;
	int debugPrintMode;
	LogCategoryFlag includeLogFlags;
	LogCategoryFlag traceLogFlags;
	LogCategoryFlag excludeLogFlags;
	int inputFocusFlags;
	int hotkeysFocusFlags;

	char moviefilename [MAX_PATH+1];
	char exefilename [MAX_PATH+1];
	char commandline [160];
	char thisprocessPath [MAX_PATH+1];

	static const char* defaultConfigFilename;

	int Save_Config(const char* filename);

	int Save_As_Config(HWND hWnd, HINSTANCE hInst);

	int Load_Config(const char* filename);

	int Load_As_Config(HWND hWnd, HINSTANCE hInst);

}
