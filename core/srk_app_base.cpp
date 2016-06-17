#include "srk_defs.h"
#pragma hdrstop
#include "srk_app_base.h"
#include "srk_svr_login_packet.h"
#include "srk_codec_io.h"

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL) Version 1.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You should NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL), doing so will indicate your agreement
	to the the terms which may be differ for each version of the software.

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

#define MONITOR_REFRESH_USER_SERIAL_DIFF		20	// That sets the refresh user flag
#define MONITOR_MAX_REFRESH_USER_FAILURES		5	// After which user is removed
// Monitor won't run if MAX_PENDING_TASKS are active in Sonork
#define MONITOR_MAX_PENDING_TASKS			2


SONORK_C_CSTR	szUserData	="UserData";
SONORK_C_CSTR	szAuths		="Auths";
SONORK_C_CSTR	szUserGroups	="UserGroups";
SONORK_C_CSTR	szAlias		="Alias";
SONORK_C_CSTR	szCtrlData	="CtrlData";
SONORK_C_CSTR	szOnlSound	="OnlSound";
SONORK_C_CSTR	szMsgSound	="MsgSound";
SONORK_C_CSTR	szHost		="Host";
SONORK_C_CSTR	szNotes		="Notes";
SONORK_C_CSTR	szPassword	="Password";
SONORK_C_CSTR	szData		="Data";
SONORK_C_CSTR	szLang		="Lang";
SONORK_C_CSTR	szServers	="Servers";

static const char * szUserNotesName[TSonorkExtUserData::NOTES_TYPE_COUNT]=
	{{"LNotes"},{"RNotes"}};





// ----------------------------------------------------------------------------
// Sonork event handlers
// ----------------------------------------------------------------------------

void
	TSonorkAppBase::ProcessSonorkEvent(void*,TSonorkClientEvent*E)
{
	SONORK_RESULT	result;
	union {
		TSonorkAuthReqData* 		auth_req;
	}D;
	switch(E->EventType())
	{
		case SONORK_CLIENT_EVENT_STATUS_CHANGE:
			PSE_StatusChange(
				  E->Status()
				, E->ErrorInfo()
				, E->StatusFlags());
			break;

		case SONORK_CLIENT_EVENT_GLOBAL_TASK:
			PSE_GlobalTask(E->TaskFunction()
				,E->ErrorInfo());
			break;

		case SONORK_CLIENT_EVENT_MONITOR:
			PSE_Monitor(E->MonitorIdle());
			break;

		case SONORK_CLIENT_EVENT_MSG:
			E->Msg()->header.time.ToLocal();
			OnSonorkMsg(E->Msg());
			break;

		case SONORK_CLIENT_EVENT_CTRL_MSG:
			OnSonorkCtrlMsg(E->CtrlMsgSender()
				, E->CtrlMsg()
				, E->CtrlMsgData());
			break;

		case SONORK_CLIENT_EVENT_SYS_MSG:
			OnSonorkSysMsg( E->SysMsgIndex() , E->SysMsg() );
			break;

		case SONORK_CLIENT_EVENT_USER_AUTH:
			D.auth_req = E->UserAuthReq();
			// An user in the authorization list cannot be in the user list
			DelUser( D.auth_req->user_data.userId , true );
			D.auth_req->header.time.ToLocal();
			result = SaveAuthReq( D.auth_req );
			if( result == SONORK_RESULT_OK )
				OnSonorkUserAuth( D.auth_req );
			break;

		case SONORK_CLIENT_EVENT_USER_ADD:
			PSE_AddUser( E->UserData(), E->UserSetAuth(), E->UserSetNotes() , E->UserSetFlags());
			break;

		case SONORK_CLIENT_EVENT_USER_SET:
			PSE_SetUser( E->UserData(), E->UserSetAuth(), E->UserSetNotes() );
			break;

		case SONORK_CLIENT_EVENT_USER_SID:
			PSE_UserSid( E->SidLocus()
				, E->SidUserSerial()
				, E->SidVersion()
				, E->SidText()
				);
			break;

		case SONORK_CLIENT_EVENT_USER_DEL:
			PSE_DelUser( E->UserId() );
			break;

		case SONORK_CLIENT_EVENT_USER_SYNCH_END:
			PSE_UserSynchEnd( E->UserId() , E->ErrorInfo() );
			break;

		case SONORK_CLIENT_EVENT_USER_GROUP_ADD:
			OnSonorkAddGroup( E->Group() );
			break;

		case SONORK_CLIENT_EVENT_WAPP_ADD:
			OnSonorkAddWapp( E->WappData() );
			break;

		case SONORK_CLIENT_EVENT_LOAD_LOGIN_REQ:
			PSE_LoadLoginRequest(E->LoginReqPacket());
			break;

		default:
			break;
	}

}
void
 TSonorkAppBase::PSE_GlobalTask(
	  SONORK_FUNCTION	function
	, const TSonorkError*	pERR)
{
	TSonorkExtUserData *UD;
	TSonorkListIterator	I;

	if( pERR == NULL ) // NB!: pERR is NULL when task is starting
	{

		switch( function )
		{
			case SONORK_FUNCTION_SET_SID:
				sonork_printf("SET_SID: STARTS, MODE{USER=%u,CTRL=%u}"
					,profile_user.addr.sidFlags.SidMode()
					,profile.ctrl_data.header.sidFlags.SidMode());
			break;
			case SONORK_FUNCTION_USER_LIST:
				// DelUserGroup( 0 ):
				//  Deletes the "Groups" folder from the bf_file
				//  (includes all sub-folders)
				DelUserGroup( 0 );

				UserList().BeginEnum(I);
				while( (UD=UserList().EnumNext(I)) != NULL)
				{
					if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
					{
						UD->RunFlags().Set(SONORK_URF_REFRESH_MARK);
					}
					else
					{
						UD->RunFlags().Clear(SONORK_URF_REFRESH_MARK);
						MarkForRefresh(UD);
					}
				}
				UserList().EndEnum(I);
			break;
		}
	}
	else
	{
		switch( function )
		{
			case SONORK_FUNCTION_SET_SID:
			if( pERR->Result() == SONORK_RESULT_OK)
			{
				profile_user.addr.SetOld(sonork.UserAddress());
				profile.ctrl_data.header.sidFlags.Set( sonork.UserAddress().sidFlags );
				sonork_printf("SET_SID: ENDS, MODE{USER=%u,CTRL=%u}"
					,profile_user.addr.sidFlags.SidMode()
					,profile.ctrl_data.header.sidFlags.SidMode());
				SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_CTRL_DATA );
			}
			break;
			case SONORK_FUNCTION_USER_LIST:
			{
				TSonorkExtUserDataQueue	dQ;
				UserList().BeginEnum(I);
				while( (UD=UserList().EnumNext(I)) != NULL)
					if(UD->RunFlags().Test(SONORK_URF_REFRESH_MARK))
						dQ.Add(UD);
				UserList().EndEnum(I);
				while((UD=dQ.RemoveFirst())!=NULL)
					DelUser(UD->userId , true);
			}
			break;
		}
	}

	OnSonorkGlobalTask(function,pERR);
}
void
 TSonorkAppBase::PSE_LoadLoginRequest(TSonorkSvrLoginReqPacket*REQ)
{
	DWORD	login_flags=0;
	login_flags|=OnSonorkLoadLoginRequest(login_flags);
	REQ->Prepare2(&profile_user
		     ,profile_password.CStr()
		     ,login_flags
		     ,app.version);
}
void
 TSonorkAppBase::PSE_StatusChange(SONORK_NETIO_STATUS status
	,const TSonorkError* pERR
	,DWORD flags)
{
	if(status == SONORK_NETIO_STATUS_DISCONNECTED
	|| status == SONORK_NETIO_STATUS_CONNECTED)
	{
		profile_user.addr.SetOld(sonork.UserAddress());
	}
	OnSonorkStatusChange(status,pERR,flags);
}
void	TSonorkAppBase::PSE_AddUser(
		TSonorkUserData*addUD
		,const TSonorkAuth2*auth
		,const TSonorkDynString*notes
		,DWORD msg_flags)
{
	if(addUD!=NULL)
	{
		TSonorkExtUserData *extUD;
		extUD=UserList().Get(addUD->userId);
		if(extUD==NULL)
		{
			TSonorkExtUserData eUD(SONORK_USER_TYPE_AUTHORIZED);
			eUD.TSonorkUserData::Set(*addUD);
			eUD.ClearExtData();
			eUD.ctrl_data.auth.Set(*auth);
			extUD = wUserList().Add(&eUD);
			if(extUD==NULL)return;
		}
		else
		{
			extUD->TSonorkUserData::Set(*addUD);
			extUD->ctrl_data.auth.Set(*auth);
		}
		extUD->RunFlags().Clear(SONORK_URF_REFRESH_MARK);
		_MarkForRefresh(extUD,false);
		_SaveUser(extUD);
		SaveUserNotes(extUD->userId,notes,TSonorkExtUserData::REMOTE_NOTES);
		DelAuthReq(extUD->userId);
		if(extUD->UserType() != SONORK_USER_TYPE_AUTHORIZED)
		{
			OnSonorkDelUser( extUD->userId , extUD );
			extUD->SetUserType(SONORK_USER_TYPE_AUTHORIZED);
		}
		OnSonorkAddUser(extUD,msg_flags);
	}
}
void	TSonorkAppBase::PSE_SetUser(
	TSonorkUserData*pUD
	,const TSonorkAuth2*auth
	,const TSonorkDynString*notes)
{
	TSonorkExtUserData *extUD;

	if(pUD==NULL)return;

	extUD=UserList().Get(pUD->userId);

	if(extUD==NULL)return;

	extUD->TSonorkUserData::Set(*pUD);
	extUD->ctrl_data.auth.Set(*auth);
	if(extUD->TestCtrlFlag(SONORK_UCF_SERVER_SYNCH_ALIAS))
		extUD->display_alias.Set(pUD->alias);

	_MarkForRefresh(extUD,false);
	extUD->RunFlags().Clear(SONORK_URF_REFRESH_MARK);
	if( extUD->IsValidUserType() )
	{
		_SaveUser(extUD);
		SaveUserNotes(extUD->userId,notes,TSonorkExtUserData::REMOTE_NOTES);
		OnSonorkSetUser(extUD,auth,notes);
	}
}


