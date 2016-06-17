#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop
#include "srktaskwin.h"
#include "srk_uts.h"

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

TSonorkTaskWin::TSonorkTaskWin(TSonorkWin*parent,UINT def_flags,UINT sys_flags)
	:TSonorkWin(parent,def_flags,sys_flags)
{
	current_task.type 	= SONORK_WIN_TASK_NONE;
	current_task.id		=SONORK_INVALID_TASK_ID;
	current_task.flags	=0;
}

void TSonorkTaskWin::OnCleanup()
{
	if( IsTaskPending() )
	{
		CancelCurrentTask(false);
	}
}
bool
 TSonorkTaskWin::MayStartTask(TSonorkError*ERR, SONORK_WIN_TASK_TYPE type)	const
{
	if( IsTaskPending() )
	{
		if(ERR)ERR->SetSys(SONORK_RESULT_SERVICE_BUSY, GSS_APPBUSY, SONORK_MODULE_LINE );
		return false;
	}
	if( !SonorkApp.CxReady() )
	{
		if(ERR)ERR->SetSys(SONORK_RESULT_INVALID_OPERATION, GSS_NOTCXTED,SONORK_MODULE_LINE);
		return false;
	}
	else
	if( type == SONORK_WIN_TASK_UTS )
	{
		if( !SonorkApp.UTS_Enabled() )
		{
			if(ERR)ERR->SetSys( SONORK_RESULT_NOT_AVAILABLE, GSS_NOUTSLNK, SONORK_MODULE_LINE );
			return false;
		}

	}
	return true;

}

// ----------------------------------------------------------------------------
// NOTES ON THE BEHAVIOUR OF OnTaskStart() AND OnTaskEnd() WHEN
// StartUtsTask() IS INVOKED:
// ----------------------------------------------------------------------------

// Uts()->SendPacket will invoke the supplied TSonorkTaskWin::SonorkUtsHandler
// callback if the packet is sent immediately, but it won't if it is queued.

// TSonorkTaskWin::SonorkUtsHandler will invoke OnTaskEnd() and clear
// <current_task.type> (And therefore set IsTaskPending() to false)
// so that OnTaskStart() is not invoked after a call to OnTaskEnd().

// In other words:
// OnTaskStart() is invoked if, and only if, the packet is queued to
//  indicate that the caller will receive an OnTaskEnd()
//  AFTER the call to StartUtsTask() returns.
// OnTaskEnd() will be invoked always if there are no errors and it
//  may be called BEFORE StartUtsTask() returns.

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkTaskWin::StartUtsTask(TSonorkError&				ERR
								, SONORK_UTS_LINK_ID 		uts_link_id
								, DWORD 					cmd
								, BYTE 						version
								, const TSonorkCodecAtom*   A
								, DWORD						task_flags
								, GLS_INDEX 				gls_task
								, const SONORK_DWORD2*		pTag)
{

	TSonorkTag tag;
	TRACE_DEBUG("StartUtsTask LINK:%08x CMD:%u",uts_link_id,cmd);
	if(MayStartTask(&ERR,SONORK_WIN_TASK_UTS))
	{
		current_task.flags	= task_flags;
		current_task.type	= SONORK_WIN_TASK_UTS;
		current_task.id		= uts_link_id;

		SonorkApp.UTS_Server()->SendPacket(ERR
				, uts_link_id
				, cmd
				, version
				, A
				, this
				, SonorkUtsHandler
				, pTag);
	}

	if( ERR.Result() == SONORK_RESULT_OK )
	{
		// If IsTaskPending() returns false, it means that
		// SendPacket() has already invoked our TSonorkTaskWin::SonorkUtsHandler
		// callback to indicate that the packet was sent immediately
		// so we don't invoke OnTaskStart()

		assert( IsTaskPending() != false );
		if(pTag)tag.Set(*pTag);
		OnTaskStart(SONORK_WIN_TASK_UTS , tag );
	}
	else
	{
		TRACE_DEBUG("StartUtsTask failed ERR=%s/%s"
			,ERR.ResultName()
			,ERR.Text().CStr());
		if( !(task_flags & SONORK_TASKWIN_F_NO_ERROR_BOX) )
			TaskErrorBox(gls_task,&ERR);
	}
	return ERR.Result();
}

