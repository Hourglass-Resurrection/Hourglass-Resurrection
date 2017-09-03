#include "MainMenu.h"
#include <stdio.h>
#include "Config.h"
#include "resource.h"
#include "shared/ipc.h"
using namespace Config;

void HelperFuncInsertMenuByID(HMENU smenu,
                              UINT pos,
                              UINT flags,
                              UINT_PTR id,
                              LPCWSTR suffix,
                              LPCWSTR str,
                              LPCWSTR disableReason)
{
    WCHAR Str_Tmp[1024];
#if 0
	GetPrivateProfileStringA(language_name[Language], str, def, Str_Tmp, 1024, Language_Path);
#endif
    if (disableReason && (flags & (MF_DISABLED | MF_GRAYED)))
        swprintf(Str_Tmp, ARRAYSIZE(Str_Tmp), L"%s (%s)", str, disableReason);
    else
        wcscpy(Str_Tmp, str);
    wcscat(Str_Tmp, suffix);
#if 0
	AddHotkeySuffix(Str_Tmp, id, suffix, true);
#endif
    InsertMenuW(smenu, pos, flags, id, Str_Tmp);
}

__forceinline void HelperFuncInsertMenu(HMENU smenu,
                                        UINT pos,
                                        UINT flags,
                                        HMENU menu,
                                        LPCWSTR suffix,
                                        LPCWSTR str,
                                        LPCWSTR disableReason)
{
    HelperFuncInsertMenuByID(smenu,
                             pos,
                             flags,
                             reinterpret_cast<UINT_PTR>(menu),
                             suffix,
                             str,
                             disableReason);
}

void Build_Main_Menu(HMENU& MainMenu, HWND hWnd)
{
    unsigned int Flags;
    int i = 0, j = 0;
    WCHAR str[1024];
    //char Str_Tmp[1024] = { 0 };

    if (MainMenu)
        DestroyMenu(MainMenu);

    MainMenu = CreateMenu();

    HMENU Files = CreatePopupMenu();
    HMENU Graphics = CreatePopupMenu();
    HMENU CPU = CreatePopupMenu();
    HMENU Sound = CreatePopupMenu();
    HMENU Input = CreatePopupMenu();
    HMENU Exec = CreatePopupMenu();
    HMENU Options = CreatePopupMenu();
    HMENU TAS_Tools = CreatePopupMenu();
#if 0
	HMENU Time = CreatePopupMenu();
#endif
    HMENU Time = TAS_Tools;
    HMENU Avi = CreatePopupMenu();
    HMENU Help = CreatePopupMenu();
#if 0
	HMENU FilesChangeState = CreatePopupMenu();
	HMENU FilesSaveState = CreatePopupMenu();
	HMENU FilesLoadState = CreatePopupMenu();
	HMENU FilesHistory = CreatePopupMenu();
	HMENU GraphicsRender = CreatePopupMenu();
#endif
    HMENU GraphicsMemory = CreatePopupMenu();
#if 0
	HMENU GraphicsLayers = CreatePopupMenu();
	HMENU WindowOptions = CreatePopupMenu();
	HMENU CPUSlowDownSpeed = CreatePopupMenu();
#endif
    HMENU SoundFormat = CreatePopupMenu();
    HMENU TimeFastForward = CreatePopupMenu();
    HMENU TimeRate = CreatePopupMenu();
    HMENU ExecMultithreading = CreatePopupMenu();
    HMENU ExecTimers = CreatePopupMenu();
    HMENU ExecMessageSync = CreatePopupMenu();
    HMENU ExecWaitSync = CreatePopupMenu();
    HMENU ExecDlls = CreatePopupMenu();
    HMENU Locale = CreatePopupMenu();
    HMENU Affinity = CreatePopupMenu();
    HMENU DebugLogging = CreatePopupMenu();
    HMENU Performance = CreatePopupMenu();
#if 0
	HMENU Tools_Movies = CreatePopupMenu();
	HMENU Tools_AVI = CreatePopupMenu();
	HMENU Lua_Script = CreatePopupMenu();
#endif
    HMENU InputHotkeyFocus = CreatePopupMenu();
    HMENU InputInputFocus = CreatePopupMenu();

    std::wstring exe_filename_only;
    if (!exeFileExists)
    {
        size_t slash = exe_filename.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
        {
            exe_filename_only = exe_filename.substr(slash + 1);
        }
        else
        {
            exe_filename_only = exe_filename;
        }
    }
    std::wstring movie_filename_only;
    if (!movieFileExists)
    {
        size_t slash = movie_filename.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
        {
            movie_filename_only = movie_filename.substr(slash + 1);
        }
        else
        {
            movie_filename_only = movie_filename;
        }
    }
    bool on = true;

    Flags = MF_BYPOSITION | MF_POPUP | MF_STRING;

    i = 0;
    HelperFuncInsertMenu(MainMenu, i++, Flags, Files, L"", L"&File", 0);
    HelperFuncInsertMenu(MainMenu, i++, Flags, Graphics, L"", L"&Graphics", 0);
    HelperFuncInsertMenu(MainMenu, i++, Flags, Sound, L"", L"&Sound", 0);
    HelperFuncInsertMenu(MainMenu, i++, Flags, Exec, L"", L"&Runtime", 0);
    HelperFuncInsertMenu(MainMenu, i++, Flags, Input, L"", L"&Input", 0);
    HelperFuncInsertMenu(MainMenu, i++, Flags, TAS_Tools, L"", L"&Tools", 0);
    HelperFuncInsertMenu(MainMenu,
                         i++,
                         Flags,
                         Avi,
                         L"",
                         (localTASflags.aviMode && started && (aviFrameCount || aviSoundFrameCount))
                             ? ((aviFrameCount && aviSoundFrameCount) ? L"&AVI!!" : L"&AVI!")
                             : L"&AVI",
                         0);
#if 0
	HelperFuncInsertMenuByID(MainMenu, i++, Flags, Help, "", "&Help", 0);
#endif

    // File Menu
    Flags = MF_BYPOSITION | MF_STRING;
    i = 0;

    on = !exe_filename_only.empty() && !started;
    swprintf(str,
             ARRAYSIZE(str),
             L"&Open Executable... %s%s%s",
             on ? L"(now open: \"" : L"",
             on ? exe_filename_only.c_str() : L"",
             on ? L"\")" : L"");
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | (started ? MF_GRAYED : 0),
                             ID_FILES_OPENEXE,
                             L"\tCtrl+O",
                             str,
                             L"can't change while running");
    on = !movie_filename_only.empty() && !started;
    swprintf(str,
             ARRAYSIZE(str),
             L"&Open Movie... %s%s%s",
             on ? L"(now open: \"" : L"",
             on ? movie_filename_only.c_str() : L"",
             on ? L"\")" : L"");
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | (started ? MF_GRAYED : 0),
                             ID_FILES_OPENMOV,
                             L"\tCtrl+M",
                             str,
                             L"can't change while running");

    InsertMenuW(Files, i++, MF_SEPARATOR, NULL, NULL);

