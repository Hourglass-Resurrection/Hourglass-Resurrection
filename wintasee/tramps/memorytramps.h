/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define AllocateUserPhysicalPages TrampAllocateUserPhysicalPages
TRAMPFUNC BOOL WINAPI TrampAllocateUserPhysicalPages(HANDLE hProcess,
                                                     PULONG_PTR NumberOfPages,
                                                     PULONG_PTR PageArray) TRAMPOLINE_DEF
#define FreeUserPhysicalPages TrampFreeUserPhysicalPages
TRAMPFUNC BOOL WINAPI TrampFreeUserPhysicalPages(HANDLE hProcess,
                                                 PULONG_PTR NumberOfPages,
                                                 PULONG_PTR PageArray) TRAMPOLINE_DEF
#define GetProcessHeap TrampGetProcessHeap
TRAMPFUNC HANDLE WINAPI TrampGetProcessHeap() TRAMPOLINE_DEF
#define GetProcessHeaps TrampGetProcessHeaps
TRAMPFUNC DWORD WINAPI TrampGetProcessHeaps(DWORD NumberOfHeaps,
                                            PHANDLE ProcessHeaps) TRAMPOLINE_DEF
#define GlobalAlloc TrampGlobalAlloc
TRAMPFUNC HGLOBAL WINAPI TrampGlobalAlloc(UINT uFlags, SIZE_T dwBytes) TRAMPOLINE_DEF
#define GlobalCompact TrampGlobalCompact
TRAMPFUNC SIZE_T WINAPI TrampGlobalCompact(DWORD dwMinSize) TRAMPOLINE_DEF
#define GlobalFix TrampGlobalFix
TRAMPFUNC void WINAPI TrampGlobalFix(HGLOBAL hMem) TRAMPOLINE_DEF_VOID
#define GlobalFlags TrampGlobalFlags
TRAMPFUNC UINT WINAPI TrampGlobalFlags(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalFree TrampGlobalFree
TRAMPFUNC HGLOBAL WINAPI TrampGlobalFree(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalHandle TrampGlobalHandle
TRAMPFUNC HGLOBAL WINAPI TrampGlobalHandle(LPCVOID pMem) TRAMPOLINE_DEF
#define GlobalLock TrampGlobalLock
TRAMPFUNC LPVOID WINAPI TrampGlobalLock(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalReAlloc TrampGlobalReAlloc
TRAMPFUNC HGLOBAL WINAPI TrampGlobalReAlloc(HGLOBAL hMem,
                                            SIZE_T dwBytes,
                                            UINT uFlags) TRAMPOLINE_DEF
#define GlobalSize TrampGlobalSize
TRAMPFUNC SIZE_T WINAPI TrampGlobalSize(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalUnfix TrampGlobalUnfix
TRAMPFUNC void WINAPI TrampGlobalUnfix(HGLOBAL hMem) TRAMPOLINE_DEF_VOID
#define GlobalUnlock TrampGlobalUnlock
TRAMPFUNC BOOL WINAPI TrampGlobalUnlock(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalUnWire TrampGlobalUnWire
TRAMPFUNC BOOL WINAPI TrampGlobalUnWire(HGLOBAL hMem) TRAMPOLINE_DEF
#define GlobalWire TrampGlobalWire
TRAMPFUNC LPVOID WINAPI TrampGlobalWire(HGLOBAL hMem) TRAMPOLINE_DEF
#define HeapAlloc TrampHeapAlloc
TRAMPFUNC LPVOID WINAPI TrampHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes) TRAMPOLINE_DEF
#define HeapCompact TrampHeapCompact
TRAMPFUNC SIZE_T WINAPI TrampHeapCompact(HANDLE hHeap, DWORD dwFlags) TRAMPOLINE_DEF
#define HeapCreate TrampHeapCreate
TRAMPFUNC HANDLE WINAPI TrampHeapCreate(DWORD flOptions,
                                        SIZE_T dwInitialSize,
                                        SIZE_T dwMaximumSize) TRAMPOLINE_DEF
#define HeapDestroy TrampHeapDestroy
TRAMPFUNC BOOL WINAPI TrampHeapDestroy(HANDLE hHeap) TRAMPOLINE_DEF
#define HeapFree TrampHeapFree
TRAMPFUNC BOOL WINAPI TrampHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) TRAMPOLINE_DEF
#define HeapLock TrampHeapLock
TRAMPFUNC BOOL WINAPI TrampHeapLock(HANDLE hHeap) TRAMPOLINE_DEF
#define HeapQueryInformation TrampHeapQueryInformation
TRAMPFUNC BOOL WINAPI HeapQueryInformation(
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation,
    SIZE_T HeapInformationLength,
    PSIZE_T ReturnLength) TRAMPOLINE_DEF
#define HeapReAlloc TrampHeapReAlloc
TRAMPFUNC LPVOID WINAPI TrampHeapReAlloc(HANDLE hHeap,
                                         DWORD dwFlags,
                                         LPVOID lpMem,
                                         SIZE_T dwBytes) TRAMPOLINE_DEF
#define HeapSetInformation TrampHeapSetInformation
TRAMPFUNC BOOL WINAPI TrampHeapSetInformation(HANDLE HeapHandle,
                                              HEAP_INFORMATION_CLASS HeapInformationClass,
                                              PVOID HeapInformation,
                                              SIZE_T HeapInformationLength) TRAMPOLINE_DEF
#define HeapSize TrampHeapSize
TRAMPFUNC SIZE_T WINAPI TrampHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) TRAMPOLINE_DEF
#define HeapUnlock TrampHeapUnlock
TRAMPFUNC BOOL WINAPI TrampHeapUnlock(HANDLE hHeap) TRAMPOLINE_DEF
#define HeapValidate TrampHeapValidate
TRAMPFUNC BOOL WINAPI TrampHeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) TRAMPOLINE_DEF
#define HeapWalk TrampHeapWalk
TRAMPFUNC BOOL WINAPI TrampHeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry) TRAMPOLINE_DEF
#define IsBadCodePtr TrampIsBadCodePtr
TRAMPFUNC BOOL WINAPI TrampIsBadCodePtr(FARPROC lpfn) TRAMPOLINE_DEF
#define IsBadReadPtr TrampIsBadReadPtr
TRAMPFUNC BOOL WINAPI TrampIsBadReadPtr(const void *lp, UINT_PTR ucb) TRAMPOLINE_DEF
#define IsBadStringPtrA TrampIsBadStringPtrA
TRAMPFUNC BOOL WINAPI TrampIsBadStringPtrA(LPCSTR lpsz, UINT_PTR ucchMax) TRAMPOLINE_DEF
#define IsBadStringPtrW TrampIsBadStringPtrW
TRAMPFUNC BOOL WINAPI TrampIsBadStringPtrW(LPCWSTR lpsz, UINT_PTR ucchMax) TRAMPOLINE_DEF
#define IsBadWritePtr TrampIsBadWritePtr
TRAMPFUNC BOOL WINAPI TrampIsBadWritePtr(LPVOID lp, UINT_PTR ucb) TRAMPOLINE_DEF
#define LocalAlloc TrampLocalAlloc
TRAMPFUNC HLOCAL WINAPI TrampLocalAlloc(UINT uFlags, SIZE_T uBytes) TRAMPOLINE_DEF
#define LocalCompact TrampLocalCompact
TRAMPFUNC SIZE_T WINAPI TrampLocalCompact(UINT uMinFree) TRAMPOLINE_DEF
#define LocalFlags TrampLocalFlags
TRAMPFUNC UINT WINAPI TrampLocalFlags(HLOCAL hMem) TRAMPOLINE_DEF
#define LocalFree TrampLocalFree
TRAMPFUNC HLOCAL WINAPI TrampLocalFree(HLOCAL hMem) TRAMPOLINE_DEF
#define LocalHandle TrampLocalHandle
TRAMPFUNC HLOCAL WINAPI TrampLocalHandle(LPCVOID pMem) TRAMPOLINE_DEF
#define LocalLock TrampLocalLock
TRAMPFUNC LPVOID WINAPI TrampLocalLock(HLOCAL hMem) TRAMPOLINE_DEF
#define LocalReAlloc TrampLocalReAlloc
TRAMPFUNC HLOCAL WINAPI TrampLocalReAlloc(HLOCAL hMem, SIZE_T uBytes, UINT uFlags) TRAMPOLINE_DEF
#define LocalShrink TrampLocalShrink
TRAMPFUNC SIZE_T WINAPI TrampLocalShrink(HLOCAL hMem, UINT cbNewSize) TRAMPOLINE_DEF
#define LocalSize TrampLocalSize
TRAMPFUNC SIZE_T WINAPI TrampLocalSize(HLOCAL hMem) TRAMPOLINE_DEF
#define LocalUnlock TrampLocalUnlock
TRAMPFUNC BOOL WINAPI TrampLocalUnlock(HLOCAL hMem) TRAMPOLINE_DEF
#define VirtualAlloc TrampVirtualAlloc
TRAMPFUNC LPVOID WINAPI TrampVirtualAlloc(LPVOID lpAddress,
                                          SIZE_T dwSize,
                                          DWORD flAllocationType,
                                          DWORD flProtect) TRAMPOLINE_DEF
