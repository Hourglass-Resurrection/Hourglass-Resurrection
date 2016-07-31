/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>

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
	int debugPrintMode;
	int timescale, timescaleDivisor;
	int frameAdvanceHeld;
	int allowLoadInstalledDlls, allowLoadUxtheme;
	int storeVideoMemoryInSavestates;
	int appLocale;
	unsigned int movieVersion;
	int osVersionMajor, osVersionMinor;
    std::array<bool, static_cast<size_t>(LogCategory::NUM_LOG_CATEGORIES)> log_categories;
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
        CMD_PRINT_MESSAGE = 0x0000000000000002,
        CMD_PRINT_MESSAGE_REPLY,
    };

    struct CommandFrame
    {
        Command command;
        DWORD command_data_size;
        LPVOID command_data;
    };

    class PrintMessage
    {
    public:
        PrintMessage() :
            m_pos(0)
        {
            m_message[0] = '\0';
        }
        LPCSTR GetMessage() const
        {
            return m_message;
        }
        template<class T>
        PrintMessage& operator<<(const T& value)
        {
            CHAR format[16];
            /*
             * Build format string based on size of T.
             */
            snprintf(format, ARRAYSIZE(format), "0x0%d%s%%X", sizeof(T) * 2, sizeof(T) > 4 ? "I64X" : "l");
            m_pos += snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, format, value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        template<class T>
        PrintMessage& operator<<(T* value)
        {
            m_pos += snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "0x%p", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        PrintMessage& operator<<(const float& value)
        {
            m_pos += snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "%g", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        PrintMessage& operator<<(const double& value)
        {
            m_pos += snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "%g", value);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        PrintMessage& operator<<(const bool& value)
        {
            m_pos += snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "0x%s", value ? "true" : "false");
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        PrintMessage& operator<<(LPCSTR str)
        {
            m_pos = snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "%s", str);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
        PrintMessage& operator<<(LPCWSTR str)
        {
            m_pos = snprintf(m_message + m_pos, ARRAYSIZE(m_message) - m_pos, "%S", str);
            m_pos = std::min(m_pos, ARRAYSIZE(m_message));
            return *this;
        }
    private:
        CHAR m_message[4096];
        size_t m_pos;
    };
}
