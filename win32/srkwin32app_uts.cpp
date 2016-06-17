#include "srkwin32app.h"
#include "srkappstr.h"
#pragma hdrstop
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

BOOL
 TSonorkWin32App::UTS_MayActAsServer()	const
{
	return !( profile.ctrl_data.header.flags.Test(SONORK_PCF_USING_PROXY)
		|| sonork.UsingSocks() );
}

void
 TSonorkWin32App::UTS_SetLink(const TSonorkId& userId
	, SONORK_UTS_LINK_ID linkId
	, TSonorkExtUserData*UD
	, BOOL update_ui)
{
	char tmp[24];
	SONORK_C_CSTR	pAlias;
	if(UD)
	{
		pAlias=UD->display_alias.CStr();
		TRACE_DEBUG(
			"App:SetUtsLink(%u.%u , '%s' , %08x -> %08x)"
			, userId.v[0]
			, userId.v[1]
			, pAlias
			, UD->run_data.uts_link_id
			, linkId
			);
		if(UD->run_data.uts_link_id == linkId)
			return;
		UD->run_data.uts_link_id = linkId;
	}
	else
	{
		pAlias=tmp;
		userId.GetStr( tmp );
		TRACE_DEBUG(
			"App:SetUtsLink(%s , %08x)"
			,tmp
			,linkId);
	}
	if( CxStatus() >= SONORK_APP_CX_STATUS_READY )
	{
		BroadcastAppEvent( SONORK_APP_EVENT_USER_UTS
			,SONORK_APP_EM_USER_LIST_AWARE
			,linkId
			,(void*)&userId);
		if(userId != ProfileUserId())
		{
			if( update_ui )
			{
				Set_UI_Event(
					linkId != SONORK_INVALID_LINK_ID
						?SONORK_UI_EVENT_UTS_LINKED
						:SONORK_UI_EVENT_UTS_UNLINKED
					, pAlias
					, 0);
			}
			Service_OnUserUtsLink(userId,linkId);
		}
	}
}

