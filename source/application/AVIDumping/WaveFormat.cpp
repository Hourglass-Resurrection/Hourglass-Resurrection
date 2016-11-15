/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "WaveFormat.h"

WaveFormat::WaveFormat(const LPWAVEFORMATEX format) : m_format(nullptr)
{
	Init(format);
}

WaveFormat::WaveFormat(const WAVEFORMATEX& format) : m_format(nullptr)
{
	Init(const_cast<LPWAVEFORMATEX>(&format));
}

WaveFormat::WaveFormat(const WaveFormat& format) : m_format(nullptr)
{
	Init(format.m_format);
}

WaveFormat::~WaveFormat()
{
	free(m_format);
}

WaveFormat& WaveFormat::operator =(const WaveFormat& format)
{
	Init(format.m_format);
	return *this;
}

void WaveFormat::Init(const LPWAVEFORMATEX format)
{
	if (format == m_format)
	{
		return;
	}

	if (m_format)
	{
		free(m_format);
		m_format = nullptr;
	}

	if (format)
	{
		size_t size = sizeof(WAVEFORMATEX) + format->cbSize;

		// This simplifies in-place format changing
		acmMetrics(nullptr, ACM_METRIC_MAX_SIZE_FORMAT, &size);

		// We use malloc instead of new, because LPWAVEFORMATEX has a variable size
		m_format = static_cast<LPWAVEFORMATEX>(malloc(size));
		memcpy(m_format, format, size);
	}
}

WaveFormat::operator LPWAVEFORMATEX()
{
	return m_format;
}

WaveFormat::operator const LPWAVEFORMATEX() const
{
	return m_format;
}

size_t WaveFormat::GetSize()
{
	if (m_format)
	{
		return sizeof(WAVEFORMATEX) + m_format->cbSize;
	}
	else
	{
		return 0;
	}
}

size_t WaveFormat::GetMaxSize()
{
	if (m_format)
	{
		size_t size = GetSize();
		acmMetrics(nullptr, ACM_METRIC_MAX_SIZE_FORMAT, &size);
		return size;
	}
	else
	{
		return 0;
	}
}
