#if !defined(SRKWAITWIN_H)
#define SRKWAITWIN_H

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

#if !defined(SRKDIALOGWIN_H)
# include "srkdialogwin.h"
#endif

#if !defined(SRKBICHO_DEFS_H)
# include "srkbicho_defs.h"
#endif

enum SONORK_WAIT_WIN_EVENT
{
  SONORK_WAIT_WIN_EVENT_NONE	// Param: None
, SONORK_WAIT_WIN_EVENT_CREATE	// Param: None
, SONORK_WAIT_WIN_EVENT_RESULT	// Param: Result
, SONORK_WAIT_WIN_EVENT_DESTROY	// Param: None
, SONORK_WAIT_WIN_EVENT_TIMER	// Param is current seconds
, SONORK_WAIT_WIN_EVENT_APP_EVENT	// Param is TSonorkWaitWinEvent_AppEvent*
};
enum SONORK_WAIT_WIN_FLAGS
{
  SONORK_WAIT_WIN_F_SECONDS_MASK	= 0x00000ff // only valid as return value for EVENT_CREATE
, SONORK_WAIT_WIN_F_RESULT_MASK		= 0x0000f00
, SONORK_WAIT_WIN_F_RESULT_NONE		= 0x0000000
, SONORK_WAIT_WIN_F_RESULT_TIMEOUT	= 0x0000100
, SONORK_WAIT_WIN_F_RESULT_CANCEL	= 0x0000200
, SONORK_WAIT_WIN_F_RESULT_ACCEPT	= 0x0000300
, SONORK_WAIT_WIN_F_UI_MASK		= 0x0ff0000
, SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON	= 0x0010000
, SONORK_WAIT_WIN_F_NO_CANCEL_BUTTON	= 0x0020000
, SONORK_WAIT_WIN_F_NO_COUNTER		= 0x0040000
, SONORK_WAIT_WIN_F_NO_CHECKBOX		= 0x0080000
, SONORK_WAIT_WIN_F_CHECKBOX_DISABLED	= 0x0100000
, SONORK_WAIT_WIN_F_CHECKBOX_CHECKED	= 0x0200000
, SONORK_WAIT_WIN_F_IS_NOTICE		= 0x0400000
, SONORK_WAIT_WIN_F_INTERNAL_MASK	= 0xf000000
, SONORK_WAIT_WIN_F_DESTROYED		= 0x1000000
, SONORK_WAIT_WIN_F_IN_CALLBACK		= 0x2000000
, SONORK_WAIT_WIN_F_WAIT_CANCEL		= 0x4000000
, SONORK_WAIT_WIN_F_MESSAGE_BOX		= 0x8000000
};
#define SONORK_WAIT_WIN_RESULT(f)	((f)&SONORK_WAIT_WIN_F_RESULT_MASK)
struct
 TSonorkWaitWinEvent_AppEvent
{
	UINT event;
	UINT param;
	void*data;
	bool result;
};

class TSonorkWaitWin;
typedef void (SONORK_CALLBACK fnSonorkWaitWinCallback)(TSonorkWaitWin*
		, LPARAM cb_param
		, LPVOID cb_data
		, SONORK_WAIT_WIN_EVENT event
		, LPARAM event_param);
typedef fnSonorkWaitWinCallback* pfnSonorkWaitWinCallback;

class TSonorkWaitWin
:public TSonorkWin
{
private:
	bool			OnDrawItem(DRAWITEMSTRUCT*);
	SKIN_SIGN		sign;
	SONORK_SEQUENCE  	notice_sequence;
	SONORK_APP_SOUND	notice_sound;
	SONORK_APP_SOUND	remind_sound;
	DWORD			wait_flags;
	DWORD			cur_seconds;
	DWORD			max_seconds;
	struct{
		pfnSonorkWaitWinCallback	proc;
		LPVOID				data;
		LPARAM 				param;
	}cb;
	TSonorkShortString	pmb_text;
	GLS_INDEX		pmb_caption;


	void	Update_Counter();
	void	Update_UI();
	void	OnTimer(UINT);
	void	OnBeforeDestroy();
	bool	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	bool	OnCreate();
	bool	OnAppEvent(UINT event, UINT param,void*data);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	void	InvokeCB(SONORK_WAIT_WIN_EVENT,LPARAM);
	bool	ConditionalDestroy();

public:

	TSonorkWaitWin(pfnSonorkWaitWinCallback	cb_proc
		, LPARAM 			cb_param
		, LPVOID			cb_data
		, DWORD 			sys_flags
		, SKIN_SIGN			sign
		, DWORD 			dialog_id);

	DWORD
		WaitFlags() const
		{ return wait_flags; }


	DWORD
		CurSeconds() const
		{ return cur_seconds;}

	DWORD
		TimeoutSeconds() const
		{ return max_seconds;}

	void
		SetTimeoutSeconds(DWORD);

	void
		SetSign(SKIN_SIGN new_sign);

	void
		SetStatus(GLS_INDEX info
				, DWORD secs
				, DWORD flags
				, DWORD mask);
	void
		SetStatusAutoTimeout(DWORD secs);

	bool
		CancelWait(const char* 	text
			, GLS_INDEX	caption
			, DWORD		secs);	// If secs=0 a messagebox is displayed

	void
		SetNotice( SONORK_SEQUENCE  notice_seq
			,  SONORK_APP_SOUND notice
			,  SONORK_APP_SOUND remind = SONORK_APP_SOUND_REMIND)
		{
			notice_sequence =notice_seq;
			notice_sound    =notice;
			remind_sound	=remind;
		}

};
bool Sonork_StartWaitWin(
	  pfnSonorkWaitWinCallback	cb_proc
	, LPARAM			cb_param
	, LPVOID			cb_data
	, DWORD 			sys_flags=0
	, SKIN_SIGN			sign=SKIN_SIGN_QUERY
	, DWORD 			dialog_id=IDD_WAIT);


#endif