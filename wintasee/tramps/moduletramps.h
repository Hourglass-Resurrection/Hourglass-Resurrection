/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef MODULETRAMPS_H_INCL
#define MODULETRAMPS_H_INCL

//#define LoadLibraryExW TrampLoadLibraryExW
//TRAMPFUNC HMODULE WINAPI LoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags) TRAMPOLINE_DEF
#define LdrLoadDll TrampLdrLoadDll
TRAMPFUNC NTSTATUS NTAPI LdrLoadDll(PWCHAR PathToFile, ULONG Flags, struct _LSA_UNICODE_STRING* ModuleFileName, PHANDLE ModuleHandle) TRAMPOLINE_DEF
//#define LdrUnloadDll TrampLdrUnloadDll
//TRAMPFUNC NTSTATUS NTAPI LdrUnloadDll(HANDLE ModuleAddress) TRAMPOLINE_DEF

#define CallNextHookEx TrampCallNextHookEx
TRAMPFUNC LRESULT WINAPI CallNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF

//#define RegisterUserApiHook TrampRegisterUserApiHook
//TRAMPFUNC BOOL WINAPI RegisterUserApiHook(HINSTANCE hInst, FARPROC func) TRAMPOLINE_DEF

#define KiUserCallbackDispatcher TrampKiUserCallbackDispatcher
TRAMPFUNC VOID NTAPI KiUserCallbackDispatcher(ULONG ApiNumber, PVOID InputBuffer, ULONG InputLength) TRAMPOLINE_DEF_VOID


#define CoCreateInstance TrampCoCreateInstance
TRAMPFUNC HRESULT STDAPICALLTYPE CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv) TRAMPOLINE_DEF
#define CoCreateInstanceEx TrampCoCreateInstanceEx
TRAMPFUNC HRESULT STDAPICALLTYPE CoCreateInstanceEx(REFCLSID Clsid, LPUNKNOWN punkOuter, DWORD dwClsCtx, struct _COSERVERINFO* pServerInfo, DWORD dwCount, struct tagMULTI_QI* pResults) TRAMPOLINE_DEF
#define CoGetClassObject TrampCoGetClassObject
TRAMPFUNC HRESULT STDAPICALLTYPE CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved, REFIID riid, LPVOID FAR* ppv) TRAMPOLINE_DEF
#define IUnknown_QueryInterface_Proxy TrampIUnknown_QueryInterface_Proxy
TRAMPFUNC HRESULT STDMETHODCALLTYPE IUnknown_QueryInterface_Proxy(IUnknown __RPC_FAR * This,REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject) TRAMPOLINE_DEF

// not sure exactly where these belong but this seems close enough
#define RtlAllocateHeap TrampRtlAllocateHeap
TRAMPFUNC PVOID NTAPI RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size) TRAMPOLINE_DEF
#define RtlCreateHeap TrampRtlCreateHeap
TRAMPFUNC PVOID NTAPI RtlCreateHeap(ULONG Flags, PVOID HeapBase, SIZE_T ReserveSize, SIZE_T CommitSize, PVOID Lock, struct RTL_HEAP_PARAMETERS* Parameters) TRAMPOLINE_DEF
#define NdrAllocate TrampNdrAllocate
TRAMPFUNC PVOID RPC_ENTRY NdrAllocate(PMIDL_STUB_MESSAGE pStubMsg, size_t Len) TRAMPOLINE_DEF
#define NdrClientInitializeNew TrampNdrClientInitializeNew
TRAMPFUNC void RPC_ENTRY NdrClientInitializeNew(PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDescriptor, unsigned int ProcNum) TRAMPOLINE_DEF_VOID
#define NdrClientInitialize TrampNdrClientInitialize
TRAMPFUNC void RPC_ENTRY NdrClientInitialize(PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDescriptor, unsigned int ProcNum) TRAMPOLINE_DEF_VOID


// not sure exactly where these belong but this seems close enough
#define CreateProcessA TrampCreateProcessA
TRAMPFUNC BOOL WINAPI CreateProcessA(
	LPCSTR lpApplicationName,
	LPSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) TRAMPOLINE_DEF
