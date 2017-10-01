/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>

#include "shared/version.h"
#include "../wintaser.h"

// HACK!
#include "../Config.h"

#include "application/GUI/Core/DlgBase.h"
#include "application/GUI/Objects/CheckableMenuItem.h"
#include "application/GUI/Objects/CheckboxButton.h"
#include "application/GUI/Objects/EditControl.h"
#include "application/GUI/Objects/GroupBox.h"
#include "application/GUI/Objects/MenuItem.h"
#include "application/GUI/Objects/MenuSeparator.h"
#include "application/GUI/Objects/PushButton.h"
#include "application/GUI/Objects/RadioButton.h"
#include "application/GUI/Objects/StaticText.h"
#include "application/GUI/Objects/SubMenu.h"

#include "MainWindow.h"

MainWindow::MainWindow() :
    DlgBase(
        std::wstring(L"Hourglass-Resurrection v") + std::to_wstring(VERSION) + L"." + std::to_wstring(MINORVERSION)
#ifdef _DEBUG
                     + L" (debug)"
#endif
        , 0, 0, 301, 178)
{
    using namespace Config;
    RadioButton read_only(L"Read-Only", 235, 44, 50, 10, this); read_only.SetAsNewGroup(); read_only.RegisterOnClickCallback([](WORD) { HourglassCore::SetMovieReadOnly(true); return true; });
    RadioButton read_write(L"Read+Write", 235, 56, 54, 10, this); read_write.RegisterOnClickCallback([](WORD) { HourglassCore::SetMovieReadOnly(false); return true; });
    EditControl movie_filename(L"", 7, 13, 230, 14, this); movie_filename.SetEnabled(false);
    StaticText(L"Movie File", 10, 3, 32, 8, this);
    PushButton browse_movie(L"Browse...", 246, 13, 48, 14, this);
    StaticText(L"Frame:", 12, 32, 27, 8, this);
    StaticText(L"Re-records:", 49, 61, 39, 8, this);
    StaticText(L"Frames per Second:", 21, 47, 80, 8, this);
    EditControl fps(std::to_wstring(localTASflags.framerate), 91, 45, 40, 12, this);
    EditControl curframe(L"0", 38, 30, 40, 12, this); curframe.SetEnabled(false);
    EditControl maxframe(L"0", 91, 30, 40, 12, this); maxframe.SetEnabled(false);
    StaticText frameslash(L"/", 83, 32, 8, 8, this);
    EditControl rerecs(L"", 91, 59, 40, 12, this);
    StaticText movietime(L"", 144, 32, 149, 8, this);
    StaticText curfps(L"Current FPS: 0", 144, 47, 82, 8, this);
    EditControl exe(L"", 7, 134, 230, 14, this); exe.SetEnabled(false);
    StaticText(L"Game Executable", 10, 125, 56, 8, this);
    PushButton browse_exe(L"Browse...", 246, 134, 48, 14, this); browse_exe.SetFocus();
    StaticText(L"System Time: ", 21, 79, 66, 8, this);
    EditControl sysclock(std::to_wstring(localTASflags.initialTime), 71, 77, 60, 12, this);
    StaticText movie_status(L"Current Status: Playing", 144, 61, 91, 8, this);
    PushButton run_record(L"Run and Record New Movie", 8, 154, 99, 17, this); run_record.SetEnabled(false);
    PushButton play(L"Run and Play Existing Movie", 192, 154, 102, 17, this); play.SetEnabled(false);
    PushButton stop(L"Stop Running", 116, 154, 67, 17, this); stop.SetEnabled(false);
    CheckboxButton paused(L"Paused", 236, 69, 39, 10, this);
    GroupBox(L"Multithreading and Wait Sync", 13, 94, 118, 23, this);
    RadioButton thread_allow(L"Allow", 20, 103, 33, 10, this); thread_allow.SetAsNewGroup(); thread_allow.SetMarked(localTASflags.threadMode == 2);
    RadioButton thread_wrap(L"Wrap", 54, 103, 33, 10, this); thread_wrap.SetMarked(localTASflags.threadMode == 1);
    RadioButton thread_disable(L"Disable", 88, 103, 39, 10, this); thread_disable.SetMarked(localTASflags.threadMode == 0);
    CheckboxButton mute(L"Mute", 246, 108, 39, 10, this);
    CheckboxButton f_forward(L"Fast-Forward", 236, 82, 57, 10, this);
    StaticText(L"Command Line Arguments", 143, 94, 97, 8, this);
    EditControl cmdline(L"", 140, 104, 97, 12, this);

    RegisterCloseEventCallback(std::bind(&MainWindow::OnCloseEvent, this));

    CreateMenu();
}

