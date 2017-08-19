/*
* Copyright (c) 2017- Hourglass Resurrection Team
* Hourglass Resurrection is licensed under GPL v2.
* Refer to the file COPYING.txt in the project root.
*/

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

template<DWORD to>
inline DWORD AlignValueTo(DWORD val)
{
    static_assert(to > 0, "'to' cannot be 0 nor negative");
    static_assert((to & (to - 1)) == 0, "'to' must be a power of 2");
    return (val + (to - 1)) & ~(to - 1);
}
