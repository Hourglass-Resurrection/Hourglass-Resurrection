/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <DIA SDK/include/dia2.h>

class DbgHelpLoadCallback : public IDiaLoadCallback
{
    /*
     * From IUnknown, overload these to turn this class into a regular C++ object.
     */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                             _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE NotifyDebugDir(BOOL executable, DWORD data_length, BYTE* data);

    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(LPCOLESTR dbg_path, HRESULT result_code);

    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(LPCOLESTR pdb_path, HRESULT result_code);

    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess();

    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess();
};
