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

#include "srkmainwin.h"
#include "srkmsgwin.h"
#include "srkuserdatawin.h"
#include "srkdbmaintwin.h"
#include "srkslidewin.h"
#include "srkfiletxgui.h"
#include "srksysconwin.h"
#include "srkremindwin.h"
#include "srkbicho.h"
#include "srkauthreqwin.h"
#include "srkmaininfowin.h"
#include "srk_uts.h"
#include "srk_url_codec.h"
#include "srk_winregkey.h"
#include "srk_task_atom.h"
#include "srk_codec_file.h"
#include "srk_email_codec.h"
#include "srk_cfg_names.h"
#include "srk_zip.h"

#define SONORK_APP_SERVICE_CACHE_SECS		420

#define SONORK_APP_TIMER_ID			10
#define SONORK_APP_DEFAULT_QUERY_TIMEOUT	15000
#define	EVENTS_SOUND_MSECS			(90*1000)
#define	BLINK_TASK_MSECS			1000
#define	MONITOR_TASK_MSECS			(BLINK_TASK_MSECS*3)
#define SYS_MSG_CHECK_MSECS			(30*60*1000)
#define CONNECT_RETRY_MSECS   			(MONITOR_TASK_MSECS*10)
#define	START_CONNECT_DELAY_MSECS		(MONITOR_TASK_MSECS*3)
#define CONNECT_READYGO_DELAY_MSECS		(MONITOR_TASK_MSECS*2)
#define	FLUSH_TASK_MSECS			(MONITOR_TASK_MSECS*100) // Every 5 minutes
#define	SONORK_APP_CHECK_ALARM_VERY_LATE_MINS	30
#define	SONORK_APP_CHECK_ALARM_VERY_SOON_SECS	15
#define SONORK_APP_CHECK_ALARM_SOON_SECS	20
#define	SONORK_APP_CHECK_ALARM_VERY_LATE_SECS	(SONORK_APP_CHECK_ALARM_VERY_LATE_MINS*60)

#define SONORK_APP_CHECK_EMAIL_VERY_SOON_SECS	20
#define SONORK_APP_CHECK_EMAIL_SOON_SECS	30

static UINT gWappMenuCmd;
static UINT gMtplMenuCmd;
static HWND gAppFocusHwnd=NULL;
static HWND gAppForegroundHwnd=NULL;

SONORK_C_CSTR	szSonorkAppMode;
TSonorkClock	SonorkAppTimerClock;


static struct
{
	SYSTEMTIME* 	ptr;
	HINSTANCE	hInstance;
}LUAT;// Last User Activity Time

struct TSonorkWin32AppInitData
{
#define INIT_APP_F_NO_INI_SELECT		0x001000
#define INIT_APP_F_SAVE_STARTUP_MODE		0x040000
#define INIT_APP_F_START_MINIMIZED		0x080000

	TSonorkId		user_id;
	DWORD			flags;
	DWORD			app_run_flags;
	SONORK_APP_START_MODE	start_mode;
	char		root_dir[SONORK_MAX_PATH];
	char		data_dir[SONORK_MAX_PATH];
	char		temp_dir[SONORK_MAX_PATH];
	char		user_name[48];
	char		language[48];
	char		password[SONORK_USER_PASS_MAX_SIZE];
	struct {
		char	host[48];
		DWORD	tcp_port;
		DWORD	udp_port;
	}server;
};

struct TSonorkWin32AppTaskTimers
{
	UINT	cx_status_msecs;
	UINT	blink_msecs;
	UINT	monitor_msecs;
	UINT	flush_msecs;
	UINT	sys_msg_msecs;
	UINT	alarm_check_msecs;
	UINT	alarm_check_limit;
	UINT 	event_sound_msecs;
	UINT	mail_check_msecs;
	UINT	mail_check_limit;
};
TSonorkWin32AppTaskTimers	task_timer;


static bool FindParallelInstance( SONORK_C_CSTR cfg_name );
extern int SONORK_ExecuteSetupDialog(TSonorkWin*);

enum RESOLVE_PHASE
{
	RESOLVE_PHASE_SOCKS
,	RESOLVE_PHASE_SONORK
};

// TSonorkWin POKEs private for the hidden win32.work_win
// 
enum SONORK_APP_WIN_POKE
{
	POKE_SHOW_SETUP_DIALOG	= SONORK_WIN_POKE_01
};


// ----------------------------------------------------------------------------


bool TSonorkWin32App::MayStartGlobalTask() const	// true if connected and no tasks pending
{
	return CxReady() && !GlobalTaskPending();
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::IsPseudoCxStatus()  const
{
	return	win32.cx_status==SONORK_APP_CX_STATUS_READY_AND_GO
			||
		(win32.cx_status>SONORK_APP_CX_STATUS_CONNECTING_FIRST
			&&	win32.cx_status<=SONORK_APP_CX_STATUS_CONNECTING_LAST);

}

// ----------------------------------------------------------------------------


SONORK_RESULT
 TSonorkWin32App::RecvFile(
	  const TSonorkId& 	userId
	, TSonorkCCacheMark* 	mark
	, TSonorkShortString& 	path)
{
	TSonorkCCacheEntry *	CL;
	TSonorkMsg		msg;
	TSonorkFileInfo		fileInfo;
	SONORK_RESULT		result;
	TSonorkCCache* 		cache;
	TSonorkMsgWin*		msgWin;
	TSonorkFileTxGui*	fileWin;
	TSonorkExtUserData*	UD;

	UD = UserList().Get( userId );

	if( UD == NULL )
		return SONORK_RESULT_INVALID_PARAMETER;


	msgWin=GetUserMsgWin(userId);
	if( msgWin == NULL )
	{
		TRACE_DEBUG("RecvFile______ 1");
		cache=GrabTmpMsgCache( userId );
		if(cache==NULL)
			return SONORK_RESULT_SERVICE_BUSY;
	}
	else
	{
		cache=msgWin->Cache();
		if(cache==NULL)
			return SONORK_RESULT_INTERNAL_ERROR;
	}

	for(;;)
	{
		CL = cache->GetByMark(mark , NULL );
		if(!CL)
		{
			result=SONORK_RESULT_NO_DATA;
			break;
		}

		result = GetMsg( CL->dat_index , &msg );
		if( result != SONORK_RESULT_OK )
			break;

		if( (msg.DataServiceType() != SONORK_SERVICE_TYPE_SONORK_FILE )
		||  !(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING ) )
		{
			result=SONORK_RESULT_INVALID_OPERATION;
			break;
		}

		if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_DELETED) )
		{
			result=SONORK_RESULT_NO_DATA;
			break;
		}

		if( IsMsgLocked( userId , *mark ) )
		{
			result=SONORK_RESULT_SERVICE_BUSY;
			break;
		}
		result = fileInfo.CODEC_Read(&msg.ExtData());
		if( result != SONORK_RESULT_OK )
			break;

		if( CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_PROCESSED )
		{
			if(CL->ExtIndex()!=SONORK_INVALID_INDEX)
				GetExtData(CL->ExtIndex(),&path,NULL);
			else
				path.Clear();
			break;
		}
		fileWin=new TSonorkFileTxGui(
			 UD->display_alias.CStr()
			,path.CStr()
			,*mark
			,fileInfo
			,0);
		if(!fileWin->Create())
		{
			delete fileWin;
			result=SONORK_RESULT_INTERNAL_ERROR;
		}
		else
		{
			result=SONORK_RESULT_OK_PENDING;
		}
		break;
	}
	if( msgWin == NULL )
		ReleaseTmpMsgCache(cache);
	TRACE_DEBUG("RecvFile______ 0");
	return result;
}

// ----------------------------------------------------------------------------

SONORK_LOGIN_MODE
	TSonorkWin32App::OnSonorkLoadLoginRequest(DWORD& login_flags)
{
	// The server always updates some fields of our user data uppon login
	// (like the region & language) unless the SONORK_LOGIN_CF_NO_USER_DATA_UPDATE
	// is set.
	// If we must refresh our own data (for example: after recovering a profile)
	// we set the flag so that we don't overwrite the existing data
	// before overwriting it.
	if( ProfileCtrlFlags().Test(SONORK_PCF_MUST_REFRESH_DATA) )
	{
		login_flags|=SONORK_LOGIN_RF_NO_USER_DATA_UPDATE;
	}
	return IntranetMode()
		?SONORK_LOGIN_MODE_INTRANET
		:SONORK_LOGIN_MODE_INTERNET;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SetBichoSequenceError(bool sound)
{
	win32.bicho->SetSequence(SONORK_SEQUENCE_ERROR);
	if(sound)
		AppSound( SONORK_APP_SOUND_ERROR );
}
void	TSonorkWin32App::SetBichoSequence(SONORK_SEQUENCE set_seq, bool restart_if_already_set)
{
	win32.bicho->SetSequence(set_seq,restart_if_already_set);
}
BOOL
 TSonorkWin32App::SetBichoSequenceIf(SONORK_SEQUENCE if_seq,SONORK_SEQUENCE set_seq)
{
	if(win32.bicho->GetSequence() == if_seq)
	{
		SetBichoSequence(set_seq);
		return true;
	}
	return false;
}
BOOL
 TSonorkWin32App::SetBichoSequenceIfNot(SONORK_SEQUENCE if_seq,SONORK_SEQUENCE set_seq)
{
	if(win32.bicho->GetSequence() == if_seq)
	{
		return false;
	}
	SetBichoSequence(set_seq);
	return true;
}


TSonorkServiceData*
 TSonorkWin32App::GetServiceDataFromCache(
				SONORK_SERVICE_TYPE     service_type
			, 	DWORD			service_instance
			,	DWORD			service_version)
{
	int 	   i;
	UINT	   age;
	TSonorkServiceData* SI;
	for(i=0;i<SONORK_APP_SERVICE_CACHE_SIZE;i++)
	{
		if(svc_cache.data[i].ServiceType() == SONORK_SERVICE_TYPE_NONE)
			continue;

		if( (svc_cache.data[i].ServiceInstance()   == service_instance
			 || service_instance == 0)
		&&  svc_cache.data[i].ServiceType() == service_type
		&&  svc_cache.data[i].ServiceVersionNumber() >= service_version )
		{
			age = svc_cache.clk[i].IntervalSecsCurrent();
			if( age > SONORK_APP_SERVICE_CACHE_SECS )
			{
				svc_cache.data[i].Clear();
			}
			else
			{
				SI = new TSonorkServiceData;
				SI->Set(svc_cache.data[i]);
				return SI;
			}
		}
	}
	return NULL;
}
void
 TSonorkWin32App::ReportServiceDataUsageToChache(
    const TSonorkServiceData*SI
  , bool success)
{
	int 	i,best_replacement_no  =-1;
	UINT    best_replacement_age, age;
	if(!SI)return;
	for(i=0;i<SONORK_APP_SERVICE_CACHE_SIZE;i++)
	{
		if( svc_cache.data[i].ServiceInstance() == SI->header.ServiceInstance()
		&&  svc_cache.data[i].ServiceType() 	== SI->header.ServiceType() )
		{
			if(!success)
				svc_cache.data[i].Clear();
			else
				svc_cache.clk[i].LoadCurrent();
			return;
		}

		if( svc_cache.data[i].ServiceType() == SONORK_SERVICE_TYPE_NONE )
		{
			best_replacement_no  = i;
			best_replacement_age = 99999;
		}
		else
		{
			age =  svc_cache.clk[i].IntervalSecsCurrent() ;
			if( best_replacement_no == -1 || age > best_replacement_age )
			{
				best_replacement_no = i;
				best_replacement_age=age;
			}
		}
	}
	if( success && best_replacement_no != -1)
	{
		svc_cache.clk[best_replacement_no].LoadCurrent();
		svc_cache.data[best_replacement_no].Set( *SI );
		// Discard Notes: We don't need them
		svc_cache.data[best_replacement_no].notes.Clear();
	}

}

// ----------------------------------------------------------------------------
// SONORK ENGINE EVENT HANDLERS
// ----------------------------------------------------------------------------


void	TSonorkWin32App::OnSonorkSysMsg( DWORD index, TSonorkSysMsg* sm )
{
	SONORK_TIME_RELATION rel;

	rel = sm->Time().RelDateTime( profile.ctrl_data.LastSysMsgTime() ) ;
	if( rel == SONORK_TIME_AFTER )
		profile.ctrl_data.wLastSysMsgTime().Set( sm->Time() );


	Set_UI_Event(SONORK_UI_EVENT_SYS_MSG
		, sm->text.ToCStr()
		, index==0	// Open SysConsole and do Sound on first message
		? ( SONORK_UI_EVENT_F_LOG
		  | SONORK_UI_EVENT_F_LOG_AS_UNREAD
		  | SONORK_UI_EVENT_F_LOG_AUTO_OPEN
		  | SONORK_UI_EVENT_F_SOUND
		  | SONORK_UI_EVENT_F_BICHO)
		: ( SONORK_UI_EVENT_F_LOG
		  | SONORK_UI_EVENT_F_LOG_AS_UNREAD)
		);
}

void	TSonorkWin32App::OnSonorkCtrlMsg(
	 const TSonorkUserLocus1*	sender
	,const TSonorkCtrlMsg*		msg
	,TSonorkDynData*		data)
{
	Service_ProcessCtrlMsg(
		  sender
		, SONORK_INVALID_LINK_ID
		, msg
		, data->Buffer()
		, data->DataSize());
}

void	TSonorkWin32App::OnSonorkDelGroup( SONORK_GROUP_TYPE group_type, DWORD group_no )
{
	if( group_type == SONORK_GROUP_TYPE_USER)
	{
		TSonorkMainGroupViewItem *eUG;
		eUG = win32.main_win->GetGroupViewItem( (SONORK_VIEW_GROUP)group_no , false) ;
		DelUserGroup( group_no );
		if( eUG != NULL )
		{
			win32.main_win->DelViewItem( eUG );
		}
	}
}
void
	TSonorkWin32App::OnSonorkAddGroup( const TSonorkGroup* UG )
{
	TSonorkMainGroupViewItem *eUG;
	DWORD			view_base_group;
	if( !(UG->GroupNo() > 0 && UG->GroupNo()<SONORK_MAX_USER_GROUPS) )return;

	switch( UG->GroupType() )
	{
		case SONORK_GROUP_TYPE_USER:
			view_base_group = SONORK_VIEW_USER_GROUP_FIRST;
			break;
		case SONORK_GROUP_TYPE_WAPP:
			view_base_group = SONORK_VIEW_WAPP_GROUP_FIRST;
			break;
		default:
			return;
	}
	eUG = win32.main_win->GetGroupViewItem(
			(SONORK_VIEW_GROUP)( view_base_group + UG->GroupNo() )
			, false) ;
	if(  eUG == NULL )
	{
		if( UG->GroupType() == SONORK_GROUP_TYPE_USER )
		{
			if( SaveUserGroup( UG ) != SONORK_RESULT_OK )
				return;
		}
		win32.main_win->AddViewItem
		(
			new TSonorkCustomGroupViewItem(view_base_group,UG)
		);
	}
}

void
 TSonorkWin32App::OnSonorkSetGroup( const TSonorkGroup* UG )
{
	TSonorkCustomGroupViewItem *eUG;
	if( UG->GroupType() != SONORK_GROUP_TYPE_USER)return;

	if(!(UG->GroupNo() > 0 && UG->GroupNo()<SONORK_MAX_USER_GROUPS))
		return;
	eUG = (TSonorkCustomGroupViewItem*)win32.main_win->GetGroupViewItem(
			(SONORK_VIEW_GROUP)UG->GroupNo() , false) ;
	if(  eUG != NULL )
	{
		if( eUG->IsCustomLocalUserGroup() )
		if( SaveUserGroup( UG ) == SONORK_RESULT_OK )
		{
			eUG->name.Set( UG->name.CStr() );
			win32.main_win->UpdateViewItemAttributes(eUG
				, 0, 0
				, SONORK_TREE_VIEW_UPDATE_F_FORCE_PAINT
				| SONORK_TREE_VIEW_UPDATE_F_SORT);
		}
	}
	else
		OnSonorkAddGroup(UG);

}
void
 TSonorkWin32App::OnSonorkAddWapp( const TSonorkWappData*WD )
{
	DWORD MenuCmd;
	if(WD->AppType()==SONORK_WAPP_TYPE_WAPP)
	{
		MenuCmd = gWappMenuCmd++;
	}
	else
	if(WD->AppType()==SONORK_WAPP_TYPE_MTPL)
	{
		MenuCmd = gMtplMenuCmd++;
	}
	else
		return;
	win32.main_win->AddViewItem(new TSonorkWebAppViewItem(MenuCmd,WD));
}
// -------------------------
// OnAddUser()
//  The App is adding an user to the user list.
//  This event is fired either when an authorization request
//  was accepted and the server is telling us that the user
//  belongs now to our user list or when we are executing
//  a full user list refresh.
//  We always set the SONORK_APP_F_REFRESHING_USER_LIST flag
//  in OnGlobalTask() when we refresh the list.
void
 TSonorkWin32App::OnSonorkAddUser(TSonorkExtUserData*UD, DWORD msg_sys_flags)
{
	// Create a new view item
	if(win32.main_win->GetExtUserViewItem(UD)==NULL)
	{
		// Add it to the user group
		win32.main_win->AddViewItem( new TSonorkExtUserViewItem(UD) );
	}

	// An user in our user list should never be in the authorizations group
	// because this last group holds those users that are waiting to enter
	// the user list.
	// This check applies specially for this sequence:
	//  1) We ask for an authorization to add another user,
	//     and we place the user in our authorization list; then..
	//  2) The user accepts the authorization, event that
	//     makes the server send us the USER_ADD message
	//     which fired this OnAddUser() handler; then..
	//  3) We add the user to the user list and remove
	//     it from the authorizations list.

	DelAuthReqViewItem( UD->userId );


	// If we're refreshing or self triggered, don't play the sound
	if( !TestRunFlag(SONORK_WAPP_RF_REFRESHING_LIST)
	&& !(msg_sys_flags&SONORK_MSG_SF_SELF_TRIGGERED))
	{
		win32.main_win->ExpandGroupViewItemLevels(
			SONORK_VIEW_SYS_GROUP_LOCAL_USERS
			,true);

		Set_UI_Event(SONORK_UI_EVENT_ADD_USER
			, csNULL
			, 0
			, UD);
	}

}

void	TSonorkWin32App::OnSonorkSetUser(TSonorkExtUserData*UD
			,const TSonorkAuth2*
			,const TSonorkDynString*)
{
	BroadcastAppEvent_SetUser( UD , SONORK_APP_EVENT_SET_USER_F_ALL );
}
// -------------------------
// OnSonorkUserSid()
// The application is telling us that a user in our user list
//  has changed its SID (Session Identifier) status.
// This event is fired when the other user connects/disconnects
//  or changes anything in its SID such as user mode, address,
//  region, language, etc.
// Also, we check that the status == READY before
//  starting to make sounds,
void	TSonorkWin32App::OnSonorkUserSid(TSonorkExtUserData*UD,const TSonorkUserLocus3&old_locus)
{
	char tmp[48];
	UD->addr.physAddr.GetStr(tmp);
	ProcessUserSidChange(UD ,&old_locus , CxStatus() >= SONORK_APP_CX_STATUS_READY_AND_GO );

}

// -------------------------
// OnDelUser()
//  The App is about to delete an user from the user list.
//  This event is fired either when an authorization request
//  was declined or when the other user deleted us from their
//  user list.
//  TSonorkExtApp will delete both the user and any pending
//  authorizations from the configuration
void
 TSonorkWin32App::OnSonorkDelUser(const TSonorkId& user_id , TSonorkExtUserData* deletedUD)
{

	// From the authorizations group
	DelAuthReqViewItem( user_id );
	// Delete the view items
	// From the users group
	win32.main_win->DelViewItem( win32.main_win->GetExtUserViewItem(user_id) );


	// Tell all windows that this user is no longer valid
	BroadcastAppEvent(SONORK_APP_EVENT_DEL_USER
		,SONORK_APP_EM_USER_LIST_AWARE
		,0
		,(void*)&user_id);

	if(deletedUD!=NULL)
	{
		SetUnreadMsgCount(deletedUD,0);
		Set_UI_Event(SONORK_UI_EVENT_DEL_USER
			, csNULL
			, 0
			, deletedUD);
	}

}

void
 TSonorkWin32App::OnSonorkUserAuth( TSonorkAuthReqData* RD )
{
	TSonorkAuthReqViewItem*	view_item;
	bool			new_item;
//	TSonorkError		tERR;


	AppSound( SONORK_APP_SOUND_NOTICE );

	// Get the corresponding view item
	view_item = win32.main_win->GetAuthReqViewItem( RD->user_data.userId );
	if( view_item == NULL )
	{
		new_item=true;
		// Does not exist yet.
		SONORK_MEM_NEW( view_item = new TSonorkAuthReqViewItem );
	}
	else
	{
		new_item=false;
	}

	// <sent_by_us> is true if the user that requested
	// the authorization is US
	view_item->userId.Set( RD->user_data.userId );
	view_item->alias.Set( RD->user_data.alias );
	view_item->sent_by_us = (RD->RequestorId() == ProfileUserId() );
	view_item->user_info_flags.Set( RD->user_data.InfoFlags() );
	if( new_item )
	{
		win32.main_win->AddViewItem(view_item);
		if( !(view_item->sent_by_us ) )
		{
			IncCounter(SONORK_APP_COUNTER_PENDING_AUTHS , 1);
		}
	}
	else
	{
		win32.main_win->RepaintViewItem( view_item );
	}

	if( ProfileCtrlFlags().Test(SONORK_PCF_AUTO_AUTH)
		&& !view_item->sent_by_us) // Auto accept
	{

#define A_size	64
		UINT 			P_size;
		TSonorkDataPacket*	P;
		TSonorkAuth2		a_auth;
		a_auth.tag = 0;
		a_auth.flags.Clear();
		a_auth.flags.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
		a_auth.pin = RD->header.auth.pin;
		//auth.pin should be the same as the received pin
		P=SONORK_AllocDataPacket(A_size);
		P_size = P->E_AknAuth_R(A_size, RD->RequestorId() , a_auth);
		StartSonorkRequest(P,P_size,NULL,NULL);
#undef A_size


	}
	else
	{
		win32.main_win->ExpandGroupViewItemLevels(
			SONORK_VIEW_SYS_GROUP_AUTHS
			,true);
		win32.main_win->EnsureVisibleGroupViewItem(SONORK_VIEW_SYS_GROUP_AUTHS , true);
	}
}

void	TSonorkWin32App::DelAuthReqViewItem(const TSonorkId& user_id )
{
	TSonorkAuthReqViewItem *VI;
	VI = win32.main_win->GetAuthReqViewItem(user_id);
	if( VI != NULL )
	{
		if( !VI->sent_by_us )
		{
			IncCounter(SONORK_APP_COUNTER_PENDING_AUTHS , -1);
		}
		win32.main_win->DelViewItem( VI );
	}
}

// -------------------------
// OnGlobalTask()
// The application is telling us that a global task is starting/ending.
//  Global tasks are special tasks that must be serialized (i.e. only
//   one of them may be active at a time).
// These tasks include refreshing the user list, changing the online status, etc.
// Note that <pERR> is valid ONLY if <task_end> is true.

void
 TSonorkWin32App::OnSonorkGlobalTask(
	 SONORK_FUNCTION 	function
	,const TSonorkError*	taskERR)
{
	bool update_ui=true;;
	// On Global task:
	// We check the current CxStatus() because some global functions
	// like SET_SID and REFRESH_USER_LIST are executed during the
	// connection phase (after CONNECTED and before READY state)
	// We don't update interface functions nor statuses while
	//  in connection phase because that is handled by SetCxStatus()

//	TRACE_DEBUG("GLOBAL: %s %s", SONORK_FunctionName(function), task_end?"ENDS":"BEGINS");
	if( taskERR == NULL )
	{
		switch( function )
		{
			case SONORK_FUNCTION_USER_LIST:
			{
				// Set the REFRESHING_USER_LIST so we don't
				//  sort the view every time AddUser() is called;
				//  we will sort it when the refresh ends.
				SetRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);

				win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_LOCAL_USERS);


				if(CxStatus() >= SONORK_APP_CX_STATUS_READY)
				{
					update_ui=false;
					Set_UI_Event_TaskStart(GLS_MS_RFRESHING
					,SONORK_UI_EVENT_F_BICHO);
				}
			}
			break;

			case SONORK_FUNCTION_WAPP_LIST:
				update_ui=false;
				SetRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
				ClearWappList();
			break;

			case SONORK_FUNCTION_GET_SYS_MSGS:
				update_ui=false;
			break;


		}
	}
	else
	{
		switch( function )
		{
			case SONORK_FUNCTION_SET_SID:
				BroadcastAppEvent(SONORK_APP_EVENT_SID_CHANGED
					, SONORK_APP_EM_PROFILE_AWARE
					, ProfileSidFlags().SidMode()
					, NULL);
			break;

			case SONORK_FUNCTION_USER_LIST:
				ClearRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
				win32.main_win->AfterMassUpdate(SONORK_VIEW_TAB_1);
				ResetMainView();
				if(CxStatus() >= SONORK_APP_CX_STATUS_READY)
				{
					update_ui=false;
					Set_UI_Event_TaskEnd(taskERR
					, LangString(GLS_MS_ULIST_RFSHD)
					, SONORK_UI_EVENT_F_BICHO
					| SONORK_UI_EVENT_F_SOUND
					);
				}
			break;

			case SONORK_FUNCTION_WAPP_LIST:

				ClearRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
				win32.main_win->AfterMassUpdate(SONORK_VIEW_TAB_2);
				win32.main_win->ExpandGroupViewItemLevels(
					SONORK_VIEW_SYS_GROUP_WEB_APPS
					,true
					,3);

				win32.main_win->LoadWappTemplatesMenuFromViewItems();
				BroadcastAppEvent(SONORK_APP_EVENT_USER_MTPL_CHANGE
					,0,0,NULL);
			break;

			case SONORK_FUNCTION_GET_SYS_MSGS:
				update_ui=false;
			break;
		}
	}

	if(CxStatus() < SONORK_APP_CX_STATUS_READY)
	{
		SONORK_APP_CX_STATUS new_status;
		if( taskERR==NULL )return;
		if( taskERR->Result() == SONORK_RESULT_OK )
		{
			if(CxStatus() == SONORK_APP_CX_STATUS_REFRESHING_LIST)
				new_status = SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS;
			else
			if(CxStatus() == SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS)
				new_status = SONORK_APP_CX_STATUS_REGISTERING_SID;
			else
			if(CxStatus() == SONORK_APP_CX_STATUS_REGISTERING_SID )
				new_status = SONORK_APP_CX_STATUS_READY;
			else
			{
				TRACE_DEBUG("INVALID STATUS: %02u",CxStatus());
				new_status = SONORK_APP_CX_STATUS_IDLE;
			}
		}
		else
		{
			new_status = SONORK_APP_CX_STATUS_IDLE;
		}
		SetCxStatus( new_status , *taskERR );
	}
	else
	{
		if( update_ui )
		{

			if(taskERR != NULL)
			{
				Set_UI_Event_TaskEnd(taskERR
				,	NULL	// default string
				,	 SONORK_UI_EVENT_F_NO_SOUND
					|SONORK_UI_EVENT_F_BICHO
					|SONORK_UI_EVENT_F_CLEAR_IF_NO_ERROR
					|SONORK_UI_EVENT_F_NO_LOG
				);
			}
			else
			{
				Set_UI_Event_TaskStart(GLS_MS_PWAIT
					,SONORK_UI_EVENT_F_BICHO
					|SONORK_UI_EVENT_F_NO_LOG);
			}
//FIX THIS!!
//			SetMainHint_None();
		}
		win32.main_win->UpdateInterfaceFunctions();
	}
}
void
 TSonorkWin32App::ClearWappList()
{
	int i;
	gWappMenuCmd=SONORK_APP_CM_WAPP_BASE;
	gMtplMenuCmd=SONORK_APP_CM_MTPL_BASE;
	for(i=0; GetMenuItemCount(menus.user_mtpl) && i<500;i++ )
		DeleteMenu(menus.user_mtpl,0,MF_BYPOSITION);
	win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_WEB_APPS);
	BroadcastAppEvent(SONORK_APP_EVENT_USER_MTPL_CHANGE,0,0,NULL);
}

