/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <vector>

#include "MemoryManager.h"

/*
 * TODO: Critical sections around allocations?
 * -- Warepire
 */

bool MemoryManagerInternal::memory_manager_inited = false;
ptrdiff_t MemoryManagerInternal::minimum_allowed_address = 0;
std::vector<MemoryManagerInternal::MemoryObjectDescription,
            ManagedAllocator<MemoryManagerInternal::MemoryObjectDescription>>
                MemoryManagerInternal::memory_objects;

void MemoryManagerInternal::InsertMemoryObjectDescription(MemoryObjectDescription& mod)
{
    auto i = memory_objects.begin();
    while (i != memory_objects.end())
    {
        if (i->address > mod.address)
        {
            break;
        }
        i++;
    }
    if (i == memory_objects.begin())
    {
        memory_objects.insert(memory_objects.begin(), mod);
    }
    else if (i == memory_objects.end())
    {
        memory_objects.push_back(mod);
    }
    else
    {
        memory_objects.insert(i, mod);
    }
}

void* MemoryManagerInternal::FindBestFitAddress(unsigned int bytes, bool internal)
{
    struct FreeSpace {
        void* address;
        SIZE_T size;
    };

    ptrdiff_t start_address = internal ? LARGE_ADDRESS_SPACE_START : minimum_allowed_address;
    ptrdiff_t end_address = internal ? LARGE_ADDRESS_SPACE_END : LARGE_ADDRESS_SPACE_START;
    FreeSpace best_gap;
    ZeroMemory(&best_gap, sizeof(best_gap));
    FreeSpace this_gap;
    void* candidate_address = nullptr;

    for (auto& mo : memory_objects)
    {
        if (reinterpret_cast<ptrdiff_t>(mo.address) >= start_address &&
            reinterpret_cast<ptrdiff_t>(mo.address) < end_address)
        {
            if (candidate_address != nullptr)
            {
                this_gap.address = candidate_address;
                this_gap.size = static_cast<char*>(candidate_address)
                                - static_cast<char*>(mo.address);
                if (this_gap.size > bytes)
                {
                    if (best_gap.size > this_gap.size || best_gap.size == 0)
                    {
                        best_gap = this_gap;
                    }
                }
            }
            candidate_address = static_cast<char*>(mo.address) + mo.bytes;
        }
    }
    if (best_gap.size == 0)
    {
        return nullptr;
    }
    return best_gap.address;
}

SIZE_T MemoryManagerInternal::FindLargestInternalGap()
{
    ptrdiff_t gap = 0;
    ptrdiff_t this_gap;
    ptrdiff_t last_memory_block_end = 0;
    for (auto& mo : memory_objects)
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
    /*
     * TODO: Inform wintaser about added regions.
     * -- Warepire
     */
    if (MemoryManagerInternal::memory_manager_inited)
    {
        /*
         * TODO: Print warning about trying to init again! Or assert?
         *       Trying to init again would be a horrible bug somewhere.
         * -- Warepire
         */
        return;
    }
    MEMORY_BASIC_INFORMATION mbi;
    void* allocation_base;
    SIZE_T region_size = 0;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    MemoryManagerInternal::minimum_allowed_address =
        reinterpret_cast<ptrdiff_t>(si.lpMinimumApplicationAddress);
    void* region_address = si.lpMinimumApplicationAddress;
    VirtualQuery(region_address, &mbi, sizeof(mbi));
    allocation_base = mbi.AllocationBase;
    while (region_address < si.lpMaximumApplicationAddress)
    {
        /*
         * Add full allocations and not page-regions.
         * This prevents excess MemoryObjectDescriptions for the same object.
         */
        while (mbi.State != MEM_FREE && mbi.AllocationBase == allocation_base)
        {
            region_size += mbi.RegionSize;
            region_address = static_cast<BYTE*>(mbi.BaseAddress) + mbi.RegionSize;
            VirtualQuery(region_address, &mbi, sizeof(mbi));
        }
        if (mbi.State == MEM_FREE && region_size != 0)
        {
            RegisterExistingAllocation(allocation_base, region_size);
            region_size = 0;
        }
        region_address = static_cast<BYTE*>(mbi.BaseAddress) + mbi.RegionSize;
        allocation_base = mbi.AllocationBase;
        VirtualQuery(region_address, &mbi, sizeof(mbi));
    }
    MemoryManagerInternal::memory_manager_inited = true;
}

void* MemoryManager::Allocate(unsigned int bytes, unsigned int flags, bool internal)
{
    /*
     * TODO: Decide a naming scheme, to easier ID segments in a save state.
     * TODO: Inform wintaser about added region.
     * -- Warepire
     */
    void* target_address = MemoryManagerInternal::FindBestFitAddress(bytes, internal);
    /*
     * Ensure GetLastError() returns an error-code from CreateFileMapping.
     */
    SetLastError(ERROR_SUCCESS);
    HANDLE map_file = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        nullptr,
                                        PAGE_READWRITE,
                                        0,
                                        bytes,
                                        nullptr);
    if (GetLastError() == ERROR_ALREADY_EXISTS || map_file == nullptr)
    {
        return nullptr;
    }
    void *allocation = MapViewOfFileEx(map_file,
                                       flags == 0 ? FILE_MAP_WRITE : flags,
                                       0,
                                       0,
                                       bytes,
                                       target_address);
    if (allocation == nullptr)
    {
        return nullptr;
    }
    MemoryManagerInternal::MemoryObjectDescription mod;
    mod.address = allocation;
    mod.object = map_file;
    mod.bytes = bytes;
    mod.flags = flags;
    MemoryManagerInternal::InsertMemoryObjectDescription(mod);
    return allocation;
}

void MemoryManager::RegisterExistingAllocation(void* address, unsigned int bytes)
{
    /*
     * TODO: Inform wintaser about added region.
     * -- Warepire
     */
    if (bytes == 0)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(address, &mbi, sizeof(mbi)) != 0)
        {
            bytes = mbi.RegionSize;
        }
        else
        {
            /*
             * TODO: Log warning that getting size failed and that future allocations may crash.
             * -- Warepire
             */
        }
    }
    if (bytes != 0)
    {
        MemoryManagerInternal::MemoryObjectDescription mod;
        mod.address = address;
        mod.object = INVALID_HANDLE_VALUE;
        mod.bytes = bytes;
        mod.flags = 0;
        MemoryManagerInternal::InsertMemoryObjectDescription(mod);
    }
}

void MemoryManager::Deallocate(void* object)
{
    auto it = MemoryManagerInternal::memory_objects.begin();
    for (; it != MemoryManagerInternal::memory_objects.end(); it++)
    {
        if (it->address == object)
        {
            /*
             * TODO: Inform wintaser about removed region.
             * -- Warepire
             */
            /*
             * Only unmap and close allocations that are not set to INVALID_HANDLE_VALUE,
             * INVALID_HANDLE_VALUE is used to denote an allocation not made by us, so we
             * cannot safely destroy it.
             * Since this scenario would only occur if a stack is deallocated or DLL is
             * unloaded, we shouldn't in theory leak any memory doing it like this.
             * -- Warepire
             */
            if (it->object != INVALID_HANDLE_VALUE)
            {
                UnmapViewOfFile(it->address);
                CloseHandle(it->object);
            }
            MemoryManagerInternal::memory_objects.erase(it);
            break;
        }
    }
    if (it == MemoryManagerInternal::memory_objects.end())
    {
        /*
         * TODO: Log warning about trying to remove something we don't have a entry of!
         * -- Warepire
         */
    }
}
