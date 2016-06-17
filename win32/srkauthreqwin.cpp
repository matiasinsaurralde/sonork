#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkauthreqwin.h"
#include "srkuserdatawin.h"

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

#define SONORK_WIN_F_AUTH_NOT_LOADED	SONORK_WIN_F_USER_01
#define SONORK_WIN_F_AUTH_SENT_BY_US	SONORK_WIN_F_USER_02
#define SONORK_WIN_F_AUTH_DENYING_REQ   SONORK_WIN_F_USER_03
// ----------------------------------------------------------------------------

TSonorkAuthReqWin::TSonorkAuthReqWin(const TSonorkId& userId)
	:TSonorkTaskWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_AUTH_REQ
	|SONORK_WIN_DIALOG
	|IDD_AUTH_REQ
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
	RD.user_data.userId.Set(userId);
	SetEventMask(SONORK_APP_EM_CX_STATUS_AWARE
		|SONORK_APP_EM_PROFILE_AWARE
		|SONORK_APP_EM_USER_LIST_AWARE);

}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::CmdInfo()
{
	SonorkApp.OpenUserDataWin(
		  &RD.user_data
		, NULL
		, this
		, TSonorkUserDataWin::TAB_INFO);

}


// ----------------------------------------------------------------------------

bool
 TSonorkAuthReqWin::OnCreate()
{
	char tmp[80];
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_AUTH_REQ_ACCEPT,	GLS_OP_ACCEPT	}
	,	{IDC_AUTH_REQ_DENY	,	GLS_OP_DENY		}
	,	{IDL_AUTH_REQ_ALIAS	,	GLS_LB_ALIAS	}
	,	{IDL_AUTH_REQ_NAME	,	GLS_LB_NAME	}
	,	{IDL_AUTH_REQ_NOTES	,	GLS_LB_NOTES	}
	,	{IDL_AUTH_REQ_TIME	,	GLS_LB_TIME	}
	,	{IDC_AUTH_REQ_INFO	,	GLS_LB_INFO	}
	,	{IDL_AUTH_REQ_OPTIONS	,	GLS_LB_OPTS	}
	,	{IDCANCEL		, 	GLS_OP_CLOSE	}
	,	{-1			,	GLS_LB_REQUSRS	}
	,	{0			,	GLS_NULL	}
	};
	LoadLangEntries( gls_table, true );
	SetCtrlText(IDC_AUTH_REQ_GUID,RD.user_data.userId.GetStr(tmp));
	if( SonorkApp.LoadAuthReq(&RD) != SONORK_RESULT_OK )
	{
		SetWinUsrFlag(SONORK_WIN_F_AUTH_NOT_LOADED);
		RD.user_data.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1,false);
		ClrCtrlText(IDC_AUTH_REQ_NAME);
		ClrCtrlText(IDC_AUTH_REQ_ALIAS);
		ClrCtrlText(IDC_AUTH_REQ_TIME);
		ClrCtrlText(IDC_AUTH_REQ_NOTES);
	}
	else
	{

		TSonorkWin32App::MakeTimeStr( RD.RequestTime() , tmp, MKTIMESTR_DATE|MKTIMESTR_TIME);
		SetCtrlText(IDC_AUTH_REQ_ALIAS,RD.user_data.alias.CStr());
		SetCtrlText(IDC_AUTH_REQ_NAME,RD.user_data.name.CStr());
		SetCtrlText(IDC_AUTH_REQ_TIME,tmp);
		SetCtrlText(IDC_AUTH_REQ_NOTES,RD.notes.ToCStr());
		if( RD.RequestorId() == SonorkApp.ProfileUserId() )
		{
			SetWinUsrFlag(SONORK_WIN_F_AUTH_SENT_BY_US);
			SetCtrlText(IDC_AUTH_REQ_DENY,GLS_OP_FORGET);
		}
	}
	SetCtrlText(IDC_AUTH_REQ_HELP
		,TestWinUsrFlag(SONORK_WIN_F_AUTH_SENT_BY_US)
			?GLS_AUTHREQOHLP
			:GLS_AUTHREQIHLP);
	SetCtrlText(IDL_AUTH_REQ_INFO
		,TestWinUsrFlag(SONORK_WIN_F_AUTH_SENT_BY_US)
			?GLS_LB_RCPTR
			:GLS_LB_SNDR
			);

	// Status bar
	status_bar=GetDlgItem(IDC_AUTH_REQ_STATUS);
	UpdateCxStatus();
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::UpdateButtons()
{
	BOOL enabled;
	enabled = !(TestWinUsrFlag(SONORK_WIN_F_AUTH_NOT_LOADED)
		|| IsTaskPending()) && SonorkApp.CxReady();
	SetCtrlEnabled(IDC_AUTH_REQ_ACCEPT,enabled && !TestWinUsrFlag(SONORK_WIN_F_AUTH_SENT_BY_US));
	SetCtrlEnabled(IDC_AUTH_REQ_DENY,enabled);
	SetCtrlEnabled(IDC_AUTH_REQ_INFO,enabled);

}

