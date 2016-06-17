#if !defined(SRKWIN_H)
#define SRKWIN_H

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

#include "srkwin_defs.h"

#if !defined(SONORK_APP_BUILD)
#error Include is for APPLICATION build only
#endif

#if !defined(SB_SETICON)
#	define 	SB_SETICON		(WM_USER + 15)
#endif

struct TSonorkWinToolTipText
{
	char		str[SONORK_WIN_MAX_TOOLTIP_LENGTH+4];
	GLS_INDEX	gls;
};

enum SONORK_WIN_CTRL_TEXT_FLAG
{
  SONORK_WIN_CTF_OWNER_DRAW	=0x01000000
, SONORK_WIN_CTF_BOLD		=0x02000000
, SONORK_WIN_CTF_ELLIPSIS	=0x04000000
, SONORK_WIN_CTF_BYPOSITION	=0x08000000
};
enum SONORK_WIN_CTRL_TEXT_MASKS
{
  SONORK_WIN_CTRL_TEXT_ID	=0x00ffffff
, SONORK_WIN_CTRL_TEXT_FLAGS	=0xff000000
};

struct TSonorkWinGlsEntry
{
	UINT		id;	// May OR with any SONORK_WIN_CTRL_TEXT_FLAGS
	GLS_INDEX	gls_index;
};

typedef TSonorkWinGlsEntry* TSonorkWinGlsEntries;


struct TSonorkWinCreateInfo
{
	HWND		parent;
	DWORD		style;
	DWORD		ex_style;
	HINSTANCE   hinstance;
	DWORD		resource_id;
	DWORD		sys_flags;
	DWORD		usr_flags;
};
struct TSonorkWinStartInfo
{
	struct _POS{
		POINT	pt;
		SIZE	sz;
	}pos;
	DWORD	win_flags;
	BYTE	reserved[ 64-(sizeof(_POS)+sizeof(DWORD)) ];
};

union TSonorkWinNotify
{
	LPARAM		lParam;
	NMHDR		hdr;
	TBNOTIFY	tbn;
	NMLISTVIEW	lview;
	NM_UPDOWN	updown;
	TV_DISPINFO	tdispinfo;
	NMTREEVIEW	tview;
	NMTVGETINFOTIP	ttip;
	DRAWITEMSTRUCT	draw;
	NMCUSTOMDRAW	cdraw;
	NMTVCUSTOMDRAW	tdraw;
	NMSELCHANGE	selchange;
	NMDATETIMECHANGE datetime;
	SONORK_DWORD2	tag;
	POINT		pt;
};

#define SONORK_CENTER_WIN_F_CREATE	0x0001
#define SONORK_CENTER_WIN_F_REDRAW	0x0002

// SONORK_WIN_TOOLBAR_BUTTON_FLAGS
// should be comined with cmd
// (only lower word of CMD is used as command)
enum SONORK_WIN_TOOLBAR_BUTTON_FLAGS
{
	SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR	= 0x10000000
};

// SONORK_WIN_TOOLBAR_SEPARATOR_SIZE is only used
// when the SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR flag is set
#define SONORK_WIN_TOOLBAR_SEPARATOR_SIZE(n) ( ((n)&0xff)<<16 )

struct TSonorkWinToolBarButton
{
	UINT		cmd;
	UINT		icon;
	GLS_INDEX	gls;
	DWORD		state;
	DWORD		style;
};

// -------------------------------------------------------
// GLOBAL functions

// Defined by application
extern HINSTANCE	SonorkApp_Instance();
extern void		SonorkApp_ReportWinCreate(class TSonorkWin*,bool create);
extern SONORK_C_CSTR	SonorkApp_LangString(GLS_INDEX index);


// -------------------------------------------------------
// TSonorkWin

class TSonorkWin
{
friend class	TSonorkWinCtrl;
friend class	TSonorkWin32App;

private:

	struct
	{
		TSonorkWin*		parent;
		HWND 			hwnd;
		DWORD			defFlags;
		DWORD			sysFlags;
		DWORD			usrFlags;
		DWORD			eventMask;
		RECT			rect;
	}win;


	void
		WmPaint(HDC);

	void
		WmSize(UINT,POINTS*);

