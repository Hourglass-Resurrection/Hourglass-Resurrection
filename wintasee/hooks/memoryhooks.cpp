/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */
 
#include <global.h>
#include <shared\ipc.h>

HOOKFUNC BOOL WINAPI MyAllocateUserPhysicalPages(HANDLE hProcess,
                                                 PULONG_PTR NumberOfPages,
                                                 PULONG_PTR PageArray)
{
    debugprintf(__FUNCTION__ "(0x%p, 0x%p) called: 0x%p\n", NumberOfPages, PageArray, hProcess);
    /*
     * NYI
     */
    return MyAllocateUserPhysicalPages(hProcess, NumberOfPages, PageArray);
}

HOOKFUNC BOOL WINAPI MyFreeUserPhysicalPages(HANDLE hProcess,
                                             PULONG_PTR NumberOfPages,
                                             PULONG_PTR PageArray)
{
    debugprintf(__FUNCTION__ "(0x%p, 0x%p) called: 0x%p\n", NumberOfPages, PageArray, hProcess);
    /*
     * NYI
     */
    return FreeUserPhysicalPages(hProcess, NumberOfPages, PageArray);
}

HOOKFUNC HANDLE WINAPI MyGetProcessHeap()
{
    debugprintf(__FUNCTION__ " called.\n");
    return GetProcessHeap();
}

HOOKFUNC DWORD WINAPI MyGetProcessHeaps(DWORD NumberOfHeaps, PHANDLE ProcessHeaps)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%p) called.\n", NumberOfHeaps, ProcessHeaps);
    return GetProcessHeaps(NumberOfHeaps, ProcessHeaps);
}

HOOKFUNC HGLOBAL WINAPI MyGlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%X) called.\n", uFlags, dwBytes);
    return GlobalAlloc(uFlags, dwBytes);
}

HOOKFUNC SIZE_T WINAPI MyGlobalCompact(DWORD dwMinFree)
{
    debugprintf(__FUNCTION__ "(0x%X) called.\n", dwMinFree);
    return GlobalCompact(dwMinFree);
}

HOOKFUNC void WINAPI MyGlobalFix(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalFix(hMem);
}

HOOKFUNC UINT WINAPI MyGlobalFlags(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalFlags(hMem);
}

HOOKFUNC HGLOBAL WINAPI MyGlobalFree(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalFree(hMem);
}

HOOKFUNC HGLOBAL WINAPI MyGlobalHandle(LPCVOID pMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", pMem);
    return GlobalHandle(pMem);
}

HOOKFUNC LPVOID WINAPI MyGlobalLock(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalLock(hMem);
}

HOOKFUNC HGLOBAL WINAPI MyGlobalReAlloc(HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%X 0x%X) called.\n", hMem, dwBytes, uFlags);
    return GlobalReAlloc(hMem, dwBytes, uFlags);
}

HOOKFUNC SIZE_T WINAPI MyGlobalSize(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalSize(hMem);
}

HOOKFUNC void WINAPI MyGlobalUnfix(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalUnfix(hMem);
}

HOOKFUNC BOOL WINAPI MyGlobalUnlock(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalUnlock(hMem);
}

HOOKFUNC BOOL WINAPI MyGlobalUnWire(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalUnWire(hMem);
}

HOOKFUNC LPVOID WINAPI MyGlobalWire(HGLOBAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return GlobalWire(hMem);
}

HOOKFUNC LPVOID WINAPI MyHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%X) called: 0x%p\n", dwFlags, dwBytes, hHeap);
    return HeapAlloc(hHeap, dwFlags, dwBytes);
}

HOOKFUNC SIZE_T WINAPI MyHeapCompact(HANDLE hHeap, DWORD dwFlags)
{
    debugprintf(__FUNCTION__ "(0x%X) called: 0x%p\n", dwFlags, hHeap);
    return HeapCompact(hHeap, dwFlags);
}

HOOKFUNC HANDLE WINAPI MyHeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%X 0x%X) called.\n", flOptions, dwInitialSize, dwMaximumSize);
    return HeapCreate(flOptions, dwInitialSize, dwMaximumSize);
}

