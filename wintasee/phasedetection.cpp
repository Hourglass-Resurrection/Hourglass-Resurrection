/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <MemoryManager\MemoryManager.h>
#include <phasedetection.h>
#include <print.h>

bool PhaseDetector::AdvanceAndCheckCycleBoundary(Key key)
{
	bool isCycleBoundary = true;
	auto& keyFound = keyTimes.find(key);
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
