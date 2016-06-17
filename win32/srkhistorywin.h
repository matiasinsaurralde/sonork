#if !defined(SRKHISTORYWIN_H)
#define SRKHISTORYWIN_H

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

#include "srk_ccache.h"

#if !defined(SRKSKIN_H)
# include "srkskin.h"
#endif

// ----------------------------------------------------------------------------
// HISTORY WIN EVENTS
// ----------------------------------------------------------------------------
// All events pass a TSonorkHistoryWinEvent* <E> as parameter (see below)
// Depending on which event is fired, some members/methods of <E>
//  are valid or invalid. The Event() member, which returns the
//  SONORK_HISTORY_WIN_EVENT fired, is always valid.

// ---------------------------------
// SONORK_HIST_WIN_EVENT_LINE_PAINT
//  A line is being painted.
// Members:
//  LineNo	: FILE line number (*1)
//  Line	: TSonorkCache's Entry for the line (TSonorkCCacheEntry*)
//  ClickFlags	: *INVALID*
//  ClickPoint  : *INVALID*
//  PaintContext: (Read/Write) Paint context (see below for details)
// Return values:
//  none. The handler must modify the <flags> of the PaintContext
//   if it does custom painting on the context

// ---------------------------------
// SONORK_HIST_WIN_EVENT_LINE_CLICK
//  A line has been clicked or the focus has changed.
//  If the click makes the selection change, the SEL_CHANGED flag will
//  be set and a separate SEL_CHANGE event will NOT be generated.
// Members:
//  LineNo	: FILE line number (*1)
//  Line	: *INVALID*
//  ClickFlags	: A combination of SONORK_HISTORY_WIN_CLICK_FLAGS
//		: FOCUS_CHANGED	: The focus has changed from one line to another
//		: SEL_CHANGED	: The selection was changed because of the click
//		: INVOKED	: The event was generated because a function
//				  was invoked. (i.e. SetFocusedLine )

//  ClickPoint  : Valid if ClickFlags() have one the
//		: DOUBLE_CLICK, RIGHT_CLICK or OPEN_CLICK flags
//		: undefined otherwise. Screen coordinates.
// PaintContext	: *INVALID*
// Return values: none.

// ---------------------------------
// SONORK_HIST_WIN_EVENT_LINE_DRAG
//  A line is being dragged.
// Valid members:
//  LineNo	: FILE line number (*1)
//  Line	: *INVALID*
//  ClickFlags	: *INVALID*
//  ClickPoint  : Point of drag (Screen coordinates)
// PaintContext	: *INVALID*
// Return values: none.

// ---------------------------------
// SONORK_HIST_WIN_EVENT_LINE_DELETE
//  The current line is to be deleted
// Valid members:
//  LineNo	: FILE number for current focused line.
//  Line	: TSonorkCache's Entry for the line (TSonorkCCacheEntry*)
//  ClickFlags	: *INVALID*
//  ClickPoint  : *INVALID*
// PaintContext	: *INVALID*
// Return values:
// A combination of SONORK_HISTORY_WIN_DELETE_FLAGS
// ---------------------------------
// SONORK_HIST_WIN_EVENT_SEL_CHANGE
//  The current selection has changed.
//  The "Selection" methods of TSonorkHistoryWin may be
//  used to retrieve information about the current selection.
//  When the selection changed because of a line click, the
//  LINE_CLICK event is fired with the SEL_CHANGED flag set
//  in ClickFlags() and a separate SEL_CHANGE event is NOT generated.
// Valid members:
//  LineNo	: FILE number for current focused line.
//  Line	: *INVALID*
//  ClickFlags	: *INVALID*
//  ClickPoint  : *INVALID*
// PaintContext	: *INVALID*
// Return values: none.


// ---------------------------------
// (*1) About FILE and VIEW line numbers
//  The FILE line number is the position within the history file.
//  The VIEW line number is the position within the current view.
//  The VIEW line is normaly between 0 and 128, the FILE line has no limit
//  A (DWORD)-1 (or 0xffffffff) value indicates an invalid line number
//  for both VIEW and FILE line numbers.

// ---------------------------------
// SONORK_HIST_WIN_EVENT_HINT_CLICK

