/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(RAMWATCH_C_INCL) && !defined(UNITY_BUILD)
#define RAMWATCH_C_INCL

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0500
// Windows Header Files:
#include <windows.h>
#include "resource.h"
#include "ramsearch.h"
#include "ramwatch.h"
#include <assert.h>
#include <windows.h>
#include <string>

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

static HMENU ramwatchmenu;
static HMENU rwrecentmenu;
static HACCEL RamWatchAccels = NULL;
char rw_recent_files[MAX_RECENT_WATCHES][1024];
//char Watch_Dir[1024]="";
const unsigned int RW_MENU_FIRST_RECENT_FILE = 600;
bool RWfileChanged = false; //Keeps track of whether the current watch file has been changed, if so, ramwatch will prompt to save changes
bool AutoRWLoad = false;    //Keeps track of whether Auto-load is checked
bool RWSaveWindowPos = false; //Keeps track of whether Save Window position is checked
char currentWatch[1024];
int ramw_x, ramw_y;			//Used to store ramwatch dialog window positions
AddressWatcher rswatches[MAX_WATCH_COUNT];
int WatchCount=0;

extern HWND RamWatchHWnd;
extern HWND hWnd;
extern HINSTANCE hInst;
extern CRITICAL_SECTION g_processMemCS;
extern char exefilename [MAX_PATH+1];
extern char thisprocessPath [MAX_PATH+1];
extern bool started;
static char Str_Tmp_RW [1024];

void init_list_box(HWND Box, const char* Strs[], int numColumns, int *columnWidths); //initializes the ram search and/or ram watch listbox

#define MESSAGEBOXPARENT (RamWatchHWnd ? RamWatchHWnd : hWnd)

bool QuickSaveWatches();
bool ResetWatches();

RSVal GetCurrentValue(AddressWatcher& watch)
{
	return ReadValueAtHardwareAddress(watch.Address, watch.Size, watch.Type);
}

bool IsSameWatch(const AddressWatcher& l, const AddressWatcher& r)
{
	return ((l.Address == r.Address) && (l.Size == r.Size) && (l.Type == r.Type)/* && (l.WrongEndian == r.WrongEndian)*/);
}

bool VerifyWatchNotAlreadyAdded(const AddressWatcher& watch, int skipIndex=-1)
{
	for (int j = 0; j < WatchCount; j++)
	{
		if(j == skipIndex)
			continue;
		if(IsSameWatch(rswatches[j], watch))
		{
			if(RamWatchHWnd)
				SetForegroundWindow(RamWatchHWnd);
			return false;
		}
	}
	return true;
}


bool InsertWatch(const AddressWatcher& Watch, char *Comment)
{
	if(!VerifyWatchNotAlreadyAdded(Watch))
		return false;

	if(WatchCount >= MAX_WATCH_COUNT)
		return false;

	int i = WatchCount++;
	AddressWatcher& NewWatch = rswatches[i];
	NewWatch = Watch;
	//if(NewWatch.comment) free(NewWatch.comment);
	NewWatch.comment = (char *) malloc(strlen(Comment)+2);
	EnterCriticalSection(&g_processMemCS);
	NewWatch.CurValue = GetCurrentValue(NewWatch);
	LeaveCriticalSection(&g_processMemCS);
	strcpy(NewWatch.comment, Comment);
	ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged=true;

	return true;
}



LRESULT CALLBACK PromptWatchNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets the description of a watched address
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			//Clear_Sound_Buffer();

			GetWindowRect(hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			strcpy(Str_Tmp_RW,"Enter a name for this RAM address.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp_RW);
			strcpy(Str_Tmp_RW,"");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT2,WM_SETTEXT,0,(LPARAM)Str_Tmp_RW);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp_RW,80);
					InsertWatch(rswatches[WatchCount],Str_Tmp_RW);
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case ID_CANCEL:
				case IDCANCEL:
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}

