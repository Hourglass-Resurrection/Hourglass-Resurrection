/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef PRINT_H_INCL
#define PRINT_H_INCL

int debugprintf(const char* fmt, ...);
int cmdprintf(const char* fmt, ...);

#define CONCATENATE(arg1, arg2) CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1 ## arg2

/*
 * Concatenate with empty because otherwise
 * it puts all __VA_ARGS__ arguments into the first one.
 * -- YaLTeR
 */
#define FMT_ARGS_0(...)
#define FMT_ARGS_1(arg, ...) #arg " = 0x%lX"
#define FMT_ARGS_2(arg, ...) #arg " = 0x%lX, " FMT_ARGS_1(__VA_ARGS__)
#define FMT_ARGS_3(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_2(__VA_ARGS__),)
#define FMT_ARGS_4(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_3(__VA_ARGS__),)
#define FMT_ARGS_5(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_4(__VA_ARGS__),)
#define FMT_ARGS_6(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_5(__VA_ARGS__),)
#define FMT_ARGS_7(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_6(__VA_ARGS__),)
#define FMT_ARGS_8(arg, ...) #arg " = 0x%lX, " CONCATENATE(FMT_ARGS_7(__VA_ARGS__),)

/*
 * Using the MSVC preprocessor comma erasure for
 * correct handling of 0 arguments.
 */
#define FOR_EACH_ADD_ARG(...) __0, __VA_ARGS__
#define FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_ARG_N(__1, __2, __3, __4, __5, __6, __7, __8, __9, N, ...) N
#define FOR_EACH_NARG_(...) CONCATENATE(FOR_EACH_ARG_N(__VA_ARGS__),)
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(FOR_EACH_ADD_ARG(__VA_ARGS__), FOR_EACH_RSEQ_N())

#define FMT_ARGS_(N, ...) CONCATENATE(FMT_ARGS_, N)(__VA_ARGS__)
#define FMT_ARGS(...) FMT_ARGS_(FOR_EACH_NARG(__VA_ARGS__), __VA_ARGS__)

#define ENTER(...) debugprintf("%s(" FMT_ARGS(__VA_ARGS__) ") called.\n", __FUNCTION__, __VA_ARGS__)

//#define dinputdebugprintf verbosedebugprintf
#define ddrawdebugprintf verbosedebugprintf
#define d3ddebugprintf verbosedebugprintf
#define dsounddebugprintf verbosedebugprintf
#define dmusicdebugprintf verbosedebugprintf
#define sdldebugprintf verbosedebugprintf
#define gldebugprintf verbosedebugprintf
#define registrydebugprintf verbosedebugprintf
#define timedebugprintf verbosedebugprintf

#define DDRAW_ENTER VERBOSE_ENTER
#define D3D_ENTER VERBOSE_ENTER
#define DSOUND_ENTER VERBOSE_ENTER
#define DMUSIC_ENTER VERBOSE_ENTER
#define SDL_ENTER VERBOSE_ENTER
#define GL_ENTER VERBOSE_ENTER
#define REGISTRY_ENTER VERBOSE_ENTER
#define TIME_ENTER VERBOSE_ENTER

#if defined(_DEBUG) && 0//1
    #define verbosedebugprintf debugprintf
    #define VERBOSE_ENTER ENTER
#else
    #define verbosedebugprintf(...) ((void)0)
    #define VERBOSE_ENTER(...) ((void)0)
#endif


#include "../shared/logcat.h"

extern LogCategoryFlag& g_includeLogFlags;
extern LogCategoryFlag& g_excludeLogFlags;

// generally should always be enabled because it's controlled by a runtime option,
// but maybe some faster-than-release configuration could disable it in the future
#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
    int logprintf_internal(LogCategoryFlag cat, const char* fmt, ...);
    #define debuglog(cat, ...)         ((((cat) & g_includeLogFlags) && !((cat) & g_excludeLogFlags)) ? logprintf_internal(cat, __VA_ARGS__) : 0)
#else
    #define debuglog(cat, ...)         0
#endif


#endif