	void
		WmMove(POINTS*);

	void
		WmDestroy();

	LRESULT
		WmInitDialog(WPARAM,LPARAM);

	bool
		WmCommand(UINT id, UINT notify_code, HWND sender);

	bool
		WmSysCommand(UINT id);

	LRESULT
		WmNotify(WPARAM,NMHDR*);

	bool
		WmDrawItem(DRAWITEMSTRUCT*);

	bool
		WmMeasureItem(MEASUREITEMSTRUCT*);

	void
		WmParentNotify	(int event, UINT id, LPARAM lParam);

	void
		_OnAfterCreate();



	virtual bool
		InitCreateInfo(TSonorkWinCreateInfo*);

static LRESULT CALLBACK
		Win_Proc(HWND hwnd,UINT uMsg ,WPARAM wParam ,LPARAM lParam);

static LRESULT CALLBACK
		Win_Modal_Proc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

static LRESULT CALLBACK
		Win_Modeless_Proc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

protected:
	virtual LRESULT
		WinProc(UINT uMsg ,WPARAM wParam ,LPARAM lParam);

	virtual LRESULT
		OnNotify(WPARAM,TSonorkWinNotify*){return false;}

	virtual void
		OnChildCreate(UINT, HWND){}

	virtual void
		OnChildDestroy(UINT, HWND){}

	virtual void
		OnChildLButton(UINT , int, int){}

	virtual void
		OnMove(){}

	virtual void
		OnSize(UINT){}

	virtual bool
		OnMinMaxInfo(MINMAXINFO*){return false;}

	virtual LRESULT
		OnGetDlgCode(MSG*){ return 0L; }

	virtual void
		OnMouseMove(UINT keys,int x,int y){}

	virtual void
		OnLButtonDown(UINT keys,int x,int y){}

	virtual void
		OnLButtonDblClk(UINT keys,int x,int y){}

	virtual void
		OnLButtonUp(UINT keys,int x,int y){}

	virtual void
		OnRButtonDown(UINT keys,int x,int y){}

	virtual void
		OnRButtonUp(UINT keys,int x,int y){}

	virtual void
		OnCancelMode(){}

	virtual	bool
		OnSetFocus(bool){return false;}

	virtual void
		OnToolTipText(UINT id, HWND, TSonorkWinToolTipText&TTT ){}

	virtual void
		OnShowWindow(BOOL show, int status){}

	virtual void
		OnActivate(DWORD flags,BOOL minimized){}

	virtual void
		OnMinimize(BOOL){}

	virtual bool
		OnVScroll(int code, int pos, HWND scroll_hwnd){return false;}

	virtual void
		OnPaint(HDC, RECT&, BOOL){}

	virtual bool
		OnEraseBG(HDC){return false;}

	virtual BOOL
		OnQueryClose()
		{ return false;}

	virtual bool
		OnSysCommand(UINT)
		{ return false;}

	virtual bool
		OnCommand(UINT id,HWND hwnd, UINT notify_code)
		{return false;}

	virtual bool
		OnKeyDown(bool is_sys,WPARAM,LPARAM)
		{return false;}

	virtual bool
		OnKeyUp(bool is_sys,WPARAM,LPARAM)
		{return false;}

	virtual bool
		OnChar(bool is_sys,TCHAR, LPARAM)
		{return false;}

	virtual bool
		OnMeasureItem(MEASUREITEMSTRUCT*)
		{return false;}

	virtual bool
		OnDrawItem(DRAWITEMSTRUCT*)
		{return false;}

	virtual void
		OnItemKillFocus(UINT id,HWND hwnd){}

	virtual void
		OnItemSetFocus(UINT id,HWND hwnd){}

	// If OnCtlColor is NOT processed by overloaded function
	// it *MUST* called TSonorkWin::OnCtlColor
	virtual LRESULT
		OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual void
		OnInitMenu(HMENU){}

	virtual void
		OnInitMenuPopup(HMENU ,UINT,BOOL){}


	virtual LRESULT
		OnCtlWinMsg(class TSonorkWinCtrl*,UINT,WPARAM,LPARAM);

	virtual void
		OnTimer(UINT id)
		{}

