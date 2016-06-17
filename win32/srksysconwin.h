#if !defined(SRKSYSCONWIN_H)
#define SRKSYSCONWIN_H

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


#if !defined(SRKHISTORYWIN_H)
# include "srkhistorywin.h"
#endif

#if !defined(IDD_MAIN)
#	include "guv2.rh"
#endif

class TSonorkSysConsoleWin
:public TSonorkWin
{
private:
	TSonorkHistoryWin*	history_win;
	TSonorkCCache*		cache;

	bool OnCreate();
	void OnAfterCreate();
	void OnBeforeDestroy();
	void	OnSize(UINT);
	bool	OnMinMaxInfo(MINMAXINFO*);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	BOOL	PreTranslateMessage(MSG*){return false;}

	void	RealignControls();

	static  DWORD SONORK_CALLBACK
		cbHistoryEvent(void*param,struct TSonorkHistoryWinEvent*E);
	void	PaintViewLine(const TSonorkCCacheEntry*, TSonorkHistoryWinPaintCtx*);
	void	OnLineFocused(DWORD);
	void	OnLineDblClick();
	bool	MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry*);
public:
	TSonorkSysConsoleWin(TSonorkWin*pParent,TSonorkCCache*);

	TSonorkCCache*	Cache()
			{ return cache; }

	bool	IsOpen() const
		{	return cache->IsOpen();	}

	DWORD	Lines()	const
		{ return cache->Lines();}

	int	CacheSize()	const
		{	return (int)cache->CacheSize();	}

// View Offset

	DWORD	ViewOffset()	const
		{ return history_win->ViewOffset();}

	void	SetViewOffset(DWORD line_no)
		{ history_win->SetViewOffset(line_no);}

// Focused line

	DWORD	FocusedLine()	const
		{ return history_win->FocusedLine();}

	DWORD	SetFocusedLine(DWORD line_no, bool force_event)
		{ return history_win->SetFocusedLine(line_no,force_event);	}

	DWORD	FocusLineUp()
		{ return history_win->FocusLineUp();}

	DWORD	FocusLineDown()
		{ return history_win->FocusLineDown();}

	void 	FocusPageUp()
		{ history_win->FocusPageUp();}

	void 	FocusPageDown()
		{ history_win->FocusPageDown();}

	void	EnableFocus(BOOL v)
		{ history_win->EnableFocus(v);}

	BOOL	FocusEnabled()
		{ return history_win->FocusEnabled();}

// Selection

	const	TSonorkCCacheSelection& Selection() const
		{ return	history_win->Selection(); }

	DWORD	SelectedLines() const
		{ return	history_win->Selection().Items(); }

	bool	SelectionActive()	const
		{ return history_win->SelectionActive(); }

	void	InitEnumSelection(TSonorkListIterator& I) const
		{ history_win->InitEnumSelection(I);}

	DWORD	EnumNextSelection(TSonorkListIterator& I) const
		{ return history_win->EnumNextSelection(I);}

	void	AddSelection(DWORD line_no)
		{ history_win->AddSelection(line_no); }

	void	ToggleSelection(DWORD line_no)
		{ history_win->ToggleSelection(line_no); }

	void	ClearSelection()
		{ history_win->ClearSelection(); }

	bool	IsLineSelected(DWORD line_no)	const
		{ return history_win->IsLineSelected(line_no); }

	void	SelectToggleFocused()
		{ history_win->SelectToggleFocused(); }

	void	DelSelectedLines();

	void	SortSelection()
		{ history_win->SortSelection();}

	void	EnableSelect(BOOL v)
		{ history_win->EnableSelect(v);}

	BOOL	SelectEnabled()
			{ return history_win->SelectEnabled();}

// Dragging

	void	EnableLineDrag(BOOL v)
		{	history_win->EnableDrag(v); }

	BOOL	LineDragEnabled()
		{ return history_win->DragEnabled(); }

// Line Visibility

	bool	MakeLineVisible(DWORD line_no, SONORK_VIEW_LINE_POS pos = SONORK_VIEW_LINE_BOTTOM)
		{ return history_win->MakeLineVisible(line_no, pos);}

	bool	MakeLastLineVisible(SONORK_VIEW_LINE_POS pos = SONORK_VIEW_LINE_BOTTOM)
		{ return history_win->MakeLastLineVisible(pos);}

	SONORK_VIEW_LINE_VISIBILITY
			GetLineVisibility(DWORD line_no)	const
		{ return history_win->GetLineVisibility(line_no);}

// GUI
	TSonorkHistoryWin*
			HistoryWin(){	return history_win; }


	void	SetDefColors(SKIN_COLOR c)
		{ history_win->SetDefColors(c);	}

	void	SetHintMode(SONORK_C_CSTR str, bool update_window)
		{ history_win->SetHintMode(str,update_window); }


// Single Line

	void	SetMaxScanLines( int v )
		{ history_win->SetMaxScanLines(v);}

	void	SetMarginsEx(  int l_margin, int r_margin)
		{ history_win->SetMarginsEx(l_margin,r_margin);}

	void	SetPaddingEx(int padding, int spacing)
		{ history_win->SetPaddingEx(padding,spacing); }

	int 	GetMaxScanLines() const
		{ return history_win->GetMaxScanLines();}

// Lines

	SONORK_RESULT
			Add(TSonorkCCacheEntry&CH,DWORD *line_no=NULL)
			{	return history_win->Add(CH,line_no); }

	TSonorkCCacheEntry*
		Get(DWORD line_no
			, SONORK_C_CSTR* str
			, SONORK_CCACHE_SCAN_DIRECTION scan_direction=SONORK_CCACHE_SD_RANDOM)
		{ return cache->Get(line_no, str, NULL , scan_direction); }

	TSonorkCCacheEntry*
		GetFocused(SONORK_C_CSTR*str , DWORD*line_no=NULL)
		{ return history_win->GetFocused(str,line_no);}

	SONORK_RESULT
		Set(DWORD line_no , TSonorkTag&tag , DWORD *ext_index)
		{ return history_win->Set(line_no, tag, ext_index); }

	SONORK_RESULT
		SetDatIndex(DWORD line_no , DWORD dat_index)
		{ return history_win->SetDatIndex(line_no, dat_index); }

	void	AfterAdd()
		{ history_win->AfterAdd(); }

//	void 	PaintViewLine( DWORD line_no )			{ history_win->PaintViewLine( line_no); }

	void	Clear();
	void	UpdateSkin();

	BOOL
		FocusNextUnread(BOOL bring_to_front);

};

#endif