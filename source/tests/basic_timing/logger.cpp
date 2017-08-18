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