#define VirtualAllocEx TrampVirtualAllocEx
TRAMPFUNC LPVOID WINAPI TrampVirtualAllocEx(HANDLE hProcess,
                                            LPVOID lpAddress,
                                            SIZE_T dwSize,
                                            DWORD flAllocationType,
                                            DWORD flProtect) TRAMPOLINE_DEF
#define VirtualFree TrampVirtualFree
TRAMPFUNC BOOL WINAPI TrampVirtualFree(LPVOID lpAddress,
                                       SIZE_T dwSize,
                                       DWORD flFreeType) TRAMPOLINE_DEF
#define VirtualFreeEx TrampVirtualFreeEx
TRAMPFUNC BOOL WINAPI TrampVirtualFreeEx(HANDLE hProcess,
                                         LPVOID lpAddress,
                                         SIZE_T dwSize,
                                         DWORD flFreeType) TRAMPOLINE_DEF
#define VirtualLock TrampVirtualLock
TRAMPFUNC BOOL WINAPI TrampVirtualLock(LPVOID lpAddress, SIZE_T dwSize) TRAMPOLINE_DEF
#define VirtualProtect TrampVirtualProtect
TRAMPFUNC BOOL WINAPI TrampVirtualProtect(LPVOID lpAddress,
                                          SIZE_T dwSize,
                                          DWORD flNewProtect,
                                          PDWORD lpflOldProtect) TRAMPOLINE_DEF
#define VirtualProtectEx TrampVirtualProtectEx
TRAMPFUNC BOOL WINAPI TrampVirtualProtectEx(HANDLE hProcess,
                                            LPVOID lpAddress,
                                            SIZE_T dwSize,
                                            DWORD flNewProtect,
                                            PDWORD lpflOldProtect) TRAMPOLINE_DEF
#define VirtualUnlock TrampVirtualUnlock
TRAMPFUNC BOOL WINAPI TrampVirtualUnlock(LPVOID lpAddress, SIZE_T dwSize) TRAMPOLINE_DEF
