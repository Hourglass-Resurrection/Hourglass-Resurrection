/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

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
 * handling addresses of >2 GB may still use as much as possible of the 2 GB as possible. We must
 * for that reason be really careful about our own allocations in that space and do all we can to
 * stay in our own allotted region.
 *
 * All of the above means that new and malloc are entirely forbidden(!) in the DLL. To handle STL
 * containers special allocators were implemented, these must be used with any STL container inside
 * the DLL code.
 *
 * Implemented as a namespace to prevent it being instanciated.
 *
 * Reference material (including reference material linked by the webpage):
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa366912%28v=vs.85%29.aspx
 *
 * -- Warepire
 */

namespace MemoryManager
{
    void* Allocate(unsigned int bytes, unsigned int flags, bool internal = false);
    void Deallocate(void* object);
};
