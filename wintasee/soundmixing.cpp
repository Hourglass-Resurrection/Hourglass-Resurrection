/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(SOUNDMIXING_C_INCL) //&& !defined(UNITY_BUILD)
#define SOUNDMIXING_C_INCL

#include "global.h"

// this is used mainly for clamping from -32678 to 32767
//#define clamptofullsignedrange(x,lo,hi) ((x)>=(lo)?((x)<=(hi)?(x):(hi)):(lo))
// but, this version is faster for the no-need-to-clamp case:
#define clamptofullsignedrange(x,lo,hi) (((unsigned int)((x)-(lo))<=(unsigned int)((hi)-(lo)))?(x):(((x)<0)?(lo):(hi)))
// TODO: maybe _mm_adds_pi16 or _mm_adds_pi8 would be even faster? especially if vectorized.

#if _MSC_VER < 1400 && !defined(__restrict)
#define __restrict 
#endif

// this is the mixing uber-function that does all the hard work.
// there is a heavy focus on performance here,
// so this tries to do everything and do it "well enough", with nothing really fancy.
// 
// FREQUENCY ADJUSTMENT:
//   this handles conversion from any frequency to any other frequency.
//   the adjustment amount is decided implicitly based on the ratio between size and outSize.
// BIT RATE CONVERSION:
//   this handles all possible combinations of bitrate conversions between the following types:
//   (8-bit signed, 8-bit unsigned, 16-bit signed, and 16-bit unsigned)
//   as determined by the types you specify for fromtype and totype.
//   in practice, only the following 4 combinations are useful:
//   (8-bit unsigned to 8-bit unsigned, 16-bit signed to 16-bit signed,
//    8-bit unsigned to 16-bit signed, and 16-bit signed to 8-bit unsigned)
//   however, it is recommended the mixing destination buffer always be 16-bit, for quality reasons.
// CHANNEL CONVERSION:
//   this handles all 4 possible combinations of channel conversions
//   (mono to mono, stereo to stereo, mono to stereo, and stereo to mono)
//   as determined by the values you specify for fromchannels and tochannels.
//   however, it is recommended the mixing destination buffer always be stereo, for quality reasons.
// PANNING AND VOLUME ADJUSTMENT:
//   the cached volume levels given are applied to the source audio when mixing.
//   this automatically includes panning, which applies for every case except (mono to mono) conversion.
// CLIPPING:
//   values exceeding the bounds of the destination bitrate will be clamped to within the valid range.
// INTERPOLATION:
//   this performs linear interpolation of samples.
//   some games would sound very noticeably wrong otherwise.
template<typename fromtype, typename totype, int fromchannels, int tochannels>
static void Mix(const unsigned char*__restrict buf, unsigned char*__restrict outbuf, DWORD size, DWORD outSize, bool sizeReachesBufferEnd, CachedVolumeAndPan& volumes)
{
	enum { fromshift = (2+sizeof(fromtype)-sizeof(totype))<<3 }; // 16 when both buffers have the same bit/sample... this is for combining differing bitrates
	enum { maxto = (1<<(8*sizeof(totype)-1))-1 }; // clamping magnitude (127 or 32767)
	enum { tosignoffset = (totype(-1)<0)?0:-(1<<(8*sizeof(totype)-1)) }; // add this to make numbers in the "to" buffer signed
	enum { fromsignoffset = (fromtype(-1)<0)?0:-(1<<(8*sizeof(fromtype)-1)) }; // add this to make numbers in the "from" buffer signed
	enum { toincrement = sizeof(totype) * tochannels };
	enum { fromincrement = sizeof(fromtype) * fromchannels };
	
	DWORD frac = 0; // amount of next sample to use, out of 1024 (for linear interpolation)
	DWORD fracnumer = (size*(toincrement<<10));
	DWORD fracincrement = fracnumer / outSize;
	// unfortunately needs to be very exact (can't drift even by 1 or clicking becomes audible), so we also need:
	DWORD fracErrorIncrement = fracnumer % outSize;
	DWORD fracError = 0;

	DWORD offsetRemainder = 0;
	DWORD inOffset = 0;
	for(DWORD i = 0; i < outSize; outbuf += toincrement, i += toincrement)
	{
		DWORD offset = inOffset;
		offset -= offset % fromincrement; // prevent starting from the wrong speaker (compiler should be smart enough not to do a modulo op here)
		const unsigned char* inbuf = buf + offset;
		offset += fromincrement;
		if(sizeReachesBufferEnd && (offset > (size - fromincrement))) // take pos2IsLastSample into account
			offset = size - fromincrement; // note: don't subtract bufferSize instead, that doesn't work right (slight clicking) even in the looping case
		const unsigned char* inbuf2 = buf + offset; // next sample position (for interpolation)
		int myL = (int)((fromtype*)inbuf)[0] + fromsignoffset;
		int myR = (int)((fromtype*)inbuf)[fromchannels-1] + fromsignoffset;
		int myL2 = (int)((fromtype*)inbuf2)[0] + fromsignoffset;
		int myR2 = (int)((fromtype*)inbuf2)[fromchannels-1] + fromsignoffset;
		int otherL = (int)((totype*)outbuf)[0] + tosignoffset;
		int otherR = (int)((totype*)outbuf)[tochannels-1] + tosignoffset;
		int lvas = volumes.leftVolumeAsScale;
		int rvas = volumes.rightVolumeAsScale;
		int mixedL = (otherL + ((int)(((myL * (int)lvas) >> fromshift) * (1024-frac) + ((myL2 * (int)lvas) >> fromshift) * frac) >> 10));
		int mixedR = (otherR + ((int)(((myR * (int)rvas) >> fromshift) * (1024-frac) + ((myR2 * (int)rvas) >> fromshift) * frac) >> 10));
		if(tochannels != 1)
		{
			// stereo output
			((totype*)outbuf)[0] = (clamptofullsignedrange(mixedL,-maxto-1,maxto))-tosignoffset;
			((totype*)outbuf)[1] = (clamptofullsignedrange(mixedR,-maxto-1,maxto))-tosignoffset;
		}
		else
		{
			// monaural output
			int mixed = (mixedL + mixedR) >> 1;
			((totype*)outbuf)[0] = clamptofullsignedrange(mixed,-maxto-1,maxto)-tosignoffset;
		}

		// I'm going to lots of trouble to avoid using integer division or modulus
		// in this inner loop, since they used to be slowing it down drastically.
		// in my tests, this function was using 60% of the frame time (in cave story),
		// and now it runs fully 4 times faster than it did before.

		offsetRemainder += size*toincrement;
		while(offsetRemainder >= outSize) // definitely needs to be while, not if
		{
			offsetRemainder -= outSize;
			inOffset++;
		}

		fracError += fracErrorIncrement;
		if(fracError >= outSize)
		{
			fracError -= outSize;
			frac++;
		}
		frac = (frac + fracincrement) & 0x3FF;
	}
}