void	TSonorkAppBase::PSE_UserSid(
		const TSonorkUserLocus3*	nLocus
	, 	const TSonorkSerial*		uSerial
	,	const TSonorkVersion*		sVersion
	,	      TSonorkDynString*		sText)
{
	TSonorkExtUserData *extUD;
	extUD=UserList().Get(nLocus->userId);
	if(extUD!=NULL)
	{
		TSonorkUserLocus3 oLocus;
		extUD->GetLocus3(&oLocus);
		if(nLocus->SidMode() != SONORK_SID_MODE_DISCONNECTED)
		{
			extUD->RunValue(SONORK_URV_CURRENT_SID_SEQ_NO)+=1;
			extUD->addr.sid.Set(nLocus->sid);
			extUD->addr.sidFlags.Set(nLocus->sidFlags);
			extUD->addr.physAddr.Set(nLocus->physAddr);
			extUD->wRegion().Set(nLocus->region);
			extUD->sid_text.Set( sText->ToCStr() );
			extUD->addr.version.Set(*sVersion);
			if( !uSerial->IsZero() )
			if(  uSerial->AbsDiff(extUD->Serial()) > MONITOR_REFRESH_USER_SERIAL_DIFF )
			{
				_MarkForRefresh(extUD,true);
			}
		}
		else
		{
			extUD->sid_text.Clear();
			extUD->addr.version.Clear();

			extUD->RunValue(SONORK_URV_PROCESSED_SID_SEQ_NO)=
			extUD->RunValue(SONORK_URV_CURRENT_SID_SEQ_NO)=0;
			if( extUD->addr.sidFlags.SidMode() == SONORK_SID_MODE_DISCONNECTED)
				return;
			extUD->addr.Clear();
		}
		OnSonorkUserSid(extUD,oLocus);
	}

}

