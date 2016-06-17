#if !defined(SRKMFCAPI_H)
#define SRKMFCAPI_H

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

class	CWnd;
class   SrkBrowserDialog;

#include "srk_defs.h"

#if defined(SONORK_MFC_DLL_BUILD)
#	define WM_SRK_MFC_COMMAND	(WM_APP+100)
#endif

enum SRK_MFC_WIN_TYPE
{
	SRK_MFC_WIN_NONE
,	SRK_MFC_WIN_BROWSER
};

enum SRK_MFC_EVENT
{
	SRK_MFC_EVENT_NONE
,	SRK_MFC_EVENT_01
,	SRK_MFC_EVENT_02
,	SRK_MFC_EVENT_03
,	SRK_MFC_EVENT_04
,	SRK_MFC_EVENT_05
,	SRK_MFC_EVENT_06
,	SRK_MFC_EVENT_07
,	SRK_MFC_EVENT_08
,	SRK_MFC_EVENT_09
,	SRK_MFC_EVENT_10
};
enum SRK_MFC_CMD
{
	SRK_MFC_CMD_NONE
,	SRK_MFC_CMD_01
,	SRK_MFC_CMD_02
,	SRK_MFC_CMD_03
,	SRK_MFC_CMD_04
,	SRK_MFC_CMD_05
,	SRK_MFC_CMD_06
,	SRK_MFC_CMD_07
,	SRK_MFC_CMD_08
,	SRK_MFC_CMD_09
,	SRK_MFC_CMD_10
,	SRK_MFC_CMD_11
,	SRK_MFC_CMD_12
,	SRK_MFC_CMD_13
,	SRK_MFC_CMD_14
,	SRK_MFC_CMD_15
,	SRK_MFC_CMD_16
,	SRK_MFC_CMD_17
,	SRK_MFC_CMD_18
,	SRK_MFC_CMD_19
,	SRK_MFC_CMD_20
};

struct TSrkMfcClientData
{
	HWND	hwnd;
};

typedef	BYTE*	PASCAL WINAPI	fnSrkMfcMemAlloc(UINT);
typedef	void	PASCAL WINAPI	fnSrkMfcMemFree(void*);
typedef	void	PASCAL WINAPI	fnSrkMfcTrace(const char*);

struct TSrkMfcContext
{
	fnSrkMfcMemAlloc*	cb_alloc;
	fnSrkMfcMemFree*		cb_free;
	fnSrkMfcTrace*		cb_trace;
	UINT				ev_msg;	// IN
	UINT				cm_msg;	// OUT
};


#if defined(SONORK_MFC_DLL_BUILD)
struct TSrkMfcClient
:public TSrkMfcClientData
{
	CWnd		cwnd;
	LRESULT		Dispatch(SRK_MFC_EVENT,LPARAM);
};
#endif

struct TSrkMfcWindowCreateInfo
{
	TSrkMfcClientData	owner;
	char				err_msg[64];
};

struct TSrkMfcWindowHandle
{

#if !defined(SONORK_MFC_DLL_BUILD)
private:
#endif

	SRK_MFC_WIN_TYPE type;
	HWND			hwnd;
	DWORD			reserved[8];

public:
	HWND	Handle()	const
	{	return hwnd; }

#if !defined(SONORK_MFC_DLL_BUILD)
	LRESULT	Exec(SRK_MFC_CMD,LPARAM);
	BOOL	PreTranslateMessage(MSG*);
#endif

};

#if defined(SONORK_MFC_DLL_BUILD)
struct TSrkMfcWindow
:public TSrkMfcWindowHandle
{
	TSrkMfcClient	owner;
	struct MFC
	{
		union DATA
		{
			void*				ptr;
			CWnd*				cwnd;
			SrkBrowserDialog*	browser;
		}data;
	}mfc;
	TSrkMfcWindow(SRK_MFC_WIN_TYPE p_type){type=p_type;}
	~TSrkMfcWindow(){}


};
extern TSrkMfcContext* ctx;
#endif


typedef	BOOL PASCAL WINAPI
	fnSrkMfcInitialize(DWORD v,TSrkMfcContext*);

typedef	TSrkMfcWindowHandle* PASCAL WINAPI
	fnSrkMfcCreateWindow(SRK_MFC_WIN_TYPE type,TSrkMfcWindowCreateInfo*);

typedef	void PASCAL WINAPI
	fnSrkMfcDestroyWindow(TSrkMfcWindowHandle*);

typedef	BOOL PASCAL WINAPI
	fnSrkMfcPreTranslateMessage(TSrkMfcWindowHandle*,MSG*);

#endif