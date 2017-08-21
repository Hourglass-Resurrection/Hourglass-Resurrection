/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include "Arguments.h"

namespace Arguments
{
    namespace
    {
        void ParseInternal(int argc, LPWSTR argv[])
        {
            for (int i = 0; i < argc; ++i)
            {
                std::wstring arg = argv[i];

                if (arg == L"--play")
                {
                    g_args.m_play = true;
                }
                else if (arg == L"--quit-on-movie-end")
                {
                    g_args.m_quit_on_movie_end = true;
                }
                else if (arg == L"--ignore-md5")
                {
                    g_args.m_ignore_md5 = true;
                }
                else if (arg == L"--game" && i + 1 < argc)
                {
                    g_args.m_game_filename = argv[++i];
                }
                else if (arg == L"--movie" && i + 1 < argc)
                {
                    g_args.m_movie_filename = argv[++i];
                }
            }
        }
    }

    Arguments g_args{};

    void Parse(LPCWSTR lpCmdLine)
    {
        int argc;
        LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);

        if (argv)
        {
            ParseInternal(argc, argv);
            LocalFree(argv);
        }
    }
}
