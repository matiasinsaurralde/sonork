#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkwaitwin.h"

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

#define WW_POKE_MESSAGE_BOX	SONORK_WIN_POKE_01
#define UWF_DESTROY_WAIT	SONORK_WIN_F_USER_01


bool Sonork_StartWaitWin(
	  pfnSonorkWaitWinCallback	cb_proc
	, LPARAM			cb_param
	, LPVOID			cb_data
	, DWORD 			sys_flags
	, SKIN_SIGN			sign
	, DWORD 			dialog_id)
{
	TSonorkWaitWin*W;
	SONORK_MEM_NEW(
		W=new TSonorkWaitWin(cb_proc,cb_param,cb_data,sys_flags,sign,dialog_id)
	);
	if( W->Create() )
		return true;
	SONORK_MEM_DELETE(W);
	return false;
}

// --------------------------------------------------
// TSonorkWaitWin

TSonorkWaitWin::TSonorkWaitWin(
		  pfnSonorkWaitWinCallback	p_cb_proc
		, LPARAM 			p_cb_param
		, LPVOID			p_cb_data
		, DWORD 			p_sys_flags
		, SKIN_SIGN			p_sign
		, DWORD 			p_dialog_id
		)
:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|p_dialog_id
	,p_sys_flags
	|SONORK_WIN_SF_NO_WIN_PARENT
	|SONORK_WIN_SF_NO_CLOSE)
{
	wait_flags      = 0;
	cb.proc 	= p_cb_proc;
	cb.param	= p_cb_param;
	cb.data 	= p_cb_data;
	sign		= p_sign;
	notice_sound = SONORK_APP_SOUND_NOTICE;
	remind_sound = SONORK_APP_SOUND_REMIND;
	notice_sequence = SONORK_SEQUENCE_CALL;
	SetEventMask(SONORK_APP_EM_ENUMERABLE);

}
void
 TSonorkWaitWin::SetStatus(GLS_INDEX gls_info
				, DWORD secs
				, DWORD flags
				, DWORD mask)
{
	SetCtrlText(IDC_WAIT_INFO, gls_info );
	mask&=SONORK_WAIT_WIN_F_UI_MASK;
	wait_flags&=~mask;
	wait_flags|=(flags&mask);
	Update_UI();
	SetTimeoutSeconds( secs );
}

void
 TSonorkWaitWin::SetStatusAutoTimeout(DWORD secs)
{
	SetStatus(GLS_MS_PWAIT
		,secs
		,SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON
		|SONORK_WAIT_WIN_F_NO_CANCEL_BUTTON
		|SONORK_WAIT_WIN_F_NO_COUNTER
		,SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON
		|SONORK_WAIT_WIN_F_NO_CANCEL_BUTTON
		|SONORK_WAIT_WIN_F_NO_COUNTER);
}


bool
 TSonorkWaitWin::CancelWait(const char* text
			, GLS_INDEX	caption
			, DWORD		wait_secs)
{
	if( wait_flags&(SONORK_WAIT_WIN_F_WAIT_CANCEL|SONORK_WAIT_WIN_F_MESSAGE_BOX) )
		return false;
	if(wait_secs>10)wait_secs=10;

	max_seconds=wait_secs;
	cur_seconds=0;
	Update_Counter();
	SetCtrlText(IDCANCEL,GLS_OP_CLOSE);	
	wait_flags|=SONORK_WAIT_WIN_F_WAIT_CANCEL
		   |SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON
		   |SONORK_WAIT_WIN_F_CHECKBOX_DISABLED;
	wait_flags&=~(SONORK_WAIT_WIN_F_NO_COUNTER
		   |SONORK_WAIT_WIN_F_NO_CANCEL_BUTTON);
	Update_UI();

	InvokeCB( SONORK_WAIT_WIN_EVENT_RESULT
		, SONORK_WAIT_WIN_F_RESULT_CANCEL );

	SetCtrlText( IDC_WAIT_INFO, text );
	pmb_text.Set(text);
	pmb_caption=caption;
	if( wait_secs == 0 )
	{
		wait_flags|=SONORK_WAIT_WIN_F_MESSAGE_BOX;
		PostPoke(WW_POKE_MESSAGE_BOX,0);
	}
	return true;
}

