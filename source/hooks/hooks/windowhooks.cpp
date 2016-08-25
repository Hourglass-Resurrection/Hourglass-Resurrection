/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../wintasee.h"
#include "../tls.h"
#include "../msgqueue.h"
#include "../locale.h"
#include <map>

using Log = DebugLog<LogCategory::WINDOW>;

namespace Hooks
{
    static int createWindowDepth = 0;

    std::map<HWND, WNDPROC> hwndToOrigHandler;
    //std::map<HWND, BOOL> hwndDeniedDeactivate;
    //std::map<HWND, BOOL> hwndRespondingToPaintMessage;

    HOOK_FUNCTION(HWND, WINAPI, CreateWindowExA, DWORD dwExStyle, LPCSTR lpClassName,
        LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    HOOKFUNC HWND WINAPI MyCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
        LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        ENTER(X, Y, nWidth, nHeight, dwStyle, dwExStyle);
        createWindowDepth++;

        //if(tasflags.forceWindowed && X == 0 && Y == 0 && nWidth > 640 && nHeight > 480)
        //{
        //	// check for exact matches with the screen size
        //	// (because this might be a fake-fullscreen window)
        //	if(nWidth == GetSystemMetrics(SM_CXSCREEN) && nHeight == GetSystemMetrics(SM_CYSCREEN))
        //	{
        //		nWidth = 640;
        //		nHeight = 480;
        //		dwStyle &= ~WS_POPUP;
        //		dwStyle |= WS_CAPTION;
        //	}
        //}

        HWND oldGamehwnd = gamehwnd;

        ThreadLocalStuff& curtls = tls;
        curtls.callerisuntrusted++;
        curtls.treatDLLLoadsAsClient++;
        HWND hwnd = CreateWindowExA(dwExStyle, lpClassName,
            lpWindowName, dwStyle, X, Y, nWidth, nHeight,
            hWndParent, hMenu, hInstance, lpParam);

        MySetWindowTextA(hwnd, lpWindowName);

        curtls.treatDLLLoadsAsClient--;
        curtls.callerisuntrusted--;
        LOG() << "made hwnd = " << hwnd;
#ifdef EMULATE_MESSAGE_QUEUES
        if (hwnd)
        {
            MessageQueue& mq = curtls.messageQueue;
            //		if(mq.attachedWindows.empty())
            //			mq.attachedWindows.insert((HWND)NULL); // so PostMessage with a NULL HWND knows to post to the current thread
            mq.attachedWindows.push_back(hwnd);
        }
#endif
        createWindowDepth--;
        if (hwnd && createWindowDepth == 0)
        {
            if (!oldGamehwnd)
            {
                curtls.createdFirstWindow = true;
                gamehwnd = hwnd;
            }

            WNDPROC oldProc = (WNDPROC)MyGetWindowLongA(hwnd, GWL_WNDPROC);
            if (!oldProc)
            {
                WNDCLASSEXA cls = { sizeof(WNDCLASSEXA) };
                GetClassInfoExA(hInstance, lpClassName, &cls);
                if (cls.lpfnWndProc)
                {
                    oldProc = cls.lpfnWndProc;
                    LOG() << "WARNING: had to retrieve wndproc from wndclass (\""
                          << lpClassName << ") for some reason...\n";
                }
            }
            LOG() << "oldProc[" << hwnd << "] = " << reinterpret_cast<DWORD>(oldProc);
            hwndToOrigHandler[hwnd] = oldProc;
            SetWindowLongA(hwnd, GWL_WNDPROC, (LONG)MyWndProcA);
            IPC::SendIPCMessage(IPC::Command::CMD_HWND, &hwnd, sizeof(&hwnd));

            if (tasflags.windowActivateFlags & 2)
            {
                // hmm, I'm getting desyncs all of a sudden...
                // the wintaser window flickers every time it happens
                // but I don't know what could be causing that.
                // well, maybe it's happening less now for some reason,
                // but it's something to watch for (possible bugs here)

    //			tasflags.windowActivateFlags ^= 2;
    //			ShowWindow(hwnd, TRUE);
                SetForegroundWindow(hwnd);
                //SetActiveWindow(hwnd);
                //SetFocus(hwnd);
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                //			tasflags.windowActivateFlags ^= 2;
            }
            else
            {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            SetActiveWindow(hwnd);


            // FIXME TEMP maybe need to hook SetActiveWindow / SetForegroundWindow etc instead
            DispatchMessageInternal(hwnd, WM_ACTIVATE, WA_ACTIVE, (LPARAM)hwnd);
            DispatchMessageInternal(hwnd, WM_SETFOCUS, 0, 0);

            /*WINDOWPOS pos = {
                hwnd,//HWND    hwnd;
                hWndParent,//HWND    hwndInsertAfter;
                X,//int     x;
                Y,//int     y;
                nWidth,//int     cx;
                nHeight,//int     cy;
                SWP_NOREDRAW|SWP_NOACTIVATE|SWP_FRAMECHANGED,//UINT    flags;
            };*/
            //DispatchMessageInternal(hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);

            CREATESTRUCTA create = {
                lpParam,//LPVOID    lpCreateParams;
                hInstance,//HINSTANCE hInstance;
                hMenu,//HMENU     hMenu;
                hWndParent,//HWND      hwndParent;
                nHeight,//int       cy;
                nWidth,//int       cx;
                Y,//int       y;
                X,//int       x;
                static_cast<LONG>(dwStyle),//LONG      style;
                lpWindowName,//LPCTSTR   lpszName;
                lpClassName,//LPCTSTR   lpszClass;
                dwExStyle,//DWORD     dwExStyle;
            };
            DispatchMessageInternal(hwnd, WM_CREATE, 0, (LPARAM)&create);
        }
        return hwnd;
    }
    HOOK_FUNCTION(HWND, WINAPI, CreateWindowExW, DWORD dwExStyle, LPCWSTR lpClassName,
        LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    HOOKFUNC HWND WINAPI MyCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
        LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        ENTER(X, Y, nWidth, nHeight, dwStyle, dwExStyle);
        LOG() << "WARNING: Not yet finished";
        createWindowDepth++;
        HWND oldGamehwnd = gamehwnd;
        ThreadLocalStuff& curtls = tls;
        curtls.callerisuntrusted++;
        curtls.treatDLLLoadsAsClient++;
        HWND hwnd = CreateWindowExW(dwExStyle, lpClassName,
            lpWindowName, dwStyle, X, Y, nWidth, nHeight,
            hWndParent, hMenu, hInstance, lpParam);
        curtls.treatDLLLoadsAsClient--;
        curtls.callerisuntrusted--;
        LOG() << "made hwnd = " << hwnd;
#ifdef EMULATE_MESSAGE_QUEUES
        if (hwnd)
        {
            MessageQueue& mq = curtls.messageQueue;
            //		if(mq.attachedWindows.empty())
            //			mq.attachedWindows.insert((HWND)NULL); // so PostMessage with a NULL HWND knows to post to the current thread
            mq.attachedWindows.push_back(hwnd);
        }
#endif
        createWindowDepth--;
#if 0 // FIXME should be enabled but currently breaks Iji due to some bug
        if (hwnd && createWindowDepth == 0)
        {
            if (!oldGamehwnd)
            {
                curtls.createdFirstWindow = true;
                gamehwnd = hwnd;
            }

            //WNDCLASSEXA cls = { sizeof(WNDCLASSEXA) };
            //GetClassInfoExA(hInstance, lpClassName, &cls);

            WNDPROC oldProc = (WNDPROC)MyGetWindowLongW(hwnd, GWL_WNDPROC);
            if (!oldProc)
            {
                WNDCLASSEXW cls = { sizeof(WNDCLASSEXW) };
                GetClassInfoExW(hInstance, lpClassName, &cls);
                if (cls.lpfnWndProc)
                {
                    oldProc = cls.lpfnWndProc;
                    debuglog(LCF_WINDOW | LCF_TODO, "had to retrieve wndproc from wndclass (\"%S\") for some reason...\n", lpClassName);
                }
            }
            debuglog(LCF_WINDOW, "oldProc[0x%X] = 0x%X\n", hwnd, oldProc);
            debuglog(LCF_WINDOW, "oldProc[0x%X] = 0x%X\n", hwnd, oldProc);
            hwndToOrigHandler[hwnd] = oldProc;
            SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)MyWndProcW);
            cmdprintf("HWND: %d", hwnd);

            if (tasflags.windowActivateFlags & 2)
            {
                // hmm, I'm getting desyncs all of a sudden...
                // the wintaser window flickers every time it happens
                // but I don't know what could be causing that.
                // well, maybe it's happening less now for some reason,
                // but it's something to watch for (possible bugs here)

    //			tasflags.windowActivateFlags ^= 2;
    //			ShowWindow(hwnd, TRUE);
                SetForegroundWindow(hwnd);
                //SetActiveWindow(hwnd);
                //SetFocus(hwnd);
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                //			tasflags.windowActivateFlags ^= 2;
            }
            else
            {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            SetActiveWindow(hwnd);


            // FIXME TEMP maybe need to hook SetActiveWindow / SetForegroundWindow etc instead
            DispatchMessageInternal(hwnd, WM_ACTIVATE, WA_ACTIVE, (LPARAM)hwnd, false);
            DispatchMessageInternal(hwnd, WM_SETFOCUS, 0, 0, false);

            /*WINDOWPOS pos = {
                hwnd,//HWND    hwnd;
                hWndParent,//HWND    hwndInsertAfter;
                X,//int     x;
                Y,//int     y;
                nWidth,//int     cx;
                nHeight,//int     cy;
                SWP_NOREDRAW|SWP_NOACTIVATE|SWP_FRAMECHANGED,//UINT    flags;
            };*/
            //SendMessageW(hwnd, toggleWhitelistMessage(WM_WINDOWPOSCHANGED), 0, (LPARAM)&pos);

            CREATESTRUCTW create = {
                lpParam,//LPVOID    lpCreateParams;
                hInstance,//HINSTANCE hInstance;
                hMenu,//HMENU     hMenu;
                hWndParent,//HWND      hwndParent;
                nHeight,//int       cy;
                nWidth,//int       cx;
                Y,//int       y;
                X,//int       x;
                dwStyle,//LONG      style;
                lpWindowName,//LPCTSTR   lpszName;
                lpClassName,//LPCTSTR   lpszClass;
                dwExStyle,//DWORD     dwExStyle;
            };
            DispatchMessageInternal(hwnd, WM_CREATE, 0, (LPARAM)&create, false);

            // trying to get the stupid splash screen to work, not sure how to fake the paint event well enough for it
            //InvalidateRect(hwnd, NULL, TRUE);
            //PostMessageInternal(hwnd, WM_PAINT, 0, 0, false);
            //InvalidateRect(hwnd, NULL, TRUE);
            //DispatchMessageInternal(hwnd, WM_PAINT, 0, 0, false);
        }
#endif
        return hwnd;
    }

