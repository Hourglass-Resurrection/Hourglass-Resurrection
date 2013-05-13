/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(MESSAGEHOOKS_INCL) && !defined(UNITY_BUILD)
#define MESSAGEHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../tls.h"
#include "../wintasee.h"
#include "../msgqueue.h"
#include <map>
#include <vector>

extern std::map<HWND, WNDPROC> hwndToOrigHandler;


static MessageActionFlags GetMessageActionFlags(UINT message, WPARAM wParam, LPARAM lParam);


HOOKFUNC BOOL WINAPI MyPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam); // extern? (I mean, move to header)
LRESULT DispatchMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/); // extern? (I mean, move to header)
void PostMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, struct MessageQueue* pmq/*=NULL*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/); // extern? (I mean, move to header)

// TODO: move to shared?
static const char* GetWindowsMessageName(UINT message)
{
	switch(message)
	{
#define GWMNcase(wm) case wm: return #wm;
		GWMNcase(WM_NULL)
		GWMNcase(WM_PAINT)
		GWMNcase(WM_CREATE)
		GWMNcase(WM_DESTROY)
		GWMNcase(WM_MOVE)
		GWMNcase(WM_SIZE)
		GWMNcase(WM_ACTIVATE)
		GWMNcase(WM_SETFOCUS)
		GWMNcase(WM_KILLFOCUS)
		GWMNcase(WM_ENABLE)
		GWMNcase(WM_SETREDRAW)
		GWMNcase(WM_SETTEXT)
		GWMNcase(WM_GETTEXT)
		GWMNcase(WM_GETTEXTLENGTH)
		GWMNcase(WM_CLOSE)
		GWMNcase(WM_QUERYENDSESSION)
		GWMNcase(WM_QUERYOPEN)
		GWMNcase(WM_ENDSESSION)
		GWMNcase(WM_QUIT)
		GWMNcase(WM_ERASEBKGND)
		GWMNcase(WM_SYSCOLORCHANGE)
		GWMNcase(WM_SHOWWINDOW)
		GWMNcase(WM_WININICHANGE)
		GWMNcase(WM_DEVMODECHANGE)
		GWMNcase(WM_ACTIVATEAPP)
		GWMNcase(WM_FONTCHANGE)
		GWMNcase(WM_TIMECHANGE)
		GWMNcase(WM_CANCELMODE)
		GWMNcase(WM_SETCURSOR)
		GWMNcase(WM_MOUSEACTIVATE)
		GWMNcase(WM_CHILDACTIVATE)
		GWMNcase(WM_QUEUESYNC)
		GWMNcase(WM_GETMINMAXINFO)
		GWMNcase(WM_PAINTICON)
		GWMNcase(WM_ICONERASEBKGND)
		GWMNcase(WM_NEXTDLGCTL)
		GWMNcase(WM_SPOOLERSTATUS)
		GWMNcase(WM_DRAWITEM)
		GWMNcase(WM_MEASUREITEM)
		GWMNcase(WM_DELETEITEM)
		GWMNcase(WM_VKEYTOITEM)
		GWMNcase(WM_CHARTOITEM)
		GWMNcase(WM_SETFONT)
		GWMNcase(WM_GETFONT)
		GWMNcase(WM_SETHOTKEY)
		GWMNcase(WM_GETHOTKEY)
		GWMNcase(WM_QUERYDRAGICON)
		GWMNcase(WM_COMPAREITEM)
		GWMNcase(WM_GETOBJECT)
		GWMNcase(WM_COMPACTING)
		GWMNcase(WM_COMMNOTIFY)
		GWMNcase(WM_WINDOWPOSCHANGING)
		GWMNcase(WM_WINDOWPOSCHANGED)
		GWMNcase(WM_POWER)
		GWMNcase(WM_COPYDATA)
		GWMNcase(WM_CANCELJOURNAL)
		GWMNcase(WM_NOTIFY)
		GWMNcase(WM_INPUTLANGCHANGEREQUEST)
		GWMNcase(WM_INPUTLANGCHANGE)
		GWMNcase(WM_TCARD)
		GWMNcase(WM_HELP)
		GWMNcase(WM_USERCHANGED)
		GWMNcase(WM_NOTIFYFORMAT)
		GWMNcase(WM_CONTEXTMENU)
		GWMNcase(WM_STYLECHANGING)
		GWMNcase(WM_STYLECHANGED)
		GWMNcase(WM_DISPLAYCHANGE)
		GWMNcase(WM_GETICON)
		GWMNcase(WM_SETICON)
		GWMNcase(WM_NCCREATE)
		GWMNcase(WM_NCDESTROY)
		GWMNcase(WM_NCCALCSIZE)
		GWMNcase(WM_NCHITTEST)
		GWMNcase(WM_NCPAINT)
		GWMNcase(WM_NCACTIVATE)
		GWMNcase(WM_GETDLGCODE)
		GWMNcase(WM_SYNCPAINT)
		GWMNcase(WM_NCMOUSEMOVE)
		GWMNcase(WM_NCLBUTTONDOWN)
		GWMNcase(WM_NCLBUTTONUP)
		GWMNcase(WM_NCLBUTTONDBLCLK)
		GWMNcase(WM_NCRBUTTONDOWN)
		GWMNcase(WM_NCRBUTTONUP)
		GWMNcase(WM_NCRBUTTONDBLCLK)
		GWMNcase(WM_NCMBUTTONDOWN)
		GWMNcase(WM_NCMBUTTONUP)
		GWMNcase(WM_NCMBUTTONDBLCLK)
		GWMNcase(WM_NCXBUTTONDOWN)
		GWMNcase(WM_NCXBUTTONUP)
		GWMNcase(WM_NCXBUTTONDBLCLK)
		case /*WM_NCUAHDRAWCAPTION*/0xAE: return "WM_NCUAHDRAWCAPTION";
		case /*WM_NCUAHDRAWFRAME*/0xAF: return "WM_NCUAHDRAWFRAME";
		case /*WM_INPUT_DEVICE_CHANGE*/0x00FE: return "WM_INPUT_DEVICE_CHANGE";
		case /*WM_INPUT*/0x00FF: return "WM_INPUT";
		GWMNcase(WM_KEYDOWN)
		GWMNcase(WM_KEYUP)
		GWMNcase(WM_CHAR)
		GWMNcase(WM_DEADCHAR)
		GWMNcase(WM_SYSKEYDOWN)
		GWMNcase(WM_SYSKEYUP)
		GWMNcase(WM_SYSCHAR)
		GWMNcase(WM_SYSDEADCHAR)
		case /*WM_UNICHAR*/0x0109: return "WM_UNICHAR";
		GWMNcase(WM_IME_STARTCOMPOSITION)
		GWMNcase(WM_IME_ENDCOMPOSITION)
		GWMNcase(WM_IME_COMPOSITION)
		GWMNcase(WM_INITDIALOG)
		GWMNcase(WM_COMMAND)
		GWMNcase(WM_SYSCOMMAND)
		GWMNcase(WM_TIMER)
		GWMNcase(WM_HSCROLL)
		GWMNcase(WM_VSCROLL)
		GWMNcase(WM_INITMENU)
		GWMNcase(WM_INITMENUPOPUP)
		case /*	*/0x0118: return "WM_SYSTIMER";
		GWMNcase(WM_MENUSELECT)
		GWMNcase(WM_MENUCHAR)
		GWMNcase(WM_ENTERIDLE)
		GWMNcase(WM_MENURBUTTONUP)
		GWMNcase(WM_MENUDRAG)
		GWMNcase(WM_MENUGETOBJECT)
		GWMNcase(WM_UNINITMENUPOPUP)
		GWMNcase(WM_MENUCOMMAND)
		GWMNcase(WM_CHANGEUISTATE)
		GWMNcase(WM_UPDATEUISTATE)
		GWMNcase(WM_QUERYUISTATE)
		GWMNcase(WM_CTLCOLORMSGBOX)
		GWMNcase(WM_CTLCOLOREDIT)
		GWMNcase(WM_CTLCOLORLISTBOX)
		GWMNcase(WM_CTLCOLORBTN)
		GWMNcase(WM_CTLCOLORDLG)
		GWMNcase(WM_CTLCOLORSCROLLBAR)
		GWMNcase(WM_CTLCOLORSTATIC)
		GWMNcase(WM_MOUSEMOVE)
		GWMNcase(WM_LBUTTONDOWN)
		GWMNcase(WM_LBUTTONUP)
		GWMNcase(WM_LBUTTONDBLCLK)
		GWMNcase(WM_RBUTTONDOWN)
		GWMNcase(WM_RBUTTONUP)
		GWMNcase(WM_RBUTTONDBLCLK)
		GWMNcase(WM_MBUTTONDOWN)
		GWMNcase(WM_MBUTTONUP)
		GWMNcase(WM_MBUTTONDBLCLK)
		GWMNcase(WM_MOUSEWHEEL)
		GWMNcase(WM_XBUTTONDOWN)
		GWMNcase(WM_XBUTTONUP)
		GWMNcase(WM_XBUTTONDBLCLK)
		case /*WM_MOUSEHWHEEL*/0x020E: return "WM_MOUSEHWHEEL";
		GWMNcase(WM_PARENTNOTIFY)
		GWMNcase(WM_ENTERMENULOOP)
		GWMNcase(WM_EXITMENULOOP)
		GWMNcase(WM_NEXTMENU)
		GWMNcase(WM_SIZING)
		GWMNcase(WM_CAPTURECHANGED)
		GWMNcase(WM_MOVING)
		GWMNcase(WM_POWERBROADCAST)
		GWMNcase(WM_DEVICECHANGE)
		GWMNcase(WM_MDICREATE)
		GWMNcase(WM_MDIDESTROY)
		GWMNcase(WM_MDIACTIVATE)
		GWMNcase(WM_MDIRESTORE)
		GWMNcase(WM_MDINEXT)
		GWMNcase(WM_MDIMAXIMIZE)
		GWMNcase(WM_MDITILE)
		GWMNcase(WM_MDICASCADE)
		GWMNcase(WM_MDIICONARRANGE)
		GWMNcase(WM_MDIGETACTIVE)
		GWMNcase(WM_MDISETMENU)
		GWMNcase(WM_ENTERSIZEMOVE)
		GWMNcase(WM_EXITSIZEMOVE)
		GWMNcase(WM_DROPFILES)
		GWMNcase(WM_MDIREFRESHMENU)
		GWMNcase(WM_IME_SETCONTEXT)
		GWMNcase(WM_IME_NOTIFY)
		GWMNcase(WM_IME_CONTROL)
		GWMNcase(WM_IME_COMPOSITIONFULL)
		GWMNcase(WM_IME_SELECT)
		GWMNcase(WM_IME_CHAR)
		case /*WM_IME_SYSTEM*/0x0287: return "WM_IME_SYSTEM";
		GWMNcase(WM_IME_REQUEST)
		GWMNcase(WM_IME_KEYDOWN)
		GWMNcase(WM_IME_KEYUP)
		GWMNcase(WM_MOUSEHOVER)
		GWMNcase(WM_MOUSELEAVE)
		GWMNcase(WM_NCMOUSEHOVER)
		GWMNcase(WM_NCMOUSELEAVE)
		GWMNcase(WM_CUT)
		GWMNcase(WM_COPY)
		GWMNcase(WM_PASTE)
		GWMNcase(WM_CLEAR)
		GWMNcase(WM_UNDO)
		GWMNcase(WM_RENDERFORMAT)
		GWMNcase(WM_RENDERALLFORMATS)
		GWMNcase(WM_DESTROYCLIPBOARD)
		GWMNcase(WM_DRAWCLIPBOARD)
		GWMNcase(WM_PAINTCLIPBOARD)
		GWMNcase(WM_VSCROLLCLIPBOARD)
		GWMNcase(WM_SIZECLIPBOARD)
		GWMNcase(WM_ASKCBFORMATNAME)
		GWMNcase(WM_CHANGECBCHAIN)
		GWMNcase(WM_HSCROLLCLIPBOARD)
		GWMNcase(WM_QUERYNEWPALETTE)
		GWMNcase(WM_PALETTEISCHANGING)
		GWMNcase(WM_PALETTECHANGED)
		GWMNcase(WM_HOTKEY)
		case /*WM_POPUPSYSTEMMENU*/0x0313: return "WM_POPUPSYSTEMMENU";
		GWMNcase(WM_PRINT)
		GWMNcase(WM_PRINTCLIENT)
		GWMNcase(WM_APPCOMMAND)
		case /*WM_CLIPBOARDUPDATE*/0x031D: return "WM_CLIPBOARDUPDATE";
		case /*WM_GETTITLEBARINFOEX*/0x033F: return "WM_GETTITLEBARINFOEX";
		GWMNcase(WM_APP)
