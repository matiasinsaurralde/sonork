#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkwinctrl.h"
#include "srkmainwin.h"
#include "srkmsgwin.h"
#include "srkinputwin.h"
#include "srksidmodewin.h"
#include "srkgrpmsgwin.h"
#include "srkaboutwin.h"
#include "srkuserdatawin.h"
#include "srkusearchwin.h"
#include "srkbicho.h"
#include "srknetcfgwin.h"
#include "srkmyinfowin.h"
#include "srkmaininfowin.h"
#include "srk_winregkey.h"
#include "srk_cfg_names.h"
#include "srk_url_codec.h"
#include "srk_stream_lr.h"
#include "srk_email_codec.h"


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

// ----------------------------------------------------------------------------
// DEFINITIONS & DECLARATIONS
// ----------------------------------------------------------------------------


#define MOVING_VIEW_ITEM	SONORK_WIN_F_USER_01
#define DELETING_VIEW_ITEM	SONORK_WIN_F_USER_02
#define DRAGGING_USER_DATA	SONORK_WIN_F_USER_03
#define INPUT_MODE		SONORK_WIN_F_USER_04
#define CHANGING_SID_MODE	SONORK_WIN_F_USER_05
#define PSEUDO_SONORK_SID_MODE_VISIBILITY	(0xffff)

#define INPUT_CMD_OPEN		1
#define INPUT_CMD_CLOSE		2
#define INPUT_CMD_EXEC		3
#define TAB_BUTTON_WIDTH	(SKIN_ICON_SW*2+8)
#define TAB_BUTTON_HEIGHT	(SKIN_ICON_SH+6)

#define GTV_FULL_HIT_FLAGS	(TVHT_ONITEM|TVHT_ONITEMINDENT|TVHT_ONITEMRIGHT)
#define GTV_ICON_HIT_FLAGS	(TVHT_ONITEMICON)

#define MAX_USER_MODE_SIZE	144
#define LEFT_DELTA		1
#define TAB_TOP_DELTA		(2)
#define TAB_LEFT_DELTA		(TAB_BUTTON_HEIGHT+4)
#define TAB_RIGHT_DELTA		(2)
#define TAB_BOTTOM_DELTA	(2)

#define TAB_TOP_MARGIN		(2)
#define TAB_BOTTOM_MARGIN	(2)
#define MODE_BOTTOM_MARGIN	(2)
#define INFO_BOTTOM_MARGIN	(0)
#define TAB_TITLE_HEIGHT	(15)

// ----------------------------------------------------------------------------

enum POP_STATE
{
	POP_DISCONNECTED
,	POP_CONNECT
,	POP_USER
,	POP_PASS
,	POP_STAT
,	POP_UIDL_REQ
,	POP_UIDL_LIST
,	POP_TOP_REQ
,	POP_TOP_LIST
,	POP_ERROR
,	POP_QUIT
};
#define POP_BUFFER_SIZE		512 // 1024
#define POP_TIMER_MSECS		250 // 100
#define POP_IDLE_SECS		60



// ----------------------------------------------------------------------------

struct TSonorkPopCheckState
{
	POP_STATE 			state;
	bool				iterating;
	bool				session_complete;
	UINT 				top_count;
	TSonorkRawTcpEngine 		tcp_eng;
	TSonorkClock			tcp_clock;
	TSonorkStreamLineReader 	line_reader;
	TSonorkEmailAccount*		cur_acc;
	TSonorkListIterator		acc_iterator;
	TSonorkEmailAccountQueue	acc_queue;
	TSonorkEmailAccountQueue	not_queue;
	FILE*				file;
	TSonorkShortString		file_path;
	HANDLE				resolve_handle;
	DWORD				c_msg_no;
	TSonorkShortString		c_msg_subject;
	TSonorkShortString		c_msg_to;
	TSonorkShortString		c_msg_from;
	TSonorkShortString		c_first_msg_uidl;
	TSonorkShortString		c_last_msg_uidl;
};


// ----------------------------------------------------------------------------

// TOOL_BAR_BASE_ID
//  The toolbar buttons are asigned IDs that are equal to
//  their curresponing menu command (CM_xx value) plus TOOL_BAR_BASE_ID
//  The menu CM_xxx values are defined in the resource
//  or in the MENU_COMMAND enumeration in this module
#define TOOL_BAR_BASE_ID	1000
#define TOOL_BAR_BUTTONS	5
#define TOOL_BAR_ID		500
#define TOOL_BAR_ICON_SW	SKIN_ICON_SW
#define TOOL_BAR_ICON_SH	SKIN_ICON_SH

#define TOOL_BAR_AREA_HEIGHT	SONORK_ICON_SH
#define TOOL_BAR_STYLE (WS_CHILD | WS_VISIBLE \
			| TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_LIST\
			| CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_NORESIZE)
#define AUTH_MENU_ITEMS	7
static struct
{
	SONORK_AUTH_FLAG0	auth_flag;
	UINT			id;
}visibility_xlat[SONORK_APP_VISIBILITY_GROUPS]=
{{SONORK_AUTH_F0_PRIVATE_1	,CM_VISIBILITY_01}
,{SONORK_AUTH_F0_PRIVATE_2	,CM_VISIBILITY_02}
,{SONORK_AUTH_F0_PRIVATE_3	,CM_VISIBILITY_03}
,{SONORK_AUTH_F0_PRIVATE_4	,CM_VISIBILITY_04}
,{SONORK_AUTH_F0_PRIVATE_5	,CM_VISIBILITY_05}
,{SONORK_AUTH_F0_PRIVATE_6	,CM_VISIBILITY_06}
,{SONORK_AUTH_F0_PRIVATE_7	,CM_VISIBILITY_07}
};
static struct
{
	SONORK_AUTH_FLAG1	auth_flag;
	UINT			id;
}auth_xlat[AUTH_MENU_ITEMS]=
{{SONORK_AUTH_F1_HIDE_EMAIL	,CM_AUTH_NEMAIL}
,{SONORK_AUTH_F1_HIDE_ADDR	,CM_AUTH_NADDR}
,{SONORK_AUTH_F1_BUSY		,CM_AUTH_BUSY}
,{SONORK_AUTH_F1_NOT_BUSY	,CM_AUTH_NBUSY}
,{SONORK_AUTH_F1_FRIENDLY	,CM_AUTH_FRIENDLY}
,{SONORK_AUTH_F1_NOT_FRIENDLY	,CM_AUTH_NFRIENDLY}
,{SONORK_AUTH_F1_DISCONNECTED	,CM_AUTH_NCX}
};



// ----------------------------------------------------------------------------
// ASYNC TASKS
// ----------------------------------------------------------------------------


void
  TSonorkMainWin::OnSonorkTaskData( const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size )
{
	if( task.type == TASK_SET_USER )
	{
		OnSetUserTaskData(P,P_size);
		return;
	}
	if( P->Function() == SONORK_FUNCTION_SET_GROUP )
	{
		OnSetGroupTaskData(P,P_size);
		return;
	}

	if( task.type == TASK_REFRESH_PROFILE )
	{
		SonorkApp.ProcessUserProfileData(P,P_size);
		return;
	}

}
void
  TSonorkMainWin::OnSetGroupTaskData(TSonorkDataPacket*P, UINT P_size)
{
	TSonorkGroup UG;
	if( task.type == TASK_ADD_GROUP )
	{
		if( P->SubFunction() == SONORK_SUBFUNC_SET_GROUP_ADD )
		if( P->D_AddGroup_A(P_size,&UG) )
		{
			SonorkApp.OnSonorkAddGroup(&UG);
		}
	}
	else
	if( task.type == TASK_SET_GROUP )
	{
		if( P->SubFunction() == SONORK_SUBFUNC_SET_GROUP_SET )
		if( P->D_SetGroup_A(P_size,&UG) )
		{
			SonorkApp.OnSonorkSetGroup(&UG);
		}
	}
	else
	if( task.type == TASK_DEL_GROUP )
	{
		SONORK_GROUP_TYPE	group_type;
		if( P->SubFunction() == SONORK_SUBFUNC_SET_GROUP_DEL )
		if( P->D_DelGroup_A(P_size,&group_type,&UG.header.group_no) )
		{
			SonorkApp.OnSonorkDelGroup(group_type,UG.header.group_no);
		}
	}
}

void
  TSonorkMainWin::OnSetUserTaskData(TSonorkDataPacket*P, UINT P_size)
{
	TSonorkExtUserData	*UD;
	UINT set_user_flags=0;
	TSonorkAuth2		auth;
	TSonorkId			user_id;
	if( P->Function() == SONORK_FUNCTION_SET_AUTH )
	if( P->SubFunction() == SONORK_SUBFUNC_NONE )
	{
		if(P->D_SetAuth_A(P_size,user_id,auth))
		if((UD=SonorkApp.GetUser(user_id))!=NULL)
		{
			UD->ctrl_data.auth.Set( auth );
			SonorkApp.SaveUser( user_id );
			// When we store info, we can only modify the AUTH
			// all other attributes are controlled by the owner
			// We generated the event for SET_F_DISP_ALIAS and SET_F_L_NOTES
			// just before we start the Task, in  CmdStore()
			set_user_flags = SONORK_APP_EVENT_SET_USER_F_AUTH_FLAGS;
		}
		if(set_user_flags != 0)
			SonorkApp.BroadcastAppEvent_SetUser( UD,   set_user_flags);
	}
}

void
  TSonorkMainWin::OnTaskEnd(SONORK_WIN_TASK_TYPE
	, const SONORK_DWORD2&
	, const TSonorkError*ERR)
{
	task.ERR.Set( *ERR );
	if(task.ERR.Result() != SONORK_RESULT_OK)
		SonorkApp.SetBichoSequenceError(false);
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, task.type);
}

SONORK_RESULT
  TSonorkMainWin::Task_AddUser(
	const TSonorkId& 	user_id
	,const TSonorkText& 	text)
{
	UINT			A_size,P_size;
	TSonorkDataPacket*	P;
	TSonorkAuth2    	auth;

	A_size = text.CODEC_Size() + 64;
	auth.tag = 0;
	auth.flags.Clear();
	auth.flags.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
	auth.pin = SonorkApp.GenTrackingNo( user_id );

	P = SONORK_AllocDataPacket( A_size );
	P_size = P->E_ReqAuth_R(A_size,user_id,auth,text);
	return Task_Start(TASK_DEL_USER
		, P
		, P_size
		, GLS_TK_ADDUSR
		, 0);
}


SONORK_RESULT
  TSonorkMainWin::Task_SetUserAuth(
	  const TSonorkExtUserData* UD
	, const TSonorkAuth2& auth)
{
#define A_size	128
	TSonorkDataPacket	*P;
	UINT			P_size;

	P = SONORK_AllocDataPacket( A_size );
	P_size = P->E_SetAuth_R(A_size,UD->userId,auth);
	return Task_Start(TASK_SET_USER
		, P
		, P_size
		, GLS_TK_PUTINFO
		, 0);
#undef A_size
}
SONORK_RESULT
  TSonorkMainWin::Task_DelUser( const TSonorkId& user_id )
{
#define A_size	64
	UINT			P_size;
	TSonorkDataPacket*	P;
	P=SONORK_AllocDataPacket(A_size);
	P_size = P->E_DelAuth_R(A_size, user_id );
	return Task_Start(TASK_DEL_USER
		, P
		, P_size
		, GLS_TK_DELUSR
		, 0);
#undef A_size
}

SONORK_RESULT
  TSonorkMainWin::Task_AssignGroupToUser(
	 const TSonorkExtUserData*UD
	,DWORD group_no)
{
	TSonorkAuth2  auth;
	group_no&=SONORK_AUTH_TAG_GROUP_MASK;
	auth.Set(UD->ctrl_data.auth);
	if(auth.GetGroupNo() == group_no)
		return SONORK_RESULT_OK;
	auth.SetGroupNo( group_no );
	return Task_SetUserAuth( UD, auth );
}


SONORK_RESULT
  TSonorkMainWin::Task_RefreshUserProfile( DWORD taskFlags )
{
#define A_size	128
	TSonorkDataPacket	*P;
	UINT			P_size;

	P = SONORK_AllocDataPacket( A_size );
	P_size = P->E_GetUserData_R(A_size
			,SonorkApp.ProfileUserId()
			,SONORK_USER_INFO_MAX_LEVEL
			,0
			,0
			,0);
	return Task_Start(TASK_REFRESH_PROFILE
		, P
		, P_size
		, GLS_TK_GETINFO
		, taskFlags
		);
#undef A_size
}

SONORK_RESULT
  TSonorkMainWin::Task_SetGroup(TSonorkGroup*UG,bool add)
{
	TSonorkDataPacket	*P;
	UINT			A_size,P_size;

	A_size = ::CODEC_Size(UG) + 32;
	P = SONORK_AllocDataPacket( A_size );
	if(add)
		P_size = P->E_AddGroup_R(A_size,UG);
	else
		P_size = P->E_SetGroup_R(A_size,UG);
	return Task_Start(
		add?TASK_ADD_GROUP:TASK_SET_GROUP
		, P
		, P_size
		, GLS_TK_PUTINFO
		, 0);
}

SONORK_RESULT
  TSonorkMainWin::Task_DelGroup(SONORK_GROUP_TYPE type,DWORD group_no)
{
	TSonorkDataPacket	*P;
	UINT			P_size;
#define A_size	64
	P = SONORK_AllocDataPacket( A_size );
	P_size = P->E_DelGroup_R(A_size,type,group_no);
	return Task_Start( TASK_DEL_GROUP , P , P_size , GLS_TK_PUTINFO , 0 );
#undef A_size
}

SONORK_RESULT
  TSonorkMainWin::Task_Start(TASK_TYPE tt
	,TSonorkDataPacket*P
	,UINT P_size
	,GLS_INDEX gls
	,DWORD taskFlags)
{
	StartSonorkTask(task.ERR
		, P
		, P_size
		, taskFlags
		, gls
		, NULL);
	SONORK_FreeDataPacket( P );
	if( task.ERR.Result() == SONORK_RESULT_OK )
	{
		task.type =tt;
		Task_UpdateStatus();
	}
	return task.ERR.Result();
}


void
  TSonorkMainWin::Task_UpdateStatus()
{
	SONORK_C_CSTR	str;
	if( task.type != TASK_NONE )
	{
		str = SonorkApp.LangString(GLS_MS_PWAIT);
		SonorkApp.Set_UI_Event_TaskStart( str, 0 );
	}
	else
	{
		SonorkApp.Set_UI_Event_TaskEnd( &task.ERR, NULL , 0 );
	}
}


UINT
  TSonorkMainWin::Pop_AddNotifications(bool abort)
{
	TSonorkEmailAccount*	acc;
	TSonorkEmailExceptQueue	exc_queue;
	UINT		 	not_count=0;
	TSonorkShortString	path;


	if( task.pop->not_queue.Items() )
	{
		// All items are removed from <not_queue> so that the queue does not
		// attempt to delete them when destroyed. (items are deleted
		// when < pop->acc_queue > is destroyed).
		// This should be done regardless of whether we abort or not.
		while( (acc=task.pop->not_queue.RemoveFirst()) != NULL )
		{
			SonorkApp.GetTempPath(task.pop->file_path
				, "pop"
				, NULL
				, acc->UID());
			if(!abort)
			{
				Pop_LoadNotificationExceptions(&exc_queue);
				not_count+=Pop_AddAccountNotifications(&exc_queue);
			}
			DeleteFile( task.pop->file_path.CStr() );
		}
	}
	return not_count;
}
void
  TSonorkMainWin::Pop_LoadNotificationExceptions(TSonorkEmailExceptQueue* exc_queue)
{
	TSonorkAtomDb		db;
	int			i,mi;
	TSonorkEmailExcept*  	email_exc=NULL;

	if( SonorkApp.OpenAppDb(SONORK_APP_DB_EMAIL_EXCEPT , db , false ) )
	{
		mi = (int)db.Items();
		for(i=0;i<mi;i++)
		{
			if(email_exc == NULL )
				SONORK_MEM_NEW( email_exc = new TSonorkEmailExcept );
			if(db.Get(i,email_exc)!=SONORK_RESULT_OK)continue;
			exc_queue->Add(email_exc);
			email_exc=NULL;
		}
		if( email_exc!=NULL )
			SONORK_MEM_DELETE(email_exc);
		db.Close();
	}
}



UINT
  TSonorkMainWin::Pop_AddAccountNotifications(TSonorkEmailExceptQueue* exc_queue)
{
	enum POP_FILE_LINE
	{
		POP_FILE_LINE_FROM
	,	POP_FILE_LINE_TO
	,	POP_FILE_LINE_SUBJECT
	};
	FILE *	file;
	TSonorkTempBuffer	buffer(384);
	POP_FILE_LINE		file_line;
	int			len;
	BOOL			may_notify;
	TSonorkShortString& 	m_to	  	= task.pop->c_msg_to;
	TSonorkShortString& 	m_from	  	= task.pop->c_msg_from;
	TSonorkShortString& 	m_subject	= task.pop->c_msg_subject;
	UINT			not_count=0;

	file = fopen(task.pop->file_path.CStr(),"rt");
	if(!file)return 0;
	file_line = POP_FILE_LINE_FROM;
	while( fgets(buffer, SONORK_MAX_PATH-1, file) != NULL )
	{
		len = strlen(buffer.CStr());
		if(len>0)
			if(*(buffer.CStr()+len-1)=='\n')
			{
				len--;
				*(buffer.CStr()+len)=0;
			}

		if( file_line == POP_FILE_LINE_FROM )
		{
			if(*buffer.CStr()!='+')break;
			file_line = POP_FILE_LINE_TO;
			m_from.Set( buffer.CStr() + 1);
		}
		else
		if( file_line == POP_FILE_LINE_TO )
		{
			m_to.Set( buffer.CStr() );
			file_line = POP_FILE_LINE_SUBJECT;
		}
		else
		if( file_line == POP_FILE_LINE_SUBJECT )
		{
			may_notify = !exc_queue->Contains(m_from.CStr(), m_to.CStr(), buffer.CStr() );
			if( SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_IGNORE_EMAILS) )
				may_notify=!may_notify;

			if( may_notify )
			{
				m_subject.Set( buffer.CStr() );
				wsprintf(buffer.CStr()
					, "%-.16s: %-.56s\r\n%-.16s: %-.56s\r\n%-.16s: %-.144s"
					, SonorkApp.LangString(GLS_MSG_FROM)
					, m_from.CStr()
					, SonorkApp.LangString(GLS_MSG_TO)
					, m_to.CStr()
					, SonorkApp.LangString(GLS_MSG_SUBJ)
					, m_subject.CStr() );
				SonorkApp.Set_UI_Event(SONORK_UI_EVENT_INCOMMING_EMAIL
					,buffer.CStr()
					,not_count==0
					
					?SONORK_UI_EVENT_F_BICHO
					|SONORK_UI_EVENT_F_SOUND
					|SONORK_UI_EVENT_F_LOG
					|SONORK_UI_EVENT_F_LOG_AS_UNREAD

					:SONORK_UI_EVENT_F_LOG
					|SONORK_UI_EVENT_F_LOG_AS_UNREAD
					);
//				SonorkApp.SysLog(SONORK_UI_EVENT_INCOMING_EMAIL , SONORK_APP_CCF_UNREAD , buffer.CStr());
				not_count++;
			}
			file_line = POP_FILE_LINE_FROM;
		}
	}
	fclose(file);
	return not_count;
}


