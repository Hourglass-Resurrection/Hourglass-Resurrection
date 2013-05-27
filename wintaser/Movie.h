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
	char author[64]; // Authors will have to do with 63 characters (last char needed for null-termination).
	char keyboardLayoutName [KL_NAMELENGTH]; // "00000409" for standard US layout
	unsigned int fps;
	unsigned int it;
	unsigned int crc;
	unsigned int fsize;
	char exefname[(MAX_PATH+1)-3]; // Technically the file name can be 257 chars max, though it'd stupid we have to cover it.
	int desyncDetectionTimerValues [16];
	unsigned int version;
	char commandline[8192-(MAX_PATH+1)]; // Windows can handle command lines of 8191 chars + null termination. This includes the program path though so we need to make sure our command line remains valid.
	bool headerBuilt; // When true, the header is properly populated.
	
	// note: these aren't the only things in the movie file format.
	// see SaveMovieToFile or LoadMovieFromFile for the rest.
	Movie();
};

/*static*/ bool SaveMovieToFile(Movie& movie, char* filename);
/*static*/ bool LoadMovieFromFile(/*out*/ Movie& movie, const char* filename/*, bool forPreview=false*/);