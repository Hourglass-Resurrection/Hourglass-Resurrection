/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include "../external/ddraw.h"

#include <vfw.h>
#pragma comment(lib, "vfw32.lib")

struct WaveFormat
{
public:
    WaveFormat(const LPWAVEFORMATEX format = nullptr);
    WaveFormat(const WAVEFORMATEX& format);
    WaveFormat(const WaveFormat& format);
    WaveFormat& operator=(const WaveFormat& format);
    ~WaveFormat();
    operator LPWAVEFORMATEX();
    operator const LPWAVEFORMATEX() const;
    size_t GetSize();
    size_t GetMaxSize();
    LPWAVEFORMATEX m_format;

private:
    void Init(const LPWAVEFORMATEX format);
};
