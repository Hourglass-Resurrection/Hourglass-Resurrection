/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>

namespace Utils
{
    namespace Exceptions
    {
        class WindowsException : public std::runtime_error
        {
        public:
            WindowsException(DWORD code, LPCSTR message);
            DWORD code() const;
        private:
            DWORD m_code;
        };

        /*
         * This must be called for each thread.
         */
        void InitWindowsExceptionsHandler();
    }
}
