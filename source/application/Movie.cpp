#include <windows.h>

#include "Movie.h" // <vector> included through Movie.h

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "shared\version.h"
#include "CustomDLGs.h"
//#include "..\shared\ipc.h"

//extern TasFlags localTASflags;

Movie::Movie()
{
	currentFrame = 0;
	rerecordCount = 0;
	version = VERSION;
	for(int i = 0; i < KL_NAMELENGTH; i++)
	{
		keyboardLayoutName[i] = 0;
	}
	author[0] = '\0';
	commandline[0] = '\0';
	headerBuilt = false;
	fps = 0;
	it = 0;
	memset(fmd5, 0, sizeof(fmd5));
	fsize = 0;
	memset(desyncDetectionTimerValues, 0, sizeof(desyncDetectionTimerValues));
}

// TODO: save incrementally?, and work on the format

// NOTE: FPS and InitialTime used to have +1, we don't know why and we removed it as we made the values unsigned.
// If problems arise, revert back to old behavior!
#define MOVIE_TYPE_IDENTIFIER 0x02486752 // "\02HgR"
/*static*/ bool SaveMovieToFile(Movie& movie, char* filename)
{
	//bool hadUnsaved = unsaved;
	//unsaved = false;

	//if(movie.frames.size() == 0)
	//	return true; // Technically we did "Save" the movie...

	FILE* file = fopen(filename, "wb");
	if(!file) // File failed to open
	{

		if(errno == EACCES)
		{
			// If for some reason the file has been set to read-only in Windows since the last save,
			// we attempt to save it to a new file just as a safety-measure... because people...
			int dotLocation = (strrchr(filename, '.'))-filename+1; // Wohoo for using pointers as values!
			char newFilename[MAX_PATH+1];
			strncpy(newFilename, filename, dotLocation);
			newFilename[dotLocation+1] = '\0'; // Make sure the string is null-terminated before we cat it.
			strcat(newFilename, "new.hgr\0");

			char str[1024];
			sprintf(str, "Permission on \"%s\" denied, attempting to save to %s.", filename, newFilename);
			CustomMessageBox(str, "Warning!", MB_OK | MB_ICONWARNING);

			file = fopen(newFilename, "wb");
			
			// Update to the new filename
			strcpy(filename, newFilename);
		}

		if (!file) // New file also failed OR first file failed for some other reason
		{
			char str[1024];
			sprintf(str, "Saving movie data to \"%s\" failed.\nReason: %s", filename, strerror(errno));
			CustomMessageBox(str, "Error!", MB_OK | MB_ICONERROR);
			return false;
		}

		//if(!filename || !*filename || strlen(filename) > MAX_PATH-10)
		//{
		//	debugprintf("FAILED TO SAVE MOVIE FILE.\n");
		//	return;
		//}
		//char str [1024];
		//sprintf(str, "Warning: unable to open \"%s\" for writing.\n"
		//	"The movie will be saved to \"%s.wtf\" instead.\n", filename, filename);
		//strcat((char*)filename, ".wtf");
		//CustomMessageBox(str, "Movie Save Warning", MB_OK | MB_ICONWARNING);
		//file = (filename && *filename) ? fopen(filename, "wb") : NULL;
		//if(!file)
		//{
		//	debugprintf("FAILED TO SAVE MOVIE FILE.\n");
		//	return;
		//}
	}
	
	int identifier = MOVIE_TYPE_IDENTIFIER;
	fwrite(&identifier, 4, 1, file);

	unsigned int length = movie.frames.size();
	fwrite(&length, 4, 1, file);

	unsigned int rerecords = movie.rerecordCount;
	fwrite(&rerecords, 4, 1, file);

	const char* author = movie.author;
	unsigned int authorLen = strlen(author);
	fwrite(&authorLen, 4, 1, file);
	if (authorLen) // If the author-field contains something, write that too
	{
		fwrite(author, authorLen, 1, file);
	}

	const char* keyboardLayoutName = movie.keyboardLayoutName;
	fwrite(keyboardLayoutName, 8, 1, file); // KL_NAMELENGTH == 9 and includes the NULL

	unsigned int fps = movie.fps;
	fwrite(&fps, 4, 1, file);

	unsigned int it = movie.it;
	fwrite(&it, 4, 1, file);

	// MD5 checksum
	fwrite(&movie.fmd5, 4, 4, file);

	unsigned int fsize = movie.fsize;
	fwrite(&fsize, 4, 1, file);

	fwrite(&movie.desyncDetectionTimerValues, 16, 4, file);

	unsigned int version = movie.version;
	fwrite(&version, 4, 1, file);

	// Windows limitation for command lines are 8191 characters, program included.
	// e.g. "C:/Program Files/games/some game/game.exe /whatever /and /beyond" cannot be more than 8191 characters in total.
	// No commandline should need to be that long, but we should still apply a limitation of 8191-(MAX_PATH+1) instead of 160.
	// CreateProcess has a limit of 32767 characters, which I find odd since Windows itself has a limit of 8191.
	// -- Warepire
	const char* commandline = movie.commandline;
	unsigned int commandlineLen = strlen(commandline);
	fwrite(&commandlineLen, 4, 1, file);
	if(commandlineLen) // If a commandline was entered, write that too.
	{
		fwrite(commandline, 1, commandlineLen, file);
	}

	// write remaining padding before movie data
	// Why is there an assert here?
	// TODO: clean up this assert
	// assert(ftell(file) <= 1024-4);
	//while(ftell(file) < 1024)
	//	fputc(0, file);
	// ^^^^^^^^^^^^^^^^^^^^^^^^
	// That is no longer valid since we have a header of variable size
	// Perhaps we shall introduce a fixed number of null-bytes instead?
	if(movie.frames.size() > 0)
	{
		unsigned char* input_buffer = (unsigned char*)malloc(4+1+256+13+4*sizeof(XINPUT_GAMEPAD)+1); // Max size of a frame input.
		for (unsigned int i=0; i<movie.frames.size(); i++){
			int size = movie.frames[i].inputs.serialize(input_buffer);
			fwrite(input_buffer, 1, size, file);
		}
	}

	fclose(file);

	return true;
}

