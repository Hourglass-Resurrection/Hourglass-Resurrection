#include <stdio.h>
#include "MainMenu.h"
#include "Config.h"
#include "resource.h"
#include "shared/ipc.h"
using namespace Config;

void HelperFuncInsertMenuByID(HMENU smenu, UINT pos, UINT flags, UINT_PTR id, LPCSTR suffix, LPCSTR str, LPCSTR disableReason)
{
	char Str_Tmp[1024];
#if 0
	GetPrivateProfileStringA(language_name[Language], str, def, Str_Tmp, 1024, Language_Path);
#endif
	if (disableReason && (flags & (MF_DISABLED | MF_GRAYED)))
		sprintf(Str_Tmp, "%s (%s)", str, disableReason);
	else
		strcpy(Str_Tmp, str);
	strcat(Str_Tmp, suffix);
#if 0
	AddHotkeySuffix(Str_Tmp, id, suffix, true);
#endif
	InsertMenu(smenu, pos, flags, id, Str_Tmp);
}

__forceinline void HelperFuncInsertMenu(HMENU smenu, UINT pos, UINT flags, HMENU menu, LPCSTR suffix, LPCSTR str, LPCSTR disableReason)
{
	HelperFuncInsertMenuByID(smenu, pos, flags, reinterpret_cast<UINT_PTR>(menu), suffix, str, disableReason);
}

void Build_Main_Menu(HMENU& MainMenu, HWND hWnd)
{
	unsigned int Flags;
	int i = 0, j = 0;
	char str[1024];
	char Str_Tmp[1024] = { 0 };

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
	HMENU DebugLoggingInclude = CreatePopupMenu();
	HMENU DebugLoggingTrace = CreatePopupMenu();
	HMENU DebugLoggingExclude = CreatePopupMenu();
	HMENU Performance = CreatePopupMenu();
#if 0
	HMENU Tools_Movies = CreatePopupMenu();
	HMENU Tools_AVI = CreatePopupMenu();
	HMENU Lua_Script = CreatePopupMenu();
#endif
	HMENU InputHotkeyFocus = CreatePopupMenu();
	HMENU InputInputFocus = CreatePopupMenu();



	const char* exefilenameonly = std::max(strrchr(exefilename, '\\'), strrchr(exefilename, '/'));
	if (exefilenameonly) ++exefilenameonly;
	if (!exeFileExists) exefilenameonly = NULL;
	const char* moviefilenameonly = std::max(strrchr(moviefilename, '\\'), strrchr(moviefilename, '/'));
	if (moviefilenameonly) ++moviefilenameonly;
	if (!movieFileExists) moviefilenameonly = NULL;
	bool on = true;

	Flags = MF_BYPOSITION | MF_POPUP | MF_STRING;

	i = 0;
	HelperFuncInsertMenu(MainMenu, i++, Flags, Files, "", "&File", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, Graphics, "", "&Graphics", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, Sound, "", "&Sound", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, Exec, "", "&Runtime", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, Input, "", "&Input", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, TAS_Tools, "", "&Tools", 0);
	HelperFuncInsertMenu(MainMenu, i++, Flags, Avi, "", (localTASflags.aviMode&&started && (aviFrameCount || aviSoundFrameCount)) ? ((aviFrameCount&&aviSoundFrameCount) ? "&AVI!!" : "&AVI!") : "&AVI", 0);
#if 0
	HelperFuncInsertMenuByID(MainMenu, i++, Flags, Help, "", "&Help", 0);
#endif

	// File Menu
	Flags = MF_BYPOSITION | MF_STRING;
	i = 0;

	on = exefilenameonly && !started;
	sprintf(str, "&Open Executable... %s%s%s", on ? "(now open: \"" : "", on ? exefilenameonly : "", on ? "\")" : "");
	HelperFuncInsertMenuByID(Files, i++, Flags | (started ? MF_GRAYED : 0), ID_FILES_OPENEXE, "\tCtrl+O", str, "can't change while running");
	on = moviefilenameonly && !started;
	sprintf(str, "&Open Movie... %s%s%s", on ? "(now open: \"" : "", on ? moviefilenameonly : "", on ? "\")" : "");
	HelperFuncInsertMenuByID(Files, i++, Flags | (started ? MF_GRAYED : 0), ID_FILES_OPENMOV, "\tCtrl+M", str, "can't change while running");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_PLAYMOV, "\tCtrl+P", "&Play Movie...", 0);
	HelperFuncInsertMenuByID(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_WATCHMOV, "", "&Watch From Beginning", "must be running");
