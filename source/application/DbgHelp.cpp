/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <memory>
#include <vector>

/*
 * Default location of the DIA SDK within VS2015 Community edition.
 */
#include <../../DIA SDK/include/dia2.h>
#pragma comment(lib, "../../DIA SDK/lib/diaguids.lib")

#include "DbgHelp.h"

/*
 * TODO: Break up into several files, create directory!
 * -- Warepire
 */

namespace
{
    /*
     * Custom unique_ptr deleter for COM objects.
     * TODO: Put in some sort of utils header / namespace?
     */
    template<class T>
    class COMObjectDeleter
    {
    public:
        void operator()(T* ptr)
        {
            ptr->Release();
        }
    };

    template<class T>
    using UniqueCOMPtr = std::unique_ptr<T, COMObjectDeleter<T>>;
}

class StackWalkHelper : public IDiaStackWalkHelper
{
public:
    StackWalkHelper(HANDLE process, const CONTEXT& context);

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
    HANDLE m_process;
    CONTEXT m_thread_context;
};

StackWalkHelper::StackWalkHelper(HANDLE process, const CONTEXT& context) :
    m_process(process)
{
    memcpy(&m_thread_context, &context, sizeof(CONTEXT));
}

HRESULT StackWalkHelper::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG StackWalkHelper::AddRef()
{
    return 0;
}

ULONG StackWalkHelper::Release()
{
    return 0;
}

HRESULT StackWalkHelper::get_registerValue(DWORD index, ULONGLONG* register_value)
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

HRESULT StackWalkHelper::put_registerValue(DWORD index, ULONGLONG register_value)
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

