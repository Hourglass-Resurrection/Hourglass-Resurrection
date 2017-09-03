/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "shared/ipc.h"

namespace Hooks
{
    extern bool watchForCLLApiNum;
    extern int cllApiNum;
    extern DllLoadInfos dllLoadInfos;

    //HOOK_FUNCTION_DECLARE(HMODULE WINAPI LoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
    HOOK_FUNCTION_DECLARE(NTSTATUS,
                          NTAPI,
                          LdrLoadDll,
                          PWCHAR PathToFile,
                          ULONG Flags,
                          struct _LSA_UNICODE_STRING* ModuleFileName,
                          PHANDLE ModuleHandle);
    //HOOK_FUNCTION_DECLARE(NTSTATUS NTAPI LdrUnloadDll(HANDLE ModuleAddress);

    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          CallNextHookEx,
                          HHOOK hhk,
                          int nCode,
                          WPARAM wParam,
                          LPARAM lParam);

    //HOOK_FUNCTION_DECLARE(BOOL WINAPI RegisterUserApiHook(HINSTANCE hInst, FARPROC func);

    HOOK_FUNCTION_DECLARE(VOID,
                          NTAPI,
                          KiUserCallbackDispatcher,
                          ULONG ApiNumber,
                          PVOID InputBuffer,
                          ULONG InputLength);

    HOOK_FUNCTION_DECLARE(HRESULT,
                          STDAPICALLTYPE,
                          CoCreateInstance,
                          REFCLSID rclsid,
                          LPUNKNOWN pUnkOuter,
                          DWORD dwClsContext,
                          REFIID riid,
                          LPVOID* ppv);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          STDAPICALLTYPE,
                          CoCreateInstanceEx,
                          REFCLSID Clsid,
                          LPUNKNOWN punkOuter,
                          DWORD dwClsCtx,
                          struct _COSERVERINFO* pServerInfo,
                          DWORD dwCount,
                          struct tagMULTI_QI* pResults);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          STDAPICALLTYPE,
                          CoGetClassObject,
                          REFCLSID rclsid,
                          DWORD dwClsContext,
                          LPVOID pvReserved,
                          REFIID riid,
                          LPVOID FAR* ppv);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          STDMETHODCALLTYPE,
                          IUnknown_QueryInterface_Proxy,
                          IUnknown __RPC_FAR* This,
                          REFIID riid,
                          void __RPC_FAR* __RPC_FAR* ppvObject);

    // not sure exactly where these belong but this seems close enough
    HOOK_FUNCTION_DECLARE(PVOID,
                          NTAPI,
                          RtlAllocateHeap,
                          PVOID HeapHandle,
                          ULONG Flags,
                          SIZE_T Size);
    HOOK_FUNCTION_DECLARE(PVOID,
                          NTAPI,
                          RtlCreateHeap,
                          ULONG Flags,
                          PVOID HeapBase,
                          SIZE_T ReserveSize,
                          SIZE_T CommitSize,
                          PVOID Lock,
                          struct RTL_HEAP_PARAMETERS* Parameters);
    HOOK_FUNCTION_DECLARE(PVOID, RPC_ENTRY, NdrAllocate, PMIDL_STUB_MESSAGE pStubMsg, size_t Len);
    HOOK_FUNCTION_DECLARE(void,
                          RPC_ENTRY,
                          NdrClientInitializeNew,
                          PRPC_MESSAGE pRpcMsg,
                          PMIDL_STUB_MESSAGE pStubMsg,
                          PMIDL_STUB_DESC pStubDescriptor,
                          unsigned int ProcNum);
    HOOK_FUNCTION_DECLARE(void,
                          RPC_ENTRY,
                          NdrClientInitialize,
                          PRPC_MESSAGE pRpcMsg,
                          PMIDL_STUB_MESSAGE pStubMsg,
                          PMIDL_STUB_DESC pStubDescriptor,
                          unsigned int ProcNum);

    // not sure exactly where these belong but this seems close enough
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          CreateProcessA,
                          LPCSTR lpApplicationName,
                          LPSTR lpCommandLine,
                          LPSECURITY_ATTRIBUTES lpProcessAttributes,
                          LPSECURITY_ATTRIBUTES lpThreadAttributes,
                          BOOL bInheritHandles,
                          DWORD dwCreationFlags,
                          LPVOID lpEnvironment,
                          LPCSTR lpCurrentDirectory,
                          LPSTARTUPINFOA lpStartupInfo,
                          LPPROCESS_INFORMATION lpProcessInformation);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          CreateProcessW,
                          LPCWSTR lpApplicationName,
                          LPWSTR lpCommandLine,
                          LPSECURITY_ATTRIBUTES lpProcessAttributes,
                          LPSECURITY_ATTRIBUTES lpThreadAttributes,
                          BOOL bInheritHandles,
                          DWORD dwCreationFlags,
                          LPVOID lpEnvironment,
                          LPCWSTR lpCurrentDirectory,
                          LPSTARTUPINFOW lpStartupInfo,
                          LPPROCESS_INFORMATION lpProcessInformation);
    HOOK_FUNCTION_DECLARE(VOID, WINAPI, ExitProcess, DWORD dwExitCode);
    //HOOK_FUNCTION_DECLARE(SC_HANDLE APIENTRY OpenServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess);
    //HOOK_FUNCTION_DECLARE(SC_HANDLE APIENTRY OpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess);

    //HOOK_FUNCTION_DECLARE(FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

    //HOOK_FUNCTION_DECLARE(BOOL WINAPI IsDebuggerPresent(VOID);
    //HOOK_FUNCTION_DECLARE(VOID WINAPI OutputDebugStringA(LPCSTR lpOutputString);
    //HOOK_FUNCTION_DECLARE(VOID WINAPI OutputDebugStringW(LPCWSTR lpOutputString);
    //HOOK_FUNCTION_DECLARE(NTSTATUS NTAPI NtQueryInformationProcess(HANDLE ProcessHandle, /*PROCESSINFOCLASS*/DWORD ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

    // TODO: delete unused stuff

    //// TODO: dynamically allocate these somehow.
    //// I got tired of fiddling with that and just statically allocated a bunch of them for now.
    //// (the problem I'm trying to solve here is how to define one hook function used across many DLLs and have it call the trampoline function of the correct DLL for each one)
    //    //X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) \
    //X(10) X(11) X(12) X(13) X(14) X(15) X(16) X(17) X(18) X(19) \
    //X(20) X(21) X(22) X(23) X(24) X(25) X(26) X(27) X(28) X(29) \
    //X(30) X(31) X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39) \
    //X(40) X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) // 48 functions should be enough for anybody
    //typedef HRESULT (STDAPICALLTYPE *TypeOfDllGetClassObject) (REFCLSID rclsid, REFIID riid, LPVOID *ppv);
    //    //CodeGen_X_List
    //#undef X
    //TypeOfDllGetClassObject ArrayTrampDllGetClassObject[] =
    //{
    //    //    CodeGen_X_List
    //#undef X
    //	NULL
    //};
    //const char* ArrayNameDllGetClassObject[] =
    //{
    //    //    CodeGen_X_List
    //#undef X
    //	NULL
    //};

    ////    //HOOK_FUNCTION_DECLARE(HRESULT STDAPICALLTYPE TrampDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

    void UpdateLoadedOrUnloadedDllHooks();

    void ApplyModuleIntercepts();

    void ModuleDllMainInit();
}
