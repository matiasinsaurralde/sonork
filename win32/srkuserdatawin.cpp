#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop

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

#include "srkuserdatawin.h"
#include "srkdialogwin.h"
#include "srkchatwin.h"
#include "srk_file_io.h"
#include "srk_zip.h"

//extern HBITMAP Sonork_LoadBitmap(HDC tDC, const char*file_name);

#define	IDC_AUTH_NEMAIL			IDC_UINFO_AUTH1
#define	IDC_AUTH_NADDR			IDC_UINFO_AUTH2
#define	IDC_AUTH_FRIENDLY		IDC_UINFO_AUTH3
#define	IDC_AUTH_NFRIENDLY		IDC_UINFO_AUTH4
#define	IDC_AUTH_BUSY			IDC_UINFO_AUTH5
#define	IDC_AUTH_NBUSY			IDC_UINFO_AUTH6
#define	IDC_AUTH_NCX			IDC_UINFO_AUTH7
#define IDC_AUTH_UTS_ENABLED		IDC_UINFO_AUTH8

#define UWF_CX_ONLY_ALERT		SONORK_WIN_F_USER_01
#define UWF_SERVICES_QUERIED		SONORK_WIN_F_USER_02
#define UWF_PIC_LOADED			SONORK_WIN_F_USER_03
#define POKE_REFRESH			SONORK_WIN_POKE_01
#define POKE_PROCESS_PIC		SONORK_WIN_POKE_02
#define POKE_CONNECT_SERVICE		SONORK_WIN_POKE_03

#define UPDATE_F_ALL	0xffff
#define UPDATE_F_AUTHS	0x0001
#define UPDATE_F_LNOTES	0x0002
#define UPDATE_F_RNOTES	0x0004
#define UPDATE_F_INFO	0x0008
#define UPDATE_F_NOTES	(UPDATE_F_LNOTES|UPDATE_F_RNOTES)

static struct
{
	SONORK_AUTH_FLAG0	auth_flag;
	UINT			id;
}visibility_xlat[SONORK_APP_VISIBILITY_GROUPS]=
{{SONORK_AUTH_F0_PRIVATE_1,IDC_UINFO_PRIV1}
,{SONORK_AUTH_F0_PRIVATE_2,IDC_UINFO_PRIV2}
,{SONORK_AUTH_F0_PRIVATE_3,IDC_UINFO_PRIV3}
,{SONORK_AUTH_F0_PRIVATE_4,IDC_UINFO_PRIV4}
,{SONORK_AUTH_F0_PRIVATE_5,IDC_UINFO_PRIV5}
,{SONORK_AUTH_F0_PRIVATE_6,IDC_UINFO_PRIV6}
,{SONORK_AUTH_F0_PRIVATE_7,IDC_UINFO_PRIV7}
};



// ----------------------------------------------------------------------------

void
 TSonorkUserDataWin::OnPoke_ConnectService()
{
	TSonorkUserServer *userver;
	userver = (TSonorkUserServer *)service_list.GetSelectedItem();
	if(!userver)return;
	if(userver->ServiceId() != SONORK_SERVICE_ID_SONORK_CHAT)return;
	TSonorkChatWin::CreateClient( userver );	
}

// ----------------------------------------------------------------------------

void
 TSonorkUserDataWin::StartRemoteQuery(QUERY_TYPE qt)
{
	TSonorkServiceQuery		handle;
	SONORK_CTRLMSG_CMD		cmd;
	SONORK_RESULT			result;

	if( !(
	     SonorkApp.CxReady()
	  && ctx_user->IsOnline()
	  && ctx_user->addr.version.VersionNumber() >= MAKE_VERSION_NUMBER(1,5,0,7)
	  && service.query_type == QUERY_NONE) ) 
		return;

	if( service.instance == 0 )
	{
		SONORK_DWORD2			sii_tag;
		TSonorkServiceInstanceInfo	sii;
		sii.SetInstanceInfo(
				  SONORK_SERVICE_ID_SONORK
				, SONORK_SERVICE_TYPE_NONE
				, 0	// hflags
				, 0	// instance: Don't know yet, set to 0
				, SONORK_APP_VERSION_NUMBER);

		sii_tag.v[0]=(DWORD)this;
		sii_tag.v[1]=0;
		if(SonorkApp.Service_Register(
			  SONORK_APP_SERVICE_TYPE_NONE
			, &sii
			, SKIN_ICON_SONORK
			, ServiceCallback
			, &sii_tag)!=SONORK_RESULT_OK)
			return;
		service.instance=sii.ServiceInstance();

	}

	ctx_user->GetUserHandle(&handle);
	handle.LoadTarget(SONORK_SERVICE_ID_SONORK
		, SONORK_SERVICE_LOCATOR_INSTANCE(0) );
	if( qt == QUERY_PIC )
		cmd = SONORK_APP_CTRLMSG_QUERY_PIC;
	else
		cmd = SONORK_APP_CTRLMSG_QUERY_SERVICES;
	result = SonorkApp.Service_StartQuery(
		  service.instance
		, &handle
		, cmd
		, 0
		, 0	// usr_flags
		, SONORK_MSECS_FOR_QUERY_REPLY
		, NULL	// data
		, 0	// data_size
		, NULL	// query_tag
		);
	if( result == SONORK_RESULT_OK )
	{
		service.query_id = handle.QueryId();
		service.query_type = qt;

		ClearWinUsrFlag(UWF_CX_ONLY_ALERT);
		TSonorkWin::SetStatus_PleaseWait(status_bar);
		UpdateQueryButtons();

		if( qt == QUERY_PIC )
		{
			::InvalidateRect(pic.hwnd,NULL,true);
		}
		else
		{
			SetWinUsrFlag(UWF_SERVICES_QUERIED);
			service_list.DelAllItems();
		}
	}



}
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkUserDataWin::ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		//event_tag
			, TSonorkAppServiceEvent*	E)
{
	TSonorkUserDataWin*_this = (TSonorkUserDataWin*)handler_tag.v[0];
	union {
		SONORK_FILE_HANDLE	handle;
		TSonorkUserServer*	userver;
	}D;
	union {
		bool		flag;
		SKIN_ICON       icon;
	}B;
	union {
		char*			tmp;
		const char*		str;
	}P;
	int		index;
	SONORK_C_CSTR	type_str;
	switch( event_id )
	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:
		E->get_name.str->Set(_this->ctx_user->alias.CStr());
		return 1;

	case SONORK_APP_SERVICE_EVENT_QUERY_AKN:
		if( _this->service.query_type == QUERY_PIC )
		{
			if( E->query_akn.Cmd() != SONORK_APP_CTRLMSG_QUERY_PIC )
				break;
			if( (E->query_akn.CmdFlags()&SONORK_CTRLMSG_CF_ACCEPT) )
			{
				TSonorkWin::SetStatus(
					_this->status_bar
					, GLS_MS_RCVING
					, SKIN_HICON_FILE_DOWNLOAD);

				break;
			}
			if( !(E->query_akn.CmdFlags()&SONORK_CTRLMSG_CF_DATA) )
				break;

			if( E->query_akn.DataSize() < 16 )
				break;

			B.flag=false;
			P.tmp = new char[SONORK_MAX_PATH];
			SonorkApp.GetTempPath(P.tmp,"zpic");
			D.handle =  SONORK_IO_OpenFile(
					P.tmp
				, SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS);
			if( D.handle != SONORK_INVALID_FILE_HANDLE )
			{
				if(SONORK_IO_WriteFile(D.handle
					,E->query_akn.Data()
					,E->query_akn.DataSize())==0)
				{
					B.flag=true;
				}
				SONORK_IO_CloseFile( D.handle );

			}
			if( B.flag )
			{
				_this->PostPoke(POKE_PROCESS_PIC,(LPARAM)P.tmp);
				break;
			}
			delete[] P.tmp;
		}
		else
		if( _this->service.query_type == QUERY_SERVICES )
		{
			if( E->query_akn.Cmd() != SONORK_APP_CTRLMSG_QUERY_SERVICES )
				break;
			if( !(E->query_akn.CmdFlags()&SONORK_CTRLMSG_CF_DATA ))
				break;
			SONORK_MEM_NEW( D.userver = new TSonorkUserServer );
			if( D.userver->CODEC_ReadMemNoSize(
				  E->query_akn.Data()
				, E->query_akn.DataSize()) == SONORK_RESULT_OK)
			{
				P.str=SonorkApp.Service_GetDescription(
					  D.userver->ServiceInstanceInfo()
					, &type_str
					, &B.icon);
				index=_this->service_list.AddItem(
					  P.str
					, B.icon
					, (LPARAM)D.userver);
				_this->service_list.SetItemText(
					  index
					, 1
					, type_str);
				_this->service_list.SetItemText(
					  index
					, 2
					, D.userver->name.ToCStr());
				_this->service_list.SetItemText(
					  index
					, 3
					, D.userver->text.ToCStr());
				break;
			}
			SONORK_MEM_DELETE( D.userver );
		}
		break;

	case SONORK_APP_SERVICE_EVENT_QUERY_END:
		if( _this->service.query_id == E->query_end.QueryId() )
		{
			if( _this->service.query_type == QUERY_PIC)
				::InvalidateRect(_this->pic.hwnd,NULL,true);

			_this->service.query_id=0;
			_this->service.query_type=QUERY_NONE;
			_this->UpdateQueryButtons();
			TSonorkWin::SetStatus_None(_this->status_bar);
		}
		break;
	}
	return 0;
}