void
  TSonorkMainWin::Pop_SaveAccounts()
{
	TSonorkEmailAccount* 	acc;
	TSonorkAtomDb	  	db;

	assert( task.pop != NULL );
	if( SonorkApp.OpenAppDb(SONORK_APP_DB_EMAIL_ACCOUNT , db , true ) )
	{
		while( (acc = task.pop->acc_queue.RemoveFirst()) != NULL )
		{
			db.Add(acc);
			delete acc;
		}
		db.Close();
	}

}

void
  TSonorkMainWin::Pop_ProcessLine(SONORK_C_CSTR str)
{
	TSonorkTempBuffer	buffer(256);

	*buffer.CStr()=0;
	if(    task.pop->state != POP_TOP_LIST
	    && task.pop->state != POP_UIDL_LIST
	    && task.pop->state != POP_QUIT)
	{
		if(*str!='+')
		{
			SonorkApp.LangSprintf(buffer.CStr()
					, GLS_EM_CHKERR
					, task.pop->cur_acc->Name()
					, str);
			SonorkApp.Set_UI_Event(SONORK_UI_EVENT_ERROR
					,buffer.CStr()
					,0);
			task.pop->state = POP_ERROR;
		}
	}
	switch(task.pop->state)
	{

		case POP_CONNECT:
			wsprintf(buffer.CStr(),"USER %s"
				,task.pop->cur_acc->str[SONORK_EMAIL_ACCOUNT_STR_LOGIN_NAME].CStr()
				);
			task.pop->state = POP_USER;
			break;

		case POP_USER:
			wsprintf(buffer.CStr(),"PASS %s"
				,task.pop->cur_acc->str[SONORK_EMAIL_ACCOUNT_STR_LOGIN_PASS].CStr()
			);
			task.pop->state = POP_PASS;
			break;

		case POP_PASS:
			wsprintf(buffer.CStr(),"STAT");
			task.pop->state = POP_STAT;
			break;

		case POP_STAT:
			task.pop->top_count = Pop_ProcessValue( str );
			if( task.pop->top_count == 0)
			{
				task.pop->c_last_msg_uidl.Set(NULL);
				task.pop->c_first_msg_uidl.Set(NULL);
				task.pop->session_complete = true;
				Pop_StartQuit(buffer.CStr());
				break;
			}
			Pop_StartUIDL(buffer.CStr());
			break;

		case POP_UIDL_REQ:
			task.pop->state = POP_UIDL_LIST;
			break;

		case POP_TOP_REQ:
			task.pop->state = POP_TOP_LIST;
			break;

		case POP_UIDL_LIST:
			if(*str=='.' && *(str+1)==0)
			{
				Pop_StartNextTopList( buffer );
			}
			else
			{
				Pop_ProcessUIDL( str );
			}
			break;

		case POP_TOP_LIST:
			if(*str=='.' && *(str+1)==0)
			{
				Pop_StartNextTopList( buffer );
			}
			else
			{
				Pop_ProcessTopList(str);
			}
			break;

		default:
			break;

		case POP_ERROR:
			strcpy(buffer.CStr(),"QUIT");
			task.pop->state=POP_QUIT;
			break;


		case POP_QUIT:
			task.pop->state=POP_DISCONNECTED;
			break;
	}

	if(*buffer.CStr())
	{
		strcat(buffer.CStr(),"\r\n");
		task.pop->tcp_eng.Send( buffer.CStr() , strlen(buffer.CStr()) );
	}


}

UINT
  TSonorkMainWin::Pop_ProcessValue(SONORK_C_CSTR str)
{
	char *eol,tmp[48];
	lstrcpyn(tmp,str+4,10);
	eol=strchr(tmp,' ');
	if(!eol)return 0;
	*eol=0;
	return strtoul(tmp,NULL,10);
}

void
  TSonorkMainWin::Pop_StartUIDL(SONORK_C_STR tmp)
{
	strcpy(tmp,"UIDL");
	task.pop->c_first_msg_uidl.Clear();
	task.pop->c_last_msg_uidl.Clear();
	task.pop->c_msg_no 	= 0;
	task.pop->state 		= POP_UIDL_REQ;
}

void
  TSonorkMainWin::Pop_ProcessUIDL(SONORK_C_CSTR str)
{
	UINT msg_no;
	const char *uidl;
	char tmp[24];


	uidl=strchr(str,' ');
	if(!uidl)return;

	lstrcpyn( tmp , str , 12 );
	tmp[uidl - str]=0;
	msg_no = strtoul(tmp,NULL,10);
	uidl++;


	while( msg_no == 1 )
	{
		// First UIDL
		if( !stricmp( uidl , task.pop->cur_acc->FirstUIDL() ) )
		{
			if( task.pop->cur_acc->LastMsgNo() > 1
			    &&
			    task.pop->cur_acc->LastMsgNo() <= task.pop->top_count )
			{
				// First UIDL is unchanged: We can start the next phase
				// (TOP LIST) at the previous last message
				task.pop->c_msg_no = task.pop->cur_acc->LastMsgNo();
			}
			break;
		}
		task.pop->cur_acc->SetFirstUIDL( uidl );
		break;
	}
	if( msg_no > task.pop->c_msg_no )
	if( !stricmp( uidl , task.pop->cur_acc->LastUIDL() ) )
	{
		// Found UIDL of the previous last message: We can start the next phase
		// (TOP LIST) at this message position.
		task.pop->c_msg_no = msg_no;
	}

	if( msg_no == task.pop->top_count )
	{
		// This is the last UIDL, store it and if everything goes fine
		// set the account's last UIDL to this one.
		task.pop->c_last_msg_uidl.Set( uidl );
	}
}

void
  TSonorkMainWin::Pop_StartNextTopList( SONORK_C_STR buffer )
{
	if(task.pop->state == POP_TOP_LIST)
	{
		Pop_StopTopList();
	}
	if( task.pop->c_msg_no++ < task.pop->top_count )
	{
		task.pop->c_msg_to.Clear();
		task.pop->c_msg_from.Clear();
		task.pop->c_msg_subject.Set(SonorkApp.LangString(GLS_LB_NA));
		wsprintf(buffer,"TOP %u 0",task.pop->c_msg_no);
		task.pop->state = POP_TOP_REQ;

	}
	else
	{
		task.pop->session_complete = true;
		Pop_StartQuit( buffer );
	}
}


void
  TSonorkMainWin::Pop_ProcessTopList(SONORK_C_CSTR str)
{
	if(!strnicmp(str,"From: ",6))
	{
		task.pop->c_msg_from.Set(str+6);
	}
	else
	if(!strnicmp(str,"To: ",4))
	{
		task.pop->c_msg_to.Set(str+4);
	}
	else
	if(!strnicmp(str,"Subject: ",9))
	{
		task.pop->c_msg_subject.Set(str+9);
	}
	else
		return;
}

void
  TSonorkMainWin::Pop_StopTopList()
{
	assert( task.pop->state == POP_TOP_LIST );
	task.pop->cur_acc->header.flags|=SONORK_EMAIL_ACCOUNT_F_PENDING;
	fprintf(task.pop->file,"+%s\n",task.pop->c_msg_from.CStr());
	fprintf(task.pop->file,"%s\n",task.pop->c_msg_to.CStr());
	fprintf(task.pop->file,"%s\n",task.pop->c_msg_subject.CStr());
}

void
 TSonorkMainWin::Pop_StartQuit(SONORK_C_STR buffer)
{
	strcpy(buffer,"QUIT");
	task.pop->state = POP_QUIT;
}

void
 TSonorkMainWin::OnTimer( UINT id )
{
	TSonorkShortString*	str;
	if( task.pop != NULL )
	{
		id = task.pop->tcp_eng.Recv( 0 );
		if( id != 0)
		{
			task.pop->line_reader.Append(
				(SONORK_C_CSTR)task.pop->tcp_eng.Buffer()
				,id);
			task.pop->tcp_clock.Set( SonorkApp.CurrentClock() );
			id = 0;
		}
		else
			id = SonorkApp.CurrentClock().IntervalSecsAfter(task.pop->tcp_clock);

		while( (str=task.pop->line_reader.RemoveLine()) != NULL )
		{
			Pop_ProcessLine(str->CStr());
			delete str;
		}

		if( id < POP_IDLE_SECS
			&& task.pop->tcp_eng.Status()!=SONORK_NETIO_STATUS_DISCONNECTED
			&& task.pop->state != POP_DISCONNECTED)
			return;

		PostPoke( SONORK_MAINWIN_POKE_CHECK_MAIL  , 0 );
	}
	else
		KillTimer( id );

}


bool
 TSonorkMainWin::Pop_Connect(const TSonorkPhysAddr& phys_addr)
{
	if( task.pop->tcp_eng.Connect( phys_addr ) != 0)
	{
		return false;
	}
	task.pop->top_count 		= 0;
	task.pop->state 			= POP_CONNECT;
	task.pop->line_reader.Clear();
	task.pop->tcp_clock.Set( SonorkApp.CurrentClock() );
	SonorkApp.GetTempPath(task.pop->file_path
		, "pop"
		, NULL
		, task.pop->cur_acc->UID());
	task.pop->file = fopen(task.pop->file_path.CStr(),"wt");

	// We continue in OnTimer()
	SetAuxTimer( POP_TIMER_MSECS );

	return true;
}

void
 TSonorkMainWin::Pop_Start()
{
	char		*c,tmp[128];
	DWORD		port;
	assert( task.pop != NULL);
	Pop_Stop( false );
	assert( task.pop->iterating && task.pop->cur_acc == NULL );

	for(;;)
	{
		task.pop->cur_acc=task.pop->acc_queue.EnumNext( task.pop->acc_iterator );
		if( task.pop->cur_acc == NULL )
			break;
		if( task.pop->cur_acc->header.flags & SONORK_EMAIL_ACCOUNT_F_CHECK_MAIL)
			break;
	}
	if( !task.pop->cur_acc )
		Task_StopMailCheck( false );
	else
	{
		task.pop->session_complete= false;
		task.pop->cur_acc->header.flags&=~SONORK_EMAIL_ACCOUNT_F_PENDING;
		lstrcpyn(tmp
			,task.pop->cur_acc->str[SONORK_EMAIL_ACCOUNT_STR_INCOMMING_SERVER].CStr()
			,sizeof(tmp));
		c=strchr(tmp,':');
		if( c )
		{
			*c++=0;
			port=strtoul(c,NULL,10);
		}
		else
			port=110;


		task.pop->resolve_handle = SonorkApp.AsyncResolve(
			  task.ERR
			, Handle()
			, SONORK_PHYS_ADDR_TCP_1
			, tmp
			, port
			);
		if( task.pop->resolve_handle == NULL )
		{
			// Resolution failed: Do next account
			PostPoke(SONORK_MAINWIN_POKE_CHECK_MAIL,0);
		}
		// else: App is resolving, we'll continue
		// when
	}
}

void
 TSonorkMainWin::Pop_Stop( bool abort)
{
	bool	delete_file=true;
	assert( task.pop!=NULL );
	if( task.pop->cur_acc != NULL )
	{
		if( task.pop->session_complete && !abort)
		{
			task.pop->cur_acc->SetLastMsgNo( task.pop->top_count );
			task.pop->cur_acc->SetLastUIDL( task.pop->c_last_msg_uidl.CStr() );
			if( task.pop->cur_acc->header.flags&SONORK_EMAIL_ACCOUNT_F_PENDING)
			{
				task.pop->not_queue.Add( task.pop->cur_acc );
				delete_file=false;
			}
		}
		if( task.pop->resolve_handle != NULL )
		{
			SonorkApp.CancelAsyncResolve( Handle() );
			task.pop->resolve_handle=NULL;
		}
		KillAuxTimer();
		if(task.pop->file)
		{
			fprintf(task.pop->file,"-\n");
			fclose(task.pop->file);
			task.pop->file=NULL;
			if(delete_file)
			{
				::DeleteFile(task.pop->file_path.CStr());
			}
		}
		task.pop->line_reader.Clear();
		task.pop->tcp_eng.Shutdown();
		task.pop->cur_acc=NULL;
		task.pop->state = POP_DISCONNECTED;
	}
}


bool
 TSonorkMainWin::Task_StartMailCheck()
{
	if( MultiTestWinSysFlags(SONORK_WIN_SF_DESTROYING|SONORK_WIN_SF_DESTROYED)
		|| SonorkApp.GetSysDialog(SONORK_SYS_DIALOG_MY_INFO) != NULL)
		return false;
	if( task.pop != NULL )
		return true;



	task.pop = new TSonorkPopCheckState;
	task.pop->state 	= POP_DISCONNECTED;
	task.pop->cur_acc	= NULL;
	task.pop->file 	= NULL;
	task.pop->resolve_handle = NULL;

	SonorkApp.LoadEmailAccounts(&task.pop->acc_queue);
	if( !task.pop->acc_queue.Items() )
	{
		task.pop->iterating=false;
		Task_StopMailCheck( true );
	}
	else
	{
		task.pop->tcp_eng.SetRecvBufferSize(POP_BUFFER_SIZE);
		task.pop->acc_queue.BeginEnum( task.pop->acc_iterator );
		task.pop->iterating=true;
		PostPoke( SONORK_MAINWIN_POKE_CHECK_MAIL, 0 );
	}
	return true;

}

void
 TSonorkMainWin::Task_StopMailCheck(bool abort)
{
	if( task.pop != NULL )
	{
		Pop_Stop( abort );
		if( task.pop->iterating )
			task.pop->acc_queue.EndEnum( task.pop->acc_iterator );

		Pop_AddNotifications( abort );

		if( !abort )
		{
			Pop_SaveAccounts();
		}
		delete task.pop;
		task.pop = NULL;
	}
}



// ----------------------------------------------------------------------------
// 		APPLICATION EVENTS
// ----------------------------------------------------------------------------
bool
 TSonorkMainWin::OnAppEvent(UINT event, UINT wParam, void*event_data )
{
	TSonorkViewItemPtrs	VP;
	switch( event )
	{
		case SONORK_APP_EVENT_SET_LANGUAGE:
			//
			// LoadMenuLabels();
			::InvalidateRect(sid_mode_combo.hwnd,NULL,true);
			GetActiveTree().InvalidateRect(NULL,true);
			// Force redraw of the hint window
			InfoWin()->ClearEvent( true );
			return true;

		case SONORK_APP_EVENT_MAINTENANCE:
		case SONORK_APP_EVENT_SHUTDOWN:
			return true;// processed: Don't do default processing

		case SONORK_APP_EVENT_SKIN_COLOR_CHANGED:
			UpdateSkinDependantItems();
			return true;
			
		case SONORK_APP_EVENT_CX_STATUS:
			if( !SonorkApp.IsPseudoCxStatus() )
				OnAppEventCxStatus(wParam);
			return true;
		case SONORK_APP_EVENT_SET_USER:
			// When the user changes an attribute that alters
			//  the appearance of its corresponding view item,
			//  we invalidate the tree view on the region occupied
			//  by the item.
			// Note that even though a SID and MSG_COUNT change does affect
			//  the appearance of the user, we don't respond to the
			//  TSonorkAppEventSetUser::SET_F_SID nor F_MSG_COUNT because those
			//  operations are more complex than a simple refresh and are handled
			//  by the application: When the user connects/disconnects, the
			//  user view node has to be moved from one group to another
			//  and when the message counter leaves/enters zero (0) the
			//  user list has to be re-sorted.
			//   See TSonorkWin32App::ProcessUserSidChange() for more details
			//   on how that is handled.

			if(wParam &	(
				 SONORK_APP_EVENT_SET_USER_F_AUTH_FLAGS
				|SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS))
			{
				TSonorkAppEventSetUser *E=(TSonorkAppEventSetUser *)event_data;
				VP.item = GetExtUserViewItem( E->user_data );
				if( VP.item != NULL )
				{
					if(wParam & SONORK_APP_EVENT_SET_USER_F_AUTH_FLAGS)
						RecalcViewItemParent( VP.item );
					if(wParam & SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS)
						RepaintViewItem( VP.item );
				}
			}
			return true;

		case SONORK_APP_EVENT_OPEN_PROFILE:
			UpdateSkinDependantItems();
			UpdateInterfaceFunctions();
			UpdateCaption();
			return true;

		case SONORK_APP_EVENT_SET_PROFILE:
			UpdateCaption();
			::InvalidateRect( sid_mode_combo.hwnd, NULL , false);
			return true;

		case SONORK_APP_EVENT_SID_CHANGED:
			UpdateSidModeIndicator();
			return true;

		case SONORK_APP_EVENT_SYS_DIALOG:
			SonorkApp.SetBichoSequence(event_data!=0?SONORK_SEQUENCE_OPEN_WINDOW:SONORK_SEQUENCE_CLOSE_WINDOW);

			// When the main GROUP_SEND dialog
			// opens, we automatically show the users
			// tree to enable selecting users.
			if( wParam == SONORK_SYS_DIALOG_GRP_MSG)
			{
				SetActiveTab(SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS
					, false
					, true );
			}
			return true;
			
		case SONORK_APP_EVENT_DESKTOP_SIZE_CHANGE:
		default:
			return false;
	}

}

// =========================================
// 		USER INTERFACE
// =========================================

TSonorkExtUserData*
	TSonorkMainWin::GetContextUser() const
{
	if( ContextViewItemIs(SONORK_TREE_ITEM_EXT_USER) )
		return view.ctxVP.user->UserData();
	return NULL;
}
bool
	TSonorkMainWin::ContextViewItemIs(SONORK_TREE_ITEM_TYPE t) const
{
	if(view.ctxVP.item == NULL )return false;
	if(view.ctxVP.item->Type()!=t)return false;
	return true;
}