#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_PLAYMOV, "\tCtrl+P", "&Play Movie...", 0);
	HelperFuncInsertMenuByID(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_WATCHMOV, "", "&Watch From Beginning", "must be running");
#endif
    swprintf(str,
             ARRAYSIZE(str),
             started ? L"&Watch From Beginning" : L"&Play Movie %s%s%s",
             !movie_filename_only.empty() ? L"\"" : L"",
             movie_filename_only.c_str(),
             !movie_filename_only.empty() ? L"\"" : L"");
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | (!(movieFileExists && exeFileExists) ? MF_GRAYED : 0),
                             started ? ID_FILES_WATCHMOV : ID_FILES_PLAYMOV,
                             L"\tCtrl+P",
                             str,
                             movieFileExists ? L"must open an executable first"
                                             : L"must open a movie first");

    InsertMenuW(Files, i++, MF_SEPARATOR, NULL, NULL);

    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | ((started || !exeFileExists) ? MF_GRAYED : 0),
                             ID_FILES_RECORDMOV,
                             L"\tCtrl+R",
                             L"&Record New Movie...",
                             started ? L"must stop running first"
                                     : L"must open an executable first");
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | ((!started || localTASflags.playback) ? MF_GRAYED : 0),
                             ID_FILES_BACKUPMOVIE,
                             L"",
                             L"Backup Movie to File...",
                             L"Must be recording");
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_RESUMEMOVAS, "", (playback||!started) ? "Resume Recording to &Different File..." : "Continue Recording to &Different File...", "must be running");
#endif
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | ((!started || !localTASflags.playback || finished) ? MF_GRAYED
                                                                                        : 0),
                             ID_FILES_RESUMERECORDING,
                             L"",
                             L"Resume Recording from Now",
                             L"movie must be playing");
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags | ((!started||finished)?MF_GRAYED:0), ID_FILES_SPLICE, "", "Splice...", "movie must be playing or recording");
#endif
    InsertMenuW(Files, i++, MF_SEPARATOR, NULL, NULL);
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_SAVECONFIG, "", "Save Config", 0);
#endif
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags,
                             ID_FILES_SAVECONFIGAS,
                             L"",
                             L"Save Config As...",
                             0);
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags,
                             ID_FILES_LOADCONFIGFROM,
                             L"",
                             L"Load Config From...",
                             0);

#if 0
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	if (strcmp(Recent_Rom[0], ""))
	{
		HelperFuncInsertMenuByID(Files, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, FilesHistory, "", "&Recent EXEs", 0);
		InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	}
#endif

    InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Files,
                             i++,
                             Flags | (!started ? MF_GRAYED : 0),
                             ID_FILES_STOP_RELAY,
                             L"",
                             L"Stop Running",
                             L"already stopped");
    InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_QUIT, L"\tAlt+F4", L"E&xit", 0);