#undef GWMNcase
		default:
			if(isMessageWhitelisted(message))
				return GetWindowsMessageName(toggleWhitelistMessage(message));
			if(message >= 0xC000 && message <= 0xFFFF)
			{
				static char temp [64];
				if(GetClipboardFormatNameA(message,temp,ARRAYSIZE(temp))) // hack, should call NtUserGetAtomName instead?
					return temp;
			}
			return "?";
	}
}


static MessageActionFlags GetMessageActionFlags(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: some messages that use MAF_BYPASSGAME (or maybe MAF_INTERCEPT)
	//       are probably needed by certain games (for them to run at all).
	//       there are two ways of fixing those cases:
	//   (1) upgrade it to (MAF_PASSTHROUGH | MAF_RETURN_OS)
	//   (2) call SendMessage with toggleWhitelistMessage(message) in a definitely deterministic place
	//       and downgrade the message action to MAF_INTERCEPT (so the os doesn't see it twice).
	//       number (2) is preferable because it's more desync-proof.

	// TODO: maybe some more messages above WM_USER need to be filtered, to prevent desyncs? especially if there's some way they can come from other applications or extra system dlls
//#pragma message("FIXMEEE")
//return MAF_PASSTHROUGH | MAF_RETURN_OS; // FIXME TEMP

//	if(message == toggleWhitelistMessage(WM_WINDOWPOSCHANGED-2))
//		return MAF_BYPASSGAME | MAF_RETURN_OS;

	if(isMessageWhitelisted(message)) // things sent with SendMessage toggleWhitelistMessage(message) are considered safe to send to the game.
		return MAF_PASSTHROUGH | MAF_RETURN_OS;

	if(tasflags.messageSyncMode == 3)
		return MAF_PASSTHROUGH | MAF_RETURN_OS;

	//if(message >= 0xC000 && message <= 0xFFFF)
	//	return MAF_PASSTHROUGH | MAF_RETURN_OS; // FIXME: TEMP HACK since windows won't (not sure about this though) deliver unregistered things in RegisterWindowMessage range, see msg.h for what needs to be fixed

	// we ignore input messages that don't have their bits inverted
	if(message >= WM_KEYFIRST && message <= WM_KEYLAST)
		return MAF_INTERCEPT | MAF_RETURN_1;

//	if(message >= toggleWhitelistMessage(WM_KEYFIRST) && message <= toggleWhitelistMessage(WM_KEYLAST))
//		return MAF_PASSTHROUGH | MAF_RETURN_OS;
	if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
		return MAF_INTERCEPT | MAF_RETURN_1;
//	if(message >= toggleWhitelistMessage(WM_MOUSEFIRST) && message <= toggleWhitelistMessage(WM_MOUSELAST))
//		return MAF_INTERCEPT | MAF_RETURN_1; // FIXME: TODO: mouse support is not implemented

//	if(message >= 0xC0F0 && message <= 0xC0FF) // not sure what this is but I'm blacklisting it for now
//		return MAF_INTERCEPT | MAF_RETURN_1;

	switch(message)
	{
	case WM_SIZE:
	//case WM_WINDOWPOSCHANGING:
	//case WM_WINDOWPOSCHANGED:
	//case WM_SYSCOMMAND: // TODO: at least block SC_SCREENSAVE and SC_MONITORPOWER ... but everything's blocked by default now so nevermind
	//case WM_NCCREATE:
	//case WM_NCDESTROY:
	//case WM_NCCALCSIZE:
	//case WM_NCHITTEST:
	//case WM_NCPAINT:
	//case WM_NCACTIVATE:
	//case WM_SYNCPAINT:
		return MAF_BYPASSGAME | MAF_RETURN_OS;

	//case WM_PAINT:
	//case WM_ERASEBKGND:
		//if(message == WM_PAINT || message == WM_NCPAINT || message == WM_ERASEBKGND)
		//	hwndRespondingToPaintMessage[hWnd] = false;
	case WM_TIMECHANGE:
	case WM_QUEUESYNC:
//	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
	case WM_ENTERIDLE:
//	case WM_TIMER:
		return MAF_INTERCEPT | MAF_RETURN_0;

	case /*WM_SYSTIMER*/0x0118:
		//return MAF_INTERCEPT | MAF_RETURN_1; // FIXME TODO not sure about this
		return MAF_BYPASSGAME | MAF_RETURN_OS; // still not sure

	case WM_NCMOUSEMOVE:
	case WM_SIZING:
		return MAF_BYPASSGAME | MAF_RETURN_CUSTOM;

	case WM_ERASEBKGND:
		return MAF_INTERCEPT | MAF_RETURN_CUSTOM;
	}

	if((message >= WM_IME_SETCONTEXT && message <= WM_IME_KEYUP)
	|| (message >= WM_IME_STARTCOMPOSITION && message <= WM_IME_KEYLAST))
	{
		return MAF_INTERCEPT | MAF_RETURN_1;
	}

	//if(!(tasflags.windowActivateFlags & 1))
	{
		if(message == WM_ACTIVATEAPP || message == WM_NCACTIVATE || message == WM_ACTIVATE)
		{
			if(wParam)
				return MAF_BYPASSGAME | MAF_RETURN_OS;
			else
				return MAF_INTERCEPT | MAF_RETURN_OS;
		}
		else if(message == WM_KILLFOCUS)
		{
			return MAF_INTERCEPT | MAF_RETURN_OS;
		}
//		else if((message == WM_NCACTIVATE || message == WM_ACTIVATE) //&& wParam == 0
//			|| message == WM_KILLFOCUS)
//		{
//			if(!(message == WM_ACTIVATE && LOWORD(wParam) == WA_ACTIVE))
//			{
//				return MAF_INTERCEPT | MAF_RETURN_OS;
//
////				if(message == WM_NCACTIVATE)
////					return MAF_INTERCEPT | MAF_RETURN_1; // activating the window can cause desync otherwise (TODO: find why) (IWBTG) // solved by adding a check for tls.callerisuntrusted in MyPostMessageA
//			}
//		}

		// disabled because it causes desync somehow (probably not anymore but it's obsolete code)
		//// after skipping the deactivate event we have to also skip activate events
		//// because some games freeze if they more activates in a row than they should
		//if((message == WM_NCACTIVATE || message == WM_ACTIVATE)
		//|| message == WM_SETFOCUS)
		//{
		//	if(hwndDeniedDeactivate[hWnd])
		//	{
		//		maf |= MAF_INTERCEPT | MAF_RETURN_1;
		//	}
		//}
	}

	//if(message == /*WM_INPUT*/)0x00FF
	//{
	//	// FIXME: TODO
	//	// see msdn documentation "About Raw Input"
	//}

	//if(message==WM_KILLFOCUS || message==WM_ACTIVATEAPP)
	//{
	//}

	if(message == WM_NCDESTROY)
	{
		if(tls.destroyWindowDepth)
		{
			// fix for Ninja Senki startup
			// (an alternate fix would be to make MyDestroyWindow always return TRUE,
			// but this way seems more correct and less likely to cause side effects)
			return MAF_PASSTHROUGH | MAF_RETURN_OS;
		}
	}

	{
		if(message > WM_NCXBUTTONDBLCLK && message < WM_KEYFIRST) // ???
		{
			return MAF_INTERCEPT | MAF_RETURN_0;
		}

		// these messages would bypass the game anyway,
		// so this check only affects the debug log now. ("CONSIDER:")
		if(message == WM_PAINT || message == WM_NCPAINT || message == WM_ERASEBKGND || message == WM_SETICON || message == 0xAE/*WM_NCUAHDRAWCAPTION*/ || message == 0xAF/*WM_NCUAHDRAWFRAME*/
		|| message == WM_GETICON || message == WM_DISPLAYCHANGE || message == WM_STYLECHANGED || message == WM_STYLECHANGING || message == WM_CONTEXTMENU
		|| message >= WM_SETREDRAW && message <= WM_GETTEXTLENGTH
		|| message >= WM_NCCREATE && message <= WM_NCXBUTTONDBLCLK
		|| message >= 0xB9/*???*/ && message <= 0xB9/*???*/
//		|| message >= WM_NCCREATE && message <= /*WM_INPUT*/0x00FF /*WM_NCXBUTTONDBLCLK*/
		|| message >= WM_IME_STARTCOMPOSITION && message <= WM_IME_KEYLAST
		|| message >= WM_DEVICECHANGE && message <= WM_NCMOUSELEAVE
		|| message == WM_POWER
		|| message == WM_WINDOWPOSCHANGING
		//|| message == WM_WINDOWPOSCHANGED
		)
		{
			return MAF_BYPASSGAME | MAF_RETURN_OS;
		}
	}

	//if((tasflags.threadMode == 2 || tasflags.threadMode == 3) /*&& !(GetAsyncKeyState('R') & 0x8000)*/)
//		if(message == WM_TIMER)
//			return MAF_BYPASSGAME | MAF_RETURN_OS;

	//if(message >= WM_USER)
	//{
	//	return MAF_PASSTHROUGH | MAF_RETURN_OS;
	//}

	switch(message)
	{
	case WM_ACTIVATE:
	case WM_SETFOCUS:
	case WM_WINDOWPOSCHANGED:
	case WM_CREATE:
	case WM_DISPLAYCHANGE:
		return MAF_INTERCEPT | MAF_RETURN_0; // already handled via inverted message send
	case WM_TIMER:
		if(tasflags.timersMode == 2)
		{
			debuglog(LCF_MESSAGES|LCF_FREQUENT|LCF_TIMEFUNC|LCF_DESYNC, "asynchronous 0x%X (%s), 0x%X, 0x%X\n", message, GetWindowsMessageName(message), wParam, lParam);
			return MAF_PASSTHROUGH | MAF_RETURN_OS;
		}
		return MAF_INTERCEPT | MAF_RETURN_0; // already handled via inverted message send
	case WM_MOVE:
	case WM_DESTROY:
	case WM_PARENTNOTIFY:
	case WM_GETMINMAXINFO:
	case WM_CANCELMODE:
	case WM_ENABLE:
	case WM_NOTIFYFORMAT:
	case WM_SETCURSOR:
	case WM_NOTIFY:
	case WM_SHOWWINDOW:
		break;//return MAF_INTERCEPT | MAF_RETURN_0; // maybe ok to ditch?
	case WM_SYSCOMMAND:
		return MAF_BYPASSGAME | MAF_RETURN_CUSTOM;
	case WM_COMMAND:
		if(VerifyIsTrustedCaller(!tls.callerisuntrusted))
			return MAF_PASSTHROUGH | MAF_RETURN_OS; // hack to fix F2 command in Eternal Daughter
		break;
	default:
		debuglog(LCF_MESSAGES|LCF_FREQUENT|LCF_TODO, "CONSIDER: 0x%X (%s), 0x%X, 0x%X\n", message, GetWindowsMessageName(message), wParam, lParam);
		//cmdprintf("SHORTTRACE: 3,50");
		break;
	case WM_NULL:
		return MAF_INTERCEPT | MAF_RETURN_0;
	}

	return MAF_BYPASSGAME | MAF_RETURN_OS;

} // GetMessageActionFlags

