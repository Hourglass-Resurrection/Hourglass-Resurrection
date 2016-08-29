/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <string>

#include "logcat.h"
#include <mmsystem.h>

struct TasFlags
{
	int playback;
	int framerate;
	int keylimit;
	int forceSoftware;
	int windowActivateFlags;
	int threadMode;
	unsigned int threadStackSize;
	int timersMode;
	int messageSyncMode;
	int waitSyncMode;
	int aviMode;
	int emuMode;
	int forceWindowed;
	int fastForward;
	int forceSurfaceMemory;
	int audioFrequency;
	int audioBitsPerSecond;
	int audioChannels;
	int stateLoaded;
	int fastForwardFlags;
	int initialTime;
	int timescale, timescaleDivisor;
	int frameAdvanceHeld;
	int allowLoadInstalledDlls, allowLoadUxtheme;
	int storeVideoMemoryInSavestates;
	int appLocale;
	unsigned int movieVersion;
	int osVersionMajor, osVersionMinor;
    std::array<bool, static_cast<std::underlying_type<LogCategory>::type>(LogCategory::NUM_LOG_CATEGORIES)> log_categories;
#ifdef _USRDLL
	char reserved [256]; // just-in-case overwrite guard
#endif
};
#ifdef _USRDLL
extern TasFlags tasflags;
#endif

struct DllLoadInfo
{
	bool loaded; // true=load, false=unload
	char dllname [64];
};
struct DllLoadInfos
{
	int numInfos; // (must be the first thing in this struct, due to an assumption in AddAndSendDllInfo)
	DllLoadInfo infos [128];
};


struct InfoForDebugger // GeneralInfoFromDll
{
	int frames;
	int ticks;
	int addedDelay;
	int lastNewTicks;
};


enum
{
	CAPTUREINFO_TYPE_NONE, // nothing sent
	CAPTUREINFO_TYPE_NONE_SUBSEQUENT, // nothing sent and it's the same frame/time as last time
	CAPTUREINFO_TYPE_PREV, // reuse previous frame's image (new sleep frame)
	CAPTUREINFO_TYPE_DDSD, // locked directdraw surface description
};

struct LastFrameSoundInfo
{
	DWORD size;
	unsigned char* buffer;
	LPWAVEFORMATEX format;
};


enum
{
	EMUMODE_EMULATESOUND = 0x01,
	EMUMODE_NOTIMERS = 0x02,
	EMUMODE_NOPLAYBUFFERS = 0x04,
	EMUMODE_VIRTUALDIRECTSOUND = 0x08,
};

enum
{
	FFMODE_FRONTSKIP = 0x01,
	FFMODE_BACKSKIP = 0x02,
	FFMODE_SOUNDSKIP = 0x04,
	FFMODE_RAMSKIP = 0x08,
	FFMODE_SLEEPSKIP = 0x10,
	FFMODE_WAITSKIP = 0x20,
};


struct TrustedRangeInfo
{
	DWORD start, end;
};
struct TrustedRangeInfos
{
	int numInfos;
	TrustedRangeInfo infos [32]; // the first one is assumed to be the injected dll's range
};

#ifndef SUCCESSFUL_EXITCODE
#define SUCCESSFUL_EXITCODE 4242
#endif

#include <algorithm>
#include <cstdio>

/*
 * Communication interface for the DLL, all communication is initiated by the DLL .
 */

/*
 * Undefine some WinAPI names that get in the way.
 */
#undef GetMessage

namespace IPC
{
    enum class Command : DWORD64
    {
        /*
         * Ignore command values 0 and 1 to be able to distinguish between a command and a
         * zeroed out command frame.
         */
         /*
          * TODO: Remove some of these commands
          * -- Warepire
          */
        CMD_DUMMY_ENTRY_MIN_OPCODE = 0x0000000000000001,
        CMD_DEBUG_MESSAGE,
        CMD_DEBUG_MESSAGE_REPLY,
        CMD_FRAME_BOUNDARY,
        CMD_FRAME_BOUNDARY_REPLY,
        CMD_FPS_UPDATE,
        CMD_FPS_UPDATE_REPLY,
        CMD_SAVE_STATE,
        CMD_SAVE_STATE_REPLY,
        CMD_LOAD_STATE,
        CMD_LOAD_STATE_REPLY,
        CMD_DLL_VERSION,
        CMD_DLL_VERSION_REPLY,
        CMD_COMMAND_BUF,
        CMD_COMMAND_BUF_REPLY,
        CMD_INPUT_BUF,
        CMD_INPUT_BUF_REPLY,
        CMD_DLL_LOAD_INFO_BUF,
        CMD_DLL_LOAD_INFO_BUF_REPLY,
        CMD_TRUSTED_RANGE_INFO_BUF,
        CMD_TRUSTED_RANGE_INFO_BUF_REPLY,
        CMD_TAS_FLAGS_BUF,
        CMD_TAS_FLAGS_BUF_REPLY,
        CMD_SOUND_INFO,
        CMD_SOUND_INFO_REPLY,
        CMD_GENERAL_INFO,
        CMD_GENERAL_INFO_REPLY,
        CMD_PALETTE_ENTRIES,
        CMD_PALETTE_ENTRIES_REPLY,
        CMD_GIMME_DLL_LOAD_INFOS,
        CMD_GIMME_DLL_LOAD_INFOS_REPLY,
        CMD_KEYBOARD_LAYOUT_NAME,
        CMD_KEYBOARD_LAYOUT_NAME_REPLY,
        CMD_GAMMA_RAMP_BUF,
        CMD_GAMMA_RAMP_BUF_REPLY,
        CMD_MOUSE_REG,
        CMD_MOUSE_REG_REPLY,
        CMD_HWND,
        CMD_HWND_REPLY,
        CMD_POST_DLL_MAIN_DONE,
        CMD_POST_DLL_MAIN_DONE_REPLY,
        CMD_WATCH_ADDRESS,
        CMD_WATCH_ADDRESS_REPLY,
        CMD_UNWATCH_ADDRESS,
        CMD_UNWATCH_ADDRESS_REPLY,
        CMD_KILL_ME,
        CMD_KILL_ME_REPLY,
        CMD_SUSPEND_ALL_THREADS,
        CMD_SUSPEND_ALL_THREADS_REPLY,
        CMD_RESUME_ALL_THREADS,
        CMD_RESUME_ALL_THREADS_REPLY,
        CMD_SUGGEST_THREAD_NAME,
        CMD_SUGGEST_THREAD_NAME_REPLY,
        CMD_DENIED_THREAD,
        CMD_DENIED_THREAD_REPLY,
        CMD_STACK_TRACE,
        CMD_STACK_TRACE_REPLY,
    };