void
 TSonorkWin32App::UTS_Event_Warning(TSonorkUTSEvent*E)
{
	char tmp[128];
	char *str;
	strcpy(tmp,inet_ntoa(E->LinkPhysAddr().data.inet1.addr.sin_addr));
	str=tmp+strlen(tmp);
	*str++=' ';

	switch(E->Warning())
	{
		case SONORK_UTS_WARNING_SOCKS_DENIED:
			strcpy(str,"SOCKS denied connect request");
			break;

		case SONORK_UTS_WARNING_AUTH_TIMEOUT:
			strcpy(str,"did not authenticate within allowed period");
			break;
		case SONORK_UTS_WARNING_PROTOCOL:
			strcpy(str,"is presenting bogus behaviour");
			break;
		default:
		return;

	}

	SysLog( SONORK_UI_EVENT_WARNING
		, SKIN_ICON_SECURITY_WARN
		, tmp
		, false
		, false
		);
}
void
 TSonorkWin32App::UTS_Event_SidPin(TSonorkUTSEvent*E)
{
	E->SetLoginPinType(SONORK_PIN_TYPE_64);
	EncodePin64(
		 E->LoginPin()
		,E->LinkUserId()
		,E->LinkServiceId()
		,0
		);
}
void
 TSonorkWin32App::UTS_Event_Login(TSonorkUTSEvent*E)
{
	UINT 	A_size,P_size;
	TSonorkTag			tag;
	TSonorkDataPacket *	P;
	TSonorkError		ERR;
	TSonorkExtUserData*	UD;
	A_size = 128;
	TRACE_DEBUG("TSonorkWin32App::UtsLogin(%x,%u.%u,SVC=%x)"
		,E->LinkId()
		,E->LinkUserId().v[0]
		,E->LinkUserId().v[1]
		,E->LinkServiceId()
		);
	if( E->LinkServiceId() == SONORK_SERVICE_ID_NONE
	||  E->LinkServiceId() == SONORK_SERVICE_ID_SONORK)
	{
		UD = UserList().Get( E->LinkUserId() );
		if( UD != NULL )
		{
			if( UD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
			if( UD->TestCtrlFlag(SONORK_UCF_SONORK_UTS_DISABLED) )
			{
				ERR.SetSys(SONORK_RESULT_NOT_ACCEPTED
					, GSS_OPNOTALLOWED
					, SONORK_MODULE_LINE);
				E->SetLoginAuthorizationDenied(ERR);
				return;
			}
		}

	}

	tag.v[0]=E->LinkId();
	tag.v[1]=0;
	P=SONORK_AllocDataPacket( A_size );
	P_size = P->E_IdentifyUser_R(A_size
				,E->LinkUserLocus()
				,E->LoginPin()
				,E->LoginPinType()
				,E->LinkServiceId()
				,0
				);
	sonork.Request(ERR,P,P_size,SonorkClientRequestHandler,this,&tag);
	SONORK_FreeDataPacket( P );
	if(ERR.Result() == SONORK_RESULT_OK)
		E->SetLoginAuthorizationPending();
	else
		E->SetLoginAuthorizationDenied(ERR);
}
void
 TSonorkWin32App::UTS_Event_Status(TSonorkUTSEvent*E)
{
	TSonorkExtUserData	*UD;
	if( E->LinkStatus() == SONORK_NETIO_STATUS_CONNECTED
	||  E->LinkStatus() == SONORK_NETIO_STATUS_DISCONNECTED
	||  E->LinkStatus() == SONORK_NETIO_STATUS_DISCONNECTING)
	if( !E->LinkUserId().IsZero() && !(E->LinkUserId() == ProfileUserId()) )
	{
		UD = GetUser(E->LinkUserId());
		UTS_SetLink(E->LinkUserId()
			,E->LinkStatus() == SONORK_NETIO_STATUS_CONNECTED?E->LinkId():0
			,UD
			,true);
	}
}
void
 TSonorkWin32App::UTS_Event_Msg(
	TSonorkUTSEvent*gutsE
	,TSonorkUTSPacket*P
	, UINT P_size)
{
	TSonorkMsg	msg;
	SONORK_RESULT	result;
//	TRACE_DEBUG("TSonorkWin32App::UtsMsg(P_size=%u)",P_size);
	result = P->D_Atom(P_size,&msg);
	if( result == SONORK_RESULT_OK )
	{
		// Make sure the message header is correct:
		// Set the User Id of the sender, load the local time
		// and strip off any flags that only a server could have set
		msg.header.userId.Set(gutsE->LinkUserId());
		msg.header.time.SetTime_Local();
		msg.header.sysFlags&=~SONORK_MSG_SFM_SERVER_CONTROLLED;

		ConsumeMsgEvent(SONORK_APP_EVENT_SONORK_MSG_RCVD
			, &msg
			, SONORK_APP_MPF_UTS|SONORK_APP_MPF_ADD_USER
			, 0
			, NULL
			, 0
			, 0);


	}
	else
		TRACE_DEBUG("TSonorkWin32App::UtsMsg::TGP_D_Atom() failed (%s)"
			,SONORK_ResultName(result));
}

void
 TSonorkWin32App::UTS_Event_CtrlMsg(
	  TSonorkUTSEvent*gutsE
	, TSonorkUTSPacket*P
	, UINT P_size)
{
	TSonorkCtrlMsg 		msg;
	DWORD			data_size;
	BYTE*			data;

	data = P->D_CtrlMsg( P_size, &msg, &data_size);
	if( data )
	{
		Service_ProcessCtrlMsg(
			 &gutsE->LinkUserLocus()
			,gutsE->LinkId()
			,&msg
			,data
			,data_size);
	}
}

void SONORK_CALLBACK
	TSonorkWin32App::SonorkUtsEventHandler(void*param,struct TSonorkUTSEvent*E)
{
	TSonorkWin32App *_this=(TSonorkWin32App*)param;

	switch(E->Event())
	{
		case SONORK_UTS_EVENT_SID_PIN:
			_this->UTS_Event_SidPin(E);
		break;

		case SONORK_UTS_EVENT_LOGIN:
			_this->UTS_Event_Login(E);
		break;

		case SONORK_UTS_EVENT_STATUS:
			_this->UTS_Event_Status(E);
		break;

		case SONORK_UTS_EVENT_WARNING:
			_this->UTS_Event_Warning(E);
		break;

		case SONORK_UTS_EVENT_DATA:
			switch( E->DataPacketCmd() )
			{
				case SONORK_UTS_CMD_MSG:
					_this->UTS_Event_Msg(E,E->DataPacket(),E->DataPacketSize());
				break;

				case SONORK_UTS_CMD_CTRL_MSG:
					_this->UTS_Event_CtrlMsg(E,E->DataPacket(),E->DataPacketSize());
				break;

				default:
					// UNKNOWN COMMAND

				break;
			}
		break;

		case SONORK_UTS_EVENT_CLEAR_TO_SEND:
		default:
		break;
	}
}


SONORK_UTS_LINK_ID
 TSonorkWin32App::UTS_GetLink(const TSonorkId&user_id)
{
	if( win32.uts_server == NULL || !CxReady() )
		return 0;
	return win32.uts_server->FindLink( user_id
			, SONORK_SERVICE_ID_NONE
			, 0
			, SONORK_SERVICE_ID_SONORK
			, 0
			);
}
SONORK_RESULT
	TSonorkWin32App::UTS_ConnectByLocus(TSonorkUserLocus1*locus
		, TSonorkPhysAddr*phys_addr,TSonorkError*pERR)
{
	TSonorkError		ERR;
	if( win32.uts_server == NULL || !CxReady())
		ERR.SetOk();
	else
	{
		SONORK_UTS_LINK_ID 	link_id;
		for(;;)
		{
			if( phys_addr == NULL || locus == NULL )
			{
				ERR.SetSys(SONORK_RESULT_INVALID_PARAMETER,GSS_BADADDR,GSS_BADADDR);
				break;
			}

			link_id =win32.uts_server->FindLink(
					  locus->userId
					, SONORK_SERVICE_ID_NONE
					, 0
					, SONORK_SERVICE_ID_SONORK
					, 0
					);

			if(  link_id != SONORK_INVALID_LINK_ID )
			{
				win32.uts_server->ResetLinkTimeout( link_id );
				ERR.Set(SONORK_RESULT_OK,"Already connected",0,true);
				break;
			}
			if(!phys_addr->IsValid(SONORK_PHYS_ADDR_TCP_1))
			{
				ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE,GSS_BADADDR,GSS_BADADDR);
				break;
			}
			else
			{
				TSonorkUTSDescriptor D;
				D.locus.Set(*locus);
				D.serviceId	= SONORK_SERVICE_ID_SONORK;
				D.instance	= 0;
				D.flags		= SONORK_UTS_KERNEL_APP_FLAGS;
				win32.uts_server->ConnectToUts(ERR
					, &D
					, *phys_addr
					, SONORK_UTS_LINK_F_MUST_AKN);
				break;
			}
		}
	}
	if(pERR)pERR->Set(ERR);
	return ERR.Result();
}