bool InsertWatch(const AddressWatcher& Watch, HWND parent)
{
	if(!VerifyWatchNotAlreadyAdded(Watch))
		return false;

	if(!parent)
		parent = RamWatchHWnd;
	if(!parent)
		parent = hWnd;

	int prevWatchCount = WatchCount;

	rswatches[WatchCount] = Watch;
	EnterCriticalSection(&g_processMemCS);
	rswatches[WatchCount].CurValue = GetCurrentValue(rswatches[WatchCount]);
	LeaveCriticalSection(&g_processMemCS);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_PROMPT), parent, (DLGPROC) PromptWatchNameProc);

	return WatchCount > prevWatchCount;
}

void Update_RAM_Watch()
{
	BOOL watchChanged[MAX_WATCH_COUNT] = {0};

	if(WatchCount)
	{
		// update cached values and detect changes to displayed listview items

		EnterCriticalSection(&g_processMemCS);
		for(int i = 0; i < WatchCount; i++)
		{
			RSVal prevCurValue = rswatches[i].CurValue;
			RSVal newCurValue = GetCurrentValue(rswatches[i]);
			if(!prevCurValue.CheckBinaryEquality(newCurValue))
			{
				rswatches[i].CurValue = newCurValue;
				watchChanged[i] = TRUE;
			}
		}
		LeaveCriticalSection(&g_processMemCS);
	}

	// refresh any visible parts of the listview box that changed
	HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
	int top = ListView_GetTopIndex(lv);
	int bottom = top + ListView_GetCountPerPage(lv) + 1; // +1 is so we will update a partially-displayed last item
	if(top < 0) top = 0;
	if(bottom > WatchCount) bottom = WatchCount;
	int start = -1;
	for(int i = top; i <= bottom; i++)
	{
		if(start == -1)
		{
			if(i != bottom && watchChanged[i])
			{
				start = i;
				//somethingChanged = true;
			}
		}
		else
		{
			if(i == bottom || !watchChanged[i])
			{
				ListView_RedrawItems(lv, start, i-1);
				start = -1;
			}
		}
	}
}

bool AskSave()
{
	//This function asks to save changes if the watch file contents have changed
	//returns false only if a save was attempted but failed or was cancelled
	if(RWfileChanged && WatchCount > 0)
	{
		int answer = MessageBox(MESSAGEBOXPARENT, "Save Changes?", "Ram Watch", MB_YESNOCANCEL);
		if(answer == IDYES)
			if(!QuickSaveWatches())
				return false;
		return (answer != IDCANCEL);
	}
	return true;
}


void UpdateRW_RMenu(HMENU menu, unsigned int mitem, unsigned int baseid)
{
	MENUITEMINFO moo;
	int x;

	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;

	GetMenuItemInfo(GetSubMenu(ramwatchmenu, 0), mitem, FALSE, &moo);
	moo.hSubMenu = menu;
	moo.fState = strlen(rw_recent_files[0]) ? MFS_ENABLED : MFS_GRAYED;

	SetMenuItemInfo(GetSubMenu(ramwatchmenu, 0), mitem, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < MAX_RECENT_WATCHES; x++)
	{
		RemoveMenu(menu, baseid + x, MF_BYCOMMAND);
	}

	// Recreate the menus
	for(x = MAX_RECENT_WATCHES - 1; x >= 0; x--)
	{  
		char tmp[128 + 5];

		// Skip empty strings
		if(!strlen(rw_recent_files[x]))
		{
			continue;
		}

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;

		// Fill in the menu text.
		if(strlen(rw_recent_files[x]) < 128)
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, rw_recent_files[x]);
		}
		else
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, rw_recent_files[x] + strlen( rw_recent_files[x] ) - 127);
		}

		// Insert the menu item
		moo.cch = strlen(tmp);
		moo.fType = 0;
		moo.wID = baseid + x;
		moo.dwTypeData = tmp;
		InsertMenuItem(menu, 0, 1, &moo);
	}
}

