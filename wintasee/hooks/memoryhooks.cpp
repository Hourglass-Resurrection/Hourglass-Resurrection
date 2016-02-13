/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <atomic>

#include <global.h>
#include <MemoryManager\MemoryManager.h>
#include <shared\ipc.h>
#include <Utils.h>
/*
 * TODO: Including this breaks input hooks.
 * -- Warepire
 */
//#include <wintasee.h>

/*
 * This map will contain all memory allocations made by the GlobalAlloc and LocalAlloc families,
 * and their respective lock counts and/or flags.
 * According to MSDN docs the value returned by the [X]Flags calls contain no flags or
 * [x]_DISCARDED. According to the WinBase.h header, the flags may also contain [X]_DISCARDABLE.
 * Since [X]_FIXED allocations cannot be discarded, or carry a lock count, they are indicated by
 * the value UINT_MAX. This is because they must be separated from unknown (invalid) pointers.
 */
LazyType<SafeMap<LPVOID, UINT>> g_alloc_flags;

/*
 * Emulate the Windows Heap objects using the minimum information necessary.
 */
struct HeapObject
{
    SafeVector<LPVOID> m_heap_segments;
    DWORD m_heap_flags;
    std::atomic_flag m_heap_lock;
    SIZE_T m_heap_current_size;
    SIZE_T m_heap_maximum_size;

    HeapObject(DWORD flags, SIZE_T current_size, SIZE_T max_size) :
        m_heap_flags(flags),
        m_heap_current_size(current_size),
        m_heap_maximum_size(max_size)
    {
        m_heap_lock.clear();
    }
};
HeapObject* g_default_heap = nullptr;
LazyType<SafeVector<HeapObject*>> g_heaps;

HOOKFUNC LPVOID WINAPI MyGlobalLock(HGLOBAL hMem);
HOOKFUNC BOOL WINAPI MyGlobalUnlock(HGLOBAL hMem);
HOOKFUNC LPVOID WINAPI MyHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
HOOKFUNC BOOL WINAPI MyHeapLock(HANDLE hHeap);
HOOKFUNC BOOL WINAPI MyHeapUnlock(HANDLE hHeap);
HOOKFUNC BOOL WINAPI MyIsBadReadPtr(const void *lp, UINT_PTR ucb);
HOOKFUNC BOOL WINAPI MyIsBadWritePtr(LPVOID lp, UINT_PTR ucb);

HOOKFUNC BOOL WINAPI MyAllocateUserPhysicalPages(HANDLE hProcess,
                                                 PULONG_PTR NumberOfPages,
                                                 PULONG_PTR PageArray)
{
    ENTER(hProcess, NumberOfPages, PageArray);
    MessageBoxA(nullptr,//gamehwnd,
                __FUNCTION__ " is not implemented.\n"
                "The application will most likely experience problems with SaveStates now.",
                "Warning!",
                MB_ICONWARNING | MB_OK);
    /*
     * NYI
     */
    return AllocateUserPhysicalPages(hProcess, NumberOfPages, PageArray);
}

HOOKFUNC BOOL WINAPI MyFreeUserPhysicalPages(HANDLE hProcess,
                                             PULONG_PTR NumberOfPages,
                                             PULONG_PTR PageArray)
{
    ENTER(hProcess, NumberOfPages, PageArray);
    MessageBoxA(nullptr,//gamehwnd,
                __FUNCTION__ " is not implemented.\n"
                "The application will most likely experience problems with SaveStates now.",
                "Warning!",
                MB_ICONWARNING | MB_OK);
    /*
     * NYI
     */
    return FreeUserPhysicalPages(hProcess, NumberOfPages, PageArray);
}

/*
 * Even though assigned a default heap by Windows, we cannot use that one.
 * It's not guaranteed that heap can be restored on load-state.
 * Use a different heap as the "default".
 */