MainWindow::~MainWindow()
{
}

void MainWindow::CreateMenu()
{
    // Hacky...
    // TODO: Disable reasons?

    using namespace Config;

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

    SubMenu file(L"&File", nullptr, this);

    on = !exe_filename_only.empty() && !started;
    MenuItem(std::wstring(L"&Open Executable...") + (on ? L"(now open: \"" : L"") + (on ? exe_filename_only : L"") + (on ? L"\")" : L""),
                       L"Ctrl+O", /*L"Can't change while running", */&file, this).SetEnabled(!started);
    on = !movie_filename_only.empty() && !started;
    MenuItem(std::wstring(L"&Open Movie...") + (on ? L"(now open: \"" : L"") + (on ? movie_filename_only : L"") + (on ? L"\")" : L""),
                       L"Ctrl+M", /*L"Can't change while running", */&file, this).SetEnabled(!started);
    MenuSeparator(&file, this);
    MenuItem(L"&Play Movie", L"Ctrl+P", /*!movie_filename_only.empty() ? L"No movie opened" : L"Already playing", */&file, this).SetEnabled(movie_filename_only.empty() || started);
    MenuItem(L"&Watch From Beginning", L"", /*L"Movie is not playing", */&file, this).SetEnabled(started && localTASflags.playback);
    MenuSeparator(&file, this);
    MenuItem(L"&Record New Movie...", L"Ctrl+R", /*started ? L"Must stop running first" : L"Must open an executable first", */&file, this).SetEnabled(!started && exeFileExists);
    MenuItem(L"Backup Movie to File...", L"", /*L"Must be recording", */&file, this).SetEnabled(started && !localTASflags.playback);
    MenuItem(L"Resume Recording from Now", L"", /*L"Movie must be playing", */&file, this).SetEnabled(started && localTASflags.playback && !finished);
    MenuSeparator(&file, this);
    MenuItem(L"Save Config As...", L"", &file, this);
    MenuItem(L"Load Config From...", L"", &file, this);
    MenuSeparator(&file, this);
    MenuItem(L"Stop Running", L"", /*L"Already stopped", */&file, this).SetEnabled(started);
    MenuSeparator(&file, this);
    MenuItem(L"E&xit", L"Alt+F4", &file, this).RegisterOnClickHandler([this](WORD) { return OnCloseEvent(); });

    SubMenu graphics(L"&Graphics", nullptr, this);

    CheckableMenuItem(L"Allow &Fullscreen / Display Mode Changes", L"", /*L"Can't change while running", */&graphics, this).SetEnabled(!started).SetChecked(!localTASflags.forceWindowed);
    MenuSeparator(&graphics, this);
    CheckableMenuItem(L"&Allow Hardware Acceleration", L"", /*L"Can't change while running", */&graphics, this).SetEnabled(!started).SetChecked(!localTASflags.forceSoftware);

    SubMenu surface_memory(L"Surface &Memory", /*started ? L"Can't change while running" : L"Hardware acceleration must be enabled", */&graphics, this);
    surface_memory.SetEnabled(!localTASflags.forceSoftware && !started);

    CheckableMenuItem(L"&Let Game Choose", L"", &surface_memory, this).SetChecked(localTASflags.forceSurfaceMemory == 0);
    CheckableMenuItem(L"&System Memory               (Slow Draw, Fast Read)", L"", &surface_memory, this).SetChecked(localTASflags.forceSurfaceMemory == 1);
    CheckableMenuItem(L"&Non-Local Video Memory (Varies)", L"", &surface_memory, this).SetChecked(localTASflags.forceSurfaceMemory == 2);
    CheckableMenuItem(L"Local &Video Memory        (Fast Draw, Slow Read)", L"", &surface_memory, this).SetChecked(localTASflags.forceSurfaceMemory == 3);

    SubMenu sound(L"&Sound", nullptr, this);

    CheckableMenuItem(L"&Use Software Mixing", L"", &sound, this).SetEnabled(!started).SetChecked(localTASflags.emuMode & EMUMODE_EMULATESOUND); // L"can't change while running"

    SubMenu sound_format(L"&Format", &sound, this); sound_format.SetEnabled(!started || localTASflags.emuMode & EMUMODE_EMULATESOUND); // started ? L"can't change while running" : L"software mixing must be enabled"

    CheckableMenuItem(L"8000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 8000);
    CheckableMenuItem(L"&11025 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 11025);
    CheckableMenuItem(L"12000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 12000);
    CheckableMenuItem(L"16000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 16000);
    CheckableMenuItem(L"&22050 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 22050);
    CheckableMenuItem(L"24000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 24000);
    CheckableMenuItem(L"32000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 32000);
    CheckableMenuItem(L"&44100 Hz (default)", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 44100);
    CheckableMenuItem(L"48000 Hz", L"", &sound_format, this).SetChecked(localTASflags.audioFrequency == 48000);
    MenuSeparator(&sound_format, this);
    CheckableMenuItem(L"8 bit", L"", &sound_format, this).SetChecked(localTASflags.audioBitsPerSecond == 8);
    CheckableMenuItem(L"16 bit (default)", L"", &sound_format, this).SetChecked(localTASflags.audioBitsPerSecond == 16);
    MenuSeparator(&sound_format, this);
    CheckableMenuItem(L"&Mono", L"", &sound_format, this).SetChecked(localTASflags.audioChannels == 1);
    CheckableMenuItem(L"&Stereo (default)", L"", &sound_format, this).SetChecked(localTASflags.audioChannels == 2);

    MenuSeparator(&sound, this);
    CheckableMenuItem(L"&Mute Sound", L"", &sound, this).SetChecked(localTASflags.emuMode & EMUMODE_NOPLAYBUFFERS);
    CheckableMenuItem(L"&Disable DirectSound Creation", L"", &sound, this).SetChecked(localTASflags.emuMode & EMUMODE_VIRTUALDIRECTSOUND);

    SubMenu runtime(L"&Runtime", nullptr, this);

    SubMenu app_locale(L"App &Locale", &runtime, this);

    int curAppLocale = localTASflags.appLocale ? localTASflags.appLocale : tempAppLocale;
    CheckableMenuItem(L"Use system locale", L"", &app_locale, this).SetEnabled(!tempAppLocale || !started).SetChecked(curAppLocale == 0); // L"movie has forced non-system locale"
    CheckableMenuItem(L"Force &Japanese locale", L"", &app_locale, this).SetEnabled(curAppLocale || !started).SetChecked(curAppLocale == 1041); // L"can't enable while running"
    CheckableMenuItem(L"Force &Chinese (Simplified) locale", L"", &app_locale, this).SetEnabled(curAppLocale || !started).SetChecked(curAppLocale == 2052); // L"can't enable while running"
    CheckableMenuItem(L"Force &Korean locale", L"", &app_locale, this).SetEnabled(curAppLocale || !started).SetChecked(curAppLocale == 1042); // L"can't enable while running"
    CheckableMenuItem(L"Force &English locale", L"", &app_locale, this).SetEnabled(curAppLocale || !started).SetChecked(curAppLocale == 1033); //  L"can't enable while running"

    MenuSeparator(&runtime, this);

    SubMenu affinity(L"&Affinity", &runtime, this);

    CheckableMenuItem(L"Use only one physical core (Might help older games to run correctly)", L"", &affinity, this).SetEnabled(!started).SetChecked(limitGameToOnePhysicalCore); //  L"can't change after starting"
    CheckableMenuItem(L"Disable Hyper-Threading", L"", &affinity, this).SetEnabled(!started).SetChecked(disableHyperThreading); // L"can't change after starting"

    MenuSeparator(&runtime, this);

    SubMenu multithreading(L"&Multithreading Mode", &runtime, this);

    CheckableMenuItem(L"&Disable (prevent thread creation)", L"", &multithreading, this).SetChecked(localTASflags.threadMode == 0);
    CheckableMenuItem(L"&Wrap (recycle threads)", L"", &multithreading, this).SetEnabled(usedThreadMode < 2).SetChecked(localTASflags.threadMode == 1); // L"can't set while running after normal threads created"
    CheckableMenuItem(L"&Allow (normal thread creation)", L"", &multithreading, this).SetEnabled(usedThreadMode != 1).SetChecked(localTASflags.threadMode == 2); // L"can't set while running after wrapped threads created"
    CheckableMenuItem(L"Allow &known threads only", L"", &multithreading, this).SetEnabled(usedThreadMode != 1).SetChecked(localTASflags.threadMode == 3); // L"can't set while running after wrapped threads created"
    CheckableMenuItem(L"Allow &unknown threads only", L"", &multithreading, this).SetEnabled(usedThreadMode != 1).SetChecked(localTASflags.threadMode == 4); // L"can't set while running after wrapped threads created"
    CheckableMenuItem(L"Allow &trusted threads only", L"", &multithreading, this).SetEnabled(usedThreadMode != 1).SetChecked(localTASflags.threadMode == 5); // L"can't set while running after wrapped threads created"

    SubMenu mm_timers(L"Multimedia &Timer Mode", &runtime, this);

    CheckableMenuItem(L"&Disable (prevent timer creation)", L"", &mm_timers, this).SetChecked(localTASflags.timersMode == 0);
    CheckableMenuItem(L"&Synchronous (run timers at frame boundaries)", L"", &mm_timers, this).SetChecked(localTASflags.timersMode == 1);
    CheckableMenuItem(L"&Asynchronous (run timers in separate threads)", L"", &mm_timers, this).SetChecked(localTASflags.timersMode == 2);

    SubMenu message_sync(L"Message &Sync Mode", &runtime, this);

    CheckableMenuItem(L"&Synchronous (no timeouts)", L"", &message_sync, this).SetChecked(localTASflags.messageSyncMode == 0);
    CheckableMenuItem(L"Semi-S&ynchronous (some timeouts)", L"", &message_sync, this).SetChecked(localTASflags.messageSyncMode == 1);
    CheckableMenuItem(L"&Asynchronous (any timeouts)", L"", &message_sync, this).SetChecked(localTASflags.messageSyncMode == 2);
    CheckableMenuItem(L"&Unchecked (native messaging)", L"", &message_sync, this).SetChecked(localTASflags.messageSyncMode == 3);
    
    SubMenu wait_sync(L"&Wait Sync Mode", &runtime, this);

    CheckableMenuItem(L"Synchronous &Wait (infinite timeouts)", L"", &wait_sync, this).SetChecked(localTASflags.waitSyncMode == 0);
    CheckableMenuItem(L"Synchronous &Skip (limited timeouts)", L"", &wait_sync, this).SetChecked(localTASflags.waitSyncMode == 1);
    CheckableMenuItem(L"&Asynchronous (unchecked waits)", L"", &wait_sync, this).SetChecked(localTASflags.waitSyncMode == 2);

    SubMenu dll_loading(L"&DLL Loading", &runtime, this);

    CheckableMenuItem(L"Allow loading any custom/installed DLLs (e.g. Fraps) (can affect sync)", L"", &dll_loading, this).SetChecked(localTASflags.allowLoadInstalledDlls);
    CheckableMenuItem(L"Allow loading uxtheme.dll (for non-classic window styles on XP)", L"", &dll_loading, this).SetChecked(localTASflags.allowLoadUxtheme);
    MenuSeparator(&dll_loading, this);
    CheckableMenuItem(L"Allow other DLLs to run first (this is a hack currently required for RotateGear)", L"", &dll_loading, this).SetChecked(runDllLast);

    CheckableMenuItem(L"Disable Pause Helper", L"", &runtime, this).SetChecked(truePause);
    CheckableMenuItem(L"Wait until sub-process creation", L"", &runtime, this).SetEnabled(!started).SetChecked(onlyHookChildProcesses); // L"can't change while running"
    MenuSeparator(&runtime, this);
    
    SubMenu performance(L"&Performance", &runtime, this);

    CheckableMenuItem(L"Store Video Memory in Savestates", L"", &performance, this).SetChecked(localTASflags.storeVideoMemoryInSavestates);
    CheckableMenuItem(L"Store Guarded Memory Pages in Savestates", L"", &performance, this).SetChecked(storeGuardedPagesInSavestates);
    MenuSeparator(&performance, this);
    MenuItem(L"Discard All Savestates Now", L"", &performance, this).SetEnabled(started); // L"must be running"
    MenuItem(L"Delete All Savestates Now", L"", &performance, this);

    SubMenu debug(L"&Debug Logging", &runtime, this);

    CheckableMenuItem(L"Uncategorized", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::ANY)]);
    CheckableMenuItem(L"Hooking", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::HOOK)]);
    CheckableMenuItem(L"Time", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::TIME)]);
    CheckableMenuItem(L"Det. Timer", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DETTIMER)]);
    CheckableMenuItem(L"Sync", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::SYNC)]);
    CheckableMenuItem(L"DirectDraw", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DDRAW)]);
    CheckableMenuItem(L"Direct3D", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::D3D)]);
    CheckableMenuItem(L"OpenGL", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::OGL)]);
    CheckableMenuItem(L"GDI", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::GDI)]);
    CheckableMenuItem(L"SDL", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::SDL)]);
    CheckableMenuItem(L"DirectInput", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DINPUT)]);
    CheckableMenuItem(L"Windows Input", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WINPUT)]);
    CheckableMenuItem(L"XInput", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::XINPUT)]);
    CheckableMenuItem(L"DirectSound", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::DSOUND)]);
    CheckableMenuItem(L"Windows Sound", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WSOUND)]);
    CheckableMenuItem(L"Process", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::PROCESS)]);
    CheckableMenuItem(L"Module", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::MODULE)]);
    CheckableMenuItem(L"Messages", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::MESSAGES)]);
    CheckableMenuItem(L"Window", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::WINDOW)]);
    CheckableMenuItem(L"File", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::FILEIO)]);
    CheckableMenuItem(L"Registry", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::REGISTRY)]);
    CheckableMenuItem(L"Thread", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::THREAD)]);
    CheckableMenuItem(L"Timers", L"", &debug, this).SetChecked(localTASflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(LogCategory::TIMERS)]);
    MenuSeparator(&debug, this);
    MenuItem(L"All", L"", &debug, this);
    MenuItem(L"None (uncategorized only)", L"", &debug, this);

    SubMenu input(L"&Input", nullptr, this);

    MenuItem(L"Configure &Inputs...", L"", &input, this);
    MenuSeparator(&input, this);
    
    SubMenu enable_hotkeys(L"Enable Hotkeys When", &input, this);

    CheckableMenuItem(L"Hourglass has Focus", L"", &enable_hotkeys, this).SetChecked(hotkeysFocusFlags & FOCUS_FLAG_TASER);
    CheckableMenuItem(L"Game Window has Focus", L"", &enable_hotkeys, this).SetChecked(hotkeysFocusFlags & FOCUS_FLAG_TASEE);
    CheckableMenuItem(L"Other Programs have Focus", L"", &enable_hotkeys, this).SetChecked(hotkeysFocusFlags & FOCUS_FLAG_OTHER);

    SubMenu enable_input(L"Enable Game Input When", &input, this);

    CheckableMenuItem(L"Hourglass has Focus", L"", &enable_input, this).SetChecked(inputFocusFlags & FOCUS_FLAG_TASER);
    CheckableMenuItem(L"Game Window has Focus", L"", &enable_input, this).SetChecked(inputFocusFlags & FOCUS_FLAG_TASEE);
    CheckableMenuItem(L"Other Programs have Focus", L"", &enable_input, this).SetChecked(inputFocusFlags&FOCUS_FLAG_OTHER);

    MenuSeparator(&input, this);
    CheckableMenuItem(L"Frame Advance Skips \"&Lag\" Frames", L"", &input, this).SetChecked(advancePastNonVideoFrames);

    SubMenu tools(L"Tools", nullptr, this);

    CheckableMenuItem(L"Pause", L"", &tools, this).SetChecked(paused);
    MenuSeparator(&tools, this);

    SubMenu slow_motion(L"&Slow Motion", &tools, this);

    CheckableMenuItem(L"Speed &Up", L"", &slow_motion, this).SetChecked(localTASflags.timescale == localTASflags.timescaleDivisor);
    CheckableMenuItem(L"Slow &Down", L"", &slow_motion, this).SetChecked(localTASflags.timescale * 8 == localTASflags.timescaleDivisor);
    MenuSeparator(&slow_motion, this);
    CheckableMenuItem(L"&100% (normal speed)", L"", &slow_motion, this).SetChecked(localTASflags.timescale == localTASflags.timescaleDivisor);
    CheckableMenuItem(L"75%", L"", &slow_motion, this).SetChecked(localTASflags.timescale * 4 == localTASflags.timescaleDivisor * 3);
    CheckableMenuItem(L"50%", L"", &slow_motion, this).SetChecked(localTASflags.timescale * 2 == localTASflags.timescaleDivisor);
    CheckableMenuItem(L"25%", L"", &slow_motion, this).SetChecked(localTASflags.timescale * 4 == localTASflags.timescaleDivisor);
    CheckableMenuItem(L"12%", L"", &slow_motion, this).SetChecked(localTASflags.timescale * 8 == localTASflags.timescaleDivisor);
    
    MenuSeparator(&tools, this);
    CheckableMenuItem(L"Fast-Forward", L"", &tools, this).SetChecked(fastforward);
    
    SubMenu ff_options(L"&Fast-Forward Options", &tools, this);

    CheckableMenuItem(L"Frontbuffer Frameskip", L"", &ff_options, this).SetChecked(localTASflags.fastForwardFlags & FFMODE_FRONTSKIP);
    CheckableMenuItem(L"Backbuffer Frameskip", L"", &ff_options, this).SetChecked(localTASflags.fastForwardFlags & FFMODE_BACKSKIP);
    CheckableMenuItem(L"Soundskip", L"", &ff_options, this).SetChecked(recoveringStale || (localTASflags.fastForwardFlags & FFMODE_SOUNDSKIP)).SetEnabled(!recoveringStale);
    CheckableMenuItem(L"RAM Search/Watch Skip", L"", &ff_options, this).SetChecked(localTASflags.fastForwardFlags & FFMODE_RAMSKIP);
    CheckableMenuItem(L"Sleep Skip", L"", &ff_options, this).SetChecked(localTASflags.fastForwardFlags & FFMODE_SLEEPSKIP);
    CheckableMenuItem(L"Wait Skip", L"", &ff_options, this).SetChecked(localTASflags.fastForwardFlags & FFMODE_WAITSKIP);

    MenuSeparator(&tools, this);

    MenuItem(L"RAM &Watch", L"", &tools, this);
    MenuItem(L"&RAM Search", L"", &tools, this);

    SubMenu avi(L"&AVI", nullptr, this);
    CheckableMenuItem(L"Capture Video and Audio", L"", &avi, this).SetEnabled(!started && (localTASflags.emuMode & EMUMODE_EMULATESOUND)).SetChecked((localTASflags.aviMode & (1 | 2)) == (1 | 2)); // L"software sound mixing must be enabled"
    CheckableMenuItem(L"Capture Video Only", L"", &avi, this).SetChecked((localTASflags.aviMode & (1 | 2)) == 1);
    CheckableMenuItem(L"Capture Audio Only", L"", &avi, this).SetEnabled(!started && !!(localTASflags.emuMode & EMUMODE_EMULATESOUND)).SetChecked(started && !((localTASflags.aviMode & (1 | 2)) == 2)); // L"software sound mixing must be enabled"
    CheckableMenuItem(L"AVI Capture Disabled", L"", &avi, this).SetChecked((localTASflags.aviMode & (1 | 2)) == 0);
    MenuSeparator(&avi, this);
    MenuItem(L"Split AVI now", L"", &avi, this).SetEnabled(localTASflags.aviMode && started && (aviFrameCount || aviSoundFrameCount)); // L"must be capturing"
}

void MainWindow::Spawn(int show_window)
{
    SpawnDialogBox(nullptr, DlgMode::INDIRECT);
    ShowDialogBox(show_window);
    UpdateWindow();
}

bool MainWindow::OnCloseEvent()
{
    //SendMessage(WM_COMMAND, IDC_BUTTON_STOP, 0);
    HourglassCore::PrepareForExit();
    DestroyDialog();
    return true;
}
