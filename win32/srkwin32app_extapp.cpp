#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkwin32app_extapp.h"
#include "srkmainwin.h"
#include "srkwaitwin.h"
#include "srk_winregkey.h"


// ----------------------------------------------------------------------------
// Declarations

enum CIRCUIT_TYPE
{
  CIRCUIT_NONE
, CIRCUIT_LOCAL
, CIRCUIT_REMOTE
};

// ----------------------------------------------------------------------------
// Static functions declarations

static UINT
 EA_UnquotedStrCopy(char *t_ptr, const char *s_ptr, UINT *eoc, bool quoted);

static UINT
 EA_LoadToken(SONORK_C_CSTR* s_d_ptr, SONORK_C_STR t_ptr, UINT tgt_size);

static bool
 EA_IsDelimiter(char c);


// ----------------------------------------------------------------------------
// Static variables declarations

#define VALUE_TYPE_PREFIX_LENGTH	3
static SONORK_C_CSTR szAbsValue="AV:";
static SONORK_C_CSTR szRelValue="RV:";
static SONORK_C_CSTR szTmpValue="TV:";
static SONORK_C_CSTR szNotFoundMark=">><<";

		
// ----------------------------------------------------------------------------
// SONORK_EXT_APP service locator
// Service callback for the SONORK_SERVICE_ID_EXT_APP locator:
//  Needed to process invitations/ remote requests
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkWin32App::ServiceCallback_ExtApp(SONORK_DWORD2&	handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			event_tag
			, TSonorkAppServiceEvent*	E)
{
	union {
		DWORD				value;
		TSonorkExtAppPendingReq*	pending_req;
	}D;
	SONORK_RESULT		result;
	SONORK_CTRLMSG_CMD	reply_cmd;

	TSonorkWin32App*	_this=(TSonorkWin32App*)handler_tag.v[0];

	switch(event_id)
	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:
		E->get_name.str->Set("External application locator");
		return 1;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ:
		event_tag->v[0]=0; // We don't have any data about it
		event_tag->v[1]=CIRCUIT_REMOTE;
		return SONORK_APP_SERVICE_CIRCUIT_ACCEPTED;


	case SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN:
		if( event_tag->v[1] == CIRCUIT_LOCAL )
		{
			// Load the D.pending_req pointer
			D.value=event_tag->v[0];
			if( D.value == 0)
				break;// Don't have info about circuit (how come?)

			// Load the remote's circuit
			D.pending_req->remote.circuit.Set( *E->circuit_open.handle );

			if( D.pending_req->owner == NULL )
			{
				// The owner window has been already destroyed
				// we don't process any more this circuit
				break;
			}

			// The other side accepted the circuit
			//  now we must send the data and wait
			//  for the other side to reply.
			// We increase the timeout.

			D.pending_req->owner->SetStatus(
				  GLS_MS_CXWFREPLY
				, SONORK_SECS_FOR_PROTOCOL_INITIATION
				, 0 ,0);

			// Send our request using the circuit
			_this->Service_SendCircuitData(
				  _this->ExtAppServiceInstance()
				, E->circuit_open.handle
				, SONORK_CTRLMSG_CMD_START
				, 0
				, 0
				, &D.pending_req->request);
		}
	break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_DATA:
		if( event_tag->v[1] == CIRCUIT_LOCAL )
		{
			// Process data sent when WE requested somthing
			// to the other side.
			// Probably the status of our request.

			// Load the D.pending_req pointer
			D.value=event_tag->v[0];
			if( D.value == 0)
				break;// Don't have info about circuit (how come?)

			if( D.pending_req->owner == NULL )
			{
				// The owner window has been already destroyed
				// we don't process any more this circuit
				break;
			}

			if( E->circuit_data.Cmd() != SONORK_CTRLMSG_CMD_START)
			{
				// Don't know the message, maybe newer versions
				// send more feedback or test us: Ignore
				break;
			}

			if( E->circuit_data.CmdFlags()&SONORK_CTRLMSG_CF_ACCEPT)
			{
				// Got the ACCEPT message indicating
				// that the application is supported
				// by the other side and that it is now
				// waiting for the user's response
				D.pending_req->owner->SetTimeoutSeconds(
					SONORK_SECS_FOR_USER_RESPONSE
					);
			}
			else
			if( E->circuit_data.CmdFlags()&SONORK_CTRLMSG_CF_RESULT)
			{
				// Got the RESULT message indicating
				// the final outcome of our negociation.

				if( E->circuit_data.UserParam() == SONORK_RESULT_OK )
				{
					// Set <accepted> to true
					//  and put the dialog in "AutoTimeout"
					//  mode: This mode disables all buttons
					//  and timeouts after N seconds.
					// When the TIMEOUT event occurs, the
					//  dialog will still launch the application
					//  because <accepted> is true.
					D.pending_req->processed=true;
					D.pending_req->owner->SetStatusAutoTimeout(
						SONORK_SECS_FOR_APP_TO_START);
				}
				else
				{

					// The user cancelled or the application
					// is not supported.
					D.pending_req->owner->CancelWait(
						_this->SysString(
							E->circuit_data.UserParam() == SONORK_RESULT_NOT_SUPPORTED
							?GSS_OPNOTSUPPORT
							:GSS_REQDENIED)
						, GLS_OP_STARTAPP
						, 0);
				}
			}
		}
		else
		if( event_tag->v[1] == CIRCUIT_REMOTE )
		{
			// Process data sent when the OTHER is requesting
			// something, probably to start an application.

			D.pending_req = NULL;

			reply_cmd=MakeSonorkCtrlMsgCmd(E->circuit_data.Cmd()
				,SONORK_CTRLMSG_CF_RESULT);
			if( E->circuit_data.Cmd() != SONORK_CTRLMSG_CMD_START)
			{
				result=SONORK_RESULT_NOT_SUPPORTED;
			}
			else
			{
				SONORK_MEM_NEW(D.pending_req=new TSonorkExtAppPendingReq);
				result = D.pending_req->request.CODEC_ReadMemNoSize(
					 E->circuit_data.Data()
					,E->circuit_data.DataSize());
			}

			if( result == SONORK_RESULT_OK )
			{
				D.pending_req->EA = _this->GetExtApp(
					D.pending_req->request.app_name.CStr()
					,SONORK_EXT_APP_CTRL_CONTEXT
					,D.pending_req->view_name);
				if( D.pending_req->EA == NULL )
					result= SONORK_RESULT_NOT_SUPPORTED;
			}

			if( result == SONORK_RESULT_OK )
			{
				E->circuit_data.GetCircuitHandle(D.pending_req->remote.circuit);
				D.pending_req->using_circuit=true;
				if(Sonork_StartWaitWin(
					  LaunchExtApp_UserResponse_Waiter
					, (LPARAM)_this
					, D.pending_req
					, 0
					, SKIN_SIGN_PLUGINS))
				{
					// WaitForUserResponse_Handler
					// now owns D.pending_req, so don't
					// delete it
					event_tag->v[0]=(DWORD)D.pending_req;
					D.pending_req=NULL;

					reply_cmd=MakeSonorkCtrlMsgCmd(E->circuit_data.Cmd()
						,SONORK_CTRLMSG_CF_ACCEPT);
				}
				else
				{
					result = SONORK_RESULT_INTERNAL_ERROR;
				}
			}
			if( D.pending_req != NULL )
				SONORK_MEM_DELETE( D.pending_req );
			if( reply_cmd != SONORK_CTRLMSG_CMD_NONE )
			{
				// Don't know how to handle this request.
				_this->Service_Reply(
				  reply_cmd
				, result
				, 0, NULL, 0);
			}
		}
	break;

	case SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED:

		// Load the D.pending_req pointer
		D.value=event_tag->v[0];
		if( D.value == 0)
			break;// Don't have info about circuit (how come?)

		if( D.pending_req->owner == NULL )
		{
			// The owner window has been already destroyed
			// we don't process any more this circuit
			break;
		}
		// The circuit has been closed.
		// We should have received the RESULT packet in
		// CIRCUIT_DATA handler before this occured.
		// If not, it means that something happened before
		// (like the remote side disconnected)
		// We check the <accepted> flag to see if
		// we got the result before that happened; if
		// not we Cancel the wait dialog.

		D.pending_req->remote.circuit.systemId=0;
		if(D.pending_req->processed == false )
		{
			D.pending_req->owner->CancelWait(
				  _this->SysString(
					E->circuit_close.result==SONORK_RESULT_TIMEOUT
					  ?GSS_REQTIMEOUT
					  :GSS_USRCANCEL)
				, GLS_OP_STARTAPP
				, event_tag->v[1] == CIRCUIT_REMOTE
					? 5	// Show 5 seconds if was remote request
					: 0	// Show dialog if WE requested
				);
		}


	break;
	case SONORK_APP_SERVICE_EVENT_POKE_DATA:
	break;

	}
	return 0;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkWin32App::OnOldCtrlMsg_ExtApp(
	 const TSonorkUserHandle&	handle
	,const TSonorkCtrlMsg*		reqMsg
	,const void*			data
	,DWORD 				data_size)
{
	TSonorkCtrlMsg	aknMsg;
	DWORD		param1;
	if( reqMsg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_EXEC_START )
	{
		TSonorkExtAppPendingReq	*RI;
		SONORK_MEM_NEW(RI = new TSonorkExtAppPendingReq);
		param1 = RI->request.CODEC_ReadMem((BYTE*)data,data_size);
		if(  param1 == SONORK_RESULT_OK)
		{
			RI->EA = GetExtApp(RI->request.app_name.CStr()
					,SONORK_EXT_APP_CTRL_CONTEXT
					,RI->view_name);
			if( RI->EA == NULL )
				param1 =  SONORK_RESULT_NOT_SUPPORTED;
			else
			{
				RI->remote.query.TSonorkUserHandle::Set( handle );
				RI->remote.query.systemId = reqMsg->OldQueryId();
				if(Sonork_StartWaitWin(
					  LaunchExtApp_UserResponse_Waiter
					, (LPARAM)this
					, RI
					, 0
					, SKIN_SIGN_PLUGINS))
				{
					RI=NULL;
				}
				else
					param1 = SONORK_RESULT_INTERNAL_ERROR;
			}
		}
		if( RI!=NULL )
			SONORK_MEM_DELETE(RI);

		aknMsg.cmdFlags= SONORK_OLD_CTRLMSG_CMD_EXEC_STATUS;
	}
	else
	{
		return SONORK_APP_OLD_CTRLMSG_RF_DEFAULT;
	}

	aknMsg.ClearOldParams();

	aknMsg.sysFlags	= 0;
	aknMsg.SetOldServiceId(SONORK_SERVICE_ID_EXTERNAL_APP);
	aknMsg.SetOldQueryId(reqMsg->OldQueryId());
	aknMsg.body.o.param[0]	= param1;

	SendCtrlMsg(&handle
		,&aknMsg
		,NULL
		,0) ;

	return SONORK_APP_OLD_CTRLMSG_RF_MAY_END_QUERY;
}



