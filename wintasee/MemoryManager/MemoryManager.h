/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>

#include <atomic>

#include <print.h>

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
 * There is a downside to this approach, and that is if the games use any libc, libc++ or STL
 * contaners in it's own code, as there will be 2 layers doing the same thing, which means both the
 * game and we will be keeping a memory map, wasting a bit of RAM. This is unavoidable.
 * Microsoft also opted for enforcing an Allocation Granularity that may be much larger than the
 * page size in the system. This is to be fully compatible with the exotic Alpha AVX RISC CPU.
 * The compatibility with this CPU puts extra strain on this implementation, as it becomes extra
 * important to put small allocations in the same block, or code like
 * "char* ptr = malloc(sizeof(char));" is going to provide a very nice chock, instead of allocating
 * a small number of bytes, it will allocate at least 64 kilobytes.
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
 * https://blogs.msdn.microsoft.com/oldnewthing/20031008-00/?p=42223/
 *
 * -- Warepire
 */
class MemoryManager
{
public:
    enum AllocationFlags : UINT
    {
        ALLOC_WRITE     = 0x00000000,
        ALLOC_READONLY  = 0x00000001,
        ALLOC_EXECUTE   = 0x00000002,
        ALLOC_ZEROINIT  = 0x00000004,
        REALLOC_NO_MOVE = 0x00000008,
    };

    static void Init();

    static LPVOID Allocate(UINT bytes, UINT flags, bool internal = false);
    static LPVOID Reallocate(LPVOID address, UINT bytes, UINT flags, bool internal = false);
    static void Deallocate(LPVOID address);

    static SIZE_T GetSizeOfAllocation(LPCVOID address);

private:
    static std::atomic_flag allocation_lock;
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
    virtual ~ManagedAllocator() {}

    pointer address(reference x) const
    {
        debugprintf(__FUNCTION__ " called.\n");
        return &x;
    }
    const_pointer address(const_reference x) const
    {
        debugprintf(__FUNCTION__ " called.\n");
        return &x;
    }

    virtual pointer allocate(size_type n, const_pointer hint = 0)
    {
        debugprintf(__FUNCTION__ " called.\n");
        return reinterpret_cast<pointer>(MemoryManager::Allocate(n * sizeof(value_type), 0, true));
    }
    virtual void deallocate(pointer p, size_type n)
    {
        debugprintf(__FUNCTION__ " called.\n");
        MemoryManager::Deallocate(p);
    }

    size_type max_size() const
    {
        debugprintf(__FUNCTION__ " called.\n");
        /*
         * Copy Microsoft's own implementation of max_size.
         */
        return static_cast<size_type>(-1) / sizeof(value_type);
    }

    template<class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        debugprintf(__FUNCTION__ " called.\n");
        /*
         * Placement-new, will not attempt to allocate space.
         */
        ::new (reinterpret_cast<LPVOID>(p)) U(std::forward<Args>(args)...);
    }
    template<class U>
    void destroy(U* p)
    {
        debugprintf(__FUNCTION__ " called.\n");
        p->~U();
    }
};

/*
 * Comparators for the ManagedAllocator.
 * since it's a stateless allocator, they're equal by default.
 */
template<class T1, class T2>
bool operator==(const ManagedAllocator<T1>&, const ManagedAllocator<T2>&)
{
    return true;
}

template<class T1, class T2>
bool operator!=(const ManagedAllocator<T1>&, const ManagedAllocator<T2>&)
{
    return false;
}
