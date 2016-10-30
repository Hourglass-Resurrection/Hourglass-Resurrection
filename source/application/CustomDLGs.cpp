#include <windows.h>

#include "CustomDLGs.h"

extern HWND hWnd;

// A normal MessageBox, but the parent has a default value now.
int NormalMessageBox(LPCWSTR text, LPCWSTR caption, UINT type, HWND parent)
{
    if (parent == nullptr)
    {
        parent = hWnd;
    }
    return MessageBoxW(parent, text, caption, type);
}

// like MessageBox but gives us focus for the message and restores focus afterward
int CustomMessageBox(LPCWSTR text, LPCWSTR caption, UINT type, HWND parent)
{
    if (parent == nullptr)
    {
        parent = hWnd;
    }
    HWND prevwnd = GetForegroundWindow();
    SetForegroundWindow(parent);
    int rv = MessageBoxW(parent, text, caption, type);
    SetForegroundWindow(prevwnd);
    return rv;
}