// ----------------------------------------------------------------------------
SONORK_RESULT
 TSonorkWin32App::LaunchExtApp(const TSonorkExtApp*EA
		,const TSonorkExtUserData*UD
		,bool init_mode)
{
	SONORK_RESULT		result;
	char			start_dir_suffix[48];
	if( EA == NULL )
		result = SONORK_RESULT_INVALID_PARAMETER;
	else
	{
		result = SONORK_RESULT_OK;
		if(EA->Context() == SONORK_EXT_APP_USER_CONTEXT )
		{
			if( UD == NULL )
				result = SONORK_RESULT_INVALID_OPERATION;
			else
			if( (EA->flags & SONORK_APP_EAF_SEND_REQ) && init_mode )
			{
				TSonorkExtAppPendingReq*pending_req;
				SONORK_MEM_NEW( pending_req = new TSonorkExtAppPendingReq );

				result = LaunchExtApp_Remote(
						 EA
						,UD
						,pending_req);
				if( result == SONORK_RESULT_OK)
				if( EA->flags & SONORK_APP_EAF_WAIT_AKN )
				{
					if(Sonork_StartWaitWin(
						  LaunchExtApp_RemoteAkn_Waiter
						, (LPARAM)this
						, pending_req
						, SONORK_APP_EM_CTRL_MSG_CONSUMER
						, SKIN_SIGN_PLUGINS))
					{
						result = SONORK_RESULT_OK_PENDING;
						// StartExtApp_WaitForRemoteAkn_Handler
						// now owns "pending_req": Don't delete it
						pending_req=NULL;
					}
					else
					{
						result = SONORK_RESULT_INTERNAL_ERROR;
					}
				}
				if( pending_req != NULL )
					SONORK_MEM_DELETE( pending_req );
			}
		}
	}
	if( result == SONORK_RESULT_OK )
	{
		TSonorkTempBuffer	params(LEA_MAX_PARAMS_SIZE+2);
		TSonorkTempBuffer	start_dir(SONORK_MAX_PATH);

		if(ParseExtAppParams(EA,params.CStr(),UD))
		{
			STARTUPINFO			start_info;
			PROCESS_INFORMATION	proc_info;
			start_info.cb = sizeof(start_info);
			GetStartupInfo(&start_info);
			start_info.dwFlags=0;
			wsprintf(start_dir_suffix,"%u.%u\\%s"
						,ProfileUserId().v[0]
						,ProfileUserId().v[1]
						,szPrivateDirUser);
			GetDirPath(start_dir.CStr(),SONORK_APP_DIR_DATA ,start_dir_suffix);
			if(CreateProcess(
					NULL
				,	params.CStr()
				, 	NULL
				,	NULL
				,	false
				,	0
				,	NULL
				, 	start_dir.CStr()
				,	&start_info
				,	&proc_info))
			{
				CloseHandle( proc_info.hThread );
				CloseHandle( proc_info.hProcess );
				SetBichoSequence(SONORK_SEQUENCE_OPEN_WINDOW);
				result=SONORK_RESULT_OK;
			}
			else
			{
				result=SONORK_RESULT_OS_ERROR;
			}
		}
		else
			result = SONORK_RESULT_CONFIGURATION_ERROR;
		if( result != SONORK_RESULT_OK )
		{
			LangSprintf(params.CStr()
				, GLS_ERR_SPAWN
				, EA->Name()
				, GetLastError());

			Set_UI_Event(SONORK_UI_EVENT_ERROR
			 , params.CStr()
			 ,  SONORK_UI_EVENT_F_BICHO
			   |SONORK_UI_EVENT_F_SOUND
			   |SONORK_UI_EVENT_F_LOG
			   |SONORK_UI_EVENT_F_LOG_AS_UNREAD
			 );
		}
	}
	return result;
}

