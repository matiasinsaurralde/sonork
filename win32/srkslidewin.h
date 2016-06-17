#if !defined(SRKSLIDEWIN_H)
#define SRKSLIDEWIN_H

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

#include "srkwin.h"


enum SONORK_SLIDER_WIN_POS
{
	SONORK_SLIDER_WIN_DISABLED
,	SONORK_SLIDER_WIN_TOP_LEFT
,	SONORK_SLIDER_WIN_TOP_RIGHT
,	SONORK_SLIDER_WIN_BOTTOM_RIGHT
,	SONORK_SLIDER_WIN_BOTTOM_LEFT
};

class TSonorkSlideWin
:public TSonorkWin
{
	bool OnBeforeCreate(TSonorkWinCreateInfo*CI);
	void 	OnLButtonDown(UINT keys,int x,int y);
	void 	OnPaint(HDC, RECT&, BOOL);
	bool 	OnEraseBG(HDC){return true;}

	RECT			text_rect;
	struct unEVENT
	{
		SONORK_UI_EVENT_TYPE	type;
		SKIN_ICON		icon;
		TSonorkShortString	text;
		DWORD			age;
		DWORD			ttl;
		COLORREF 		fg_color;
		COLORREF 		bg_color;
		union{
			TSonorkId	gu_id;
		}data;
	}event;
	
	void
		DoClearEvent();
public:
	TSonorkSlideWin();

	SONORK_UI_EVENT_TYPE
		EventType() const
		{
			return event.type;
		}

	TSonorkId&
		UserId()
		{	return event.data.gu_id; }

	void
		SetEvent(SONORK_UI_EVENT_TYPE
			, SONORK_C_CSTR
			, SKIN_ICON
			, COLORREF fg_color
			, COLORREF bg_color
			, DWORD	   ttl);
	void
		ClearEvent();

	void
		TimeSlot(UINT msecs);

};

#endif
