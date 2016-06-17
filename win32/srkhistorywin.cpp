#include "srkwin32app.h"
#pragma hdrstop
#include "srkhistorywin.h"

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

#define DEFAULT_TEXT_LEFT_MARGIN	(SONORK_HIST_WIN_LICON_SW+SONORK_HIST_WIN_RICON_SW+4)
#define DEFAULT_TEXT_RIGHT_MARGIN	2
#define DEFAULT_TEXT_PADDING		6
#define DEFAULT_TEXT_SPACING		2

#define TEXT_PADDING			view.t_padding
#define	TEXT_SPACING			view.t_spacing
#define MAX_SCROLL_DELTA		2
#define MAX_SCAN_LINES			20
#define ICON_TOP_MARGIN			0
#define	CALLBACK_HINT_STRING		((char*)0xffffffff)
#define DT_BASE_FLAGS         	 	(DT_LEFT|DT_NOPREFIX|DT_WORDBREAK|DT_EXPANDTABS)
//DT_END_ELLIPSIS|

#define SCAN_LINE_EXTRA_HEIGHT 		((TEXT_PADDING*2) + TEXT_SPACING)

#define PAINT_EVENT		paint_event
#define PAINT_CTX		paint_event.info.paint_line.context
#define PAINT_FLAGS		paint_event.info.paint_line.context->flags




void
 TSonorkHistoryWin::StartPaintView(HDC tDC)
{
	assert( PAINT_CTX == NULL );
	PAINT_CTX = new TSonorkHistoryWinPaintCtx;
	// Fixed values
	PAINT_CTX->sepaR.left	=
	PAINT_CTX->leftR.left	= 0;

	PAINT_CTX->sepaR.right	=
	PAINT_CTX->lineR.right  = Width();
	PAINT_CTX->lineR.top	= 0 - scan.offset*scan.text_height;

	PAINT_CTX->lineR.left  	=
	PAINT_CTX->leftR.right	= view.l_margin;
	PAINT_CTX->textR.left	= view.l_margin + view.t_padding;

	PAINT_CTX->textR.right	= Width() - view.r_margin - view.t_padding;

	PAINT_EVENT.line_no	= ViewOffset();
	PAINT_CTX->lines	= cache->Lines();
	PAINT_CTX->bg_brush	= NULL;
	PAINT_CTX->bg_color	=
	PAINT_CTX->fg_color	= 0x80000000;	// impossible values
	PAINT_CTX->view.bg_brush= CreateSolidBrush( sonork_skin.Color(view.color, SKIN_CI_BG) );
	PAINT_CTX->view.sp_brush= CreateSolidBrush( sonork_skin.Color(view.color, SKIN_CI_SP) );

	PAINT_CTX->tDC 		= tDC;
	PAINT_CTX->tFont 	= sonork_skin.Font(SKIN_FONT_CONSOLE);
	SaveDC( PAINT_CTX->tDC );
	SelectObject( PAINT_CTX->tDC, PAINT_CTX->tFont );
	SetBkMode( PAINT_CTX->tDC, TRANSPARENT );
	PAINT_CTX->SetLineColor( 0xffffff );
	PAINT_CTX->SetTextColor( 0 );

}
void
 TSonorkHistoryWin::EndPaintView()
{
	assert( PAINT_CTX != NULL );
	RestoreDC(PAINT_CTX->tDC,-1);
	if(PAINT_CTX->bg_brush)
		DeleteObject(PAINT_CTX->bg_brush);
	DeleteObject(PAINT_CTX->view.bg_brush);
	DeleteObject(PAINT_CTX->view.sp_brush);
	delete PAINT_CTX;
	PAINT_CTX = NULL;

}
// DoPaintViewLine updates PAINT_CTX->lineR.bottom
// and all other rectangles.. the only thing it does not
// update is the PAINT_CTX->lineR.top
void
 TSonorkHistoryWin::DoPaintViewLine(DWORD dt_flags)
{
	PAINT_FLAGS = view.line[PAINT_CTX->vline].flags
			&
			SONORK_HIST_WIN_PAINT_VLINE_MASK;

	if(PAINT_EVENT.line_no == view.focus)
		PAINT_FLAGS|=SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED;
	else
	if(v_selection.Contains( PAINT_EVENT.line_no ) )
		PAINT_FLAGS|=SONORK_HIST_WIN_PAINT_F_LINE_SELECTED;

	PAINT_CTX->leftR.top 	= PAINT_CTX->lineR.top;

	PAINT_CTX->textR.top   	= PAINT_CTX->lineR.top
				  + TEXT_PADDING;

	PAINT_CTX->textR.bottom	= PAINT_CTX->textR.top
				  + view.line[PAINT_CTX->vline].text_height;

	PAINT_CTX->sepaR.top	=
	PAINT_CTX->leftR.bottom	= PAINT_CTX->textR.bottom
				  + view.t_padding;

	PAINT_CTX->lineR.bottom	= PAINT_CTX->lineR.top
				  + view.line[PAINT_CTX->vline].text_height
				  + view.t_padding*2;

	PAINT_CTX->sepaR.bottom	= PAINT_CTX->lineR.bottom
				  + view.t_spacing;

	PAINT_CTX->e_icon =
	PAINT_CTX->l_icon = SKIN_ICON_NONE;
	PAINT_CTX->r_icon = view.line[PAINT_CTX->vline].r_icon;

	cb.ptr(cb.tag,&PAINT_EVENT);
	
	view.line[PAINT_CTX->vline].flags =
		(short)(PAINT_FLAGS&SONORK_HIST_WIN_PAINT_VLINE_MASK);
	view.line[PAINT_CTX->vline].r_icon =
			PAINT_CTX->r_icon;


	if(!(PAINT_FLAGS&SONORK_HIST_WIN_PAINT_F_LINE_PAINTED))
	{
		::FillRect(PAINT_CTX->tDC
			,&PAINT_CTX->lineR
			,PAINT_CTX->bg_brush);
	}
	if(!(PAINT_FLAGS&SONORK_HIST_WIN_PAINT_F_TEXT_PAINTED))
	{
		::DrawText(PAINT_CTX->tDC
			, PAINT_CTX->str_ptr
			, PAINT_CTX->str_len
			,&PAINT_CTX->textR
			, dt_flags);
	}
	if(!(PAINT_FLAGS&SONORK_HIST_WIN_PAINT_F_PADD_PAINTED))
	{
		::FillRect(PAINT_CTX->tDC
			,&PAINT_CTX->sepaR
			,PAINT_CTX->view.sp_brush);
	}
	if(!(PAINT_FLAGS&SONORK_HIST_WIN_PAINT_F_LEFT_PAINTED))
	{
		if(view.l_margin)
		{
			::FillRect(PAINT_CTX->tDC
				,&PAINT_CTX->leftR
				,PAINT_CTX->bg_brush);

		}

		if( PAINT_CTX->l_icon != SKIN_ICON_NONE )
		{
			sonork_skin.DrawIcon(PAINT_CTX->tDC
					, PAINT_CTX->l_icon
					, PAINT_CTX->leftR.left + 1
					, PAINT_CTX->textR.top
					, ILD_NORMAL);
		}
		if( PAINT_CTX->e_icon != SKIN_ICON_NONE )
		{
			if( PAINT_CTX->r_icon != SKIN_ICON_NONE )
			{
				sonork_skin.DrawIcon(PAINT_CTX->tDC
					, PAINT_CTX->r_icon
					, PAINT_CTX->leftR.left + SONORK_HIST_WIN_LICON_SW + 2
					, PAINT_CTX->textR.top+3
					, ILD_NORMAL);
			}
			sonork_skin.DrawIcon(PAINT_CTX->tDC
					, PAINT_CTX->e_icon
					, PAINT_CTX->leftR.left + SONORK_HIST_WIN_LICON_SW + 2
					, PAINT_CTX->textR.top
					, ILD_NORMAL);
		}
		else
		if( PAINT_CTX->r_icon != SKIN_ICON_NONE )
		{
			sonork_skin.DrawIcon(PAINT_CTX->tDC
				, PAINT_CTX->r_icon
				, PAINT_CTX->leftR.left + SONORK_HIST_WIN_LICON_SW + 2
				, PAINT_CTX->textR.top
				, ILD_NORMAL);
		}
	}

	PAINT_CTX->lineR.bottom += view.t_spacing;
}

