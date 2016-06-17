#if !defined(SRKCONSOLE_H)
#define SRKCONSOLE_H

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

#if defined(SONORK_APP_BUILD)
# include "srk_dragdrop.h"

# if !defined(SRKWINCTRL_H)
#  include "srkwinctrl.h"
# endif

#endif

#if !defined(SRKHISTORYWIN_H)
# include "srkhistorywin.h"
#endif

#if !defined(IDD_MAIN)
#	include "guv2.rh"
#endif

enum SONORK_CONSOLE_BUTTON
{
  SONORK_CONSOLE_BUTTON_FIRST		=1000
, SONORK_CONSOLE_BUTTON_FIRST_CUSTOM	=SONORK_CONSOLE_BUTTON_FIRST
, SONORK_CONSOLE_BUTTON_03   		=SONORK_CONSOLE_BUTTON_FIRST
, SONORK_CONSOLE_BUTTON_02
, SONORK_CONSOLE_BUTTON_01
, SONORK_CONSOLE_BUTTON_LAST_CUSTOM	=SONORK_CONSOLE_BUTTON_01
, SONORK_CONSOLE_BUTTON_DELETE
, SONORK_CONSOLE_BUTTON_PROCESS
, SONORK_CONSOLE_BUTTON_PREVIOUS
, SONORK_CONSOLE_BUTTON_NEXT
, SONORK_CONSOLE_BUTTON_CLOSE
, SONORK_CONSOLE_BUTTON_LAST		=SONORK_CONSOLE_BUTTON_CLOSE
};
#define	SONORK_CONSOLE_BUTTONS	(SONORK_CONSOLE_BUTTON_LAST-SONORK_CONSOLE_BUTTON_FIRST+1)
enum SONORK_CONSOLE_OUTPUT_MODE
{
  SONORK_CONSOLE_OUTPUT_NONE
, SONORK_CONSOLE_OUTPUT_HISTORY
, SONORK_CONSOLE_OUTPUT_VIEW
};

enum SONORK_CONSOLE_TOOLBAR_FLAGS
{
  SONORK_CONSOLE_TBF_PROCESS		=0x00010000
, SONORK_CONSOLE_TBF_DELETE		=0x00020000
, SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS	=0x00040000
, SONORK_CONSOLE_TBF_BUTTON_01		=0x00100000
, SONORK_CONSOLE_TBF_BUTTON_02		=0x00200000
, SONORK_CONSOLE_TBF_BUTTON_03		=0x00400000
, SONORK_CONSOLE_TBM_USER		=0x0000ffff
, SONORK_CONSOLE_TBM_BUTTONS		=0x00ff0000
};

#define SONORK_CONSOLE_VKEY_WM_CHAR	0x0001
#define SONORK_CONSOLE_VKEY_CONTROL	0x0002

#define SONORK_CONSOLE_FG_COLOR		0x000000
#define SONORK_CONSOLE_BG_COLOR_I	0xf0fff0
#define SONORK_CONSOLE_BG_COLOR_O	0xffffff
#define SONORK_CONSOLE_BG_COLOR_F	0xFFC0C0
#define SONORK_CONSOLE_BG_COLOR_S	0xC0FFFF



enum SONORK_CONSOLE_EVENT
{
	SONORK_CONSOLE_EVENT_NONE
,	SONORK_CONSOLE_EVENT_HISTORY_EVENT
,	SONORK_CONSOLE_EVENT_PROCESS
,	SONORK_CONSOLE_EVENT_CLOSE_VIEW
,	SONORK_CONSOLE_EVENT_VKEY
,	SONORK_CONSOLE_EVENT_TOOLBAR_TOOL_TIP
,	SONORK_CONSOLE_EVENT_TOOLBAR_NOTIFY
,	SONORK_CONSOLE_EVENT_INPUT_RESIZED
,	SONORK_CONSOLE_EVENT_EXPORT
};

enum SONORK_CONSOLE_EXPORT_FORMAT
{
	SONORK_CONSOLE_EXPORT_NONE
,	SONORK_CONSOLE_EXPORT_TEXT
,	SONORK_CONSOLE_EXPORT_HTML
};

enum SONORK_CONSOLE_EXPORT_SECTION
{
	SONORK_CONSOLE_EXPORT_SECTION_START
,	SONORK_CONSOLE_EXPORT_SECTION_COMMENTS
,	SONORK_CONSOLE_EXPORT_SECTION_LINE
,	SONORK_CONSOLE_EXPORT_SECTION_END
};

