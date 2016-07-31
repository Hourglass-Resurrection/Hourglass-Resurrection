/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

enum class LogCategory
{
	ANY,
	//LCF_UNTESTED = 1 << 0, // things that have significant but not-yet-tested implementations. should be combined with other flags.
	//LCF_DESYNC   = 1 << 1, // things considered worth inspecting in cases of desync. should be combined with other flags.
	//LCF_FREQUENT = 1 << 2, // things that tend to get called very often (such as more than once per frame). should be combined with other flags.
	//LCF_ERROR    = 1 << 3, // things that shouldn't happen. should be combined with other flags.
	//LCF_TODO     = 1 << 4, // things known to require further implementation in the future. should be combined with other flags.
	//LCF_FRAME    = 1 << 5, // things associated with frame boundaries. should be combined with other flags.
	HOOK, // hooking notifications
	TIME, // time-related winapi functions
    DETTIMER,
	//LCF_TIMESET  = 1 << 8, // notifications of setting the internal time
	//LCF_TIMEGET  = 1 << 9, // notifications of getting the internal time
	SYNC,
	DDRAW,
	D3D,
	OGL,
	GDI,
	SDL,
	DINPUT,
	WINPUT,
    XINPUT,
	DSOUND,
	WSOUND, // non-directsound sound output, like wavout
	PROCESS,
	MODULE, // DLL functions and COM object stuff too.
	MESSAGES, // windows messages
	WINDOW, // windows windows
	FILEIO,
	REGISTRY, // windows registry or system properties
	THREAD,
	TIMERS, // as in async timer objects
    NUM_LOG_CATEGORIES,
	// warning: adding any more categories would require changing the type of LogCategoryFlag to a 64-bit int (and making associated changes so it prints correctly, etc.)
};

