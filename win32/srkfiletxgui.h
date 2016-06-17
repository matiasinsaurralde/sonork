#if !defined(SRKFILETXGUI_H)
#define SRKFILETXGUI_H

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

#include "srkfiletxeng.h"


class TSonorkFileTxGui
:public TSonorkTaskWin
{
public:

private:
	HWND				progress_hwnd;
	HWND				status_hwnd;
	TSonorkFileTxEng	*	tx_eng;
	DWORD				progress_limit;
	DWORD				msg_task_id;
	TSonorkCCacheMark		msg_mark;
	TSonorkError			taskERR;
	struct _USER
	{
		TSonorkShortString	alias;
		TSonorkMsg		msg;
	}user;
	struct _TARGET
	{
		UINT			count;
		UINT			sent;
		TSonorkId	*	ptr;
	}target;
	TSonorkId			sender;

	bool	OnCreate();
	void	OnAfterCreate();
	void	OnBeforeDestroy();
	bool 	OnAppEvent(UINT event, UINT param, void*data);

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);


	void
		MarkCCacheLine(DWORD,bool save_path);

	void
		StartProgress(DWORD offset, DWORD size);



	static void SONORK_CALLBACK
		FileTxEngHandler(void *param,TSonorkFileTxEngEvent*);
	void
		OnTxEng_Error(TSonorkFileTxEngEvent*E);

	void
		OnTxEng_Target_Result(TSonorkDataServerPutFileTargetResult*);

	void
		OnTxEng_Phase(TSonorkFileTxEngEvent*E);

	void
		OnTxEng_Phase_File_Request(TSonorkFileTxEngEvent*E);
		
	void
		OnTxEng_Phase_File_Complete(BOOL close_wait);

	void
		SendNotificationMessage( const TSonorkId*sonork_id , bool sent_by_server );

public:
	TSonorkFileTxGui(SONORK_DWORD2* target
		, UINT 			target_count
		, SONORK_C_CSTR 	alias
		, TSonorkShortStringQueue*
		, UINT tx_flags);
	TSonorkFileTxGui(
		  SONORK_C_CSTR alias
		, SONORK_C_CSTR path
		, TSonorkCCacheMark&
		,const TSonorkFileInfo& file_info
		, UINT tx_flags);
        ~TSonorkFileTxGui();
};

char*
	DottedValue(char* buffer, DWORD value, bool* allow_kb);

#endif