void
 TSonorkWaitWin::InvokeCB(SONORK_WAIT_WIN_EVENT event , LPARAM event_param )
{
	if( wait_flags & SONORK_WAIT_WIN_F_DESTROYED )
		return;

	// Allow this section because we need
	// to set the current result/flags regardless
	// of wether the callback is called or not
	if( event == SONORK_WAIT_WIN_EVENT_DESTROY)
	{
		wait_flags|=SONORK_WAIT_WIN_F_DESTROYED;
	}
	else
	if( event == SONORK_WAIT_WIN_EVENT_RESULT )
	{
		if( SONORK_WAIT_WIN_RESULT(wait_flags) != SONORK_WAIT_WIN_F_RESULT_NONE)
			return; // already reported the result
		wait_flags|=(event_param&SONORK_WAIT_WIN_F_RESULT_MASK);
		assert( SONORK_WAIT_WIN_RESULT(wait_flags) != SONORK_WAIT_WIN_F_RESULT_NONE );
	}
	
	if( !(wait_flags & SONORK_WAIT_WIN_F_IN_CALLBACK) )
	{
		wait_flags|=SONORK_WAIT_WIN_F_IN_CALLBACK;
		cb.proc(this , cb.param , cb.data , event , event_param);
		wait_flags&=~SONORK_WAIT_WIN_F_IN_CALLBACK;

		ConditionalDestroy();
	}
}

bool
 TSonorkWaitWin::ConditionalDestroy( )
{
	if( wait_flags&(SONORK_WAIT_WIN_F_IN_CALLBACK
			|SONORK_WAIT_WIN_F_MESSAGE_BOX
			|SONORK_WAIT_WIN_F_WAIT_CANCEL) )
		return false;
	if( SONORK_WAIT_WIN_RESULT(wait_flags) == SONORK_WAIT_WIN_F_RESULT_NONE )
		return false;
	KillAuxTimer();
	PostPoke(SONORK_WIN_POKE_DESTROY,0);
	return true;
}

LRESULT
 TSonorkWaitWin::OnPoke(SONORK_WIN_POKE poke,LPARAM )
{
	if( poke==SONORK_WIN_POKE_DESTROY )
	{
		Destroy();
	}
	else
	if( poke == WW_POKE_MESSAGE_BOX)
	{
		MessageBox(pmb_text.CStr()
			,pmb_caption
			,MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
		wait_flags&=~SONORK_WAIT_WIN_F_MESSAGE_BOX;
		ConditionalDestroy();
	}
	return 0;
}

void
 TSonorkWaitWin::SetTimeoutSeconds(DWORD nv)
{
	max_seconds=nv;
	cur_seconds=0;
	Update_Counter();
}

bool
 TSonorkWaitWin::OnCreate()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDOK			|SONORK_WIN_CTF_BOLD
		,	GLS_OP_ACCEPT	}
	,	{IDCANCEL               |SONORK_WIN_CTF_BOLD
		,	GLS_OP_DENY	}
	,	{IDC_WAIT_CHECKBOX
		,	GLS_EA_START	}
	,	{IDG_WAIT_INFO		|SONORK_WIN_CTF_BOLD
		,	GLS_NULL	}
	,	{IDC_WAIT_LABEL		|SONORK_WIN_CTF_BOLD
		,	GLS_NULL	}
	,	{IDC_WAIT_SECS		
		,	GLS_NULL	}
	,	{0
		,	GLS_NULL	}
	};
	LoadLangEntries(gls_table,false);

	InvokeCB( SONORK_WAIT_WIN_EVENT_CREATE , (LPARAM)&wait_flags);
	max_seconds = (wait_flags & SONORK_WAIT_WIN_F_SECONDS_MASK);
	if( !max_seconds )
		max_seconds=950;	// Set seconds to default
	wait_flags&=SONORK_WAIT_WIN_F_UI_MASK;
	cur_seconds	=0;
	SetCtrlChecked(IDC_WAIT_CHECKBOX,wait_flags & SONORK_WAIT_WIN_F_CHECKBOX_CHECKED);
	SetAuxTimer(1000);
	Update_Counter();
	Update_UI();
	if( wait_flags & SONORK_WAIT_WIN_F_IS_NOTICE )
	{
		SonorkApp.AppSound( notice_sound );
		SonorkApp.SetBichoSequence( notice_sequence );
	}
	ShowWindow(SW_SHOWNORMAL);
	return true;
}
void
 TSonorkWaitWin::OnBeforeDestroy()
{
  // First call to InvokeCB will be ignored if result has already been reported
  InvokeCB( SONORK_WAIT_WIN_EVENT_RESULT , SONORK_WAIT_WIN_F_RESULT_CANCEL );

  InvokeCB( SONORK_WAIT_WIN_EVENT_DESTROY, 0 );
}


