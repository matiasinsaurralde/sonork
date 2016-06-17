#if !defined(SRKGRPMSGWIN_H)
#define SRKGRPMSGWIN_H

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
#include "srk_dragdrop.h"

struct TSonorkGrpMsgItem
{
	TSonorkId		user_id;
	TSonorkShortString	alias;
	DWORD			flags;
};
DeclareSonorkQueueClass(TSonorkGrpMsgItem);

class TSonorkGrpMsgWin
:public TSonorkTaskWin
{
public:
	enum TAB_INDEX
	{
		TAB_TEXT
	,	TAB_FILE
	};
	enum STATE
	{
		STATE_IDLE
	,	STATE_SENDING
	,	STATE_COMPLETE
	};

private:

	struct USERS
	{
		HWND		hwnd;
		int		width;
	}users;

	struct
	{
		HWND		hwnd;
		TAB_INDEX	index;
		int		top;
	}tab;

	HWND				inputHwnd;
	TSonorkDropTarget	inputDropTarget;

	struct
	{
		HWND	hwnd;
		int	width;
	}send_btn;

	struct {
		HWND	hwnd;
		SIZE	size;
	}toolbar;

	struct {
		HWND	hwnd;
		int	height;
	}status;

	struct T_SEND{
		TSonorkMsgHandleEx	handle;
		TSonorkMsg		msg;
		TSonorkListIterator 	iterator;
		TSonorkGrpMsgItemQueue	queue;
		TSonorkGrpMsgItem*	pCurItem;
		STATE			state;
		UINT			success_count;
		UINT			process_count;
	}send;

	struct T_FILE{
		HWND			list;
		HWND			browse;
		SIZE			browse_sz;
		TSonorkDropTarget	drop_target;
	}t_file;

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	bool	OnMinMaxInfo(MINMAXINFO*);
	bool	OnCommand(UINT id,HWND hwnd, UINT code);
	void	OnSize(UINT);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);
	void	OnToolTipText(UINT id, HWND, TSonorkWinToolTipText&TTT );
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);

//	static void SONORK_CALLBACK
//		FileTxEngHandler(void *param,TSonorkFileTxEngEvent*);

	void	RealignControls();
	bool	OnAppEvent(UINT event, UINT param, void*data);
	void	_SetTab(TAB_INDEX b, bool update_button, bool forced);
	void	LoadLabels();

	void	SetupToolBar();
	void	UpdateToolBar();
	void	LoadSendListFromMainView();
	UINT	ClearSendList();// returns no of items loaded before clearing
	void	CmdSend();
	void	CmdClear(bool user_click);
	void	CmdFileBrowse();

	void
		EnableInterface(BOOL);


protected:
	bool OnCreate();
	void OnBeforeDestroy();

public:
	TSonorkGrpMsgWin(bool is_main_view);

	void	SetTab(TAB_INDEX b)
			{ _SetTab(b,true,false);}
	void
		AddFile(SONORK_C_CSTR);
	void
		AddUser(const TSonorkId&, SONORK_C_CSTR alias);
	void
		SetCompress(BOOL);

	// Call AfterAddUsers() after adding users with AddUser()
	void
		AfterAddUsers();

	void
		ProcessDrop(TSonorkClipData*CD);
};

#endif