SONORK_RESULT
 TSonorkWin32App::UTS_ConnectByUserData(
		TSonorkExtUserData*	UD
		,bool 		    	forced
		,TSonorkError*		pERR)
{
	TSonorkError		ERR;
	if( win32.uts_server == NULL || !CxReady() )
		ERR.SetOk();
	else
	{
		for(;;)
		{
			if(UD->UtsLinkId() != SONORK_INVALID_LINK_ID)
			{
				if( win32.uts_server->ResetLinkTimeout( UD->UtsLinkId() ) )
				{
					ERR.Set(SONORK_RESULT_OK,"Already connected",0,true);
					break;
				}
				
				// guts did not find the Uts Link for the user:
				//  Update the user data
				UTS_SetLink(UD->userId
					,SONORK_INVALID_LINK_ID
					,UD
					,true);
			}

			if( !forced )
			{
				if( !AmIVisibleToUser(UD) )
				{
					ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE,GSS_OPNOTALLOWED,GSS_OPNOTALLOWED);
					break;
				}
			}

			if( UD->addr.sidFlags.SidMode() == SONORK_SID_MODE_DISCONNECTED )
			{
				ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE,GSS_NOTCXTED,GSS_NOTCXTED);
				break;
			}

			if(!UD->addr.sidFlags.UtsServerEnabled()
			|| (UD->UserType() == SONORK_USER_TYPE_AUTHORIZED
				&& UD->TestCtrlFlag(SONORK_UCF_SONORK_UTS_DISABLED)) )
			{
				ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE
					,GSS_OPNOTALLOWED
					,SONORK_MODULE_LINE
					);
				break;
			}

			if(!UD->addr.physAddr.IsValid(SONORK_PHYS_ADDR_TCP_1))
			{
				ERR.SetSys(SONORK_RESULT_NOT_AVAILABLE,GSS_BADADDR,GSS_BADADDR);
				break;
			}
			else
			{
				TSonorkUTSDescriptor D;
				D.serviceId	= SONORK_SERVICE_ID_SONORK;
				D.instance	= 0;
				D.flags		= SONORK_UTS_KERNEL_APP_FLAGS;
				UD->GetLocus1(&D.locus);
				win32.uts_server->ConnectToUts(ERR
					, &D
					, UD->addr.physAddr
					, SONORK_UTS_LINK_F_MUST_AKN);
				break;
			}
		}
	}
	sonork_printf("ConnectUtsByUserData: %s %s"
		,ERR.ResultName()
		,ERR.Text().CStr());
	if(pERR)pERR->Set(ERR);
	return ERR.Result();
}
