/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <vector>

#include "MemoryManager.h"

namespace
{
    static bool memory_manager_inited = false;
    static const ptrdiff_t LARGE_ADDRESS_SPACE_START = 0x80000000;
    static const ptrdiff_t LARGE_ADDRESS_SPACE_END = 0xC0000000;
    static ptrdiff_t minimum_allowed_address = 0;
    struct MemoryObjectDescription
    {
        void* address;
        /*
         * INVALID_HANDLE_VALUE means that we must never delete the address ourselves.
         * An INVALID_HANDLE_VALUE entry means that this was allocated by Windows.
         */
        HANDLE object;
        unsigned int bytes;
        unsigned int flags;
    };

    /*
    * Special allocator for the vector we use as a memory object tracker.
    * A lot of things in this allocator must break coding standard as the internal
    * implementation of the STL container expects things to be like this.
    * -- Warepire
    */
    template<class T>
    class MemoryObjectsAllocator
    {
    public:
        typedef T               value_type;
        typedef T*              pointer;
        typedef const T*        const_pointer;
        typedef T&              reference;
        typedef const T&        const_reference;
        typedef std::size_t     size_type;
        typedef std::ptrdiff_t  difference_type;

        template<class U>
        struct rebind
        {
            typedef MemoryObjectsAllocator<U> other;
        };

        /*
        * Microsoft cannot follow standards with nothrow-declaring stuff... sigh...
        */
        __nothrow MemoryObjectsAllocator() {}
        __nothrow MemoryObjectsAllocator(const MemoryObjectsAllocator& alloc) {}
        template<class U>
        __nothrow MemoryObjectsAllocator(const MemoryObjectsAllocator<U>& alloc) {}
        ~MemoryObjectsAllocator() {}

        pointer address(reference x) const
        {
            return &x;
        }
        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate(size_type n, const_pointer hint = 0)
        {
            return reinterpret_cast<pointer>(MemoryManager::Allocate(n * sizeof(value_type), 0, true));
        }
        void deallocate(pointer p, size_type n)
        {
            MemoryManager::Deallocate(p);
        }

        size_type max_size() const
        {
            difference_type gap = 0;
            difference_type this_gap;
            difference_type last_memory_block_end = 0;
            for (auto& mo : memory_objects)
            {
                if (reinterpret_cast<difference_type>(mo.address) >= LARGE_ADDRESS_SPACE_START &&
                    reinterpret_cast<difference_type>(mo.address) < LARGE_ADDRESS_SPACE_END)
                {
                    if (last_memory_block_end != 0)
                    {
                        this_gap =
                            (last_memory_block_end
                            - reinterpret_cast<difference_type>(mo.address))
                            / sizeof(value_type);
                        if (gap < this_gap)
                        {
                            gap = this_gap;
                        }
                    }
                    last_memory_block_end = reinterpret_cast<difference_type>(mo.address) + mo.bytes;
                }
            }
            return static_cast<size_type>(gap);
        }

        template<class U, class... Args>
        void construct(U* p, Args&&... args)
        {
            /*
            * Placement-new, will not attempt to allocate space.
            */
            ::new ((void*)p) U(std::forward<Args>(args)...);
        }
        template<class U>
        void destroy(U* p)
        {
            p->~U();
        }
    };

    /*
    * Do NOT add objects to this vector manually, use the below function.
    */
    std::vector<MemoryObjectDescription,
                MemoryObjectsAllocator<MemoryObjectDescription>> memory_objects;
    /*
    * Helper function to insert MemoryObjectDescriptions sorted, this way it becomes easier
    * to find the best spots in memory to place new allocations.
    */
    void InsertMemoryObjectDescriptionSorted(MemoryObjectDescription& mod)
    {
        auto i = memory_objects.begin();
        for (; i != memory_objects.end(); i++)
        {
            if (i->address > mod.address) {
                break;
            }
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
            memory_objects.insert(--i, mod);
        }
    }

    /*
     * Returns a nullptr if no suitable address was found.
     * This lets MapViewOfFileEx try on it's own, but if we failed, the chances are not good.
     */
    void* FindBestFitAddress(unsigned int bytes, bool internal = false)
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
}


namespace MemoryManager
{
    /*
     * We need to discover everything that has been allocated before we started running.
     * This is to make sure we do not try to allocate memory where memory is already allocted.
     */
    void Init()
    {
        if (memory_manager_inited)
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
        minimum_allowed_address = reinterpret_cast<ptrdiff_t>(si.lpMinimumApplicationAddress);
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
                region_address = static_cast<char*>(mbi.BaseAddress) + mbi.RegionSize;
                VirtualQuery(region_address, &mbi, sizeof(mbi));
            }
            if (mbi.State == MEM_FREE && region_size != 0)
            {
                RegisterExistingAllocation(allocation_base, region_size);
                region_size = 0;
            }
            region_address = static_cast<char*>(mbi.BaseAddress) + mbi.RegionSize;
            allocation_base = mbi.AllocationBase;
            VirtualQuery(region_address, &mbi, sizeof(mbi));
        }
        memory_manager_inited = true;
    }
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal)
    {
        /*
         * TODO: Decide a naming scheme, to easier ID segments in a save state.
         * -- Warepire
         */
        void* target_address = FindBestFitAddress(bytes, internal);
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
        MemoryObjectDescription mod;
        mod.address = allocation;
        mod.object = map_file;
        mod.bytes = bytes;
        mod.flags = flags;
        InsertMemoryObjectDescriptionSorted(mod);
        return allocation;
    }
    /*
     * This one is so that we can mark used file addresses as "used" in our
     * manager, this way we can produce valid addresses for our allocations.
     */
    void RegisterExistingAllocation(void* address, unsigned int bytes)
    {
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
            MemoryObjectDescription mod;
            mod.address = address;
            mod.object = INVALID_HANDLE_VALUE;
            mod.bytes = bytes;
            mod.flags = 0;
            InsertMemoryObjectDescriptionSorted(mod);
        }
    }
    void Deallocate(void* object)
    {
    }
};