void	TSonorkAppBase::PSE_DelUser(const TSonorkId&user_id)
{
	DelUser( user_id , true);
	DelAuthReq( user_id );
}

void	TSonorkAppBase::PSE_Monitor(BOOL idle)
{
	OnSonorkMonitor(idle);
	TSonorkListIterator I;
	TSonorkExtUserData	*UD;
	// Don't 'monitor' it alreay busy
	if(sonork.PendingRequests()>=MONITOR_MAX_PENDING_TASKS)
		return;

	// <force_monitor_count> indicates that there are pending tasks
	// to be done by the monitor; we execute them immediately
	if(AppRunValue(SONORK_ARV_MONITOR_TASKS)>0)
		AppRunValue(SONORK_ARV_MONITOR_TASKS)-=1;
	else
	{
		// if no <force_monitor_count> tasks are pending,
		// we execute the monitor only while idle
		if( !idle )
			return;
	}
	// We've been idle for quite a while now: Check if there are
	// users to be synchronized and if not, ping the server.
	UserList().BeginEnum(I);
	while((UD=UserList().EnumNext(I))!=NULL)
	{
		if(!UD->TestCtrlFlag(SONORK_UCF_SERVER_SYNCH_PENDING))
			continue;
			
		if(UD->TestCtrlFlag(SONORK_UCF_SERVER_SYNCHING))
			continue;

		// Only one user per idle time, we want to keep the
		// bandwidth utilization to a minimum
		if(!UD->TestCtrlFlag(SONORK_UCF_SERVER_SYNCH_SKIP))
		{
			TSonorkError	dummyERR;
			UD->SetCtrlFlag(SONORK_UCF_SERVER_SYNCHING);
			UD->SetCtrlFlag(SONORK_UCF_SERVER_SYNCH_SKIP);
			sonork.RefreshUser(	dummyERR //dummy error
							,UD->UserId()
							,UD->Auth());
			// Set UD to NULL so it is not deleted
			// when this loop exits.
			UD = NULL;
			break;
		}
		// SERVER_SYNCH_SKIP is used for the case
		// where we have the user listed in the local profile
		// but the (remote) server does not (so the reality is
		// that we should not have this user in our local list,
		// but that's another issue that should be handled
		// by the container application)

		// So, getting back to SERVER_SYNCH_SKIP: the flag
		// is set every time the RefreshUser() is invoked
		// and cleared when the operation finishes succesfully
		// (when the SonorkEvent_SetUser() event is fired)
		// IF the operation does NOT finish succesfully, this
		// flag will not be cleared (nor will SERVER_SYNCH)
		// so next time we iterate the list, we skip those
		// that could not be succesfully refreshed.
		// (Note that the next-next time we iterate the user list the
		//  flag will not be set and we'll retry (probably failing again)
		//  However, with this method, failure to refresh an
		//  user will slow down the process but will not stop it)

		UD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCH_SKIP);
		if((UD->RunValue(SONORK_URV_REFRESH_FAIL_COUNT)+=1)>MONITOR_MAX_REFRESH_USER_FAILURES)
		{
			// We break here so that UD will not be NULL at the
			// exit point of this loop and it will be deleted.
			// Why not delete it right here?
			//  Because we can't modify the user list while
			//  we're iterating it.
			break;
		}
		// Increment SONORK_ARV_MONITOR_TASKS so that we come back
		// and check if the user was refreshed.
		AppRunValue(SONORK_ARV_MONITOR_TASKS)+=1;
	}
	UserList().EndEnum(I);
	if( UD != NULL )
	{
		DelUser(UD->userId,true);
	}
}

void	TSonorkAppBase::PSE_UserSynchEnd(const TSonorkId&user_id, const TSonorkError*pERR)
{
	TSonorkExtUserData	*extUD;
	extUD = GetUser(user_id);
	if(extUD!=NULL && pERR->Result()==SONORK_RESULT_OK)
	{
		extUD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCH_PENDING);
		AppRunValue(SONORK_ARV_MONITOR_TASKS)+=1;
	}
}


// ----------------------------------------------------------------------------
// Constructor

TSonorkAppBase::TSonorkAppBase():TSonorkEventHandler()
	,profile_user(SONORK_USER_INFO_MAX_LEVEL)
	,sonork(this,0)
{
	profile_user.userId.Clear();
	bf_key.app=bf_key.profile=bf_key.users=NULL;
}



TSonorkExtUserData*
 TSonorkAppBase::AddRemoteUser(const TSonorkId& userId,SONORK_C_CSTR palias, const TSonorkAuth2*pAuth)
{
	TSonorkExtUserData *extUD;
	char			tmp[SONORK_USER_ALIAS_MAX_SIZE+48];
	if(userId.IsZero() || userId==ProfileUserId())
		return NULL;
	extUD=UserList().Get(userId);
	if(extUD==NULL)
	{
		TSonorkExtUserData tUD(SONORK_USER_TYPE_NOT_AUTHORIZED);
		tUD.Clear(false);
		tUD.userId.Set(userId);
		if(palias)
			SONORK_StrCopy(tmp,SONORK_USER_ALIAS_MAX_SIZE,palias);
		else
			tmp[0]=0;
		if(!tmp[0])
			userId.GetStr(tmp);
		extUD = wUserList().Add(&tUD);
		if(extUD!=NULL)
		{
			extUD->ClearCtrlData();
			extUD->ClearRunData();
			if(pAuth)
				extUD->Auth().Set(*pAuth);
			else
			{
				extUD->Auth().Clear();
				extUD->Auth().SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
			}
			extUD->alias.Set(tmp);
			extUD->display_alias.Set(tmp);

			_MarkForRefresh(extUD,true);
			extUD->SetCtrlFlag(SONORK_UCF_SERVER_SYNCH_ALIAS);

			_SaveUser(extUD);
			OnSonorkAddUser(extUD,SONORK_MSG_SF_SELF_TRIGGERED);
		}
	}
	return extUD;

}


