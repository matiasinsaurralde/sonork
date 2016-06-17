#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop
#include "srkmaininfowin.h"


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

#define INFO_HEIGHT  		16

#define INFO_X_MARGIN		2
#define HINT_X_MARGIN		3


#define INFO_DT_FLAGS	(DT_NOPREFIX|DT_VCENTER|DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS)
#define HINT_DT_FLAGS	(DT_NOPREFIX|DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER)

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::OnMainViewSelection(int count)
{
	char tmp[80];
	if( !count )
	{
		if( event.type == SONORK_UI_EVENT_SEL_COUNT )
			ClearEvent( false );
	}
	else
	{
		SonorkApp.LangSprintf(tmp,GLS_IF_USEL,count);
		SetEvent(SONORK_UI_EVENT_SEL_COUNT
			,tmp
			,SKIN_ICON_INFO
			,sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_FG)
			,sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_BG)
			,SONORK_UI_EVENT_TTL_FOREVER
			);
	}
}

// ----------------------------------------------------------------------------

TSonorkMainInfoWin::TSonorkMainInfoWin(TSonorkWin*parent)
	:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL|SONORK_WIN_TYPE_NONE
		,0)
{
	event.type = SONORK_UI_EVENT_NONE;
	event.data.count=0;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::ClearEvent(bool force_redraw)
{
	event.age = 0;
	event.ttl =SONORK_UI_EVENT_TTL_FOREVER;
	if( SonorkApp.GetCounter(SONORK_APP_COUNTER_EVENTS) > 0 )
	{
		if( force_redraw == false
		&&  event.type == SONORK_UI_EVENT_EVENT_COUNT
		&&  event.data.count == (DWORD)SonorkApp.GetCounter(SONORK_APP_COUNTER_EVENTS) )
		{
			// we're already displaying the event counter
			// with the correct value
			return;
		}
		event.data.count=(DWORD)SonorkApp.GetCounter(SONORK_APP_COUNTER_EVENTS);
		event.text.SetBufferSize(128);
		SonorkApp.LangSprintf(event.text.Buffer()
			,GLS_IF_EVTC
			,event.data.count
			);
		event.icon	=SKIN_ICON_INFO;
		event.type 	=SONORK_UI_EVENT_EVENT_COUNT;
	}
	else
	{
		if( event.type	== SONORK_UI_EVENT_NONE && force_redraw == false )
		{
			// we're already displaying the (NONE)
			return;
		}
		event.icon	= SKIN_ICON_UP_ARROW;
		event.type	= SONORK_UI_EVENT_NONE;
		event.text.Set(SonorkApp.LangString(GLS_IF_MNU));
		// Don't set "Sonork icon opens menu" message on tray icon
		// use generic connection status instead
	}
	event.fg_color	=sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_FG);
	event.bg_color	=sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_BG);
	InvalidateRect(NULL,false);
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::SetEvent(SONORK_UI_EVENT_TYPE	type
			, SONORK_C_CSTR		str
			, SKIN_ICON		icon
			, COLORREF 		fg_color
			, COLORREF 		bg_color
			, DWORD	   		ttl)
 {
	if( type == SONORK_UI_EVENT_NONE || str==NULL )
	{
		ClearEvent( false );
		return;
	}
	event.age  = 0;
	event.ttl  = ttl;
	event.type = type;
	event.text.Set(str);
	event.icon = icon;
	event.fg_color = fg_color;
	event.bg_color = bg_color;
	InvalidateRect(NULL,false);
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainInfoWin::OnBeforeCreate(TSonorkWinCreateInfo*CI)
{
	RECT rect;
	CI->style&=~WS_BORDER;
	CI->style|=WS_VISIBLE;

	rect.top	= 0;
	rect.bottom	= INFO_HEIGHT;
	::AdjustWindowRectEx(&rect,CI->style,false,CI->ex_style);
	MoveWindow(0,0,10,rect.bottom);
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::TimeSlot(UINT msecs)
{
	event.age+=msecs;
	if(event.age > event.ttl)
		ClearEvent(false);
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::OnPaint(HDC tDC, RECT&, BOOL)
{
	RECT 	rect, t_rect;
	HBRUSH		brush;
	rect.left 	= 0;
	rect.right	= Width();
	rect.top 	= 0;
	rect.bottom	= Height();
	t_rect.left	= INFO_X_MARGIN*2+SKIN_ICON_SW ;
	t_rect.right	= rect.right-INFO_X_MARGIN;
	t_rect.top	= rect.top;
	t_rect.bottom	= rect.bottom;

	SaveDC(tDC);
	SetBkMode(tDC,TRANSPARENT);
	SelectObject(tDC , sonork_skin.Font(SKIN_FONT_SMALL));
	SetTextColor(tDC , event.fg_color);
	brush = CreateSolidBrush(event.bg_color);
	FillRect(tDC,&rect,brush);
	DrawText(tDC
			,event.text.CStr()
			,event.text.Length()
			,&t_rect
			,INFO_DT_FLAGS);
	sonork_skin.DrawIcon(tDC, event.icon, INFO_X_MARGIN, rect.top );
	DeleteObject(brush);
	RestoreDC(tDC,-1);

}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::OnMouseMove(UINT ,int ,int )
{
	UpdateMouseCursor( false );
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::UpdateMouseCursor( bool forced )
{
	switch( EventType() )
	{
		case SONORK_UI_EVENT_USER_CONNECT:
		case SONORK_UI_EVENT_USER_DISCONNECT:
		case SONORK_UI_EVENT_ADD_USER:
		case SONORK_UI_EVENT_EVENT_COUNT:
			::SetCursor(sonork_skin.Cursor(SKIN_CURSOR_HAND));
		break;

		default:
		if( forced )
			::SetCursor(sonork_skin.Cursor(SKIN_CURSOR_ARROW));
		break;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainInfoWin::OnLButtonDown(UINT ,int ,int )
{
	switch( EventType() )
	{
		default:
			break;

		case SONORK_UI_EVENT_USER_CONNECT:
		case SONORK_UI_EVENT_USER_DISCONNECT:
		case SONORK_UI_EVENT_ADD_USER:
			SonorkApp.OpenMsgWindow( SonorkApp.UserList().Get( event.data.gu_id ) , SONORK_MSG_WIN_OPEN_FOREGROUND);
			break;

		case SONORK_UI_EVENT_EVENT_COUNT:
			SonorkApp.OpenNextEvent( false );
			break;
	}
}

// ----------------------------------------------------------------------------

void TSonorkMainInfoWin::OnSize(UINT size_type)
{
	if(size_type==SIZE_RESTORED)
	{
		InvalidateRect(NULL,false);
	}
}