HOOKFUNC HANDLE WINAPI MyGetProcessHeap()
{
    ENTER();
    HeapObject* rv = g_default_heap;
    if (rv == nullptr)
    {
        /*
         * TODO: Make this code path use MyHeapCreate
         * -- Warepire
         */
        rv = static_cast<HeapObject*>(MemoryManager::Allocate(sizeof(*rv),
                                                              MemoryManager::ALLOC_WRITE));
        if (rv == nullptr)
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }
        else
        {
            rv = ::new HeapObject(0, 0x100000, 0);
            LPVOID first_segment = MyHeapAlloc(rv, 0, 0x100000);
            if (first_segment == nullptr)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                MemoryManager::Deallocate(rv);
                rv = nullptr;
            }
            else
            {
                rv->m_heap_segments.push_back(first_segment);
                g_default_heap = rv;
                /*
                 * Keep the default heap in the front of the vector.
                 */
                g_heaps().insert(g_heaps().begin(), rv);
            }
        }
    }
    return rv;
}

HOOKFUNC DWORD WINAPI MyGetProcessHeaps(DWORD NumberOfHeaps, PHANDLE ProcessHeaps)
{
    ENTER(NumberOfHeaps, ProcessHeaps);
    DWORD rv = (g_heaps().size() > NumberOfHeaps) ? NumberOfHeaps : g_heaps().size();
    if (g_default_heap == nullptr)
    {
        if (MyGetProcessHeap() == nullptr)
        {
            *ProcessHeaps = nullptr;
            return 0;
        }
    }
    for (DWORD i = 0; i < rv; ++i)
    {
        ProcessHeaps[i] = g_heaps()[i];
    }
    return rv;
}

HOOKFUNC HGLOBAL WINAPI MyGlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
    ENTER(uFlags, dwBytes);
    UINT alloc_flags = 0;
    LPVOID address = nullptr;

    if ((uFlags & GMEM_ZEROINIT) == GMEM_ZEROINIT)
    {
        alloc_flags |= MemoryManager::ALLOC_ZEROINIT;
    }
    address = MemoryManager::Allocate(dwBytes, alloc_flags);
    if (address != nullptr)
    {
        if ((uFlags & GMEM_MOVEABLE) == GMEM_MOVEABLE)
        {
            g_alloc_flags()[address] = (uFlags & GMEM_DISCARDABLE);
        }
        else
        {
            g_alloc_flags()[address] = UINT_MAX;
        }
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return address;
}

HOOKFUNC SIZE_T WINAPI MyGlobalCompact(DWORD dwMinFree)
{
    ENTER(dwMinFree);
    return 0;
}

HOOKFUNC void WINAPI MyGlobalFix(HGLOBAL hMem)
{
    ENTER(hMem);
    MyGlobalLock(hMem);
}

HOOKFUNC UINT WINAPI MyGlobalFlags(HGLOBAL hMem)
{
    ENTER(hMem);
    auto it = g_alloc_flags().find(hMem);
    if (it != g_alloc_flags().end())
    {
        if (it->second == UINT_MAX)
        {
            return 0;
        }
        return it->second;
    }
    SetLastError(ERROR_INVALID_HANDLE);
    return GMEM_INVALID_HANDLE;
}

HOOKFUNC HGLOBAL WINAPI MyGlobalFree(HGLOBAL hMem)
{
    ENTER(hMem);
    if (g_alloc_flags().find(hMem) != g_alloc_flags().end())
    {
        MemoryManager::Deallocate(hMem);
        return nullptr;
    }
    SetLastError(ERROR_INVALID_HANDLE);
    return hMem;
}

HOOKFUNC HGLOBAL WINAPI MyGlobalHandle(LPCVOID pMem)
{
    ENTER(pMem);
    /*
     * We don't bother with HGLOBAL data structures.
     */
    LPVOID address = const_cast<LPVOID>(pMem);
    if (address == nullptr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        address = nullptr;
    }
    else if (g_alloc_flags().find(address) == g_alloc_flags().end())
    {
        SetLastError(ERROR_INVALID_HANDLE);
        address = nullptr;
    }
    return address;
}