template<int fromchannels, int tochannels>
static void Mix(const unsigned char* buf, unsigned char* outbuf, int myBitsPerSample, int outBitsPerSample, DWORD size, DWORD outSize, bool sizeReachesBufferEnd, CachedVolumeAndPan& volumes)
{
	// note: WAV uses unsigned for 8-bit and signed for 16-bit (it makes a big difference!)
	if(myBitsPerSample <= 8 && outBitsPerSample <= 8)
		Mix<unsigned char,unsigned char,fromchannels,tochannels>(buf, outbuf, size, outSize, sizeReachesBufferEnd, volumes);
	else if(myBitsPerSample > 8 && outBitsPerSample > 8)
		Mix<signed short,signed short,fromchannels,tochannels>(buf, outbuf, size, outSize, sizeReachesBufferEnd, volumes);
	else if(outBitsPerSample > 8)
		Mix<unsigned char,signed short,fromchannels,tochannels>(buf, outbuf, size, outSize, sizeReachesBufferEnd, volumes);
	else
		Mix<signed short,unsigned char,fromchannels,tochannels>(buf, outbuf, size, outSize, sizeReachesBufferEnd, volumes);
}

void MixFromToInternal(DWORD pos1, DWORD pos2, DWORD outPos1, DWORD outPos2, bool pos2IsLastSample,
	DWORD outSamplesPerSec, WORD myBitsPerSample, WORD outBitsPerSample, WORD myChannels, WORD outChannels, WORD myBlockSize, WORD outBlockSize,
	unsigned char* buffer, unsigned char* contiguousMixOutBuf, CachedVolumeAndPan& volumes)
{
	if(pos2 <= pos1 || outPos2 <= outPos1)
		return; // not allowed

	unsigned char* buf = buffer + pos1;
	const DWORD size = pos2 - pos1;
	
	unsigned char* outbuf = contiguousMixOutBuf + outPos1;
	const DWORD outSize = outPos2 - outPos1;

//		debugprintf("size=%d, outsize=%d\n", size, outSize);
	//debugprintf("blocksize=%d, outblock=%d\n", myBlockSize, outBlockSize);

	if(myChannels == 1 && outChannels == 1)
		Mix<1,1>(buf, outbuf, myBitsPerSample, outBitsPerSample, size, outSize, pos2IsLastSample, volumes);
	else if(myChannels == 1 && outChannels == 2)
		Mix<1,2>(buf, outbuf, myBitsPerSample, outBitsPerSample, size, outSize, pos2IsLastSample, volumes);
	else if(myChannels == 2 && outChannels == 1)
		Mix<2,1>(buf, outbuf, myBitsPerSample, outBitsPerSample, size, outSize, pos2IsLastSample, volumes);
	else if(myChannels == 2 && outChannels == 2)
		Mix<2,2>(buf, outbuf, myBitsPerSample, outBitsPerSample, size, outSize, pos2IsLastSample, volumes);
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