// ----------------------------------------------------------------------------
// CONSTRUCTOR / CREATION / DESTRUCTION
// ----------------------------------------------------------------------------

TSonorkUserDataWin::TSonorkUserDataWin(
		   TSonorkWin*			parent	// NOT null
		,  TSonorkExtUserData*		pUD	// NOT null
		,  const TSonorkDynString*	pnotes	// may be null
		,  TAB 				ptab	// See enum TAB
		,  DWORD 			pflags	// See enum SHOW_FLAGS
		)
	:TSonorkTaskWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_USER_DATA
	|SONORK_WIN_DIALOG
	|IDD_USERDATA
	,0)
{
	tab_ctrl.page=ptab;
	assert( pUD != NULL );
	show_flags=pflags;
	SetEventMask(SONORK_APP_EM_CX_STATUS_AWARE|SONORK_APP_EM_USER_LIST_AWARE);
	if(!pnotes)
	{
		show_flags&=~SF_SAVE_NEW_USER_NOTES;
		ctx_remote_notes.Clear();
	}
	else
	{
		ctx_remote_notes.Set(*pnotes);

	}
	ctx_user = pUD ;
	if( show_flags & (SF_SAVE_NEW_USER_NOTES|SF_SAVE_NEW_USER_DATA) )
	{
		SaveData(
			  show_flags&SF_SAVE_NEW_USER_DATA
			, show_flags&SF_SAVE_NEW_USER_NOTES);
	}
	service.instance=service.query_id=0;
        service.query_type=QUERY_NONE;
}
/*
TSonorkUserDataWin::TSonorkUserDataWin(TSonorkWin*parent
		,const TSonorkUserDataNotes*UDN
		,TAB tab)
	:TSonorkTaskWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_USER_DATA
	|SONORK_WIN_DIALOG
	|IDD_USERDATA
	,GAWD_CX_STATUS_AWARE
	|GAWD_USER_LIST_AWARE)
{
	ctx_remote_notes.Set( UDN->notes );
	ctx_user = SonorkApp.UserList().Get( UDN->user_data.userId );
	if( ctx_user != NULL )
	{
		SetWinUsrFlag( USER_IN_LOCAL_LIST );

		// Selectively copy the data we don't have or
		// the updated fields: Don't just copy all because
		// we probably have better information in our
		// local list than what the public gets.


		ctx_user->alias.Set( UDN->user_data.alias );
		ctx_user->name.Set( UDN->user_data.name );

		// Check if email available (some users don't make
		// this visible to the public, but DO make them visible
		// to users in their lists ).
		if( UDN->user_data.email.Length() )
			ctx_user->email.Set( UDN->user_data.email );
		ctx_user->CopyUserInfo( &UDN->user_data );

	}
	else
	{
		SONORK_MEM_NEW( ctx_user=new TSonorkExtUserData(SONORK_USER_TYPE_UNKNOWN) );
		ctx_user->ClearExtData();
		ctx_user->Set( UDN->user_data );
	}
	SetWinUsrFlag( UPDATED_DATA_PROVIDED );


	tab_ctrl.page=tab;
}
*/


TSonorkUserDataWin::~TSonorkUserDataWin()
{
	if( (show_flags & SF_DELETE_USER_DATA)
	&&   ctx_user != NULL)
	{
		TRACE_DEBUG("UserDataWin:: DELETE %x",ctx_user);
		SONORK_MEM_DELETE(ctx_user);
	}
}