bool CanMessageReachGame(LPMSG msg)
{
	return msg && !(GetMessageActionFlags(msg->message, msg->wParam, msg->lParam) & (MAF_BYPASSGAME|MAF_INTERCEPT));
}
bool CanMessageReachGame(MessageActionFlags maf)
{
	return !(maf & (MAF_BYPASSGAME|MAF_INTERCEPT));
}

void FinalizeWndProcMessage(UINT& message, WPARAM& wParam, LPARAM& lParam)
{
//	if(message >= toggleWhitelistMessage(WM_KEYFIRST) && message <= toggleWhitelistMessage(WM_KEYLAST))
//	{
////		debuglog(LCF_MESSAGES|LCF_KEYBOARD, "inverted message 0x%X (%s), 0x%X, 0x%X\n", message, GetWindowsMessageName(message), wParam, lParam);
//		message = toggleWhitelistMessage(message);
//	}
//	if(message >= toggleWhitelistMessage(WM_MOUSEFIRST) && message <= toggleWhitelistMessage(WM_MOUSELAST))
//	{
////		debuglog(LCF_MESSAGES|LCF_MOUSE, "inverted message 0x%X (%s), 0x%X, 0x%X\n", message, GetWindowsMessageName(message), wParam, lParam);
//		message = toggleWhitelistMessage(message);
//	}
	if(isMessageWhitelisted(message))
	{
		message = toggleWhitelistMessage(message);
//		if(message == WM_WINDOWPOSCHANGED-2)
//			message = WM_WINDOWPOSCHANGED;
	}
}
void FinalizeWndProcMessage(LPMSG msg)
{
	if(msg)
		FinalizeWndProcMessage(msg->message, msg->wParam, msg->lParam);
}


