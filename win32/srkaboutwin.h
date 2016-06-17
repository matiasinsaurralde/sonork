#if !defined(SRKABOUTWIN_H)
#define SRKABOUTWIN_H

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

class TSonorkAboutWin
:public TSonorkWin
{
	HWND	img_hwnd;
	HDC		mem_dc;
	HDC		scr_dc;
	HBITMAP	mem_bm;
	SIZE	mem_size;
	SIZE	scr_size;
	RECT	scroll_rect;
	HBRUSH	bgBrush;
	int	logo_height_th;
	int	scroll_y;
	int	bottom_margin;
	bool	OnCreate();
	void 	OnDestroy();
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	int		DrawBitmapText(HDC dc, SONORK_C_CSTR str, int len, bool calc);
	void	DrawTextArea(HDC,UINT);
	void	OnTimer(UINT);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
public:
	TSonorkAboutWin(TSonorkWin*parent);
};

#endif