#if 0
	// Menu FilesChangeState
	
	HelperFuncInsertMenuByID(FilesChangeState, i++, Flags, ID_FILES_PREVIOUSSTATE, "Previous State", "", "Previous State");
	HelperFuncInsertMenuByID(FilesChangeState, i++, Flags, ID_FILES_NEXTSTATE, "Next State", "", "Next State");
	InsertMenu(FilesChangeState, i++, MF_SEPARATOR, NULL, NULL);
	for(j = 0; j < 10; j++)
	{
		sprintf(Str_Tmp ,"Set &%d", (j+1)%10);
		HelperFuncInsertMenuByID(FilesChangeState, i++, Flags | (Current_State == ((j+1)%10) ? MF_CHECKED : MF_UNCHECKED), ID_FILES_SETSTATE_1 + j, Str_Tmp, "", Str_Tmp);
	}

	HelperFuncInsertMenuByID(FilesSaveState, i++, Flags, ID_FILES_SAVESTATE, "Save State", "\tF5", "Quick &Save");
	HelperFuncInsertMenuByID(FilesSaveState, i++, Flags, ID_FILES_SAVESTATEAS, "Save State as", "\tShift+F5", "&Save State as...");
	InsertMenu(FilesSaveState, i++, MF_SEPARATOR, NULL, NULL);
	for(j = 0; j < 10; j++)
	{
		sprintf(Str_Tmp ,"Save &%d", (j+1)%10);
		HelperFuncInsertMenuByID(FilesSaveState, i++, Flags, ID_FILES_SAVESTATE_1 + j, Str_Tmp, "", Str_Tmp);
	}

	HelperFuncInsertMenuByID(FilesLoadState, i++, Flags, ID_FILES_LOADSTATE, "Load State", "\tF8", "Quick &Load");
	HelperFuncInsertMenuByID(FilesLoadState, i++, Flags, ID_FILES_LOADSTATEAS, "Load State as", "\tShift+F8", "&Load State...");
	InsertMenu(FilesLoadState, i++, MF_SEPARATOR, NULL, NULL);
	for(j = 0; j < 10; j++)
	{
		sprintf(Str_Tmp ,"Load &%d", (j+1)%10);
		HelperFuncInsertMenuByID(FilesLoadState, i++, Flags, ID_FILES_LOADSTATE_1 + j, Str_Tmp, "", Str_Tmp);
	}

	// Menu FilesHistory
	
	for(i = 0; i < MAX_RECENT_ROMS; i++)
	{
		if (strcmp(Recent_Rom[i], ""))
		{
			char tmp[1024];
			switch (Detect_Format(Recent_Rom[i]) >> 1)
			{
				default:
					strcpy(tmp, "[---]    - "); // does not exist anymore
					break;
				case 1:
					strcpy(tmp, "[MD]   - ");
					break;
			}
			Get_Name_From_Path(Recent_Rom[i], Str_Tmp);
			strcat(tmp, Str_Tmp);
			// & is an escape sequence in windows menu names, so replace & with &&
			int len = strlen(tmp);
			for(int j = 0; j < len && len < 1023; j++)
				if(tmp[j] == '&')
					memmove(tmp+j+1, tmp+j, strlen(tmp+j)+1), ++len, ++j;
			HelperFuncInsertMenuByID(FilesHistory, i, Flags, ID_FILES_OPENRECENTROM0 + i, tmp, "", tmp);
		}
		else break;
	}
#endif

    // Graphics Menu
    Flags = MF_BYPOSITION | MF_STRING;

    i = 0;
    HelperFuncInsertMenuByID(Graphics,
                             i++,
                             Flags | (!localTASflags.forceWindowed ? MF_CHECKED : MF_UNCHECKED)
                                 | (started ? MF_GRAYED : 0),
                             ID_GRAPHICS_ALLOWFULLSCREEN,
                             L"",
                             L"Allow &Fullscreen / Display Mode Changes",
                             L"can't change while running");
    InsertMenu(Graphics, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Graphics,
                             i++,
                             Flags | (!localTASflags.forceSoftware ? MF_CHECKED : MF_UNCHECKED)
                                 | (started ? MF_GRAYED : 0),
                             ID_GRAPHICS_FORCESOFTWARE,
                             L"",
                             L"&Allow Hardware Acceleration",
                             L"can't change while running");
    HelperFuncInsertMenu(Graphics,
                         i++,
                         Flags | MF_POPUP
                             | ((localTASflags.forceSoftware || started) ? MF_GRAYED : 0),
                         GraphicsMemory,
                         L"",
                         L"Surface &Memory",
                         started ? L"can't change while running"
                                 : L"hardware acceleration must be enabled");

    // GraphicsMemory Menu
    i = 0;
    HelperFuncInsertMenuByID(GraphicsMemory,
                             i++,
                             Flags | ((localTASflags.forceSurfaceMemory == 0) ? MF_CHECKED
                                                                              : MF_UNCHECKED),
                             ID_GRAPHICS_MEM_DONTCARE,
                             L"",
                             L"&Let Game Choose",
                             0);
    HelperFuncInsertMenuByID(GraphicsMemory,
                             i++,
                             Flags | ((localTASflags.forceSurfaceMemory == 1) ? MF_CHECKED
                                                                              : MF_UNCHECKED),
                             ID_GRAPHICS_MEM_SYSTEM,
                             L"",
                             L"&System Memory               (Slow Draw, Fast Read)",
                             0);
    HelperFuncInsertMenuByID(GraphicsMemory,
                             i++,
                             Flags | ((localTASflags.forceSurfaceMemory == 2) ? MF_CHECKED
                                                                              : MF_UNCHECKED),
                             ID_GRAPHICS_MEM_NONLOCAL,
                             L"",
                             L"&Non-Local Video Memory (Varies)",
                             0);
    HelperFuncInsertMenuByID(GraphicsMemory,
                             i++,
                             Flags | ((localTASflags.forceSurfaceMemory == 3) ? MF_CHECKED
                                                                              : MF_UNCHECKED),
                             ID_GRAPHICS_MEM_LOCAL,
                             L"",
                             L"Local &Video Memory        (Fast Draw, Slow Read)",
                             0);

    // Execution Menu
    i = 0;
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Locale, L"", L"App &Locale", 0);
    InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Affinity, L"", L"&Affinity", 0);
    InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenu(Exec,
                         i++,
                         Flags | MF_POPUP,
                         ExecMultithreading,
                         L"",
                         L"&Multithreading Mode",
                         0);
    HelperFuncInsertMenu(Exec,
                         i++,
                         Flags | MF_POPUP,
                         ExecTimers,
                         L"",
                         L"Multimedia &Timer Mode",
                         0);
    HelperFuncInsertMenu(Exec,
                         i++,
                         Flags | MF_POPUP,
                         ExecMessageSync,
                         L"",
                         L"Message &Sync Mode",
                         0);
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecWaitSync, L"", L"&Wait Sync Mode", 0);
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecDlls, L"", L"&DLL Loading", 0);
    HelperFuncInsertMenuByID(Exec,
                             i++,
                             Flags | ((truePause) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_USETRUEPAUSE,
                             L"",
                             L"Disable Pause Helper",
                             0);
    HelperFuncInsertMenuByID(Exec,
                             i++,
                             Flags | ((onlyHookChildProcesses) ? MF_CHECKED : MF_UNCHECKED)
                                 | (started ? MF_GRAYED : 0),
                             ID_EXEC_ONLYHOOKCHILDPROC,
                             L"",
                             L"Wait until sub-process creation" /*" (might help IWBTG)"*/,
                             L"can't change while running");
    InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Performance, L"", L"&Performance", 0);
    HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, DebugLogging, L"", L"&Debug Logging", 0);