void
	TSonorkMainWin::OnInitMenu(HMENU hmenu)
{
	UINT online_enable,aux,disabled_flags;
	TSonorkExtUserData* UD;

	SonorkApp.CancelAutoAwaySidMode();
//	Beep(800,100);

	disabled_flags = MF_BYCOMMAND|MF_GRAYED|MF_DISABLED;
	online_enable = (SonorkApp.CxReady()?MF_BYCOMMAND|MF_ENABLED:disabled_flags);
	if( hmenu == SonorkApp.MainMenu()  )
	{
		if(!SonorkApp.SetBichoSequenceIf(SONORK_SEQUENCE_SLEEP,SONORK_SEQUENCE_WAKEUP))
			SonorkApp.SetBichoSequenceIfNot(SONORK_SEQUENCE_WORK,SONORK_SEQUENCE_LOOK_DN);

		view.ctxVP.item = NULL;
		::EnableMenuItem(hmenu
			,CM_CONNECT
			,(SonorkApp.CxActiveOrReady()?MF_BYCOMMAND|MF_GRAYED:MF_BYCOMMAND|MF_ENABLED));

		::EnableMenuItem(hmenu
			,CM_DISCONNECT
			,(SonorkApp.CxActiveOrPending()?MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_GRAYED));

		::EnableMenuItem(hmenu
			,CM_APP_CHAT
			,online_enable);

		::EnableMenuItem(hmenu
			,CM_CFGNETWORK
			,SonorkApp.CxActiveOrReady()
			?MF_BYCOMMAND|MF_GRAYED:MF_BYCOMMAND|MF_ENABLED);

		if( SonorkApp.IntranetMode() )
			::EnableMenuItem(hmenu
				,CM_SWITCHMODE
				, SonorkApp.TestRunFlag(SONORK_WAPP_RF_ALLOW_INTERNET_MODE)
				?MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_GRAYED);
	}
	else
	if( hmenu == SonorkApp.UserMenu() )
	{
		SonorkApp.SetBichoSequenceIfNot(SONORK_SEQUENCE_WORK,SONORK_SEQUENCE_LOOK_DN);
		::EnableMenuItem( hmenu , CM_DELETE, online_enable);
		if((UD=GetContextUser())!=NULL)
		{
			DeleteMenu(hmenu
				,CM_ADD_USER_CTX_USER
				,MF_BYCOMMAND);
			if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED)
			{
				aux=MF_BYPOSITION|MF_ENABLED;
			}
			else
			{
				aux=MF_BYPOSITION|MF_GRAYED|MF_DISABLED;
				TSonorkWin::AppendMenu(hmenu
					,CM_DELETE
					,CM_ADD_USER_CTX_USER
					|SONORK_WIN_CTF_ELLIPSIS
					|SONORK_WIN_CTF_OWNER_DRAW
					,GLS_OP_AUTHUSR
					);
			}
			EnableMenuItem(hmenu
				,CM_USER_CONTEXT_POPUP_AUTH
				,aux);
			EnableMenuItem(hmenu
				,CM_USER_CONTEXT_POPUP_VISIB
				,aux);
			EnableMenuItem(hmenu
				,CM_USER_CONTEXT_POPUP_MTPL
				,SonorkApp.CxReady()
				?MF_BYPOSITION|MF_ENABLED
				:MF_BYPOSITION|MF_GRAYED|MF_DISABLED);

		}
	}
	else
	if( hmenu == SonorkApp.UserGroupMenu() )
	{
		::EnableMenuItem( hmenu , CM_ADD_USER_CTX_GLOBAL, online_enable);
		if( ContextViewItemIs(SONORK_TREE_ITEM_GROUP) )
		{
			if( !view.ctxVP.group->IsLocalUserGroup() )
				aux = disabled_flags;
			else
				aux = online_enable;

			::EnableMenuItem( hmenu , CM_ADD_GROUP, aux);

			if( !view.ctxVP.group->IsCustomLocalUserGroup() )
				aux=disabled_flags;
			else
				aux = online_enable;
			::EnableMenuItem( hmenu , CM_RENAME, aux);

			if( view.ctxVP.group->SubFolders() != 0
			|| !view.ctxVP.group->IsCustomLocalUserGroup() )
				aux = disabled_flags;
			else
				aux = online_enable;
		}
		else
			aux = disabled_flags;
		::EnableMenuItem( hmenu , CM_DELETE, aux);
	}

}
void	TSonorkMainWin::OnInitMenuPopup(HMENU hmenu,UINT ,BOOL sys_menu)
{
	TSonorkRegKey 	regKEY;
	SONORK_SID_MODE	sid_mode;
	DWORD		i,c;
	TSonorkExtUserData* UD;


	if( sys_menu )return;
	if( hmenu == SonorkApp.UserAppsMenu() )
	{
		SonorkApp.UpdateUserExtAppMenu(GetContextUser());
	}
	else
	if( hmenu == SonorkApp.UserVisib() )
	{
		if((UD =GetContextUser()) != NULL )
		if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
		{
			c = (SonorkApp.CxReady()?MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_GRAYED);
			for(i=0;i<SONORK_APP_VISIBILITY_GROUPS;i++)
			{
				CheckMenuItem(hmenu
				, visibility_xlat[i].id
				, UD->ctrl_data.auth.TestFlag(visibility_xlat[i].auth_flag)
				  ?MF_BYCOMMAND|MF_CHECKED
				  :MF_BYCOMMAND|MF_UNCHECKED);
				EnableMenuItem(hmenu,visibility_xlat[i].id,c);
			}
		}
	}
	else
	if( hmenu == SonorkApp.UserAuth() )
	{
		if((UD =GetContextUser()) != NULL )
		if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
		{
			c = (SonorkApp.CxReady()?MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_GRAYED);
			for(i=0;i<AUTH_MENU_ITEMS;i++)
			{
				CheckMenuItem(hmenu
				, auth_xlat[i].id
				, UD->ctrl_data.auth.TestFlag(auth_xlat[i].auth_flag)
				  ?MF_BYCOMMAND|MF_CHECKED
				  :MF_BYCOMMAND|MF_UNCHECKED);
				EnableMenuItem(hmenu,auth_xlat[i].id,c);
			}
			CheckMenuItem(hmenu
			, CM_AUTH_UTS_ENABLED
			, UD->TestCtrlFlag(SONORK_UCF_SONORK_UTS_DISABLED)
				?MF_BYCOMMAND|MF_UNCHECKED
				:MF_BYCOMMAND|MF_CHECKED
				);
		}
	}
	else
	if( hmenu == SonorkApp.TrayIconMenu() )
	{
		i=ComboBox_GetCurSel(sid_mode_combo.hwnd);
		if( i == (DWORD)-1 || SonorkApp.CxStatus() == SONORK_APP_CX_STATUS_IDLE)
			sid_mode=SONORK_SID_MODE_DISCONNECTED;
		else
			sid_mode = (SONORK_SID_MODE)ComboBox_GetItemData(sid_mode_combo.hwnd,(int)i);
		for(i=CM_TRAY_SID_MODE_ONLINE
		; i<=CM_TRAY_SID_MODE_INVISIBLE
		; i++)
		{
			switch(i)
			{
				case CM_TRAY_SID_MODE_ONLINE:
				 c=(sid_mode==SONORK_SID_MODE_ONLINE);
				 break;

				case CM_TRAY_SID_MODE_BUSY:
				 c=(sid_mode==SONORK_SID_MODE_BUSY);
				 break;

				case CM_TRAY_SID_MODE_AT_WORK:
				 c=(sid_mode==SONORK_SID_MODE_AT_WORK);
				 break;

				case CM_TRAY_SID_MODE_AWAY:
				 c=(sid_mode==SONORK_SID_MODE_AWAY);
				 break;

				case CM_TRAY_SID_MODE_FRIENDLY:
    				 c=(sid_mode==SONORK_SID_MODE_FRIENDLY);
				 break;
				case CM_TRAY_SID_MODE_INVISIBLE:
				 c=(sid_mode==SONORK_SID_MODE_INVISIBLE);
				 break;

			}
			CheckMenuItem(hmenu
				, i
				, c?MF_BYCOMMAND|MF_CHECKED:MF_BYCOMMAND|MF_UNCHECKED);
		}
		CheckMenuItem(hmenu
		, CM_DISCONNECT
		, sid_mode==SONORK_SID_MODE_DISCONNECTED
			?MF_BYCOMMAND|MF_CHECKED:MF_BYCOMMAND|MF_UNCHECKED);
	}
}

void	TSonorkMainWin::EnableCommand( UINT id , bool v)
{
	::EnableMenuItem(SonorkApp.MainMenu()
		,id
		,MF_BYCOMMAND|(v?MF_ENABLED:MF_GRAYED));
	SetToolBarState( id
		, v?TBSTATE_ENABLED:0 );
}
void	TSonorkMainWin::CheckCommand( UINT id , bool v)
{
	::CheckMenuItem(SonorkApp.MainMenu(),id,MF_BYCOMMAND|(v?MF_CHECKED:MF_UNCHECKED));
	SetToolBarState( id
		, TBSTATE_ENABLED|(v?TBSTATE_CHECKED:0)
		);
}


// ----------------------
// UpdateUserModeIndicator()
//	Updates the interface to reflect current user mode (sid mode)
void	TSonorkMainWin::UpdateSidModeIndicator()
{
	int 		index;
	SONORK_SID_MODE sm;

	// translate the sid mode to the equivalent one in the combo box
	if( SonorkApp.IsProfileOpen() )
	{
		sm=SonorkApp.ProfileSidFlags().SidMode();
		switch(sm)
		{
		case SONORK_SID_MODE_ONLINE:
		case SONORK_SID_MODE_ONLINE_02:
		case SONORK_SID_MODE_ONLINE_03:
			sm=SONORK_SID_MODE_ONLINE;
			break;

		case SONORK_SID_MODE_BUSY:
		case SONORK_SID_MODE_BUSY_02:
		case SONORK_SID_MODE_BUSY_03:
			sm=SONORK_SID_MODE_BUSY;
			break;

		case SONORK_SID_MODE_AT_WORK:
		case SONORK_SID_MODE_AT_WORK_02:
		case SONORK_SID_MODE_AT_WORK_03:
			sm=SONORK_SID_MODE_AT_WORK;
			break;

		case SONORK_SID_MODE_FRIENDLY:
		case SONORK_SID_MODE_FRIENDLY_02:
		case SONORK_SID_MODE_FRIENDLY_03:
			sm=SONORK_SID_MODE_FRIENDLY;
			break;

		case SONORK_SID_MODE_AWAY:
		case SONORK_SID_MODE_AWAY_AUTO:
		case SONORK_SID_MODE_AWAY_HOLD:
		case SONORK_SID_MODE_AWAY_PHONE:
		case SONORK_SID_MODE_AWAY_02:
		case SONORK_SID_MODE_AWAY_03:
			sm=SONORK_SID_MODE_AWAY;
			break;

		case SONORK_SID_MODE_INVISIBLE:
		case SONORK_SID_MODE_INVISIBLE_02:
		case SONORK_SID_MODE_INVISIBLE_03:
			sm=SONORK_SID_MODE_INVISIBLE;
			break;

		default:
			break;

		}
	}
	else
		sm =SONORK_SID_MODE_DISCONNECTED;

	index = ComboBox_FindStringExact(sid_mode_combo.hwnd
			,-1,
			(SONORK_C_CSTR)sm);
	ComboBox_SetCurSel(sid_mode_combo.hwnd,index);
}


/*
// ----------------------
// UpdateEventsIndicator()
// Updates the interface to reflect the ammount of unread
//  user messages
void	TSonorkMainWin::UpdateEventsIndicator()
{
	UINT state;
	state = SonorkApp.GetEventsCounter() > 0 ? TBSTATE_ENABLED:0;
	SetToolBarState( CM_OPEN_NEXT_EVENT , state);
}
*/
void	TSonorkMainWin::UpdateConnectIndicators()
{
	SetToolBarIcon(CM_CONNECT
		, 	SonorkApp.CxReady()
			?SKIN_ICON_CONNECTED
			:SonorkApp.CxActiveOrPending()
			?SKIN_ICON_CONNECTING
			:SKIN_ICON_DISCONNECTED
		);

}

// ----------------------
// OnAppEventCxStatus()
// 	Updates the interface to reflect current connection status
void	TSonorkMainWin::OnAppEventCxStatus(UINT old_status)
{
	UpdateConnectIndicators();
	if( SonorkApp.CxReady() )
	{
		if(old_status<=SONORK_APP_CX_STATUS_READY)
			GetTree(SONORK_VIEW_TAB_TREE_APPS).InvalidateRect(NULL,false);
		if(SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_DISCONNECT))
			EnableCommand(CM_CONNECT,false);
	}
	else
	if( SonorkApp.CxStatus() == SONORK_APP_CX_STATUS_IDLE )
		::InvalidateRect( sid_mode_combo.hwnd, NULL , false);

	UpdateInterfaceFunctions();
}

// ----------------------
// UpdateInterfaceFunctions()
// Enables/disables the interface functions.
// + It is invoked by TSonorkWin32App::OnGlobalTask() when a
//   global task starts/ends.
// + and by OnAppEventCxStatus() to update those items
//   that work only on certain connection statuses.
// + also by OnCreate() on initialization

#define MAY_CHANGE_SID_MODE	( SonorkApp.IsProfileOpen() && \
				!(SonorkApp.CxConnecting()||SonorkApp.GlobalTaskPending()))

void	TSonorkMainWin::UpdateInterfaceFunctions()
{
	bool enable;

	// User mode
	{
		// Can't use SonorkApp::MayStartGlobalTask() to test if the user mode
		// function is available because the user mode may also be changed while
		// off-line (it only can't be changed *while* connecting)
		// SonorkApp::MayStartGlobalTask() applies only to connect-time functions.
		enable = MAY_CHANGE_SID_MODE;
		::EnableWindow(sid_mode_combo.hwnd, enable );
	}
	// Menu and tool bar
	{
		enable=SonorkApp.MayStartGlobalTask();
		EnableCommand(CM_REFRESH_USERS,enable);
		EnableCommand(CM_ADD_USER_CTX_GLOBAL,enable);
	}
}


// ----------------------------------------------------------------------------
// User interface commands

