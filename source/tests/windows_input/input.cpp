/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "shared/logger.h"

namespace Input
{
    namespace
    {
        unsigned int gs_frame_counter = 0;

        void DumpGetKeyState()
        {
            std::vector<SHORT> state(256);

            for (size_t i = 0; i < 256; ++i)
            {
                state[i] = GetKeyState(i);
            }

            Logger::WriteLine(L"    GetKeyState:");
            Logger::WriteBytes(state);
        }

        void DumpGetKeyboardState()
        {
            std::vector<BYTE> state(256);
            GetKeyboardState(state.data());

            Logger::WriteLine(L"    GetKeyboardState:");
            Logger::WriteBytes(state);
        }

        void DumpGetAsyncKeyState()
        {
            std::vector<SHORT> state(256);

            for (size_t i = 0; i < 256; ++i)
            {
                state[i] = GetAsyncKeyState(i);
            }

            Logger::WriteLine(L"    GetAsyncKeyState:");
            Logger::WriteBytes(state);
        }
    }

    void PreRender()
    {
        ++gs_frame_counter;

        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" pre render:");

        DumpGetAsyncKeyState();
        DumpGetKeyState();
        DumpGetKeyboardState();

        Logger::WriteLine(L"");
    }

    void PostRender()
    {
        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" post render:");

        DumpGetAsyncKeyState();
        DumpGetKeyState();
        DumpGetKeyboardState();

        Logger::WriteLine(L"");
    }
}
