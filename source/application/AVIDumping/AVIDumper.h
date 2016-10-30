/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

void ProcessCaptureFrameInfo(LPCVOID frameCaptureInfoRemoteAddr, int frameCaptureInfoType);
void ProcessCaptureSoundInfo();

// Must be called once and ONCE only, recommended to make the call from the initialization code of the whole program!
void InitAVICriticalSections();

bool SetAVIFilename(LPCWSTR filename);
void SetLastFrameSoundInfo(void* soundInfoPointer);
void SetGammaRamp(void* gammaRampPointer, HANDLE process);
void SetPaletteEntriesPointer(void* pointer);
void SetCaptureProcess(HANDLE process);

void CloseAVI();
void SplitAVINow();
void HandleAviSplitRequests();
