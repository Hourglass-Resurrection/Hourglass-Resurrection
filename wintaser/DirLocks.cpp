#include <windows.h>

#include "DirLocks.h"

#include <map>
#include "CustomDLGs.h"

static std::map<LockTypes, HANDLE> locks;

bool LockDirectory(char* directory, LockTypes type)
{
	char name[MAX_PATH+1];
	strcpy(name, directory);
	strcat(name, "/hourglass.lock\0");
	// FILE_FLAG_DELETE_ON_CLOSE is really handy here as it means we don't have to manage the deletion process for the lock-files ourselves.
	HANDLE rv = CreateFile(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, (FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_DELETE_ON_CLOSE), NULL);
	if(rv == INVALID_HANDLE_VALUE)
	{
		// Locking directory failed, issue an error-message and stop.
		char str[1024];
		sprintf(str, "Locking the directory '%s' failed\nPlease make sure that Hourglass has rights to create files in this directory or choose another directory", directory);
		CustomMessageBox(str, "Error!", (MB_OK | MB_ICONERROR));
		return false;
	}
	locks[type] = rv; // TODO: Check if element already exists? Should probably close the old element if it does?
	return true; // Locking directory succeeded
}

void UnlockAllDirectories()
{
	for(std::map<LockTypes,HANDLE>::iterator i = locks.begin(); i != locks.end(); ++i)
	{
		CloseHandle(i->second);
	}
}

void UnlockDirectory(LockTypes type)
{
	CloseHandle(locks[type]);	
}