void
 TSonorkHistoryWin::DoPaintView(DWORD flags , SONORK_C_CSTR )
{
	int				vH,right;
	int				topDY;
	SONORK_CCACHE_TEXT_STATUS	tstatus;
	vH			= Height();

	// Save values initialized in context because we'll modify them
	topDY		= -PAINT_CTX->lineR.top;
	right		=  PAINT_CTX->textR.right;

	PAINT_CTX->lineR.bottom = PAINT_CTX->lineR.top;
	if( !(flags & PAINT_VIEW_F_CALC ) )
	{
		for(  PAINT_CTX->vline=0
			; PAINT_EVENT.line_no<PAINT_CTX->lines
			; PAINT_CTX->vline++
			 ,PAINT_EVENT.line_no++)
		{
			PAINT_EVENT.pEntry = cache->Get(PAINT_EVENT.line_no
					, &PAINT_CTX->str_ptr
					, NULL);
			if(PAINT_EVENT.pEntry==NULL)
			{
				PAINT_CTX->str_ptr="";
				PAINT_CTX->str_len=0;
			}
			else
			{
				PAINT_CTX->str_len=strlen(PAINT_CTX->str_ptr);
			}
			// DoPaintViewLine updates PAINT_CTX->lineR.bottom
			// and all other rectangles.. the only thing it does not
			// update is the PAINT_CTX->lineR.top
			// No clipping is necesary because we're drawing the view
			// in top-bottom direction and anything that is drawn outside
			// in a line will be overwritten by the next line
			// (The SDK says NOCLIP is somewhat faster)
			DoPaintViewLine(DT_BASE_FLAGS);

			if( PAINT_CTX->lineR.bottom >= vH )
			{
				view.visible_lines = PAINT_CTX->vline + 1;
				break;
			}
			PAINT_CTX->lineR.top = PAINT_CTX->lineR.bottom;
		}

	}
	else
	{
		int 	line_height, text_height, scan_lines;

		ClearWinUsrFlag(SONORK_HIST_WIN_F_VIEW_RECALC_PENDING);

		view.visible_lines 	= 0;

		for( PAINT_CTX->vline=0;
				PAINT_CTX->vline<SONORK_HIST_WIN_MAX_VIEW_LINES
				&& PAINT_EVENT.line_no<PAINT_CTX->lines
				; PAINT_CTX->vline++
				 ,PAINT_EVENT.line_no++
				)
		{
			PAINT_EVENT.pEntry = cache->Get(PAINT_EVENT.line_no
					, &PAINT_CTX->str_ptr
					, &tstatus);
			if(PAINT_EVENT.pEntry==NULL)
			{
				PAINT_CTX->str_ptr="";
				PAINT_CTX->str_len=0;
			}
			else
			{
				PAINT_CTX->str_len=strlen(PAINT_CTX->str_ptr);
			}

			PAINT_CTX->textR.right	= right;
			PAINT_CTX->textR.top   	= 0;
			PAINT_CTX->textR.bottom	= scan.text_height;

			::DrawText(PAINT_CTX->tDC
				, PAINT_CTX->str_ptr
				, PAINT_CTX->str_len
				, &PAINT_CTX->textR
				, DT_BASE_FLAGS|DT_CALCRECT);

			view.line[PAINT_CTX->vline].flags=0;
			scan_lines 	= PAINT_CTX->textR.bottom / scan.text_height;
			if(scan_lines > scan.max_lines )
			{
				scan_lines  = scan.max_lines;
				text_height = scan.max_lines * scan.text_height;
				view.line[PAINT_CTX->vline].r_icon=SKIN_ICON_ELLIPSES;
			}
			else
			{
				view.line[PAINT_CTX->vline].r_icon=
					tstatus==SONORK_CCACHE_TEXT_STATUS_LONG
					?SKIN_ICON_ELLIPSES
					:SKIN_ICON_NONE;
				text_height = scan_lines * scan.text_height;

				if( text_height < PAINT_CTX->textR.bottom )
				{
					scan_lines++;
					text_height += scan.text_height;
				}
			}
			line_height =  text_height + SCAN_LINE_EXTRA_HEIGHT;

			view.line[PAINT_CTX->vline].line_no 	= PAINT_EVENT.line_no;
			view.line[PAINT_CTX->vline].scan_lines	= (short)scan_lines;
			// Top is loaded without the scan.offset delta
			view.line[PAINT_CTX->vline].top	    	= (short)(PAINT_CTX->lineR.top + topDY);
			view.line[PAINT_CTX->vline].text_height	= (short)text_height;

			if( view.visible_lines == 0)
			{
				if( flags & PAINT_VIEW_F_DRAW )
				{
					PAINT_CTX->textR.right	= right;
					// Updates PAINT_CTX->lineR.bottom
					DoPaintViewLine(DT_BASE_FLAGS);

				}
				else
					PAINT_CTX->lineR.bottom = PAINT_CTX->lineR.top + line_height;

				if( PAINT_CTX->lineR.bottom >= vH )
					view.visible_lines = PAINT_CTX->vline + 1;
			}
			PAINT_CTX->lineR.top += line_height;
		}
		view.calc_lines = PAINT_CTX->vline;
		if(view.visible_lines == 0)
			view.visible_lines =view.calc_lines;
	}
	if( flags & PAINT_VIEW_F_DRAW )
	{
		if(PAINT_CTX->lineR.bottom < vH )
		{
			PAINT_CTX->lineR.left	= 0;
			PAINT_CTX->lineR.top	= PAINT_CTX->lineR.bottom;
			PAINT_CTX->lineR.bottom = vH;
			::FillRect(PAINT_CTX->tDC
				,&PAINT_CTX->lineR
				,PAINT_CTX->ViewBgBrush() );

		}
	}

}