SONORK_RESULT
 TSonorkWin32App::LaunchExtApp_Remote(
	 const TSonorkExtApp*		EA
	,const TSonorkExtUserData*	UD
	,TSonorkExtAppPendingReq*	pending_req)
{
	TSonorkCtrlMsg		oldMsg;
	SONORK_RESULT		result;
	SONORK_DWORD2		circuit_tag;

	pending_req->local_instance=ExtAppServiceInstance();
	pending_req->request.app_name.Set(EA->Name());
	pending_req->request.phys_addr.Set(ProfileUser().addr.physAddr);
	pending_req->request.alias.Set(ProfileUser().alias.CStr());

	UD->GetUserHandle(&pending_req->remote.user);

	if(UD->SidVersion().UsesOldCtrlMsg() )
	{
		oldMsg.cmdFlags	= SONORK_OLD_CTRLMSG_CMD_EXEC_START;
		oldMsg.sysFlags	= 0;
		oldMsg.SetOldServiceId(SONORK_SERVICE_ID_EXTERNAL_APP);
		oldMsg.SetOldQueryId(GenTrackingNo(UD->userId));
		oldMsg.ClearOldParams();

		result = SendCtrlMsg(
				 &pending_req->remote.user
				,&oldMsg
				,&pending_req->request
				, NULL);	// Not using tag (old method)
		// we're using the old method
		pending_req->remote.circuit.LoadTarget(SONORK_SERVICE_ID_NONE,0);
		// Save the query id
		pending_req->remote.query.systemId=oldMsg.OldQueryId();
	}
	else
	{
		circuit_tag.v[0]=(DWORD)pending_req;
		circuit_tag.v[1]=CIRCUIT_LOCAL;
		// Connect to the other side's locator
		pending_req->remote.circuit.LoadTarget(
			  SONORK_SERVICE_ID_EXTERNAL_APP
			, SONORK_SERVICE_LOCATOR_INSTANCE(0));

		if( EA->Flags() & SONORK_APP_EAF_WAIT_AKN )
		{
			result=Service_OpenCircuit(
				  ExtAppServiceInstance()
				, &pending_req->remote.circuit
				, 0
				, 0
				, SONORK_MSECS_FOR_CIRCUIT_CONNECTION
				, NULL
				, 0
				, &circuit_tag);
			pending_req->using_circuit=true;
		}
		else
		{
			result=Service_SendPokeData(
				  ExtAppServiceInstance()
				, &pending_req->remote.service
				, SONORK_CTRLMSG_CMD_START
				, 0
				, 0
				, &pending_req->request);
		}
	}
	return result;
}

// ----------------------------------------------------------------------------

void SONORK_CALLBACK
 TSonorkWin32App::LaunchExtApp_UserResponse_Waiter(TSonorkWaitWin*WW
		, LPARAM cb_param
		, LPVOID cb_data
		, SONORK_WAIT_WIN_EVENT event
		, LPARAM event_param)
{
#define pending_req 		((TSonorkExtAppPendingReq*)cb_data)
#define USING_CIRCUIT		(pending_req->using_circuit)
#define CIRCUIT_ACTIVE		(pending_req->using_circuit && pending_req->remote.circuit.CircuitId()!=0)
	char *tmp;
	const char *p_alias;
	TSonorkExtUserData* 		UD;
	SONORK_RESULT	result;
	DWORD		app_started;
	TSonorkWin32App*_this=(TSonorkWin32App*)cb_param;

	switch( event )
	{
		case SONORK_WAIT_WIN_EVENT_CREATE:

			pending_req->owner=WW;

			SONORK_MEM_NEW( tmp = new char[SONORK_MAX_PATH * 3] );
			UD = _this->GetUser( pending_req->remote.user.userId );
			if( UD != NULL )
				p_alias = UD->display_alias.CStr();
			else
				p_alias= pending_req->request.alias.CStr();
			SonorkApp.LangSprintf(
				  tmp
				, GLS_EA_ADVMSG
				, p_alias
				, pending_req->remote.user.userId.v[0]
				, pending_req->remote.user.userId.v[1]
				, pending_req->view_name);
			WW->SetCtrlText(IDC_WAIT_LABEL,tmp);

			sprintf(tmp,"%s:\r\n%s"
				, _this->LangString(GLS_LB_FILE)
				, pending_req->EA->Cmd());
			WW->SetCtrlText(IDC_WAIT_INFO , tmp);
			WW->SetWindowText(GLS_OP_STARTAPP);
			*((DWORD*)event_param)=
				   SONORK_SECS_FOR_USER_RESPONSE
				  |SONORK_WAIT_WIN_F_CHECKBOX_CHECKED
				  |SONORK_WAIT_WIN_F_IS_NOTICE;

			SONORK_MEM_DELETE_ARRAY( tmp );
			break;

		case SONORK_WAIT_WIN_EVENT_DESTROY:

			pending_req->owner=NULL;
			if(CIRCUIT_ACTIVE)
			{
				_this->Service_CloseCircuit(
					  _this->ExtAppServiceInstance()
					, pending_req->remote.circuit.CircuitId()
					, SONORK_RESULT_USER_TERMINATION);
			}
			SONORK_MEM_DELETE( pending_req );
			break;


		case SONORK_WAIT_WIN_EVENT_RESULT:

			pending_req->processed=true;
			if( event_param == SONORK_WAIT_WIN_F_RESULT_ACCEPT )
			{
				result=SONORK_RESULT_OK;
				if( WW->GetCtrlChecked(IDC_WAIT_CHECKBOX) )
				{
					_this->LaunchExtApp(
					    pending_req->EA
					,   _this->GetUser(pending_req->remote.user.userId)
					,   true);
					app_started=0x1;
				}
				else
					app_started=0x0;
			}
			else
				result=SONORK_RESULT_NOT_ACCEPTED;

			if( USING_CIRCUIT )
			{
				_this->Service_SendCircuitData(
					  _this->ExtAppServiceInstance()
					, &pending_req->remote.circuit
					, MakeSonorkCtrlMsgCmd(SONORK_CTRLMSG_CMD_START
						,SONORK_CTRLMSG_CF_RESULT)
					, result
					, app_started
					, NULL
					, 0);
			}
			else
			{
				TSonorkCtrlMsg	aknMsg;

				aknMsg.ClearOldParams();
				aknMsg.body.o.param[0]  = result;
				aknMsg.cmdFlags	= SONORK_OLD_CTRLMSG_CMD_EXEC_RESULT;
				aknMsg.sysFlags	= 0;
				aknMsg.SetOldServiceId(SONORK_SERVICE_ID_EXTERNAL_APP);
				aknMsg.SetOldQueryId( pending_req->remote.query.QueryId() );

				_this->SendCtrlMsg(
					&pending_req->remote.user
					,&aknMsg
					,NULL
					,0) ;
			}

		break;

	}

#undef 	pending_req
#undef 	USING_CIRCUIT
#undef 	CIRCUIT_ACTIVE

}

// ----------------------------------------------------------------------------