enum SONORK_HISTORY_WIN_EVENT
{
  SONORK_HIST_WIN_EVENT_NONE
, SONORK_HIST_WIN_EVENT_LINE_PAINT
, SONORK_HIST_WIN_EVENT_LINE_CLICK
, SONORK_HIST_WIN_EVENT_LINE_DRAG
, SONORK_HIST_WIN_EVENT_LINE_DELETE
, SONORK_HIST_WIN_EVENT_SEL_CHANGE
, SONORK_HIST_WIN_EVENT_HINT_PAINT
, SONORK_HIST_WIN_EVENT_HINT_CLICK
, SONORK_HIST_WIN_EVENT_GET_TEXT

};

// ClickFlags for the SONORK_HIST_WIN_EVENT_LINE_CLICK event
enum SONORK_HISTORY_WIN_CLICK_FLAGS
{
  SONORK_HIST_WIN_FLAG_FOCUS_CHANGED	= 0x00000001 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_DOUBLE_CLICK	= 0x00000002 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_RIGHT_CLICK	= 0x00000004 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_LICON_CLICK	= 0x00000010 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_RICON_CLICK	= 0x00000020 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_SEL_CHANGED	= 0x00000100 // LINE_CLICK
, SONORK_HIST_WIN_FLAG_INVOKED		= 0x00000200 // LINE_CLICK
// FORCE_FOCUS_EVENT: Used internally by _FocusLine() to
//  force a LINE_CLICK event even when the focus did not change
, SONORK_HIST_WIN_FLAG_FORCE_EVENT	= 0x10000000
};


// Values taken by MakeLineVisible() and MakeLastLineVisible()
enum SONORK_VIEW_LINE_POS
{
  SONORK_VIEW_LINE_TOP
, SONORK_VIEW_LINE_BOTTOM
};

// Values returned by GetLineVisibility
enum SONORK_VIEW_LINE_VISIBILITY
{
  SONORK_VIEW_LINE_NOT_VISIBLE
, SONORK_VIEW_LINE_PARTIALLY_VISIBLE
, SONORK_VIEW_LINE_VISIBLE
};
enum SONORK_HISTORY_WIN_TEXT_TYPE
{
  SONORK_HIST_WIN_TEXT_GENERIC
};

// Flags returned by SetFocusedLine() and _SetFocusedLine()
enum SONORK_VIEW_SET_FOCUS_RETURN_FLAGS
{
  SONORK_VIEW_SET_FOCUS_RF_FOCUS_CHANGED	= 0x000001
, SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED	= 0x000002
, SONORK_VIEW_SET_FOCUS_RF_OFFSET_CHANGED    	= 0x000004
, SONORK_VIEW_SET_FOCUS_RF_VIEW_PAINTED		= 0x000008
, SONORK_VIEW_SET_FOCUS_RF_CORRECTED		= 0x000010
, SONORK_VIEW_SET_FOCUS_RF_DISABLED		= 0x000020
};

// Flags used by <flags> of PaintContext() for the
//  SONORK_HIST_WIN_EVENT_LINE_PAINT event.
enum SONORK_HISTORY_WIN_PAINT_FLAGS
{
  SONORK_HIST_WIN_PAINT_F_HOT_LICON		= 0x00000001 // LINE_PAINT
// SONORK_HIST_WIN_PAINT_F_HOT_RICON		= 0x00000002 // LINE_PAINT
, SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED		= 0x00010000 // LINE_PAINT
, SONORK_HIST_WIN_PAINT_F_LINE_SELECTED		= 0x00020000 // LINE_PAINT
, SONORK_HIST_WIN_PAINT_F_LINE_PAINTED		= 0x00100000 // Line background was painted
, SONORK_HIST_WIN_PAINT_F_LEFT_PAINTED		= 0x00200000 // Left margin was painted
, SONORK_HIST_WIN_PAINT_F_TEXT_PAINTED		= 0x00400000 // Text was painted
, SONORK_HIST_WIN_PAINT_F_PADD_PAINTED		= 0x00800000 // Padding (Spacing) was painted
, SONORK_HIST_WIN_PAINT_F_LICON_PAINTED		= 0x01000000 // Left icon was painted
, SONORK_HIST_WIN_PAINT_F_RICON_PAINTED		= 0x02000000 // Right icon  was painted
// Internaly used
, SONORK_HIST_WIN_PAINT_VLINE_MASK	 	= 0x00000fff
};
enum SONORK_HISTORY_WIN_DELETE_FLAGS
{
  SONORK_HIST_WIN_DELETE_F_ALLOW		= 0x00000001
};