struct TSonorkConsoleExportEvent
{
	SONORK_CONSOLE_EXPORT_FORMAT 	format;
	DWORD				flags;
	void *				tag;
	SONORK_CONSOLE_EXPORT_SECTION	section;
	FILE *				file;
	SYSTEMTIME			st;
	union
	{
		void*			ptr;
		SONORK_C_CSTR		comments;
		TSonorkCCacheEntry*     line;
	}data;
};

#if defined(SONORK_APP_BUILD)

typedef DWORD SONORK_CALLBACK TSonorkConsoleCallback(void*
			,SONORK_CONSOLE_EVENT 	pEvent
			,DWORD			pIndex
			,void*			pData);
typedef TSonorkConsoleCallback*	TSonorkConsoleCallbackPtr;

class TSonorkConsole
:public TSonorkWin
{
private:
#define	SONORK_CONSOLE_WF_ENABLE_DRAG_ACCEPT	SONORK_WIN_F_USER_01
#define	SONORK_CONSOLE_WF_IN_DRAG_LOOP		SONORK_WIN_F_USER_02

	struct unTOOLBAL{
		HWND				hwnd;
		SIZE				size;
		UINT				flags;
	}toolbar;

	struct unOUTPUT{
		HWND				hwnd;
		TSonorkWinCtrl	 		ctrl;
	}output;

	TSonorkHistoryWin*			history_win;
	SONORK_CONSOLE_OUTPUT_MODE	output_mode;
	DWORD					ignoreVKey;
	struct unCB{
		TSonorkConsoleCallbackPtr	ptr;
		void*					tag;
	}cb;


	void	SetupToolBar();


	bool OnCreate();
	void OnAfterCreate();
	void OnBeforeDestroy();
	
//	void 	OnMouseMove(UINT keys,int x,int y);
//	void 	OnLButtonUp(UINT keys,int x,int y);

	void	OnSize(UINT);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	OnToolTipText(UINT id, HWND, TSonorkWinToolTipText&TTT );
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT	OnCtlWinMsg(class TSonorkWinCtrl*,UINT,WPARAM,LPARAM);
	LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);

	bool	ProcessVKey(DWORD, DWORD );
	void	RealignControls(SONORK_C_CSTR pCaller);

	static  DWORD	SONORK_CALLBACK
		     	cbHistoryEvent(void*param,struct TSonorkHistoryWinEvent*E);
// Event Handlers

	virtual void	On_GcLineDrag(DWORD line_no);

	TSonorkCCache*	cache;

	bool
		MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry*);

	SONORK_CONSOLE_EXPORT_FORMAT
		GetExportPath(TSonorkShortString& path, SONORK_C_CSTR pKeyName, SONORK_C_CSTR pFileName);
	bool
		GetExportComments(TSonorkShortString& S, BOOL selection);
	LRESULT
		GenExportEvent(TSonorkConsoleExportEvent*);
	void	SetOutputMode(SONORK_CONSOLE_OUTPUT_MODE,DWORD flags);


public:
	TSonorkConsole(TSonorkWin*, TSonorkCCache*, TSonorkConsoleCallbackPtr cb_ptr, void *cb_tag, UINT sys_flags);
	~TSonorkConsole();

	// Returns true if an unread line was found.
	//  <line_was_focused> returns the result of focusing the found line
	//   by invoking history_win->FocusLine().
	//  <line_was_focused> will awlays be <false> if function returns <false>.
	bool
		FocusNextUnreadMsg( DWORD start_line , bool force_event)
		{  return history_win->FocusNextUnreadMsg(start_line,force_event);}

	bool
		FocusLinked(bool force_event)	// Focuses message linked to current focus
		{ return history_win->FocusLinked(force_event);}

	void
		ScrollLine(int dir );	// -1 = up, 1=down

	void
		ScrollPage(int dir);

	UINT
		GetToolBarState(SONORK_CONSOLE_BUTTON);

	void
		SetToolBarState(SONORK_CONSOLE_BUTTON,UINT);

	void
		SetToolBarIconAndStyle(SONORK_CONSOLE_BUTTON,SKIN_ICON,DWORD);

	void
		EnableToolBar(SONORK_CONSOLE_BUTTON,BOOL);

	TSonorkCCache*
		Cache()
		{ return cache; }
	void
		AssignCache(TSonorkCCache*C)
		{ cache = C; }

	bool
		IsOpen()		const
		{	return cache->IsOpen();	}

	DWORD
		Lines()			const
		{ return cache->Lines();}

	int	CacheSize()	const
		{	return (int)cache->CacheSize();	}

// View Offset

	DWORD
		ViewOffset()		const
		{ return history_win->ViewOffset();}

	void
		SetViewOffset(DWORD line_no)
		{ history_win->SetViewOffset(line_no);}