SONORK_RESULT
 TSonorkAppBase::OpenApp(
	  SONORK_C_CSTR 	config_file_dir
	, SONORK_C_CSTR 	config_file_name
	, DWORD 		app_version
	, DWORD			flags
	, SONORK_OS_TYPE        os_type
	, DWORD 		os_version)
{
	SONORK_RESULT result;
	bool	created=false;
	app.version.Load(app_version , flags , os_type , os_version);
	CloseApp();
	result = bf_file.Open(config_file_dir,config_file_name,true);

label_00:
	if( result == SONORK_RESULT_OK )
	{
		bf_key.app =bf_file.OpenDirBlock(NULL,"App",true);
		if(bf_key.app == NULL )
		{
			result = bf_file.Result();
			CloseApp();
		}
		else
			ConfigFile().Read(bf_key.app,szCtrlData,&app.ctrl_data);
	}
	if(result!=SONORK_RESULT_OK && !created)
	{
		created=true;
		result = bf_file.Create(config_file_dir,config_file_name);
		goto label_00;
	}
	return result;
}
void
 TSonorkAppBase::CloseApp()
{
	SONORK_ZeroMem(&app.run_data,sizeof(app.run_data));
	if( bf_key.app != NULL )
	{
		SaveAppCtrlData();
		SONORK_MEM_DELETE(bf_key.app);
		bf_key.app=NULL;
	}
	bf_file.Close();

}

SONORK_RESULT	TSonorkAppBase::CreateProfile(
				 TSonorkId& 	user_id
				, SONORK_C_CSTR alias
				, SONORK_C_CSTR password)
{

	SONORK_RESULT result;
	result = _OpenProfile(user_id,true,password);

	if(result==SONORK_RESULT_OK)
	{
		profile_user.alias.Set(alias);
		profile_user.name.Set(alias);
		profile_password.Set(password);
		sound.message.Clear();
		sound.login.Clear();
		profile.ctrl_data.CODEC_Clear();
	}
	_CloseProfile( result==SONORK_RESULT_OK , false );//Will save if result=OK

	return result;
}
SONORK_RESULT	TSonorkAppBase::GetProfileInfo( TSonorkUserData&UD , TSonorkProfileCtrlData*CD )
{
	char tmp[48];
	UD.userId.GetStr(tmp);
	return GetProfileInfo(tmp,&UD,CD);

}

SONORK_RESULT	TSonorkAppBase::GetProfileInfo(SONORK_C_CSTR gu_id_str
										,TSonorkUserData*UD
										,TSonorkProfileCtrlData*CD)
{
	TSonorkBfBlock*B,*user_key;
	SONORK_RESULT	result;

	if(!UD&&!CD)
		return SONORK_RESULT_INVALID_PARAMETER;

	B=bf_file.OpenDirBlock(NULL,"Profiles",false);
	if(B)
	{
		user_key=ConfigFile().OpenDirBlock(B,gu_id_str,false);
		if(user_key)
		{
			if(UD!=NULL)
			{
				result=ConfigFile().Read(user_key,szUserData,UD);
				if(result==SONORK_RESULT_OK
					&&
					UD->GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_3)
					result=SONORK_RESULT_NO_DATA;
			}
			else
				result=SONORK_RESULT_OK;
			if( result==SONORK_RESULT_OK && CD != NULL )
			{
				ConfigFile().Read(user_key,szCtrlData,CD );

			}
			SONORK_MEM_DELETE(user_key);
		}
		else
			result=ConfigFile().Result();
		SONORK_MEM_DELETE(B);
	}
	else
		result=ConfigFile().Result();
	return result;
}


SONORK_RESULT
 TSonorkAppBase::_OpenProfile(TSonorkId& userId
	, bool create
	, SONORK_C_CSTR open_password)
{
	TSonorkBfBlock*	B;
	SONORK_RESULT	result;
	_CloseProfile(true,true);
	profile_user.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_3, false);
	profile_user.userId.Set(userId);

	B=bf_file.OpenDirBlock(NULL,"Profiles",create);
	if(B)
	{
		char tmp[48];
		profile_user.userId.GetStr(tmp);
		bf_key.profile=bf_file.OpenDirBlock(B,tmp,create);
		if(bf_key.profile)
			result=SONORK_RESULT_OK;
		else
			result=bf_file.Result();
		SONORK_MEM_DELETE(B);
	}
	else
		result=bf_file.Result();
	if(result==SONORK_RESULT_OK)
	{
		if((bf_key.users=bf_file.OpenDirBlock(bf_key.profile,"Users",true))==NULL)
			result=bf_file.Result();
	}

	if( result == SONORK_RESULT_OK )
	{
		ReadProfileItem(szCtrlData , &profile.ctrl_data );
		if(ProfileCtrlFlags().Test(SONORK_PCF_NO_SAVE_PASS))
		{
			profile_password.Clear();
			ProfileCtrlFlags().Clear(SONORK_PCF_PASS_PROTECT);
		}
		else
		{
			if(ReadProfileItem(szPassword,&profile_password)!=SONORK_RESULT_OK)
			{
				ProfileCtrlFlags().Clear(SONORK_PCF_NO_SAVE_PASS);
				ProfileCtrlFlags().Clear(SONORK_PCF_PASS_PROTECT);
			}
			else
			if( ProfileCtrlFlags().Test(SONORK_PCF_PASS_PROTECT) )
			{
				if( strcmp(open_password
					?open_password:""
					,profile_password.CStr()) )
					result=SONORK_RESULT_ACCESS_DENIED;
			}
		}

	}
	
	profile_user.addr.version.Set( app.version );
	if( result == SONORK_RESULT_OK )
	{
		profile.open=true;
	}

	return result;
}

void
 TSonorkAppBase::_CloseProfile(bool save, bool gen_close_event)
{
	if(IsProfileOpen())
	{
		profile.open=false;
		if( gen_close_event )
			OnSonorkUserProfileOpen( false );
		if(save)
		{
			_SaveUserList();
			SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_ALL );
		}

	}
	if(bf_key.profile)SONORK_MEM_DELETE(bf_key.profile);
	if(bf_key.users)SONORK_MEM_DELETE(bf_key.users);
	bf_key.profile=bf_key.users=NULL;
	profile_user.Clear( false ); // false: Don't reset info level
//	profile_user.addr.sid_version.;
}


