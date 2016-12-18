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

HRESULT DbgHelpLoadCallback::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    if (riid == __uuidof(IDiaLoadCallback))
    {
        *ppvObject = this;
        return S_OK;
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

HRESULT DbgHelpLoadCallback::NotifyDebugDir(BOOL executable, DWORD data_length, BYTE* data)
{
    debugprintf(L"[Hourglass][DebugSymbols] Found debug directory in %s, length=%d\n", executable ? L"executable" : L"dbg-file", data_length);
    return S_OK;
}

HRESULT DbgHelpLoadCallback::NotifyOpenDBG(LPCOLESTR dbg_path, HRESULT result_code)
{
    if (result_code == S_OK)
    {
        debugprintf(L"[Hourglass][DebugSymbols] Loaded dbg-file from \"%s\"\n", dbg_path);
    }
    return S_OK;
}

HRESULT DbgHelpLoadCallback::NotifyOpenPDB(LPCOLESTR pdb_path, HRESULT result_code)
{
    if (result_code == S_OK)
    {
        debugprintf(L"[Hourglass][DebugSymbols] Loaded pdb-file from \"%s\"\n", pdb_path);
    }
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
