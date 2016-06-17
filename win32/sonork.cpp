#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL).

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	SSCL for more details.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).
*/

//#define GENERATE_DEBUG_INFO
//#define DEBUG_WINDOW_VISIBLE




HINSTANCE	sonork_app_instance;
TSonorkWin32App SonorkApp;

int PASCAL WinMain(HINSTANCE hCurInstance, HINSTANCE , LPSTR pszCmdLine, int )
{
	sonork_app_instance = hCurInstance;
	SonorkApp.Run(pszCmdLine);
	return 1;
}



#if !defined(GENERATE_DEBUG_INFO)
void DbgOpen(const char*){}
void DbgClose(){}
void sonork_puts(const char*){}
void DbgPrint(const char *,...){}

#else	// defined(GENERATE_DEBUG_INFO)

FILE *dbg_file;
void DbgOpen(const char*cmd_line)
{
	char tmp[64];
	sprintf(tmp,"dbg_%s.txt",cmd_line);
#if defined(DEBUG_WINDOW_VISIBLE)
	AllocConsole();
#endif
	dbg_file=fopen(tmp,"wt");
}

void sonork_puts(const char *str)
{
	DbgPrint( str );
}
void DbgPrint(const char *fmt,...)
{
//	static int 	cc=0;
	SYSTEMTIME	st;
	static char tmp[1024];
	DWORD 		len;
	va_list		argptr;
	GetLocalTime(&st);
//	len=sprintf(tmp,"%04d:",cc++);
	len=sprintf(tmp,"%04u.%03u:"
		,st.wMinute * 60 + st.wSecond
		,st.wMilliseconds
		);
	va_start(argptr, fmt);
		len += vsprintf(tmp+len, fmt, argptr);
	va_end(argptr);
	strcpy(tmp+len,"\n");
	if( dbg_file )
		fputs(tmp,dbg_file);
	strcpy(tmp+len,"\r\n");
#if defined(DEBUG_WINDOW_VISIBLE)
	DWORD o_len;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), tmp, len+2, &o_len, NULL);
#endif
}

void DbgClose()
{
	if(dbg_file!=NULL)fclose(dbg_file);
#if defined(DEBUG_WINDOW_VISIBLE)
	FreeConsole();
#endif
}




#endif




#define WinMain
