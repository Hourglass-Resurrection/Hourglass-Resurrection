/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

 #pragma once

/*
 * Assert macro for fatal states in the DLL.
 * When Hourglass captures the exception it prints the call stack which in combination with the
 * rest of the log gives us an idea how the bad state was reached.
 * This can't be a function call as that may in some cases alter the call stack too much.
 */
 #define DLL_ASSERT() \
    _asm \
    { \
        int 3\
    }
