#pragma once

enum LockTypes
{
    MOVIE = 0,
    SSTATE,
};

bool LockDirectory(LPCWSTR directory, LockTypes type);

void UnlockAllDirectories();
void UnlockDirectory(LockTypes type);
