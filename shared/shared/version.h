#pragma once

// VERSION and MINORVERSION usage:
// VERSION is the most important define, it should be bumped everytime a change is made that can affect movie sync in any way. This is the part of
// the version number that we check against in the movies.
// MINORVERSION is for other fixes, like a crashing dialog fix etc, things that need to be fixed quickly, but doesn't affect movie sync in any way.
// This number is purely cosmetic and is only used to tell fix-builds from eachother.
// Every time VERSION is bumped, MINORVERSION should be reset to 0.

// This versioning is for release-builds only, it is not encouraged to use interim builds for TASing as they may contain unfinished features
// and other works in progress that can severly hamper the functionality of the program.

#define VERSION 1
#define MINORVERSION 0