	// These are methods are called ONLY if Create() was invoked
	// OnBeforeCreate is invoked even if Create() fails
	// NOTE: OnAfterDestroy is NOT invoked for modal dialogs
	virtual bool
		OnBeforeCreate(TSonorkWinCreateInfo*)
		{ return true; }

	virtual bool
		OnCreate()      	// Only if Create() succeeded
		{ return true;}

	virtual void
		OnAfterCreate(){}		// Only if Create() succeeded

	virtual void
		OnBeforeDestroy(){} 		// Only if Create() succeeded

	virtual void
		OnDestroy(){}			// Only if Create() succeeded

	virtual void
		OnCleanup(){}			// Only if Create() succeeded (use for subclassing)

	virtual void
		OnAfterDestroy(){}		// Only if Create() succeeded

	virtual LRESULT
		OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM)
		{return 0L;}

	virtual LRESULT
		OnPoke(SONORK_WIN_POKE,LPARAM)
		{return 0L;}

	virtual LRESULT
		OnMfcEvent(SRK_MFC_EVENT,LPARAM)
		{return 0L;}

	virtual bool
		OnAppEvent(UINT event, UINT param,void*data){return false;}

	virtual void
		LoadLabels(){}

	virtual void
		SkinChange(){}

public:
	TSonorkWin(TSonorkWin*parent,DWORD def_flags,DWORD sys_flags);
	virtual ~TSonorkWin();

	TSonorkWin*
		Parent() const
		{ return win.parent;}

	HWND
		Handle() const
		{ return win.hwnd;	}

	// win.rect really holds the left,top,width and height
	// and not left,top,right,bottom
	int
		Left()	const
		{ return win.rect.left;}

	int
		Top() const
		{ return win.rect.top;}

	int
		Width() const
		{ return win.rect.right;}

	int
		Height() const
		{ return win.rect.bottom;}

	int
		Right()	const
		{ return win.rect.left+win.rect.right;}

	int
		Bottom() const
		{ return win.rect.top+win.rect.bottom;}

