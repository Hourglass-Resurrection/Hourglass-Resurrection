/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
 * Default location of the DIA SDK within VS2015 Community edition.
 */
#include <../../DIA SDK/include/dia2.h>

#include "../logging.h"

#include "DbgHelpLoadCallback.h"

HRESULT DbgHelpLoadCallback::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG DbgHelpLoadCallback::AddRef()
{
    return 0;
}

ULONG DbgHelpLoadCallback::Release()
{
    return 0;
}

HRESULT DbgHelpLoadCallback::NotifyDebugDir(BOOL executable, DWORD data_length, BYTE * data)
{
    debugprintf(L"Found Debug Directory in %s, length=%d\n", executable ? L"executable" : L"dbg-file", data_length);
    return S_OK;
}

HRESULT DbgHelpLoadCallback::NotifyOpenDBG(LPCOLESTR dbg_path, HRESULT result_code)
{
    debugprintf(L"Loading dbg-file from \"%s\", result_code=0x%X\n", dbg_path, result_code);
    return S_OK;
}

HRESULT DbgHelpLoadCallback::NotifyOpenPDB(LPCOLESTR pdb_path, HRESULT result_code)
{
    debugprintf(L"Loading pdb-file from \"%s\", result_code=0x%X\n", pdb_path, result_code);
    return S_OK;
}

HRESULT DbgHelpLoadCallback::RestrictRegistryAccess()
{
    return S_OK;
}

HRESULT DbgHelpLoadCallback::RestrictSymbolServerAccess()
{
    return S_OK;
}