    HOOK_FUNCTION(BOOL, WINAPI, CloseWindow, HWND hWnd);
    HOOKFUNC BOOL WINAPI MyCloseWindow(HWND hWnd)
    {
        ENTER(hWnd);
        BOOL rv = CloseWindow(hWnd);
        return rv;
    }

    HOOK_FUNCTION(BOOL, WINAPI, DestroyWindow, HWND hWnd);
    HOOKFUNC BOOL WINAPI MyDestroyWindow(HWND hWnd)
    {
        ENTER(hWnd);
        ThreadLocalStuff& curtls = tls;
        curtls.destroyWindowDepth++;
        BOOL rv = DestroyWindow(hWnd);
        curtls.destroyWindowDepth--;
#ifdef EMULATE_MESSAGE_QUEUES
        if (rv)
        {
            MessageQueue& mq = tls.messageQueue;
            std::vector<HWND>& v = tls.messageQueue.attachedWindows;
            v.erase(std::remove(v.begin(), v.end(), hWnd), v.end());
        }
#endif
        return rv;
    }

    HOOK_FUNCTION(HWND, WINAPI, GetActiveWindow);
    HOOKFUNC HWND WINAPI MyGetActiveWindow()
    {
        ENTER(gamehwnd);
        return gamehwnd;
    }
    HOOK_FUNCTION(HWND, WINAPI, GetForegroundWindow);
    HOOKFUNC HWND WINAPI MyGetForegroundWindow()
    {
        ENTER(gamehwnd);
        return gamehwnd;
    }
    HOOK_FUNCTION(HWND, WINAPI, GetFocus);
    HOOKFUNC HWND WINAPI MyGetFocus()
    {
        ENTER(gamehwnd);
        return gamehwnd;
    }


