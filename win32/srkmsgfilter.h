#if !defined(GUMSGFILTER_H)
#define GUMSGFILTER_H

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

#if !defined(SRKMSGWIN_H)
#include "srkmsgwin.h"
#endif


#define SONORK_MFF_TEXT_MAX_SIZE		32
#define SONORK_MFF_PLAIN_TEXT			0x000001
#define SONORK_MFF_URL					0x000002
#define SONORK_MFF_FILE					0x000004
#define SONORK_MFF_OTHER				0x000008
#define SONORK_MFF_SENT					0x000010
#define SONORK_MFF_RECVD				0x000020
#define SONORK_MFF_PROTECTED			0x000040
#define SONORK_MFF_REPLY				0x000080
#define SONORK_MFF_QUERY				0x000100
#define SONORK_MFF_EMAIL				0x000200
#define SONORK_MFF_REVERSE				0x001000
#define SONORK_MFF_INCTEXT				0x010000
#define SONORK_MFF_NOTTEXT				0x020000
#define SONORK_MFF_LENGTH				0x040000
#define SONORK_MFF_AGE					0x080000

struct TSonorkMsgFilter
{
	DWORD	cur_line;
	DWORD	end_line;
	DWORD	flags;
	struct{
		DWORD min,max;
	}length;
	struct{
		DWORD min,max;
	}age;
	TSonorkShortString	inc_text;
	TSonorkShortString	not_text;

	DWORD	FindFirst(DWORD max_scan_lines);
	DWORD	FindNext(DWORD max_scan_lines);

};

class TSonorkMsgFilterWin
:public TSonorkWin
{
	struct {
		HWND 	hwnd;
		int	height;
	}status;
	TSonorkCCache		*i_cache;
	TSonorkConsole		*i_console;
	TSonorkShortString	cache_path;
	TSonorkMsgWinContext	context;
	TSonorkMsgFilter	filter;
	char			age_label[48];
	SIZE			top_frame_size;
	struct
	{
		DWORD	min,max;
	}i_range;

public:
	bool	OnCreate();
	void	OnBeforeDestroy();
	void	OnSize(UINT );
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	void	OnInitMenu(HMENU);
	bool	OnMinMaxInfo(MINMAXINFO*);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	void	LoadLabels();
	void	RealignControls();
	LRESULT ProcessVKey(UINT vKey, DWORD flags);

	void	StartFilter();
	void	ContinueFilter();
	void	CmdDelSelected();
	bool	CmdProcess();
	bool	CmdOpenFile();
	void	CmdLocateMain();


	static DWORD SONORK_CALLBACK
				ConsoleCallback(void*
					,SONORK_CONSOLE_EVENT 	pEvent
					,DWORD				pIndex
					,void*				pData);
	DWORD	OnHistoryWinEvent(TSonorkHistoryWinEvent*E);


	void	OpenMsg(BOOL dbl_clicked);
	void	UpdateMsgInfoText(DWORD line_no,TSonorkCCacheEntry*);

public:
	TSonorkMsgFilterWin(TSonorkMsgWin*owner);

	SONORK_CONSOLE_OUTPUT_MODE
			GetOutputMode()		const
			{	return context.console->GetOutputMode();}

	void	SetHintMode(SONORK_C_CSTR str, bool update_window)
			{ context.console->SetHintMode(str,update_window); }

};



#endif