LRESULT CustomHandleWndProcMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT defaultReturnValue)
{
	switch(message)
	{
	case WM_NCMOUSEMOVE:
		while(ShowCursor(1) < 0) {}
		break;

	case WM_ERASEBKGND:
		if(!RedrawScreen() && IsWindowFakeFullscreen(hWnd))
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
			RECT rect;
			GetClientRect(hWnd, &rect);
			FillRect(ps.hdc, &rect, black);
			EndPaint(hWnd, &ps);
			return 1;
		}
		break;

	case WM_SIZING:
		if(fakeDisplayValid && IsWindowFakeFullscreen(hWnd))
		{
			// make the fake-fullscreen window easy to reset to exactly 100% size

			LPRECT r = (LPRECT) lParam;
			RECT minRect = {0,0,fakeDisplayWidth,fakeDisplayHeight};
			RECT doubleRect = {0,0,fakeDisplayWidth*2,fakeDisplayHeight*2};
			AdjustWindowRect(&minRect, WS_OVERLAPPEDWINDOW, FALSE);
			AdjustWindowRect(&doubleRect, WS_OVERLAPPEDWINDOW, FALSE);
			int snapWidth = minRect.right - minRect.left;
			int snapHeight = minRect.bottom - minRect.top;
			int snapWidth2 = doubleRect.right - doubleRect.left;
			int snapHeight2 = doubleRect.bottom - doubleRect.top;

			bool snapX = false;
			bool snapY = false;

			if(abs((r->right - r->left) - snapWidth) < 28)
				snapX = true;
			else if(abs((r->right - r->left) - snapWidth2) < 28)
			{	snapX = true; snapWidth = snapWidth2; }
			if(abs((r->bottom - r->top) - snapHeight) < 28)
				snapY = true;
			else if(abs((r->bottom - r->top) - snapHeight2) < 28)
			{	snapY = true; snapHeight = snapHeight2; }

			if(snapX)
				if(wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
					r->left = r->right - snapWidth;
				else
					r->right = r->left + snapWidth;
			if(snapY)
				if(wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
					r->top = r->bottom - snapHeight;
				else
					r->bottom = r->top + snapHeight;
			return 1;
		}
		break;

	case WM_SYSCOMMAND:
		if((wParam & SC_CLOSE) == SC_CLOSE)
		{
			if(hWnd == gamehwnd)
			{
				// user clicked on the game winddow's close (X) button.
				// they probably expect that to stop the game, so let's ask the debugger to kill us.
				cmdprintf("KILLME: 0");
			}
		}
		break;
	}

	// TODO: re-implement some of these?
	{
		// TODO: some games need this to start up, have to find what's wrong with the code below that...
		//{
		//	WNDPROC origProc = hwndToOrigHandler[hWnd];
		//	verbosedebugprintf(__FUNCTION__ "hwndToOrigHandler[0x%X] == 0x%X.\n", hWnd, (DWORD)origProc);
		//	verbosedebugprintf(__FUNCTION__ "MyWndProc == 0x%X.\n", (DWORD)MyWndProc);
		//	if(origProc && !inPauseHandler)
		//	{
		//		LRESULT rv = origProc(hWnd, message, wParam, lParam);
		//		verbosedebugprintf("origProc returned 0x%X\n", rv);
		//		return rv;
		//	}
		//	else
		//	{
		//		LRESULT rv = DefWindowProc(hWnd, message, wParam, lParam);
		//		verbosedebugprintf("DefWindowProc returned 0x%X\n", rv);
		//		return rv;
		//	}
		//}

		//if(message == WM_PAINT || message == WM_NCPAINT || message == WM_ERASEBKGND || message == WM_SETICON)
		//{
		//	hwndRespondingToPaintMessage[hWnd] = true;
		//}

		//if(inPauseHandler && (message == WM_PAINT || message == WM_NCPAINT || message == WM_NCACTIVATE || message == WM_ACTIVATE || message == WM_ERASEBKGND))
		//{
		//	if(a_bltsaved)
		//	{
		//		RECT rect;
		//		GetClientRect(hWnd, (LPRECT)&rect);
		//		ClientToScreen(hWnd, (LPPOINT)&rect.left);
		//		ClientToScreen(hWnd, (LPPOINT)&rect.right);
		//		int width = a_bltsaved->right - a_bltsaved->left;
		//		int height = a_bltsaved->top - a_bltsaved->bottom;
		//		a_bltsaved->left = rect.left+1;
		//		a_bltsaved->top = rect.top+1;
		//		a_bltsaved->right = rect.left+1 + width;
		//		a_bltsaved->bottom = rect.top+1 + height;
		//	}

		//	RedrawScreen();
		//	ValidateRect(hWnd, NULL);

		//	return 1;
		//}

	//	hwndDeniedDeactivate[hWnd] = TRUE;
	}

	return defaultReturnValue;
}

HOOKFUNC LRESULT WINAPI MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
HOOKFUNC LRESULT WINAPI MyDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// MyWndProcInternal
LRESULT DispatchMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/)
{
	LONG untrusted = tls.callerisuntrusted;
	//untrusted = VerifyIsTrustedCaller(!untrusted) ? 0 : (untrusted ? untrusted : 1);
	if(/*inPauseHandler || */(untrusted  > (InSendMessage()?1:0))) // if it's the OS or pause handler calling us back,
	{                                 // we can't rely on it doing so consistently across systems,
		if(!(maf & (MAF_INTERCEPT|MAF_BYPASSGAME)))
		{
			maf |= MAF_BYPASSGAME; // so bypass the game and pass it straight back to the OS
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ " bypassing due to indefproc: 0x%X, 0x%X (%s), 0x%X, 0x%X\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
		}
	}

	// figure out which game WndProc function to call, if applicable
	WNDPROC origProc;
	if(!(maf & (MAF_INTERCEPT|MAF_BYPASSGAME)))
	{
		origProc = hwndToOrigHandler[hWnd];

		if(!origProc) // if there's no game WndProc to call
		{
			maf |= MAF_BYPASSGAME; // we're forced to bypass the game
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ " bypassing due to no origProc: 0x%X, 0x%X (%s), 0x%X, 0x%X\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
		}
	}

	LRESULT rv = 0;
	if(maf & MAF_INTERCEPT)
	{
		debuglog(LCF_MESSAGES|LCF_FREQUENT, "denied WndProc: 0x%X, 0x%X (%s), 0x%X, 0x%X\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
	}
	else if(maf & MAF_BYPASSGAME)
	{
		debuglog(LCF_MESSAGES|LCF_FREQUENT, "DefWndProc: 0x%X, 0x%X (%s), 0x%X, 0x%X\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
		rv = (ascii?MyDefWindowProcA:MyDefWindowProcW)(hWnd, message, wParam, lParam);
	}
	else // MAF_PASSTHROUGH
	{
		debuglog(LCF_MESSAGES|LCF_FREQUENT/*|LCF_DESYNC*/ /*|LCF_TODO*/, "WndProc: 0x%X, 0x%X (%s), 0x%X, 0x%X\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
		//cmdprintf("SHORTTRACE: 3,50");

		//rv = origProc(hWnd, message, wParam, lParam); // usually works, but according to MSDN origProc could be "a special internal value meaningful only to CallWindowProc" in some cases
		rv = (ascii?CallWindowProcA:CallWindowProcW)(origProc, hWnd, message, wParam, lParam);
	}

	if(maf & MAF_RETURN_CUSTOM)
	{
		return CustomHandleWndProcMessage(hWnd, message, wParam, lParam, rv);
	}
	else if(maf & MAF_RETURN_1)
	{
		return 1;
	}
	else if(maf & MAF_RETURN_0)
	{
		return 0;
	}
	else // MAF_RETURN_OS
	{
		return rv;
	}
}

LRESULT CALLBACK MyWndProcA(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MessageActionFlags maf = GetMessageActionFlags(message, wParam, lParam);
	FinalizeWndProcMessage(message, wParam, lParam);
	return DispatchMessageInternal(hWnd, message, wParam, lParam, true, maf);
}

LRESULT CALLBACK MyWndProcW(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MessageActionFlags maf = GetMessageActionFlags(message, wParam, lParam);
	FinalizeWndProcMessage(message, wParam, lParam);
	return DispatchMessageInternal(hWnd, message, wParam, lParam, false, maf);
}

HOOKFUNC LONG WINAPI MyGetWindowLongA(HWND hWnd, int nIndex);
HOOKFUNC LONG WINAPI MyGetWindowLongW(HWND hWnd, int nIndex);






HOOKFUNC LRESULT WINAPI MyCallNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam)
{
	//cmdprintf("SHORTTRACE: 3,50");
	LRESULT rv;
	if(nCode < 0) // negative supposedly means calling the hook is required... never seen that happen but I'll trust msdn for now
	{
		debuglog(LCF_MESSAGES|LCF_HOOK|LCF_UNTESTED, __FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X) called.\n", hhk, nCode, wParam, lParam);
		rv = CallNextHookEx(hhk, nCode, wParam, lParam);
	}
	else
	{
		//debuglog(LCF_MESSAGES|LCF_HOOK|LCF_FREQUENT, __FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X) called.\n", hhk, nCode, wParam, lParam);
		rv = 0;
	}
	return rv;
}

//HOOKFUNC BOOL WINAPI MyRegisterUserApiHook(HINSTANCE hInst, FARPROC func)
//{
//	//debugprintf(__FUNCTION__ " called.\n");
//	return 1;
//}

#ifdef EMULATE_MESSAGE_QUEUES

static void TrackMessageQueueStatusChange(MessageQueue& mq, DWORD message, int delta)
{
	if(delta)
	{
		switch(message)
		{
		case WM_TIMER:
			mq.timer += delta;
			if(mq.timer > 0) mq.queueStatus |= QS_TIMER;
			else             mq.queueStatus &= ~QS_TIMER;
			break;
		case WM_PAINT:
			if(delta > 0) mq.queueStatus |= QS_TIMER;
			else          mq.queueStatus &= ~QS_TIMER;
			break;
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			mq.key += delta;
			if(mq.key > 0) mq.queueStatus |= QS_KEY;
			else           mq.queueStatus &= ~QS_KEY;
			break;
		case WM_HOTKEY:
			mq.hotkey += delta;
			if(mq.hotkey > 0) mq.queueStatus |= QS_HOTKEY;
			else              mq.queueStatus &= ~QS_HOTKEY;
			break;
		case WM_MOUSEMOVE:
			mq.mousemove += delta;
			if(mq.mousemove > 0) mq.queueStatus |= QS_MOUSEMOVE;
			else                 mq.queueStatus &= ~QS_MOUSEMOVE;
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			mq.mousebutton += delta;
			if(mq.mousebutton > 0) mq.queueStatus |= QS_MOUSEBUTTON;
			else                   mq.queueStatus &= ~QS_MOUSEBUTTON;
			break;
		case /*WM_INPUT*/0x00FF:
		case /*WM_INPUT_DEVICE_CHANGE*/0x00FE:
			mq.rawinput += delta;
			if(mq.rawinput > 0) mq.queueStatus |= /*QS_RAWINPUT*/0x0400;
			else                mq.queueStatus &= ~/*QS_RAWINPUT*/0x0400;
			break;
		case WM_QUIT:
			if(delta > 0)
				mq.quit = true;
			else
				mq.quit = false;
			// no break
		default:
			if(delta > 0)
				mq.queueStatus |= QS_POSTMESSAGE | QS_ALLPOSTMESSAGE;
			break;
		// TODO: how to detect when to set QS_SENDMESSAGE? maybe won't do that, or need to implement incomingSentMessagesA/W
		}
	}
	if(delta <= 0)
		mq.queueStatusAtLastGet = mq.queueStatus;
}

static void AddMessageToQueue(MessageQueue& mq, MyMSG& mymsg)
{
	mq.messages.push_back(mymsg);
	MSG& msg = mymsg/*.msg*/;
	TrackMessageQueueStatusChange(mq, msg.message, 1);
	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": added message (0x%X, 0x%X (%s), 0x%X, 0x%X) to internal message queue.\n", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
}
//static void AddMessageToQueue(MessageQueue& mq, MSG& msg, bool ascii)
//{
//	MyMSG mymsg = {msg, ascii};
//	AddMessageToQueue(mq, mymsg);
//}

// TransferMessageQueue
static void InternalizeMessageQueue()
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;

	MessageQueue& mq = curtls.messageQueue;

	MSG msg;
	while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE|PM_NOYIELD))
	{
		if(CanMessageReachGame(&msg))
		{
			AddMessageToQueue(mq, msg/*, true*/);
		}
		else if(msg.hwnd)
		{
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X), but handling it internally.\n", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
			MyWndProcA(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		}
		else
		{
			debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ ": got message (0x%X (%s), 0x%X, 0x%X), but unable to handle it due to no HWND.\n", msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
		}
	}

	curtls.callerisuntrusted--;
}
#else
// temp hack, maybe doesn't help much anyway

struct PostedMessage
{
	HWND hWnd;
	UINT Msg;
	WPARAM wParam;
	LPARAM lParam;
};
std::vector<PostedMessage> postedMessages;
static CRITICAL_SECTION s_postedMessagesCS;
bool hasPostedMessages = false;

void HandlePostedMessages()
{
	if(!postedMessages.empty())
	{
		EnterCriticalSection(&s_postedMessagesCS);
		int size = postedMessages.size();
		for(int i = 0; i < size; i++)
		{
			PostedMessage pm = postedMessages[i];
			PostMessageA(pm.hWnd, pm.Msg, pm.wParam, pm.lParam);
		}
		postedMessages.clear();
		hasPostedMessages = false;
		LeaveCriticalSection(&s_postedMessagesCS);
	}
}

#endif

void PostMessageInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool ascii/*=true*/, struct MessageQueue* pmq/*=NULL*/, MessageActionFlags maf/*=MAF_PASSTHROUGH|MAF_RETURN_OS*/)
{
#ifdef EMULATE_MESSAGE_QUEUES
	if(CanMessageReachGame(maf))
	{
		POINT pt; MyGetCursorPos(&pt);
		//MyMSG mymsg = {hWnd, Msg, wParam, lParam, detTimer.GetTicks(), pt/*, ascii*/};
		MyMSG mymsg = {hWnd, message, wParam, lParam, detTimer.GetTicks(), pt/*, ascii*/}; // TEMP
		AddMessageToQueue(pmq?(*pmq):tls.messageQueue, mymsg);
	}
	else if(hWnd)
	{
		debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X), but handling it internally.\n", hWnd, message, GetWindowsMessageName(message), wParam, lParam);
		(ascii?MyWndProcA:MyWndProcW)(hWnd, message, wParam, lParam);
	}
	else
	{
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ ": got message (0x%X (%s), 0x%X, 0x%X), but unable to handle it due to no HWND.\n", message, GetWindowsMessageName(message), wParam, lParam);
	}