void
 TSonorkMainWin::CmdGroupMsg(TSonorkClipData*CD)
{
	TSonorkGrpMsgWin *W;
	if( ContextViewItemIs(SONORK_TREE_ITEM_GROUP) )
	{
		if( view.ctxVP.group->IsUserGroup() )
		{
			SetSelection( view.ctxVP.item , true);
		}
	}
	W=(TSonorkGrpMsgWin*)SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_GRP_MSG);
	if(CD&&W)
		W->ProcessDrop(CD);
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdAddUser( bool show_err_msg )
{
	DWORD 		group_no=0;
	BOOL		request_auth;
	SONORK_C_CSTR	group_name;
	TSonorkUSearchWin *W;
	TSonorkTreeItem	*item;
	if( !SonorkApp.CxReady() )
	{
		if(show_err_msg)
			MessageBox(GLS_MS_CXONLY,GLS_OP_ADDUSR,MB_OK|MB_ICONASTERISK);
		return;
	}
	if( ContextViewItemIs(SONORK_TREE_ITEM_GROUP) )
	{
		request_auth = view.ctxVP.group->IsLocalUserGroup();
		if( request_auth )
		{
			group_no=view.ctxVP.group->Group();
			item=view.ctxVP.group;
		}
		else
		{
			item=view.group.remote_users;
		}
	}
	else
	{
		request_auth 	= true;
		item		= view.group.local_users;
	}

	{
		SKIN_ICON	dummy_icon;
		group_name=item->GetLabel( false , dummy_icon );
	}
	W = (TSonorkUSearchWin*)SonorkApp.GetSysDialog( SONORK_SYS_DIALOG_USER_SEARCH );
	if( W == NULL )
	{
		W=new TSonorkUSearchWin(group_no , group_name , request_auth);
		SonorkApp.ShowNewSysDialog( W );
	}
	else
	{
		W->SetAddGroup(group_no , group_name , request_auth);
		SonorkApp.ShowExistingSysDialog( W );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdUserAuth(UINT id, bool visibility)
{
	int i;
	TSonorkExtUserData	*UD;
	TSonorkAuth2		auth;

	if((UD = GetContextUser()) == NULL)
		return;

	auth.Set(UD->ctrl_data.auth);
	if( visibility )
	{
		for(i=0;i<SONORK_APP_VISIBILITY_GROUPS;i++)
			if(visibility_xlat[i].id==id)
			{
				auth.flags.Toggle(visibility_xlat[i].auth_flag);
				break;
			}
		if(i==SONORK_APP_VISIBILITY_GROUPS)return;
	}
	else
	{
		for(i=0;i<AUTH_MENU_ITEMS;i++)
			if(auth_xlat[i].id==id)
			{
				auth.flags.Toggle(auth_xlat[i].auth_flag);
				break;
			}
		if(i==AUTH_MENU_ITEMS)return;
	}
	Task_SetUserAuth( UD , auth );

}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdDelete()
{
	if( view.ctxVP.item == NULL )
		return;
	if( view.ctxVP.item->Type() == SONORK_TREE_ITEM_EXT_USER)
	{
		TSonorkTempBuffer tmp(384);
		int l;
		TSonorkId	userId;
		l=SonorkApp.LangSprintf(tmp.CStr()
			,GLS_MS_SURE_TK
			,SonorkApp.LangString(GLS_TK_DELUSR));
		userId.Set(view.ctxVP.user->UserData()->userId);
		wsprintf(tmp.CStr() + l,
			"\n%u.%u %s"
			,userId.v[0]
			,userId.v[1]
			,view.ctxVP.user->UserData()->display_alias.CStr());

		if(MessageBox(tmp.CStr(),GLS_OP_DELUSR,MB_ICONQUESTION|MB_YESNO)==IDYES)
		{
			Task_DelUser( userId );
		}
	}
	else
	if( view.ctxVP.item->Type() == SONORK_TREE_ITEM_GROUP )
	{
		CmdModifyGroup( CM_DELETE );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetSidMode( SONORK_SID_MODE mode )
{
	// Test flag to prevent our own changes invoking us again
	if(TestWinUsrFlag(CHANGING_SID_MODE))
		return;

	if( mode == SONORK_SID_MODE_DISCONNECTED)
	{
		PostPoke( SONORK_MAINWIN_POKE_CONNECT_DISCONNECT , false );
	}
	else
	if(IS_VALID_SID_MODE(mode))
	{
		if( mode != SonorkApp.ProfileSidFlags().SidMode() )
		{
			SetWinUsrFlag(CHANGING_SID_MODE);
			if( SonorkApp.SetSidMode(mode)!=SONORK_RESULT_OK )
				UpdateSidModeIndicator();
			ClearWinUsrFlag(CHANGING_SID_MODE);
		}
		if( SonorkApp.CxStatus() == SONORK_APP_CX_STATUS_IDLE )
			CmdConnect( true );
	}
	else
	if( mode == PSEUDO_SONORK_SID_MODE_VISIBILITY )
	{
		UpdateSidModeIndicator();
		TSonorkSidModeWin(this).Execute();
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdSidModeSelected()
{
	int index;
	index=ComboBox_GetCurSel(sid_mode_combo.hwnd);
	if( index == -1 )return;
	SetSidMode( (SONORK_SID_MODE)ComboBox_GetItemData(sid_mode_combo.hwnd,index) );
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdCfgNetwork()
{
	if( SonorkApp.CxConnecting() )
		return ;
	if( SonorkApp.CxActiveOrPending() )
		SonorkApp.Disconnect();
	TSonorkNetCfgWin(this).Execute();
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdConnect( BOOL connect )
{
	// Don't try to connect while login dialog is open
	if(SonorkApp.IsProfileOpen())
	{
		if( connect )
		{
			if( !(SonorkApp.CxReady() || SonorkApp.CxConnecting()) )
			{
				TSonorkError ERR;
				SonorkApp.Connect(ERR);
			}
		}
		else
		{
			if( SonorkApp.CxActiveOrPending() )
			{
				if(!SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_DISCONNECT))
					SonorkApp.Disconnect();
			}
		}
		return;
	}
	ShowLoginDialog();
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::ShowLoginDialog()
{
	TSonorkWin*W=SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_MY_INFO);
	W->PostPoke(SONORK_WIN_POKE_SET_TAB , TSonorkMyInfoWin::TAB_LOGIN);
}


/*void
	TSonorkMainWin::CmdTellAFriend()
{
	TSonorkTempBuffer	buffer(512);
	char				l_tmp[128],*ptr;
	char				s_tmp[24];
	DWORD				aux;
	if( SonorkApp.IsProfileOpen() && !SonorkApp.IntranetMode())
	{
		ptr=buffer.CStr();
		SonorkApp.ProfileUserId().GetStr(s_tmp);
		ptr+=sprintf(ptr,"http://sonork/wapp/pub/tell-a-friend/default.asp?GuId=%s"
			, SONORK_UrlEncode(l_tmp,sizeof(l_tmp),s_tmp));

		ptr+=sprintf(ptr,"&Alias=%s"
			, SONORK_UrlEncode(l_tmp,sizeof(l_tmp),SonorkApp.ProfileUser().alias.CStr()));

		ptr+=sprintf(ptr,"&Name=%s"
			, SONORK_UrlEncode(l_tmp,sizeof(l_tmp),SonorkApp.ProfileUser().name.CStr()));

		aux=SonorkApp.ProfileRegion().GetLanguage();
		SonorkApp.ProfileRegion().GetStr(s_tmp,16);
		sprintf(ptr,"&Region=%s&Lang=%s&Version=%08x"
			, SONORK_UrlEncode(l_tmp,sizeof(l_tmp),s_tmp)
			, &aux
			, SONORK_FULL_APP_VERSION);
		SonorkApp.ShellOpenFile(this, buffer.CStr(), false);
	}
}
*/

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::PinWindow( BOOL left , UINT width )
{
	RECT rect;
	if( width == 0)
	{
		GetWindowRect(&rect);
		width=rect.right - rect.left;
	}
	if(width<MAIN_WIN_MIN_WIDTH)
		width=MAIN_WIN_MIN_WIDTH;
	else
	if(width>MAIN_WIN_MAX_WIDTH)
		width=MAIN_WIN_MAX_WIDTH;
	MoveWindow(	 left?SonorkApp.DesktopOrigin().x:SonorkApp.DesktopLimit().x - width
				,SonorkApp.DesktopOrigin().y
				,width
				,SonorkApp.DesktopSize().cy);
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdModifyGroup(UINT id)
{
	if( !ContextViewItemIs(SONORK_TREE_ITEM_GROUP) )
			return;

	if( 	(id == CM_ADD_GROUP && view.ctxVP.group->IsLocalUserGroup())
		||
		(id==CM_RENAME && view.ctxVP.group->IsCustomLocalUserGroup()) )
	{
		TSonorkInputWin W(this);
		TSonorkGroup UG;

		W.help.Set(SonorkApp.LangString(id==CM_RENAME?GLS_OP_RENAM:GLS_OP_ADDGRP));
		W.prompt.Set(SonorkApp.LangString(GLS_LB_NAME));
		if( id == CM_ADD_GROUP )
		{
			UG.header.parent_no 	= view.ctxVP.group->Group();
			UG.header.depth	  	= view.ctxVP.group->Depth()+1;
			UG.header.group_no 	= GetFreeGroupNo();
		}
		else
		{
			UG.header.parent_no 	= view.ctxVP.custom_group->parent_no;
			UG.header.depth	  	= view.ctxVP.group->Depth();
			UG.header.group_no 	= view.ctxVP.group->Group();
			W.input.Set(view.ctxVP.custom_group->name);
		}
		if(W.Execute() == IDOK)
		{
			UG.name.Set(W.input.CStr());
			UG.header.group_type = SONORK_GROUP_TYPE_USER;
			Task_SetGroup( &UG, id == CM_ADD_GROUP  );
		}
	}
	else
	if( id == CM_DELETE && view.ctxVP.group->IsCustomLocalUserGroup() )
	{
		if( view.ctxVP.group->SubFolders() == 0 )
			Task_DelGroup( SONORK_GROUP_TYPE_USER, view.ctxVP.group->Group() );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdSwitchMode()
{
	SonorkApp.SwitchMode( NULL
	,	SonorkApp.IsControlKeyDown()
		?SONORK_APP_SWITCH_MODE_F_NEWWIN
		:SONORK_APP_SWITCH_MODE_F_QUERY
	);
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CmdUserAuth(TSonorkExtUserData*UD)
{
	if( UD )
	{
		if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
		{
			SonorkApp.OpenUserDataWin(UD
				, (TSonorkWin*)NULL
				, TSonorkUserDataWin::TAB_AUTH);
		}
		else
		{
			TSonorkText text;
			if( SonorkApp.AskAddUserText(this,text) )
			{
				Task_AddUser( UD->userId , text);
			}
		}
	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnCommand(UINT id,HWND cm_hwnd, UINT code)
{
	TSonorkExtUserData	*ctxUser;
	union{
		int   		i;
		POINT 		pt;
		TSonorkWin	*W;
	}D;
	SonorkApp.CancelAutoAwaySidMode();
	if( cm_hwnd == tool_bar.hwnd )	// ToolBar
	{
		// When the toolbar is clicked, we simulate a menu click
		//  by setting <hwnd> to NULL and substracting TOOL_BAR_BASE_ID
		//  from the id to get the corresponding CM_xxx value
		//  (See SetupToolBar)
		view.ctxVP.item = NULL;
		if(code != BN_CLICKED)
			return false;
		id-=TOOL_BAR_BASE_ID;
		cm_hwnd=NULL;
		if( id == CM_CONNECT)
		{
			if( SonorkApp.CxActiveOrReady() )
				id = CM_DISCONNECT;
		}
	}
	if( cm_hwnd == NULL )
	{
		HideTreeToolTips();
		// Sender is menu
		ctxUser =  GetContextUser();
		switch(id)
		{
			case CM_VISIBILITY:
				TSonorkSidModeWin(this).Execute();
				break;
				
			case CM_ABOUT:
				TSonorkAboutWin(this).Execute();
				break;
				
			case CM_QUIT:
				CloseApp( true );
				break;

			case CM_DISCONNECT:
				CmdConnect(false);
				break;

			case CM_CONNECT:
				if( !SonorkApp.IsControlKeyDown() )
				{
					CmdConnect(true);
					break;
				}
				ShowLoginDialog();
				break;

			case CM_MY_INFO:
			case CM_PREFERENCES:
				D.W = SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_MY_INFO);
				if(id==CM_PREFERENCES && D.W!=NULL)
					D.W->PostPoke(SONORK_WIN_POKE_SET_TAB , TSonorkMyInfoWin::TAB_PREFS);
			break;

			case CM_CFGNETWORK:
				CmdCfgNetwork();
			break;

			case CM_SWITCHMODE:
				CmdSwitchMode();
			break;

			case CM_ADD_USER_CTX_GLOBAL:
				CmdAddUser( true );
			break;

			case CM_ADD_USER_CTX_USER:
				CmdUserAuth(ctxUser);
				break;


			case CM_REFRESH_USERS:
				SonorkApp.RefreshUserList(NULL);
			break;

			case CM_DBMAINT_USERS:
				SonorkApp.ShowUserDbMaintenance();
			break;

			case CM_ADD_GROUP:
			case CM_RENAME:
					CmdModifyGroup(id);
			break;

			case CM_TELL_A_FRIEND:
//				CmdTellAFriend();
			break;

			case CM_DELETE:
				CmdDelete();
			break;

			case CM_GROUP_MSG:
				CmdGroupMsg(NULL);
				break;

			case CM_PIN_WINDOW:
				D.i=SonorkApp.IsControlKeyDown();
				SonorkApp.SetBichoSequenceIfNot(SONORK_SEQUENCE_WORK
					,D.i?SONORK_SEQUENCE_SURPRISED_LEFT:SONORK_SEQUENCE_SURPRISED_RIGHT);
				PinWindow(D.i , MAIN_WIN_PINNED_WIDTH);
				break;

			case CM_USER_MSG:
				SonorkApp.OpenMsgWindow( ctxUser , SONORK_MSG_WIN_OPEN_FOREGROUND );
				break;

			case CM_USER_INFO:
				SonorkApp.OpenUserDataWin(ctxUser
					,NULL
					,TSonorkUserDataWin::TAB_INFO);
				break;

			case CM_EAPP_CONFIG:
				SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_EXT_APP_CONFIG);
			break;
			
			case CM_SERVICE_LIST:
				SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_SERVICES);
			break;


			case CM_EAPP_RELOAD:
				SonorkApp.PostAppCommand(SONORK_APP_COMMAND_RELOAD_EXT_APPS,0);
				break;

			case CM_AUTH_UTS_ENABLED:
				if(ctxUser)
				{
					ctxUser->CtrlFlags().Toggle(SONORK_UCF_SONORK_UTS_DISABLED);
				}
				break;

			case CM_AUTH_BUSY:
			case CM_AUTH_NBUSY:
			case CM_AUTH_FRIENDLY:
			case CM_AUTH_NFRIENDLY:
			case CM_AUTH_NCX:
			case CM_AUTH_NADDR:
			case CM_AUTH_NEMAIL:
				CmdUserAuth(id,false);
				break;
			case CM_VISIBILITY_01:
			case CM_VISIBILITY_02:
			case CM_VISIBILITY_03:
			case CM_VISIBILITY_04:
			case CM_VISIBILITY_05:
			case CM_VISIBILITY_06:
			case CM_VISIBILITY_07:
				CmdUserAuth(id,true);
				break;

			default:
				if( id == CM_WAPP_OPEN
				||  (id >=SONORK_APP_CM_WAPP_BASE && id < SONORK_APP_CM_WAPP_LIMIT)
				||  (id >=SONORK_APP_CM_MTPL_BASE && id < SONORK_APP_CM_MTPL_LIMIT)
				 )
				{
					SonorkApp.LaunchWebAppByCmd( this, id , ctxUser , NULL);
					break;
				}

			case CM_APP_CHAT:
			case CM_APP_CLIPBOARD:
			case CM_APP_REMINDER:
			case CM_APP_SYS_CONSOLE:
			case CM_APP_SNAP_SHOT:
			case CM_APP_TRACK:
				SonorkApp.LaunchAppByCmd( id , ctxUser , NULL);
				break;

			case CM_TRAY_SID_MODE_ONLINE:
			 SetSidMode(SONORK_SID_MODE_ONLINE);
			 break;

			case CM_TRAY_SID_MODE_BUSY:
			 SetSidMode(SONORK_SID_MODE_BUSY);
			 break;

			case CM_TRAY_SID_MODE_AT_WORK:
			 SetSidMode(SONORK_SID_MODE_AT_WORK);
			 break;

			case CM_TRAY_SID_MODE_AWAY:
			 SetSidMode(SONORK_SID_MODE_AWAY);
			 break;

			case CM_TRAY_SID_MODE_FRIENDLY:
			 SetSidMode(SONORK_SID_MODE_FRIENDLY);
			 break;
			case CM_TRAY_SID_MODE_INVISIBLE:
			 SetSidMode(SONORK_SID_MODE_INVISIBLE);
			 break;

		}
		view.ctxVP.item = NULL;
		return true;
	}

	if( cm_hwnd == bicho.hwnd )
	{
		D.pt.x=0;
		D.pt.y=TOOL_BAR_AREA_HEIGHT;//SONORK_ICON_FRAME_SH;
		ClientToScreen(&D.pt);
		TrackPopupMenu(SonorkApp.MainMenu()
			,TPM_LEFTALIGN  | TPM_LEFTBUTTON
			,D.pt.x
			,D.pt.y
			,0
			,Handle()
			,NULL);
		return true;

	}
	if( id == IDC_MAIN_SID_MODE )
	{
		if( TestWinSysFlag(SONORK_WIN_SF_INITIALIZED) )
		{
			if( code == CBN_SELENDOK )
			{
				CmdSidModeSelected();
			}
		}
		return true;
	}
	if( id == IDC_MAIN_LOGO  )
	{
		if( code == STN_CLICKED)
		{
			SonorkApp.ShellOpenFile( this
				, SonorkApp.ReferrerUrl()
				, false);
		}
		return true;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Constructor/Destructor

TSonorkMainWin::TSonorkMainWin()
	:TSonorkTaskWin(NULL
	,SONORK_WIN_CLASS_MAIN
	|SONORK_WIN_TYPE_MAIN
	|SONORK_WIN_DIALOG
	|IDD_MAIN
	,SONORK_WIN_SF_NO_WIN_PARENT
	|SONORK_WIN_SF_NO_DEL_ON_DESTROY
	|SONORK_WIN_SF_APP_COLORS
	)
{
	view.sel_count	= 0;
	task.pop	= NULL;
	task.type	= TASK_NONE;
	SetEventMask(SONORK_APP_EM_SKIN_AWARE);

}

// ----------------------------------------------------------------------------
// Creation/Destruction

void
 TSonorkMainWin::OnAfterCreate()
{

	UpdateInterfaceFunctions();
	UpdateConnectIndicators();
	OnActivate(IsActive() && IsVisible()?WA_ACTIVE:0,IsIconic());

}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::UpdateCaption()
{
	UINT	res_id;
	char	tmp[64];
	SONORK_C_CSTR	ptr;
	if(!SonorkApp.IsProfileOpen())
	{
		ptr = szSONORK;
		res_id = 2;
	}
	else
	{
		wsprintf(tmp,"%u.%u %-.16s"
			,SonorkApp.ProfileUserId().v[0]
			,SonorkApp.ProfileUserId().v[1]
			,SonorkApp.ProfileUser().alias.CStr());
		ptr=tmp;
		res_id =
			SonorkApp.ProfileUser().InfoFlags().GetSex() == SONORK_SEX_F?3:2;
	}
	SetWindowText( ptr );
	if( SonorkApp.TestCfgFlag(SONORK_WAPP_CF_INTRANET_MODE) )
		res_id+=10;

	SetCaptionIcon(
			(HICON)LoadImage(SonorkApp.Instance()
				, MAKEINTRESOURCE(IDI_SONORK)
				, IMAGE_ICON
				, 32
				, 32
				, LR_DEFAULTCOLOR|LR_SHARED	)
			,ICON_BIG
		);
	SetCaptionIcon(
			(HICON)LoadImage(SonorkApp.Instance()
				, MAKEINTRESOURCE(res_id)
				, IMAGE_ICON
				, 16
				, 16
				, LR_DEFAULTCOLOR|LR_SHARED	)
			,ICON_SMALL
		);
}

// ----------------------------------------------------------------------------
// Freddy Casco 429609
// code 21726 data=1560
static
 HBITMAP _LoadSkinBitmap(HDC tDC , char *p_name)
{
	char	file_name[SONORK_MAX_PATH];
	HBITMAP	bm;
	SonorkApp.GetDirPath(file_name
		,SONORK_APP_DIR_SKIN
		, p_name);
	bm = (HBITMAP)LoadImage(NULL
				,file_name
				,IMAGE_BITMAP
				,0
				,0
				,LR_LOADFROMFILE);
	if( bm )
	{
		SelectObject( tDC, bm );
		return bm;
	}
	return NULL;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::UpdateSkinDependantItems()
{
	int i;
	for(i=SONORK_VIEW_TAB_FIRST;i<=SONORK_VIEW_TAB_LAST;i++)
		view.tree[i].UpdateSkin();
	::SendMessage(view.titleHwnd
			, WM_SETFONT
			, (WPARAM)sonork_skin.Font(SKIN_FONT_BOLD)
			, 0);
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnCreate()
{
	DWORD	aux;
	UINT	i;
	union {
		TSonorkMainInfoWin	*info;
		TSonorkBicho        	*bicho;
	}D;
	RECT 			rect;
	SIZE 			size;
	HDC			skDC;
	HBITMAP			skBM;

	view.ctxTab = SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS;
	// Tab Control
	{
		TC_ITEM tcHitem;

		view.titleHwnd	= GetDlgItem(IDC_MAIN_TAB_TITLE );
		view.tabHwnd 	= GetDlgItem(IDC_MAIN_TAB );


		aux = ::GetWindowLong(view.tabHwnd,GWL_STYLE);

		aux|=TCS_VERTICAL|TCS_MULTILINE  ;//TCS_RIGHT|
		aux&=~(WS_BORDER|TCS_SINGLELINE);
		::SetWindowLong(view.tabHwnd,GWL_STYLE,aux);
		TabCtrl_SetItemSize(view.tabHwnd
			,TAB_BUTTON_WIDTH
			,TAB_BUTTON_HEIGHT);


		tcHitem.mask=TCIF_TEXT;
		tcHitem.lParam=0;
		tcHitem.pszText="";

		TabCtrl_InsertItem(view.tabHwnd,SONORK_VIEW_TAB_1,&tcHitem);
		TabCtrl_InsertItem(view.tabHwnd,SONORK_VIEW_TAB_2,&tcHitem);
	}

	// Tree view
	{
		view.tree[SONORK_VIEW_TAB_1].SetHandle( GetDlgItem( IDC_MAIN_TREE_1 ) );
		view.tree[SONORK_VIEW_TAB_2].SetHandle( GetDlgItem( IDC_MAIN_TREE_2 ) );
		for(i=SONORK_VIEW_TAB_FIRST;i<=SONORK_VIEW_TAB_LAST;i++)
		{
			view.ctrl[i].AssignCtrl(this, view.tree[i].Handle()  , 0);
		}
	}
	SetActiveTab(view.ctxTab, true, true);


	::SetClassLong( Handle()
		, GCL_HICON
		, (LONG)sonork_skin.AppHicon() );
	UpdateCaption();

	// Tool bar
	SetupToolBar();


	// Info Win
	{
		D.info	= new TSonorkMainInfoWin(this);
		D.info->Create();
		D.info->GetWindowSize(&size);
		info.height 	=size.cy;
		info.hwnd	=D.info->Handle();
	}


	view.update_count=0;
	view.dropHitem = NULL;
	view.group.local_users	=new TSonorkSysGroupViewItem(SONORK_VIEW_SYS_GROUP_LOCAL_USERS);
	view.group.remote_users	=new TSonorkSysGroupViewItem(SONORK_VIEW_SYS_GROUP_REMOTE_USERS);
	view.group.auths	=new TSonorkSysGroupViewItem(SONORK_VIEW_SYS_GROUP_AUTHS);
	view.group.webapps	=new TSonorkSysGroupViewItem(SONORK_VIEW_SYS_GROUP_WEB_APPS);
	view.group.extapps	=new TSonorkSysGroupViewItem(SONORK_VIEW_SYS_GROUP_EXT_APPS);

	AddViewItem(view.group.local_users);
	AddViewItem(view.group.remote_users);
	AddViewItem(view.group.auths);
	AddViewItem(view.group.extapps);
	AddViewItem(view.group.webapps);


	view.drop_target.AssignCtrl( Handle()
		, view.tree[SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS].Handle() );




	// Sonork "Bicho"
	skDC = CreateCompatibleDC(NULL);
	SaveDC( skDC );
	skBM = _LoadSkinBitmap(skDC,"default\\main.bmp");
	D.bicho		= new TSonorkBicho(this, skBM?skDC:NULL);
	D.bicho->Create();
	bicho.hwnd	= D.bicho->Handle();
	RestoreDC( skDC , -1);
	DeleteDC( skDC );


	// Sonork Logo
	logo.hwnd = GetDlgItem(IDC_MAIN_LOGO);
	rect.top	= 0;
	rect.bottom	= SKIN_LARGE_LOGO_SH;
	::AdjustWindowRectEx(&rect
		,::GetWindowLong(logo.hwnd,GWL_STYLE)
		,false
		,::GetWindowLong(logo.hwnd,GWL_EXSTYLE));
	logo.height = rect.bottom - rect.top ;


	// User mode
	{
		sid_mode_combo.hwnd=GetDlgItem ( IDC_MAIN_SID_MODE );
		aux = ::GetWindowLong(sid_mode_combo.hwnd,GWL_STYLE);
		aux&=~(WS_BORDER);
		::SetWindowLong(sid_mode_combo.hwnd,GWL_STYLE,aux);
		aux = ::GetWindowLong(sid_mode_combo.hwnd,GWL_EXSTYLE);
		aux&=~(WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE|WS_EX_STATICEDGE);
		::SetWindowLong(sid_mode_combo.hwnd,GWL_EXSTYLE,aux);

		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_ONLINE);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_BUSY);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_AT_WORK);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_AWAY);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_FRIENDLY);
		if( !SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_INVISIBLE) )
			ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_INVISIBLE);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)PSEUDO_SONORK_SID_MODE_VISIBILITY);
		ComboBox_AddString(sid_mode_combo.hwnd,(char*)SONORK_SID_MODE_DISCONNECTED);
		::GetWindowRect(sid_mode_combo.hwnd,&rect);
		sid_mode_combo.height = rect.bottom - rect.top ;
	}

	UpdateSkinDependantItems();

	return true;
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnSysCommand(UINT id)
{
	if(id==SC_CLOSE)
	{
		CloseApp( true );
		return true;
	}
	return false;
}	

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::CloseApp(bool interactive)
{
	if( interactive )
	{
		if(SonorkApp.TestCfgFlag( SONORK_WAPP_CF_NO_CLOSE ))
			return;
		if( GetKeyState(VK_CONTROL) & GetKeyState(VK_SHIFT) & 0x8000)
		{
			if(MessageBox(	"Uninstall Sonork?\n"
					"Desinstalar Sonork?"
					,csNULL
					,MB_YESNO|MB_ICONQUESTION)!=IDYES)
				return;

			if(TSonorkMyInfoWin::SetPreferenceSetting(MINFO_PREF_START_WITH_WINDOWS
				,0) != 0)
			{
				MessageBox("You need administrative rights to install/remove Sonork"
					,csNULL
					,MB_OK|MB_ICONWARNING);
				return;
			}
			SonorkApp.SetRunFlag(SONORK_WAPP_RF_UNINSTALL);
		}
	}
	Destroy();
}

// ----------------------------------------------------------------------------

void    TSonorkMainWin::OnBeforeDestroy()
{
	UINT i;
	Task_StopMailCheck( true );

	SonorkApp.OnMainWinDestroying();

	for(i=0;i<SONORK_VIEW_TAB_COUNT;i++)
		view.ctrl[i].ReleaseCtrl();

	view.drop_target.Enable( false );
	ClearViewGroup( SONORK_VIEW_GROUP_NONE );


}

// ----------------------------------------------------------------------------

void	TSonorkMainWin::OnDestroy()
{}

// ----------------------------------------------------------------------------

void	TSonorkMainWin::OnAfterDestroy()
{
	SonorkApp.PostAppCommand(SONORK_APP_COMMAND_HALTED,0);
}

// ----------------------------------------------------------------------------
// 		HELPERS & ETC
// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetToolBarIcon( UINT btn_id, SKIN_ICON icon)
{
	::SendMessage(tool_bar.hwnd
		,TB_CHANGEBITMAP
		,btn_id + TOOL_BAR_BASE_ID
		,MAKELPARAM(icon,0));

}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetToolBarState( UINT btn_id, UINT state)
{
	::SendMessage(tool_bar.hwnd
		,TB_SETSTATE
		,btn_id + TOOL_BAR_BASE_ID
		,MAKELONG(state,0));
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetupToolBar()
{
	static TSonorkWinToolBarButton	btn_info[TOOL_BAR_BUTTONS]=
	{
		{	CM_CONNECT			+ TOOL_BAR_BASE_ID
			, SKIN_ICON_DISCONNECTED
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{	CM_ADD_USER_CTX_GLOBAL		+ TOOL_BAR_BASE_ID
			, SKIN_ICON_ADD_USER
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON }
	,	{	CM_PIN_WINDOW			+ TOOL_BAR_BASE_ID
			, SKIN_ICON_ADJUST
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{	CM_APP_CLIPBOARD		+ TOOL_BAR_BASE_ID
			, SKIN_ICON_NOTES
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{	CM_APP_REMINDER			 + TOOL_BAR_BASE_ID
			, SKIN_ICON_TIME
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	};

	tool_bar.hwnd=TSonorkWin::CreateToolBar(
				  Handle()
				, TOOL_BAR_ID
				, TOOL_BAR_STYLE
				, TOOL_BAR_BUTTONS
				, btn_info
				, &tool_bar.size);
}


// ----------------------------------------------------------------------------
// Sizing

void	TSonorkMainWin::OnSize(UINT sz_type)
{
	if( sz_type == SIZE_RESTORED )
	{
		RECT rect;
		GetWindowRect(&rect);

		RealignControls();
	}
	else
	if( sz_type == SIZE_MINIMIZED )
	{
		if(SonorkApp.IsProfileOpen())
		if(!SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_HIDE_ON_MINIMIZE))
			ShowWindow(SW_HIDE);

	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MAIN_WIN_MIN_WIDTH;//164;
	MMI->ptMinTrackSize.y=MAIN_WIN_MIN_HEIGHT;//400;
	MMI->ptMaxTrackSize.x=MAIN_WIN_MAX_WIDTH;//164*2;
	MMI->ptMaxTrackSize.y=SonorkApp.DesktopSize().cy;
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::RealignControls()
{
	const UINT H = Height() ;
	const UINT W = Width();
	const UINT VW= Width() - (TAB_LEFT_DELTA+TAB_RIGHT_DELTA);
	UINT	top;
	UINT	tab_area_height;
	UINT	i;
	HDWP 	defer_handle;

	top		= 0;

	//ticker_height = ticker.win->IsEmbedded()?ticker.height:0;
	defer_handle	= BeginDeferWindowPos( 12 );

	defer_handle=
		DeferWindowPos(defer_handle
			,bicho.hwnd
			,NULL
			,LEFT_DELTA
			,top
			,0
			,0
			,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE);

	defer_handle=
		DeferWindowPos(defer_handle
			,tool_bar.hwnd
			,NULL
			,LEFT_DELTA + SONORK_ICON_SW + (LEFT_DELTA<<1)
			,top + ((TOOL_BAR_AREA_HEIGHT-tool_bar.size.cy)>>1)
			,tool_bar.size.cx//Width()-(LEFT_DELTA<<1)
			,TOOL_BAR_AREA_HEIGHT	//SONORK_ICON_SH
			,SWP_NOZORDER|SWP_NOACTIVATE);

	top+=SONORK_ICON_SH;

	defer_handle=
		DeferWindowPos(defer_handle
			,info.hwnd
			,NULL
			,0
			,top
			,W
			,info.height
			,SWP_NOZORDER|SWP_NOACTIVATE);

	top+= info.height + INFO_BOTTOM_MARGIN + TAB_TOP_MARGIN;


	tab_area_height   =	H -
				(info.height
				+ logo.height
				+ SONORK_ICON_SH
				+ TAB_BOTTOM_MARGIN
				+ TAB_TOP_MARGIN
				+ INFO_BOTTOM_MARGIN
				+ MODE_BOTTOM_MARGIN
				+ sid_mode_combo.height);

	defer_handle=
		DeferWindowPos(defer_handle
			,view.tabHwnd
			,HWND_BOTTOM
			,0
			,top
			,W
			,tab_area_height //TAB_BAR_HEIGHT//view.tab_height
			,SWP_NOACTIVATE);

	top+=TAB_TOP_DELTA;
	
	defer_handle=
		DeferWindowPos(defer_handle
			,view.titleHwnd
			,HWND_TOP
			,TAB_LEFT_DELTA
			,top
			,VW
			,TAB_TITLE_HEIGHT //TAB_BAR_HEIGHT//view.tab_height
			,SWP_NOACTIVATE);


	top+=TAB_TITLE_HEIGHT;
	view.top	=top;
	view.height =tab_area_height - TAB_TOP_DELTA - TAB_BOTTOM_DELTA - TAB_TITLE_HEIGHT;

	for(i=0;i<SONORK_VIEW_TAB_COUNT;i++)
		defer_handle=
			DeferWindowPos(defer_handle
				,view.tree[i].Handle()
				,HWND_TOP
				,TAB_LEFT_DELTA
				,view.top
				,VW
				,view.height
				,SWP_NOACTIVATE	);


	top+=view.height + TAB_BOTTOM_DELTA + TAB_BOTTOM_MARGIN;

	defer_handle=
		DeferWindowPos(defer_handle
			,sid_mode_combo.hwnd
			,NULL
			,0
			,top
			,W
			,200 // sid_mode_combo.hwnd is a combo box: Height refers to open list
			,SWP_NOZORDER|SWP_NOACTIVATE);

	top+=  sid_mode_combo.height  + MODE_BOTTOM_MARGIN + 1;
	defer_handle=
		DeferWindowPos(defer_handle
		,logo.hwnd
		,NULL
		,0
		,top
		,W
		,logo.height
		,SWP_NOZORDER|SWP_NOACTIVATE);


	EndDeferWindowPos(defer_handle);

	// Substract borders to get the client area
	// which we use in TSonorkMainWin::SetFocusTarget()
//	view.tree_height - GetSystemMetrics(SM_CYBORDER)*2;
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnMeasureItem( MEASUREITEMSTRUCT* DS  )
{

	return SonorkApp.MenuMeasureItem(DS);
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::OnDrawItem( DRAWITEMSTRUCT*DS )
{
	HDC		tDC;
	SKIN_ICON 	ic;
	int 		x,y;
	RECT&		rcItem=DS->rcItem;
	tDC=DS->hDC;

	if( DS->CtlType== ODT_MENU )
	{
		return SonorkApp.MenuDrawItem(DS);
	}
	else
	if( DS->CtlID == IDC_MAIN_SID_MODE )
	{
		DrawSidMode( DS );
	}
	else
	if( DS->CtlID == IDC_MAIN_TAB )
	{
		::FillRect(tDC
			,&rcItem
			, sonork_skin.Brush(SKIN_BRUSH_DIALOG));
		x = rcItem.left + ((rcItem.right-rcItem.left) - SKIN_ICON_SW)/2;
		y = rcItem.top  + ((rcItem.bottom-rcItem.top) - SKIN_ICON_SH)/2;
		switch(DS->itemID)
		{
			case SONORK_VIEW_TAB_1:
			ic=SKIN_ICON_USERS;
			break;

			case SONORK_VIEW_TAB_2:
			ic=SKIN_ICON_APP;
			break;

			default:
			ic=SKIN_ICON_ERROR;
			break;

		}
		sonork_skin.DrawIcon(tDC
			, ic
			, x
			, y
			,ILD_NORMAL);

	}
	else
	if( DS->CtlID == IDC_MAIN_LOGO )
	{
		sonork_skin.DrawLargeLogo(tDC
			, &rcItem
			, true);
	}
	else
		return false;
	return true;
}

// ----------------------------------------------------------------------------
// 		TREE VIEW
// ----------------------------------------------------------------------------

LRESULT
 TSonorkMainWin::OnNotify(WPARAM ,TSonorkWinNotify*N)
{
	/*
	union {
		HTREEITEM		hitem;
		NMHDR			*hdr;
		TV_KEYDOWN		*key;
		TV_DISPINFO 		*dispinfo;
		NM_TREEVIEW		*view;
		NMTVGETINFOTIP		*tip;
		NMCUSTOMDRAW		*cdraw;
		NMTVCUSTOMDRAW		*tdraw;
	}N;
	*/
	char 			*tgt;
	GLS_INDEX 		gls_index;
	POINT			pt;
	TSonorkExtUserData	*UD;
	TSonorkViewItemPtrs	VP;

	if( N->hdr.hwndFrom == GetActiveTree().Handle()
	 || N->hdr.code == TVN_DELETEITEM)
	{

// ----------------------------
// Context Tree View

		switch(N->hdr.code)
		{
			// -----------------
			// Special notes on how clicking events are handled for the tree view
			// -----
			// The TreeView sends NM_CLICK/NM_DBLCLK/... notifications
			// when the mouse is [double] clicked, regardless where it
			// is clicked, so we can't rely only on which
			// item is currently selected.
			// For example: Suppose item <X> is selected and the user
			// double clicks on the expand [+] button of ANOTHER <Y>
			// item; this does NOT select the <Y> item, but sill generates
			// a NM_DBLCLK while <X> is selected.
			// To fix this, we get the mouse cursor position and see which item is
			// below it; if there is one, we are sure that we will process the
			// item that the user clicked.

// ------

			case NM_CLICK:
			if(!SonorkApp.IsSelectKeyDown())
			{
				if( view.sel_count )
					ClearSelection(true);
				// Get mouse cursor position.
				::GetCursorPos(&pt);
				VP.ptr  = GetActiveTree().HitTest(
					pt.x
					,pt.y
					,GTV_ICON_HIT_FLAGS);
				if( VP.item!=NULL )
					if( VP.item->Type() == SONORK_TREE_ITEM_GROUP )
						SendPoke(SONORK_MAINWIN_POKE_TOGGLE_EXPAND_ITEM
							, (LPARAM)VP.item);
				break;
			}
			// break ommited: Continue to NM_DBLCLK

// ------

			case NM_DBLCLK:

				::GetCursorPos(&pt);
				VP.ptr = GetActiveTree().HitTest(
						pt.x
						,pt.y
						,GTV_FULL_HIT_FLAGS);
				if(VP.item!=NULL)
				{
					if(N->hdr.code == NM_DBLCLK)
					{
						if( VP.item->Type() == SONORK_TREE_ITEM_GROUP)
							if( !VP.group->GetEventCount() )
								break;
						SendPoke(SONORK_MAINWIN_POKE_EXEC_ITEM,(LPARAM)VP.item);
					}
					else
					{
						SendPoke(SONORK_MAINWIN_POKE_TOGGLE_SELECT_ITEM
							,(LPARAM)VP.item);
					}
				}

				view.ctxVP.item=NULL;

			break;

// ------

			case NM_RCLICK:
			{
				// Get mouse cursor position.
				::GetCursorPos(&pt);

				view.ctxVP.ptr = GetActiveTree().HitTest(
						pt.x
						,pt.y
						,GTV_FULL_HIT_FLAGS);

				if(view.ctxVP.item != NULL)
				{
					GetActiveTree().FocusItem( view.ctxVP.item );
					OnViewRClick(pt);
					// Don't set the view.ctxVP.item to NULL
					// (we'll use it when the WM_COMMAND is sent
					// and reset it there )
					return true;
				}

			}
			break;

// ------

			case NM_CUSTOMDRAW:
				return TSonorkTreeItem::OnCustomDraw(&N->tdraw);

			case TVN_GETDISPINFO:		// asking for presentation
				return TSonorkTreeItem::OnGetDispInfo(&N->tdispinfo);

// ------
			case TVN_DELETEITEM:       // a item is being deleted
			{
				VP.item=(TSonorkMainViewItem*)N->tview.itemOld.lParam;

				// Please see TSonorkMainWin::RecalcViewItemParent()
				//  for notes on the MOVING_VIEW_ITEM flag
				if( !TestWinUsrFlag(MOVING_VIEW_ITEM) )
				{
					assert(VP.item!=NULL);
					if((VP.item->v_flags & SONORK_VIEW_ITEM_F_SELECTED) )
						if(view.sel_count)
						{
							view.sel_count--;
							_BroadcastSelectionEvent();
						}
					SONORK_MEM_DELETE(VP.item);
				}
				else
					VP.item->hitem=NULL;
			}
			break;

// ------
			case TVN_BEGINDRAG:

			if(view.ctxTab == SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS
			|| view.ctxTab == SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS)
			{
				DoDrag(&N->tview);

			}
			return FALSE;

// ------
			case TVN_GETINFOTIP:

			if(N->ttip.cchTextMax < 192 || SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_TOOL_TIPS))
				break;
			VP.item=(TSonorkMainViewItem*)N->ttip.lParam;
			assert( VP.item != NULL );
			tgt = N->ttip.pszText;
			switch( VP.item->Type() )
			{
				case SONORK_TREE_ITEM_EXT_USER:
				{
					UD = VP.user->UserData();
					SonorkApp.GetUserModeViewInfo(UD->SidMode(),UD->InfoFlags(),&gls_index);
					tgt += wsprintf(tgt,"%u.%u %-.32s\n[%s]"
						, UD->userId.v[0]
						, UD->userId.v[1]
						, UD->name.CStr()
						, SonorkApp.LangString(gls_index));
					if( !UD->email.IsNull() )
						if(*UD->email.CStr())
						{
							tgt+=wsprintf(tgt,"\n%-.32s",UD->email.CStr());
						}
					if(TSonorkWin32App::MakeTimeStr( UD->BornTime() ,tgt+1, MKTIMESTR_DATE))
						*tgt='\n';
					else
						*tgt=0;
				}
				break;

				case SONORK_TREE_ITEM_GROUP:
				switch( VP.group->Group() )
				{
					case SONORK_VIEW_SYS_GROUP_LOCAL_USERS:
						gls_index=GLS_TH_USRS;
						break;

					case SONORK_VIEW_SYS_GROUP_AUTHS:
						gls_index=GLS_TH_AUTHS;
						break;

					case SONORK_VIEW_SYS_GROUP_REMOTE_USERS:
						gls_index=GLS_TH_RUSRS;
						break;

					default:
						return 0L;
				}
				lstrcpyn(tgt
					,SonorkApp.LangString(gls_index)
					,N->ttip.cchTextMax);
				break;


				default:
				break;

			}
			break;
			
			case TVN_ITEMEXPANDING:
			if(N->tview.action == TVE_COLLAPSE)
			{
				VP.item=(TSonorkMainViewItem*)N->tview.itemNew.lParam;
				if(VP.item->parent==NULL)
					return true;
			}
			return false;


// ------
			case TVN_ITEMEXPANDED:
			default:

			break;
		}
	}
	else
	if( N->hdr.hwndFrom == view.tabHwnd )
	{
		if(N->hdr.code == TCN_SELCHANGE)
			SetActiveTab(
				(SONORK_VIEW_TAB)TabCtrl_GetCurSel(view.tabHwnd)
				, false
				, false);
	}
	else
	if( N->hdr.hwndFrom == tool_bar.hwnd )
	{
		if( N->hdr.code == NM_CUSTOMDRAW )
		{
			if( N->cdraw.dwDrawStage == CDDS_PREPAINT )
			{
				return  CDRF_NOTIFYITEMDRAW ;
			}
			else
			if( N->cdraw.dwDrawStage == CDDS_ITEMPREPAINT )
			{
				::FillRect(N->cdraw.hdc
					,&N->cdraw.rc
					,sonork_skin.Brush(SKIN_BRUSH_DIALOG));
				return TBCDRF_NOETCHEDEFFECT |TBCDRF_NOMARK;
			}
		}

	}
	else
	if( N->hdr.code == NM_CUSTOMDRAW )
	{
		// UNHANDLED NM_CUSTOM DRAW
	}
	return 0L;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkMainWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	TSonorkViewItemPtrs 	VP;
	long			view_client_y;
	union {
		TSonorkDropMsg*		drop_msg;
		TSonorkDropExecute*	drop_exec;
	}D;
	TSonorkClipData 	*CD;
	TSonorkExtUserData 	*UD;

	switch(event)
	{
		case SONORK_DRAG_DROP_INIT:
			SetDropTarget(NULL,-1);
			break;

		case SONORK_DRAG_DROP_QUERY:
		{
			SonorkApp.ProcessDropQuery(
				(TSonorkDropQuery*)lParam
				,	(
					 SONORK_DROP_ACCEPT_F_SONORKCLIPDATA
					|SONORK_DROP_ACCEPT_F_FILES
					|SONORK_DROP_ACCEPT_F_URL
					)
				);
		}
		break;
		case SONORK_DRAG_DROP_UPDATE:
		{
			D.drop_msg=(TSonorkDropMsg*)lParam;
			VP.ptr  = GetActiveTree().HitTest(
						 D.drop_msg->point->x
						,D.drop_msg->point->y
						,GTV_FULL_HIT_FLAGS
						,&view_client_y);
			if( VP.item != NULL )
			{
				if(VP.item->Type() == SONORK_TREE_ITEM_EXT_USER)
				{
					if( TestWinUsrFlag(DRAGGING_USER_DATA) )
						VP.item=NULL;
				}
				else
				if( VP.item->Type() ==  SONORK_TREE_ITEM_GROUP )
				{
					if( VP.group->IsUserGroup() )
						GetActiveTree().ExpandItemLevels(
							  VP.item->hitem
							, TVE_EXPAND
							, 0);
				}
				else
				{
					VP.item = NULL;
				}
			}

			if( VP.item == NULL )
			{
				*D.drop_msg->drop_effect=DROPEFFECT_NONE;
				SetDropTarget( NULL , view_client_y);
			}
			else
			{
				*D.drop_msg->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE;
				SetDropTarget( VP.item->hitem  , view_client_y);
			}
		}
		break;
		case SONORK_DRAG_DROP_CANCEL:
			SetDropTarget( NULL , -1);
		break;
		case SONORK_DRAG_DROP_EXECUTE:
		{
			D.drop_exec=(TSonorkDropExecute*)lParam;
			VP.ptr = GetActiveTree().HitTest(
					 D.drop_exec->point->x
					,D.drop_exec->point->y
					,GTV_FULL_HIT_FLAGS);
			while( VP.item != NULL )
			{
				if(VP.item->Type() == SONORK_TREE_ITEM_EXT_USER)
				{
					if( !TestWinUsrFlag(DRAGGING_USER_DATA))
					{
						CD = SonorkApp.ProcessDropExecute( D.drop_exec );
						SonorkApp.ProcessUserDropData(VP.user->UserData()
							,CD
							,true);
					}
					break;
				}

				if(VP.item->Type() == SONORK_TREE_ITEM_GROUP)
				{
					CD = SonorkApp.ProcessDropExecute( D.drop_exec );
					if(!CD)break;
					if( TestWinUsrFlag(DRAGGING_USER_DATA) )
					{
						if( VP.group->IsLocalUserGroup()
						&&  CD->DataType() == SONORK_CLIP_DATA_USER_DATA )
						{
							UD = SonorkApp.GetUser(CD->GetUserData()->userId);
							if( UD != NULL )
							if( UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
							{
								Task_AssignGroupToUser(
									  UD
									, VP.custom_group->Group() );
							}
						}
					}
					else
					{
						view.ctxVP.item = VP.item;
						CmdGroupMsg( CD );      	
					}
					CD->Release();
				}
				
				break;
			}
			SetDropTarget( NULL , -1);
		}
		break;
	}
	return 0L;
}

// ----------------------------------------------------------------------------

UINT
 TSonorkMainWin::_ToggleSelectItem( TSonorkMainViewItem* VI )
{
	UINT			sel_count=0;
	UINT			aux;
	TSonorkViewItemPtrs	VP;
	if( VI != NULL)
	{
		if( VI->Type() == SONORK_TREE_ITEM_EXT_USER )
		{
			if(VI->v_flags & SONORK_VIEW_ITEM_F_SELECTED)
			{
				VI->v_flags&=~SONORK_VIEW_ITEM_F_SELECTED;
				if(view.sel_count)
					view.sel_count--;
			}
			else
			{
				sel_count++;
				view.sel_count++;
				VI->v_flags|=SONORK_VIEW_ITEM_F_SELECTED;
			}
			GetActiveTree().Repaint( VI );
		}
		else
		if( VI->Type() == SONORK_TREE_ITEM_GROUP )
		{
			for( 	 VP.ptr = GetActiveTree().GetChild( VI )
				;VP.ptr != NULL
				;VP.ptr = GetActiveTree().GetNextSibling(VP.ptr))
			{
				aux=_ToggleSelectItem( VP.item );
				GetActiveTree().ExpandItemLevels( VP.item
					, aux?TVE_EXPAND:TVE_COLLAPSE
					, 0 );
				sel_count += aux;
			}
		}
	}
	return sel_count;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkMainWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	TASK_TYPE 		task_type;
	TSonorkViewItemPtrs 	VP;
	switch(wParam)
	{
		case SONORK_WIN_POKE_NET_RESOLVE_RESULT:
			if(task.pop == NULL )
				break;
#define RR  ((TSonorkAppNetResolveResult*)lParam)
			task.pop->resolve_handle = NULL;
			if( RR->ERR.Result() == SONORK_RESULT_OK )
				if(Pop_Connect(*RR->physAddr))
					break;
#undef RR
		// break ommited: Continue to SONORK_MAINWIN_POKE_CHECK_MAIL
		
		case SONORK_MAINWIN_POKE_CHECK_MAIL:
			Pop_Start();
		break;

		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			task_type = task.type;
			task.type	= TASK_NONE;
			Task_UpdateStatus();
			if(task_type == TASK_DEL_USER)
				MessageBox(GLS_AUTHREQSENT,GLS_OP_DELUSR,MB_OK);
		break;

		case SONORK_MAINWIN_POKE_SHOW_LOGIN_DIALOG:
			ShowLoginDialog();
		break;
		case SONORK_MAINWIN_POKE_CONNECT_DISCONNECT:
			CmdConnect( lParam!=0 );
		break;

		case SONORK_MAINWIN_POKE_TOGGLE_SELECT_ITEM:
			SetSelection((TSonorkMainViewItem*)lParam, false);
			break;

		case SONORK_MAINWIN_POKE_TOGGLE_EXPAND_ITEM:
			VP.item = (TSonorkMainViewItem*)lParam;
			if( VP.item != NULL)
			if( VP.item->Type() == SONORK_TREE_ITEM_GROUP )
				GetActiveTree().ExpandItemLevels(
					  VP.item->hitem
					, TVE_TOGGLE
					, 0);
			break;

		case SONORK_MAINWIN_POKE_EXEC_ITEM:
		case SONORK_MAINWIN_POKE_EXEC_ITEM_NO_CTRL:
			VP.item = (TSonorkMainViewItem*)lParam;
			if( VP.item != NULL)
			switch( VP.item->Type() )
			{
				case SONORK_TREE_ITEM_EXT_USER:
				{
					if(SonorkApp.IsControlKeyDown()
						&& wParam!=SONORK_MAINWIN_POKE_EXEC_ITEM_NO_CTRL)
						SonorkApp.OpenUserDataWin(
							 VP.user->UserData()
							,(TSonorkWin*)NULL
							,TSonorkUserDataWin::TAB_AUTH);
					else
						SonorkApp.OpenMsgWindow(VP.user->UserData() , SONORK_MSG_WIN_OPEN_FOREGROUND );
				}
				break;
				
				case SONORK_TREE_ITEM_GROUP:
				VP.item = FindFirstItemWithEvents(
					 GetActiveTree()
					,VP.item->hitem);
				if(VP.item)
				{
					GetActiveTree().FocusItem(VP.item);
					PostPoke( wParam , (LPARAM)VP.item );
				}
				break;
				
				case SONORK_TREE_ITEM_AUTH_REQ:
				{
					SonorkApp.OpenUserAuthReqWin(VP.auth->userId);
				}
				break;

				case SONORK_TREE_ITEM_APP:
					if(VP.app->MayExecute())
					{
						if(VP.app->AppType() == SONORK_APP_TYPE_WEB)
						{
							SonorkApp.LaunchWebApp(this, VP.webapp->WappData() , NULL , NULL );
						}
						else
						if( VP.app->AppType() == SONORK_APP_TYPE_EXTERNAL
						||  VP.app->AppType() == SONORK_APP_TYPE_INTERNAL)
						{
							SonorkApp.LaunchAppByCmd( VP.app->AppMenuCmd()
								, NULL
								, NULL );
						}
					}
					else
					{
						SonorkApp.SetBichoSequenceError(true);
						ErrorBox(GLS_ERR_OPNEEDCX , NULL, SKIN_SIGN_ERROR);
					}
					break;
				default:
					break;
			}
			break;


		case SONORK_WIN_POKE_DESTROY:
			Destroy(IDOK);
		break;
	}

	return 0L;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::OnActivate(DWORD flags , BOOL )
{
	if( (flags==WA_ACTIVE||flags==WA_CLICKACTIVE) && Initialized() )
	{
		SonorkApp.OnMainWinActivated();

		// Don't auto-set focus to the active tree when the window is
		// activated with a mouse click because the user might be
		// clicking on the tree and by setting the focus we'll
		// focus the user mode combo (which was not clicked)
		// And if the user actually DOES click the combo, then there
		// is no reason for setting the focus, so it works in both
		// cases.

		if( flags != WA_CLICKACTIVE )
		{
			// Post the command: If SetFocus() is called
			// from within the WM_ACTIVATE handler, it does
			// not work. 
			SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND
				, (LPARAM)GetActiveTree().Handle());
		}

	}

}


// ----------------------------------------------------------------------------

void
 TSonorkMainWin::OnViewRClick(POINT& pt)
{
	SonorkApp.CancelAutoAwaySidMode();
	if( view.ctxVP.item != NULL )
	{
		HMENU	menu;
		HideTreeToolTips();
		switch( view.ctxVP.item->Type() )
		{
			case SONORK_TREE_ITEM_EXT_USER:
				if( view.sel_count == 0 )
					menu = SonorkApp.UserMenu();
				else
					menu = SonorkApp.UserSelectionMenu();
			break;


			case SONORK_TREE_ITEM_GROUP:
			if( view.ctxVP.group->IsUserGroup() )
			{
				menu = SonorkApp.UserGroupMenu();
				break;
			}
			else
			if( view.ctxVP.group->Group() == SONORK_VIEW_SYS_GROUP_EXT_APPS)
			{
				menu = SonorkApp.EappPopupMenu();
				break;
			}
			// break ommited, pass on to default: (no menu)

			default:
				view.ctxVP.item = NULL;
				return;
		}
		TrackPopupMenu(menu
			, TPM_LEFTALIGN  | TPM_LEFTBUTTON
			,pt.x
			,pt.y
			,0
			,Handle()
			,NULL);
	}
}



// ----------------------------------------------------------------------------
// Add/Del items
// ----------------------------------------------------------------------------

void
 TSonorkMainWin::AddViewItem( TSonorkMainViewItem* view_item )
{
	TSonorkViewItemPtrs	parent;
	TSonorkViewItemPtrs	VI;
	int			tree_no;


	tree_no = view_item->GetTree();

	VI.item = view_item;
	assert( VI.item->hitem == NULL );

	parent.item  = GetViewItemIntendedParent( VI.item );
	view.tree[ tree_no ].AddItem( parent.item
		, view_item
		, SonorkApp.TestRunFlag(SONORK_WAPP_RF_REFRESHING_LIST));

	if( parent.item != NULL )
	{
		assert( parent.item->Type() == SONORK_TREE_ITEM_GROUP );

		parent.group->f_count++;

		if( VI.item->Type() == SONORK_TREE_ITEM_GROUP )
			VI.group->depth = parent.group->Depth()+1;

		// Items that point to an invalid parent get assigned
		// a default parent; so this is a good time to
		// fix invalid references:
		// 	Check if the item was loaded in its intended
		// 	disposition group and, if not so, correct.
		if( VI.item->GetDispGroup() != parent.group->Group() )
		{
			// Not in the parent group pointed by the item: fix
			if( VI.item->Type() == SONORK_TREE_ITEM_EXT_USER )
			{
				TSonorkExtUserData*UD;
				UD=VI.user->UserData();
				UD->ctrl_data.auth.SetGroupNo(parent.group->Group());
			}
			else
			if( VI.item->Type() == SONORK_TREE_ITEM_GROUP )
			if( VI.group->IsCustomLocalUserGroup() )
			{
				VI.custom_group->parent_no = parent.group->Group();
			}
		}
	}

}

void
 TSonorkMainWin::DelViewItem(TSonorkMainViewItem*view_item)
{
	if(view_item==NULL)
		return;
	if( view_item->parent )
	{
		assert( view_item->parent->Type() == SONORK_TREE_ITEM_GROUP );
		((TSonorkMainGroupViewItem*)(view_item->parent))->f_count--;
	}
	view.tree[view_item->GetTree()].DelItem( view_item , false );
}


// ----------------------------------------------------------------------------
// Get tree items
// ----------------------------------------------------------------------------

TSonorkAppViewItem*
 TSonorkMainWin::GetAppViewItem(DWORD cmd)
{
	TSonorkMainViewItem *view_item;

	if( cmd>= SONORK_APP_CM_WAPP_BASE && cmd<SONORK_APP_CM_WAPP_LIMIT
		|| cmd>= SONORK_APP_CM_MTPL_BASE && cmd<SONORK_APP_CM_MTPL_LIMIT)
		view_item=view.group.webapps;
	else
		view_item=view.group.extapps;

	return (TSonorkAppViewItem*)
		view.tree[SONORK_VIEW_TAB_TREE_APPS].FindItem(
			  SONORK_TREE_ITEM_APP
			, cmd
			, view_item->hItem());
}


// ----------------------------------------------------------------------------

TSonorkMainGroupViewItem*
 TSonorkMainWin::GetGroupViewItem(SONORK_VIEW_GROUP grp, bool use_default) const
{
	SONORK_VIEW_TAB		top_view_tree;
	TSonorkMainGroupViewItem* 	top_view_item;
	TSonorkMainGroupViewItem* 	found_view_item;
	assert( grp > SONORK_VIEW_GROUP_ROOT );
	if( grp >= SONORK_VIEW_USER_GROUP_FIRST	&& grp <= SONORK_VIEW_USER_GROUP_LAST)
	{
		if( grp == SONORK_VIEW_SYS_GROUP_LOCAL_USERS )
			return view.group.local_users;
		top_view_tree	= SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS;
		top_view_item	= view.group.local_users;
	}
	else
	if( grp >= SONORK_VIEW_WAPP_GROUP_FIRST	&& grp <= SONORK_VIEW_WAPP_GROUP_LAST)
	{
		if( grp == SONORK_VIEW_SYS_GROUP_WEB_APPS )
			return view.group.webapps;

		top_view_tree = SONORK_VIEW_TAB_TREE_APPS;
		top_view_item = view.group.webapps;
	}
	else
	{
		switch(grp)
		{
			case SONORK_VIEW_SYS_GROUP_REMOTE_USERS:
				return view.group.remote_users;
			case SONORK_VIEW_SYS_GROUP_AUTHS:
				return view.group.auths;
			case SONORK_VIEW_SYS_GROUP_EXT_APPS:
				return view.group.extapps;
			default:
				return NULL;
			// SONORK_VIEW_SYS_GROUP_LOCAL_USERS
			// and SONORK_VIEW_SYS_GROUP_WEB_APPS
			// are handled by the code above
		}
	}
	found_view_item = (TSonorkMainGroupViewItem*)
			view.tree[top_view_tree].FindItem(
				SONORK_TREE_ITEM_GROUP
				, grp
				, top_view_item->hitem);
	if(found_view_item!=NULL)
		return found_view_item;

	if(!use_default)
		return NULL;

	return top_view_item;
}

// ----------------------------------------------------------------------------

TSonorkExtUserViewItem*
 TSonorkMainWin::GetExtUserViewItem(const TSonorkId& user_id)
{
	return GetExtUserViewItem( SonorkApp.GetUser( user_id ) );
}

// ----------------------------------------------------------------------------

TSonorkExtUserViewItem*
 TSonorkMainWin::GetExtUserViewItem(const TSonorkExtUserData*UD)
{
	SONORK_VIEW_TAB	tb;
	TSonorkMainViewItem* view_item;
	if( UD == NULL )return NULL;

	if(UD->UserType() == SONORK_USER_TYPE_AUTHORIZED)
	{
		tb		= SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS;
		view_item 	= view.group.local_users;
	}
	else
	{
		tb 		= SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS;
		view_item 	= view.group.remote_users;
	}
	return (TSonorkExtUserViewItem*)
		view.tree[tb].FindItem(
		  SONORK_TREE_ITEM_EXT_USER
		, (DWORD)UD
		, view_item->hitem);
}

// ----------------------------------------------------------------------------

TSonorkAuthReqViewItem*
	TSonorkMainWin::GetAuthReqViewItem(const TSonorkId& userId)
{
	TSonorkViewItemPtrs 	VP;
	const TSonorkTreeView&	TV=view.tree[SONORK_VIEW_TAB_TREE_AUTHS];

	for(	VP.ptr = TV.GetChild( view.group.auths )
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if( VP.item->Type() != SONORK_TREE_ITEM_AUTH_REQ )
			continue;
		if( VP.auth->userId == userId )
			return VP.auth;
	}
	return NULL;
}

// ----------------------------------------------------------------------------

TSonorkMainViewItem*
 TSonorkMainWin::GetViewItemIntendedParent(TSonorkMainViewItem* view_item) const
{
	SONORK_VIEW_GROUP 	intended_parent_group;
	SONORK_VIEW_GROUP 	group;
	TSonorkMainViewItem*	parent;

	intended_parent_group = view_item->GetDispGroup();
	if( intended_parent_group == SONORK_VIEW_GROUP_ROOT )
	{
		parent = NULL;
	}
	else
	{
		parent = GetGroupViewItem( intended_parent_group , true );
		if( parent == NULL )
		{
			if( view_item->Type() == SONORK_TREE_ITEM_EXT_USER )
			{
				parent = view.group.local_users;
			}
			else
			if( view_item->Type() == SONORK_TREE_ITEM_GROUP )
			{
				group = ((TSonorkMainGroupViewItem*)view_item)->Group();
				if(group>=SONORK_VIEW_USER_GROUP_FIRST && group<=SONORK_VIEW_USER_GROUP_LAST)
				{
					parent = view.group.local_users;
				}
				else
				if(group>=SONORK_VIEW_WAPP_GROUP_FIRST && group<=SONORK_VIEW_WAPP_GROUP_LAST)
				{
					parent = view.group.webapps;
				}
			}
		}
		assert( parent != NULL );
	}
	return parent;
}


// ----------------------------------------------------------------------------
// Find tree items
// ----------------------------------------------------------------------------

TSonorkAuthReqViewItem*
 TSonorkMainWin::FindFirstIncommingAuthReqViewItem()
{
	return (TSonorkAuthReqViewItem*)
	       view.tree[SONORK_VIEW_TAB_TREE_AUTHS].FindItem(
		SONORK_TREE_ITEM_AUTH_REQ
		,1
		,view.group.auths->hitem);
}

// ----------------------------------------------------------------------------

TSonorkMainViewItem*
 TSonorkMainWin::FindFirstItemWithEvents(const TSonorkTreeView& TV,HTREEITEM pItem)
{
	return (TSonorkMainViewItem*)TV.GetFirstItemWithEvents(pItem);
}

// ---------------------------------------------------------------------------
// Count/Load items
// ---------------------------------------------------------------------------

int
 TSonorkMainWin::CountAuthReqViewItems(bool incomming, bool outgoing)
{

const	TSonorkTreeView&	TV=view.tree[SONORK_VIEW_TAB_TREE_AUTHS];
	TSonorkViewItemPtrs	VP;
	int			count=0;
	for( 	 VP.ptr = TV.GetChild( view.group.auths )
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if( VP.ptr->Type() != SONORK_TREE_ITEM_AUTH_REQ )
			continue;

		if( VP.auth->Incomming() )
		{
			if(incomming)count++;
		}
		else
		{
			if(outgoing)count++;
		}
	}
	return count;
}

// ----------------------------------------------------------------------------

HMENU
 TSonorkMainWin::LoadWappTemplatesMenuFromViewItems(HTREEITEM cItem)
{
	HMENU			c_menu;
	HMENU			s_menu;
	SKIN_ICON		dummy;
const	TSonorkTreeView&	TV=view.tree[SONORK_VIEW_TAB_TREE_MTPL];
	TSonorkViewItemPtrs	VP;
	BOOL			is_root;
	is_root=(cItem == NULL);
	if( is_root )
	{
		cItem = view.group.webapps->hitem;
		c_menu=SonorkApp.UserMtplMenu();
	}
	else
		c_menu=NULL;

	for(     VP.ptr = TV.GetChild(cItem)
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if( VP.item->Type() == SONORK_TREE_ITEM_APP)
		{
			if( VP.app->AppType() == SONORK_APP_TYPE_WEB)
			if( VP.webapp->WappType() == SONORK_WAPP_TYPE_MTPL)
			if( VP.webapp->WappFlags() & SONORK_WAPP_F_USER_CONTEXT)
			{
				if(c_menu == NULL)
					c_menu=CreatePopupMenu();
				::AppendMenu(c_menu
					,MF_STRING
					,VP.app->AppMenuCmd()
					,VP.app->GetLabel(false,dummy));
			}
		}
		else
		if( VP.item->Type() == SONORK_TREE_ITEM_GROUP )
		{
			s_menu = LoadWappTemplatesMenuFromViewItems( VP.item->hitem );
			if( s_menu != NULL )
			{
				if(c_menu == NULL)c_menu=CreatePopupMenu();
				::AppendMenu(c_menu
					,MF_POPUP
					,(UINT)s_menu
					,VP.item->GetLabel(false,dummy));
			}
		}
	}
	if( is_root )
	{
		TSonorkWin::AppendMenu(c_menu
			,0
			,CM_WAPP_OPEN|SONORK_WIN_CTF_ELLIPSIS
			,GLS_OP_OPEN
			);
	}
	return c_menu;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkMainWin::GetFreeGroupNo()
{
	DWORD	group_no=1;
	while(_GetFreeGroupNo(TVI_ROOT,group_no,0) != false)
		{}
	return group_no;
}

// ----------------------------------------------------------------------------

bool
 TSonorkMainWin::_GetFreeGroupNo(HTREEITEM cHitem, DWORD& group_no, UINT level)
{
const	TSonorkTreeView&	TV=view.tree[SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS];
	bool 			modified=false;
	TSonorkViewItemPtrs	VP;

	for( 	 VP.ptr = TV.GetChild( cHitem )
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if( VP.item->Type() != SONORK_TREE_ITEM_GROUP)
			continue;
		if( (DWORD)VP.group->Group() == group_no )
		{
			group_no++;
			modified=true;
		}
		if(_GetFreeGroupNo(VP.ptr->hItem(),group_no,level+1))
			modified=true;
	}
	return modified;
}

// ----------------------------------------------------------------------------
// Focus/expansion
// ----------------------------------------------------------------------------

void
 TSonorkMainWin::ExpandGroupViewItemLevels(SONORK_VIEW_GROUP grp
	, BOOL expand
	, int  sub_levels)
{
	TSonorkMainViewItem *VI;

	VI = GetGroupViewItem( grp , true );

	assert( VI != NULL );


	view.tree[ VI->GetTree() ].ExpandItemLevels( VI
		, expand?TVE_EXPAND:TVE_COLLAPSE
		, sub_levels );
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::FocusViewItem(TSonorkMainViewItem*view_item)
{
	SONORK_VIEW_TAB tab;
	if(view_item == NULL)return;

	tab = view_item->GetTree();

	SetActiveTab( tab , false , true );

	view.tree[tab].FocusItem( view_item );
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::EnsureVisibleViewItem(TSonorkMainViewItem* view_item, BOOL force_view_visible)
{
	SONORK_VIEW_TAB tab;
	if(view_item == NULL)return;

	tab = view_item->GetTree();
	if(force_view_visible)
		SetActiveTab( tab , false , true );
	view.tree[tab].EnsureVisible(  view_item );
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::EnsureVisibleGroupViewItem(SONORK_VIEW_GROUP grp, BOOL force_view_visible)
{
	EnsureVisibleViewItem( GetGroupViewItem(grp, true ) , force_view_visible);
}

// ----------------------------------------------------------------------------
// Clear items
// ----------------------------------------------------------------------------

void
 TSonorkMainWin::ClearViewGroup(SONORK_VIEW_GROUP grp)
{
	ClearSelection( !TestWinSysFlag(SONORK_WIN_SF_DESTROYING) );
	if( grp == SONORK_VIEW_GROUP_NONE )
	{
		// NB:grp==SONORK_VIEW_GROUP_NONE
		// all groups in all trees are delete
		for(int i=SONORK_VIEW_TAB_FIRST ; i<=SONORK_VIEW_TAB_LAST ; i++)
			view.tree[i].DelAllItems();
			
	}
	else
	{
		TSonorkMainGroupViewItem *gvi=GetGroupViewItem( grp , true );
		assert( gvi!=NULL );
		view.tree[ gvi->GetTree() ].DelItemChildren(
			  gvi
			, SonorkApp.TestRunFlag(SONORK_WAPP_RF_REFRESHING_LIST));
		gvi->b_count	=
		gvi->e_count	=
		gvi->f_count	=	0;
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::ClearViewItemChildren(HWND tHwnd, HTREEITEM parent)
{
	HTREEITEM	cHitem,n_item;
	for(	cHitem = TreeView_GetChild(tHwnd,parent)
		; 	cHitem != NULL
		; 	cHitem = n_item)
	{
		n_item = TreeView_GetNextSibling(tHwnd,cHitem);
		ClearViewItemChildren(tHwnd,cHitem);
		TreeView_DeleteItem(tHwnd, cHitem);
	}
}

// ----------------------------------------------------------------------------
// Redraw/Update
// ----------------------------------------------------------------------------

// -----------------------------
// RecalcViewItemParent()
//  Checks if the current visual (real) parent is the same parent as the
//   node's attributes and updates the visual parent if they are not.
// Notes on TVN_DELETEITEM and the MOVING_VIEW_ITEM flag:
//  When we move a TSonorkMainViewItem from one tree view item (tv_item) to another,
//  we delete the HTREEITEM that is currently holding the TSonorkMainViewItem and
//  insert the TSonorkMainViewItem into another new HTREEITEM. The deletion will
//  fire the TVN_DELETEITEM event which normaly deletes the TSonorkMainViewItem
//  associated to the HTREEITEM (by the TVN_DELETEITEM handler).
//  To prevent the deletion of the TSonorkMainViewItem by the TVN_DELETEITEM handler,
//  we temporarily set the MOVING_VIEW_ITEM flag.
//  While this flag is set, the TVN_DELETEITEM handler will not delete
//  the TSonorkMainViewItem associated to the associate HTREEITEM.

void
 TSonorkMainWin::RecalcViewItemParent(TSonorkMainViewItem*view_item)
{
	if( (void*)view_item->Parent() != (void*)GetViewItemIntendedParent( view_item) )
	{
		SetWinUsrFlag(MOVING_VIEW_ITEM);
		DelViewItem( view_item );
		AddViewItem( view_item );
		ClearWinUsrFlag(MOVING_VIEW_ITEM);
	}
}

// ----------------------------------------------------------------------------
// RepaintViewItem()
// Invalidates the region occupied by the item
//  if <update_item_attr> is true, we also set the HTREEITEM attributes that
//  are not handled by the NM_CUSTOMDRAW (such as the BOLD attribute)

void
 TSonorkMainWin::RepaintViewItem( TSonorkMainViewItem*	view_item )
{
	view.tree[view_item->GetTree()].Repaint( view_item );
}


// ----------------------------------------------------------------------------
// UpdateViewItemAttributes()
// Should be invoked after modifying the view item is such way that
//  its visual attributes have changed.
// The SONORK_MAINWIN_UPDATE_ITEM_PROPAGATE flag will propagate
//  the b_delta and e_delta flags all the way to the top and
//  doing the same as UpdateViewItemAttributes() on each of them.
// The SONORK_MAINWIN_UPDATE_ITEM_SORT will sort items, starting
//  at the item's PARENT. 

void
 TSonorkMainWin::UpdateViewItemAttributes(
			  TSonorkMainViewItem*	view_item
			, int			b_delta
			, int 			e_delta
			, UINT			flags)
{
	if( !view_item )return;

	view.tree[view_item->GetTree()].UpdateItemAttributes(view_item,b_delta,e_delta,flags);
}

// ----------------------------------------------------------------------------
// AfterExtUserViewItemUpdate
//  Calls AfterViewItemUpdate() on the view item of an user

void
 TSonorkMainWin::UpdateUserViewItemAttributes(const TSonorkExtUserData*UD
				, int	sid_delta
				, int 	msg_delta
				, UINT	flags)
{
	UpdateViewItemAttributes(GetExtUserViewItem(UD),sid_delta,msg_delta,flags);
}

void
 TSonorkMainWin::AfterMassUpdate(SONORK_VIEW_TAB tb)
{
	if( tb == SONORK_VIEW_TAB_ALL )
	{
		for(int i=SONORK_VIEW_TAB_FIRST;i<=SONORK_VIEW_TAB_LAST;i++)
			view.tree[i].AfterMassOp( false );
	}
	else
		view.tree[tb].AfterMassOp( false );
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// Sorting
// ----------------------------------------------------------------------------

void
	TSonorkMainWin::SortViewGroup(SONORK_VIEW_GROUP grp, bool recursive)
{
	TSonorkMainViewItem *theITEM;

	theITEM = GetGroupViewItem( grp , true );

	assert(theITEM!=NULL);

	view.tree[theITEM->GetTree()].SortChildren( theITEM, recursive );
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------


void
 TSonorkMainWin::OnToolTipText(UINT id, HWND , TSonorkWinToolTipText&TTT )
{
	// Don't check the HWND parameter against the HWND of the tool bar:
	//  they don't match (apparently the message is sent with the handle
	//  of the tooltip window as the parameter and not with the handle
	//  of the control/tool bar that is requesting the tooltip)
	switch(id)
	{
		case CM_CONNECT + TOOL_BAR_BASE_ID:
			TTT.gls = SonorkApp.CxActiveOrPending()?GLS_OP_DISCONNECT:GLS_OP_CONNECT;
			break;

		case CM_ADD_USER_CTX_GLOBAL + TOOL_BAR_BASE_ID:
			TTT.gls = GLS_OP_FNDUSRS;
		break;

		case CM_APP_CLIPBOARD+ TOOL_BAR_BASE_ID:
			TTT.gls = GLS_LB_CLPBRD;
		break;

		case CM_APP_REMINDER+ TOOL_BAR_BASE_ID:
			TTT.gls = GLS_LB_RMINDLST;
		break;

		case CM_PIN_WINDOW + TOOL_BAR_BASE_ID:
			TTT.gls = GLS_LB_ADJWPOS;
		break;

	}

}


LRESULT
 TSonorkMainWin::OnCtlWinMsg(TSonorkWinCtrl*WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	BOOL		ctrl_down;
	if( uMsg == WM_GETDLGCODE )
	{
		if( lParam )
		{
			MSG *msg = (MSG*)lParam;
			if( msg->message == WM_KEYDOWN &&
				(msg->wParam == VK_RETURN || msg->wParam == VK_ESCAPE))
				return DLGC_WANTMESSAGE;
		}
	}
	else
	if( uMsg == WM_CHAR )
	{
		while( WC->Id() == IDC_MAIN_TREE_1 || WC->Id() == IDC_MAIN_TREE_2 )
		{
			ctrl_down =GetKeyState(VK_CONTROL)&0x80000;

			if( wParam == VK_RETURN
				|| (wParam==10 && ctrl_down) ) // M=ENTER J=CTRL+ENTER
			{
				//TreeView_GetSelection(GetCtxView());
				PostPoke( SONORK_MAINWIN_POKE_EXEC_ITEM
					, (LPARAM)GetActiveTree().GetSelectedItem()
					);
				return 0;
			}

			if( ctrl_down )
			{
				if( wParam == VK_SPACE ) // CTRL+SPACE
					SonorkApp.OpenNextEvent( false );
				else
					break;
				return 0;
			}

// -- break out of while --
			break;
		}
	}
	return WC->DefaultProcessing(uMsg,wParam,lParam);

}

void TSonorkMainWin::HideTreeToolTips()
{
	GetActiveTree().HideToolTip();
}

void
	TSonorkMainWin::SetHintMode(SONORK_C_CSTR , bool )
{
	// FIX THIS!!
	//SonorkApp.SetMainHint(str, 0, SONORK_APP_HINT_INFO);
}


// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetSelection(TSonorkMainViewItem*VI, bool clear_prev)
{
	UINT	aux;
	SONORK_VIEW_TAB		tab;
	TSonorkViewItemPtrs 	VP;
const	TSonorkTreeView*	TV;

	if(clear_prev)
		ClearSelection( true );

	if(VI==NULL)return ;
	VP.item = VI;

	tab = VP.item->GetTree();

	if( tab	!= view.ctxTab)	// item does not belong to visible tree
		return;

	if( view.ctxTab != SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS
	&&  view.ctxTab != SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS)
		return;

	if( VP.item->Type() != SONORK_TREE_ITEM_EXT_USER )
	{
		// Not an user: Check if its a group
		if( VP.item->Type() != SONORK_TREE_ITEM_GROUP)
			return;
		// Check if the group is valid
		if( !VP.group->IsUserGroup() )
			return;
	}
	TV = &GetActiveTree();
	aux = _ToggleSelectItem( VP.item ); // Toggle selection assumes ActiveTree()
	if(VP.item->Type() == SONORK_TREE_ITEM_GROUP)
	if(VP.group->IsCustomLocalUserGroup())
	{
		TV->ExpandItemLevels( VP.item
			, aux?TVE_EXPAND:TVE_COLLAPSE
			, 0);
	}
	TV->EnsureVisible(VP.item);
	_BroadcastSelectionEvent();
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::ClearSelection(BOOL redraw)
{
	if( view.sel_count )
	{
		_ClearSelection( GetActiveTree() , TVI_ROOT );
		view.sel_count=0;
		_BroadcastSelectionEvent();
		if(redraw)
			GetActiveTree().InvalidateRect(NULL,true);
	}

}

// ----------------------------------------------------------------------------

UINT
 TSonorkMainWin::GetSelection(SONORK_DWORD2List* list)
{
	if(list)list->Clear();
	if( view.sel_count )
	{
		if(list)
		{
			_GetSelection( GetActiveTree() , list , TVI_ROOT);
			view.sel_count = list->Items();
		}
		return view.sel_count;
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::_ClearSelection(const TSonorkTreeView& TV,HTREEITEM pHitem)
{
	TSonorkViewItemPtrs	VP;
	for( 	 VP.ptr = TV.GetChild( pHitem )
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if( VP.item->v_flags & SONORK_VIEW_ITEM_F_SELECTED )
			VP.item->v_flags &=~SONORK_VIEW_ITEM_F_SELECTED;
		_ClearSelection( TV , VP.item->hItem() );
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::_GetSelection(const TSonorkTreeView& TV
	, SONORK_DWORD2List*pList
	, HTREEITEM pHitem)
{

	TSonorkViewItemPtrs	VP;
	for( 	 VP.ptr = TV.GetChild( pHitem )
		;VP.ptr != NULL
		;VP.ptr = TV.GetNextSibling(VP.ptr))
	{
		if(VP.item->Type() == SONORK_TREE_ITEM_EXT_USER)
		{
			if(VP.item->v_flags & SONORK_VIEW_ITEM_F_SELECTED)
			{
				pList->AddItem( &VP.user->UserData()->userId );
			}
		}
		else
		if(VP.item->Type() == SONORK_TREE_ITEM_GROUP)
		{
			_GetSelection( TV , pList , VP.item->hItem() );
		}
	}
}


// ----------------------------------------------------------------------------

void
 TSonorkMainWin::_BroadcastSelectionEvent()
{
	if(!TestWinSysFlag(SONORK_WIN_SF_DESTROYING))
	{
		SonorkApp.BroadcastAppEvent(SONORK_APP_EVENT_MAIN_VIEW_SELECTION
			,SONORK_APP_EM_MAIN_VIEW_AWARE
			,view.sel_count
			,0);
		InfoWin()->OnMainViewSelection(view.sel_count);
	}
}

// ----------------------------------------------------------------------------
const TSonorkTreeView&
 TSonorkMainWin::GetTree(SONORK_VIEW_TAB t) const
{
	if( !(t>=0&& t<SONORK_VIEW_TAB_COUNT) )
		assert(t>=0&& t<SONORK_VIEW_TAB_COUNT);
	return view.tree[t];
}


// ----------------------------------------------------------------------------

void
 TSonorkMainWin::SetActiveTab(SONORK_VIEW_TAB nv, bool forced ,  bool update_button)
{
	GLS_INDEX	gls;
	assert(nv>=0&& nv<SONORK_VIEW_TAB_COUNT);
	if( view.ctxTab != nv || forced )
	{
		// First time _SetCtxView is called: No previous tab
		HideTreeToolTips();
		ClearSelection( false );
		view.ctxTab=nv;
		gls = (nv == SONORK_VIEW_TAB_1)?GLS_LB_USRS:GLS_LB_APPS;
		view.tree[SONORK_VIEW_TAB_1].ShowWindow(view.ctxTab==SONORK_VIEW_TAB_1?SW_SHOW:SW_HIDE);
		view.tree[SONORK_VIEW_TAB_2].ShowWindow(view.ctxTab==SONORK_VIEW_TAB_2?SW_SHOW:SW_HIDE);
		SetCtrlText( IDC_MAIN_TAB_TITLE, gls );
		if( update_button )
			TabCtrl_SetCurSel(view.tabHwnd,nv);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMainWin::DrawSidMode( DRAWITEMSTRUCT*S )
{

	HFONT 			p_font;
	SKIN_ICON     	icon;
	SONORK_C_CSTR		str;
	UINT			icon_draw_flags;
	HDC		    	dc=S->hDC;
	SONORK_SID_MODE		sid_mode;
	p_font 	= (HFONT)::SelectObject( dc , sonork_skin.Font(SKIN_FONT_MAIN_TREE) );
	for(;;)
	{
		if( S->itemState & ODS_COMBOBOXEDIT	)
		{
			if( SonorkApp.CxStatus() == SONORK_APP_CX_STATUS_IDLE )
			{
				sid_mode = SONORK_SID_MODE_DISCONNECTED;
				break;
			}
		}
		sid_mode    = (SONORK_SID_MODE)S->itemData;
		if( SonorkApp.CxConnecting() && sid_mode != SONORK_SID_MODE_DISCONNECTED)
			S->itemState|=ODS_DISABLED;
		break;
	}
	if( sid_mode == PSEUDO_SONORK_SID_MODE_VISIBILITY )
	{
		icon = SKIN_ICON_VIS_PRIVATE;
		str = SonorkApp.LangString(GLS_UM_VISIBILITY);
	}
	else
	icon = SonorkApp.GetUserModeViewInfo(  sid_mode , SonorkApp.ProfileUser().InfoFlags(), &str );
	::SetBkMode(dc,TRANSPARENT);
	if(S->itemState&ODS_DISABLED)
	{
		::FillRect(S->hDC
			,&S->rcItem
			, GetSysColorBrush(COLOR_BTNFACE));
		::SetTextColor(dc
			, GetSysColor(COLOR_GRAYTEXT)	);
		icon_draw_flags=ILD_BLEND25;
	}
	else
	{
		icon_draw_flags=ILD_TRANSPARENT;
		if(S->itemState&ODS_SELECTED)
		{
			::FillRect(dc
				, &S->rcItem
				, GetSysColorBrush(COLOR_INFOBK	));
			::SetTextColor(dc
				, GetSysColor(COLOR_INFOTEXT) );
		}
		else
		{
			::FillRect(S->hDC
				,&S->rcItem
				, GetSysColorBrush(COLOR_BTNFACE));
			::SetTextColor(dc
				, GetSysColor(COLOR_BTNTEXT));
		}
	}
	sonork_skin.DrawIcon(dc
		, icon
		, S->rcItem.left+2
		, S->rcItem.top+1
		, icon_draw_flags);

	while( S->itemState & ODS_COMBOBOXEDIT	)
	{
		if( sid_mode == SONORK_SID_MODE_INVISIBLE)
			break;
		if( SonorkApp.ProfileSidFlags().IsPrivate() )
		{
			icon = SKIN_ICON_VIS_PRIVATE;
		}
		else
		if( SonorkApp.ProfileSidFlags().IsPublic() || !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NOT_PUBLIC_SID))
		{
			icon = SonorkApp.ProfileSidFlags().TrackerEnabled()
			||     !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_INVITATIONS)
				?SKIN_ICON_TRACKER
				:SKIN_ICON_VIS_PUBLIC;
		}
		else
		break;

		sonork_skin.DrawIcon(dc
			, icon
			, S->rcItem.right-(SKIN_ICON_SW+2)
			, S->rcItem.top+1
			, icon_draw_flags);
		break;
	}
	S->rcItem.left+=4+SKIN_ICON_SW;
	::DrawText(dc
		, str
		, strlen(str)
		, &S->rcItem
		, DT_NOPREFIX|DT_LEFT|DT_SINGLELINE|DT_VCENTER);
	::SelectObject( S->hDC, p_font );
}

// ----------------------------------------------------------------------------

#define VIEW_SCROLL_THRESHOLD	10
void
 TSonorkMainWin::SetDropTarget(HTREEITEM hItem, int y)
{
	if( view.dropHitem != hItem )
	{
		view.dropHitem=hItem;
		GetActiveTree().SelectDropTarget(hItem);
	}
	if( y>=0)
	{
		UINT wParam;
		if( y < VIEW_SCROLL_THRESHOLD )
			wParam = MAKEWPARAM( SB_LINEUP		 , 0);
		else
		if( y > view.height - VIEW_SCROLL_THRESHOLD )
			wParam = MAKEWPARAM( SB_LINEDOWN	 , 0);
		else
			return;
		GetActiveTree().SendMessage( WM_VSCROLL, wParam , 0);
	}
}


// ----------------------------------------------------------------------------

void
 TSonorkMainWin::DoDrag(NM_TREEVIEW* nmTV)
{
	DWORD 			aux;
	TSonorkViewItemPtrs 	VP;
	TSonorkDropSourceData 	*SD;
	TSonorkClipData		*sCD;
	TSonorkClipData		*nCD;
	const TSonorkId		*pUserId;
	TSonorkListIterator		I;
	VP.item = (TSonorkMainViewItem*)nmTV->itemNew.lParam;
	if(VP.item==NULL)return;
	if(VP.item->Type() == SONORK_TREE_ITEM_EXT_USER)
	{
		SONORK_DWORD2List	list;
		::ClientToScreen(nmTV->hdr.hwndFrom,&nmTV->ptDrag);
		if(view.sel_count != 0)
			GetSelection(&list);

		SD=new TSonorkDropSourceData;
		sCD=SD->GetClipData();
		if( list.Items() == 0)
		{
			sCD->SetUserData( VP.user->UserData() );
			SetWinUsrFlag(DRAGGING_USER_DATA);
			SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
			ClearWinUsrFlag(DRAGGING_USER_DATA);
		}
		else
		{
			sCD->SetSonorkClipDataQueue();
			list.InitEnum(I);
			while( (pUserId = (TSonorkId*)list.EnumNext(I)) != NULL )
			{
				nCD = new TSonorkClipData();
				nCD->SetUserId( pUserId );
				sCD->AddSonorkClipData( nCD );
				nCD->Release();
			}
			SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
		}
		SD->Release();

	}
}


// ----------------------------------------------------------------------------
// TREE VIEW ITEMS
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TSonorkMainViewItem
LRESULT
 TSonorkMainViewItem::CustomDraw_Prepaint(NMTVCUSTOMDRAW* CD) const
{
	TSonorkExtUserData	*UD;

	::FillRect( CD->nmcd.hdc
	, &CD->nmcd.rc
	, sonork_skin.Brush(SKIN_BRUSH_MAIN_VIEW));
	if( v_flags & SONORK_VIEW_ITEM_F_SELECTED )
	{
		CD->clrText
			= sonork_skin.Color(SKIN_COLOR_MSG_SELECT,SKIN_CI_FG);
		CD->clrTextBk
			= sonork_skin.Color(SKIN_COLOR_MSG_SELECT,SKIN_CI_BG);
	}
	else
	if( CD->nmcd.uItemState&(CDIS_FOCUS|CDIS_SELECTED) )
	{
		CD->clrText
			= sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_FG);
		CD->clrTextBk
			= sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_BG);
	}
	else
	{
		if( GetEventCount() )
		{
			CD->clrText
				= sonork_skin.Color(SKIN_COLOR_MAIN_EXT
					,SKIN_CI_MAIN_EVENT);
		}
		else
		if( Type() == SONORK_TREE_ITEM_EXT_USER )
		{
#define	_this	((TSonorkExtUserViewItem*)this)
			UD = _this->UserData();
			if( UD->IsOnline() )
			{
				CD->clrText
					= sonork_skin.Color(SKIN_COLOR_MAIN_EXT
						,SKIN_CI_MAIN_ONLINE);
			}
			else
			{
				CD->clrText
					= sonork_skin.Color(SKIN_COLOR_MAIN_EXT
						,SKIN_CI_MAIN_OFFLINE);
			}
#undef	_this

		}
		else
		if( Type() == SONORK_TREE_ITEM_GROUP )
		{
#define	_this	((TSonorkMainGroupViewItem*)this)
			if( _this->IsUserGroup() || _this->Group()==SONORK_VIEW_SYS_GROUP_AUTHS)
			{
				if( _this->GetBoldCount() )
				{
					CD->clrText
						= sonork_skin.Color(SKIN_COLOR_MAIN_EXT
							,SKIN_CI_MAIN_ONLINE);
				}
				else
				{
					CD->clrText
						= sonork_skin.Color(SKIN_COLOR_MAIN_EXT
							,SKIN_CI_MAIN_OFFLINE);
				}
			}
			else
			{
				return CDRF_DODEFAULT;
			}
#undef 	_this
		}
		else
		if( Type() == SONORK_TREE_ITEM_APP )
		{
#define	_this	((TSonorkAppViewItem*)this)

			if( !_this->MayExecute() )
			{
				CD->clrText
					= sonork_skin.Color(SKIN_COLOR_MAIN_EXT,SKIN_CI_MAIN_OFFLINE);
			}
#undef	_this
		}
	}
	return CDRF_DODEFAULT;
}


// ----------------------------------------------------------------------------
// TSonorkAppViewItem
// Base class for all application view items

TSonorkAppViewItem::TSonorkAppViewItem(SONORK_APP_TYPE ptype,DWORD pcmd)
	:TSonorkMainViewItem(SONORK_TREE_ITEM_APP)
{
	app_type	=ptype;
	app_menu_cmd 	=pcmd;
}

// ----------------------------------------------------------------------------
// TSonorkIntAppViewItem
// Holds information about Internal (or built-in) applications

TSonorkIntAppViewItem::TSonorkIntAppViewItem(DWORD pcmd,GLS_INDEX papp_name)
 :TSonorkAppViewItem(SONORK_APP_TYPE_INTERNAL,pcmd)
{
	app_name = papp_name;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkIntAppViewItem::AppFlags() const
{
	return 0;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkIntAppViewItem::MayExecute() const
{

	if( AppMenuCmd() == CM_APP_CHAT)
		return SonorkApp.CxReady();
	return true;
}

// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkIntAppViewItem::GetLabel(BOOL,SKIN_ICON& icon)	const
{
	switch( AppMenuCmd() )
	{
		case CM_APP_CHAT:
			icon = SKIN_ICON_CHAT;
			break;

		case CM_APP_EMAIL:
			icon = SKIN_ICON_EMAIL;
			break;

		case CM_APP_CLIPBOARD:
			icon = SKIN_ICON_NOTES;
			break;

		case CM_APP_REMINDER:
			icon = SKIN_ICON_TIME;
			break;

		case CM_APP_SYS_CONSOLE:
			icon = SKIN_ICON_PANEL;
			break;

		case CM_APP_SNAP_SHOT:
			icon = SKIN_ICON_COLORS;
			break;

		default:
			icon = MayExecute()
				?SKIN_ICON_PLUGIN_OK
				:SKIN_ICON_PLUGIN_ERROR;
			break;
	}
	return SonorkApp.LangString(app_name);
}

// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkIntAppViewItem::AppName() const
{
	return SonorkApp.LangString(app_name);
}

// ----------------------------------------------------------------------------
// TSonorkExtAppViewItem
// Holds information about External (or detected) applications

TSonorkExtAppViewItem::TSonorkExtAppViewItem(DWORD pcmd
	, SONORK_C_CSTR 	pname
	, DWORD 		pflags
	, SKIN_ICON 		picon)
	:TSonorkAppViewItem(SONORK_APP_TYPE_EXTERNAL,pcmd)
{
	app_name.Set(pname);
	app_flags	=pflags;
	app_icon	=picon;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkExtAppViewItem::MayExecute() const
{
	if(app_flags&(SONORK_APP_EAF_CONNECTED|SONORK_APP_EAF_NON_ZERO_IP))
		return SonorkApp.CxReady();
	return true;
}

// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkExtAppViewItem::GetLabel(BOOL,SKIN_ICON& icon) const
{
	if( app_icon == SKIN_ICON_NONE )
		icon = MayExecute()?SKIN_ICON_PLUGIN_OK:SKIN_ICON_PLUGIN_ERROR;
	else
		icon = app_icon;
	return app_name.CStr();
}


// ----------------------------------------------------------------------------
// TSonorkWAppViewItem
// Holds information about Web applications

TSonorkWebAppViewItem::TSonorkWebAppViewItem(DWORD pcmd,const TSonorkWappData*WD)
	:TSonorkAppViewItem(SONORK_APP_TYPE_WEB,pcmd)
{
	wapp_data.Set(*WD);
}

SONORK_C_CSTR
 TSonorkWebAppViewItem::GetLabel(BOOL,SKIN_ICON& icon)	const
{
	if(wapp_data.AppType() == SONORK_WAPP_TYPE_WAPP)
		icon = SKIN_ICON_WEB_APP;
	else
	{
		if(WappFlags()&SONORK_WAPP_F_USER_CONTEXT)
			icon = SKIN_ICON_USER_TEMPLATE;
		else
			icon = SKIN_ICON_TEMPLATE;
	}
	return wapp_data.AppName();
}

SONORK_VIEW_GROUP
 TSonorkWebAppViewItem::GetDispGroup() 			const
{
	return (SONORK_VIEW_GROUP)(SONORK_VIEW_WAPP_GROUP_FIRST + wapp_data.header.group_no);
}

// ----------------------------------------------------------------------------
// TSonorkMainGroupViewItem
// Base class for all group-type view items

TSonorkMainGroupViewItem::TSonorkMainGroupViewItem(SONORK_VIEW_GROUP p_group)
	:TSonorkMainViewItem(SONORK_TREE_ITEM_GROUP)
{
	group = p_group;
	b_count=
	e_count=
	f_count=
	depth=0;
}


// ----------------------------------------------------------------------------
// TSonorkSysGroupViewItem
// System groups or built-in groups

SONORK_VIEW_TAB	TSonorkMainGroupViewItem::GetTree() const
{
	if( group <= SONORK_VIEW_USER_GROUP_LAST || group==SONORK_VIEW_SYS_GROUP_LOCAL_USERS)
		return SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS;
	else
	if( group >= SONORK_VIEW_SYS_GROUP_EXT_APPS)
		return SONORK_VIEW_TAB_TREE_APPS;
	else
	if( group == SONORK_VIEW_SYS_GROUP_AUTHS)
		return SONORK_VIEW_TAB_TREE_AUTHS;
	else
		return SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS;
}

SONORK_C_CSTR
 TSonorkSysGroupViewItem::GetLabel(BOOL expanded,SKIN_ICON& icon) const
{
	GLS_INDEX gls;
	switch( group )
	{
		case SONORK_VIEW_SYS_GROUP_LOCAL_USERS:
			gls=GLS_LB_AUTHUSRS;
			break;

		case SONORK_VIEW_SYS_GROUP_REMOTE_USERS:
			gls=GLS_LB_RUSRS;
			break;

		case SONORK_VIEW_SYS_GROUP_AUTHS:
			gls=GLS_LB_REQUSRS;
			break;

		case SONORK_VIEW_SYS_GROUP_WEB_APPS:
			gls=GLS_LB_WAPPS;
			icon=expanded
				?(f_count?SKIN_ICON_OPEN_FOLDER:SKIN_ICON_OPEN_FOLDER_BW)
				:(f_count?SKIN_ICON_FOLDER:SKIN_ICON_FOLDER_BW);

			return SonorkApp.LangString(gls);

		case SONORK_VIEW_SYS_GROUP_EXT_APPS:
			gls=GLS_LB_APPS;
			break;

		default:
			return "??";
	}
	icon=expanded
		?(b_count?SKIN_ICON_OPEN_FOLDER:SKIN_ICON_OPEN_FOLDER_BW)
		:(b_count?SKIN_ICON_FOLDER:SKIN_ICON_FOLDER_BW);
	return SonorkApp.LangString(gls);
}

// ----------------------------------------------------------------------------
// TSonorkUserGroupViewItem
// Groups created dynamically by the user

TSonorkCustomGroupViewItem::TSonorkCustomGroupViewItem(	DWORD pbase, const TSonorkGroup*UG)
	:TSonorkMainGroupViewItem( (SONORK_VIEW_GROUP)(pbase + UG->GroupNo()) )
{
	base_no		= pbase;
	parent_no	= UG->ParentNo();
	name.Set(UG->name.CStr());
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkCustomGroupViewItem::GetStateFlags() const
{
	 return b_count?TVIS_BOLD:0;
}

// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkCustomGroupViewItem::GetLabel(BOOL expanded,SKIN_ICON& icon) const
{
	icon=b_count
		?(expanded?SKIN_ICON_OPEN_FOLDER:SKIN_ICON_FOLDER)
		:(expanded?SKIN_ICON_OPEN_FOLDER_BW:SKIN_ICON_FOLDER_BW);
	return name.CStr();
}

// ----------------------------------------------------------------------------

UINT
 TSonorkCustomGroupViewItem::GetSortIndex()	const
{
	return  ((b_count > 0)	?2:0);
}


// ----------------------------------------------------------------------------
// TSonorkExtUserViewItem
// Item referencing an user

DWORD
 TSonorkExtUserViewItem::GetStateFlags() const
{
	return UD->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) || UD->IsOnline()
			?TVIS_BOLD:0;
}
// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkExtUserViewItem::GetLabel( BOOL,SKIN_ICON& icon ) const
{
	icon = SonorkApp.GetUserModeIcon(UD);
	return UD->display_alias.CStr();
}

// ----------------------------------------------------------------------------

SONORK_VIEW_TAB
 TSonorkExtUserViewItem::GetTree() const
{
	if( UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
		return SONORK_VIEW_TAB_TREE_AUTHORIZED_USERS;
	else
		return SONORK_VIEW_TAB_TREE_NOT_AUTHORIZED_USERS;
}

// ----------------------------------------------------------------------------

SONORK_VIEW_GROUP
 TSonorkExtUserViewItem::GetDispGroup()	const
{
	if( UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
		return (SONORK_VIEW_GROUP)( UD->GetVisualGroupNo() );
	else
		return SONORK_VIEW_SYS_GROUP_REMOTE_USERS;
}

// ----------------------------------------------------------------------------

UINT
 TSonorkExtUserViewItem::GetSortIndex()	const
{
	return  (UD->IsOnline()?12:10);
}



// ----------------------------------------------------------------------------
// TSonorkAuthReqViewItem
// Holds a pending authorization request

SONORK_C_CSTR
 TSonorkAuthReqViewItem::GetLabel(BOOL,SKIN_ICON& icon) const
{
	icon=sent_by_us?SKIN_ICON_OUTGOING:SKIN_ICON_INCOMMING;
	return alias.CStr();
}

// ----------------------------------------------------------------------------