    struct CommandFrame
    {
        Command command;
        DWORD command_data_size;
        LPCVOID command_data;
    };

    class FPSInfo
    {
    public:
        FPSInfo(FLOAT fps, FLOAT logical_fps)
        {
            m_fps = fps;
            m_logical_fps = logical_fps;
        }
        FLOAT GetFPS() const
        {
            return m_fps;
        }
        FLOAT GetLogicalFPS() const
        {
            return m_logical_fps;
        }
    private:
        FLOAT m_fps;
        FLOAT m_logical_fps;
    };

    class FrameBoundaryInfo
    {
    public:
        FrameBoundaryInfo(DWORD total_ran_frames, LPCVOID capture_info, DWORD capture_info_type)
        {
            m_total_ran_frames = total_ran_frames;
            m_capture_info = capture_info;
            m_capture_info_type = capture_info_type;
        }
        DWORD GetTotalRanFrames() const
        {
            return m_total_ran_frames;
        }
        LPCVOID GetCaptureInfo() const
        {
            return m_capture_info;
        }
        DWORD GetCaptureInfoType() const
        {
            return m_capture_info_type;
        }
    private:
        DWORD m_total_ran_frames;
        LPCVOID m_capture_info;
        DWORD m_capture_info_type;
    };

    class StackTrace
    {
    public:
        StackTrace(int min_depth, int max_depth)
        {
            m_min_depth = min_depth;
            m_max_depth = max_depth;
        }
        int GetMinDepth() const
        {
            return m_min_depth;
        }
        int GetMaxDepth() const
        {
            return m_max_depth;
        }
    private:
        int m_min_depth;
        int m_max_depth;
    };

    class SuggestThreadName
    {
    public:
        LPCSTR GetThreadName() const
        {
            return m_thread_name;
        }
        void SetThreadName(LPCSTR thread_name)
        {
            strncpy(m_thread_name, thread_name, ARRAYSIZE(m_thread_name));
        }
    private:
        CHAR m_thread_name[256];
    };

    class AutoWatch
    {
    public:
        AutoWatch(LPCVOID address, CHAR size, CHAR type, LPCSTR comment)
        {
            m_address = reinterpret_cast<UINT>(address);
            m_size = size;
            m_type = type;
            strncpy(m_comment, comment, ARRAYSIZE(m_comment));
        }
        UINT GetAddress() const
        {
            return m_address;
        }
        CHAR GetSize() const
        {
            return m_size;
        }
        CHAR GetType() const
        {
            return m_type;
        }
        LPCSTR GetComment() const
        {
            return m_comment;
        }
    private:
        /*
         * For now, just a dumb copy-paste from the AddressWatcher struct + comment field
         */
        unsigned int m_address;
        char m_size;
        char m_type;
        char m_comment[256];
    };

    class DebugMessage
    {
    public:
        DebugMessage() :
            m_pos(0)
        {
            m_message[0] = L'\0';
        }
        LPCWSTR GetMessage() const
        {
            return m_message;
        }
        template<class T>
        DebugMessage& operator<<(const T& value)
        {
            WCHAR format[16];
            /*
             * Build format string based on size of T.
             */
            swprintf(format, ARRAYSIZE(format), L"0x%%0%d%sX", sizeof(T) * 2, sizeof(T) > 4 ? L"I64X" : L"l");
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, format, value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        template<class T>
        DebugMessage& operator<<(T* value)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"0x%p", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        DebugMessage& operator<<(const float& value)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"%g", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        DebugMessage& operator<<(const double& value)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"%g", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        DebugMessage& operator<<(const bool& value)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"%s", value ? L"true" : L"false");
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        DebugMessage& operator<<(LPCSTR str)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"%S", str);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        DebugMessage& operator<<(LPCWSTR str)
        {
            m_pos += swprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, L"%s", str);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
    private:
        WCHAR m_message[4096];
        size_t m_pos;
    };
}
