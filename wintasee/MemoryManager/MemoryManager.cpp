/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <atomic>
#include <list>

#include "MemoryManager.h"
#include <print.h>
#include <Utils.h>

namespace MemoryManagerInternal
{
    static bool memory_manager_inited = false;

    static const ptrdiff_t LARGE_ADDRESS_SPACE_START = 0x80000000;
    static const ptrdiff_t LARGE_ADDRESS_SPACE_END = 0xC0000000;
    static ptrdiff_t minimum_allowed_address = 0;
    static ptrdiff_t maximum_allowed_address = 0;
    static DWORD allocation_granularity = 0;

    template<class T>
    class InternalAllocator :
        public ManagedAllocator<T>
    {
    public:
        typedef typename ManagedAllocator<T>::value_type           value_type;
        typedef typename ManagedAllocator<T>::pointer              pointer;
        typedef typename ManagedAllocator<T>::const_pointer        const_pointer;
        typedef typename ManagedAllocator<T>::reference            reference;
        typedef typename ManagedAllocator<T>::const_reference      const_reference;
        typedef typename ManagedAllocator<T>::size_type            size_type;
        typedef typename ManagedAllocator<T>::difference_type      difference_type;

        template<class U>
        struct rebind
        {
            typedef InternalAllocator<U>    other;
        };

        __nothrow InternalAllocator() {}
        __nothrow InternalAllocator(const InternalAllocator& alloc) {}
        template<class U>
        __nothrow InternalAllocator(const InternalAllocator<U>& alloc) {}

        pointer allocate(size_type n, const_pointer hint = 0)
        {
            debugprintf(__FUNCTION__ " called.\n");
            return reinterpret_cast<pointer>(AllocateUnprotected(n * sizeof(value_type), 0, true));
        }
        void deallocate(pointer p, size_type n)
        {
            debugprintf(__FUNCTION__ " called.\n");
            DeallocateUnprotected(p);
        }
    };

    struct MemoryBlockDescription
    {
        LPVOID block_address;
        UINT bytes;
        bool free;
    };

    struct MemoryObjectDescription
    {
        LPVOID address;
        HANDLE object;
        UINT bytes;
        UINT flags;
        std::list<MemoryBlockDescription,
                  InternalAllocator<MemoryBlockDescription>> blocks;
    };

    static std::list<MemoryObjectDescription,
                     InternalAllocator<MemoryObjectDescription>> memory_objects;

    static LPVOID AllocateUnprotected(UINT bytes, UINT flags, bool internal);
    static LPVOID AllocateInExistingBlock(UINT bytes, UINT flags, bool internal);
    static LPVOID AllocateWithNewBlock(UINT bytes, UINT flags, bool internal);

    static LPVOID ReallocateUnprotected(LPVOID address, UINT bytes, UINT flags, bool internal);

    static void DeallocateUnprotected(LPVOID address);

    bool ObjectComparator(const MemoryObjectDescription& first,
                          const MemoryObjectDescription& second)
    {
        return first.address < second.address;
    }

    bool BlockComparator(const MemoryBlockDescription& first,
                         const MemoryBlockDescription& second)
    {
        return first.block_address < second.block_address;
    }
};

template<class T1, class T2>
bool operator==(const MemoryManagerInternal::InternalAllocator<T1>&,
                const MemoryManagerInternal::InternalAllocator<T2>&)
{
    return true;
}

template<class T1, class T2>
bool operator!= (const MemoryManagerInternal::InternalAllocator<T1>&,
                 const MemoryManagerInternal::InternalAllocator<T2>&)
{
    return false;
}

LPVOID MemoryManagerInternal::AllocateInExistingBlock(UINT bytes, UINT flags, bool internal)
{
    ptrdiff_t start_address = internal ? LARGE_ADDRESS_SPACE_START : minimum_allowed_address;
    /*
     * TODO: Check for large address awareness and decide upon that instead of internal when
     *       deciding the end_address.
     * -- Warepire
     */
    ptrdiff_t end_address = internal ? LARGE_ADDRESS_SPACE_END : LARGE_ADDRESS_SPACE_START;
    MemoryObjectDescription* best_object = nullptr;
    MemoryBlockDescription* best_block = nullptr;
    bytes += (8 - (bytes % 8));
    debugprintf(__FUNCTION__ "(0x%X 0x%X %s) called.\n", bytes, flags, internal == true ? "true" : "false");
    for (auto& mo : memory_objects)
    {
        if ((mo.flags & 0x3) != (flags & 0x3) &&
            reinterpret_cast<ptrdiff_t>(mo.address) < start_address &&
            reinterpret_cast<ptrdiff_t>(mo.address) > end_address)
        {
            continue;
        }
        for (auto& mb : mo.blocks)
        {
            if (mb.free == true && mb.bytes >= bytes)
            {
                if (best_block == nullptr || best_block->bytes > mb.bytes)
                {
                    best_object = &mo;
                    best_block = &mb;
                }
                /*
                 * Perfect block found, stop searching.
                 */
                if (mb.bytes == bytes)
                {
                    goto memory_block_search_done;
                }
            }
        }
    }

memory_block_search_done:
    if (best_block == nullptr)
    {
        return nullptr;
    }
    LPVOID allocation = best_block->block_address;
    if (best_block->bytes == bytes)
    {
        best_block->free = false;
    }
    else
    {
        MemoryBlockDescription mbd;
        mbd.block_address = allocation;
        mbd.bytes = bytes;
        mbd.free = false;

        best_block->block_address = static_cast<LPBYTE>(best_block->block_address) + bytes;
        best_block->bytes -= bytes;

        best_object->blocks.push_back(mbd);

        best_object->blocks.sort(BlockComparator);

        if ((flags & MemoryManager::ALLOC_ZEROINIT) == MemoryManager::ALLOC_ZEROINIT)
        {
            memset(allocation, 0, bytes);
        }
    }

    return allocation;
}