void SONORK_CALLBACK
 TSonorkWin32App::LaunchExtApp_RemoteAkn_Waiter(TSonorkWaitWin*WW
		, LPARAM cb_param
		, LPVOID cb_data
		, SONORK_WAIT_WIN_EVENT event
		, LPARAM event_param)
{
#define pending_req 		((TSonorkExtAppPendingReq*)cb_data)
#define USING_CIRCUIT		(pending_req->using_circuit)
#define CIRCUIT_ACTIVE		(pending_req->using_circuit && pending_req->remote.circuit.CircuitId()!=0)
#define OldCtrlQueryId		(pending_req->remote.query.QueryId())
#define WaitAppEvent		((TSonorkWaitWinEvent_AppEvent*)event_param)
	SONORK_SYS_STRING	gss=GSS_NULL;
	TSonorkWin32App*	_this=(TSonorkWin32App*)cb_param;
	const TSonorkCtrlMsg*	aknMsg;
	switch( event )
	{

	case SONORK_WAIT_WIN_EVENT_CREATE:
			WW->SetCtrlText(IDC_WAIT_LABEL, pending_req->request.app_name.CStr());
			WW->SetCtrlText(IDC_WAIT_INFO, GLS_MS_CXTING );
			WW->SetWindowText(GLS_OP_STARTAPP);
			*((DWORD*)event_param)=
				 SONORK_SECS_FOR_CIRCUIT_CONNECTION
				|SONORK_WAIT_WIN_F_NO_CHECKBOX
				|SONORK_WAIT_WIN_F_NO_ACCEPT_BUTTON;
			pending_req->owner=WW;
		return;


	case SONORK_WAIT_WIN_EVENT_DESTROY:
			pending_req->owner=NULL;
			if(CIRCUIT_ACTIVE)
			{
				_this->Service_CloseCircuit(
					  _this->ExtAppServiceInstance()
					, pending_req->remote.circuit.CircuitId()
					, SONORK_RESULT_USER_TERMINATION);
			}
			SONORK_MEM_DELETE( pending_req );

		return;


	case SONORK_WAIT_WIN_EVENT_APP_EVENT:

			if( WaitAppEvent->event !=  SONORK_APP_EVENT_OLD_CTRL_MSG
			||  WaitAppEvent->param != SONORK_SERVICE_ID_EXTERNAL_APP
			||  USING_CIRCUIT)
				break;

			aknMsg = ((TSonorkAppEventOldCtrlMsg*)WaitAppEvent->data)->msg;
			if( aknMsg->OldQueryId() != OldCtrlQueryId )
				break;

			if( aknMsg->OldCmd() != SONORK_OLD_CTRLMSG_CMD_EXEC_STATUS
			&&  aknMsg->OldCmd() != SONORK_OLD_CTRLMSG_CMD_EXEC_RESULT)
				break;

			if( aknMsg->body.o.param[0] == SONORK_RESULT_OK)
			{
				if(aknMsg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_EXEC_STATUS)
				{

				// The other side has started its Wait dialog
				// and will sit there until the user clicks
				// any of the buttons or until it times-out.
				// We increase our timeout.

					WW->SetStatus(GLS_MS_CXWFREPLY
						,SONORK_SECS_FOR_USER_RESPONSE
						,0,0);
				}
				else
				{

				// The other user has accepted and started
				// the application: We ask for a small timeout,
				// and set the <accepted> to true and wait until
				// the wait dialog times-out: When we process
				// the TIMEOUT result, we know if we should
				// start or not the application by checking
				// if the <accepted> flag is set.
				// This is to let the other side start before
				// us as we're probably connecting to a server.
				
					pending_req->processed=true;
					WW->SetStatusAutoTimeout(
						SONORK_SECS_FOR_APP_TO_START);
				}
				break;
			}
		// Other side could not start the application
		// or the user denied the operation.
		gss = GSS_REQDENIED;
		break;

	case SONORK_WAIT_WIN_EVENT_RESULT:
			// We MUST end by timeout
			// Otherwise the user pressed accept or cancel
			// which is basically the same thing in this
			// situation.
			// If Cancel() was called, the message has
			// already been set.
			if( event_param != SONORK_WAIT_WIN_F_RESULT_TIMEOUT)
				break;

			if(  pending_req->processed )
			{
				_this->LaunchExtApp(
					_this->GetExtApp( pending_req->request.app_name.CStr()
						, SONORK_EXT_APP_USER_CONTEXT
						, NULL )
					, _this->GetUser( pending_req->remote.user.userId )
					, false);
				return;
			}
			gss = GSS_REQTIMEOUT;
		break;

	default:
		return;

	}
	if( gss != GSS_NULL )
	{
		// When Cancel() is called from within this callback,
		// the callback is not re-invoked. So after Cancel()
		// we'll only receive one more call: EVENT_DESTROY
		// Also, Cancel() will show the message box only
		// if Cancel() has not been previously called.
		WW->CancelWait(_this->SysString(gss)
			,  GLS_OP_STARTAPP
			,  0	// Show dialog box (Secs=0)
			);
	}

#undef 	pending_req
#undef 	USING_CIRCUIT
#undef 	CIRCUIT_ACTIVE
#undef  OldCtrlQueryId
#undef	WaitAppEvent
}

// ----------------------------------------------------------------------------

const TSonorkExtApp*
	TSonorkWin32App::GetExtApp(SONORK_C_CSTR 	app_name
		,SONORK_EXT_APP_CONTEXT 		context
		,SONORK_C_STR 				view_name) const
{
	TSonorkExtApp *	EA;
	TSonorkListIterator	I;
	ext_apps.BeginEnum(I);
	while( (EA=ext_apps.EnumNext(I)) != NULL )
	{
		if( EA->Context() != context )
			continue;
		if( !stricmp( app_name ,EA->name.CStr() ) )
			break;
	}
	ext_apps.EndEnum(I);
	while(EA && view_name != NULL)
	{
		if(context == SONORK_EXT_APP_CTRL_CONTEXT)
		{
			if( EA->ViewName() != NULL )
			{
				lstrcpyn(view_name
					,EA->ViewName()
					,LEA_MAX_VIEW_NAME_SIZE);
				break;
			}
		}
		lstrcpyn(view_name,EA->Name(),LEA_MAX_VIEW_NAME_SIZE);
		break;
	}
	return EA;
}

// ----------------------------------------------------------------------------
// Loading & Parsing of external applications

