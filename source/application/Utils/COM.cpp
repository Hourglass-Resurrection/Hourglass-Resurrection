/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>

#include "COM.h"

bool Utils::COM::COMInstance::ms_initialized = false;

void Utils::COM::COMInstance::Init()
{
    static COMInstance s_com_instance;
}

Utils::COM::COMInstance::COMInstance()
{
    if (ms_initialized)
    {
        throw std::exception("COM library already initialized!");
    }

    HRESULT rv = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    if (rv != S_OK && rv != S_FALSE)
    {
        throw std::exception("COM library failed to initialize!");
    }

    ms_initialized = true;
}

Utils::COM::COMInstance::~COMInstance()
{
    if (ms_initialized)
    {
        CoUninitialize();
    }

    ms_initialized = false;
}