#else
	(ascii?PostMessageA:PostMessageW)(hWnd, isMessageWhitelisted(message) ? message : toggleWhitelistMessage(message), wParam, lParam);
#endif
}


HOOKFUNC BOOL WINAPI MySendNotifyMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, true, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO?
//		return SendNotifyMessageA(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	//if(tls.isFrameThread)
	if(tls_IsPrimaryThread())
	{
		LRESULT rv = SendNotifyMessageA(hWnd, whitelistUserMessage(Msg), wParam, lParam);
		return rv;
	}
	else
	{
		PostedMessage pm = {hWnd, whitelistUserMessage(Msg), wParam, lParam};
		EnterCriticalSection(&s_postedMessagesCS);
		postedMessages.push_back(pm);
		LeaveCriticalSection(&s_postedMessagesCS);
		return 1;
	}
#endif
}
HOOKFUNC BOOL WINAPI MySendNotifyMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, false, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO?
//		return SendNotifyMessageW(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	//if(tls.isFrameThread)
	if(tls_IsPrimaryThread())
	{
		LRESULT rv = SendNotifyMessageW(hWnd, whitelistUserMessage(Msg), wParam, lParam);
		return rv;
	}
	else
	{
		PostedMessage pm = {hWnd, whitelistUserMessage(Msg), wParam, lParam};
		EnterCriticalSection(&s_postedMessagesCS);
		postedMessages.push_back(pm);
		LeaveCriticalSection(&s_postedMessagesCS);
		return 1;
	}
#endif
}

HOOKFUNC LRESULT WINAPI MySendMessageTimeoutA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X, 0x%X, %d) called. (untested)\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam, fuFlags, uTimeout);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		if(lpdwResult)
			*lpdwResult = 1;
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		if(lpdwResult)
			*lpdwResult = 1;
		return 1;
	}
	if(hWnd == HWND_BROADCAST)
	{
		// broadcasts aren't supported
		SetLastError(ERROR_TIMEOUT);
		if(lpdwResult)
			*lpdwResult = 0;
		return 0;
	}
	if(tasflags.messageSyncMode < 2)
	{
		// force highest possible timeout if in a "synchronous-as-possible" mode
		uTimeout = 0x7fffffff;
		fuFlags &= ~SMTO_ABORTIFHUNG;
		fuFlags |= SMTO_BLOCK | SMTO_NOTIMEOUTIFNOTHUNG;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, true, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO?
//		return SendMessageTimeoutA(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	return SendMessageTimeoutA(hWnd, whitelistUserMessage(Msg), wParam, lParam, fuFlags, uTimeout, lpdwResult);
#endif
}
HOOKFUNC LRESULT WINAPI MySendMessageTimeoutW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X, 0x%X, %d) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam, fuFlags, uTimeout);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		if(lpdwResult)
			*lpdwResult = 1;
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		if(lpdwResult)
			*lpdwResult = 1;
		return 1;
	}
	if(hWnd == HWND_BROADCAST)
	{
		// broadcasts aren't supported
		SetLastError(ERROR_TIMEOUT);
		if(lpdwResult)
			*lpdwResult = 0;
		return 0;
	}
	if(tasflags.messageSyncMode < 2)
	{
		// force highest possible timeout if in a "synchronous-as-possible" mode
		uTimeout = 0x7fffffff;
		fuFlags &= ~SMTO_ABORTIFHUNG;
		fuFlags |= SMTO_BLOCK | SMTO_NOTIMEOUTIFNOTHUNG;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, false, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO?
//		return SendMessageTimeoutW(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	return SendMessageTimeoutW(hWnd, whitelistUserMessage(Msg), wParam, lParam, fuFlags, uTimeout, lpdwResult);
#endif
}
HOOKFUNC LRESULT WINAPI MySendMessageCallbackA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X, 0x%X, 0x%X) called. (untested)\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam, lpResultCallBack, dwData);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(hWnd == HWND_BROADCAST)
	{
		// broadcasts aren't supported
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		LRESULT lResult = DispatchMessageInternal(hWnd, Msg, wParam, lParam, true, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		if(lpResultCallBack)
			lpResultCallBack(hWnd, Msg, dwData, lResult);
		return 1;
	}
	//else if(tasflags.messageSyncMode < 2)
	//{
	//	// TODO?
	//	LRESULT lResult = SendMessageA(hWnd, Msg, wParam, lParam);
	//	if(lpResultCallBack)
	//		lpResultCallBack(hWnd, Msg, dwData, lResult);
	//	return 1;
	//}
	//else
	{
		// TODO?
//		return SendMessageCallbackA(hWnd, Msg, wParam, lParam, lpResultCallBack, dwData);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	if(tasflags.messageSyncMode >= 2)
	{
		return SendMessageCallbackA(hWnd, whitelistUserMessage(Msg), wParam, lParam, lpResultCallBack, dwData);
	}
	else
	{
		LRESULT lResult = SendMessageA(hWnd, whitelistUserMessage(Msg), wParam, lParam);
		if(lpResultCallBack)
			lpResultCallBack(hWnd, Msg, dwData, lResult);
		return 1;
	}
#endif
}
HOOKFUNC LRESULT WINAPI MySendMessageCallbackW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X, 0x%X, 0x%X) called. (untested)\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam, lpResultCallBack, dwData);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(hWnd == HWND_BROADCAST)
	{
		// broadcasts aren't supported
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		LRESULT lResult = DispatchMessageInternal(hWnd, Msg, wParam, lParam, false, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		if(lpResultCallBack)
			lpResultCallBack(hWnd, Msg, dwData, lResult);
		return 1;
	}
	//else if(tasflags.messageSyncMode < 2)
	//{
	//	// TODO?
	//	LRESULT lResult = SendMessageW(hWnd, Msg, wParam, lParam);
	//	if(lpResultCallBack)
	//		lpResultCallBack(hWnd, Msg, dwData, lResult);
	//	return 1;
	//}
	//else
	{
		// TODO?
//		return SendMessageCallbackW(hWnd, Msg, wParam, lParam, lpResultCallBack, dwData);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	if(tasflags.messageSyncMode >= 2)
	{
		return SendMessageCallbackW(hWnd, whitelistUserMessage(Msg), wParam, lParam, lpResultCallBack, dwData);
	}
	else
	{
		LRESULT lResult = SendMessageW(hWnd, whitelistUserMessage(Msg), wParam, lParam);
		if(lpResultCallBack)
			lpResultCallBack(hWnd, Msg, dwData, lResult);
		return 1;
	}
#endif
}


HOOKFUNC LRESULT WINAPI MySendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, false, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO
//		return SendMessageW(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	LRESULT rv = SendMessageW(hWnd, whitelistUserMessage(Msg), wParam, lParam);
	return rv;
#endif
}

HOOKFUNC LRESULT WINAPI MySendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
//	if((Msg >= toggleWhitelistMessage(WM_KEYFIRST) && Msg <= toggleWhitelistMessage(WM_KEYLAST))
//	|| (Msg >= toggleWhitelistMessage(WM_MOUSEFIRST) && Msg <= toggleWhitelistMessage(WM_MOUSELAST)))
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		return DispatchMessageInternal(hWnd, Msg, wParam, lParam, true, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
	}
	else
	{
		// TODO
//		return SendMessageA(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	LRESULT rv = SendMessageA(hWnd, whitelistUserMessage(Msg), wParam, lParam);
	return rv;
#endif
}

HOOKFUNC BOOL WINAPI MyPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		//cmdprintf("SHORTTRACE: 3,50");
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		PostMessageInternal(hWnd, Msg, wParam, lParam, true, &mq, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		return TRUE;
	}
	else
	{
		// TODO
//		return PostMessageW(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	//if(tls.isFrameThread)
	if(tls_IsPrimaryThread() || tasflags.messageSyncMode == 2)
	{
			tls.callerisuntrusted++;
		BOOL rv = PostMessageW(hWnd, whitelistUserMessage(Msg), wParam, lParam);
			tls.callerisuntrusted--;
		return rv;
	}
	else
	{
		PostedMessage pm = {hWnd, whitelistUserMessage(Msg), wParam, lParam};
		EnterCriticalSection(&s_postedMessagesCS);
		postedMessages.push_back(pm);
		hasPostedMessages = true;
		LeaveCriticalSection(&s_postedMessagesCS);
		return TRUE;
	}
#endif
}

HOOKFUNC BOOL WINAPI MyPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);

