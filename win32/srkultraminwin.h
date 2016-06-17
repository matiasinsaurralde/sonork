#if !defined(SRKULTRAMINWIN_H)
#define SRKULTRAMINWIN_H

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

#define	ULTRA_MIN_TEXT_WIDTH	(80)
#define ULTRA_MIN_FULL_HEIGHT	(20)
#define ULTRA_MIN_FULL_WIDTH	(ULTRA_MIN_TEXT_WIDTH+SKIN_ICON_SW*2+3)


struct TSonorkUltraMinPaint
{
	char		text[80];
	SKIN_ICON	icon;
	DWORD		fg_color;
	DWORD		bg_color;
};
struct TSonorkUltraMinMouse
{
	POINT	point;
	DWORD	keys;
};

class TSonorkUltraMinWin
:public TSonorkWin
{
	TSonorkWin*		owner;
	RECT			fullRECT;
	RECT			textRECT;
	bool	OnBeforeCreate(TSonorkWinCreateInfo*CI);
	bool	OnCreate();
	void	OnBeforeDestroy();
	void 	OnPaint(HDC, RECT&, BOOL);
	bool 	OnEraseBG(HDC){ return true;}
	void 	OnLButtonDown(UINT keys,int x,int y);
	void 	OnLButtonUp(UINT keys,int x,int y);
	void 	OnLButtonDblClk(UINT keys,int x,int y);
	void 	OnMouseMove(UINT keys,int x,int y);
public:
	TSonorkUltraMinWin(TSonorkWin*parent);
	bool	Show(int x,int y);
};

#endif