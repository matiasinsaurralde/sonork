#if !defined(SRKWAPPWIN_H)
#define SRKWAPPWIN_H

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


#include "srktaskwin.h"
#include "srkwinctrl.h"
#include "srk_dragdrop.h"
#include "srk_url_codec.h"
#include "srkbrowser_defs.h"

enum SONORK_WAPP_URL_TYPE
{
	SONORK_WAPP_URL_INVALID	=-1
,	SONORK_WAPP_URL_HTML
,	SONORK_WAPP_URL_VOID	=100
,	SONORK_WAPP_URL_SEND_MSG
,	SONORK_WAPP_URL_GOTO
,	SONORK_WAPP_URL_ADD_USER
,	SONORK_WAPP_URL_USER_INFO
,	SONORK_WAPP_URL_SUBMIT
,	SONORK_WAPP_URL_EXTENSION_FIRST	= SONORK_WAPP_URL_VOID
,	SONORK_WAPP_URL_EXTENSION_LAST	= SONORK_WAPP_URL_SUBMIT
};


struct TSonorkWappUrl
{
	SONORK_WAPP_URL_TYPE	type;
	TSonorkShortString  	str;
	TSonorkUrlParams	params;

	TSonorkWappUrl(){}
	TSonorkWappUrl(SONORK_WAPP_URL_TYPE,SONORK_C_CSTR);
	SONORK_WAPP_URL_TYPE	Type() const
	{
		return type;
	}
	SONORK_C_CSTR		CStr() const
	{
		return str.CStr();
	}

	BOOL	IsValid() const
	{
        	return type!=SONORK_WAPP_URL_INVALID;
	}
	BOOL	IsSonorkExtension() const
	{
		return ( type>=SONORK_WAPP_URL_EXTENSION_FIRST
			&& type<=SONORK_WAPP_URL_EXTENSION_LAST);
	}
	void Clear();
	void LoadUrl(SONORK_C_CSTR);
	void Set(const TSonorkWappUrl&O);
};
enum SONORK_WAPP_WIN_SET_TARGET_FLAGS
{
  SONORK_WAPP_WIN_SET_TARGET_NO_NAME_LOOKUP	= 0x000001
, SONORK_WAPP_WIN_SET_TARGET_FORCE_NAME_LOOKUP	= 0x000002
, SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID	= 0x100000
};
enum SONORK_WAPP_WIN_UPDATE_INTERFACE_FLAGS
{
  SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION	= 0x000001
, SONORK_WAPP_WIN_UPDATE_INTERFACE_STATUS	= 0x000002
, SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET	= 0x000004
//, SONORK_WAPP_WIN_UPDATE_INTERFACE_CONTROLS	= 0x000008
};
enum SONORK_WAPP_WIN_NAVIGATE_STATUS
{
  SONORK_WAPP_WIN_NAVIGATE_IDLE		= 0
, SONORK_WAPP_WIN_NAVIGATE_REQUESTING	= 1
, SONORK_WAPP_WIN_NAVIGATE_DOWNLOADING	= 2
};
enum SONORK_WAPP_WIN_FORM_FLAGS
{
	SONORK_WAPP_WIN_FORM_F_TEST_MODE	= 0x10000000
};
struct TSonorkWappForm
{
	DWORD			form_flags;
	DWORD			pc_flags;
	DWORD			usr_flags;
	TSonorkShortString  	title;
	TSonorkShortString	target;
	void Clear()
	{
		pc_flags=usr_flags=form_flags=0;
		title.Clear();
		target.Clear();
	}
};

