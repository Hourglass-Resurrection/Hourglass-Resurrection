/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <windows.h>
#include <algorithm>
#include "AudioConverterStream.h"

AudioConverterStream::AudioConverterStream(WaveFormat sourceFormat, WaveFormat destFormat, WaveFormat** prevSourceFormats , int numPrevSourceFormats)
	: outBuffer(nullptr), outSize(0), outBufferAllocated(0), stream(nullptr), isProxy(false), m_failed(false), subConverter(nullptr), startOffset(0)
{
	if (sourceFormat.GetSize() == destFormat.GetSize())
	{
		LPWAVEFORMATEX src = static_cast<LPWAVEFORMATEX>(sourceFormat);
		LPWAVEFORMATEX dest = static_cast<LPWAVEFORMATEX>(destFormat);

		if (memcmp(src, dest, sourceFormat.GetSize()) == 0)
		{
			// Source and destination formats are identical, so we'll just pass the input through to
			// the output
			isProxy = true;
			return;
		}
	}

	MMRESULT mm = acmStreamOpen(&stream, nullptr, sourceFormat, destFormat, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME);

	if (mm != MMSYSERR_NOERROR)
	{
		// Not supported directly, so try letting ACM suggest an intermediate format, so we can
		// perform the conversion in multiple stages.
		WaveFormat intermediateFormat;

		// This weird looping structure is because we must try all combinations of four flags, and
		// even when a given combination yields a valid suggestion, we must be able to backtrack and
		// continue looping (tryMoreSuggestions) if the conversion based on that suggestion later
		// fails or dead-ends.
		bool foundSuggest = false;
		int chan, samp, bits, form, done;

		for (done = 0; done < 1 && !foundSuggest; foundSuggest ? 0 : ++done)
		{
			for (chan = 1; chan >= 0 && !foundSuggest; foundSuggest ? 0 : --chan)
			{
				for (samp = 1; samp >= 0 && !foundSuggest; foundSuggest ? 0 : --samp)
				{
					for (bits = 0; bits <= 1 && !foundSuggest; foundSuggest ? 0 : ++bits)
					{
						for (form = 0; form <= 1 && !foundSuggest; foundSuggest ? 0 : ++form)
						{
							{
								int flags = 0;
								flags |= chan ? ACM_FORMATSUGGESTF_NCHANNELS      : 0;
								flags |= samp ? ACM_FORMATSUGGESTF_NSAMPLESPERSEC : 0;
								flags |= bits ? ACM_FORMATSUGGESTF_WBITSPERSAMPLE : 0;
								flags |= form ? ACM_FORMATSUGGESTF_WFORMATTAG     : 0;

								intermediateFormat = destFormat;
								MMRESULT mmSuggest = acmFormatSuggest(NULL, sourceFormat, intermediateFormat, intermediateFormat.GetSize(), flags);
								if (mmSuggest == MMSYSERR_NOERROR)
								{
									// Got a possibly-valid suggestion, but it might be a suggestion to
									// do absolutely nothing (which would be bad), so we first make sure
									// there's some sort of change involved:
									if (!FormatsMatch(sourceFormat, intermediateFormat))
									{
										// We got a suggestion
										foundSuggest = true;

										// Now check to see if it's identical to a previous conversion
										// state. If it is, then we'll revert foundSuggest to false to
										// prevent endless conversion cycles.
										for (int prev = 0; prev < numPrevSourceFormats && prevSourceFormats && foundSuggest; prev++)
										{
											WaveFormat& oldFormat = *prevSourceFormats[prev];

											if (FormatsMatch(oldFormat, intermediateFormat))
											{
												// We already went through this exact format
												foundSuggest = false;
											}
										}
									}
								}
							}
						tryMoreSuggestions:
							continue;
						}
					}
				}
			}
		}

		if (!foundSuggest)
		{
			m_failed = true;
			return;
		}

		// we'll handle conversion to the intermediate format
		mm = acmStreamOpen(&stream, nullptr, sourceFormat, intermediateFormat, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME);
		if (mm != MMSYSERR_NOERROR)
		{
			if (!done)
			{
				foundSuggest = false;
				goto tryMoreSuggestions; // continue the search
			}

			// reached dead end
			m_failed = true;
			return;
		}

		// create temporary updated conversion history for cycle prevention
		size_t prevSize = sizeof(WaveFormat*) * (numPrevSourceFormats + 1);
		WaveFormat** prevFormats = static_cast<WaveFormat**>(alloca(prevSize));

		if (prevSourceFormats)
		{
			memcpy(prevFormats, prevSourceFormats, prevSize);
		}

		prevFormats[numPrevSourceFormats] = &sourceFormat;

		// delegate the rest of the conversion to a new converter (recursive construction)
		subConverter = new AudioConverterStream(intermediateFormat, destFormat, prevFormats, numPrevSourceFormats + 1);

		if (subConverter->m_failed)
		{
			delete subConverter;
			subConverter = nullptr;

			if (!done)
			{
				foundSuggest = false;
				goto tryMoreSuggestions; // continue the search
			}

			// reached dead end
			m_failed = true;
			return;
		}
	}

	// prepare the stream header
	memset(&header, 0, sizeof(ACMSTREAMHEADER));
	header.cbStruct = sizeof(ACMSTREAMHEADER);
	header.pbSrc = inWorkBuffer;
	header.cbSrcLength = sizeof(inWorkBuffer);
	header.pbDst = outWorkBuffer;
	header.cbDstLength = sizeof(outWorkBuffer);
	mm = acmStreamPrepareHeader(stream, &header, 0);

	if (mm != MMSYSERR_NOERROR)
	{
		m_failed = true;
	}
}