#endif
	sprintf(str, started ? "&Watch From Beginning" : "&Play Movie %s%s%s", moviefilenameonly ? "\"" : "", moviefilenameonly ? moviefilenameonly : "", moviefilenameonly ? "\"" : "");
	HelperFuncInsertMenuByID(Files, i++, Flags | (!(movieFileExists&&exeFileExists) ? MF_GRAYED : 0), started ? ID_FILES_WATCHMOV : ID_FILES_PLAYMOV, "\tCtrl+P", str, movieFileExists ? "must open an executable first" : "must open a movie first");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

	HelperFuncInsertMenuByID(Files, i++, Flags | ((started || !exeFileExists) ? MF_GRAYED : 0), ID_FILES_RECORDMOV, "\tCtrl+R", "&Record New Movie...", started ? "must stop running first" : "must open an executable first");
	HelperFuncInsertMenuByID(Files, i++, Flags | ((!started || localTASflags.playback) ? MF_GRAYED : 0), ID_FILES_BACKUPMOVIE, "", "Backup Movie to File...", "Must be recording");
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_RESUMEMOVAS, "", (playback||!started) ? "Resume Recording to &Different File..." : "Continue Recording to &Different File...", "must be running");
#endif
	HelperFuncInsertMenuByID(Files, i++, Flags | ((!started || !localTASflags.playback || finished) ? MF_GRAYED : 0), ID_FILES_RESUMERECORDING, "", "Resume Recording from Now", "movie must be playing");
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags | ((!started||finished)?MF_GRAYED:0), ID_FILES_SPLICE, "", "Splice...", "movie must be playing or recording");
#endif
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
#if 0
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_SAVECONFIG, "", "Save Config", 0);
#endif
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_SAVECONFIGAS, "", "Save Config As...", 0);
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_LOADCONFIGFROM, "", "Load Config From...", 0);

#if 0
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	if (strcmp(Recent_Rom[0], ""))
	{
		HelperFuncInsertMenuByID(Files, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, FilesHistory, "", "&Recent EXEs", 0);
		InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	}