void UpdateRWRecentArray(const char* addString, unsigned int arrayLen, HMENU menu, unsigned int menuItem, unsigned int baseId)
{
	// Try to find out if the filename is already in the recent files list.
	for(unsigned int x = 0; x < arrayLen; x++)
	{
		if(strlen(rw_recent_files[x]))
		{
			if(!strcmp(rw_recent_files[x], addString))    // Item is already in list.
			{
				// If the filename is in the file list don't add it again.
				// Move it up in the list instead.

				int y;
				char tmp[1024];

				// Save pointer.
				strcpy(tmp,rw_recent_files[x]);
				
				for(y = x; y; y--)
				{
					// Move items down.
					strcpy(rw_recent_files[y],rw_recent_files[y - 1]);
				}

				// Put item on top.
				strcpy(rw_recent_files[0],tmp);

				// Update the recent files menu
				UpdateRW_RMenu(menu, menuItem, baseId);

				return;
			}
		}
	}

	// The filename wasn't found in the list. That means we need to add it.

	// Move the other items down.
	for(unsigned int x = arrayLen - 1; x; x--)
	{
		strcpy(rw_recent_files[x],rw_recent_files[x - 1]);
	}

	// Add the new item.
	strcpy(rw_recent_files[0], addString);

	// Update the recent files menu
	UpdateRW_RMenu(menu, menuItem, baseId);
}


void RWAddRecentFile(const char *filename)
{
	UpdateRWRecentArray(filename, MAX_RECENT_WATCHES, rwrecentmenu, RAMMENU_FILE_RECENT, RW_MENU_FIRST_RECENT_FILE);
}

void OpenRWRecentFile(int memwRFileNumber)
{
	if(!ResetWatches())
		return;

	int rnum = memwRFileNumber;
	if((unsigned int)rnum >= MAX_RECENT_WATCHES)
		return; //just in case

	char* x;

	while(true)
	{
		x = rw_recent_files[rnum];
		if(!*x) 
			return;		//If no recent files exist just return.  Useful for Load last file on startup (or if something goes screwy)

		if(rnum) //Change order of recent files if not most recent
		{
			RWAddRecentFile(x);
			rnum = 0;
		}
		else
		{
			break;
		}
	}

	strcpy(currentWatch,x);
	strcpy(Str_Tmp_RW,currentWatch);

	//loadwatches here
	FILE *WatchFile = fopen(Str_Tmp_RW,"rb");
	if(!WatchFile)
	{
		int answer = MessageBox(MESSAGEBOXPARENT,"Error opening file.","ERROR",MB_OKCANCEL);
		if(answer == IDOK)
		{
			rw_recent_files[rnum][0] = '\0';	//Clear file from list 
			if(rnum)							//Update the ramwatch list
				RWAddRecentFile(rw_recent_files[0]); 
			else
				RWAddRecentFile(rw_recent_files[1]);
		}
		return;
	}
	const char DELIM = '\t';
	AddressWatcher Temp;
	char mode;
	fgets(Str_Tmp_RW,1024,WatchFile);
	sscanf(Str_Tmp_RW,"%c%*s",&mode);
	//if((mode == '1' && !(SegaCD_Started)) || (mode == '2' && !(_32X_Started)))
	//{
	//	char Device[8];
	//	strcpy(Device,(mode > '1')?"32X":"SegaCD");
	//	sprintf(Str_Tmp_RW,"Warning: %s not started. \nWatches for %s addresses will be ignored.",Device,Device);
	//	MessageBox(MESSAGEBOXPARENT,Str_Tmp_RW,"Possible Device Mismatch",MB_OK);
	//}
	int WatchAdd;
	fgets(Str_Tmp_RW,1024,WatchFile);
	sscanf(Str_Tmp_RW,"%d%*s",&WatchAdd);
	WatchAdd+=WatchCount;
	for (int i = WatchCount; i < WatchAdd; i++)
	{
		while(i < 0)
			i++;
		do {
			fgets(Str_Tmp_RW,1024,WatchFile);
		} while(Str_Tmp_RW[0] == '\n');
		sscanf(Str_Tmp_RW,"%*05X%*c%08X%*c%c%*c%c%*c%d",&(Temp.Address),&(Temp.Size),&(Temp.Type),&(Temp.WrongEndian));
		Temp.WrongEndian = 0;
		char* Comment = strrchr(Str_Tmp_RW,DELIM) + 1;
		if(Comment == (char*)NULL + 1)
			continue;
		char* newline = strrchr(Comment,'\n');
		if(newline)
			*newline = '\0';
		InsertWatch(Temp,Comment);
	}

	fclose(WatchFile);
	if(RamWatchHWnd)
		ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged=false;
	return;
}

