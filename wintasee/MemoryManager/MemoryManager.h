/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>

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
 * It is really important to keep the allocations apart, since some games that are not capable of
 * handling addresses of >2 GB may still use as much of the 2 GB as possible. We must for that
 * reason be really careful about our own allocations in that space and do all we can to stay in
 * our own allotted region.
 *
 * All of the above means that new and malloc are entirely forbidden(!) in the DLL. To handle STL
 * containers special allocators were implemented, these must be used with any STL container inside
 * the DLL code.
 *
 * The best approach would be to use namespaces, as that would be much prettier, but unfortunately
 * that does not play along with STL allocators as they cannot be forward-declared.
 * This causes the problem that the entire anonymous namespace would be defined in the header-file,
 * eliminating all the advantages of an anonymous namespace. Therefor this class variant with all
 * static members is used instead, and to protect the memory manager core as much as possible, it's
 * declared fully private, and defines the classes that shall have access to it's internals as
 * friends.
 *
 * Reference material (including reference material linked by the webpage):
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa366912%28v=vs.85%29.aspx
 *
 * -- Warepire
 */
class MemoryManager
{
public:
    /*
     * We need to discover everything that has been allocated before we started running.
     * This is to make sure we do not try to allocate memory where memory is already allocted.
     */
    static void Init();
    static void* Allocate(unsigned int bytes, unsigned int flags, bool internal = false);
    /*
     * This one is so that we can mark used file addresses as "used" in our manager, this way we
     * can produce valid addresses for our allocations.
     */
    static void RegisterExistingAllocation(void* address, unsigned int bytes);
    static void Deallocate(void* object);
};

/*
 * Gotta break some of the coding standard here as the function names etc are expected to be named
 * like they are here. Otherwise the allocator doesn't work.
 * -- Warepire
 */
template<class T>
class ManagedAllocator
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
        typedef ManagedAllocator<U> other;
    };

    /*
     * Microsoft cannot follow standards with nothrow-declaring stuff... sigh...
     */
    __nothrow ManagedAllocator() {}
    __nothrow ManagedAllocator(const ManagedAllocator& alloc) {}
    template<class U>
    __nothrow ManagedAllocator(const ManagedAllocator<U>& alloc) {}
    ~ManagedAllocator() {}

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
        return MemoryManagerInternal::FindLargestInternalGap();
    }

    template<class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        /*
         * Placement-new, will not attempt to allocate space.
         */
        ::new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
    }
    template<class U>
    void destroy(U* p)
    {
        p->~U();
    }
};

/*
 * I don't like exposing this class as much as I am, but it's necessary since the STL allocator
 * class needs access to internal functions here to be able to perform the max_size() operation.
 * At least everything here is private.
 * -- Warepire
 */
class MemoryManagerInternal
{
private:
    friend class MemoryManager;
    template<class T>
    friend class ManagedAllocator;

    static bool memory_manager_inited;
    static const ptrdiff_t LARGE_ADDRESS_SPACE_START = 0x80000000;
    static const ptrdiff_t LARGE_ADDRESS_SPACE_END = 0xC0000000;
    static ptrdiff_t minimum_allowed_address;
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
     * Do NOT add objects to this vector manually, use the below function.
     */
    static std::vector<MemoryObjectDescription,
                       ManagedAllocator<MemoryObjectDescription>> memory_objects;

    /*
     * Helper function to insert MemoryObjectDescriptions sorted, this way it becomes easier
     * to find the best spots in memory to place new allocations.
     */
    static void InsertMemoryObjectDescription(MemoryObjectDescription& mod);

    /*
     * Returns a nullptr if no suitable address was found.
     * This lets MapViewOfFileEx try on it's own, but if we failed, the chances are not good.
     */
    static void* FindBestFitAddress(unsigned int bytes, bool internal = false);

    static SIZE_T FindLargestInternalGap();
};
