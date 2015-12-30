/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <list>

#include "MemoryManager.h"

/*
 * TODO: Critical sections around allocations?
 * -- Warepire
 */

bool MemoryManagerInternal::ms_memory_manager_inited = false;
ptrdiff_t MemoryManagerInternal::ms_minimum_allowed_address = 0;
ptrdiff_t MemoryManagerInternal::ms_maximum_allowed_address = 0;
DWORD MemoryManagerInternal::ms_allocation_granularity = 0;
std::list<MemoryManagerInternal::MemoryObjectDescription,
          ManagedAllocator<MemoryManagerInternal::MemoryObjectDescription>>
              MemoryManagerInternal::ms_memory_objects;

LPVOID MemoryManagerInternal::AllocateInExistingBlock(UINT bytes, bool internal)
{
    ptrdiff_t start_address = internal ? LARGE_ADDRESS_SPACE_START : ms_minimum_allowed_address;
    /*
     * TODO: Check for large address awareness and decide upon that instead of internal when
     *       deciding the end_address.
     * -- Warepire
     */
    ptrdiff_t end_address = internal ? LARGE_ADDRESS_SPACE_END : LARGE_ADDRESS_SPACE_START;
    MemoryObjectDescription* best_object = nullptr;
    MemoryBlockDescription* best_block = nullptr;
    for (auto& mo : ms_memory_objects)
    {
        if (mo.flags != 0 &&
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

        auto Comparator = [](const MemoryBlockDescription& first,
                             const MemoryBlockDescription& second)
        {
            return first.block_address < second.block_address;
        };
        best_object->blocks.sort(Comparator);
    }

    return allocation;
}

LPVOID MemoryManagerInternal::AllocateWithNewBlock(UINT bytes, UINT flags, bool internal)
{
    /*
     * Calculate the size of the mapped file.
     */
    SIZE_T file_size = ms_allocation_granularity;
    while (bytes > file_size)
    {
        file_size += ms_allocation_granularity;
    }

    /*
     * Find the best suitable address to allocate at.
     */
    ptrdiff_t start_address = internal ? LARGE_ADDRESS_SPACE_START : ms_minimum_allowed_address;
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
        current_address = static_cast<LPBYTE>(current_address) + ms_allocation_granularity;
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
                                        PAGE_READWRITE,
                                        0,
                                        file_size,
                                        nullptr);
    if (GetLastError() == ERROR_ALREADY_EXISTS || map_file == nullptr)
    {
        return nullptr;
    }
    void *allocation = MapViewOfFileEx(map_file,
                                       flags == 0 ? FILE_MAP_WRITE : flags,
                                       0,
                                       0,
                                       file_size,
                                       best_gap.BaseAddress);
    if (allocation == nullptr)
    {
        return nullptr;
    }
    MemoryManagerInternal::MemoryObjectDescription mod;
    MemoryManagerInternal::MemoryBlockDescription mbd;
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

    ms_memory_objects.push_back(mod);

    auto Comparator = [](const MemoryObjectDescription& first,
                         const MemoryObjectDescription& second)
    {
        return first.address < second.address;
    };
    ms_memory_objects.sort(Comparator);

    return allocation;
}

/*
 * TODO: Broken
 */
SIZE_T MemoryManagerInternal::FindLargestInternalGap()
{
    ptrdiff_t gap = 0;
    ptrdiff_t this_gap;
    ptrdiff_t last_memory_block_end = 0;
    for (auto& mo : ms_memory_objects)
    {
        if (reinterpret_cast<ptrdiff_t>(mo.address) >= LARGE_ADDRESS_SPACE_START &&
            reinterpret_cast<ptrdiff_t>(mo.address) < LARGE_ADDRESS_SPACE_END)
        {
            if (last_memory_block_end != 0)
            {
                this_gap =
                    (last_memory_block_end - reinterpret_cast<ptrdiff_t>(mo.address));
                if (gap < this_gap)
                {
                    gap = this_gap;
                }
            }
            last_memory_block_end = reinterpret_cast<ptrdiff_t>(mo.address) + mo.bytes;
        }
    }
    return static_cast<SIZE_T>(gap);
}

