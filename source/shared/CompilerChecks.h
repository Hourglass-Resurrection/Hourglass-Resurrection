/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
 * This header provides various macros for "noisy" alerts on hacks applied due to
 * compiler shortcomings.
 */

#define _CURRENTLY_USED_MSC_VER 1911

static_assert(!(_MSC_VER < _CURRENTLY_USED_MSC_VER), "Your toolchain version is too old. Compilation aborted. Update Visual Studio.");

#define ABORT_ON_NEW_COMPILER(str) \
        static_assert(!(_MSC_VER > _CURRENTLY_USED_MSC_VER), str)
