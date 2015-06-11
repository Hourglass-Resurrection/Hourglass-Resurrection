/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>

#include <memory>
#include <vector>

/*
 * Memory Manager
 * --------------
 * This is a manager of dynamic allocations that allows us to have full control of the heap.
 * This manager uses a best fit allocation model to not waste more RAM than necessary, and it
 * allocates all the objects as Shared Memory Objects. Using Shared Memory Objects allows for easy
 * access from the executable, in order to have an effective RAM Search.
 * Another benefit to Shared Memory Objects is that it becomes easier to manage save states with
 * these over regular RAM pages as we can have full READ / WRITE access from the executable side,
 * as well as shared ownership.
 *
 * The memory manager is also responsible for the heap allocations done by the DLL itself, the way
 * it is solved is that the Allocation function takes an optional boolean to allocate internally.
 * Allocating internally means that we will use an address outside of the regular scope, but still
 * within the 32bit application limit. This is handled by being "large address aware" which gives
 * us this access to the "unreachable" >2 GB address space.
 *
 * All of the above means that new and malloc are entirely forbidden(!) in the dll, to handle STL
 * containers special allocators were implemented, these must be used with any STL container inside
 * the DLL code.
 *
 * Implemented as a fully static unnamed class to prevent any further instancing.
 *
 * -- Warepire
 */

static class MemoryManager
{
public:
    static void* Allocate(unsigned int bytes, unsigned int flags, bool internal = false);
    static void Deallocate(void* object);

private:
    static struct MemoryObjectDescription
    {
        void* address;
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

    static std::vector<MemoryObjectDescription, MemoryObjectsAllocator<MemoryObjectDescription>> sm_memory_objects;

} memory_manager;