const RECT*
		WinRect() const
		{ return &win.rect;}

	void
		FillBg(HDC,HBRUSH);

	BOOL	IsDialog() const
		{ return win.defFlags & SONORK_WIN_DIALOG;  	}
		
	BOOL
		IsModal() const
		{ return TestWinSysFlag(SONORK_WIN_SF_MODAL);	}

	BOOL
		Initialized() const
		{ return TestWinSysFlag(SONORK_WIN_SF_INITIALIZED);	}

	UINT
		WinResId() const
		{ return (UINT)(win.defFlags & SONORK_WIN_RES_ID_MASK);}

	SONORK_WIN_CLASS
		WinClass() const
		{ return (SONORK_WIN_CLASS)(win.defFlags & SONORK_WIN_CLASS_MASK);}

	SONORK_WIN_TYPE
		WinType() const
		{ return (SONORK_WIN_TYPE)(win.defFlags & SONORK_WIN_TYPE_MASK);}

	SONORK_C_CSTR
		WinClassName() const
		{ return Win_ClassName(WinClass());}

	BOOL
		MultiTestWinSysFlags(UINT flags) const
		{ return (win.sysFlags&flags);}

	void
		MultiClearWinSysFlags(UINT flags)
		{	win.sysFlags&=~flags;}

	BOOL
		TestWinSysFlag(SONORK_WIN_SYS_FLAG flag) const
		{	return (win.sysFlags&flag);}

	void
		SetWinSysFlag(SONORK_WIN_SYS_FLAG flag)
		{	win.sysFlags|=flag;}

	void
		ClearWinSysFlag(SONORK_WIN_SYS_FLAG flag)
		{	win.sysFlags&=~flag;}
		
	void
		ToggleWinSysFlag(SONORK_WIN_SYS_FLAG flag)
		{   win.sysFlags^=flag;}

	DWORD
		WinSysFlags()	const
		{   return win.sysFlags;}

	BOOL
		MultiTestWinUsrFlags(DWORD flags) const
		{ return (win.usrFlags&flags);}

	void
		MultiClearWinUsrFlags(DWORD flags)
		{ win.usrFlags&=~flags;}

	void
		MultiSetWinUsrFlags(DWORD flags)
		{ win.usrFlags|=flags;}

	BOOL
		TestWinUsrFlag(SONORK_WIN_USR_FLAG flag) const
		{ return (win.usrFlags&flag);}

	void
		SetWinUsrFlag(SONORK_WIN_USR_FLAG flag)
		{ win.usrFlags|=flag;}

	void
		ClearWinUsrFlag(SONORK_WIN_USR_FLAG flag)
		{ win.usrFlags&=~flag;}

	void
		ToggleWinUsrFlag(SONORK_WIN_USR_FLAG flag)
		{  win.usrFlags^=flag;}

	BOOL
		ChangeWinUsrFlag(SONORK_WIN_USR_FLAG flag, BOOL v);

	DWORD
		WinUsrFlags() const
		{   return win.usrFlags;}

	void
		SetEventMask(DWORD eventMask)
		{ win.eventMask = eventMask;}

	DWORD
		GetEventMask() const
		{ return win.eventMask; }
		
	virtual bool
		Create();
		
	virtual int
		Execute();

	void
		ShowWindow(int);

	void
		MoveWindow(int x, int y, int w, int h, bool redraw=true);

	void
		SetWindowPos(HWND hWndInsertAfter,int,int,int,int,UINT);

	void
		ScreenToClient(POINT*p);

	void
		ScreenToClient(RECT*r);

	void
		ClientToScreen(POINT*p);

	void
		ClientToScreen(RECT*r);
		
	void
		GetWindowRect(RECT*);

	void
		GetClientRect(RECT*);

	void
		GetWindowSize(SIZE*);
		
	HMENU
		GetMenu() const
		{ return ::GetMenu(Handle());}
	void
		SetWindowText(SONORK_C_CSTR);
	void
		SetWindowText(GLS_INDEX);
	void
		SetFont(HFONT, bool redraw);

	void
		UpdateWindow();

	void
		InvalidateRect(RECT*,bool erase_bg);

	void
		EndDialog(int code);

	void
		Destroy(int code=IDCANCEL);

	void
		QueryClose();

	void
		SetStayOnTop(BOOL);

	BOOL
		GetStayOnTop();

	void
		SetCaptionIcon(HICON,UINT type=ICON_BIG);

	void
		SetCaptionIcon(SKIN_HICON,UINT type=ICON_BIG);

	BOOL
		IsIconic() const
		{ return ::IsIconic(Handle());}

	BOOL
		IsEnabled() const
		{ return ::IsWindowEnabled(Handle());}

	BOOL
		IsVisible() const
		{ return ::IsWindowVisible(Handle());}

	BOOL
		IsActive() const
		{ return TestWinSysFlag(SONORK_WIN_SF_ACTIVE);}

	LONG
		SetWindowLong(UINT pos, LONG value)
		{ return ::SetWindowLong(Handle(),pos,value);}

	LONG
		GetWindowLong(UINT pos) const
		{ return ::GetWindowLong(Handle(),pos);}
	LONG
		SetClassLong(UINT pos, LONG value)
		{ return ::SetClassLong(Handle(),pos,value);}

	LONG
		GetClassLong(UINT pos) const
		{ return ::GetClassLong(Handle(),pos);}

	void
		EnableWindow(BOOL enable) const
		{ ::EnableWindow(Handle(),enable);}

	UINT
		SetAuxTimer(UINT msecs);

	void
		KillAuxTimer();

	UINT
		SetTimer(UINT id, UINT msecs);

	BOOL
		KillTimer(UINT id);

	HDC
		GetDC();
		
	int
		ReleaseDC(HDC);

// ----------------------
// TOOL TIPS

	void
		AddToolTipCtrl(UINT count,UINT*id_list);

	void
		AddToolTipCtrl(HWND hwnd);

	void
		AddToolTipRect(UINT id, int x1, int y1, int x2, int y2);

