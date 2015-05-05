/*
* (c) 2015- Hourglass Resurrection Team
* Hourglass Resurrection is licensed under GPL v2.
* Refer to the file COPYING.txt in the project root.
*/

#pragma once

#include <cstdlib>
#include "WaveFormat.h"

struct ConvertOutput
{
	BYTE* m_buffer;
	int m_size;
};

class AudioConverterStream
{
public:
	AudioConverterStream(WaveFormat sourceFormat, WaveFormat destFormat, WaveFormat** prevSourceFormats = nullptr, int numPrevSourceFormats = 0);
	~AudioConverterStream();
	ConvertOutput Convert(const BYTE* inBuffer, int inSize);
	bool m_failed;

private:
	BYTE* outBuffer;
	int outSize;
	ACMSTREAMHEADER header;
	BYTE inWorkBuffer[16 * 1024];
	BYTE outWorkBuffer[4 * 1024];
	int outBufferAllocated;
	int startOffset;
	HACMSTREAM stream;
	bool isProxy;
	AudioConverterStream* subConverter;
	void ReserveOutBuffer(int size);
	bool FormatsMatch(const WaveFormat& formatA, const WaveFormat& formatB);
};
