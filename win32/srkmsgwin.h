#if !defined(SRKMSGWIN_H)
#define SRKMSGWIN_H

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

#if !defined(SRKCONSOLE_H)
# include "srkconsole.h"
#endif
#if !defined(SRK_SERVICES_H)
# include "srk_services.h"
#endif

class	TSonorkMsgWin;

enum SONORK_MSG_WIN_PROCESS_TYPE
{
	SONORK_MSG_WIN_PROCESS_URL         	=0x00001
,	SONORK_MSG_WIN_PROCESS_FILE		=0x00002
,	SONORK_MSG_WIN_PROCESS_MSG_TEMPLATE	=0x00003
};
struct TSonorkMsgWinContext
{
	DWORD			line_no;
	DWORD			line_flags;
	TSonorkMsg		cur_msg;
	TSonorkMsg	 	tmp_msg;
	TSonorkFileInfo		file_info;
	SONORK_C_CSTR		default_text;
	TSonorkCCache		*cache;
	TSonorkConsole		*console;
	TSonorkMsgWin		*msg_win;
};


enum TOOL_BAR_BUTTON;

#define SONORK_MSG_WIN_DEFAULT_INPUT_HEIGHT		70
#define SONORK_MSG_WIN_POKE_OPEN_MSG			SONORK_WIN_POKE_01
#define SONORK_MSG_WIN_POKE_CLOSE_MSG			SONORK_WIN_POKE_02
#define SONORK_MSG_WIN_POKE_AFTER_SEND			SONORK_WIN_POKE_03
#define SONORK_MSG_WIN_POKE_OPEN_INIT			SONORK_WIN_POKE_04
#define SONORK_MSG_WIN_POKE_FIND_LINKED			SONORK_WIN_POKE_05




// lparam = true: incomming , false:outgoing
#define SONORK_MSG_WIN_POKE_NEW_MSG			SONORK_WIN_POKE_06

// lparam = true: Open global event, false: open next local only
#define SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD		SONORK_WIN_POKE_07

#define SONORK_MSG_WIN_POKE_OPEN_SID_MSG		SONORK_WIN_POKE_08
#define SONORK_MSG_WIN_POKE_FOCUS_INPUT_WIN		SONORK_WIN_POKE_09
#define SONORK_MSG_WIN_POKE_RECV_FILE			SONORK_WIN_POKE_10

class TSonorkMsgWin
:public TSonorkWin
{
public:

private:
	enum SEND_MODE
	{
		SEND_MODE_NORMAL	= IDC_MSG_SEND
	,	SEND_MODE_REPLY		= IDC_MSG_REPLY
	,	SEND_MODE_QUERY		= IDC_MSG_QUERY
	};
		struct unTOOLBAR{
			HWND			hwnd;
			SIZE			size;
		}toolbar;

		struct unINPUT{
			HWND			hwnd;
			int			height;
			TSonorkWinCtrl		ctrl;
			TSonorkDropTarget	drop_target;
		}input;

		struct unSTATUS{
			HWND			hwnd;
			int			height;
		}status;
		
		struct unLOGO{
			HWND			hwnd;
			int			height;
			int			min_width;
		}logo;
		
		struct unBTN{
			HWND		query;
			HWND 		reply;
			HWND		send;
			SIZE		size;
		}btn;
		HWND			 separator;



		TSonorkTime		last_flash_time;
		DWORD           	sent_messages_counter;
		struct {
			int x,y;
			TSonorkUltraMinWin*	win;
		}ultra_min;

		
		TSonorkExtUserData*		user_data;
		TSonorkMsgWinContext		context;

		struct _SEND_CONTEXT
		{
			TSonorkCCacheMark 	mark;
		}send_ctx;



		bool	OnCreate();
		void	OnAfterCreate();
		void	OnBeforeDestroy();
		void	OnDestroy();
		void 	OnMove();
		void	OnSize(UINT);
		void	OnActivate(DWORD flags,BOOL minimized);
		bool	OnMinMaxInfo(MINMAXINFO*);
		bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
		LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
		bool	OnDrawItem(DRAWITEMSTRUCT*);
		void	OnToolTipText(UINT id, HWND, TSonorkWinToolTipText&TTT );
		LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);
		LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
		LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);
		void	OnInitMenu(HMENU);
		void 	OnCancelMode();
		void 	OnMouseMove(UINT ,int ,int y);
		void 	OnLButtonUp(UINT ,int ,int );
		void	SetResizingInput( BOOL v );

		void	RealignControls();
		void	LoadLabels();
		void	SetupToolBar();
		void	OnToolBarButton(UINT);

		BOOL	IsToolBarButtonChecked(TOOL_BAR_BUTTON);
		void	SetToolBarButtonState(TOOL_BAR_BUTTON,UINT);
		void	SetToolBarButtonEnabled(TOOL_BAR_BUTTON,BOOL);
		void	CmdSendMessage( SEND_MODE);
		void	CmdSendFile();
		DWORD	CmdProcess( DWORD flags );
		DWORD	CmdProcessMsgTemplate(DWORD flags);
		DWORD	CmdProcessFile(DWORD flags);
		void	CmdSelectThread();
		void	CmdSearch();
		void	CmdScrollLock(bool update_button);
		void	CmdDeletePrevious();
		void	CmdDeleteAll();

		static UINT CALLBACK
			SaveMsgFileDialogHook(
				  HWND dlg_hwnd,
				  UINT uMsg,
				  WPARAM wParam,
				  LPARAM lParam 
				);



		static DWORD SONORK_CALLBACK
				ConsoleCallback(void*
					,SONORK_CONSOLE_EVENT 	pEvent
					,DWORD			pIndex
					,void*			pData);

		DWORD
			OnHistoryWinEvent(TSonorkHistoryWinEvent*E);
		void
			OnHistoryWin_FocusChanged();

		DWORD
			OnHistoryWin_LineDelete(const TSonorkCCacheEntry*CE);
			
		void
			OnHistoryWin_HintPaint( HDC, const RECT*);
		