AudioConverterStream::~AudioConverterStream()
{
	if (isProxy)
	{
		return;
	}

	delete subConverter;
	acmStreamUnprepareHeader(stream, &header, 0);
	acmStreamClose(stream, 0);
	free(outBuffer);
}

ConvertOutput AudioConverterStream::Convert(const BYTE* inBuffer, int inSize)
{
	if (isProxy)
	{
		// Source and destination formats are identical, so just pass the input through to the output
		ConvertOutput rv = { const_cast<BYTE*>(inBuffer), inSize };
		return rv;
	}

	outSize = 0;
	int prevSrcLength = header.cbSrcLength;
	int usedInSize = 0;

	while (inSize + startOffset)
	{
		int curInSize = std::min(inSize, static_cast<int>(sizeof(inWorkBuffer) - startOffset));
		memcpy(inWorkBuffer + startOffset, inBuffer + usedInSize, curInSize);
		usedInSize += curInSize;
		inSize -= curInSize;
		header.cbSrcLength = curInSize + startOffset;
		MMRESULT mm = acmStreamConvert(stream, &header, ACM_STREAMCONVERTF_BLOCKALIGN);

		if (mm != MMSYSERR_NOERROR)
		{
			m_failed = true;
			break;
		}

		// Append to output
		int prevOutSize = outSize;
		outSize += header.cbDstLengthUsed;
		ReserveOutBuffer(outSize);
		memcpy(outBuffer + prevOutSize, outWorkBuffer, header.cbDstLengthUsed);

		// Recycle unused input
		startOffset = header.cbSrcLength - header.cbSrcLengthUsed;
		
		// startOffset = number of leftover bytes
		if (startOffset > 0 && header.cbSrcLengthUsed)
		{
			memmove(inWorkBuffer, inWorkBuffer + header.cbSrcLengthUsed, startOffset);
		}

		if (header.cbDstLengthUsed == 0 && header.cbSrcLengthUsed == 0)
		{
			break;
		}
	}

	header.cbSrcLength = prevSrcLength;

	if (header.cbDstLengthUsed == 0 && header.cbSrcLengthUsed == 0 && startOffset == sizeof(inWorkBuffer))
	{
		m_failed = true;
	}

	if (subConverter)
	{
		return subConverter->Convert(outBuffer, outSize);
	}

	ConvertOutput rv = { outBuffer, outSize };
	return rv;
}

void AudioConverterStream::ReserveOutBuffer(int size)
{
	if (outBufferAllocated < size)
	{
		outBufferAllocated = size;
		outBuffer = static_cast<BYTE*>(realloc(outBuffer, size));
	}
}

bool AudioConverterStream::FormatsMatch(const WaveFormat& formatA, const WaveFormat& formatB)
{
	return formatA.m_format->nChannels      == formatB.m_format->nChannels
		&& formatA.m_format->nSamplesPerSec == formatB.m_format->nSamplesPerSec
		&& formatA.m_format->wBitsPerSample == formatB.m_format->wBitsPerSample
		&& formatA.m_format->wFormatTag     == formatB.m_format->wFormatTag;
}