void
	TSonorkWin32App::LoadMainView()
{
	TSonorkListIterator	I;
	TSonorkViewItemPtrs	VP;

	ClearMainView();

	// Load user groups
	{
		TSonorkGroup *UG;
		TSonorkGroupQueue	Q;
		LoadUserGroups(Q);
		while((UG=Q.RemoveFirst()) != NULL )
		{
			if(UG->GroupNo() > 0 && UG->GroupNo()<SONORK_MAX_USER_GROUPS)
				win32.main_win->AddViewItem(
					new TSonorkCustomGroupViewItem(SONORK_VIEW_USER_GROUP_FIRST,UG)
				);
			SONORK_MEM_DELETE(UG);
		}

	}

	// Load user list
	{
		TSonorkExtUserData *UD;
		UserList().BeginEnum(I);
		while( (UD=UserList().EnumNext(I)) != NULL)
		{
			// Add user view item
			VP.user = new TSonorkExtUserViewItem(UD);
			win32.main_win->AddViewItem(VP.user);
		// Increment unread message counter
			counter[SONORK_APP_COUNTER_UNREAD_MSGS]+=(int)UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT);
		}
		UserList().EndEnum(I);
	}

	// Load authorizations list
	{
		TSonorkAuthReqData		*RD;
		TSonorkAuthReqDataQueue	RD_list;


		LoadAuthReqList(RD_list, true, true);
		while( (RD=RD_list.RemoveFirst()) != NULL )
		{
			SONORK_MEM_NEW( VP.auth = new TSonorkAuthReqViewItem );
			VP.auth->userId.Set( RD->user_data.userId );
			VP.auth->alias.Set( RD->user_data.alias );
			VP.auth->sent_by_us = (RD->RequestorId() == ProfileUserId() );
			if( !VP.auth->sent_by_us )
				counter[SONORK_APP_COUNTER_PENDING_AUTHS]++;
			VP.auth->user_info_flags.Set( RD->user_data.InfoFlags() );
			win32.main_win->AddViewItem(VP.auth);
			SONORK_MEM_DELETE( RD );
		}
	}
	RecalcCounter(SONORK_APP_COUNTER_EVENTS);

}
void
 TSonorkWin32App::ClearMainView()
{
	win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_LOCAL_USERS);
	win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_REMOTE_USERS);
	win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_AUTHS);
	memset(counter,0,sizeof(counter));
	RecalcCounter(SONORK_APP_COUNTER_EVENTS);
}

void
 TSonorkWin32App::OnSonorkUserProfileOpen(bool open)
{
	TSonorkError 		tERR;
	TSonorkTempBuffer 	buffer(SONORK_MAX_PATH);
	char 			userIdStr[24];
	char			*path_suffix;
	bool			db_reset;
//	sonork_printf("OnSonorkUserProfileOpen(%u) starts",open);
	if(TestRunFlag(SONORK_WAPP_RF_NO_PROFILE_EVENTS))
		return;
	BroadcastAppEvent(SONORK_APP_EVENT_SID_CHANGED
		, SONORK_APP_EM_PROFILE_AWARE
		, ProfileSidFlags().SidMode()
		, NULL);
	ProfileUserId().GetStr(userIdStr);
	if( open )
	{
		TSonorkShortString  lang_name;

		if(!profile.ctrl_data.LastSysMsgTime().IsValid()
		|| profile.ctrl_data.LastSysMsgTime().Year() > CurrentTime().Year()+1 )
			profile.ctrl_data.wLastSysMsgTime().Clear();

		if(TestRunFlag(SONORK_WAPP_RF_APP_INITIALIZED))
		{
		// Test SONORK_WAPP_RF_APP_INITIALIZED: If it is NOT set, the
		// profile has been opened from within InitApp() which invokes
		// RebuildMenus() after finishing initialization, so we don't
		// call it here.
			RebuildTrayMenu();
		}

		if(!ProfileCtrlFlags().Test(SONORK_PCF_INITIALIZED))
		{
			ProfileCtrlFlags().Set(SONORK_PCF_MUST_REFRESH_LIST);
			ProfileCtrlFlags().Set(SONORK_PCF_MUST_REFRESH_DATA);
		}

		RequestCheckAlarms( SONORK_APP_CHECK_ALARM_VERY_SOON_SECS );


		if(ReadProfileItem(szLang, &lang_name)==SONORK_RESULT_OK)
			LangLoad(tERR , lang_name.CStr() , true );

		ReadProfileItem("Skin", &TSonorkSkinCodecAtom(&sonork_skin));

		// Update user language to current setting
		wProfileRegion().SetLanguage(lang.table.LangCode());


		SetRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
		LoadMainView();
		ClearRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
		win32.main_win->AfterMassUpdate(SONORK_VIEW_TAB_ALL);
		
		ResetMainView();


		// Make sure temporal folder exists
		GetDirPath(buffer.CStr(),SONORK_APP_DIR_TEMP,NULL);
		TSonorkWin32App::CreateDirIfNotExists( buffer.CStr() );

		// Make sure profile's directories exists
		GetDirPath(buffer.CStr(),SONORK_APP_DIR_DATA,userIdStr);
		path_suffix = buffer.CStr() + strlen(buffer.CStr());
		*path_suffix++='\\';
		*path_suffix=0;

		TSonorkWin32App::CreateDirIfNotExists( buffer.CStr() );

		strcpy(path_suffix , szPrivateDirClip);
		TSonorkWin32App::CreateDirIfNotExists( buffer.CStr() );

		strcpy(path_suffix , szPrivateDirUser);
		TSonorkWin32App::CreateDirIfNotExists( buffer.CStr() );

		// Open message db
		wsprintf(path_suffix,"%s-msg(%s)",userIdStr,szSonorkAppMode);



		for(db_reset=false;;db_reset=true)
		{
			if(db.msg.Open( ProfileUserId(), buffer.CStr() , db_reset )==SONORK_RESULT_OK)
				break;
			if(db_reset)
				break;
		}
		if( db_reset )
			db.msg.SetValue(SONORK_APP_ATOM_DB_VALUE_VERSION,SONORK_APP_CURRENT_ATOM_DB_VERSION);
		db.msg.RecomputeItems();

		wsprintf(path_suffix,"%s-ext(%s)",userIdStr,szSonorkAppMode);

		for(db_reset=false;;db_reset=true)
		{
			if(db.ext.Open( ProfileUserId(), buffer.CStr() , db_reset )==SONORK_RESULT_OK)
				break;
			if(db_reset)
				break;
		}
		if( db_reset )
			db.ext.SetValue(SONORK_APP_ATOM_DB_VALUE_VERSION,SONORK_APP_CURRENT_ATOM_DB_VERSION);
		db.ext.RecomputeItems();

		// Update the global message counter

		// Tell all windows we've open the profile and ready to use it
		BroadcastAppEvent(SONORK_APP_EVENT_OPEN_PROFILE
			,SONORK_APP_EM_PROFILE_AWARE
			,true
			,NULL);

		// If APP centeral control management(CCM) flags says the messages
		// should popup then we force the flag on the current user.
		if( TestCfgFlag(SONORK_WAPP_CF_POPUP_MSG_WIN ) )
			ProfileCtrlFlags().Set(SONORK_PCF_POPUP_MSG_WIN);

		ReadProfileItem(szAuthExcludeList
		,&TSonorkSimpleDataListAtom(&auth_exclude_list,SONORK_ATOM_SONORK_ID,true));

		if( ProfileCtrlValue(SONORK_PCV_MAX_APP_VERSION) < SONORK_APP_VERSION_NUMBER )
		{
			ProfileCtrlValue(SONORK_PCV_MAX_APP_VERSION)=SONORK_APP_VERSION_NUMBER;
			if(ProfileCtrlValue(SONORK_PCV_MAX_APP_VERSION) < 0x010500 )
			{
				ProfileCtrlValue(SONORK_PCV_SLIDER_POS) = SONORK_SLIDER_WIN_BOTTOM_RIGHT;
				ProfileCtrlValue(SONORK_PCV_AUTO_AWAY_MINS)=5;
			}
		}
	}
	else
	{
		sonork_skin.SetDefaultColors(false,true);
		auth_exclude_list.Clear();
		win32.run_flags&=~SONORK_WAPP_RF_CX_PENDING;
		BroadcastAppEvent(SONORK_APP_EVENT_OPEN_PROFILE
			,SONORK_APP_EM_PROFILE_AWARE
			,false
			,NULL);

		SetRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
		ClearMainView();
		ClearRunFlag(SONORK_WAPP_RF_REFRESHING_LIST);
		win32.main_win->AfterMassUpdate(SONORK_VIEW_TAB_ALL);

		db.msg.Close();
		db.ext.Close();
//----------------------------------------------------------------------------
// OLD IPC:
//----------------------------------------------------------------------------
//		win32.ipc_server->UserId().Clear();
		console.sys->Clear( true );
		db.sys.Clear();
		AppRunValue(SONORK_ARV_FIRST_UNREAD_SYS_MSG) = 0;
	}
//	sonork_printf("OnSonorkUserProfileOpen(%u) ends",open);

}
void
 TSonorkWin32App::ResetMainView()
{
	win32.main_win->ExpandGroupViewItemLevels(SONORK_VIEW_SYS_GROUP_LOCAL_USERS
		, true);
	win32.main_win->ExpandGroupViewItemLevels(SONORK_VIEW_SYS_GROUP_AUTHS
		, true);
	win32.main_win->ExpandGroupViewItemLevels(SONORK_VIEW_SYS_GROUP_REMOTE_USERS
		 , true);
	win32.main_win->EnsureVisibleGroupViewItem(SONORK_VIEW_SYS_GROUP_LOCAL_USERS
		, true);
}
void
 TSonorkWin32App::OnSonorkStatusChange(SONORK_NETIO_STATUS status,const TSonorkError*pERR,DWORD flags)
{
	switch(status)
	{
		case SONORK_NETIO_STATUS_CONNECTED:
			OnSonorkConnect(pERR,flags);
			break;
		case SONORK_NETIO_STATUS_DISCONNECTED:
			OnSonorkDisconnect(pERR,flags);
			break;
		case SONORK_NETIO_STATUS_CONNECTING:
			// We ignore CONNECTING status because this status is always set
			// when Gu::Connect() returns OK; result that we handle in
			// OnCxNetNameResolve, which is the place where we invoked
			// Gu::Connect().
			break;
	}
}

void
 TSonorkWin32App::OnSonorkConnect(const TSonorkError*
	, DWORD login_akn_flags)
{
	TSonorkError ERR;
	char	tmp[128];

	for(int i=0;i<SONORK_APP_SERVICE_CACHE_SIZE;i++)
		svc_cache.data[i].Clear();

	if( login_akn_flags & SONORK_ULF_REFRESH_USER_LIST )
	{
		// The server is telling us the the user list should be refreshed
		ProfileCtrlFlags().Set(SONORK_PCF_MUST_REFRESH_LIST);
	}


	// We set the status to READY only after executing the SET_SID
	// for the first time. See OnGlobalTask() for more info.


	// We activate UTS only if it is local enabled and if the
	// server has not requested us to enable it
	if( !( TestCfgFlag( SONORK_WAPP_CF_NO_UTS ) || ( login_akn_flags & SONORK_ULF_DISABLE_UTS ) ) )
	if( win32.uts_server == NULL )
	{
		TSonorkUTSDescriptor descriptor;
		UINT			link_flags;
		SONORK_MEM_NEW(
			win32.uts_server = new TSonorkUTS(SONORK_APP_MAX_UTS_CONNECTIONS)
		);
		descriptor.locus.userId.Set(ProfileUserId());
		descriptor.locus.sid.Set(wProfileUser().addr.sid);
		descriptor.serviceId	= SONORK_SERVICE_ID_SONORK;
		descriptor.instance	= 0;
		descriptor.flags        = SONORK_UTS_KERNEL_APP_FLAGS;
		link_flags=UTS_MayActAsServer()?0:SONORK_UTS_LINK_F_NO_LISTEN;
		if(win32.uts_server->Startup(ERR
				,descriptor
				,SonorkUtsEventHandler
				,this
				,link_flags
				,INADDR_ANY
				,win32.server_profile.nat.range.v[0]
				,win32.server_profile.nat.range.v[1]
				) == SONORK_INVALID_LINK_ID)
		{
			wsprintf(tmp,"Start Uts failed (%s/%d)"
				,ERR.ResultName()
				,ERR.Code());
			Set_UI_Event(
				  SONORK_UI_EVENT_DEBUG
				, tmp
				, SONORK_UI_EVENT_F_LOG );
			SONORK_MEM_DELETE( win32.uts_server );
			win32.uts_server=NULL;
			wProfileUser().addr.physAddr.Inet1()->sin_port=0;
		}
		else
		{
			char tmp[64];
			win32.uts_server->SocksInfo().Set(sonork.SocksInfo());
			wProfileUser().addr.physAddr.SetInet1Port(
				win32.uts_server->MainLink()->PhysAddr().GetInet1Port());
			ProfileUser().addr.physAddr.GetStr(tmp);
			sonork_printf("UTS_ADDR: %s",tmp);
		}

	}
	assert( CxStatus() == SONORK_APP_CX_STATUS_LINKING);
	SetCxStatus( SONORK_APP_CX_STATUS_LINKED,ERR);
}
void
 TSonorkWin32App::OnSonorkDisconnect(const TSonorkError*pERR, DWORD)
{
	SetCxStatus( SONORK_APP_CX_STATUS_IDLE , *pERR );
}

// ==================================================
// 	ENGINE WRAPPERS
// ==================================================
// ---------------
// RefreshUserList()
//  Asks the engine to synchronize the local user list with
//  with the remote list kept in the server.
SONORK_RESULT
	TSonorkWin32App::RefreshUserList(TSonorkError* pERR)
{
	TSonorkError tmpERR;
	if(pERR == NULL )
		pERR = &tmpERR;
	if( CxStatus() < SONORK_APP_CX_STATUS_LINKED )
		pERR->SetSys(SONORK_RESULT_NOT_READY,GSS_NOTINIT,SONORK_MODULE_LINE);
	else
	if(GlobalTaskPending())
		pERR->SetSys(SONORK_RESULT_SERVICE_BUSY,GSS_APPBUSY,SONORK_MODULE_LINE);
	else
	{
		sonork.RefreshUserList(*pERR);
		if( pERR->Result() == SONORK_RESULT_OK )
			ProfileCtrlFlags().Clear(SONORK_PCF_MUST_REFRESH_LIST);
	}
	return pERR->Result();
}

// ---------------
// SetSidFlags()
//  Asks the engine to synchronize the local sid in ProfileSidFlags()
//  with the remote sid kept in the server and by the engine
//  in ProfileUser().Address() (also syncs ProfileUser().Region())
SONORK_RESULT
	TSonorkWin32App::SetSidFlags( TSonorkSidFlags& sid_flags )
{
	TSonorkError ERR;
	wProfileSidFlags().Set( sid_flags );
	return DoSyncSid( ERR );
}
SONORK_RESULT
	TSonorkWin32App::SetSidMode( SONORK_SID_MODE sid_mode )
{
	TSonorkError ERR;
	wProfileSidFlags().SetSidMode( sid_mode );
	return DoSyncSid( ERR );
}
SONORK_RESULT
	TSonorkWin32App::CancelAutoAwaySidMode()
{

	if( CxReady() && ProfileSidFlags().SidMode() == SONORK_SID_MODE_AWAY_AUTO )
		return SetSidMode( win32.saved_auto_away_sid_mode );
	return SONORK_RESULT_OK;
}
SONORK_RESULT
TSonorkWin32App::DoSyncSid(TSonorkError&ERR)
{
	TSonorkPhysAddr 	sidPhysAddr;
	SONORK_SID_MODE		sid_mode;
	TSonorkSidFlags&	sidFlags=wProfileSidFlags();
	TSonorkDynString	sid_msg;
	char			tmp[32];

	if(!IsProfileOpen())
	{
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_NOTINIT,0);
		return ERR.Result();
	}

	sidFlags.DisableUts();
	sidFlags.ClearPublic();
	sid_mode=sidFlags.SidMode();
	if(!IS_VALID_SID_MODE(sid_mode))
		sidFlags.SetSidMode(sid_mode=SONORK_SID_MODE_ONLINE);

	while( !sidFlags.IsPrivate()
	     && sid_mode != SONORK_SID_MODE_INVISIBLE
	     && sid_mode != SONORK_SID_MODE_INVISIBLE_02
	     && sid_mode != SONORK_SID_MODE_INVISIBLE_03)
	{
		if( ProfileCtrlFlags().Test( SONORK_PCF_NOT_PUBLIC_SID ) )
			break;
		if( ProfileCtrlFlags().Test( SONORK_PCF_PUBLIC_SID_WHEN_FRIENDLY ) )
			if(sid_mode!=SONORK_SID_MODE_FRIENDLY)
				break;
		sidFlags.SetPublic();
		if( !ProfileCtrlFlags().Test( SONORK_PCF_NO_INVITATIONS) )
			sidFlags.EnableTracker();
		break;
	}
	if( CxStatus() < SONORK_APP_CX_STATUS_REGISTERING_SID )
	{
		SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_CTRL_DATA );
		BroadcastAppEvent(SONORK_APP_EVENT_SID_CHANGED
			, SONORK_APP_EM_PROFILE_AWARE
			, ProfileSidFlags().SidMode()
			, NULL);
		ERR.SetOk();
	}
	else
	{
		if( sonork.Busy() )
		{
			ERR.SetSys(SONORK_RESULT_SERVICE_BUSY,GSS_ENGBUSY,0);
		}
		else
		{
			if( sonork.UsingSocks() || ProfileCtrlFlags().Test(SONORK_PCF_USING_PROXY) )
				sidFlags.SetUsingSocks();
			else
				sidFlags.ClearUsingSocks();
			if( win32.uts_server == NULL )
			{
				sidPhysAddr.Clear();
			}
			else
			{
				sidFlags.EnableUtsClient();
				sidPhysAddr.Set(ProfileUser().addr.physAddr);
				if(	!sidFlags.UsingSocks()
					&& sidPhysAddr.IsValid(SONORK_PHYS_ADDR_TCP_1) )
				{
					sidFlags.EnableUtsServer();
				}
			}
			sid_mode = NormalizeSidMode( sidFlags.SidMode() );
			sprintf(tmp,"SidMsg%02u",sid_mode);
			ReadProfileItem(tmp,&sid_msg);
			TRACE_DEBUG("DoSyncSid(%u)"
				,sidFlags.SidMode());

			sonork.SyncSid(ERR
				,sidFlags
				,sidPhysAddr
				,ProfileRegion()
				,sid_msg);
		}
	}
	return ERR.Result();
}
// ==================================================
// 		CONNECTION ESTABLISHEMENT
// ==================================================

// ----------------------------------
// Connect()
// Initiates connect sequence, the connection
//    status is obtained with CxStatus() and it starts by reading
//    configuration and resolving names before connecting the GU engine.
// The result of the resolution is handled in OnCxNetNameResolve()
//  and only when the last name is resolved, Gu.Connect() is invoked
//  from within OnCxNetNameResolve()
// On non-fatal errors, the status is reset to ACTIVE so that
//  the application will retry to connect.
SONORK_RESULT
	TSonorkWin32App::Connect( TSonorkError& ERR )
{
	SONORK_APP_CX_STATUS    new_status;
	
	if( CxStatus() > SONORK_APP_CX_STATUS_IDLE )
	{
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_ENGBUSY,SONORK_MODULE_LINE);
		return ERR.Result();
	}

	win32.run_flags&=~(SONORK_WAPP_RF_NO_AUTO_LOGIN
						|SONORK_WAPP_RF_CX_PENDING
						|SONORK_WAPP_RF_CONNECT_NOW);

	if( !IsProfileOpen() )
	{
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_NOTINIT,SONORK_MODULE_LINE);
		return ERR.Result();
	}


	ERR.SetResult(LoadServerProfile(ProfileServerProfile().CStr()
		,win32.server_profile
		,false
		,NULL));
	if(ERR.Result() != SONORK_RESULT_OK )
	{
		ERR.SetSys(SONORK_RESULT_CONFIGURATION_ERROR,GSS_BADSVRCFG,SONORK_MODULE_LINE);
		PostAppCommand(SONORK_APP_COMMAND_LOGIN_FATAL_ERROR,ERR.Result());
		Set_UI_Event(SONORK_UI_EVENT_SONORK_DISCONNECT
				, GLS_MS_DCXTED
				, 0);
		return ERR.Result();
//		new_status = SONORK_APP_CX_STATUS_IDLE;
	}
	if( win32.server_profile.socks.IsEnabled() )
	{
//			SysLogV(SONORK_UI_EVENT_GENERIC,0,"SOCKS: %s:%u"
//				,win32.server_profile.socks.HostName()
//				,win32.server_profile.socks.TcpPort());
		AsyncResolve( ERR
			, AppWinHandle()
			, SONORK_PHYS_ADDR_TCP_1
			, win32.server_profile.socks.HostName()
			, win32.server_profile.socks.TcpPort()
			, RESOLVE_PHASE_SOCKS);
	}
	else
	{
		sonork.wSocksInfo().Clear();
		AsyncResolve( ERR, AppWinHandle()
			, win32.server_profile.sonork.AddrType()
			, win32.server_profile.sonork.HostName()
			, win32.server_profile.sonork.DefaultPort()
			, RESOLVE_PHASE_SONORK);
	}
	if(ERR.Result() != SONORK_RESULT_OK)
		new_status=SONORK_APP_CX_STATUS_IDLE;
	else
	{
		new_status=SONORK_APP_CX_STATUS_NAME_RESOLVE;
		SetRunFlag( SONORK_WAPP_RF_CX_PENDING );
	}
	SetCxStatus(new_status,ERR);
	return ERR.Result();
}

// ----------------------------------
// OnCxNetNameResolve
// Handles the names resolution results of the sequence initiated in Connect()

void
	TSonorkWin32App::OnCxNetNameResolve(TSonorkAppNetResolveResult*RR)
{
	char 		str[256];
	char 		addr_str[48];
	TSonorkError	tERR;
	if( RR->ERR.Result() == SONORK_RESULT_OK )
	{
		switch(RR->tag)
		{
			case RESOLVE_PHASE_SOCKS:
				sonork.wSocksInfo().physAddr.Set(*RR->physAddr);
				sonork.wSocksInfo().version = 4;
				sonork.SocksInfo().physAddr.GetStr(addr_str);
				AsyncResolve( tERR
					, AppWinHandle()
					, win32.server_profile.sonork.AddrType()
					, win32.server_profile.sonork.HostName()
					, win32.server_profile.sonork.DefaultPort()
					, RESOLVE_PHASE_SONORK);
				if( tERR.Result() == SONORK_RESULT_OK)
					return;	// not a break, must return

				break;

			case RESOLVE_PHASE_SONORK:
				RR->physAddr->GetStr(addr_str);

				sonork.Connect(tERR , *RR->physAddr );
				if(tERR.Result() == SONORK_RESULT_OK )
				{
					LangSprintf(str,GLS_MS_CXTINGHOST,addr_str);
					SetCxStatus(SONORK_APP_CX_STATUS_LINKING,tERR);
					Set_UI_Event_TaskStart( str , SONORK_UI_EVENT_F_NO_BICHO);
//					//SetMainHint( str , 0 , SONORK_APP_HINT_INFO );
				}
				else
				{
					LangSprintf(str,GLS_MS_CXHOSTFAIL,addr_str);
					SetCxStatus(SONORK_APP_CX_STATUS_IDLE,tERR);

					Set_UI_Event_TaskEnd(  &tERR
					, str
					, SONORK_UI_EVENT_F_NO_BICHO
					| SONORK_UI_EVENT_F_NO_SOUND
					| SONORK_UI_EVENT_F_NO_LOG
					);
					//SetMainHint( str , 0 , SONORK_APP_HINT_ERROR);
				}
				return;		// not a break, must return

			default:
				break;
		}
	}
	SetCxStatus(SONORK_APP_CX_STATUS_IDLE,RR->ERR);
	LangSprintf(str,GLS_MS_RSLVHOSTFAIL,RR->host->CStr());
	Set_UI_Event_TaskEnd(&RR->ERR
		,str
		, SONORK_UI_EVENT_F_NO_BICHO
		| SONORK_UI_EVENT_F_NO_SOUND
		| SONORK_UI_EVENT_F_NO_LOG);
	//SetMainHint(str,0,SONORK_APP_HINT_ERROR);
}