int Change_File_L(char *Dest, char *Dir, char *Titre, char *Filter, char *Ext, HWND hwnd)
{
	OPENFILENAME ofn;

	SetCurrentDirectory(thisprocessPath);

	if(!strcmp(Dest, ""))
	{
		strcpy(Dest, "default.");
		strcat(Dest, Ext);
	}

	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Dest;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dir;
	ofn.lpstrTitle = Titre;
	ofn.lpstrDefExt = Ext;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn)) return 1;

	return 0;
}

int Change_File_S(char *Dest, char *Dir, char *Titre, char *Filter, char *Ext, HWND hwnd)
{
	OPENFILENAME ofn;

	SetCurrentDirectory(thisprocessPath);

	if(!strcmp(Dest, ""))
	{
		strcpy(Dest, "default.");
		strcat(Dest, Ext);
	}

	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Dest;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dir;
	ofn.lpstrTitle = Titre;
	ofn.lpstrDefExt = Ext;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if(GetSaveFileName(&ofn)) return 1;

	return 0;
}

bool Save_Watches()
{
	char* slash = max(strrchr(exefilename, '\\'), strrchr(exefilename, '/'));
	strcpy(Str_Tmp_RW,slash ? slash+1 : exefilename);
	char* dot = strrchr(Str_Tmp_RW, '.');
	if(dot) *dot = 0;
	strcat(Str_Tmp_RW,".wch");
	if(Change_File_S(Str_Tmp_RW, thisprocessPath, "Save Watches", "Watchlist\0*.wch\0All Files\0*.*\0\0", "wch", RamWatchHWnd))
	{
		FILE *WatchFile = fopen(Str_Tmp_RW,"r+b");
		if(!WatchFile) WatchFile = fopen(Str_Tmp_RW,"w+b");
		//fputc(SegaCD_Started?'1':(_32X_Started?'2':'0'),WatchFile);
		fputc('0',WatchFile);
		fputc('\n',WatchFile);
		strcpy(currentWatch,Str_Tmp_RW);
		RWAddRecentFile(currentWatch);
		sprintf(Str_Tmp_RW,"%d\n",WatchCount);
		fputs(Str_Tmp_RW,WatchFile);
		const char DELIM = '\t';
		for (int i = 0; i < WatchCount; i++)
		{
			sprintf(Str_Tmp_RW,"%05X%c%08X%c%c%c%c%c%d%c%s\n",i,DELIM,rswatches[i].Address,DELIM,rswatches[i].Size,DELIM,rswatches[i].Type,DELIM,rswatches[i].WrongEndian,DELIM,rswatches[i].comment);
			fputs(Str_Tmp_RW,WatchFile);
		}
		
		fclose(WatchFile);
		RWfileChanged=false;
		//TODO: Add to recent list function call here
		return true;
	}
	return false;
}

bool QuickSaveWatches()
{
if(RWfileChanged==false) return true; //If file has not changed, no need to save changes
if(currentWatch[0] == NULL) //If there is no currently loaded file, run to Save as and then return
	{
		return Save_Watches();
	}
		
		strcpy(Str_Tmp_RW,currentWatch);
		FILE *WatchFile = fopen(Str_Tmp_RW,"r+b");
		if(!WatchFile) WatchFile = fopen(Str_Tmp_RW,"w+b");
		//fputc(SegaCD_Started?'1':(_32X_Started?'2':'0'),WatchFile);
		fputc('0',WatchFile);
		fputc('\n',WatchFile);
		sprintf(Str_Tmp_RW,"%d\n",WatchCount);
		fputs(Str_Tmp_RW,WatchFile);
		const char DELIM = '\t';
		for (int i = 0; i < WatchCount; i++)
		{
			sprintf(Str_Tmp_RW,"%05X%c%08X%c%c%c%c%c%d%c%s\n",i,DELIM,rswatches[i].Address,DELIM,rswatches[i].Size,DELIM,rswatches[i].Type,DELIM,rswatches[i].WrongEndian,DELIM,rswatches[i].comment);
			fputs(Str_Tmp_RW,WatchFile);
		}
		fclose(WatchFile);
		RWfileChanged=false;
		return true;
}