// ----------------------------------------------------------------------------
// 		COMMANDS
// ----------------------------------------------------------------------------
bool
 TSonorkUserDataWin::CmdStore()
{
	UINT			set_flags=0;
	int			idx;
	HWND			hwnd;
	char			tmp[64];
	TSonorkDynString	local_notes;


	if( !(show_flags&SF_USER_IN_LOCAL_LIST) )
	{
		// Can't change any attributes for users not in our list
		return true;
	}

	if( page_win[TAB_INFO]->GetEditCtrlModified(IDC_UINFO_DALIAS) )
	{
		set_flags|=SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS;
		page_win[TAB_INFO]->GetCtrlText( IDC_UINFO_DALIAS, ctx_user->display_alias);
		if(ctx_user->display_alias.Length() < 2)
		{
			ctx_user->display_alias.Set( ctx_user->alias );
			page_win[TAB_INFO]->SetCtrlText( IDC_UINFO_DALIAS, ctx_user->alias.CStr() );
		}
	}


	if( page_win[TAB_NOTES]->GetEditCtrlModified(IDC_UINFO_LNOTES ))
	{
		set_flags|=SONORK_APP_EVENT_SET_USER_F_L_NOTES;
		page_win[TAB_NOTES]->GetCtrlText( IDC_UINFO_LNOTES, local_notes);
		SonorkApp.SaveUserNotes( ctx_user->userId
			, &local_notes
			, TSonorkExtUserData::LOCAL_NOTES);
	}

	if( show_flags&SF_USER_IN_AUTHORIZED_LIST )
	{
		// These attributes only apply to authorized users

		hwnd = page_win[TAB_ALERT]->GetDlgItem(IDC_UINFO_ALERT_MSGSOUND);
		idx = ComboBox_GetCurSel(hwnd);
		if(idx<=1)
		{
			ctx_user->sound.message.Clear();
			ctx_user->SetClearCtrlFlag(SONORK_UCF_MUT_MSG_SOUND,idx==1);
		}
		else
		{
			ComboBox_GetString(hwnd,idx,tmp);
			ctx_user->sound.message.Set(tmp);
		}

		hwnd = page_win[TAB_ALERT]->GetDlgItem(IDC_UINFO_ALERT_ONLSOUND);
		idx = (int)ComboBox_GetCurSel(hwnd);
		if(idx<=1)
		{
			ctx_user->sound.online.Clear();
			ctx_user->SetClearCtrlFlag(SONORK_UCF_MUT_ONL_SOUND,idx==1);
		}
		else
		{
			ComboBox_GetString(hwnd,idx,tmp);
			ctx_user->sound.online.Set(tmp);
		}
		ctx_user->SetClearCtrlFlag(SONORK_UCF_SONORK_UTS_DISABLED
				,!page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_UTS_ENABLED));

		ctx_user->SetClearCtrlFlag(SONORK_APP_UCF_NO_SLIDER_CX_DISPLAY
				,!page_win[TAB_ALERT]->GetCtrlChecked(IDC_UINFO_ALERT_SLIDE));

		ctx_user->SetClearCtrlFlag(SONORK_APP_UCF_NO_STATUS_CX_DISPLAY
				,!page_win[TAB_ALERT]->GetCtrlChecked(IDC_UINFO_ALERT_STATUS));

	}
	page_win[TAB_NOTES]->SetEditCtrlModified(IDC_UINFO_LNOTES , false);
	page_win[TAB_INFO]->SetEditCtrlModified(IDC_UINFO_DALIAS,false);

	SaveData( true , false );

	if(set_flags)
		SonorkApp.BroadcastAppEvent_SetUser( ctx_user , set_flags);

	// Only save authorization flags if connected
	// and user is in our user list.
	if( SonorkApp.CxReady() && (show_flags&SF_USER_IN_AUTHORIZED_LIST) )
	{
		TSonorkAuth2	auth;
		auth.Set(ctx_user->ctrl_data.auth);

		auth.SetClearFlag(SONORK_AUTH_F1_HIDE_EMAIL,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_NEMAIL));
		auth.SetClearFlag(SONORK_AUTH_F1_HIDE_ADDR,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_NADDR));
		auth.SetClearFlag(SONORK_AUTH_F1_BUSY,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_BUSY));
		auth.SetClearFlag(SONORK_AUTH_F1_NOT_BUSY,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_NBUSY));
		auth.SetClearFlag(SONORK_AUTH_F1_FRIENDLY,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_FRIENDLY));
		auth.SetClearFlag(SONORK_AUTH_F1_NOT_FRIENDLY,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_NFRIENDLY));
		auth.SetClearFlag(SONORK_AUTH_F1_DISCONNECTED,page_win[TAB_AUTH]->GetCtrlChecked(IDC_AUTH_NCX));


		for( idx=0;idx<SONORK_APP_VISIBILITY_GROUPS;idx++)
			auth.SetClearFlag(visibility_xlat[idx].auth_flag,page_win[TAB_AUTH]->GetCtrlChecked(visibility_xlat[idx].id));

		// Only save authorization flags if they've changed
		if( !(auth.flags == ctx_user->ctrl_data.auth.flags))
		{
			UINT			P_size;
			TSonorkDataPacket	*P;
			SONORK_DWORD2	tag;

	#define A_size 128

			tag.v[0]=SONORK_FUNCTION_SET_AUTH;
			P = SONORK_AllocDataPacket( A_size );
			P_size = P->E_SetAuth_R(A_size,ctx_user->userId,auth);
			StartSonorkTask(taskERR
				,P
				,P_size
				,0
				,GLS_TK_PUTINFO
				,&tag);
			SONORK_FreeDataPacket( P );
	#undef A_size
			return false;
		}
	}
        return true;
}
void
 TSonorkUserDataWin::CmdRefresh()
{
	UINT			P_size;
	TSonorkDataPacket	*P;
	SONORK_DWORD2		tag;

	if(tab_ctrl.page == TAB_ONL )
	{
		StartRemoteQuery(QUERY_SERVICES);
	}
	else
	{
#define A_size 	128
		P = SONORK_AllocDataPacket( A_size );
		P_size = P->E_GetUserData_R(A_size
				, ctx_user->userId
				, SONORK_USER_INFO_LEVEL_1
				, ( show_flags&SF_USER_IN_AUTHORIZED_LIST )
					?0
					:SONORK_GETUSERDATA_F_USE_PUBLIC
				, ctx_user->ctrl_data.auth.pin
				, 0);
		tag.v[0]=SONORK_FUNCTION_GET_USER_DATA;
		StartSonorkTask(taskERR
			,P
			,P_size
			,SONORK_TASKWIN_DEFAULT_FLAGS
			,GLS_TK_GETINFO
			,&tag);
		SONORK_FreeDataPacket( P );
#undef A_size
	}

}


// ==================================================
// 		GU TASK HANDLERS
// ==================================================
void
 TSonorkUserDataWin::OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2& tag)
{
	GLS_INDEX gls_index;

	gls_index =
		tag.v[0] == SONORK_FUNCTION_GET_USER_DATA
			?GLS_MS_RFRESHING
			:GLS_MS_STORING;

	ClearWinUsrFlag(UWF_CX_ONLY_ALERT);
	TSonorkWin::SetStatus(status_bar,gls_index,SKIN_HICON_BUSY);
	UpdateQueryButtons();

}