HOOKFUNC BOOL WINAPI MyHeapDestroy(HANDLE hHeap)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hHeap);
    return HeapDestroy(hHeap);
}

HOOKFUNC BOOL WINAPI MyHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%p) called: 0x%p\n", dwFlags, lpMem, hHeap);
    return HeapFree(hHeap, dwFlags, lpMem);
}

HOOKFUNC BOOL WINAPI MyHeapLock(HANDLE hHeap)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hHeap);
    return HeapLock(hHeap);
}

HOOKFUNC BOOL WINAPI MyHeapQueryInformation(HANDLE HeapHandle,
                                            HEAP_INFORMATION_CLASS HeapInformationClass,
                                            PVOID HeapInformation,
                                            SIZE_T HeapInformationLength,
                                            PSIZE_T ReturnLength)
{
    debugprintf(__FUNCTION__ " called: 0x%p\n", HeapHandle);
    return HeapQueryInformation(HeapHandle, HeapInformationClass, HeapInformation, HeapInformationLength, ReturnLength);
}

HOOKFUNC LPVOID WINAPI MyHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%p 0x%) called: 0x%p\n", dwFlags, lpMem, dwBytes, hHeap);
    return HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
}

HOOKFUNC BOOL WINAPI MyHeapSetInformation(HANDLE HeapHandle,
                                          HEAP_INFORMATION_CLASS HeapInformationClass,
                                          PVOID HeapInformation,
                                          SIZE_T HeapInformationLength)
{
    debugprintf(__FUNCTION__ " called: %0x%p\n", HeapHandle);
    return HeapSetInformation(HeapHandle, HeapInformationClass, HeapInformation, HeapInformationLength);
}

HOOKFUNC SIZE_T WINAPI MyHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%p) called: 0x%p\n", dwFlags, lpMem, hHeap);
    return HeapSize(hHeap, dwFlags, lpMem);
}

HOOKFUNC BOOL WINAPI MyHeapUnlock(HANDLE hHeap)
{
    debugprintf(__FUNCTION__ " called: 0x%p\n", hHeap);
    return HeapUnlock(hHeap);
}

HOOKFUNC BOOL WINAPI MyHeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%p) called: 0x%p\n", dwFlags, lpMem, hHeap);
    return HeapValidate(hHeap, dwFlags, lpMem);
}

HOOKFUNC BOOL WINAPI MyHeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry)
{
    debugprintf(__FUNCTION__ "(0x%p) called: 0x%p\n", lpEntry, hHeap);
    return HeapWalk(hHeap, lpEntry);
}

HOOKFUNC BOOL WINAPI MyIsBadCodePtr(FARPROC lpfn)
{
    debugprintf(__FUNCTION__ " called: 0x%p\n", lpfn);
    return IsBadCodePtr(lpfn);
}

HOOKFUNC BOOL WINAPI MyIsBadReadPtr(const void *lp, UINT_PTR ucb)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lp, ucb);
    return IsBadReadPtr(lp, ucb);
}

HOOKFUNC BOOL WINAPI MyIsBadStringPtrA(LPCSTR lpsz, UINT_PTR ucchMax)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lpsz, ucchMax);
    return IsBadStringPtrA(lpsz, ucchMax);
}

HOOKFUNC BOOL WINAPI MyIsBadStringPtrW(LPCWSTR lpsz, UINT_PTR ucchMax)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lpsz, ucchMax);
    return IsBadStringPtrW(lpsz, ucchMax);
}

HOOKFUNC BOOL WINAPI MyIsBadWritePtr(LPVOID lp, UINT_PTR ucb)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lp, ucb);
    return IsBadWritePtr(lp, ucb);
}

HOOKFUNC HLOCAL WINAPI MyLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%X) called.\n", uFlags, uBytes);
    return LocalAlloc(uFlags, uBytes);
}

HOOKFUNC SIZE_T WINAPI MyLocalCompact(UINT uMinFree)
{
    debugprintf(__FUNCTION__ "(0x%X) called.\n", uMinFree);
    return LocalCompact(uMinFree);
}

