#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srktrackerwin.h"
#include "srkultraminwin.h"
#include "srk_url_codec.h"


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


#define TRK_SPACING			2
#define TRK_INPUT_HEIGHT		64
#define TRK_SEPARATOR_HEIGHT		4
#define TRK_SEPARATOR_PADDING		4
#define TRK_REPLY_BUTTON_HEIGHT		26
#define TRK_BUTTON_WIDTH		64
#define TRK_MAX_TEXT_SIZE		1496
#define TRK_MAX_ALIAS_SIZE		SONORK_USER_ALIAS_MAX_SIZE
#define TRK_MAX_LINE_SIZE		(TRK_MAX_ALIAS_SIZE+TRK_MAX_TEXT_SIZE+16)
#define TRK_CACHE_SIZE			110
//#define TRK_LEFT_MARGIN			20
//#define TRK_RIGHT_MARGIN		2
#define TRK_TEXT_PADDING		4
#define TRK_TEXT_SPACING		1
#define TRK_TRACKING_NO_PREFIX_MASK	0x3fff
#define TRK_TRACKING_NO_SEQUENCE_MASK	0x3ffff
#define TRK_TRACKING_NO_PREFIX_SHIFT	18
#define TRK_CIRCUIT_MARKER		0x1234 // just for safety(bug checking)

#define TRK_CIRCUIT_REQ_PARAM_INVITE	1
#define TRK_CTRLMSG_CMD_USER_DATA	SONORK_CTRLMSG_CMD_01
#define TRK_CTRLMSG_CMD_TEXT_DATA	SONORK_CTRLMSG_CMD_02


#define	TRK_POKE_SEND			SONORK_WIN_POKE_01
#define	TRK_POKE_FIND_LINKED		SONORK_WIN_POKE_02
#define UWF_QUERY_SAVE			SONORK_WIN_F_USER_01
#define UWF_CONNECTED			SONORK_WIN_F_USER_02
#define UWF_EVENT_PENDING		SONORK_WIN_F_USER_03
#define UWF_SELF_REQUESTED_CLOSE	SONORK_WIN_F_USER_04
// Use UWF_NO_SOUNDS to prevent AddText() from playing soft
// sound when new lines are added and window is not active
#define UWF_NO_SOUNDS			SONORK_WIN_F_USER_05

#define TOOL_BAR_ID		500
#define TOOL_BAR_BUTTONS	5

enum TOOL_BAR_BUTTON
{
  TOOL_BAR_BUTTON_SAVE		= 1000
, TOOL_BAR_BUTTON_SOUND
, TOOL_BAR_BUTTON_ADD_USER
, TOOL_BAR_BUTTON_SCROLL_LOCK
, TOOL_BAR_BUTTON_ULTRA_MINIMIZE
};

void
  TSonorkTrackerWin::DoSound( bool forced )
{
	if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_SOUND)& TBSTATE_CHECKED)
	|| TestWinUsrFlag(UWF_NO_SOUNDS))
		return;
	if( forced || last_sound_time!=SonorkApp.CurrentTime())
	{
		SonorkApp.AppSound( SONORK_APP_SOUND_MSG_LO );
		last_sound_time=SonorkApp.CurrentTime();
	}
}

// ----------------------------------------------------------------------------

void
  TSonorkTrackerWin::UltraMinimizedPaint(TSonorkUltraMinPaint* P)
{

	if( TestWinUsrFlag(UWF_EVENT_PENDING) )
	{
		P->icon = SKIN_ICON_EVENT;
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_FG);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_BG);
	}
	else
	{
		P->icon = SonorkApp.GetUserModeIcon( &user_data );
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MAIN_EXT
				,SKIN_CI_MAIN_ONLINE);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG);
	}
	strcpy(P->text,user_data.alias.CStr());
}

// ----------------------------------------------------------------------------

void
  TSonorkTrackerWin::ConditionalUltraMinimize()
{
	RECT	rect;
	if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_ULTRA_MINIMIZE)& TBSTATE_CHECKED) )
		return;

	if(ultra_min.win != NULL)
		return;
	ShowWindow(SW_HIDE);
	if(ultra_min.x==-1)
	{
		GetWindowRect(&rect);
		ultra_min.x = rect.left;
		ultra_min.y = rect.top;
	}

	ultra_min.win=new TSonorkUltraMinWin(this);
	ultra_min.win->Show(ultra_min.x,ultra_min.y);
}

// ----------------------------------------------------------------------------

BOOL
  TSonorkTrackerWin::IsConnected() const
{
	return TestWinUsrFlag(UWF_CONNECTED);
}

// ----------------------------------------------------------------------------
// TSonorkTrackerWin::ServiceCallback
// Service callback for the window instance
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
  TSonorkTrackerWin::ServiceCallback(
			 SONORK_DWORD2&			handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			event_tag
			,TSonorkAppServiceEvent*	E)
{
	TSonorkTrackerWin *_this;
	_this=(TSonorkTrackerWin*)handler_tag.v[0];
	switch(event_id)
	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:
		E->get_name.str->Set(_this->user_data.alias);
		return 1;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER )
			break;
		E->circuit_open.GetCircuitHandle(_this->circuit);
		_this->SetWindowText( GLS_MS_CXWFREPLY );
		break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_UPDATE:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER )
			break;
		E->circuit_update.GetCircuitHandle(_this->circuit);
	break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_DATA:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER )
			break;
		_this->OnCircuitData(
			  E->circuit_data.Cmd()
			, E->circuit_data.UserParam()
			, E->circuit_data.UserFlags()
			, E->circuit_data.Data()
			, E->circuit_data.DataSize());
	break;
	case SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER )
			break;
		_this->OnCircuitClose(E->circuit_close.Result());
	break;

	}
	return 0;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkTrackerWin::Service_Register(
	 TSonorkAppServiceCallbackPtr 	cb_ptr
	,void*				cb_dat)
{
	SONORK_DWORD2			sii_tag;
	TSonorkServiceInstanceInfo	sii;

	sii_tag.v[0]=(DWORD)cb_dat;
	sii_tag.v[1]=0;
	sii.SetInstanceInfo(
			  SONORK_SERVICE_ID_SONORK_TRACKER
			, SONORK_SERVICE_TYPE_SONORK_CHAT
			, 0	// hflags
			, 0	// instance: Don't know yet, set to 0
			, SONORK_TRACKER_VERSION_NUMBER);


	if(SonorkApp.Service_Register(
		  SONORK_APP_SERVICE_TYPE_CLIENT
		, &sii
		, SKIN_ICON_TRACKER
		, cb_ptr
		, &sii_tag)==SONORK_RESULT_OK)
		return sii.ServiceInstance();
	return 0;

}

// ----------------------------------------------------------------------------

