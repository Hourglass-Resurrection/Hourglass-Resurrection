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
    static const unsigned int large_address_space_start = 0x80000000;
    struct MemoryObjectDescription
    {
        void* address;  // TODO: Can this be gotten from the handle?
        HANDLE object;  // TODO: Can this be gotten from the address?
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
                if (reinterpret_cast<difference_type>(mo.address) >= large_address_space_start)
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
             * TODO: Print warning about trying to init again!
             * -- Warepire
             */
            return;
        }
        MEMORY_BASIC_INFORMATION mbi;
        void* allocation_base;
        SIZE_T region_size = 0;
        SYSTEM_INFO si;
        GetSystemInfo(&si);
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
        }
        memory_manager_inited = true;
    }
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal)
    {
        /*
         * TODO: Decide a naming scheme, to easier ID segments in a save state.
         * TODO: Write  function that scans the memory_objects vector for a
         *       suitable region of memory.
         * TODO: Know where EVERYTHING is, incl, threads, stacks etc.
         *       MapViewOfFileEx doesn't take hints, it takes orders.
         *       And fails if the address cannot contain the alloc.
         * -- Warepire
         */
        void* hint_address = nullptr;
        HANDLE map_file =
            CreateFileMapping(INVALID_HANDLE_VALUE,
                              nullptr,
                              PAGE_READWRITE,
                              0,
                              bytes,
                              nullptr);
        void *allocation =
            MapViewOfFileEx(map_file,
                            flags == 0 ? FILE_MAP_WRITE : flags,
                            0,
                            0,
                            bytes,
                            hint_address);
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