void TSonorkHistoryWin::PaintViewLinePair(int vline1,int vline2)
{
	HDC tDC;
	if( v_hint_text != NULL)return;
	tDC=GetDC();
	StartPaintView(tDC);

	for( PAINT_CTX->vline=0
		,PAINT_EVENT.line_no=ViewOffset()
		;PAINT_EVENT.line_no<Lines()
		&& PAINT_CTX->vline < view.visible_lines
		; PAINT_CTX->vline++
		 ,PAINT_EVENT.line_no++)
	{
		if( PAINT_CTX->vline == vline1
		||  PAINT_CTX->vline == vline2)
		{
			PAINT_EVENT.pEntry = cache->Get(PAINT_EVENT.line_no
					, &PAINT_CTX->str_ptr
					, NULL);
			if(PAINT_EVENT.pEntry==NULL)
			{
				PAINT_CTX->str_ptr="";
				PAINT_CTX->str_len=0;
			}
			else
			{
				PAINT_CTX->str_len	= strlen(PAINT_CTX->str_ptr);
			}
			// Updates PAINT_CTX->lineR.bottom
			DoPaintViewLine( DT_BASE_FLAGS ); // Use clipping
			PAINT_CTX->lineR.top = PAINT_CTX->lineR.bottom;
		}
		else
			PAINT_CTX->lineR.top+=view.line[PAINT_CTX->vline].text_height + SCAN_LINE_EXTRA_HEIGHT;
	}
	EndPaintView();
	ReleaseDC( tDC );
}
void TSonorkHistoryWin::PaintView(DWORD flags,SONORK_C_CSTR pCaller)
{
	HDC	tDC;

	tDC = GetDC();
	StartPaintView( tDC );

	DoPaintView( flags , pCaller );

	EndPaintView();
	ReleaseDC( tDC );

}


void TSonorkHistoryWin::OnPaint(HDC t_dc, RECT&rect, BOOL )
{
	if( v_hint_text != NULL )
	{
		DrawHint(t_dc,rect,v_hint_text);
	}
	else
	if( cache->IsOpen() )
	{
		StartPaintView(t_dc);
		DoPaintView(TestWinUsrFlag(SONORK_HIST_WIN_F_VIEW_RECALC_PENDING)
					?PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC
					:PAINT_VIEW_F_DRAW
				, "OnPaint" );
		EndPaintView();

	}
	else
		DrawHint(t_dc,rect,"N/A");
}

bool
 TSonorkHistoryWin::OnEraseBG(HDC)
{
	return true;
}


void
 TSonorkHistoryWin::EnableSelect(BOOL v)
{
	v_selection.Clear();
	if( v )
	{
		ClearWinUsrFlag(SONORK_HIST_WIN_F_NO_SELECT);
	}
	else
	{
		SetWinUsrFlag(SONORK_HIST_WIN_F_NO_SELECT);
	}
	if(Handle())InvalidateRect(NULL,false);
}

void
 TSonorkHistoryWin::EnableDrag(BOOL v)
{
	if( v )
	{
		ClearWinUsrFlag(SONORK_HIST_WIN_F_NO_DRAG);
	}
	else
	{
		SetWinUsrFlag(SONORK_HIST_WIN_F_NO_DRAG);
	}
}

void
 TSonorkHistoryWin::EnableFocus(BOOL v)
{
	if( v )
	{
		ClearWinUsrFlag(SONORK_HIST_WIN_F_NO_FOCUS);
	}
	else
	{
		SetWinUsrFlag(SONORK_HIST_WIN_F_NO_FOCUS);
	}
	if(Handle())InvalidateRect(NULL,false);
}
void TSonorkHistoryWin::UpdateView(SONORK_C_CSTR pCaller)
{
	if( Initialized() )
	{
		PaintView(PAINT_VIEW_F_CALC|PAINT_VIEW_F_DRAW,pCaller);
		MakeLineVisible(view.focus,SONORK_VIEW_LINE_BOTTOM);
	}
}

void
 TSonorkHistoryWin::SetPaddingEx(int padding, int spacing)
{
	if(padding<0)padding=0;
	if(spacing<0)spacing=0;
	view.t_padding=padding;
	view.t_spacing=spacing;
	UpdateView("SetPadding");
}

void
 TSonorkHistoryWin::SetMarginsEx(int l_margin, int r_margin)
{
	if(l_margin<1)l_margin=1;
	if(r_margin<1)r_margin=1;

	view.l_margin = l_margin;
	view.r_margin = r_margin;
	UpdateView("SetMargins");

}

void
 TSonorkHistoryWin::SetMaxScanLines(int v)
{
	if(v<1)v=1;
	else
	if(v>MAX_SCAN_LINES)v=MAX_SCAN_LINES;
	scan.max_lines=v;
	UpdateView("SetMaxScanLines");
}


void
 TSonorkHistoryWin::ScrollPage( int dir)
{
	DWORD paint_view_flags;
	if( !view.visible_lines || !scan.lines)
	{
		ScrollLine(dir);
		return;
	}
	if( dir == -1 )
	{
		DWORD view_offset_delta;
		int	  new_scan_offset;
		DWORD aHeight;
		DWORD lHeight;
		DWORD vHeight;

		// Page Up

		if( ViewOffset() == 0)
		{
			ScrollLine(dir);
			return;
		}

		if( ViewOffset() > SONORK_HIST_WIN_MAX_VIEW_LINES-1 )
			view_offset_delta = SONORK_HIST_WIN_MAX_VIEW_LINES-1;
		else
			view_offset_delta = ViewOffset();


		aHeight		 = (1+scan.offset) * scan.text_height + SCAN_LINE_EXTRA_HEIGHT;
		view.offset -= view_offset_delta ;
		scan.offset  = 0;
		PaintView(PAINT_VIEW_F_CALC,"PageUp");
		vHeight=Height();
		while( view_offset_delta )
		{
			view_offset_delta--;
			lHeight = view.line[view_offset_delta].text_height + SCAN_LINE_EXTRA_HEIGHT;
			aHeight += lHeight;
			if( aHeight > vHeight )
			{
				new_scan_offset = (aHeight - vHeight)/scan.text_height;
				if( new_scan_offset > view.line[view_offset_delta].scan_lines )
					new_scan_offset = view.line[view_offset_delta].scan_lines-1;
				break;
			}
		}
		view.offset += view_offset_delta ;
		scan.offset  = new_scan_offset;
		paint_view_flags = PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC;
	}
	else
	if( dir == 1 )
	{
		// Page Down
		view.offset = view.line[view.visible_lines-1].line_no;
		scan.offset = 0;
		paint_view_flags = PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC;
	}
	else
		return;

	PaintView(paint_view_flags,"ScrollPage");
	UpdateScrollBar();
}