SONORK_RESULT	TSonorkAppBase::SaveAppCtrlData()
{
	if(bf_key.app!=NULL)
		return ConfigFile().Write(bf_key.app,szCtrlData,&app.ctrl_data);

	return   SONORK_RESULT_INVALID_OPERATION;
}

// SaveCurrentProfile(flags)
// flags are a combination of SONORK_APP_BASE_SAVE_PROFILE_FLAGS


SONORK_RESULT
	TSonorkAppBase::SaveCurrentProfile(DWORD save_flags)
{
	SONORK_RESULT		result;


	if( bf_key.users == NULL )
		return   SONORK_RESULT_INVALID_OPERATION;

	if( save_flags & SONORK_APP_BASE_SPF_SAVE_PASSWORD)
	{
		if(ProfileCtrlFlags().Test(SONORK_PCF_NO_SAVE_PASS))
		{
			TSonorkShortString dummy_pass;
			// User opted for NOT writing the password to the file,
			// so before writing it, clear it.
			dummy_pass.Clear();
			WriteProfileItem(szPassword,&dummy_pass);
			ProfileCtrlFlags().Clear(SONORK_PCF_PASS_PROTECT);
		}
		else
		{
			WriteProfileItem(szPassword,&profile_password);
			if(profile_password.Length()<1)
				ProfileCtrlFlags().Clear(SONORK_PCF_PASS_PROTECT);
		}
	}

	if( save_flags&SONORK_APP_BASE_SPF_SAVE_USER_DATA )
	{

		result=WriteProfileItem(szUserData,&profile_user);
	}
	else
		result=SONORK_RESULT_OK;
		
	if( save_flags & SONORK_APP_BASE_SPF_SAVE_CTRL_DATA )
	{
		WriteProfileItem(szCtrlData,&profile.ctrl_data);
	}

	if( save_flags & SONORK_APP_BASE_SPF_SAVE_SOUNDS )
	{
		WriteProfileItem(szOnlSound,&sound.login);
		WriteProfileItem(szMsgSound,&sound.message);
	}

	bf_file.Flush();
	return result;
}


SONORK_RESULT	TSonorkAppBase::DelProfile(TSonorkId& user_id,SONORK_C_CSTR password)
{
	SONORK_RESULT	result;
	result=_OpenProfile(user_id,false,password);
	if(result==SONORK_RESULT_OK)
	{
		TSonorkBfBlock*B;
		_CloseProfile(false,false);
		B=bf_file.OpenDirBlock(NULL,"Profiles",false);
		if(B)
		{
			char tmp[48];
			user_id.GetStr(tmp);
			result = bf_file.DeleteDirBlock(B,tmp);
			SONORK_MEM_DELETE(B);
		}
		else
			result=bf_file.Result();

	}
	return result;
}
SONORK_RESULT	TSonorkAppBase::OpenProfile(TSonorkId& userId,SONORK_C_CSTR password)
{
	SONORK_RESULT	result;
	result=_OpenProfile(userId,false,password);
	if(result==SONORK_RESULT_OK)
	{
		result=ReadProfileItem(szUserData,&profile_user);
		if(result==SONORK_RESULT_OK)
		{
			if(profile_user.GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_3)
				result=SONORK_RESULT_NO_DATA;
			else
			if(profile_user.userId!=userId)
				result=SONORK_RESULT_INTERNAL_ERROR;
			else
			{
				bool use_default;

				if(ReadProfileItem(szOnlSound,&sound.login)!=SONORK_RESULT_OK)
					use_default=true;
				else
					use_default=sound.login.Length()<1;

				if(use_default)sound.login.Clear();

				if(ReadProfileItem(szMsgSound,&sound.message)!=SONORK_RESULT_OK)
					use_default=true;
				else
					use_default=sound.message.Length()<1;

				if(use_default)sound.message.Clear();
			}
		}
	}
	if(result==SONORK_RESULT_OK)
	{
		// We don't check if the user list is loaded without errors
		// because the user data is already loaded and an error
		// while loading the list should not prevent the application
		// from opening the profile: The error can be fixed by refreshing
		// the user list from the server.
		_LoadUserList();
		OnSonorkUserProfileOpen(true);
	}
	if(result!=SONORK_RESULT_OK)
	{
		// If ReadAtom failed, our data size would've been reset to zero
		profile_user.SetUserInfoLevel(SONORK_USER_INFO_MAX_LEVEL,false);
		_CloseProfile(false,false);
	}
	return result;
}



SONORK_RESULT
	TSonorkAppBase::LoadUser(TSonorkExtUserData*UD)
{
	TSonorkBfBlock*			B;
	SONORK_RESULT			result;
	char				str[24];
	TSrkExtUserDataAtom	A(UD);

// Dir block name is the GuId and under "Users"

	UD->userId.GetStr(str);
	B=bf_file.OpenDirBlock(bf_key.users,str,false);
	if(B != NULL)
	{
		for(;;)
		{
			result=bf_file.Read(B,szUserData,&A);
			if( result!= SONORK_RESULT_OK)
				break;
			if(UD->GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_1)
			{
				result=SONORK_RESULT_NO_DATA;
				break;
			}
	// If szAlias could not be loaded or is zero-length
	// load DisplayAlias with real alias
			if( UD->display_alias.Length()<2 )
				UD->display_alias.Set(UD->alias);

			if(UD->TestCtrlFlag(SONORK_UCF_MUT_ONL_SOUND))
				UD->sound.online.Clear();

			if(UD->TestCtrlFlag(SONORK_UCF_MUT_MSG_SOUND))
				UD->sound.message.Clear();

	// Initialize all RUN data to zero
			UD->ClearRunData();
			result=SONORK_RESULT_OK;
			break;
		}
		SONORK_MEM_DELETE(B);
	}
	else
		result=bf_file.Result();
	return result;

}

SONORK_RESULT   TSonorkAppBase::SaveUser(const TSonorkId& user_id)
{
	TSonorkExtUserData *UD;
	if((UD = GetUser(user_id))==NULL)
		return SONORK_RESULT_INVALID_PARAMETER;
	else
		return _SaveUser(UD);
}

