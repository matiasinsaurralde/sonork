#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkfiletxgui.h"
#include "srk_file_io.h"

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


#define POKE_SEND_MESSAGE_RESULT	SONORK_WIN_POKE_01
#define POKE_CONTINUE			SONORK_WIN_POKE_02
#define NOTIFICATION_MSG_SENT		SONORK_WIN_F_USER_02
//#define CCACHE_LINE_MARKED		SONORK_WIN_F_USER_03
#define PROGRESS_SIZE(sz)		(sz>>10)


TSonorkFileTxGui::TSonorkFileTxGui(SONORK_DWORD2* target_list
		, UINT 			target_count
		, SONORK_C_CSTR p_alias
		, TSonorkShortStringQueue*queue
		, UINT tx_flags)
	:TSonorkTaskWin(NULL
			,SONORK_WIN_CLASS_NORMAL
			|SONORK_WIN_TYPE_NONE
			|SONORK_WIN_DIALOG
			|IDD_FILETX
			,SONORK_WIN_SF_NO_WIN_PARENT)
{
	assert(target_list != NULL && target_count);
	SetEventMask(SONORK_APP_EM_MSG_PROCESSOR);

	target.count=target_count;
	target_count*=sizeof(SONORK_DWORD2);
	target.ptr = SONORK_MEM_ALLOC(TSonorkId,target_count);
	memcpy(target.ptr,target_list,target_count);
	user.alias.Set(p_alias);
	msg_mark.Clear();
	tx_eng = new TSonorkFileTxEng(this,queue,FileTxEngHandler,this,tx_flags);
}


TSonorkFileTxGui::TSonorkFileTxGui(
	  SONORK_C_CSTR alias
	, SONORK_C_CSTR p_path
	, TSonorkCCacheMark& mark
	, const TSonorkFileInfo& file_info, UINT tx_flags)
	:TSonorkTaskWin(NULL
			,SONORK_WIN_CLASS_NORMAL
			|SONORK_WIN_TYPE_NONE
			|SONORK_WIN_DIALOG
			|IDD_FILETX
			,SONORK_WIN_SF_NO_WIN_PARENT)
{
	SetEventMask(SONORK_APP_EM_MSG_PROCESSOR);
	target.ptr = NULL;
	sender.Set( file_info.locus.creator );
	user.alias.Set(alias);
	msg_mark.Set( mark );
	tx_eng = new TSonorkFileTxEng(this,file_info,p_path,FileTxEngHandler,this,tx_flags);
}
TSonorkFileTxGui::~TSonorkFileTxGui()
{
	if(target.ptr)SONORK_MEM_FREE(target.ptr);
}


// ----------------------------------------------------------------------------
// OnCreate:
//  Just after window handle has been created but before TSonorkWin's Create()
//  returns to the caller.
//  Sets the window icon and caption
//  Creates the TSonorkFileTxEng engine that will transfer the file
//  And loads the startup position from the configuration file.
//  Because this dialog (IDD_FILETX) has not the WS_VISIBLE flag
//  it will remain invisible until OnAfterCreate() shows it.

bool
 TSonorkFileTxGui::OnCreate()
{
	SKIN_HICON hicon;

	msg_task_id=0;

	// Get the handle of the status bar
	status_hwnd=GetDlgItem(IDC_FILETX_STATUS);

	// Get the handle of the progress bar
	progress_hwnd = GetDlgItem(IDC_FILETX_PROGRESS);

	// Set the window's caption
	SetWindowText( user.alias.CStr() );


	// Try to create the engine
	if(!tx_eng->Create())
	{
		// Failed: When a TSonorkWin' Create() fails
		// the application will not automatically delete
		// the class instance: We must do it manually.
		delete tx_eng;
		tx_eng=NULL;

		// Post ourselves the destruction message.
		// TSonorkWin's Destroy() or EndDialog() should
		// not be invoked from within OnCreate()

		PostPoke(SONORK_WIN_POKE_DESTROY,IDCANCEL);
		// Return value of OnCreate() is ignored.
		return true;
	}

	if( tx_eng->TxMode()==SONORK_FILE_TX_MODE_SEND )
	{
		// We're sending a file or files
		hicon = SKIN_HICON_FILE_UPLOAD;

		// Leave the file name text in blank for now
	}
	else
	{
		// We're downloading or deleting a file
		hicon = (tx_eng->TxMode()==SONORK_FILE_TX_MODE_RECV)
				?SKIN_HICON_FILE_DOWNLOAD
				:SKIN_HICON_FILE_DELETE;

		// Set the file name's text
		SetCtrlText(IDC_FILETX_FILENAME
			,tx_eng->FileInfo().name.CStr());
	}

	// Set the caption
	SetCaptionIcon( hicon );

	// Load the startup position from the application's configuration file
	SonorkApp.TransferWinStartInfo( this , true, "FileTxGui", NULL);
	return true;
}