#if 0
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_SUSPEND, "", "Suspend Secondary Threads", "must be running");
	HelperFuncInsertMenuByID(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_RESUME, "", "Resume Secondary Threads", "must be running");
	HelperFuncInsertMenuByID(Exec, i++, Flags | ((truePause)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_USETRUEPAUSE, "", "Don't handle any messages while paused", 0);
#endif

    // Time Menu
    i = 0;
    HelperFuncInsertMenuByID(Time,
                             i++,
                             Flags | (paused ? MF_CHECKED : MF_UNCHECKED),
                             ID_TIME_TOGGLE_PAUSE,
                             L"",
                             L"Pause",
                             0);
    InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenu(Time, i++, Flags | MF_POPUP, TimeRate, L"", L"&Slow Motion", 0);
    InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Time,
                             i++,
                             Flags | (fastforward ? MF_CHECKED : MF_UNCHECKED),
                             ID_TIME_TOGGLE_FASTFORWARD,
                             L"",
                             L"Fast-Forward",
                             0);
    HelperFuncInsertMenu(Time,
                         i++,
                         Flags | MF_POPUP,
                         TimeFastForward,
                         L"",
                         L"&Fast-Forward Options",
                         0);
    if (Time == TAS_Tools)
        InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);

    // Input Menu
    i = 0;
#if 0
	HelperFuncInsertMenuByID(Input, i++, Flags, ID_INPUT_HOTKEYS, "", "Configure &Hotkeys...", 0);