void MemoryManager::Init()
{
    if (MemoryManagerInternal::ms_memory_manager_inited)
    {
        /*
         * TODO: Assert?
         *       Trying to init twice or more would be a horrible bug somewhere.
         * -- Warepire
         */
        return;
    }
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    MemoryManagerInternal::ms_minimum_allowed_address =
        reinterpret_cast<ptrdiff_t>(si.lpMinimumApplicationAddress);
    MemoryManagerInternal::ms_maximum_allowed_address =
        reinterpret_cast<ptrdiff_t>(si.lpMaximumApplicationAddress);
    /*
     * TODO: Will this need to be calculated using dwPageSize?
     * -- Warepire
     */
    MemoryManagerInternal::ms_allocation_granularity = si.dwAllocationGranularity;
    MemoryManagerInternal::ms_memory_manager_inited = true;
}

void* MemoryManager::Allocate(UINT bytes, UINT flags, bool internal)
{
    if (bytes == 0)
    {
        return nullptr;
    }
    /*
     * TODO: Decide a naming scheme, to easier ID segments in a save state.
     *       Inform wintaser about region changes.
     * -- Warepire
     */
    if (flags == 0 &&
        /*
         * If the allocation needs 50% or more of a block, go immediately for a new block.
         * This includes allocations that needs more than one block.
         */
        (bytes * 2) < MemoryManagerInternal::ms_allocation_granularity)
    {
        LPVOID allocation = MemoryManagerInternal::AllocateInExistingBlock(bytes * 2, internal);
        if (allocation != nullptr)
        {
            return allocation;
        }
    }
    return MemoryManagerInternal::AllocateWithNewBlock(bytes, flags, internal);
}

void* MemoryManager::Reallocate(LPVOID address, UINT bytes, UINT flags, bool internal)
{
    if (address == nullptr)
    {
        return Allocate(bytes, flags, internal);
    }
    if (bytes == 0)
    {
        Deallocate(address);
        return nullptr;
    }

    for (auto mod = MemoryManagerInternal::ms_memory_objects.begin();
         mod != MemoryManagerInternal::ms_memory_objects.end();
         ++mod)
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
                    if (flags == 0 && mod->flags == 0 &&
                        (bytes * 2) < MemoryManagerInternal::ms_allocation_granularity)
                    {
                        INT adjustment = ((bytes * 2) - mbd->bytes);
                        auto temp_mbd = mbd;
                        ++temp_mbd;
                        if (temp_mbd != mod->blocks.end() &&
                            temp_mbd->free == true &&
                            static_cast<INT64>(temp_mbd->bytes) > static_cast<INT64>(adjustment))
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
                            mbd->bytes = bytes * 2;
                            return mbd->block_address;
                        }
                    }
                    /*
                     * Adjustment is not possible, allocate somewhere else.
                     */
                    LPVOID allocation = Allocate(bytes, flags, internal);
                    if (allocation == nullptr)
                    {
                        return nullptr;
                    }
                    memcpy(allocation, address, mbd->bytes);
                    Deallocate(address);
                    return allocation;
                }
            }
        }
    }
    return nullptr;
}

void MemoryManager::Deallocate(LPVOID address)
{
    if (address == nullptr)
    {
        return;
    }

    for (auto mod = MemoryManagerInternal::ms_memory_objects.begin();
         mod != MemoryManagerInternal::ms_memory_objects.end();
         ++mod)
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
                        MemoryManagerInternal::ms_memory_objects.erase(mod);
                        return;
                    }
                }
            }
        }
    }
    /*
     * TODO: Log warning about trying to remove something we don't have a entry of!
     * -- Warepire
     */
}
