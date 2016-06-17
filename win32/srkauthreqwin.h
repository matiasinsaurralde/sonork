#if !defined(SRKAUTHREQWIN_H)
#define SRKAUTHREQWIN_H

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

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

class TSonorkAuthReqWin
:public TSonorkTaskWin
{
private:
	HWND	status_bar;

	bool	OnCreate();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnDrawItem(DRAWITEMSTRUCT*);

	bool	OnAppEvent(UINT event, UINT param, void*data);

	void	UpdateCxStatus();
	void	UpdateButtons();

	void	CmdAccept();
	void	CmdDeny();
	void	CmdInfo();

	void	OnTaskStart(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&);
	void	OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);
	void	OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&, const TSonorkError*);

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	TSonorkError	taskERR;
public:
	TSonorkAuthReqWin(const TSonorkId&);
	TSonorkAuthReqData	RD;

	BOOL
		IsUserId(const TSonorkId& userId) const
		{
			return RD.user_data.userId == userId;
		}

};
#endif