#define CreateProcessW TrampCreateProcessW
TRAMPFUNC BOOL WINAPI CreateProcessW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) TRAMPOLINE_DEF
#define ExitProcess TrampExitProcess
TRAMPFUNC VOID WINAPI ExitProcess(DWORD dwExitCode) TRAMPOLINE_DEF_VOID
//#define OpenServiceA TrampOpenServiceA
//TRAMPFUNC SC_HANDLE APIENTRY OpenServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess) TRAMPOLINE_DEF
//#define OpenServiceW TrampOpenServiceW
//TRAMPFUNC SC_HANDLE APIENTRY OpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess) TRAMPOLINE_DEF



//#define GetProcAddress TrampGetProcAddress
//TRAMPFUNC FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName) TRAMPOLINE_DEF

//#define IsDebuggerPresent TrampIsDebuggerPresent
//TRAMPFUNC BOOL WINAPI IsDebuggerPresent(VOID) TRAMPOLINE_DEF
//#define OutputDebugStringA TrampOutputDebugStringA
//TRAMPFUNC VOID WINAPI OutputDebugStringA(LPCSTR lpOutputString) TRAMPOLINE_DEF_VOID
//#define OutputDebugStringW TrampOutputDebugStringW
//TRAMPFUNC VOID WINAPI OutputDebugStringW(LPCWSTR lpOutputString) TRAMPOLINE_DEF_VOID
//#define NtQueryInformationProcess TrampNtQueryInformationProcess
//TRAMPFUNC NTSTATUS NTAPI NtQueryInformationProcess(HANDLE ProcessHandle, /*PROCESSINFOCLASS*/DWORD ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) TRAMPOLINE_DEF



// TODO: delete unused stuff

//// TODO: dynamically allocate these somehow.
//// I got tired of fiddling with that and just statically allocated a bunch of them for now.
//// (the problem I'm trying to solve here is how to define one hook function used across many DLLs and have it call the trampoline function of the correct DLL for each one)
//#define CodeGen_X_List \
//X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) \
//X(10) X(11) X(12) X(13) X(14) X(15) X(16) X(17) X(18) X(19) \
//X(20) X(21) X(22) X(23) X(24) X(25) X(26) X(27) X(28) X(29) \
//X(30) X(31) X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39) \
//X(40) X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) // 48 functions should be enough for anybody
//typedef HRESULT (STDAPICALLTYPE *TypeOfDllGetClassObject) (REFCLSID rclsid, REFIID riid, LPVOID *ppv);
//#define X(y) TRAMPFUNC HRESULT STDAPICALLTYPE Tramp##y##DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) TRAMPOLINE_DEF
//CodeGen_X_List
//#undef X
//TypeOfDllGetClassObject ArrayTrampDllGetClassObject[] = 
//{
//#define X(y) Tramp##y##DllGetClassObject,
//    CodeGen_X_List
//#undef X
//	NULL
//};
//const char* ArrayNameDllGetClassObject[] = 
//{
//#define X(y) "unknown dll",
//    CodeGen_X_List
//#undef X
//	NULL
//};


////#define DllGetClassObject TrampDllGetClassObject
//TRAMPFUNC HRESULT STDAPICALLTYPE TrampDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) TRAMPOLINE_DEF

#ifdef TlsSetValue
#error this shouldn't happen (TlsSetValue already defined)
#endif

// hack: because we need to call TlsSetValue/TlsGetValue potentially very early in the startup process,
// let the TlsSetValue/TlsGetValue macros act like trampolines both before and after their respective functions have been hooked.
// this might seem universally safer, but the reason we usually avoid this way of defining trampolines is because:
// if the function doesn't exist in the DLL, it will cause the game to immediately crash on startup.
// for example, it's not safe to do this for FlsSetValue/FlsGetValue because those don't exist on Windows XP.
TRAMPFUNC BOOL WINAPI TrampTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) TRAMPOLINE_DEF_CUSTOM(return TlsSetValue(dwTlsIndex,lpTlsValue))
#define TlsSetValue TrampTlsSetValue
TRAMPFUNC LPVOID WINAPI TrampTlsGetValue(DWORD dwTlsIndex) TRAMPOLINE_DEF_CUSTOM(return TlsGetValue(dwTlsIndex))
#define TlsGetValue TrampTlsGetValue

#define FlsSetValue TrampFlsSetValue
TRAMPFUNC BOOL WINAPI FlsSetValue(DWORD dwFlsIndex, LPVOID lpFlsData) TRAMPOLINE_DEF
#define FlsGetValue TrampFlsGetValue
TRAMPFUNC PVOID WINAPI FlsGetValue(DWORD dwFlsIndex) TRAMPOLINE_DEF

#endif
