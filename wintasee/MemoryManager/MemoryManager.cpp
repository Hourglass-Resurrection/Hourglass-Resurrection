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
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal);
    void Deallocate(void* object);
};

namespace
{
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
    class MemoryObjectsAllocator :
          public std::allocator
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

        pointer address(reference x) const;
        const_pointer address(const_reference x) const;

        pointer allocate(size_type n, MemoryObjectsAllocator<void>::const_pointer hint);
        void deallocate(pointer p, size_type n);

        size_type max_size() const noexcept;

        template<class U, class... Args>
        void construct(U* p, Args&&... args);
        template<class U>
        void destroy(U* p);
    };

    std::vector<MemoryObjectDescription, MemoryObjectsAllocator<MemoryObjectDescription>> sm_memory_objects;
}