class TSonorkWappWin
:public TSonorkTaskWin
{
//	enum TARGET_TYPE	{		TARGET_NONE	,	TARGET_GUID	};

	struct _PAGE
	{
		TSonorkShortString  title;
	}page;

	struct _TASK
	{
		TSonorkMsgHandleEx	handle;
		TSonorkWappUrl		post_url;
		TSonorkCCacheMark	reply_mark;
	}task;

	struct {
		HWND	hwnd;
		SIZE	size;
	}toolbar;

	struct _TARGET
	{
		HWND			hwnd;
		TSonorkShortString  	str;
	}target;

	int			top_height
				,status_height;

	HWND		 	status_hwnd;

	struct _HOME
	{
		TSonorkShortString	url;
		SONORK_WAPP_TYPE	type;
		DWORD			app_id,url_id;
		SONORK_DWORD4		pin;
	}wapp;

	struct _BROWSER
	{
		HWND				hwnd;
		SONORK_WAPP_WIN_NAVIGATE_STATUS status;
		TSrkMfcWindowHandle* 		window;
		TSonorkWinCtrl			wc_ctl;
	}browser;

	bool	OnCreate();
	void	OnAfterCreate();
	void	OnBeforeDestroy();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	void	OnSize(UINT);
	bool	OnMinMaxInfo(MINMAXINFO*);
	LRESULT	OnCtlWinMsg(class TSonorkWinCtrl*,UINT,WPARAM,LPARAM);
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);
	LRESULT OnMfcEvent(SRK_MFC_EVENT,LPARAM);
	bool 	OnAppEvent(UINT event, UINT param, void*data);
	void	OnToolTipText(UINT id, HWND  , TSonorkWinToolTipText&TTT );

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	void	OnPoke_Navigate(TSonorkWappUrl*);
	void	OnPoke_Navigate_SendMsg(TSonorkWappUrl*);
	void		OnPoke_Navigate_SendMsg_Test(TSonorkWappForm*,TSonorkWappUrl*,TSonorkMsg*);
	bool		Load_Msg(TSonorkMsg&,TSonorkUrlParams&);
	void	OnPoke_Navigate_AddUser(TSonorkWappUrl*);
	void	OnPoke_Navigate_UserInfo(TSonorkWappUrl*);
	void	OnPoke_Navigate_Goto(TSonorkWappUrl*);
	void	OnPoke_SendMsg_Result();
	void	OnPoke_WappUrl_Result();


	void 	OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);
	void 	OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&, const TSonorkError*);

	void RealignControls();

	void	SetNavigateStatus( SONORK_WAPP_WIN_NAVIGATE_STATUS );
	void 	BrowserEvent_OnBeforeNavigate(TSrkMfcBrowserEvBeforeNavigate*);
	void 	BrowserEvent_OnNavigateComplete(SONORK_C_CSTR);
	void 	BrowserEvent_OnDownload(BOOL begin);
	void 	BrowserEvent_OnProgressChange(LONG cur_v, LONG max_v);
	void 	BrowserEvent_OnDocumentComplete(SONORK_C_CSTR url);
	void 	BrowserEvent_OnTitleChange(SONORK_C_CSTR);

	void	LoadMainForm(IUnknown*II , SONORK_C_CSTR action);
	void	ParseForm( TSonorkWappForm*form
			, TSonorkUrlParams* params
			, TSonorkWappUrl* post_url);


	void	ClearMainForm();

	void	PrepareSonorkQueryString( TSonorkShortString& , SONORK_C_CSTR base_url);


	// UpdateInterface()
	// takes as parameter a combination of the
	// SONORK_WAPP_WIN_UPDATE_INTERFACE_FLAGS
	void	UpdateInterface(DWORD flags);

	void	SetWorking( SONORK_FUNCTION );
	void	SetupToolBar();

	// ProcessUrl() returns true if url navigation should be canceled
	bool	PreProcessUrl(const char *url, const char *data, UINT data_size);
	void	GetWappUrl();

	BOOL	GetToolBarChecked(UINT);
	void	SetToolBarChecked(UINT,BOOL);
	void 	MarkMsg(DWORD flags);


// ExecBrowserCmd:
//  Executes the command on the browser immediately

	LRESULT	ExecBrowserCmd(SRK_MFC_CMD,LPARAM);
	LRESULT	ExecBrowserCmd_Navigate(DWORD cmd,SONORK_C_CSTR,int i_param);
	LRESULT	ExecBrowserCmd_Navigate_Url(SONORK_C_CSTR url);
	LRESULT	ExecBrowserCmd_Navigate_Stop();
	LRESULT	ExecBrowserCmd_Navigate_History(int step);
	//Browser_CombineURL: set base to NULL to combine with current
	BOOL	ExecBrowserCmd_CombineURL(TSonorkShortString&
		, SONORK_C_CSTR base
		, SONORK_C_CSTR append);
	IUnknown*
		ExecBrowserCmd_GetFormInfo(	SONORK_C_CSTR           name
					, TSonorkShortString*	action);
	BOOL	ExecBrowserCmd_GetFormElement(IUnknown*
					, SONORK_C_CSTR 	name
					, TSonorkShortString*	value);


	BOOL	PreTranslateMessage(MSG*);

public:
	TSonorkWappWin(const TSonorkWappData*,const TSonorkExtUserData*UD, const TSonorkCCacheMark*cMark);
	TSonorkWappWin(const char *path,const TSonorkExtUserData*UD);
	BOOL	ReplyMode()	const ;

	void 	SetReplyMark(const TSonorkExtUserData *UD, const TSonorkCCacheMark* pMark);
	
	void	SetTarget_User(const TSonorkExtUserData *UD
			, DWORD flags);

	void	SetTarget_ID(const TSonorkId& gu_id
			, const char *name
			, DWORD flags);

	void	SetTarget_Str(SONORK_C_CSTR str
			, DWORD flags);

	void	ClearTarget();


// Browser

	HWND    Browser_Handle() 	const {	return browser.hwnd;		 }
	BOOL	Browser_Created()	const { return browser.hwnd!=NULL;}

	void	Navigate_Home(bool immediate);
	void	Navigate_WappUrl(const TSonorkWappUrl&, bool immediate);


};

#endif