// ----------------------------------------------------------------------------
// OnAfterCreate:
//  Invoked after TSonorkWin's Create() has returned.
//  If not invisible, shows the window and, if profile configuration specifies,
//   asks the user to confirm that this file should be uploaded.
//  Starts the engine.

void
 TSonorkFileTxGui::OnAfterCreate()
{
	TSonorkShortString *s;
	if( !(tx_eng->TxFlags() &SONORK_FILE_TX_F_INVISIBLE ) )
	{
		// Not invisible
		ShowWindow(SW_SHOW);

		// If applicable, query the user before sending
		if( tx_eng->TxMode()==SONORK_FILE_TX_MODE_SEND )
		if( SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_CONFIRM_FILE_SEND) )
		{
			s=tx_eng->SendQueue()->Peek();
			if(s)
			{
				TSonorkTempBuffer buffer(SONORK_MAX_PATH+128);
				SonorkApp.LangSprintf(buffer.CStr()
					, GLS_QU_SFILUSR
					, s->CStr()
					, user.alias.CStr());
				if(MessageBox(buffer.CStr() , GLS_OP_PUTFILE , MB_ICONQUESTION|MB_YESNO) != IDYES)
				{
					Destroy();
					return;
				}

			}
		}
	}

	// Ok, engine, do your job:
	// TSonorkFileTxEng::Continue() will post the message to the engine,
	// this means that no action is yet taken when Continue() returns
	// (it will not invoke our callback nor do anything).
	// The actual execution of the CONTINUE command will only
	//  take place after WE return control to the app.
	tx_eng->Continue();
}

// ----------------------------------------------------------------------------
// OnBeforeDestroy():
//  Invoked just before destroying the windows handle and before TSonorkWin
//  performs any cleanups (destroy timers, tooltips, etc.)
//  Saves the current position on screen to the profile's configuration file.
//  If the engine has not been destroyed yet, we do it now to prevent
//   it from calling our handlers after we're gone.

void
 TSonorkFileTxGui::OnBeforeDestroy()
{
	if(IsVisible() && !IsIconic() )
	{
		// Save our current position
		SonorkApp.TransferWinStartInfo( this , false, "FileTxGui", NULL);
	}
	
	if( tx_eng != NULL )
	{
		// No error: Assume FORCED_TERMINATION or whatever.. we're
		// don't care any more: We've been terminated.
		tx_eng->Terminate( NULL );

		// Mark engine as destroyed
		tx_eng=NULL;
	}
}

// ----------------------------------------------------------------------------
// FileTxEngHandler
// Handles the events fired by our TSonorkFileTxEng engine
// It is static, but we get the a pointer to <this> as the callback parameter
// we provided the engine with when it was created.