// Focused line

	DWORD	FocusedLine()	const
		{ return history_win->FocusedLine();}

	DWORD	SetFocusedLine(DWORD line_no, bool force_event)
		{ return history_win->SetFocusedLine(line_no,force_event);	}

	DWORD
		FocusLineUp()
		{ return history_win->FocusLineUp();}

	DWORD
		FocusLineDown()
		{ return history_win->FocusLineDown();}

	DWORD
		FocusPageUp()
		{ return history_win->FocusPageUp();}

	DWORD
		FocusPageDown()
		{ return history_win->FocusPageDown();}

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

	void
		DelSelectedLines();

	void
		DelPreviousLines(DWORD line_no);
		
	void
		DelAllLines();

	void	SortSelection()
			{ history_win->SortSelection();}

	void	EnableSelect(BOOL v)
			{ history_win->EnableSelect(v);}

	BOOL	SelectEnabled()
			{ return history_win->SelectEnabled();}

// Dragging

	void	EnableLineDrag(BOOL v)
		{ history_win->EnableDrag(v); }

	BOOL	LineDragEnabled() const
		{ return history_win->DragEnabled(); }

	BOOL	DragAcceptEnabled() const
		{ return TestWinUsrFlag(SONORK_CONSOLE_WF_ENABLE_DRAG_ACCEPT); }

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
		HistoryWin()
		{ return history_win; }


	void
		SetDefColors(SKIN_COLOR c)
		{ history_win->SetDefColors(c);	}

	void
		SetHintMode(SONORK_C_CSTR str, bool update_window)
		{ history_win->SetHintMode(str,update_window); }

	void
		SetCallbackHintMode()
		{ history_win->SetCallbackHintMode();}

	HWND
		OutputHandle()
		{	return output.hwnd;	}


	void	SetOutputText( SONORK_C_CSTR str )
		{ SetCtrlText(IDC_CONSOLE_OUTPUT,str); }

	SONORK_CONSOLE_OUTPUT_MODE
		GetOutputMode()		const
		{ return output_mode;}


	void
		SetToolBarFlags(UINT flags);

	UINT	GetToolBarFlags()		const
		{ return  toolbar.flags ; }

	UINT	GetToolBarUserFlags()	const
		{ return toolbar.flags&SONORK_CONSOLE_TBM_USER;	}

// Single Line

	void	SetMaxScanLines( int v )
			{ history_win->SetMaxScanLines(v);}

	void	SetMarginsEx(  int l_margin
			     , int r_margin)
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
			, SONORK_CCACHE_TEXT_STATUS*	str_status=NULL

			, SONORK_CCACHE_SCAN_DIRECTION	scan_direction=SONORK_CCACHE_SD_RANDOM)

		{ return cache->Get( line_no, str, str_status , scan_direction ); }

	TSonorkCCacheEntry*
		GetFocused(SONORK_C_CSTR*str , DWORD*line_no=NULL)
		{ return history_win->GetFocused(str,line_no);}

	SONORK_RESULT
		Set(DWORD line_no , TSonorkTag&tag , DWORD *ext_index)
		{ return history_win->Set(line_no, tag, ext_index); }

	SONORK_RESULT
		SetDatIndex(DWORD line_no , DWORD dat_index)
		{ return history_win->SetDatIndex(line_no, dat_index); }

	void
		AfterAdd()
		{ history_win->AfterAdd(); }

	void
		PaintViewLine( DWORD line_no )
		{ history_win->PaintViewLine( line_no); }

	void	Clear();


// ----------------------
// Clipboard

	void
		CopyToClipboard(LPARAM tag)
		{	history_win->CopyToClipboard(tag); }

	
// SONORK_CONSOLE_EXPORT_F_ASK_PATH is assumed if export_format is NULL
// selection is selection is active.
#define SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS		0x0001
#define SONORK_CONSOLE_EXPORT_F_ASK_PATH		0x0002
#define SONORK_CONSOLE_EXPORT_F_ADD_TIME_SUFFIX		0x0004
// SONORK_CONSOLE_EXPORT_F_SELECTION flag is output: Output always exports
// selection is selection is active.
#define SONORK_CONSOLE_EXPORT_F_SELECTION		0x1000


	bool	Export(SONORK_C_STR		pPath // must be at least SONORK_MAX_PATH
				,DWORD 		flags
				,void*		tag
				,SONORK_CONSOLE_EXPORT_FORMAT*
						export_format=NULL
				);

	void
		OpenView(DWORD flags);

	void
		CloseView();

// Used for exporting

	static  void
		SepLine( FILE *file , UINT length);

	static SONORK_C_CSTR
		WinToRGB( DWORD win_bgr, char *buffer);


};
#endif

#endif