bool Load_Watches(bool clear, const char* filename)
{
	const char DELIM = '\t';
	FILE* WatchFile = fopen(filename,"rb");
	if(!WatchFile)
	{
		MessageBox(MESSAGEBOXPARENT,"Error opening file.","ERROR",MB_OK);
		return false;
	}
	if(clear)
	{
		if(!ResetWatches())
		{
			fclose(WatchFile);
			return false;
		}
	}
	strcpy(currentWatch,filename);
	RWAddRecentFile(currentWatch);
	AddressWatcher Temp;
	char mode;
	fgets(Str_Tmp_RW,1024,WatchFile);
	sscanf(Str_Tmp_RW,"%c%*s",&mode);
	//if((mode == '1' && !(SegaCD_Started)) || (mode == '2' && !(_32X_Started)))
	//{
	//	char Device[8];
	//	strcpy(Device,(mode > '1')?"32X":"SegaCD");
	//	sprintf(Str_Tmp_RW,"Warning: %s not started. \nWatches for %s addresses will be ignored.",Device,Device);
	//	MessageBox(MESSAGEBOXPARENT,Str_Tmp_RW,"Possible Device Mismatch",MB_OK);
	//}
	int WatchAdd;
	fgets(Str_Tmp_RW,1024,WatchFile);
	sscanf(Str_Tmp_RW,"%d%*s",&WatchAdd);
	WatchAdd+=WatchCount;
	for (int i = WatchCount; i < WatchAdd; i++)
	{
		while(i < 0)
			i++;
		do {
			fgets(Str_Tmp_RW,1024,WatchFile);
		} while(Str_Tmp_RW[0] == '\n');
		sscanf(Str_Tmp_RW,"%*05X%*c%08X%*c%c%*c%c%*c%d",&(Temp.Address),&(Temp.Size),&(Temp.Type),&(Temp.WrongEndian));
		Temp.WrongEndian = 0;
		char* Comment = strrchr(Str_Tmp_RW,DELIM) + 1;
		if(Comment == (char*)NULL + 1)
			continue;
		char* newline = strrchr(Comment,'\n');
		if(newline)
			*newline = '\0';
		InsertWatch(Temp,Comment);
	}
	
	fclose(WatchFile);
	if(RamWatchHWnd)
		ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged=false;
	return true;
}

bool Load_Watches(bool clear)
{
	char* slash = max(strrchr(exefilename, '\\'), strrchr(exefilename, '/'));
	strcpy(Str_Tmp_RW,slash ? slash+1 : exefilename);
	char* dot = strrchr(Str_Tmp_RW, '.');
	if(dot) *dot = 0;
	strcat(Str_Tmp_RW,".wch");
	if(Change_File_L(Str_Tmp_RW, thisprocessPath, "Load Watches", "Watchlist\0*.wch\0All Files\0*.*\0\0", "wch", RamWatchHWnd))
	{
		return Load_Watches(clear, Str_Tmp_RW);
	}
	return false;
}

bool ResetWatches()
{
	if(!AskSave())
		return false;
	for (;WatchCount>=0;WatchCount--)
	{
		free(rswatches[WatchCount].comment);
		rswatches[WatchCount].comment = NULL;
	}
	WatchCount++;
	if(RamWatchHWnd)
		ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged = false;
	currentWatch[0] = NULL;
	return true;
}

void RemoveWatch(int watchIndex)
{
	if((unsigned int)watchIndex >= (unsigned int)WatchCount)
		return;
	free(rswatches[watchIndex].comment);
	rswatches[watchIndex].comment = NULL;
	for (int i = watchIndex; i <= WatchCount; i++)
		rswatches[i] = rswatches[i+1];
	WatchCount--;
}

void RemoveWatch(const AddressWatcher &watch, int ignoreIndex) {
	for (int i = 0; i < WatchCount; i++) {
		if (i == ignoreIndex)
			continue;
		if (IsSameWatch(rswatches[i],watch)) {
			RemoveWatch(i);
			break;
		}
	}
}


bool ReplaceWatch(const AddressWatcher& Watch, char* Comment, int index)
{
	if(index < WatchCount)
		RemoveWatch(index);
	if(InsertWatch(Watch,Comment))
	{
		int moveCount = (WatchCount-1)-index;
		if(moveCount > 0)
		{
			AddressWatcher Temp = rswatches[WatchCount-1];
			memmove(&(rswatches[index + 1]),&(rswatches[index]),sizeof(AddressWatcher)*moveCount);
			rswatches[index] = Temp;
		}
		return true;
	}
	return false;
}


