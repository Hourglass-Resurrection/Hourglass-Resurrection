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

class DbgHelpLoadCallback : public IDiaLoadCallback
{
    /*
     * From IUnknown, overload these to turn this class into a regular C++ object.
     */
    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE NotifyDebugDir(
        /* [in] */ BOOL executable,
        /* [in] */ DWORD data_length,
        /* [size_is][in] */ BYTE *data);

    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(
        /* [in] */ LPCOLESTR dbg_path,
        /* [in] */ HRESULT result_code);

    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(
        /* [in] */ LPCOLESTR pdb_path,
        /* [in] */ HRESULT result_code);

    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess();

    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess();
};
