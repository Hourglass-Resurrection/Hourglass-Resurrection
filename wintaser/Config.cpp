#include "Config.h"
#include <stdio.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

#include "ramwatch.h"
#include "logging.h"

char Str_Tmp [1024] = {0};

const char* Config::defaultConfigFilename = "hourglass.cfg";

int Config::Save_Config(const char* filename)
{
	if(!filename)
		filename = defaultConfigFilename;
	char Conf_File[1024];

	if(*filename && filename[1] == ':')
		strcpy(Conf_File, filename);
	else
		sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

	WritePrivateProfileStringA("General", "Exe path", exefilename, Conf_File);
	WritePrivateProfileStringA("General", "Movie path", moviefilename, Conf_File);
	WritePrivateProfileStringA("General", "Command line", commandline, Conf_File);

	SetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
	SetPrivateProfileIntA("Graphics", "Force Windowed", forceWindowed, Conf_File);
	SetPrivateProfileIntA("Tools", "Fast Forward Flags", fastForwardFlags, Conf_File);
	if(advancePastNonVideoFramesConfigured)
		SetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
	SetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
	SetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

	SetPrivateProfileIntA("Debug", "Debug Logging Mode", debugPrintMode, Conf_File);
	SetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
	SetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

	wsprintf(Str_Tmp, "%d", AutoRWLoad);
	WritePrivateProfileString("Watches", "AutoLoadWatches", Str_Tmp, Conf_File);
	
	wsprintf(Str_Tmp, "%d", RWSaveWindowPos);
	WritePrivateProfileString("Watches", "SaveWindowPosition", Str_Tmp, Conf_File);
	
	if (RWSaveWindowPos)
	{
	SetPrivateProfileIntA("Watches", "Ramwatch_X", ramw_x, Conf_File);
	SetPrivateProfileIntA("Watches", "Ramwatch_Y", ramw_y, Conf_File);
	}

	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		WritePrivateProfileStringA("Watches", str, &rw_recent_files[i][0], Conf_File);	
	}

	// TODO: save hotkeys!
	//SaveHotkeys(Conf_File, true);
	//SaveHotkeys(Conf_File, false);

	return 1;
}


int Config::Save_As_Config(HWND hWnd, HINSTANCE hInst)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectoryA(thisprocessPath);

	strcpy(&Name[0], defaultConfigFilename);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = thisprocessPath;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if(GetSaveFileName(&ofn))
	{
		Save_Config(Name);
		debugprintf("config saved in \"%s\".\n", Name);
		return 1;
	}
	else return 0;
}


int Config::Load_Config(const char* filename)
{
	if(!filename)
		filename = defaultConfigFilename;
	char Conf_File[1024];

	if(*filename && filename[1] == ':')
		strcpy(Conf_File, filename);
	else
		sprintf(Conf_File, "%s\\%s", thisprocessPath, filename);

	GetPrivateProfileStringA("General", "Exe path", exefilename, exefilename, MAX_PATH, Conf_File);
	GetPrivateProfileStringA("General", "Movie path", moviefilename, moviefilename, MAX_PATH, Conf_File);
	GetPrivateProfileStringA("General", "Command line", commandline, commandline, ARRAYSIZE(commandline), Conf_File);

	nextLoadRecords = 0!=GetPrivateProfileIntA("General", "Movie Read Only", nextLoadRecords, Conf_File);
	forceWindowed = GetPrivateProfileIntA("Graphics", "Force Windowed", forceWindowed, Conf_File);
	fastForwardFlags = GetPrivateProfileIntA("Tools", "Fast Forward Flags", fastForwardFlags, Conf_File);
	advancePastNonVideoFrames = GetPrivateProfileIntA("Input", "Skip Lag Frames", advancePastNonVideoFrames, Conf_File);
	advancePastNonVideoFramesConfigured = 0!=GetPrivateProfileIntA("Input", "Skip Lag Frames", 0, Conf_File);
	inputFocusFlags = GetPrivateProfileIntA("Input", "Background Input Focus Flags", inputFocusFlags, Conf_File);
	hotkeysFocusFlags = GetPrivateProfileIntA("Input", "Background Hotkeys Focus Flags", hotkeysFocusFlags, Conf_File);

	debugPrintMode = GetPrivateProfileIntA("Debug", "Debug Logging Mode", debugPrintMode, Conf_File);
	traceEnabled = 0!=GetPrivateProfileIntA("Debug", "Load Debug Tracing", traceEnabled, Conf_File);
	crcVerifyEnabled = 0!=GetPrivateProfileIntA("General", "Verify CRCs", crcVerifyEnabled, Conf_File);

	if (RWSaveWindowPos)
	{
	ramw_x = GetPrivateProfileIntA ("Watches", "Ramwatch_X", 0, Conf_File);
	ramw_y = GetPrivateProfileIntA ("Watches", "Ramwatch_Y", 0, Conf_File);
	}

	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		GetPrivateProfileStringA("Watches", str, "", &rw_recent_files[i][0], 1024, Conf_File);
	}

	// TODO: Load hotkeys.
	//LoadHotkeys(Conf_File, true);
	//LoadHotkeys(Conf_File, false);

	// done loading

	//UpdateReverseLookup(true);
	//UpdateReverseLookup(false);
	
	// Reeeeeeeeeeeeally weird code...
	static bool first = true;
	if(first)
		first = false;
	else
		debugprintf("loaded config from \"%s\".\n", Conf_File);

	return 1;
}


int Config::Load_As_Config(HWND hWnd, HINSTANCE hInst)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectoryA(thisprocessPath);

	strcpy(&Name[0], defaultConfigFilename);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = thisprocessPath;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(GetOpenFileName(&ofn))
	{
		Load_Config(Name);
		return 1;
	}
	else return 0;
}