LPVOID MemoryManagerInternal::AllocateWithNewBlock(UINT bytes, UINT flags, bool internal)
{
    /*
     * Calculate the size of the mapped file and make sure the allocation is a multible of 8 bytes.
     */
    SIZE_T file_size = allocation_granularity;
    bytes += (8 - (bytes % 8));
    debugprintf(__FUNCTION__ "(0x%X 0x%X %s) called.\n", bytes, flags, internal == true ? "true" : "false");
    while (bytes > file_size)
    {
        file_size += allocation_granularity;
    }

    /*
     * Find the best suitable address to allocate at.
     */
    ptrdiff_t start_address = internal ? LARGE_ADDRESS_SPACE_START : minimum_allowed_address;
    /*
     * TODO: Check for large address awareness and decide upon that instead of internal when
     *       deciding the end_address.
     * -- Warepire
     */
    ptrdiff_t end_address = internal ? LARGE_ADDRESS_SPACE_END : LARGE_ADDRESS_SPACE_START;
    MEMORY_BASIC_INFORMATION best_gap;
    memset(&best_gap, 0, sizeof(best_gap));
    MEMORY_BASIC_INFORMATION this_gap;
    LPVOID current_address = reinterpret_cast<LPVOID>(start_address);

    while (reinterpret_cast<ptrdiff_t>(current_address) < end_address)
    {
        VirtualQuery(current_address, &this_gap, sizeof(this_gap));
        current_address = static_cast<LPBYTE>(current_address) + allocation_granularity;
        if (this_gap.State == MEM_FREE && this_gap.RegionSize >= file_size &&
            (best_gap.RegionSize > this_gap.RegionSize || best_gap.RegionSize == 0))
        {
            best_gap = this_gap;
            /*
             * Perfect match, break early.
             */
            if (best_gap.RegionSize == file_size)
            {
                break;
            }
        }
    }
    /*
     * No memory allocation possible, no space available to allocate at.
     */
    if (best_gap.RegionSize == 0)
    {
        return nullptr;
    }

    /*
     * Ensure GetLastError() returns an error-code from CreateFileMapping.
     */
    SetLastError(ERROR_SUCCESS);
    HANDLE map_file = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        nullptr,
                                        PAGE_EXECUTE_READWRITE,
                                        0,
                                        file_size,
                                        nullptr);
    if (GetLastError() == ERROR_ALREADY_EXISTS || map_file == nullptr)
    {
        return nullptr;
    }

    DWORD access = FILE_MAP_WRITE;
    if ((flags & MemoryManager::ALLOC_READONLY) == MemoryManager::ALLOC_READONLY)
    {
        access = FILE_MAP_READ;
    }
    if ((flags & MemoryManager::ALLOC_EXECUTE) == MemoryManager::ALLOC_EXECUTE)
    {
        access |= FILE_MAP_EXECUTE;
    }
    void *allocation = MapViewOfFileEx(map_file,
                                       access,
                                       0,
                                       0,
                                       file_size,
                                       best_gap.BaseAddress);
    if (allocation == nullptr)
    {
        CloseHandle(map_file);
        return nullptr;
    }

    if ((flags & MemoryManager::ALLOC_ZEROINIT) == MemoryManager::ALLOC_ZEROINIT)
    {
        memset(allocation, 0, bytes);
    }
    MemoryObjectDescription mod;
    MemoryBlockDescription mbd;
    mod.address = allocation;
    mod.object = map_file;
    mod.bytes = file_size;
    mod.flags = flags;
    mbd.block_address = allocation;
    mbd.bytes = bytes;
    mbd.free = false;
    mod.blocks.push_back(mbd);
    mbd.block_address = static_cast<LPBYTE>(allocation) + bytes;
    mbd.bytes = file_size - bytes;
    mbd.free = true;
    mod.blocks.push_back(mbd);

    memory_objects.push_back(mod);

    memory_objects.sort(ObjectComparator);

    return allocation;
}