void
	TSonorkWin32App::ProcessUserProfileData(TSonorkDataPacket*P, UINT P_size)
{
	TSonorkUserDataNotes	UDN;
	if( P->Function() != SONORK_FUNCTION_GET_USER_DATA )return;
	if( P->SubFunction() != 0)return;
	if( P->D_GetUserData_A(P_size, UDN) )
	{
		BroadcastAppEvent(SONORK_APP_EVENT_SET_PROFILE
			,SONORK_APP_EM_PROFILE_AWARE
			,0
			,&UDN);
	}
}
// ----------------------------------
// SetCxStatus
//  Sets the connection sequence phase

void
	TSonorkWin32App::SetCxStatus(
			SONORK_APP_CX_STATUS new_status
			, const TSonorkError&  cxERR)
{
	TSonorkError		tERR;
	SONORK_APP_CX_STATUS 	old_status;
	TSonorkListIterator	I;
	TSonorkExtUserData*	UD;
	TSonorkUserLocus3	locus;
	TSonorkAppTask*		TASK;
	int			i;
	DWORD			event_flags;
	GLS_INDEX		event_msg;

	old_status = win32.cx_status;
	TRACE_DEBUG("_SetCxStatus( %02u -> %02u )" , old_status , new_status );

	if( old_status == new_status )
		return;
	if( old_status > new_status )
	{
		TRACE_DEBUG("ERR: %u '%s' %u L:%u"
			, cxERR.result
			, cxERR.text.CStr()
			, cxERR.code
			, cxERR.local);
	}
	win32.cx_status=new_status;
	task_timer.cx_status_msecs=0;

	UpdateTrayIcon();
//----------------------------------------------------------------------------
// OLD IPC:
//----------------------------------------------------------------------------
//	win32.ipc_server->SetCxStatus( new_status );
	if( new_status == SONORK_APP_CX_STATUS_IDLE )
	{

		event_flags=0;
		win32.run_flags&=~(SONORK_WAPP_RF_REFRESHING_LIST
					|SONORK_WAPP_RF_CX_PENDING
					|SONORK_WAPP_RF_TRACKER_CACHE_LOADED);


		// SetErrorSequence, make error sound if not a SELF disconnection
		// and was already connected
		SetBichoSequenceError(
		!TestRunFlag(SONORK_WAPP_RF_SELF_DISCONNECT)
		&& old_status >= SONORK_APP_CX_STATUS_CONNECTING_LAST);

		tERR.SetSys(SONORK_RESULT_NETWORK_ERROR,GSS_NETCXLOST,SONORK_MODULE_LINE);

		while( (TASK = win32.task_queue.RemoveFirst()) != NULL )
			OnAppTaskResult(TASK,&tERR,false);

		Service_OnSonorkDisconnect();
		
		if( win32.uts_server != NULL )
		{
			win32.uts_server->Shutdown();
			SONORK_MEM_DELETE( win32.uts_server );
			win32.uts_server=NULL;
		}
		for(i=0;i<SONORK_APP_SERVICE_CACHE_SIZE;i++)
			svc_cache.data[i].Clear();

		// We won't received SID notifications for each user but
		// we know they should all be disconnected because we're not
		// connected!; update the main view to denote this situation.
		UserList().BeginEnum(I);
		while( (UD=UserList().EnumNext(I)) != NULL)
		{
			if(UD->UtsLinkId() != 0)
				UTS_SetLink(UD->userId,0,UD,false);
			if(UD->addr.sidFlags.SidMode() != SONORK_SID_MODE_DISCONNECTED)
			{
				UD->GetLocus3(&locus);
				UD->addr.Clear();
				ProcessUserSidChange(UD,&locus,false);
			}
		}
		UserList().EndEnum(I);
		win32.main_win->SortViewGroup(SONORK_VIEW_SYS_GROUP_LOCAL_USERS , true);
		if( old_status > SONORK_APP_CX_STATUS_CONNECTING_LAST )
		{
			event_flags|=SONORK_UI_EVENT_F_LOG;
		}

		if(old_status >= SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS )
			ClearWappList();
		if(TestRunFlag(SONORK_WAPP_RF_SELF_DISCONNECT))
		{
			event_msg  =GLS_MS_DCXTED;
			event_flags|=SONORK_UI_EVENT_F_WARNING;
			//SetMainHint( GLS_MS_DCXTED , 3000 , SONORK_APP_HINT_WARNING );
		}
		else
		{
			if(cxERR.Result() == SONORK_RESULT_ACCESS_DENIED
			|| cxERR.Result() == SONORK_RESULT_INVALID_MODE
			|| cxERR.Result() == SONORK_RESULT_INVALID_SERVER
			|| cxERR.Result() == SONORK_RESULT_INVALID_VERSION
			|| cxERR.Result() == SONORK_RESULT_CODEC_ERROR)
			{
				// Access denied: useless to keep trying so
				// don't keep the status active and tell the
				// user his/her data is wrong
				event_msg  =GLS_MS_DCXTED;
				//SetMainHint(GLS_MS_DCXTED , 3000 , SONORK_APP_HINT_ERROR);
				PostAppCommand(SONORK_APP_COMMAND_LOGIN_FATAL_ERROR,cxERR.Result());
			}
			else
			{
				// Keep trying
				event_msg  = old_status > SONORK_APP_CX_STATUS_CONNECTING_LAST
					?GLS_MS_CXLOST_R
					:GLS_MS_CXFAIL_R;
				event_flags|=SONORK_UI_EVENT_F_WARNING;
				/*
				SetMainHint(old_status > SONORK_APP_CX_STATUS_CONNECTING_LAST
					?GLS_MS_CXLOST_R
					:GLS_MS_CXFAIL_R, 0
					, SONORK_APP_HINT_WARNING);
				*/
				SetRunFlag(SONORK_WAPP_RF_CX_PENDING);
			}
		}
		Set_UI_Event(SONORK_UI_EVENT_SONORK_DISCONNECT
				, event_msg
				, event_flags);

	}
	else
	if( new_status < SONORK_APP_CX_STATUS_LINKED )
	{
		SetBichoSequence(SONORK_SEQUENCE_WORK);
	}
	else
	if( new_status == SONORK_APP_CX_STATUS_LINKED )
	{
		Set_UI_Event( SONORK_UI_EVENT_SONORK_CONNECT
			, GLS_MS_CXTED
			, SONORK_UI_EVENT_F_LOG
			| SONORK_UI_EVENT_F_SOUND
			| SONORK_UI_EVENT_F_BICHO);
	}
	else
	if(new_status == SONORK_APP_CX_STATUS_READY)
	{
		win32.run_flags&=~(SONORK_WAPP_RF_SETUP_INIT|SONORK_WAPP_RF_CX_PENDING);
	}
	else
	if(new_status == SONORK_APP_CX_STATUS_READY_AND_GO)
	{
		// Wait 5 seconds before checking sys messages
		task_timer.sys_msg_msecs = SYS_MSG_CHECK_MSECS - 5000;
		win32.run_flags|=SONORK_WAPP_RF_TRACKER_SYNCH_PENDING;
		ProfileCtrlFlags().Set(SONORK_PCF_INITIALIZED);
		if(ProfileCtrlFlags().Test(SONORK_PCF_MUST_REFRESH_DATA))
			win32.main_win->Task_RefreshUserProfile(
				SONORK_TASKWIN_F_NO_ERROR_BOX
				);
		RequestCheckEmailAccounts( SONORK_APP_CHECK_EMAIL_VERY_SOON_SECS );
	}

	BroadcastAppEvent(SONORK_APP_EVENT_CX_STATUS
		,SONORK_APP_EM_CX_STATUS_AWARE
		,old_status
		,NULL);

	if( new_status >= SONORK_APP_CX_STATUS_LINKED && new_status < SONORK_APP_CX_STATUS_READY )
	{
		if( new_status == SONORK_APP_CX_STATUS_LINKED )
		{
			// Check if there is configuration to refresh, otherwise
			// skip to the SONORK_APP_CX_STATUS_REGISTERING_SID phase
			if(ProfileCtrlFlags().Test(SONORK_PCF_MUST_REFRESH_LIST)
				|| TestRunFlag(SONORK_WAPP_RF_SETUP_INIT))
			{
				RefreshUserList(&tERR);
				new_status = SONORK_APP_CX_STATUS_REFRESHING_LIST;
			}
			else
			{
				tERR.SetOk();
				new_status = SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS;
			}
		}

		if(new_status == SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS )
		{
			sonork.RefreshWappList(tERR);
		}
		else
		if( new_status == SONORK_APP_CX_STATUS_REGISTERING_SID )
		{
			DoSyncSid( tERR );
		}
		if(tERR.Result() != SONORK_RESULT_OK)
		{
			// Failed to start the task.. which means
			// we will never reach READY mode: Disconnect
			// and set the CX_PENDING flag so we retry later

			SetCxStatus( SONORK_APP_CX_STATUS_IDLE , tERR );
			SetRunFlag( SONORK_WAPP_RF_CX_PENDING );
		}
		else
			win32.cx_status=new_status;
	}
}

// ----------------------------------
// Disconnect()
// Disconnects/cancels connect sequence;
// Sets the status to IDLE
void		TSonorkWin32App::Disconnect()
{
	if( CxActiveOrPending() )
	{
		// Set the SONORK_APP_F_NO_CX_STATUS_UPDATE so that
		// the OnSonorkDisconnect does not update the user interface:
		//  we will do that
		if(CxStatus()==SONORK_APP_CX_STATUS_NAME_RESOLVE)
		{
			CancelAsyncResolve(win32.work_win);
		}

		if(win32.uts_server!=NULL)
			win32.uts_server->Shutdown();


		win32.run_flags&=~SONORK_WAPP_RF_CX_PENDING;
		win32.run_flags|=SONORK_WAPP_RF_SELF_DISCONNECT ;
		if( sonork.Status() != SONORK_NETIO_STATUS_DISCONNECTED)
			sonork.Disconnect();
		else
			SetCxStatus( SONORK_APP_CX_STATUS_IDLE , TSonorkErrorOk() );
		win32.run_flags&=~SONORK_WAPP_RF_SELF_DISCONNECT;

	}
}



// ==================================================
// 		NAME RESOLUTION
// ==================================================
// AsyncResolve() does an asynchrounous name resolution by invoking
//   the socks function WSAAsyncGetHostByName() with the handle of the
//   main application window and the WM_SONORK_ASYNC_NET_NAME_RESOLVE message.
//  It stores the resolution information and the handle of the window
//    that requested the resolution in the application's own resolve queue.
//  When the resolution is over (the application window receives
//   the WM_SONORK_ASYNC_NET_NAME_RESOLVE from the socks system), the
//   application's WmGuAsyncNetNameResolve() looks up the stored
//   information from the resolve queue and sends a WM_SONORK_APP_MESSAGE
//   to the window that requested the resolution.
//  The WM_SONORK_APP_MESSAGE will have:
//     wParam = SONORK_APP_MSG_NET_NAME_RESOLUTION
//     lParam = pointer to a TSonorkAppWin32ResolveResult structure,
//	   the TSonorkAppWin32ResolveResult contains:
//          TSonorkError 	ERR  		:	Error code
//          TSonorkPhysAddr*phys_addr   :   result (only if ERR.Result()==SONORK_RESULT_OK)
//			DWORD		tag			:   The tag supplied when calling Resolve()
//  The result structure is owned and deleted by the application;
//    the receiving window should copy it if needed.
// --------------------------------------------------

HANDLE TSonorkWin32App::AsyncResolve(TSonorkError&ERR,HWND hwnd
			,SONORK_PHYS_ADDR_TYPE	r_type
			,const char *		r_host
			,DWORD 			r_port
			,DWORD 			r_tag)
{
	TSonorkAppNetResolveData	*RD;

	SONORK_MEM_NEW(RD=new TSonorkAppNetResolveData);

	if( *r_host >='0' && *r_host<='9')
	{
		// Dotted address: we don't need to use a DNS server;
		// set the <handle> to a dummy value and <pseudo> to true
		RD->handle	= (HANDLE)GetTickCount();
		RD->pseudo  = true;
		RD->physAddr.SetInet1(
				r_type
			, 	r_host
			,(WORD)r_port);
	}
	else
	{
		RD->pseudo  = false;
		RD->handle	= WSAAsyncGetHostByName(
					    AppWinHandle()
					,   WM_SONORK_ASYNC_NET_NAME_RESOLVE
					,   r_host
					,   (char*)RD->hostent
					,   MAXGETHOSTSTRUCT);

		if( RD->handle == NULL )
		{
			ERR.Set(SONORK_RESULT_NETWORK_ERROR
			,"Cannot resolve name"
			,WSAGetLastError()
			,true);
			SONORK_MEM_DELETE(RD);
			return NULL;
		}
	}
	RD->owner_hwnd	=hwnd;
	RD->host.Set(r_host);
	RD->tag			=r_tag;
	RD->physAddr.SetType((SONORK_PHYS_ADDR_TYPE)r_type);
	RD->physAddr.SetInet1Port((WORD)r_port);
	win32.resolve_queue.Add(RD);
	ERR.SetOk();
	if( hwnd == AppWinHandle() )
	{
		TSonorkTempBuffer tmp(256);
		LangSprintf(tmp.CStr()
			,GLS_MS_RSLVHOST
			,r_host);
		Set_UI_Event_TaskStart( tmp.CStr()
			, SONORK_UI_EVENT_F_NO_BICHO
			| SONORK_UI_EVENT_F_NO_SOUND
			| SONORK_UI_EVENT_F_NO_LOG);
	}
	if( RD->pseudo == true )
	{
		// Post a fake resolve result to the work window
		::PostMessage(win32.work_win,WM_SONORK_ASYNC_NET_NAME_RESOLVE,(WPARAM)RD->handle,0);
	}
	return RD->handle;
}

void
 TSonorkWin32App::WmSonorkAsyncNetNameResolve(WPARAM wParam ,LPARAM lParam)
{
	WORD	    				err_code;
	TSonorkListIterator 			I;
	TSonorkAppNetResolveData		*RD;
	TSonorkAppNetResolveResult		RR;

	// Lookup the resolve information for the handle
	win32.resolve_queue.BeginEnum(I);
	while( (RD=win32.resolve_queue.EnumNext(I)) !=NULL )
		if( RD->handle ==  (HANDLE)wParam )
			break;
	win32.resolve_queue.EndEnum(I);

	if( RD == NULL )
		return;	// Not found: Should never happen

	win32.resolve_queue.Remove(RD);
	
	err_code=WSAGETASYNCERROR(lParam);
	if(err_code == 0)
	{
		if(!RD->pseudo)
		{
			HOSTENT*  hostent;

			hostent = (HOSTENT*)RD->hostent;
			RD->physAddr.SetInet1Addr(*((unsigned long *)(*hostent->h_addr_list)));
		}
		RR.ERR.SetOk();
	}
	else
	{
		RD->physAddr.Clear();
		RR.ERR.Set(SONORK_RESULT_NETWORK_ERROR
			,"Cannot resolve name"
			,err_code
			,true);
	}

	// Send the result to the owner window
	//  Note that the owner window could be the application window itself (us)
	if( IsWindow(RD->owner_hwnd) )
	{
		RR.physAddr     =&RD->physAddr;
		RR.host		=&RD->host;
		RR.tag 		= RD->tag;
		::SendMessage(RD->owner_hwnd,WM_SONORK_POKE
			,SONORK_WIN_POKE_NET_RESOLVE_RESULT
			,(LPARAM)&RR);
	}
	SONORK_MEM_DELETE( RD );
}
void	TSonorkWin32App::CancelAsyncResolve(HWND hwnd)
{
	TSonorkListIterator 			I;
	TSonorkAppNetResolveData 		*RD;
	win32.resolve_queue.BeginEnum(I);
	while( (RD=win32.resolve_queue.EnumNext(I)) !=NULL )
		if( RD->owner_hwnd ==  hwnd )
			break;
	win32.resolve_queue.EndEnum(I);
	if( RD == NULL )return;
	if( !RD->pseudo )
		WSACancelAsyncRequest( RD->handle );
	win32.resolve_queue.Remove(RD);
	SONORK_MEM_FREE( RD );
}

// ==================================================
// APPLICATION INVISIBLE WINDOW HANDLERS
// ==================================================





void
 TSonorkWin32App::WmSonorkPoke(WPARAM wParam ,LPARAM lParam)
{
	switch(wParam)
	{
		case SONORK_WIN_POKE_NET_RESOLVE_RESULT:
			OnCxNetNameResolve((TSonorkAppNetResolveResult*)lParam);
			break;
			
		case POKE_SHOW_SETUP_DIALOG:
			if( !CxActiveOrReady() )
				SONORK_ExecuteSetupDialog(win32.main_win);
			break;
	}
}
void
 TSonorkWin32App::SwitchMode(const char*switch_flags, UINT flags)
{
	TSonorkTempBuffer path(SONORK_MAX_PATH);
	char	params[192];
	int		l;

	wsprintf(params,"%s(%s)"
		,ConfigFileName()
		,IntranetMode()?"ex":"in"); // NB!: Extension reversed


	if( !FindParallelInstance( params ) )
	{
		if( flags & SONORK_APP_SWITCH_MODE_F_QUERY)
		{
			LangSprintf(params,GLS_OP_CHGMDX,IntranetMode()?szInternet:szIntranet);
			l = win32.main_win->MessageBox(GLS_QU_OPENNW
					,params
					,MB_ICONQUESTION|MB_YESNOCANCEL|MB_DEFBUTTON2);
			if(l==IDCANCEL)return;
			if(l==IDYES)flags|=SONORK_APP_SWITCH_MODE_F_NEWWIN;
		}
		wsprintf(path.CStr()
			,"%s\\%s"
			,win32.root_dir.CStr()
			,szSrkClientImage);
		wsprintf(params,"-%s -CfgFile%s %s%s"
			,IntranetMode()?szInternet:szIntranet
			,ConfigFileName()
			,switch_flags?switch_flags:""
			,TestRunFlag(SONORK_WAPP_RF_SETUP_INIT)?" -SetupInit":"");

		ShellExecute(win32.main_win->Handle()
			,NULL
			,path.CStr()
			,params
			,win32.root_dir.CStr()
			,SW_SHOWDEFAULT);
	}
	else
		flags|=SONORK_APP_SWITCH_MODE_F_NEWWIN;
		
	if( !(flags&SONORK_APP_SWITCH_MODE_F_NEWWIN) )
		win32.main_win->PostPoke(SONORK_WIN_POKE_DESTROY,0);
	else
		win32.main_win->ShowWindow(SW_MINIMIZE);
}
void	TSonorkWin32App::FocusNextWindow( TSonorkWin* caller )
{
	TSonorkListIterator	I;
	SONORK_WIN_TYPE		win_type;
	TSonorkWin			*eWin,*pWin,*fWin;
	BOOL				caller_found;
	win_type = caller->WinType();
	if(win_type!=SONORK_WIN_TYPE_SONORK_MSG
	&& win_type!=SONORK_WIN_TYPE_SONORK_CHAT)
		return;

	fWin = pWin = NULL;
	caller_found = false;
	win32.win_list.BeginEnum(I);
	while( (eWin=win32.win_list.EnumNext(I)) !=NULL )
	{
		win_type = eWin->WinType();
		if(win_type != SONORK_WIN_TYPE_SONORK_CHAT
		&& win_type != SONORK_WIN_TYPE_SONORK_MSG)
			continue;
		if( eWin == caller )
		{
			caller_found=true;
		}
		else
		if( caller_found )
		{
			fWin = eWin;
			break;
		}
		else
		if(pWin==NULL)
		{
			pWin = eWin;
		}
	}
	win32.win_list.EndEnum(I);
	if( fWin == NULL )
	{
		if(pWin==NULL)
			return;
		fWin=pWin;
	}
	PostAppCommand( SONORK_APP_COMMAND_FOREGROUND_HWND , (LPARAM)fWin->Handle() );
}

void
 TSonorkWin32App::WmSonorkAppCommand(WPARAM wParam ,LPARAM lParam)
{
	TSonorkWin *W;
	GLS_INDEX		gls;
	SONORK_SYS_STRING	gss;
	UINT			aux;
	switch(wParam)
	{
	
		case SONORK_APP_COMMAND_FOREGROUND_HWND:
			if( gAppForegroundHwnd!=NULL && gAppForegroundHwnd!=(HWND)-1)
			{
				if( IsWindow(gAppForegroundHwnd) )
				{
					if( IsWindowEnabled(gAppForegroundHwnd) )
					{
						SetForegroundWindow ( gAppForegroundHwnd );
					}
				}
			}
			gAppForegroundHwnd=NULL;
			break;

		case SONORK_APP_COMMAND_FOCUS_HWND:
			if( gAppFocusHwnd!=NULL && gAppFocusHwnd!=(HWND)-1 )
			if( IsWindow(gAppFocusHwnd) )
			if( IsWindowEnabled(gAppFocusHwnd)
				&& IsWindowVisible(gAppFocusHwnd)
				&& !IsIconic(gAppFocusHwnd))
			{
				SetFocus(gAppFocusHwnd);
			}
			gAppFocusHwnd=NULL;
			break;

		case SONORK_APP_COMMAND_LOGIN_FATAL_ERROR:

			SetRunFlag( SONORK_WAPP_RF_NO_AUTO_LOGIN );
				aux = MB_OK;
			if( lParam == SONORK_RESULT_ACCESS_DENIED )
			{
				gls = GLS_ERR_BADLOGIN;
			}
			else
			if( lParam == SONORK_RESULT_INVALID_SERVER )
			{
				gls = GLS_ERR_BADSVR;
			}
			else
			if( lParam == SONORK_RESULT_INVALID_MODE )
			{
				gls = IntranetMode()?GLS_ERR_CXINET:GLS_ERR_CXINTRA;
				aux = MB_YESNO;
			}
			else
			if( lParam == SONORK_RESULT_CONFIGURATION_ERROR )
			{
				gls = GLS_NULL;
				gss = GSS_BADSVRCFG;
			}
			else
				gls = GLS_ERR_CXVER;

			aux = win32.main_win->MessageBox(
				 gls!=GLS_NULL?LangString(gls):SysString(gss)
				,szSONORK,aux|MB_ICONSTOP);
			if(lParam==SONORK_RESULT_INVALID_MODE)
			{
				if( aux == IDYES )
				{
					SwitchMode(NULL, 0);
				}
			}
			else
				ShowLoginDialog();
			break;

		case SONORK_APP_COMMAND_SWITCH_MODE:
			SwitchMode((const char*)lParam, 0);
			break;
			
		case SONORK_APP_COMMAND_WIN_CREATED:
			W=(TSonorkWin*)lParam;
			if( W->GetEventMask() != 0 )
			{
				win32.win_list.Add(W);
			}
			W->_OnAfterCreate();
			break;

		case SONORK_APP_COMMAND_WIN_DESTROYED:

			W=(TSonorkWin*)lParam;
			W->OnAfterDestroy();
			win32.win_list.Remove(W);
			if(!W->TestWinSysFlag(SONORK_WIN_SF_NO_DEL_ON_DESTROY))
				delete W;
			break;

		case SONORK_APP_COMMAND_HALTED:
			PostQuitMessage(lParam);
			break;

			
		case SONORK_APP_COMMAND_TERMINATE:
			if(win32.main_win && !TestRunFlag(SONORK_WAPP_RF_APP_TERMINATING))
			{
                                SetRunFlag(SONORK_WAPP_RF_APP_TERMINATING);
				win32.main_win->PostPoke(SONORK_WIN_POKE_DESTROY,0);
			}
			break;


		case SONORK_APP_COMMAND_REFRESH:
			if(win32.main_win)
			{
				if(win32.main_win->IsIconic() || !win32.main_win->IsVisible())
					win32.main_win->ShowWindow(SW_RESTORE);
				::SetForegroundWindow ( win32.main_win->Handle() );
			}
			TrayMessage(NIM_DELETE, NULL, NULL);
			TrayMessage(NIM_ADD, GetTrayIcon(), szSONORK );

			break;
			
		case SONORK_APP_COMMAND_CLEAR_IPC_ENTRY:
			ClearIpcEntry(lParam);
			break;

		case SONORK_APP_COMMAND_RELOAD_EXT_APPS:
			LoadExtApps();
			LoadExtAppMenu();
			break;

		case SONORK_APP_COMMAND_PSEUDO_TASK_RESULT:
			OnAppTaskResult((TSonorkAppTask*)lParam
				, &TSonorkErrorOk()
				, true );
			break;

		default:
			break;
	}

}

LRESULT CALLBACK
	TSonorkWin32App::WinProc(HWND hwnd,UINT uMsg ,WPARAM wParam ,LPARAM lParam)
{
	TSonorkWin32App *_this;
	// The GWL_USERDATA is set after Create() to the App's pointer
	_this=(TSonorkWin32App*)::GetWindowLong(hwnd,GWL_USERDATA);
	if(_this != NULL)
	{
		switch(uMsg)
		{
			case WM_TIMER:
				// The application window creates only one timer
				_this->WmTimer();
				return 0;

			case WM_SONORK_COMMAND:
				_this->WmSonorkAppCommand(wParam,lParam);
				return 0;

			case WM_SONORK_POKE:
				_this->WmSonorkPoke(wParam,lParam);
				return 0;

			case WM_SONORK_ASYNC_NET_NAME_RESOLVE:
				_this->WmSonorkAsyncNetNameResolve(wParam,lParam);
				return 0;

			case WM_SONORK_TRAY_MESSAGE:
				_this->WmSonorkTrayMessage(wParam,lParam);
				return 0;

			case WM_SONORK_SERVICE:
				_this->WmSonorkService(wParam,lParam);
				return 0;
				
			case WM_SONORK_IPC:
				return _this->WmSonorkIpc(wParam,lParam);



//----------------------------------------------------------------------------
// OLD IPC:
//----------------------------------------------------------------------------
//			return _this->win32.ipc_server->IPC_Handler(wParam,lParam);

			case WM_QUERYENDSESSION:
				_this->win32.run_flags&=~SONORK_WAPP_RF_UNINSTALL;
				if( _this->win32.main_win != NULL )
					_this->win32.main_win->Destroy();
				break;
				
			case WM_ENDSESSION:
				_this->Exit();
				break;

			case WM_PARENTNOTIFY:
				break;

			case WM_SETTINGCHANGE:
				_this->WmSettingChange(wParam ,lParam);
				break;
			case WM_COPYDATA:
				return _this->WmCopyData((HWND)wParam,(const COPYDATASTRUCT*)lParam);
		}
	}
	return	DefWindowProc( hwnd, uMsg, wParam, lParam );
}