// ----------------------------------------------------------------------------

bool
 TSonorkAuthReqWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		switch(id )
		{
			case IDC_AUTH_REQ_ACCEPT:
				CmdAccept();
				break;
			case IDC_AUTH_REQ_DENY:
				CmdDeny();
				break;
			case IDC_AUTH_REQ_INFO:
				CmdInfo();
				break;
			default:
				return false;
		}
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::CmdAccept()
{
	UINT			A_size,P_size;
	TSonorkDataPacket*	P;
	SONORK_DWORD2	tag;
	if(TestWinUsrFlag(SONORK_WIN_F_AUTH_NOT_LOADED))return;
	if(TestWinUsrFlag(SONORK_WIN_F_AUTH_SENT_BY_US))return;
	A_size=64;
	P=SONORK_AllocDataPacket(A_size);
	RD.wRequestAuth().tag = 0;
	RD.wRequestAuth().flags.Clear();
	RD.wRequestAuth().flags.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
	//RD.auth.pin should be the same as the received pin
	tag.v[0] = SONORK_FUNCTION_AKN_AUTH;
	P_size = P->E_AknAuth_R(A_size, RD.user_data.userId , RD.RequestAuth());

	ClearWinUsrFlag(SONORK_WIN_F_AUTH_DENYING_REQ);
	StartSonorkTask(taskERR
		,P
		,P_size
		,0
		,GLS_TK_ADDUSR
		,&tag);
	SONORK_FreeDataPacket( P );

}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::CmdDeny()
{
	UINT			A_size,P_size;
	SONORK_DWORD2	tag;
	TSonorkDataPacket*	P;

	if(TestWinUsrFlag(SONORK_WIN_F_AUTH_NOT_LOADED))return;

	tag.v[0] = SONORK_FUNCTION_DEL_AUTH;
	A_size=64;
	P=SONORK_AllocDataPacket(A_size);
	P_size = P->E_DelAuth_R(A_size, RD.user_data.userId );
	SetWinUsrFlag(SONORK_WIN_F_AUTH_DENYING_REQ);
	StartSonorkTask(taskERR
		,P
		,P_size
		,0
		,GLS_TK_DENYREQ
		,&tag);
	SONORK_FreeDataPacket( P );
}

// ----------------------------------------------------------------------------

bool
 TSonorkAuthReqWin::OnAppEvent(UINT event, UINT , void*)
{
	switch(event)
	{
		case SONORK_APP_EVENT_CX_STATUS:
			UpdateCxStatus();
			return true;
			
		default:
			return false;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::OnTaskStart(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&)
{
	TSonorkWin::SetStatus_PleaseWait(status_bar);
	UpdateButtons();
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*, UINT )
{
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2&tag, const TSonorkError* pERR)
{
	// We post ourselves a message: Can't diplay modal dialogs
	//  from within a Task handler... we'd halt the whole Sonork engine
	TSonorkWin::SetStatus_None(status_bar);
	taskERR.Set( *pERR );
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, tag.v[0]);
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkAuthReqWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM )
{
	if(wParam != SONORK_WIN_POKE_SONORK_TASK_RESULT)return 0L;
	UpdateButtons();
	if( taskERR.Result() == SONORK_RESULT_OK )
	{
		ErrorBox(GLS_AUTHREQSENT,NULL,SKIN_SIGN_USERS);
		Destroy();
	}
	else
	{
		TaskErrorBox(
			TestWinUsrFlag(SONORK_WIN_F_AUTH_DENYING_REQ)
			?GLS_TK_DENYREQ:GLS_TK_ADDUSR
			,&taskERR);
	}
	return 0L;
}

// ----------------------------------------------------------------------------

bool
 TSonorkAuthReqWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDC_AUTH_REQ_SIGN )
	{
		sonork_skin.DrawSign(S->hDC,SKIN_SIGN_USERS,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkAuthReqWin::UpdateCxStatus()
{
	if(SonorkApp.CxReady())
		TSonorkWin::SetStatus_None(status_bar);
	else
		TSonorkWin::SetStatus(status_bar,GLS_MS_CXONLY,SKIN_HICON_ALERT);
	UpdateButtons();

}

