/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// nitsuja: I am using it. Thanks, Zoltan Csizmadia at zoltan_csizmadia@yahoo.com.
//          I modified it to print out an external process's call stack.
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.h
//

#ifndef EXTENDEDTRACE_H_INCLUDED
#define EXTENDEDTRACE_H_INCLUDED

#if 1//defined(_DEBUG) && defined(WIN32)

#include <windows.h>
#include <tchar.h>

#if defined(_AFX) || defined(_AFXDLL)
#define TRACEF									         TRACE
#else
#define TRACEF									         OutputDebugStringFormat
void OutputDebugStringFormat( LPCTSTR, ... );
#endif

//#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )	InitSymInfo( IniSymbolPath, GetCurrentProcess() )
#define EXTENDEDTRACEINITIALIZEEX( IniSymbolPath, hProcess )	InitSymInfo( IniSymbolPath, hProcess )
#define EXTENDEDTRACEUNINITIALIZE(hProcess)			         UninitSymInfo(hProcess)
#define SRCLINKTRACECUSTOM( Msg, File, Line)       SrcLinkTrace( Msg, File, Line )
#define SRCLINKTRACE( Msg )                        SrcLinkTrace( Msg, __FILE__, __LINE__ )
#define FNPARAMTRACE()							         FunctionParameterInfo( GetCurrentThread() )
#define THREADFNPARAMTRACE( hThread )					 FunctionParameterInfo( hThread )
#define STACKTRACEMSG( Msg, hProcess )					         StackTrace( Msg, hProcess )
#define STACKTRACE(hProcess)							         StackTrace( GetCurrentThread(), _T(""), hProcess )
#define THREADSTACKTRACEMSG( hThread, Msg, hProcess )		   StackTrace( hThread, Msg, hProcess )
#define THREADSTACKTRACE( hThread, hProcess )				   StackTrace( hThread, _T(""), hProcess )
#define LOADSYMBOLS( hProcess, dllname )				   LoadModuleSymbols( hProcess, dllname )
#define LOADSYMBOLS2( hProcess, dllname, dllhandle, dllbase )		LoadModuleSymbols( hProcess, dllname, dllhandle, (DWORD64)(dllbase) )

BOOL InitSymInfo( PCSTR, HANDLE );
BOOL UninitSymInfo( HANDLE );
void SrcLinkTrace( LPCTSTR, LPCTSTR, ULONG );
void StackTrace( HANDLE, LPCTSTR, HANDLE );
void StackTraceOfDepth( HANDLE, LPCTSTR, int, int, HANDLE );
void FunctionParameterInfo();
void LoadModuleSymbols(HANDLE hProcess, PSTR name);
void LoadModuleSymbols(HANDLE hProcess, PSTR name, HANDLE hFile, DWORD base);

#else

#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )   ((void)0)
#define EXTENDEDTRACEINITIALIZEEX( IniSymbolPath, hProcess )   ((void)0)
#define EXTENDEDTRACEUNINITIALIZE()			         ((void)0)
#define TRACEF									            ((void)0)
#define SRCLINKTRACECUSTOM( Msg, File, Line)	      ((void)0)
#define SRCLINKTRACE( Msg )						      ((void)0)
#define FNPARAMTRACE()							         ((void)0)
#define THREADFNPARAMTRACE( hThread )							         ((void)0)
#define STACKTRACEMSG( Msg )					         ((void)0)
#define STACKTRACE()						         	   ((void)0)
#define THREADSTACKTRACEMSG( hThread, Msg )		   ((void)0)
#define THREADSTACKTRACE( hThread )				      ((void)0)
#define LOADSYMBOLS( hProcess, dllname )				   ((void)0)
#define LOADSYMBOLS2( hProcess, dllname, dllhandle, dllbase )		((void)0)

#endif

extern bool traceEnabled;

#endif
