/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

static void EnableDisablePlayRecordButtons(HWND hDlg);

void DetectWindowsVersion(int *major, int *minor);

bool IsWindowsXP();
bool IsWindowsVista();
bool IsWindows7();
bool IsWindows8();

std::wstring NormalizePath(const std::wstring& path);
std::wstring TranslateDeviceName(const std::wstring& filename);
std::wstring GetFileNameFromProcessHandle(HANDLE hProcess);
std::wstring GetFileNameFromFileHandle(HANDLE file);

int GetThreadSuspendCount(HANDLE hThread);

void SetThreadSuspendCount(HANDLE hThread, int count);

void UpdateFrameCountDisplay(int frameCount, int frequency);

void UpdateRerecordCountDisplay();

LONGLONG CalcFilesize(const std::wstring& path);
LONGLONG CalcExeFilesize();

std::wstring GetFilenameWithoutPath(const std::wstring& path);
std::wstring GetExeFilenameWithoutPath();

template<typename T>
void ClearAndDeallocateContainer(T& c);


//SharedDataHandle* FindMatchingDataBlock(SaveState::MemoryRegion& region);

void ReceiveCommandSlotPointer(LPVOID command_slot_pointer);
void SendCommand();
void ClearCommand();

void ReceiveInputsPointer(LPVOID remote_inputs_pointer);

void SendTASFlags();

void ReceiveTASFlagsPointer(LPVOID tas_flags_pointer);

void ReceiveDllLoadInfosPointer(LPVOID dll_load_info_pointer);


void ReceiveGeneralInfoPointer(LPVOID general_info_pointer);

void ReceiveSoundCaptureInfoPointer(LPVOID sound_capture_info_pointer);

void ReceiveGammaRampData(LPVOID gamma_ramp_pointer, HANDLE process);

void ReceivePaletteEntriesPointer(LPVOID palette_entries_pointer);

void ReceiveKeyboardLayout(LPVOID keyboard_layout_pointer, HANDLE hProcess);

void CheckSrcDllVersion(DWORD version);

//void SuggestThreadName(DWORD thread_id, DWORD process_id, IPC::SuggestThreadName& thread_name);

void SaveMovie(const std::wstring& filename);

int LoadMovie(const std::wstring& filename);

void SaveGameStatePhase2(int slot);

struct Movie;

bool MovieStatePreceeds(const Movie& shorter, const Movie& longer);

void LoadGameStatePhase2(int slot);
void RecoverStaleState(int slot);

void LoadGameStatePhase2(int slot);

bool CreateAVIFile();


void SendCommand();

void ClearCommand();

void SetFastForward(bool set);

void SaveGameStatePhase1(int slot);

void LoadGameStatePhase1(int slot);

void HandleRemotePauseEvents();

void SendTASFlags();

bool InputHasFocus(bool isHotkeys);

void RecordLocalInputs();

void InjectCurrentMovieFrame();

void RefreshSavestates(int frameCount);

void SuspendAllExcept(int ignoreThreadID);
void ResumeAllExcept(int ignoreThreadID);
void CheckDialogChanges(int frameCount);

void CheckHotkeys(int frameCount, bool frameSynced);

void AddAndSendDllInfo(LPCWSTR filename, bool loaded, HANDLE hProcess);

void UpdateGeneralInfoDisplay();

void UpdateFrameCountDisplay(int frameCount, int frequency);

void CheckDialogChanges(int frameCount);

void DoPow2Logic(int frameCount);
void SleepFrameBoundary(const char* frameInfo, int threadId);

//void FrameBoundary(IPC::FrameBoundaryInfo& frame_info, int threadId);

void DoPow2Logic(int frameCount);


void SuspendAllExcept(int ignoreThreadID);

void ResumeAllExcept(int ignoreThreadID);

//void ReceiveFrameRate(IPC::FPSInfo& fps_info);

void ReceiveHWND(HWND hwnd);

void PrintPrivileges(HANDLE hProcess);

std::wstring AbsolutifyPath(const std::wstring& path);

std::wstring ExceptionCodeToDescription(DWORD code);

int GetBreakpointIndex(DWORD address, DWORD threadId);

void AddBreakpoint(DWORD address, DWORD threadId, HANDLE hProcess);

void RemoveBreakpoint(DWORD address, DWORD threadId, HANDLE hProcess);

bool SetProgramCounter(HANDLE hThread, DWORD address);

DWORD GetProgramCounter(HANDLE hThread);

void CloseThreadHandles(HANDLE threadHandleToNotClose, HANDLE hProcess);

void DebuggerThreadFuncCleanup(HANDLE threadHandleToClose, HANDLE hProcess);

HANDLE GetProcessHandle(PROCESS_INFORMATION& processInfo, const DEBUG_EVENT& de);

void HandleThreadExitEvent(DEBUG_EVENT& de, PROCESS_INFORMATION& processInfo);


void OnMovieStart();

DWORD WINAPI DebuggerThreadFunc(LPVOID lpParam);

DWORD WINAPI AfterDebugThreadExitThread(LPVOID lpParam);

void OnAfterDebugThreadExit();

void TerminateDebuggerThread(DWORD maxWaitTime);

void WaitForOtherThreadToTerminate(HANDLE& thread, DWORD maxWaitTime);



bool PreTranslateMessage(MSG& msg);

DWORD GetErrorModeXP();

static void EnableDisablePlayRecordButtons(HWND hDlg);

void BringGameWindowToForeground();

BOOL DirectoryExists(LPCWSTR path);

void SplitToValidPath(LPCWSTR initialPath, LPCWSTR defaultDir, LPWSTR filename, LPWSTR directory);

BOOL SetWindowTextAndScrollRight(HWND hEdit, LPCWSTR lpString);

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

namespace HourglassCore
{
    void SetMovieReadOnly(bool read_only);

    void PrepareForExit();
}