LPVOID MemoryManagerInternal::AllocateUnprotected(UINT bytes, UINT flags, bool internal)
{
    debugprintf(__FUNCTION__ "(0x%X 0x%X %s) called.\n", bytes, flags, internal == true ? "true" : "false");
    if (bytes == 0)
    {
        return nullptr;
    }
    /*
     * TODO: Decide a naming scheme, to easier ID segments in a save state.
     *       Inform wintaser about region changes.
     * -- Warepire
     */
    /*
     * If the allocation needs 50% or more of a block, go immediately for a new block.
     * This includes allocations that needs more than one block.
     */
    if ((bytes * 2) < allocation_granularity)
    {
        LPVOID allocation = AllocateInExistingBlock(bytes * 2, flags, internal);
        if (allocation != nullptr)
        {
            return allocation;
        }
    }
    return AllocateWithNewBlock(bytes, flags, internal);
}

LPVOID MemoryManagerInternal::ReallocateUnprotected(LPVOID address, UINT bytes, UINT flags, bool internal)
{
    debugprintf(__FUNCTION__ "(0x%p 0x%X 0x%X %s) called.\n", address, bytes, flags, internal == true ? "true" : "false");

    if (address == nullptr)
    {
        return AllocateUnprotected(bytes, flags, internal);
    }
    if (bytes == 0)
    {
        DeallocateUnprotected(address);
        return nullptr;
    }

    UINT realloc_bytes = (bytes * 2) + (8 - ((bytes * 2) % 8));

    for (auto mod = memory_objects.begin(); mod != memory_objects.end(); ++mod)
    {
        if (mod->address <= address &&
            (static_cast<LPBYTE>(mod->address) + mod->bytes) > address)
        {
            for (auto mbd = mod->blocks.begin(); mbd != mod->blocks.end(); ++mbd)
            {
                if (mbd->block_address == address)
                {
                    /*
                     * Attempt to adjust at the current location.
                     */
                    if ((mod->flags & 0x3) == (flags & 0x3) &&
                        realloc_bytes < allocation_granularity)
                    {
                        INT adjustment = (realloc_bytes - mbd->bytes);
                        if (adjustment == 0)
                        {
                            return mbd->block_address;
                        }
                        auto temp_mbd = mbd;
                        ++temp_mbd;
                        if (temp_mbd != mod->blocks.end() &&
                            static_cast<INT64>(temp_mbd->bytes) > static_cast<INT64>(adjustment))
                        {
                            bool adjusted = true;
                            if (temp_mbd->free == true)
                            {
                                if (temp_mbd->bytes == adjustment)
                                {
                                    mod->blocks.erase(temp_mbd);
                                }
                                else
                                {
                                    temp_mbd->block_address =
                                        static_cast<LPBYTE>(temp_mbd->block_address) + adjustment;
                                    temp_mbd->bytes -= adjustment;
                                }
                            }
                            else if (adjustment < 0)
                            {
                                MemoryBlockDescription new_mbd;
                                new_mbd.block_address =
                                    static_cast<LPBYTE>(temp_mbd->block_address) - adjustment;
                                new_mbd.bytes = abs(adjustment);
                                new_mbd.free = true;
                                mod->blocks.push_back(new_mbd);

                                mod->blocks.sort(BlockComparator);
                            }
                            else
                            {
                                adjusted = false;
                            }
                            if (adjusted == true)
                            {
                                mbd->bytes = realloc_bytes;
                                if ((flags & MemoryManager::ALLOC_ZEROINIT) ==
                                        MemoryManager::ALLOC_ZEROINIT &&
                                    adjustment > 0)
                                {
                                    memset(static_cast<LPBYTE>(mbd->block_address) + adjustment,
                                           0,
                                           adjustment);
                                }
                                return mbd->block_address;
                            }
                        }
                    }
                    if ((flags & MemoryManager::REALLOC_NO_MOVE) == MemoryManager::REALLOC_NO_MOVE)
                    {
                        return nullptr;
                    }
                    /*
                     * Adjustment is not possible, allocate somewhere else.
                     */
                    LPVOID allocation = AllocateUnprotected(bytes, flags, internal);
                    if (allocation == nullptr)
                    {
                        return nullptr;
                    }
                    memcpy(allocation, address, mbd->bytes);
                    DeallocateUnprotected(address);
                    return allocation;
                }
            }
        }
    }
    return nullptr;
}

