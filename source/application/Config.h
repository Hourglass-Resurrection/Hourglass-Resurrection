/*
 * Copyright(c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "shared/ipc.h"

#ifndef FOCUS_FLAGS_DEFINED
#define FOCUS_FLAGS_DEFINED
enum
{
    FOCUS_FLAG_TASER = 0x01,
    FOCUS_FLAG_TASEE = 0x02,
    FOCUS_FLAG_OTHER = 0x04,
};
#endif
/* Contains a lot of variables about the state of a given movie.
 * Or so I suppose. That's a lot of things to be found there!
 */
namespace Config
{ // A namespace is maybe not the best solution...

    // TODO: Comment *everything* !
    extern TasFlags localTASflags; /** TODO What is the role of localTASflags ?*/
    //extern int audioFrequency;
    //extern int audioBitsPerSecond;
    //extern int audioChannels;
    extern bool paused;
    extern bool fastforward;
    extern bool started;
    //extern bool playback;
    extern bool finished;
    extern bool
        nextLoadRecords; // false if next load switches to playback, true if next load switches to recording... aka readonly/read-only toggle
    extern bool recoveringStale;
    extern bool exeFileExists;
    extern bool movieFileExists;
    extern bool movieFileWritable;
    //extern int forceWindowed;
    extern int truePause;
    extern int onlyHookChildProcesses;
    //extern int forceSurfaceMemory;
    //extern int forceSoftware;
    //extern int aviMode;
    //extern int emuMode;
    //extern int fastForwardFlags;
    //extern int timescale, timescaleDivisor;
    //extern int allowLoadInstalledDlls, allowLoadUxtheme;
    extern int runDllLast;
    extern int advancePastNonVideoFrames;
    extern bool advancePastNonVideoFramesConfigured;
    //extern int threadMode;
    extern int usedThreadMode;
    extern bool limitGameToOnePhysicalCore;
    extern bool disableHyperThreading;
    //extern int timersMode;
    //extern int messageSyncMode;
    //extern int waitSyncMode;
    extern int aviFrameCount;
    extern int aviSoundFrameCount;
    extern bool traceEnabled;
    //extern int storeVideoMemoryInSavestates;
    extern int storeGuardedPagesInSavestates;
    //extern int appLocale;
    extern int tempAppLocale;
    /*extern LogCategoryFlag includeLogFlags;
	extern LogCategoryFlag traceLogFlags;
	extern LogCategoryFlag excludeLogFlags;*/
    extern int inputFocusFlags;
    extern int hotkeysFocusFlags;

    extern std::wstring movie_filename;
    extern std::wstring exe_filename;
    extern std::wstring command_line;
    extern std::wstring this_process_path;

    static LPCWSTR defaultConfigFilename = L"hourglass.cfg";

    /* Saves the configuration.
     * If no filename is given, the default configuration file is used.
     * @see defaultConfigFilename
     * @param filename a char* with the path 
     */
    int Save_Config(LPCWSTR filename = nullptr);

    /*
     * Saves in the default config file the window and the program's instance.
     */
    int Save_As_Config();

    /* Loads the configuration.
     * If no filename is given, the default configuration file is used.
     * @see defaultConfigFilename
     * @param filename a char* with the path 
     */
    int Load_Config(LPCWSTR filename = nullptr);

    /* 
     * Loads from the default config file the window and the program's instance.
     */
    int Load_As_Config();
}