SONORK_RESULT   TSonorkAppBase::_SaveUser(TSonorkExtUserData*UD)
{
	SONORK_RESULT	result;
	if(UD->GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_1)
		result=SONORK_RESULT_INTERNAL_ERROR;
	else
	if( UD->IsValidUserType() )
	{
		TSonorkBfBlock  *B;
		char 		str[24];
		TSrkExtUserDataAtom	A(UD);

		if(UD->UserType() == SONORK_USER_TYPE_NOT_AUTHORIZED)
			UD->addr.Clear();
		UD->UserId().GetStr(str);
		B=bf_file.OpenDirBlock(bf_key.users,str,true);
		if( B != NULL)
		{
			if(UD->TestCtrlFlag(SONORK_UCF_MUT_ONL_SOUND))
				UD->sound.online.Clear();
				
			if(UD->TestCtrlFlag(SONORK_UCF_MUT_MSG_SOUND))
				UD->sound.message.Clear();

			result=bf_file.Write(B,szUserData,&A);

			SONORK_MEM_DELETE(B);
		}
		else
			result=bf_file.Result();
	}
	else
		result=SONORK_RESULT_INVALID_PARAMETER;
	return result;
}

SONORK_RESULT   TSonorkAppBase::DelUser(const TSonorkId& user_id, bool generate_event)
{
	SONORK_RESULT	result;
	char 		tmp[48];
	TSonorkExtUserData*extUD;
	user_id.GetStr(tmp);
	result=bf_file.DeleteDirBlock(bf_key.users,tmp);

	extUD = UserList().Get(user_id);
	if(generate_event)
		OnSonorkDelUser( user_id , extUD );

	if(extUD != NULL )
		wUserList().Del(user_id);
	return result;
}
SONORK_RESULT   TSonorkAppBase::WriteUserItem(const TSonorkId&user_id, SONORK_C_CSTR name, const TSonorkCodecAtom*A)
{
	SONORK_RESULT	result;
	TSonorkBfBlock     *B;
	char tmp[32];
	user_id.GetStr(tmp);
	B=bf_file.OpenDirBlock(bf_key.users,tmp,true);
	if( B != NULL)
	{
		result=bf_file.Write(B,name,A);
		SONORK_MEM_DELETE(B);
	}
	else
		result=bf_file.Result();
	return result;
}

SONORK_RESULT   TSonorkAppBase::ReadUserItem(const TSonorkId&user_id, SONORK_C_CSTR name, TSonorkCodecAtom*A)
{
	SONORK_RESULT	result;
	TSonorkBfBlock     *B;
	char tmp[32];
	user_id.GetStr(tmp);
	B=bf_file.OpenDirBlock(bf_key.users,tmp,false);
	if( B != NULL)
	{
		result=bf_file.Read(B,name,A);
		SONORK_MEM_DELETE(B);
	}
	else
	{
		A->CODEC_Clear();
		result=bf_file.Result();
	}
	return result;
}

SONORK_RESULT   TSonorkAppBase::LoadUserNotes(const TSonorkId&user_id
			,TSonorkDynString*notes
			,TSonorkExtUserData::NOTES_TYPE type)
{
	return ReadUserItem( user_id, szUserNotesName[type],notes);
}
SONORK_RESULT   TSonorkAppBase::SaveUserNotes(const TSonorkId&user_id
			,const TSonorkDynString*notes
			,TSonorkExtUserData::NOTES_TYPE type)
{
	return WriteUserItem( user_id, szUserNotesName[type],notes);
}

// ----------------------------------------------------------------------------
// Groups
// ----------------------------------------------------------------------------


SONORK_RESULT	TSonorkAppBase::SaveUserGroup( const TSonorkGroup*UG )
{
	char	tmp[24];
	assert( UG != NULL );
	sprintf(tmp,"%u",UG->GroupNo());
	return WriteProfileSubItem(szUserGroups,tmp,UG);
}

// if no=0 all groups are deleted
SONORK_RESULT	TSonorkAppBase::DelUserGroup(DWORD no)
{
	char		tmp[24];
	SONORK_RESULT	result;
	if( no == 0)
		result = DelProfileItem( szUserGroups , SONORK_BF_BLOCK_TYPE_DIR );
	else
	{
		sprintf(tmp,"%u",no);
		result= DelProfileSubItem( szUserGroups , tmp , SONORK_BF_BLOCK_TYPE_DATA );
	}
	return result;
}

SONORK_RESULT	TSonorkAppBase::LoadUserGroups(TSonorkGroupQueue& Q)
{
	SONORK_RESULT	result;
	TSonorkBfEnumHandle* E;

	if(!IsProfileOpen())
		return SONORK_RESULT_INVALID_OPERATION;

	E = bf_file.OpenEnum(bf_key.profile,szUserGroups,SONORK_BF_ENUM_DATA);
	if(E != NULL)
	{
		TSonorkGroup *UG=NULL;
		while( (result=bf_file.EnumNext(E)) == SONORK_RESULT_OK )
		{
			SONORK_MEM_NEW( UG = new TSonorkGroup );
			result= ReadProfileSubItem(szUserGroups,E->Name(),UG);

			if( result == SONORK_RESULT_OK )
			if( UG->name.Type() == SONORK_DYN_STRING_TYPE_C )
			if( UG->GroupType() == SONORK_GROUP_TYPE_USER )
			{
				Q.AddSorted(UG);
				UG=NULL;
			}
		}
		SONORK_MEM_DELETE(E);
		if( UG != NULL )SONORK_MEM_DELETE( UG );
		if(result==SONORK_RESULT_NO_DATA)
			result=SONORK_RESULT_OK;
	}
	else
		result=bf_file.Result();
	return result;

}


// ----------------------------------------------------------------------------
// Auth Req
// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkAppBase::SaveAuthReq(const TSonorkAuthReqData* RD)
{
	return WriteProfileSubItem( szAuths , RD->user_data.userId , RD);
}

SONORK_RESULT
	TSonorkAppBase::LoadAuthReq(TSonorkAuthReqData*	RD)
{
	return ReadProfileSubItem( szAuths , RD->user_data.userId , RD);
}

