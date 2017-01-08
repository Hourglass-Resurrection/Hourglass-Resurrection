/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atlbase.h>
#include <atlcom.h>
/*
 * Default location of the DIA SDK within VS2015 Community edition.
 */
#include <../../DIA SDK/include/dia2.h>

#include "../logging.h"
#include "DbgHelpPrivate.h"

#include "DbgHelpStackWalkHelper.h"

namespace
{
    /*
     * Helper template function for getting a specific IDiaTable from an IDiaSession.
     */
    template<class T>
    CComPtr<T> GetDiaTable(CComPtr<IDiaSession> session)
    {
        CComPtr<IDiaEnumTables> enum_tables;
        IDiaTable* table;
        CComPtr<T> desired_table;
        ULONG num_tables_found;
        if (session->getEnumTables(&enum_tables) != S_OK)
        {
            return nullptr;
        }

        while (enum_tables->Next(1, &table, &num_tables_found) == S_OK && num_tables_found == 1)
        {
            HRESULT rv = table->QueryInterface(__uuidof(T), reinterpret_cast<LPVOID*>(&desired_table));
            table->Release();
            if (rv == S_OK)
            {
                break;
            }
        }
        return desired_table;
    }
}

DbgHelpStackWalkHelper::DbgHelpStackWalkHelper(const DbgHelpPrivate* priv, const CONTEXT& context) :
    m_priv(priv)
{
    memcpy(&m_thread_context, &context, sizeof(CONTEXT));
}

