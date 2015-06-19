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
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal)
    {
        return nullptr;
    }
    /*
     * This one is so that we can mark memory mapped file addresses as "used" in our
     * manager, this way we can produce better hints in our Best Fit allocation methods.
     * If we cannot register the allocation, it's unfortunate, but we should still be able
     * to provide fair enough hints, so there's no need for fatal failure.
     */
    void RegisterMemoryMappedFileAllocation(void* address, SIZE_T bytes)
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
                 * TODO: Log warning that getting size failed.
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
