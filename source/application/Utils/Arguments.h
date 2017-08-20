/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <optional>
#include <string>

namespace Arguments
{
    struct Arguments
    {
        std::optional<std::wstring> m_game_filename;
        std::optional<std::wstring> m_movie_filename;
        bool m_play;
        bool m_quit_on_movie_end;
        bool m_ignore_md5;
    };

    extern Arguments g_args;

    void Parse(LPCWSTR lpCmdLine);
}