#endif

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Files, i++, Flags | (!started?MF_GRAYED:0), ID_FILES_STOP_RELAY, "", "Stop Running", "already stopped");
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Files, i++, Flags, ID_FILES_QUIT, "\tAlt+F4", "E&xit", 0);

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
	HelperFuncInsertMenuByID(Graphics, i++, Flags | (!localTASflags.forceWindowed ? MF_CHECKED : MF_UNCHECKED) | (started ? MF_GRAYED : 0), ID_GRAPHICS_ALLOWFULLSCREEN, "", "Allow &Fullscreen / Display Mode Changes", "can't change while running");
	InsertMenu(Graphics, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Graphics, i++, Flags | (!localTASflags.forceSoftware ? MF_CHECKED : MF_UNCHECKED) | (started ? MF_GRAYED : 0), ID_GRAPHICS_FORCESOFTWARE, "", "&Allow Hardware Acceleration", "can't change while running");
	HelperFuncInsertMenu(Graphics, i++, Flags | MF_POPUP | ((localTASflags.forceSoftware||started) ? MF_GRAYED : 0), GraphicsMemory, "", "Surface &Memory", started ? "can't change while running" : "hardware acceleration must be enabled");

	// GraphicsMemory Menu
	i = 0;
	HelperFuncInsertMenuByID(GraphicsMemory, i++, Flags | ((localTASflags.forceSurfaceMemory == 0) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_DONTCARE, "", "&Let Game Choose", 0);
	HelperFuncInsertMenuByID(GraphicsMemory, i++, Flags | ((localTASflags.forceSurfaceMemory == 1) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_SYSTEM, "", "&System Memory               (Slow Draw, Fast Read)", 0);
	HelperFuncInsertMenuByID(GraphicsMemory, i++, Flags | ((localTASflags.forceSurfaceMemory == 2) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_NONLOCAL, "", "&Non-Local Video Memory (Varies)", 0);
	HelperFuncInsertMenuByID(GraphicsMemory, i++, Flags | ((localTASflags.forceSurfaceMemory == 3) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_MEM_LOCAL, "", "Local &Video Memory        (Fast Draw, Slow Read)", 0);


	// Execution Menu
	i = 0;
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Locale, "", "App &Locale", 0);
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Affinity, "", "&Affinity", 0);
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecMultithreading, "", "&Multithreading Mode", 0);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecTimers, "", "Multimedia &Timer Mode", 0);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecMessageSync, "", "Message &Sync Mode", 0);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecWaitSync, "", "&Wait Sync Mode", 0);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, ExecDlls, "", "&DLL Loading", 0);
	HelperFuncInsertMenuByID(Exec, i++, Flags | ((truePause)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_USETRUEPAUSE, "", "Disable Pause Helper", 0);
	HelperFuncInsertMenuByID(Exec, i++, Flags | ((onlyHookChildProcesses)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_EXEC_ONLYHOOKCHILDPROC, "", "Wait until sub-process creation" /*" (might help IWBTG)"*/, "can't change while running");
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, Performance, "", "&Performance", 0);
	HelperFuncInsertMenu(Exec, i++, Flags | MF_POPUP, DebugLogging, "", "&Debug Logging", 0);
#if 0
	InsertMenu(Exec, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_SUSPEND, "", "Suspend Secondary Threads", "must be running");
	HelperFuncInsertMenuByID(Exec, i++, Flags | (!started?MF_GRAYED:0), ID_EXEC_RESUME, "", "Resume Secondary Threads", "must be running");
	HelperFuncInsertMenuByID(Exec, i++, Flags | ((truePause)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_USETRUEPAUSE, "", "Don't handle any messages while paused", 0);
#endif


	// Time Menu
	i = 0;
	HelperFuncInsertMenuByID(Time, i++, Flags | (paused?MF_CHECKED:MF_UNCHECKED), ID_TIME_TOGGLE_PAUSE, "", "Pause", 0);
	InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenu(Time, i++, Flags | MF_POPUP, TimeRate, "", "&Slow Motion", 0);
	InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Time, i++, Flags | (fastforward?MF_CHECKED:MF_UNCHECKED), ID_TIME_TOGGLE_FASTFORWARD, "", "Fast-Forward", 0);
	HelperFuncInsertMenu(Time, i++, Flags | MF_POPUP, TimeFastForward, "", "&Fast-Forward Options", 0);
	if(Time == TAS_Tools) InsertMenu(Time, i++, MF_SEPARATOR, NULL, NULL);


	// Input Menu
	i = 0;
#if 0
	HelperFuncInsertMenuByID(Input, i++, Flags, ID_INPUT_HOTKEYS, "", "Configure &Hotkeys...", 0);	
#endif
	HelperFuncInsertMenuByID(Input, i++, Flags, ID_INPUT_INPUTS, "", "Configure &Inputs...", 0);	
	HelperFuncInsertMenu(Input, i++, Flags | MF_POPUP, InputHotkeyFocus, "", "Enable Hotkeys When", 0);
	InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenu(Input, i++, Flags | MF_POPUP, InputInputFocus, "", "Enable Game Input When", 0);
	InsertMenu(Input, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Input, i++, Flags | (advancePastNonVideoFrames?MF_CHECKED:MF_UNCHECKED), ID_INPUT_SKIPLAGFRAMES, "", "Frame Advance Skips \"&Lag\" Frames", 0);

	i = 0;
	HelperFuncInsertMenuByID(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_TASER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_TASER, "", "Hourglass has Focus", 0);
	HelperFuncInsertMenuByID(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_TASEE)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_TASEE, "", "Game Window has Focus", 0);
	HelperFuncInsertMenuByID(InputHotkeyFocus, i++, Flags | ((hotkeysFocusFlags&FOCUS_FLAG_OTHER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDHOTKEY_OTHER, "", "Other Programs have Focus", 0);

	i = 0;
	HelperFuncInsertMenuByID(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_TASER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_TASER, "", "Hourglass has Focus", 0);
	HelperFuncInsertMenuByID(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_TASEE)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_TASEE, "", "Game Window has Focus", 0);
	HelperFuncInsertMenuByID(InputInputFocus, i++, Flags | ((inputFocusFlags&FOCUS_FLAG_OTHER)?MF_CHECKED:MF_UNCHECKED), ID_INPUT_BACKGROUNDINPUTS_OTHER, "", "Other Programs have Focus", 0);


	// Fast-Forward Submenu
	i = 0;
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((localTASflags.fastForwardFlags&FFMODE_FRONTSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_FRONTSKIP, "", "Frontbuffer Frameskip", 0);
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((localTASflags.fastForwardFlags&FFMODE_BACKSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_BACKSKIP, "", "Backbuffer Frameskip", 0);
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((recoveringStale||(localTASflags.fastForwardFlags&FFMODE_SOUNDSKIP))?MF_CHECKED:MF_UNCHECKED) | (recoveringStale ? MF_GRAYED : 0), ID_TIME_FF_SOUNDSKIP, "", "Soundskip", "always on while recovering stale");
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((localTASflags.fastForwardFlags&FFMODE_RAMSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_RAMSKIP, "", "RAM Search/Watch Skip", 0);
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((localTASflags.fastForwardFlags&FFMODE_SLEEPSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_SLEEPSKIP, "", "Sleep Skip", 0);
	HelperFuncInsertMenuByID(TimeFastForward, i++, Flags | ((localTASflags.fastForwardFlags&FFMODE_WAITSKIP)?MF_CHECKED:MF_UNCHECKED), ID_TIME_FF_WAITSKIP, "", "Wait Skip", 0);

	// Time Rate Submenu
	i = 0;
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale==localTASflags.timescaleDivisor)?MF_GRAYED:0), ID_TIME_RATE_FASTER, "", "Speed &Up", "already at 100%");
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale*8==localTASflags.timescaleDivisor)?MF_GRAYED:0), ID_TIME_RATE_SLOWER, "", "Slow &Down", "already at slowest option");
	InsertMenu(TimeRate, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale==localTASflags.timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_100, "", "&100% (normal speed)", 0);
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale*4==localTASflags.timescaleDivisor*3)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_75, "", "75%", 0);
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale*2==localTASflags.timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_50, "", "50%", 0);
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale*4==localTASflags.timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_25, "", "25%", 0);
	HelperFuncInsertMenuByID(TimeRate, i++, Flags | ((localTASflags.timescale*8==localTASflags.timescaleDivisor)?MF_CHECKED:MF_UNCHECKED), ID_TIME_RATE_12, "", "12%", 0);

	// Affinity Submenu
	i = 0;
	HelperFuncInsertMenuByID(Affinity, i++, Flags | ((limitGameToOnePhysicalCore)?MF_CHECKED:MF_UNCHECKED) | ((started)?MF_GRAYED:0), ID_EXEC_LIMITONEPHYSICALCORE, "", "Use only one physical core (Might help older games to run correctly)", "can't change after starting");
	HelperFuncInsertMenuByID(Affinity, i++, Flags | ((disableHyperThreading)?MF_CHECKED:MF_UNCHECKED) | ((started)?MF_GRAYED:0), ID_EXEC_DISABLEHYPERTHREADS, "", "Disable Hyper-Threading", "can't change after starting");

	// Multithreading Submenu
	i = 0;
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_THREADS_DISABLE, "", "&Disable (prevent thread creation)", 0);
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 1)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode>=2?MF_GRAYED:0), ID_EXEC_THREADS_WRAP, "", "&Wrap (recycle threads)", "can't set while running after normal threads created");
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 2)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_ALLOW, "", "&Allow (normal thread creation)", "can't set while running after wrapped threads created");
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 3)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_KNOWN, "", "Allow &known threads only", "can't set while running after wrapped threads created");
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 4)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_UNKNOWN, "", "Allow &unknown threads only", "can't set while running after wrapped threads created");
	HelperFuncInsertMenuByID(ExecMultithreading, i++, Flags | ((localTASflags.threadMode == 5)?MF_CHECKED:MF_UNCHECKED) | (usedThreadMode==1?MF_GRAYED:0), ID_EXEC_THREADS_TRUSTED, "", "Allow &trusted threads only", "can't set while running after wrapped threads created");
	

	// Timers Submenu
	i = 0;
	HelperFuncInsertMenuByID(ExecTimers, i++, Flags | ((localTASflags.timersMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_DISABLE, "", "&Disable (prevent timer creation)", 0);
	HelperFuncInsertMenuByID(ExecTimers, i++, Flags | ((localTASflags.timersMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_SYNC, "", "&Synchronous (run timers at frame boundaries)", 0);
	HelperFuncInsertMenuByID(ExecTimers, i++, Flags | ((localTASflags.timersMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_TIMERS_ASYNC, "", "&Asynchronous (run timers in separate threads)", 0);

	// Message Sync Submenu
	i = 0;
	HelperFuncInsertMenuByID(ExecMessageSync, i++, Flags | ((localTASflags.messageSyncMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_SYNC, "", "&Synchronous (no timeouts)", 0);
	HelperFuncInsertMenuByID(ExecMessageSync, i++, Flags | ((localTASflags.messageSyncMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_SEMISYNC, "", "Semi-S&ynchronous (some timeouts)", 0);
	HelperFuncInsertMenuByID(ExecMessageSync, i++, Flags | ((localTASflags.messageSyncMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_ASYNC, "", "&Asynchronous (any timeouts)", 0);
	HelperFuncInsertMenuByID(ExecMessageSync, i++, Flags | ((localTASflags.messageSyncMode == 3)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_MESSAGES_DESYNC, "", "&Unchecked (native messaging)", 0);

	// Wait Sync Submenu
	i = 0;
	HelperFuncInsertMenuByID(ExecWaitSync, i++, Flags | ((localTASflags.waitSyncMode == 0)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_SYNCWAIT, "", "Synchronous &Wait (infinite timeouts)", 0);
	HelperFuncInsertMenuByID(ExecWaitSync, i++, Flags | ((localTASflags.waitSyncMode == 1)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_SYNCSKIP, "", "Synchronous &Skip (limited timeouts)", 0);
	HelperFuncInsertMenuByID(ExecWaitSync, i++, Flags | ((localTASflags.waitSyncMode == 2)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_WAITSYNC_ASYNC, "", "&Asynchronous (unchecked waits)", 0);

	// Dll Loading Submenu	
	i = 0;
	HelperFuncInsertMenuByID(ExecDlls, i++, Flags | ((localTASflags.allowLoadInstalledDlls)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_INSTALLED, "", "Allow loading any custom/installed DLLs (e.g. Fraps) (can affect sync)", 0);
	HelperFuncInsertMenuByID(ExecDlls, i++, Flags | ((localTASflags.allowLoadUxtheme)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_UXTHEME, "", "Allow loading uxtheme.dll (for non-classic window styles on XP)", 0);
	InsertMenu(ExecDlls, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(ExecDlls, i++, Flags | ((runDllLast)?MF_CHECKED:MF_UNCHECKED), ID_EXEC_DLLS_RUNLAST, "", "Allow other DLLs to run first (this is a hack currently required for RotateGear)", 0);

	// Locale Submenu	
	i = 0;
	int curAppLocale = localTASflags.appLocale ? localTASflags.appLocale : tempAppLocale;
	HelperFuncInsertMenuByID(Locale, i++, Flags | ((curAppLocale==0)?MF_CHECKED:MF_UNCHECKED) | ((tempAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_SYSTEM, "", "Use system locale", "movie has forced non-system locale");
	HelperFuncInsertMenuByID(Locale, i++, Flags | ((curAppLocale==1041)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_JAPANESE, "", "Force &Japanese locale", "can't enable while running");	
	HelperFuncInsertMenuByID(Locale, i++, Flags | ((curAppLocale==2052)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_CHINESE, "", "Force &Chinese (Simplified) locale", "can't enable while running");	
	HelperFuncInsertMenuByID(Locale, i++, Flags | ((curAppLocale==1042)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_KOREAN, "", "Force &Korean locale", "can't enable while running");	
	HelperFuncInsertMenuByID(Locale, i++, Flags | ((curAppLocale==1033)?MF_CHECKED:MF_UNCHECKED) | ((!curAppLocale&&started)?MF_GRAYED:0), ID_EXEC_LOCALE_ENGLISH, "", "Force &English locale", "can't enable while running");

	// Performance Submenu
	i = 0;
	HelperFuncInsertMenuByID(Performance, i++, Flags | ((traceEnabled)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_DEBUGLOG_TOGGLETRACEENABLE, "", "Load All Symbols and dbghelp.dll", "can't change while running");
	HelperFuncInsertMenuByID(Performance, i++, Flags | ((crcVerifyEnabled)?MF_CHECKED:MF_UNCHECKED) | (started?MF_GRAYED:0), ID_DEBUGLOG_TOGGLECRCVERIFY, "", "Check CRCs on movie playback", "can't change while running");
	InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Performance, i++, Flags | ((localTASflags.storeVideoMemoryInSavestates)?MF_CHECKED:MF_UNCHECKED), ID_PERFORMANCE_TOGGLESAVEVIDMEM, "", "Store Video Memory in Savestates", 0);
	HelperFuncInsertMenuByID(Performance, i++, Flags | ((storeGuardedPagesInSavestates)?MF_CHECKED:MF_UNCHECKED), ID_PERFORMANCE_TOGGLESAVEGUARDED, "", "Store Guarded Memory Pages in Savestates", 0);
	InsertMenu(Performance, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Performance, i++, Flags | (!started?MF_GRAYED:0), ID_PERFORMANCE_DEALLOCSTATES, "", "Discard All Savestates Now", "must be running");
	HelperFuncInsertMenuByID(Performance, i++, Flags, ID_PERFORMANCE_DELETESTATES, "", "Delete All Savestates Now", 0);


	// Debug Log Submenu
	i = 0;
	//InsertMenu(DebugLogging, i++, MF_SEPARATOR, NULL, NULL);
	//HelperFuncInsertMenu(DebugLogging, i++, Flags | MF_POPUP, DebugLoggingInclude, "", "&Print Categories", 0);
	//HelperFuncInsertMenu(DebugLogging, i++, Flags | MF_POPUP, DebugLoggingTrace, "", "&Trace Categories", 0);
	//HelperFuncInsertMenu(DebugLogging, i++, Flags | MF_POPUP, DebugLoggingExclude, "", "&Exclude Categories", 0);

	//i = 0;
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_ERROR, "", "error", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_UNTESTED, "", "untested", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TODO, "", "todo", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DESYNC, "", "desync", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_FRAME, "", "frame", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_HOOK, "", "hook", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMEFUNC, "", "timefunc", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMESET, "", "timeset", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMEGET, "", "timeget", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SYNCOBJ, "", "syncobj", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WAIT, "", "wait", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SLEEP, "", "sleep", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DDRAW, "", "directdraw", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_D3D, "", "direct3d", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_OGL, "", "opengl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_GDI, "", "gdi", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_SDL, "", "sdl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DINPUT, "", "directinput", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_KEYBOARD, "", "keyboard", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MOUSE, "", "mouse", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_JOYPAD, "", "joypad", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_DSOUND, "", "directsound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WSOUND, "", "othersound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_PROCESS, "", "process", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MODULE, "", "module", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_MESSAGES, "", "messages", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_WINDOW, "", "window", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_FILEIO, "", "file", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_REGISTRY, "", "registry", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_THREAD, "", "thread", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_TIMERS, "", "timers", 0);
	//InsertMenu(DebugLoggingInclude, i++, MF_SEPARATOR, NULL, NULL);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_ALL, "", "all", 0);
	//HelperFuncInsertMenuByID(DebugLoggingInclude, i++, Flags | ((includeLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_INCLUDE_LCF_NONE, "", "none (uncategorized only)", 0);

	//i = 0;
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_ERROR, "", "error", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_UNTESTED, "", "untested", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TODO, "", "todo", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DESYNC, "", "desync", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_FRAME, "", "frame", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_HOOK, "", "hook", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMEFUNC, "", "timefunc", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMESET, "", "timeset", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMEGET, "", "timeget", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SYNCOBJ, "", "syncobj", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WAIT, "", "wait", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SLEEP, "", "sleep", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DDRAW, "", "directdraw", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_D3D, "", "direct3d", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_OGL, "", "opengl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_GDI, "", "gdi", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_SDL, "", "sdl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DINPUT, "", "directinput", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_KEYBOARD, "", "keyboard", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MOUSE, "", "mouse", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_JOYPAD, "", "joypad", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_DSOUND, "", "directsound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WSOUND, "", "othersound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_PROCESS, "", "process", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MODULE, "", "module", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_MESSAGES, "", "messages", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_WINDOW, "", "window", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_FILEIO, "", "file", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_REGISTRY, "", "registry", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_THREAD, "", "thread", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_TIMERS, "", "timers", 0);
	//InsertMenu(DebugLoggingTrace, i++, MF_SEPARATOR, NULL, NULL);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_ALL, "", "all (categorized only)", 0);
	//HelperFuncInsertMenuByID(DebugLoggingTrace, i++, Flags | ((traceLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_TRACE_LCF_NONE, "", "none", 0);

	//i = 0;
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FREQUENT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FREQUENT, "", "frequent", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_ERROR)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_ERROR, "", "error", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_UNTESTED)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_UNTESTED, "", "untested", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TODO)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TODO, "", "todo", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DESYNC)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DESYNC, "", "desync", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FRAME)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FRAME, "", "frame", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_HOOK)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_HOOK, "", "hook", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMEFUNC)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMEFUNC, "", "timefunc", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMESET)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMESET, "", "timeset", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMEGET)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMEGET, "", "timeget", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SYNCOBJ)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SYNCOBJ, "", "syncobj", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WAIT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WAIT, "", "wait", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SLEEP)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SLEEP, "", "sleep", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DDRAW)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DDRAW, "", "directdraw", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_D3D)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_D3D, "", "direct3d", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_OGL)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_OGL, "", "opengl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_GDI)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_GDI, "", "gdi", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_SDL)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_SDL, "", "sdl", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DINPUT)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DINPUT, "", "directinput", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_KEYBOARD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_KEYBOARD, "", "keyboard", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MOUSE)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MOUSE, "", "mouse", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_JOYPAD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_JOYPAD, "", "joypad", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_DSOUND)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_DSOUND, "", "directsound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WSOUND)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WSOUND, "", "othersound", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_PROCESS)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_PROCESS, "", "process", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MODULE)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MODULE, "", "module", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_MESSAGES)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_MESSAGES, "", "messages", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_WINDOW)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_WINDOW, "", "window", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_FILEIO)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_FILEIO, "", "file", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_REGISTRY)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_REGISTRY, "", "registry", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_THREAD)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_THREAD, "", "thread", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags&LCF_TIMERS)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_TIMERS, "", "timers", 0);
	//InsertMenu(DebugLoggingExclude, i++, MF_SEPARATOR, NULL, NULL);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags==~0)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_ALL, "", "all (categorized only)", 0);
	//HelperFuncInsertMenuByID(DebugLoggingExclude, i++, Flags | ((excludeLogFlags==0)?MF_CHECKED:MF_UNCHECKED), ID_EXCLUDE_LCF_NONE, "", "none", 0);


	// Sound Menu

	i = 0;

	HelperFuncInsertMenuByID(Sound, i++, Flags | ((localTASflags.emuMode & EMUMODE_EMULATESOUND) ? MF_CHECKED : MF_UNCHECKED) | (started?MF_GRAYED:0), ID_SOUND_SOFTWAREMIX, "", "&Use Software Mixing", "can't change while running");
	HelperFuncInsertMenu(Sound, i++, Flags | MF_POPUP | (started||!(localTASflags.emuMode&EMUMODE_EMULATESOUND) ? MF_GRAYED : 0), SoundFormat, "", "&Format", started ? "can't change while running" : "software mixing must be enabled");
	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Sound, i++, Flags | ((localTASflags.emuMode & EMUMODE_NOPLAYBUFFERS) ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_NOPLAYBUFFERS, "", (localTASflags.emuMode&EMUMODE_EMULATESOUND) ? ((localTASflags.aviMode&2) ? "&Mute Sound" : "&Mute Sound (Skip Mixing)") : "&Mute Sound (Skip Playing)", 0);
	HelperFuncInsertMenuByID(Sound, i++, Flags | ((localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND) ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_VIRTUALDIRECTSOUND, "", /*(emuMode&EMUMODE_EMULATESOUND) ? "&Virtual DirectSound" :*/ "&Disable DirectSound Creation", 0);



	// Sound Format Menu
	i = 0;
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 8000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_8000, "8000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 11025 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_11025, "&11025 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 12000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_12000, "12000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 16000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_16000, "16000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 22050 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_22050, "&22050 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 24000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_24000, "24000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 32000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_32000, "32000 Hz");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 44100 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_44100, "&44100 Hz (default)");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioFrequency == 48000 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_48000, "48000 Hz");
	InsertMenu(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioBitsPerSecond == 8 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_BITS_8, "8 bit");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioBitsPerSecond == 16 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_BITS_16, "16 bit (default)");
	InsertMenu(SoundFormat, i++, MF_SEPARATOR, NULL, NULL);
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioChannels == 1 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_CHANNELS_1, "&Mono");
	InsertMenu(SoundFormat, i++, Flags | (localTASflags.audioChannels == 2 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_CHANNELS_2, "&Stereo (default)");

	// TAS Tools Menu
	HelperFuncInsertMenuByID(TAS_Tools,i++,Flags,ID_RAM_WATCH,"","RAM &Watch", 0);

	bool ramSearchAvailable = true;
	const char* ramSearchString = "&RAM Search (not done)";
#if defined(_MSC_VER) && (_MSC_VER <= 1310)
	ramSearchAvailable = false;
#endif
	HelperFuncInsertMenuByID(TAS_Tools,i++,Flags|(ramSearchAvailable ? MF_ENABLED : MF_DISABLED | MF_GRAYED),ID_RAM_SEARCH,"",ramSearchString,"compiler too old");

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
	HelperFuncInsertMenuByID(Avi,i++,Flags | ((localTASflags.aviMode&(1|2))==(1|2)?MF_CHECKED:MF_UNCHECKED) | ((started&&!(localTASflags.emuMode&EMUMODE_EMULATESOUND))?MF_GRAYED:0),ID_AVI_BOTH,"","Capture Video and Audio", "software sound mixing must be enabled");
	HelperFuncInsertMenuByID(Avi,i++,Flags | ((localTASflags.aviMode&(1|2))==(1  )?MF_CHECKED:MF_UNCHECKED),ID_AVI_VIDEO,"","Capture Video Only", 0);
	HelperFuncInsertMenuByID(Avi,i++,Flags | ((localTASflags.aviMode&(1|2))==(  2)?MF_CHECKED:MF_UNCHECKED) | ((started&&!(localTASflags.emuMode&EMUMODE_EMULATESOUND))?MF_GRAYED:0),ID_AVI_AUDIO,"","Capture Audio Only", "software sound mixing must be enabled");
	HelperFuncInsertMenuByID(Avi,i++,Flags | ((localTASflags.aviMode&(1|2))==( 0 )?MF_CHECKED:MF_UNCHECKED),ID_AVI_NONE,"",localTASflags.aviMode ? "Stop / Disable" : "AVI Capture Disabled", 0);
	InsertMenu(Avi, i++, MF_SEPARATOR, NULL, NULL);
	HelperFuncInsertMenuByID(Avi,i++,Flags | ((!localTASflags.aviMode||!started||(!aviFrameCount&&!aviSoundFrameCount))?MF_GRAYED:0),ID_AVI_SPLITNOW,"","Split AVI now", "must be capturing");
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
	RECT oldrect={}, newrect={};
	GetWindowRect(tlhwnd, &oldrect);



	SetMenu(hWnd, MainMenu);


	// if setting the menu pushed the window contents down,
	// extend the bottom of the window by the same amount.
	// (less painful than trying to directly measure the menubar height or width.)
	GetWindowRect(tlhwnd, &newrect);
	int change = newrect.top - oldrect.top;
	if(change)
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);
		SetWindowPos(hWnd, NULL, 0, 0, rect.right-rect.left, change+rect.bottom-rect.top, SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	}
}