#define SONORK_HIST_WIN_LICON_SW	SKIN_ICON_SW
#define SONORK_HIST_WIN_LICON_SH	SKIN_ICON_SH
#define SONORK_HIST_WIN_RICON_SW	10
#define SONORK_HIST_WIN_RICON_SH	SKIN_ICON_SH

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,4)
#endif

struct TSonorkHistoryWinPaintCtx
{
friend class TSonorkHistoryWin;
private:
	HDC		tDC;
	HFONT		tFont;
	DWORD		lines;
	int		vline;
	RECT		lineR;
	RECT		leftR;
	RECT		textR;
	RECT		sepaR;
	SONORK_C_CSTR   str_ptr;
	int		str_len;
	COLORREF	fg_color;
	COLORREF	bg_color;
	HBRUSH		bg_brush;

	struct {
		HBRUSH		bg_brush;
		HBRUSH		sp_brush;
	}view;

public:
	HDC
		hDC() 	const
		{ return tDC; }

	HFONT
		hFont()	const
		{ return tFont; }

	// flags: Combination of SONORK_HISTORY_WIN_PAINT_FLAGS
	// 	  Modify this flag to change behaviour of painter
	DWORD		flags;
	SKIN_ICON	l_icon;
	SKIN_ICON	r_icon;
	SKIN_ICON	e_icon;

const	RECT*
		LineRect() const
		{ return &lineR;}

const	RECT*
		TextRect() const
		{ return &textR;}

const	RECT*	LeftRect() const
		{ return &leftR;}


	COLORREF
		SetTextColor(COLORREF c);

	HBRUSH
		SetLineColor(COLORREF c);

	HBRUSH
		ViewBgBrush() const
		{ return view.bg_brush;}

	HBRUSH
		ViewSpBrush() const
		{ return view.sp_brush;}

};

struct TSonorkHistoryWinEvent
{
friend class TSonorkHistoryWin;
private:
	DWORD				line_no;
	SONORK_HISTORY_WIN_EVENT 	event;
	TSonorkCCacheEntry*		pEntry;
	union {
		struct {
			TSonorkHistoryWinPaintCtx*	context;
		}paint_line;
		struct
		{
			DWORD		flags;
			POINT		point;
			SKIN_ICON	more_icon;
		}click;
		struct
		{
			HDC	dc;
			RECT*	rect;
		}paint_hint;
		struct
		{
			SONORK_HISTORY_WIN_TEXT_TYPE	type;
			LPARAM				tag;
			TSonorkDynData*			data;
		}get_text;
	}info;
public:
	SONORK_HISTORY_WIN_EVENT
		Event()	const
		{ return event; }

	DWORD	LineNo() const
		{ return line_no;}

const TSonorkCCacheEntry*
		Line() const
		{ return pEntry;}

	DWORD	ClickFlags() const
		{ return info.click.flags;}

	const POINT&
		ClickPoint() const
		{ return info.click.point;}

	DWORD	ClickMoreIcon() const
		{ return info.click.more_icon;}

	TSonorkHistoryWinPaintCtx*
		PaintContext()
		{ return info.paint_line.context; }

	HDC	HintDC()
		{ return info.paint_hint.dc;}

const	RECT*	HintRect()
		{ return info.paint_hint.rect;}

// Get Text
	SONORK_HISTORY_WIN_TEXT_TYPE
		GetTextType() const
		{ return info.get_text.type;	}

	LPARAM
		GetTextTag() const
		{ return info.get_text.tag;	}

	TSonorkDynData*
		GetTextData()
		{ return info.get_text.data;	}
};


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

#if defined(SONORK_APP_BUILD)
// Param is the parameter supplied by the call when TSonorkWistoryWin was constructed
typedef DWORD	(SONORK_CALLBACK TSonorkHistoryWinCallback)(void*param,TSonorkHistoryWinEvent*E);
typedef TSonorkHistoryWinCallback* TSonorkHistoryWinCallbackPtr;


#include "srkwin.h"

#define SONORK_HIST_WIN_DEFAULT_MAX_SCAN_LINES	8
#define SONORK_HIST_WIN_F_NO_FOCUS		SONORK_WIN_F_USER_01
#define SONORK_HIST_WIN_F_NO_SELECT		SONORK_WIN_F_USER_02
#define SONORK_HIST_WIN_F_NO_DRAG		SONORK_WIN_F_USER_03
#define SONORK_HIST_WIN_F_VIEW_RECALC_PENDING	SONORK_WIN_F_USER_04

