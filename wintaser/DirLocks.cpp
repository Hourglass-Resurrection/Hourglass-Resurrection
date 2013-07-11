#include <windows.h>

#include "DirLocks.h"

#include <map>
#include "CustomDLGs.h"
#include "logging.h"

#ifdef _DEBUG
	#define _DIRLOCKDEBUG
#endif

#ifdef _DIRLOCKDEBUG || 0 //1
	#define dirlockdebug debugprintf
	#define DirLockPrintLastError PrintLastError
#else
	#define dirlockdebug(...) ((void)0)
	#define DirLockPrintLastError(...) ((void)0)
#endif

static std::map<LockTypes, HANDLE> locks;

static const char * LockTypesToString[] = { "Movie", "SaveState", };

bool LockDirectory(char* directory, LockTypes type)
{
	dirlockdebug("DirLocking: Locking directory '%s' for %ss.\n", directory, LockTypesToString[(unsigned int)type]);
	// A different directory is already locked, since we will no longer need to hang on to that directory, we'll just Unlock it.
	if(locks.find(type) != locks.end()) 
	{
		dirlockdebug("DirLocking: Found existing lock when locking directory for %s\n.", LockTypesToString[(unsigned int)type]);
		UnlockDirectory(type);
	}

	char name[MAX_PATH+1];
	strcpy(name, directory);
	strcat(name, "hourglass.lock\0");
	// FILE_FLAG_DELETE_ON_CLOSE is really handy here as it means we don't have to manage the deletion process for the lock-files ourselves.
	HANDLE rv = CreateFile(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, (FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_DELETE_ON_CLOSE), NULL);
	if(rv == INVALID_HANDLE_VALUE)
	{
		DirLockPrintLastError("DirLocking: CreateFile", GetLastError());
		// Locking directory failed, issue an error-message and stop.
		char str[1024];
		sprintf(str, "Locking the directory '%s' for %ss failed\nPlease make sure that Hourglass has rights to create files in this directory or choose another directory", directory, LockTypesToString[(unsigned int)type]);
		CustomMessageBox(str, "Error!", (MB_OK | MB_ICONERROR));
		return false;
	}
	locks[type] = rv;
	return true; // Locking directory succeeded
}

void UnlockAllDirectories()
{
	dirlockdebug("DirLocking: Releasing all locked directories.\n");
	for(std::map<LockTypes,HANDLE>::iterator i = locks.begin(); i != locks.end(); ++i)
	{
		CloseHandle(i->second);
	}
}

void UnlockDirectory(LockTypes type)
{
	dirlockdebug("DirLocking: Unlocking directory for %ss.\n", LockTypesToString[(unsigned int)type]);
	std::map<LockTypes,HANDLE>::iterator it = locks.find(type);
	if(it == locks.end())
	{
		dirlockdebug("DirLocking: ERROR: Couldn't find a locked directory for %ss.\n", LockTypesToString[(unsigned int)type]);
		return; // Directory is not locked, this should never happen...
	}
	CloseHandle(it->second);
	locks.erase(it); // Remove the entry after releasing the lock, so we don't give false positives about locked directories.
}