BOOL
 TSonorkWin32App::WmCopyData(HWND , const COPYDATASTRUCT*)
{
	return false;
}

void
 TSonorkWin32App::WmSettingChange(WPARAM wParam ,LPARAM )
{
	TSonorkDesktopSize old_size;
	if(wParam & SPI_GETWORKAREA )
	{
		memcpy(&old_size,&desktop_size,sizeof(TSonorkDesktopSize));
		UpdateDesktopSize();
		BroadcastAppEvent(SONORK_APP_EVENT_DESKTOP_SIZE_CHANGE,0,0,&old_size);
	}
}

// ==================================================
// 			MAIN LOOP
// ==================================================
void
 TSonorkWin32App::Init_MainWin()
{
	TSonorkWinStartInfo SI;
	bool	pin_window;
	if(ConfigFile().ReadRaw(AppKey(),"SI",&SI,sizeof(SI))!=SONORK_RESULT_OK)
	{
		SONORK_ZeroMem(&SI,sizeof(SI));
		pin_window=true;
	}
	else
	{
		if(	  SI.pos.pt.x<0
			|| SI.pos.pt.x>DesktopLimit().x-MAIN_WIN_MIN_WIDTH
			|| SI.pos.pt.y<0
			|| SI.pos.pt.y>DesktopLimit().y-MAIN_WIN_MIN_HEIGHT
			|| SI.pos.sz.cx<MAIN_WIN_MIN_WIDTH
			|| SI.pos.sz.cy<MAIN_WIN_MIN_HEIGHT)
			pin_window=true;
		else
			pin_window=false;
	}

	win32.main_win = new TSonorkMainWin;

	win32.main_win->Create();
//	win32.task_win = new TSonorkAsyncTaskWin(win32.main_win);
//	win32.task_win->Create();


	win32.main_win->TransferStartInfo( &SI , true );
	if(pin_window)
		win32.main_win->PinWindow( false , MAIN_WIN_PINNED_WIDTH);
	win32.info_win 	= win32.main_win->InfoWin();
	win32.bicho	= win32.main_win->Bicho();
}

void
 TSonorkWin32App::Run(const char*cmd_line)
{
	if( Init(cmd_line) )
	{
		SetRunFlag(SONORK_WAPP_RF_APP_INITIALIZED);
		Main();
	}
	Exit();
}


void
 TSonorkWin32App::Main()
{
	int 			rv;
	MSG    			msg;
	assert( win32.main_win && win32.bicho && win32.info_win );

	{
		if(	TestCfgFlag(SONORK_WAPP_CF_NO_CLOSE) )
			::EnableMenuItem(MainMenu()
				,CM_QUIT
				,MF_BYCOMMAND|MF_GRAYED);
		if(	TestCfgFlag(SONORK_WAPP_CF_NO_NET_CONFIG) )
			::EnableMenuItem(MainMenu()
				,CM_CFGNETWORK,MF_BYCOMMAND|MF_GRAYED);
		win32.main_win->UpdateWindow();
		TrayMessage(NIM_ADD, GetTrayIcon(),szSONORK);
	}

	win32.info_win->ClearEvent( true );

	SonorkAppTimerClock.LoadCurrent();
	SetTimer( win32.work_win
		, SONORK_APP_TIMER_ID
		, SONORK_APP_TIMER_MSECS
		, NULL);

	// If we've been instructed from the command line that
	// we should show the login dialog (SONORK_WAPP_RF_SHOW_LOGIN)
	// or that we've been invoked after the installation (SONORK_WAPP_RF_SETUP_INIT)
	// show the login dialog
	if( RunFlags()&(SONORK_WAPP_RF_SHOW_LOGIN|SONORK_WAPP_RF_SETUP_INIT) )
	{
		ClearRunFlag(SONORK_WAPP_RF_SHOW_LOGIN);
		ShowLoginDialog();
	}

	for(;;)
	{
		rv=::GetMessage(&msg,NULL,0,0);
		if(rv<=0)
		{
			if(rv==0)break;
			// else rv is negative: Error condition
			continue;
		}
		if(msg.message >=WM_KEYFIRST && msg.message<=WM_KEYLAST)
		{
			if(sonork_active_win!=NULL)
				if(sonork_active_win->PreTranslateMessage(&msg))
					continue;
		}
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	KillTimer( win32.work_win, SONORK_APP_TIMER_ID );
	//if( win32.exec_mode > EXEC_MODE_SETUP )
	{
		TrayMessage(NIM_DELETE, NULL, NULL);
		delete win32.main_win;
	}
}

// ==================================================
// 			INITIALIZATION & CLEANUP
// ==================================================
TSonorkWin32App::TSonorkWin32App()
{
	LUAT.hInstance		= NULL;
	LUAT.ptr		= NULL;
	win32.work_win		= NULL;
	win32.main_win		= NULL;
	win32.slide_win		= NULL;
	win32.zip		= NULL;

//----------------------------------------------------------------------------
// OLD IPC:
//----------------------------------------------------------------------------
//	win32.ipc_server	= NULL;
	win32.uts_server	= NULL;
	win32.cx_status		= SONORK_APP_CX_STATUS_IDLE;
	win32.serial_no		=
	win32.run_flags		= 0;
	SONORK_ZeroMem(&menus,sizeof(menus));
	console.tmp.refCount	= 0;
	console.tmp.ptr =
	console.sys = NULL;

	SONORK_ZeroMem(&counter,sizeof(counter));

	SONORK_ZeroMem(win32.sys_dialog,sizeof(win32.sys_dialog));
}
TSonorkWin32App::~TSonorkWin32App()
{
}


bool
 TSonorkWin32App::Init_User(TSonorkWin32AppInitData*ID)
{
	SONORK_RESULT	result;

	if( ID->user_id.IsZero() )
	{
		result=SONORK_RESULT_NO_DATA;
	}
	else
	{
		result=OpenProfile(ID->user_id,ID->password);
	}

	if(result!=SONORK_RESULT_OK)
	{
		ClearCfgFlag( SONORK_WAPP_CF_NO_CLOSE );
		return false;
	}
	return true;
}




// ==================================================
// User interface & Window handling
// ==================================================

void	TSonorkWin32App::BroadcastAppEvent(SONORK_APP_EVENT event
		,UINT mask
		,UINT param
		,void* data)
{
	TSonorkListIterator I;
	TSonorkWin *		win;
	union
	{
		void			*ptr;
		TSonorkUserDataNotes	*UDN;
	}D;

	if( event == SONORK_APP_EVENT_SET_PROFILE )
	{
		D.ptr=data;
		if( D.UDN->user_data.userId == ProfileUserId()
		&&  D.UDN->user_data.GetUserInfoLevel() >= ProfileUser().GetUserInfoLevel() )
		{
			ProfileCtrlFlags().Clear(SONORK_PCF_MUST_REFRESH_DATA);
			wProfileUser().Set( D.UDN->user_data );
			SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_ALL );
			WriteProfileItem("Notes",&D.UDN->notes);
			RebuildTrayMenu();
		}
	}

	win32.main_win->DispatchAppEvent(event,param,data);
	if(mask==0)mask=0xffffffff;
	
	win32.win_list.BeginEnum(I);
	while( (win=win32.win_list.EnumNext(I)) !=NULL )
	{
		if( !(win->GetEventMask()&mask) )
			continue;
		win->DispatchAppEvent(event,param,data);
	}
	win32.win_list.EndEnum(I);

	BroadcastAppEventToIpc(event,mask,param,data);

}
void	TSonorkWin32App::BroadcastAppEvent_SetUser(TSonorkExtUserData*UD,UINT set_user_flags)
{
	TSonorkAppEventSetUser	E;
	E.user_id.Set(UD->userId);
	E.user_data = UD;
	BroadcastAppEvent( SONORK_APP_EVENT_SET_USER
		,SONORK_APP_EM_USER_LIST_AWARE
		,set_user_flags
		,&E);
}

void	TSonorkWin32App::ShowLoginDialog()
{
	assert( win32.main_win != NULL );
	win32.main_win->PostPoke(SONORK_MAINWIN_POKE_SHOW_LOGIN_DIALOG,0);
}

void	TSonorkWin32App::ShowSetupDialog()
{
	::PostMessage(win32.work_win,WM_SONORK_POKE,POKE_SHOW_SETUP_DIALOG,0);
}


void
	TSonorkWin32App::UpdateDesktopSize()
{
	RECT rect;
	::SystemParametersInfo(SPI_GETWORKAREA, 0 , &rect , 0);
	desktop_size.origin.x	= rect.left;
	desktop_size.origin.y	= rect.top;
	desktop_size.limit.x	= rect.right;
	desktop_size.limit.y	= rect.bottom;
	desktop_size.size.cx	= rect.right - rect.left;
	desktop_size.size.cy	= rect.bottom - rect.top;
}



// -------------------
// PostMessage()
// Used to send a command to the application it is
// processed by WmSonorkAppCommand
void
 TSonorkWin32App::PostAppCommand(SONORK_APP_COMMAND cmd, LPARAM lParam)
{
	// Notes on FOCUS_HWND and FOREGROUND_HWND:
	// We store the HWND being focused/brought to foreground
	// and clear it when the command is processed in WmSonorkAppCommand
	// to avoid going back and forth between the requesting windows.
	if( cmd == SONORK_APP_COMMAND_FOCUS_HWND )
	{
		if( gAppFocusHwnd != NULL )
		{
			// Command has been posted already, just
			// store new value and don't repost-command
			// When WmSonorkAppCommand processes it, the
			// new HWND will be processed.
			gAppFocusHwnd=(HWND)lParam;
			return;
		}
		gAppFocusHwnd=(HWND)lParam;
	}
	else
	if( cmd == SONORK_APP_COMMAND_FOREGROUND_HWND )
	{
		// We test for Iconic and Visibility here and not in
		// WmSonorkAppCommand (where the command is actually processed)
		// because new requests update previous ones until the
		// processing actually takes place, so that only one window
		// if finally brought to foreground. However, the calling
		// window must become visible, regardless of which one finally
		// ends on top.
		if( !IsWindowVisible((HWND)lParam) )
		{
			::ShowWindow((HWND)lParam,SW_SHOW);
		}
		else
		if( !IsIconic((HWND)lParam) )
		{
			::ShowWindow((HWND)lParam,SW_RESTORE);
		}
		if( gAppForegroundHwnd != NULL )
		{
			// Command has been posted already, just
			// store new value and don't repost-command
			// When WmSonorkAppCommand processes it, the
			// new HWND will be processed.
			gAppForegroundHwnd=(HWND)lParam;
			return;
		}
		gAppForegroundHwnd=(HWND)lParam;

	}
	::PostMessage(win32.work_win, WM_SONORK_COMMAND, cmd, lParam);
}
void
 TSonorkWin32App::CancelPostAppCommand(SONORK_APP_COMMAND cmd, LPARAM lParam)
{
	if( cmd == SONORK_APP_COMMAND_FOCUS_HWND )
	{
		if( (LPARAM)gAppFocusHwnd == lParam )
		{
			// Don't set to NULL so that PostAppCommand
			// does not re-post. WmSonorkAppCommand()
			// simply ignores (HWND)-1
			gAppFocusHwnd=(HWND)-1;
		}
		return;
	}
	else
	if( cmd == SONORK_APP_COMMAND_FOREGROUND_HWND )
	{
		if( (LPARAM)gAppForegroundHwnd == lParam )
		{
			// Don't set to NULL so that PostAppCommand
			// does not re-post. WmSonorkAppCommand()
			// simply ignores (HWND)-1
			gAppForegroundHwnd=(HWND)-1;
		}
		return;
	}
}





SKIN_ICON
 TSonorkWin32App::GetUserModeViewInfo(SONORK_SID_MODE mode
	,const TSonorkUserInfoFlags& 	flags
	,SONORK_C_CSTR*			dStr) const
{
	GLS_INDEX gls_index;
	SKIN_ICON icon;
	icon = GetUserModeViewInfo(mode,flags,&gls_index);
	if(dStr)*dStr=LangString(gls_index);
	return icon;
}



/*void	TSonorkWin32App::PopulateComboBoxUserModes(HWND combo_box,bool include_visibility)
{
	ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_ONLINE);
	ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_BUSY);
	ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_AT_WORK);
	ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_AWAY);
	ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_FRIENDLY);
	if( !TestCfgFlag(SONORK_WAPP_CF_NO_INVISIBLE) )
		ComboBox_AddString(combo_box,(char*)SONORK_SID_MODE_FULL_INVISIBLE);
	if(include_visibility && !TestCfgFlag(SONORK_WAPP_CF_NO_VISIBILITY))
		ComboBox_AddString(combo_box,(char*)0);
}

void	TSonorkWin32App::SetComboBoxUserMode(HWND combo_box)
{
	int index;
	bool profile_open=IsProfileOpen();
	if( profile_open )
	{
		SONORK_SID_MODE sid_mode;
		sid_mode = ProfileSidFlags().SidMode();
		index = ComboBox_FindString(combo_box,-1,sid_mode);
	}
	else
	{
		index =-1;
	}
	ComboBox_SetIndex(combo_box,index);
	::EnableWindow(combo_box, profile_open && !CxConnecting() );
}
*/




// ==================================================
// 		MESSAGE CONSOLE CACHE
// ==================================================

#define FSTAT_NONE	0
#define FSTAT_IDLE	1
#define FSTAT_FREE	2
#define FSTAT_USED	3


void
 TSonorkWin32App::MakeProfileFilePath(SONORK_C_STR path, SONORK_C_CSTR name_ext)
{
	char fname[80];
	sprintf(fname,"%u.%u\\%u.%u-%s"
			,ProfileUserId().v[0]
			,ProfileUserId().v[1]
			,ProfileUserId().v[0]
			,ProfileUserId().v[1]
			,name_ext);
	GetDirPath(path,SONORK_APP_DIR_DATA,fname);
}
void
 TSonorkWin32App::MakeMsgCachePath(SONORK_C_STR path, const TSonorkId& userId)
{
	char fname[64];
	sprintf(fname,"%u.%u\\%u.%u-%u.%u(%s)"
			,ProfileUserId().v[0]
			,ProfileUserId().v[1]
			,ProfileUserId().v[0]
			,ProfileUserId().v[1]
			,userId.v[0]
			,userId.v[1]
			,szSonorkAppMode);
	GetDirPath(path,SONORK_APP_DIR_DATA,fname);
}

TSonorkCCache*
 TSonorkWin32App::GrabTmpMsgCache( const TSonorkId& userId )
{
	char path[SONORK_MAX_PATH];


	TRACE_DEBUG("GrabTmp(%u.%u) cur{rc=%u uid=%u.%u}"
		,userId.v[0],userId.v[1]
		,console.tmp.refCount
		,console.tmp.userId.v[0],console.tmp.userId.v[1]);

	if(console.tmp.refCount==0)
	{
		assert( !console.tmp.ptr->IsOpen() );
		console.tmp.userId.Set(userId);
		MakeMsgCachePath( path , userId );

		if( console.tmp.ptr->Open(path) == SONORK_RESULT_OK )
		{
			console.tmp.refCount++;
			return console.tmp.ptr;
		}
	}
	else
	if(console.tmp.userId==userId)
	{
		console.tmp.refCount++;
		return console.tmp.ptr;
	}
	return NULL;
}
void
 TSonorkWin32App::ReleaseTmpMsgCache(TSonorkCCache*C)
{
	TRACE_DEBUG("ReleaseTmp(rc=%u uid=%u.%u",console.tmp.refCount
		,console.tmp.userId.v[0],console.tmp.userId.v[1]);
	assert( console.tmp.ptr == C );
	assert( console.tmp.refCount>0 );
	if(--console.tmp.refCount==0 )
	{
		console.tmp.ptr->Close();
	}
}
TSonorkCCache*
 TSonorkWin32App::CreateMsgCache(DWORD text_size,DWORD cache_size)
{
	return new TSonorkCCache(text_size,cache_size,MsgCCacheCallback,this);
}

TSonorkCCache*
 TSonorkWin32App::GrabSharedMsgCache( const TSonorkId& userId )
{
	char path[SONORK_MAX_PATH];
	TSonorkCCache* pCache;
	pCache = CreateMsgCache();
	MakeMsgCachePath(path,userId);
	if( pCache->Open(path) == SONORK_RESULT_OK )
	{
		return pCache;
	}
	else
	{
		delete pCache;
		return NULL;
	}
}
void	TSonorkWin32App::ReleaseSharedMsgCache(TSonorkCCache*pCache)
{
	if(pCache==NULL)return;
	pCache->Close();
	delete pCache;
}



// ==================================================
// 		MESSAGES
// ==================================================

void
 TSonorkWin32App::SendUrl(TSonorkExtUserData*UD,SONORK_C_CSTR url)
{
	TSonorkMsg	 	msg;
	TSonorkMsgHandleEx	handle;
	msg.SetStr( 0 , url );
	msg.header.dataSvcInfo.SetInfo(SONORK_SERVICE_ID_NONE,SONORK_SERVICE_TYPE_URL,0);
	PrepareMsg(handle
		,&msg
		,0					// sys_flags
		,0					// usr_flags
		,SONORK_APP_MPF_NO_SERVICE_PARSE	// pc_flags
		,0                     			// reply no
		,(TSonorkServiceId*)NULL		// sourceService
		);
	SendMsgUser(handle,win32.main_win,UD,&msg);

}
BOOL
 TSonorkWin32App::IsMsgLocked(const TSonorkId& userId, const TSonorkCCacheMark&mark)
{
	TSonorkAppEventMsgQueryLock E;
	E.owner = NULL;
	E.userId.Set( userId );
	E.markPtr = &mark;
	BroadcastAppEvent(SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK
		,SONORK_APP_EM_MSG_PROCESSOR
		,0
		,&E);
	return E.owner != NULL;
}
bool	TSonorkWin32App::SetMsgTag(TSonorkExtUserData*UD
		,TSonorkCCache*		MC
		,TSonorkCCacheMark*	mark
		,DWORD 			ccFlags
		,DWORD 			ccMask
		,DWORD*			extIndex)
{
	TSonorkCCacheEntry *pEntry;
	pEntry=MC->GetByMark( mark , NULL);

	if( pEntry == NULL )
		return false;

	return SetMsgTag(UD
			,MC
			,mark->line_no
			,pEntry
			,ccFlags
			,ccMask
			,extIndex);
}
bool
  TSonorkWin32App::SetMsgTag(TSonorkExtUserData*UD
		,TSonorkCCache*		MC
		,DWORD 			lineNo
		,TSonorkCCacheEntry*	pEntry
		,DWORD 			ccFlags
		,DWORD 			ccMask
		,DWORD*			extIndex)
{
	BOOL 		was_unread, is_unread;
	int		msg_cc;
	TSonorkTag 	dbTAG;

	was_unread = pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD;
	pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS]&=~ccMask;
	pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS]|=(ccFlags&ccMask);
	is_unread = pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD;
	if(MC->Set(	lineNo
			,pEntry->tag
			,extIndex) != SONORK_RESULT_OK )
		return false;
	if( db.msg.Get( pEntry->dat_index, NULL, &dbTAG ) == SONORK_RESULT_OK )
	{
		dbTAG.v[SONORK_DB_TAG_FLAGS] = pEntry->tag.v[SONORK_CCACHE_TAG_FLAGS];
		if( extIndex != NULL )
			dbTAG.v[SONORK_DB_TAG_INDEX] = *extIndex;
		db.msg.SetTag( pEntry->dat_index, dbTAG );
	}

	while( UD != NULL )
	{
		if(was_unread)
		{
			if(!is_unread)
				msg_cc=-1;
			else
				break;
		}
		else
		{
			if(is_unread)
				msg_cc=1;
			else
				break;
		}
		IncUnreadMsgCount(UD,msg_cc);
		break;
	}
	return true;
}

bool
  TSonorkWin32App::MarkMsgProcessed(const TSonorkId&userId
			,TSonorkCCacheMark& mark
			,DWORD ccFlags
			,DWORD ccMask
			,DWORD*extIndex)
{
	TSonorkAppEventMsgProcessed E;
	TSonorkCCache *	MC;
	bool		rv;
	E.userId.Set(userId);
	E.markPtr			= &mark;
	E.pcFlags			= SONORK_APP_MPF_STORED;
	E.extIndex			= extIndex;
	E.ccFlags			= ccFlags;
	E.ccMask 			= ccMask;
	
	BroadcastAppEvent(SONORK_APP_EVENT_SONORK_MSG_PROCESSED
		,SONORK_APP_EM_MSG_CONSUMER
		,0
		,&E);

	if(E.pcFlags & SONORK_APP_MPF_PROCESSED)
		return true;

	TRACE_DEBUG("MarkMsgProcessed______ 1");
	MC=GrabTmpMsgCache(E.userId);
	if(MC)
	{
		TSonorkCCacheEntry *pEntry;
		pEntry=MC->GetByMark( &mark , NULL);
		if( pEntry != NULL )
		{
			rv = SetMsgTag(UserList().Get(E.userId)
				,MC
				,mark.line_no
				,pEntry
				,ccFlags
				,ccMask
				,extIndex);
		}
		else
			rv=false;
		ReleaseTmpMsgCache(MC);
	}
	else
		rv=false;
	TRACE_DEBUG("MarkMsgProcessed______ 0");
	return rv;
}


void
 TSonorkWin32App::PrepareMsg(
		  TSonorkMsgHandle&	handle
		, TSonorkMsg*           msg
		, DWORD 		sysFlags
		, DWORD 		usrFlags
		, DWORD 		pcFlags
		, DWORD 		replyTrackingNo
		, TSonorkServiceId*	sourceService
		)
{
	assert( msg!=NULL );
	PrepareMsg(handle
		, &msg->header
		, &msg->text
		, sysFlags
		, usrFlags
		, pcFlags
		, replyTrackingNo
		, sourceService
		);
}
void
 TSonorkWin32App::PrepareMsg(
		  TSonorkMsgHandle&	handle
		, TSonorkMsgHeader*	header
		, TSonorkText*		text
		, DWORD 		sysFlags
		, DWORD 		usrFlags
		, DWORD 		pcFlags
		, DWORD 		replyTrackingNo
		, TSonorkServiceId*	sourceService
		)
{

	if(!(pcFlags&SONORK_APP_MPF_NO_TIME_SET))
		header->time.SetTime_GMT();

	if( pcFlags & SONORK_APP_MPF_CLEAR_SERVICE )
	{
		header->dataSvcInfo.Clear();
	}
	else
	while( !(pcFlags&SONORK_APP_MPF_NO_SERVICE_PARSE) )
	{
		if( text !=NULL )
		{
			if(SONORK_IsUrl( text->CStr() ) )
			{

				header->dataSvcInfo.SetInfo(
					 SONORK_SERVICE_ID_NONE
					,SONORK_SERVICE_TYPE_URL
					,0);
				break;

			}
		}
		header->dataSvcInfo.Clear();
		break;
	}
	header->sysFlags	     	= sysFlags;
	header->trackingNo.v[1]		= replyTrackingNo;
	if( replyTrackingNo == 0 )
	{
		header->usrFlags=usrFlags&~SONORK_MSG_UF_REPLY;
	}
	else
	{
		header->usrFlags=usrFlags|SONORK_MSG_UF_REPLY;
	}
	if( text )
	{
		text->SetRegion( ProfileRegion() );
	}
	handle.mark.Clear();
	handle.taskId	= SONORK_INVALID_TASK_ID;
	handle.pcFlags	= pcFlags;
	handle.handleId	= GenSelfTrackingNo();
	if( sourceService )
		header->sourceService.Set( *sourceService );
	else
		header->sourceService.Clear();

}


SONORK_RESULT
 TSonorkWin32App::SendMsgUser(
		  TSonorkMsgHandleEx&		handle
		, TSonorkWin*			win
		, const TSonorkExtUserData*	UD
		, TSonorkMsg*           	msg
		, const TSonorkMsgTarget*	target
		)
{
	TSonorkUserLocus1	locus;
	DWORD			was_silent;
	if( UD != NULL )
	{
		was_silent=(handle.pcFlags&SONORK_APP_MPF_SILENT);
		handle.pcFlags|=SONORK_APP_MPF_SILENT;
		UD->GetLocus1(&locus);
		SendMsgLocus(
			 handle
			,win
			,&locus
			,UD->UtsLinkId()
			,msg
			,target
			);
                if(!was_silent)
			handle.pcFlags&=~SONORK_APP_MPF_SILENT;
	}
	else
	{
		handle.taskId=SONORK_INVALID_TASK_ID;
		handle.ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_BADHANDLE,SONORK_MODULE_LINE);
	}
	if( handle.ERR.Result() != SONORK_RESULT_OK )
		if( !(handle.pcFlags & SONORK_APP_MPF_SILENT) && win != NULL)
		{
			win->TaskErrorBox(GLS_TK_SNDMSG , &handle.ERR);
		}
	return handle.ERR.Result();
}