public:

		void static
			OnHistoryWin_LinePaint(const TSonorkCCacheEntry*
				, TSonorkHistoryWinPaintCtx*);
				
		void static
			OnHistoryWin_GetText(TSonorkMsg&
				, const TSonorkCCacheEntry*
				, SONORK_HISTORY_WIN_TEXT_TYPE
				, TSonorkDynData*);

		static TSonorkCCacheEntry *
			LoadCurrentMsg(TSonorkMsgWinContext&	CTX
				    , DWORD&			process_flags
				    , HWND			status_hwnd);

private:
		bool OnAppEvent(UINT event, UINT param, void*data);
		bool	OnAppEventMsg_Store( TSonorkAppEventMsg*E);
		bool	OnAppEventMsg_Sent(TSonorkAppEventMsg*E);
		bool	OnAppEventMsgProcessed(TSonorkAppEventMsgProcessed*E);
		LRESULT ProcessVKey(UINT vKey, DWORD flags);


		// Synchronizes the values TSonorkExtUserData::CV_UNREAD_MSG_COUNT
		// and TSonorkExtUserData::CV_FIRST_UNREAD of <user_data>
		// with the contents of the message console
		void	RescanMsgFile(DWORD start_line);

#define SONORK_MSG_WIN_F_UPDATE_UI_CAPTION	0x0001
#define SONORK_MSG_WIN_F_UPDATE_UI_ICON		0x0002
#define SONORK_MSG_WIN_F_UPDATE_UI_MSG_COUNT	0x0004
#define SONORK_MSG_WIN_F_UPDATE_UI_SID_TEXT	0x0008
		void	UpdateUserInterface(DWORD flags);


#define SONORK_MSG_WIN_F_UPDATE_SB_FOCUS	0x0001
#define SONORK_MSG_WIN_F_UPDATE_SB_LINE_COUNT	0x0001 // same as a SB_FOCUS
#define SONORK_MSG_WIN_F_UPDATE_SB_SELECTION	0x0001 // same as a SB_FOCUS
#define SONORK_MSG_WIN_F_UPDATE_SB_MSG_INFO	0x0002
// UpdateStatusBar()
//  if MSG_INFO is set:
//    if pEntry is NULL UpdateStatusText() will try to read the CCacheEntry.
//    if pEntry is not NULL, is should point to the currently focused line
//  if MSG_INFO is not set, pEntry is ignored and no information about the
//    currently focused message is updated on the status bar.
		void	UpdateStatusBar( DWORD	flags, TSonorkCCacheEntry*pEntry);


		void	UpdateMtplButtons();
		bool	MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry*);


		void	FindLinked();
		void	FocusInputWin();
		void	TransferStartInfo(TSonorkWinStartInfo*, BOOL load);

		void	ShowMtplMenu(BOOL reply);

		void	PostToolbarCmd(TOOL_BAR_BUTTON);

	void
		UltraMinimizedPaint(struct TSonorkUltraMinPaint*);


public:
	TSonorkMsgWin(TSonorkExtUserData*,TSonorkCCache*);
	~TSonorkMsgWin();

	TSonorkExtUserData*
		UserData(){ return user_data;}

	BOOL
		IsUserId(const TSonorkId& userId) const
		{return user_data->userId == userId;}


	BOOL
		FocusNextUnreadMsg(BOOL);

	BOOL
		IsUltraMinimized() const
		{ return ultra_min.win != NULL; }

	void
		UltraMinimize();

	SONORK_CONSOLE_OUTPUT_MODE
			GetOutputMode()		const
			{	return context.console->GetOutputMode();}

	void	SetHintMode(SONORK_C_CSTR str, bool update_window)
			{ context.console->SetHintMode(str,update_window); }

	void
		OpenMsgWithFlags( DWORD process_flags );

	void
		CloseMsg();

	TSonorkConsole*
		Console()
		{	return context.console;}

	TSonorkCCache*
		Cache()
		{	return context.cache; }
	void
		CmdExport(TSonorkConsole*);

	
	void
		BlinkWindow();

	void
		ShowSidMsg(BOOL show, BOOL forced);

	void
		HideSidMsg();

	static void
		SetupConsole(TSonorkConsole*);

	DWORD
		OnConsoleExport(TSonorkConsoleExportEvent*);

	static SKIN_ICON
		GetMsgIcon(DWORD flags);


};


enum MSG_WIN_SB_SECTION
{
	MSG_WIN_SB_LABEL	= 0 |SBT_NOBORDERS |SBT_OWNERDRAW
,	MSG_WIN_SB_DATE		= 1|0
,	MSG_WIN_SB_SID_MSG	= 2|0
,	MSG_WIN_SB_HINT		= 3|0
,	MSG_WIN_SB_SECTIONS	= 4
};

void		InitMsgWinStatus(HWND);
void		DrawMsgWinStatus(DRAWITEMSTRUCT*);
void		SetMsgWinStatus(HWND 			sb
				, MSG_WIN_SB_SECTION	section
				, SONORK_C_CSTR 	pStr
				, SKIN_HICON 		hIcon);

#endif