BOOL
 TSonorkTrackerWin::Call(const TSonorkUserData*UD, SONORK_C_CSTR text)
{
	char 	tmp[192];
	SONORK_RESULT	result;
	SONORK_DWORD2	circuit_tag;
	TSonorkTrackerInviteData	invitation;
	assert( UD != NULL );

	user_data.Set( *UD );
	if( user_data.GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1)
		return false;

	SetWinUsrFlag(UWF_NO_SOUNDS);
	
	if( !UD->addr.sidFlags.TrackerEnabled() )
	{
		SonorkApp.LangSprintf(tmp
			, GLS_TR_USRNENA
			, UD->alias.CStr());
		AddSysLine(tmp);
		result = SONORK_RESULT_NOT_ACCEPTED;
	}
	else
	{

		service_instance = Service_Register(ServiceCallback,this);

		user_data.GetUserHandle(&circuit);
		circuit.LoadTarget( SONORK_SERVICE_ID_SONORK_TRACKER
				, SONORK_SERVICE_LOCATOR_INSTANCE(0));
		circuit_tag.v[0]=0;
		circuit_tag.v[1]=TRK_CIRCUIT_MARKER;

		invitation.CODEC_Clear();
		invitation.user_data.Set( SonorkApp.ProfileUser() );
		invitation.text.SetStr(0,text);

		result= SonorkApp.Service_OpenCircuit(
				  service_instance
				, &circuit
				, TRK_CIRCUIT_REQ_PARAM_INVITE
				, 0
				, SONORK_MSECS_FOR_CIRCUIT_CONNECTION
				, &invitation
				, &circuit_tag
				);
		if( result == SONORK_RESULT_OK)
		{
			SonorkApp.SetBichoSequence(SONORK_SEQUENCE_RADAR);
			SonorkApp.AppSound(SONORK_APP_SOUND_TRACKER);
			SonorkApp.LangSprintf(tmp
				, GLS_TR_CALLING
				, user_data.alias.CStr());
			SetWindowText(tmp);

			ClearWinUsrFlag(UWF_NO_SOUNDS);
			return true;
		}
	}
	ClearWinUsrFlag( UWF_SELF_REQUESTED_CLOSE );
	OnCircuitClose( result );
	ClearWinUsrFlag(UWF_NO_SOUNDS);
	return false;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkTrackerWin::Accept(const TSonorkTrackerPendingInvitation* pending_req)
{

	SONORK_RESULT	result;
	SONORK_DWORD2	sii_tag;

	user_data.Set( pending_req->invite_data.user_data );
	circuit.Set(pending_req->circuit);


	if( user_data.GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1)
	{
		TRACE_DEBUG("INVALID USER INFO LEVEL");
		return false;
	}

	SetWinUsrFlag(UWF_NO_SOUNDS);

	sii_tag.v[0]=(DWORD)this;
	sii_tag.v[1]=0;

	service_instance = pending_req->service_instance;
	result= SonorkApp.Service_SetCallback(
			  service_instance
			, ServiceCallback
			, &sii_tag
			);

	if( result == SONORK_RESULT_OK)
	{
		SonorkApp.SetBichoSequenceIf(SONORK_SEQUENCE_RADAR
			, SONORK_SEQUENCE_CALL);
		SetWinUsrFlag(UWF_CONNECTED);
		SetWindowText( user_data.alias.CStr() );
		AddSysLine( GLS_MS_CXTED );
		SendProfileData();

		ClearWinUsrFlag(UWF_NO_SOUNDS);
		return true;
	}
	ClearWinUsrFlag( UWF_SELF_REQUESTED_CLOSE );
	OnCircuitClose( result );
	service_instance=0;
	ClearWinUsrFlag(UWF_NO_SOUNDS);
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnCircuitData(
			  SONORK_CTRLMSG_CMD cmd
			, DWORD //user_param
			, DWORD //user_flags
			, const BYTE* data
			, DWORD data_size)
{
	switch(cmd)
	{
		case TRK_CTRLMSG_CMD_TEXT_DATA:
		{
			OnCircuitData_Text(data,data_size);
		}
		break;
		case TRK_CTRLMSG_CMD_USER_DATA:
		{
			OnCircuitData_User(data,data_size);
		}
		break;


	}
}
// ----------------------------------------------------------------------------
void
 TSonorkTrackerWin::OnCircuitData_Text(const BYTE* data , DWORD data_size)
{
	if(temp_TD.CODEC_ReadMem(data,data_size)!=SONORK_RESULT_OK)
		return;
	temp_TD.header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]|=SONORK_CCLF_INCOMMING;
	temp_TD.data.Clear();
	AddUserText(&temp_TD);
}

void
 TSonorkTrackerWin::OnCircuitData_User(const BYTE* data , DWORD data_size)
{
	TSonorkUserData	UD;
	if(UD.CODEC_ReadMem(data,data_size)!=SONORK_RESULT_OK)
	{
		SonorkApp.Service_CloseCircuit(
		  service_instance
		, circuit.CircuitId()
		, SONORK_RESULT_CODEC_ERROR);
		return;
	}
	user_data.Set( UD );
	if(!TestWinUsrFlag(UWF_CONNECTED))
	{
		SonorkApp.SetBichoSequenceIf(SONORK_SEQUENCE_RADAR
			, SONORK_SEQUENCE_CALL);
		SetWinUsrFlag(UWF_CONNECTED);
		AddSysLine( GLS_MS_CXTED );
	}
	SetWindowText( user_data.alias.CStr() );
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OpenLine( DWORD line_no )
{
	TSonorkCCacheEntry * 	CL;
	SONORK_C_CSTR		pStr;

	if((CL = console->Get( line_no , &pStr)) == NULL )
		return;
	if( CL->tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]&SONORK_TRACKER_TEXT_FLAG_URL )
	{
		SonorkApp.OpenUrl(this , pStr);
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnCircuitClose(SONORK_RESULT result)
{
	char tmp[160];
	SONORK_SYS_STRING	gss;
	SONORK_C_CSTR		str;
	if( TestWinUsrFlag(UWF_SELF_REQUESTED_CLOSE) )
	{
		SonorkApp.SetBichoSequenceIf(SONORK_SEQUENCE_RADAR, SONORK_SEQUENCE_ERROR);
	}
	else
	{
		// Only do error sound the disconnection was unexpected
		SonorkApp.SetBichoSequenceError( true );
	}

	switch(result)
	{
		case SONORK_RESULT_TIMEOUT:
			gss=GSS_REQTIMEOUT;
			break;
		case SONORK_RESULT_INVALID_HANDLE:
		case SONORK_RESULT_INVALID_OPERATION:
			gss=GSS_OPNOTALLOWED;
			break;
		case SONORK_RESULT_NOT_ACCEPTED:
		case SONORK_RESULT_ACCESS_DENIED:
			gss=GSS_REQDENIED;
			break;
		case SONORK_RESULT_NETWORK_ERROR:
			gss=GSS_NETERR;
			break;
		case SONORK_RESULT_USER_TERMINATION:
		case SONORK_RESULT_FORCED_TERMINATION:
			gss=GSS_USRCANCEL;
			break;
		case SONORK_RESULT_CODEC_ERROR:
		case SONORK_RESULT_PROTOCOL_ERROR:
			gss=GSS_BADNETPROTOCOL;
			break;

		default:
			gss=GSS_NULL;
			break;

	}

	str=SonorkApp.LangString(
			TestWinUsrFlag(UWF_CONNECTED)
			?GLS_MS_DCXTED
			:GLS_MS_CXFAIL);
	strcpy(tmp,str);
	if( gss!=GSS_NULL)
	{
		wsprintf(tmp+strlen(tmp)
			, " (%s)"
			, SonorkApp.SysString(gss));
	}
	AddSysLine(tmp);
	wsprintf(tmp
		    , "%s (%s)"
		    , user_data.alias.CStr()
		    , str);
	SetWindowText( tmp );


	circuit.Clear();
	ClearWinUsrFlag(UWF_CONNECTED);
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::CmdAddUser()
{
	TSonorkError	ERR;
	if(!IsConnected())
		return;
	SonorkApp.CmdAddUser(NULL,ERR,user_data.userId,0);

}
// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::SendInputText( BOOL reply )
{
	TSonorkCCacheEntry *CL;
	if(!IsConnected())
		return;
	ConditionalUltraMinimize();
	GetCtrlText( IDC_TRACKER_INPUT , temp_TD.text );
	temp_TD.text.CutAt(TRK_MAX_TEXT_SIZE);
	if( temp_TD.text.Length() )
	{
		temp_TD.data.Clear();
		temp_TD.header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]=
		temp_TD.header.tag.v[SONORK_TRACKER_TEXT_TAG_ATTR]= 0;
		temp_TD.header.tracking_no.v[1]=0;
		temp_TD.header.reserved.Clear();
		if(reply)
		{
			// Replying to
			if((CL = console->GetFocused(NULL,NULL))!=NULL)
			{
				temp_TD.header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]|=SONORK_CCLF_IN_THREAD;
				temp_TD.header.tracking_no.v[1] = CL->tracking_no.v[0];
			}
		}
		if( SONORK_IsUrl( temp_TD.text.CStr() ) )
			temp_TD.header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]|=SONORK_TRACKER_TEXT_FLAG_URL;

		temp_TD.header.tracking_no.v[0]=SonorkApp.GenTrackingNo(user_data.userId);
		SonorkApp.Service_SendCircuitData(
				  service_instance
				, &circuit
				, TRK_CTRLMSG_CMD_TEXT_DATA
				, 0
				, 0
				, &temp_TD);

		AddUserText(&temp_TD);
		SetCtrlText( IDC_TRACKER_INPUT , (char*)NULL);
		temp_TD.text.Clear();
	}

}


// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::AddUserText( TSonorkTrackerTextData* TD)
{
	TD->header.tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]&=~SONORK_TRACKER_TEXT_FLAG_SYSTEM;
	AddText(  TD->text.ToCStr()
		, TD->header.tag
		, TD->header.tracking_no);
	SetWinUsrFlag(UWF_QUERY_SAVE);
}


// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::AddSysLine(SONORK_C_CSTR str)
{
	TSonorkTempBuffer tmp(TRK_MAX_TEXT_SIZE + 64);
	SONORK_DWORD2 tag;
	SONORK_DWORD2 tracking_no;
	tag.Set(SONORK_TRACKER_TEXT_FLAG_SYSTEM
		,sonork_skin.Color(SKIN_COLOR_CHAT_EXT,SKIN_CI_CHAT_SYS));
	tracking_no.Clear();
	AddText(str
		,tag
		,tracking_no);
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::AddSysLine( GLS_INDEX gls )
{
	AddSysLine(SonorkApp.LangString(gls));
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::AddText(SONORK_C_CSTR pStr
		, const SONORK_DWORD2& tag
		, const SONORK_DWORD2& tracking_no)
{
	TSonorkCCacheEntry		CL;
	TSonorkCodecLCStringAtom	A;

	A.ptr 			= (char*)pStr;
	A.length_or_size    	= strlen(pStr);

	db.Add(&A,&CL.dat_index);

	CL.tag.Set( tag );
	CL.tracking_no.Set(tracking_no);
	CL.time.SetTime_Local();
	CL.ext_index = 0;


	console->Add( CL );
	if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_SCROLL_LOCK)& TBSTATE_CHECKED ))
		console->MakeLastLineVisible();
	if( tag.v[SONORK_TRACKER_TEXT_TAG_FLAG]&(SONORK_CCLF_INCOMMING|SONORK_TRACKER_TEXT_FLAG_SYSTEM) )
	if( !IsActive() || IsUltraMinimized())
	{
		DoSound( false );

		if( !IsUltraMinimized())
		{
			FlashWindow(Handle() , true);
			Sleep(50);
			FlashWindow(Handle() , true);
		}
		else
		{
			if(!TestWinUsrFlag(UWF_EVENT_PENDING))
				ultra_min.win->InvalidateRect(NULL,false);
		}
		SetWinUsrFlag(UWF_EVENT_PENDING);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::SendProfileData()
{
	const TSonorkUserData*UD;
	if( !IsConnected() )
		return;
	UD = &SonorkApp.ProfileUser();
	SonorkApp.Service_SendCircuitData(
			  service_instance
			, &circuit
			, TRK_CTRLMSG_CMD_USER_DATA
			, 0
			, 0
			, UD);
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkTrackerWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch(wParam)
	{
		case TRK_POKE_SEND:
			SendInputText( lParam );
		break;

		case TRK_POKE_FIND_LINKED:
			console->FocusLinked( false );
		break;

		case SONORK_WIN_POKE_DESTROY:
			Destroy();
		break;

		case  SONORK_WIN_POKE_ULTRA_MIN_DESTROY:
			ultra_min.x = ultra_min.win->Left();
			ultra_min.y = ultra_min.win->Top();
			ultra_min.win=NULL;

			console->MakeLastLineVisible();
			SonorkApp.PostAppCommand(
				 SONORK_APP_COMMAND_FOREGROUND_HWND
				,(LPARAM)Handle());

			break;

		case SONORK_WIN_POKE_ULTRA_MIN_PAINT:
			UltraMinimizedPaint((TSonorkUltraMinPaint*)lParam);
			break;


	}
	return 0L;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkTrackerWin::OnQueryClose()
{
	if(TestWinUsrFlag(UWF_QUERY_SAVE))
	{
		UINT id;
		id = MessageBox(GLS_QU_SAVEXIT
			,GLS_TR_NAME
			,MB_YESNOCANCEL|MB_ICONQUESTION);
		if(id==IDCANCEL)
			return true;
		Stop();
		if(id==IDYES)
			CmdSave();
	}
	return false;
}

// ----------------------------------------------------------------------------

bool
 TSonorkTrackerWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=320;
	MMI->ptMinTrackSize.y=240;
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnActivate(DWORD flags , BOOL )
{
	if( flags==WA_INACTIVE	)
	{
		SonorkApp.CancelPostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)input.hwnd);
	}
	else
	{
		if( !IsUltraMinimized() && !IsIconic() )
		{
			ClearWinUsrFlag(UWF_EVENT_PENDING);
			SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND
				, (LPARAM)input.hwnd);
		}

	}
}

