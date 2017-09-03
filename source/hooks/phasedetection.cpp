/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "phasedetection.h"
#include "print.h"

using Log = DebugLog<LogCategory::DDRAW>;

bool PhaseDetector::AdvanceAndCheckCycleBoundary(Key key)
{
    bool isCycleBoundary = true;
    std::map<Key, Time>::iterator keyFound = keyTimes.find(key);
    if (keyFound != keyTimes.end())
    {
        if (keyFound->second < lastFrameTime)
        {
            if (currentTime - keyFound->second <= maxDiscardDist)
            {
                isCycleBoundary = false;
                LOG() << "frame eliminated (" << keyFound->first << ", " << keyFound->second << ")";
            }
            else
            {
                keyTimes.erase(keyFound);
            }
        }
    }
    currentTime++;
    keyTimes[key] = currentTime;
    if (isCycleBoundary)
        lastFrameTime = currentTime;
    return isCycleBoundary;
}

void PhaseDetector::Reset()
{
    currentTime = 0;
    lastFrameTime = 0;
    keyTimes.clear();
}