void
 TSonorkHistoryWin::ScrollLine( int dir)
{
	DWORD paint_view_flags;
	bool  update_scroll_bar;
	if( !view.calc_lines )return;
	if( dir == -1 )
	{
		// Line Up
		if( scan.offset )
		{
			scan.offset--;
			update_scroll_bar=false;
		}
		else
		{
			if(!ViewOffset())return;
			view.offset--;
			PaintView(PAINT_VIEW_F_CALC,"LineUp");
			scan.offset = view.line[0].scan_lines-1;
			update_scroll_bar=true;
		}
		paint_view_flags = PAINT_VIEW_F_DRAW;
	}
	else
	if(dir==1)
	{
		// Line Down
		if( scan.offset < view.line[0].scan_lines-1 )
		{
			scan.offset++;
			paint_view_flags = PAINT_VIEW_F_DRAW;
			update_scroll_bar=false;
		}
		else
		{
			if(ViewOffset() >= Lines() - 1)
				return;
			scan.offset=0;
			view.offset++;
			paint_view_flags = PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC;
			update_scroll_bar=true;
		}
	}
	else
		return;
	PaintView(paint_view_flags,"ScrollLine");
	if(update_scroll_bar)
		UpdateScrollBar();
}

bool
 TSonorkHistoryWin::OnVScroll(int code , int value, HWND )
{
	switch( code )
	{
		case SB_PAGEDOWN:
			ScrollPage(1);
			break;

		case SB_LINEDOWN:
			ScrollLine(1);
		break;

		case SB_PAGEUP:
			ScrollPage(-1);
		break;

		case SB_LINEUP:
			ScrollLine(-1);
		break;

		case SB_THUMBTRACK:
			if( Lines() )
			{
				view.offset = (DWORD)value;
				scan.offset = 0;
				if( ViewOffset() >= Lines() )
					view.offset = Lines()-1;
				PaintView(PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC,"ScrollLine");
				UpdateScrollBar();
			}
		break;

		default:
			return true;
	}
	return true;
}

void
 TSonorkHistoryWin::UpdateScrollBar()
{
	SCROLLINFO si;
	si.cbSize	= sizeof(si);
	si.fMask    = SIF_DISABLENOSCROLL|SIF_RANGE|SIF_POS;
	si.nMin		= 0;
	si.nMax		= (int)Lines();
	si.nPos		= (int)ViewOffset();
	SetScrollInfo(Handle(), SB_VERT , &si, true);

	cache->SetValue( SONORK_CCACHE_VIEW_OFFSET, ViewOffset());
}

bool
 TSonorkHistoryWin::OnAppEvent(UINT event, UINT , void*)
{
	if( event == SONORK_APP_EVENT_SKIN_COLOR_CHANGED )
	{
		InvalidateRect(NULL,true);
		return true;
	}
	return false;

}
TSonorkHistoryWin::~TSonorkHistoryWin()
{

}
TSonorkHistoryWin::TSonorkHistoryWin(TSonorkWin*parent
		, TSonorkHistoryWinCallbackPtr		cb_ptr
		, void* 				cb_tag
		, TSonorkCCache*			p_msg_console)
	:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		,0)
{
	SetEventMask(SONORK_APP_EM_SKIN_AWARE);
	paint_event.event = SONORK_HIST_WIN_EVENT_LINE_PAINT;
	paint_event.info.paint_line.context  = NULL;

	cb.ptr	= cb_ptr;
	cb.tag	= cb_tag;
	cache	= p_msg_console;

	view.calc_lines	= view.visible_lines = scan.offset = 0;
	scan.max_lines	= SONORK_HIST_WIN_DEFAULT_MAX_SCAN_LINES;
	view.focus	= SONORK_INVALID_INDEX;
	v_hint_text	= NULL;
}
// ----------------------
// Clipboard

void
 TSonorkHistoryWin::CopyToClipboard(LPARAM tag)
{
	TSonorkListIterator 	I;
	void		*	ptr;
	HANDLE			hData;
	TSonorkHistoryWinEvent	EV;
	TSonorkDynData		dd;

	EV.event=SONORK_HIST_WIN_EVENT_GET_TEXT;
	EV.info.get_text.type=SONORK_HIST_WIN_TEXT_GENERIC;
	EV.info.get_text.tag =tag;
	EV.info.get_text.data=&dd;

	if(!SelectionActive())
	{
		EV.pEntry = GetFocused( NULL, &EV.line_no );
		if( EV.pEntry == NULL )return;
		cb.ptr(cb.tag,&EV);
	}
	else
	{
		InitEnumSelection(I);
		while( (EV.line_no = EnumNextSelection(I)) != SONORK_INVALID_INDEX)
		{
			EV.pEntry = cache->Get(EV.line_no
						, NULL
						, NULL
						, SONORK_CCACHE_SD_RANDOM);
			if( EV.pEntry == NULL )continue;
			if(dd.DataSize())dd.AppendStr("\r\n",false);
			cb.ptr(cb.tag,&EV);
		}
	}
	if(!dd.DataSize())return;

	if( !OpenClipboard(Handle()) )return;
	dd.AppendStr("\r\n",true);
	hData=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,dd.DataSize());
	while(hData)
	{
		ptr=GlobalLock(hData);
		if(!ptr)break;
		memcpy(ptr,dd.Buffer(),dd.DataSize());
		GlobalUnlock(ptr);
		if( !EmptyClipboard() )break;
		SetClipboardData(CF_TEXT,hData);
		hData=NULL;
	}
	if(hData)
		GlobalFree(hData);
	CloseClipboard();
}

void
 TSonorkHistoryWin::SetDefColors(SKIN_COLOR c)
{
	view.color=c;
	if(Handle())InvalidateRect(NULL,true);
}


TSonorkCCacheEntry*TSonorkHistoryWin::GetFocused(SONORK_C_CSTR *p_str, DWORD*p_line_no)
{
	if( IsOpen() )
	{
		TSonorkCCacheEntry* C;
		if( (C=cache->Get( view.focus , p_str , NULL )) != NULL )
			if( p_line_no)
				*p_line_no=view.focus;
		return C;
	}
	return NULL;
}

SONORK_VIEW_LINE_VISIBILITY
	TSonorkHistoryWin::PaintViewLine( DWORD line_no ) 
{
	SONORK_VIEW_LINE_VISIBILITY lv;
	lv = GetLineVisibility( line_no );
	if( lv > SONORK_VIEW_LINE_NOT_VISIBLE )
	{
		PaintViewLinePair( line_no - view.offset, SONORK_INVALID_INDEX );
	}
	return lv;

}