void
 TSonorkTrackerWin::OnSize(UINT size_type)
{
	if( size_type == SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
	{
		RealignControls();
	}
}

// ---------------------------------------------------------------------------

void
 TSonorkTrackerWin::RealignControls()
{
	const UINT H = Height() ;
	const UINT W = Width();
	int   	y,consoleH,inputW;
	UINT	cW;
	HDWP 	defer_handle;
	HWND	separator=GetDlgItem(IDC_TRACKER_SEPARATOR);

	cW		= W-TRK_SPACING * 2;
	consoleH	= H -
				(input.height
				+ TRK_SEPARATOR_HEIGHT
				+ TRK_SEPARATOR_PADDING*2
				+ TRK_SPACING * 3
				+ toolbar.height
				+ 4 // bottom spacing
				);
	inputW		=cW
			- TRK_BUTTON_WIDTH
			- TRK_SPACING*2;

	defer_handle = BeginDeferWindowPos( 7 );

	defer_handle = DeferWindowPos(defer_handle
		,toolbar.hwnd
		,NULL
		,0
		,TRK_SPACING
		,W
		,toolbar.height
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y = TRK_SPACING*2+toolbar.height;

	defer_handle = DeferWindowPos(defer_handle
		,console->Handle()
		,NULL
		,TRK_SPACING
		,y
		,cW
		,consoleH
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=consoleH+TRK_SEPARATOR_PADDING;

	defer_handle = DeferWindowPos(defer_handle
		,separator
		,NULL
		,TRK_SPACING
		,y
		,cW
		,TRK_SEPARATOR_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=TRK_SEPARATOR_HEIGHT+TRK_SEPARATOR_PADDING;

	defer_handle = DeferWindowPos(defer_handle
		,input.hwnd
		,NULL
		,TRK_SPACING
		,y
		,inputW
		,input.height
		,SWP_NOZORDER|SWP_NOACTIVATE);

	consoleH = inputW + TRK_SPACING * 3;
	defer_handle = DeferWindowPos(defer_handle
		,GetDlgItem(IDC_TRACKER_REPLY)
		,NULL
		,consoleH
		,y
		,TRK_BUTTON_WIDTH
		,TRK_REPLY_BUTTON_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=TRK_REPLY_BUTTON_HEIGHT+TRK_SPACING;

	defer_handle = DeferWindowPos(defer_handle
		,GetDlgItem(IDOK)
		,NULL
		,consoleH
		,y
		,TRK_BUTTON_WIDTH
		,input.height - (TRK_REPLY_BUTTON_HEIGHT+TRK_SPACING)
		,SWP_NOZORDER|SWP_NOACTIVATE);


	EndDeferWindowPos(defer_handle);
	 ::InvalidateRect(separator,NULL,false);
}

// ---------------------------------------------------------------------------

bool
 TSonorkTrackerWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	DWORD		state;
	int		delta;
	SKIN_ICON	icon;
	if(S->CtlID == IDOK || S->CtlID==IDC_TRACKER_REPLY)
	{
		icon = S->CtlID == IDOK
			?SKIN_ICON_SEND_MSG
			:SKIN_ICON_REPLY_MSG;
		if(S->itemState&ODS_SELECTED)
		{
			delta=1;
			state = DFCS_BUTTONPUSH|DFCS_PUSHED;
		}
		else
		{
			state = DFCS_BUTTONPUSH;
			delta = 0;
		}
		DrawFrameControl(S->hDC
			, &S->rcItem
			, DFC_BUTTON
			, state);

		sonork_skin.DrawIcon(S->hDC
			, icon
			, S->rcItem.left + delta + ( (S->rcItem.right - S->rcItem.left) - SKIN_ICON_SW )/2
			, S->rcItem.top  + delta + ( (S->rcItem.bottom - S->rcItem.top) - SKIN_ICON_SH )/2
			, S->itemState&ODS_DISABLED?ILD_BLEND25:ILD_NORMAL);
		return true;
	}
	return false;

}

// ----------------------------------------------------------------------------

bool
 TSonorkTrackerWin::OnCommand(UINT id,HWND hwnd, UINT code)
{
	if( hwnd == toolbar.hwnd )
	{
		if(code==BN_CLICKED)
		switch(id)
		{
			case TOOL_BAR_BUTTON_SAVE:
				CmdSave();
			break;

			case TOOL_BAR_BUTTON_SCROLL_LOCK:
			if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_SCROLL_LOCK)& TBSTATE_CHECKED ))
				console->MakeLastLineVisible();
			break;

			case TOOL_BAR_BUTTON_SOUND:
				// Clear UWF_NO_SOUNDS just in case
				// we left it on by mystake (you never know..)
				ClearWinUsrFlag(UWF_NO_SOUNDS);
				DoSound( true );
			break;

			case TOOL_BAR_BUTTON_ADD_USER:
				CmdAddUser();
				break;

			case TOOL_BAR_BUTTON_ULTRA_MINIMIZE:
				ConditionalUltraMinimize();
			break;

			default:
				return false;
		}
		return true;
	}
	else
	if(code==BN_CLICKED && (id==IDOK || id==IDC_TRACKER_REPLY))
	{
		SendInputText(id==IDC_TRACKER_REPLY);
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND
			,(LPARAM)input.hwnd);
	}
	else
	if( hwnd == NULL )
	{
		switch(id)
		{
			case CM_MSGS_TOGGLE_SEL:
				console->SelectToggleFocused();
			break;

			case CM_MSGS_CLEAR_SEL:
				console->ClearSelection();
			break;

			case CM_CLIPBOARD_COPY:
				console->CopyToClipboard(0);
			break;

			case CM_EXPORT:
				CmdSave();
			break;
			
			case CM_MSGS_SINGLE_LINE_MODE:
				console->SetMaxScanLines(
					console->GetMaxScanLines() == 1
					?SONORK_HIST_WIN_DEFAULT_MAX_SCAN_LINES
					:1);
				break;

			default:
				return false;

		}
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------
// Console/History callbacks
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkTrackerWin::ConsoleCallback(void* 		pTag
				,SONORK_CONSOLE_EVENT 	gcEvent
				,DWORD			pIndex
				,void*			pData)
{
	TSonorkTrackerWin*_this = (TSonorkTrackerWin*)pTag;
	switch(gcEvent)
	{
	case SONORK_CONSOLE_EVENT_HISTORY_EVENT:
		return _this->OnHistory_Event((TSonorkHistoryWinEvent*)pData);

	case SONORK_CONSOLE_EVENT_EXPORT:
		return _this->OnConsole_Export((TSonorkConsoleExportEvent*)pData);

	case SONORK_CONSOLE_EVENT_VKEY:
		if( pIndex == 10 || pIndex == VK_RETURN )
		{
			_this->SendPoke(TRK_POKE_SEND,0);
			return true;
		}
		else
		if( pIndex == VK_ESCAPE )
		{
			_this->ClrCtrlText(IDC_TRACKER_INPUT);
			return true;
		}
		break;
	}
	return 0L;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkTrackerWin::OnHistory_Event(TSonorkHistoryWinEvent*E)
{
	switch( E->Event() )
	{
	case SONORK_HIST_WIN_EVENT_LINE_PAINT:
		OnHistory_LinePaint( E->Line(), E->PaintContext() );
	break;

	case SONORK_HIST_WIN_EVENT_GET_TEXT:
		OnHistoryWin_GetText(E->LineNo(),E->GetTextData());
	break;

	case SONORK_HIST_WIN_EVENT_SEL_CHANGE:
	break;

	case SONORK_HIST_WIN_EVENT_LINE_CLICK:
		if( !(E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED ))
		{
		// Don't search for linked line if focus has changed
		// because it does not look right. If we do that, the
		// user will click on the line (at the icon) but
		//  the focus will go elsewhere (to the linked line).
			if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_LICON_CLICK) )
			{
				PostPoke(TRK_POKE_FIND_LINKED,false);
				break;
			}
		}
		if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
		{
			OpenLine(E->LineNo());
			break;
		}

		if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
		{
			TrackPopupMenu(SonorkApp.ChatViewMenu()
					, TPM_LEFTALIGN  | TPM_LEFTBUTTON
					,E->ClickPoint().x
					,E->ClickPoint().y
					,0
					,Handle()
					,NULL);
		}
	break;

	case SONORK_HIST_WIN_EVENT_LINE_DRAG:
		OnHistory_LineDrag( E->LineNo() );
	break;

	}
	return 0L;
}


// ---------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnHistory_LinePaint(
 	const TSonorkCCacheEntry*CL, TSonorkHistoryWinPaintCtx*CTX)
{
	DWORD 		cFlags=CL->tag.v[SONORK_TRACKER_TEXT_TAG_FLAG];
	DWORD&		ctxFlags=CTX->flags;
//	HBRUSH		brush;
//	SKIN_ICON	l_icon;
	SKIN_COLOR	color;
//	ctxFlags |= SONORK_HIST_WIN_PAINT_F_LEFT_PAINTED;
	ctxFlags &=~(SONORK_HIST_WIN_PAINT_F_HOT_LICON);
	if( cFlags&SONORK_TRACKER_TEXT_FLAG_URL)
	{
		CTX->r_icon=SKIN_ICON_MORE_URL;
	}


	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED )
	{
		color = SKIN_COLOR_MSG_FOCUS;
	}
	else
	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_SELECTED )
	{
		color = SKIN_COLOR_MSG_SELECT;
	}
	else
	if( cFlags & SONORK_APP_CCF_INCOMMING )
	{
		color = SKIN_COLOR_MSG_IN_OLD;
	}
	else
	{
		color = SKIN_COLOR_MSG_OUT;
	}
	CTX->SetTextColor(sonork_skin.Color(color ,SKIN_CI_FG));
	CTX->SetLineColor(sonork_skin.Color(color ,SKIN_CI_BG));
