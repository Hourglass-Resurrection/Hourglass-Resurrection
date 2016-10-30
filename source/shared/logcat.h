/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

enum class LogCategory
{
	ANY,
	HOOK,
	TIME,
    DETTIMER,
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
};

