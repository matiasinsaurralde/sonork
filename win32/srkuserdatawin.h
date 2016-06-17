#if !defined(SRKUSERDATAWIN_H)
#define SRKUSERDATAWIN_H

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

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

#if !defined(SRKLISTVIEW_H)
# include "srklistview.h"
#endif

#if !defined(SRKBITMAP_H)
# include "srkbitmap.h"
#endif

class TSonorkUserDataWin
:public TSonorkTaskWin
{
friend void TSonorkWin32App::OpenUserDataWin(
			  const TSonorkId&
			, const TSonorkUserData*	user	// may be null
			, const TSonorkDynString*	notes	// may be null
			, TSonorkWin*			owner	// may be NULL
			, UINT tab );
public:
	enum TAB
	{
	  TAB_INFO
	, TAB_NOTES
	, TAB_ONL
	, TAB_AUTH
	, TAB_ALERT
	, TABS
	};
	enum SHOW_FLAGS
	{
	  SF_USER_IN_LOCAL_LIST		=0x00001
	, SF_USER_IN_AUTHORIZED_LIST	=0x00002
	, SF_SAVE_NEW_USER_DATA		=0x00010
	, SF_SAVE_NEW_USER_NOTES	=0x00020
	, SF_DELETE_USER_DATA  		=0x00100
	, SF_AUTO_REFRESH		=0x00200
	};
	enum QUERY_TYPE
	{
		QUERY_NONE
	,	QUERY_PIC
	,	QUERY_SERVICES
	};
private:
	HWND	status_bar;
	DWORD	show_flags;

	char	uid_str[24];
	struct {
		HWND	hwnd;
		TAB	page;
	}tab_ctrl;

	struct unPIC{
		HWND			hwnd;
		TSonorkBitmap	bm;
		RECT			bm_rect;
	}pic;
	struct
	{
		DWORD			instance;
		DWORD			query_id;
		QUERY_TYPE      query_type;
	}service;
	TSonorkError	taskERR;

	TSonorkWin*	page_win[TABS];

	TSonorkExtUserData*	ctx_user;
	TSonorkDynString	ctx_remote_notes;
	TSonorkListView		service_list;

	bool	OnCreate();
	void	OnAfterCreate();
	void	OnBeforeDestroy();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnDrawItem(DRAWITEMSTRUCT*);

	LRESULT
		OnNotify(WPARAM,TSonorkWinNotify*);

	LRESULT
		OnChildDialogDraw(TSonorkChildDialogNotify*);

	LRESULT
		OnChildDialogNotify(TSonorkChildDialogNotify*);

	// UpdateCxItems(): Enables/Disables items that can be changed only
	// when connected to the server.
	void
		UpdateCxItems();

	void
		UpdateQueryButtons();
		
	// CxItemsEnable() : Used by UpdateCxItems() to enum windows
	static BOOL CALLBACK
		CxItemsEnable( HWND hwnd, LPARAM lParam );

	static DWORD SONORK_CALLBACK
		ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);


	bool
		OnAppEvent(UINT event, UINT param, void*data);


	void
		OnTaskStart(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&);
		
	void
		OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);

	void
		OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&, const TSonorkError*);

	LRESULT
		OnPoke(SONORK_WIN_POKE,LPARAM);

	void
		OnPoke_ProcessPic(char*);

	void
		OnPoke_ConnectService();

	void
		CmdRefresh();

	// returns true if may destroy
	bool
		CmdStore();

	void
		SaveData( BOOL user_info
			, BOOL remote_notes);

	void
		SetTab(TAB, bool manual_change);

	// Loads the information in memory into the form
	void
		UI_LoadAuths();

	void
		UI_LoadLocalNotes();

	void
		UI_LoadRemoteNotes(BOOL from_file);

	void
		UI_LoadInfo();

	void
		StartRemoteQuery(QUERY_TYPE);
private:

	TSonorkUserDataWin(
		   TSonorkWin*			powner		// NOT null
		,  TSonorkExtUserData*		user_data	// NOT null
		,  const TSonorkDynString*	notes		// may be null
		,  TAB 				tab             // See enum TAB
		,  DWORD 			flags		// See enum SHOW_FLAGS
		);

public:
	~TSonorkUserDataWin();

	BOOL
		IsUserId(const TSonorkId& userId) const
		{ return ctx_user->userId == userId; }

};
#endif