SONORK_VIEW_LINE_VISIBILITY
	TSonorkHistoryWin::GetLineVisibility( DWORD line_no ) const
{
	if(view.visible_lines>0)
	{

		if( line_no >= view.line[0].line_no
			&&  line_no <= view.line[view.visible_lines-1].line_no)
		{
			int vline = line_no - view.offset;
			if(vline == 0)
			{
				if(scan.offset)
					return SONORK_VIEW_LINE_PARTIALLY_VISIBLE;
			}
			else
			{
				int topDY = scan.offset*scan.text_height;
				if(   view.line[vline].top
					+ view.line[vline].text_height
					+ SCAN_LINE_EXTRA_HEIGHT
					- topDY
					> Height() )
				{
					return SONORK_VIEW_LINE_PARTIALLY_VISIBLE;
				}
			}
			return SONORK_VIEW_LINE_VISIBLE;
		}
	}
	return SONORK_VIEW_LINE_NOT_VISIBLE;
}
bool TSonorkHistoryWin::MakeLineVisible( DWORD line_no , SONORK_VIEW_LINE_POS pos)
{
	if( line_no>=Lines() || !view.visible_lines)
	{
		// Cannot make line visible
		return false;
	}
	if( GetLineVisibility( line_no ) >= SONORK_VIEW_LINE_VISIBLE)
		return false;
	if( pos == SONORK_VIEW_LINE_TOP || line_no==0)
	{
		view.offset = line_no;
		scan.offset = 0;
	}
	else
	{
		DWORD new_view_delta;
		int   new_scan_offset;
		int   aHeight;
		int   lHeight;
		int   vHeight;

		if( line_no > SONORK_HIST_WIN_MAX_VIEW_LINES-1 )
			new_view_delta  = SONORK_HIST_WIN_MAX_VIEW_LINES-1;
		else
			new_view_delta  = line_no;


		view.offset  = line_no - new_view_delta;
		scan.offset  = new_scan_offset = 0;
		PaintView(PAINT_VIEW_F_CALC,"MkVisible");
		aHeight =   0;//SCAN_LINE_EXTRA_HEIGHT;
		vHeight	=	Height();
		while( new_view_delta )
		{
			lHeight = view.line[new_view_delta].text_height + SCAN_LINE_EXTRA_HEIGHT;
			aHeight += lHeight;
			if( aHeight >= vHeight )
			{
				aHeight -= vHeight;
				new_scan_offset = aHeight / scan.text_height;
				if(new_scan_offset * scan.text_height < aHeight)
					new_scan_offset++;

				if( new_scan_offset >= view.line[new_view_delta].scan_lines )
				{
					// Max scan line is view.line[new_view_delta].scan_lines
					new_view_delta++;
					new_scan_offset = 0;view.line[new_view_delta].scan_lines-1;
				}
				break;

			}
			new_view_delta--;
		}
		view.offset += new_view_delta ;
		scan.offset  = new_scan_offset;
	}
	PaintView(PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC,"MkLineVisible");
	UpdateScrollBar();
	return true;
}

DWORD
 TSonorkHistoryWin::_SetFocusedLine(DWORD line_no, DWORD ev_flags , POINT* ev_point )
{
	DWORD	old_focused_line;
	DWORD	r_flags;

	if( !FocusEnabled() )
		return SONORK_VIEW_SET_FOCUS_RF_DISABLED;

	if( !Lines() )
	{
		if(view.focus!=SONORK_INVALID_INDEX)
			r_flags	   = SONORK_VIEW_SET_FOCUS_RF_FOCUS_CHANGED;
		else
			r_flags    = 0;
		view.focus = SONORK_INVALID_INDEX;
	}
	else
	{
		if( line_no>=Lines() )
		{
			r_flags = SONORK_VIEW_SET_FOCUS_RF_CORRECTED;
			line_no = Lines()-1;
		}
		else
			r_flags = 0;

		old_focused_line = view.focus;
		if( line_no != view.focus )
		{
			r_flags		|= SONORK_VIEW_SET_FOCUS_RF_FOCUS_CHANGED;
			ev_flags	|= SONORK_HIST_WIN_FLAG_FOCUS_CHANGED;
			view.focus 	= line_no;
			cache->SetValue( SONORK_CCACHE_FOCUS_LINE , view.focus );
		}

		if( GetLineVisibility( view.focus ) < SONORK_VIEW_LINE_VISIBLE )
		{
			if( MakeLineVisible( line_no
				, line_no<=view.offset
					?SONORK_VIEW_LINE_TOP
					:SONORK_VIEW_LINE_BOTTOM
				) )
			{
				r_flags|=SONORK_VIEW_SET_FOCUS_RF_OFFSET_CHANGED
					|SONORK_VIEW_SET_FOCUS_RF_VIEW_PAINTED;
			}
		}


		if( !(r_flags&SONORK_VIEW_SET_FOCUS_RF_VIEW_PAINTED)
		  && view.visible_lines != 0
		  && old_focused_line != view.focus)
		{
			r_flags|=SONORK_VIEW_SET_FOCUS_RF_VIEW_PAINTED;
			PaintViewLinePair (
					old_focused_line - ViewOffset()
				,	view.focus - ViewOffset());
		}
	}

	if(	(r_flags&SONORK_VIEW_SET_FOCUS_RF_FOCUS_CHANGED)
	||	(ev_flags&SONORK_HIST_WIN_FLAG_FORCE_EVENT))
	{
		TSonorkHistoryWinEvent	EV;
		r_flags|=SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED;
		EV.event 		= SONORK_HIST_WIN_EVENT_LINE_CLICK;

		EV.info.click.flags	= ev_flags;
		EV.line_no 		= view.focus;
		if(ev_point)
		{
			EV.info.click.point.x = ev_point->x;
			EV.info.click.point.y = ev_point->y;
		}
		cb.ptr(cb.tag,&EV);
	}
	return r_flags;
}

DWORD
 TSonorkHistoryWin::SetFocusedLine(DWORD line_no, bool force_event)
{
	return
		_SetFocusedLine(line_no
		,force_event
		?SONORK_HIST_WIN_FLAG_INVOKED|SONORK_HIST_WIN_FLAG_FORCE_EVENT
		:SONORK_HIST_WIN_FLAG_INVOKED
		,NULL);
}

bool
 TSonorkHistoryWin::SetViewOffset(DWORD new_view_offset)
{
	if(!Lines())
		return false;
	if( new_view_offset >= Lines() )
		new_view_offset = Lines() -1;
	if( new_view_offset == ViewOffset())
		return false;
	view.offset = new_view_offset;
	UpdateScrollBar();
	InvalidateRect(NULL,false);
	return true;
}



bool TSonorkHistoryWin::OnBeforeCreate(TSonorkWinCreateInfo*CI)
{
	CI->style|=WS_VSCROLL;//|WS_BORDER;
//	CI->ex_style|=WS_EX_CLIENTEDGE;
	CI->ex_style|=WS_EX_STATICEDGE;
	return true;
}
void
 TSonorkHistoryWin::OnAfterCreate()
{
	PaintView(PAINT_VIEW_F_CALC, "OnAfterCreate()");
	if(view.focus!=SONORK_INVALID_INDEX )
		SetFocusedLine(view.focus, true);
}
LRESULT
 TSonorkHistoryWin::OnGetDlgCode(MSG*)
{
	return DLGC_WANTARROWS;
}

