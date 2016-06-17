#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkchatwin.h"
#include "srktrackerwin.h"
#include "srk_file_io.h"
// ----------------------------------------------------------------------------

#define SONORK_APP_MAX_SERVICE_ENTRIES	256

enum WM_SONORK_SERVICE_WPARAM
{
   WM_SONORK_SERVICE_WPARAM_CLEAR_ENTRY         = 1
,  WM_SONORK_SERVICE_WPARAM_CLEAR_QUERY
,  WM_SONORK_SERVICE_WPARAM_CLEAR_CIRCUIT
,  WM_SONORK_SERVICE_WPARAM_ACCEPT_CIRCUIT
};



// ----------------------------------------------------------------------------

struct SONORK_APP_SERVICE
{
	UINT				cookie;
	TSonorkServiceEntryQueue	entries;
	TSonorkServiceQueryQueue	queries;
	TSonorkServiceCircuitQueue	circuits;
	TSonorkServiceCircuitQueue	pending_circuits;
	TSonorkAppServiceEvent		event;
	DWORD				active_type;
	DWORD				active_task_type;
	TSonorkServiceEntry*		active_entry;

}service;
// Global: Pointer to the entry currently in the callback proc
// used by the IPC to determine the instance id of the entry being invoked.
TSonorkServiceEntry*		Service_DispatchEvent_Entry;


// ----------------------------------------------------------------------------
void
 TSonorkWin32App::Service_BeginEnum(TSonorkListIterator& I)
{
	service.entries.BeginEnum(I);
}

const TSonorkServiceEntry*
 TSonorkWin32App::Service_EnumNext(TSonorkListIterator& I, TSonorkShortString*name)
{
	TSonorkServiceEntry*E;
	TSonorkAppServiceEvent	event;
	for(;;)
	{
		if((E=service.entries.EnumNext(I))==NULL)
			return NULL;
		if(!(E->flags&SONORK_APP_SERVICE_EF_REGISTERED))
			continue;
		if( name != NULL )
		{
			event.get_name.str = name;
			if(!Service_DispatchEvent(
				  E
				, SONORK_APP_SERVICE_EVENT_GET_NAME
				, NULL
				, &event))
			{
				name->Clear();
			}
		}
		return E;
	}
}