//	::FillRect(CTX->hDC(),CTX->LeftRect(),brush);
	if( cFlags & SONORK_TRACKER_TEXT_FLAG_SYSTEM )
	{
		CTX->l_icon=SKIN_ICON_INFO;
	}
	else
	{
		if( cFlags&SONORK_CCLF_IN_THREAD )
		{
			ctxFlags|=SONORK_HIST_WIN_PAINT_F_HOT_LICON;
			CTX->l_icon = cFlags&SONORK_CCLF_INCOMMING
					?SKIN_ICON_IN_REPLY
					:SKIN_ICON_OUT_REPLY;
		}
		else
		{
			CTX->l_icon = cFlags&SONORK_CCLF_INCOMMING
					?SKIN_ICON_INCOMMING
					:SKIN_ICON_OUTGOING;


		}
	}


}


// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnHistoryWin_GetText(
	 DWORD		line_no
	,TSonorkDynData*DD)
{
	SONORK_C_CSTR	pszText;
	if(cache->Get( line_no, &pszText , NULL ))
		DD->AppendStr(pszText,false);
}


// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnHistory_LineDrag(DWORD line_no)
{
	TSonorkDropSourceData 	*SD;
	TSonorkClipData         *sCD;
	const TSonorkCCacheEntry*pE;
	SONORK_C_CSTR		pszText;
	DWORD			aux=0;
	TSonorkTempBuffer	buffer( TRK_MAX_LINE_SIZE );

	SD = new TSonorkDropSourceData;
	sCD = SD->GetClipData();
	if( console->SelectionActive() )
	{
		TSonorkListIterator I;
		TSonorkClipData    *nCD;
		DWORD			line_no;
		sCD->SetSonorkClipDataQueue();
		console->SortSelection();
		console->InitEnumSelection(I);
		while((line_no = console->EnumNextSelection(I))!=SONORK_INVALID_INDEX)
		{
			pE = cache->Get( line_no , &pszText , NULL );
			if( pE==NULL )
				continue;
			nCD = new TSonorkClipData;
			nCD->SetCStr( SONORK_CLIP_DATA_TEXT , pszText );

			if(sCD->AddSonorkClipData( nCD ))
				aux++;
			nCD->Release();
		}
	}
	else
	{
		pE = cache->Get(line_no , &pszText , NULL );
		if( pE!=NULL )
		{
			sCD->SetCStr( SONORK_CLIP_DATA_TEXT , pszText );
			aux=1;
		}
	}
	if( aux > 0 )
	{
		aux=0;
		SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
	}
	SD->Release();
}

// ----------------------------------------------------------------------------

BOOL SONORK_CALLBACK
 TSonorkTrackerWin::CCacheCallback(void*tag,TSonorkCCacheEntry*pE,char*str,UINT str_size)
{
	TSonorkTrackerWin *_this = (TSonorkTrackerWin*)tag;
	TSonorkCodecLCStringAtom	A;
	A.ptr = str;
	A.length_or_size = str_size;
	if(_this->db.Get(pE->dat_index,&A) != SONORK_RESULT_OK )
		strcpy(str,"<error>");
	return false;
}

// ----------------------------------------------------------------------------

