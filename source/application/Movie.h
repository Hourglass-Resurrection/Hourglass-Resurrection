#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include "shared/input.h"

struct MovieFrame
{
	CurrentInput inputs;
};

#include <vector>
struct Movie
{
	std::vector<MovieFrame> frames;
	int currentFrame;
	unsigned int rerecordCount;
	char author[64]; // Authors will have to do with 63 characters (last char needed for null-termination).
	char keyboardLayoutName [KL_NAMELENGTH]; // "00000409" for standard US layout
	unsigned int fps;
	unsigned int it;
	unsigned int fmd5[4]; // MD5 Checksum of the exefile
	unsigned int fsize;
	int desyncDetectionTimerValues[16];
	unsigned int version;
	char commandline[8192-1-(MAX_PATH+1)]; // Windows can handle command lines of 8192 with the null termination char. This includes the program path though so we need to make sure our command line remains valid.
	bool headerBuilt; // When true, the header is properly populated.
	
	// note: these aren't the only things in the movie file format.
	// see SaveMovieToFile or LoadMovieFromFile for the rest.
	Movie();
};

/*static*/ bool SaveMovieToFile(Movie& movie, char* filename);
/*static*/ bool LoadMovieFromFile(/*out*/ Movie& movie, const char* filename/*, bool forPreview=false*/);