#endif
    HelperFuncInsertMenuByID(Input, i++, Flags, ID_INPUT_INPUTS, L"", L"Configure &Inputs...", 0);
    HelperFuncInsertMenu(Input,
                         i++,
                         Flags | MF_POPUP,
                         InputHotkeyFocus,
                         L"",
                         L"Enable Hotkeys When",
                         0);
    InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenu(Input,
                         i++,
                         Flags | MF_POPUP,
                         InputInputFocus,
                         L"",
                         L"Enable Game Input When",
                         0);
    InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Input,
                             i++,
                             Flags | (advancePastNonVideoFrames ? MF_CHECKED : MF_UNCHECKED),
                             ID_INPUT_SKIPLAGFRAMES,
                             L"",
                             L"Frame Advance Skips \"&Lag\" Frames",
                             0);

    i = 0;
    HelperFuncInsertMenuByID(InputHotkeyFocus,
                             i++,
                             Flags | ((hotkeysFocusFlags & FOCUS_FLAG_TASER) ? MF_CHECKED
                                                                             : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDHOTKEY_TASER,
                             L"",
                             L"Hourglass has Focus",
                             0);
    HelperFuncInsertMenuByID(InputHotkeyFocus,
                             i++,
                             Flags | ((hotkeysFocusFlags & FOCUS_FLAG_TASEE) ? MF_CHECKED
                                                                             : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDHOTKEY_TASEE,
                             L"",
                             L"Game Window has Focus",
                             0);
    HelperFuncInsertMenuByID(InputHotkeyFocus,
                             i++,
                             Flags | ((hotkeysFocusFlags & FOCUS_FLAG_OTHER) ? MF_CHECKED
                                                                             : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDHOTKEY_OTHER,
                             L"",
                             L"Other Programs have Focus",
                             0);

    i = 0;
    HelperFuncInsertMenuByID(InputInputFocus,
                             i++,
                             Flags | ((inputFocusFlags & FOCUS_FLAG_TASER) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDINPUTS_TASER,
                             L"",
                             L"Hourglass has Focus",
                             0);
    HelperFuncInsertMenuByID(InputInputFocus,
                             i++,
                             Flags | ((inputFocusFlags & FOCUS_FLAG_TASEE) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDINPUTS_TASEE,
                             L"",
                             L"Game Window has Focus",
                             0);
    HelperFuncInsertMenuByID(InputInputFocus,
                             i++,
                             Flags | ((inputFocusFlags & FOCUS_FLAG_OTHER) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_INPUT_BACKGROUNDINPUTS_OTHER,
                             L"",
                             L"Other Programs have Focus",
                             0);

    // Fast-Forward Submenu
    i = 0;
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((localTASflags.fastForwardFlags & FFMODE_FRONTSKIP)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_FF_FRONTSKIP,
                             L"",
                             L"Frontbuffer Frameskip",
                             0);
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((localTASflags.fastForwardFlags & FFMODE_BACKSKIP)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_FF_BACKSKIP,
                             L"",
                             L"Backbuffer Frameskip",
                             0);
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((recoveringStale
                                       || (localTASflags.fastForwardFlags & FFMODE_SOUNDSKIP))
                                          ? MF_CHECKED
                                          : MF_UNCHECKED)
                                 | (recoveringStale ? MF_GRAYED : 0),
                             ID_TIME_FF_SOUNDSKIP,
                             L"",
                             L"Soundskip",
                             L"always on while recovering stale");
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((localTASflags.fastForwardFlags & FFMODE_RAMSKIP)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_FF_RAMSKIP,
                             L"",
                             L"RAM Search/Watch Skip",
                             0);
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((localTASflags.fastForwardFlags & FFMODE_SLEEPSKIP)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_FF_SLEEPSKIP,
                             L"",
                             L"Sleep Skip",
                             0);
    HelperFuncInsertMenuByID(TimeFastForward,
                             i++,
                             Flags | ((localTASflags.fastForwardFlags & FFMODE_WAITSKIP)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_FF_WAITSKIP,
                             L"",
                             L"Wait Skip",
                             0);

    // Time Rate Submenu
    i = 0;
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags | ((localTASflags.timescale == localTASflags.timescaleDivisor)
                                          ? MF_GRAYED
                                          : 0),
                             ID_TIME_RATE_FASTER,
                             L"",
                             L"Speed &Up",
                             L"already at 100%");
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags
                                 | ((localTASflags.timescale * 8 == localTASflags.timescaleDivisor)
                                        ? MF_GRAYED
                                        : 0),
                             ID_TIME_RATE_SLOWER,
                             L"",
                             L"Slow &Down",
                             L"already at slowest option");
    InsertMenu(TimeRate, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags | ((localTASflags.timescale == localTASflags.timescaleDivisor)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_RATE_100,
                             L"",
                             L"&100% (normal speed)",
                             0);
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags | ((localTASflags.timescale * 4
                                       == localTASflags.timescaleDivisor * 3)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_TIME_RATE_75,
                             L"",
                             L"75%",
                             0);
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags
                                 | ((localTASflags.timescale * 2 == localTASflags.timescaleDivisor)
                                        ? MF_CHECKED
                                        : MF_UNCHECKED),
                             ID_TIME_RATE_50,
                             L"",
                             L"50%",
                             0);
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags
                                 | ((localTASflags.timescale * 4 == localTASflags.timescaleDivisor)
                                        ? MF_CHECKED
                                        : MF_UNCHECKED),
                             ID_TIME_RATE_25,
                             L"",
                             L"25%",
                             0);
    HelperFuncInsertMenuByID(TimeRate,
                             i++,
                             Flags
                                 | ((localTASflags.timescale * 8 == localTASflags.timescaleDivisor)
                                        ? MF_CHECKED
                                        : MF_UNCHECKED),
                             ID_TIME_RATE_12,
                             L"",
                             L"12%",
                             0);

    // Affinity Submenu
    i = 0;
    HelperFuncInsertMenuByID(
        Affinity,
        i++,
        Flags | ((limitGameToOnePhysicalCore) ? MF_CHECKED : MF_UNCHECKED)
            | ((started) ? MF_GRAYED : 0),
        ID_EXEC_LIMITONEPHYSICALCORE,
        L"",
        L"Use only one physical core (Might help older games to run correctly)",
        L"can't change after starting");
    HelperFuncInsertMenuByID(Affinity,
                             i++,
                             Flags | ((disableHyperThreading) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((started) ? MF_GRAYED : 0),
                             ID_EXEC_DISABLEHYPERTHREADS,
                             L"",
                             L"Disable Hyper-Threading",
                             L"can't change after starting");

    // Multithreading Submenu
    i = 0;
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 0) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_THREADS_DISABLE,
                             L"",
                             L"&Disable (prevent thread creation)",
                             0);
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 1) ? MF_CHECKED : MF_UNCHECKED)
                                 | (usedThreadMode >= 2 ? MF_GRAYED : 0),
                             ID_EXEC_THREADS_WRAP,
                             L"",
                             L"&Wrap (recycle threads)",
                             L"can't set while running after normal threads created");
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 2) ? MF_CHECKED : MF_UNCHECKED)
                                 | (usedThreadMode == 1 ? MF_GRAYED : 0),
                             ID_EXEC_THREADS_ALLOW,
                             L"",
                             L"&Allow (normal thread creation)",
                             L"can't set while running after wrapped threads created");
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 3) ? MF_CHECKED : MF_UNCHECKED)
                                 | (usedThreadMode == 1 ? MF_GRAYED : 0),
                             ID_EXEC_THREADS_KNOWN,
                             L"",
                             L"Allow &known threads only",
                             L"can't set while running after wrapped threads created");
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 4) ? MF_CHECKED : MF_UNCHECKED)
                                 | (usedThreadMode == 1 ? MF_GRAYED : 0),
                             ID_EXEC_THREADS_UNKNOWN,
                             L"",
                             L"Allow &unknown threads only",
                             L"can't set while running after wrapped threads created");
    HelperFuncInsertMenuByID(ExecMultithreading,
                             i++,
                             Flags | ((localTASflags.threadMode == 5) ? MF_CHECKED : MF_UNCHECKED)
                                 | (usedThreadMode == 1 ? MF_GRAYED : 0),
                             ID_EXEC_THREADS_TRUSTED,
                             L"",
                             L"Allow &trusted threads only",
                             L"can't set while running after wrapped threads created");

    // Timers Submenu
    i = 0;
    HelperFuncInsertMenuByID(ExecTimers,
                             i++,
                             Flags | ((localTASflags.timersMode == 0) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_TIMERS_DISABLE,
                             L"",
                             L"&Disable (prevent timer creation)",
                             0);
    HelperFuncInsertMenuByID(ExecTimers,
                             i++,
                             Flags | ((localTASflags.timersMode == 1) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_TIMERS_SYNC,
                             L"",
                             L"&Synchronous (run timers at frame boundaries)",
                             0);
    HelperFuncInsertMenuByID(ExecTimers,
                             i++,
                             Flags | ((localTASflags.timersMode == 2) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_TIMERS_ASYNC,
                             L"",
                             L"&Asynchronous (run timers in separate threads)",
                             0);

    // Message Sync Submenu
    i = 0;
    HelperFuncInsertMenuByID(ExecMessageSync,
                             i++,
                             Flags | ((localTASflags.messageSyncMode == 0) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_EXEC_MESSAGES_SYNC,
                             L"",
                             L"&Synchronous (no timeouts)",
                             0);
    HelperFuncInsertMenuByID(ExecMessageSync,
                             i++,
                             Flags | ((localTASflags.messageSyncMode == 1) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_EXEC_MESSAGES_SEMISYNC,
                             L"",
                             L"Semi-S&ynchronous (some timeouts)",
                             0);
    HelperFuncInsertMenuByID(ExecMessageSync,
                             i++,
                             Flags | ((localTASflags.messageSyncMode == 2) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_EXEC_MESSAGES_ASYNC,
                             L"",
                             L"&Asynchronous (any timeouts)",
                             0);
    HelperFuncInsertMenuByID(ExecMessageSync,
                             i++,
                             Flags | ((localTASflags.messageSyncMode == 3) ? MF_CHECKED
                                                                           : MF_UNCHECKED),
                             ID_EXEC_MESSAGES_DESYNC,
                             L"",
                             L"&Unchecked (native messaging)",
                             0);

    // Wait Sync Submenu
    i = 0;
    HelperFuncInsertMenuByID(ExecWaitSync,
                             i++,
                             Flags
                                 | ((localTASflags.waitSyncMode == 0) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_WAITSYNC_SYNCWAIT,
                             L"",
                             L"Synchronous &Wait (infinite timeouts)",
                             0);
    HelperFuncInsertMenuByID(ExecWaitSync,
                             i++,
                             Flags
                                 | ((localTASflags.waitSyncMode == 1) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_WAITSYNC_SYNCSKIP,
                             L"",
                             L"Synchronous &Skip (limited timeouts)",
                             0);
    HelperFuncInsertMenuByID(ExecWaitSync,
                             i++,
                             Flags
                                 | ((localTASflags.waitSyncMode == 2) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_WAITSYNC_ASYNC,
                             L"",
                             L"&Asynchronous (unchecked waits)",
                             0);

    // Dll Loading Submenu
    i = 0;
    HelperFuncInsertMenuByID(
        ExecDlls,
        i++,
        Flags | ((localTASflags.allowLoadInstalledDlls) ? MF_CHECKED : MF_UNCHECKED),
        ID_EXEC_DLLS_INSTALLED,
        L"",
        L"Allow loading any custom/installed DLLs (e.g. Fraps) (can affect sync)",
        0);
    HelperFuncInsertMenuByID(ExecDlls,
                             i++,
                             Flags | ((localTASflags.allowLoadUxtheme) ? MF_CHECKED : MF_UNCHECKED),
                             ID_EXEC_DLLS_UXTHEME,
                             L"",
                             L"Allow loading uxtheme.dll (for non-classic window styles on XP)",
                             0);
    InsertMenu(ExecDlls, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(
        ExecDlls,
        i++,
        Flags | ((runDllLast) ? MF_CHECKED : MF_UNCHECKED),
        ID_EXEC_DLLS_RUNLAST,
        L"",
        L"Allow other DLLs to run first (this is a hack currently required for RotateGear)",
        0);

    // Locale Submenu
    i = 0;
    int curAppLocale = localTASflags.appLocale ? localTASflags.appLocale : tempAppLocale;
    HelperFuncInsertMenuByID(Locale,
                             i++,
                             Flags | ((curAppLocale == 0) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((tempAppLocale && started) ? MF_GRAYED : 0),
                             ID_EXEC_LOCALE_SYSTEM,
                             L"",
                             L"Use system locale",
                             L"movie has forced non-system locale");
    HelperFuncInsertMenuByID(Locale,
                             i++,
                             Flags | ((curAppLocale == 1041) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((!curAppLocale && started) ? MF_GRAYED : 0),
                             ID_EXEC_LOCALE_JAPANESE,
                             L"",
                             L"Force &Japanese locale",
                             L"can't enable while running");
    HelperFuncInsertMenuByID(Locale,
                             i++,
                             Flags | ((curAppLocale == 2052) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((!curAppLocale && started) ? MF_GRAYED : 0),
                             ID_EXEC_LOCALE_CHINESE,
                             L"",
                             L"Force &Chinese (Simplified) locale",
                             L"can't enable while running");
    HelperFuncInsertMenuByID(Locale,
                             i++,
                             Flags | ((curAppLocale == 1042) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((!curAppLocale && started) ? MF_GRAYED : 0),
                             ID_EXEC_LOCALE_KOREAN,
                             L"",
                             L"Force &Korean locale",
                             L"can't enable while running");
    HelperFuncInsertMenuByID(Locale,
                             i++,
                             Flags | ((curAppLocale == 1033) ? MF_CHECKED : MF_UNCHECKED)
                                 | ((!curAppLocale && started) ? MF_GRAYED : 0),
                             ID_EXEC_LOCALE_ENGLISH,
                             L"",
                             L"Force &English locale",
                             L"can't enable while running");

    // Performance Submenu
    i = 0;
    //HelperFuncInsertMenuByID(Performance, i++, Flags | ((traceEnabled)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_DEBUGLOG_TOGGLETRACEENABLE, "", "Load All Symbols and dbghelp.dll", "can't change while running");
    InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Performance,
                             i++,
                             Flags | ((localTASflags.storeVideoMemoryInSavestates) ? MF_CHECKED
                                                                                   : MF_UNCHECKED),
                             ID_PERFORMANCE_TOGGLESAVEVIDMEM,
                             L"",
                             L"Store Video Memory in Savestates",
                             0);
    HelperFuncInsertMenuByID(Performance,
                             i++,
                             Flags | ((storeGuardedPagesInSavestates) ? MF_CHECKED : MF_UNCHECKED),
                             ID_PERFORMANCE_TOGGLESAVEGUARDED,
                             L"",
                             L"Store Guarded Memory Pages in Savestates",
                             0);
    InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Performance,
                             i++,
                             Flags | (!started ? MF_GRAYED : 0),
                             ID_PERFORMANCE_DEALLOCSTATES,
                             L"",
                             L"Discard All Savestates Now",
                             L"must be running");
    HelperFuncInsertMenuByID(Performance,
                             i++,
                             Flags,
                             ID_PERFORMANCE_DELETESTATES,
                             L"",
                             L"Delete All Savestates Now",
                             0);

    // Debug Log Submenu
    i = 0;
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::ANY)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_ANY,
        L"",
        L"Uncategorized",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::HOOK)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_HOOK,
        L"",
        L"Hooking",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::TIME)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_TIME,
        L"",
        L"Time",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::DETTIMER)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_DETTIMER,
        L"",
        L"Det. Timer",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::SYNC)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_SYNC,
        L"",
        L"Sync",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::DDRAW)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_DDRAW,
        L"",
        L"DirectDraw",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::D3D)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_D3D,
        L"",
        L"Direct3D",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::OGL)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_OGL,
        L"",
        L"OpenGL",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::GDI)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_GDI,
        L"",
        L"GDI",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::SDL)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_SDL,
        L"",
        L"SDL",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::DINPUT)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_DINPUT,
        L"",
        L"DirectInput",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::WINPUT)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_WINPUT,
        L"",
        L"Windows Input",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::XINPUT)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_XINPUT,
        L"",
        L"XInput",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::DSOUND)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_DSOUND,
        L"",
        L"DirectSound",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::WSOUND)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_WSOUND,
        L"",
        L"Windows Sound",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::PROCESS)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_PROCESS,
        L"",
        L"Process",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::MODULE)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_MODULE,
        L"",
        L"Module",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::MESSAGES)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_MESSAGES,
        L"",
        L"Messages",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::WINDOW)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_WINDOW,
        L"",
        L"Window",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::FILEIO)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_FILEIO,
        L"",
        L"File",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::REGISTRY)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_REGISTRY,
        L"",
        L"Registry",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::THREAD)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_THREAD,
        L"",
        L"Thread",
        0);
    HelperFuncInsertMenuByID(
        DebugLogging,
        i++,
        Flags | ((localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(
                     LogCategory::TIMERS)])
                     ? MF_CHECKED
                     : MF_UNCHECKED),
        ID_INCLUDE_LCF_TIMERS,
        L"",
        L"Timers",
        0);
    InsertMenuW(DebugLogging, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(DebugLogging, i++, Flags, ID_INCLUDE_LCF_ALL, L"", L"All", 0);
    HelperFuncInsertMenuByID(DebugLogging,
                             i++,
                             Flags,
                             ID_INCLUDE_LCF_NONE,
                             L"",
                             L"None (uncategorized only)",
                             0);

    // Sound Menu

    i = 0;

    HelperFuncInsertMenuByID(Sound,
                             i++,
                             Flags | ((localTASflags.emuMode & EMUMODE_EMULATESOUND) ? MF_CHECKED
                                                                                     : MF_UNCHECKED)
                                 | (started ? MF_GRAYED : 0),
                             ID_SOUND_SOFTWAREMIX,
                             L"",
                             L"&Use Software Mixing",
                             L"can't change while running");
    HelperFuncInsertMenu(Sound,
                         i++,
                         Flags | MF_POPUP
                             | (started || !(localTASflags.emuMode & EMUMODE_EMULATESOUND)
                                    ? MF_GRAYED
                                    : 0),
                         SoundFormat,
                         L"",
                         L"&Format",
                         started ? L"can't change while running"
                                 : L"software mixing must be enabled");
    InsertMenuW(Sound, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Sound,
                             i++,
                             Flags
                                 | ((localTASflags.emuMode & EMUMODE_NOPLAYBUFFERS) ? MF_CHECKED
                                                                                    : MF_UNCHECKED),
                             ID_SOUND_NOPLAYBUFFERS,
                             L"",
                             (localTASflags.emuMode & EMUMODE_EMULATESOUND)
                                 ? ((localTASflags.aviMode & 2) ? L"&Mute Sound"
                                                                : L"&Mute Sound (Skip Mixing)")
                                 : L"&Mute Sound (Skip Playing)",
                             0);
    HelperFuncInsertMenuByID(Sound,
                             i++,
                             Flags | ((localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED),
                             ID_SOUND_VIRTUALDIRECTSOUND,
                             L"",
                             /*(emuMode&EMUMODE_EMULATESOUND) ? "&Virtual DirectSound" :*/
                             L"&Disable DirectSound Creation",
                             0);

    // Sound Format Menu
    i = 0;
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 8000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_8000,
                L"8000 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 11025 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_11025,
                L"&11025 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 12000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_12000,
                L"12000 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 16000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_16000,
                L"16000 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 22050 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_22050,
                L"&22050 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 24000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_24000,
                L"24000 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 32000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_32000,
                L"32000 Hz");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 44100 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_44100,
                L"&44100 Hz (default)");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioFrequency == 48000 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_RATE_48000,
                L"48000 Hz");
    InsertMenuW(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioBitsPerSecond == 8 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_BITS_8,
                L"8 bit");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioBitsPerSecond == 16 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_BITS_16,
                L"16 bit (default)");
    InsertMenuW(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioChannels == 1 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_CHANNELS_1,
                L"&Mono");
    InsertMenuW(SoundFormat,
                i++,
                Flags | (localTASflags.audioChannels == 2 ? MF_CHECKED : MF_UNCHECKED),
                ID_SOUND_CHANNELS_2,
                L"&Stereo (default)");

    // TAS Tools Menu
    HelperFuncInsertMenuByID(TAS_Tools, i++, Flags, ID_RAM_WATCH, L"", L"RAM &Watch", 0);

    bool ramSearchAvailable = true;
    LPCWSTR ramSearchString = L"&RAM Search (not done)";
