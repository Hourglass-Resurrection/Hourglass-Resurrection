#include <windows.h>

#include "CustomDLGs.h"

extern HWND hWnd;

// A normal MessageBox, but the parent has a default value now.
int NormalMessageBox(LPCSTR lpText, LPCSTR lpCaption, UINT uType, HWND parent)
{
	if(!parent)
		parent = hWnd;
	return MessageBox(parent, lpText, lpCaption, uType);
}

// like MessageBox but gives us focus for the message and restores focus afterward
int CustomMessageBox(LPCSTR lpText, LPCSTR lpCaption, UINT uType, HWND parent)
{
	if(!parent)
		parent = hWnd;
	HWND prevwnd = GetForegroundWindow();
	SetForegroundWindow(parent);
	int rv = MessageBox(parent, lpText, lpCaption, uType);
	SetForegroundWindow(prevwnd);
	return rv;
}
