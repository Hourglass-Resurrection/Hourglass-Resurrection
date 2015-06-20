#include "Config.h"
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#include <stdio.h>

#include "ramwatch.h"
#include "logging.h"
#include <shared\version.h>

namespace Config{

	//#include "../shared/ipc.h"
// Globalized the TAS-flags struct, should reduce memory usage some.
	TasFlags localTASflags =
	{
		true, //playback
		60, //framerate
		8, //keylimit
		0, //forceSoftware
		0, //windowActivateFlags
		1, //threadMode
		0, //threadStackSize
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

	//int audioFrequency;
	//int audioBitsPerSecond;
	//int audioChannels;
	bool paused = false;
	bool fastforward;
	bool started = false;
	//bool playback;
	bool finished = false;
	bool nextLoadRecords = true; // false if next load switches to playback, true if next load switches to recording... aka readonly/read-only toggle
	bool recoveringStale = false;
	bool exeFileExists = false;
	bool movieFileExists = false;
	bool movieFileWritable = false;
	//int forceWindowed = 1;
	int truePause = 0;
	int onlyHookChildProcesses = 0;
	//int forceSurfaceMemory;
	//int forceSoftware;
	//int aviMode;
	//int emuMode;
	//int fastForwardFlags;
	//int timescale, timescaleDivisor;
	//int allowLoadInstalledDlls, allowLoadUxtheme;
	int runDllLast = 0;
	int advancePastNonVideoFrames = 0;
	bool advancePastNonVideoFramesConfigured = false;
	//int threadMode;
	int usedThreadMode = -1;
	bool limitGameToOnePhysicalCore = false;
	bool disableHyperThreading = false;
	//int timersMode;
	//int messageSyncMode;
	//int waitSyncMode;
	int aviFrameCount;
	int aviSoundFrameCount;
	bool traceEnabled = true;
	bool crcVerifyEnabled = true;
	//int storeVideoMemoryInSavestates;
	int storeGuardedPagesInSavestates = 1;
	//int appLocale;
	int tempAppLocale = 0;
	//int debugPrintMode;
	LogCategoryFlag includeLogFlags = LCF_ERROR;
	LogCategoryFlag traceLogFlags = LCF_NONE;
	LogCategoryFlag excludeLogFlags = LCF_NONE|LCF_FREQUENT;
	int inputFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_OTHER|FOCUS_FLAG_TASER; // allowbackgroundinput;
	int hotkeysFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_TASER; // allowbackgroundhotkeys;

	char moviefilename [MAX_PATH+1];
	char exefilename [MAX_PATH+1];
	char commandline [160];
	char thisprocessPath [MAX_PATH+1];

	//const char* defaultConfigFilename = "hourglass.cfg";


	int Save_Config(const char* filename)
	{
		char Str_Tmp [1024] = {0};
		if(!filename)
			filename = defaultConfigFilename;
		char Conf_File[1024];

		if(*filename && filename[1] == ':')
			strcpy(Conf_File, filename);
		else
			sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

		WritePrivateProfileStringA("General", "Exe path", exefilename, Conf_File);
		WritePrivateProfileStringA("General", "Movie path", moviefilename, Conf_File);
		WritePrivateProfileStringA("General", "Command line", commandline, Conf_File);

		SetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
		//SetPrivateProfileIntA("Graphics", "Force Windowed", localTASflags.forceWindowed, Conf_File);
		SetPrivateProfileIntA("Tools", "Fast Forward Flags", localTASflags.fastForwardFlags, Conf_File);
		if(advancePastNonVideoFramesConfigured)
			SetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
		SetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
		SetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

		SetPrivateProfileIntA("Debug", "Debug Logging Mode", localTASflags.debugPrintMode, Conf_File);
		SetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
		SetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

		wsprintf(Str_Tmp, "%d", AutoRWLoad);
		WritePrivateProfileString("Watches", "AutoLoadWatches", Str_Tmp, Conf_File);

		wsprintf(Str_Tmp, "%d", RWSaveWindowPos);
		WritePrivateProfileString("Watches", "SaveWindowPosition", Str_Tmp, Conf_File);

		if (RWSaveWindowPos)
		{
			SetPrivateProfileIntA("Watches", "Ramwatch_X", ramw_x, Conf_File);
			SetPrivateProfileIntA("Watches", "Ramwatch_Y", ramw_y, Conf_File);
		}

		for(int i = 0; i < MAX_RECENT_WATCHES; i++)
		{
			char str[256];
			sprintf(str, "Recent Watch %d", i+1);
			WritePrivateProfileStringA("Watches", str, &rw_recent_files[i][0], Conf_File);	
		}

		// TODO: save hotkeys!
		//SaveHotkeys(Conf_File, true);
		//SaveHotkeys(Conf_File, false);

		return 1;
	}


	int Save_As_Config(HWND hWnd, HINSTANCE hInst)
	{
		char Name[2048];
		OPENFILENAME ofn;

		SetCurrentDirectoryA(thisprocessPath);

		strcpy(&Name[0], defaultConfigFilename);
		memset(&ofn, 0, sizeof(OPENFILENAME));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hWnd;
		ofn.hInstance = hInst;
		ofn.lpstrFile = Name;
		ofn.nMaxFile = 2047;
		ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrInitialDir = thisprocessPath;
		ofn.lpstrDefExt = "cfg";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

		if(GetSaveFileName(&ofn))
		{
			Save_Config(Name);
			debugprintf("config saved in \"%s\".\n", Name);
			return 1;
		}
		else return 0;
	}


	int Load_Config(const char* filename)
	{
		if(!filename)
			filename = defaultConfigFilename;
		char Conf_File[1024];

		if(*filename && filename[1] == ':')
			strcpy(Conf_File, filename);
		else
			sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

		GetPrivateProfileStringA("General", "Exe path", exefilename, exefilename, MAX_PATH, Conf_File);
		GetPrivateProfileStringA("General", "Movie path", moviefilename, moviefilename, MAX_PATH, Conf_File);
		GetPrivateProfileStringA("General", "Command line", commandline, commandline, ARRAYSIZE(commandline), Conf_File);

		nextLoadRecords = 0!=GetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
		//localTASflags.forceWindowed = GetPrivateProfileIntA("Graphics", "Force Windowed", localTASflags.forceWindowed, Conf_File);
		localTASflags.fastForwardFlags = GetPrivateProfileIntA("Tools", "Fast Forward Flags", localTASflags.fastForwardFlags, Conf_File);
		advancePastNonVideoFrames = GetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
		advancePastNonVideoFramesConfigured = 0!=GetPrivateProfileIntA("Input", "Skip Lag Frames", 0, Conf_File);
		inputFocusFlags = GetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
		hotkeysFocusFlags = GetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

		localTASflags.debugPrintMode = GetPrivateProfileIntA("Debug", "Debug Logging Mode", localTASflags.debugPrintMode, Conf_File);
		traceEnabled = 0!=GetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
		crcVerifyEnabled = 0!=GetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

		if (RWSaveWindowPos)
		{
			ramw_x = GetPrivateProfileIntA ("Watches", "Ramwatch_X", 0, Conf_File);
			ramw_y = GetPrivateProfileIntA ("Watches", "Ramwatch_Y", 0, Conf_File);
		}

		for(int i = 0; i < MAX_RECENT_WATCHES; i++)
		{
			char str[256];
			sprintf(str, "Recent Watch %d", i+1);
			GetPrivateProfileStringA("Watches", str, "", &rw_recent_files[i][0], 1024, Conf_File);
		}

		// TODO: Load hotkeys.
		//LoadHotkeys(Conf_File, true);
		//LoadHotkeys(Conf_File, false);

		// done loading

		//UpdateReverseLookup(true);
		//UpdateReverseLookup(false);

		// Reeeeeeeeeeeeally weird code...
		static bool first = true;
		if(first)
			first = false;
		else
			debugprintf("loaded config from \"%s\".\n", Conf_File);

		return 1;
	}


	int Load_As_Config(HWND hWnd, HINSTANCE hInst)
	{
		char Name[2048];
		OPENFILENAME ofn;

		SetCurrentDirectoryA(thisprocessPath);

		strcpy(&Name[0], defaultConfigFilename);
		memset(&ofn, 0, sizeof(OPENFILENAME));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hWnd;
		ofn.hInstance = hInst;
		ofn.lpstrFile = Name;
		ofn.nMaxFile = 2047;
		ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrInitialDir = thisprocessPath;
		ofn.lpstrDefExt = "cfg";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		if(GetOpenFileName(&ofn))
		{
			Load_Config(Name);
			return 1;
		}
		else return 0;
	}

}