//#pragma message("FIXMEEE")
//	debugprintf(__FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	//if(Msg==WM_CLOSE)cmdprintf("DEBUGREDALERT: 0");//{return 1;}

	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	std::vector<HWND>& v = tls.messageQueue.attachedWindows;
	if(std::find(v.begin(), v.end(), hWnd) != v.end())
	{
		PostMessageInternal(hWnd, Msg, wParam, lParam, false, &mq, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		return TRUE;
	}
	else
	{
		// TODO
//		return PostMessageA(hWnd, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	BOOL rv;

	//if(tls.isFrameThread)
	if(tls_IsPrimaryThread() || tasflags.messageSyncMode >= 2)
	{
			tls.callerisuntrusted++;
	//	if(tasflags.threadMode == 2 || tasflags.threadMode == 3)
			rv = PostMessageA(hWnd, whitelistUserMessage(Msg), wParam, lParam);
	//	else
	//		rv = SendMessageA(hWnd, Msg, wParam, lParam);
			tls.callerisuntrusted--;
		return rv;
	}
	else
	{
		PostedMessage pm = {hWnd, whitelistUserMessage(Msg), wParam, lParam};
		EnterCriticalSection(&s_postedMessagesCS);
		postedMessages.push_back(pm);
		hasPostedMessages = true;
		LeaveCriticalSection(&s_postedMessagesCS);
		return TRUE;
	}
#endif
}


HOOKFUNC BOOL WINAPI MyPostThreadMessageA(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", idThread, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	//cmdprintf("SHORTTRACE: 3,50");
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(!tls_IsPrimaryThread() && tasflags.messageSyncMode < 2 && idThread != GetCurrentThreadId())
	{
		debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s), 0x%X) denied because we're not in the main thread.\n", Msg, GetWindowsMessageName(Msg), idThread);
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	if(idThread == GetCurrentThreadId())
	{
		PostMessageInternal(NULL, Msg, wParam, lParam, true, &mq, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		return TRUE;
	}
	else
	{
		// TODO
//		return PostThreadMessageA(idThread, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	tls.callerisuntrusted++;
	LRESULT rv = PostThreadMessageA(idThread, whitelistUserMessage(Msg), wParam, lParam);
	tls.callerisuntrusted--;
	return rv;
#endif
}
HOOKFUNC BOOL WINAPI MyPostThreadMessageW(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", idThread, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	//cmdprintf("SHORTTRACE: 3,50");
	if(isMessageWhitelisted(Msg))
	{
		debuglog(LCF_MESSAGES|LCF_TODO, __FUNCTION__ "(0x%X (%s)) denied because it's in a reserved range.\n", Msg, GetWindowsMessageName(Msg));
		return 1;
	}
	if(tls.callerisuntrusted)
	{
		debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s), 0x%X) denied because the caller is untrusted.\n", Msg, GetWindowsMessageName(Msg), idThread);
		return 1;
	}
	if(!tls_IsPrimaryThread() && tasflags.messageSyncMode < 2 && idThread != GetCurrentThreadId())
	{
		debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s), 0x%X) denied because we're not in the main thread.\n", Msg, GetWindowsMessageName(Msg), idThread);
		return 1;
	}
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	if(idThread == GetCurrentThreadId())
	{
		PostMessageInternal(NULL, Msg, wParam, lParam, false, &mq, GetMessageActionFlags(Msg,wParam,lParam)&~MAF_BYPASSGAME);
		return TRUE;
	}
	else
	{
		// TODO
//		return PostThreadMessageW(idThread, Msg, wParam, lParam);
		debuglog(LCF_MESSAGES|LCF_TODO|LCF_UNTESTED, __FUNCTION__ "(0x%X (%s)) denied because different-thread destination is NYI.\n", Msg, GetWindowsMessageName(Msg));
		return 0;
	}
#else
	tls.callerisuntrusted++;
	LRESULT rv = PostThreadMessageW(idThread, whitelistUserMessage(Msg), wParam, lParam);
	tls.callerisuntrusted--;
	return rv;
#endif
}

HOOKFUNC VOID WINAPI MyPostQuitMessage(int nExitCode)
{
	debuglog(LCF_PROCESS|LCF_ERROR, __FUNCTION__"(%d) called.", nExitCode);
	MyPostMessageA(NULL, WM_QUIT, nExitCode, 0);
}





HOOKFUNC BOOL WINAPI MyTranslateMessage(CONST MSG *lpMsg)
{
	MSG& msg = *const_cast<MSG*>(lpMsg);

	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", (DWORD)msg.hwnd, (DWORD)msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
	
	if(!gamehwnd)
		gamehwnd = msg.hwnd;

	//// left
	//MyTranslateMessage msg=0x100, lparam=0x 14D0001, wparam=0x27, hwnd=0x190464, pt=0x427, time=0x12A
	//MyTranslateMessage msg=0x101, lparam=0xC14D0001, wparam=0x27, hwnd=0x190464, pt=0x427, time=0x12A
	//// right
	//MyTranslateMessage msg=0x100, lparam=0x14B0001, wparam=0x25, hwnd=0x190464, pt=0x427, time=0x12A
	//MyTranslateMessage msg=0x101, lparam=0xC14B0001, wparam=0x25, hwnd=0x190464, pt=0x427, time=0x12A

	//ThreadBoundary(1);

	//BOOL rv = TranslateMessage(lpMsg);
	//return rv;

	// we already send WM_CHAR ourselves so there's no need to translate
	switch(msg.message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return TRUE;
	default:
		return FALSE;
	}
}

HOOKFUNC LRESULT WINAPI MyDispatchMessageA(CONST MSG *lpMsg)
{
	if(lpMsg) debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", (DWORD)lpMsg->hwnd, (DWORD)lpMsg->message, GetWindowsMessageName(lpMsg->message), lpMsg->wParam, lpMsg->lParam);
	//if(lpMsg && (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST))
	//	return 1;
#ifdef EMULATE_MESSAGE_QUEUES
	const MSG& msg = *lpMsg;
	//return MyWndProcA(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	return DispatchMessageInternal(msg.hwnd, msg.message, msg.wParam, msg.lParam, true);
#else
//	LRESULT rv = DispatchMessageA(lpMsg);
	const MSG& msg = *lpMsg;
	LRESULT rv = DispatchMessageInternal(msg.hwnd, msg.message, msg.wParam, msg.lParam, true, MAF_PASSTHROUGH|MAF_RETURN_OS);
	return rv;
#endif
}
HOOKFUNC LRESULT WINAPI MyDispatchMessageW(CONST MSG *lpMsg)
{
	if(lpMsg) debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", (DWORD)lpMsg->hwnd, (DWORD)lpMsg->message, GetWindowsMessageName(lpMsg->message), lpMsg->wParam, lpMsg->lParam);
	//if(lpMsg && (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST))
	//	return 1;
#ifdef EMULATE_MESSAGE_QUEUES
	const MSG& msg = *lpMsg;
	//return MyWndProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	return DispatchMessageInternal(msg.hwnd, msg.message, msg.wParam, msg.lParam, false);
#else
//	LRESULT rv = DispatchMessageW(lpMsg);
	const MSG& msg = *lpMsg;
	LRESULT rv = DispatchMessageInternal(msg.hwnd, msg.message, msg.wParam, msg.lParam, false, MAF_PASSTHROUGH|MAF_RETURN_OS);
	return rv;
#endif
}


HOOKFUNC BOOL WINAPI MyGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
//	verbosedebugprintf(__FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
#ifdef EMULATE_MESSAGE_QUEUES
	if(!lpMsg)
		return -1;
	bool firstWait = true;
	MessageQueue& mq = tls.messageQueue;
	while(true)
	{
		InternalizeMessageQueue();
		mq.queueStatus &= ~QS_POSTMESSAGE;
		if(!wMsgFilterMin && !wMsgFilterMax)
			mq.queueStatus &= ~QS_ALLPOSTMESSAGE;
		std::vector<MyMSG>::iterator iter;
		for(iter = mq.messages.begin(); iter != mq.messages.end(); iter++)
		{
			MyMSG& mymsg = *iter;
			MSG& msg = mymsg/*.msg*/;
			if(!hWnd || hWnd == msg.hwnd || (hWnd == (HWND)-1 && !msg.hwnd))
			{
				if((!wMsgFilterMin && !wMsgFilterMax) || (msg.message >= wMsgFilterMin && msg.message <= wMsgFilterMax))
				{
					*lpMsg = msg;
					debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X) from internal message queue.\n", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
					// TODO: convert ascii<->unicode depending on send function and IsWindowUnicode results?
					mq.messages.erase(iter);
					TrackMessageQueueStatusChange(mq, msg.message, -1);
					return TRUE;
				}
			}
		}
		if(mq.quit)
		{
			lpMsg->message = WM_QUIT;
			TrackMessageQueueStatusChange(mq, WM_QUIT, -1);
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": WM_QUIT.\n");
			return FALSE;
		}
		Sleep(5); // TEMP (should probably wait on a custom event instead)
		if(firstWait)
		{
			firstWait = false;
			debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " started waiting.\n");
		}
	}
	if(!firstWait)
		debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " finished waiting.\n");
#else
	while(true)
	{
		//if(tasflags.threadMode == 3)
		//	cmdprintf("WAITING: %u", 0);
		BOOL rv = FALSE;
//		if(tasflags.threadMode == 2 /*|| tasflags.threadMode == 3*/ /*|| !tls_IsPrimaryThread()*/)
		if(tasflags.framerate <= 0 && tasflags.messageSyncMode != 0)
		{
			tls.callerisuntrusted++;
			rv = GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
			tls.callerisuntrusted--;
		}
		else
		{
//			while(!rv /*|| !CanMessageReachGame(lpMsg)*/)
			{
				tls.callerisuntrusted++;
				rv = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE);
				tls.callerisuntrusted--;
			}
		}
		debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X), rv=0x%X.\n", lpMsg->hwnd, lpMsg->message, GetWindowsMessageName(lpMsg->message), lpMsg->wParam, lpMsg->lParam, rv);
		//if(tasflags.threadMode == 3)
		//	cmdprintf("WAITED: %u", 0);
		//if(!rv || (CanMessageReachGame(lpMsg)))
		if(rv && (CanMessageReachGame(lpMsg)))
		{
			//if(!inPauseHandler)
			if(tls_IsPrimaryThread() && VerifyIsTrustedCaller(!tls.callerisuntrusted))
				tls.peekedMessage = TRUE;
			FinalizeWndProcMessage(lpMsg);
			return rv;
		}
		else
		{
			if(rv)
			{
				debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X), but handling it internally.\n", lpMsg->hwnd, lpMsg->message, GetWindowsMessageName(lpMsg->message), lpMsg->wParam, lpMsg->lParam);
				FinalizeWndProcMessage(lpMsg);
				MyWndProcA(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": continuing to wait.\n");
			}
			else if(!wMsgFilterMin)
			{
				// REALLY UGLY HACK FOR LYLE IN CUBE SECTOR
				// TODO: figure out a better way to make that game neither desync nor deadlock
				MSG tmp = {}; // WM_NULL
				*lpMsg = tmp;
				return TRUE;
			}
		}
	}