bool
 TSonorkHistoryWin::OnKeyDown(bool ,WPARAM wParam,LPARAM)
{
	switch( wParam )
	{
		case VK_UP:
			FocusLineUp();
			break;
		case VK_DOWN:
			FocusLineDown();
			break;
		case VK_PRIOR:
			FocusPageUp();
			break;
		case VK_NEXT:
			FocusPageDown();
			break;
		default:
			return false;
	}
	return true;
}
bool
 TSonorkHistoryWin::OnCreate()
{
	view.color	= SKIN_COLOR_MSG_VIEW;
//	view.bg_color	= 0xff0000;
//	view.sp_color	= 0x0000ff;
	
	view.l_margin	= DEFAULT_TEXT_LEFT_MARGIN;
	view.r_margin	= DEFAULT_TEXT_RIGHT_MARGIN;
	view.t_padding	= DEFAULT_TEXT_PADDING;
	view.t_spacing	= DEFAULT_TEXT_SPACING;

	{
		HDC tDC;
		SIZE sz;
		tDC = GetDC();
		SaveDC( tDC );
		SelectObject( tDC, sonork_skin.Font(SKIN_FONT_CONSOLE)  );
		::GetTextExtentPoint32(tDC,"Ay",2,&sz);
		scan.text_height = sz.cy;
		RestoreDC( tDC , -1);
		ReleaseDC( tDC);
	}
	if(cache->IsOpen())
	{
		view.focus	 = cache->GetValue( SONORK_CCACHE_FOCUS_LINE );
		if( view.focus >= Lines() && view.focus!=SONORK_INVALID_INDEX)
		{
			view.focus=SONORK_INVALID_INDEX;
			cache->SetValue( SONORK_CCACHE_FOCUS_LINE , SONORK_INVALID_INDEX);
		}
		view.offset	 = cache->GetValue( SONORK_CCACHE_VIEW_OFFSET );
		if( ViewOffset() >= Lines() )
		{
			view.offset = 0;
			cache->SetValue( SONORK_CCACHE_VIEW_OFFSET , 0);
		}
		UpdateScrollBar();
	}


	return true;
}
void TSonorkHistoryWin::AfterClear()
{
	view.focus 	= SONORK_INVALID_INDEX;
	view.offset = scan.offset = 0;
	v_selection.Clear();
	cache->SetValue( SONORK_CCACHE_FOCUS_LINE , view.focus);
	UpdateScrollBar();
	InvalidateRect(NULL,false);
	if( FocusEnabled() )
	{
		TSonorkHistoryWinEvent	EV;
		EV.event		= SONORK_HIST_WIN_EVENT_LINE_CLICK;
		EV.info.click.flags	= SONORK_HIST_WIN_FLAG_INVOKED|SONORK_HIST_WIN_FLAG_FOCUS_CHANGED;
		EV.line_no 		= view.focus;
		cb.ptr(cb.tag,&EV);
	}
}

void TSonorkHistoryWin::AfterAdd()
{
	PaintView(PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC,"AfterAdd()");
	UpdateScrollBar();
}
SONORK_RESULT TSonorkHistoryWin::Add(TSonorkCCacheEntry& CL,DWORD*line_no)
{
	SONORK_RESULT result;
	result = cache->Add(CL,line_no);
	if( result == SONORK_RESULT_OK )
		AfterAdd();
	return result;
}
SONORK_RESULT TSonorkHistoryWin::SetDatIndex(DWORD line_no , DWORD dat_index)
{
	SONORK_RESULT result;
	result = cache->SetDatIndex(line_no, dat_index );
	if( result == SONORK_RESULT_OK )
	{
		PaintView(PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC,"SetDatIndex");
	}
	return result;
}
SONORK_RESULT TSonorkHistoryWin::Set(DWORD line_no,TSonorkTag& tag,DWORD *ext_index)
{
	SONORK_RESULT result;
	result = cache->Set(line_no, tag,ext_index);
	if( result == SONORK_RESULT_OK )
	{
		line_no -= ViewOffset();
		PaintViewLinePair(line_no,SONORK_INVALID_INDEX);
	}
	return result;
}

void TSonorkHistoryWin::OnLButtonDown(UINT ,int x,int y)
{
	OnMouseDown(x, y, 0);
}
void TSonorkHistoryWin::OnRButtonDown(UINT ,int x,int y)
{
	OnMouseDown(x, y, SONORK_HIST_WIN_FLAG_RIGHT_CLICK);
}

void	TSonorkHistoryWin::GenSelChangeEvent()
{
	TSonorkHistoryWinEvent	EV;
	EV.event 	= SONORK_HIST_WIN_EVENT_SEL_CHANGE;
	EV.line_no 	= view.focus;
	cb.ptr(cb.tag,&EV);
}

