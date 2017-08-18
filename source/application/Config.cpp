/*
 * Copyright(c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "Config.h"
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#include <stdio.h>

#include "ramwatch.h"
#include "logging.h"
#include "shared/version.h"
#include "utils/File.h"

namespace
{
    BOOL SetPrivateProfileIntW(LPCWSTR app_name, LPCWSTR key_name, INT value, LPCWSTR filename)
    {
        WCHAR tmp_str[48];
        swprintf(tmp_str, ARRAYSIZE(tmp_str), L"%d", value);
        return WritePrivateProfileStringW(app_name, key_name, tmp_str, filename);
    }
}

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
        {{{ true }}},
		//LCF_NONE|LCF_NONE, //includeLogFlags|traceLogFlags,
		//LCF_ERROR, //excludeLogFlags
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
	//int storeVideoMemoryInSavestates;
	int storeGuardedPagesInSavestates = 1;
	//int appLocale;
	int tempAppLocale = 0;
	/*LogCategoryFlag includeLogFlags = LCF_ERROR;
	LogCategoryFlag traceLogFlags = LCF_NONE;
	LogCategoryFlag excludeLogFlags = LCF_NONE|LCF_FREQUENT;*/
	int inputFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_OTHER|FOCUS_FLAG_TASER; // allowbackgroundinput;
	int hotkeysFocusFlags = FOCUS_FLAG_TASEE|FOCUS_FLAG_TASER; // allowbackgroundhotkeys;

    std::wstring movie_filename;
    std::wstring exe_filename;
    std::wstring command_line;
    std::wstring this_process_path;

	//const char* defaultConfigFilename = "hourglass.cfg";


	int Save_Config(LPCWSTR filename)
	{
        WCHAR Str_Tmp[1024] = { 0 };
        if (filename == nullptr)
        {
            filename = defaultConfigFilename;
        }
        WCHAR Conf_File[1024];

        if (*filename && filename[1] == ':')
        {
            wcscpy(Conf_File, filename);
        }
        else
        {
            swprintf(Conf_File, ARRAYSIZE(Conf_File), L"%s\\%s", this_process_path.c_str(), filename);
        }

        WritePrivateProfileStringW(L"General", L"Exe path", exe_filename.c_str(), Conf_File);
        WritePrivateProfileStringW(L"General", L"Movie path", movie_filename.c_str(), Conf_File);
        WritePrivateProfileStringW(L"General", L"Command line", command_line.c_str(), Conf_File);

        SetPrivateProfileIntW(L"General", L"Movie Read Only", nextLoadRecords, Conf_File);
        //SetPrivateProfileIntA("Graphics", "Force Windowed", localTASflags.forceWindowed, Conf_File);
        SetPrivateProfileIntW(L"Tools", L"Fast Forward Flags", localTASflags.fastForwardFlags, Conf_File);
        if (advancePastNonVideoFramesConfigured)
        {
            SetPrivateProfileIntW(L"Input", L"Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
        }
        SetPrivateProfileIntW(L"Input", L"Background Input Focus Flags", inputFocusFlags, Conf_File);;
        SetPrivateProfileIntW(L"Input", L"Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

        SetPrivateProfileIntW(L"Debug", L"Load Debug Tracing", traceEnabled, Conf_File);

        swprintf(Str_Tmp, ARRAYSIZE(Str_Tmp), L"%d", AutoRWLoad);
        WritePrivateProfileStringW(L"Watches", L"AutoLoadWatches", Str_Tmp, Conf_File);

        swprintf(Str_Tmp, ARRAYSIZE(Str_Tmp), L"%d", RWSaveWindowPos);
        WritePrivateProfileStringW(L"Watches", L"SaveWindowPosition", Str_Tmp, Conf_File);

        if (RWSaveWindowPos)
        {
            SetPrivateProfileIntW(L"Watches", L"Ramwatch_X", ramw_x, Conf_File);
            SetPrivateProfileIntW(L"Watches", L"Ramwatch_Y", ramw_y, Conf_File);
        }

        for (int i = 0; i < MAX_RECENT_WATCHES; i++)
        {
            swprintf(Str_Tmp, ARRAYSIZE(Str_Tmp), L"Recent Watch %d", i + 1);
            WritePrivateProfileStringW(L"Watches", Str_Tmp, &rw_recent_files[i][0], Conf_File);
        }

		// TODO: save hotkeys!
		//SaveHotkeys(Conf_File, true);
		//SaveHotkeys(Conf_File, false);

        return 1;
    }


    int Save_As_Config()
    {
        std::wstring config_file_name = Utils::File::GetFileNameSave(this_process_path, {
            Utils::File::FileFilter::Config,
            Utils::File::FileFilter::AllFiles,
        });

        if (config_file_name.empty())
        {
            return 0;
        }
        else
        {
            Save_Config(config_file_name.c_str());
            debugprintf(L"config saved in \"%s\".\n", config_file_name);
            return 1;
        }
	}


	int Load_Config(LPCWSTR filename)
	{
		if(!filename)
			filename = defaultConfigFilename;
		WCHAR Conf_File[1024];
        WCHAR temp_str[1024];

		if(*filename && filename[1] == ':')
			wcscpy(Conf_File, filename);
		else
			swprintf(Conf_File, ARRAYSIZE(Conf_File), L"%s\\%s", this_process_path.c_str(), filename);

		GetPrivateProfileStringW(L"General", L"Exe path", exe_filename.c_str(), temp_str, MAX_PATH, Conf_File);
        exe_filename = temp_str;
		GetPrivateProfileStringW(L"General", L"Movie path", movie_filename.c_str(), temp_str, MAX_PATH, Conf_File);
        movie_filename = temp_str;
		GetPrivateProfileStringW(L"General", L"Command line", command_line.c_str(), temp_str, ARRAYSIZE(temp_str), Conf_File);
        command_line = temp_str;

		nextLoadRecords = 0!=GetPrivateProfileIntW(L"General", L"Movie Read Only", nextLoadRecords, Conf_File);
		//localTASflags.forceWindowed = GetPrivateProfileIntA("Graphics", "Force Windowed", localTASflags.forceWindowed, Conf_File);
		localTASflags.fastForwardFlags = GetPrivateProfileIntW(L"Tools", L"Fast Forward Flags", localTASflags.fastForwardFlags, Conf_File);
		advancePastNonVideoFrames = GetPrivateProfileIntW(L"Input", L"Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
		advancePastNonVideoFramesConfigured = 0!=GetPrivateProfileIntW(L"Input", L"Skip Lag Frames", 0, Conf_File);
		inputFocusFlags = GetPrivateProfileIntW(L"Input", L"Background Input Focus Flags", inputFocusFlags, Conf_File);
		hotkeysFocusFlags = GetPrivateProfileIntW(L"Input", L"Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

		traceEnabled = 0!=GetPrivateProfileIntW(L"Debug", L"Load Debug Tracing", traceEnabled, Conf_File);

		if (RWSaveWindowPos)
		{
            ramw_x = GetPrivateProfileIntW(L"Watches", L"Ramwatch_X", 0, Conf_File);
            ramw_y = GetPrivateProfileIntW(L"Watches", L"Ramwatch_Y", 0, Conf_File);
		}

		for(int i = 0; i < MAX_RECENT_WATCHES; i++)
		{
			WCHAR str[256];
			swprintf(str, ARRAYSIZE(str), L"Recent Watch %d", i+1);
			GetPrivateProfileStringW(L"Watches", str, L"", &rw_recent_files[i][0], 1024, Conf_File);
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
			debugprintf(L"loaded config from \"%S\".\n", Conf_File);

		return 1;
	}


    int Load_As_Config()
    {
        std::wstring config_file_name = Utils::File::GetFileNameOpen(this_process_path, {
            Utils::File::FileFilter::Config,
            Utils::File::FileFilter::AllFiles,
        });

        if (config_file_name.empty())
        {
            return 0;
        }
        else
        {
            Load_Config(config_file_name.c_str());
            return 1;
        }
    }
}
