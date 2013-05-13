#pragma once

// not the most thought-out movie format
// Hardware supports pretty much all keys on a keyboard to be pressed at the same time,
// only most keyboards suck and only allow a few keys to be pressed at the same time.
// heldKeyIDs needs to be expanded to handle this.
struct MovieFrame
{
	//int mousePosX;
	//int mousePosY;
	//-- mouse buttons --

	unsigned char heldKeyIDs [8];
};

#include <vector>
struct Movie
{
	std::vector<MovieFrame> frames;
	int currentFrame;
	unsigned int rerecordCount;
	char keyboardLayoutName [KL_NAMELENGTH]; // "00000409" for standard US layout
	unsigned int fps;
	unsigned int it;
	unsigned int crc;
	unsigned int fsize;
	char exefname[48];
	int desyncDetectionTimerValues [16];
	unsigned int version;
	char commandline[160];
	bool headerBuilt;
	
	// note: these aren't the only things in the movie file format.
	// see SaveMovieToFile or LoadMovieFromFile for the rest.
	Movie();
};

/*static*/ bool SaveMovieToFile(Movie& movie, const char* filename);
/*static*/ bool LoadMovieFromFile(/*out*/ Movie& movie, const char* filename/*, bool forPreview=false*/);