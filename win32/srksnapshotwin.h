#if !defined(SRKSNAPSHOTWIN_H)
#define SRKSNAPSHOTWIN_H

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

#include "srkwinctrl.h"

class TSonorkSnapshotWin
:public TSonorkWin
{
	enum CAPTURE_STATE
	{
		CAPTURE_NONE
	,	CAPTURE_ORIGIN
	,	CAPTURE_END
	,	CAPTURE_SNAP
	};
	HCURSOR			hcursor;
	struct {
		HDC	dc;
		HBITMAP	bm;
	}scr;


	struct {
		HDC	dc;
		SIZE	sz;
		RECT	focus_rect;
		RECT	visible_focus_rect;
	}view;
	TSonorkWinCtrl	viewCtrl;

	struct {
		HDC	dc;
		HBITMAP	bm;
		SIZE	sz;
	}mem;
	BITMAP 	bmp;
	struct
	{
		SIZE	size;
	}button;

	bool	OnCreate();
	void	OnBeforeDestroy();
	void 	OnDestroy();
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnMinMaxInfo(MINMAXINFO*);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT	OnCtlWinMsg(TSonorkWinCtrl*,UINT,WPARAM,LPARAM);
	void	OnSize(UINT);
	void	OnTimer(UINT);
	void	Realign();

	void
		ShowFocusRect(bool);
	void
		EndCrop(bool crop_to_focus_rect);

	BOOL
		SaveBitmap(const char*file_name);

	bool
		CmdSave(TSonorkShortString&);
public:
	TSonorkSnapshotWin();

};

#endif