// ----------------------
// CONTROLS

	HWND
		GetDlgItem(UINT id) const;

	void
		SetCtrlVisible(UINT id, BOOL);

	void
		SetCtrlEnabled(UINT id, BOOL);

	void
		SetCtrlText(UINT id, GLS_INDEX);

	void
		AppendCtrlText(UINT id, SONORK_C_CSTR,SONORK_C_CSTR str2=NULL);

	void
		SetCtrlDropText(UINT id, struct TSonorkClipData*, SONORK_C_CSTR append_prefix);

	void
		SetCtrlText(UINT id, SONORK_C_CSTR);

	void
		SetCtrlUint(UINT id, DWORD v);

	void
		SetCtrlTime(UINT id, const TSonorkTime&, DWORD flags);

	void
		SetCtrlsFont(UINT*pPtr,HFONT font);

	void
		SetCtrlFont(UINT id,HFONT font);

	DWORD
		GetCtrlUint(UINT id);

	void
		GetCtrlTime(UINT id, TSonorkTime&);

	void
		LoadLangEntries(const TSonorkWinGlsEntry*, bool set_def_entries);

	void
		LoadDefLangEntries();// Sets default text for IDOK/IDCANCEL/IDHELP buttons

	void
		ClrCtrlText(UINT id)
		{ SetCtrlText(id,(SONORK_C_CSTR)NULL);}

	void
		SetCtrlCheckState(UINT id, UINT state);

	void
		SetCtrlChecked(UINT id, BOOL checked);

	bool
		SetCtrlPoint(UINT id, int x, int y);				// Client coords

	bool
		SetCtrlRect(UINT id, int x, int y, int w, int h);	// Client coords

	bool
		SetCtrlSize(UINT id, int w, int h);

	void
		SetEditCtrlMaxLength(UINT id, int v);

	void
		SetEditCtrlModified(UINT id,BOOL);

	void
		InvalidateCtrl( UINT id, RECT*rect=NULL, bool erase_bg =true);

	bool
		GetCtrlText(UINT id, TSonorkShortString&);

	bool
		GetCtrlText(UINT id, TSonorkDynString&);

	bool
		GetCtrlText(UINT id, SONORK_C_STR buffer, int buffer_size);

	UINT
		GetCtrlCheckState(UINT id) const;

	BOOL
		GetCtrlChecked(UINT id) const;

	bool
		GetCtrlPoint(UINT id, POINT*) const;		// Client coords

	bool
		GetCtrlRect(UINT id, RECT*rect) const;	// Screen cords

	UINT
		GetCtrlHeight(UINT id) const;

	UINT
		GetCtrlWidth(UINT id) const;

	bool
		GetCtrlSize(UINT id, SIZE*size) const;

	bool
		GetEditCtrlModified(UINT id) const;

	BOOL
		IsCtrlEnabled(UINT id) const;

	virtual BOOL
		IsUserId(const TSonorkId&) const
		{ return false; }

	int
		MessageBox(const char *title, const char *text, UINT style);

	int
		MessageBox(const char *text, GLS_INDEX	title, UINT flags);

	int
		MessageBox(GLS_INDEX text, GLS_INDEX	title, UINT flags);

	int
		MessageBox(GLS_INDEX text, const char *title, UINT flags);

	void
		ErrorBox(SONORK_C_CSTR text, TSonorkError*pERR=NULL, SKIN_SIGN sign=SKIN_SIGN_ERROR);

	void
		ErrorBox(GLS_INDEX text, TSonorkError*pERR=NULL, SKIN_SIGN sign=SKIN_SIGN_ERROR);

	void
		TaskErrorBox(GLS_INDEX, TSonorkError*,SKIN_SIGN sign=SKIN_SIGN_ERROR);


	LRESULT
		SendMessage(UINT, WPARAM,LPARAM);

	BOOL
		PostMessage(UINT, WPARAM,LPARAM);

	LRESULT
		SendPoke(SONORK_WIN_POKE,LPARAM) const;

	BOOL
		PostPoke(SONORK_WIN_POKE,LPARAM) const;

	virtual void
		SetHintMode(SONORK_C_CSTR, bool update_window) // Set to NULL to leave hint mode
		{}

	void
		SetHintModeLang(GLS_INDEX, bool update_window);

	void
		ClearHintMode()
		{SetHintMode((char*)NULL,false);}

	// Used to send messages from the application to the windows
	void
		DispatchAppEvent(UINT event, UINT param,void*data);

	// Subclasses are recommend to call the TSonorkWin GetSetStartInfo before
	// processing this method
	virtual void
		TransferStartInfo(TSonorkWinStartInfo*, BOOL load);

	virtual BOOL
		PreTranslateMessage(MSG*);
		