#if defined(_MSC_VER) && (_MSC_VER <= 1310)
    ramSearchAvailable = false;
#endif
    HelperFuncInsertMenuByID(TAS_Tools,
                             i++,
                             Flags | (ramSearchAvailable ? MF_ENABLED : MF_DISABLED | MF_GRAYED),
                             ID_RAM_SEARCH,
                             L"",
                             ramSearchString,
                             L"compiler too old");

#if 0
	i = 0;
	HelperFuncInsertMenuByID(Lua_Script,i++,Flags,IDC_NEW_LUA_SCRIPT,"New Lua Script Window...","","&New Lua Script Window...");
	HelperFuncInsertMenuByID(Lua_Script,i++,Flags | (!LuaScriptHWnds.empty() ? MF_ENABLED : MF_DISABLED|MF_GRAYED),IDC_CLOSE_LUA_SCRIPTS,"Close All Lua Windows","","&Close All Lua Windows");
	if(!LuaScriptHWnds.empty())
	{
		InsertMenu(Lua_Script, i++, MF_SEPARATOR, NULL, NULL);
		for(unsigned int j=0; j<LuaScriptHWnds.size(); j++)
		{
			GetWindowText(LuaScriptHWnds[j], Str_Tmp, 1024);
			HelperFuncInsertMenuByID(Lua_Script,i++,Flags,IDC_LUA_SCRIPT_0+j,Str_Tmp,"",Str_Tmp);
		}
	}

	{
		int dividerI = i;
		for(unsigned int j=0; j<MAX_RECENT_SCRIPTS; j++)
		{
			const char* pathPtr = Recent_Scripts[j];
			if(!*pathPtr)
				continue;

			HWND IsScriptFileOpen(const char* Path);
			if(IsScriptFileOpen(pathPtr))
				continue;

			// only show some of the path
			const char* pathPtrSearch;
			int slashesLeft = 2;
			for(pathPtrSearch = pathPtr + strlen(pathPtr) - 1; 
				pathPtrSearch != pathPtr && slashesLeft >= 0;
				pathPtrSearch--)
			{
				char c = *pathPtrSearch;
				if(c == '\\' || c == '/')
					slashesLeft--;
			}
			if(slashesLeft < 0)
				pathPtr = pathPtrSearch + 2;
			strcpy(Str_Tmp, pathPtr);

			if(i == dividerI)
				InsertMenu(Lua_Script, i++, MF_SEPARATOR, NULL, NULL);

			HelperFuncInsertMenuByID(Lua_Script,i++,Flags,ID_LUA_OPENRECENTSCRIPT0+j,Str_Tmp,"",Str_Tmp);
		}
	}