void
 TSonorkWaitWin::Update_UI()
{
	SetCtrlVisible( IDC_WAIT_SECS
		, !(wait_flags &  SONORK_WAIT_WIN_F_NO_COUNTER));

	SetCtrlVisible( IDC_WAIT_CHECKBOX
		, !(wait_flags &  SONORK_WAIT_WIN_F_NO_CHECKBOX));

	SetCtrlEnabled( IDC_WAIT_CHECKBOX
		, !(wait_flags &  SONORK_WAIT_WIN_F_CHECKBOX_DISABLED));

	SetCtrlEnabled( IDOK
		, !(wait_flags & SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON) );

	SetCtrlEnabled( IDCANCEL
		, !(wait_flags & SONORK_WAIT_WIN_F_NO_CANCEL_BUTTON) );
}
void	TSonorkWaitWin::Update_Counter()
{
	SetCtrlUint(IDC_WAIT_SECS,max_seconds-cur_seconds);
}

void
 TSonorkWaitWin::OnTimer(UINT )
{

	if( wait_flags &
	(SONORK_WAIT_WIN_F_DESTROYED		// Destroyed
	|SONORK_WAIT_WIN_F_IN_CALLBACK          // In callback
	|SONORK_WAIT_WIN_F_MESSAGE_BOX))	// Displaying (or about to) Messagebox
		return;

	if( wait_flags & SONORK_WAIT_WIN_F_WAIT_CANCEL )
	{
		if( ++cur_seconds >= max_seconds )
		{
			KillAuxTimer();
			Destroy();
		}
		else
			Update_Counter();
		return;
	}
	
	// Check if already sent the result
	if( wait_flags & SONORK_WAIT_WIN_F_RESULT_MASK)
		return;
	if( ++cur_seconds >= max_seconds )
	{
		InvokeCB( SONORK_WAIT_WIN_EVENT_RESULT
			, SONORK_WAIT_WIN_F_RESULT_TIMEOUT);
	}
	else
	{
		if( (cur_seconds&0x7)==7 )
		{
			if( wait_flags & SONORK_WAIT_WIN_F_IS_NOTICE )
			    SonorkApp.AppSound( remind_sound );
		}
		InvokeCB( SONORK_WAIT_WIN_EVENT_TIMER
			, cur_seconds);
		Update_Counter();
	}

}
bool
 TSonorkWaitWin::OnCommand(UINT id,HWND , UINT notify_code)
{
	if(notify_code == BN_CLICKED )
	{
		DWORD result;
		if( wait_flags& SONORK_WAIT_WIN_F_WAIT_CANCEL )
		{
			if( id == IDCANCEL )
			{
				Destroy();
				return true;
			}
		}
		else
		{
			if( id == IDOK )
				result=SONORK_WAIT_WIN_F_RESULT_ACCEPT;
			else
			if( id == IDCANCEL )
				result=SONORK_WAIT_WIN_F_RESULT_CANCEL;
			else
				return false;
			InvokeCB( SONORK_WAIT_WIN_EVENT_RESULT
				, result );
		}
		return true;
	}
	return false;
}

bool
 TSonorkWaitWin::OnAppEvent(UINT event, UINT param,void*data)
{
	TSonorkWaitWinEvent_AppEvent E;
	E.event = event;
	E.param = param;
	E.data  = data;
	E.result= false;
	InvokeCB( SONORK_WAIT_WIN_EVENT_APP_EVENT, (LPARAM)&E);
	return E.result;
}
void
 TSonorkWaitWin::SetSign(SKIN_SIGN new_sign)
{
	sign=new_sign;
	if(Handle()!=NULL)
		InvalidateCtrl(IDC_WAIT_ICON,NULL,false);
}
bool
 TSonorkWaitWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDC_WAIT_ICON )
	{
		::FillRect(S->hDC,&S->rcItem,GetSysColorBrush(COLOR_3DFACE));
		sonork_skin.DrawSign(S->hDC,sign,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