HOOKFUNC UINT WINAPI MyLocalFlags(HLOCAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return LocalFlags(hMem);
}

HOOKFUNC HLOCAL WINAPI MyLocalFree(HLOCAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return LocalFree(hMem);
}

HOOKFUNC HLOCAL WINAPI MyLocalHandle(LPCVOID pMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", pMem);
    return LocalHandle(pMem);
}

HOOKFUNC LPVOID WINAPI MyLocalLock(HLOCAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return LocalLock(hMem);
}

HOOKFUNC HLOCAL WINAPI MyLocalReAlloc(HLOCAL hMem, SIZE_T uBytes, UINT uFlags)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX 0x%X) called.\n", hMem, uBytes, uFlags);
    return LocalReAlloc(hMem, uBytes, uFlags);
}

HOOKFUNC SIZE_T WINAPI MyLocalShrink(HLOCAL hMem, UINT cbNewSize)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%X) called.\n", hMem, cbNewSize);
    return LocalShrink(hMem, cbNewSize);
}

HOOKFUNC SIZE_T WINAPI MyLocalSize(HLOCAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return LocalSize(hMem);
}

HOOKFUNC BOOL WINAPI MyLocalUnlock(HLOCAL hMem)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", hMem);
    return LocalUnlock(hMem);
}

HOOKFUNC LPVOID WINAPI MyVirtualAlloc(LPVOID lpAddress,
                                      SIZE_T dwSize,
                                      DWORD flAllocationType,
                                      DWORD flProtect)
{
    debugprintf(__FUNCTION__ "(0x%p %x%lX 0x%X 0x%X) called.\n", lpAddress, dwSize, flAllocationType, flProtect);
    return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

HOOKFUNC LPVOID WINAPI MyVirtualAllocEx(HANDLE hProcess,
                                        LPVOID lpAddress,
                                        SIZE_T dwSize,
                                        DWORD flAllocationType,
                                        DWORD flProtect)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%p %x%lX 0x%X 0x%X) called.\n", hProcess, lpAddress, dwSize, flAllocationType, flProtect);
    return VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
}

HOOKFUNC BOOL WINAPI MyVirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD flFreeType)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX 0x%X) called.\n", lpAddress, dwSize, flFreeType);
    return VirtualFree(lpAddress, dwSize, flFreeType);
}

HOOKFUNC BOOL WINAPI MyVirtualFreeEx(HANDLE hProcess,
                                     LPVOID lpAddress,
                                     SIZE_T dwSize,
                                     DWORD flFreeType)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%p 0x%lX 0x%X) called.\n", hProcess, lpAddress, dwSize, flFreeType);
    return VirtualFreeEx(hProcess, lpAddress, dwSize, flFreeType);
}

HOOKFUNC BOOL WINAPI MyVirtualLock(LPVOID lpAddress, SIZE_T dwSize)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lpAddress, dwSize);
    return VirtualLock(lpAddress, dwSize);
}

HOOKFUNC BOOL WINAPI MyVirtualProtect(LPVOID lpAddress,
                                      SIZE_T dwSize,
                                      DWORD flNewProtect,
                                      PDWORD lpflOldProtect)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX 0x%X 0x%p) called.\n", lpAddress, dwSize, flNewProtect, lpflOldProtect);
    return VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

HOOKFUNC BOOL WINAPI MyVirtualProtectEx(HANDLE hProcess,
                                        LPVOID lpAddress,
                                        SIZE_T dwSize,
                                        DWORD flNewProtect,
                                        PDWORD lpflOldProtect)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%p 0x%lX 0x%X 0x%p) called.\n", hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
    return VirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

HOOKFUNC BOOL WINAPI MyVirtualUnlock(LPVOID lpAddress, SIZE_T dwSize)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%lX) called.\n", lpAddress, dwSize);
    return VirtualUnlock(lpAddress, dwSize);
}