void SONORK_CALLBACK
 TSonorkFileTxGui::FileTxEngHandler(void *param,TSonorkFileTxEngEvent*E)
{
	TSonorkFileTxGui *_this = (TSonorkFileTxGui*)param;
	char tmp[128];
        GLS_INDEX	gls;
	switch( E->event )
	{
		case SONORK_FILE_TX_EVENT_ERROR:
		// We handle all errors together in OnTxEng_Error()
			_this->OnTxEng_Error(E);
		break;

		case SONORK_FILE_TX_EVENT_PHASE:
			// Reuse tmp
			_this->OnTxEng_Phase(E);
			break;

		case SONORK_FILE_TX_EVENT_TARGET_RESULT:
			_this->OnTxEng_Target_Result(E->target_result);
			break;


		case SONORK_FILE_TX_EVENT_PROGRESS:
			::SendMessage(_this->progress_hwnd
				,PBM_SETPOS
				,PROGRESS_SIZE(E->progress.offset)
				,0);
			if( E->progress.phase == SONORK_FILE_TX_PHASE_SENDING
			||  E->progress.phase == SONORK_FILE_TX_PHASE_RECEIVING)
			{
				gls=GLS_MS_TX_INFO2;
			}
			else
			if( E->progress.phase == SONORK_FILE_TX_PHASE_COMPRESSING)
			{
				gls=GLS_MS_ZIP_INFO;
			}
			else
			{
				gls=GLS_MS_UNZIP_INFO;
			}

			SonorkApp.LangSprintf(tmp
				,gls
				,E->progress.offset
				,_this->progress_limit);
			TSonorkWin::SetStatus(_this->status_hwnd
				,tmp
				,SKIN_HICON_BUSY);

			break ;

	}
}

void
 TSonorkFileTxGui::OnTxEng_Target_Result(TSonorkDataServerPutFileTargetResult* R)
{
	char	tmp[64];
	if( target.ptr == NULL )return;
	if(R->ERR.Result() == SONORK_RESULT_OK )
	{
		target.sent++;
		if(target.count>1)
		{
			wsprintf(tmp
				,"%s %u/%u"
				,SonorkApp.LangString(GLS_MS_SNDINGMSG)
				,target.sent
				,target.count);
			TSonorkWin::SetStatus(status_hwnd,tmp,SKIN_HICON_BUSY);
		}
		SendNotificationMessage( &R->header.sonork_id, true );
		user.msg.Clear();
	}
	else
	if(target.count==1)
	{
		TaskErrorBox(GLS_TK_SNDMSG,&R->ERR);
		Destroy();
	}
}

void
 TSonorkFileTxGui::OnTxEng_Phase(TSonorkFileTxEngEvent*E)
{

	switch(E->phase.phase)
	{
		default:
		case SONORK_FILE_TX_PHASE_NONE:
			break;

		case SONORK_FILE_TX_PHASE_RECEIVING:
			MarkCCacheLine(0,false);
			// break ommited, should follow to Status "please wait"
		case SONORK_FILE_TX_PHASE_RESOLVING:
		case SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM:
		case SONORK_FILE_TX_PHASE_SENDING:
			TSonorkWin::SetStatus_PleaseWait(status_hwnd);
			break;
		case SONORK_FILE_TX_PHASE_COMPRESSING:
		case SONORK_FILE_TX_PHASE_EXPANDING:
			StartProgress(E->phase_size.offset,E->phase_size.size);
			break;

		case SONORK_FILE_TX_PHASE_CONNECTING:
			TSonorkWin::SetStatus(status_hwnd,GLS_MS_CXTING,SKIN_HICON_BUSY);
			break;

		case SONORK_FILE_TX_PHASE_AUTHORIZING:
			TSonorkWin::SetStatus(status_hwnd,GLS_MS_AUTHING,SKIN_HICON_BUSY);
			break;

		case SONORK_FILE_TX_PHASE_REQUESTING:
			OnTxEng_Phase_File_Request( E );
			break;

		case SONORK_FILE_TX_PHASE_WAIT_RECV_CONFIRM:
			// File has been completely received, we're just
			// waiting for the confirmation we send to the
			// server to be replied. Still, regardless of the
			// the outcome of the confirmation packet, the
			// file has been successfully downloaded.
			TSonorkWin::SetStatus_PleaseWait(status_hwnd);
			OnTxEng_Phase_File_Complete( true );
			break;

		case SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE:
			OnTxEng_Phase_File_Complete( false );
			break;

		case SONORK_FILE_TX_PHASE_FINISHED:
			Destroy(IDOK);
			break;
	}
}
void
 TSonorkFileTxGui::StartProgress(DWORD offset, DWORD size)
{
	progress_limit=size;
	::SendMessage(progress_hwnd,PBM_SETPOS
		,0
		,MAKELPARAM(0, PROGRESS_SIZE(offset) ));
	::SendMessage(progress_hwnd,PBM_SETRANGE
		,0
		,MAKELPARAM(0, PROGRESS_SIZE(progress_limit) ));
}