class TSonorkHistoryWin
:public TSonorkWin
{
public:
	struct ViewLine
	{
		DWORD		line_no;
		short		top;
		short		text_height;
		short		scan_lines;
		short		flags;
		SKIN_ICON	r_icon;
	};



private:

#define SONORK_HIST_WIN_MAX_VIEW_LINES	80
	struct
	{
		DWORD		offset;
		DWORD		focus;
		DWORD		calc_lines;
		int			visible_lines;
		int			 l_margin
					,r_margin
					,t_padding
					,t_spacing;
		ViewLine	line[SONORK_HIST_WIN_MAX_VIEW_LINES];
		SKIN_COLOR	color;
	}view;
	struct
	{
		int		offset;
		int		text_height;
		int		lines;
		int		max_lines;
	}scan;

	TSonorkCCache*			cache;
	TSonorkHistoryWinEvent		paint_event;
	SONORK_C_CSTR			v_hint_text;
	TSonorkCCacheSelection		v_selection;
	struct {
		void*		      		tag;
		TSonorkHistoryWinCallbackPtr	ptr;
	}cb;

	void StartPaintView(HDC);
	void EndPaintView();
	void OnPaint(HDC, RECT&, BOOL);
	bool	OnEraseBG(HDC);
	void OnSize(UINT);
	bool OnBeforeCreate(TSonorkWinCreateInfo*);
	bool OnCreate();
	void OnAfterCreate();
	bool OnAppEvent(UINT event, UINT param, void*data);
	void OnLButtonDown(UINT keys,int x,int y);
	void OnLButtonDblClk(UINT keys,int x,int y);
	void OnRButtonDown(UINT keys,int x,int y);
	void OnMouseDown(int x, int y, DWORD ev_flags);
	bool OnKeyDown(bool ,WPARAM wParam,LPARAM);
	LRESULT	OnGetDlgCode(MSG*);

	bool OnVScroll(int code, int pos, HWND scroll_hwnd);


	void DrawHint(HDC, RECT&, SONORK_C_CSTR);

	// Returns combination of SONORK_VIEW_SET_FOCUS_RETURN_FLAGS
	DWORD _SetFocusedLine(DWORD line_no
			, DWORD event_flags
			, POINT*event_point);

	void UpdateScrollBar();

	void GenSelChangeEvent();


#define PAINT_VIEW_F_CALC	0x1
#define PAINT_VIEW_F_DRAW	0x2

	void PaintView(DWORD flags, SONORK_C_CSTR pCaller);
	void PaintViewLinePair(int view_line_1,int view_line_2);
	void	DoPaintView(DWORD flags, SONORK_C_CSTR pCaller);
	void		DoPaintViewLine(DWORD);
	void UpdateView(SONORK_C_CSTR pCaller);

	int	 GetLineAt(int x,int y,DWORD&ev_flags);
public:
	TSonorkHistoryWin(TSonorkWin*parent
		, 	TSonorkHistoryWinCallbackPtr	cb_ptr
		,	void* 				cb_tag
		, 	TSonorkCCache*cache
		);
	~TSonorkHistoryWin();

	// Avoid using Cache() unless absolutely necesary
	// because the interface may loose synch with the file
	// (nothing that a full repaint won't fix, but full repaints
	//  are expensive compared to the internal paint handling
	//  that TSonorkHistoryWin does)
	TSonorkCCache*
		Cache()
		{ return cache; }


	bool	IsOpen() const
		{ return cache->IsOpen(); }

	DWORD	Lines()	const
		{ return cache->Lines(); }

	int	CacheSize()  const
		{ return (int)cache->CacheSize(); }

// View Offset

	DWORD	ViewOffset() const
		{ return view.offset;}

	bool
		SetViewOffset(DWORD);

	void
		AdjustViewAndFocus(DWORD focus);


// Focused line

	DWORD
		FocusedLine()	const
		{ return view.focus; }

	// Returns combination of SONORK_VIEW_SET_FOCUS_RETURN_FLAGS
	DWORD
		SetFocusedLine(DWORD line_no, bool force_event);

	void
		SetMaxScanLines(int);

	int	GetMaxScanLines() const
		{ return scan.max_lines;}

	void
		SetMarginsEx(  int left_margin , int right_margin );

	void
		SetPaddingEx(int padding, int spacing);

	DWORD
		FocusLineUp();

	DWORD
		FocusLineDown();

	DWORD
		FocusPageUp();

	DWORD
		FocusPageDown();

	void
		EnableFocus(BOOL);

	BOOL
		FocusEnabled() const
		{ return !TestWinUsrFlag(SONORK_HIST_WIN_F_NO_FOCUS);}

	bool
		FocusNextUnreadMsg( DWORD start_line , bool force_focus_event);

		// Focuses message linked to current focus
	bool
		FocusLinked(bool force_focus_event);

// ----------------------
// Selection

	void	EnableSelect(BOOL);

	BOOL
		SelectEnabled()
			{ return !TestWinUsrFlag(SONORK_HIST_WIN_F_NO_SELECT);}

	const	TSonorkCCacheSelection&
		Selection() const
			{ return	v_selection; }

	bool	SelectionActive()	const
			{ return v_selection.Active(); }

	void	InitEnumSelection(TSonorkListIterator& I) const
			{ v_selection.InitEnum(I);}

	DWORD	EnumNextSelection(TSonorkListIterator& I) const
			{ return v_selection.EnumNext(I);}

	void
		AddSelection(DWORD line_no);

	void
		ToggleSelection(DWORD line_no);

	void
		ClearSelection();

	bool	IsLineSelected(DWORD line_no) const
			{ return v_selection.Contains(line_no); }

	void	SelectToggleFocused()
			{ ToggleSelection(FocusedLine()); }

	void
		DelSelectedLines();

	void
		DelPreviousLines(DWORD line_no);

	void
		DelAllLines();

	void	SortSelection()
			{ v_selection.Sort();}

// ----------------------
// Dragging

	void
		EnableDrag(BOOL);

	BOOL	DragEnabled()
			{ return !TestWinUsrFlag(SONORK_HIST_WIN_F_NO_DRAG);}

// ----------------------
// Line Visibility

	// MakeLineVisible and MakeLastLineVisible()
	// return true if the operation updated the affected
	// or false if the operation did not redraw any lines
	// (i.e. the lines where already visible)
	// if <make_top> is true, the list will be the first
	// line, otherwise it'll be the last line

	bool
		MakeLineVisible(DWORD, SONORK_VIEW_LINE_POS);
		
	bool
		MakeLastLineVisible(SONORK_VIEW_LINE_POS);

	SONORK_VIEW_LINE_VISIBILITY
		GetLineVisibility(DWORD)	const;

// GUI

	void
		SetDefColors( SKIN_COLOR );

	void
		SetHintMode(SONORK_C_CSTR, bool update_window); // Set to NULL to leave hint mode

	void
		SetCallbackHintMode();

// Single Line


// Lines
	SONORK_RESULT
		Add(TSonorkCCacheEntry&CH,DWORD *line_no=NULL);

	SONORK_RESULT
			Set(DWORD line_no,TSonorkTag&tag,DWORD *ext_index);

	SONORK_RESULT
			SetDatIndex(DWORD line_no , DWORD dat_index);

	TSonorkCCacheEntry*
			GetFocused(SONORK_C_CSTR*,DWORD*line_no=NULL);

	TSonorkCCacheEntry*
			Get(DWORD line_no
			, SONORK_C_CSTR* str
			, SONORK_CCACHE_TEXT_STATUS*	str_status=NULL
			, SONORK_CCACHE_SCAN_DIRECTION 	scan_direction=SONORK_CCACHE_SD_RANDOM)
			{ return cache->Get(line_no, str, str_status , scan_direction); }

// Painting/Updating
	// Call AfterAdd() if a line has been appended directly
	// to the message console bypassing TSonorkHistoryWin::Add()
	// There is no performance penalty/advantage when adding
	//  lines directly to the message console and invoking
	//  AfterAdd() afterwards.-
	void	AfterAdd();
	void	AfterClear();

	SONORK_VIEW_LINE_VISIBILITY
			PaintViewLine( DWORD line_no );

	void	ScrollLine(int);	// 1 or -1	(other values are ignored)
	void	ScrollPage(int);	// 1 or -1  (other values are ignored)

// ----------------------
// Clipboard

	void
		CopyToClipboard(LPARAM tag);
};


#endif

#endif
