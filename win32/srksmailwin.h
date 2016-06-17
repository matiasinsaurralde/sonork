#if !defined(SRKSMAILWIN_H)
#define SRKSMAILWIN_H

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

#include "srk_email_codec.h"

class TSonorkSmailWin
:public TSonorkWin
{
	PROCESS_INFORMATION		proc_info;
	TSonorkShortString			tmp_file;
	TSonorkEmailAccountQueue	acc_queue;
	TSonorkMsg					msg;

	bool	OnCreate();
	void	OnAfterCreate();
	void	OnBeforeDestroy();
	void	OnTimer(UINT);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	LoadLabels();
	void	LoadAccounts();
	bool	OnAppEvent(UINT event, UINT , void* data);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	const TSonorkExtUserData*	user_data;

	void	DoSend();
	bool	PrepareParameters(SONORK_C_STR , TSonorkEmailAccount* , SONORK_C_CSTR to);
	void	EndSend();
	void	DelTempFile();
	void	SaveMsg();
public:
	TSonorkSmailWin(TSonorkWin*parent,const TSonorkExtUserData*);
};

#endif