SONORK_RESULT	TSonorkAppBase::LoadAuthReqList(TSonorkAuthReqDataQueue& RD_list, bool incomming, bool outgoing)
{
	SONORK_RESULT	result;
	TSonorkBfEnumHandle* E;
	if(!IsProfileOpen())
		return SONORK_RESULT_INVALID_OPERATION;
	E= bf_file.OpenEnum(bf_key.profile,szAuths,SONORK_BF_ENUM_DATA);
	if(E != NULL)
	{
		TSonorkAuthReqData *RD=NULL;
		while( (result=bf_file.EnumNext(E)) == SONORK_RESULT_OK )
		{
			if(RD == NULL )
			{
				SONORK_MEM_NEW( RD = new TSonorkAuthReqData );
			}
			if(!RD->user_data.userId.SetStr(E->Name()))
				continue;
			if(LoadAuthReq(RD)!=SONORK_RESULT_OK)
				continue;
			if(RD->user_data.GetUserInfoLevel()<SONORK_USER_INFO_LEVEL_1)
				continue;
			if( RD->RequestorId() == ProfileUserId() )
			{
				if( !outgoing )
					continue;
			}
			else
			{
				if( !incomming )
					continue;
			}
			RD_list.Add(RD);
			RD=NULL;
		}
		SONORK_MEM_DELETE(E);
		if( RD != NULL )SONORK_MEM_DELETE( RD );
		if(result==SONORK_RESULT_NO_DATA)
			result=SONORK_RESULT_OK;
	}
	else
		result=bf_file.Result();
	return result;
}

SONORK_RESULT	TSonorkAppBase::DelAuthReq(const TSonorkId&user_id)
{
	return DelProfileSubItem(szAuths , user_id , SONORK_BF_BLOCK_TYPE_DATA);
}


// ----------------------------------------------------------------------------
// Users
// ----------------------------------------------------------------------------

#define MAX_INVALID_IDS_PER_LOAD	4
SONORK_RESULT
	TSonorkAppBase::_LoadUserList()
{
	SONORK_RESULT		result;
	TSonorkBfEnumHandle*	E;
	DWORD			invalid_id_count;
	char			invalid_id_list[MAX_INVALID_IDS_PER_LOAD][26];
//	DWORD			user_list_size;

	if(!IsProfileOpen())
		return SONORK_RESULT_INVALID_OPERATION;

//	user_list_size = ProfileCtrlValue( SONORK_PCV_USER_LIST_SIZE );
//	if( user_list_size > 192 )user_list_size = 192;
//	sonork_printf("::_LoadUserList() EULSZ=%u",ProfileCtrlValue( SONORK_PCV_USER_LIST_SIZE ));
	wUserList().Clear();
//	wUserList().PrepareFor( user_list_size );

//	user_list_size=0;
	E= bf_file.OpenEnum(bf_key.profile,"Users",SONORK_BF_ENUM_DIR);
	if(E != NULL)
	{
		TSonorkExtUserData  UD(SONORK_USER_TYPE_UNKNOWN);
		invalid_id_count=0;
		while( (result=bf_file.EnumNext(E)) == SONORK_RESULT_OK  )
		{
			if(!UD.userId.SetStr(E->Name()))
				continue;
			if(LoadUser(&UD)!=SONORK_RESULT_OK)
			{
				if(invalid_id_count<MAX_INVALID_IDS_PER_LOAD)
				{
					SONORK_StrCopy(
						invalid_id_list[invalid_id_count++]
						,24
						,E->Name());
				}
				continue;
			}
			UD.wAddress().Clear();
			if(UD.GetUserInfoLevel()>SONORK_USER_INFO_LEVEL_1)
				UD.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1,true);
			if( UD.IsValidUserType() )
				wUserList().Add(&UD);
		}
		SONORK_MEM_DELETE(E);
		if(result==SONORK_RESULT_NO_DATA)
			result=SONORK_RESULT_OK;
		ProfileCtrlValue( SONORK_PCV_USER_LIST_SIZE ) = UserList().Items();
		sonork_printf("::_LoadUserList() SIZE=%u (INVALID=%u)"
			,UserList().Items()
			,invalid_id_count);
		while( invalid_id_count-- )
		{
			bf_file.DeleteDirBlock(bf_key.users
			,invalid_id_list[invalid_id_count]);
		}
	}
	else
		result=bf_file.Result();
	return result;
}
SONORK_RESULT
	TSonorkAppBase::_SaveUserList()
{
	TSonorkExtUserData*  	UD;
	TSonorkListIterator	I;
	UINT			count;

	UserList().BeginEnum(I);
	for(count=0;(UD=UserList().EnumNext(I))!=NULL;count++)
		_SaveUser(UD);
	UserList().EndEnum(I);
	
	wUserList().Clear();
	ProfileCtrlValue(SONORK_PCV_USER_LIST_SIZE)=count;

	return SONORK_RESULT_OK;
}


// ----------------------------------------------------------------------------
// Generic Profile I/O
// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkAppBase::ReadProfileItem(SONORK_C_CSTR name, TSonorkCodecAtom*A)
	{
		return   bf_file.Read(bf_key.profile,name,A);
	}

SONORK_RESULT
	TSonorkAppBase::ReadProfileItem(SONORK_C_CSTR name, TSonorkShortString*S)
	{
		return	bf_file.Read(bf_key.profile,name,S);
	}
SONORK_RESULT
	TSonorkAppBase::ReadProfileItem(SONORK_C_CSTR name, SONORK_C_STR buffer, DWORD buffer_size)
	{
		TSonorkShortString S;
		SONORK_RESULT	result;
		if( (result=ReadProfileItem(name,&S)) == SONORK_RESULT_OK)
		{
			SONORK_StrCopy(buffer,buffer_size,S.CStr());
		}
		else
			*buffer=0;
		return result;
	}


SONORK_RESULT
	TSonorkAppBase::WriteProfileItem(SONORK_C_CSTR name, const TSonorkCodecAtom*A)
	{
		return   bf_file.Write(bf_key.profile,name,A);
	}

