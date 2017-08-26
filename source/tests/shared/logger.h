/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Logger
{
    void Write(const std::wstring& line);
    void WriteLine(const std::wstring& line);
    void WriteBytes(const std::uint8_t* bytes, size_t size);

    template<typename T>
    void WriteBytes(const std::vector<T>& bytes)
    {
        WriteBytes(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size() * sizeof(T));
    }
}