TSonorkTrackerWin::TSonorkTrackerWin()
	:TSonorkWin(NULL
	, SONORK_WIN_CLASS_NORMAL
	 |SONORK_WIN_TYPE_NONE
	 |SONORK_WIN_DIALOG
	 |IDD_TRACKER
	, SONORK_WIN_SF_NO_WIN_PARENT
	)
{
	cache = NULL;
	service_instance=0;
	ultra_min.win = NULL;
	ultra_min.x=-1;
	circuit.Clear();
	SetEventMask( SONORK_APP_EM_PROFILE_AWARE|SONORK_APP_EM_SKIN_AWARE );

}

// ----------------------------------------------------------------------------

TSonorkTrackerWin::~TSonorkTrackerWin()
{
	if(cache)
		delete cache;
}

// ----------------------------------------------------------------------------

bool
 TSonorkTrackerWin::OnCreate()
{
	char			name[48];
	TSonorkTempBuffer 	path(SONORK_MAX_PATH);
	SIZE			size;
	bool			db_reset;
	static TSonorkWinToolBarButton
		btn_info[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_BUTTON_SAVE
			, SKIN_ICON_FILE_DOWNLOAD
			, GLS_OP_STORE
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
	,	{	TOOL_BAR_BUTTON_SOUND
			, SKIN_ICON_SOUND
			, GLS_OP_SOUND
			, TBSTATE_ENABLED|TBSTATE_CHECKED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE }
	,	{	TOOL_BAR_BUTTON_ADD_USER
			, SKIN_ICON_ADD_USER
			, GLS_OP_ADDUSR
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
	,	{	TOOL_BAR_BUTTON_SCROLL_LOCK
			, SKIN_ICON_NO_SCROLL
			, GLS_LB_SCRLCK
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE }
	,	{ 	TOOL_BAR_BUTTON_ULTRA_MINIMIZE
			, SKIN_ICON_CLOSE_UP
			, GLS_OP_UMIN
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE  }
	};

	toolbar.hwnd=TSonorkWin::CreateToolBar(Handle()
			,	TOOL_BAR_ID
			,	 WS_VISIBLE
				| TBSTYLE_TOOLTIPS
				| TBSTYLE_FLAT
				| TBSTYLE_LIST
				| CCS_NOPARENTALIGN
				| CCS_NODIVIDER
				| CCS_NORESIZE
			, TOOL_BAR_BUTTONS
			, btn_info
			, &size);
	toolbar.height = size.cy;

	input.hwnd	= GetDlgItem( IDC_TRACKER_INPUT );
	input.height	= TRK_INPUT_HEIGHT;
	input.drop_target.AssignCtrl( Handle(), input.hwnd);

	SetEditCtrlMaxLength(IDC_TRACKER_INPUT
		,TRK_MAX_TEXT_SIZE-1 );

	wsprintf(name,"~%u.%u~%x_wsl"
		,SonorkApp.ProfileUserId().v[0]
		,SonorkApp.ProfileUserId().v[1]
		,this);
	SonorkApp.GetDirPath(path.CStr(),SONORK_APP_DIR_TEMP,name);
	db_reset=true;
	db.Open( SonorkApp.ProfileUserId() , path.CStr() , db_reset );

	SonorkApp.GetTempPath(path.CStr(),"wsl",NULL,(DWORD)this);
	cache = new TSonorkCCache(TRK_MAX_LINE_SIZE
			, TRK_CACHE_SIZE
			, CCacheCallback
			, this);
	cache->Open( path.CStr() );
	cache->Clear( true );

	console = new TSonorkConsole(this,cache,ConsoleCallback,this,0);
	console->Create();

	input.ctrl.AssignCtrl(console,input.hwnd,IDC_TRACKER_INPUT);

	console->EnableSelect( true );
	console->EnableLineDrag( true );
//	console->SetMargins(TRK_LEFT_MARGIN,TRK_RIGHT_MARGIN);
	console->SetPaddingEx(TRK_TEXT_PADDING,TRK_TEXT_SPACING);
	console->SetDefColors(SKIN_COLOR_CHAT);



	RealignControls();
	console->ShowWindow(SW_SHOW);
	SetCaptionIcon( SKIN_HICON_TRACKER );
	ShowWindow(SW_SHOW);
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::Stop()
{
	SetWinUsrFlag(UWF_SELF_REQUESTED_CLOSE);
	if( service_instance != 0)
	{
		SonorkApp.Service_Unregister(service_instance , false);
		circuit.Clear();
		service_instance=0;
	}
}
// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::OnBeforeDestroy()
{
	Stop();
	input.ctrl.ReleaseCtrl();
	input.drop_target.Enable(false);
	console->Destroy();
	cache->Close();
	db.Close();
}

// ----------------------------------------------------------------------------

bool
 TSonorkTrackerWin::OnAppEvent(UINT event, UINT  ,void* )
{
	switch( event)
	{
		case SONORK_APP_EVENT_SET_PROFILE:
		case SONORK_APP_EVENT_SID_CHANGED:
			SendProfileData();
			return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkTrackerWin::LoadLabels()
{
	::SendMessage(input.hwnd
		, WM_SETFONT
		, (WPARAM)sonork_skin.Font(SKIN_FONT_LARGE)
		, MAKELPARAM(1,0));
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkTrackerWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	union
	{
		LPARAM			lParam;
		TSonorkDropQuery*	query;
		TSonorkDropMsg*		msg;
		TSonorkDropExecute*	exec;
	}D;
	TSonorkClipData*   	CD;
	D.lParam=lParam;
	switch(event)
	{
		case SONORK_DRAG_DROP_QUERY:
			SonorkApp.ProcessDropQuery(D.query
			,SONORK_DROP_ACCEPT_F_SONORKCLIPDATA
			|SONORK_DROP_ACCEPT_F_URL
			|SONORK_DROP_ACCEPT_F_TEXT
			);
		break;
		case SONORK_DRAG_DROP_UPDATE:
			*D.msg->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE;
		break;

		case SONORK_DRAG_DROP_CANCEL:
		break;

		case SONORK_DRAG_DROP_EXECUTE:
			CD = SonorkApp.ProcessDropExecute(D.exec);
			if( CD != NULL )
			{
				SetCtrlDropText( IDC_TRACKER_INPUT , CD , "\r\n");
				CD->Release() ;
			}
		break;
	}
	return 0;
}

// ----------------------------------------------------------------------------
// Export

struct TSonorkTrackerExportTag
{
	TSonorkShortString	str;
	char			bg_color[16];
};

void
 TSonorkTrackerWin::CmdSave()
{
	TSonorkShortString 		path;
	TSonorkTrackerExportTag    	tag;

	TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_CHAT, SKIN_CI_BG) , tag.bg_color);

	console->ClearSelection();
	console->CloseView();
	path.SetBufferSize(SONORK_MAX_PATH);
	strcpy(path.Buffer(),"chat");
	console->Export(path.Buffer()
		,SONORK_CONSOLE_EXPORT_F_ASK_PATH
		|SONORK_CONSOLE_EXPORT_F_ADD_TIME_SUFFIX
		|SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS
		,&tag
		,NULL);
	ClearWinUsrFlag(UWF_QUERY_SAVE);
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkTrackerWin::OnConsole_Export(TSonorkConsoleExportEvent* EV)
{
	char		     	tmp1[64];
	char                 	fg_color[16];
	TSonorkTrackerExportTag	*tag;
	TSonorkCCacheEntry	*pEntry;
	SONORK_C_CSTR		line_str;

	tag = (TSonorkTrackerExportTag*)EV->tag;

	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_LINE)
	{
		pEntry =EV->data.line;
		if(db.Get(pEntry->dat_index
			,&TSonorkCodecShortStringAtom(&tag->str)) != SONORK_RESULT_OK )
			return 0L;
		TSonorkConsole::WinToRGB(pEntry->tag.v[SONORK_TRACKER_TEXT_TAG_ATTR]&0xffffff , fg_color );
		line_str = tag->str.CStr();
	}
	else
	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_START )
	{
		lstrcpyn(tmp1,user_data.alias.CStr(),sizeof(tmp1));
	}

	if( EV->format == SONORK_CONSOLE_EXPORT_HTML )
	{
		switch(EV->section)
		{
			case SONORK_CONSOLE_EXPORT_SECTION_START:
				{

					fputs("<HTML>\n<HEAD>\n<TITLE>",EV->file);
					fprintf(
						EV->file
						,"%u.%u : "
						,SonorkApp.ProfileUserId().v[0]
						,SonorkApp.ProfileUserId().v[1]
						);
					SONORK_HtmlPuts( EV->file, tmp1 );
					fprintf(EV->file," (%u/%u/%u %u:%u)</TITLE>\n"
						"</HEAD>\n"
						"<BODY>\n"
						"<BASEFONT FACE=\"Arial,Helvetica\">\n"
						,EV->st.wYear
						,EV->st.wMonth
						,EV->st.wDay
						,EV->st.wHour
						,EV->st.wMinute);
					fprintf(EV->file,
						"<CENTER>\n"
						"<TABLE BORDER=0 CELLSPACING=1 CELLPADDING=4>\n");
				}
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_COMMENTS:
				if(*EV->data.comments)
				{
					fputs(" <TR>\n"
						 "  <TD COLSPAN=2 BGCOLOR=#F0F0E0><font size=+1><b>"
						 ,EV->file);
					SONORK_HtmlPuts(EV->file,EV->data.comments);
					fputs("</b></font></TD>\n </TR>\n",EV->file);
				}
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:

				fprintf(EV->file,"<TR><TD BGCOLOR=#%s><font color=#%s>"
					,tag->bg_color
					,fg_color);

				SONORK_HtmlPuts(EV->file, line_str );
				fputs("</TD></TR>\n",EV->file);
				
				break;
			case SONORK_CONSOLE_EXPORT_SECTION_END:
			fprintf(EV->file,
				"</TABLE>\n"
				"<br><br><FONT SIZE=-2>Sonork Export</FONT>"
				"</CENTER>\n"
				"</BODY>\n"
				"</HTML>\n");
				break;
		}
	}
	else
	if( EV->format == SONORK_CONSOLE_EXPORT_TEXT )
	{
		switch(EV->section)
		{
			case SONORK_CONSOLE_EXPORT_SECTION_START:
				{
					fprintf(EV->file,"%u.%u: %s (%u/%u/%u %u:%u)\n"
						,SonorkApp.ProfileUserId().v[0]
						,SonorkApp.ProfileUserId().v[1]
						,tmp1
						,EV->st.wYear
						,EV->st.wMonth
						,EV->st.wDay
						,EV->st.wHour
						,EV->st.wMinute);
					TSonorkConsole::SepLine(EV->file,8);
				}
				break;
			case SONORK_CONSOLE_EXPORT_SECTION_COMMENTS:
				if(*EV->data.comments)
				{
					fprintf(EV->file,"%s\n",EV->data.comments);
				}
				fprintf(EV->file,"\n");
				TSonorkConsole::SepLine(EV->file,8);
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:
				fputs(line_str , EV->file);
				fputc('\n',EV->file);
				TSonorkConsole::SepLine(EV->file,2);
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_END:
				fprintf(EV->file,"Sonork Export\n");
			break;
		}
	}

	return 0L;
}

// ----------------------------------------------------------------------------
// TSonorkTrackerInviteData
// ----------------------------------------------------------------------------

void
 TSonorkTrackerInviteData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	user_data.Clear();
	text.Clear();
}
void
 TSonorkTrackerInviteData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&user_data);
	CODEC.Write(&text);
}
void
 TSonorkTrackerInviteData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) );
	CODEC.Skip((aux&0xfff)-sizeof(header));
	CODEC.Read(&user_data);
	CODEC.Read(&text);
}
DWORD
 TSonorkTrackerInviteData::CODEC_DataSize() const
{
	return sizeof(DWORD)
		+ sizeof(header)
		+ user_data.CODEC_Size()
		+ text.CODEC_Size();
}