LRESULT CALLBACK EditWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets info for a RAM Watch, and then inserts it into the Watch List
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int index;
	static char s,t = s = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			//Clear_Sound_Buffer();
			

			GetWindowRect(hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			index = (int)lParam;
			sprintf(Str_Tmp_RW,"%08X",rswatches[index].Address);
			SetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp_RW);
			if(rswatches[index].comment != NULL)
				SetDlgItemText(hDlg,IDC_PROMPT_EDIT,rswatches[index].comment);
			s = rswatches[index].Size;
			t = rswatches[index].Type;
			switch(s)
			{
				case 'b':
					SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'w':
					SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'l':
					SendDlgItemMessage(hDlg, IDC_8_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					s = 0;
					break;
			}
			switch(t)
			{
				case 's':
					SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'u':
					SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'h':
					SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'f':
					SendDlgItemMessage(hDlg, IDC_FLOAT, BM_SETCHECK, BST_CHECKED, 0);
					if(s == 'b' || s == 'w')
					{
						SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_8_BYTES, BM_SETCHECK, BST_UNCHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_UNCHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_UNCHECKED, 0);
						s = 'd';
					}
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),false);
					break;
				default:
					t = 0;
					break;
			}

			return true;
			break;
		
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SIGNED:
					t='s';
					return true;
				case IDC_UNSIGNED:
					t='u';
					return true;
				case IDC_HEX:
					t='h';
					return true;
				case IDC_FLOAT:
					t='f';
					return true;
				case IDC_1_BYTE:
					s = 'b';
					return true;
				case IDC_2_BYTES:
					s = 'w';
					return true;
				case IDC_4_BYTES:
					s = 'd';
					return true;
				case IDC_8_BYTES:
					s = 'l';
					return true;
				case IDOK:
				{
					if(s && t)
					{
						AddressWatcher Temp;
						Temp.Size = s;
						Temp.Type = t;
						Temp.WrongEndian = false; //replace this when I get little endian working properly
						GetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp_RW,1024);
						char *addrstr = Str_Tmp_RW;
						if(strlen(Str_Tmp_RW) > 8) addrstr = &(Str_Tmp_RW[strlen(Str_Tmp_RW) - 9]);
						for(int i = 0; addrstr[i]; i++) {if(toupper(addrstr[i]) == 'O') addrstr[i] = '0';}
						sscanf(addrstr,"%08X",&(Temp.Address));

						//if((Temp.Address & ~0xFFFFFF) == ~0xFFFFFF)
						//	Temp.Address &= 0xFFFFFF;

						bool canceled = false;
						if(!VerifyWatchNotAlreadyAdded(Temp, index))
						{
							int result = MessageBox(hDlg,"Watch already exists. Replace it?","REPLACE",MB_YESNO);
							if(result == IDYES)
								RemoveWatch(Temp, index);
							if(result == IDNO)
								canceled = true;
						}
						if(!canceled)
						{
							if(IsHardwareAddressValid(Temp.Address) || !started || (index < WatchCount && Temp.Address == rswatches[index].Address))
							{
								GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp_RW,80);
								ReplaceWatch(Temp,Str_Tmp_RW,index);
								if(RamWatchHWnd)
								{
									ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
								}
								EndDialog(hDlg, true);
							}
							else
							{
								MessageBox(hDlg,"Invalid Address","ERROR",MB_OK);
							}
						}
					}
					else
					{
						strcpy(Str_Tmp_RW,"Error:");
						if(!s)
							strcat(Str_Tmp_RW," Size must be specified.");
						if(!t)
							strcat(Str_Tmp_RW," Type must be specified.");
						MessageBox(hDlg,Str_Tmp_RW,"ERROR",MB_OK);
					}
					RWfileChanged=true;
					return true;
					break;
				}
				case ID_CANCEL:
				case IDCANCEL:
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}




LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_MOVE: {
			RECT wrect;
			GetWindowRect(hDlg,&wrect);
			ramw_x = wrect.left;
			ramw_y = wrect.top;
			break;
			};
			
		case WM_INITDIALOG: {

			GetWindowRect(hWnd, &r);  //Ramwatch window
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2); // TASer window
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			
			// push it away from the main window if we can
			const int width = (r.right-r.left);
			const int height = (r.bottom - r.top);
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}
			
			//-----------------------------------------------------------------------------------
			//If user has Save Window Pos selected, override default positioning
			if(RWSaveWindowPos)	
			{
				//If ramwindow is for some reason completely off screen, use default instead 
				if(ramw_x > (-width*2) || ramw_x < (width*2 + GetSystemMetrics(SM_CYSCREEN))   ) 
					r.left = ramw_x;	  //This also ignores cases of windows -32000 error codes
				//If ramwindow is for some reason completely off screen, use default instead 
				if(ramw_y > (0-height*2) ||ramw_y < (height*2 + GetSystemMetrics(SM_CYSCREEN))	)
					r.top = ramw_y;		  //This also ignores cases of windows -32000 error codes
			}
			//-------------------------------------------------------------------------------------
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			
			ramwatchmenu=GetMenu(hDlg);
			rwrecentmenu=CreateMenu();
			UpdateRW_RMenu(rwrecentmenu, RAMMENU_FILE_RECENT, RW_MENU_FIRST_RECENT_FILE);
			
			const char* names[3] = {"Address","Value","Notes"};
			int widths[3] = {62,64,64+51+53};
			init_list_box(GetDlgItem(hDlg,IDC_WATCHLIST),names,3,widths);
			if(!ResultCount)
				reset_address_info();
			else
				signal_new_frame();
			ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
			//if(littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);

			RamWatchAccels = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCELERATOR1));

			// due to some bug in windows, the arrow button width from the resource gets ignored, so we have to set it here
			SetWindowPos(GetDlgItem(hDlg,ID_WATCHES_UPDOWN), 0,0,0, 30,60, SWP_NOMOVE);

			Update_RAM_Watch();

			DragAcceptFiles(hDlg, TRUE);

			return true;
			break;
		}
		
		case WM_INITMENU:
			CheckMenuItem(ramwatchmenu, RAMMENU_FILE_AUTOLOAD, AutoRWLoad ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(ramwatchmenu, RAMMENU_FILE_SAVEWINDOW, RWSaveWindowPos ? MF_CHECKED : MF_UNCHECKED);
			break;

		case WM_MENUSELECT:
 		case WM_ENTERSIZEMOVE:
			//Clear_Sound_Buffer();
			break;

		case WM_NOTIFY:
		{
			LPNMHDR lP = (LPNMHDR) lParam;
			switch(lP->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_DISPINFO *Item = (LV_DISPINFO *)lParam;
					Item->item.mask = LVIF_TEXT;
					Item->item.state = 0;
					Item->item.iImage = 0;
					const unsigned int iNum = Item->item.iItem;
					static char num[64];
					switch(Item->item.iSubItem)
					{
						case 0:
							sprintf(num,"%08X",rswatches[iNum].Address);
							Item->item.pszText = num;
							return true;
						case 1: {
							RSVal rsval = rswatches[iNum].CurValue;
							int t = rswatches[iNum].Type;
							int size = rswatches[iNum].Size;
							rsval.print(num, size, t);
							Item->item.pszText = num;
						}	return true;
						case 2:
							Item->item.pszText = rswatches[iNum].comment ? rswatches[iNum].comment : "";
							return true;

						default:
							return false;
					}
				}
				case LVN_ODFINDITEM:
				{	
					// disable search by keyboard typing,
					// because it interferes with some of the accelerators
					// and it isn't very useful here anyway
					SetWindowLong(hDlg, DWL_MSGRESULT, ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST)));
					return 1;
				}
			}
		}
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case RAMMENU_FILE_SAVE:
					QuickSaveWatches();
					break;

				case RAMMENU_FILE_SAVEAS:	
				//case IDC_C_SAVE:
					return Save_Watches();
				case RAMMENU_FILE_OPEN:
					return Load_Watches(true);
				case RAMMENU_FILE_APPEND:
				//case IDC_C_LOAD:
					return Load_Watches(false);
				case RAMMENU_FILE_NEW:
				//case IDC_C_RESET:
					ResetWatches();
					return true;
				case IDC_C_WATCH_REMOVE:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					RemoveWatch(watchIndex);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);	
					RWfileChanged=true;
					SetFocus(GetDlgItem(hDlg,IDC_WATCHLIST));
					return true;
				case IDC_C_WATCH_EDIT:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) watchIndex);
					SetFocus(GetDlgItem(hDlg,IDC_WATCHLIST));
					return true;
				case IDC_C_WATCH:
					rswatches[WatchCount].Address = rswatches[WatchCount].WrongEndian = 0;
					rswatches[WatchCount].Size = 'b';
					rswatches[WatchCount].Type = 's';
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					SetFocus(GetDlgItem(hDlg,IDC_WATCHLIST));
					return true;
				case IDC_C_WATCH_DUPLICATE:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					rswatches[WatchCount].Address = rswatches[watchIndex].Address;
					rswatches[WatchCount].WrongEndian = rswatches[watchIndex].WrongEndian;
					rswatches[WatchCount].Size = rswatches[watchIndex].Size;
					rswatches[WatchCount].Type = rswatches[watchIndex].Type;
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					SetFocus(GetDlgItem(hDlg,IDC_WATCHLIST));
					return true;
				case IDC_C_WATCH_UP:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if(watchIndex == 0 || watchIndex == -1)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex - 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex - 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					RWfileChanged=true;
					return true;
				}
				case IDC_C_WATCH_DOWN:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if(watchIndex >= WatchCount - 1 || watchIndex == -1)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex + 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex + 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					RWfileChanged=true;
					return true;
				}
				case ID_WATCHES_UPDOWN:
				{
					int delta = ((LPNMUPDOWN)lParam)->iDelta;
					SendMessage(hDlg, WM_COMMAND, delta<0 ? IDC_C_WATCH_UP : IDC_C_WATCH_DOWN,0);
					break;
				}
				case RAMMENU_FILE_AUTOLOAD:
				{
					AutoRWLoad ^= 1;
					CheckMenuItem(ramwatchmenu, RAMMENU_FILE_AUTOLOAD, AutoRWLoad ? MF_CHECKED : MF_UNCHECKED);
					break;
				}
				case RAMMENU_FILE_SAVEWINDOW:
				{
					RWSaveWindowPos ^=1;
					CheckMenuItem(ramwatchmenu, RAMMENU_FILE_SAVEWINDOW, RWSaveWindowPos ? MF_CHECKED : MF_UNCHECKED);
					break;
				}
				case IDC_C_ADDCHEAT:
				{
//					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST)) | (1 << 24);
//					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITCHEAT), hDlg, (DLGPROC) EditCheatProc,(LPARAM) searchIndex);
				}
				case IDOK:
				case IDCANCEL:
					RamWatchHWnd = NULL;
					DragAcceptFiles(hDlg, FALSE);
					EndDialog(hDlg, true);
					return true;
				default:
					if(LOWORD(wParam) >= RW_MENU_FIRST_RECENT_FILE && LOWORD(wParam) < RW_MENU_FIRST_RECENT_FILE+MAX_RECENT_WATCHES)
					OpenRWRecentFile(LOWORD(wParam) - RW_MENU_FIRST_RECENT_FILE);
			}
			break;
		
		case WM_KEYDOWN: // handle accelerator keys
		{
			SetFocus(GetDlgItem(hDlg,IDC_WATCHLIST));
			MSG msg;
			msg.hwnd = hDlg;
			msg.message = uMsg;
			msg.wParam = wParam;
			msg.lParam = lParam;
			if(RamWatchAccels && TranslateAccelerator(hDlg, RamWatchAccels, &msg))
				return true;
		}	break;

		case WM_CLOSE:
			RamWatchHWnd = NULL;
			DragAcceptFiles(hDlg, FALSE);
			EndDialog(hDlg, true);
			return true;

		case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			DragQueryFile(hDrop, 0, Str_Tmp_RW, 1024);
			DragFinish(hDrop);
			return Load_Watches(true, Str_Tmp_RW);
		}	break;
	}

	return false;
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
