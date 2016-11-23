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

#include "DbgHelpPrivate.h"

class StackWalkHelper : public IDiaStackWalkHelper
{
public:
    StackWalkHelper(const DbgHelpPrivate* priv, const CONTEXT& context);

    /*
    * From IUnknown, overload these to turn this class into a regular C++ object.
    */
    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE get_registerValue(
        /* [in] */ DWORD index,
        /* [retval][out] */ ULONGLONG *register_value);
    HRESULT STDMETHODCALLTYPE put_registerValue(
        /* [in] */ DWORD index,
        /* [in] */ ULONGLONG register_value);
    HRESULT STDMETHODCALLTYPE readMemory(
        /* [in] */ enum MemoryTypeEnum type,
        /* [in] */ ULONGLONG virtual_address,
        /* [in] */ DWORD buffer_length,
        /* [out] */ DWORD *bytes_read,
        /* [size_is][out] */ BYTE *buffer);
    HRESULT STDMETHODCALLTYPE searchForReturnAddress(
        /* [in] */ IDiaFrameData *frame,
        /* [out] */ ULONGLONG *return_address);
    HRESULT STDMETHODCALLTYPE searchForReturnAddressStart(
        /* [in] */ IDiaFrameData *frame,
        /* [in] */ ULONGLONG start_address,
        /* [out] */ ULONGLONG *return_address);
    HRESULT STDMETHODCALLTYPE frameForVA(
        /* [in] */ ULONGLONG virtual_address,
        /* [out] */ IDiaFrameData **frame);
    HRESULT STDMETHODCALLTYPE symbolForVA(
        /* [in] */ ULONGLONG virtual_address,
        /* [out] */ IDiaSymbol **symbol);
    HRESULT STDMETHODCALLTYPE pdataForVA(
        /* [in] */ ULONGLONG virtual_address,
        /* [in] */ DWORD data_length,
        /* [out] */ DWORD *bytes_read,
        /* [size_is][out] */ BYTE *buffer);
    HRESULT STDMETHODCALLTYPE imageForVA(
        /* [in] */ ULONGLONG virtual_address_context,
        /* [out] */ ULONGLONG *virtual_address_image_start);
    HRESULT STDMETHODCALLTYPE addressForVA(
        /* [in] */ ULONGLONG va,
        /* [out] */ DWORD *pISect,
        /* [out] */ DWORD *pOffset);
    HRESULT STDMETHODCALLTYPE numberOfFunctionFragmentsForVA(
        /* [in] */ ULONGLONG vaFunc,
        /* [in] */ DWORD cbFunc,
        /* [out] */ DWORD *pNumFragments);
    HRESULT STDMETHODCALLTYPE functionFragmentsForVA(
        /* [in] */ ULONGLONG vaFunc,
        /* [in] */ DWORD cbFunc,
        /* [in] */ DWORD cFragments,
        /* [out] */ ULONGLONG *pVaFragment,
        /* [out] */ DWORD *pLenFragment);

private:
    const DbgHelpPrivate* m_priv;
    CONTEXT m_thread_context;
};
