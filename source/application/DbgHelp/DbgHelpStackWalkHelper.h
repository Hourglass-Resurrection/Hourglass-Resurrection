/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <DIA SDK/include/dia2.h>

#include "DbgHelpPrivate.h"

class DbgHelpStackWalkHelper : public IDiaStackWalkHelper
{
public:
    DbgHelpStackWalkHelper(const DbgHelpPrivate* priv, const CONTEXT& context);

    /*
     * From IUnknown, overload these to turn this class into a regular C++ object.
     */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                             _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE get_registerValue(DWORD index, ULONGLONG* register_value);
    HRESULT STDMETHODCALLTYPE put_registerValue(DWORD index, ULONGLONG register_value);
    HRESULT STDMETHODCALLTYPE readMemory(enum MemoryTypeEnum type,
                                         ULONGLONG virtual_address,
                                         DWORD buffer_length,
                                         DWORD* bytes_read,
                                         BYTE* buffer);
    HRESULT STDMETHODCALLTYPE searchForReturnAddress(IDiaFrameData* frame,
                                                     ULONGLONG* return_address);
    HRESULT STDMETHODCALLTYPE searchForReturnAddressStart(IDiaFrameData* frame,
                                                          ULONGLONG start_address,
                                                          ULONGLONG* return_address);
    HRESULT STDMETHODCALLTYPE frameForVA(ULONGLONG virtual_address, IDiaFrameData** frame);
    HRESULT STDMETHODCALLTYPE symbolForVA(ULONGLONG virtual_address, IDiaSymbol** symbol);
    HRESULT STDMETHODCALLTYPE pdataForVA(ULONGLONG virtual_address,
                                         DWORD data_length,
                                         DWORD* bytes_read,
                                         BYTE* buffer);
    HRESULT STDMETHODCALLTYPE imageForVA(ULONGLONG virtual_address_context,
                                         ULONGLONG* virtual_address_image_start);
    HRESULT STDMETHODCALLTYPE addressForVA(ULONGLONG va, DWORD* pISect, DWORD* pOffset);
    HRESULT STDMETHODCALLTYPE numberOfFunctionFragmentsForVA(ULONGLONG vaFunc,
                                                             DWORD cbFunc,
                                                             DWORD* pNumFragments);
    HRESULT STDMETHODCALLTYPE functionFragmentsForVA(ULONGLONG vaFunc,
                                                     DWORD cbFunc,
                                                     DWORD cFragments,
                                                     ULONGLONG* pVaFragment,
                                                     DWORD* pLenFragment);

private:
    const DbgHelpPrivate* m_priv;
    CONTEXT m_thread_context;
};