SONORK_RESULT
 TSonorkWin32App::SendMsgLocus(
		  TSonorkMsgHandleEx&		handle
		, TSonorkWin*			win
		, TSonorkUserLocus1*		locus
		, SONORK_UTS_LINK_ID 		utsLinkId
		, TSonorkMsg*           	msg
		, const TSonorkMsgTarget*	target
		)
{
	TSonorkDataPacket*	P;
	UINT			A_size,P_size;
	bool			message_sent=false;
	bool			send_by_engine=true;
	SONORK_DWORD2		task_tag;
	TSonorkAppTask*		taskPtr;


	SONORK_MEM_NEW( taskPtr= new TSonorkAppTask(SONORK_APP_TASK_SEND_MSG) );


	task_tag.v[0] = (DWORD)taskPtr;
	task_tag.v[1] = (DWORD)-1;

        handle.pcFlags&=~SONORK_APP_MPF_STORED;
	msg->header.userId.Set( locus->userId );

	if( target )
	{
		msg->header.target.Set( *target );
	}
	else
	{
		msg->header.target.userId.Set( msg->header.userId );
		msg->header.target.service.Clear();
	}


	if( !(handle.pcFlags&SONORK_APP_MPF_NO_TRACKING_NUMBER) )
		msg->header.trackingNo.v[0] = GenTrackingNo( locus->userId );

	A_size=::CODEC_Size(locus) + ::CODEC_Size(msg) + 8;

	if( handle.pcFlags & SONORK_APP_MPF_DONT_SEND )
	{
		message_sent = true;
		handle.ERR.SetOk();
	}
	else
	if( !(handle.pcFlags & SONORK_APP_MPF_NO_UTS) )
	{
		if( utsLinkId!= SONORK_INVALID_LINK_ID
			&& UTS_Enabled() )
		{

			win32.uts_server->SendPacket(handle.ERR
				, utsLinkId
				, SONORK_UTS_CMD_MSG
				, 0
				, msg
				, this
				, SonorkUtsRequestHandler
				, &task_tag);
			if( handle.ERR.Result() == SONORK_RESULT_OK )
			{
				message_sent = true;
			}
		}

		if( !message_sent )
		{
			if(handle.pcFlags & SONORK_APP_MPF_UTS)
			{
				handle.ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE
					, GSS_NOUTSLNK
					, SONORK_MODULE_LINE);
				send_by_engine = false;

			}
			else
			{
				send_by_engine = true;
			}
		}
	}

	if( !message_sent && send_by_engine )
	{
		P=SONORK_AllocDataPacket( A_size );
		P_size = P->E_PutMsg_R(A_size,locus,msg);
		StartSonorkRequest(P,P_size,&task_tag,&handle.ERR);
		if( handle.ERR.Result() == SONORK_RESULT_OK )
		{
			message_sent = true;
		}
	}
	if( message_sent )
	{
		handle.pcFlags=
			ConsumeMsgEvent(SONORK_APP_EVENT_SONORK_MSG_SENDING
				, msg
				, handle.pcFlags
				, 0
				, &taskPtr->send_msg.mark
				, handle.handleId
				, task_tag.v[0]
				);

		if( handle.pcFlags & SONORK_APP_MPF_STORED )
		{
			handle.mark.Set(taskPtr->send_msg.mark);
		}
		else
		{
			handle.mark.Clear();
		}
		win32.task_queue.Add( taskPtr );
		taskPtr->send_msg.pcFlags	= handle.pcFlags;
		taskPtr->send_msg.handleId	= handle.handleId;
		memcpy(&taskPtr->send_msg.header
			, &msg->header
			, sizeof(TSonorkMsgHeader));

		if( handle.pcFlags & SONORK_APP_MPF_DONT_SEND )
		{
			PostAppCommand(SONORK_APP_COMMAND_PSEUDO_TASK_RESULT
			,(DWORD)taskPtr);
		}
	}
	else
	{
		task_tag.v[0]=0;
		SONORK_MEM_DELETE( taskPtr );

		assert( handle.ERR.Result() != SONORK_RESULT_OK );
		if( !(handle.pcFlags & SONORK_APP_MPF_SILENT) && win != NULL)
		{
			win->TaskErrorBox(GLS_TK_SNDMSG , &handle.ERR);
		}
		handle.mark.Clear();
	}
	handle.taskId=task_tag.v[0];
	return handle.ERR.Result();
}


// ----------------------------------------------------------------------------
// OnSonorkMsg()
//  A Sonork message has arrived.

void
 TSonorkWin32App::OnSonorkMsg(TSonorkMsg* msg)
{
	ConsumeMsgEvent(SONORK_APP_EVENT_SONORK_MSG_RCVD
		, msg
		, SONORK_APP_MPF_ADD_USER
		, 0
		, NULL
		, 0
		, 0);
}

// ----------------------------------------------------------------------------

// returns modified pcFlags

