/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>
#include <string>

#include "COM.h"

namespace Utils::COM
{
    bool COMInstance::ms_initialized = false;

    void COMInstance::Init()
    {
        static COMInstance s_com_instance;
    }

    COMInstance::COMInstance()
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

    COMInstance::~COMInstance()
    {
        if (ms_initialized)
        {
            CoUninitialize();
        }

        ms_initialized = false;
    }

    COMLibrary::COMLibrary() : m_handle(nullptr), m_dll_get_class_object(nullptr)
    {
    }

    COMLibrary::~COMLibrary()
    {
        if (m_handle)
        {
            FreeLibrary(m_handle);
        }
    }

    void COMLibrary::Load(LPCWSTR name)
    {
        if (m_handle)
        {
            throw std::exception("COM library already loaded!");
        }

        m_handle = LoadLibraryW(name);

        if (!m_handle)
        {
            DWORD error = GetLastError();
            throw std::exception(
                ("Failed to load the COM library: " + std::to_string(error)).c_str());
        }

        m_dll_get_class_object =
            reinterpret_cast<DllGetClassObject_t>(GetProcAddress(m_handle, "DllGetClassObject"));

        if (!m_dll_get_class_object)
        {
            throw std::exception("Failed to get the pointer to DllGetClassObject()!");
        }
    }
}