HRESULT DbgHelpStackWalkHelper::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    if (riid == __uuidof(IDiaStackWalkHelper))
    {
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG DbgHelpStackWalkHelper::AddRef()
{
    return 0;
}

ULONG DbgHelpStackWalkHelper::Release()
{
    return 0;
}

HRESULT DbgHelpStackWalkHelper::get_registerValue(DWORD index, ULONGLONG* register_value)
{
    if (register_value == nullptr)
    {
        return E_POINTER;
    }

    switch (index)
    {
    case CV_REG_AL:
        *register_value = LOBYTE(static_cast<WORD>(m_thread_context.Eax));
        break;
    case CV_REG_CL:
        *register_value = LOBYTE(static_cast<WORD>(m_thread_context.Ecx));
        break;
    case CV_REG_DL:
        *register_value = LOBYTE(static_cast<WORD>(m_thread_context.Edx));
        break;
    case CV_REG_BL:
        *register_value = LOBYTE(static_cast<WORD>(m_thread_context.Ebx));
        break;
    case CV_REG_AH:
        *register_value = HIBYTE(static_cast<WORD>(m_thread_context.Eax));
        break;
    case CV_REG_CH:
        *register_value = HIBYTE(static_cast<WORD>(m_thread_context.Ecx));
        break;
    case CV_REG_DH:
        *register_value = HIBYTE(static_cast<WORD>(m_thread_context.Edx));
        break;
    case CV_REG_BH:
        *register_value = HIBYTE(static_cast<WORD>(m_thread_context.Ebx));
        break;
    case CV_REG_AX:
        *register_value = static_cast<WORD>(m_thread_context.Eax);
        break;
    case CV_REG_CX:
        *register_value = static_cast<WORD>(m_thread_context.Ecx);
        break;
    case CV_REG_DX:
        *register_value = static_cast<WORD>(m_thread_context.Edx);
        break;
    case CV_REG_BX:
        *register_value = static_cast<WORD>(m_thread_context.Ebx);
        break;
    case CV_REG_SP:
        *register_value = static_cast<WORD>(m_thread_context.Esp);
        break;
    case CV_REG_BP:
        *register_value = static_cast<WORD>(m_thread_context.Ebp);
        break;
    case CV_REG_SI:
        *register_value = static_cast<WORD>(m_thread_context.Esi);
        break;
    case CV_REG_DI:
        *register_value = static_cast<WORD>(m_thread_context.Edi);
        break;
    case CV_REG_EAX:
        *register_value = m_thread_context.Eax;
        break;
    case CV_REG_ECX:
        *register_value = m_thread_context.Ecx;
        break;
    case CV_REG_EDX:
        *register_value = m_thread_context.Edx;
        break;
    case CV_REG_EBX:
        *register_value = m_thread_context.Ebx;
        break;
    case CV_REG_ESP:
        *register_value = m_thread_context.Esp;
        break;
    case CV_REG_EBP:
        *register_value = m_thread_context.Ebp;
        break;
    case CV_REG_ESI:
        *register_value = m_thread_context.Esi;
        break;
    case CV_REG_EDI:
        *register_value = m_thread_context.Edi;
        break;
    case CV_REG_ES:
        *register_value = m_thread_context.SegEs;
        break;
    case CV_REG_CS:
        *register_value = m_thread_context.SegCs;
        break;
    case CV_REG_SS:
        *register_value = m_thread_context.SegSs;
        break;
    case CV_REG_DS:
        *register_value = m_thread_context.SegDs;
        break;
    case CV_REG_FS:
        *register_value = m_thread_context.SegFs;
        break;
    case CV_REG_GS:
        *register_value = m_thread_context.SegGs;
        break;
    case CV_REG_IP:
        *register_value = static_cast<WORD>(m_thread_context.Eip);
        break;
    case CV_REG_FLAGS:
        *register_value = static_cast<WORD>(m_thread_context.EFlags);
        break;
    case CV_REG_EIP:
        *register_value = m_thread_context.Eip;
        break;
    case CV_REG_EFLAGS:
        *register_value = m_thread_context.EFlags;
        break;
    case CV_REG_CR0:
    case CV_REG_CR1:
    case CV_REG_CR2:
    case CV_REG_CR3:
    case CV_REG_CR4:
    case CV_REG_DR0:
    case CV_REG_DR1:
    case CV_REG_DR2:
    case CV_REG_DR3:
    case CV_REG_DR4:
    case CV_REG_DR5:
    case CV_REG_DR6:
    case CV_REG_DR7:
        return E_FAIL;
    default:
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT DbgHelpStackWalkHelper::put_registerValue(DWORD index, ULONGLONG register_value)
{
    static constexpr DWORD LOBYTE_MASK = MAXDWORD - MAXBYTE;
    static constexpr DWORD HIBYTE_MASK = MAXDWORD - (MAXWORD - MAXBYTE);
    static constexpr DWORD WORD_MASK = MAXDWORD - MAXWORD;
    switch (index)
    {
        /*
        * TODO: Cast register_value?
        * -- Warepire
        */
    case CV_REG_AL:
        m_thread_context.Eax = (m_thread_context.Eax & LOBYTE_MASK) + (register_value & MAXBYTE);
        break;
    case CV_REG_CL:
        m_thread_context.Ecx = (m_thread_context.Ecx & LOBYTE_MASK) + (register_value & MAXBYTE);
        break;
    case CV_REG_DL:
        m_thread_context.Edx = (m_thread_context.Edx & LOBYTE_MASK) + (register_value & MAXBYTE);
        break;
    case CV_REG_BL:
        m_thread_context.Ebx = (m_thread_context.Ebx & LOBYTE_MASK) + (register_value & MAXBYTE);
        break;
    case CV_REG_AH:
        m_thread_context.Eax = (m_thread_context.Eax & HIBYTE_MASK) + ((register_value & MAXBYTE) << 8);
        break;
    case CV_REG_CH:
        m_thread_context.Ecx = (m_thread_context.Ecx & HIBYTE_MASK) + ((register_value & MAXBYTE) << 8);
        break;
    case CV_REG_DH:
        m_thread_context.Edx = (m_thread_context.Edx & HIBYTE_MASK) + ((register_value & MAXBYTE) << 8);
        break;
    case CV_REG_BH:
        m_thread_context.Ebx = (m_thread_context.Ebx & HIBYTE_MASK) + ((register_value & MAXBYTE) << 8);
        break;
    case CV_REG_AX:
        m_thread_context.Eax = (m_thread_context.Eax & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_CX:
        m_thread_context.Ecx = (m_thread_context.Ecx & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_DX:
        m_thread_context.Edx = (m_thread_context.Edx & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_BX:
        m_thread_context.Ebx = (m_thread_context.Ebx & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_SP:
        m_thread_context.Esp = (m_thread_context.Esp & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_BP:
        m_thread_context.Ebp = (m_thread_context.Ebp & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_SI:
        m_thread_context.Esi = (m_thread_context.Esi & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_DI:
        m_thread_context.Edi = (m_thread_context.Edi & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_EAX:
        m_thread_context.Eax = (register_value & MAXDWORD);
        break;
    case CV_REG_ECX:
        m_thread_context.Ecx = (register_value & MAXDWORD);
        break;
    case CV_REG_EDX:
        m_thread_context.Edx = (register_value & MAXDWORD);
        break;
    case CV_REG_EBX:
        m_thread_context.Ebx = (register_value & MAXDWORD);
        break;
    case CV_REG_ESP:
        m_thread_context.Esp = (register_value & MAXDWORD);
        break;
    case CV_REG_EBP:
        m_thread_context.Ebp = (register_value & MAXDWORD);
        break;
    case CV_REG_ESI:
        m_thread_context.Esi = (register_value & MAXDWORD);
        break;
    case CV_REG_EDI:
        m_thread_context.Edi = (register_value & MAXDWORD);
        break;
    case CV_REG_ES:
        m_thread_context.SegEs = (register_value & MAXDWORD);
        break;
    case CV_REG_CS:
        m_thread_context.SegCs = (register_value & MAXDWORD);
        break;
    case CV_REG_SS:
        m_thread_context.SegSs = (register_value & MAXDWORD);
        break;
    case CV_REG_DS:
        m_thread_context.SegDs = (register_value & MAXDWORD);
        break;
    case CV_REG_FS:
        m_thread_context.SegFs = (register_value & MAXDWORD);
        break;
    case CV_REG_GS:
        m_thread_context.SegGs = (register_value & MAXDWORD);
        break;
    case CV_REG_IP:
        m_thread_context.Eip = (m_thread_context.Eip & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_FLAGS:
        m_thread_context.EFlags = (m_thread_context.EFlags & WORD_MASK) + (register_value & MAXWORD);
        break;
    case CV_REG_EIP:
        m_thread_context.Eip = (register_value & MAXDWORD);
        break;
    case CV_REG_EFLAGS:
        m_thread_context.EFlags = (register_value & MAXDWORD);
        break;
    case CV_REG_CR0:
    case CV_REG_CR1:
    case CV_REG_CR2:
    case CV_REG_CR3:
    case CV_REG_CR4:
    case CV_REG_DR0:
    case CV_REG_DR1:
    case CV_REG_DR2:
    case CV_REG_DR3:
    case CV_REG_DR4:
    case CV_REG_DR5:
    case CV_REG_DR6:
    case CV_REG_DR7:
        return E_FAIL;
    default:
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT DbgHelpStackWalkHelper::readMemory(MemoryTypeEnum type, ULONGLONG virtual_address,
                                           DWORD buffer_length, DWORD* bytes_read, BYTE* buffer)
{
    /*
     * We do not limit the access to memory depending on type.
     * TODO: Can some memory have read access disabled?
     * -- Warepire
     */
    if (bytes_read == nullptr)
    {
        return E_POINTER;
    }
    if (ReadProcessMemory(m_priv->GetProcess(), reinterpret_cast<LPCVOID>(virtual_address),
                          buffer, buffer_length, bytes_read) == FALSE)
    {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT DbgHelpStackWalkHelper::searchForReturnAddress(IDiaFrameData* frame, ULONGLONG* return_address)
{
    return E_NOTIMPL;

    SIZE_T read_bytes;
    DWORD return_address_location;
    DWORD length_pushed_params;
    DWORD length_pushed_regs;
    BOOL has_entrypoint = FALSE;
    if (frame == nullptr || return_address == nullptr)
    {
        return E_POINTER;
    }
    if (frame->get_functionStart(&has_entrypoint) != S_OK || has_entrypoint == FALSE)
    {
        *return_address = 0;
        return S_FALSE;
    }
    if (frame->get_lengthParams(&length_pushed_params) != S_OK)
    {
        *return_address = 0;
        return S_FALSE;
    }
    if (frame->get_lengthSavedRegisters(&length_pushed_regs) != S_OK)
    {
        *return_address = 0;
        return S_FALSE;
    }

    return_address_location = m_thread_context.Ebp + length_pushed_params;
    /*
     * A pushed CS register is always 16 bits (2 bytes), any other pushed register will be
     * a multiple of 4 bytes.
     */
    if ((length_pushed_regs % 4) != 0)
    {
        return_address_location += 2;
    }


    if ((ReadProcessMemory(m_priv->GetProcess(), reinterpret_cast<LPCVOID>(return_address_location),
                           return_address, sizeof(DWORD), &read_bytes) == FALSE) ||
        read_bytes != sizeof(DWORD))
    {
        *return_address = 0;
        return S_FALSE;
    }

    return S_OK;
}

HRESULT DbgHelpStackWalkHelper::searchForReturnAddressStart(IDiaFrameData* frame, ULONGLONG start_address, ULONGLONG* return_address)
{
    /*
     * Keep this unimplemented for now.
     */
    return E_NOTIMPL;
}

HRESULT DbgHelpStackWalkHelper::frameForVA(ULONGLONG virtual_address, IDiaFrameData** frame)
{
    auto data = m_priv->GetModuleData(virtual_address);
    if (data == nullptr || data->m_module_symbol_session == nullptr)
    {
        return E_FAIL;
    }

    auto enum_frame_data = GetDiaTable<IDiaEnumFrameData>(data->m_module_symbol_session);
    if (enum_frame_data == nullptr)
    {
        return E_FAIL;
    }

    return enum_frame_data->frameByVA(virtual_address, frame);
}

HRESULT DbgHelpStackWalkHelper::symbolForVA(ULONGLONG virtual_address, IDiaSymbol** symbol)
{
    /*
     * TODO: Register found symbol if it's a function-tag?
     */
    HRESULT rv;
    CComPtr<IDiaEnumSymbolsByAddr> symbols_by_addr;
    auto data = m_priv->GetModuleData(virtual_address);
    if (data == nullptr || data->m_module_symbol_session == nullptr)
    {
        return E_FAIL;
    }

    rv = data->m_module_symbol_session->getSymbolsByAddr(&symbols_by_addr);
    if (rv != S_OK)
    {
        return rv;
    }

    rv = symbols_by_addr->symbolByVA(virtual_address, symbol);
    if (rv != S_OK)
    {
        return rv;
    }

    return rv;
}

HRESULT DbgHelpStackWalkHelper::pdataForVA(ULONGLONG virtual_address, DWORD data_length, DWORD* bytes_read, BYTE* buffer)
{
    /*
     * 64-bit specific
     * TODO: Implement when 64-bit is supported.
     */
    return E_NOTIMPL;
}

HRESULT DbgHelpStackWalkHelper::imageForVA(ULONGLONG virtual_address_context, ULONGLONG* virtual_address_image_start)
{
    auto data = m_priv->GetModuleData(virtual_address_context);
    if (data == nullptr || data->m_module_symbol_session == nullptr)
    {
        return S_FALSE;
    }
    return data->m_module_symbol_session->get_loadAddress(virtual_address_image_start);
}

/*
 * TODO: What are these meant to do?
 * These have literally ZERO documentation. According to Microsoft they "don't even exist",
 * except for in the DIA SDK header, but no explanations given there...
 * -- Warepire
 */
HRESULT DbgHelpStackWalkHelper::addressForVA(ULONGLONG va, DWORD* pISect, DWORD* pOffset)
{
    return E_NOTIMPL;
}

HRESULT DbgHelpStackWalkHelper::numberOfFunctionFragmentsForVA(ULONGLONG vaFunc, DWORD cbFunc, DWORD* pNumFragments)
{
    return E_NOTIMPL;
}

HRESULT DbgHelpStackWalkHelper::functionFragmentsForVA(ULONGLONG vaFunc, DWORD cbFunc, DWORD cFragments, ULONGLONG * pVaFragment, DWORD * pLenFragment)
{
    return E_NOTIMPL;
}