void
 TSonorkUserDataWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	UINT set_user_flags=0;
	TSonorkUserDataNotes ND;
	if( P->SubFunction() != 0)return;
	if( P->Function() == SONORK_FUNCTION_GET_USER_DATA )
	{
		if( P->D_GetUserData_A(P_size,ND) )
		if( ND.user_data.GetUserInfoLevel() >= SONORK_USER_INFO_LEVEL_1 )
		if( ctx_user->userId == ND.user_data.userId )
		{
			ctx_user->TSonorkUserData::Set( ND.user_data );
			ctx_remote_notes.Set(ND.notes.ToCStr());
			if( show_flags & SF_USER_IN_LOCAL_LIST )
			{
				// All attributes changed except display alias and local notes
				set_user_flags = (SONORK_APP_EVENT_SET_USER_F_ALL)&
						~(SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS
						  |SONORK_APP_EVENT_SET_USER_F_L_NOTES);
			}
		}
	}
	else
	if( P->Function() == SONORK_FUNCTION_SET_AUTH )
	{
		if(P->D_SetAuth_A(P_size,ND.user_data.userId,ND.auth))
		if(ND.user_data.userId == ctx_user->userId)
		{
			ctx_user->ctrl_data.auth.Set( ND.auth );
			// When we store info, we can only modify the AUTH
			// all other attributes are controlled by the owner
			// We generated the event for SET_F_DISP_ALIAS and SET_F_L_NOTES
			// just before we start the Gu Task, in  CmdStore()
			set_user_flags = SONORK_APP_EVENT_SET_USER_F_AUTH_FLAGS;
		}
	}
	else
		return;
	if(set_user_flags != 0)
	{
		SaveData( true , set_user_flags&SONORK_APP_EVENT_SET_USER_F_R_NOTES );
		SonorkApp.BroadcastAppEvent_SetUser( ctx_user ,  set_user_flags);
	}

}

void
 TSonorkUserDataWin::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2& tag, const TSonorkError*pERR)
{

	// We post ourselves a message: Can't diplay modal dialogs
	// 	from within a GuTask handler... we'd halt the whole GU engine
	taskERR.Set( *pERR );
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, tag.v[0]);
	UpdateQueryButtons();
}


// ==================================================
// 		FORM UPDATE
// ==================================================

void
 TSonorkUserDataWin::SaveData( BOOL user_info, BOOL remote_notes )
{
	// Can't save any data for users not in our local list
	if( !(show_flags & SF_USER_IN_LOCAL_LIST ) )
		return;

	if( user_info )
		SonorkApp.SaveUser( ctx_user->userId );

	if( remote_notes )
		SonorkApp.SaveUserNotes( ctx_user->userId
			, &ctx_remote_notes
			, TSonorkExtUserData::REMOTE_NOTES);
}

void
 TSonorkUserDataWin::UI_LoadInfo()
{
	char 		tmp[80];
	char		szNA[64];
	int    		aux;
	GLS_INDEX	gls;

	const TSonorkLangCodeRecord	*REC;
	if( ctx_user->GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_1)
		return;
	wsprintf(szNA,"(%s)",SonorkApp.LangString(GLS_LB_NA));

	ctx_user->Serial().GetStr(tmp);
	page_win[TAB_ONL]->SetCtrlText(IDL_USERONL_SERIAL,tmp);
	ctx_user->addr.physAddr.GetStr(tmp);
	page_win[TAB_ONL]->SetCtrlText(IDC_USERONL_ADDR,*tmp?tmp:szNA);
	page_win[TAB_ONL]->SetCtrlText(IDC_USERONL_VERSION
		,ctx_user->SidVersion().GetStr(tmp));


	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_USRID,uid_str);
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_ALIAS,ctx_user->alias.CStr());
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_NAME,ctx_user->name.CStr());
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_EMAIL,*ctx_user->email.CStr()?ctx_user->email.CStr():szNA);

	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_DALIAS,ctx_user->display_alias.CStr());
	page_win[TAB_INFO]->SetEditCtrlModified( IDC_UINFO_DALIAS , false );

	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_TZ
		,ctx_user->Region().GetTimeZoneStr(tmp));
	TSonorkWin32App::MakeTimeStr( ctx_user->BornTime() ,tmp, MKTIMESTR_DATE);
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_BDATE,tmp);

	REC = SonorkApp.CountryCodeTable().GetRecordByCode(ctx_user->Region().GetCountry());
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_COUNTRY,REC?REC->name:szNA);

	REC = SonorkApp.LanguageCodeTable().GetRecordByCode(ctx_user->Region().GetLanguage());
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_LANGUAGE,REC?REC->name:szNA);

	aux=ctx_user->InfoFlags().GetSex();
	if( aux == SONORK_SEX_M)
		gls=GLS_LB_SEXM;
	else
	if( aux == SONORK_SEX_F)
		gls=GLS_LB_SEXF;
	else
		gls=GLS_LB_SEXN;
	page_win[TAB_INFO]->SetCtrlText(IDC_UINFO_SEX,gls);
}
void
 TSonorkUserDataWin::UI_LoadAuths()
{
	TSonorkAuth2& A=ctx_user->ctrl_data.auth;
	int idx;
	if( !(show_flags & SF_USER_IN_AUTHORIZED_LIST) )
	{
		// No authorizations for the users
		return;
	}
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_NEMAIL	,A.HideEmail());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_NADDR	,A.HideAddr());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_FRIENDLY	,A.ShowFriendly());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_NFRIENDLY	,A.ShowNotFriendly());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_BUSY	,A.ShowBusy());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_NBUSY	,A.ShowNotBusy());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_NCX		,A.ShowDisconnected());
	page_win[TAB_AUTH]->SetCtrlChecked(IDC_AUTH_UTS_ENABLED	,!ctx_user->TestCtrlFlag(SONORK_UCF_SONORK_UTS_DISABLED));

	for( idx=0;idx<SONORK_APP_VISIBILITY_GROUPS;idx++)
		page_win[TAB_AUTH]->SetCtrlChecked(visibility_xlat[idx].id,A.TestFlag(visibility_xlat[idx].auth_flag));

}
void
 TSonorkUserDataWin::UI_LoadLocalNotes()
{
	TSonorkDynString	notes;

	if( show_flags & SF_USER_IN_LOCAL_LIST )
	{
		SonorkApp.LoadUserNotes(ctx_user->userId
			,&notes
			, TSonorkExtUserData::LOCAL_NOTES);
	}
	else
	{
		// Can't load anything for users not in our list
		notes.Clear();
	}
	page_win[TAB_NOTES]->SetCtrlText(IDC_UINFO_LNOTES,notes.ToCStr());
	page_win[TAB_NOTES]->SetEditCtrlModified( IDC_UINFO_LNOTES , false );
}
void
 TSonorkUserDataWin::UI_LoadRemoteNotes(BOOL from_file)
{
	if( (show_flags & SF_USER_IN_LOCAL_LIST) && from_file)
	{
		SonorkApp.LoadUserNotes(ctx_user->userId
			, &ctx_remote_notes
			, TSonorkExtUserData::REMOTE_NOTES);
	}
	page_win[TAB_NOTES]->SetCtrlText(IDC_UINFO_RINFO
		,ctx_remote_notes.ToCStr());
}




