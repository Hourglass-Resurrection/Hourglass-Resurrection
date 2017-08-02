/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

namespace TrustedModule
{
    void OnLoad(DWORD process_id, const std::wstring& path);

    void OnUnload(DWORD process_id, const std::wstring& path);

    bool IsTrusted(DWORD process_id, const std::wstring& path);
}