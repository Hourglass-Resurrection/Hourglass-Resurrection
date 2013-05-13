/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef PHASEDETECTION_H_INCL
#define PHASEDETECTION_H_INCL

#include <map>

class PhaseDetector
{
public:
	typedef unsigned long Key;
	typedef unsigned long Time;

	bool AdvanceAndCheckCycleBoundary(Key key);

	void Reset();
	PhaseDetector() : maxDiscardDist(15) { Reset(); }

	Time maxDiscardDist;

private:
	Time currentTime;
	Time lastFrameTime;
	std::map<Key,Time> keyTimes;
};

#endif