void SONORK_CALLBACK
	TSonorkTaskWin::SonorkUtsHandler(void *param, const SONORK_DWORD2& tag, const TSonorkUTSLink*LINK, const TSonorkError*pERR)
{
	TSonorkTaskWin *_this=(TSonorkTaskWin*)param;
	TRACE_DEBUG("UtsEnd LINK=%08x(%08x) TType=%u(%u) ERR=%s"
			, LINK->Id()
			, _this->current_task.id
			, SONORK_WIN_TASK_UTS
			, _this->CurrentTaskType()
			, pERR->ResultName());

	if( LINK->Id() != _this->current_task.id
	|| _this->TestWinSysFlag(SONORK_WIN_SF_DESTROYING)
	|| _this->CurrentTaskType() != SONORK_WIN_TASK_UTS )
		return;
	_this->current_task.type 	= SONORK_WIN_TASK_NONE;
	_this->current_task.id 		= SONORK_INVALID_TASK_ID;
	_this->current_task.flags	= 0;
	_this->OnTaskEnd(SONORK_WIN_TASK_UTS
		, tag
		, pERR
		);
}

// ----------------------------------------------------------------------------
// NOTES ON THE BEHAVIOUR OF OnTaskStart() AND OnTaskEnd() WHEN
// StartSonorkTask() IS INVOKED:
// ----------------------------------------------------------------------------

// StartSonorkTask() always invokes both OnTaskStart() and OnTaskEnd()
// if no errors occurr.

// OnTaskStart() will always be invoked BEFORE StartSonorkTask() returns
// OnTaskEnd() will always be invoked AFTER StartSonorkTask() returns

// None of them are invoked if the function fails

// ----------------------------------------------------------------------------

SONORK_RESULT
	TSonorkTaskWin::StartSonorkTask(TSonorkError& ERR
	, TSonorkDataPacket*P
	, UINT 			P_size
	, DWORD			task_flags
	, GLS_INDEX		gls_task
	, const SONORK_DWORD2*	pTag)
{

	TSonorkTag tag;
	if(MayStartTask(&ERR))
	{
		SonorkApp.StartSonorkRequest(ERR,P,P_size,SonorkClientHandler
			,this
			,pTag
			,&current_task.id);
	}
	if( ERR.Result() == SONORK_RESULT_OK )
	{
		current_task.flags	= task_flags;
		current_task.type	= SONORK_WIN_TASK_SONORK;
		assert( IsTaskPending() != false );
		if(pTag)tag.Set(*pTag);
		OnTaskStart(SONORK_WIN_TASK_SONORK , tag );
	}
	else
	{
		if( !(task_flags & SONORK_TASKWIN_F_NO_ERROR_BOX) )
		{
			TaskErrorBox(gls_task,&ERR);
		}
	}
	return ERR.Result();
}


// CancelCurrentTask is equivalent to EndCurrentTask with ERR=USER_TERMINATION
void
 TSonorkTaskWin::EndCurrentTask(TSonorkError&ERR, bool invoke_handler)
{
	DWORD	 				task_id;
	TSonorkTag	   	  		task_tag;
	SONORK_WIN_TASK_TYPE	task_type;
	SONORK_FUNCTION	  		function;

	task_id 			= current_task.id;
	task_type 			= current_task.type;
	current_task.id		= SONORK_INVALID_TASK_ID;
	current_task.flags	= 0;
	if( task_type == SONORK_WIN_TASK_SONORK )
	{
		SonorkApp.CancelSonorkRequest( task_id , &function, &task_tag);
	}
	else
	if( task_type == SONORK_WIN_TASK_UTS )
	{
		if(SonorkApp.UTS_Server() != NULL)
			SonorkApp.UTS_Server()->CancelCallback(task_id, this , &task_tag);

	}
	else
		return;

	if( invoke_handler )
	{
		// Don't copy if they are the same
		OnTaskEnd( task_type , task_tag , &ERR);
	}
}
void
 TSonorkTaskWin::CancelCurrentTask(bool invoke_handler)
{
	TSonorkError ERR;
	ERR.SetSys(SONORK_RESULT_USER_TERMINATION
		,GSS_USRCANCEL
		,SONORK_MODULE_LINE);
	EndCurrentTask(ERR,invoke_handler);
};