// ----------------------------------------------------------------------------
// TSonorkTrackerTextData
// ----------------------------------------------------------------------------

void
 TSonorkTrackerTextData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	text.Clear();
	data.Clear();
}

void
 TSonorkTrackerTextData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&text);
	CODEC.Write(&data);
}

void
 TSonorkTrackerTextData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	aux&=0xfff;
	if( aux < sizeof(header))
	{
		CODEC.ReadDWN((DWORD*)&header,SIZE_IN_DWORDS(aux));
	}
	else
	{
		CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
		CODEC.Skip( aux - sizeof(header));
	}

	CODEC.Read(&text);
	CODEC.Read(&data);
}

DWORD
 TSonorkTrackerTextData::CODEC_DataSize()	const
{
	return sizeof(DWORD)
		+ sizeof(header)
		+ ::CODEC_Size(&text)
		+ ::CODEC_Size(&data);
}

// ----------------------------------------------------------------------------
// Global handlers
// ----------------------------------------------------------------------------

void
  TSonorkTrackerWin::Init(const TSonorkUserData*UD, SONORK_C_CSTR text)
{
	TSonorkTrackerWin*W;
	if( UD == NULL )
		return;
	W=new TSonorkTrackerWin;
	if(!W->Create())
	{
		delete W;
		return;
	}
	W->Call( UD , text);
}