bool
 TSonorkUserDataWin::OnCreate()
{
	union {
		TC_ITEM 	tc_item;
		const TSonorkLangCodeRecord	*REC;
		HDC		dc;
		char		tmp[128];
	}D;
	int i;
	UINT	height;
	RECT	rect;
	SIZE	size;
	static GLS_INDEX tab_gls[TABS]=
	{
		GLS_LB_INFO
	,	GLS_LB_NOTES
	,	GLS_LB_NETINFO
	,	GLS_LB_AUTHVIS
	,	GLS_LB_ALERTS
	};
	static UINT tab_dlg_id[TABS]=
	{
		IDD_USERINFO
	,	IDD_USERNOTES
	,	IDD_USERONL
	,	IDD_USERAUTH
	,	IDD_USERALERT
	};

	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDOK		   	,	GLS_OP_STORE	}
	,	{IDC_UINFO_REFRESH	,	GLS_OP_REFRESH	}
	,	{0		   	,	GLS_NULL	}
	};

	static TSonorkWinGlsEntry gls_info[]=
	{	{IDL_UINFO_USRID
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_SRKID	}
	,	 {IDL_UINFO_ALIAS
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_ALIAS	}
	,	{IDL_UINFO_DALIAS
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_DALIAS	}
	,	{IDL_UINFO_NAME
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_NAME	}
	,	{IDL_UINFO_EMAIL
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_EMAIL	}
	,	{IDL_UINFO_TZ
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_TZ	}
	,	{IDL_UINFO_BDATE
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_BDATE	}
	,	{IDL_UINFO_COUNTRY
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_COUNTRY	}
	,	{IDL_UINFO_LANGUAGE
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_LANG	}
	,	{IDL_UINFO_SEX
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_SEX	}
	,	{IDG_USERINFO_CONTACT
					,	GLS_LB_PERSINFO	}
	,	{IDC_UINFO_PIC_REFRESH
					,	GLS_OP_REFRESH	}
	,	{0			,	GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_notes[]=
	{	{IDL_UINFO_RNOTES	,	GLS_LB_REMINFO	}
	,	{IDL_UINFO_LNOTES	,	GLS_LB_NOTES	}
	,	{IDL_UINFO_RNOTES_HELP	,	GLS_UI_RNHLP	}
	,	{IDL_UINFO_LNOTES_HELP	,	GLS_UI_LNHLP	}
	,	{0			,	GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_alrt[]=
	{
		{IDL_UINFO_ALERT_ONLSOUND ,	GLS_LB_CX   	}
	,	{IDL_UINFO_ALERT_MSGSOUND ,	GLS_LB_MSGS 	}
	,	{IDG_UINFO_ALERT_ALTSOUNDS,	GLS_LB_ALTSNDS	}
	,	{IDG_UINFO_ALERT_ALTSOUNDS,	GLS_LB_ALTSNDS	}
	,	{IDG_UINFO_ALERT_CX	,	GLS_UA_CXALRT	}
	,	{IDC_UINFO_ALERT_SLIDE	,	GLS_UA_SLIDE	}
	,	{IDC_UINFO_ALERT_STATUS ,	GLS_UA_INFO	}
	,	{IDC_UINFO_ALERT_POPUP	,	GLS_UA_POPUP	}
	,	{0			,	GLS_NULL    	}
	};
	static TSonorkWinGlsEntry gls_onl[]=
	{
		{IDL_USERONL_ADDR
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_NETADDR	}
	,	{IDL_USERONL_VERSION
		 |SONORK_WIN_CTF_BOLD	,	GLS_LB_VERSION	}
	,	{IDG_USERONL_NETINFO
					,	GLS_LB_NETINFO	}
	,	{0			,	GLS_NULL    	}
	};
	static TSonorkWinGlsEntry gls_auth[]=
	{	{IDC_UINFO_PRIV1	,	GLS_VG_01	}
	,	{IDC_UINFO_PRIV2	,	GLS_VG_02	}
	,	{IDC_UINFO_PRIV3	,	GLS_VG_03	}
	,	{IDC_UINFO_PRIV4	,	GLS_VG_04	}
	,	{IDC_UINFO_PRIV5	,	GLS_VG_05	}
	,	{IDC_UINFO_PRIV6	,	GLS_VG_06	}
	,	{IDC_UINFO_PRIV7	,	GLS_VG_07	}
	,	{IDC_AUTH_NEMAIL	,	GLS_UA_NEMAIL}
	,	{IDC_AUTH_NADDR		,	GLS_UA_NADDR	}
	,	{IDC_AUTH_NCX		,	GLS_UA_NCX	}
	,	{IDC_AUTH_UTS_ENABLED	, 	GLS_OP_AUTO_UTS	}
	,	{IDC_AUTH_FRIENDLY	,	GLS_UA_FRIENDLY	}
	,	{IDC_AUTH_NFRIENDLY	,	GLS_UA_NFRIENDLY	}
	,	{IDC_AUTH_BUSY		,	GLS_UA_BUSY	}
	,	{IDC_AUTH_NBUSY		,	GLS_UA_NBUSY	}
	,	{IDC_UINFO_GROUPS	,	GLS_UA_GRPS}
	,	{IDC_UINFO_AUTHS	,	GLS_UA_AUTHS	}
	,	{IDC_UINFO_AUTHS_HELP	,	GLS_UA_HLP	}
	,	{0			,	GLS_NULL	}
	};
	static TSonorkListViewColumn
		service_list_cols[]=
	{       {GLS_LB_APP	, 112}
	,	{GLS_LB_TYPE	, 48}
	,	{GLS_LB_NAME	, 96}
	,	{GLS_LB_COMMENTS, 256}
	};

	assert( ctx_user != NULL );

	ctx_user->userId.GetStr( uid_str );
	wsprintf(D.tmp,"%s: %s %s"
		, SonorkApp.LangString(GLS_LB_INFO)
		, uid_str
		, ctx_user->display_alias.CStr() );

	// Status bar
	{
		status_bar=GetDlgItem( IDC_USERDATA_STATUSBAR); //CreateStatusWindow(IDC_USER_INFO_STATUS,"",WS_VISIBLE|CCS_BOTTOM,&status_bar.height);
	}

// ----------------------------------------------------
// TAB CONTROL

#define MARGIN	2

	tab_ctrl.hwnd=GetDlgItem(IDC_UINFO_TAB);

	TabCtrl_GetItemRect(tab_ctrl.hwnd,0,&rect);
	height = (rect.bottom  - rect.top) + 1;
	::GetWindowRect(tab_ctrl.hwnd,&rect);
	ScreenToClient(&rect);

	size.cx=GetSystemMetrics(SM_CXBORDER) + MARGIN;
	size.cy=GetSystemMetrics(SM_CYBORDER) + MARGIN;
	rect.left 	+= size.cx;
	rect.top  	+= height + size.cy + MARGIN;
	rect.right	-= size.cx;
	rect.bottom	-= size.cy;


	D.tc_item.mask=TCIF_TEXT|TCIF_IMAGE;
	D.tc_item.lParam=0;
	D.tc_item.iImage=-1;
	for(i=0;i<TABS;i++)
	{
		if( i<=TAB_ONL || (show_flags & SF_USER_IN_AUTHORIZED_LIST) )
		{
			D.tc_item.pszText=(char*)SonorkApp.LangString(tab_gls[i]);
			TabCtrl_InsertItem(tab_ctrl.hwnd,i,&D.tc_item);
		}
		page_win[i] = new TSonorkChildDialogWin(this
				,tab_dlg_id[i]
				,SONORK_WIN_SF_NO_CLOSE);
		TSonorkWin::CenterWin(page_win[i],rect,SONORK_CENTER_WIN_F_CREATE);
	}

	pic.hwnd=page_win[TAB_INFO]->GetDlgItem(IDC_UINFO_PIC);
	::GetClientRect( pic.hwnd ,&rect);
	pic.bm_rect.left   = (rect.right - SONORK_USER_PIC_SW)/2-2;
	pic.bm_rect.top	   = (rect.bottom - SONORK_USER_PIC_SH)/2-2;
	pic.bm_rect.bottom = pic.bm_rect.top+SONORK_USER_PIC_SH+4;
	pic.bm_rect.right  = pic.bm_rect.left+SONORK_USER_PIC_SW+4;
	pic.bm.InitHwnd( pic.hwnd );



	this->LoadLangEntries(gls_table,false);
	page_win[TAB_INFO]->LoadLangEntries(gls_info,false);
	page_win[TAB_NOTES]->LoadLangEntries(gls_notes,false);
	page_win[TAB_AUTH]->LoadLangEntries(gls_auth,false);
	page_win[TAB_ONL]->LoadLangEntries(gls_onl,false);
	page_win[TAB_ALERT]->LoadLangEntries(gls_alrt,false);

	service_list.SetHandle(page_win[TAB_ONL]->GetDlgItem(IDC_USERONL_LIST)
		, true , true );
	service_list.AddColumns(4,service_list_cols);


	SetTab(tab_ctrl.page, true );


	UI_LoadAuths();
	UI_LoadLocalNotes();
	UI_LoadRemoteNotes( true );
	UI_LoadInfo();

	UpdateCxItems();


	SonorkApp.TransferWinStartInfo( this , true, "UserData", NULL);
	::SetWindowPos(tab_ctrl.hwnd
		,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	return true;
}
void
 TSonorkUserDataWin::OnAfterCreate()
{
	TSonorkTempBuffer path(SONORK_MAX_PATH+64);
	TSonorkEnumDirHandle*DH;
	HWND	m_hwnd,o_hwnd;
	int	l;
	m_hwnd = page_win[TAB_ALERT]->GetDlgItem(IDC_UINFO_ALERT_MSGSOUND);
	o_hwnd = page_win[TAB_ALERT]->GetDlgItem(IDC_UINFO_ALERT_ONLSOUND);

	if( show_flags & SF_USER_IN_AUTHORIZED_LIST )
	{
		wsprintf(path.CStr(),"(%s)",SonorkApp.LangString(GLS_LB_DEFLT));
		ComboBox_AddString(m_hwnd,path.CStr());
		ComboBox_AddString(o_hwnd,path.CStr());

		wsprintf(path.CStr(),"(%s)",SonorkApp.LangString(GLS_LB_NONE));
		ComboBox_AddString(m_hwnd,path.CStr());
		ComboBox_AddString(o_hwnd,path.CStr());

		SonorkApp.GetDirPath(path.CStr(),SONORK_APP_DIR_SOUND,NULL);
		DH=SONORK_IO_EnumOpenDir(path.CStr(),NULL,"*.wav");
		if(DH)
		{
			do{
				if(DH->flags&SONORK_FILE_ATTR_DIRECTORY)continue;
				l=strlen(DH->name);
				if(l<5||l>42)continue;
				ComboBox_AddString(m_hwnd,DH->name);
				ComboBox_AddString(o_hwnd,DH->name);
			}while(SONORK_IO_EnumNextDir(DH)==SONORK_RESULT_OK);
			SONORK_IO_EnumCloseDir(DH);
		}
		if(ctx_user->TestCtrlFlag(SONORK_UCF_MUT_MSG_SOUND))
			l=1;
		else
		{
			l=ComboBox_FindStringExact(m_hwnd,-1,ctx_user->sound.message.CStr());
			if(l==LB_ERR)l=0;
		}
		ComboBox_SetCurSel(m_hwnd,l);

		if(ctx_user->TestCtrlFlag(SONORK_UCF_MUT_ONL_SOUND))
			l=1;
		else
		{
			l=ComboBox_FindStringExact(o_hwnd,-1,ctx_user->sound.online.CStr());
			if(l==LB_ERR)l=0;
		}
		ComboBox_SetCurSel(o_hwnd,l);

		page_win[TAB_ALERT]->SetCtrlChecked(IDC_UINFO_ALERT_SLIDE
			,!ctx_user->TestCtrlFlag(SONORK_APP_UCF_NO_SLIDER_CX_DISPLAY));
		page_win[TAB_ALERT]->SetCtrlChecked(IDC_UINFO_ALERT_STATUS
			,!ctx_user->TestCtrlFlag(SONORK_APP_UCF_NO_STATUS_CX_DISPLAY));
	}
	else
	{
		::EnableWindow(m_hwnd,false);
		::EnableWindow(o_hwnd,false);
	}
	page_win[TAB_INFO]->SetCtrlEnabled( IDC_UINFO_DALIAS
		, show_flags & SF_USER_IN_LOCAL_LIST);
	if( MayStartTask() && (show_flags&SF_AUTO_REFRESH))
		PostPoke(POKE_REFRESH,0);

	if( show_flags & SF_USER_IN_LOCAL_LIST )
	{
		SonorkApp.GetDirPath(path.CStr()
			, SONORK_APP_DIR_DATA
			, uid_str);
		strcat(path.CStr(),".bmp");
		if( pic.bm.LoadFile(path.CStr()
				,SONORK_USER_PIC_SW
				,SONORK_USER_PIC_SH) )
		{
			SetWinUsrFlag( UWF_PIC_LOADED );
		}
	}
	if( !TestWinUsrFlag( UWF_PIC_LOADED ) )
	{
		pic.bm.CreateBlank(SONORK_USER_PIC_SW
			, SONORK_USER_PIC_SH
			, GetSysColorBrush(COLOR_3DDKSHADOW) );
	}
	::InvalidateRect(pic.hwnd,NULL,true);
}


void
 TSonorkUserDataWin::OnBeforeDestroy()
{
	SonorkApp.TransferWinStartInfo( this , false, "UserData", NULL);
	if(service.instance != 0)
	{
		SonorkApp.Service_Unregister(
			  service.instance
			, false);
		service.instance=0;
	}

}

// ==================================================
// 		EVENT HANDLERS
// ==================================================

bool 	TSonorkUserDataWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		switch( id )
		{
			case IDOK:
				// If IsTaskPending() it means
				// that we're storing the data
				// onto the server also, don't
				// quit yet
				if( !CmdStore() )
					break;
				Destroy(IDOK);
				break;
			case IDC_UINFO_REFRESH:
				CmdRefresh();
				break;
			default:
				return false;
		}
		return true;
	}
	return false;
}


bool
 TSonorkUserDataWin::OnAppEvent(UINT event, UINT param , void* data)
{
	TSonorkAppEventUserSid*userSid;
	TSonorkExtUserData*pUD;
	switch( event )
	{
		case SONORK_APP_EVENT_CX_STATUS:
			UpdateCxItems();
			return true;


		case SONORK_APP_EVENT_USER_SID:
			if( param != SONORK_APP_EVENT_USER_SID_LOCAL )
				break;
				
			userSid=(TSonorkAppEventUserSid*)data;
			pUD=userSid->local.userData;
			if( pUD->userId==ctx_user->userId)
			{
				UpdateQueryButtons();
				if( userSid->local.onlineDir != 0)
				{
					if( userSid->local.onlineDir==-1 )
						service_list.DelAllItems();
					else
					if(tab_ctrl.page == TAB_ONL )
					{
                                 		StartRemoteQuery(QUERY_SERVICES);
					}
				}

			}

			break;


		case SONORK_APP_EVENT_DEL_USER:
			{
				TSonorkId* deletedUserId=(TSonorkId*)data;
				if( *deletedUserId == ctx_user->userId )
				{
					Destroy(IDCANCEL);
				}
			}
			return true;
	}
	return false;
}
bool
  TSonorkUserDataWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDC_USERDATA_SIGN )
	{
		sonork_skin.DrawSign(S->hDC,SKIN_SIGN_QUERY,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

LRESULT
 TSonorkUserDataWin::OnPoke(SONORK_WIN_POKE poke,LPARAM lParam)
{
	switch( poke )
	{

		case POKE_PROCESS_PIC:
			OnPoke_ProcessPic((char*)lParam);
			break;

		case POKE_CONNECT_SERVICE:
			OnPoke_ConnectService();
			break;

		case SONORK_WIN_POKE_CHILD_DRAW_ITEM:
			return OnChildDialogDraw((TSonorkChildDialogNotify*)lParam);

		case SONORK_WIN_POKE_CHILD_NOTIFY:
			return OnChildDialogNotify((TSonorkChildDialogNotify*)lParam);

		case POKE_REFRESH:
			CmdRefresh();
			break;

		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			// Update the auths form to whatever it is now, irrelevant
			// of whether we stored them in the server successfully or not:
			//  If we DID store them , the form will reflect the flags accepted
			//  by the server.
			//  If we did NOT store them , the form will be reset to the real flags
			UI_LoadAuths();
			if(taskERR.Result() != SONORK_RESULT_OK )
			{
				TaskErrorBox(
					lParam == SONORK_FUNCTION_GET_USER_DATA
						?GLS_TK_GETINFO
						:GLS_TK_PUTINFO
					, &taskERR
					, SKIN_SIGN_USERS);

			}
			else
			{
				if( lParam == SONORK_FUNCTION_GET_USER_DATA )
				{
					UI_LoadInfo();
					UI_LoadRemoteNotes( false );
					TSonorkWin::SetStatus(status_bar,GLS_MS_RFRESHED,SKIN_HICON_INFO);
					ClearWinUsrFlag(UWF_CX_ONLY_ALERT);
				}
				else
				{
					Destroy(IDOK);
					break;
				}
			}
			break;
	}
	return 0;
}

LRESULT
 TSonorkUserDataWin::OnChildDialogDraw(TSonorkChildDialogNotify* DN)
{
	DRAWITEMSTRUCT *S;

	S=DN->draw;
	if( DN->dialog_id == IDD_USERINFO)
	{
		if( S->CtlID == IDC_UINFO_PIC )
		{
			::FillRect(S->hDC
				,&S->rcItem
				,GetSysColorBrush(
				  service.query_type==QUERY_PIC
				  ?COLOR_INFOBK
				  :COLOR_3DFACE));
			if( pic.bm.Loaded()
			&& service.query_type!=QUERY_PIC)
			{
				::DrawEdge(
				   S->hDC
					,&pic.bm_rect
					, EDGE_RAISED
					, BF_RECT);
				pic.bm.BitBlt(S->hDC
					, pic.bm_rect.left+2
					, pic.bm_rect.top +2 );
			}

		}
	}
	else
	if( DN->dialog_id == IDD_USERALERT)
	{
		if( S->CtlID == IDT_UINFO_ALERT_MSGSOUND
		 || S->CtlID == IDT_UINFO_ALERT_ONLSOUND)
		{
			sonork_skin.DrawIcon(S->hDC
				,SKIN_ICON_SOUND
				,S->rcItem.left
				,S->rcItem.top);
			return true;
		}
	}
	return false;
}
void
 TSonorkUserDataWin::SetTab(TAB tb, bool manual_change)
{
	if(ctx_user->UserType() != SONORK_USER_TYPE_AUTHORIZED && tb == TAB_AUTH)
		tb = TAB_INFO;

	if( tb!= tab_ctrl.page || manual_change )
	{
		SetWinUsrFlag(SONORK_WIN_F_UPDATING);
		if(manual_change)
			TabCtrl_SetCurSel(tab_ctrl.hwnd,tb);
		// Update page
		tab_ctrl.page = tb;
		for(int i=0;i<TABS;i++)
			page_win[i]->ShowWindow( tb == i?SW_SHOW:SW_HIDE);
		if( (tb == TAB_AUTH || tb==TAB_NOTES || tb==TAB_ONL) &&
			!SonorkApp.CxReady())
		{
			SetWinUsrFlag(UWF_CX_ONLY_ALERT);
			TSonorkWin::SetStatus(status_bar,GLS_MS_CXONLY,SKIN_HICON_ALERT);
		}
		else
		if(TestWinUsrFlag(UWF_CX_ONLY_ALERT))
		{
			ClearWinUsrFlag(UWF_CX_ONLY_ALERT);
			TSonorkWin::SetStatus_None(status_bar);
		}
		ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
		UpdateQueryButtons();
		if( tb == TAB_ONL && !TestWinUsrFlag(UWF_SERVICES_QUERIED) )
			StartRemoteQuery(QUERY_SERVICES);
	}
}
LRESULT
 TSonorkUserDataWin::OnChildDialogNotify(TSonorkChildDialogNotify*DN)
{
	TSonorkWinNotify *N=DN->N;
	if( DN->dialog_id == IDD_USERONL)
	{
		if(N->hdr.hwndFrom != service_list.Handle() )
			return 0L;
		if( N->hdr.code == LVN_DELETEITEM )
		{
			if( N->lview.lParam != 0)
			{
				SONORK_MEM_DELETE(
					(TSonorkUserServer*)(N->lview.lParam)
				);
			}
		}
		else
		if( N->hdr.code == NM_DBLCLK)
		{
			PostPoke(POKE_CONNECT_SERVICE ,0 );

		}

	}
	else
	if( DN->dialog_id == IDD_USERALERT )
	{
		if( N->hdr.code == BN_CLICKED )
		{
			int		id;
			HWND		hwnd;
			char 		tmp[64];
			if(N->hdr.idFrom == IDT_UINFO_ALERT_ONLSOUND)
				id = IDC_UINFO_ALERT_ONLSOUND;
			else
			if(N->hdr.idFrom == IDT_UINFO_ALERT_MSGSOUND )
				id = IDC_UINFO_ALERT_MSGSOUND;
			else
				return 0;
			hwnd =  page_win[TAB_ALERT]->GetDlgItem(id);
			id = (int)ComboBox_GetCurSel(hwnd);
			if(id<=1)return 0;
			ComboBox_GetString(hwnd,id,tmp);
			SonorkApp.WavSound(tmp );
			return true;
		}
	}
	else
	if( DN->dialog_id == IDD_USERINFO )
	{
		if( N->hdr.code == BN_CLICKED
		&& N->hdr.idFrom == IDC_UINFO_PIC_REFRESH)
		{

			StartRemoteQuery(QUERY_PIC);
			return true;
		}

	}
	return false;
}
LRESULT
 TSonorkUserDataWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if( N->hdr.hwndFrom == tab_ctrl.hwnd )
	if( N->hdr.code	  == TCN_SELCHANGE )
	if(!TestWinUsrFlag(SONORK_WIN_F_UPDATING))
	{
		SetTab( (TAB)TabCtrl_GetCurSel(tab_ctrl.hwnd), false);
	}
	return 0;
}

BOOL CALLBACK
 TSonorkUserDataWin::CxItemsEnable( HWND hwnd, LPARAM lParam )
{
	TSonorkUserDataWin*_this = (TSonorkUserDataWin*)lParam;
	HWND parent_hwnd;

	parent_hwnd = ::GetParent(hwnd);
	if( parent_hwnd == _this->page_win[TAB_AUTH]->Handle() )
	{
		switch(::GetWindowLong(hwnd,GWL_ID))
		{
				case IDC_AUTH_NEMAIL:
				case IDC_AUTH_NADDR:
				case IDC_AUTH_BUSY:
				case IDC_AUTH_NBUSY:
				case IDC_AUTH_FRIENDLY:
				case IDC_AUTH_NFRIENDLY:
				case IDC_AUTH_NCX:
//				case IDC_AUTH_UTS_ENABLED:
				case IDC_UINFO_PRIV1:
				case IDC_UINFO_PRIV2:
				case IDC_UINFO_PRIV3:
				case IDC_UINFO_PRIV4:
				case IDC_UINFO_PRIV5:
				case IDC_UINFO_PRIV6:
				case IDC_UINFO_PRIV7:
					::EnableWindow( hwnd
						, SonorkApp.CxReady()
						&& !SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_VISIBILITY));
				break;
		}
	}
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkUserDataWin::UpdateCxItems()
{
	// Enable auth win controls,  we don't enable/disable the auth dialog
	//  itself because the appearance of the controls don't change if we
	//  do that (they don't get grayed out)
	EnumChildWindows(page_win[TAB_AUTH]->Handle(), CxItemsEnable, (LPARAM)this);

	UpdateQueryButtons();
}

// ----------------------------------------------------------------------------

void
TSonorkUserDataWin::UpdateQueryButtons()
{
	BOOL qb,rb;

	qb=  SonorkApp.CxReady()
	  && ctx_user->IsOnline()
	  && ctx_user->addr.version.VersionNumber() >= MAKE_VERSION_NUMBER(1,5,0,7)
	  && service.query_id==0;

	if( tab_ctrl.page >= TAB_ALERT )
	{
		rb=false;
	}
	else
	if( tab_ctrl.page == TAB_ONL )
	{
		// Refresh button acts differently when ONLINE tab is
		// displayed: It refreshes the remote users' services list
		rb=qb;
	}
	else
	{
		rb=SonorkApp.CxReady() && !IsTaskPending();
	}
	SetCtrlEnabled(IDC_UINFO_REFRESH,rb);
	page_win[TAB_INFO]->SetCtrlEnabled( IDC_UINFO_PIC_REFRESH
		, qb && pic.bm.Initialized() );
	::EnableWindow(service_list.Handle(), qb);

}


// ----------------------------------------------------------------------------

void
 TSonorkUserDataWin::OnPoke_ProcessPic(char *zip_path)
{
	TSonorkTempBuffer temp_path(SONORK_MAX_PATH);
	TSonorkTempBuffer user_path(SONORK_MAX_PATH);
	TSonorkZipStream* ZH;
	int 	i;
	bool	bBool;
	TSonorkBitmap	tmpBM;

	bBool=false;

	if(!pic.bm.Initialized() )
		goto clean_01;

	SonorkApp.GetTempPath(temp_path.CStr(),uid_str,".bmp");
	ZH = SonorkApp.ZipEngine()->OpenForInflate(
			  zip_path
			, 4096);
	if(ZH == NULL)
		goto clean_01;

	if(SonorkApp.ZipEngine()->InitInflateFileByNumber(
		  ZH
		, 0
		, temp_path.CStr()
		) )
	{

		for(i=0;i<32;i++)
		{
			if(!SonorkApp.ZipEngine()->DoInflateFile(ZH) )
			{
				break;
			}
			if( ZH->OperationComplete() )
			{
				bBool=true;
				break;
			}
		}
	}
	SonorkApp.ZipEngine()->Close(ZH);

	if( !bBool )
		goto clean_02;

	tmpBM.InitHwnd( pic.hwnd );
	if(tmpBM.LoadFile(temp_path.CStr()
			,SONORK_USER_PIC_SW
			,SONORK_USER_PIC_SH))
	{
		pic.bm.Transfer(tmpBM);
		::InvalidateRect(pic.hwnd,NULL,true);
		SonorkApp.GetDirPath(user_path.CStr()
			, SONORK_APP_DIR_DATA
			, uid_str);
		strcat(user_path.CStr(),".bmp");
		CopyFile(temp_path.CStr(),user_path.CStr(),false );
	}

clean_02:
	DeleteFile( temp_path.CStr() );
clean_01:
	DeleteFile( zip_path);
	delete[] zip_path;


}

// ----------------------------------------------------------------------------

