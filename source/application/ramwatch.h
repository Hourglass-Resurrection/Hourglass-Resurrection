/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "ramsearch.h"

bool ResetWatches();
void OpenRWRecentFile(int memwRFileNumber);
extern bool AutoRWLoad;
extern bool RWSaveWindowPos;
#define MAX_RECENT_WATCHES 5
extern WCHAR rw_recent_files[MAX_RECENT_WATCHES][1024];
extern bool AskSave();
extern int ramw_x;
extern int ramw_y;
extern bool RWfileChanged;

// AddressWatcher is self-contained now
struct AddressWatcher
{
    unsigned int Address; // hardware address
    char Size;
    char Type;
    LPWSTR comment; // NULL means no comment, non-NULL means allocated comment
    bool WrongEndian;
    RSVal CurValue;
};
#define MAX_WATCH_COUNT 256
extern AddressWatcher rswatches[MAX_WATCH_COUNT];
extern int WatchCount; // number of valid items in rswatches

extern char Watch_Dir[1024];

bool InsertWatch(const AddressWatcher& Watch, LPCWSTR Comment);
void RemoveWatch(const AddressWatcher& Watch, int ignoreIndex = -1);
bool InsertWatch(const AddressWatcher& Watch, HWND parent = NULL); // asks user for comment
void Update_RAM_Watch();
bool Load_Watches(bool clear, const char* filename);