int	 TSonorkHistoryWin::GetLineAt(int x,int y,DWORD&ev_flags)
{
	int		vline,mappedY;
	if( !Lines() || !view.visible_lines )
		return -1;
	mappedY = y  + scan.offset*scan.text_height;
	for(  vline = 0
		; vline < view.visible_lines
		; vline++)
	{
		if( mappedY >= view.line[vline].top
		&&  mappedY <
			view.line[vline].top
			+ view.line[vline].text_height
			+ SCAN_LINE_EXTRA_HEIGHT)
		{
			if( ev_flags&SONORK_HIST_WIN_FLAG_RIGHT_CLICK)
				break;

			if( x > view.l_margin )
				break;
			if( x < SONORK_HIST_WIN_LICON_SW+1 )
			{
				if(view.line[vline].flags&SONORK_HIST_WIN_PAINT_F_HOT_LICON)
					ev_flags|=SONORK_HIST_WIN_FLAG_LICON_CLICK;
				break;
			}

			if(view.line[vline].r_icon != SKIN_ICON_NONE)
				ev_flags|=SONORK_HIST_WIN_FLAG_RICON_CLICK;
			break;
		}
	}
	if(vline >= view.visible_lines)
		return -1;
	return vline;
}
void TSonorkHistoryWin::OnLButtonDblClk(UINT ,int x,int y)
{
	if( view.focus != SONORK_INVALID_INDEX)
	if( !SonorkApp.IsSelectKeyDown() )
	{
		TSonorkHistoryWinEvent	EV;
		int			vline;
		DWORD	ev_flags=SONORK_HIST_WIN_FLAG_DOUBLE_CLICK;

		vline = GetLineAt(x,y,ev_flags);
		if( vline<0 )return;

		// Only report double click if line being double-clicked is the
		// focused one (this should always be the case as the first click
		//  will generate an OnMouseDown() event... but no harm to double-check)
		if( view.line[vline].line_no == view.focus )
		{
			EV.event  		= SONORK_HIST_WIN_EVENT_LINE_CLICK;
			EV.pEntry		= NULL;
			EV.line_no		= view.focus;
			EV.info.click.flags     = ev_flags;
			EV.info.click.point.x	= x;
			EV.info.click.point.y	= y;
			ClientToScreen( &EV.info.click.point );
			cb.ptr(cb.tag,&EV);

		}
	}
}
void
 TSonorkHistoryWin::OnMouseDown(int x, int y, DWORD ev_flags)
{
	DWORD			line_no;
	DWORD			focus_flags;
	int			vline;
	BOOL			no_drag;
	POINT			point;
	TSonorkCCacheEntry *	pE;
	TSonorkHistoryWinEvent	EV;

	if( v_hint_text != NULL )
	{
		EV.event = SONORK_HIST_WIN_EVENT_HINT_CLICK;
		cb.ptr(cb.tag,&EV);
		return;
	}
	vline = GetLineAt(x,y,ev_flags);
	if( vline<0 )
		return;


	line_no = view.line[vline].line_no;
	pE	= cache->Get( line_no , NULL , NULL );
	if(pE==NULL)return;
	EV.pEntry = NULL;

	if(SonorkApp.IsSelectKeyDown()
	   && SelectEnabled()
	   &&!(ev_flags&(SONORK_HIST_WIN_FLAG_LICON_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK)))
	{
		ev_flags|=SONORK_HIST_WIN_FLAG_SEL_CHANGED;
	}

	no_drag   = !DragEnabled()
		||(ev_flags&	(SONORK_HIST_WIN_FLAG_SEL_CHANGED
				|SONORK_HIST_WIN_FLAG_RIGHT_CLICK
				|SONORK_HIST_WIN_FLAG_LICON_CLICK
				|SONORK_HIST_WIN_FLAG_RICON_CLICK));

	if( ev_flags&SONORK_HIST_WIN_FLAG_SEL_CHANGED )
		v_selection.Toggle( line_no );

	// Load click point coordinates because _FocusLine() may generate
	// a LINE_CLICK event wich requires the point coordinates if either
	// one of the OPEN_CLICK, RIGHT_CLICK flags are set.
	point.x = x;
	point.y = y;
	ClientToScreen( &point );

	focus_flags = _SetFocusedLine( line_no , ev_flags , &point);


	if( ev_flags&(SONORK_HIST_WIN_FLAG_RIGHT_CLICK
			|SONORK_HIST_WIN_FLAG_LICON_CLICK
			|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
	{
		if( !(focus_flags & SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED) )
		{
			focus_flags|=SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED;
			// Reload click point because
			// PaintViewLine() overwrites v_event
			EV.event 		= SONORK_HIST_WIN_EVENT_LINE_CLICK;
			EV.line_no 		= line_no;
			EV.info.click.flags	= ev_flags;
			EV.info.click.point.x 	= point.x;
			EV.info.click.point.y 	= point.y;
			cb.ptr(cb.tag,&EV);
		}
	}

	if( ev_flags&SONORK_HIST_WIN_FLAG_SEL_CHANGED )
	{
	// If selecting, the selection change needs to be painted.
	//  if _FocusLine() did not already draw the lines, we do it now.
	// We must also generate the event for the selection change
		if( !(focus_flags & SONORK_VIEW_SET_FOCUS_RF_VIEW_PAINTED) )
		{
			PaintViewLinePair( vline, SONORK_INVALID_INDEX );
		}
		if( !(focus_flags & SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED) )
		{
			GenSelChangeEvent();
		}
	}

	if( !no_drag  )
	{
		if(DragDetect(Handle(),point))
		{
			EV.event 		= SONORK_HIST_WIN_EVENT_LINE_DRAG;
			EV.line_no 		= line_no;
			EV.info.click.flags	= ev_flags;
			EV.info.click.point.x 	= point.x;
			EV.info.click.point.y 	= point.y;
			cb.ptr(cb.tag,&EV);
		}
	}

}

void
 TSonorkHistoryWin::OnSize(UINT size_type)
{
	if(size_type==SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
	{

		SetWinUsrFlag(SONORK_HIST_WIN_F_VIEW_RECALC_PENDING);
		if(scan.text_height)
			scan.lines = Height()/scan.text_height;
		InvalidateRect(NULL,false);
	}
}

bool
 TSonorkHistoryWin::FocusNextUnreadMsg( DWORD start_line , bool force_focus_event)
{
	DWORD line_no,end_line;
	bool found=false;
	TSonorkCCacheEntry* CL;
	end_line = cache->Lines();
	if( end_line > 0)
	{
		if( start_line >= end_line ) start_line = end_line - 1;
		for( line_no=start_line ; line_no < end_line ; line_no++)
		{
			CL = cache->Get( line_no , NULL , NULL , SONORK_CCACHE_SD_RANDOM );
			if(CL)
				if( CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD )
				{
					found=true;
					break;
				}
		}
		SetFocusedLine( line_no , force_focus_event);
	}
	return found;
}

// Focuses message linked to current focus

bool
 TSonorkHistoryWin::FocusLinked(bool force_focus_event)
{
	TSonorkCCacheEntry *	CL;
	DWORD	  cur_line_no
		, found_line_no;

	CL = GetFocused( NULL , &cur_line_no );
	if( CL == NULL )
		return false;
	if(cache->GetLinked(
			  cur_line_no
			, CL
			, found_line_no
			, NULL
			, SONORK_CCACHE_GET_LINKED_DEFAULT) == NULL )
		return false;
	SetFocusedLine( found_line_no , force_focus_event);
	return true;
}

DWORD
 TSonorkHistoryWin::FocusLineUp()
{
	if(FocusedLine() > 0 && FocusedLine() != SONORK_INVALID_INDEX)
		return SetFocusedLine( FocusedLine()-1 , false);
	return 0;
}

DWORD
 TSonorkHistoryWin::FocusLineDown()
{
	return SetFocusedLine( FocusedLine()+1 , false);
}

DWORD
 TSonorkHistoryWin::FocusPageUp()
{ return 0;}

DWORD
 TSonorkHistoryWin::FocusPageDown()
{ return 0;}

void
 TSonorkHistoryWin::AddSelection(DWORD line_no)
{
	if( line_no<Lines() )
	{
		v_selection.Add( line_no );
		PaintViewLinePair( line_no - ViewOffset(), SONORK_INVALID_INDEX );
		GenSelChangeEvent();
	}
}

void
 TSonorkHistoryWin::ToggleSelection(DWORD line_no)
{
	if( line_no<Lines() )
	{
		v_selection.Toggle( line_no );
		PaintViewLinePair( line_no - ViewOffset() , SONORK_INVALID_INDEX );
		GenSelChangeEvent();
	}
}

void	TSonorkHistoryWin::ClearSelection()
{
	if(v_selection.Active())
	{
		v_selection.Clear();
		InvalidateRect(NULL,false);
		GenSelChangeEvent();
	}
}

void
 TSonorkHistoryWin::DelPreviousLines(DWORD line_no)
{
	TSonorkHistoryWinEvent	EV;
	DWORD	offset;
	if( v_selection.Active() )
	{
		v_selection.Clear();
		GenSelChangeEvent();
	}

	for(offset=0;;)
	{
		v_selection.Clear();
		for(	 EV.line_no=0
			;EV.line_no + offset <line_no
			&& v_selection.Items()<SONORK_CCACHE_MAX_SELECTION_SIZE
			;EV.line_no++)
		{
			EV.pEntry = cache->Get(	(DWORD)EV.line_no
						, NULL
						, NULL
						, SONORK_CCACHE_SD_FORWARD);

			if( EV.pEntry != NULL )
			{
				EV.event = SONORK_HIST_WIN_EVENT_LINE_DELETE;
				if( cb.ptr(cb.tag,&EV) & SONORK_HIST_WIN_DELETE_F_ALLOW)
				{
					offset++;
					v_selection.Add(EV.line_no);
				}
			}
		}
		if( v_selection.Active() )
			cache->Del( v_selection  );
		else
			break;
	}
	AdjustViewAndFocus(0);
}

void
 TSonorkHistoryWin::DelAllLines()
{
	TSonorkHistoryWinEvent	EV;
	if( v_selection.Active() )
	{
		v_selection.Clear();
		GenSelChangeEvent();
	}
	for(;;)
	{
		v_selection.Clear();
		for(	 EV.line_no=0
			;EV.line_no<Lines()
			&& v_selection.Items()<SONORK_CCACHE_MAX_SELECTION_SIZE
			;EV.line_no++)
		{
			EV.pEntry = cache->Get(	(DWORD)EV.line_no
						, NULL
						, NULL
						, SONORK_CCACHE_SD_FORWARD);

			if( EV.pEntry != NULL )
			{
				EV.event = SONORK_HIST_WIN_EVENT_LINE_DELETE;
				if( cb.ptr(cb.tag,&EV) & SONORK_HIST_WIN_DELETE_F_ALLOW)
					v_selection.Add(EV.line_no);
			}
		}
		if( v_selection.Active() )
			cache->Del( v_selection  );
		else
			break;
	}
	AdjustViewAndFocus(0);

}

void
 TSonorkHistoryWin::AdjustViewAndFocus(DWORD new_focus)
{
	if( ViewOffset() >= cache->Lines())
	{
		if( cache->Lines() )
			view.offset = cache->Lines()-1;
		else
			view.offset = 0;
	}
	scan.offset=0;
	if( v_hint_text == NULL )
		PaintView(PAINT_VIEW_F_DRAW|PAINT_VIEW_F_CALC, "DelSelected" );
	// SetFocusedLine() will adjust focused_line if necesary
	SetFocusedLine(new_focus,true);
	UpdateScrollBar();
}

void
 TSonorkHistoryWin::DelSelectedLines()
{
	TSonorkListIterator 	I;
	TSonorkHistoryWinEvent	EV;
	TSonorkCCacheSelection	del_selection;

	if(!v_selection.Active())
	{
		if(!FocusEnabled())
			return;
		if(FocusedLine()!=SONORK_INVALID_INDEX);
			v_selection.Add(FocusedLine());
		if(!v_selection.Active())
			return;

	}

	v_selection.InitEnum(I);
	while( (EV.line_no = v_selection.EnumNext(I)) != SONORK_INVALID_INDEX)
	{
		EV.pEntry = cache->Get(	(DWORD)EV.line_no
					, NULL
					, NULL
					, SONORK_CCACHE_SD_RANDOM);

		if( EV.pEntry != NULL )
		{
			EV.event = SONORK_HIST_WIN_EVENT_LINE_DELETE;
			if( cb.ptr(cb.tag,&EV) & SONORK_HIST_WIN_DELETE_F_ALLOW)
				del_selection.Add(EV.line_no);
		}
	}
	v_selection.Clear();
	GenSelChangeEvent();
	if( del_selection.Active() )
	{
		cache->Del( del_selection  );
		AdjustViewAndFocus(view.focus);
	}
}

#define HINT_DT_FLAGS DT_NOPREFIX|DT_CENTER|DT_WORDBREAK
void TSonorkHistoryWin::DrawHint(HDC t_dc, RECT&rect, SONORK_C_CSTR msg)
{
	int l,h;
	RECT text_rect;
	HBRUSH	brush;
	TSonorkHistoryWinEvent	EV;

	SaveDC(t_dc);

	if( msg == CALLBACK_HINT_STRING )
	{
		EV.event 		= SONORK_HIST_WIN_EVENT_HINT_PAINT;
		EV.info.paint_hint.dc	= t_dc;
		EV.info.paint_hint.rect	= &rect;
		rect.left=rect.top=0;
		rect.right=Width();
		rect.bottom=Height();
		cb.ptr(cb.tag,&EV);
	}
	else
	{
		brush = CreateSolidBrush( sonork_skin.Color(view.color, SKIN_CI_BG) );
		l=strlen(msg);
		SelectObject(t_dc,sonork_skin.Font(SKIN_FONT_LARGE));
		text_rect.left	= 8;
		text_rect.right	= Width()-8*2;
		text_rect.top	= text_rect.bottom=0;
		h=::DrawText(t_dc
			,msg
			,l
			,&text_rect
			,HINT_DT_FLAGS|DT_CALCRECT);
		text_rect.right	= Width()-8*2;
		text_rect.top	= (Height()-h)/2;
		text_rect.bottom= text_rect.top+h+8;

		::SetTextColor(t_dc, sonork_skin.Color(view.color, SKIN_CI_FG) );
		::SetBkMode(t_dc,TRANSPARENT);
		::FillRect(t_dc,&rect,brush);
		::DrawText(t_dc
			,msg
			,l
			,&text_rect
			,HINT_DT_FLAGS);
		DeleteObject(brush);
	}
	RestoreDC(t_dc,-1);

}
void
 TSonorkHistoryWin::SetHintMode(SONORK_C_CSTR str, bool update_window)
{
	if(str==NULL && v_hint_text==NULL)return;
	v_hint_text = str;
	InvalidateRect(NULL,false);
	if(update_window)UpdateWindow();
}
void
 TSonorkHistoryWin::SetCallbackHintMode()
{
	if(v_hint_text==CALLBACK_HINT_STRING)return;
	v_hint_text = CALLBACK_HINT_STRING;
	InvalidateRect(NULL,false);
}
bool
 TSonorkHistoryWin::MakeLastLineVisible( SONORK_VIEW_LINE_POS pos )
{
	if( Lines() )
		return MakeLineVisible( Lines() - 1, pos);
	return false;
}

// ----------------------------------------------------------------------------
// TSonorkHistoryWinPaintCtx
// ----------------------------------------------------------------------------

COLORREF
 TSonorkHistoryWinPaintCtx::SetTextColor(COLORREF c)
{
	if(c != fg_color )
	{
		fg_color = c;
		::SetTextColor(tDC,fg_color);
	}
	return fg_color;
}

// ----------------------------------------------------------------------------

HBRUSH
 TSonorkHistoryWinPaintCtx::SetLineColor(COLORREF c)
{
	if( c != bg_color )
	{
		if(bg_brush)::DeleteObject( bg_brush );
		bg_color = c;
		bg_brush = ::CreateSolidBrush( bg_color );
	}
	return bg_brush;
}

// ----------------------------------------------------------------------------