// NOTE: FPS and InitialTime used to have -1, we don't know why and we removed it as we made the values unsigned.
// If problems arise, revert back to old behavior!
// returns 1 on success, 0 on failure, -1 on cancel
/*static*/ bool LoadMovieFromFile(/*out*/ Movie& movie, const char* filename/*, bool forPreview*/)
{
	//if(unsaved && forPreview)
	//	return 0; // never replace movie data with a preview if we haven't saved it yet

	FILE* file = fopen(filename, "rb");
	if(!file)
	{
		char str[1024];
		sprintf(str, "The movie file '%s' could not be opened.\nReason: %s", filename, strerror(errno));
		CustomMessageBox(str, "Error!", (MB_OK | MB_ICONERROR));
		return false;
	}
	
	unsigned int identifier;
	fread(&identifier, 4, 1, file);

	if(identifier != MOVIE_TYPE_IDENTIFIER)
	{
		fclose(file);
		char str[1024];
		sprintf(str, "The movie file '%s' is not a valid Hourglass Resurrection movie.\nProbable causes are that the movie file has become corrupt\nor that it wasn't made with Hourglass Resurrection.", filename);
		CustomMessageBox(str, "Error!", MB_OK | MB_ICONERROR);
		return false;
	}

	unsigned int length;
	fread(&length, 4, 1, file);

	unsigned int rerecords;
	fread(&rerecords, 4, 1, file);

	unsigned int authorLen;
	fread(&authorLen, 4, 1, file);
	char* author = NULL;
	if(authorLen) // If this is non-zero, there is an author name in the movie.
	{
		if(authorLen > 63) // Sanity check, the author field cannot be more than 63 chars long.
		{
			fclose(file);
			char str[1024];
			sprintf(str, "The movie file '%s' cannot be loaded because the author name is too long.\nProbable causes are that the movie file has become corrupt\nor that it wasn't made with Hourglass.", filename);
			CustomMessageBox(str, "Error!", MB_OK | MB_ICONERROR);
			return false;
		}
		author = new char[authorLen+1]; // Need room for the null-termination.
		fread(author, authorLen, 1, file);
		author[authorLen] = '\0'; // Make sure the string is null-terminated.
	}
	
	char keyboardLayoutName[KL_NAMELENGTH];// = movie.keyboardLayoutName;
	fread(keyboardLayoutName, 8, 1, file); // KL_NAMELENGTH == 9 and includes the NULL

	unsigned int fps = 0;
	fread(&fps, 4, 1, file);

	unsigned int it = 0;
	fread(&it, 4, 1, file);

	unsigned int fmd5[4] = { 0 };
	fread(fmd5, 4, 4, file);

	unsigned int fsize = 0;
	fread(&fsize, 4, 1, file);

	int desyncDetectionTimerValues[16];
	fread(&desyncDetectionTimerValues, 16, 4, file);

	unsigned int version;
	fread(&version, 4, 1, file);
	/*if(version == 0)
		if(movie.desyncDetectionTimerValues[0])
			version = 51; // or 49
		else
			version = 39; // or older*/

	unsigned int commandlineLen;
	fread(&commandlineLen, 4, 1, file);
	char* commandline = NULL;//= movie.commandline;
	if(commandlineLen) // There's a command line in the movie file.
	{
		if(commandlineLen > (8192-1)-(MAX_PATH+1)) // We have exceeded the maximum allowed command line. Something's wrong.
		{
			fclose(file);
			if (author)
			{
				delete[] author; // Prevent memory leak
			}
			char str[1024];
			sprintf(str, "The movie file '%s' cannot be loaded because the command line is too long.\nProbable causes are that the movie file has become corrupt\nor that it wasn't made with Hourglass.", filename);
			CustomMessageBox(str, "Error!", MB_OK | MB_ICONERROR);
			return false;
		}
		commandline = new char[commandlineLen+1];
		fread(commandline, commandlineLen, 1, file);
		commandline[commandlineLen] = '\0'; // Ensure proper null-termination.
	}
	//fread(cmdline, 1, 160, file);
	//cmdline[strlen(cmdline)] = '\0'; // properly null-terminate the string.
	//cmdline[min(ARRAYSIZE(cmdline),ARRAYSIZE(commandline))-1] = 0;


	//bool failed = false;
	if(length > 0)// && magic == MAGIC)
	{
		// skip remaining padding before movie data
		// Why is there an assert here?
		// TODO: clean up this assert
		// assert(ftell(file) <= 1024-4);
	//	fseek(file, 1024, SEEK_SET);
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// The above line is no longer accurate since the header now is of variable size.
		// Perhaps we should add a fixed number of null-bytes?

		movie.currentFrame = 0;
		movie.rerecordCount = /*(localTASflags.playback || forPreview) ? */rerecords;// : 0;
		movie.frames.resize(length);
		movie.fps = fps;
		movie.it = it;
		
		memcpy(movie.fmd5, fmd5, 4*4);

		movie.fsize = fsize;

		// We've gotten far enough, it's safe to put these values in the movie struct
		if(author)
		{
			strcpy(movie.author, author);
		}
		memcpy(movie.desyncDetectionTimerValues, desyncDetectionTimerValues, (sizeof(int)*16));

		if(commandline)
		{
			strcpy(movie.commandline, commandline);
		}

		// We now import the inputs into the movie.frames structure.
		movie.frames.clear();

		// First, we determine how long is the rest of the file (might be improved).
		long current_file_pos = ftell(file); // Current position in file.
		fseek(file, 0L, SEEK_END); // Seek to the end of file.
		long end_file_pos = ftell(file); // Last position in file.
		fseek(file, current_file_pos, SEEK_SET); // Go back to the previous position.
		long file_size = end_file_pos - current_file_pos; // Length of the remaining part of the file.

		// Then, we import the whole rest of the file into a buffer.
		unsigned char* all_inputs = (unsigned char*)malloc(file_size);
		fread(all_inputs, 1, file_size, file);
		long current_pos = 0;
		while(current_pos < file_size){
			MovieFrame mf;
			int size = mf.inputs.unserialize(all_inputs+current_pos);
			movie.frames.push_back(mf);
			current_pos += size;
		}
		free(all_inputs);
	}
	else // empty movie file... do we really need this anymore? I mean, it would fail earlier if there was a problem.
	{
		movie.currentFrame = 0;
		movie.rerecordCount = 0;
		movie.frames.clear();
		movie.version = VERSION;
		//failed = true;
	}

	//if(!forPreview)
	//	unsaved = false; // either we just opened the movie and have no changes to save yet, or we just failed and have nothing to save at all
	fclose(file);

	/*if(failed)
	{
		if(localTASflags.playback && !forPreview)
			CustomMessageBox("The movie file is invalid or empty.", "Unable to play movie", MB_OK | MB_ICONERROR);
		return 0;
	}*/

	/*if(!localTASflags.playback && !forPreview && (length + rerecords*4 > 1800))
	{
		int result = CustomMessageBox("Really record over this movie?", "Record Movie", MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON1);
		if(result == IDCANCEL)
			return -1;
	}*/

	/*if(fpsp1 > 0 && (localTASflags.playback || forPreview))
	{
		DWORD movieFramerate = fpsp1 - 1;
		if(localTASflags.playback && !forPreview && movieFramerate != localTASflags.framerate)
		{
			int result = CustomMessageBox("Really start playing this movie with a different framerate than it was recorded with?", "Play Movie", MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON2);
			if(result == IDCANCEL)
				return -1;
			if(result == IDYES)
				movieFramerate = localTASflags.framerate;
		}
		localTASflags.framerate = movieFramerate;
		char str [256];
		sprintf(str, "%d", localTASflags.framerate);
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_FPS), str);
	}*/

	/*if(localTASflags.playback || forPreview)
	{
		DWORD movieInitialTime = itp1 ? (itp1 - 1) : 6000;
		if(localTASflags.playback && !forPreview && movieInitialTime != localTASflags.initialTime)
		{
			int result = CustomMessageBox("Really start playing this movie with a different initial system time than it was recorded with?", "Play Movie", MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON2);
			if(result == IDCANCEL)
				return -1;
			if(result == IDYES)
				movieInitialTime = localTASflags.initialTime;
		}
		localTASflags.initialTime = movieInitialTime;
		char str [256];
		sprintf(str, "%u", localTASflags.initialTime);
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYSTEMCLOCK), str);
	}*/

	/*if(localTASflags.playback && !forPreview && crc && crcVerifyEnabled)
	{
		int curcrc = CalcExeCrc();
		if(crc != curcrc)
		{
			const char* curexefname = GetExeFilenameWithoutPath();
			int curfsize = CalcExeFilesize();

			char str [512];
			sprintf(str,
				"EXE mismatch!\n"
				"\n"
				"Movie's: 0x%08X, %d bytes, %s\n"
				"Yours:    0x%08X, %d bytes, %s\n"
				"\n"
				"Play movie anyway?",
				crc, fsize, exefname,
				curcrc, curfsize, curexefname
			);

			int result = CustomMessageBox(str, "Play Movie", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);
			if(result == IDNO)
				return -1;
		}
	}*/

	// TODO: Find out what forPreview is!
//	if(localTASflags.playback && !forPreview && version != VERSION)
//	{
//		char movver [64];
//		sprintf(movver, "v.%d", version);
		/*if(version == 51)
			strcpy(mvvs, "r49 or r51");
		else if(version == 39)
			strcpy(mvvs, "r39 or older");
		else if((int)version == -1)
			strcpy(mvvs, "unknown version (r57 or later)");
		else
			sprintf(mvvs, "r%d", version);*/

		//int myVersion = VERSION;
//		char prgver [64];
//		sprintf(prgver, "v.%d", (unsigned int)VERSION);
		/*if((int)VERSION == -1)
			strcpy(myvs, "unknown version (r57 or later)");
		else
		{
			sprintf(myvs, "r%d", myVersion);
		}*/

		//bool probableDesyncs = false;
		//if(version != VERSION /*|| version < myVersion-40*/) // not very scientific here. change as needed.
		//	probableDesyncs = true;
		// in the future: if it's known whether certain versions sync with the current version, set probablyDesync depending on the version numbers.
		// and maybe add more specific warning messages, if warranted

		//bool assumeOK = false;
//#if SRCVERSION >= 71 && SRCVERSION <= 78 // a range of definitely sync-compatible versions
//		if(version >= 71 && version < SRCVERSION)
//			assumeOK = true;
//#endif

//		char str [1024];
//		sprintf(str,
//			"This movie was recorded using a different version of Hourglass.\n"
//			"\n"
//			"Movie's version: %s\n"
//			"Program version: %s\n"
//			"\n"
//			"%s\n"
//			"%s\n"
//			"\n"
//			"%s\n"
//			"%s\n",
//			movver, prgver,
//			// Merge the strings below?
//			/*probableDesyncs?*/"This may lead to the movie desyncing.",//:"This could cause desyncs if there were sync changes in-between those versions.",
//			/*(version != VERSION)?*/"If it would desync you might want to use the movies version of Hourglass.",//:"",
//			/*(version != VERSION)?*/"Do you want to try playing back the movie anyway?",//:"",
//			/*(version != VERSION)?*/"(Click \"Yes\" to continue, or click \"No\" to stop loading the movie)"//:""
//		);
		/*int result;
		if(assumeOK)
			result = IDYES;
		else*/
//		int result = CustomMessageBox(str, "Warning!", (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1)); // MS_DEFBUTTON1 marks "Yes" as the default choice.
		
		/*if(result == IDYES)
			movie.version = VERSION;
		else if(result == IDNO)
			movie.version = version;
		else //IDCANCEL
			return -1;*/
//		if(result == IDNO) return -1;

//		movie.version = VERSION;
		
		/*else
		{
			if(result == IDYES)
				movie.version = version;
			else //IDNO
				return -1;
		}*/
//	}
//	else
//	{
//		movie.version = version;
//	}

	/*if(localTASflags.playback || forPreview)
	{		
		bool useMovieCommandline = true;
		if(localTASflags.playback && !forPreview && strcmp(commandline, cmdline))
		{
			int result = CustomMessageBox("The command line in the movie does not match the current command line.\nDo you want to use the movies command line?\n\n(Click \"Yes\" to use movie's command line, \"No\" to use current command line or \"Cancel\" to stop loading the movie)", "Warning!", MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON1);
			if(result == IDCANCEL)
				return -1;
			if(result == IDNO)
				useMovieCommandline = false;
		}
		if(useMovieCommandline)
			strcpy(commandline, cmdline);

		SetWindowText(GetDlgItem(hWnd, IDC_EDIT_COMMANDLINE), commandline);
	}*/
	
	if (commandline)
	{
		delete[] commandline;
	}
	if (author)
	{
		delete[] author;
	}

	return true;
}
