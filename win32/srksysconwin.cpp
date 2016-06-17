#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srksysconwin.h"

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

#define SYSCON_LEFT_MARGIN		(SKIN_ICON_SW + 2 + 32)
#define SYSCON_RIGHT_MARGIN		2
#define SYSCON_TEXT_PADDING		2
#define SYSCON_TEXT_SPACING		1
#define	MIN_WIDTH			160
#define MIN_HEIGHT			(SKIN_ICON_SH*2+64)
#define MAX_WIDTH			480
#define MAX_HEIGHT			480

// ----------------------------------------------------------------------------

BOOL
 TSonorkSysConsoleWin::FocusNextUnread( BOOL bring_to_front )
{
	TSonorkCCacheEntry* CL;
	DWORD line_no;
	line_no = SonorkApp.AppRunValue(SONORK_ARV_FIRST_UNREAD_SYS_MSG);
	// GetNext returns last line number in <line_no> if none is found
	CL = cache->GetNext(line_no , SONORK_APP_CCF_UNREAD, SONORK_APP_CCF_UNREAD);
	SonorkApp.AppRunValue(SONORK_ARV_FIRST_UNREAD_SYS_MSG) = line_no;
	if( CL != NULL )
	{
		if( bring_to_front && ( !IsActive() || IsIconic() ) )
			SonorkApp.PostAppCommand( SONORK_APP_COMMAND_FOREGROUND_HWND , (LPARAM)Handle() );
	}
	history_win->SetFocusedLine( line_no , false );
	return CL!=NULL;
}

// ----------------------------------------------------------------------------

bool
 TSonorkSysConsoleWin::OnCreate()
{
	assert( cache != NULL );

	SetWindowText( GLS_LB_SYSCON );
	history_win = new TSonorkHistoryWin(
			this
		,	cbHistoryEvent
		,	this
		, 	cache );
	history_win->Create();
	history_win->EnableSelect( false );
	history_win->EnableFocus( true );
	history_win->EnableDrag( false );
	history_win->SetMarginsEx(SYSCON_LEFT_MARGIN,SYSCON_RIGHT_MARGIN);
	history_win->SetPaddingEx(SYSCON_TEXT_PADDING,SYSCON_TEXT_SPACING);
	history_win->SetDefColors(SKIN_COLOR_SYSCON);
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SYS_CONSOLE,true,"SysCon");
	RealignControls();
	history_win->ShowWindow(SW_SHOW);
	
	SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND
			, (LPARAM)history_win->Handle());

	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::OnAfterCreate()
{
	FocusNextUnread( false );
}

// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::OnBeforeDestroy()
{
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_SYS_CONSOLE,false,"SysCon");

}

// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::OnSize(UINT size_type)
{
	if( size_type == SIZE_RESTORED )
		RealignControls();
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkSysConsoleWin::OnPoke(SONORK_WIN_POKE,LPARAM)
{return 0L;}

// ----------------------------------------------------------------------------

#define SPACING 2
void
 TSonorkSysConsoleWin::RealignControls()
{
	const int H = (int)Height() ;
	const int W = (int)Width();

	if( !history_win )return;

	history_win->SetWindowPos(
		 NULL
		,SPACING
		,SPACING
		,W-SPACING
		,H	- SPACING*2
		,SWP_NOZORDER|SWP_NOACTIVATE);


}

// ----------------------------------------------------------------------------

bool
 TSonorkSysConsoleWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MIN_WIDTH;
	MMI->ptMaxTrackSize.x=MAX_WIDTH;
	MMI->ptMinTrackSize.y=MIN_HEIGHT;
	MMI->ptMaxTrackSize.y=MAX_HEIGHT;
	return true;
}

// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkSysConsoleWin::cbHistoryEvent( void*param , TSonorkHistoryWinEvent*E )
{
	TSonorkSysConsoleWin*_this = (TSonorkSysConsoleWin*)param;
	if( E->Event() == SONORK_HIST_WIN_EVENT_LINE_PAINT)
	{
		_this->PaintViewLine( E->Line(), E->PaintContext() );
	}
	else
	if( E->Event() == SONORK_HIST_WIN_EVENT_LINE_CLICK)
	{
		if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED )
		{
			_this->OnLineFocused(E->LineNo());
		}
		if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
		{
			_this->OnLineDblClick();
		}
	}
	return 0L;
}

// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::OnLineDblClick()
{
	TSonorkCCacheEntry *	CL;
	DWORD			line_no;

	CL = history_win->GetFocused( NULL , &line_no );
	if( CL == NULL )return;
	
	if( CL->ext_index == SONORK_UI_EVENT_INCOMMING_EMAIL)
		SonorkApp.OpenMailReader(history_win);
}


// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::OnLineFocused(DWORD line_no)
{
	TSonorkCCacheEntry *pEntry;
	pEntry = cache->Get( line_no , NULL, NULL);
	if( pEntry == NULL )return;
	if( pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD )
		MarkLineAsRead( line_no , pEntry );
}

// ----------------------------------------------------------------------------

bool
 TSonorkSysConsoleWin::MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry*pEntry)
{
	if(!pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD)
		return false;
	pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS]&=~SONORK_APP_CCF_UNREAD;
	SonorkApp.IncCounter(SONORK_APP_COUNTER_SYS_CONSOLE,-1);
	if(cache->Set(line_no,pEntry->tag,NULL) != SONORK_RESULT_OK )
		return false;
	history_win->PaintViewLine( line_no );
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkSysConsoleWin::PaintViewLine(const TSonorkCCacheEntry*CL, TSonorkHistoryWinPaintCtx*CTX)
{
	DWORD 		cFlags=CL->tag.v[SONORK_CCACHE_TAG_FLAGS];
	HBRUSH		brush;
	RECT		tRECT;
	char		tmp[32];
	SKIN_ICON	icon;
	SKIN_COLOR	color;
	SYSTEMTIME	SI;

	CL->time.GetTime(&SI);
	wsprintf(tmp,"%02u:%02u"
		,SI.wHour
		,SI.wMinute);

	if( CTX->flags & SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED ) // Line is focused
	{
		color = SKIN_COLOR_MSG_FOCUS;
	}
	else
	if( cFlags & SONORK_APP_CCF_UNREAD)
	{
		color = SKIN_COLOR_SYSCON_NEW;
	}
	else
	{
		color = SKIN_COLOR_SYSCON;
	}
	brush = CTX->SetLineColor( sonork_skin.Color(color,SKIN_CI_BG) );
	CTX->SetTextColor( sonork_skin.Color(color,SKIN_CI_FG) );
	icon = (SKIN_ICON)CL->tag.v[SONORK_CCACHE_TAG_INDEX];
	::SelectObject(CTX->hDC(),sonork_skin.Font(SKIN_FONT_SMALL));
	::FillRect(CTX->hDC(),CTX->LeftRect(),brush);
	sonork_skin.DrawIcon(CTX->hDC()
			, icon
			, CTX->LeftRect()->left + 1
			, CTX->TextRect()->top );
	tRECT.left  = CTX->LeftRect()->left  	+ SKIN_ICON_SW + 2;
	tRECT.right = CTX->LeftRect()->right 	- 2;
	tRECT.top   = CTX->TextRect()->top 		;
	tRECT.bottom= tRECT.top + SKIN_ICON_SH; //CTX->TextRect()->bottom 	;
	::DrawText(CTX->hDC()
		, tmp
		, -1
		, &tRECT
		, DT_SINGLELINE|DT_NOPREFIX|DT_CENTER|DT_VCENTER);
	::SelectObject(CTX->hDC(),CTX->hFont());
	CTX->flags|=SONORK_HIST_WIN_PAINT_F_LEFT_PAINTED;
}

// ----------------------------------------------------------------------------

TSonorkSysConsoleWin::TSonorkSysConsoleWin(TSonorkWin*pParent,TSonorkCCache*pcache)
	:TSonorkWin(pParent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_SYSCONSOLE
	,0)
{
	cache			=pcache;
}