void MemoryManagerInternal::DeallocateUnprotected(LPVOID address)
{
    debugprintf(__FUNCTION__ "(0x%p) called.\n", address);
    if (address == nullptr)
    {
        return;
    }

    for (auto mod = memory_objects.begin(); mod != memory_objects.end(); ++mod)
    {
        if (mod->address <= address &&
            (static_cast<LPBYTE>(mod->address) + mod->bytes) > address)
        {
            for (auto mbd = mod->blocks.begin(); mbd != mod->blocks.end(); ++mbd)
            {
                if (mbd->block_address == address)
                {
                    /*
                     * Attempt to merge blocks.
                     */
                    auto temp_mbd = mbd;
                    ++temp_mbd;
                    if (temp_mbd != mod->blocks.end() && temp_mbd->free == true)
                    {
                        mbd->bytes += temp_mbd->bytes;
                        mod->blocks.erase(temp_mbd);
                        mbd->free = true;
                    }
                    else if (mbd != mod->blocks.begin())
                    {
                        --temp_mbd;
                        --temp_mbd;
                        if (temp_mbd->free == true)
                        {
                            temp_mbd->bytes += mbd->bytes;
                            mod->blocks.erase(mbd);
                        }
                    }
                    /*
                     * Check if the block is still in use.
                     */
                    if (mod->blocks.size() != 1)
                    {
                        return;
                    }
                    else
                    {
                        UnmapViewOfFile(mod->address);
                        CloseHandle(mod->object);
                        /*
                         * TODO: Inform wintaser about removed region.
                         * -- Warepire
                         */
                        memory_objects.erase(mod);
                        return;
                    }
                }
            }
        }
    }
    debugprintf(__FUNCTION__ " WARNING: Attempted removal of unknown memory!.\n");
}

std::atomic_flag MemoryManager::allocation_lock;

void MemoryManager::Init()
{
    debugprintf(__FUNCTION__ "(void) called.\n");
    if (MemoryManagerInternal::memory_manager_inited)
    {
        DLL_ASSERT();
    }
    /*
     * TODO: Call GetSystemInfo in the executable?
     * -- Warepire
     */
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    MemoryManagerInternal::minimum_allowed_address =
        reinterpret_cast<ptrdiff_t>(si.lpMinimumApplicationAddress);
    MemoryManagerInternal::maximum_allowed_address =
        reinterpret_cast<ptrdiff_t>(si.lpMaximumApplicationAddress);
    /*
     * TODO: Will this need to be calculated using dwPageSize?
     * -- Warepire
     */
    MemoryManagerInternal::allocation_granularity = si.dwAllocationGranularity;
    allocation_lock.clear();
    MemoryManagerInternal::memory_manager_inited = true;
}

LPVOID MemoryManager::Allocate(UINT bytes, UINT flags, bool internal)
{
    if (MemoryManagerInternal::memory_manager_inited == false)
    {
        DLL_ASSERT();
    }
    while (allocation_lock.test_and_set() == true);
    LPVOID rv = MemoryManagerInternal::AllocateUnprotected(bytes, flags, internal);
    allocation_lock.clear();
    return rv;
}

LPVOID MemoryManager::Reallocate(LPVOID address, UINT bytes, UINT flags, bool internal)
{
    if (MemoryManagerInternal::memory_manager_inited == false)
    {
        DLL_ASSERT();
    }
    while (allocation_lock.test_and_set() == true);
    LPVOID rv = MemoryManagerInternal::ReallocateUnprotected(address, bytes, flags, internal);
    allocation_lock.clear();
    return rv;
}

void MemoryManager::Deallocate(LPVOID address)
{
    if (MemoryManagerInternal::memory_manager_inited == false)
    {
        DLL_ASSERT();
    }
    while (allocation_lock.test_and_set() == true);
    MemoryManagerInternal::DeallocateUnprotected(address);
    allocation_lock.clear();
}

SIZE_T MemoryManager::GetSizeOfAllocation(LPCVOID address)
{
    if (MemoryManagerInternal::memory_manager_inited == false)
    {
        DLL_ASSERT();
    }
    while (allocation_lock.test_and_set() == true);
    SIZE_T rv = 0;
    debugprintf(__FUNCTION__ "(0x%p) called.\n", address);
    if (address != nullptr)
    {
        for (auto mod = MemoryManagerInternal::memory_objects.begin();
             mod != MemoryManagerInternal::memory_objects.end();
             ++mod)
        {
            if (mod->address <= address &&
                (static_cast<LPBYTE>(mod->address) + mod->bytes) > address)
            {
                for (auto mbd = mod->blocks.begin(); mbd != mod->blocks.end(); ++mbd)
                {
                    if (mbd->block_address == address)
                    {
                        if (mbd->free != true)
                        {
                            rv = mbd->bytes;
                        }
                        goto get_size_of_allocation_done;

                    }
                }
            }
        }
    }
get_size_of_allocation_done:
    allocation_lock.clear();
    return rv;
}
