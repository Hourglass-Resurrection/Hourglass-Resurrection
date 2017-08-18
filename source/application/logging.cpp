/*
* Copyright (c) 2017- Hourglass Resurrection Team
* Copyright (c) 2011 nitsuja and contributors
* Hourglass Resurrection is licensed under GPL v2.
* Refer to the file COPYING.txt in the project root.
*/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <fstream>
#include <sstream>

#include "logging.h"

#include "shared/ipc.h"
#include "Config.h"

#include "DbgHelp/DbgHelp.h"

namespace
{
    std::wofstream gs_debuglogfile;

    CRITICAL_SECTION gs_debug_print_cs;
}

void InitDebugCriticalSection()
{
	InitializeCriticalSection(&gs_debug_print_cs);
}

void PrintLastError(LPCWSTR lpszFunction, DWORD dw)
{
	if(!dw)
		return;

	LPVOID lpMsgBuf;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL );

    DebugLog() << lpszFunction << " failed, error " << dw << ": " << lpMsgBuf;
    LocalFree(lpMsgBuf);
}

IDbgHelpStackWalkCallback::Action PrintStackTrace(IDbgHelpStackWalkCallback& data)
{
    /*
     * C++ dictates that classes inside functions can't declare template methods,
     * classes inside lamdas can however, so, just wrap it.
     * -- Warepire
     */
    auto dummy = [&data]() {
        std::wostringstream oss(L"STACK FRAME: ");
        oss.setf(std::ios_base::showbase);
        oss.setf(std::ios_base::hex, std::ios_base::basefield);

        /*
         * Add the module and function name.
         */
        oss << data.GetModuleName() << L"!" << data.GetProgramCounter();
        oss << L" : " << data.GetFunctionName() << L'(';

        size_t arg_number = 1;
        for (const auto& parameter : data.GetParameters())
        {
            if (arg_number > 1)
            {
                oss << L", ";
            }

            /*
             * Add the parameter type and name.
             */
            oss << parameter.m_type.GetName() << L' ' << parameter.m_name << L" = ";

            /*
             * Add the parameter value.
             */
            if (parameter.m_value.has_value())
            {
                std::visit([&oss](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, char>
                               || std::is_same_v<T, wchar_t>
                               || std::is_same_v<T, char16_t>
                               || std::is_same_v<T, char32_t>)
                    {
                        /*
                         * Print the characters.
                         */
                        oss << static_cast<int>(value) << L" \'" << value << L'\'';
                    }
                    else if constexpr (std::is_same_v<T, void*>)
                    {
                        /*
                         * Pointers ignore showbase.
                         */
                        oss << L"0x" << value;
                    }
                    else
                    {
                        oss << value;
                    }
                }, parameter.m_value.value());
            }

            ++arg_number;
        }

        oss << L')';

        /*
         * Add the unsure status display.
         */
        if (data.GetUnsureStatus() > 0)
        {
            oss << L'?';
        }

        DebugLog() << oss.str();

        return IDbgHelpStackWalkCallback::Action::CONTINUE;
    };
    return dummy();
};

// TODO: Break filewriting out into it's own globally set function that can be replaced.
DebugLog::~DebugLog()
{
    m_buffer << L"\n";
    if (IsDebuggerPresent())
    {
        OutputDebugStringW(m_buffer.str().c_str());
    }
    EnterCriticalSection(&gs_debug_print_cs);
    if (!gs_debuglogfile.is_open())
    {
        gs_debuglogfile.open("hourglasslog.txt", std::wifstream::out);
    }
    if (gs_debuglogfile.is_open())
    {
        gs_debuglogfile << m_buffer.str();
        gs_debuglogfile.flush();
    }
    LeaveCriticalSection(&gs_debug_print_cs);
}
