/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

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

    void WriteBytes(const std::uint8_t* bytes, size_t size)
    {
        std::wostringstream oss;
        oss.setf(std::ios_base::hex, std::ios_base::basefield);
        oss.fill(L'0');

        size_t lines = size / 16;

        for (size_t i = 0; i < lines; ++i)
        {
            const uint8_t* current_line = &bytes[i * 16];

            oss << std::setw(2) << static_cast<int>(current_line[0]);

            for (size_t j = 1; j < 8; ++j)
            {
                oss << L' ' << std::setw(2) << static_cast<int>(current_line[j]);
            }

            oss << L' ';

            for (size_t j = 8; j < 16; ++j)
            {
                oss << L' ' << std::setw(2) << static_cast<int>(current_line[j]);
            }

            oss << L'\n';
        }

        const size_t remaining_size = size - lines * 16;
        if (remaining_size > 0)
        {
            /*
             * Print the last (incomplete) line.
             */
            const uint8_t* current_line = &bytes[lines * 16];

            oss << std::setw(2) << static_cast<int>(current_line[0]);

            for (size_t j = 1; j < std::min<size_t>(remaining_size, 8); ++j)
            {
                oss << L' ' << std::setw(2) << static_cast<int>(current_line[j]);
            }

            if (remaining_size > 8)
            {
                oss << L' ';

                for (size_t j = 8; j < remaining_size; ++j)
                {
                    oss << L' ' << std::setw(2) << static_cast<int>(current_line[j]);
                }
            }

            oss << L'\n';
        }

        Logger::Write(oss.str());
    }
}