void
 TSonorkWin32App::LoadExtApps()
{
	DWORD			aux;
	TSonorkTempBuffer	ini_file(SONORK_MAX_PATH);
	TSonorkTempBuffer	t_section(LEA_SECTION_MAX_SIZE);

	ClearExtApps();

	GetExtAppsIniFile( ini_file.CStr() );
	aux = GetPrivateProfileSection("Apps",t_section.CStr(),LEA_SECTION_MAX_SIZE-4,ini_file.CStr());
	*(DWORD*)(t_section.CStr()+aux)=0;
	if(aux > 2)
	{
		TSonorkExtAppLoadInfo		LI;
		char				*equ;
		LI.ini_file = ini_file.CStr();

		for(LI.app_name=t_section.CStr()
			;*LI.app_name !=0
			;LI.app_name +=  strlen(LI.app_name) + 1)
		{
			equ = strchr(LI.app_name,'=');
			if(equ == NULL)continue;
			*equ++=0;
			if(strlen(LI.app_name) >= LEA_MAX_APP_NAME_SIZE || *equ!='1')
				continue;
			LoadExtAppInfo( LI );
		}

	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ClearExtApps()
{
	TSonorkExtApp *EA;
	ClearExtAppMenu();
	while( (EA=ext_apps.RemoveFirst()) != NULL )
		SONORK_MEM_DELETE( EA );
	sonork_skin.image_list.dyn_icons=0;
}


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::LoadExtAppMenu()
{
	SONORK_EXT_APP_CONTEXT	context;
	UINT			no;
static	TSonorkWinGlsEntry
	predefined_view_apps[]=
	{{CM_APP_CHAT 		, GLS_CR_NAME}
	,{CM_APP_EMAIL 		, GLS_EM_SND}
	,{CM_APP_CLIPBOARD 	, GLS_LB_CLPBRD}
	,{CM_APP_REMINDER 	, GLS_LB_RMINDLST}
	,{CM_APP_SYS_CONSOLE 	, GLS_LB_SYSCON}
	,{CM_APP_SNAP_SHOT	, GLS_LB_SCRSHT}
	,{0,GLS_NULL}};
static	TSonorkWinGlsEntry
	predefined_user_apps[]=
	{{CM_APP_CHAT  , GLS_CR_NAME}
	,{CM_APP_EMAIL , GLS_EM_SND}
	,{CM_APP_TRACK , GLS_TR_CALL}
	,{0,GLS_NULL}};

	ClearExtAppMenu();

	for(no=0;predefined_view_apps[no].id!=0;no++)
	{
		win32.main_win->AddViewItem
		(
			new TSonorkIntAppViewItem( predefined_view_apps[no].id
				, predefined_view_apps[no].gls_index)
		);
	}

	for(no=0;predefined_user_apps[no].id!=0;no++)
	{
		AppendMenu(UserAppsMenu()
			, MF_STRING
			, predefined_user_apps[no].id
			, LangString(predefined_user_apps[no].gls_index) );
	}

	if( ext_apps.Items() )
	{
		TSonorkExtApp *EA;
		TSonorkListIterator	I;
		TSonorkTempBuffer	ini_file(SONORK_MAX_PATH);
		char			section[LEA_MAX_APP_NAME_SIZE + 16];
		char			v_name[LEA_MAX_VALUE_SIZE];
		char			v_data[LEA_MAX_VIEW_NAME_SIZE];
		SONORK_C_CSTR		view_name;
		SONORK_C_CSTR		app_name;

		GetExtAppsIniFile( ini_file.CStr() );
		ext_apps.BeginEnum(I);
		for(no=0; (EA=ext_apps.EnumNext(I)) != NULL ; no++)
		{
			if( EA->view_name != NULL )
			{
				SONORK_MEM_FREE(EA->view_name);
				EA->view_name=NULL;
			}
			context =EA->Context();
			app_name = EA->name.CStr();
			wsprintf(section
				, "%s.%s"
				, app_name
				, sonork_ext_app_context_name[context] );
			wsprintf(v_name,"name.%s",lang.table.LangLexeme());
			GetPrivateProfileString(section
				,v_name
				,szNotFoundMark
				,v_data
				,LEA_MAX_VALUE_SIZE
				,ini_file);
			if(IS_NOT_FOUND_MARK(v_data))
			{
				GetPrivateProfileString(section
					,"name"
					,szNotFoundMark
					,v_data
					,LEA_MAX_VALUE_SIZE
					,ini_file);
				if(IS_NOT_FOUND_MARK(v_data))
					strcpy(v_data,"%01");
			}

			view_name = v_data;
			if( v_data[0] == '%' )
			{
				wsprintf(section,"%s.name",app_name);
				lstrcpyn(v_name,v_data+1,LEA_MAX_VALUE_SIZE);
				GetPrivateProfileString(section
					,v_name
					,szNotFoundMark
					,v_data
					,LEA_MAX_VIEW_NAME_SIZE
					,ini_file);
				if(IS_NOT_FOUND_MARK(v_data))
					view_name = app_name;
			}

			if(context == SONORK_EXT_APP_USER_CONTEXT)
			{
				AppendMenu(UserAppsMenu()
					, MF_STRING
					, SONORK_APP_CM_EAPP_BASE + no
					, view_name );
			}
			else
			if(context == SONORK_EXT_APP_MAIN_CONTEXT)
			{
				win32.main_win->AddViewItem
				(
					new TSonorkExtAppViewItem(SONORK_APP_CM_EAPP_BASE + no
						, view_name
						, EA->flags
						, EA->icon)
				);
			}
			else
			{
				EA->view_name=SONORK_CStr(view_name);
			}
		}
		ext_apps.EndEnum(I);
	}
	win32.main_win->ExpandGroupViewItemLevels(SONORK_VIEW_SYS_GROUP_EXT_APPS,true);

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ClearExtAppMenu()
{
	int i;
	if(!(win32.run_flags&(SONORK_WAPP_RF_APP_TERMINATING|SONORK_WAPP_RF_APP_TERMINATED)))
	{
		win32.main_win->ClearViewGroup(SONORK_VIEW_SYS_GROUP_EXT_APPS);
		// If a catastrophic failure such as the resource corrupted/invalid
		// does not allow us to delete the menu, we'll bail out of
		// this loop with the <i> counter.
		for(i=0; GetMenuItemCount(menus.user_apps) && i<500;i++ )
			DeleteMenu(menus.user_apps,0,MF_BYPOSITION);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::UpdateUserExtAppMenu(const TSonorkUserData*pUD)
{
	TSonorkExtApp *EA;
	TSonorkListIterator	I;
	bool		is_non_zero_ip;
	bool		is_online;
	UINT		id;
	UINT		enable;
	if( pUD == NULL )
		return;

	is_non_zero_ip	= pUD->addr.physAddr.GetInet1Addr() != 0;
	is_online	= pUD->addr.sidFlags.SidMode() != SONORK_SID_MODE_DISCONNECTED;

	enable=MF_BYCOMMAND|(is_online?MF_ENABLED:MF_GRAYED);
	::EnableMenuItem( UserAppsMenu() , CM_APP_CHAT , enable);
	::EnableMenuItem( UserAppsMenu() , CM_APP_TRACK , enable);

	ext_apps.BeginEnum(I);
	for( id = SONORK_APP_CM_EAPP_BASE ; (EA=ext_apps.EnumNext(I)) != NULL ; id++)
	{
		if( EA->Context() != SONORK_EXT_APP_USER_CONTEXT )
			continue;

		if(EA->flags & SONORK_APP_EAF_NON_ZERO_IP)
			enable = is_non_zero_ip;
		else
			enable = true;
		if( enable )
			if(EA->flags & SONORK_APP_EAF_CONNECTED)
				enable = is_online;
		::EnableMenuItem( UserAppsMenu()
			, id
			, MF_BYCOMMAND|(enable?MF_ENABLED:MF_GRAYED));
	}
	ext_apps.EndEnum(I);
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::LoadExtAppInfo(TSonorkExtAppLoadInfo&LI)
{
	TSonorkTempBuffer		buffer( LEA_LINE_MAX_SIZE  );
	int					context,iIcon;
	SKIN_ICON			sIcon;
	HICON				hIcon;
	TSonorkExtAppLoadSection*SEC;
	TSonorkExtApp*			EA;

	for( context=0,SEC = LI.sec
		;context<SONORK_EXT_APP_CONTEXTS
		;context++, SEC++)
	{
		wsprintf(LI.section_name
			, "%s.%-.8s"
			, LI.app_name
			, sonork_ext_app_context_name[context]);
		GetPrivateProfileString(LI.section_name
			,"Cmd"
			,szNotFoundMark,buffer.CStr(),LEA_LINE_MAX_SIZE,LI.ini_file);
		SEC->type 	= buffer.CStr();
		SEC->attr	= strchr(SEC->type,',');
		if(SEC->attr == NULL)continue;
		*SEC->attr++=0;
		SEC->cmd=strchr(SEC->attr,',');
		if(SEC->cmd == NULL)continue;
		*SEC->cmd++=0;
		SEC->extra_params=strchr(SEC->cmd,',');
		if(SEC->extra_params)
			*SEC->extra_params++=0;

		if(*SEC->type==0||*SEC->cmd==0)
			continue;

		if(!stricmp(SEC->type,"REG"))
			EA = LoadExtAppInfo_Reg(SEC);
		else
		if(!stricmp(SEC->type,"EXE"))
			EA = LoadExtAppInfo_Exe(SEC);
		else
			continue;
		if( EA != NULL )
		{
			if(context == SONORK_EXT_APP_MAIN_CONTEXT )
			if(sonork_skin.image_list.dyn_icons<SKIN_MAX_DYN_ICONS)
			{
				iIcon = (int)GetPrivateProfileInt(LI.section_name,"Icon",-1,LI.ini_file);
				if(iIcon>=0 && iIcon<=128)
				{
					lstrcpyn(buffer.CStr() , EA->Cmd(), EA->CmdLen()+1);
					iIcon=ExtractIconEx(buffer.CStr(),iIcon,NULL,&hIcon,1);
					if(iIcon==1 && hIcon != NULL)
					{
						sIcon = SKIN_DYN_ICON(sonork_skin.image_list.dyn_icons);
						if(ImageList_ReplaceIcon(sonork_skin.image_list.icon
							,sIcon
							,hIcon) != -1)
						{
							EA->icon = sIcon;
							sonork_skin.image_list.dyn_icons++;
						}
						DestroyIcon(hIcon);
					}
				}
			}
			EA->name.Set(LI.app_name);
			GetPrivateProfileString(LI.section_name,"Flags","",buffer.CStr(),LEA_LINE_MAX_SIZE,LI.ini_file);
			EA->flags =LoadExtAppInfo_Flags(buffer.CStr()) & ~(SONORK_APP_EA_CONTEXT_MASK);
			EA->flags|=(context&SONORK_APP_EA_CONTEXT_MASK);
			ext_apps.Add( EA );
		}
	}
}

// ----------------------------------------------------------------------------

UINT
 TSonorkWin32App::LoadExtAppInfo_Flags( char*buffer )
{
	char *p1,*p2;
	UINT flags=0;
	p1=buffer;
	while(p1)
	{
		p2=strchr(p1,',');
		if(p2)*p2++=0;
		SONORK_StrTrim(p1);
		if(!stricmp(p1,"NonZeroIP"))
			flags|=SONORK_APP_EAF_NON_ZERO_IP;
		else
		if(!stricmp(p1,"SendReq"))
			flags|=SONORK_APP_EAF_SEND_REQ;
		else
		if(!stricmp(p1,"WaitAkn"))
			flags|=SONORK_APP_EAF_WAIT_AKN;
		else
		if(!stricmp(p1,"Online"))
			flags|=SONORK_APP_EAF_CONNECTED;
		p1=p2;
	}

	return flags;
}


// ----------------------------------------------------------------------------

TSonorkExtApp*
 TSonorkWin32App::LoadExtAppInfo_Exe(TSonorkExtAppLoadSection*SEC)
{
	TSonorkExtApp*EA;
	TSonorkTempBuffer	search_path(SONORK_MAX_PATH+64);
	char			*dummy;
	const char		*cmd;
	DWORD			aux;
	SONORK_EXT_APP_PARAM_TYPE
				param_type;

	param_type = ParseExtAppParams_Type(SEC->cmd);
	if( param_type != SONORK_EXT_APP_PARAM_RELATIVE
		&& param_type != SONORK_EXT_APP_PARAM_ABSOLUTE )
		return NULL;

	cmd = SEC->cmd+VALUE_TYPE_PREFIX_LENGTH; // Skip Value type
	SONORK_MEM_NEW
	(
		EA = new TSonorkExtApp(SONORK_EXT_APP_TYPE_EXE)
	);
	
	// The command may contain parameters after it,
	// SetCmd() loads the first parameter into
	// EA->full_cmd and then splits it into EA->cmd and EA->params
	// it appends <extra_params> to EA->params
	if( param_type == SONORK_EXT_APP_PARAM_RELATIVE)
	{
		// Relative: Means that the <EA->cmd> is not the full path,
		// search in the path
		if(!SearchPath(NULL
				,cmd
				,NULL
				,SONORK_MAX_PATH
				,search_path.CStr()
				,&dummy))
		{
			param_type = SONORK_EXT_APP_PARAM_INVALID;
		}
		else
		{
			EA->SetCmd(search_path.CStr() , SEC->extra_params , true);
		}
	}
	else
	{
		aux = GetFileAttributes( cmd );
		if( aux==(DWORD)-1
			|| (aux&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_OFFLINE)) )
			param_type = SONORK_EXT_APP_PARAM_INVALID;
		else
			EA->SetCmd( cmd , SEC->extra_params , true );
	}
	if( param_type == SONORK_EXT_APP_PARAM_INVALID )
	{
		SONORK_MEM_DELETE( EA );
		return NULL;
	}
	return EA;
}

// ----------------------------------------------------------------------------

TSonorkExtApp*
	TSonorkWin32App::LoadExtAppInfo_Reg(TSonorkExtAppLoadSection*SEC)
{
	HKEY			root_key;
	TSonorkRegKey		KEY;
	LONG			errc;
	TSonorkExtApp*		EA=NULL;
	char			*key_name,*ptr,*value_name;
	SONORK_EXT_APP_PARAM_TYPE	param_type;
#define KEY_VALUE_SIZE	512
	TSonorkTempBuffer	key_value(KEY_VALUE_SIZE);

	param_type = ParseExtAppParams_Type(SEC->cmd);
	if( param_type != SONORK_EXT_APP_PARAM_RELATIVE
	 && param_type != SONORK_EXT_APP_PARAM_ABSOLUTE )
	{
		return NULL;
	}

	if( strlen(SEC->cmd)  > 256)
	{
		return NULL;
	}

	ptr		= SEC->cmd+VALUE_TYPE_PREFIX_LENGTH;
	key_name 	= strchr(ptr,'\\');
	if(!key_name)
	{
		return NULL;
	}
	*key_name++=0;

	if(!stricmp(ptr,"HLM"))
		root_key = HKEY_LOCAL_MACHINE;
	else
	if(!stricmp(ptr,"HCR"))
		root_key = HKEY_CLASSES_ROOT;
	else
	if(!stricmp(ptr,"HCU"))
		root_key = HKEY_CURRENT_USER;
	else
	{
		return NULL;
	}


	if(param_type==SONORK_EXT_APP_PARAM_RELATIVE)
		value_name=NULL;
	else
	{
		value_name = ptr = key_name;
		while(*ptr)
		{
			if(*ptr=='\\')
				value_name=ptr;
			ptr++;
		}
		*value_name++=0;
	}

	errc=KEY.Open(root_key,key_name,false,KEY_READ);
	if( errc == ERROR_SUCCESS )
	{
		errc = KEY.QueryValue(value_name,key_value.CStr(),KEY_VALUE_SIZE);
		KEY.Close();
	}
	if( errc == ERROR_SUCCESS)
	{
		SONORK_MEM_NEW( EA = new TSonorkExtApp(SONORK_EXT_APP_TYPE_REG) );
		EA->SetCmd(key_value.CStr()
			, SEC->extra_params
			, strchr(SEC->attr,'Q')!=NULL	// quote the command if <Q> is present in the attributes
			);
	}
	return EA;
}


// ----------------------------------------------------------------------------

SONORK_EXT_APP_PARAM_TYPE
 TSonorkWin32App::ParseExtAppParams_Type( SONORK_C_CSTR str )
{
	if(!strnicmp(str,szRelValue,VALUE_TYPE_PREFIX_LENGTH))
		return SONORK_EXT_APP_PARAM_RELATIVE;
	if(!strnicmp(str,szAbsValue,VALUE_TYPE_PREFIX_LENGTH))
		return SONORK_EXT_APP_PARAM_ABSOLUTE;
	if(!strnicmp(str,szTmpValue,VALUE_TYPE_PREFIX_LENGTH))
		return SONORK_EXT_APP_PARAM_TEMPORAL;
	return SONORK_EXT_APP_PARAM_INVALID;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::ParseExtAppParams(const TSonorkExtApp*EA
		,SONORK_C_STR	params
		,const TSonorkExtUserData*UD)
{

	TSonorkTempBuffer	ini_file(SONORK_MAX_PATH);
	TSonorkTempBuffer	aux_buffer(SONORK_MAX_PATH);
	TSonorkTempBuffer	param_value(LEA_LINE_MAX_SIZE);
	char			section[LEA_MAX_APP_NAME_SIZE+10];
	char			param_name[LEA_MAX_PARAM_NAME_SIZE+8];
	int			c_len;
	const char		*s_ptr;
	char			*c_ptr,*value_ptr;
	SONORK_EXT_APP_PARAM_TYPE
				param_type;

	c_len = EA->Context();
	if( (c_len == SONORK_EXT_APP_USER_CONTEXT) && UD == NULL )
		return false;

	strcpy(param_name,"param.");
	wsprintf(section,"%s.%s"
		,EA->name.CStr()
		,sonork_ext_app_context_name[c_len]);

	GetExtAppsIniFile(ini_file.CStr());

	c_len = 0;
	s_ptr = EA->Cmd();
	c_ptr = params;
	while( *s_ptr && c_len<LEA_MAX_PARAMS_SIZE-1 )
	{
		if( *s_ptr == '%')
		{
			s_ptr++;
			if( EA_LoadToken(&s_ptr,param_name+6,LEA_MAX_PARAM_NAME_SIZE) )
			{
				GetPrivateProfileString(section,param_name,szNotFoundMark,param_value.CStr(),LEA_LINE_MAX_SIZE,ini_file);
				param_type = ParseExtAppParams_Type(param_value.CStr());
			}
			else
				param_type = SONORK_EXT_APP_PARAM_INVALID;

			if(param_type != SONORK_EXT_APP_PARAM_INVALID
			&& param_type !=SONORK_EXT_APP_PARAM_ABSOLUTE)
			{
				if( param_type == SONORK_EXT_APP_PARAM_RELATIVE )
				{
					value_ptr=param_value.CStr()+VALUE_TYPE_PREFIX_LENGTH;
					ExpandExtAppVar( value_ptr
						, aux_buffer.CStr()
						, SONORK_MAX_PATH
						, UD);
					value_ptr = aux_buffer.CStr();
				}
				else
				if( param_type == SONORK_EXT_APP_PARAM_TEMPORAL )
				{
					if(ParseExtAppParams_MakeTempFile(aux_buffer.CStr(),EA,value_ptr,UD))
						value_ptr = aux_buffer.CStr();
					else
						param_type = SONORK_EXT_APP_PARAM_INVALID;
				}
				else
					param_type = SONORK_EXT_APP_PARAM_INVALID;
			}
			if(param_type == SONORK_EXT_APP_PARAM_INVALID)
			{
				c_len = -1;
				break;
			}
			while( *value_ptr!=0 && c_len<LEA_MAX_PARAMS_SIZE-1 )
			{
				*c_ptr++ = *value_ptr++;
				c_len++;
			}
		}
		else
		{
			*c_ptr++ = *s_ptr++;
			c_len++;
		}
	}
	*c_ptr=0;
	return c_len>=0;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::ParseExtAppParams_MakeTempFile(SONORK_C_STR tgt
		,const TSonorkExtApp*EA
		,SONORK_C_STR ea_file_name
		,const TSonorkExtUserData*UD)
{
#define I_BUFFER_SIZE	256
#define O_BUFFER_SIZE	(I_BUFFER_SIZE*2)
	TSonorkTempBuffer	i_buffer(I_BUFFER_SIZE);
	TSonorkTempBuffer	o_buffer(O_BUFFER_SIZE);
	char			str[32];
	FILE			*i_file;
	FILE			*o_file;
	bool			rv=false;
	GetDirPath(tgt,SONORK_APP_DIR_APPS,EA->name.CStr());
	strcat(tgt,"\\");
	strcat(tgt,ea_file_name);
	i_file = fopen(tgt,"rt");
	if(i_file != NULL)
	{
		if( UD != NULL )
			UD->userId.GetStrEx(str,NULL,".txt");
		else
			strcpy(str,"0.0.txt");
		GetTempPath(tgt ,"LEA", str , (DWORD)-1);
		o_file = fopen( tgt , "wt");
		if(o_file != NULL )
		{
			while( fgets(i_buffer,I_BUFFER_SIZE,i_file) != NULL )
			{
				ExpandExtAppVar(i_buffer.CStr()
					,o_buffer.CStr()
					,O_BUFFER_SIZE
					,UD);
				fputs(o_buffer,o_file);
			}
			fclose(o_file);
			rv=true;
		}
		fclose(i_file);
	}
	return rv;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::ExpandExtAppVar(SONORK_C_CSTR s_ptr
	, SONORK_C_STR t_ptr
	, UINT tgt_size
	, const TSonorkExtUserData*UD)
{
	UINT 			t_len,p_len;
	SONORK_C_STR		p_ptr;
	char			c,param_name[LEA_MAX_PARAM_NAME_SIZE];
	t_len = 0;
	assert( tgt_size>0 );
	tgt_size--;
	while(*s_ptr && t_len < tgt_size)
	{
		c = *s_ptr++;
		if( c=='%' )
		{
			if( *s_ptr=='%' )
			{
				s_ptr++;
			}
			else
			{
				EA_LoadToken(&s_ptr,param_name,LEA_MAX_PARAM_NAME_SIZE);
				if(!strnicmp(param_name,"User",4))
				{
					if(UD==NULL)continue;
					p_ptr=param_name+4;
					if(!stricmp(p_ptr,szSonorkId))
					{
						p_len = wsprintf(t_ptr,"%u.%u",UD->userId.v[0],UD->userId.v[1]);
					}
					else
					if(!stricmp(p_ptr,"IP"))
					{
						p_len = wsprintf(t_ptr,"%s",inet_ntoa(UD->addr.physAddr.data.inet1.addr.sin_addr));
					}
					else
					if(!stricmp(p_ptr,szAlias))
					{
						p_len = wsprintf(t_ptr,"%.28s",UD->display_alias.CStr());
					}
					else
					if(!stricmp(p_ptr,"Sid"))
					{
						p_len = wsprintf(t_ptr,"%u.%u",UD->addr.sid.v[0],UD->addr.sid.v[1]);
					}
					else
					if(!stricmp(p_ptr,"SidFlags"))
					{
						p_len = wsprintf(t_ptr,"%u.%u.%u.%u"
								,UD->addr.sidFlags.v[0].v[0]
								,UD->addr.sidFlags.v[0].v[1]
								,UD->addr.sidFlags.v[1].v[0]
								,UD->addr.sidFlags.v[1].v[1]
								);
					}
					else
						continue;
				}
				else
				if(!strnicmp(param_name,"App",3))
				{
					p_ptr=param_name+3;
					if(!stricmp(p_ptr,"IpcName"))
						p_len = wsprintf(t_ptr
							,"%s"
							,ConfigName());
					else
						continue;
				}
				else
					continue;
				t_len += p_len;
				t_ptr += p_len;
				continue;
			}
		}
		*t_ptr++=c;
		t_len++;
	}
	*t_ptr=0;
}

// ----------------------------------------------------------------------------
// Global variables definitions

SONORK_C_CSTR
 sonork_ext_app_context_name[SONORK_EXT_APP_CONTEXTS]=
{
	{"MAIN"}
,	{"USER"}
,	{"CTRL"}
};


// ----------------------------------------------------------------------------
// TSonorkExtApp

TSonorkExtApp::TSonorkExtApp(SONORK_EXT_APP_TYPE t)
{
	type=t;
	icon = SKIN_ICON_NONE;
	view_name=NULL;
}

TSonorkExtApp::~TSonorkExtApp()
{
	if(view_name)SONORK_MEM_FREE(view_name);
}

void
 TSonorkExtApp::SetCmd(const char *p_cmd, const char *extra_params, bool quoted)
{
	UINT	c_len,e_len;
	char	*ptr;
	c_len = (UINT)(strlen(p_cmd));
	e_len = (UINT)(extra_params==NULL? 0 : strlen(extra_params));
	cmd.SetBufferSize(c_len + e_len + 2);
	ptr = cmd.Buffer();
	ptr += EA_UnquotedStrCopy( ptr , p_cmd , &cmd_len, quoted);
	if( e_len )
	{
		*ptr++=' ';
		EA_UnquotedStrCopy( ptr , extra_params , NULL , false);
	}
}

// ----------------------------------------------------------------------------
// TSonorkExtAppReq

enum SONORK_EXT_APP_REQ_CODEC_FLAGS
{
	SONORK_EXT_APP_REQ_CODEC_F_ALIAS	= 0x00010000
};

TSonorkExtAppReqAtom::TSonorkExtAppReqAtom()
{
	SONORK_ZeroMem(params,sizeof(params));
}
DWORD
 TSonorkExtAppReqAtom::CODEC_DataSize() const
{
	return  ::CODEC_Size(&app_name)
		+	::CODEC_Size(&phys_addr)
		+	::CODEC_Size(&data)
		+	::CODEC_Size(&alias)
		+	sizeof(params)
		+	sizeof(DWORD);
}
void
 TSonorkExtAppReqAtom::CODEC_Clear()
{
	SONORK_ZeroMem(params,sizeof(params));
	app_name.Clear();
	phys_addr.Clear();
	data.Clear();
	alias.Clear();
}
void
 TSonorkExtAppReqAtom::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW(SONORK_EXT_APP_REQ_CODEC_F_ALIAS|sizeof(params));
	CODEC.WriteDWN(params,SIZEOF_IN_DWORDS(params));
	CODEC.Write(&app_name);
	CODEC.WritePA1(&phys_addr);
	CODEC.Write(&data);
	CODEC.Write(&alias);
}
void
 TSonorkExtAppReqAtom::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD codec_flags;
	CODEC.ReadDW(&codec_flags);
	CODEC.ReadDWN(params,SIZEOF_IN_DWORDS(params));
	CODEC.Read(&app_name);
	CODEC.ReadPA1(&phys_addr);
	CODEC.Read(&data);
	if( codec_flags & SONORK_EXT_APP_REQ_CODEC_F_ALIAS)
		CODEC.Read(&alias);
	else
		alias.Clear();
}

// ----------------------------------------------------------------------------
// Static functions definitions

// ----------------------------------------------------------------------------
// EA_UnquotedStrCopy() copies the source without the quotes
// and parses the special sequence \" (bar + quote: converts it to a single quote)
// if <eoc> is not NULL, it returns the position of the first
// blank which is not quoted. If no blank is found, <eoc>
// is the position of the terminating 0.
// if quoted is true, EA_UnquotedStrCopy() assumes the whole string is quoted
// and hence <eoc> will always point to the terminating null.

static UINT
	EA_UnquotedStrCopy(char *t_ptr, const char *s_ptr, UINT *p_eoc, bool quoted)
{
	char *o_t_ptr;
	UINT	 t_len
		,eoc=(UINT)-1;
	if( quoted )
	{
		strcpy(t_ptr , s_ptr );
		t_len = strlen(t_ptr);
	}
	else
	{
		o_t_ptr = t_ptr;
		while( *s_ptr )
		{
			if(*s_ptr =='\"')
			{
				quoted=!quoted;
				s_ptr++;
				continue;
			}
			if(*s_ptr==' ' && !quoted && eoc==-1)
				eoc=(t_ptr-o_t_ptr);

			*t_ptr++=*s_ptr++;
		}
		*t_ptr=0;
		t_len=t_ptr - o_t_ptr;
	}
	if( p_eoc != NULL )
	{
		if(eoc==(UINT)-1)
			eoc = t_len;
		*p_eoc=eoc;
	}
	return t_len;
}


// ----------------------------------------------------------------------------
// EA_LoadToken

static UINT
 EA_LoadToken(SONORK_C_CSTR* s_d_ptr, SONORK_C_STR t_ptr, UINT tgt_size)
{
	UINT	t_len;
	SONORK_C_CSTR s_ptr;

	assert( tgt_size > 0);
	s_ptr = *s_d_ptr;
	tgt_size --;
	for(  t_len=0
		; !EA_IsDelimiter(*s_ptr) && t_len<tgt_size
		; t_len++)
		*t_ptr++=*s_ptr++;
	*t_ptr=0;
	*s_d_ptr = s_ptr;
	return t_len;
}

// ----------------------------------------------------------------------------
// EA_IsDelimiter

static bool
 EA_IsDelimiter(char c)
{
	if( c>=0 && c<=' ' ) return true;
	return strchr("|°¬!\"#$%&/()=?¡'¿´¨*+~[]{}^;,:.-_",c)!=NULL;
}