// ----------------------------------------------------------------------------
// STATIC members

static const char*
		Win_ClassName( SONORK_WIN_CLASS );

static bool
		InitModule(SONORK_C_CSTR app_class_name);

static void
		ExitModule();

// On ToolBar functions, <size> is output, after toolbar is loaded (if loaded)
static HWND
		CreateToolBar(HWND,UINT id,UINT style,UINT btn_count,const TSonorkWinToolBarButton*,SIZE*size);
		
static void
		LoadToolBar(HWND,UINT bnt_count,const TSonorkWinToolBarButton*,SIZE*size);
		
static void
		SetMenuText(HMENU, UINT id, GLS_INDEX);

static void
		SetMenuText(HMENU, UINT id, SONORK_C_CSTR);

static void
		SetMenuText(HMENU,const TSonorkWinGlsEntry*);

static void
		AppendMenu(HMENU menu
			, UINT insert_before_id
			, UINT id
			, GLS_INDEX gls_index);

static void
		AppendMenuNoData(HMENU menu);

static void
		SetStatus(HWND,GLS_INDEX str, SKIN_HICON ic);

static void
		SetStatus(HWND,SONORK_C_CSTR str, SKIN_HICON ic);

static void
		SetStatus_PleaseWait(HWND hwnd);

static void
		SetStatus_None(HWND hwnd);

static void
		CenterWin(TSonorkWin*win,RECT& rect, UINT flags);
		
static TSonorkWin*
		Handle_to_SonorkWin(HWND hwnd);

};

DeclareSonorkQueueClass(TSonorkWin);






// <sonork_win_move_type> and <sonork_win_origin>
// shared, used by any window that needs a reference point while moving,
// or resizing. Before using, must capture mouse to ensure it is the
// only window using these global variables.
extern UINT	sonork_win_move_type;
extern POINT	sonork_win_move_origin;
// <sonork_active_win>
// shared, holds the last window that received WM_ACTIVATE<active=true>.
// is is cleared when that same window receives WM_ACTIVATE<active=false>
extern TSonorkWin*	sonork_active_win;	// shared, set by all windows
#define SONORK_DEBUG
//#undef SONORK_DEBUG
extern void DbgPrint(const char *fmt,...);

#if defined(SONORK_DEBUG)
#	define	TRACE_DEBUG	DbgPrint
#else
#	define	TRACE_DEBUG
#endif




// -----------------------------------
// Helper definitions

int 	ComboBox_AddString(HWND hwnd, SONORK_C_CSTR str);
int 	ComboBox_GetString(HWND hwnd, int index, SONORK_C_STR str);
int 	ComboBox_DelString(HWND hwnd, int index);
void    ComboBox_Clear(HWND);
int	ComboBox_SetCurSel(HWND, int index);
int	ComboBox_GetCurSel(HWND);
int	ComboBox_FindStringExact(HWND,int index,SONORK_C_CSTR str);
int	ComboBox_FindString(HWND,int index,SONORK_C_CSTR str);
int	ComboBox_SetItemData(HWND,int index,DWORD);
DWORD	ComboBox_GetItemData(HWND,int index);
int	ComboBox_GetCount(HWND);


int 	ListBox_AddString(HWND hwnd, SONORK_C_CSTR str);
int 	ListBox_GetString(HWND hwnd, int index, SONORK_C_STR str);
int 	ListBox_DelString(HWND hwnd, int index);
void    ListBox_Clear(HWND);
int	ListBox_SetCurSel(HWND, int index);
int	ListBox_GetCurSel(HWND);
int	ListBox_FindStringExact(HWND,int index,SONORK_C_CSTR str);
int	ListBox_FindString(HWND,int index,SONORK_C_CSTR str);
int	ListBox_SetItemData(HWND,int index,DWORD);
DWORD	ListBox_GetItemData(HWND,int index);
int	ListBox_GetCount(HWND);

DWORD	ToolBar_GetButtonState(HWND,UINT b);
BOOL	ToolBar_SetButtonState(HWND,UINT b,DWORD);
void	ListView_ClearItemByItem(HWND);

#endif