void ApplyMemoryIntercepts()
{
    static const InterceptDescriptor intercepts [] = 
    {
        MAKE_INTERCEPT(1, KERNEL32, AllocateUserPhysicalPages),
        MAKE_INTERCEPT(1, KERNEL32, FreeUserPhysicalPages),
        MAKE_INTERCEPT(1, KERNEL32, GetProcessHeap),
        MAKE_INTERCEPT(1, KERNEL32, GetProcessHeaps),
        MAKE_INTERCEPT(1, KERNEL32, GlobalAlloc),
        MAKE_INTERCEPT(1, KERNEL32, GlobalCompact),
        MAKE_INTERCEPT(1, KERNEL32, GlobalFix),
        MAKE_INTERCEPT(1, KERNEL32, GlobalFlags),
        MAKE_INTERCEPT(1, KERNEL32, GlobalFree),
        MAKE_INTERCEPT(1, KERNEL32, GlobalHandle),
        MAKE_INTERCEPT(1, KERNEL32, GlobalLock),
        MAKE_INTERCEPT(1, KERNEL32, GlobalReAlloc),
        MAKE_INTERCEPT(1, KERNEL32, GlobalSize),
        MAKE_INTERCEPT(1, KERNEL32, GlobalUnfix),
        MAKE_INTERCEPT(1, KERNEL32, GlobalUnlock),
        MAKE_INTERCEPT(1, KERNEL32, GlobalUnWire),
        MAKE_INTERCEPT(1, KERNEL32, GlobalWire),
        MAKE_INTERCEPT(1, KERNEL32, HeapAlloc),
        MAKE_INTERCEPT(1, KERNEL32, HeapCompact),
        MAKE_INTERCEPT(1, KERNEL32, HeapCreate),
        MAKE_INTERCEPT(1, KERNEL32, HeapDestroy),
        MAKE_INTERCEPT(1, KERNEL32, HeapFree),
        MAKE_INTERCEPT(1, KERNEL32, HeapLock),
        MAKE_INTERCEPT(1, KERNEL32, HeapQueryInformation),
        MAKE_INTERCEPT(1, KERNEL32, HeapReAlloc),
        MAKE_INTERCEPT(1, KERNEL32, HeapSetInformation),
        MAKE_INTERCEPT(1, KERNEL32, HeapSize),
        MAKE_INTERCEPT(1, KERNEL32, HeapUnlock),
        MAKE_INTERCEPT(1, KERNEL32, HeapValidate),
        MAKE_INTERCEPT(1, KERNEL32, HeapWalk),
        MAKE_INTERCEPT(1, KERNEL32, IsBadCodePtr),
        MAKE_INTERCEPT(1, KERNEL32, IsBadReadPtr),
        MAKE_INTERCEPT(1, KERNEL32, IsBadStringPtrA),
        MAKE_INTERCEPT(1, KERNEL32, IsBadStringPtrW),
        MAKE_INTERCEPT(1, KERNEL32, IsBadWritePtr),
        MAKE_INTERCEPT(1, KERNEL32, LocalAlloc),
        MAKE_INTERCEPT(1, KERNEL32, LocalCompact),
        MAKE_INTERCEPT(1, KERNEL32, LocalFlags),
        MAKE_INTERCEPT(1, KERNEL32, LocalFree),
        MAKE_INTERCEPT(1, KERNEL32, LocalHandle),
        MAKE_INTERCEPT(1, KERNEL32, LocalLock),
        MAKE_INTERCEPT(1, KERNEL32, LocalReAlloc),
        MAKE_INTERCEPT(1, KERNEL32, LocalShrink),
        MAKE_INTERCEPT(1, KERNEL32, LocalSize),
        MAKE_INTERCEPT(1, KERNEL32, LocalUnlock),
        MAKE_INTERCEPT(1, KERNEL32, VirtualAlloc),
        MAKE_INTERCEPT(1, KERNEL32, VirtualAllocEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualFree),
        MAKE_INTERCEPT(1, KERNEL32, VirtualFreeEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualLock),
        MAKE_INTERCEPT(1, KERNEL32, VirtualProtect),
        MAKE_INTERCEPT(1, KERNEL32, VirtualProtectEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualUnlock)
    };
    ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}
