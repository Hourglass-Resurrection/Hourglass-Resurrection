/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef IPC_H_INCL
#define IPC_H_INCL

#include "logcat.h"

struct TasFlags
{
	int playback;
	int framerate;
	int keylimit;
	int forceSoftware;
	int windowActivateFlags;
	int threadMode;
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
	LogCategoryFlag includeLogFlags;
	LogCategoryFlag excludeLogFlags;
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

#endif // IPC_H_INCL
