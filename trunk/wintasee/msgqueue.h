/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef MSGQUEUE_H_INCL
#define MSGQUEUE_H_INCL

#include "../shared/msg.h"

typedef int MessageActionFlags; enum
{
	// one of these... (what to call)
	MAF_PASSTHROUGH    = 0x00, // call game's WndProc
	MAF_BYPASSGAME     = 0x01, // call OS's DefWndProc only (overrides MAF_PASSTHROUGH if combined with it)
	MAF_INTERCEPT      = 0x02, // call nothing (overrides MAF_BYPASSGAME and/or MAF_PASSTHROUGH if combined with them)

	// combined with one of these... (what to return)
	MAF_RETURN_OS      = 0x00, // OS or game return value (or 0 if MAF_INTERCEPT is set)
	MAF_RETURN_0       = 0x10, // return 0 (overrides MAF_RETURN_OS if combined with it)
	MAF_RETURN_1       = 0x20, // return 1 (overrides MAF_RETURN_OS and/or MAF_RETURN_0 if combined with them)
	MAF_RETURN_CUSTOM  = 0x40, // do custom extra action and return a custom value (overrides MAF_RETURN_0 and MAF_RETURN_1 and MAF_RETURN_OS if combined with any or all of them)
};

#ifdef EMULATE_MESSAGE_QUEUES

//struct MyMSG
//{
//	MSG msg;
//	bool ascii;
//};
#define MyMSG MSG // TODO: use IsWindowUnicode?

//#include <set>

// FIXME/TODO: use this and replace (or at least filter) all OS-generated windows messages
// also todo: wrap all filesystem and registry access. for savestates if nothing else.
struct MessageQueue
{
	static const int MAX_MESSAGES = 10000; // as per default USERPostMessageLimit
	std::vector<MyMSG> messages;
	WORD queueStatus; // e.g. QS_KEY. indicates which sorts of new messages are in the queue. for GetQueueStatus and MsgWaitForMultipleObjects[Ex].
	WORD queueStatusAtLastGet;
	std::vector<HWND> attachedWindows;
	MSG lastGot;
	// queueStatus tracking
	bool quit;
	int timer;
	int key;
	int hotkey;
	int mousemove;
	int mousebutton;
	int rawinput;
	//std::vector<MSG> incomingSentMessagesA, incomingSentMessagesW;
};

// TODO: note to self about unusual cases to handle (implement) that I might forget about:
// SendMessageTimeout SendMessageCallback SendNotifyMessage ReplyMessage PostThreadMessage AttachThreadInput IsDialogMessage RegisterWindowMessage GetMessageTime GetMessagePos GetQueueStatus WaitMessage SendDlgItemMessage BroadcastSystemMessage InSendMessage InSendMessageEx
// read and follow closely: "About Messages and Message Queues" h ttp://msdn.microsoft.com/en-us/library/ms644927%28v=VS.85%29.aspx

//static std::map<DWORD,MessageQueue*> threadIdToMessageQueueMap;
//static std::map<HWND,MessageQueue*> hwndToMessageQueueMap;
//GetCurrentThreadId()

void InternalizeMessageQueue();

#endif

#endif
