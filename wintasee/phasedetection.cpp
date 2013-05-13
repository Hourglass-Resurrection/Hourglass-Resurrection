/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(PHASEDETECTION_C_INCL) && !defined(UNITY_BUILD)
#define PHASEDETECTION_C_INCL

#include "phasedetection.h"
#include "print.h"

bool PhaseDetector::AdvanceAndCheckCycleBoundary(Key key)
{
	bool isCycleBoundary = true;
	std::map<Key,Time>::iterator keyFound = keyTimes.find(key);
	if(keyFound != keyTimes.end())
	{
		if(keyFound->second < lastFrameTime)
		{
			if(currentTime - keyFound->second <= maxDiscardDist)
			{
				isCycleBoundary = false;
				ddrawdebugprintf("frame eliminated (0x%X, %d)", keyFound->first, keyFound->second);
			}
			else
			{
				keyTimes.erase(keyFound);
			}
		}
	}
	currentTime++;
	keyTimes[key] = currentTime;
	if(isCycleBoundary)
		lastFrameTime = currentTime;
	return isCycleBoundary;
}

void PhaseDetector::Reset()
{
	currentTime = 0;
	lastFrameTime = 0;
	keyTimes.clear();
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
