/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "shared/ipc.h"
#include "ipc.h"

#define CONCATENATE(arg1, arg2) CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1 ## arg2

/*
 * Concatenate with empty because otherwise
 * it puts all __VA_ARGS__ arguments into the first one.
 * -- YaLTeR
 */
#define FMT_ARGS_0(...)
#define FMT_ARGS_1(arg, ...) #arg << " = " << arg <<
#define FMT_ARGS_2(arg, ...) #arg << " = " << arg << ", " << FMT_ARGS_1(__VA_ARGS__)
#define FMT_ARGS_3(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_2(__VA_ARGS__),)
#define FMT_ARGS_4(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_3(__VA_ARGS__),)
#define FMT_ARGS_5(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_4(__VA_ARGS__),)
#define FMT_ARGS_6(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_5(__VA_ARGS__),)
#define FMT_ARGS_7(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_6(__VA_ARGS__),)
#define FMT_ARGS_8(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_7(__VA_ARGS__),)
#define FMT_ARGS_9(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_8(__VA_ARGS__),)
#define FMT_ARGS_10(arg, ...) #arg << " = " << arg << ", " << CONCATENATE(FMT_ARGS_9(__VA_ARGS__),)

/*
 * Using the MSVC preprocessor comma erasure for
 * correct handling of 0 arguments.
 */
#define FOR_EACH_ADD_ARG(...) __0, __VA_ARGS__
#define FOR_EACH_RSEQ_N() 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_ARG_N(__1, __2, __3, __4, __5, __6, __7, __8, __9, __10, __11, N, ...) N
#define FOR_EACH_NARG_(...) CONCATENATE(FOR_EACH_ARG_N(__VA_ARGS__),)
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(FOR_EACH_ADD_ARG(__VA_ARGS__), FOR_EACH_RSEQ_N())

#define FMT_ARGS_(N, ...) CONCATENATE(FMT_ARGS_, N)(__VA_ARGS__)
#define FMT_ARGS(...) FMT_ARGS_(FOR_EACH_NARG(__VA_ARGS__), __VA_ARGS__)

/*
 * The final << after the FMT_ARGS() isn't missing, FMT_ARGS() may evaluate to nothing.
 */
#define ENTER(...) Log() << __func__ << "(" << FMT_ARGS(__VA_ARGS__) ") called."
#define LEAVE(...) Log() << __func__ << " returned (" << FMT_ARGS(__VA_ARGS__) ")."
#define LOG() Log() << __func__ << ": "

#define DEBUG_LOG() DebugLog<>() << __func__ << ": "

#define VERBOSE_LOG() VerboseLog() << __func__ << ": "

#include "shared/logcat.h"

/*
 * TODO: Some dumb forward declares because code is badly structured
 * -- Warepire
 */
namespace Hooks
{
    int getCurrentThreadstamp();
    int getCurrentFramestamp();
    int getCurrentTimestamp();
}

constexpr const char* LogCategoryToString(LogCategory category)
{
    switch (category)
    {
    case LogCategory::ANY:
        return "";
    case LogCategory::HOOK:
        return "[Hook]";
    case LogCategory::TIME:
        return "[Time]";
    case LogCategory::DETTIMER:
        return "[Det. Timer]";
    case LogCategory::SYNC:
        return "[Synchronization]";
    case LogCategory::DDRAW:
        return "[DDraw]";
    case LogCategory::D3D:
        return "[D3D]";
    case LogCategory::OGL:
        return "[OpenGL:TODO:Stop wrapping D3D!]";
    case LogCategory::GDI:
        return "[GDI]";
    case LogCategory::SDL:
        return "[SDL:TODO:Remove!]";
    case LogCategory::DINPUT:
        return "[DInput]";
    case LogCategory::WINPUT:
        return "[WinInput]";
    case LogCategory::XINPUT:
        return "[XInput]";
    case LogCategory::DSOUND:
        return "[DSound";
    case LogCategory::WSOUND:
        return "[WSound]";
    case LogCategory::PROCESS:
        return "[Process]";
    case LogCategory::MODULE:
        return "[Module]";
    case LogCategory::MESSAGES:
        return "[Messages]";
    case LogCategory::WINDOW:
        return "[Window]";
    case LogCategory::FILEIO:
        return "[File I/O]";
    case LogCategory::REGISTRY:
        return "[Registry]";
    case LogCategory::THREAD:
        return "[Thread]";
    case LogCategory::TIMERS:
        return "[Timers]";
    default:
        return "[Unknown]";
    }
}

template<LogCategory category = LogCategory::ANY>
class DebugLog
{
public:
    DebugLog() : m_category(category)
    {
        if (tasflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(m_category)] != true)
        {
            return;
        }
        m_print_message << LogCategoryToString(category);
        int threadStamp = Hooks::getCurrentThreadstamp();
        if (threadStamp)
        {
            m_print_message << "[" << threadStamp << "]";
        }
        else
        {
            m_print_message << "[MAIN]";
        }
        m_print_message << "[f=" << Hooks::getCurrentFramestamp() << "]";
        m_print_message << "[t=" << Hooks::getCurrentTimestamp() << "] ";
    }

    DebugLog(const DebugLog<category>&) = delete;
    ~DebugLog()
    {
        if (tasflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(m_category)] != true)
        {
            return;
        }
        IPC::SendIPCMessage(IPC::Command::CMD_DEBUG_MESSAGE, &m_print_message, sizeof(m_print_message));
    }
    DebugLog& operator=(const DebugLog&) = delete;

    template<class T>
    DebugLog& operator<<(const T& value)
    {
        if (tasflags.log_categories[static_cast<std::underlying_type<LogCategory>::type>(m_category)] != true)
        {
            return *this;
        }
        m_print_message << value;
        return *this;
    }
private:
    IPC::DebugMessage m_print_message;
    LogCategory m_category;
};

class VerboseLog
{
public:
    VerboseLog();
    VerboseLog(const VerboseLog&) = delete;
    ~VerboseLog();
    VerboseLog& operator=(const VerboseLog&) = delete;

    template<class T>
    VerboseLog& operator<<(const T& value)
    {
#ifdef VERBOSE_DEBUG
        m_print_message << value;
#endif
        return *this;
    }
private:
#ifdef VERBOSE_DEBUG
    IPC::DebugMessage m_print_message;
#endif
};