    HOOK_FUNCTION(LONG, WINAPI, GetWindowLongA, HWND hWnd, int nIndex);
    HOOKFUNC LONG WINAPI MyGetWindowLongA(HWND hWnd, int nIndex)
    {
        ENTER(nIndex, hWnd);
        if (nIndex == GWL_WNDPROC)
        {
            std::map<HWND, WNDPROC>::iterator found = hwndToOrigHandler.find(hWnd);
            if (found != hwndToOrigHandler.end())
            {
                LOG() << "returning known WNDPROC: "
                      << reinterpret_cast<LPVOID>(found->second);
                return (LONG)found->second;
            }
            else
            {
                LONG rv = GetWindowLongA(hWnd, nIndex);
                LOG() << "returning unknown WNDPROC: " << rv;
                return rv;
            }
        }
        LONG rv = GetWindowLongA(hWnd, nIndex);
        LOG() << "rv: "<< rv;
        return rv;
    }
    HOOK_FUNCTION(LONG, WINAPI, GetWindowLongW, HWND hWnd, int nIndex);
    HOOKFUNC LONG WINAPI MyGetWindowLongW(HWND hWnd, int nIndex)
    {
        ENTER(nIndex, hWnd);
        if (nIndex == GWL_WNDPROC)
        {
            std::map<HWND, WNDPROC>::iterator found = hwndToOrigHandler.find(hWnd);
            if (found != hwndToOrigHandler.end())
            {
                LOG() << "returning known WNDPROC: "
                      << reinterpret_cast<LPVOID>(found->second);
                return (LONG)found->second;
            }
            else
            {
                LONG rv = GetWindowLongW(hWnd, nIndex);
                LOG() << "returning unknown WNDPROC: " << rv;
                return rv;
            }
        }
        LONG rv = GetWindowLongW(hWnd, nIndex);
        LOG() << "rv: " << rv;
        return rv;
    }