// ----------------------------------------------------------------------------
// TSonorkTrackerWin::LocatorServiceCallback
// Service callback for the SONORK_TRACKER locator:
//  Needed to process invitations
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
  TSonorkTrackerWin::LocatorServiceCallback(
			 SONORK_DWORD2&			//handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			event_tag
			,TSonorkAppServiceEvent*	E)
{

	union {
		DWORD					value;
		TSonorkTrackerPendingInvitation*	pending_req;
	}D;

	switch(event_id)
	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:
		E->get_name.str->Set("Weasel locator");
		return 1;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ:
	if( E->circuit_req.UserParam() == TRK_CIRCUIT_REQ_PARAM_INVITE )
	{
		while( !SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_INVITATIONS) )
		{
			if( SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_PUBLIC_INVITATIONS) )
			{
				if(SonorkApp.UserList().Get( E->circuit_req.SenderUserId() ) == NULL )
					break;
			}

			SONORK_MEM_NEW(D.pending_req = new TSonorkTrackerPendingInvitation);
			if( D.pending_req->invite_data.CODEC_ReadMemNoSize(
				  E->circuit_req.Data()
				, E->circuit_req.DataSize()) == SONORK_RESULT_OK)
			{
				D.pending_req->wait_win=NULL;
				D.pending_req->processed=false;
				D.pending_req->service_instance=0;
				D.pending_req->invite_data.user_data.userId.Set(
					E->circuit_req.SenderUserId()
					);
				E->circuit_req.GetCircuitHandle(
					D.pending_req->circuit);
				if(Sonork_StartWaitWin(
					  TSonorkTrackerWin::UserResponseWaitCallback
					, 0
					, D.pending_req))
				{
					// The TRK_CIRCUIT_MARKER value
					// is used just for safety(bug checking)
					E->circuit_req.accept_instance=D.pending_req->service_instance;
					event_tag->v[0]=(DWORD)D.pending_req;
					event_tag->v[1]=TRK_CIRCUIT_MARKER;
					return SONORK_APP_SERVICE_CIRCUIT_ACCEPTED;
				}
			}
			SONORK_MEM_DELETE( D.pending_req );
			break;
		}
	}
	return SONORK_APP_SERVICE_CIRCUIT_NOT_ACCEPTED;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER
		 || event_tag->v[0] == 0)
			break;

		// Load the D.pending_req pointer
		D.value=event_tag->v[0];

		// Update the remote's circuit
		D.pending_req->circuit.Set( *E->circuit_open.handle );
	break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_UPDATE:

		if( event_tag->v[1] != TRK_CIRCUIT_MARKER
		 || event_tag->v[0] == 0)
			break;

		// Load the D.pending_req pointer
		D.value=event_tag->v[0];
		E->circuit_update.GetCircuitHandle(D.pending_req->circuit);

	break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED:
		if( event_tag->v[1] != TRK_CIRCUIT_MARKER
		 || event_tag->v[0] == 0)
			break;

		// Load the D.pending_req pointer
		D.value=event_tag->v[0];
		if( D.pending_req->wait_win == NULL )
		{
			// The owner window has been already destroyed
			// we don't process any more this circuit
			break;
		}
		D.pending_req->circuit.systemId=0;
		if( D.pending_req->processed )
		{
			// The owner window has been already destroyed
			// we don't process any more this circuit
			break;
		}
		D.pending_req->wait_win->CancelWait(
			  SonorkApp.SysString(
				E->circuit_close.result==SONORK_RESULT_TIMEOUT
				  ?GSS_REQTIMEOUT
				  :GSS_USRCANCEL)
			, GLS_OP_STARTAPP
			, 5);
	break;


	}
	return 0;
}

// ----------------------------------------------------------------------------
// TSonorkTrackerWin::UserResponseWaitCallback
// Waits for the user to accept or deny an invitation.
// It is passed as the callback function to a TSonorkWaitWin
// ----------------------------------------------------------------------------

void SONORK_CALLBACK
 TSonorkTrackerWin::UserResponseWaitCallback(TSonorkWaitWin*WW
		, LPARAM //cb_param
		, LPVOID cb_data
		, SONORK_WAIT_WIN_EVENT event
		, LPARAM event_param)
{
#define pending_req		((TSonorkTrackerPendingInvitation*)cb_data)
#define CIRCUIT_ID		pending_req->circuit.system_id
	TSonorkTrackerWin*	weasel_win;
	char*                   tmp;
	switch( event )
	{
		case SONORK_WAIT_WIN_EVENT_CREATE:
			pending_req->wait_win=WW;
			SONORK_MEM_NEW( tmp = new char[pending_req->invite_data.text.CODEC_Size() + 128] );

			sprintf(tmp
			  , "%s: %s\r\n"
			  , SonorkApp.LangString(GLS_LB_NOTES)
			  , pending_req->invite_data.text.ToCStr()
			  );
			WW->SetCtrlText( IDC_WAIT_INFO , tmp);
			SonorkApp.LangSprintf(tmp
				, GLS_TR_INVMSG
				, pending_req->invite_data.user_data.alias.CStr()
				, pending_req->invite_data.user_data.userId.v[0]
				, pending_req->invite_data.user_data.userId.v[1]);
			WW->SetCtrlText( IDC_WAIT_LABEL , tmp );
			WW->SetWindowText( GLS_OP_STARTAPP);
			WW->SetCaptionIcon( SKIN_HICON_TRACKER );
			WW->SetNotice(	SONORK_SEQUENCE_RADAR
				, 	SONORK_APP_SOUND_TRACKER
				, 	SONORK_APP_SOUND_TRACKER);
			SONORK_MEM_DELETE_ARRAY( tmp );

			*((DWORD*)event_param)=
				   SONORK_SECS_FOR_USER_RESPONSE
				  |SONORK_WAIT_WIN_F_NO_CHECKBOX
				  |SONORK_WAIT_WIN_F_IS_NOTICE;
			pending_req->service_instance=
				TSonorkTrackerWin::Service_Register(
					LocatorServiceCallback
					,NULL);
			

			break;

		case SONORK_WAIT_WIN_EVENT_DESTROY:
			SonorkApp.SetBichoSequenceIf(SONORK_SEQUENCE_RADAR
				, SONORK_SEQUENCE_IDLE);

			pending_req->wait_win=NULL;
			if( pending_req->service_instance != 0 )
			{
				SonorkApp.Service_CloseCircuit(
				  pending_req->service_instance
				, pending_req->circuit.CircuitId()
				, SONORK_RESULT_NOT_ACCEPTED);
				// This will also destroy the circuit
				SonorkApp.Service_Unregister(
					pending_req->service_instance , false);
			}
			SONORK_MEM_DELETE( pending_req );
			break;


		case SONORK_WAIT_WIN_EVENT_RESULT:
			pending_req->processed=true;
			if( event_param == SONORK_WAIT_WIN_F_RESULT_ACCEPT )
			{
				weasel_win=new TSonorkTrackerWin();
				if(!weasel_win->Create())
				{
					delete weasel_win;
					return;
				}
				if(weasel_win->Accept(pending_req))
				{
					// Set <service_instance> to 0 so that
					// EVENT_DESTROY does not destroy the
					// instance (and circuit) which are
					// now owned by <weasel_win>
					pending_req->service_instance=0;
				}
				else
					weasel_win->Destroy();

			}
			// else
			//  the EVENT_DESTROY will close the circuit
		break;

	}

#undef 	pending_req
#undef 	CIRCUIT_ID
}

