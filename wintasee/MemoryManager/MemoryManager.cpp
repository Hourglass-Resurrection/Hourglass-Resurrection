/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <memory>
#include <vector>

#include "MemoryManager.h"

namespace MemoryManager
{
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal)
    {
        return nullptr;
    }
    void Deallocate(void* object)
    {
    }
};

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
        typedef value_type      T
        typedef pointer         T*
        typedef const_pointer   const T*
        typedef reference       T&
        typedef const_reference const T&
        typedef size_type       std::size_t
        typedef difference_type std::ptrdiff_t
        typedef rebind          template<class U> struct rebind { typedef MemoryObjectsAllocator<U> other; };

        MemoryObjectsAllocator() nothrow;
        MemoryObjectsAllocator(const MemoryObjectAllocator& alloc) nothrow;
        template<class U>
        MemoryObjectsAllocator(const MemoryObjectAllocator<U>& alloc) nothrow;
        ~MemoryObjectsAllocator();

        pointer address(reference x) const
        {
            return &x;
        }
        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate(size_type n, MemoryObjectsAllocator<void>::const_pointer hint = 0)
        {
            MemoryManager::Allocate(n * sizeof(value_type), 0, true);
        }
        void deallocate(pointer p, size_type n)
        {
            MemoryManager::Deallocate(p);
        }

        size_type max_size() const noexcept
        {
            difference_type gap = 0;
            difference_type this_gap;
            difference_type last_memory_block_end = 0;
            for (auto& mo : memory_objects)
            {
                if (mo.address >= large_address_space_start)
                {
                    if (last_memory_block_end != 0)
                    {
                        this_gap = (last_memory_block_end - mo.address) / sizeof(value_type);
                        if (gap < this_gap)
                        {
                            gap = this_gap;
                        }
                    }
                    last_memory_block_end = mo.address + mo.bytes;
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
            ::new ((void*)p) U (std::forward<Args>(args)...);
        }
        template<class U>
        void destroy(U* p)
        {
            p->~U();
        }
    };

    /*
     * Always insert into this vector, keeping it sorted.
     */
    std::vector<MemoryObjectDescription, MemoryObjectsAllocator<MemoryObjectDescription>> memory_objects;
}