// ----------------------------------------------------------------------------
// OnTxEng_Phase_File_Request
// This event is fired once for each file being sent/received
void
 TSonorkFileTxGui::OnTxEng_Phase_File_Request(TSonorkFileTxEngEvent*E)
{
	UINT		i;
	SONORK_DWORD2*	pdw2;

	target.sent=0;
	ClearWinUsrFlag( NOTIFICATION_MSG_SENT );

	SetCtrlText(IDC_FILETX_FILENAME
		,tx_eng->FileInfo().name.CStr());
	StartProgress(E->phase_size.offset,E->phase_size.size);

	// Ok, when in SEND mode, we must load the targets for the file
	if( tx_eng->TxMode() == SONORK_FILE_TX_MODE_SEND )
	{
		assert( target.ptr != NULL );
		if( tx_eng->IsNewProtocol() )
		{
		// This is the new 'official' way: More than one target
		// may be specified for each file (Currently up to approx. 4096)
			for(	i=0
				,pdw2=target.ptr
				;i<target.count
				;i++
				,pdw2++)
			{
				E->phase_req_send.target.list->AddItem(pdw2);
			}
		}
		else
		{
		// This is the old way and is kept here only to be compatible
		// with old data servers: Only one target per file
			E->phase_req_send.target.user->Set(*target.ptr);
		}
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxGui::OnTxEng_Phase_File_Complete( BOOL close_wait )
{
	DWORD 	mark_flags;
	switch( tx_eng->TxMode() )
	{
		case SONORK_FILE_TX_MODE_SEND:
			if(!TestWinUsrFlag(NOTIFICATION_MSG_SENT) )
			{
				assert( target.ptr != NULL );
				SendNotificationMessage( target.ptr , false );
			}
			else
			{
				tx_eng->Continue();
			}
		return;


		case SONORK_FILE_TX_MODE_RECV:
			mark_flags = SONORK_APP_CCF_PROCESSED;
		break;

		case SONORK_FILE_TX_MODE_DELETE:
			mark_flags = SONORK_APP_CCF_PROCESSED|SONORK_APP_CCF_DELETED;
		break;

		default:
			return;
	}
	MarkCCacheLine(mark_flags
		,tx_eng->TxMode()!=SONORK_FILE_TX_MODE_DELETE);
	if( tx_eng != NULL && !close_wait )
	{
		if( tx_eng->TxMode() == SONORK_FILE_TX_MODE_RECV )
		{
			UINT action=tx_eng->TxFlags()&SONORK_FILE_TX_FM_ACTION;
			if(action!=0)
			{
				SONORK_C_CSTR	o_path;
				o_path = tx_eng->FilePath().CStr();
				if( action ==  SONORK_FILE_TX_FV_OPEN_FOLDER )
				{
					SONORK_C_STR FileNamePtr;
					FileNamePtr = (char*)SONORK_IO_GetFileName( o_path );
					*FileNamePtr = 0;
				}
				else
				if( action ==  SONORK_FILE_TX_FV_OPEN_FILE )
				{
					o_path = tx_eng->FilePath().CStr();
				}
				else
					o_path=NULL;

				if( o_path != NULL )
				{
					SonorkApp.ShellOpenFile( NULL , o_path , false );
				}
			}
		}
		tx_eng->Continue();
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxGui::OnTxEng_Error(TSonorkFileTxEngEvent*E)
{
	TSonorkShortString 	tmp;
	SONORK_C_CSTR		str;
	GLS_INDEX		gls=GLS_NULL;
	SONORK_SYS_STRING	sys=GSS_NULL;
	bool			no_report=false;

	if( TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
		return;
	switch( E->error.tx_err )
	{

		case SONORK_FILE_TX_ERROR_UNKNOWN:
			sys = GSS_INTERNALERROR;
			break;

		case SONORK_FILE_TX_ERROR_RESOLVE:
			gls = GLS_MS_NODATASVRS;
			break;

		case SONORK_FILE_TX_ERROR_AUTHORIZE:
			gls = GLS_MS_AUTHDENIED;
			break;

		case SONORK_FILE_TX_ERROR_REQUEST:
			gls = GLS_MS_REQDENIED;
			if((E->error.pERR->Result() == SONORK_RESULT_NO_DATA
			|| E->error.pERR->Result() == SONORK_RESULT_ACCESS_DENIED)
			&& tx_eng->TxMode()!=SONORK_FILE_TX_MODE_SEND)
			{
				// Don't report as error if the file
				// was to be deleted
				no_report = tx_eng->TxMode()==SONORK_FILE_TX_MODE_DELETE;
				MarkCCacheLine(
					SONORK_APP_CCF_DELETED|SONORK_APP_CCF_PROCESSED
					,false);
			}
			break;

		case SONORK_FILE_TX_ERROR_FILE_OPEN:
			sys = GSS_FILEOPENERR;
			break;

		case SONORK_FILE_TX_ERROR_FILE_IO:
			sys = tx_eng->TxMode()==SONORK_FILE_TX_MODE_SEND
				?GSS_FILERDERR
				:GSS_FILEWRERR;
			break;

		case SONORK_FILE_TX_ERROR_NET_IO:
			sys = GSS_NETERR;
			break;

		case SONORK_FILE_TX_ERROR_TERMINATED:
			tx_eng = NULL;
			// Be sure to set either <gls> or <sys> here
			// otherwise the code below will test <tx_eng>
			sys = GSS_USRCANCEL;
			break;

		case SONORK_FILE_TX_ERROR_CONFIRM:
		default:
			break;
	}
	if( !no_report )
	{

		if(gls != GLS_NULL)
		{
			str=SonorkApp.LangString(gls);
		}
		else
		if(sys != GSS_NULL)
		{
			str=SonorkApp.SysString(sys);
		}
		else
		{
			if(tx_eng->TxMode()==SONORK_FILE_TX_MODE_RECV)
				gls=GLS_TK_GETFILE;
			else
			if(tx_eng->TxMode()==SONORK_FILE_TX_MODE_SEND)
				gls=GLS_TK_PUTFILE;
			else
				gls=GLS_TK_DELFILE;
			MkTaskErrorString( tmp, gls );
			str=tmp.CStr();
		}
		TSonorkWin::SetStatus(status_hwnd,str,SKIN_HICON_ERROR);
		ErrorBox(str,E->error.pERR);
	}
	Destroy(IDCANCEL);
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkFileTxGui::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch(wParam)
	{
		case POKE_SEND_MESSAGE_RESULT:
			if(taskERR.Result() == SONORK_RESULT_OK)
			{
				user.msg.Clear();
				tx_eng->Continue();
			}
			else
			{
				TaskErrorBox(GLS_TK_SNDMSG,&taskERR);
				Destroy();
			}
		break;


		case SONORK_WIN_POKE_DESTROY:
		{
			Destroy((UINT)lParam);
		}
		break;
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxGui::SendNotificationMessage( const TSonorkId*pTarget,bool sent_by_server )
{
	TSonorkTempBuffer 	msg_text(SONORK_MAX_PATH+64);
	TSonorkMsgHandleEx	handle;
	char size_str[16],*size_ptr;
	bool in_kb;
	DWORD pc_flags;
	TSonorkUserLocus1	locus;


	SetWinUsrFlag( NOTIFICATION_MSG_SENT );
	TRACE_DEBUG("SendNotificationMessage( %u )",sent_by_server);


	in_kb=true;
	size_ptr = DottedValue(size_str
			,tx_eng->FileInfo().attr.orig_size.v[0]
			,&in_kb);

	sprintf(msg_text.CStr()
	, "%s (%s %s)"
	, tx_eng->FileInfo().name.CStr()
	, size_ptr
	, in_kb?"Kb":"bytes"
	);
	user.msg.SetStr( 0 , msg_text.CStr());

	// SONORK_APP_MPF_NO_SEND
	//   Don't send, just store.
	locus.userId.Set(*pTarget);
	locus.sid.Clear();
	pc_flags=sent_by_server
		  ?SONORK_APP_MPF_NO_SERVICE_PARSE
		   |SONORK_APP_MPF_DONT_SEND
		   |SONORK_APP_MPF_NO_TRACKING_NUMBER
		   |SONORK_APP_MPF_SILENT
		  :SONORK_APP_MPF_NO_SERVICE_PARSE;
	user.msg.header.dataSvcInfo.SetInfo(SONORK_SERVICE_ID_NONE
		,SONORK_SERVICE_TYPE_SONORK_FILE
		,0);
	SonorkApp.PrepareMsg(
		  handle
		, &user.msg
		, 0				// sys_flags
		, SONORK_MSG_UF_AUTOMATIC	// usr_flags
		, pc_flags
		, 0				// reply_tracking_no
		, NULL				// sourceService
		);
	if( sent_by_server )
		user.msg.header.trackingNo.Set(tx_eng->SendTrackingNo());

	tx_eng->FileInfo().CODEC_Write( &user.msg.ExtData() );
	SonorkApp.SendMsgLocus(
		  handle
		, this
		, &locus
		, SONORK_INVALID_LINK_ID
		, &user.msg
		);
	sender.Set(locus.userId);
	msg_mark.Set(handle.mark);
	MarkCCacheLine( SONORK_APP_CCF_PROCESSED , true );
	if( sent_by_server )
	{
		msg_task_id=0;
		return;
	}


	if(handle.ERR.Result() != SONORK_RESULT_OK)
	{
		Destroy();
	}
	else
	{
		msg_task_id=handle.taskId;
		TSonorkWin::SetStatus(status_hwnd,GLS_MS_SNDINGMSG,SKIN_HICON_BUSY);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxGui::MarkCCacheLine(DWORD flags,bool save_path)
{
	DWORD*	p_ext_index=NULL;
	DWORD	ext_index;

	if( save_path )
	{
		SONORK_RESULT	 result;
		TSonorkTag	 ext_tag;
		ext_tag.v[SONORK_DB_TAG_FLAGS] = SONORK_EXT_DB_FILE_PATH_FULL;
		ext_tag.v[SONORK_DB_TAG_INDEX] = 0;
		// Save file path to Ext(ra) database
		result = SonorkApp.AddExtData(&ext_index
				,&tx_eng->FilePath()
				,&ext_tag);
		if(result ==SONORK_RESULT_OK)
		{
			// Save index for item
			p_ext_index = &ext_index;
		}
		else
		{
			// Uhh. failed..
			TRACE_DEBUG("AddExtData:FAILED: %s",SONORK_ResultName(result));
		}

	}
	SonorkApp.MarkMsgProcessed(
		 sender
		,msg_mark
		,flags
		,SONORK_APP_CCF_UNREAD
		|SONORK_APP_CCF_DELETED
		|SONORK_APP_CCF_PROCESSED
		,p_ext_index);
}

// ----------------------------------------------------------------------------

bool
 TSonorkFileTxGui::OnAppEvent(UINT event, UINT , void*data)
{
	union {
		void				*ptr;
		TSonorkAppEventMsg		*msg;
		TSonorkAppEventMsgQueryLock	*query;
	}E;
	if( tx_eng != NULL )
	{
		E.ptr = data;
		if( event == SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK )
		{
			if( tx_eng->FileIsRemote() )
			{
				if( E.query->owner   == NULL )
				if( E.query->userId == sender )
				if( msg_mark.Equ( E.query->markPtr ))
				{
					E.query->owner = this;
				}
				return true;
			}
		}
		else
		if( event == SONORK_APP_EVENT_SONORK_MSG_SENT )
		{
			if( E.msg->taskId == msg_task_id )
			{
				msg_task_id=0;
				taskERR.Set( *E.msg->ERR );
				PostPoke(POKE_SEND_MESSAGE_RESULT,0);
				return true;
			}
		}
	}
	return false;
}

// ----------------------------------------------------------------------------

char*
 DottedValue(char* buffer, DWORD value, bool* allow_kb)
{
	char tmp[16],*size_ptr;
	UINT l,d;
	if( allow_kb != NULL )
	{
		if(*allow_kb && value > 65535)
			value>>=10;
		else
			*allow_kb=false;
	}
	ultoa(value, tmp, 10);
	l=strlen(tmp);
	size_ptr = buffer+16;
	*size_ptr=0;d=0;
	while(l--)
	{
		*--size_ptr=tmp[l];
		if(++d==3&&l>0)
		{
			*--size_ptr='.';
			d=0;
		}
	}
	return size_ptr;
}