void SONORK_CALLBACK
	TSonorkTaskWin::SonorkClientHandler(void*param,TSonorkClientRequestEvent*E)
{
	TSonorkTaskWin*_this = (TSonorkTaskWin*)param;

	if( E->RequestTaskId() != _this->CurrentTaskId()
	|| _this->TestWinSysFlag(SONORK_WIN_SF_DESTROYING)
	|| _this->CurrentTaskType() != SONORK_WIN_TASK_SONORK )
		return;

	switch( E->EventType() )
	{
		case SONORK_CLIENT_REQUEST_EVENT_PACKET:
			_this->OnSonorkTaskData(
							E->RequestTag()
						,   E->Packet()
						,	E->PacketSize() );
			break;

		case SONORK_CLIENT_REQUEST_EVENT_END:
			_this->current_task.type 	= SONORK_WIN_TASK_NONE;
			_this->current_task.id 		= SONORK_INVALID_TASK_ID;
			_this->current_task.flags	= 0;
			_this->OnTaskEnd(SONORK_WIN_TASK_SONORK
				, E->RequestTag()
				, E->ErrorInfo());
			break;
	}
}

/*
SONORK_RESULT	TSonorkTaskWin::TaskSendMsgToUser(TSonorkError&ERR
	, TSonorkExtUserData*	UD
	, const TSonorkMsg&		msg
	, DWORD					task_flags
	, const SONORK_DWORD2*	tag)
{
	TSonorkUserLocus1 locus;
	UD->GetLocus1(&locus);
	return TaskSendMsgToLocus(ERR,locus,msg,task_flags,tag,UD->UtsLinkId());
}

SONORK_RESULT	TSonorkTaskWin::TaskSendMsgToLocus(
	  TSonorkError&				ERR
	, const TSonorkUserLocus1&	locus
	, const TSonorkMsg&			msg
	, DWORD						task_flags
	, const SONORK_DWORD2*		tag
	, SONORK_UTS_LINK_ID 		guts_link_id)
{
	UINT			A_size,P_size;
	bool			send_by_engine=true;
	A_size=::CODEC_Size(&locus) + ::CODEC_Size(&msg) + 8;

	TRACE_DEBUG("SendMsg(%u.%u) F=%x LINK=%08x"
			, locus.gu_id.v[0]
			, locus.gu_id.v[1]
			, task_flags
			, guts_link_id);
	if( !(task_flags & SONORK_TASKWIN_F_NO_UTS) )
	{
		if( guts_link_id!= SONORK_INVALID_TASK_ID )
		{
			StartUtsTask(ERR
				, guts_link_id
				, SONORK_UTS_CMD_MSG
				, 0
				, &msg
				, task_flags|SONORK_TASKWIN_F_NO_ERROR_BOX
				, GLS_TK_SNDMSG
				, tag);
			if( ERR.Result() == SONORK_RESULT_OK )
				return ERR.Result();
		}
		if(task_flags & SONORK_TASKWIN_F_UTS_ONLY)
		{
			ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE
				, GSS_NOGUTSLNK
				, SONORK_MODULE_LINE);
			send_by_engine=false;
		}
	}

	if( send_by_engine )
	{
		TSonorkDataPacket*	P;
		P=SONORK_AllocDataPacket( A_size );
		P_size = P->E_PutMsg_R(A_size,&locus,&msg);
		StartSonorkTask(ERR
				,P
				,P_size
				,task_flags|SONORK_TASKWIN_F_NO_ERROR_BOX
				,GLS_TK_SNDMSG
				,tag);
		SONORK_FreeDataPacket( P );
	}
	TRACE_DEBUG("SendMsg(%u.%u) = %s"
			, locus.gu_id.v[0]
			, locus.gu_id.v[1]
			, ERR.ResultName());
	if( ERR.Result() != SONORK_RESULT_OK )
	{
		if( !(task_flags & SONORK_TASKWIN_F_NO_ERROR_BOX) )
		{
			TaskErrorBox(GLS_TK_SNDMSG,&ERR);
		}
	}
	return ERR.Result();
}
*/