    HOOK_FUNCTION(LONG, WINAPI, SetWindowLongA, HWND hWnd, int nIndex, LONG dwNewLong);
    HOOKFUNC LONG WINAPI MySetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        ENTER(hWnd, nIndex, dwNewLong);
        if (nIndex == GWL_WNDPROC)
        {
            // the game is trying to change this window's procedure.
            // since we need it to stay replaced with our own winproc,
            // update our pointer to the original winproc instead.
            LONG rv = MyGetWindowLongA(hWnd, nIndex);
            LOG() << "hwndToOrigHandler[" << hWnd << "] = " << dwNewLong;
            LEAVE(rv);
            hwndToOrigHandler[hWnd] = (WNDPROC)dwNewLong;
            SetWindowLongA(hWnd, GWL_WNDPROC, (LONG)MyWndProcA);
            return rv;
        }
        if (nIndex == GWL_STYLE)
        {
            // some SDL apps create a window, attach d3d,
            // then modify the window style for fullscreen.
            // disallow that last step if the window has been fake-fullscreen locked.
            if (IsWindowFakeFullscreen(hWnd))
            {
                return MyGetWindowLongA(hWnd, nIndex);
            }
        }
        LONG rv = SetWindowLongA(hWnd, nIndex, dwNewLong);
        LEAVE(rv);
        return rv;
    }
    HOOK_FUNCTION(LONG, WINAPI, SetWindowLongW, HWND hWnd, int nIndex, LONG dwNewLong);
    HOOKFUNC LONG WINAPI MySetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        ENTER(hWnd, nIndex, dwNewLong);
        if (nIndex == GWL_WNDPROC)
        {
            // the game is trying to change this window's procedure.
            // since we need it to stay replaced with our own winproc,
            // update our pointer to the original winproc instead.
            LONG rv = MyGetWindowLongW(hWnd, nIndex);
            LOG() << "hwndToOrigHandler[" << hWnd << "] = " << dwNewLong;
            LEAVE(rv);
            hwndToOrigHandler[hWnd] = (WNDPROC)dwNewLong;
            SetWindowLongW(hWnd, GWL_WNDPROC, (LONG)MyWndProcW);
            return rv;
        }
        if (nIndex == GWL_STYLE)
        {
            // some SDL apps create a window, attach d3d,
            // then modify the window style for fullscreen.
            // disallow that last step if the window has been fake-fullscreen locked.
            if (IsWindowFakeFullscreen(hWnd))
            {
                return MyGetWindowLongW(hWnd, nIndex);
            }
        }
        LONG rv = SetWindowLongW(hWnd, nIndex, dwNewLong);
        LEAVE(rv);
        return rv;
    }


    HOOK_FUNCTION(BOOL, WINAPI, MoveWindow, HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
    HOOKFUNC BOOL WINAPI MyMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
    {
        ENTER(hWnd, X, Y, nWidth, nHeight, bRepaint);
        if (tasflags.forceWindowed)
        {
            if (IsWindowFakeFullscreen(hWnd))
            {
                RECT rect;
                if (GetWindowRect(hWnd, &rect))
                {
                    nWidth = rect.right - rect.left;
                    nHeight = rect.bottom - rect.top;
                }
            }
        }
        BOOL rv = MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, SetWindowPos, HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
    HOOKFUNC BOOL WINAPI MySetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        if (tasflags.forceWindowed)
        {
            if (IsWindowFakeFullscreen(hWnd))
                uFlags |= SWP_NOMOVE | SWP_NOSIZE;
        }
        if (tasflags.windowActivateFlags & 2)
        {
            if (hWndInsertAfter == HWND_NOTOPMOST || hWndInsertAfter == HWND_BOTTOM || hWndInsertAfter == HWND_TOP || hWndInsertAfter == NULL)
                hWndInsertAfter = HWND_TOPMOST;
        }
        else
        {
            if (hWndInsertAfter == HWND_TOPMOST)
                hWndInsertAfter = HWND_NOTOPMOST;
        }
        BOOL rv = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

        // SetWindowPos normally sends WM_WINDOWPOSCHANGED,
        // but to ensure that other things we don't control can't also cause that message to get processed,
        // we block it in GetMessageActionFlags, and send a special version of it that won't get blocked here.
        WINDOWPOS pos = { hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags, };
        //	if(fakeDisplayValid && IsWindowFakeFullscreen(hWnd))
        //		DispatchMessageInternal(hWnd, WM_WINDOWPOSCHANGED-2, 0, (LPARAM)&pos);
        //	else
        DispatchMessageInternal(hWnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);

        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, ShowWindow, HWND hWnd, int nCmdShow);
    HOOKFUNC BOOL WINAPI MyShowWindow(HWND hWnd, int nCmdShow)
    {
        if (tasflags.forceWindowed && nCmdShow == SW_MAXIMIZE)
            nCmdShow = SW_SHOWNORMAL;
        BOOL rv = ShowWindow(hWnd, nCmdShow);
        return 1;
    }

    HOOK_FUNCTION(BOOL, WINAPI, GetClientRect, HWND hWnd, LPRECT lpRect);
    HOOKFUNC BOOL WINAPI MyGetClientRect(HWND hWnd, LPRECT lpRect)
    {
        // IsWindowFakeFullscreen checks disabled because they let window position info leak to Eternal Daughter and possibly others (maybe need to use ::IsChild inside IsWindowFakeFullscreen)
        // VerifyIsTrustedCaller checks added as a hack so that DirectDrawClipper can still get the real window coordinates
        // TODO: instead of calling VerifyIsTrustedCaller we could probably have MyDirectDrawSurface::MyBlt set a flag for us. although maybe this way is safer.
        if (fakeDisplayValid/* && IsWindowFakeFullscreen(hWnd)*/ && VerifyIsTrustedCaller(!tls.callerisuntrusted))
        {
            if (!lpRect)
                return FALSE;
            lpRect->left = 0;
            lpRect->top = 0;
            lpRect->right = fakeDisplayWidth;
            lpRect->bottom = fakeDisplayHeight;
            return TRUE;
        }
        return GetClientRect(hWnd, lpRect);
    }
    HOOK_FUNCTION(BOOL, WINAPI, GetWindowRect, HWND hWnd, LPRECT lpRect);
    HOOKFUNC BOOL WINAPI MyGetWindowRect(HWND hWnd, LPRECT lpRect)
    {
        // see coments in MyGetClientRect
        if (fakeDisplayValid/* && IsWindowFakeFullscreen(hWnd)*/ && VerifyIsTrustedCaller(!tls.callerisuntrusted))
        {
            if (!lpRect)
                return FALSE;
            lpRect->left = 0;
            lpRect->top = 0;
            lpRect->right = fakeDisplayWidth;
            lpRect->bottom = fakeDisplayHeight;
            return TRUE;
        }
        return GetWindowRect(hWnd, lpRect);
    }

    HOOK_FUNCTION(BOOL, WINAPI, ClientToScreen, HWND hWnd, LPPOINT lpPoint);
    HOOKFUNC BOOL WINAPI MyClientToScreen(HWND hWnd, LPPOINT lpPoint)
    {
        // see coments in MyGetClientRect
        if (fakeDisplayValid/* && IsWindowFakeFullscreen(hWnd)*/ && VerifyIsTrustedCaller(!tls.callerisuntrusted))
        {
            return (lpPoint != NULL);
        }
        return ClientToScreen(hWnd, lpPoint);
    }
    HOOK_FUNCTION(BOOL, WINAPI, ScreenToClient, HWND hWnd, LPPOINT lpPoint);
    HOOKFUNC BOOL WINAPI MyScreenToClient(HWND hWnd, LPPOINT lpPoint)
    {
        // see coments in MyGetClientRect
        if (fakeDisplayValid/* && IsWindowFakeFullscreen(hWnd)*/ && VerifyIsTrustedCaller(!tls.callerisuntrusted))
        {
            return (lpPoint != NULL);
        }
        return ScreenToClient(hWnd, lpPoint);
    }

    HOOK_FUNCTION(BOOL, WINAPI, SetWindowTextA, HWND hWnd, LPCSTR lpString);
    HOOKFUNC BOOL WINAPI MySetWindowTextA(HWND hWnd, LPCSTR lpString)
    {
        ENTER(hWnd, lpString);
        if (tasflags.appLocale)
        {
            str_to_wstr(wstr, lpString, LocaleToCodePage(tasflags.appLocale));
            BOOL rv = MySetWindowTextW(hWnd, wstr);
            DispatchMessageInternal(hWnd, WM_SETTEXT, 0, (LPARAM)wstr, false, MAF_BYPASSGAME | MAF_RETURN_OS);
            return rv;
        }
        BOOL rv = SetWindowTextA(hWnd, lpString);
        DispatchMessageInternal(hWnd, WM_SETTEXT, 0, (LPARAM)lpString, true, MAF_BYPASSGAME | MAF_RETURN_OS);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, SetWindowTextW, HWND hWnd, LPCWSTR lpString);
    HOOKFUNC BOOL WINAPI MySetWindowTextW(HWND hWnd, LPCWSTR lpString)
    {
        ENTER(hWnd, lpString);
        BOOL rv = SetWindowTextW(hWnd, lpString);
        DispatchMessageInternal(hWnd, WM_SETTEXT, 0, (LPARAM)lpString, false, MAF_BYPASSGAME | MAF_RETURN_OS);
        return rv;
    }


    int GetDefaultMessageBoxResult(UINT uType)
    {
        if ((uType & MB_DEFBUTTON2) == MB_DEFBUTTON2)
        {
            switch (uType & 0xF)
            {
            case MB_OKCANCEL:
            case MB_RETRYCANCEL:
                return IDCANCEL;
            case MB_ABORTRETRYIGNORE:
            case MB_CANCELTRYCONTINUE:
                return IDRETRY;
            case MB_YESNO:
            case MB_YESNOCANCEL:
                return IDNO;
            }
        }
        if ((uType & MB_DEFBUTTON3) == MB_DEFBUTTON3)
        {
            switch (uType & 0xF)
            {
            case MB_ABORTRETRYIGNORE:
                return IDIGNORE;
            case MB_CANCELTRYCONTINUE:
                return IDCONTINUE;
            }
        }
        switch (uType & 0xF)
        {
        default:
        case MB_OK:
        case MB_OKCANCEL:
            return IDOK;
        case MB_YESNO:
        case MB_YESNOCANCEL:
            return IDYES;
            // instead of choosing button 1, let's try the most "keep going" option
        case MB_ABORTRETRYIGNORE:
            return IDIGNORE;
        case MB_RETRYCANCEL:
            return IDCANCEL;
        case MB_CANCELTRYCONTINUE:
            return IDCONTINUE;
        }
    }

    // TODO: better support for messageboxes and dialog boxes (like, draw them, allow a choice, and record a frame of input for them)
    HOOK_FUNCTION(int, WINAPI, MessageBoxA, HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
    HOOKFUNC int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
    {
        ENTER(lpCaption, lpText);
        cmdprintf("SHORTTRACE: 3,50");
        return GetDefaultMessageBoxResult(uType);
    }
    HOOK_FUNCTION(int, WINAPI, MessageBoxW, HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
    HOOKFUNC int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
    {
        ENTER(lpCaption, lpText);
        cmdprintf("SHORTTRACE: 3,50");
        return GetDefaultMessageBoxResult(uType);
    }
    HOOK_FUNCTION(int, WINAPI, MessageBoxExA, HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId);
    HOOKFUNC int WINAPI MyMessageBoxExA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId)
    {
        ENTER(lpCaption, lpText);
        return GetDefaultMessageBoxResult(uType);
    }
    HOOK_FUNCTION(int, WINAPI, MessageBoxExW, HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId);;
    HOOKFUNC int WINAPI MyMessageBoxExW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId)
    {
        ENTER(lpCaption, lpText);
        return GetDefaultMessageBoxResult(uType);
    }
    HOOK_FUNCTION(INT_PTR, WINAPI, DialogBoxParamA, HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOKFUNC INT_PTR WINAPI MyDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
    {
        ENTER();
        return IDOK;
    }
    HOOK_FUNCTION(INT_PTR, WINAPI, DialogBoxParamW, HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOKFUNC INT_PTR WINAPI MyDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
    {
        ENTER();
        return IDOK;
    }
    HOOK_FUNCTION(INT_PTR, WINAPI, DialogBoxIndirectParamA, HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOKFUNC INT_PTR WINAPI MyDialogBoxIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
    {
        ENTER();
        return IDOK;
    }
    HOOK_FUNCTION(INT_PTR, WINAPI, DialogBoxIndirectParamW, HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOKFUNC INT_PTR WINAPI MyDialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
    {
        ENTER();
        return IDOK;
    }

    void ApplyWindowIntercepts()
    {
        static const InterceptDescriptor intercepts[] =
        {
            MAKE_INTERCEPT(1, USER32, CreateWindowExA),
            MAKE_INTERCEPT(1, USER32, CreateWindowExW), // FIXME?
            MAKE_INTERCEPT(1, USER32, DestroyWindow),
            MAKE_INTERCEPT(1, USER32, CloseWindow),
            MAKE_INTERCEPT(1, USER32, GetActiveWindow),
            MAKE_INTERCEPT(1, USER32, GetForegroundWindow),
            MAKE_INTERCEPT(1, USER32, GetFocus),
            MAKE_INTERCEPT(1, USER32, SetWindowLongA),
            MAKE_INTERCEPT(1, USER32, SetWindowLongW),
            MAKE_INTERCEPT(1, USER32, GetWindowLongA),
            MAKE_INTERCEPT(1, USER32, GetWindowLongW),
            MAKE_INTERCEPT(1, USER32, MoveWindow),
            MAKE_INTERCEPT(1, USER32, SetWindowPos),
            MAKE_INTERCEPT(1, USER32, ShowWindow),
            MAKE_INTERCEPT(1, USER32, GetClientRect),
            MAKE_INTERCEPT(1, USER32, GetWindowRect),
            MAKE_INTERCEPT(1, USER32, ClientToScreen),
            MAKE_INTERCEPT(1, USER32, ScreenToClient),
            MAKE_INTERCEPT(1, USER32, SetWindowTextA),
            MAKE_INTERCEPT(1, USER32, SetWindowTextW),
            //MAKE_INTERCEPT(1, USER32, InvalidateRect),
            //MAKE_INTERCEPT(1, USER32, UpdateWindow),
            MAKE_INTERCEPT(1, USER32, MessageBoxA),
            MAKE_INTERCEPT(1, USER32, MessageBoxW),
            MAKE_INTERCEPT(1, USER32, MessageBoxExA),
            MAKE_INTERCEPT(1, USER32, MessageBoxExW),
            MAKE_INTERCEPT(1, USER32, DialogBoxParamA),
            MAKE_INTERCEPT(1, USER32, DialogBoxParamW),
            MAKE_INTERCEPT(1, USER32, DialogBoxIndirectParamA),
            MAKE_INTERCEPT(1, USER32, DialogBoxIndirectParamW),
        };
        ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
    }
}