HOOKFUNC LPVOID WINAPI MyGlobalLock(HGLOBAL hMem)
{
    ENTER(hMem);
    LPVOID rv = hMem;
    auto it = g_alloc_flags().find(hMem);
    if (hMem == nullptr)
    {
        SetLastError(ERROR_DISCARDED);
        rv = nullptr;
    }
    else if (it == g_alloc_flags().end())
    {
        SetLastError(ERROR_INVALID_HANDLE);
        rv = nullptr;
    }
    else if (it->second == UINT_MAX)
    {
        /*
         * MyIsBadReadPtr performs SetLastError, don't touch it here.
         */
        if (MyIsBadReadPtr(hMem, 1) == TRUE)
        {
            rv = nullptr;
        }
    }
    else if ((it->second & GMEM_DISCARDED) == GMEM_DISCARDED)
    {
        SetLastError(ERROR_DISCARDED);
        rv = nullptr;
    }
    else if ((it->second & GMEM_LOCKCOUNT) < GMEM_LOCKCOUNT)
    {
        it->second++;
    }
    return hMem;
}

HOOKFUNC HGLOBAL WINAPI MyGlobalReAlloc(HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags)
{
    ENTER(hMem, dwBytes, uFlags);
    UINT alloc_flags = 0;
    HGLOBAL rv = nullptr;
    auto it = g_alloc_flags().find(hMem);

    if ((uFlags & GMEM_ZEROINIT) == GMEM_ZEROINIT)
    {
        alloc_flags |= MemoryManager::ALLOC_ZEROINIT;
    }

    if (it == g_alloc_flags().end())
    {
        SetLastError(ERROR_NOACCESS);
    }
    else if ((uFlags & GMEM_MODIFY) == GMEM_MODIFY)
    {
        if (it->second == UINT_MAX && (uFlags & GMEM_MOVEABLE) == GMEM_MOVEABLE)
        {
            /*
             * Not really allowed on GMEM_FIXED memory, but WINE does it anyway and have not
             * experienced any problems yet.
             * Might even help some old game that did something illegal that happened to work.
             */
            it->second = 0;
            rv = hMem;
        }
        else if (it->second == UINT_MAX && (uFlags & GMEM_DISCARDABLE) == GMEM_DISCARDABLE)
        {
            it->second |= GMEM_DISCARDABLE;
            rv = hMem;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        if (it->second == UINT_MAX)
        {
            alloc_flags |= MemoryManager::REALLOC_NO_MOVE;
        }
        if (dwBytes != 0)
        {
            rv = MemoryManager::Reallocate(hMem, dwBytes, alloc_flags);
            if (rv == nullptr)
            {
                SetLastError(ERROR_OUTOFMEMORY);
            }
            else
            {
                /*
                 * Transfer flags to new block, unconditionally unset GMEM_DISCARDED.
                 * WINE does not implement use of the GMEM_DISCARDED flag.
                 *
                 * TODO: Does the returned HGLOBAL need to match the passed HGLOBAL when discarded
                 *       memory is reallocated? Docs are not clear on this.
                 * -- Warepire
                 */
                UINT temp_flags = it->second;
                if ((uFlags & GMEM_DISCARDABLE) == GMEM_DISCARDABLE)
                {
                    temp_flags |= GMEM_DISCARDABLE;
                }
                else
                {
                    temp_flags &= ~GMEM_DISCARDABLE;
                }
                temp_flags &= ~GMEM_DISCARDED;
                g_alloc_flags().erase(it);
                g_alloc_flags()[rv] = temp_flags;
            }
        }
        else if ((it->second & GMEM_LOCKCOUNT) == 0 &&
                 (it->second & GMEM_DISCARDABLE) == GMEM_DISCARDABLE)
        {
            it->second |= GMEM_DISCARDED;
            rv = hMem;
        }
        else
        {
            MemoryManager::Deallocate(hMem);
        }
    }
    return rv;
}

HOOKFUNC SIZE_T WINAPI MyGlobalSize(HGLOBAL hMem)
{
    ENTER(hMem);
    SIZE_T rv = 0;
    auto it = g_alloc_flags().find(hMem);
    if (it == g_alloc_flags().end())
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else if ((it->second & GMEM_DISCARDED) == GMEM_DISCARDED)
    {
        /*
         * Is this accurate? WINE seems to do this.
         */
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else
    {
        rv = MemoryManager::GetSizeOfAllocation(hMem);
    }
    return rv;
}

HOOKFUNC void WINAPI MyGlobalUnfix(HGLOBAL hMem)
{
    ENTER(hMem);
    MyGlobalUnlock(hMem);
}

HOOKFUNC BOOL WINAPI MyGlobalUnlock(HGLOBAL hMem)
{
    ENTER(hMem);
    BOOL rv = TRUE;
    auto it = g_alloc_flags().find(hMem);
    if (it == g_alloc_flags().end())
    {
        SetLastError(ERROR_INVALID_HANDLE);
        rv = FALSE;
    }
    else if ((it->second & GMEM_LOCKCOUNT) == 0)
    {
        SetLastError(ERROR_NOT_LOCKED);
        rv = FALSE;
    }
    else if (it->second != UINT_MAX)
    {
        it->second--;
    }
    return rv;
}

HOOKFUNC BOOL WINAPI MyGlobalUnWire(HGLOBAL hMem)
{
    ENTER(hMem);
    return MyGlobalUnlock(hMem);
}

HOOKFUNC LPVOID WINAPI MyGlobalWire(HGLOBAL hMem)
{
    ENTER(hMem);
    return MyGlobalLock(hMem);
}

HOOKFUNC LPVOID WINAPI MyHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
    ENTER(hHeap, dwFlags, dwBytes);
    LPVOID rv = nullptr;
    UINT alloc_flags = 0;
    for (auto& h : g_heaps())
    {
        if (h == hHeap)
        {
            if (dwBytes == 0 &&
                ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                 (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
            {
                RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);
            }
            if (dwBytes + h->m_heap_current_size > h->m_heap_maximum_size &&
                h->m_heap_maximum_size != 0 &&
                ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                 (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
            {
                RaiseException(STATUS_NO_MEMORY, 0, 0, nullptr);
            }
            if ((h->m_heap_flags & HEAP_CREATE_ENABLE_EXECUTE) == HEAP_CREATE_ENABLE_EXECUTE)
            {
                alloc_flags |= MemoryManager::ALLOC_EXECUTE;
            }
            if ((dwFlags & HEAP_ZERO_MEMORY) == HEAP_ZERO_MEMORY)
            {
                alloc_flags |= MemoryManager::ALLOC_ZEROINIT;
            }
            if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
            {
                MyHeapLock(hHeap);
            }
            rv = MemoryManager::Allocate(dwBytes, alloc_flags);
            if (rv != nullptr)
            {
                h->m_heap_current_size += MemoryManager::GetSizeOfAllocation(rv);
                h->m_heap_segments.push_back(rv);
            }
            if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
            {
                MyHeapUnlock(hHeap);
            }
            if (rv == nullptr &&
                ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                 (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
            {
                RaiseException(STATUS_NO_MEMORY, 0, 0, nullptr);
            }

        }
    }
    return rv;
}

HOOKFUNC SIZE_T WINAPI MyHeapCompact(HANDLE hHeap, DWORD dwFlags)
{
    ENTER(dwFlags, hHeap);
    return 0;
}

HOOKFUNC HANDLE WINAPI MyHeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
    ENTER(flOptions, dwInitialSize, dwMaximumSize);
    HeapObject* heap =
        static_cast<HeapObject*>(MemoryManager::Allocate(sizeof(*heap),
                                                         MemoryManager::ALLOC_WRITE));
    if (heap == nullptr)
    {
        SetLastError(ERROR_OUTOFMEMORY);
    }
    else
    {
        heap = ::new HeapObject(flOptions, dwInitialSize, dwMaximumSize);
        g_heaps().push_back(heap);
        LPVOID alloc = MyHeapAlloc(heap, 0, dwInitialSize);
        if (alloc == nullptr)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            MemoryManager::Deallocate(heap);
            g_heaps().pop_back();
            heap = nullptr;
        }
    }
    return heap;
}

HOOKFUNC BOOL WINAPI MyHeapDestroy(HANDLE hHeap)
{
    ENTER(hHeap);
    BOOL rv = TRUE;
    auto it = g_heaps().begin();
    if (hHeap == nullptr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else if (hHeap != g_default_heap)
    {
        if (g_heaps().front() == g_default_heap)
        {
            it++;
        }
        SetLastError(ERROR_INVALID_HANDLE);
        for (; it != g_heaps().end(); ++it)
        {
            HeapObject* h = *it;
            if (h == hHeap)
            {
                while (h->m_heap_segments.empty() == false)
                {
                    MemoryManager::Deallocate(h->m_heap_segments.back());
                    h->m_heap_segments.pop_back();
                }
                g_heaps().erase(it);
                MemoryManager::Deallocate(hHeap);
                rv = FALSE;
                SetLastError(ERROR_SUCCESS);
                break;
            }
        }
    }
    return rv;
}

HOOKFUNC BOOL WINAPI MyHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    ENTER(dwFlags, lpMem, hHeap);
    BOOL rv = FALSE;
    if (lpMem == nullptr)
    {
        rv = TRUE;
    }
    else if (hHeap == nullptr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else
    {
        for (auto& h : g_heaps())
        {
            if (h == hHeap)
            {
                for (auto s = h->m_heap_segments.begin(); s != h->m_heap_segments.end(); ++s)
                {
                    if (*s == lpMem)
                    {
                        if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                            (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
                        {
                            MyHeapLock(hHeap);
                        }
                        h->m_heap_current_size -= MemoryManager::GetSizeOfAllocation(*s);
                        MemoryManager::Deallocate(*s);
                        h->m_heap_segments.erase(s);
                        if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                            (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
                        {
                            MyHeapUnlock(hHeap);
                        }
                        rv = TRUE;
                        goto heap_segment_freed;
                    }
                }
            }
        }
        SetLastError(ERROR_INVALID_PARAMETER);
    }
heap_segment_freed:
    return rv;
}

HOOKFUNC BOOL WINAPI MyHeapLock(HANDLE hHeap)
{
    ENTER(hHeap);
    for (auto& h : g_heaps())
    {
        if (h == hHeap)
        {
            while (h->m_heap_lock.test_and_set() == true) {}
            return TRUE;
        }
    }
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

/*
 * Ripped directly out of WINE.
 */
HOOKFUNC BOOL WINAPI MyHeapQueryInformation(HANDLE HeapHandle,
                                            HEAP_INFORMATION_CLASS HeapInformationClass,
                                            PVOID HeapInformation,
                                            SIZE_T HeapInformationLength,
                                            PSIZE_T ReturnLength)
{
    ENTER(HeapHandle, HeapInformation, HeapInformation, HeapInformationLength, ReturnLength);
    BOOL rv = FALSE;
    switch (HeapInformationClass)
    {
    case HeapCompatibilityInformation:
        if (ReturnLength)
        {
            *ReturnLength = sizeof(ULONG);
        }
        if (HeapInformationLength < *ReturnLength)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            *static_cast<PUINT>(HeapInformation) = 0;
            rv = TRUE;
        }
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }
    return rv;
}

HOOKFUNC LPVOID WINAPI MyHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{
    ENTER(hHeap, dwFlags, lpMem, dwBytes);
    LPVOID rv = nullptr;
    BOOL heap_locked = FALSE;
    UINT alloc_flags = 0;
    SIZE_T old_size = 0;
    for (auto& h : g_heaps())
    {
        if (h == hHeap)
        {
            for (auto s = h->m_heap_segments.begin(); s != h->m_heap_segments.end(); ++s)
            {
                if (*s == lpMem)
                {
                    if (dwBytes == 0 &&
                        ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                         (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
                    {
                        RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);
                    }
                    if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                        (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
                    {
                        MyHeapLock(hHeap);
                        heap_locked = TRUE;
                    }
                    old_size = MemoryManager::GetSizeOfAllocation(*s);
                    if ((dwBytes - old_size) + h->m_heap_current_size > h->m_heap_maximum_size &&
                        h->m_heap_maximum_size != 0 &&
                        ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                         (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
                    {
                        if (heap_locked == TRUE)
                        {
                            MyHeapUnlock(hHeap);
                        }
                        RaiseException(STATUS_NO_MEMORY, 0, 0, nullptr);
                    }
                    if ((h->m_heap_flags & HEAP_CREATE_ENABLE_EXECUTE) == HEAP_CREATE_ENABLE_EXECUTE)
                    {
                        alloc_flags |= MemoryManager::ALLOC_EXECUTE;
                    }
                    if ((dwFlags & HEAP_ZERO_MEMORY) == HEAP_ZERO_MEMORY)
                    {
                        alloc_flags |= MemoryManager::ALLOC_ZEROINIT;
                    }
                    if ((dwFlags & HEAP_REALLOC_IN_PLACE_ONLY) == HEAP_REALLOC_IN_PLACE_ONLY)
                    {
                        alloc_flags |= MemoryManager::REALLOC_NO_MOVE;
                    }
                    rv = MemoryManager::Reallocate(*s, dwBytes, alloc_flags);
                    if (rv != nullptr)
                    {
                        SIZE_T new_size = MemoryManager::GetSizeOfAllocation(rv);
                        h->m_heap_current_size += (new_size - old_size);
                        if (rv != lpMem)
                        {
                            h->m_heap_segments.erase(s);
                            h->m_heap_segments.push_back(rv);
                        }

                    }
                    if (heap_locked == TRUE)
                    {
                        MyHeapUnlock(hHeap);
                    }
                    if (rv == nullptr &&
                        ((h->m_heap_flags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS ||
                         (dwFlags & HEAP_GENERATE_EXCEPTIONS) == HEAP_GENERATE_EXCEPTIONS))
                    {
                        RaiseException(STATUS_NO_MEMORY, 0, 0, nullptr);
                    }
                }
            }
        }
    }
    return rv;
}

/*
 * Ripped directly out of WINE.
 */
HOOKFUNC BOOL WINAPI MyHeapSetInformation(HANDLE HeapHandle,
                                          HEAP_INFORMATION_CLASS HeapInformationClass,
                                          PVOID HeapInformation,
                                          SIZE_T HeapInformationLength)
{
    ENTER(HeapHandle);
    return TRUE;
}

HOOKFUNC SIZE_T WINAPI MyHeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
    ENTER(hHeap, dwFlags, lpMem);
    BOOL heap_locked = FALSE;
    SIZE_T rv = 0;
    for (auto& h : g_heaps())
    {
        if (h == hHeap)
        {
            if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
            {
                MyHeapLock(hHeap);
                heap_locked = TRUE;
            }
            for (auto& s : h->m_heap_segments)
            {
                if (s == lpMem)
                {
                    rv = MemoryManager::GetSizeOfAllocation(s);
                    goto heap_size_done;
                }
            }
            SetLastError(ERROR_INVALID_PARAMETER);
            goto heap_size_done;
        }
    }
    SetLastError(ERROR_INVALID_HANDLE);
heap_size_done:
    if (heap_locked == TRUE)
    {
        MyHeapUnlock(hHeap);
    }
    return rv;
}

HOOKFUNC BOOL WINAPI MyHeapUnlock(HANDLE hHeap)
{
    ENTER(hHeap);
    for (auto& h : g_heaps())
    {
        if (h == hHeap)
        {
            h->m_heap_lock.clear();
            return TRUE;
        }
    }
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

HOOKFUNC BOOL WINAPI MyHeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
    ENTER(dwFlags, lpMem, hHeap);
    BOOL heap_locked = FALSE;
    BOOL rv = FALSE;
    if (hHeap != nullptr)
    {
        for (auto &h : g_heaps())
        {
            if (h == hHeap)
            {
                if ((h->m_heap_flags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE ||
                    (dwFlags & HEAP_NO_SERIALIZE) != HEAP_NO_SERIALIZE)
                {
                    MyHeapLock(hHeap);
                    heap_locked = TRUE;
                }
                for (auto& s : h->m_heap_segments)
                {
                    if (lpMem == nullptr || s == lpMem)
                    {
                        if (MyIsBadReadPtr(s, MemoryManager::GetSizeOfAllocation(s)) == FALSE &&
                            MyIsBadWritePtr(s, MemoryManager::GetSizeOfAllocation(s)) == FALSE)
                        {
                            rv = TRUE;
                            goto heap_validate_done;
                        }
                    }
                }
            }
        }
    }
heap_validate_done:
    if (heap_locked == TRUE)
    {
        MyHeapUnlock(hHeap);
    }
    return rv;
}

HOOKFUNC BOOL WINAPI MyHeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry)
{
    ENTER(hHeap, lpEntry);
    MessageBoxA(nullptr,//gamehwnd,
                __FUNCTION__ " is not implemented.\n"
                "The application will most likely experience problems now.",
                "Warning!",
                MB_ICONWARNING | MB_OK);
    /*
     * Set a possibly valid error message.
     */
    SetLastError(ERROR_NOACCESS);
    return FALSE;
}

HOOKFUNC BOOL WINAPI MyIsBadCodePtr(FARPROC lpfn)
{
    ENTER(lpfn);
    return IsBadCodePtr(lpfn);
}

HOOKFUNC BOOL WINAPI MyIsBadReadPtr(const void *lp, UINT_PTR ucb)
{
    ENTER(lp, ucb);
    return IsBadReadPtr(lp, ucb);
}

HOOKFUNC BOOL WINAPI MyIsBadStringPtrA(LPCSTR lpsz, UINT_PTR ucchMax)
{
    ENTER(lpsz, ucchMax);
    return IsBadStringPtrA(lpsz, ucchMax);
}

HOOKFUNC BOOL WINAPI MyIsBadStringPtrW(LPCWSTR lpsz, UINT_PTR ucchMax)
{
    ENTER(lpsz, ucchMax);
    return IsBadStringPtrW(lpsz, ucchMax);
}

HOOKFUNC BOOL WINAPI MyIsBadWritePtr(LPVOID lp, UINT_PTR ucb)
{
    ENTER(lp, ucb);
    return IsBadWritePtr(lp, ucb);
}

/*
 * The only difference from our point of view between Local* and Global*
 * is the value of the LMEM_DISCARDABLE flag.
 * Add special handling for that, and make Local* an "alias" for Global*
 */
HOOKFUNC HLOCAL WINAPI MyLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
    ENTER(uFlags, uBytes);
    if ((uFlags & LMEM_DISCARDABLE) == LMEM_DISCARDABLE)
    {
        uFlags &= ~LMEM_DISCARDABLE;
        uFlags |= GMEM_DISCARDABLE;
    }
    return MyGlobalAlloc(uFlags, uBytes);
}

HOOKFUNC SIZE_T WINAPI MyLocalCompact(UINT uMinFree)
{
    ENTER(uMinFree);
    return 0;
}

HOOKFUNC UINT WINAPI MyLocalFlags(HLOCAL hMem)
{
    ENTER(hMem);
    UINT rv = MyGlobalFlags(hMem);
    if (rv != GMEM_INVALID_HANDLE && (rv & GMEM_DISCARDABLE) == GMEM_DISCARDABLE)
    {
        rv |= LMEM_DISCARDABLE;
    }
    return rv;
}

HOOKFUNC HLOCAL WINAPI MyLocalFree(HLOCAL hMem)
{
    ENTER(hMem);
    return MyGlobalFree(hMem);
}

HOOKFUNC HLOCAL WINAPI MyLocalHandle(LPCVOID pMem)
{
    ENTER(pMem);
    return MyGlobalHandle(pMem);
}

HOOKFUNC LPVOID WINAPI MyLocalLock(HLOCAL hMem)
{
    ENTER(hMem);
    return MyGlobalLock(hMem);
}

HOOKFUNC HLOCAL WINAPI MyLocalReAlloc(HLOCAL hMem, SIZE_T uBytes, UINT uFlags)
{
    ENTER(hMem, uBytes, uFlags);
    if ((uFlags & LMEM_DISCARDABLE) == LMEM_DISCARDABLE)
    {
        uFlags &= ~LMEM_DISCARDABLE;
        uFlags |= GMEM_DISCARDABLE;
    }
    return MyGlobalReAlloc(hMem, uBytes, uFlags);
}

HOOKFUNC SIZE_T WINAPI MyLocalShrink(HLOCAL hMem, UINT cbNewSize)
{
    ENTER(hMem, cbNewSize);
    return 0;
}

HOOKFUNC SIZE_T WINAPI MyLocalSize(HLOCAL hMem)
{
    ENTER(hMem);
    return MyGlobalSize(hMem);
}

HOOKFUNC BOOL WINAPI MyLocalUnlock(HLOCAL hMem)
{
    ENTER(hMem);
    return MyGlobalUnlock(hMem);
}

//HOOKFUNC LPVOID WINAPI MyVirtualAlloc(LPVOID lpAddress,
//                                      SIZE_T dwSize,
//                                      DWORD flAllocationType,
//                                      DWORD flProtect)
//{
//    ENTER(lpAddress, dwSize, flAllocationType, flProtect);
//    return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
//}
//
//HOOKFUNC LPVOID WINAPI MyVirtualAllocEx(HANDLE hProcess,
//                                        LPVOID lpAddress,
//                                        SIZE_T dwSize,
//                                        DWORD flAllocationType,
//                                        DWORD flProtect)
//{
//    ENTER(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
//    if (hProcess == GetCurrentProcess())
//    {
//        return MyVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
//    }
//    MessageBoxA(nullptr,//gamehwnd,
//                __FUNCTION__ " is not implemented.\n"
//                "The application will most likely experience problems with SaveStates now.",
//                "Warning!",
//                MB_ICONWARNING | MB_OK);
//    return VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD flFreeType)
//{
//    ENTER(lpAddress, dwSize, flFreeType);
//    return VirtualFree(lpAddress, dwSize, flFreeType);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualFreeEx(HANDLE hProcess,
//                                     LPVOID lpAddress,
//                                     SIZE_T dwSize,
//                                     DWORD flFreeType)
//{
//    ENTER(hProcess, lpAddress, dwSize, flFreeType);
//    if (hProcess == GetCurrentProcess())
//    {
//        return MyVirtualFree(lpAddress, dwSize, flFreeType);
//    }
//    MessageBoxA(nullptr,//gamehwnd,
//                __FUNCTION__ " is not implemented.\n"
//                "The application will most likely experience problems with SaveStates now.",
//                "Warning!",
//                MB_ICONWARNING | MB_OK);
//    return VirtualFreeEx(hProcess, lpAddress, dwSize, flFreeType);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualLock(LPVOID lpAddress, SIZE_T dwSize)
//{
//    ENTER(lpAddress, dwSize);
//    return VirtualLock(lpAddress, dwSize);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualProtect(LPVOID lpAddress,
//                                      SIZE_T dwSize,
//                                      DWORD flNewProtect,
//                                      PDWORD lpflOldProtect)
//{
//    ENTER(lpAddress, dwSize, flNewProtect, lpflOldProtect);
//    return VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualProtectEx(HANDLE hProcess,
//                                        LPVOID lpAddress,
//                                        SIZE_T dwSize,
//                                        DWORD flNewProtect,
//                                        PDWORD lpflOldProtect)
//{
//    ENTER(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
//    if (hProcess == GetCurrentProcess())
//    {
//        return MyVirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
//    }
//    MessageBoxA(nullptr,//gamehwnd,
//                __FUNCTION__ " is not implemented.\n"
//                "The application will most likely experience problems with SaveStates now.",
//                "Warning!",
//                MB_ICONWARNING | MB_OK);
//    return VirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
//}
//
//HOOKFUNC BOOL WINAPI MyVirtualUnlock(LPVOID lpAddress, SIZE_T dwSize)
//{
//    ENTER(lpAddress, dwSize);
//    return VirtualUnlock(lpAddress, dwSize);
//}

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
        /*MAKE_INTERCEPT(1, KERNEL32, VirtualAlloc),
        MAKE_INTERCEPT(1, KERNEL32, VirtualAllocEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualFree),
        MAKE_INTERCEPT(1, KERNEL32, VirtualFreeEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualLock),
        MAKE_INTERCEPT(1, KERNEL32, VirtualProtect),
        MAKE_INTERCEPT(1, KERNEL32, VirtualProtectEx),
        MAKE_INTERCEPT(1, KERNEL32, VirtualUnlock)*/
    };
    ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}