HRESULT StackWalkHelper::readMemory(MemoryTypeEnum type, ULONGLONG virtual_address, DWORD buffer_length, DWORD* bytes_read, BYTE* buffer)
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
    if (ReadProcessMemory(m_process, reinterpret_cast<LPCVOID>(virtual_address), buffer, buffer_length, bytes_read) == FALSE)
    {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT StackWalkHelper::searchForReturnAddress(IDiaFrameData* frame, ULONGLONG* return_address)
{
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


    if ((ReadProcessMemory(m_process, reinterpret_cast<LPCVOID>(return_address_location),
                          return_address, sizeof(DWORD), &read_bytes) == FALSE) ||
        read_bytes != sizeof(DWORD))
    {
        *return_address = 0;
        return S_FALSE;
    }

    return S_OK;
}

HRESULT StackWalkHelper::searchForReturnAddressStart(IDiaFrameData* frame, ULONGLONG start_address, ULONGLONG* return_address)
{
    /*
     * Keep this unimplemented for now.
     */
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::frameForVA(ULONGLONG virtual_address, IDiaFrameData** frame)
{
    IDiaEnumFrameData* enumerator;

    return E_NOTIMPL;
}

HRESULT StackWalkHelper::symbolForVA(ULONGLONG virtual_address, IDiaSymbol** symbol)
{
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::pdataForVA(ULONGLONG virtual_address, DWORD data_length, DWORD* bytes_read, BYTE* buffer)
{
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::imageForVA(ULONGLONG virtual_address_context, ULONGLONG* virtual_address_image_start)
{
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::addressForVA(ULONGLONG va, DWORD* pISect, DWORD* pOffset)
{
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::numberOfFunctionFragmentsForVA(ULONGLONG vaFunc, DWORD cbFunc, DWORD* pNumFragments)
{
    return E_NOTIMPL;
}

HRESULT StackWalkHelper::functionFragmentsForVA(ULONGLONG vaFunc, DWORD cbFunc, DWORD cFragments, ULONGLONG * pVaFragment, DWORD * pLenFragment)
{
    return E_NOTIMPL;
}

class DbgHelpPriv
{
public:
    DbgHelpPriv();
    bool LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path);
    bool Stacktrace(HANDLE thread, INT max_depth, std::vector<DbgHelp::StackFrameInfo>* stack);
private:
    std::map<DWORD64, UniqueCOMPtr<IDiaDataSource>> m_sources;
    std::map<IDiaDataSource*, UniqueCOMPtr<IDiaSession>> m_sessions;
    std::map<IDiaSession*, UniqueCOMPtr<IDiaSymbol>> m_symbols;
    /*
     * Assume everything is compiled for the same platform.
     */
    CV_CPU_TYPE_e m_platform;
    bool m_platform_set;
};

DbgHelpPriv::DbgHelpPriv() :
    m_platform_set(false)
{
}

bool DbgHelpPriv::LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path)
{
    IDiaDataSource* source = nullptr;
    IDiaSession* sess = nullptr;
    IDiaSymbol* sym = nullptr;

    if (CoCreateInstance(CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER,
                         __uuidof(IDiaDataSource), reinterpret_cast<LPVOID*>(&source)) != S_OK)
    {
        return false;
    }

    /*
     * Now the pointer is valid, so lets turn it into a unique_ptr so that we can be sure we
     * destroy it.
     */
    UniqueCOMPtr<IDiaDataSource> data_source(source);
    source = nullptr;

    if (data_source->loadDataForExe(exec.c_str(), search_path.c_str(), nullptr) != S_OK)
    {
        return false;
    }

    if (data_source->openSession(&sess) != S_OK)
    {
        return false;
    }

    UniqueCOMPtr<IDiaSession> session(sess);
    sess = nullptr;

    if (session->get_globalScope(&sym) != S_OK)
    {
        return false;
    }

    UniqueCOMPtr<IDiaSymbol> symbol(sym);
    sym = nullptr;

    if (!m_platform_set)
    {
        DWORD platform;
        if (symbol->get_platform(&platform) != S_OK)
        {
            return false;
        }
        m_platform_set = true;
        m_platform = static_cast<CV_CPU_TYPE_e>(platform);
    }
    m_sources.emplace(module_base, std::move(data_source));
    m_sessions.emplace(data_source.get(), std::move(session));
    m_symbols.emplace(session.get(), std::move(symbol));

    return true;
}

bool DbgHelpPriv::Stacktrace(HANDLE thread, INT max_depth, std::vector<DbgHelp::StackFrameInfo>* trace)
{
    IDiaStackWalker* walker = nullptr;
    IDiaEnumStackFrames* frames = nullptr;
    CONTEXT thread_context;
    thread_context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS;
    if (!m_platform_set)
    {
        return false;
    }
    if (GetThreadContext(thread, &thread_context) == FALSE)
    {
        return false;
    }
    if (CoCreateInstance(CLSID_DiaStackWalker, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IDiaStackWalker, reinterpret_cast<LPVOID*>(&walker)) != S_OK)
    {
        return false;
    }
    UniqueCOMPtr<IDiaStackWalker> stack_walker(walker);
    walker = nullptr;

    if (stack_walker->getEnumFrames2(m_platform, nullptr, &frames) != S_OK)
    {
        return false;
    }
    UniqueCOMPtr<IDiaEnumStackFrames> stack_frames(frames);
    frames = nullptr;


    return true;
}

DbgHelp::DbgHelp(HANDLE process) :
    m_process(process),
    m_private(std::make_unique<DbgHelpPriv>())
{
    std::array<WCHAR, 0x1000> buffer;
    buffer.fill('\0');

    if (GetCurrentDirectoryW(buffer.size(), buffer.data()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_ALTERNATIVE_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"SYSTEMROOT", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data())
                      .append(L";").append(buffer.data()).append(L"\\system32")
                      .append(L";").append(buffer.data()).append(L"\\SysWOW64");
    }
    /*
    * TODO: Enable when SymbolServer support is added. Needs symsrv.dll.
    * This will enable downloading of symbols for the system DLLs making stack traces more accurate.
    * -- Warepire
    */
    //symbol_paths.append(";").append("SRV*%SYSTEMDRIVE%\\websymbols*http://msdl.microsoft.com/download/symbols");
}

DbgHelp::~DbgHelp()
{

}

void DbgHelp::LoadSymbols(HANDLE module_file, LPCWSTR module_name, DWORD64 module_base)
{
    if (m_private->LoadSymbols(module_base, module_name, m_symbol_paths))
    {
        m_loaded_modules.emplace(module_base, module_name);
    }
}

std::vector<DbgHelp::StackFrameInfo> DbgHelp::Stacktrace(HANDLE thread, INT max_depth)
{
    std::vector<DbgHelp::StackFrameInfo> trace;
    IDiaStackWalker* walker = nullptr;

    if (m_loaded_modules.empty())
    {
        return trace;
    }



    //stack_frame.AddrPC.Offset = thread_context.Eip;
    //stack_frame.AddrPC.Mode = AddrModeFlat;
    //stack_frame.AddrStack.Offset = thread_context.Esp;
    //stack_frame.AddrStack.Mode = AddrModeFlat;
    //stack_frame.AddrFrame.Offset = thread_context.Ebp;
    //stack_frame.AddrFrame.Mode = AddrModeFlat;

    //for (INT i = 0; ; i++)
    //{
    //    BOOL rv = StackWalk64Pointer(IMAGE_FILE_MACHINE_I386, m_process, thread,
    //                                 &stack_frame, &thread_context, nullptr,
    //                                 SymFunctionTableAccess64Pointer, SymGetModuleBase64Pointer,
    //                                 nullptr);
    //    if (rv == FALSE)
    //    {
    //        break;
    //    }
    //    DWORD64 address = stack_frame.AddrPC.Offset;

    //    /*
    //     * lower_bound() will return an iterator to the location in the map where 'address' should
    //     * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
    //     * module 'address' belongs to by getting this iterator, and then iterate backwards once.
    //     */
    //    auto module_it = m_loaded_modules.lower_bound(address);
    //    module_it--;
    //    trace.emplace_back(address, module_it->second);

    //    if (i >= max_depth)
    //    {
    //        break;
    //    }
    //}
    return trace;
}

//std::vector<std::wstring> DbgHelp::GetFunctionTrace(const std::vector<StacktraceInfo>& trace)
//{
//    std::vector<std::wstring> functions(trace.size());
//    DWORD64 mod_address;
//    ULONG type_index;
//
//    for (auto& i = trace.begin(); i != trace.end(); i++)
//    {
//        if (GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
//            GetSymbolTag(mod_address, type_index) == SymTagFunction)
//        {
//            functions.emplace_back(GetSymbolName(mod_address, type_index));
//        }
//        else
//        {
//            functions.emplace_back(L"?");
//        }
//    }
//    return functions;
//}

/*std::vector<std::wstring> DbgHelp::GetFullTrace(const std::vector<StacktraceInfo>& trace)
{
    std::vector<std::wstring> full_trace(trace.size());

    DWORD64 mod_address;
    ULONG type_index;

    for (auto& i = trace.begin(); i != trace.end(); i++)
    {
        if (!GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
            GetSymbolTag(mod_address, type_index) != SymTagFunction)
        {
            full_trace.emplace_back(L"?");
            continue;
        }

        std::wstring symbol;
        DWORD params = GetParamCountForClass(mod_address, type_index);
        DWORD class_index = GetParentClass(mod_address, type_index);
        if (params == GetParamCount(mod_address, type_index) && class_index != 0)
        {
            symbol.append(L"static ");
        }
        symbol.append(GetTypeName(mod_address, type_index));
        if (class_index != 0)
        {
            symbol.append(GetSymbolName(mod_address, class_index)).append(L"::");
        }
        symbol.append(GetSymbolName(mod_address, type_index)).append(L"(");

        full_trace.emplace_back(symbol);
    }

    return full_trace;
}*/