void
 TSonorkWin32App::Service_EndEnum(TSonorkListIterator& I)
{
	service.entries.EndEnum(I);
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessCtrlMsg(
	  const TSonorkUserLocus1* 	sender_locus
	, DWORD				sender_uts_link_id
	, const TSonorkCtrlMsg*		msg
	, const void*			data
	, DWORD				size)
{
	service.event.cur_msg.sender.TSonorkUserLocus1::Set( *sender_locus );
	service.event.cur_msg.sender.utsLinkId = sender_uts_link_id;
	if( msg->IsOldCtrlMsg() )
	{
		OnOldCtrlMsg(service.event.cur_msg.sender,msg,data,size);
		return;
	}


	service.event.cur_msg.sender.instance	= msg->source_instance;
	service.event.cur_msg.msg	= msg;
	service.event.cur_msg.data 	= data;
	service.event.cur_msg.size	= size;

	TRACE_DEBUG("msg!%u.%u,%x,%x C=%x:%x F=%x:%x T=%x Z=%u"
		, service.event.cur_msg.sender.userId.v[0]
		, service.event.cur_msg.sender.userId.v[1]
		, service.event.cur_msg.sender.utsLinkId
		, msg->source_instance
		, msg->Cmd()
		, msg->UserParam()
		, msg->sysFlags
		, service.event.cur_msg.msg->body.n.systemId
		, msg->target_instance
		, size);

	if( msg->IsSystemCmd() )
	{
		Service_ProcessSystemMsg();
	}
	else
	switch(msg->Type())
	{
		case SONORK_CTRLMSG_SF_TYPE_NONE:
			Service_ProcessDataMsg();
			break;

		case SONORK_CTRLMSG_SF_TYPE_QUERY:
			Service_ProcessQueryMsg();
			break;

		case SONORK_CTRLMSG_SF_TYPE_CIRCUIT:
			Service_ProcessCircuitMsg();
			break;
	}
	service.event.cur_msg.msg=NULL;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessSystemMsg()
{
	union
	{
		void		*		ptr;
		TSonorkServiceEntry*		E;
		TSonorkServiceQueryEntry*	Q;
		TSonorkServiceCircuitEntry*	C;
	}D;

	switch( service.event.cur_msg.msg->Cmd() )
	{

	case SONORK_CTRLMSG_CMD_PROBE:

		// A PROBE message can only be of flow REQ
		if( service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_REQ )
		{
			TRACE_DEBUG("  svc_msg:probe:FLOW!=REQ");
			return;
		}

		switch( service.event.cur_msg.msg->Type() )
		{

		case SONORK_CTRLMSG_SF_TYPE_NONE:
			D.E=Service_GetEntry(
				  service.event.cur_msg.msg->ServiceId()
				, service.event.cur_msg.msg->target_instance
				, true);
			break;

		case SONORK_CTRLMSG_SF_TYPE_QUERY:
			D.Q=Service_GetQuery(
				  service.event.cur_msg.msg->target_instance
				, service.event.cur_msg.msg->QueryId());
			break;

		case SONORK_CTRLMSG_SF_TYPE_CIRCUIT:
			D.C=Service_GetCircuit(
				  service.event.cur_msg.msg->target_instance
				, service.event.cur_msg.msg->CircuitId());
			break;

		default:
			D.ptr=NULL;
			break;
		}

		if(D.ptr == NULL )
		{
			// if NULL, we let the other side
			// know the PROBE failed.
			Service_ReplyCurMsg(
				  SONORK_CTRLMSG_CMD_CLOSE
				, service.event.cur_msg.msg->target_instance
				, service.event.cur_msg.msg->Type()
				, SONORK_RESULT_INVALID_HANDLE);

		}
		// if not NULL, the entry is valid and we don't sent anything
	break;

// ----------

	case SONORK_CTRLMSG_CMD_OPEN:
		TRACE_DEBUG("  svc_msg:open:%x:S=%x T=%x.%x FLOW=%x"
			, service.event.cur_msg.msg->CircuitId()
			, service.event.cur_msg.msg->source_instance
			, service.event.cur_msg.msg->ServiceId()
			, service.event.cur_msg.msg->target_instance
			, service.event.cur_msg.msg->Flow()
			);

		// An OPEN message can only be of type CIRCUIT
		if( service.event.cur_msg.msg->Type() !=  SONORK_CTRLMSG_SF_TYPE_CIRCUIT )
		{
			TRACE_DEBUG("  svc_msg:open:TYPE!=CIRCUIT");
			return;
		}
		Service_ProcessSystemMsg_OpenCircuit();
	break;

// ----------

	case SONORK_CTRLMSG_CMD_CLOSE:
		TRACE_DEBUG("  svc_msg:close:%x:S=%x T=%x TY=%x R=%s"
			, service.event.cur_msg.msg->QueryId()
			, service.event.cur_msg.msg->source_instance
			, service.event.cur_msg.msg->target_instance
			, service.event.cur_msg.msg->Type()
			, SONORK_ResultName((SONORK_RESULT)service.event.cur_msg.msg->UserParam())
			);
		// Flow does not matter for a CMD_CLOSE
		switch( service.event.cur_msg.msg->Type() )
		{

		case SONORK_CTRLMSG_SF_TYPE_QUERY:
			D.Q=Service_GetQuery(
				  service.event.cur_msg.msg->target_instance
				, service.event.cur_msg.msg->QueryId());
			if(D.Q!=NULL)
				Service_CondemnQuery(D.Q
				, (SONORK_RESULT)service.event.cur_msg.msg->UserParam()
				, true
				, true);
			else
				TRACE_DEBUG("  svc_msg:close:query:NOT FOUND");
			break;

		case SONORK_CTRLMSG_SF_TYPE_CIRCUIT:
			D.C=Service_GetCircuit(
				  service.event.cur_msg.msg->target_instance
				, service.event.cur_msg.msg->CircuitId());
			if(D.C!=NULL)
			{
				D.C->flags|=SONORK_APP_SERVICE_CF_REMOTE_DEAD;
				Service_CondemnCircuit(D.C
				, (SONORK_RESULT)service.event.cur_msg.msg->UserParam()
				, true
				, true);
			}
			else
				TRACE_DEBUG("  svc_msg:close:circuit:NOT FOUND");
			break;

		default:
			return;

		}
	break;
// ----------

	case SONORK_CTRLMSG_CMD_UTS_LINK:
		// Type does not matter for a UTS_LINK
		TRACE_DEBUG("  svc_msg:uts:S=%x T=%x.%x FLOW=%x"
			, service.event.cur_msg.msg->source_instance
			, service.event.cur_msg.msg->ServiceId()
			, service.event.cur_msg.msg->target_instance
			, service.event.cur_msg.msg->Flow()
			);
		Service_ProcessSystemMsg_UtsLink();
	break;

// ----------



	}
}

// ----------------------------------------------------------------------------
void
 TSonorkWin32App::Service_ProcessSystemMsg_UtsLink()
{
	TSonorkUserLocus3 *remote_locus;
	TSonorkUserLocus3  local_locus;
	TSonorkError	ERR;
	char tmp[48];

	if( service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_REQ
	&&  service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_AKN)
		return;

	// Sender did not send proper packet
	if(service.event.cur_msg.size<sizeof(TSonorkUserLocus3) )
		return;

	// We don't have the UTS server enabled
	if(!UTS_Server() )
		return;

	// No need for UTS_LINK: We're already connected
	if( service.event.cur_msg.sender.utsLinkId != SONORK_INVALID_LINK_ID )
		return;

	remote_locus = (TSonorkUserLocus3*)service.event.cur_msg.data;
	remote_locus->physAddr.GetStr(tmp);
	TRACE_DEBUG("  svc_msg:uts:link:%u.%u %x.%x '%s'"
		, remote_locus->userId.v[0]
		, remote_locus->userId.v[1]
		, remote_locus->sid.v[0]
		, remote_locus->sid.v[0]
		, tmp);

	UTS_ConnectByLocus(remote_locus
		,&remote_locus->physAddr
		,&ERR);
	TRACE_DEBUG("  svc_msg:uts:Connect='%s' %s"
		, ERR.ResultName()
		, ERR.Text().CStr());

	if( service.event.cur_msg.msg->Flow() ==  SONORK_CTRLMSG_SF_FLOW_REQ )
	{
		// Ok, now send back OUR address

		// Ask the other side to connect to US
		TSonorkCtrlMsg	uts_msg;

		ProfileUser().GetLocus3(&local_locus);
		local_locus.physAddr.GetStr(tmp);
		TRACE_DEBUG("  svc_msg:uts:akn:%u.%u %x.%x '%s'"
			, local_locus.userId.v[0]
			, local_locus.userId.v[1]
			, local_locus.sid.v[0]
			, local_locus.sid.v[0]
			, tmp);
		uts_msg.LoadNewCmd(
			 SONORK_CTRLMSG_CMD_UTS_LINK
			,0
			,0
			,SONORK_CTRLMSG_SF_FLOW_AKN
			|SONORK_CTRLMSG_SF_TYPE_NONE
			);
		uts_msg.source_instance =
		uts_msg.target_instance = 0;
		SendCtrlMsg(&service.event.cur_msg.sender
			, &uts_msg
			, &local_locus
			, sizeof(local_locus)
			, NULL);
	}

}
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessSystemMsg_OpenCircuit()
{
	TSonorkServiceEntry* 	handler_entry;
	TSonorkServiceEntry* 	owner_entry;
	DWORD			handler_instance;
	TSonorkExtUserData*	UD;
	TSonorkUserLocus3	local_locus;

	TSonorkServiceCircuitEntry* circuit;
	SONORK_APP_SERVICE_CIRCUIT_REQ_RESULT
				acc_result;
	SONORK_RESULT		msg_result;
	SONORK_DWORD2		circuit_tag;
	TSonorkAppServiceEvent	event;

	if( service.event.cur_msg.sender.instance == 0 )
	{
		TRACE_DEBUG("  svc_msg:open:sender=0: IGNORE");
		return;
	}

	if( service.event.cur_msg.msg->Flow() ==  SONORK_CTRLMSG_SF_FLOW_REQ )
	{
		TRACE_DEBUG("  svc_msg:open:req:%x: S=%x T=%x:%x"
		, service.event.cur_msg.msg->CircuitId()
		, service.event.cur_msg.sender.instance
		, service.event.cur_msg.msg->ServiceId()
		, service.event.cur_msg.msg->target_instance
		);

		handler_entry=
			Service_GetEntry(service.event.cur_msg.msg->ServiceId()
			, service.event.cur_msg.msg->target_instance
			, true);
		if( handler_entry == NULL )
		{
			TRACE_DEBUG("  svc_msg:open:req:NOT FOUND");
			msg_result=SONORK_RESULT_INVALID_HANDLE;
			goto send_reply;
		}
		handler_instance = handler_entry->sii.ServiceInstance();
		circuit_tag.Clear();
		service.event.circuit_req.accept_timeout = SONORK_MSECS_FOR_CIRCUIT_ACCEPTANCE;
		service.event.circuit_req.accept_instance= handler_instance;
		acc_result = (SONORK_APP_SERVICE_CIRCUIT_REQ_RESULT)
				Service_DispatchEvent(
				  handler_entry
				, SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ
				, &circuit_tag
				, &service.event
				);
		if( acc_result!=SONORK_APP_SERVICE_CIRCUIT_ACCEPT_PENDING
		&&  acc_result!=SONORK_APP_SERVICE_CIRCUIT_ACCEPTED)
		{
			TRACE_DEBUG("  svc_msg:open:req:NOT_ACCEPTED");
			msg_result=SONORK_RESULT_NOT_ACCEPTED;
			goto send_reply;
		}
		if( handler_instance != service.event.circuit_req.accept_instance )
		{
			TRACE_DEBUG("  svc_msg:open:req:owner %x->%x"
				, handler_instance
				, service.event.circuit_req.accept_instance);
			owner_entry=
				Service_GetEntry(SONORK_SERVICE_ID_NONE
					, service.event.circuit_req.accept_instance
					, true);

			if( owner_entry==NULL )
			{
				TRACE_DEBUG("  svc_msg:open:req:owner NOT FOUND");
				msg_result=SONORK_RESULT_INTERNAL_ERROR;
				goto send_reply;
			}
		}
		else
			owner_entry = handler_entry;

		SONORK_MEM_NEW(circuit=new TSonorkServiceCircuitEntry);
		circuit->entry	=owner_entry;
		circuit->handle.TSonorkServiceHandle::Set( service.event.cur_msg.sender );
		circuit->handle.systemId  = service.event.cur_msg.msg->CircuitId();
		circuit->handle.serviceId = service.event.cur_msg.msg->ServiceId();
		circuit->flags	=SONORK_APP_SERVICE_CF_ACTIVE
				|SONORK_APP_SERVICE_CF_OPEN_BY_REMOTE;
		circuit->tag.Set(circuit_tag);

		service.circuits.Add( circuit );

		TRACE_DEBUG("  svc_msg:open:req:%x:S=%x T=%x:%x:%s (%u)"
		, circuit->handle.CircuitId()
		, service.event.cur_msg.sender.instance
		, circuit->handle.serviceId
		, circuit->handle.instance
		, acc_result==SONORK_APP_SERVICE_CIRCUIT_ACCEPT_PENDING
			?"PENDING"
			:"ACCEPTED"
		, service.event.circuit_req.accept_timeout
		);

		if( acc_result==SONORK_APP_SERVICE_CIRCUIT_ACCEPT_PENDING )
		{
			circuit->max_msecs	= service.event.circuit_req.accept_timeout;
			circuit->cur_msecs	= 0;

			circuit->flags|=SONORK_APP_SERVICE_CF_PENDING;
			service.pending_circuits.Add( circuit );

			// Don't send the reply: The caller must
			// call AcceptCircuit() to activate it.
		}
		else
		{
			Service_ReplyCurMsg(
				  SONORK_CTRLMSG_CMD_OPEN
				, owner_entry->sii.ServiceInstance()
				, SONORK_CTRLMSG_SF_TYPE_CIRCUIT
				, SONORK_RESULT_OK);
			circuit->flags|=SONORK_APP_SERVICE_CF_ACCEPTED;
			event.circuit_open.handle = &circuit->handle;
			Service_DispatchEvent(
					  owner_entry
					, SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN
					, &circuit->tag
					, &event
					);
		}
		return;

	}

	if( service.event.cur_msg.msg->Flow() ==  SONORK_CTRLMSG_SF_FLOW_AKN )
	{
		msg_result=SONORK_RESULT_INVALID_HANDLE;
		TRACE_DEBUG("  svc_msg:open:akn:%x:S=%x T=%x:%x"
		, service.event.cur_msg.msg->CircuitId()
		, service.event.cur_msg.sender.instance
		, service.event.cur_msg.msg->ServiceId()
		, service.event.cur_msg.msg->target_instance
		);
		circuit = Service_GetCircuit(service.event.cur_msg.msg->target_instance
				,service.event.cur_msg.msg->CircuitId() );
		if( circuit == NULL )
		{
			TRACE_DEBUG("  svc_msg:open:akn:NOT_FOUND");
			goto send_reply;
		}
		if(!(circuit->flags & SONORK_APP_SERVICE_CF_PENDING ))
		{
			TRACE_DEBUG("  svc_msg:open:akn:NOT PENDING");
			goto send_reply;
		}
		if( circuit->handle.instance != service.event.cur_msg.sender.instance)
		{
			TRACE_DEBUG("  svc_msg:open:akn:INSTANCE(%x->%x)"
				, circuit->handle.instance
				, service.event.cur_msg.sender.instance);
		}
		circuit->handle.instance = service.event.cur_msg.sender.instance;
		circuit->flags&=~SONORK_APP_SERVICE_CF_PENDING;
		circuit->flags|=SONORK_APP_SERVICE_CF_ACCEPTED;
		assert( service.pending_circuits.Remove(circuit) );

		if( UTS_Server() )
		{
			UD = UserList().Get(circuit->handle.userId);
			if( UD != NULL )
			{
				UTS_ConnectByUserData(UD,true,NULL);
				circuit->handle.utsLinkId=UD->UtsLinkId();
			}
			else
			{
				circuit->handle.utsLinkId =
					win32.uts_server->FindLink(
					  circuit->handle.userId
					, SONORK_SERVICE_ID_NONE
					, 0
					, SONORK_SERVICE_ID_SONORK
					, 0
					);
			}
			TRACE_DEBUG(" svc_msg:open:akn:UD=%x, LNK=%x)",UD,circuit->handle.utsLinkId);
			if( circuit->handle.utsLinkId == SONORK_INVALID_LINK_ID )
			{
				// Ask the other side to connect to US
				TSonorkCtrlMsg	uts_msg;
				char	tmp[64];
				ProfileUser().GetLocus3(&local_locus);
				uts_msg.LoadNewCmd(
					 SONORK_CTRLMSG_CMD_UTS_LINK
					,0
					,0
					,SONORK_CTRLMSG_SF_FLOW_REQ
					|SONORK_CTRLMSG_SF_TYPE_NONE
					);
				uts_msg.source_instance =
				uts_msg.target_instance = 0;

				local_locus.physAddr.GetStr(tmp);
				TRACE_DEBUG("  svc_msg:uts:req:%u.%u %x.%x '%s'"
					, local_locus.userId.v[0]
					, local_locus.userId.v[1]
					, local_locus.sid.v[0]
					, local_locus.sid.v[0]
					, tmp);
				SendCtrlMsg(&circuit->handle
					, &uts_msg
					, &local_locus
					, sizeof(local_locus)
					, NULL);

			}
		}

		event.circuit_open.handle = &circuit->handle;
		Service_DispatchEvent(
				  circuit->entry
				, SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN
				, &circuit->tag
				, &event
				);
		return;
	}
	else
		return ; // CMD_OPEN may only be FLOW_REQ or FLOW_AKN

send_reply:
	Service_ReplyCurMsg(
		  SONORK_CTRLMSG_CMD_CLOSE
		, service.event.cur_msg.msg->target_instance
		, SONORK_CTRLMSG_SF_TYPE_CIRCUIT
		, msg_result);
}


// ----------------------------------------------------------------------------
// Service_AcceptCircuit
// Must load circuit->systemId with the desired
// circuit_id before calling.
// <prev_source_instance> is the instance of the
// service that received the original CIRCUIT_REQ event
SONORK_RESULT
 TSonorkWin32App::Service_AcceptCircuit(
			  DWORD			old_source_instance
			, DWORD			new_source_instance
			, TSonorkServiceCircuit*hCircuit)
{
	TSonorkServiceEntry*		owner_entry;
	TSonorkServiceCircuitEntry*	circuit_entry;
	TSonorkCtrlMsg			msg;
	SONORK_RESULT			result;
	circuit_entry = Service_GetCircuit(old_source_instance
				,hCircuit->CircuitId() );
	if( circuit_entry == NULL )
		return SONORK_RESULT_INVALID_HANDLE;

	if( !(circuit_entry->flags & SONORK_APP_SERVICE_CF_PENDING )
	||  !(circuit_entry->flags & SONORK_APP_SERVICE_CF_OPEN_BY_REMOTE )
	||   (circuit_entry->flags & SONORK_APP_SERVICE_CF_ACCEPTED) )
		return SONORK_RESULT_INVALID_OPERATION;


	if( old_source_instance != new_source_instance )
	{
		owner_entry = Service_GetEntry(
					SONORK_SERVICE_ID_NONE
					,new_source_instance
					,true);
		if( owner_entry == NULL )
			return SONORK_RESULT_INVALID_PARAMETER;
		circuit_entry->entry = owner_entry;
	}
	else
		owner_entry = circuit_entry->entry;
	hCircuit->Set( circuit_entry->handle );

	msg.LoadNewCmd(
		SONORK_CTRLMSG_CMD_OPEN
		,0
		,0
		,SONORK_CTRLMSG_SF_FLOW_AKN
		|SONORK_CTRLMSG_SF_TYPE_CIRCUIT
		);
	msg.body.n.systemId = hCircuit->CircuitId();
	result = Service_SendCtrlMsg(
			  owner_entry
			, hCircuit
			, &msg
			, NULL
			, 0
			, SONORK_CTRLMSG_SF_FLOW_AKN
			 |SONORK_CTRLMSG_SF_TYPE_CIRCUIT);
	if( result == SONORK_RESULT_OK )
	{
		circuit_entry->flags&=~SONORK_APP_SERVICE_CF_PENDING;
		::PostMessage(win32.work_win
			, WM_SONORK_SERVICE
			, WM_SONORK_SERVICE_WPARAM_ACCEPT_CIRCUIT
			, (LPARAM)circuit_entry);

	}
	return result;
}


// ----------------------------------------------------------------------------
// Service_UpdateCircuitHandle
// Must load <circuit->system_id> with the desired
// circuit_id before calling; all other members are ignored.

SONORK_RESULT
 TSonorkWin32App::Service_GetCircuitHandle(DWORD source_instance
			, TSonorkServiceCircuit* hCircuit)
{
	TSonorkServiceCircuitEntry* circuit_entry;
	circuit_entry = Service_GetCircuit(source_instance
				,hCircuit->CircuitId() );
	if( circuit_entry == NULL )
		return SONORK_RESULT_INVALID_HANDLE;
	hCircuit->Set( circuit_entry->handle );
	return SONORK_RESULT_OK;

}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_CloseCircuit(DWORD	source_instance
			, DWORD			circuit_id
			, SONORK_RESULT		close_result)
{
	TSonorkServiceCircuitEntry* circuit_entry;
	circuit_entry = Service_GetCircuit(source_instance
				,circuit_id) ;
	if( circuit_entry == NULL )
		return SONORK_RESULT_INVALID_HANDLE;
	Service_CondemnCircuit( circuit_entry, close_result, true, true);
	return SONORK_RESULT_OK;

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessDataMsg()
{
	TSonorkListIterator		I;
	TSonorkServiceEntry*		E;
	DWORD				target_instance;

	if( service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_NONE )
	{
		TRACE_DEBUG("  svc_msg:poke:FLOW!=NONE: IGNORE");
		return;
	}

	target_instance=service.event.cur_msg.msg->target_instance;
	if( service.event.cur_msg.msg->TargetIsBroadcast() )
	{
		service.entries.BeginEnum(I);
		while( (E=service.entries.EnumNext(I)) != NULL )
		{
			if( !(E->flags&SONORK_APP_SERVICE_EF_REGISTERED) )
				continue;

			if( !(target_instance & ((DWORD)E->sii.ServiceInstance()) ) )
				continue;

			if(service.event.cur_msg.msg->ServiceId() != SONORK_SERVICE_ID_NONE )
			if(service.event.cur_msg.msg->ServiceId() != E->sii.ServiceId() )
				continue;

			Service_DispatchEvent(
				  E
				, SONORK_APP_SERVICE_EVENT_POKE_DATA
				, NULL
				, &service.event
				);
		}
		service.entries.EndEnum(I);
	}
	else
	{
		E=Service_GetEntry(SONORK_SERVICE_ID_NONE
			, target_instance
			, true );
		if( E != NULL )
		{
			Service_DispatchEvent(
				  E
				, SONORK_APP_SERVICE_EVENT_POKE_DATA
				, NULL
				, &service.event
				);
		}
		else
		{
			// Entry is invalid: Report to the sender
			TRACE_DEBUG("  svc_msg:poke:ENTRY %x not found",target_instance);
			Service_ReplyCurMsg(SONORK_CTRLMSG_CMD_CLOSE
				,target_instance
				,SONORK_CTRLMSG_SF_TYPE_NONE
				,SONORK_RESULT_INVALID_HANDLE);
		}
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessCircuitMsg()
{
	DWORD	target_instance;
	TSonorkServiceCircuitEntry* circuit;
	if( service.event.cur_msg.sender.instance == 0 )
	{
		TRACE_DEBUG("  svc_msg:circuit:SENDER_INSTANCE=0: IGNORE");
		return;
	}
	if( service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_NONE )
	{
		TRACE_DEBUG("  svc_msg:circuit: FLOW!=NONE: IGNORE");
		return;
	}
	target_instance=service.event.cur_msg.msg->target_instance;
	TRACE_DEBUG("  svc_msg:circuit:%x:data:S=%x T=%x"
			, service.event.cur_msg.msg->CircuitId()
			, service.event.cur_msg.sender.instance
			, target_instance
			);
	circuit = Service_GetCircuit(target_instance
			,service.event.cur_msg.msg->CircuitId() );
	if( circuit == NULL )
	{
		TRACE_DEBUG("  svc_msg:circuit:%x:data:not found:REPLY W/CLOSE"
			, service.event.cur_msg.msg->CircuitId());
		Service_ReplyCurMsg(
		  SONORK_CTRLMSG_CMD_CLOSE
		, target_instance
		, SONORK_CTRLMSG_SF_TYPE_CIRCUIT
		, SONORK_RESULT_INVALID_HANDLE);

	}
	else
	{
		service.active_task_type=
		service.active_type	=SONORK_CTRLMSG_SF_TYPE_CIRCUIT
					|SONORK_CTRLMSG_SF_FLOW_NONE;

		service.active_entry=circuit->entry;

		Service_DispatchEvent(circuit->entry
				, SONORK_APP_SERVICE_EVENT_CIRCUIT_DATA
				, &circuit->tag
				, &service.event
				);
		service.active_type=SONORK_CTRLMSG_SF_TYPE_NONE;
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ProcessQueryMsg()
{
	TSonorkListIterator		I;
	TSonorkServiceEntry*		E;
	TSonorkServiceQueryEntry*	Q;
	DWORD				target_instance;
	SONORK_RESULT			result;

	if( service.event.cur_msg.sender.instance == 0 )
	{
		TRACE_DEBUG("  svc_msg:query:SENDER_INSTANCE=0:IGNORE");
		return;
	}
	target_instance=service.event.cur_msg.msg->target_instance;

	if( service.event.cur_msg.msg->Flow() ==  SONORK_CTRLMSG_SF_FLOW_AKN )
	{
		TRACE_DEBUG("  svc_msg:query:%x:akn:S=%x T=%x"
			, service.event.cur_msg.msg->QueryId()
			, service.event.cur_msg.sender.instance
			, target_instance
			);

		Q = Service_GetQuery(target_instance
			,service.event.cur_msg.msg->QueryId());
		if( Q != NULL )
		{
			Service_DispatchEvent(Q->entry
				, SONORK_APP_SERVICE_EVENT_QUERY_AKN
				, &Q->tag
				, &service.event
				);
		}
		else
		{
			TRACE_DEBUG("  svc_msg:query:akn:query not found");
		}
		return;
	}

	if( service.event.cur_msg.msg->Flow() !=  SONORK_CTRLMSG_SF_FLOW_REQ )
	{
		TRACE_DEBUG("  svc_msg:query:FLOW=??:IGNORED");
		return;
	}

	TRACE_DEBUG("  svc_msg:query:%x:req:S=%x T=%x.%x BC=%x"
		, service.event.cur_msg.msg->QueryId()
		, service.event.cur_msg.sender.instance
		, service.event.cur_msg.msg->ServiceId()
		, target_instance
		, service.event.cur_msg.msg->TargetIsBroadcast()
		);

	service.active_task_type=0;
	service.active_type = SONORK_CTRLMSG_SF_TYPE_QUERY
				|SONORK_CTRLMSG_SF_FLOW_AKN;

	if( service.event.cur_msg.msg->TargetIsBroadcast() )
	{
		service.entries.BeginEnum(I);
		while( (E=service.entries.EnumNext(I)) != NULL )
		{
			if( !(E->flags&SONORK_APP_SERVICE_EF_REGISTERED) )
				continue;

			if( !(target_instance & ((DWORD)E->sii.ServiceInstance()) ) )
				continue;

			if(service.event.cur_msg.msg->ServiceId() != SONORK_SERVICE_ID_NONE )
			if(service.event.cur_msg.msg->ServiceId() != E->sii.ServiceId() )
				continue;

			service.active_entry=E;

			Service_DispatchEvent(
				  E
				, SONORK_APP_SERVICE_EVENT_QUERY_REQ
				, NULL
				, &service.event
				);
			// Dispatch
		}
		service.entries.EndEnum(I);
		result=SONORK_RESULT_OK;
	}
	else
	{
		E=Service_GetEntry(
			  service.event.cur_msg.msg->ServiceId()
			, target_instance
			, true );
		if( E == NULL )
		{
			// Entry is invalid: Report to the sender
			TRACE_DEBUG("  svc_msg:query:req:entry %x not found",target_instance);
			result=SONORK_RESULT_INVALID_HANDLE;
		}
		else
		{
			service.active_entry=E;
			Service_DispatchEvent(
				  E
				, SONORK_APP_SERVICE_EVENT_QUERY_REQ
				, NULL
				, &service.event
				);
			result=SONORK_RESULT_OK;
		}
	}
	// Send QUERY_END to sender
	Service_ReplyCurMsg(SONORK_CTRLMSG_CMD_CLOSE
		,0	// No sender
		,SONORK_CTRLMSG_SF_TYPE_QUERY
		,result);
	service.active_type=SONORK_CTRLMSG_SF_TYPE_NONE;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_ReplyCurMsg(
	  SONORK_CTRLMSG_CMD	cmd
	, DWORD			sending_instance
	, DWORD 		sys_flags
	, DWORD 		usr_param)
{
	TSonorkCtrlMsg msg;
	assert( service.event.cur_msg.msg != NULL );
	sys_flags|=SONORK_CTRLMSG_SF_FLOW_AKN;
	TRACE_DEBUG("  svc_reply:S=I:%x,C:%x|%x, T=I:%x,C:%u|%u Z:%u"

		, sending_instance
		, service.event.cur_msg.msg->cmdFlags
		, service.event.cur_msg.msg->sysFlags

		, service.event.cur_msg.sender.instance
		, cmd
		, sys_flags

		, 0);
	if(service.event.cur_msg.msg->Flow() == SONORK_CTRLMSG_SF_FLOW_AKN )
	{
		TRACE_DEBUG("  svc_reply:replying to 'AKN'");
	}
	msg.LoadNewCmd(cmd
		,usr_param
		,0
		,sys_flags);
	msg.body.n.systemId = service.event.cur_msg.msg->body.n.systemId;
	msg.source_instance = sending_instance;
	msg.target_instance = service.event.cur_msg.sender.instance;
	SendCtrlMsg( &service.event.cur_msg.sender
		, &msg
		, NULL
		, 0
		, NULL);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SendCtrlMsg(
			  TSonorkServiceEntry*	source_entry
			, TSonorkServiceHandle*	target_handle
			, TSonorkCtrlMsg*	msg
			, const void*		data_ptr
			, DWORD			data_size
			, DWORD			msg_type)
{
	SONORK_DWORD2	task_tag;
	TSonorkAppTask*	task_ptr;
	SONORK_RESULT	result;

	msg->source_instance = source_entry->sii.ServiceInstance();
	msg->target_instance = target_handle->instance;
	if( msg_type != 0 )
	{
		SONORK_MEM_NEW( task_ptr= new TSonorkAppTask(SONORK_APP_TASK_SEND_SERVICE_MSG) );

		task_tag.v[0] = (DWORD)task_ptr;
		task_tag.v[1] = (DWORD)-1;
		task_ptr->send_service_msg.systemId	= msg->body.n.systemId;
		task_ptr->send_service_msg.instance     = msg->source_instance;
		task_ptr->send_service_msg.msgType 	= msg_type;
		result = SendCtrlMsg(
				  target_handle
				, msg
				, data_ptr
				, data_size
				, &task_tag);
		if(result==SONORK_RESULT_OK)
		{
			win32.task_queue.Add( task_ptr );
		}
		else
		{
			SONORK_MEM_DELETE( task_ptr );
		}
		return result;
	}
	return SendCtrlMsg( target_handle
			, msg
			, data_ptr
			, data_size
			, NULL);
}


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_OnSonorkDisconnect()
{
	TRACE_DEBUG("  svc_OnSonorkDisconnect: queries=%u circuits=%u"
		,service.queries.Items()
		,service.circuits.Items());

	Service_Cleanup( SONORK_RESULT_FORCED_TERMINATION , true );
}

void
 TSonorkWin32App::Service_OnUserNotAvaiable(const TSonorkId& user_id)
{
	TSonorkListIterator	I;
	union {
		TSonorkServiceQueryEntry*	Q;
		TSonorkServiceCircuitEntry*	C;
	}D;
	TRACE_DEBUG("  svc_User %u.%u not available queries=%u circuits=%u"
		,user_id.v[0]
		,user_id.v[1]
		,service.queries.Items()
		,service.circuits.Items()
		);
	service.queries.BeginEnum(I);
	while( (D.Q=service.queries.EnumNext(I)) != NULL )
	{
		if(D.Q->user_id == user_id )
			Service_CondemnQuery( D.Q
				, SONORK_RESULT_FORCED_TERMINATION
				, true
				, true);
	}
	service.queries.EndEnum(I);

	service.circuits.BeginEnum(I);
	while( (D.C=service.circuits.EnumNext(I)) != NULL )
	{
		if(D.C->handle.userId == user_id )
		{
			D.C->flags|=SONORK_APP_SERVICE_CF_REMOTE_DEAD;
			Service_CondemnCircuit( D.C
			, SONORK_RESULT_FORCED_TERMINATION
			, true
			, true
			);
		}
	}
	service.circuits.EndEnum(I);

}

void
 TSonorkWin32App::Service_OnUserUtsLink(const TSonorkId& user_id, DWORD uts_link_id)
{
	TSonorkListIterator	I;
	TSonorkAppServiceEvent	event;
	union {
		TSonorkServiceQueryEntry*	Q;
		TSonorkServiceCircuitEntry*	C;
	}D;
	TRACE_DEBUG("  svc_UtsLink(%u.%u -> %x) circuits=%u"
		,user_id.v[0]
		,user_id.v[1]
		,uts_link_id
		,service.circuits.Items());
	service.circuits.BeginEnum(I);
	while( (D.C=service.circuits.EnumNext(I)) != NULL )
	{
		if(D.C->flags& SONORK_APP_SERVICE_CF_ACTIVE)
		if(D.C->handle.userId == user_id )
		if(D.C->handle.utsLinkId!=uts_link_id)
		{
			D.C->handle.utsLinkId=uts_link_id;

			event.circuit_update.handle = &D.C->handle;
			TRACE_DEBUG("  svc_UtsLink:circuit:%x:%x:%x -> %x "
				,D.C->entry->sii.ServiceInstance()
				,D.C->handle.CircuitId()
				,D.C->handle.utsLinkId
				,uts_link_id);
			Service_DispatchEvent(
				 D.C->entry
				,SONORK_APP_SERVICE_EVENT_CIRCUIT_UPDATE
				,&D.C->tag
				,&event
				);
		}
	}
	service.circuits.EndEnum(I);
}


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::TimerTask_Services(UINT interval_msecs)
{
	TSonorkListIterator	I;
	union {
		TSonorkServiceQueryEntry *Q;
		TSonorkServiceCircuitEntry *C;
	}D;
	if(service.queries.Items())
	{
		service.queries.BeginEnum(I);
		while( (D.Q=service.queries.EnumNext(I)) != NULL )
		{
			if( !D.Q->max_msecs )continue;
			D.Q->cur_msecs+=interval_msecs;
			if(D.Q->cur_msecs<D.Q->max_msecs)
				continue;
			TRACE_DEBUG("  svc_monitor:query:%06x,%x,%u/%u"
				,D.Q->entry->sii.ServiceInstance()
				,D.Q->query_id
				,D.Q->cur_msecs
				,D.Q->max_msecs
				);
			Service_CondemnQuery( D.Q
				, SONORK_RESULT_TIMEOUT
				, true
				, true);
		}
		service.queries.EndEnum(I);
	}
	if( service.pending_circuits.Items() )
	{
		service.pending_circuits.BeginEnum(I);
		while( (D.C=service.pending_circuits.EnumNext(I)) != NULL )
		{
			if( !D.C->max_msecs )continue;
			D.C->cur_msecs+=interval_msecs;
			if(D.C->cur_msecs<D.C->max_msecs)
				continue;
			TRACE_DEBUG("  svc_monitor:circuit:%06x,%x,%u/%u)"
				,D.C->entry->sii.ServiceInstance()
				,D.C->handle.CircuitId()
				,D.C->cur_msecs
				,D.C->max_msecs
				);
			Service_CondemnCircuit( D.C
				, SONORK_RESULT_TIMEOUT
				, true
				, true);
		}
		service.pending_circuits.EndEnum(I);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_CondemnQuery(
	  TSonorkServiceQueryEntry* 	query
	, SONORK_RESULT 		result
	, bool				invoke_handler
	, bool				clear_item)
{
	TSonorkAppServiceEvent event;
	TRACE_DEBUG("  svc_condemn:query:%06x,%x,F=%x,Ptr=%x"
				,query->entry->sii.ServiceInstance()
				,query->query_id
				,query->flags
				,query
				);
	if( query->flags&SONORK_APP_SERVICE_QF_ACTIVE )
	{
		query->flags&=~SONORK_APP_SERVICE_QF_ACTIVE;
		if(clear_item)
		{
			query->flags|=SONORK_APP_SERVICE_QF_CLEARED;
			::PostMessage(win32.work_win
				, WM_SONORK_SERVICE
				, WM_SONORK_SERVICE_WPARAM_CLEAR_QUERY
				, (LPARAM)query);
		}
		if(invoke_handler)
		{
			event.query_end.id	= query->query_id;
			event.query_end.result  = result;
			Service_DispatchEvent(query->entry
				, SONORK_APP_SERVICE_EVENT_QUERY_END
				, &query->tag
				, &event
				);
		}
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_CondemnCircuit(
	 TSonorkServiceCircuitEntry* 	circuit
	,SONORK_RESULT 			result
	,bool				invoke_handler
	,bool				clear_item)
{
	TSonorkAppServiceEvent event;
	TSonorkCtrlMsg	msg;
	TRACE_DEBUG("  svc_condemn:circuit:%06x,%x,F=%x,Ptr=%x"
				,circuit->entry->sii.ServiceInstance()
				,circuit->handle.CircuitId()
				,circuit->flags
				,circuit
				);
	if( circuit->flags&SONORK_APP_SERVICE_CF_ACTIVE )
	{
		circuit->flags&=~SONORK_APP_SERVICE_CF_ACTIVE;

		if( !(circuit->flags &SONORK_APP_SERVICE_CF_REMOTE_DEAD) )
		{
			circuit->flags|=SONORK_APP_SERVICE_CF_REMOTE_DEAD;
			if( CxReady() )
			{
				msg.LoadNewCmd(SONORK_CTRLMSG_CMD_CLOSE
				,result
				,0
				,SONORK_CTRLMSG_SF_FLOW_NONE
				|SONORK_CTRLMSG_SF_TYPE_CIRCUIT
				);
				msg.body.n.systemId = circuit->handle.CircuitId();
				Service_SendCtrlMsg(
				  circuit->entry
				, &circuit->handle
				, &msg
				, NULL
				, 0
				, 0 );
			}
		}

		if(invoke_handler)
		{
			event.circuit_close.id=circuit->handle.CircuitId();
			event.circuit_close.result=result;
			Service_DispatchEvent(circuit->entry
				, SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED
				, &circuit->tag
				, &event
				);
		}
		if(clear_item)
		{
			circuit->flags|=SONORK_APP_SERVICE_CF_CLEARED;
			::PostMessage(win32.work_win
				, WM_SONORK_SERVICE
				, WM_SONORK_SERVICE_WPARAM_CLEAR_CIRCUIT
				, (LPARAM)circuit);
		}
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_Cleanup(SONORK_RESULT result, bool invoke_handlers)
{
	union {
		TSonorkServiceQueryEntry *Q;
		TSonorkServiceCircuitEntry *C;
	}D;
	while( (D.Q=service.queries.RemoveFirst()) != NULL )
	{
		Service_CondemnQuery(D.Q
			,result
			,invoke_handlers
			,false);
		if(!(D.Q->flags&SONORK_APP_SERVICE_QF_CLEARED))
		{
			SONORK_MEM_DELETE(D.Q);
		}
		else
		{
			D.Q->flags|=SONORK_APP_SERVICE_QF_REMOVED;
		}
	}
	while( (D.C=service.circuits.RemoveFirst()) != NULL )
	{
		Service_CondemnCircuit(D.C
			,result
			,invoke_handlers
			,false);
		if(!(D.C->flags&SONORK_APP_SERVICE_CF_CLEARED))
		{
			service.pending_circuits.Remove(D.C);
			SONORK_MEM_DELETE(D.C);
		}
		else
		{
			D.C->flags|=SONORK_APP_SERVICE_CF_REMOVED;
		}
	}
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_Register(
		  SONORK_APP_SERVICE_TYPE	svc_type
		, TSonorkServiceInstanceInfo*	svc_info
		, SKIN_ICON			svc_icon
		, TSonorkAppServiceCallbackPtr	cb_ptr
		, SONORK_DWORD2* 		cb_tag)
{
	TSonorkServiceEntry*E;
	DWORD	instance_id;
	if(TestRunFlag(SONORK_WAPP_RF_APP_TERMINATING))
		return SONORK_RESULT_FORCED_TERMINATION;

	if( service.entries.Items() > SONORK_APP_MAX_SERVICE_ENTRIES )
		return SONORK_RESULT_QUOTA_EXCEEDED;

	svc_info->info.v[TSonorkServiceInfo::V_DESCRIPTOR]&=~(SONORK_SERVICE_HFLAG_SYSTEMWIDE|SONORK_SERVICE_HFLAG_LOCATOR);
	if( svc_type == SONORK_APP_SERVICE_TYPE_CLIENT
	||  svc_type == SONORK_APP_SERVICE_TYPE_NONE)
	{
		for(;;)
		{
			if(service.cookie++ >= SONORK_SERVICE_INSTANCE_V_MAX_VALUE)
				service.cookie=SONORK_SERVICE_INSTANCE_V_MIN_VALUE;
			instance_id=service.cookie;
			if(Service_GetEntry( SONORK_SERVICE_ID_NONE
				, instance_id , false )==NULL)
				break;
		}
	}
	else
	{
		if( svc_info->ServiceId() == SONORK_SERVICE_ID_NONE )
		{
			return SONORK_RESULT_INVALID_PARAMETER;
		}
		if( svc_type == SONORK_APP_SERVICE_TYPE_SERVER )
		{
			for(;;)
			{
				if(service.cookie++ >= SONORK_SERVICE_INSTANCE_V_MAX_VALUE)
					service.cookie=SONORK_SERVICE_INSTANCE_V_MIN_VALUE;
				instance_id=SONORK_SERVICE_SERVER_INSTANCE(service.cookie);
				if(Service_GetEntry( SONORK_SERVICE_ID_NONE
					, instance_id , false )==NULL)
					break;
			}
		}
		else
		if( svc_type == SONORK_APP_SERVICE_TYPE_LOCATOR )
		{
			svc_info->info.v[TSonorkServiceInfo::V_DESCRIPTOR]|=SONORK_SERVICE_HFLAG_LOCATOR;
			if(Service_GetEntry(
					  svc_info->ServiceId()
					, SONORK_SERVICE_LOCATOR_INSTANCE(0)
					, false )!=NULL)
			{
				return SONORK_RESULT_DUPLICATE_DATA;
			}
			if(service.cookie++ >= SONORK_SERVICE_INSTANCE_V_MAX_VALUE)
				service.cookie=SONORK_SERVICE_INSTANCE_V_MIN_VALUE;
			instance_id=SONORK_SERVICE_LOCATOR_INSTANCE(service.cookie);
		}
		else
		{
			return SONORK_RESULT_INVALID_PARAMETER;
		}
	}
	svc_info->SetInstance(instance_id, svc_info->ServiceVersionNumber() );
	SONORK_MEM_NEW( E=new TSonorkServiceEntry );
	TRACE_DEBUG("  svc_register:%06x,%u,%08x,Ptr=%x"
		,instance_id
		,svc_info->ServiceId()
		,svc_info->ServiceDescriptor()
		,E
		);
	E->sii.Set(*svc_info);
	if( svc_type == SONORK_APP_SERVICE_TYPE_NONE )
		E->flags=SONORK_APP_SERVICE_EF_REGISTERED
			|SONORK_APP_SERVICE_EF_HIDDEN;
	else
		E->flags=SONORK_APP_SERVICE_EF_REGISTERED;
	if( win32.run_flags&SONORK_WAPP_RF_IN_IPC_HANDLER)
		E->flags|=SONORK_APP_SERVICE_EF_IPC;
	E->cb_ptr = cb_ptr;
	if( cb_tag )
		E->cb_tag.Set( *cb_tag );
	else
		E->cb_tag.Clear();
	E->icon = svc_icon;
	service.entries.Add( E );
	return SONORK_RESULT_OK;
}
// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SetCallback(
		  DWORD 			instance_id
		, TSonorkAppServiceCallbackPtr	cb_ptr
		, SONORK_DWORD2* 		cb_tag)
{
	TSonorkServiceEntry*E;
	TRACE_DEBUG("  svc_setcallback:%04x,<none>:Ptr=%x Tag=%x.%x"
			,instance_id
			,cb_ptr
			,cb_tag?cb_tag->v[0]:0
			,cb_tag?cb_tag->v[1]:0
			);
	E=Service_GetEntry(SONORK_SERVICE_ID_NONE, instance_id , true);
	if(E==NULL)
	{
		TRACE_DEBUG("  svc_setcallback:%04x,<none>:NOT FOUND"
			,instance_id);
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	E->cb_ptr = cb_ptr;
	if( cb_tag )
		E->cb_tag.Set( *cb_tag );
	return SONORK_RESULT_OK;
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_Unregister(
	 DWORD instance_id
	,BOOL invoke_query_callbacks
	,SONORK_SERVICE_ID service_id)
{
	TSonorkServiceEntry*E;
	E=Service_GetEntry(service_id , instance_id , true);
	if(E==NULL)
	{
		TRACE_DEBUG("  svc_unregister:%04x,%u:NOT FOUND"
			,instance_id
			,service_id);
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	Service_Unregister( E , invoke_query_callbacks);
	return SONORK_RESULT_OK;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_Unregister(TSonorkServiceEntry*entry, BOOL)
{
	TSonorkListIterator	I;
	union
	{
		TSonorkServiceQueryEntry*	Q;
		TSonorkServiceCircuitEntry*	C;
	}D;
	DWORD			instance;

	if( !(entry->flags&SONORK_APP_SERVICE_EF_REGISTERED) )
		return;

	entry->flags&=~SONORK_APP_SERVICE_EF_REGISTERED;

	if(TestRunFlag(SONORK_WAPP_RF_APP_TERMINATING))
		return ;

	instance=entry->sii.ServiceInstance();
	TRACE_DEBUG("  svc_unregister:%04x:%02x,Ptr=%x"
		,instance
		,entry->flags
		,entry);

	if(entry->flags&SONORK_APP_SERVICE_EF_EXPORTED)
		Service_CancelExport(entry);

	service.queries.BeginEnum(I);
	while( (D.Q=service.queries.EnumNext(I)) != NULL )
	{
		if( D.Q->entry == entry )
			Service_CondemnQuery(D.Q
				, SONORK_RESULT_FORCED_TERMINATION
				, false
				, true);
	}
	service.queries.EndEnum(I);

	service.circuits.BeginEnum(I);
	while( (D.C=service.circuits.EnumNext(I)) != NULL )
	{
		if( D.C->entry == entry )
			Service_CondemnCircuit(D.C
				, SONORK_RESULT_FORCED_TERMINATION
				, false
				, true);
	}
	service.circuits.EndEnum(I);

	// We don't remove the service from the queue
	// because this call might happed between a
	// BeginEnum..EndEnum block: Queues cannot be
	// modified while enumerating.
	// We post ourselves a window message that will be
	// handled from within TSonorkWin32App::DoRun()
	// where we're sure that the queue is not locked.

	::PostMessage(win32.work_win
		, WM_SONORK_SERVICE
		, WM_SONORK_SERVICE_WPARAM_CLEAR_ENTRY
		, (LPARAM)entry);
}

// ----------------------------------------------------------------------------
void
 TSonorkWin32App::WmSonorkService(WPARAM wParam ,LPARAM lParam)
{

	switch( wParam )
	{

#define entry	((TSonorkServiceEntry*)lParam)
	case WM_SONORK_SERVICE_WPARAM_CLEAR_ENTRY:
		assert( service.entries.Remove(entry) != false );
		assert( !(entry->flags&SONORK_APP_SERVICE_EF_REGISTERED) );
		SONORK_MEM_DELETE( entry );
#undef	entry
		break;

	case WM_SONORK_SERVICE_WPARAM_CLEAR_QUERY:

#define query	((TSonorkServiceQueryEntry*)lParam)
		assert( query->flags&SONORK_APP_SERVICE_QF_CLEARED);
		if( !(query->flags&SONORK_APP_SERVICE_QF_REMOVED)  )
			assert(service.queries.Remove(query));

		SONORK_MEM_DELETE(query);
#undef 	query
		break;

	case WM_SONORK_SERVICE_WPARAM_CLEAR_CIRCUIT:
#define circuit	((TSonorkServiceCircuitEntry*)lParam)

		assert( circuit->flags&SONORK_APP_SERVICE_CF_CLEARED);

		if( !(circuit->flags&SONORK_APP_SERVICE_CF_REMOVED) )
			assert(service.circuits.Remove(circuit));

		if( circuit->flags&SONORK_APP_SERVICE_CF_PENDING )
			assert(service.pending_circuits.Remove(circuit));

		SONORK_MEM_DELETE(circuit);
		break;

	case WM_SONORK_SERVICE_WPARAM_ACCEPT_CIRCUIT:
		// Don't assert nor test anything here:
		//  The circuit could've been removed, and that is ok.
		// Just don't de-reference it because it might
		//  have also been deleted. Just remove the
		// pointer from the <pending_circuits> queue
		service.pending_circuits.Remove(circuit);
		break;

#undef 	circuit

	}
}
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnAppTaskResult_SendServiceMsg(
		TSonorkAppTask_SendServiceMsg*	TASK
		,const TSonorkError*		pERR)
{
	union
	{
		TSonorkServiceQueryEntry*	Q;
		TSonorkServiceCircuitEntry*	C;
	}D;
	TRACE_DEBUG("  svc_AppTask:send:%08x:E=%x I=%x T=%x R=%s"
			,TASK
			,TASK->instance
			,TASK->systemId
			,TASK->msgType
			,pERR->ResultName());
	if( pERR->Result() == SONORK_RESULT_OK)
		return;
	if((TASK->msgType&SONORK_CTRLMSG_SF_TYPE_MASK)==SONORK_CTRLMSG_SF_TYPE_CIRCUIT)
	{
		TRACE_DEBUG("             :send:circuit:FL=%x"
			,  TASK->msgType&SONORK_CTRLMSG_SF_FLOW_MASK);

		D.C=Service_GetCircuit(TASK->instance,TASK->systemId);
		if(D.C != NULL)
		{
			Service_CondemnCircuit(D.C,pERR->Result(),true,true);
		}
		else
			TRACE_DEBUG("             :send:circuit:NOT FOUND");

	}
	else
	if((TASK->msgType&SONORK_CTRLMSG_SF_TYPE_MASK)==SONORK_CTRLMSG_SF_TYPE_QUERY)
	{

		TRACE_DEBUG("             :send:query:FL=%x"
			,  TASK->msgType&SONORK_CTRLMSG_SF_FLOW_MASK);
		D.Q=Service_GetQuery(TASK->instance,TASK->systemId);
		if(D.Q != NULL)
		{
			Service_CondemnQuery(D.Q,pERR->Result(),true,true);
		}
		else
			TRACE_DEBUG("             :send:query:NOT FOUND");
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::OnAppTaskResult_ExportService(
		 TSonorkAppTask_ExportService*	TASK
		,const TSonorkError*		pERR)
{
	TSonorkServiceEntry* E;
	TSonorkAppServiceEvent	event;

	TRACE_DEBUG("  svc_AppTask:export:%08x:E=%x O=%u R=%s"
			,TASK
			,TASK->service_info.ServiceInstance()
			,TASK->operation
			,pERR->ResultName());
	if( TASK->operation == SONORK_APP_TASK_EXPORT_OP_KILL )
		return;

	E=Service_GetEntry(SONORK_SERVICE_ID_NONE
		, TASK->service_info.ServiceInstance()
		, true);
	if( E== NULL)
		return;
	Service_LockEntry(E,false);

	if( pERR->Result() == SONORK_RESULT_OK)
	{
		if(TASK->operation == SONORK_APP_TASK_EXPORT_OP_DEL )
			event.exported.value=0;
		else
		if(TASK->operation == SONORK_APP_TASK_EXPORT_OP_ADD )
			event.exported.value=SONORK_APP_SERVICE_EF_EXPORTED;
		else
			event.exported.value=(E->flags&SONORK_APP_SERVICE_EF_EXPORTED);
	}
	else
	{
		if(TASK->operation == SONORK_APP_TASK_EXPORT_OP_DEL )
			event.exported.value=SONORK_APP_SERVICE_EF_EXPORTED;
		else
		if(TASK->operation == SONORK_APP_TASK_EXPORT_OP_ADD )
			event.exported.value=0;
		else
			event.exported.value=(E->flags&SONORK_APP_SERVICE_EF_EXPORTED);
	}

	if( (E->flags&SONORK_APP_SERVICE_EF_EXPORTED)!=event.exported.value )
	{
		E->flags&=~SONORK_APP_SERVICE_EF_EXPORTED;
		E->flags|=event.exported.value;
		Service_DispatchEvent(E
			,SONORK_APP_SERVICE_EVENT_EXPORT
			,NULL
			,&event
			);
	}
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_ExportService(DWORD instance_id, BOOL exported)
{
	TSonorkServiceEntry*E;
	E=Service_GetEntry(SONORK_SERVICE_ID_NONE , instance_id, true);
	if(E==NULL)return SONORK_RESULT_INVALID_PARAMETER;
	if( exported )
	{
		return Service_ExportService( E );
	}
	else
	{
		return Service_CancelExport( E );
	}
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_ExportService( TSonorkServiceEntry* E )
{
	TSonorkDataPacket*	P;
	UINT			A_size,P_size;
	TSonorkAppTask		*TASK;
	SONORK_DWORD2		tag;
	SONORK_RESULT		result;
	SONORK_APP_TASK_EXPORT_OP	operation;
	TSonorkUserServer	server;
	TSonorkAppServiceEvent	event;

	TRACE_DEBUG("  svc_export:%06x:%02x"
		,E->sii.ServiceInstance()
		,E->flags);
	if(E->flags&SONORK_APP_SERVICE_EF_LOCKED)
		return SONORK_RESULT_SERVICE_BUSY;

	if( !CxReady() )
		return SONORK_RESULT_NOT_READY;

	event.get_server.msg  = NULL; 
	event.get_server.data = &server;

	if( !Service_DispatchEvent(E
			,SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA
			,NULL
			,&event))
	{
		return SONORK_RESULT_INTERNAL_ERROR;

	}

	operation = (E->flags&SONORK_APP_SERVICE_EF_EXPORTED)
			?SONORK_APP_TASK_EXPORT_OP_SET
			:SONORK_APP_TASK_EXPORT_OP_ADD;

	// Overwrite what the callback loaded in the
	// information with our 'official' information
	server.header.locus.info.Set( E->sii );


	SONORK_MEM_NEW( TASK= new TSonorkAppTask(SONORK_APP_TASK_EXPORT_SERVICE) );
	tag.v[0] = (DWORD)TASK;
	tag.v[1] = (DWORD)-1;
	TASK->export_service.operation = operation;
	TASK->export_service.service_info.Set(server.header.locus.info);

	server.wUserId().Set(ProfileUserId());
	server.wSid().Set(ProfileSid());
	server.wRegion().Set(ProfileRegion());

	A_size = server.CODEC_Size() + 64;
	P=SONORK_AllocDataPacket( A_size );
	if(operation == SONORK_APP_TASK_EXPORT_OP_SET)
		P_size = P->E_SetUserServer_R(A_size , server);
	else
		P_size = P->E_AddUserServer_R(A_size , server);

	result = StartSonorkRequest(P,P_size,&tag,NULL);

	if(result == SONORK_RESULT_OK)
	{
		Service_LockEntry(E,true);
		win32.task_queue.Add( TASK );
		return SONORK_RESULT_OK;
	}
	SONORK_MEM_DELETE( TASK );
	return result;
}


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Service_LockEntry(TSonorkServiceEntry* E, BOOL s)
{
	TSonorkAppServiceEvent	event;
	TRACE_DEBUG("  svc_lock:%06x:%02x,%u"
		,E->sii.ServiceInstance()
		,E->flags,s);
	if( s )
	{
		assert( !(E->flags&SONORK_APP_SERVICE_EF_LOCKED) );
		E->flags|=SONORK_APP_SERVICE_EF_LOCKED;
	}
	else
	{
		assert( E->flags&SONORK_APP_SERVICE_EF_LOCKED );
		E->flags&=~SONORK_APP_SERVICE_EF_LOCKED;
	}
	event.busy.value=s;
	Service_DispatchEvent(E
		,SONORK_APP_SERVICE_EVENT_BUSY
		,NULL
		,&event);
}
SONORK_RESULT
 TSonorkWin32App::Service_CancelExport(TSonorkServiceEntry* E)
{
	TSonorkDataPacket*	P;
	UINT			P_size;
	TSonorkAppTask		*TASK;
	SONORK_DWORD2		tag;
	SONORK_RESULT		result;
	SONORK_APP_TASK_EXPORT_OP	operation;
	TSonorkAppServiceEvent	event;

	TRACE_DEBUG("  svc_cancel_export:%06x:%02x"
		,E->sii.ServiceInstance()
		,E->flags);
	if(!(E->flags&SONORK_APP_SERVICE_EF_EXPORTED))
		return SONORK_RESULT_OK;

	if(E->flags&SONORK_APP_SERVICE_EF_LOCKED)
		return SONORK_RESULT_SERVICE_BUSY;

	// We're no longer marked as exported
	if( !CxReady() )
	{
		// Cannot tell the Sonork server because we're not connected.
		// Leave the entry marked as NOT exported because it can't
		// exported if we're not connected.
		result = SONORK_RESULT_NOT_READY;
		E->flags&=~SONORK_APP_SERVICE_EF_EXPORTED;
	}
	else
	{
#define A_size	64
		// Has the entry been killed?
		// If the entry is not registered it means that
		// Service_CancelExport() was invoked from within
		// Service_UnregisterServer() which is going to
		// delete the entry, so it is pointless to process the
		// result of this task.
		operation=(E->flags&SONORK_APP_SERVICE_EF_REGISTERED)
			?SONORK_APP_TASK_EXPORT_OP_DEL
			:SONORK_APP_TASK_EXPORT_OP_KILL;

		SONORK_MEM_NEW( TASK= new TSonorkAppTask(SONORK_APP_TASK_EXPORT_SERVICE) );
		tag.v[0] = (DWORD)TASK;
		tag.v[1] = (DWORD)-1;
		TASK->export_service.operation = operation;
		TASK->export_service.service_info.SetInstanceInfo(
			  E->sii.ServiceId()
			, SONORK_SERVICE_TYPE_NONE
			, 0
			, E->sii.ServiceInstance()
			, 0
			);
		P=SONORK_AllocDataPacket( A_size );
		P_size = P->E_DelUserServer_R(A_size
			,ProfileUser().Sid()
			,E->sii.ServiceInstance());
		result = StartSonorkRequest(P,P_size,&tag,NULL);


		if(result == SONORK_RESULT_OK)
		{
			win32.task_queue.Add( TASK );
			if( operation == SONORK_APP_TASK_EXPORT_OP_DEL )
			{
				Service_LockEntry(E,true);
			}
		}
		else
		{
			SONORK_MEM_DELETE( TASK );
		}
	}
	
	// The dispatcher will only be invoked if the EXPORTED flag
	// was cleared, which means that we changed the flag because
	// at the top of the procedure we check that the flag is SET
	// before proceeding.
	// If an error ocurred and the flag remains SET, the dispatcher
	// will not be invoked.

	if( !(E->flags&SONORK_APP_SERVICE_EF_EXPORTED) )
	{
		event.exported.value=0;
		// The dispatcher does not send events to
		// unregistered entries: It silently
		// ignores the call.
		Service_DispatchEvent(E
			,SONORK_APP_SERVICE_EVENT_EXPORT
			,NULL
			,&event);
	}
#undef A_size
	return result;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::Init_Services()
{
	TSonorkServiceInstanceInfo	sii;
	SONORK_DWORD2			sii_tag;
	// ---------------------------------------
	// Initialization
	service.cookie=SONORK_SERVICE_INSTANCE_V_MIN_VALUE;
	service.active_type =SONORK_CTRLMSG_SF_TYPE_NONE;
	service.active_entry=NULL;
	SONORK_ZeroMem(&service_instance,sizeof(service_instance));


	// ------------------------------------
	// Install built-in locators

	sii_tag.v[0]=(DWORD)this;
	sii_tag.v[1]=0;
	// ---------------------------------------
	// Sonork Application Service Handler

	sii.SetInstanceInfo(
		SONORK_SERVICE_ID_SONORK
		, SONORK_SERVICE_TYPE_NONE
		, 0	// hflags
		, 0	// instance: Don't know yet, set to 0
		, SONORK_APP_VERSION_NUMBER);

	if(Service_Register(
			  SONORK_APP_SERVICE_TYPE_LOCATOR
			, &sii
			, SKIN_ICON_SONORK
			, ServiceCallback_Sonork
			, &sii_tag)==SONORK_RESULT_OK)
	{
		service_instance.sonork=sii.ServiceInstance();
	}

	// ---------------------------------------
	// External applications Service Handler

	sii.SetInstanceInfo(
		SONORK_SERVICE_ID_EXTERNAL_APP
		, SONORK_SERVICE_TYPE_NONE
		, 0	// hflags
		, 0	// instance: Don't know yet, set to 0
		, SONORK_APP_VERSION_NUMBER);

	if(Service_Register(
			  SONORK_APP_SERVICE_TYPE_LOCATOR
			, &sii
			, SKIN_ICON_PLUGIN_OK
			, ServiceCallback_ExtApp
			, &sii_tag)==SONORK_RESULT_OK)
	{
		service_instance.ext_app=sii.ServiceInstance();
	}

	// ---------------------------------------
	// Sonork Chat Service Handler

	sii.SetInstanceInfo(
	  SONORK_SERVICE_ID_SONORK_CHAT
	, SONORK_SERVICE_TYPE_SONORK_CHAT
	, 0	// hflags
	, 0	// instance: Don't know yet, set to 0
	, SONORK_CHAT_VERSION_NUMBER);

	if(Service_Register(
			  SONORK_APP_SERVICE_TYPE_LOCATOR
			, &sii
			, SKIN_ICON_CHAT
			, TSonorkChatWin::LocatorServiceCallback
			, &sii_tag)==SONORK_RESULT_OK)
	{
		service_instance.chat=sii.ServiceInstance();
	}

	// ---------------------------------------
	// Sonork Tracker Service Handler

	sii.SetInstanceInfo(
	  SONORK_SERVICE_ID_SONORK_TRACKER
	, SONORK_SERVICE_TYPE_NONE
	, 0	// hflags
	, 0	// instance: Don't know yet, set to 0
	, SONORK_TRACKER_VERSION_NUMBER);

	if(Service_Register(
			  SONORK_APP_SERVICE_TYPE_LOCATOR
			, &sii
			, SKIN_ICON_TRACKER
			, TSonorkTrackerWin::LocatorServiceCallback
			, &sii_tag)==SONORK_RESULT_OK)
	{
		service_instance.tracker=sii.ServiceInstance();
	}

	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Exit_Services()
{
	TSonorkServiceEntry*E;
	Service_Cleanup(SONORK_RESULT_FORCED_TERMINATION, false );
	while( (E=service.entries.RemoveFirst()) != NULL )
		SONORK_MEM_DELETE(E);
}

// ----------------------------------------------------------------------------

TSonorkServiceEntry*
  TSonorkWin32App::Service_GetEntry(
	  SONORK_SERVICE_ID 	get_service_id
	, DWORD 		get_instance_id
	, BOOL 			registered_only)
{
	TSonorkListIterator	I;
	TSonorkServiceEntry*E;
	if( get_instance_id < SONORK_SERVICE_INSTANCE_V_MIN_VALUE )
	{
		return NULL;
	}
	service.entries.BeginEnum(I);
	if( get_instance_id & SONORK_SERVICE_INSTANCE_F_LOCATOR )
	{
		while( (E=service.entries.EnumNext(I)) != NULL )
		{
			if(!(E->sii.ServiceInstance()&SONORK_SERVICE_INSTANCE_F_LOCATOR))
				continue;
			if( get_instance_id == E->sii.ServiceInstance()
			||
			((get_instance_id&SONORK_SERVICE_INSTANCE_V_MASK)==0
			&& get_service_id == E->sii.ServiceId()) )
			{
				if( registered_only )
				if( !(E->flags&SONORK_APP_SERVICE_EF_REGISTERED) )
					continue;
				break;
			}
		}
	}
	else
	{
		while( (E=service.entries.EnumNext(I)) != NULL )
		{
			if(E->sii.ServiceInstance() == get_instance_id)
			if(get_service_id == SONORK_SERVICE_ID_NONE
			|| get_service_id == E->sii.ServiceId() )
			{
				if( registered_only )
				if( !(E->flags&SONORK_APP_SERVICE_EF_REGISTERED) )
					continue;
				break;
			}
		}
	}
	service.entries.EndEnum(I);
	return E;
}

// ----------------------------------------------------------------------------

TSonorkServiceQueryEntry*
 TSonorkWin32App::Service_GetQuery(DWORD instance_id, DWORD system_id)
{
	TSonorkServiceQueryEntry*Q;
	TSonorkListIterator	I;

	service.queries.BeginEnum(I);
	while( (Q=service.queries.EnumNext(I)) != NULL )
	{
		if(!(Q->flags&SONORK_APP_SERVICE_QF_ACTIVE))
			continue;
		if(Q->query_id != system_id)
			continue;
		if(Q->entry->sii.ServiceInstance() != instance_id)
			continue;
		break;
	}
	service.queries.EndEnum(I);
	return Q;
}
// ----------------------------------------------------------------------------
TSonorkServiceCircuitEntry*
 TSonorkWin32App::Service_GetCircuit(DWORD instance_id , DWORD system_id)
{
	TSonorkServiceCircuitEntry*C;
	TSonorkListIterator	I;

	service.circuits.BeginEnum(I);
	while( (C=service.circuits.EnumNext(I)) != NULL )
	{
		if(!(C->flags&SONORK_APP_SERVICE_CF_ACTIVE))
			continue;
		if(C->handle.systemId != system_id)
			continue;
		if(C->entry->sii.ServiceInstance() != instance_id)
			continue;
		break;
	}
	service.circuits.EndEnum(I);
	return C;
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SendCircuitData(
			  DWORD			source_instance
			, TSonorkServiceCircuit*circuit
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const void*		data_ptr
			, DWORD			data_size)
{
	TSonorkServiceEntry*	service_entry;
	TSonorkCtrlMsg		msg;

	service_entry=Service_GetEntry(SONORK_SERVICE_ID_NONE, source_instance , true);
	if(service_entry==NULL)
		return SONORK_RESULT_INVALID_PARAMETER;

	msg.LoadNewCmd(cmd
		,cmd_param
		,cmd_flags
		,SONORK_CTRLMSG_SF_FLOW_NONE
		|SONORK_CTRLMSG_SF_TYPE_CIRCUIT
		);
	msg.body.n.systemId = circuit->CircuitId();

	return Service_SendCtrlMsg(service_entry
		,circuit
		,&msg
		,data_ptr
		,data_size
		,SONORK_CTRLMSG_SF_FLOW_NONE
		|SONORK_CTRLMSG_SF_TYPE_CIRCUIT);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SendCircuitData(
			  DWORD			source_instance
			, TSonorkServiceCircuit*circuit
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom*A)
{
	TSonorkDynData	DD;
	if( A )A->CODEC_Write(&DD);
	return Service_SendCircuitData( source_instance
				,circuit
				,cmd
				,cmd_param
				,cmd_flags
				,DD.Buffer()
				,DD.DataSize());
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SendPokeData(
		  DWORD				source_instance
		, TSonorkServiceHandle*		target_handle
		, SONORK_CTRLMSG_CMD	 	cmd
		, DWORD 			cmd_param
		, DWORD 			cmd_flags
		, const void*			data_ptr
		, DWORD				data_size)
{
	TSonorkServiceEntry*	service_entry;
	TSonorkCtrlMsg		msg;

	service_entry=Service_GetEntry(SONORK_SERVICE_ID_NONE, source_instance , true);
	if(service_entry==NULL)
		return SONORK_RESULT_INVALID_PARAMETER;

	msg.LoadNewCmd(cmd
		,cmd_param
		,cmd_flags
		,SONORK_CTRLMSG_SF_FLOW_NONE
		|SONORK_CTRLMSG_SF_TYPE_NONE
		);

	return Service_SendCtrlMsg(service_entry
		,target_handle
		,&msg
		,data_ptr
		,data_size
		,0);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_SendPokeData(
			  DWORD			source_instance
			, TSonorkServiceHandle*	target_handle
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom* A)
{
	TSonorkDynData	DD;
	if( A )A->CODEC_Write(&DD);
	return Service_SendPokeData( source_instance
				,target_handle
				,cmd
				,cmd_param
				,cmd_flags
				,DD.Buffer()
				,DD.DataSize());
}

SONORK_RESULT
 TSonorkWin32App::Service_OpenCircuit(
			  DWORD			source_instance
			, TSonorkServiceCircuit*handle
			, DWORD			usr_param
			, DWORD 		usr_flags
			, DWORD			timeout_msecs
			, const void*		data_ptr
			, DWORD			data_size
			, SONORK_DWORD2*	tag)
{
	TSonorkServiceEntry*		service_entry;
	TSonorkCtrlMsg			msg;
	SONORK_RESULT			result;
	TSonorkServiceCircuitEntry*	circuit;

	service_entry=Service_GetEntry(SONORK_SERVICE_ID_NONE
		, source_instance
		, true);

	if(service_entry==NULL)
		return SONORK_RESULT_INVALID_PARAMETER;

	msg.LoadNewCmd(SONORK_CTRLMSG_CMD_OPEN
		,usr_param
		,usr_flags
		,SONORK_CTRLMSG_SF_TYPE_CIRCUIT
		|SONORK_CTRLMSG_SF_FLOW_REQ);

	handle->systemId	=
	msg.body.n.systemId	= GenTrackingNo(handle->userId);
	msg.body.n.serviceId	= handle->serviceId;

	result = Service_SendCtrlMsg(service_entry
			,handle
			,&msg
			,data_ptr
			,data_size
			,SONORK_CTRLMSG_SF_TYPE_CIRCUIT
			|SONORK_CTRLMSG_SF_FLOW_REQ);

	if( result == SONORK_RESULT_OK )
	{
		SONORK_MEM_NEW(circuit=new TSonorkServiceCircuitEntry);
		circuit->entry	=service_entry;
		circuit->handle.Set( *handle );
		circuit->flags	=SONORK_APP_SERVICE_CF_ACTIVE
				|SONORK_APP_SERVICE_CF_OPEN_BY_LOCAL
				|SONORK_APP_SERVICE_CF_PENDING;
		circuit->max_msecs	= timeout_msecs;
		circuit->cur_msecs	= 0;
		if( tag != NULL )
			circuit->tag.Set(*tag);
		else
			circuit->tag.Clear();

		service.circuits.Add( circuit );
		service.pending_circuits.Add( circuit );
		TRACE_DEBUG("  svc_open:%x:S=%x T=%x.%x SZ=%u P=%x"
		, circuit->handle.systemId
		, source_instance
		, circuit->handle.serviceId
		, circuit->handle.instance
		, data_size
		, circuit);

	}
	return result;
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_OpenCircuit(
			  DWORD			source_instance
			, TSonorkServiceCircuit*handle
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, DWORD			timeout_msecs
			, const TSonorkCodecAtom*A
			, SONORK_DWORD2*	tag)
{
	TSonorkDynData	DD;
	if( A )A->CODEC_Write(&DD);
	return Service_OpenCircuit(
			  source_instance
			, handle
			, cmd_param
			, cmd_flags
			, timeout_msecs
			, DD.Buffer()
			, DD.DataSize()
			, tag);

}
// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_StartQuery(
			  DWORD			source_instance
			, TSonorkServiceQuery*	handle
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		usr_param
			, DWORD 		usr_flags
			, DWORD			timeout_msecs
			, const void*		data_ptr
			, DWORD			data_size
			, SONORK_DWORD2*	tag)
{
	TSonorkServiceEntry*		service_entry;
	TSonorkCtrlMsg			msg;
	SONORK_RESULT			result;
	TSonorkServiceQueryEntry*	Q;

	service_entry=Service_GetEntry(SONORK_SERVICE_ID_NONE, source_instance , true);
	if(service_entry==NULL)
		return SONORK_RESULT_INVALID_PARAMETER;

	msg.LoadNewCmd(cmd
		,usr_param
		,usr_flags
		,SONORK_CTRLMSG_SF_TYPE_QUERY
		|SONORK_CTRLMSG_SF_FLOW_REQ
		);

	handle->systemId	=
	msg.body.n.systemId 	= GenTrackingNo(handle->userId);
	msg.body.n.serviceId	= handle->serviceId;


	result = Service_SendCtrlMsg(service_entry
			,handle
			,&msg
			,data_ptr
			,data_size
			,SONORK_CTRLMSG_SF_TYPE_QUERY
			|SONORK_CTRLMSG_SF_FLOW_REQ);

	if( result == SONORK_RESULT_OK )
	{
		SONORK_MEM_NEW(Q=new TSonorkServiceQueryEntry);
		Q->entry	= service_entry;
		Q->user_id.Set(handle->userId);
		Q->query_id	= handle->systemId;
		Q->flags	= SONORK_APP_SERVICE_QF_ACTIVE;
		Q->max_msecs	= timeout_msecs;
		Q->cur_msecs	= 0;
		if( tag )
			Q->tag.Set( *tag );
		else
			Q->tag.Clear();
		service.queries.Add( Q );
		TRACE_DEBUG("  svc_query:%x:S=%x T=%x C=%u:%u Z=%u P=%x"
		, Q->query_id
		, source_instance
		, handle->instance
		, cmd
		, usr_param
		, data_size
		, Q
		);
	}
	return result;
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::Service_StartQuery(
			  DWORD			source_instance
			, TSonorkServiceQuery*	handle
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		usr_param
			, DWORD 		usr_flags
			, DWORD			timeout_msecs
			, const TSonorkCodecAtom* A
			, SONORK_DWORD2* 	tag)
{
	TSonorkDynData	DD;
	if( A )A->CODEC_Write(&DD);
	return Service_StartQuery(
			  source_instance
			, handle
			, cmd
			, usr_param
			, usr_flags
			, timeout_msecs
			, DD.Buffer()
			, DD.DataSize()
			, tag);
}

// ----------------------------------------------------------------------------
SONORK_RESULT
 TSonorkWin32App::Service_Reply(
			  SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const void*		data_ptr
			, DWORD			data_size)
{
	TSonorkCtrlMsg		msg;

	// Is there a remote query active?
	if( service.active_type==SONORK_CTRLMSG_SF_TYPE_NONE)
		return SONORK_RESULT_INVALID_OPERATION;

	TRACE_DEBUG(" svc_reply:%x:S=%x T=%x C=%u:%u,SZ=%u"
		, service.event.cur_msg.msg->body.n.systemId
		, service.active_entry->sii.ServiceInstance()
		, service.event.cur_msg.sender.instance
		, cmd
		, cmd_param
		, data_size
		);
	if( service.event.cur_msg.sender.instance == 0 )
	{
		return SONORK_RESULT_INVALID_OPERATION;
	}

	// Services should start not reply queries by calling this method
	// directly: They should call StartQuery or ReplyQuery

	msg.LoadNewCmd(cmd
		,cmd_param
		,cmd_flags
		,service.active_type
		);
	msg.body.n.systemId = service.event.cur_msg.msg->body.n.systemId;

	return Service_SendCtrlMsg(
		  service.active_entry
		,&service.event.cur_msg.sender
		,&msg
		,data_ptr
		,data_size
		,service.active_task_type);

}

// ----------------------------------------------------------------------------
SONORK_RESULT
 TSonorkWin32App::Service_Reply(
			  SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom*A)
{
	TSonorkDynData	DD;
	if( A )A->CODEC_Write(&DD);

	return Service_Reply(
			  cmd
			, cmd_param
			, cmd_flags
			, DD.Buffer()
			, DD.DataSize());
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkWin32App::Service_DispatchEvent(
			  TSonorkServiceEntry*		E
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data)
{
	if(E->flags&SONORK_APP_SERVICE_EF_REGISTERED)
	{
		Service_DispatchEvent_Entry=E;
		return E->cb_ptr(E->cb_tag
				,event_id
				,event_tag
				,event_data
				);
	}
	return 0;
}

// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkWin32App::ServiceCallback_Sonork(SONORK_DWORD2&	handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			//event_tag
			,TSonorkAppServiceEvent*	E)
{
	TSonorkWin32App*_this;

	_this=(TSonorkWin32App*)handler_tag.v[0];
	switch(event_id)
	{

// -------------------------

	case SONORK_APP_SERVICE_EVENT_GET_NAME:
			E->get_name.str->Set("Core locator");
	return 1;

	
// -------------------------
	case SONORK_APP_SERVICE_EVENT_QUERY_REQ:
	switch(E->query_req.Cmd())
	{
		case SONORK_APP_CTRLMSG_QUERY_PIC:
			_this->SonorkService_QueryPic(E);
		break;
		case SONORK_APP_CTRLMSG_QUERY_SERVICES:
			_this->SonorkService_QueryServices(E);
		break;

		default:
			_this->Service_Reply(
				  MakeSonorkCtrlMsgCmd(
					E->query_req.Cmd()
					,SONORK_CTRLMSG_CF_RESULT)
				, SONORK_RESULT_NOT_SUPPORTED
				, 0, NULL, 0);
		break;
	}

	break;

	}
	return 0;
}

void
 TSonorkWin32App::SonorkService_QueryServices(TSonorkAppServiceEvent* msgE)
{
	TSonorkListIterator	I;
	TSonorkServiceEntry* 	entry;
	TSonorkUserServer	server;
	TSonorkAppServiceEvent	subE;

	memcpy(&subE.cur_msg,&msgE->cur_msg,sizeof(subE.cur_msg));

	subE.get_server.data 		 = &server;

	service.entries.BeginEnum(I);
	while( (entry=service.entries.EnumNext(I)) != NULL )
	{
		if( (entry->flags&(SONORK_APP_SERVICE_EF_HIDDEN|SONORK_APP_SERVICE_EF_REGISTERED))
			!= SONORK_APP_SERVICE_EF_REGISTERED
		||  entry->sii.ServiceId() == SONORK_SERVICE_ID_SONORK)
			continue;

		server.Clear();
		if(Service_DispatchEvent(
			  entry
			, SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA
			, NULL
			, &subE
			))
		{
			Service_Reply(
				MakeSonorkCtrlMsgCmd(SONORK_APP_CTRLMSG_QUERY_SERVICES
					, SONORK_CTRLMSG_CF_DATA)
					, 0			// usr_param
					, 0			// usr_flags
					, &server
					);


		}
	}
	service.entries.EndEnum(I);

	/*if( !Service_DispatchEvent(E
			,SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA
			,NULL
			,&event))
	{
		return SONORK_RESULT_INTERNAL_ERROR;

	}
	*/


}

void
 TSonorkWin32App::SonorkService_QueryPic(TSonorkAppServiceEvent* )
{
	TSonorkTempBuffer	buffer(SONORK_USER_PIC_MAX_BYTES);
	SONORK_FILE_HANDLE	handle;
	DWORD			pic_size;
	SONORK_RESULT		result;
	SonorkApp.MakeProfileFilePath(buffer.CStr(),"pic.zip");
	handle = SONORK_IO_OpenFile_ReadOnly( buffer.CStr() );
	if( handle == SONORK_INVALID_FILE_HANDLE )
	{
		result=SONORK_RESULT_NO_DATA;
		goto exit_00;
	}
	if( SONORK_IO_GetFileSize( handle , &pic_size ) != 0)
	{
		result = SONORK_RESULT_STORAGE_ERROR;
		goto exit_01;
	}
	if( pic_size > SONORK_USER_PIC_MAX_BYTES)
	{
		result=SONORK_RESULT_CONFIGURATION_ERROR;
		goto exit_01;
	}
	if( SONORK_IO_ReadFile(handle, buffer.Ptr(), pic_size) != 0 )
	{
		result = SONORK_RESULT_STORAGE_ERROR;
		goto exit_01;
	}
	Service_Reply(
		MakeSonorkCtrlMsgCmd(SONORK_APP_CTRLMSG_QUERY_PIC
			, SONORK_CTRLMSG_CF_ACCEPT)
			, pic_size		// usr_param
			, 0			// usr_flags
			, NULL
			, 0
			);
	Service_Reply(
		MakeSonorkCtrlMsgCmd(SONORK_APP_CTRLMSG_QUERY_PIC
			, SONORK_CTRLMSG_CF_DATA)
			, 0			// usr_param
			, 0			// usr_flags
			, buffer.Ptr()
			, pic_size
			);
	result=SONORK_RESULT_OK;
exit_01:
	SONORK_IO_CloseFile(handle);
exit_00:
	if( result!=SONORK_RESULT_OK)
	{
		Service_Reply(
			MakeSonorkCtrlMsgCmd(SONORK_APP_CTRLMSG_QUERY_PIC,SONORK_CTRLMSG_CF_RESULT)
				, result		// usr_param
				, 0			// usr_flags
				, NULL
				, 0
				);
	}
}
SONORK_C_CSTR
 TSonorkWin32App::Service_GetDescription(
		const TSonorkServiceInstanceInfo&	sii
		, SONORK_C_CSTR*        ptype
		, SKIN_ICON*		picon)

{
	SKIN_ICON	icon;
	SONORK_C_CSTR	str;


	switch( sii.ServiceId() )
	{
		default:
			str="<extern>";
			icon=SKIN_ICON_QUESTION_MARK;
			break;

		case SONORK_SERVICE_ID_INVALID:
		case SONORK_SERVICE_ID_NONE:
			str="??";
			icon=SKIN_ICON_QUESTION_MARK;
			break;

		case SONORK_SERVICE_ID_SONORK:
			str=szSONORK;
			icon=SKIN_ICON_SONORK;
			break;

		case SONORK_SERVICE_ID_IPC:
			str="IPC";
			icon=SKIN_ICON_IPC;
			break;

		case SONORK_SERVICE_ID_EXTERNAL_APP:
			str="Ext.Apps/ASP";
			icon=SKIN_ICON_APP;
			break;

		case SONORK_SERVICE_ID_SONORK_CHAT:
			str = SonorkApp.LangString(GLS_CR_NAME);
			icon=SKIN_ICON_CHAT;
			break;

		case SONORK_SERVICE_ID_SONORK_TRACKER:
			str = SonorkApp.LangString(GLS_TR_NAME);
			icon = SKIN_ICON_TRACKER;
			break;

		case SONORK_SERVICE_ID_DATA_SERVER:
			str = SonorkApp.LangString(GLS_LB_FSVR);
			icon = SKIN_ICON_FILE;
			break;
	}
	if(picon)*picon=icon;
	if(ptype)
	{
		const DWORD v=sii.ServiceInstance();
		if( v  & SONORK_SERVICE_INSTANCE_F_SERVER )
			*ptype="Server";
		else
		if( v & SONORK_SERVICE_INSTANCE_F_LOCATOR )
			*ptype="Locator";
		else
			*ptype="Client";
	}
	return str;

}