SONORK_RESULT
	TSonorkAppBase::WriteProfileItem(SONORK_C_CSTR name, const TSonorkShortString*S)
	{
		return	bf_file.Write(bf_key.profile,name,S);
	}

SONORK_RESULT
	TSonorkAppBase::DelProfileItem(SONORK_C_CSTR name , SONORK_BF_BLOCK_TYPE block_type)
{
	return bf_file.DeleteBlock(block_type , bf_key.profile , name);
}

SONORK_RESULT
	TSonorkAppBase::ReadProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name, TSonorkCodecAtom*A)
{
	TSonorkBfBlock	*keyB;
	SONORK_RESULT 	result;
	if((keyB=bf_file.OpenDirBlock(bf_key.profile, key , false ))==NULL)
	{
		result = bf_file.Result();
		assert( result != SONORK_RESULT_OK );
	}
	else
	{
		result=bf_file.Read(keyB,name,A);
		SONORK_MEM_DELETE(keyB);
	}
	return result;
}

SONORK_RESULT
	TSonorkAppBase::ReadProfileSubItem(SONORK_C_CSTR key, const SONORK_DWORD2& dw, TSonorkCodecAtom*A)
{
	char tmp[24];
	dw.GetStr(tmp , 10);
	return ReadProfileSubItem(key , tmp , A);
}

SONORK_RESULT
	TSonorkAppBase::WriteProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name, const TSonorkCodecAtom*A)
{
	TSonorkBfBlock	*keyB;
	SONORK_RESULT 	result;
	if((keyB=bf_file.OpenDirBlock(bf_key.profile, key , true ))==NULL)
	{
		result = bf_file.Result();
		assert( result != SONORK_RESULT_OK );
	}
	else
	{
		result=bf_file.Write(keyB,name,A);
		SONORK_MEM_DELETE(keyB);
	}
	return result;
}

SONORK_RESULT
	TSonorkAppBase::WriteProfileSubItem(SONORK_C_CSTR key, const SONORK_DWORD2& dw, const TSonorkCodecAtom*A)
{
	char tmp[24];
	dw.GetStr(tmp , 10);
	return WriteProfileSubItem(key , tmp , A);
}

SONORK_RESULT
	TSonorkAppBase::DelProfileSubItem(SONORK_C_CSTR key, const SONORK_DWORD2&dw , SONORK_BF_BLOCK_TYPE block_type)
{
	char tmp[24];
	dw.GetStr(tmp , 10);
	return DelProfileSubItem(key , tmp , block_type);
}

SONORK_RESULT
	TSonorkAppBase::DelProfileSubItem(SONORK_C_CSTR key, SONORK_C_CSTR name , SONORK_BF_BLOCK_TYPE block_type)
{
	TSonorkBfBlock	*keyB;
	SONORK_RESULT 	result;
	if((keyB=bf_file.OpenDirBlock(bf_key.profile, key , false ))==NULL)
	{
		result = bf_file.Result();
		assert( result != SONORK_RESULT_OK );
	}
	else
	{
		result=bf_file.DeleteBlock(block_type , keyB , name );
		SONORK_MEM_DELETE(keyB);
	}
	return result;
}


// ----------------------------------------------------------------------------
// Server Profile
// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkAppBase::DeleteServerProfile(SONORK_C_CSTR name)
{
	TSonorkBfBlock*B;
	SONORK_RESULT	result;
	if( !IsAppConfigOpen() )
		return SONORK_RESULT_INVALID_OPERATION;
	if( name == NULL )
	{
		result = bf_file.DeleteDirBlock(AppKey(),szServers);
	}
	else
	{
		B=bf_file.OpenDirBlock(AppKey(),szServers,false);
		if(B)
		{
			result=bf_file.DeleteDataBlock(B,name);
			SONORK_MEM_DELETE(B);
		}
		else
			result=bf_file.Result();
	}
	return result;
}
SONORK_RESULT
	TSonorkAppBase::_LoadServerProfile(SONORK_C_CSTR name, TSonorkClientServerProfile& prof)
{
	TSonorkBfBlock*B;
	SONORK_RESULT	result;
	if( !IsAppConfigOpen() )
		return SONORK_RESULT_INVALID_OPERATION;

	B=bf_file.OpenDirBlock(AppKey(),szServers,false);
	if(B)
	{
		result=bf_file.Read(B,name,&prof);
		SONORK_MEM_DELETE(B);
	}
	else
		result=bf_file.Result();
	return result;
}
SONORK_RESULT
	TSonorkAppBase::_SaveServerProfile(SONORK_C_CSTR name, const TSonorkClientServerProfile& prof)
{
	TSonorkBfBlock*B;
	SONORK_RESULT	result;
	if( !IsAppConfigOpen() )
		return SONORK_RESULT_INVALID_OPERATION;
	B=bf_file.OpenDirBlock(AppKey(),szServers,true);
	if(B)
	{
		result=bf_file.Write(B,name,&prof);
		SONORK_MEM_DELETE(B);
	}
	else
		result=bf_file.Result();
	return result;
}




void	TSonorkAppBase::_MarkForRefresh(TSonorkExtUserData*extUD,BOOL mark)
{
	extUD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCHING);
	extUD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCH_SKIP);
	extUD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCH_ALIAS);
	if(mark)
	{
		extUD->SetCtrlFlag(SONORK_UCF_SERVER_SYNCH_PENDING);
		extUD->RunValue(SONORK_URV_REFRESH_FAIL_COUNT)=0;
		AppRunValue(SONORK_ARV_MONITOR_TASKS)+=1;
	}
	else
	{
		extUD->ClearCtrlFlag(SONORK_UCF_SERVER_SYNCH_PENDING);
	}
}



void	TSonorkAppBase::EncodePin64(
		SONORK_DWORD4& 		snd_pin
		,const TSonorkId&	tgt_uid
		,SONORK_SERVICE_ID  	tgt_service_id
		,DWORD			tgt_service_pin)
{
	SONORK_EncodePin64(snd_pin
		,profile_user.userId    // our userId
		,sonork.SidPin()    	// our Sid Pin
		,tgt_uid	    	// remote userId
		,tgt_service_id
		,tgt_service_pin);
}


// ----------------------------------------------------------------------------