#endif
}
HOOKFUNC BOOL WINAPI MyGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
//	verbosedebugprintf(__FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
#ifdef EMULATE_MESSAGE_QUEUES
	if(!lpMsg)
		return -1;
	bool firstWait = true;
	MessageQueue& mq = tls.messageQueue;
	while(true)
	{
		InternalizeMessageQueue();
		mq.queueStatus &= ~QS_POSTMESSAGE;
		if(!wMsgFilterMin && !wMsgFilterMax)
			mq.queueStatus &= ~QS_ALLPOSTMESSAGE;
		std::vector<MyMSG>::iterator iter;
		for(iter = mq.messages.begin(); iter != mq.messages.end(); iter++)
		{
			MyMSG& mymsg = *iter;
			MSG& msg = mymsg/*.msg*/;
			if(!hWnd || hWnd == msg.hwnd || (hWnd == (HWND)-1 && !msg.hwnd))
			{
				if((!wMsgFilterMin && !wMsgFilterMax) || (msg.message >= wMsgFilterMin && msg.message <= wMsgFilterMax))
				{
					*lpMsg = msg;
					debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X) from internal message queue.\n", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
					// TODO: convert ascii<->unicode depending on send function and IsWindowUnicode results?
					mq.messages.erase(iter);
					TrackMessageQueueStatusChange(mq, msg.message, -1);
					return TRUE;
				}
			}
		}
		if(mq.quit)
		{
			lpMsg->message = WM_QUIT;
			TrackMessageQueueStatusChange(mq, WM_QUIT, -1);
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": WM_QUIT.\n");
			return FALSE;
		}
		Sleep(5); // TEMP (should probably wait on a custom event instead)
		if(firstWait)
		{
			firstWait = false;
			debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " started waiting.\n");
		}
	}
	if(!firstWait)
		debuglog(LCF_MESSAGES|LCF_WAIT, __FUNCTION__ " finished waiting.\n");
#else
	while(true)
	{
		//if(tasflags.threadMode == 3)
		//	cmdprintf("WAITING: %u", 0);
//		BOOL rv = GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
		BOOL rv = FALSE;
		if(tasflags.messageSyncMode >= 2 || (!tls_IsPrimaryThread() && tasflags.messageSyncMode != 0))
		{
			tls.callerisuntrusted++;
			rv = GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
			tls.callerisuntrusted--;
		}
		else
		{
			//while(!rv /*|| !CanMessageReachGame(lpMsg)*/)
			{
				tls.callerisuntrusted++;
				rv = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE);
				tls.callerisuntrusted--;
				//if(!rv)
				//	MySleep(1);
			}
		}
		//if(tasflags.threadMode == 3)
		//	cmdprintf("WAITED: %u", 0);
		if(!rv || (CanMessageReachGame(lpMsg)))
		{
			//if(!inPauseHandler)
			if(tls_IsPrimaryThread() && VerifyIsTrustedCaller(!tls.callerisuntrusted))
				tls.peekedMessage = TRUE;
			FinalizeWndProcMessage(lpMsg);
			return rv;
		}
		else
		{
			debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": got message (0x%X, 0x%X (%s), 0x%X, 0x%X), but handling it internally.\n", lpMsg->hwnd, lpMsg->message, GetWindowsMessageName(lpMsg->message), lpMsg->wParam, lpMsg->lParam);
			FinalizeWndProcMessage(lpMsg);
			MyWndProcW(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
			debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": continuing to wait.\n");
		}
	}
//	if(tasflags.threadMode == 3)
//		cmdprintf("WAITING: %u", 0);
//	BOOL rv = GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
//	if(tasflags.threadMode == 3)
//		cmdprintf("WAITED: %u", 0);
//	if(!inPauseHandler)
//		tls.peekedMessage = TRUE;
//	return rv;
#endif
}
HOOKFUNC BOOL WINAPI MyPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
//	verbosedebugprintf(__FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
////	FrameBoundary();
	debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	if(!lpMsg)
		return FALSE;
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	{
		InternalizeMessageQueue();
		mq.queueStatus &= ~QS_POSTMESSAGE;
		if(!wMsgFilterMin && !wMsgFilterMax)
			mq.queueStatus &= ~QS_ALLPOSTMESSAGE;
		std::vector<MyMSG>::iterator iter;
		for(iter = mq.messages.begin(); iter != mq.messages.end(); iter++)
		{
			MyMSG& mymsg = *iter;
			MSG& msg = mymsg/*.msg*/;
			if(!hWnd || hWnd == msg.hwnd || (hWnd == (HWND)-1 && !msg.hwnd))
			{
				if((!wMsgFilterMin && !wMsgFilterMax) || (msg.message >= wMsgFilterMin && msg.message <= wMsgFilterMax))
				{
					*lpMsg = msg;
					debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": %s message (0x%X, 0x%X (%s), 0x%X, 0x%X) from internal message queue.\n", (wRemoveMsg & PM_REMOVE) ? "pulled" : "peeked", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
					// TODO: convert ascii<->unicode depending on send function and IsWindowUnicode results?
					if(wRemoveMsg & PM_REMOVE)
						mq.messages.erase(iter);
					TrackMessageQueueStatusChange(mq, msg.message, (wRemoveMsg & PM_REMOVE) ? -1 : 0);
					return TRUE;
				}
			}
		}
		if(mq.quit)
		{
			lpMsg->message = WM_QUIT;
			TrackMessageQueueStatusChange(mq, WM_QUIT, (wRemoveMsg & PM_REMOVE) ? -1 : 0);
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": WM_QUIT.\n");
			return TRUE;
		}
	}
	
	if(tasflags.messageSyncMode != 0)
		detTimer.GetTicks(TIMETYPE_CRAWLHACK); // potentially desync prone (but some games will need it) ... moving it here (on no-result) helped sync a bit though... and the problem that happens here is usually caused by GetMessageActionFlags being incomplete

	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": returned empty-handed.\n");
	return FALSE;
#else	
	// TODO: maybe should do wRemoveMsg |= PM_NOYIELD; here?
	wRemoveMsg |= PM_NOYIELD; // testing
	while(true)
	{
		tls.callerisuntrusted++;
		BOOL rv = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
		tls.callerisuntrusted--;

		if(!rv || (CanMessageReachGame(lpMsg)))
		{
			if(rv)
				debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ " got 0x%X (%s), 0x%X (%d), 0x%X (%d).\n", lpMsg ? lpMsg->message : 0, GetWindowsMessageName(lpMsg ? lpMsg->message : 0), lpMsg ? lpMsg->wParam : 0, lpMsg ? lpMsg->wParam : 0, lpMsg ? lpMsg->lParam : 0, lpMsg ? lpMsg->lParam : 0);
			else if(tasflags.messageSyncMode != 0)
				detTimer.GetTicks(TIMETYPE_CRAWLHACK); // potentially desync prone (but some games will need it) ... moving it here (on no-result) helped sync a bit though... and the problem that happens here is usually caused by GetMessageActionFlags being incomplete
			//if(!inPauseHandler)
			{
				if(tls_IsPrimaryThread() && VerifyIsTrustedCaller(!tls.callerisuntrusted))
					tls.peekedMessage = TRUE;
//				ThreadBoundary(100);
			}
			FinalizeWndProcMessage(lpMsg);
			return rv;
		}
		else
		{
			int prevmsg = lpMsg->message;
			FinalizeWndProcMessage(lpMsg);
			if((wRemoveMsg & PM_REMOVE) || PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE|PM_NOYIELD))
				MyWndProcA(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
			else
				debuglog(LCF_MESSAGES|LCF_WAIT|LCF_ERROR, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) failed?\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
			debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": possibly continuing to wait. (got 0x%X (%s), removed 0x%X (%s))\n", prevmsg, GetWindowsMessageName(prevmsg), lpMsg ? lpMsg->message : 0, GetWindowsMessageName(lpMsg ? lpMsg->message : 0));
			//verbosedebugprintf(__FUNCTION__"(0x%X, (0x%X -> 0x%X), 0x%X, 0x%X) looped.\n", hWnd, prevmsg, lpMsg ? lpMsg->message : 0, wMsgFilterMin, wMsgFilterMax);
		}
	}
//	return rv;
//	BOOL rv = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
//	if(!inPauseHandler)
//	{
//		tls.peekedMessage = TRUE;
//		ThreadBoundary(100);
//	}
//	return rv;
#endif
}
HOOKFUNC BOOL WINAPI MyPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
//	verbosedebugprintf(__FUNCTION__"(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) called.\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	if(!lpMsg)
		return FALSE;
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	{
		InternalizeMessageQueue();
		mq.queueStatus &= ~QS_POSTMESSAGE;
		if(!wMsgFilterMin && !wMsgFilterMax)
			mq.queueStatus &= ~QS_ALLPOSTMESSAGE;
		mq.queueStatusAtLastGet = mq.queueStatus;
		std::vector<MyMSG>::iterator iter;
		for(iter = mq.messages.begin(); iter != mq.messages.end(); iter++)
		{
			MyMSG& mymsg = *iter;
			MSG& msg = mymsg/*.msg*/;
			if(!hWnd || hWnd == msg.hwnd || (hWnd == (HWND)-1 && !msg.hwnd))
			{
				if((!wMsgFilterMin && !wMsgFilterMax) || (msg.message >= wMsgFilterMin && msg.message <= wMsgFilterMax))
				{
					*lpMsg = msg;
					debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": %s message (0x%X, 0x%X (%s), 0x%X, 0x%X) from internal message queue.\n", (wRemoveMsg & PM_REMOVE) ? "pulled" : "peeked", msg.hwnd, msg.message, GetWindowsMessageName(msg.message), msg.wParam, msg.lParam);
					// TODO: convert ascii<->unicode depending on send function and IsWindowUnicode results?
					if(wRemoveMsg & PM_REMOVE)
						mq.messages.erase(iter);
					TrackMessageQueueStatusChange(mq, msg.message, (wRemoveMsg & PM_REMOVE) ? -1 : 0);
					return TRUE;
				}
			}
		}
		if(mq.quit)
		{
			lpMsg->message = WM_QUIT;
			TrackMessageQueueStatusChange(mq, WM_QUIT, (wRemoveMsg & PM_REMOVE) ? -1 : 0);
			debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": WM_QUIT.\n");
			return TRUE;
		}
	}
	
	if(tasflags.messageSyncMode != 0)
		detTimer.GetTicks(TIMETYPE_CRAWLHACK); // potentially desync prone (but some games will need it) ... moving it here (on no-result) helped sync a bit though... and the problem that happens here is usually caused by GetMessageActionFlags being incomplete

	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ ": returned empty-handed.\n");
	return FALSE;
