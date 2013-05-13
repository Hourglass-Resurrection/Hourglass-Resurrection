/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef MSG_H_INCL
#define MSG_H_INCL

// FIXME: should get things working with EMULATE_MESSAGE_QUEUES enabled, currently it breaks Iji arrow key input
//#define EMULATE_MESSAGE_QUEUES

#ifdef EMULATE_MESSAGE_QUEUES

#define toggleWhitelistMessage(message) (message)
#define isMessageWhitelisted(message) (false)
#define whitelistUserMessage(message) (message)

#else

// flips a message into a range we'll use to be more sure that it belongs to us
// (even if the game tries to send a message in the flipped range, it will be denied)
// unfortunately, games use pretty much everything in the valid range already,
// and everything about 0xFFFF is reserved by windows.
// FIXME nothing works in all cases, need to store this data elsewhere
//    0x10000 breaks lyle in cube sector
// 0x80000000 breaks iji
//     0x8000 breaks iji
#define whitelistMsgMask 0x2000 // breaks who-knows-what... this is fundamentally flawed, need to implement MessageQueue in wintasee.cpp instead
//#ifdef whitelistMaskFilter
//	#define curWhitelistMsgMask whitelistMaskFilter(whitelistMsgMask)
//#else
//	#define curWhitelistMsgMask whitelistMsgMask
//#endif
#define toggleWhitelistMessage(message) ((message) ^ (whitelistMsgMask))
#define isMessageWhitelisted(message) ((message) & (whitelistMsgMask))
#define whitelistUserMessage(message) (((message) >= WM_USER /*&& (message) < 0xC000*/) ? (toggleWhitelistMessage(message)) : (message)) 

#endif

#endif // MSG_H_INCL