#endif

// AVI Menu
#if 0
	InsertMenu(Avi, i++, MF_SEPARATOR, NULL, NULL);
#endif
    HelperFuncInsertMenuByID(Avi,
                             i++,
                             Flags | ((localTASflags.aviMode & (1 | 2)) == (1 | 2) ? MF_CHECKED
                                                                                   : MF_UNCHECKED)
                                 | ((started && !(localTASflags.emuMode & EMUMODE_EMULATESOUND))
                                        ? MF_GRAYED
                                        : 0),
                             ID_AVI_BOTH,
                             L"",
                             L"Capture Video and Audio",
                             L"software sound mixing must be enabled");
    HelperFuncInsertMenuByID(Avi,
                             i++,
                             Flags | ((localTASflags.aviMode & (1 | 2)) == (1) ? MF_CHECKED
                                                                               : MF_UNCHECKED),
                             ID_AVI_VIDEO,
                             L"",
                             L"Capture Video Only",
                             0);
    HelperFuncInsertMenuByID(Avi,
                             i++,
                             Flags | ((localTASflags.aviMode & (1 | 2)) == (2) ? MF_CHECKED
                                                                               : MF_UNCHECKED)
                                 | ((started && !(localTASflags.emuMode & EMUMODE_EMULATESOUND))
                                        ? MF_GRAYED
                                        : 0),
                             ID_AVI_AUDIO,
                             L"",
                             L"Capture Audio Only",
                             L"software sound mixing must be enabled");
    HelperFuncInsertMenuByID(Avi,
                             i++,
                             Flags | ((localTASflags.aviMode & (1 | 2)) == (0) ? MF_CHECKED
                                                                               : MF_UNCHECKED),
                             ID_AVI_NONE,
                             L"",
                             localTASflags.aviMode ? L"Stop / Disable" : L"AVI Capture Disabled",
                             0);
    InsertMenu(Avi, i++, MF_SEPARATOR, NULL, NULL);
    HelperFuncInsertMenuByID(Avi,
                             i++,
                             Flags | ((!localTASflags.aviMode || !started
                                       || (!aviFrameCount && !aviSoundFrameCount))
                                          ? MF_GRAYED
                                          : 0),
                             ID_AVI_SPLITNOW,
                             L"",
                             L"Split AVI now",
                             L"must be capturing");
#if 0
	HelperFuncInsertMenuByID(Avi,i++,Flags,ID_AVI_SETSPLIT,"","Split AVI after...", 0);
#endif

#if 0
	// Help Menu

	i = 0;

	HelperFuncInsertMenuByID(Help, i++, Flags, ID_HELP_ABOUT, "About" ,"", "&About");
	InsertMenu(Help, i++, MF_SEPARATOR, NULL, NULL);
#endif

    HWND tlhwnd = GetDlgItem(hWnd, IDC_TOPLEFTCONTROL);
    RECT oldrect = {}, newrect = {};
    GetWindowRect(tlhwnd, &oldrect);

    SetMenu(hWnd, MainMenu);

    // if setting the menu pushed the window contents down,
    // extend the bottom of the window by the same amount.
    // (less painful than trying to directly measure the menubar height or width.)
    GetWindowRect(tlhwnd, &newrect);
    int change = newrect.top - oldrect.top;
    if (change)
    {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        SetWindowPos(hWnd,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     change + rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
}
