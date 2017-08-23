/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "logger.h"

namespace Logger
{
    namespace
    {
        std::wofstream gs_file(L"output.log");
    }

    void Write(const std::wstring& line)
    {
        gs_file << line;
    }

    void WriteLine(const std::wstring& line)
    {
        gs_file << line << '\n';
    }
}