DWORD
 TSonorkWin32App::ConsumeMsgEvent(SONORK_APP_EVENT event
		, TSonorkMsg* pMsg
		, DWORD  pcFlags
		, DWORD  ccFlags
		, TSonorkCCacheMark* pMark
		, DWORD	 handleId
		, DWORD	 taskId
		)
{
	TSonorkCCache 		*MC;
	TSonorkExtUserData	*UD;
	TSonorkAppEventMsg	E;
	bool	user_in_auth_list;


	assert( pMsg 	!= NULL );
	E.msg 		= pMsg;
	E.ERR 		= NULL;
	E.taskId	= taskId;
	E.handleId	= handleId;
	E.mark.Clear();

	if( event == SONORK_APP_EVENT_SONORK_MSG_RCVD )
	{
		assert( taskId == 0 && handleId== 0);
		E.ccFlags=(ccFlags&~(SONORK_APP_CCF_UNSENT))|SONORK_APP_CCF_INCOMMING|SONORK_APP_CCF_UNREAD;
		if( pMsg->Target().userId.IsZero() )
		{
			pMsg->header.target.userId.Set(ProfileUserId());
		}
	}
	else
	if( event == SONORK_APP_EVENT_SONORK_MSG_SENDING )
	{
		assert( taskId != 0 );
		E.ccFlags=(ccFlags&~(SONORK_APP_CCF_ERROR|SONORK_APP_CCF_INCOMMING))|SONORK_APP_CCF_UNSENT;
	}
	else
		assert( 0 );
	E.header = &pMsg->header ;

	E.pcFlags = pcFlags&~SONORK_APP_MPF_PROCESSED;
	BroadcastAppEvent(event
		,SONORK_APP_EM_MSG_CONSUMER
		,0
		,&E);

	while( !(E.pcFlags&SONORK_APP_MPF_PROCESSED)
		&&
		IS_SONORK_APP_SERVICE_ID(E.header->target.ServiceId()) )
	{
		UD=UserList().Get( E.header->userId );
		if( UD==NULL )
		{
			if( event == SONORK_APP_EVENT_SONORK_MSG_RCVD )
			{
				if(E.header->userId.v[1]==1)
				{
					// users with ID x.1 are system: Cannot ignore
					E.pcFlags|=SONORK_APP_MPF_ADD_USER;
				}
				else
				{
					user_in_auth_list = auth_exclude_list.Get( E.header->userId )!=NULL;
					if( ProfileCtrlFlags().Test(SONORK_PCF_IGNORE_NON_AUTHED_MSGS) )
					{
						if( !user_in_auth_list )
							break;
					}
					else
					{
						if( user_in_auth_list )
							break;
					}
				}
			}

			if( E.pcFlags&SONORK_APP_MPF_ADD_USER )
			{
				UD=AddRemoteUser( E.header->userId , NULL , 0 );
			}
		}

		E.pcFlags|=SONORK_APP_MPF_PROCESSED;

		if( !(E.pcFlags&(SONORK_APP_MPF_DONT_STORE|SONORK_APP_MPF_STORED)) )
		{
			TRACE_DEBUG("ConsumeMsgEvent______ 1");
			MC=GrabTmpMsgCache( E.header->userId);
			if(MC != NULL )
			{

				if(StoreMsg(MC
					,E.msg
					,E.pcFlags
					,E.ccFlags
					,UD
					,E.mark) == SONORK_RESULT_OK)
				{
					E.pcFlags|=SONORK_APP_MPF_STORED;
				}

				ReleaseTmpMsgCache(MC);
			}
			TRACE_DEBUG("ConsumeMsgEvent______ 0");
		}
		if( event == SONORK_APP_EVENT_SONORK_MSG_RCVD
		&& (E.pcFlags&SONORK_APP_MPF_STORED) )
		{
			if(UD != NULL )
			{
				if(ProfileCtrlFlags().Test(SONORK_PCF_POPUP_MSG_WIN))
				{
					OpenMsgWindow(UD,SONORK_MSG_WIN_OPEN_AUTOMATIC);
				}
				else
				{
					Set_UI_Event(SONORK_UI_EVENT_USER_MSG
					 , pMsg->ToCStr()
					 , 0
					 , UD);
				}
			}
		}
		// while() break exit
		break;
	}
	if( pMark )pMark->Set( E.mark );
	return E.pcFlags;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnAppTaskResult(TSonorkAppTask*TASK
			, const TSonorkError* pERR
			, bool remove_from_queue)
{
	if( remove_from_queue )
	{
		if( !win32.task_queue.Remove( TASK ) )
		{
			sonork_printf("***** PENDING TASK %08x NOT FOUND *****" , TASK );
			return ;
		}
	}
	assert( pERR != NULL );

	switch(TASK->type)
	{
		case SONORK_APP_TASK_SEND_MSG:
			OnAppTaskResult_SendMsg(TASK,pERR);
		break;
		case SONORK_APP_TASK_EXPORT_SERVICE:
			OnAppTaskResult_ExportService(&TASK->export_service,pERR);
		break;
		case SONORK_APP_TASK_SEND_SERVICE_MSG:
			OnAppTaskResult_SendServiceMsg(&TASK->send_service_msg,pERR);
		break;

	}
	SONORK_MEM_DELETE( TASK );
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnAppTaskResult_SendMsg(
		  TSonorkAppTask* TASK
		, const TSonorkError*pERR)
{
	TSonorkAppEventMsg	E;
	TSonorkCCache 		*MC;
	static TSonorkTime	last_err_sound;

	E.taskId 	= (DWORD)TASK;
	E.handleId	= TASK->send_msg.handleId;
	E.pcFlags	= (TASK->send_msg.pcFlags&~SONORK_APP_MPF_PROCESSED);
	E.ccFlags	= 0;
	E.msg		= NULL;
	E.header	=&TASK->send_msg.header;
	E.mark.Set( TASK->send_msg.mark );
	E.ERR		= pERR;
	if( pERR->Result() != SONORK_RESULT_OK )
	{
		E.ccFlags|=SONORK_APP_CCF_ERROR;
		if( last_err_sound != win32.cur_time)
		{
			AppSound( SONORK_APP_SOUND_ERROR );
			last_err_sound.Set(win32.cur_time);
		}
	}
	BroadcastAppEvent(SONORK_APP_EVENT_SONORK_MSG_SENT
		,SONORK_APP_EM_MSG_CONSUMER|SONORK_APP_EM_MSG_PROCESSOR
		,0
		,&E);
	if( !(E.pcFlags&SONORK_APP_MPF_PROCESSED)  )
	{
		E.pcFlags|=SONORK_APP_MPF_PROCESSED;
		if( E.pcFlags&SONORK_APP_MPF_STORED )
		{
			TRACE_DEBUG("OnAppTaskResult_SendMsg______ 1");
			MC=GrabTmpMsgCache( E.header->userId );
			if( MC != NULL )
			{
				SetMsgTag(UserList().Get(E.header->userId)
					, MC
					,&E.mark
					, E.ccFlags
					, SONORK_APP_CCF_UNSENT|SONORK_APP_CCF_ERROR
					, NULL);
				ReleaseTmpMsgCache(MC);
			}
			TRACE_DEBUG("OnAppTaskResult_SendMsg______ 0");
		}
	}
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::AddExtData(DWORD*index,const TSonorkCodecAtom*A,TSonorkTag*tag)
{
	return db.ext.Add(A,index,tag);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::AddExtData(DWORD*index,const TSonorkShortString*S,TSonorkTag*tag)
{
	return AddExtData(index
		,&TSonorkCodecShortStringAtom((TSonorkShortString*)S)
		,tag);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::GetExtData(DWORD index,TSonorkCodecAtom*A,TSonorkTag*tag)
{
	return db.ext.Get(index,A,tag);
}

// ----------------------------------------------------------------------------

SONORK_RESULT 	TSonorkWin32App::GetMsg(DWORD index, TSonorkMsg*msg)
{
	return db.msg.Get(index,msg);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::GetExtData(DWORD index,TSonorkShortString*S,TSonorkTag*tag)
{
	return GetExtData(index,&TSonorkCodecShortStringAtom(S),tag);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::DelExtData(DWORD index)
{
	return db.ext.Del(index);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::DelMsg(DWORD index)
{
	return db.msg.Del(index);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkWin32App::StoreMsg(TSonorkCCache*MC
				,TSonorkMsg*		msg
				,DWORD 			pc_flags
				,DWORD			cc_flags
				,TSonorkExtUserData*	pUserData
				,TSonorkCCacheMark& 	mark)
{
	SONORK_RESULT			result;
	TSonorkCCacheEntry		CH;
	TSonorkTag			dbTAG;

	if( pUserData )
	{
		if( !(pUserData->userId == msg->UserId() ) )
			assert( pUserData->userId == msg->UserId() );
	}
	msg->ToCStr();
	if( msg->ExtDataType() != SONORK_ATOM_NONE )
		cc_flags|=SONORK_APP_CCF_EXT_DATA;
	switch(msg->DataServiceType())
	{
		case SONORK_SERVICE_TYPE_SONORK_FILE:
			pc_flags|=SONORK_APP_MPF_NO_STRIPPING;
			cc_flags|=SONORK_APP_CCF_NO_READ_ON_OPEN;//|SONORK_APP_CCF_LONG;
			break;
		case SONORK_SERVICE_TYPE_URL:
			pc_flags|=SONORK_APP_MPF_NO_STRIPPING;
			//cc_flags|=SONORK_APP_CCF_LONG;
			// break ommited
		case SONORK_SERVICE_TYPE_NONE:
			cc_flags|=SONORK_APP_CCF_READ_ON_SELECT;
			break;
		default:
			pc_flags|=SONORK_APP_MPF_NO_STRIPPING;
			pc_flags&=~SONORK_APP_MPF_AUTO_READ_SHORT;
			break;
	}

	if(msg->UsrFlags() & SONORK_MSG_UF_NEGATIVE)
		cc_flags|=SONORK_APP_CCF_DENIED;

	if(msg->UsrFlags() & SONORK_MSG_UF_REPLY)
		cc_flags|=SONORK_APP_CCF_IN_THREAD;

	if(msg->UsrFlags() & SONORK_MSG_UF_QUERY)
	{
		cc_flags|=SONORK_APP_CCF_QUERY;
		if(msg->header.trackingNo.v[1]==0)
			cc_flags|=SONORK_APP_CCF_THREAD_START;
		if( cc_flags & SONORK_APP_CCF_INCOMMING )
		{
			pc_flags&=~SONORK_APP_MPF_AUTO_READ_SHORT;
			cc_flags&=~SONORK_APP_CCF_READ_ON_SELECT;
		}
	}

	if( !(cc_flags & SONORK_APP_CCF_INCOMMING) )
	if( !(pc_flags & SONORK_APP_MPF_NO_TIME_SET))
		msg->header.time.Set( CurrentTime() );


	if( !(cc_flags & SONORK_APP_CCF_INCOMMING)
	||	((pc_flags & SONORK_APP_MPF_AUTO_READ_SHORT)
		&& !(msg->TextLength()>MSG_CONSOLE_TEXT_SIZE)) )
		cc_flags&=~SONORK_APP_CCF_UNREAD;

	dbTAG.v[SONORK_DB_TAG_FLAGS] = cc_flags;
	dbTAG.v[SONORK_DB_TAG_INDEX] = CH.ext_index = SONORK_INVALID_INDEX;
	result = db.msg.Add(msg,&CH.dat_index,&dbTAG);

	if( result == SONORK_RESULT_OK)
	{
		CH.tag.v[SONORK_CCACHE_TAG_FLAGS] = cc_flags;
		CH.tag.v[SONORK_CCACHE_TAG_INDEX] = msg->DataServiceDescriptor();


		CH.tracking_no.Set(msg->header.trackingNo);
		CH.time.Set(msg->header.time);
		result = MC->Add( CH , &mark.line_no );
		if(result == SONORK_RESULT_OK)
		{
			mark.Set( &CH );
			if( cc_flags & SONORK_APP_CCF_INCOMMING)
			{
				if( pUserData!=NULL && (cc_flags & SONORK_APP_CCF_UNREAD) )
					IncUnreadMsgCount(pUserData,1);

				if( CxStatus() >= SONORK_APP_CX_STATUS_READY_AND_GO )
					MsgSound(pUserData , pc_flags & SONORK_APP_MPF_SOFT_SOUND );
			}
		}
	}
	return result;
}


// ----------------------------------------------------------------------------

BOOL SONORK_CALLBACK
	TSonorkWin32App::MsgCCacheCallback(void*tag,TSonorkCCacheEntry*E,char*str,UINT size)
{
	TSonorkWin32App*_this=(TSonorkWin32App*)tag;
	if(_this->db.msg.Get(E->dat_index,&_this->console.dta.msg) == SONORK_RESULT_OK )
	{
		lstrcpyn(str,_this->console.dta.msg.CStr(),size);
		return _this->console.dta.msg.TextLength()>=size;
	}
	else
		strcpy(str,"ERR");
	return false;
}

// ----------------------------------------------------------------------------

BOOL SONORK_CALLBACK
	TSonorkWin32App::SysCCacheCallback(void*tag,TSonorkCCacheEntry*E,char*str,UINT size)
{
	TSonorkWin32App*_this=(TSonorkWin32App*)tag;
	TSonorkCodecLCStringAtom	A;

	A.ptr = str;
	A.length_or_size = size;
	if(_this->db.sys.Get(E->dat_index,&A) != SONORK_RESULT_OK )
		strcpy(str,"ERR");
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ProcessUserSidChange(
				TSonorkExtUserData*UD
				,const TSonorkUserLocus3*old_locus
				,BOOL update_ui)
{
	TSonorkExtUserViewItem* view_item;
	TSonorkAppEventUserSid	E;

	E.local.userData    = UD;
	E.local.newSidMode = UD->addr.sidFlags.SidMode();
	E.local.oldSidMode =(old_locus != NULL)
				?old_locus->sidFlags.SidMode()
				:SONORK_SID_MODE_DISCONNECTED;

	// User is connecting
	if(E.local.oldSidMode == SONORK_SID_MODE_DISCONNECTED
	&& E.local.newSidMode != SONORK_SID_MODE_DISCONNECTED)
	{
		if( update_ui )
		{
			Set_UI_Event_UserConnect(UD,true);
		}
		E.local.onlineDir = 1;

	}
	else
	if(E.local.oldSidMode != SONORK_SID_MODE_DISCONNECTED
	&& E.local.newSidMode == SONORK_SID_MODE_DISCONNECTED)
	{
		// User disconnected
		E.local.onlineDir = -1;

		if( update_ui )
		{
			Service_OnUserNotAvaiable(UD->userId);
			Set_UI_Event_UserConnect(UD , false);
		}
	}
	else
		E.local.onlineDir  = 0    ;

	BroadcastAppEvent( SONORK_APP_EVENT_USER_SID
		, SONORK_APP_EM_USER_LIST_AWARE
		, SONORK_APP_EVENT_USER_SID_LOCAL
		, &E);

	view_item = win32.main_win->GetExtUserViewItem(UD);
	if( view_item != NULL )
	{
		if( E.local.onlineDir )
		{
			win32.main_win->UpdateViewItemAttributes(
				  view_item
				, E.local.onlineDir	// b_delta
				, 0       	     	// e_delta
				,SONORK_TREE_VIEW_UPDATE_F_SORT
				);
		}
		else
		{
			win32.main_win->RepaintViewItem( view_item );
		}
	}
	if(E.local.newSidMode == SONORK_SID_MODE_DISCONNECTED)
	if(win32.uts_server != NULL )
	{
		win32.uts_server->Disconnect( UD->userId , false );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::AppSound( SONORK_APP_SOUND snd )
{
	const char *file;
//	DWORD	freq=0;
	UINT	uType=(DWORD)-2;
	if( ProfileCtrlFlags().Test( SONORK_PCF_MUTE_SOUND ))
		return;
	switch(snd)
	{
		case SONORK_APP_SOUND_ERROR:
			file="error.wav";
//			freq=600;
			uType=MB_ICONHAND;
			break;

		case SONORK_APP_SOUND_NOTICE:
			file="notice.wav";
//			freq=800;
			uType=MB_ICONASTERISK;
			break;

		case SONORK_APP_SOUND_CONNECT:
			file="connect.wav";
//			freq=1000;
			uType=MB_ICONEXCLAMATION;
			break;

		case SONORK_APP_SOUND_REMIND:
//			freq=700;
			file="remind.wav";
			uType=MB_ICONQUESTION;
			break;

		case SONORK_APP_SOUND_ONLINE:
			file="online.wav";
			break;

		case SONORK_APP_SOUND_MSG_LO:
			file="msg_lo.wav";
			break;

		case SONORK_APP_SOUND_MSG_HI:
			file="msg_hi.wav";
//			freq=900;
			uType=MB_OK;
			break;

		case SONORK_APP_SOUND_TRACKER:
			file="tracker.wav";
//			freq=1200;
			uType=MB_OK;
			break;

		case SONORK_APP_SOUND_NONE:
		default:
			return;
	}
	if(AppCtrlFlags().Test(SONORK_ACF_USE_PC_SPEAKER))
	{
		if(uType!=(DWORD)-2)
			MessageBeep(uType);

//		if(freq!=0)::Beep(freq,150);
	}
	else
	{
		WavSound( file );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::WavSound(const char *file_name)
{
	TSonorkTempBuffer path(SONORK_MAX_PATH+64);
	GetDirPath(path.CStr(),SONORK_APP_DIR_SOUND,file_name);
	::PlaySound(path.CStr()
		,NULL
		,SND_ASYNC
		|SND_FILENAME
		|SND_NODEFAULT
		|SND_NOWAIT);
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::MsgSound(const TSonorkExtUserData* pUserData,BOOL soft)
{
	static TSonorkTime last_msg_time;

	if( last_msg_time == win32.cur_time )return;
	last_msg_time.Set(win32.cur_time);
	if( soft )
	{
		if(AppCtrlFlags().Test(SONORK_ACF_USE_PC_SPEAKER))
			return;
		AppSound( SONORK_APP_SOUND_MSG_LO );
	}
	else
	{
		if(pUserData!=NULL&&!AppCtrlFlags().Test(SONORK_ACF_USE_PC_SPEAKER))
		{
			if(pUserData->TestCtrlFlag(SONORK_UCF_MUT_MSG_SOUND))
				return;
			if( *pUserData->sound.message.CStr() )
			{
				WavSound( pUserData->sound.message.CStr() );
				return;
			}
		}
		AppSound( SONORK_APP_SOUND_MSG_HI );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnlSound(const TSonorkExtUserData*UD)
{
	static TSonorkTime last_onl_time;
	if( last_onl_time == win32.cur_time )return;
	last_onl_time.Set(win32.cur_time);
	if(UD!=NULL && !AppCtrlFlags().Test(SONORK_ACF_USE_PC_SPEAKER))
	{
		if(UD->TestCtrlFlag(SONORK_UCF_MUT_ONL_SOUND))
			return;
		if( *UD->sound.online.CStr() )
		{
			WavSound( UD->sound.online.CStr() );
			return;
		}
	}
	AppSound( SONORK_APP_SOUND_ONLINE );

}

// ----------------------------------------------------------------------------
// SendFiles()
//  Note: This fuction takes ownership of <queue> and DELETES it

void
 TSonorkWin32App::SendFiles(TSonorkExtUserData*UD,TSonorkShortStringQueue* queue, BOOL temporal)
{
	DWORD txFlags;
	if(queue->Items())
	{
		TSonorkFileTxGui *W;
                txFlags = temporal?SONORK_FILE_TX_F_TEMPORAL:0;
		if( !ProfileCtrlFlags().Test(SONORK_PCF_NO_COMPRESS_FILES) )
                	txFlags|=SONORK_FILE_TX_F_SEND_COMPRESS;

		W=new TSonorkFileTxGui(&UD->userId
			,1
			,UD->display_alias.CStr()
			,queue
			,txFlags);
		W->Create();
	}
	else
		SONORK_MEM_DELETE(queue);

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SendFile(TSonorkExtUserData*UD,SONORK_C_CSTR str, BOOL temporal)
{
	TSonorkShortStringQueue*queue;
	SONORK_MEM_NEW( queue = new TSonorkShortStringQueue );
	queue->Add( str );
	SendFiles( UD , queue, temporal); // Deletes queue
}

// ----------------------------------------------------------------------------
// DRAG & DROP
// ----------------------------------------------------------------------------

static bool _ProcessHDrop(TSonorkClipData*,HDROP);
static bool _ProcessFileDescriptor(TSonorkClipData*,TSonorkDropTargetData*,TSonorkDropExecute*E);

TSonorkClipData*
 TSonorkWin32App::ProcessDropExecute(  TSonorkDropExecute*E )
{
	TSonorkClipData* 	CD;
	TSonorkDropTargetData*	BF;
	bool			rv;



	BF=E->DataList().GetBestFormat();
	if(BF == NULL)return NULL;

	CD=new TSonorkClipData() ;
	if( BF->CpFormat() == cfSonorkClipData )
	{
		DWORD				data_size;
		const BYTE			*data_ptr;
		data_ptr = BF->GetData(-1,&data_size);
		if( !data_ptr )
			rv=NULL;
		else
			rv = CD->CODEC_ReadMem(data_ptr,data_size)==SONORK_RESULT_OK;
	}
	else
	if( BF->CpFormat() == CF_TEXT )
	{
		rv = (CD->SetCStr( SONORK_CLIP_DATA_TEXT, BF->GetStr() )!=NULL);
	}
	else
	if( BF->CpFormat() == CF_HDROP )
	{
		rv = _ProcessHDrop(CD,BF->GetHDrop());
	}
	else
	if(!stricmp(BF->FormatName(),CFSTR_SHELLURL))
	{
		rv = (CD->SetCStr( SONORK_CLIP_DATA_URL, BF->GetStr() )!=NULL);
	}
	else
	if( BF->CpFormat() == cfFileName ) // !stricmp(BF->FormatName(),CFSTR_FILENAME))
	{
		rv = (CD->SetCStr( SONORK_CLIP_DATA_FILE_NAME, BF->GetStr() )!=NULL);
	}
	else
	if( BF->CpFormat() == cfFileDescr ) // !stricmp(BF->FormatName(),CFSTR_FILEDESCRIPTOR)
	{
		rv = _ProcessFileDescriptor(CD,BF,E);
	}
	else
		rv = false;

	if( ! rv )
	{
		CD->Release();
		CD=NULL;
	}

	return CD;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::ProcessUserDropData(TSonorkExtUserData* UD
	, TSonorkClipData* CD
	, bool delete_drop_data)
{
	bool rv;
	if( CD==NULL)
		return false;
	if( UD == NULL )
		rv = false;
	else
	switch( CD->DataType() )
	{
		case SONORK_CLIP_DATA_URL:			// char*
			SendUrl(UD,CD->GetCStr());
			rv=true;
			break;

		case SONORK_CLIP_DATA_FILE_NAME:	// char*
			SendFile(UD,CD->GetCStr());
			rv=true;
			break;

		case SONORK_CLIP_DATA_FILE_LIST:	// TSonorkShortStringQueue*
			SendFiles(UD,(TSonorkShortStringQueue*)CD->ReleaseData());
			rv=true;
			break;

		default:
		case SONORK_CLIP_DATA_TEXT:		// char*
		case SONORK_CLIP_DATA_NONE:		// NULL
		case SONORK_CLIP_DATA_USER_ID:		// TSonorkId
			rv=false;
			break;

	}
	if( delete_drop_data)
	{
		CD->Release();
	}
	return rv;

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ProcessDropQuery( TSonorkDropQuery* Q , UINT accept_flags)
{

	if( Q->CpFormat()==cfSonorkClipData )
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_SONORKCLIPDATA)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL);
	}
	else
	if( Q->CpFormat()==CF_TEXT )
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_TEXT)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL);
	}
	else
	if(!stricmp(Q->FormatName(),CFSTR_SHELLURL))
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_URL)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL);
	}
	else
	if(Q->CpFormat() == CF_HDROP)
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_FILES)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL);
	}
	else
	if( Q->CpFormat() == cfFileName )
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_FILES)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL-1);
	}
	else
	if( Q->CpFormat() == cfFileDescr )
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_FILES)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL-2);
	}
	else
	if(!stricmp(Q->FormatName(), CFSTR_FILECONTENTS ))
	{
		if(accept_flags&SONORK_DROP_ACCEPT_F_URL)
			Q->Accept(SONORK_DROP_ACCEPT_MAX_LEVEL-3);
	}
}

// ----------------------------------------------------------------------------

bool
 _ProcessHDrop(TSonorkClipData* CD,HDROP hdrop)
{
	if(hdrop!=NULL)
	{
		UINT index,files,str_sz;
		files = DragQueryFile(hdrop,-1,NULL,0);
		if( files )
		{
			TSonorkShortString*		S;
			TSonorkShortStringQueue*	queue;
			queue = CD->SetShortStringQueue(SONORK_CLIP_DATA_FILE_LIST);
			if( queue != NULL )
			{
				for(index = 0; index<files ; index++ )
				{
					str_sz = DragQueryFile(hdrop,index,NULL,0) + 2;
					SONORK_MEM_NEW( S = new TSonorkShortString );
					S->SetBufferSize(str_sz);
					DragQueryFile(hdrop,index,S->Buffer(),str_sz);
					queue->Add( S );
				}
				return true;
			}
		}
	}
	return false;
}

// ----------------------------------------------------------------------------

bool
 _ProcessFileDescriptor(TSonorkClipData*CD,TSonorkDropTargetData*TD,TSonorkDropExecute*E)
{
	FILEGROUPDESCRIPTOR*	FG;

	TSonorkDropTargetData*		FC;

	DWORD	size;

	char *ptr,*data;


	FG = TD->GetFileGroup(0);

	if( FG == NULL )
		return false;

	if(!FG->cItems)
		return false;

	if( stricmp(SONORK_IO_GetFileExt(FG->fgd[0].cFileName),".url") != 0)
		return false;

	FC = E->DataList().GetFormat(CFSTR_FILECONTENTS);
	if(FC == NULL)
		return false;

	data=(char*)FC->GetData(0,&size);
	if(data!=NULL && size<2048)
	{
		ptr=strstr(data,"URL=");
		if( ptr!=NULL )
		{
			const char *t;
			ptr+=4;
			t=ptr;
			while( *ptr > ' ' )ptr++;
			*ptr=0;
			return CD->SetCStr(SONORK_CLIP_DATA_URL, t)!=NULL;
		}
	}
	return false;
}

// ----------------------------------------------------------------------------
// LANGUAGE
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// LangLoadCodeTable()
// Loads the code table file; <name> should not include paths nor extension,
//  the data is always read from the <root>\lang\<name>.txt file.
// Returns TRUE if loaded, FALSE if failed

BOOL
 TSonorkWin32App::LangLoadCodeTable( TSonorkError&ERR, TSonorkLangCodeTable&CT, SONORK_C_CSTR name )
{
	TSonorkTempBuffer	file_path( SONORK_MAX_PATH+8 );
	char			tmp[96];
	sprintf(tmp,"lang\\%s.txt",name);
	GetDirPath(file_path.CStr(), SONORK_APP_DIR_ROOT , tmp);
	CT.Load( ERR , file_path.CStr() );
	return ERR.Result() == SONORK_RESULT_OK;
}

// ----------------------------------------------------------------------------
// LangLoad()

// Loads the language file; <name> should
// not include paths nor extension, the data is always read
// from the <root>\lang\<name>\table.txt file.
// Returns TRUE if changed, returns FALSE if the language
// is already loaded or an error occurs. Caller should always check
// ERR.Result() when return value is FALSE.

BOOL
 TSonorkWin32App::LangLoad( TSonorkError& ERR
	, SONORK_C_CSTR name
	, BOOL update_user_reg_key)
{
	if( !SONORK_StrNoCaseCmp(lang.name.CStr(),name) )
	{
		// Don't load if already loaded
		ERR.SetOk();
		return false;
	}
	else
	{
		TSonorkTempBuffer	file_path( SONORK_MAX_PATH+8 );
		TSonorkLanguageTable    temp_table;
		TSonorkRegKey 		usrKEY;
		char			tmp[96];

		sprintf(tmp,"lang\\%s\\table.txt",name);
		GetDirPath(file_path.CStr(), SONORK_APP_DIR_ROOT , tmp);
		temp_table.Load(ERR,file_path.CStr(), GLS_ENTRIES, gls_xlat_table);

		if( ERR.Result() == SONORK_RESULT_OK )
		{

			lang.name.Set(name);
			lang.table.Transfer( temp_table );
			if(IsProfileOpen())
			{
				wProfileRegion().SetLanguage(lang.table.LangCode());
				WriteProfileItem(szLang, &lang.name);
			}
			if(TestRunFlag(SONORK_WAPP_RF_APP_INITIALIZED))
			{
				RebuildMenus();
				BroadcastAppEvent( SONORK_APP_EVENT_SET_LANGUAGE
					,0
					,lang.table.LangCode()
					,NULL);
			}
			if( update_user_reg_key )
			{
				if(usrKEY.Open(HKEY_CURRENT_USER
					,szSrkClientRegKeyRoot
					,true)==ERROR_SUCCESS)
				{
					usrKEY.SetValue(szLang,lang.name.CStr());
				}
			}
			return true;
		}
		return false;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::EnumLanguagesIntoComboBox(HWND hwnd)
{
	char *c;
	TSonorkEnumDirHandle*DH;
	TSonorkShortString path;

	GetDirPath(path,SONORK_APP_DIR_ROOT,szLang);
	DH=SONORK_IO_EnumOpenDir(path.CStr(),NULL,"*.*");
	if( DH == NULL )return;
	do{
		if(DH->flags&SONORK_FILE_ATTR_DIRECTORY)
		{
			c=strchr(DH->name,'.');
			if(c)continue;
			if(strlen(DH->name)>40)	// Don't allow very long strings
				continue;
			::SendMessage(hwnd,CB_ADDSTRING, 0, (LPARAM)(DH->name));
		}
	}while(SONORK_IO_EnumNextDir(DH)==SONORK_RESULT_OK);
	SONORK_IO_EnumCloseDir(DH);
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::Init_Lang(const char *szLangName, BOOL update_user_reg_key)
{
	bool			english=false;
	TSonorkError		ERR;
	TSonorkShortString	lang_name;
	static const char *szEnglish="english";

	if( *szLangName == 0 )
	{
		lang_name.Set(szEnglish);
		szLangName=lang_name.CStr();
	}
	else
		lang_name.Set(szLangName);

	if(!stricmp(szLangName,szEnglish))
	{
		english=true;
	}
	for( ;; )
	{
		if( LangLoad( ERR , lang_name.CStr() , update_user_reg_key ) )
			return true;
		if( ERR.Result() == SONORK_RESULT_OK)
			return true;

		if(!english)			// Have we tried english?
		{
			lang_name.Set(szEnglish);
			english=true;
			continue;
		}
		// No English: Tell the user to bugger off
		break;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Tray Icon
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::WmSonorkTrayMessage(WPARAM ,LPARAM lParam)
{
	POINT P;
	CancelAutoAwaySidMode();
	win32.slide_win->ClearEvent();
	if( lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK)
	{
		OpenNextEvent( true , lParam == WM_LBUTTONDBLCLK );
	}
	else
	if( lParam == WM_RBUTTONDOWN )
	{
		GetCursorPos(&P);
		TrackPopupMenu(menus.tray_icon
			, TPM_RIGHTALIGN
			, P.x
			, P.y
			, 0
			, win32.main_win->Handle()
			, NULL);
	}
	else
		return;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkWin32App::TrayMessage(DWORD dwMessage, HICON hIcon, SONORK_C_CSTR pszTip)
{
	BOOL res;
	NOTIFYICONDATA tnd;
	tnd.cbSize				= sizeof(NOTIFYICONDATA);
	tnd.hIcon				= hIcon;
	tnd.hWnd		   		= win32.work_win;
	tnd.uID					= 100;
	tnd.uFlags				= NIF_MESSAGE|(tnd.hIcon!=NULL?NIF_ICON:0)|(pszTip?NIF_TIP:0);
	tnd.uCallbackMessage	= WM_SONORK_TRAY_MESSAGE;
	if( pszTip != NULL )
	{
		lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
	}
	else
	{
		strcpy(tnd.szTip,szSONORK);
	}
	res = Shell_NotifyIcon(dwMessage, &tnd);
	return res;
}

// ----------------------------------------------------------------------------

HICON
 TSonorkWin32App::GetTrayIcon()
{
	if( GetCounter( SONORK_APP_COUNTER_EVENTS ) > 0 )
	{
		if(ProfileCtrlFlags().Test(SONORK_PCF_NO_TRAY_BLINK)
			|| TestRunFlag(SONORK_WAPP_RF_BLINK_ON))
			return sonork_skin.hicon.tray[2];
	}
	return sonork_skin.hicon.tray[CxReady()?1:0];
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SetTrayIconTip( SONORK_C_CSTR pszTip )
{
	TrayMessage(NIM_MODIFY, GetTrayIcon(), pszTip );
}

// ----------------------------------------------------------------------------
// TIMERS AND TASKS
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::WmTimer()
{
	static UINT timer_delay=0;
	win32.bicho->TimeSlot();

	if(sonork.TimeSlot())sonork.TimeSlot();

	if( ++timer_delay < SONORK_APP_TIMER_SKIP )
		return;
	timer_delay=0;

	task_timer.blink_msecs+=CurrentClock().IntervalMsecsAfter(  SonorkAppTimerClock );
	SonorkAppTimerClock.Set( CurrentClock() );
	if(win32.uts_server != NULL)
		win32.uts_server->TimeSlot(0);

	if( task_timer.blink_msecs >=  BLINK_TASK_MSECS )
	{
		TimerTask_Blink( task_timer.blink_msecs );
		task_timer.blink_msecs=0;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::TimerTask_Blink( UINT interval_msecs )
{
	win32.run_flags^=SONORK_WAPP_RF_BLINK_ON;
	win32.cur_time.SetTime_Local();

	win32.slide_win->TimeSlot( interval_msecs );
	win32.info_win->TimeSlot( interval_msecs );
	if( GetCounter( SONORK_APP_COUNTER_EVENTS )  > 0 )
	{
		task_timer.event_sound_msecs += interval_msecs;

		if(task_timer.event_sound_msecs >= EVENTS_SOUND_MSECS )
		{
			task_timer.event_sound_msecs=0;
			if(!ProfileCtrlFlags().Test(SONORK_PCF_NO_MSG_REMINDER))
				AppSound( SONORK_APP_SOUND_REMIND );
			Set_UI_Event(SONORK_UI_EVENT_EVENT_COUNT,GLS_NULL,0);
		}
		else
		{
			UpdateTrayIcon();
		}
	}
	task_timer.monitor_msecs+=interval_msecs;
	if( task_timer.monitor_msecs >= MONITOR_TASK_MSECS )
	{
		TimerTask_Monitor(task_timer.monitor_msecs);
		task_timer.monitor_msecs=0;
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::TimerTask_Monitor( UINT interval_msecs )
{
	union {
		TSonorkRemindListWin*	alarm_win;
	}D;
	UINT		aux_uint;


	task_timer.cx_status_msecs	+= interval_msecs;

	if( win32.cx_status >= SONORK_APP_CX_STATUS_READY_AND_GO )
	{

// Tasks done only while connected

		TimerTask_Services( interval_msecs );

		task_timer.mail_check_msecs+=interval_msecs;
		if( task_timer.mail_check_msecs >=  task_timer.mail_check_limit )
		{
			task_timer.mail_check_msecs=0;
			if(!win32.main_win->Task_StartMailCheck())
				aux_uint = SONORK_APP_CHECK_EMAIL_SOON_SECS;
			else
				aux_uint = ProfileCtrlValue(SONORK_PCV_EMAIL_CHECK_MINS)*60;
			RequestCheckEmailAccounts( aux_uint );
		}


		if( !sonork.Busy() && !sonork.PendingRequests() )
		{
			if( win32.run_flags&SONORK_WAPP_RF_TRACKER_SYNCH_PENDING )
			{
				TimerTask_SynchTrackers();
			}
			else
			if( task_timer.sys_msg_msecs >= SYS_MSG_CHECK_MSECS)
			{
				TimerTask_CheckSysMsgs();
			}
			else
				task_timer.sys_msg_msecs+= interval_msecs;
		}
	}
	else
	{
		if(TestRunFlag(SONORK_WAPP_RF_CX_PENDING)
		|| win32.cx_status > SONORK_APP_CX_STATUS_IDLE )
		{
			TimerTask_CheckCxStatus();
		}
	}
	if( IsProfileOpen() )
	{

		task_timer.flush_msecs	+= interval_msecs;

		if(task_timer.flush_msecs >= FLUSH_TASK_MSECS )
		{
			db.msg.Flush();
			db.ext.Flush();
			ConfigFile().Flush();
			BroadcastAppEvent(SONORK_APP_EVENT_FLUSH_BUFFERS,0,task_timer.flush_msecs,NULL);
			task_timer.flush_msecs=0;
		}

		task_timer.alarm_check_msecs	+= interval_msecs;
		if(task_timer.alarm_check_msecs >= task_timer.alarm_check_limit)
		{
			aux_uint=0;
			if(TestRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS ) )
			{
				aux_uint=SONORK_APP_CHECK_ALARM_SOON_SECS;
			}
			else
			{
				D.alarm_win =
					(TSonorkRemindListWin*)GetSysDialog(SONORK_SYS_DIALOG_REMIND_LIST);

				if( D.alarm_win != NULL )
					if(!D.alarm_win->MayCheckAlarms())
						aux_uint=SONORK_APP_CHECK_ALARM_SOON_SECS;
			}
			if( aux_uint == 0 )
				aux_uint = TimerTask_CheckAlarms();
			RequestCheckAlarms( aux_uint );
		}
	}
}
void
 TSonorkWin32App::TimerTask_SynchTrackers()
{
	TSonorkError		ERR;
	TSonorkAtomDb		db;
	TSonorkTrackerData*	data;
	TSonorkTrackerDataQueue queue;
	UINT			ci,mi;
	win32.run_flags&=~SONORK_WAPP_RF_TRACKER_SYNCH_PENDING;
	if( ServerVersionNumber() < MAKE_VERSION_NUMBER(1,5,0,7) )
		return ;
	if(OpenAppDb(SONORK_APP_DB_TRACKER_SUBS,db,false))
	{
		if((mi=db.Items())!=0)
		{
			SONORK_MEM_NEW( data = new TSonorkTrackerData );
			for(ci=0;ci<mi;ci++)
			if(db.Get(ci,data,NULL) == SONORK_RESULT_OK)
			{
				if( !data->header.flags.Test(SONORK_TRACKER_DATA_F0_ACTIVE) )
					continue;
				queue.Add( data );
				SONORK_MEM_NEW( data = new TSonorkTrackerData );
			}
			SONORK_MEM_DELETE( data );
			if( queue.Items() )
				sonork.RegisterTrackers(ERR,queue);
		}
		db.Close();
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::TimerTask_CheckSysMsgs()
{
	TSonorkError	ERR;
	sonork.GetSysMsgs( ERR , profile.ctrl_data.LastSysMsgTime() , ProfileRegion() );
	if( ERR.Result() == SONORK_RESULT_OK )
		task_timer.sys_msg_msecs=0;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::TimerTask_CheckCxStatus()
{
	switch( CxStatus() )
	{
		case SONORK_APP_CX_STATUS_IDLE:
			if( TestRunFlag( SONORK_WAPP_RF_CX_PENDING )
			 || TestCfgFlag( SONORK_WAPP_CF_NO_DISCONNECT) )
			{
				if( task_timer.cx_status_msecs >= CONNECT_RETRY_MSECS
					|| TestRunFlag( SONORK_WAPP_RF_CONNECT_NOW ) )
				{
					task_timer.cx_status_msecs = 0;
					if( !TestRunFlag(SONORK_WAPP_RF_NO_AUTO_LOGIN) )
					{
						win32.run_flags&=~(SONORK_WAPP_RF_CONNECT_NOW|SONORK_WAPP_RF_CX_PENDING);
						win32.main_win->PostPoke(SONORK_MAINWIN_POKE_CONNECT_DISCONNECT , true );
					}
				}
			}
			break;

		case SONORK_APP_CX_STATUS_READY:
			if( task_timer.cx_status_msecs >= CONNECT_READYGO_DELAY_MSECS )
			{
				// SetCxSatus resets the counter
				SetCxStatus( SONORK_APP_CX_STATUS_READY_AND_GO , TSonorkErrorOk() );
			}
			break;
	}
}


// ----------------------------------------------------------------------------

#define ALARM_DELTA_NOW			0
#define ALARM_DELTA_PASSED		-1

// Returns the time the next-closest alarm will
// activate, in seconds
UINT
 TSonorkWin32App::TimerTask_CheckAlarms()
{
	TSonorkRemindData		RD;
	TSonorkAtomDb			DB;
	DWORD				ci,mi;
	int	      			delta_minutes,ex;
	double				diff_time;
	char				tmp[80];
	UINT				next_check_mins;
	UINT				next_check_secs;

	TSonorkRemindAlarmWin*	W;
	next_check_mins 	= SONORK_APP_CHECK_ALARM_VERY_LATE_MINS;
	next_check_secs    	= 0;
	if( OpenAppDb( SONORK_APP_DB_REMIND , DB , false) )
	{
		mi = DB.Items();
		for(ci=0 ; ci<mi ; ci++)
		{
			if( DB.Get(ci,&RD) != SONORK_RESULT_OK )
				continue;

			if( !(RD.IsAlarmOn() && RD.AlarmTime().IsValid()) )
				continue;

			RD.AlarmTime().DiffTime(CurrentTime(),&diff_time);
			delta_minutes = (int)(diff_time/60) + RD.GetAlarmDelayMins();
			if(diff_time > delta_minutes*60)
			{
				ex=1;
			}
			else
				ex=0;
			if( delta_minutes <= 0 )
				next_check_secs = SONORK_APP_CHECK_ALARM_SOON_SECS;

			delta_minutes += ex;
			if( delta_minutes <= (int)next_check_mins )
			{
				if(delta_minutes <= 0)
				{
					delta_minutes   = ALARM_DELTA_PASSED;
					next_check_mins = 0;
				}
				else
					next_check_mins = delta_minutes;
			}
			if( delta_minutes <=  0 )
			{
				W=(TSonorkRemindAlarmWin*)RunSysDialog(SONORK_SYS_DIALOG_REMIND_ALARM);
				if(W)
				{
					memcpy(&W->header,&RD.header,sizeof(TSonorkRemindDataHeader));
					W->SetWindowText(RD.title.CStr());
					MakeTimeStr( RD.AlarmTime(),tmp, MKTIMESTR_TIME|MKTIMESTR_DATE);
					W->SetCtrlText(IDC_ALARM_TITLE,tmp);
					W->SetCtrlText(IDC_ALARM_NOTES,RD.notes.CStr());
					AppSound( SONORK_APP_SOUND_NOTICE );
					break;
				}
			}
		}
		DB.Close();
	}
	return (UINT)(next_check_mins*60)+next_check_secs;
}

void
 TSonorkWin32App::RequestSynchTrackers(bool v)
{
	if( v )
		win32.run_flags|=SONORK_WAPP_RF_TRACKER_SYNCH_PENDING;
	else
		win32.run_flags&=~SONORK_WAPP_RF_TRACKER_SYNCH_PENDING;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::RequestCheckAlarms( UINT secs )
{
	if(!secs)secs=SONORK_APP_CHECK_ALARM_VERY_SOON_SECS;
	task_timer.alarm_check_msecs = 0;
	task_timer.alarm_check_limit = secs*1000;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::OpenAppDb(SONORK_APP_DB app_db, TSonorkAtomDb& DB , bool write_mode)
{
	char 				tmp[64];
	TSonorkTempBuffer		path(SONORK_MAX_PATH);
	SONORK_C_CSTR			name;
	SONORK_APP_DIR			dir=SONORK_APP_DIR_DATA;
	switch(app_db)
	{
		case SONORK_APP_DB_REMIND:
			name="remind";
			break;
		case SONORK_APP_DB_EMAIL_ACCOUNT:
			name="email_acc";
			break;
		case SONORK_APP_DB_EMAIL_EXCEPT:
			name="email_exc";
			break;
		case SONORK_APP_DB_TRACKER_SUBS:
			name="tracker";
			break;

		case SONORK_APP_DB_TRACKER_CACHE:
		
			dir = SONORK_APP_DIR_TEMP;
			sprintf(tmp,"%u.%u-tracker(%s)"
				,ProfileUserId().v[0]
				,ProfileUserId().v[1]
				,szSonorkAppMode);

			goto do_open;

		default:
			return false;
	}

	sprintf(tmp,"%u.%u\\%u.%u-%s(%s)"
				,ProfileUserId().v[0]
				,ProfileUserId().v[1]
				,ProfileUserId().v[0]
				,ProfileUserId().v[1]
				,name
				,szSonorkAppMode);
do_open:
	GetDirPath(path.CStr(),dir,tmp);
	return DB.Open(ProfileUserId(), path.CStr(),write_mode)==SONORK_RESULT_OK;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::CancelCheckEmailAccounts()
{
	win32.main_win->Task_StopMailCheck( true );
}
void
 TSonorkWin32App::RequestCheckEmailAccounts(UINT secs)
{
	if(!secs)secs=SONORK_APP_CHECK_EMAIL_VERY_SOON_SECS;
	if( ProfileCtrlValue(SONORK_PCV_EMAIL_CHECK_MINS)<5)
		ProfileCtrlValue(SONORK_PCV_EMAIL_CHECK_MINS)=5;
	task_timer.mail_check_msecs = 0;
	task_timer.mail_check_limit = secs*1000;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::AddAlarmItem(const TSonorkRemindData* newRD)
{
	TSonorkRemindListWin* 	W;
	TSonorkAtomDb		DB;
	TSonorkRemindData	oldRD;
	DWORD			ci,mi;
	if(newRD==NULL)
		return;
	W = (TSonorkRemindListWin*)GetSysDialog(SONORK_SYS_DIALOG_REMIND_LIST);
	if( W != NULL )
		W->AddItem( newRD );
	if( OpenAppDb( SONORK_APP_DB_REMIND , DB , false) )
	{
		mi = DB.Items();
		for(ci=0 ; ci<mi ; ci++)
		{
			if( DB.Get(ci,&oldRD) != SONORK_RESULT_OK )
				continue;

			if( oldRD.equ(newRD) )
			{
				DB.Del(ci);
				break;
			}
		}
		DB.Add(newRD,NULL);

		DB.Close();
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SaveAlarmItemHeader( const TSonorkRemindDataHeader* HDR , bool delete_it)
{
	TSonorkRemindListWin* 	W;
	TSonorkAtomDb		DB;
	TSonorkRemindData	RD;
	DWORD			ci,mi;
	if(HDR==NULL)
		return;
	W = (TSonorkRemindListWin*)GetSysDialog(SONORK_SYS_DIALOG_REMIND_LIST);

	if( W != NULL )
		W->SetItemHeader(HDR,delete_it);
	if( OpenAppDb( SONORK_APP_DB_REMIND , DB , false) )
	{
		mi = DB.Items();
		for(ci=0 ; ci<mi ; ci++)
		{
			if( DB.Get(ci,&RD) != SONORK_RESULT_OK )
				continue;

			if( RD.equ(HDR) )
			{
				memcpy(&RD.header,HDR,sizeof(RD.header));
				DB.Del(ci);
				if( delete_it == false)
					DB.Add(&RD,NULL);
				break;
			}
		}
		DB.Close();
	}
}

// ----------------------------------------------------------------------------
// Unread messsages

bool
 TSonorkWin32App::OpenNextEvent(bool show_main_win_if_none_found , bool force_main_window_visible)
{
	TSonorkListIterator	I;
	TSonorkExtUserData *UD;
	TSonorkViewItemPtrs	VP;
	bool	event_found=false;

	while( GetCounter( SONORK_APP_COUNTER_EVENTS ) > 0 )
	{
		UserList().BeginEnum(I);
		while( (UD=UserList().EnumNext(I)) != NULL)
		{
			if( UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) != 0)
			{
				VP.user = win32.main_win->GetExtUserViewItem(UD);
				if( VP.user != NULL )
				{
					win32.main_win->FocusViewItem( VP.user );
					OpenMsgWindow( UD , SONORK_MSG_WIN_OPEN_FOREGROUND );
					event_found = true;
					break;
				}
			}
		}
		UserList().EndEnum(I);

		if( event_found )
			break;

		if( GetCounter(SONORK_APP_COUNTER_UNREAD_MSGS) != 0 )
		{
			// Something's wrong: we did not find any user with unread
			// messages but the global counter says there is at least one
			// user with unread messages
			RecalcCounter(SONORK_APP_COUNTER_UNREAD_MSGS );
		}

		if( GetCounter(SONORK_APP_COUNTER_SYS_CONSOLE) != 0 )
		{

			if( SysConsoleWin() == NULL )
			{
				ShowSysConsole();
				event_found = true;
				break;
			}

			if( SysConsoleWin()->FocusNextUnread(true) )
			{
				event_found = true;
				break;
			}

			// Something's wrong: we did not find any sys message unread
			// but the global counter says there is at least one
			// sys unread message: fix it
			SetCounter(SONORK_APP_COUNTER_SYS_CONSOLE
				, 0);
		}

		VP.auth = win32.main_win->FindFirstIncommingAuthReqViewItem();
		if( VP.auth  != NULL )
		{
			win32.main_win->FocusViewItem( VP.auth );
			OpenUserAuthReqWin( VP.auth->userId );
			force_main_window_visible = event_found = true;
			break;
		}
		if( GetCounter(SONORK_APP_COUNTER_PENDING_AUTHS) != 0 )
		{
			// Something's wrong: we did not find any pending auths
			// but the global counter says there is at least one
			// pending auth
			RecalcCounter(SONORK_APP_COUNTER_PENDING_AUTHS );
		}

// break from while()
		break;
	}
	if( (!event_found && show_main_win_if_none_found)
	||  force_main_window_visible )
	{
		if( win32.main_win->IsIconic() || !win32.main_win->IsVisible() )
			win32.main_win->ShowWindow(SW_RESTORE);

		PostAppCommand(SONORK_APP_COMMAND_FOREGROUND_HWND
			,(LPARAM)win32.main_win->Handle());
	}

	return event_found;
}

// ----------------------------------------------------------------------------
// COUNTERS
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SetCounter(SONORK_APP_COUNTER cc , int nv)
{
	if(nv<0)nv=0;
	if( counter[cc]!=nv )
	{
		counter[cc]=nv;
		RecalcCounter(SONORK_APP_COUNTER_EVENTS);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::RecalcCounter( SONORK_APP_COUNTER cc)
{
	TSonorkListIterator	I;
	TSonorkExtUserData *UD;
	UINT	old_counter;
	if( cc == SONORK_APP_COUNTER_UNREAD_MSGS )
	{
		counter[SONORK_APP_COUNTER_UNREAD_MSGS]=0;
		UserList().BeginEnum(I);
		while( (UD=UserList().EnumNext(I)) != NULL)
			counter[SONORK_APP_COUNTER_UNREAD_MSGS]+=(int)UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT);
		UserList().EndEnum(I);
	}
	else
	if( cc == SONORK_APP_COUNTER_PENDING_AUTHS )
	{
		counter[SONORK_APP_COUNTER_PENDING_AUTHS]
			=win32.main_win->CountAuthReqViewItems( false, true );
	}
	else
		assert( cc == SONORK_APP_COUNTER_EVENTS );

// Global counter recalc:
//  whenever a counter is calculated or the
//  counter is SONORK_APP_COUNTER_EVENTS

	task_timer.event_sound_msecs = 0;
	old_counter=counter[SONORK_APP_COUNTER_EVENTS];
	counter[SONORK_APP_COUNTER_EVENTS]
		= counter[SONORK_APP_COUNTER_UNREAD_MSGS]
		+ counter[SONORK_APP_COUNTER_PENDING_AUTHS]
		+ counter[SONORK_APP_COUNTER_SYS_CONSOLE];
	win32.info_win->ClearEvent( false );
	if( old_counter == 0 || counter[SONORK_APP_COUNTER_EVENTS]==0)
		UpdateTrayIcon();
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::IncUnreadMsgCount(TSonorkExtUserData* UD , int delta )
{
	assert( UD != NULL );
	SetUnreadMsgCount(UD,(int)UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)+delta);
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::SetUnreadMsgCount(TSonorkExtUserData* UD,int new_user_msg_count)
{
	int old_user_msg_count;
	UINT update_flags;

	if(new_user_msg_count<0)
		new_user_msg_count=0;

	assert( UD != NULL );

	old_user_msg_count = (int)UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT);
	if( old_user_msg_count == new_user_msg_count )
		return;
	UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) = (UINT)new_user_msg_count;

	if( old_user_msg_count==0||new_user_msg_count==0 )
	{
		update_flags = 	 SONORK_TREE_VIEW_UPDATE_F_SORT;
	}
	else
	{
		update_flags = 	 0;
	}
	win32.main_win->UpdateUserViewItemAttributes(
			  UD
			, 0								// sid_delta=0
			, new_user_msg_count - old_user_msg_count// msg_delta
			, update_flags);

	SetCounter(SONORK_APP_COUNTER_UNREAD_MSGS
		,	GetCounter(SONORK_APP_COUNTER_UNREAD_MSGS)
			+ new_user_msg_count
			- old_user_msg_count);

	BroadcastAppEvent_SetUser( UD , SONORK_APP_EVENT_SET_USER_F_MSG_COUNT);
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnMainWinActivated()
{
	CancelAutoAwaySidMode();
	win32.slide_win->ClearEvent();
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnMainWinDestroying()
{
	if( win32.main_win != NULL )
	{
		TSonorkWinStartInfo SI;

		SetRunFlag( SONORK_WAPP_RF_APP_TERMINATING );
		Disconnect();
		CloseProfile();
		BroadcastAppEvent(SONORK_APP_EVENT_SHUTDOWN,0,0,NULL);

		win32.main_win->TransferStartInfo(&SI , false);
		ConfigFile().WriteRaw(AppKey(),"SI",&SI,sizeof(SI));
		win32.info_win=NULL;
	}
}

// ----------------------------------------------------------------------------


void DbgOpen(const char*file_name);
void DbgClose();

// ----------------------------------------------------------------------------


static bool
	GetCmdFlagValue( SONORK_C_CSTR 	line
			,SONORK_C_CSTR 	flag
			,SONORK_C_STR	value=NULL
			,UINT		size =0)
{
	SONORK_C_CSTR rv;
	if(value != NULL )
		*value=0;
	rv = SONORK_StrStr(line,flag);
	if(rv)
	{
		UINT 	l=0;
		char	quote;
		rv+=strlen(flag);
		if(size && value!=NULL )
		{
			size--;
			if(*rv=='"'||*rv=='\'')
				{quote=*rv;rv++;}
			else
				{quote=0;}
			while(l<size
				&& *rv
				&& !(*rv==' ' && quote==0)
				&& !(*rv == quote) )
			{
				*value++=*rv++;
				l++;
			}
			*value=0;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

#if defined(_BORLANDC)
#pragma argsused
#endif
bool
 InitMode_Browse(HWND ,TSonorkWin32AppInitData*)
{
	/*
	OPENFILENAME 	OFN;
	TSonorkShortString	path;
	TSonorkShortString	default_dir;
	char			*aux,*file_name_ptr;

	default_dir.SetBufferSize(SONORK_MAX_PATH+2);
	strcpy(default_dir.Buffer(),ID->ini_file);
	file_name_ptr=NULL;
	aux = default_dir.Buffer();
	while(*aux)
	{
		if( *aux == '/' || *aux=='\\')
			file_name_ptr=aux;
		aux++;
	}
	path.SetBufferSize(SONORK_MAX_PATH+2);
	if(file_name_ptr)
	{
		*file_name_ptr++=0;
		strcpy(path.Buffer(),file_name_ptr);
	}
	else
	{
		strcpy(path.Buffer(),default_dir.CStr());
		default_dir.Clear();
	}
	if(path.Length()<2)
		strcpy(path.Buffer(),szSrkClientIniFile);
	OFN.lStructSize 	= sizeof(OFN);
	OFN.hwndOwner       	= hwndDlg;
	OFN.hInstance		= NULL;
	OFN.lpstrFilter		= NULL;
	OFN.lpstrCustomFilter 	= NULL;
	OFN.nMaxCustFilter	= 0;
	OFN.nFilterIndex	= 0;
	OFN.lpstrFile		= path.Buffer();
	OFN.nMaxFile		= SONORK_MAX_PATH;
	OFN.lpstrFileTitle	= NULL;
	OFN.nMaxFileTitle	= 0;
	OFN.lpstrInitialDir	= default_dir.CStr();
	OFN.lpstrTitle		= szSONORK;
	OFN.Flags		= OFN_EXPLORER
				| OFN_LONGNAMES
				| OFN_NOCHANGEDIR
				| OFN_PATHMUSTEXIST
				| OFN_FILEMUSTEXIST;
	OFN.nFileOffset		=
	OFN.nFileExtension	= 0;
	OFN.lpstrDefExt		= ".ini";
	OFN.lCustData		= 0;
	OFN.lpfnHook		= 0;
	OFN.lpTemplateName	= 0;
	if(!GetOpenFileName(&OFN))
	{
		return false;
	}
	else
	{
		strcpy(ID->ini_file,path.CStr());
		return true;
	}
	*/
	return false;
}

// ----------------------------------------------------------------------------

BOOL CALLBACK InitMode_DlgProc(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD	id, code;
	BOOL	useInternetMode;
	TSonorkWin32AppInitData*ID;
	switch( uMsg )
	{
		case WM_INITDIALOG:
			ID=(TSonorkWin32AppInitData*)lParam;
			::SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
			::SetWindowText(hwndDlg,SonorkApp.LangString(GLS_LB_INITMD));
			::SendMessage(hwndDlg,WM_SETICON
				,ICON_SMALL
				,(LPARAM)sonork_skin.AppHicon());
			useInternetMode = ID->start_mode!=SONORK_APP_START_MODE_INTRANET
				       && (ID->app_run_flags&SONORK_WAPP_RF_ALLOW_INTERNET_MODE);

			::SendMessage(GetDlgItem(hwndDlg,IDC_INITMODE_INTERNET)
				,BM_SETCHECK
				,useInternetMode?BST_CHECKED:BST_UNCHECKED
				,0);
			::SendMessage(GetDlgItem(hwndDlg,IDC_INITMODE_INTRANET)
				,BM_SETCHECK
				,useInternetMode?BST_UNCHECKED:BST_CHECKED
				,0);
			::SetDlgItemText(hwndDlg,IDC_INITMODE_SAVE,SonorkApp.LangString(GLS_OP_NASKNM));
			::SetDlgItemText(hwndDlg,IDCANCEL,SonorkApp.LangString(GLS_OP_CANCEL));
			::SetDlgItemText(hwndDlg,IDOK,SonorkApp.LangString(GLS_OP_ACCEPT));

			/*
			if(*ID->ini_file)
			{
				code = GetFileAttributes(ID->ini_file);
				if( code == (DWORD)-1 || (code & FILE_ATTRIBUTE_DIRECTORY ) )
					*ID->ini_file=0;
			}
			::SetDlgItemText(hwndDlg,IDC_INITMODE_INI_FILE,ID->ini_file);
			*/

			::SetDlgItemText(hwndDlg,IDL_INITMODE_INI_FILE,SonorkApp.LangString(GLS_LB_CFGFIL));
			code=!(ID->flags&INIT_APP_F_NO_INI_SELECT);
			::EnableWindow(GetDlgItem(hwndDlg,IDC_INITMODE_INI_FILE),code);
			::EnableWindow(GetDlgItem(hwndDlg,IDL_INITMODE_INI_FILE),code);
			::EnableWindow(GetDlgItem(hwndDlg,IDC_INITMODE_BROWSE),code);

			::EnableWindow(GetDlgItem(hwndDlg,IDC_INITMODE_INTERNET)
				,ID->app_run_flags&SONORK_WAPP_RF_ALLOW_INTERNET_MODE);

			::EnableWindow(GetDlgItem(hwndDlg,IDC_INITMODE_INTRANET) , true );



			return true;

		case WM_COMMAND:
			id = LOWORD(wParam);
			code = HIWORD(wParam);
			if( code == BN_CLICKED )
			{
				ID=(TSonorkWin32AppInitData*)::GetWindowLong(hwndDlg,GWL_USERDATA);
				if( id==IDCANCEL )
					ID->start_mode = SONORK_APP_START_MODE_CANCEL;
				else
				if( id == IDOK )
				{
					//ID->flags&=~INIT_APP_F_USER_INI;
					//if(!(ID->flags&INIT_APP_F_NO_INI_SELECT))
					//{
					//	GetDlgItemText(hwndDlg,IDC_INITMODE_INI_FILE,ID->ini_file,SONORK_MAX_PATH);
					//	if(strlen(ID->ini_file)>3 )ID->flags|=INIT_APP_F_USER_INI;
					//}
					ID->start_mode = (SendMessage(GetDlgItem(hwndDlg,IDC_INITMODE_INTRANET)
						,BM_GETCHECK,0,0) == BST_CHECKED)
						?SONORK_APP_START_MODE_INTRANET
						:SONORK_APP_START_MODE_INTERNET;
					if(SendMessage(GetDlgItem(hwndDlg,IDC_INITMODE_SAVE)
						,BM_GETCHECK,0,0) == BST_CHECKED)
						ID->flags |= INIT_APP_F_SAVE_STARTUP_MODE;
					else
						ID->flags &=~INIT_APP_F_SAVE_STARTUP_MODE;

				}
				else
				if( id==IDC_INITMODE_BROWSE )
				{
					//if(InitMode_Browse(hwndDlg,ID))
					//	::SetDlgItemText(hwndDlg,IDC_INITMODE_INI_FILE,ID->ini_file);

					break;
				}
				else
					break;
				EndDialog(hwndDlg,id);
			}
			break;
		default:
			break;
	}
	return false;
}

// ----------------------------------------------------------------------------

static bool
 FindParallelInstance( SONORK_C_CSTR cfg_name )
{
	HWND hwnd = FindWindow( szSONORK , cfg_name );
	if(hwnd)
	{
		::PostMessage(hwnd,WM_SONORK_COMMAND,SONORK_APP_COMMAND_REFRESH,0);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::Init(const char*cmd_line)
{
	TSonorkWin32AppInitData*ID;
	TSonorkRegKey		appKEY;
	TSonorkRegKey		currentUserKey;
	TSonorkRegKey		defaultUserKey;
	TSonorkRegKey		subKEY;
	TSonorkTempBuffer	aux_str(SONORK_MAX_PATH);
	TSonorkTempBuffer	err(SONORK_MAX_PATH*2);
	TSonorkError		tERR;
	char			cfg_file_full_name[SONORK_APP_CONFIG_FILE_NAME_SIZE+4];
	bool			auxBool;
	DWORD			auxDword;
	SONORK_RESULT		result;
        const char*		szDirName;
	static const char *szInitLangError=
	"Cannot read language file.\n\n"
	"No se pudo leer archivo de lenguaje.";
        static const char *szBadFolder=
        "The '%s' folder is invalid or does not exist.\n"
        "Current value is '%s'\n"
        "Please re-install";

	win32.run_flags = SONORK_WAPP_RF_ALLOW_INTERNET_MODE;

	win32.cfg_flags = 0;
	win32.cur_time.SetTime_Local();
	SONORK_ZeroMem(&task_timer,sizeof(task_timer));

	ID=new TSonorkWin32AppInitData;
	SONORK_ZeroMem(ID,sizeof(TSonorkWin32AppInitData));


// --------------------------------------------------------------------------
// MODELESS BLOCK STARTS
// --------------------------------------------------------------------------
// Try to define which mode we should start in:  INTRANET or INTERNET ?
// The mode defines what configuration file we use and which registry key
// we open under CURRENT_USER, if we cannot find out, set the START_QUERY
// flag so that the user is asked for the mode.

#define cmdFlag	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
	if(GetCmdFlagValue( cmd_line,"-Delay",cmdFlag,5))
		Sleep(strtoul(cmdFlag,NULL,10)&0x3fff);
#undef 	cmdFlag

	if(GetCmdFlagValue( cmd_line,"-minimized") )
		ID->flags|=INIT_APP_F_START_MINIMIZED;
                
	if(GetCmdFlagValue( cmd_line,"-auto") )
		ID->flags|=INIT_APP_F_START_MINIMIZED;

	if(GetCmdFlagValue( cmd_line,"-Intranet"))
	{
		// Force INTRANET mode if : Intranet flag is set
		// or an IniFile is specified (below)
		ID->start_mode = SONORK_APP_START_MODE_INTRANET;
	}
	else
	if(GetCmdFlagValue( cmd_line,"-Internet"))
	{
		// Force INTERNET mode
		ID->start_mode = SONORK_APP_START_MODE_INTERNET;
	}
	else
	{
		// AUTO mode: We should display a dialog asking the user which
		// mode he/she wants to run in.
		// (unless the a registry INI file location does not exist
		//  in which case we assume INTERNET)
		ID->start_mode = SONORK_APP_START_MODE_ASK;
	}


	if(GetCmdFlagValue( cmd_line,"-SetupInit"))
		SetRunFlag(SONORK_WAPP_RF_SETUP_INIT);

	if(GetCmdFlagValue( cmd_line,"-ShowLogin"))
		SetRunFlag(SONORK_WAPP_RF_SHOW_LOGIN);

// --------------------------------------------------------------------------
// APP LOCAL_MACHINE KEY (read only)

	referrer.id = 0;
	referrer.name.Set( szSONORK );
	referrer.url.Set( "http://www.sonork.com");
        for(;;)
	{
       	        result=SONORK_RESULT_CONFIGURATION_ERROR;
		if(appKEY.Open(HKEY_LOCAL_MACHINE
			,szSrkClientRegKeyRoot
			,false
			,KEY_READ)!=ERROR_SUCCESS)
		{
	                strcpy(err.CStr()
        	                ,"Cannot open registry for reading\n"
                	        "Application is not properly installed.");
                        break;
                }

                szDirName="RootDir";
		if(appKEY.QueryValue(szDirName
			,ID->root_dir
			,SONORK_MAX_PATH
			,&auxDword)==ERROR_SUCCESS)
		if( auxDword == REG_SZ )
		{
			auxDword = GetFileAttributes(ID->root_dir);
			if( auxDword != (DWORD)-1 )
			if( auxDword & FILE_ATTRIBUTE_DIRECTORY )
				result=SONORK_RESULT_OK;
		}

                if(result != SONORK_RESULT_OK )
                {
	                wsprintf(err.CStr()
        	                ,szBadFolder
	                        ,szDirName
                                ,ID->root_dir);
                        break;
                }

       	        result=SONORK_RESULT_CONFIGURATION_ERROR;
                szDirName="DataDir";
		if(appKEY.QueryValue(szDirName
			,ID->data_dir
			,SONORK_MAX_PATH
			,&auxDword)==ERROR_SUCCESS)
		if( auxDword == REG_SZ )
		{
			auxDword = GetFileAttributes(ID->data_dir);
			if( auxDword != (DWORD)-1 )
			if( (auxDword & FILE_ATTRIBUTE_DIRECTORY) && !(auxDword&FILE_ATTRIBUTE_READONLY))
				result=SONORK_RESULT_OK;
		}
                if(result != SONORK_RESULT_OK )
                {
	                wsprintf(err.CStr()
        	                ,szBadFolder
	                        ,szDirName
                                ,ID->data_dir);
                        break;
                }

       	        result=SONORK_RESULT_CONFIGURATION_ERROR;
                szDirName="TempDir";
		if(appKEY.QueryValue(szDirName
			,ID->temp_dir
			,SONORK_MAX_PATH
			,&auxDword)==ERROR_SUCCESS)
		if( auxDword == REG_SZ )
		{
			auxDword = GetFileAttributes(ID->temp_dir);
			if( auxDword != (DWORD)-1 )
			if( (auxDword & FILE_ATTRIBUTE_DIRECTORY) && !(auxDword&FILE_ATTRIBUTE_READONLY))
				result=SONORK_RESULT_OK;
		}
                if(result != SONORK_RESULT_OK )
                {
	                wsprintf(err.CStr()
        	                ,szBadFolder
	                        ,szDirName
                                ,ID->temp_dir);
                        break;
                }

		if(appKEY.QueryValue(szLang
			,ID->language
			,sizeof(ID->language)) != ERROR_SUCCESS)
		{
			ID->language[0]=0;
		}
		if(appKEY.QueryValue("NoIniSelect",&auxDword)==ERROR_SUCCESS)
			if( auxDword )
			{
				ID->flags|=INIT_APP_F_NO_INI_SELECT;
			}

		if(appKEY.QueryValue("AllowPublic",&auxDword)==ERROR_SUCCESS)
			if( auxDword == 0 )
			{
				win32.run_flags&=~SONORK_WAPP_RF_ALLOW_INTERNET_MODE;
			}
		// No need to regKEY.Close(): Next regKEY.Open() automatically closes it
		if( subKEY.Open( appKEY,"Referrer",false,KEY_READ ) == ERROR_SUCCESS)
		{
#define auxStr	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
			subKEY.QueryValue("Id",&referrer.id);
			if(subKEY.QueryValue("Name",auxStr,64)==ERROR_SUCCESS)
				referrer.name.Set(auxStr);
			if(subKEY.QueryValue("URL",auxStr,128)==ERROR_SUCCESS)
				referrer.url.Set(auxStr);
#undef	auxStr
		}

		break;
	}


// --------------------------------------------------------------------------
// MODELESS BLOCK ENDS
// --------------------------------------------------------------------------
// At this point we should know what mode we starting in or have the START_QUERY
// flag set, in which case the user must be prompted for the mode.


// --------------------------------------------------------------------------
// HKEY_CURRENT_USER CONTEXT STARTS
// --------------------------------------------------------------------------
// Here we try to gather as much information about the current user as we
// can, because if START_QUERY is set, we'll have to present him/her a dialog
// asking which mode should be used (Internet or Intranet?) and it would be
// nice to present this dialog using the person's language

	if( result == SONORK_RESULT_OK )
        {

        	win32.root_dir.Set(ID->root_dir);
                win32.data_dir.Set(ID->data_dir);
                win32.temp_dir.Set(ID->temp_dir);

#define auxStr	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
                wsprintf(auxStr,".DEFAULT\\%s",szSrkClientRegKeyRoot);
                defaultUserKey.Open(HKEY_USERS,auxStr,true);
#undef auxStr
                currentUserKey.Open(HKEY_CURRENT_USER
                        ,szSrkClientRegKeyRoot
                        ,true);

                // -----------------------------------------------------
                // User registry configuration
                // The CURRENT_USER language value always overwrites
                // the LOCAL_MACHINE value UNLESS we're in SETUP_INIT mode
                // (SETUP_INIT = we've been invoked by the SETUP application
                //  just after installation/re-installation)
                // In SETUP_INIT mode we use the LOCAL_MACHINE value unless
                // it does not exist.

                if(!TestRunFlag(SONORK_WAPP_RF_SETUP_INIT) )
                {
#define auxStr	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
                        for(;;)
                        {
                                if( currentUserKey.IsOpen() )
                                        if(currentUserKey.QueryValue(szLang,auxStr,sizeof(ID->language)) == ERROR_SUCCESS )
                                                break;

                                if( defaultUserKey.IsOpen() )
                                        if(defaultUserKey.QueryValue(szLang,auxStr,sizeof(ID->language)) == ERROR_SUCCESS )
                                                break;
                                *auxStr=0;
                                break;
                        }
                        if( *auxStr )strcpy(ID->language,auxStr);
#undef auxStr
                }

                if( ID->start_mode == SONORK_APP_START_MODE_ASK )
                {
                        for(;;)
                        {
                                if( currentUserKey.IsOpen() )
                                        if( currentUserKey.QueryValue(szStartMode, &auxDword ) == ERROR_SUCCESS )
                                                break;
                                if( defaultUserKey.IsOpen() )
                                        if( defaultUserKey.QueryValue(szStartMode, &auxDword ) == ERROR_SUCCESS )
                                                break;
                                auxDword=(DWORD)-1;
                                break;
                        }
                        auxDword&=0xff;
                        if(auxDword>=SONORK_APP_START_MODE_ASK	&& auxDword<SONORK_APP_START_MODES)
                                ID->start_mode = (SONORK_APP_START_MODE)auxDword;
                }


                sonork_skin.hicon.app   = LoadIcon(Instance(), MAKEINTRESOURCE(IDI_SONORK));

                // -----------------------------------------------------
                // Initial language load

                LangLoadCodeTable(tERR,lang.country,"country");
                LangLoadCodeTable(tERR,lang.language,"language");


                // Allow registry update if running after the installer
                // (The user already selected a language in the installer)
                if( !Init_Lang( ID->language , TestRunFlag(SONORK_WAPP_RF_SETUP_INIT) ) )
                {
                        result = SONORK_RESULT_CONFIGURATION_ERROR;
                        strcpy(err.CStr(),szInitLangError);
                }

        }

// --------------------------------------------------------------------------
// USER QUERY
// --------------------------------------------------------------------------

	if( result == SONORK_RESULT_OK && ID->start_mode == SONORK_APP_START_MODE_ASK )
	{
		// Query user
		ID->app_run_flags=win32.run_flags;

		DialogBoxParam(Instance()
			,MAKEINTRESOURCE(IDD_INITMODE)
			,NULL
			,InitMode_DlgProc
			,(LPARAM)ID);
		if(ID->start_mode == SONORK_APP_START_MODE_CANCEL)
		{
			delete ID;
			return false;
		}
	}

// --------------------------------------------------------------------------
// MODE AWARE EXECUTION STARTS
// --------------------------------------------------------------------------
// At this point START_QUERY *cannot* be set: We should already know what
//  mode we starting in because the  ConfigFileName(), str_Default() and
//  ConfigKeyName() functions we use from now on, return strings which
//  are set up according the the current mode.



	while(result==SONORK_RESULT_OK)
	{

		assert( ID->start_mode == SONORK_APP_START_MODE_INTRANET
		||	ID->start_mode == SONORK_APP_START_MODE_INTERNET);

		szSonorkAppMode = (ID->start_mode == SONORK_APP_START_MODE_INTRANET)?"in":"ex";

		if( ID->start_mode == SONORK_APP_START_MODE_INTRANET )
			SetCfgFlag( SONORK_WAPP_CF_INTRANET_MODE );
		else
		if( !(win32.run_flags&SONORK_WAPP_RF_ALLOW_INTERNET_MODE) )
		{
			result = SONORK_RESULT_INVALID_MODE;
			strcpy(err.CStr(),"Internet mode not allowed");

		}

		if(!GetCmdFlagValue( cmd_line,"-CfgFile"
			, win32.cfg_file,SONORK_APP_CONFIG_FILE_NAME_SIZE-4) )
		{
			// Command line does not specify an alternate
			// configuration file. See if we've got an INI
			// file that specifies one. If not, use default.
			strcpy(win32.cfg_file,szDefault);

		}
		wsprintf(cfg_file_full_name,"%s(%s)",win32.cfg_file,szSonorkAppMode);
		DbgOpen( cfg_file_full_name );


		wsprintf(win32.cfg_key, "CfgFile\\%s", cfg_file_full_name);

		if( currentUserKey.IsOpen() )
		{
			if(!TestRunFlag(SONORK_WAPP_RF_SETUP_INIT))
			{
				if(subKEY.Open(currentUserKey
					,ConfigKeyName()
					,false
					,KEY_READ)==ERROR_SUCCESS)
				{
#define init_app_user_id aux_str.CStr()	// Just to make sure we don't use aux_b for another thing

					if(subKEY.QueryValue(szGuId
						,init_app_user_id
						,48) == ERROR_SUCCESS )
					{
						ID->user_id.SetStr(init_app_user_id);
					}
					subKEY.Close();
#undef init_app_user_id
				}
			}

		}

		if( FindParallelInstance( ConfigName() ) )
		{
			// We've found another instance running with
			// our same configuration file: FindParallelInstance()
			// will tell the other instance's window to come to
			// the front instead of us while we silently exit.
			delete ID;
			return false;
		}

#define auxStr	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
		if(ExpandEnvironmentStrings(ID->data_dir, auxStr, SONORK_MAX_PATH))
			win32.data_dir.Set(auxStr);
		else
			win32.data_dir.Set(ID->data_dir);

		if(ExpandEnvironmentStrings(ID->temp_dir, auxStr , SONORK_MAX_PATH))
			win32.temp_dir.Set(auxStr);
		else
			win32.temp_dir.Set(ID->temp_dir);
#undef auxStr

		break;
	}

#define init_app_sys_db_path	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing

// Neel O Koon
	if( result == SONORK_RESULT_OK )
	{
		auxBool=true;
		GetDirPath(init_app_sys_db_path,SONORK_APP_DIR_TEMP,cfg_file_full_name);
		result = db.sys.Open( ProfileUserId() , init_app_sys_db_path , auxBool );
		if( result == SONORK_RESULT_OK )
		{
			console.sys = new TSonorkCCache(SYS_CONSOLE_TEXT_SIZE
				,SYS_CONSOLE_CACHE_SIZE
				,SysCCacheCallback
				,this);
			if(( result = console.sys->Open( init_app_sys_db_path ) ) == SONORK_RESULT_OK)
				console.sys->Clear( true );

		}
                
                if( result != SONORK_RESULT_OK )
                {
			wsprintf(err.CStr()
			,"Cannot open/create file '%s'"
			,init_app_sys_db_path
                        );
                }
	}
#undef init_app_sys_db_path



	if( result == SONORK_RESULT_OK )
	{
		result = OpenApp(
			 win32.data_dir.CStr()
			,cfg_file_full_name
			,SONORK_APP_VERSION_NUMBER
			,SONORK_SID_VF_SUPPORTS_EXT_SID
			|SONORK_SID_VF_SUPPORTS_EXT_MSG
			|SONORK_SID_VF_SUPPORTS_UTS
			,SONORK_OS_WIN
			,0
			);
		if(result != SONORK_RESULT_OK)
		{
			wsprintf(err.CStr()
			,"Cannot open configuration file '%s'"
			,cfg_file_full_name);
		}
	}

	UpdateDesktopSize();

	if( result == SONORK_RESULT_OK )
	{
		if( ID->flags & INIT_APP_F_SAVE_STARTUP_MODE )
		{
			if( currentUserKey.IsOpen() )
				currentUserKey.SetValue(szStartMode,ID->start_mode);
			if( defaultUserKey.IsOpen() )
				defaultUserKey.SetValue(szStartMode,ID->start_mode);
		}
		// -----------------------------------------------------
		// Network initialization

		Sonork_Net_Start(1,1);

		// -----------------------------------------------------
		// Temporal message console
		// used when the user's console is not open

		console.tmp.ptr = CreateMsgCache(8,8);
	}

	// -----------------------------------------------------
	// GDI initialization

	if( result == SONORK_RESULT_OK )
	{
		menus.global		= LoadMenu(Instance(),MAKEINTRESOURCE(IDM_GLOBAL));
		menus.main		= GetSubMenu( menus.global, CM_MAIN_MENU);
		menus.user_popup	= GetSubMenu( menus.global, CM_USER_CONTEXT_MENU );
		menus.user_apps		= GetSubMenu( menus.user_popup, CM_USER_CONTEXT_POPUP_APPS );
		menus.user_visib	= GetSubMenu( menus.user_popup, CM_USER_CONTEXT_POPUP_VISIB );
		menus.user_auth		= GetSubMenu( menus.user_popup, CM_USER_CONTEXT_POPUP_AUTH );	
		menus.user_mtpl		= GetSubMenu( menus.user_popup, CM_USER_CONTEXT_POPUP_MTPL );
		menus.msgs_popup	= GetSubMenu( menus.global, CM_MSGS_CONTEXT_MENU );
		menus.mfil_popup	= GetSubMenu( menus.global, CM_MFIL_CONTEXT_MENU );
		menus.ugrp_popup	= GetSubMenu( menus.global, CM_UGRP_CONTEXT_MENU );
		menus.usel_popup	= GetSubMenu( menus.global, CM_USEL_CONTEXT_MENU );
		menus.clip_popup	= GetSubMenu( menus.global, CM_CLIP_CONTEXT_MENU );
		menus.chat_view		= GetSubMenu( menus.global, CM_CHAT_VIEW_MENU );
		menus.chat_user		= GetSubMenu( menus.global, CM_CHAT_USER_MENU );
		menus.eapp_popup	= GetSubMenu( menus.global, CM_EAPP_CONTEXT_MENU );
		menus.tray_icon		= GetSubMenu( menus.global, CM_TRAY_MENU );
//		if( IntranetMode() )
		::DeleteMenu( menus.main , CM_TELL_A_FRIEND , MF_BYCOMMAND);
		::DeleteMenu( menus.user_mtpl, 0, MF_BYPOSITION);
		// -----------------------------------------------------
		// Skin initialization

#define init_app_icons_path	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
#define init_app_logo_path	err.CStr()
		GetDirPath(init_app_icons_path,SONORK_APP_DIR_SKIN,"default\\icons.bmp");
		GetDirPath(init_app_logo_path,SONORK_APP_DIR_SKIN,"default\\logo.bmp");
		sonork_skin.Initialize( Instance()
			, init_app_icons_path
			, init_app_logo_path
			, IntranetMode() );
#undef	init_app_logo_path
#undef 	init_app_icons_path


	}

	// -----------------------------------------------------
	// Windows initialization

	if( result == SONORK_RESULT_OK )
	{

		// -----------------------------------------------------
		// Sonork Win subsystem

		if( !TSonorkWin::InitModule(szSONORK) )
		{
			result=SONORK_RESULT_INTERNAL_ERROR;
			wsprintf(err.CStr()
				,"Cannot initialize Windows common controls\n"
				 "Is IE 4.0 properly installed?\n\n"
				 "No se pudo inicializar Controles para Windows\n"
				 "¿Está el Explorer 4.0 instalado correctamente?\n\n"
				,GetLastError());
		}
	}
	if( result == SONORK_RESULT_OK )
	{

		// -----------------------------------------------------
		// Drag Drop initialization

		SONORK_DragDropInit(WM_SONORK_DRAG_DROP);

		// Setup the invisible window that does all the work for the
		// application, it is used to generate the timer, delete/create
		// other windows and any other task that requires a window handle.
		win32.work_win=CreateWindow(
			 szSONORK
			,ConfigName()
			,0
			,0,0,0,0
			,NULL
			,NULL
			,SonorkApp.Instance()
			,NULL);

		if(win32.work_win==NULL)
		{
			result=SONORK_RESULT_INTERNAL_ERROR;
			wsprintf(err.CStr()
			,"Cannot create main application window\nError %d"
			,GetLastError());
		}
		else
		{
			SetWindowLong(win32.work_win, GWL_WNDPROC, (LONG)WinProc );
			SetWindowLong(win32.work_win, GWL_USERDATA, (LONG)this);
		}
	}



	// -----------------------------------------------------
	// Compression

	if( result == SONORK_RESULT_OK )
	{
#define zip_dll_path	aux_str.CStr()	// Just to make sure we don't use aux_b for another thing
		// LaBruja: a_zani@hotmail.com
		GetDirPath(zip_dll_path,SONORK_APP_DIR_BIN,"zlib.dll");
		win32.zip =  new TSonorkZip(zip_dll_path);
#undef	zip_dll_path
	}

	// -----------------------------------------------------
	// Services

	if( result == SONORK_RESULT_OK )
	{
		Init_Services();
		Init_Ipc();
	}


	// -----------------------------------------------------
	// Icons and Fonts


	if( result == SONORK_RESULT_OK )
	{

		// -----------------------------------------------------
		// Main Window

		Init_MainWin();

		win32.slide_win = new TSonorkSlideWin;
		win32.slide_win->Create();

		// -----------------------------------------------------
		// Default User

		Init_User( ID );

		LoadExtApps();

		RebuildMenus();


		if( !(ID->flags&INIT_APP_F_START_MINIMIZED) )
		{
			win32.main_win->ShowWindow(SW_SHOW);
			::SetForegroundWindow(win32.main_win->Handle());
		}
	}
	delete ID;

	if(result != SONORK_RESULT_OK )
	{
		MessageBox(NULL
			,err.CStr()
			,szSONORK
			,MB_OK|MB_SETFOREGROUND|MB_ICONSTOP);
		return false;
	}
	if( IsProfileOpen() )
	{
		if(!ProfileCtrlFlags().Test(SONORK_PCF_NO_AUTO_CONNECT))
		{
			win32.run_flags|=SONORK_WAPP_RF_CX_PENDING;
			task_timer.cx_status_msecs=START_CONNECT_DELAY_MSECS;
		}
	}
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Exit()
{
	if(TestRunFlag(SONORK_WAPP_RF_APP_TERMINATED))
		return;

	SetRunFlag(SONORK_WAPP_RF_APP_TERMINATED);
	Exit_Ipc();
	Exit_Services();
	ClearExtApps();

//----------------------------------------------------------------------------
// OLD IPC:
//----------------------------------------------------------------------------
//	if(win32.ipc_server!=NULL)
//	{ delete win32.ipc_server; win32.ipc_server=NULL; }

	if(win32.work_win!=NULL)
	{
		DestroyWindow(win32.work_win);
		win32.work_win=NULL;
	}
	if( win32.slide_win != NULL )
	{
		win32.slide_win->Destroy();
		win32.slide_win=NULL;
	}
	TSonorkWin::ExitModule();
	if(console.tmp.ptr!=NULL)
	{
		SONORK_MEM_DELETE(console.tmp.ptr);
		console.tmp.ptr=NULL;
	}
	if(console.sys!=NULL)
	{
		SONORK_MEM_DELETE(console.sys);
		console.sys=NULL;
	}
	db.sys.Close();
	CloseApp();
	Sonork_Net_Stop();

	if(menus.global != NULL)
		::DestroyMenu(menus.global);

	sonork_skin.Clear();
	SONORK_DragDropExit();

	if( LUAT.hInstance != NULL )
	{
		FreeLibrary(LUAT.hInstance);
		LUAT.hInstance		= NULL;
		LUAT.ptr		= NULL;
	}

	if( win32.zip != NULL )
	{
		delete win32.zip;
		win32.zip=NULL;
	}

	DbgClose();
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OpenMsgWindow(TSonorkExtUserData*UD, SONORK_MSG_WIN_OPEN_MODE open_mode)
{
	TSonorkWin *W;
	TSonorkCCache *MC;
	if(UD==NULL)return;
	W=GetUserMsgWin(UD->userId);

	if( W != NULL )
	{
		W->PostPoke( SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD , false );
		return;
	}
	MC=GrabSharedMsgCache( UD->userId );
	if(MC)
	{
		W=new TSonorkMsgWin(UD,MC);
		if(!W->Create())
                {
			TRACE_DEBUG("OpenMsgWindow(%s) CREATE FAILED",UD->display_alias.CStr());
                }
                else
                {
			W->PostPoke( SONORK_MSG_WIN_POKE_OPEN_INIT , open_mode );
                }
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OpenUserAuthReqWin(const TSonorkId& user_id)
{
	TSonorkWin *W;
	W=GetUserAuthReqWin(user_id);
	if( W == NULL )
	{
		W=new TSonorkAuthReqWin(user_id);
		W->Create();
		W->ShowWindow(SW_SHOW);
	}
	PostAppCommand(SONORK_APP_COMMAND_FOREGROUND_HWND
		,(LPARAM)W->Handle());
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OpenUserDataWin(
	  const TSonorkId& 	  userId
	, const TSonorkUserData*  puser
	, const TSonorkDynString* pnotes	// may be null
	, TSonorkWin*		  powner	// may be NULL
	, UINT tab)
{
	TSonorkUserDataWin *W;
	TSonorkExtUserData *ctx_user;
	DWORD		show_flags;
	bool		need_user_refresh;
	bool		need_note_refresh;
	if( userId == ProfileUserId() ) return;
	W=GetUserDataWin(userId);
	if( W == NULL )
	{
		if( puser )
		if( puser->GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1 )
			puser=NULL; // not valid
		ctx_user = SonorkApp.UserList().Get( userId );
		if( ctx_user != NULL )
		{
			show_flags=TSonorkUserDataWin::SF_USER_IN_LOCAL_LIST;

			if(ctx_user->UserType() == SONORK_USER_TYPE_AUTHORIZED )
				show_flags|=
					TSonorkUserDataWin::SF_USER_IN_AUTHORIZED_LIST;

			need_user_refresh=
			need_note_refresh=false;
		}
		else
		{
			show_flags=TSonorkUserDataWin::SF_DELETE_USER_DATA;

			SONORK_MEM_NEW( ctx_user=new TSonorkExtUserData(SONORK_USER_TYPE_UNKNOWN) );
			ctx_user->Clear();
			ctx_user->ClearExtData();
			if( puser )
			{
				ctx_user->Set(*puser);
				need_user_refresh=false;
				puser=NULL;	// don't copy again
			}
			else
			{
				ctx_user->userId.Set(userId);
				need_user_refresh=true;
			}
			ctx_user->display_alias.Set( ctx_user->alias.CStr() );
			need_note_refresh=true;
		}

		// If data provided, and not the same
		if( puser != NULL)
		if(((void*)puser) != ((void*)ctx_user))
		{
			ctx_user->alias.Set( puser->alias );
			ctx_user->name.Set( puser->name );
			// Does remote have email?
			if( puser->email.Length() )
				ctx_user->email.Set( puser->email );
			ctx_user->CopyUserInfo( puser );

			if( !(show_flags & TSonorkUserDataWin::SF_USER_IN_AUTHORIZED_LIST ) )
				ctx_user->addr.Set( puser->addr );
			need_user_refresh=false;
			show_flags|=
				TSonorkUserDataWin::SF_SAVE_NEW_USER_DATA;
		}
		if( pnotes != NULL )
		{
			show_flags|=
				TSonorkUserDataWin::SF_SAVE_NEW_USER_NOTES;
			need_note_refresh=false;
		}
		if( need_note_refresh || need_user_refresh )
		{
			show_flags|=TSonorkUserDataWin::SF_AUTO_REFRESH;
		}

		W=new TSonorkUserDataWin(
			  powner?powner:win32.main_win
			, ctx_user
			, pnotes
			,(TSonorkUserDataWin::TAB)tab
			, show_flags
			);
		if(!W->Create())
			delete W;
		else
		    W->ShowWindow(SW_SHOW);

		return;
	}
	PostAppCommand(SONORK_APP_COMMAND_FOREGROUND_HWND
		,(LPARAM)W->Handle());
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OpenUserDataWin(
	const TSonorkExtUserData*	UD
	, TSonorkWin*		  	powner	// may be NULL
	, UINT 				ptab)
{
	if( UD == NULL )return;
	// Make sure to set both <user> and <notes> to NULL
	// so that the there is no attempt to update current user data.
	OpenUserDataWin(UD->userId , NULL , NULL , powner, ptab);
}

// ----------------------------------------------------------------------------
void
 TSonorkWin32App::OpenUserDataWin(
	  const TSonorkUserData*	UD
	, const TSonorkDynString* 	pnotes	// may be null
	, TSonorkWin*		  	powner	// may be NULL
	, UINT 				ptab)
{
	if( UD == NULL )return;
	OpenUserDataWin(UD->userId , UD , pnotes , powner, ptab);

}

// ----------------------------------------------------------------------------
void
 TSonorkWin32App::OpenUserDataWin(
	  const TSonorkUserDataNotes*	UDN
	, TSonorkWin*		  	powner	// may be NULL
	, UINT 				ptab)
{
	if( UDN == NULL )return;
	OpenUserDataWin(&UDN->user_data,&UDN->notes,powner,ptab);
}

// ----------------------------------------------------------------------------

TSonorkWin*
 TSonorkWin32App::GetUserWin(const TSonorkId&user_id,SONORK_WIN_TYPE type)
{
	TSonorkListIterator I;
	TSonorkWin *		win;
	win32.win_list.BeginEnum(I);
	while( (win=win32.win_list.EnumNext(I)) !=NULL )
		if( win->WinType() == type )
		{
			if( win->IsUserId(user_id) )
				break;
		}
	win32.win_list.EndEnum(I);
	return win;
}

// ----------------------------------------------------------------------------

TSonorkMsgWin*
 TSonorkWin32App::GetUserMsgWin(const TSonorkId& user_id)	// May return NULL
{
	return (TSonorkMsgWin*)GetUserWin(user_id,SONORK_WIN_TYPE_SONORK_MSG);
}

// ----------------------------------------------------------------------------

TSonorkAuthReqWin*
 TSonorkWin32App::GetUserAuthReqWin(const TSonorkId&user_id)
{
	return (TSonorkAuthReqWin*)GetUserWin(user_id,SONORK_WIN_TYPE_AUTH_REQ);
}

// ----------------------------------------------------------------------------

TSonorkUserDataWin*
 TSonorkWin32App::GetUserDataWin(const TSonorkId&user_id)
{
	return (TSonorkUserDataWin*)GetUserWin(user_id,SONORK_WIN_TYPE_USER_DATA);
}

// ----------------------------------------------------------------------------

void SONORK_CALLBACK
 TSonorkWin32App::SonorkUtsRequestHandler(void *param, const SONORK_DWORD2& tag, const TSonorkUTSLink*, const TSonorkError*pERR)
{
	TSonorkWin32App *_this=(TSonorkWin32App*)param;
	if( tag.v[1] == (DWORD)-1 )
		_this->OnAppTaskResult( (TSonorkAppTask*)tag.v[0] , pERR , true);
}

// ----------------------------------------------------------------------------
// StartSonorkRequests takes ownership of <P> and deletes it

SONORK_RESULT
 TSonorkWin32App::StartSonorkRequest(
	 TSonorkDataPacket*	P
	,UINT 			P_size
	,const SONORK_DWORD2* 	tag
	,TSonorkError*		pERR)
{
	TSonorkError auxERR;
	if(!pERR)pERR=&auxERR;
	sonork.Request(*pERR,P,P_size,SonorkClientRequestHandler,this,tag);
	SONORK_FreeDataPacket( P );
	return pERR->Result();
}

// ----------------------------------------------------------------------------

void SONORK_CALLBACK
 TSonorkWin32App::SonorkClientRequestHandler(void*param,TSonorkClientRequestEvent*E)
{
	TSonorkWin32App *_this=(TSonorkWin32App*)param;

	switch( E->EventType() )
	{
		case SONORK_CLIENT_REQUEST_EVENT_PACKET:
			if( E->Packet()->Function() == SONORK_FUNCTION_IDENTIFY_USER)
			if( E->Packet()->SubFunction() == 0)
			if( _this->win32.uts_server != NULL )
			{
				TSonorkError 		ERR;
				TSonorkDataPacket *P=E->Packet();
				TSonorkUserData		UD;
				DWORD id_flags;
				if(P->D_IdentifyUser_A(E->PacketSize(),UD,id_flags))
				{
					if(SONORK_IDENTIFY_MATCH_OK( id_flags ) )
						ERR.SetOk();
					else
						ERR.SetSys(SONORK_RESULT_ACCESS_DENIED,GSS_BADID,id_flags);
				}
				else
					ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR,GSS_INTERNALERROR,0);
				_this->win32.uts_server->Authorize(E->RequestTag().v[0]
					,ERR);
			}
			break;

		case SONORK_CLIENT_REQUEST_EVENT_END:
			switch(E->RequestFunction())
			{
				case SONORK_FUNCTION_IDENTIFY_USER:
					if(E->ErrorInfo()->Result()!=SONORK_RESULT_OK)
					if( _this->win32.uts_server )
						_this->win32.uts_server->Authorize(
							E->RequestTag().v[0]
							,*E->ErrorInfo());
				break;

				default:
				if(E->RequestTag().v[1] == (DWORD)-1)
				{
					_this->OnAppTaskResult(
					  (TSonorkAppTask*)E->RequestTag().v[0]
					, E->ErrorInfo()
					, true);
				}
				break;
			}
			break;
	}
}

// ----------------------------------------------------------------------------

UINT
 TSonorkWin32App::GetMainViewSelection(SONORK_DWORD2List* list)
{
	return win32.main_win->GetSelection( list );
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkWin32App::AmIVisibleToUser(const TSonorkExtUserData*UD) const
{
	BOOL rv=true;
	SONORK_SID_MODE sm=ProfileSidFlags().SidMode();

	if(  	sm == SONORK_SID_MODE_INVISIBLE
	||	sm == SONORK_SID_MODE_INVISIBLE_02
	||	sm == SONORK_SID_MODE_INVISIBLE_03
	|| UD->UserType() != SONORK_USER_TYPE_AUTHORIZED
	|| UD->ctrl_data.auth.TestFlag(SONORK_AUTH_F1_DISCONNECTED))
		rv = false;
	else
	{
		if( ProfileSidFlags().IsPrivate() )
			if( !(UD->ctrl_data.auth.PrivateMask() & ProfileSidFlags().PrivateMask() ))
				rv = false;
	}
	return rv;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::LoadEmailAccounts(TSonorkEmailAccountQueue* Q)
{
	int	i,mi;
	TSonorkAtomDb			db;
	TSonorkEmailAccount* 	email_acc=NULL;
	if( OpenAppDb(SONORK_APP_DB_EMAIL_ACCOUNT , db , false ) )
	{
		mi = (int)db.Items();
		for(i=0;i<mi;i++)
		{
			if(email_acc == NULL )
				email_acc = new TSonorkEmailAccount ;
			if(db.Get(i,email_acc)!=SONORK_RESULT_OK)
				continue;
			Q->Add(email_acc);
			email_acc=NULL;
		}
		if( email_acc!=NULL )
			delete email_acc;
		db.Close();
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ShowUserDbMaintenance()
{
	BroadcastAppEvent(SONORK_APP_EVENT_MAINTENANCE,0,0,NULL);
	TSonorkDbMaintWin(win32.main_win,&db.msg,&db.ext).Execute();

}
// ----------------------------------------------------------------------------
/*class TDummy
{
	UINT n;
public:
	TDummy(UINT v);
	TDummy();
	~TDummy();
	void *operator new(size_t t);
	void operator delete(void*ptr);
	void *operator new[](size_t t);
	void operator delete[](void*ptr);
};
void *TDummy::operator new[](size_t t)
{
	void *ptr;
	ptr=SONORK_MEM_ALLOC(BYTE,t);
	TRACE_DEBUG("\\\\TDummy::operator new[](%u) = %x",t,ptr);
	return ptr;
}
void TDummy::operator delete[](void*ptr)
{
	TRACE_DEBUG("\\\\TDummy::operator delete[](%x)",ptr);
	SONORK_MEM_FREE(ptr);
}
void *TDummy::operator new(size_t t)
{
	void *ptr;
	ptr=SONOWK_MEM_ALLOC(BYTE,t);
	TRACE_DEBUG("\\\\TDummy::operator new(%u) = %x",t,ptr);
	return ptr;
}
void TDummy::operator delete(void*ptr)
{
	TRACE_DEBUG("\\\\TDummy::operator delete(%x)",ptr);
	SONORK_MEM_FREE(ptr);
}
TDummy::TDummy()
{
	TRACE_DEBUG("\\\\TDummy::TDummy(%x,0)",this);
	n=0;
}
TDummy::TDummy(UINT v)
{
	TRACE_DEBUG("\\\\TDummy::TDummy(%x,%u)",this,v);
	n=v;
}
TDummy::~TDummy()
{
	TRACE_DEBUG("\\\\TDummy::~TDummy(%x,%u)",this,n);
}
		TRACE_DEBUG("++TDummy ----------------------------");
		TDummy*D;
		TRACE_DEBUG("TEST:new TDummy(5)");
		D=new TDummy(5);
		TRACE_DEBUG("TEST:D=%x",D);
		delete D;
		TRACE_DEBUG("TEST:new TDummy[10]");
		D=new TDummy[10];
		for(int i=0;i<10;i++)
			TRACE_DEBUG("TEST:D[%u]=%x",i,D+i);
		delete[] D;
		TRACE_DEBUG("--TDummy ----------------------------");

JORGE ENCISO
CAPITAL CRISTALDO 464 - CASI INCAS
BARRIO INTERNACIONAL
CHOFERES DEL CHACO 2
  CUADRAS A LA MANO DERECHA
  UNA A LA DERECHA
  MEDIA

  021 554141
*/


/*
// USED TO BE IN TIMER_TASK_MONITOR FOR "AUTO_AWAY" FEATURE
// THAT DID NOT WORK DUE TO AN ERROR IN THE WIN32 SDK

//	AUTO_AWAY_MSECS
	SONORK_SID_MODE	sid_mode;
	TSonorkTime	aux_time;
	double		aux_double;
		while( LUAT.ptr != NULL && ProfileCtrlValue(SONORK_PCV_AUTO_AWAY_MINS)>0)
		{
			if( !MayStartGlobalTask() )
				break;
			aux_time.SetTime(LUAT.ptr);
			// Time ellapsed since <t2> to <this> (seconds)
			if(!win32.cur_time.DiffTime(aux_time,&aux_double))
				break;
			sid_mode=ProfileSidFlags().SidMode();
			aux_uint=(UINT)aux_double;
			if( aux_uint >= (ProfileCtrlValue(SONORK_PCV_AUTO_AWAY_MINS)*60) )
			{
				if( sid_mode == SONORK_SID_MODE_INVISIBLE
				||  sid_mode == SONORK_SID_MODE_AWAY
				||  sid_mode == SONORK_SID_MODE_AWAY_AUTO
				||  sid_mode == SONORK_SID_MODE_AWAY_PHONE
				||  sid_mode == SONORK_SID_MODE_AWAY_02
				||  sid_mode == SONORK_SID_MODE_AWAY_03
				||  sid_mode == SONORK_SID_MODE_AWAY_AUTO)
					break;

				win32.saved_auto_away_sid_mode = sid_mode;
				sid_mode=SONORK_SID_MODE_AWAY_AUTO;
			}
			else
			{
				if( sid_mode != SONORK_SID_MODE_AWAY_AUTO)
					break;
				sid_mode = win32.saved_auto_away_sid_mode;
			}
			SetSidMode( sid_mode );
			break;
		}
*/

