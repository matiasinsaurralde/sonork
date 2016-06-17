#if !defined(SRKWINCTRL_H)
#define SRKWINCTRL_H


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

#if !defined(SRKWIN_H)
# include "srkwin.h"
#endif


class TSonorkWinCtrl
{
protected:
	HWND		v_hwnd;
	UINT		v_id;
	TSonorkWin*		v_parent;
	WNDPROC     v_proc;

static	LRESULT CALLBACK WinProc(HWND , UINT , WPARAM , LPARAM);

public:
	TSonorkWinCtrl();
	TSonorkWinCtrl(TSonorkWin*parent,UINT ctrl_id);
	virtual ~TSonorkWinCtrl();

	TSonorkWin*	Parent()	{ return v_parent; }
	HWND    Handle()	const	{ return v_hwnd; }
	UINT	Id()		const	{ return v_id;}

	bool	AssignCtrl(TSonorkWin*parent,UINT ctrl_id);
	bool	AssignCtrl(TSonorkWin*parent,HWND, UINT ctrl_id);
	void	ReleaseCtrl();
	LRESULT	DefaultProcessing(UINT , WPARAM , LPARAM);


};

#endif