#else	
	// TODO: maybe should do wRemoveMsg |= PM_NOYIELD; here?
	wRemoveMsg |= PM_NOYIELD; // testing
	while(true)
	{
		tls.callerisuntrusted++;
		BOOL rv = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
		tls.callerisuntrusted--;

		if(!rv || (CanMessageReachGame(lpMsg)))
		{
			if(rv)
				debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ " got 0x%X (%s), 0x%X (%d), 0x%X (%d).\n", lpMsg ? lpMsg->message : 0, GetWindowsMessageName(lpMsg ? lpMsg->message : 0), lpMsg ? lpMsg->wParam : 0, lpMsg ? lpMsg->wParam : 0, lpMsg ? lpMsg->lParam : 0, lpMsg ? lpMsg->lParam : 0);
			else if(tasflags.messageSyncMode != 0)
				detTimer.GetTicks(TIMETYPE_CRAWLHACK); // potentially desync prone (but some games will need it) ... moving it here (on no-result) helped sync a bit though... and the problem that happens here is usually caused by GetMessageActionFlags being incomplete
			//if(!inPauseHandler)
			{
				if(tls_IsPrimaryThread() && VerifyIsTrustedCaller(!tls.callerisuntrusted))
					tls.peekedMessage = TRUE;
//				ThreadBoundary(100);
			}
			FinalizeWndProcMessage(lpMsg);
			return rv;
		}
		else
		{
			int prevmsg = lpMsg->message;
			FinalizeWndProcMessage(lpMsg);
			if((wRemoveMsg & PM_REMOVE) || PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE|PM_NOYIELD))
				MyWndProcW(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
			else
				debuglog(LCF_MESSAGES|LCF_WAIT|LCF_ERROR, __FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X, 0x%X) failed?\n", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
			debuglog(LCF_MESSAGES|LCF_WAIT|LCF_FREQUENT, __FUNCTION__ ": continuing to wait. (0x%X (%s) -> 0x%X (%s))\n", prevmsg, GetWindowsMessageName(prevmsg), lpMsg ? lpMsg->message : 0, GetWindowsMessageName(lpMsg ? lpMsg->message : 0));
			//verbosedebugprintf(__FUNCTION__"(0x%X, (0x%X -> 0x%X), 0x%X, 0x%X) looped.\n", hWnd, prevmsg, lpMsg ? lpMsg->message : 0, wMsgFilterMin, wMsgFilterMax);
		}
	}
//	BOOL rv = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
//	if(!inPauseHandler)
//	{
//		tls.peekedMessage = TRUE;
////		ThreadBoundary(1);
//	}
//	return rv;
#endif
}

HOOKFUNC DWORD WINAPI MyGetQueueStatus(UINT flags)
{
	debuglog(LCF_MESSAGES, __FUNCTION__ "(0x%X) called.\n", flags);
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	DWORD rv = ((DWORD)(mq.queueStatus & flags) << 16) | (mq.queueStatusAtLastGet & mq.queueStatus & flags);
	TrackMessageQueueStatusChange(mq, WM_NULL, 0);
	return rv;
#else
	//DWORD rv = GetQueueStatus(flags);
	//debuglog(LCF_MESSAGES|LCF_UNTESTED|LCF_DESYNC|LCF_TODO, __FUNCTION__ "(0x%X) called. returned 0x%X (untested)\n", flags, rv);
	//return rv;
	return (flags & 0xFFFF) | (flags << 16); // I'm not sure what else I can do that's deterministic... (TODO: if the windows message queue is emulated then this can be made more accurate)
#endif
}
HOOKFUNC BOOL WINAPI MyGetInputState()
{
#ifdef EMULATE_MESSAGE_QUEUES
	DWORD state = MyGetQueueStatus(QS_KEY|QS_MOUSEBUTTON|QS_HOTKEY);
	return ((state >> 16) & ~state) ? 1 : 0;
#else
//	BOOL rv = GetInputState(); // maybe this is ok? not sure.
	BOOL rv = 0;
	debuglog(LCF_KEYBOARD|LCF_TODO|LCF_FREQUENT, __FUNCTION__ " called. returned %d\n", rv);
	return rv;
#endif
}























HOOKFUNC LRESULT WINAPI MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	tls.callerisuntrusted++;
	LRESULT rv = DefWindowProcA(hWnd, Msg, wParam, lParam);
	tls.callerisuntrusted--;
	return rv;
}
HOOKFUNC LRESULT WINAPI MyDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	debuglog(LCF_MESSAGES|LCF_FREQUENT, __FUNCTION__ "(0x%X, 0x%X (%s), 0x%X, 0x%X) called.\n", hWnd, Msg, GetWindowsMessageName(Msg), wParam, lParam);
	tls.callerisuntrusted++;
	LRESULT rv = DefWindowProcW(hWnd, Msg, wParam, lParam);
	tls.callerisuntrusted--;
	return rv;
}


HOOKFUNC UINT WINAPI MyRegisterWindowMessageA(LPCSTR lpString)
{
//cmdprintf("SHORTTRACE: 3,50");
	UINT rv = RegisterWindowMessageA(lpString);
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(\"%s\") called. returned 0x%X, reserved=%d\n", lpString, rv, isMessageWhitelisted(rv));
	return rv;
}
HOOKFUNC UINT WINAPI MyRegisterWindowMessageW(LPCWSTR lpString)
{
//cmdprintf("SHORTTRACE: 3,50");
	UINT rv = RegisterWindowMessageW(lpString);
	debuglog(LCF_MESSAGES|LCF_UNTESTED, __FUNCTION__ "(\"%S\") called. returned 0x%X, reserved=%d\n", lpString, rv, isMessageWhitelisted(rv));
	return rv;
}


HOOKFUNC LONG WINAPI MyGetMessageTime(VOID)
{
	//return GetMessageTime();
	debuglog(LCF_MESSAGES|LCF_TIMEFUNC|LCF_TIMEGET, __FUNCTION__ " called.\n");
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	return mq.lastGot.time;
#else
	return detTimer.GetTicks();
#endif
}

HOOKFUNC DWORD WINAPI MyGetMessagePos(VOID)
{
	//return GetMessagePos();
	debuglog(LCF_MESSAGES|LCF_MOUSE|LCF_UNTESTED, __FUNCTION__ " called.\n");
#ifdef EMULATE_MESSAGE_QUEUES
	MessageQueue& mq = tls.messageQueue;
	return (mq.lastGot.pt.x & 0xFFFF) | ((mq.lastGot.pt.y & 0xFFFF) << 16);
#else
	return GetMessagePos(); 
#endif
}


// broadcast a message to all windows that were created by the current application that we know of.
// if send is true, sends instead of posts to probably-top-level windows.
void FakeBroadcastMessage(bool send, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	std::map<HWND, WNDPROC>::iterator iter;
	for(iter = hwndToOrigHandler.begin(); iter != hwndToOrigHandler.end();)
	{
		HWND hwnd = iter->first;
		iter++;
		if(!IsWindow(hwnd))
			continue;
		DWORD style = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
		if(send && ((hwnd==gamehwnd && (style&WS_VISIBLE))
		 || ((style & (WS_VISIBLE|WS_CAPTION)) == (WS_VISIBLE|WS_CAPTION))
		 || ((style & (WS_VISIBLE|WS_POPUP)) == (WS_VISIBLE|WS_POPUP))))
		{
			DispatchMessageInternal(hwnd, Msg, wParam, lParam, true, MAF_PASSTHROUGH|MAF_RETURN_OS);
		}
		else
		{
			PostMessageInternal(hwnd, Msg, wParam, lParam, true, NULL, MAF_PASSTHROUGH|MAF_RETURN_OS);
		}
	}
}

void FakeBroadcastDisplayChange(int width, int height, int depth)
{
	FakeBroadcastMessage(true, WM_DISPLAYCHANGE, depth, (width&0xFFFF)|((height&0xFFFF)<<16));
}



void MessageDllMainInit()
{
	InitializeCriticalSection(&s_postedMessagesCS);
}

void ApplyMessageIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, USER32, TranslateMessage),
		MAKE_INTERCEPT(1, USER32, GetMessageA),
		MAKE_INTERCEPT(1, USER32, GetMessageW),
		MAKE_INTERCEPT(1, USER32, PeekMessageA),
		MAKE_INTERCEPT(1, USER32, PeekMessageW),
		MAKE_INTERCEPT(1, USER32, GetQueueStatus),
		MAKE_INTERCEPT(1, USER32, GetInputState),
		MAKE_INTERCEPT(1, USER32, GetMessagePos),
		MAKE_INTERCEPT(1, USER32, GetMessageTime),
		MAKE_INTERCEPT(1, USER32, DispatchMessageA),
		MAKE_INTERCEPT(1, USER32, DispatchMessageW),
		MAKE_INTERCEPT(1, USER32, SendMessageA),
		MAKE_INTERCEPT(1, USER32, SendMessageW),
		MAKE_INTERCEPT(1, USER32, PostMessageA),
		MAKE_INTERCEPT(1, USER32, PostMessageW),
		MAKE_INTERCEPT(1, USER32, SendNotifyMessageA),
		MAKE_INTERCEPT(1, USER32, SendNotifyMessageW),
		MAKE_INTERCEPT(1, USER32, SendMessageTimeoutA),
		MAKE_INTERCEPT(1, USER32, SendMessageTimeoutW),
		MAKE_INTERCEPT(1, USER32, SendMessageCallbackA),
		MAKE_INTERCEPT(1, USER32, SendMessageCallbackW),
		MAKE_INTERCEPT(1, USER32, PostThreadMessageA),
		MAKE_INTERCEPT(1, USER32, PostThreadMessageW),
		MAKE_INTERCEPT(1, USER32, PostQuitMessage),

		MAKE_INTERCEPT(1, USER32, DefWindowProcA),
		MAKE_INTERCEPT(1, USER32, DefWindowProcW),

		MAKE_INTERCEPT(1, USER32, RegisterWindowMessageA),
		MAKE_INTERCEPT(1, USER32, RegisterWindowMessageW),
		MAKE_INTERCEPT(1, USER32, CallNextHookEx),
		//MAKE_INTERCEPT(1, USER32, RegisterUserApiHook),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
