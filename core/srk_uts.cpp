#include "srk_defs.h"
#pragma hdrstop
#include "srk_uts.h"
#include "srk_uts_login_packet.h"

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


#define	SONORK_UTS_MONITOR_SECS				10
#define SONORK_UTS_LINK_MAX_CONNECT_SECS		30//60
#define SONORK_UTS_LINK_MAX_AUTHORIZE_SECS		40//70
#define SONORK_UTS_LINK_DEFAULT_MAX_IDLE_SECS		600//1200
#define SONORK_UTS_MAX_SEND_SIZE                        2048
#define SONORK_UTS_LINK_MAX_WRITE_DISABLED_SECS		90
#define SONORK_UTS_SEND_BUFFER_SIZE			(SONORK_UTS_MAX_SEND_SIZE+16)

inline void
 TSonorkUTS::InvokeEventHandler(TSonorkUTSEvent*E)
{
	if(!TestUtsFlag(SONORK_UTS_SERVER_F_NO_CALLBACK))
		main.event_handler(main.handler_param,E);
}

int
 TSonorkUTS::DoListen(TSonorkUTSLinkEx*LINK,DWORD lo_port, DWORD hi_port)
{
	UINT		err_code;
	sockaddr_in	SIN;
	WORD		cur_port;
#if defined(SONORK_WIN32_BUILD)
	int 		addr_len;
#elif defined(SONORK_LINUX_BUILD)
	socklen_t 	addr_len;
#else
#error NO IMPLEMENTATION
#endif

	LINK->net.sk=CreateSocket(&err_code,false);
	if(LINK->Socket()==INVALID_SOCKET)
		return err_code;
	cur_port=(WORD)lo_port;
	for(;;)
	{
		LINK->phys_addr.Inet1()->sin_port = htons(cur_port);
		if(::bind(LINK->Socket()
			,(sockaddr*)LINK->phys_addr.Inet1()
			, sizeof (sockaddr_in))==SOCKET_ERROR)
		{
			err_code=WSAGetLastError();
			if( err_code == WSAEADDRINUSE )
			{
				if( cur_port!=0 && (DWORD)cur_port<hi_port)
				{
					cur_port++;
					continue;
				}

			}
			return err_code;
		}
		else
			break;
	}

	if( ::listen(LINK->Socket(),5) == SOCKET_ERROR )
	{
		err_code=WSAGetLastError();
		return err_code;
	}

	addr_len=sizeof(sockaddr_in);
	err_code=getsockname(LINK->Socket(),(sockaddr*)&SIN,&addr_len);

	LINK->phys_addr.Inet1()->sin_port = SIN.sin_port;

	SetStatus(LINK
		,SONORK_NETIO_STATUS_LISTENING
		,TSonorkErrorOk());
	SetFdSetDirty();
	return 0;

}

SONORK_UTS_LINK_ID
	TSonorkUTS::Startup(
			TSonorkError&			ERR
			,const TSonorkUTSDescriptor&descriptor
			,lpfnUtsEventHandler		handler
			,void* 	     			handler_param
			,UINT 	     			link_flags
			,DWORD				address
			,DWORD				lo_port
			,DWORD				hi_port
			)
{
	TSonorkUTSLinkEx*	L;
	int 			err_code;

	timeslot_clock.LoadCurrent();
	monitor_clock.Set(timeslot_clock);

	main.event_handler=handler;
	main.handler_param=handler_param;
	link_flags&=SONORK_UTS_LINK_FM_STARTUP;
	if( (L=AllocLink(SONORK_UTS_LINK_TYPE_LISTEN|link_flags))==NULL )
	{
		ERR.SetSys(SONORK_RESULT_OUT_OF_RESOURCES , GSS_NETCXERR , SONORK_MODULE_LINE );
		return SONORK_INVALID_LINK_ID;
	}

	memcpy( &L->descriptor , &descriptor , sizeof(TSonorkUTSDescriptor) );

	L->phys_addr.header.type		= SONORK_PHYS_ADDR_TCP_1;
	L->phys_addr.Inet1()->sin_family 	= AF_INET;
	L->phys_addr.Inet1()->sin_addr.s_addr	= address ;
	memset(L->phys_addr.Inet1()->sin_zero,0,sizeof(L->phys_addr.Inet1()->sin_zero));
	if( !(link_flags & SONORK_UTS_LINK_F_NO_LISTEN) )
	{
		err_code = DoListen(L, lo_port , hi_port );//phys_addr.GetInet1Port());
	}
	else
	{
		L->phys_addr.Inet1()->sin_port   = 0;
		err_code = 0;
	}
	if(!err_code)
	{
		main.link=L;

		ERR.SetOk();
		return L->Id();
	}
	else
	{
		ERR.SetSys(SONORK_RESULT_NETWORK_ERROR
			,GSS_NETERR
			,err_code);
	}
	main.link=NULL;
	main.handler_param=NULL;
	main.event_handler=NULL;
	DestroyLink(L->Id());
	return SONORK_INVALID_LINK_ID;
}

void
 TSonorkUTS::EncodeSidPin(SONORK_DWORD4& pin, DWORD& pin_type,TSonorkUTSLinkEx*LINK)
{
	TSonorkUTSEvent event(SONORK_UTS_EVENT_SID_PIN,LINK,NULL);
	event.D.login.pin.Clear();
	InvokeEventHandler(&event);
	pin.Set(event.D.login.pin);
	pin_type = event.D.login.pin_type;
}

void
 TSonorkUTS::ProcessInPacket(TSonorkUTSLinkEx*LINK)
{
	TSonorkTcpPacket *tcp_packet;
	TSonorkUTSPacket *P;
	DWORD	P_size;
	bool 	invalid_packet=false;


	tcp_packet	= LINK->net.i_packet;

	ProcessDataAkn(LINK, tcp_packet->AknCount() );

	P 			= (TSonorkUTSPacket *)tcp_packet->DataPtr();
	P_size		= tcp_packet->DataSize();
	switch( tcp_packet->Type() )
	{
		case SONORK_NETIO_HFLAG_TYPE_PACKET:
			if( P_size >= sizeof(TSonorkUTSPacket) )
			{
				if( LINK->Flags() & SONORK_UTS_LINK_F_MUST_AKN  )
					LINK->net.pending_akns++;
				/*
				sonork_printf("Uts[%08x] F:%08x PACKET, akns{ToRecv:%u ToSend:%u}"
					,LINK->Id()
					,LINK->Flags()
					,LINK->net.akn_queue.Items()
					,LINK->net.pending_akns
					);
				*/
				ProcessDataPacket(LINK,P,P_size);
			}
			else
				invalid_packet = true;
			break;


		case SONORK_NETIO_HFLAG_TYPE_CONNECT:
			if( P_size >= sizeof(TSonorkUTSPacket) )
			{
				ProcessLoginReq(LINK,P,P_size);
			}
			else
				invalid_packet = true;
			break;

		case SONORK_NETIO_HFLAG_TYPE_LOGIN:
			if( P_size >= sizeof(TSonorkUTSPacket) )
			{
				ProcessLoginAkn(LINK,P,P_size);
			}
			else
				invalid_packet = true;
			break;

		case SONORK_NETIO_HFLAG_TYPE_NONE:
			break;

		default:
			invalid_packet = true;
				break;
	}
	if( invalid_packet )
	{
		if( (!(LINK->flags&SONORK_UTS_LINK_F_WARNED)) && (LINK->Status()!=SONORK_NETIO_STATUS_CONNECTED))
		{
			LINK->flags|=SONORK_UTS_LINK_F_WARNED;
		}
	}
	Sonork_FreeTcpPacket( tcp_packet );
	LINK->net.i_packet=NULL;
	LINK->net.i_offset=0;
}

void  TSonorkUTS::ProcessDataAkn(TSonorkUTSLinkEx*LINK,DWORD packets)
{
	TSonorkUTSOEntry* E;
	if( !(LINK->Flags() & SONORK_UTS_LINK_F_MUST_AKN ) )
		return;
	
	while( packets-- )
		if( (E=LINK->net.akn_queue.RemoveFirst())!=NULL )
			EndOutputEntryTx(LINK, E, NULL);

}
void  TSonorkUTS::ProcessDataPacket(TSonorkUTSLinkEx*LINK,TSonorkUTSPacket*P,UINT P_size)
{
	TSonorkUTSEvent	event(SONORK_UTS_EVENT_DATA,LINK,NULL);
	TSonorkCryptContext& CC=LINK->CryptContext();

	if(CC.SingleBufferEncryptionEnabled())
	{
		event.D.data.packet			=P;
		event.D.data.packet_size	=P_size;
		CC.SingleBufferUncrypt(event.D.data.packet,event.D.data.packet_size);
		event.D.data.packet_cmd 	= P->Cmd();
		InvokeEventHandler(&event);
	}
	else
	{

		event.D.data.packet_size	=CC.MaxUncryptedSize(P_size);
		event.D.data.packet			=SONORK_MEM_ALLOC(TSonorkUTSPacket,event.D.data.packet_size);
		CC.DoubleBufferUncrypt(P,P_size,event.D.data.packet,&event.D.data.packet_size);

		event.D.data.packet_cmd 	= P->Cmd();
		InvokeEventHandler(&event);

		SONORK_MEM_FREE(event.D.data.packet);
	}
}




TSonorkUTS::TSonorkUTS(DWORD max_links)
{
	DWORD fd_set_size;
	if(max_links<2)max_links=2;
	link_table.max_entries=max_links;
	SONORK_MEM_NEW(link_table.ptr=new TSonorkUTSLinkEx*[max_links]);
	SONORK_ZeroMem(link_table.ptr,sizeof(TSonorkUTSLinkEx*)*max_links);

#if defined(SONORK_WIN32_BUILD)
	fd_set_size=(sizeof(u_int)*2)+(max_links*sizeof(SOCKET));
#elif defined(SONORK_LINUX_BUILD)
	fd_set_size=sizeof(fd_set);
#else
#error NO IMPLEMENTATION
#endif

	active_r_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);
	active_w_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);
	active_e_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);
	work_r_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);
	work_w_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);
	work_e_fd_set	=SONORK_MEM_ALLOC(fd_set,fd_set_size);

	FD_ZERO(active_r_fd_set);
	FD_ZERO(active_w_fd_set);
	FD_ZERO(active_e_fd_set);
	sockets_in_active_set=0;

	link_table.entries=0;
	link_table.cookie=0;
	uts_flags=0;
	monitor_interval_secs 	= SONORK_UTS_MONITOR_SECS;
	max_link_idle_secs	= SONORK_UTS_LINK_DEFAULT_MAX_IDLE_SECS;
	main.link=NULL;
	main.handler_param=NULL;
	main.event_handler=NULL;
	socks_info.Clear();
}

TSonorkUTS::~TSonorkUTS()
{
	uts_flags|=SONORK_UTS_SERVER_F_NO_CALLBACK;
	Shutdown();
	SONORK_MEM_DELETE_ARRAY(link_table.ptr);
	SONORK_MEM_FREE(active_r_fd_set);
	SONORK_MEM_FREE(active_w_fd_set);
	SONORK_MEM_FREE(active_e_fd_set);
	SONORK_MEM_FREE(work_r_fd_set);
	SONORK_MEM_FREE(work_w_fd_set);
	SONORK_MEM_FREE(work_e_fd_set);
}



void
 TSonorkUTS::SetCondemned( TSonorkUTSLinkEx* LINK  )
{
	if(!LINK->IsCondemned())
	{
		assert(LINK->status == SONORK_NETIO_STATUS_DISCONNECTED);
		assert(LINK->net.o_entry == NULL );
		assert(LINK->net.o_queue.Items() == 0);
		assert(LINK->net.akn_queue.Items() == 0);
		LINK->flags|=SONORK_UTS_LINK_F_CONDEMNED;
		del_queue.Add(LINK);
		SetQueueDirty();
	}
}
void
	TSonorkUTS::SetClearToSend(TSonorkUTSLinkEx*LINK)
{
	//sonork_printf("Uts[ID:%08x F:%08x] Clear to Send",LINK->Id(),LINK->flags);
	if( !LINK->IsClearToSend() )
	{
		if( LINK->IsDataPending() )
			assert( !LINK->IsDataPending() );
		LINK->flags|=SONORK_UTS_LINK_F_CLEAR_TO_SEND;
		cts_queue.Add(LINK);
		SetQueueDirty();
//		sonork_printf("Uts[ID:%08x F:%08x] Entered CTS queue",LINK->Id(),LINK->flags);
	}
//	else		sonork_printf("Uts[ID:%08x F:%08x] Already in CTS queue",LINK->Id(),LINK->flags);
}

void
	TSonorkUTS::SetWriteEnabled( TSonorkUTSLinkEx* LINK )
{
//	sonork_printf("Uts[%08x] F:%08x Set write enabled",LINK->Id(),LINK->flags);
	if( !LINK->IsWriteEnabled() )
	{
		LINK->flags|=SONORK_UTS_LINK_F_WRITE_ENABLED;
		SetFdSetDirty();
	}
//	else		sonork_printf("Uts[%08x] F:%08x Already write enabled",LINK->Id(),LINK->flags);
	if( LINK->IsDataPending() )
		AddToWriteQueue( LINK );
	else
		SetClearToSend( LINK );

}
void
	TSonorkUTS::AddToWriteQueue( TSonorkUTSLinkEx* LINK )
{
	assert( LINK->IsWriteEnabled() );
	assert( LINK->IsDataPending() );
	if( !LINK->IsInWriteQueue() )
	{
		LINK->flags|=SONORK_UTS_LINK_F_IN_WRITE_QUEUE;
		write_queue.Add(LINK);
		SetQueueDirty();
	}
}

void
	TSonorkUTS::SetStatus(TSonorkUTSLinkEx*LINK
		, SONORK_NETIO_STATUS 	status
		, TSonorkError&		ERR)
{
	TSonorkUTSOEntry*	E;

	if( LINK->Status() != status )
	{
		LINK->SetStatus(status);
		LINK->net.last_request_clock.Set( timeslot_clock );
		if(status<=SONORK_NETIO_STATUS_DISCONNECTING)
		{
			while((E=LINK->net.akn_queue.RemoveFirst())!=NULL)
				EndOutputEntryTx(LINK,E,&ERR);
				
			if(LINK->net.o_entry)
			{
				EndOutputEntryTx(LINK,LINK->net.o_entry,&ERR);
				LINK->net.o_entry=NULL;
			}

			while((E=LINK->net.o_queue.RemoveFirst())!=NULL)
				EndOutputEntryTx(LINK,E,&ERR);


			if(status==SONORK_NETIO_STATUS_DISCONNECTING)
				LINK->SetStatus(SONORK_NETIO_STATUS_DISCONNECTED);

			SetCondemned( LINK );
		}

		InvokeEventHandler(&TSonorkUTSEvent(SONORK_UTS_EVENT_STATUS,LINK,&ERR));

		// If the status has changed to CONNECTED
		// the LINK is ready to send (a READY_TO_SEND event will be generated)
		if(status==SONORK_NETIO_STATUS_CONNECTED)
		{
			SetWriteEnabled(LINK);
		}
	}
}

void TSonorkUTS::TS_RebuildFdSet()
{
	DWORD index;
	TSonorkUTSLinkEx	**dR;
	TSonorkUTSLinkEx	*sR;

	FD_ZERO(active_r_fd_set);
	FD_ZERO(active_w_fd_set);
	FD_ZERO(active_e_fd_set);
#if defined(SONORK_LINUX_BUILD)
	max_sk_in_active_set=INVALID_SOCKET;
#endif
	sockets_in_active_set=0;
	for( index=0,dR=link_table.ptr
	    ;index<link_table.max_entries
	    ;index++,dR++)
	{
		if(*dR)
		{

			sR=*dR;
			if(sR->Status()==SONORK_NETIO_STATUS_DISCONNECTED)
				continue;
#if defined(SONORK_LINUX_BUILD)
			if(sR->Socket()>max_sk_in_active_set
			   ||max_sk_in_active_set==INVALID_SOCKET)
				max_sk_in_active_set=sR->Socket();
#endif
			sockets_in_active_set++;
			FD_SET(sR->Socket(),active_r_fd_set);
			if(sR->Status()==SONORK_NETIO_STATUS_CONNECTING)
			{
				FD_SET(sR->Socket(),active_w_fd_set);
				FD_SET(sR->Socket(),active_e_fd_set);
			}
			else
			if(sR->Status()!=SONORK_NETIO_STATUS_LISTENING)
			{
				if( !sR->IsWriteEnabled() )
				{
					FD_SET(sR->Socket(),active_w_fd_set);
				}
			}
		}
	}
	uts_flags&=~SONORK_UTS_SERVER_F_FD_SET_DIRTY;
}

int TSonorkUTS::TS_Select(DWORD to)
{
	struct timeval tv={0,SONORK_MsecsToSelectTimeval(to)};
	int rv;

	if( sockets_in_active_set == 0)
		return 0;

#if defined(SONORK_WIN32_BUILD)
	if((work_r_fd_set->fd_count=active_r_fd_set->fd_count)!=0)
		memcpy(work_r_fd_set->fd_array
			,active_r_fd_set->fd_array
			,active_r_fd_set->fd_count*sizeof(SOCKET));

	if((work_w_fd_set->fd_count=active_w_fd_set->fd_count)!=0)
		memcpy(work_w_fd_set->fd_array
			,active_w_fd_set->fd_array
			,active_w_fd_set->fd_count*sizeof(SOCKET));

	if((work_e_fd_set->fd_count=active_e_fd_set->fd_count)!=0)
		memcpy(work_e_fd_set->fd_array
			,active_e_fd_set->fd_array
			,active_e_fd_set->fd_count*sizeof(SOCKET));
#elif defined(SONORK_LINUX_BUILD)
	*work_r_fd_set=*active_r_fd_set;
	*work_w_fd_set=*active_w_fd_set;
	*work_e_fd_set=*active_e_fd_set;
#else
# error NO IMPLEMENTATION
#endif

	rv = select(
#if defined(SONORK_WIN32_BUILD)
		0
#elif defined(SONORK_LINUX_BUILD)
		max_sk_in_active_set+1
#endif
		,work_r_fd_set
		,work_w_fd_set
		,work_e_fd_set,&tv);

	return rv;
}
void
	TSonorkUTS::TS_Accept( TSonorkUTSLinkEx* acceptR )
{
	int aux;
	SOCKET sk;
	TSonorkUTSLinkEx*newR;
	sockaddr_in sin;
	aux=sizeof(sin);
	sk=::accept(acceptR->Socket(),(sockaddr*)&sin,&aux);
	if(sk==INVALID_SOCKET)
	{
		TSonorkErrorInfo ERR(SONORK_RESULT_NETWORK_ERROR
			,"Accept connection failed"
			,WSAGetLastError(),true);
		InvokeEventHandler(&TSonorkUTSEvent(SONORK_UTS_EVENT_ERROR,acceptR,&ERR));
	}
	else
	{
		//oliver
		newR=AllocLink(SONORK_UTS_LINK_TYPE_IN_CONNECT);
		if(!newR)
		{
			// Migs
			Sonork_Net_KillSocket(sk);
			return;
		}
		newR->net.sk=sk;
		Sonork_Net_SetBufferSize(sk,SONORK_UTS_SEND_BUFFER_SIZE,-1);

		newR->phys_addr.SetInet1(SONORK_PHYS_ADDR_TCP_1,&sin);

		newR->SetStatus(SONORK_NETIO_STATUS_AUTHORIZING);

		SetFdSetDirty();

		{
			TSonorkErrorOk 	ERR;
			TSonorkUTSEvent	event(SONORK_UTS_EVENT_ACCEPT,acceptR,&ERR);
			event.D.acc.link		=newR;
			InvokeEventHandler(&event);
		}
	}
}
void TSonorkUTS::TS_SendSocksRequest( TSonorkUTSLinkEx* LINK )
{
	UINT bytes;
	BYTE socks_request[SOCKS_V4_CONNECT_REQUEST_BUFFER_SIZE];

	LINK->flags|=SONORK_UTS_LINK_F_SOCKS_REQ_SENT;

	bytes = (UINT)BuildSocksV4ConnectRequest( socks_request, LINK->phys_addr );
	::send(LINK->Socket(), (const char*)socks_request, bytes ,0);

}
void TSonorkUTS::TS_RecvSocksRequest( TSonorkUTSLinkEx* LINK , int sz )
{
	int error;

	error = ParseSocksV4ConnectResponse( RecvBuffer(), (UINT)sz);
	if( error != 0)
	{
		DoLinkError(LINK
			, SONORK_RESULT_ACCESS_DENIED
			, GSS_NETCXSVRKILL
			, error
			, true
			, SONORK_UTS_WARNING_SOCKS_DENIED);
	}
	else
	{
		SendLoginReq(LINK);
	}

}

void TSonorkUTS::DoLinkError(TSonorkUTSLinkEx*LINK
	, SONORK_RESULT result
	, SONORK_SYS_STRING gss
	, int code
	, bool kill_socket
	, SONORK_UTS_WARNING warning)
{
	TSonorkError ERR;

	ERR.SetSys(result,gss,code);

	if( warning != SONORK_UTS_WARNING_NONE )
	{
		TSonorkUTSEvent event(SONORK_UTS_EVENT_WARNING
			, LINK
			, &ERR);
		event.D.warn.warning = warning;
		InvokeEventHandler(&event);
	}
	SetStatus(LINK
		, kill_socket
			?SONORK_NETIO_STATUS_DISCONNECTED
			:SONORK_NETIO_STATUS_DISCONNECTING
		,ERR);
}

bool TSonorkUTS::TimeSlot(UINT msecs)
{
	UINT 	sk_set_count;
	int  	sz;

	timeslot_clock.LoadCurrent() ;
	if(!MainLink())return false;
	assert( link_table.ptr != NULL );

	mutex.Lock( "TimeSlot" , SONORK_MODULE_LINE );
	if(TestUtsFlag(SONORK_UTS_SERVER_F_FD_SET_DIRTY))
		TS_RebuildFdSet();

	sk_set_count=TS_Select(msecs);
	if(sk_set_count>0)
	{
		UINT index;
		TSonorkUTSLinkEx	**pLink;
		TSonorkUTSLinkEx	*LINK;
		for(index=0
			,pLink=link_table.ptr
			;index<link_table.max_entries //&& sk_found_count<sk_set_count
			;index++,pLink++)
		if(*pLink)
		{
			LINK=*pLink;
			if( FD_ISSET( LINK->Socket() , work_r_fd_set ) )
			{
				if( LINK->Status() == SONORK_NETIO_STATUS_LISTENING)
				{
					TS_Accept( LINK );
				}
				else
				{
					sz=::recv( LINK->Socket()
						,(char*)RecvBuffer()
						,SONORK_TCPIO_DEFAULT_BUFFER_SIZE
						,0);
					if(sz)
					{
						if(sz==SOCKET_ERROR)
						{
							sz=WSAGetLastError();
							DoLinkError( LINK
								, SONORK_RESULT_FORCED_TERMINATION
								, GSS_NETCXLOST
								, sz
								, true
								, SONORK_UTS_WARNING_NONE);
						}
						else
						{
							if(LINK->Status()==SONORK_NETIO_STATUS_CONNECTING)
							{
								if(LINK->IsUsingSocks())
								{
									TS_RecvSocksRequest( LINK , sz );
								}
								else
								{
									DoLinkError( LINK
										, SONORK_RESULT_INTERNAL_ERROR
										, GSS_NETCXLOST
										, SONORK_MODULE_LINE
										, true
										, SONORK_UTS_WARNING_NONE);
								}

							}
							else
								TS_Read( LINK , sz );
						}
					}
					else
					{
						DoLinkError( LINK
							, SONORK_RESULT_FORCED_TERMINATION
							, GSS_NETCXLOST
							, SONORK_MODULE_LINE
							, true
							, SONORK_UTS_WARNING_NONE);
					}
				}
			}
			if( FD_ISSET( LINK->Socket() , work_w_fd_set ) )
			{
				if( LINK->Status() == SONORK_NETIO_STATUS_CONNECTING )
				{
					if(LINK->IsUsingSocks())
					{
						if(!(LINK->Flags()&SONORK_UTS_LINK_F_SOCKS_REQ_SENT))
						{
							TS_SendSocksRequest( LINK );
						}
					}
					else
						SendLoginReq( LINK );

				}
				else
					SetWriteEnabled( LINK );
			}
			if( FD_ISSET( LINK->Socket() , work_e_fd_set ) )
			{
				if(LINK->Status()==SONORK_NETIO_STATUS_CONNECTING)
				{
					DoLinkError( LINK
						, SONORK_RESULT_FORCED_TERMINATION
						, GSS_NETCXERR
						, SONORK_MODULE_LINE
						, true
						, SONORK_UTS_WARNING_NONE);
				}
			}

		}
	}
	if(	timeslot_clock.IntervalSecsAfter(monitor_clock)
		 >= monitor_interval_secs )
	{
		TS_Monitor();
		monitor_clock.Set(timeslot_clock);
	}
	if( TestUtsFlag(SONORK_UTS_SERVER_F_QUEUE_DIRTY) )
		TS_ClearQueues();
	mutex.Unlock();

	return sk_set_count>0;
}
void TSonorkUTS::TS_Monitor()
{
// LINKS MONITOR
	TSonorkUTSLinkEx**dR,*LINK;
	UINT 		index;
	UINT		idle_secs;
	SONORK_UTS_WARNING
			warning=SONORK_UTS_WARNING_NONE;

	for(index=0,dR=link_table.ptr
		;index<link_table.max_entries
		;index++,dR++)
	{
		if(*dR)
		{
			LINK=*dR;

			idle_secs=timeslot_clock.IntervalSecsAfter( LINK->net.last_request_clock );
			if( idle_secs> 86400 ) // more than a day.. hmm.. don't think so.. calc is wrong
				 continue;

			if(LINK->Status()==SONORK_NETIO_STATUS_CONNECTING)
			{
				if( idle_secs < SONORK_UTS_LINK_MAX_CONNECT_SECS )
					continue;
			}
			else
			if(LINK->Status()==SONORK_NETIO_STATUS_AUTHORIZING)
			{
				if( idle_secs < SONORK_UTS_LINK_MAX_AUTHORIZE_SECS )
					continue;
				warning=SONORK_UTS_WARNING_AUTH_TIMEOUT;
			}
			else
			{
				// Never condemn "main" link, just reset the timeout
				if( LINK == main.link  || LINK->Status() == SONORK_NETIO_STATUS_LISTENING)
				{
					LINK->net.last_request_clock.Set( timeslot_clock );
					continue;
				}

				if( LINK->IsWriteEnabled() && LINK->net.akn_queue.Items() == 0)
				{
					if( idle_secs <= max_link_idle_secs )
						continue;
				}
				else
				{
					if( idle_secs < SONORK_UTS_LINK_MAX_WRITE_DISABLED_SECS )
						continue;
				}
			}

			DoLinkError(LINK
				, SONORK_RESULT_TIMEOUT
				, GSS_NETCXSVRKILL
				, 0
				, true
				, warning);
			warning=SONORK_UTS_WARNING_NONE;
		}
	}
}

void
	TSonorkUTS::TS_ClearQueues()
{
	TSonorkUTSLinkEx* LINK;
	uts_flags&=~SONORK_UTS_SERVER_F_QUEUE_DIRTY;

	if(cts_queue.Items())
	{
		TSonorkUTSEvent
			event(SONORK_UTS_EVENT_CLEAR_TO_SEND,NULL,NULL);

		while( (event.link=cts_queue.RemoveFirst()) != NULL )
		{
			event.link->flags&=~SONORK_UTS_LINK_F_CLEAR_TO_SEND;
			if( event.link->Status() == SONORK_NETIO_STATUS_CONNECTED )
				InvokeEventHandler(&event);
		}
	}

	while( (LINK=write_queue.RemoveFirst()) != NULL )
	{
		LINK->flags&=~SONORK_UTS_LINK_F_IN_WRITE_QUEUE;

		if( LINK->Status() != SONORK_NETIO_STATUS_DISCONNECTED )
			TS_Write( LINK );
	}

	while((LINK=del_queue.RemoveFirst())!=NULL)
	{

		if( DestroyLink( LINK->Id() ) )
			SetFdSetDirty();
	}
}


// --------------------------------------------------------------------
// Write functions

BOOL TSonorkUTS::SetBlocking(TSonorkUTSLinkEx*L,bool v)
{
    bool success=(Sonork_Net_SetBlocking(L->Socket(), v)==0);
    if(success)
		if(v)
    	{
			L->flags|=SONORK_UTS_LINK_F_BLOCKING;
			SetWriteEnabled(L);
		}
		else
		{
			L->flags&=~SONORK_UTS_LINK_F_BLOCKING;
		}
	return success;
}
void TSonorkUTS::TS_Write(TSonorkUTSLinkEx*LINK)
{
	DWORD written_bytes;
	DWORD bytes_to_write;
	DWORD err_code;

	TSonorkTcpPacket *P;

	LINK->net.last_request_clock.Set( timeslot_clock );
	assert( LINK->flags&SONORK_UTS_LINK_F_WRITE_ENABLED );

	if( LINK->net.o_entry == NULL )
	{
		if( !TS_GetNextOutputEntry(LINK) )
			return;
	}

	for(;;)
	{
		P=LINK->net.o_entry->packet;
		bytes_to_write = LINK->net.o_header.RemainingSize(LINK->net.o_offset);
		if( bytes_to_write > SONORK_UTS_MAX_SEND_SIZE )
			bytes_to_write = SONORK_UTS_MAX_SEND_SIZE;
		written_bytes=::send(LINK->Socket()
			,(const char*)P->pHeader()+LINK->net.o_offset
			,bytes_to_write
			,0);

		if( written_bytes == (DWORD)SOCKET_ERROR )
		{
			err_code=WSAGetLastError();
			if(IS_SONORK_SOCKERR_WOULDBLOCK(err_code))
			{
				LINK->flags&=~SONORK_UTS_LINK_F_WRITE_ENABLED;
				SetFdSetDirty();
			}
			else
			{
				DoLinkError(LINK
				, SONORK_RESULT_NETWORK_ERROR
				, GSS_NETCXLOST
				, err_code
				, false
				, SONORK_UTS_WARNING_NONE);
			}
			break;
		}
		LINK->net.o_offset+=written_bytes;
		if( LINK->net.o_header.TxComplete(LINK->net.o_offset) )
		{
			if( (LINK->Flags() & SONORK_UTS_LINK_F_MUST_AKN)
				&& LINK->net.o_header.Type() == SONORK_NETIO_HFLAG_TYPE_PACKET )
			{
				LINK->net.o_entry->FreeTcpPacket();
				LINK->net.akn_queue.Add(LINK->net.o_entry);

				/*
					sonork_printf("Uts[%08x] F:%08x WRITE, akns{ToRecv:%u ToSend:%u}"
					,LINK->Id()
					,LINK->Flags()
					,LINK->net.akn_queue.Items()
					,LINK->net.pending_akns
					);
				*/
					
			}
			else
			{
				EndOutputEntryTx(LINK, LINK->net.o_entry, NULL);
			}
			if(!TS_GetNextOutputEntry(LINK))
				break;
		}
	}
}

void
	TSonorkUTS::EndOutputEntryTx(TSonorkUTSLinkEx*LINK, TSonorkUTSOEntry*entry, TSonorkError* pERR)
{
/*
	sonork_printf("Uts[%08x]  EndTX (%08x,%08x,%08x) ---"
		,LINK->Id()
		,entry
		,entry->callback
		,entry->param);
*/		
	if( entry->callback!=NULL && !TestUtsFlag(SONORK_UTS_SERVER_F_NO_CALLBACK) )
	{
		if(pERR==NULL)
		{
			entry->callback(entry->param , entry->tag, LINK , &TSonorkErrorOk() );
		}
		else
		{
			entry->callback(entry->param , entry->tag, LINK , pERR );
		}
	}
	SONORK_MEM_DELETE( entry );
}

// TS_GetNextOutputEntry
//  Invoked only from TS_Write: Retrieves next pending output packet,
//  if none exists, the link will enter the ClearToSend Queue
//  (which clears the DATA_PENDING flag and sets the CLEAR_TO_SEND flag)

BOOL    TSonorkUTS::TS_GetNextOutputEntry(TSonorkUTSLinkEx*LINK)
{
	TSonorkTcpPacket *P;

	LINK->net.o_entry=LINK->net.o_queue.RemoveFirst();
	LINK->net.o_offset=0;

	if( LINK->net.o_entry == NULL )
	{
		SetClearToSend( LINK );
		return false;
	}
	else
	{
/*		sonork_printf("Uts[%08x] StartTX(%08x,%08x,%08x) +++"
			,LINK->Id()
			,LINK->net.o_entry
			,LINK->net.o_entry->callback
			,LINK->net.o_entry->param);
*/
		P = LINK->net.o_entry->packet;
		memcpy(	&LINK->net.o_header
				,P->pHeader()
				,sizeof(TSonorkTcpPacketHeader));
		P->NormalizeHeader();
		return true;
	}

}

// SendTcpPacket takes ownership of <tcp_packet>
// It should not be freed or references after calling this function
SONORK_RESULT
 TSonorkUTS::SendTcpPacket(TSonorkUTSLinkEx*LINK
			,TSonorkTcpPacket*tcp_packet
			, void *				param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag
			, bool					high_priority
			)
{
	TSonorkUTSOEntry *E;
	DWORD			akn_count;
	if( tcp_packet->AknCount() == 0 )
	{
		if( (LINK->Flags() & SONORK_UTS_LINK_F_MUST_AKN  ) && LINK->net.pending_akns )
		{
			if( LINK->net.pending_akns > SONORK_UDPIO_HFLAG_MAX_AKN_COUNT )
				akn_count = SONORK_UDPIO_HFLAG_MAX_AKN_COUNT ;
			else
				akn_count = LINK->net.pending_akns;
			tcp_packet->HFlag()|=(WORD)(SONORK_UDPIO_HFLAG_AKN_COUNT(akn_count));
			LINK->net.pending_akns-=akn_count;
			/*
			sonork_printf("Uts[%08x] F:%08x PIGGY %u, akns{ToRecv:%u ToSend:%u}"
				,LINK->Id()
				,LINK->Flags()
				,akn_count
				,LINK->net.akn_queue.Items()
				,LINK->net.pending_akns
				);
			*/
		}
	}

	SONORK_MEM_NEW(
		E= new TSonorkUTSOEntry(tcp_packet,param,callback,tag);
	);
	LINK->net.o_queue.Add( E , high_priority?SONORK_QUEUE_PRIORITY_ABOVE_NORMAL:SONORK_QUEUE_PRIORITY_NORMAL);
	if( LINK->IsWriteEnabled() )
		AddToWriteQueue( LINK );
	return SONORK_RESULT_OK;
}


SONORK_RESULT
 TSonorkUTS::SendPacket(TSonorkError& ERR
			, SONORK_UTS_LINK_ID 	id
			, DWORD 		cmd
			, BYTE 			version
			, const BYTE*		data
			, UINT 			data_size
			, void *		param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag
			)
{
	TSonorkUTSLinkEx* LINK;

	LINK=GetActiveLinkErr( id , ERR );
	if(LINK != NULL)
	{
		UINT		P_size;
		TSonorkUTSPacket	*P;
		P=SONORK_AllocUtsPacket( data_size );
		P_size = P->E_Data(data_size, cmd, version, data, data_size);
		SendUtsPacket(ERR,LINK,P,P_size,param,callback,tag);
		SONORK_FreeUtsPacket(P);
	}
	return ERR.Result();

}

SONORK_RESULT	TSonorkUTS::SendPacket(TSonorkError&ERR
			, SONORK_UTS_LINK_ID 	id
			, TSonorkUTSPacket*	P
			, UINT 			P_size
			, void *		param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag)
{
	TSonorkUTSLinkEx* LINK;

	LINK=GetActiveLinkErr( id , ERR  );
	if(LINK != NULL)
	{
		SendUtsPacket(ERR,LINK,P,P_size,param,callback,tag);
	}
	return ERR.Result();
}

SONORK_RESULT	TSonorkUTS::SendPacket(TSonorkError&	ERR
			, SONORK_UTS_LINK_ID 		id
			, DWORD 			cmd
			, BYTE 				version
			, const TSonorkCodecAtom*	A
			, void *			param
			, lpfnSonorkUTSCallback 	callback
			, const SONORK_DWORD2*		tag)
{
	UINT		P_size,A_size;
	TSonorkUTSPacket	*P;
	if(A!=NULL)
	{
		A_size=A->CODEC_Size();
		P=SONORK_AllocUtsPacket( A_size );
		P_size = P->E_Atom(A_size,cmd,version,A);
	}
	else
	{
		P=SONORK_AllocUtsPacket( 0 );
		P_size = P->E_Data(0, cmd, version, NULL, 0);
	}
	SendPacket(ERR,id,P,P_size,param,callback,tag);
	SONORK_FreeUtsPacket(P);
	return ERR.Result();

}
SONORK_RESULT
 TSonorkUTS::SendUtsPacket(TSonorkError&ERR
			, TSonorkUTSLinkEx*		LINK
			, TSonorkUTSPacket*		P
			, UINT 		  		P_size
			, void *	  		param
			, lpfnSonorkUTSCallback 	callback
			, const SONORK_DWORD2*		tag)
{
	TSonorkCryptContext& CC=LINK->CryptContext();
	UINT 			crypted_size;
	SONORK_RESULT 		t_result;
	TSonorkTcpPacket	*tcp_packet;

	crypted_size	=CC.MaxEncryptedSize(P_size);
	tcp_packet	=Sonork_AllocTcpPacket(crypted_size);
	if(CC.SingleBufferEncryptionEnabled())
	{
		memcpy(tcp_packet->DataPtr(),P,crypted_size);
		t_result=CC.SingleBufferEncrypt(tcp_packet->DataPtr(),crypted_size);
	}
	else
	{
		t_result=CC.DoubleBufferEncrypt(
			 P
			,P_size
			,tcp_packet->DataPtr()
			,&crypted_size);
		tcp_packet->SetDataSize(crypted_size);
	}
	if(t_result!=SONORK_RESULT_OK)
		ERR.SetSys(t_result
			, GSS_CRYPTFAIL
			, SONORK_MODULE_LINE );
	else
	{
		tcp_packet->HFlag()=SONORK_NETIO_HFLAG_TYPE_PACKET;
		t_result = SendTcpPacket(LINK,tcp_packet,param,callback,tag,false);
		if(t_result==SONORK_RESULT_OK)
			ERR.SetOk();
		else
			ERR.SetSys(t_result,GSS_NETERR,0);
	}
	return ERR.Result();
}






// --------------------------------------------------------------------
// Lookup functions
TSonorkUTSLinkEx**
	TSonorkUTS::GetLinkPtr(SONORK_UTS_LINK_ID id)
{
	DWORD index=id&SONORK_UTS_LINK_ID_FM_INDEX;
	TSonorkUTSLinkEx**dLINK;
	if(index<link_table.max_entries)
	{
		dLINK=link_table.ptr+index;
		if(*dLINK)
			if((*dLINK)->Id()==id)
				return dLINK;
    }
	return NULL;

}
// GetActiveLinkErr()
// If Link is not valid, ERR is set, if Link IS valid, ERR is *NOT* modified.
TSonorkUTSLinkEx *
		TSonorkUTS::GetActiveLinkErr(SONORK_UTS_LINK_ID id , TSonorkError&ERR )
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(!LINK)
		ERR.SetSys(SONORK_RESULT_INVALID_HANDLE
			, GSS_BADHANDLE
			, SONORK_MODULE_LINE);
	else
	if(LINK->Status()!=SONORK_NETIO_STATUS_CONNECTED)
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION
			, GSS_NOTCXTED
			, SONORK_MODULE_LINE);
	else
		return LINK;
	return NULL;
}

TSonorkUTSLinkEx *
		TSonorkUTS::GetActiveLink(SONORK_UTS_LINK_ID id, bool must_be_connected)
{
	TSonorkUTSLinkEx**dLINK;

	dLINK = GetLinkPtr(id);
	if(dLINK == NULL)
		return NULL;
	if((*dLINK)->IsCondemned())
		return NULL
		;
	if(must_be_connected)
		if( (*dLINK)->Status() != SONORK_NETIO_STATUS_CONNECTED )
			return NULL;

	return *dLINK;
}


bool
	TSonorkUTS::ResetLinkTimeout( SONORK_UTS_LINK_ID id )
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		if(LINK->Status()>=SONORK_NETIO_STATUS_AUTHORIZING)
		{
			LINK->net.last_request_clock.Set( timeslot_clock );
		}
		return true;
	}
	return false;
}
int
	TSonorkUTS::SetSocketBufferSize(SONORK_UTS_LINK_ID id,long send_size,long recv_size)
{
	TSonorkUTSLinkEx* LINK;
	LINK = GetActiveLink(id,false);
	if(!LINK)return SOCKET_ERROR;
	return Sonork_Net_SetBufferSize(LINK->Socket(),send_size,recv_size);
}

bool
 TSonorkUTS::SetLinkData(SONORK_UTS_LINK_ID id,DWORD data)
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		LINK->SetData( data );
		return true;
	}
	return false;
}
bool
 TSonorkUTS::GetLinkData(SONORK_UTS_LINK_ID id,DWORD*data)
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		*data = LINK->GetData();
		return true;
	}
	return false;
}

bool
	TSonorkUTS::GetLinkInfo(SONORK_UTS_LINK_ID id
		,SONORK_NETIO_STATUS*	status
		,DWORD*	    		flags
		,TSonorkUTSDescriptor*	descriptor
		)
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		if(flags)*flags=LINK->Flags();
		if(status)*status=LINK->Status();
		if(descriptor)memcpy(descriptor,&LINK->descriptor,sizeof(TSonorkUTSDescriptor));
		return true;
	}
	return false;


}


bool
	TSonorkUTS::GetPhysAddr(SONORK_UTS_LINK_ID id,TSonorkPhysAddr&phys_addr)
{
	TSonorkUTSLinkEx*LINK;
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		phys_addr.Set( LINK->phys_addr );
        return true;
    }
    return false;

}

BOOL
 TSonorkUTS::DestroyLink(SONORK_UTS_LINK_ID id)
{
	TSonorkUTSLinkEx**dLINK;
	TSonorkUTSLinkEx *LINK;
	assert(link_table.ptr != NULL);

	dLINK=GetLinkPtr(id);
	if( dLINK != NULL )
	{
		LINK=*dLINK;
		link_table.entries--;
		if( MainLink() == LINK )
			main.link=NULL;

		assert( LINK->Status() != SONORK_NETIO_STATUS_CONNECTED );
		SONORK_MEM_DELETE( LINK );

		*dLINK=NULL;
		return true;
	}
	else
		sonork_printf("Uts[%08x] not found!",id);
	return false;
}
TSonorkUTSLinkEx*
 TSonorkUTS::AllocLink(DWORD link_flags)
{
	DWORD			index;
	SONORK_UTS_LINK_ID 	id;
	TSonorkUTSLinkEx**R;
	assert(link_table.ptr != NULL);
	if( link_table.entries<link_table.max_entries )
	{
		for(index=0,R=link_table.ptr
		;index<link_table.max_entries
		;index++,R++)
		{
			if(!(*R))
			{
				if(++link_table.cookie>4096)link_table.cookie=1;
				id=(link_table.cookie<<SONORK_UTS_LINK_ID_FS_COOKIE)&SONORK_UTS_LINK_ID_FM_COOKIE;
				id|=index&SONORK_UTS_LINK_ID_FM_INDEX;
				SONORK_MEM_NEW( *R=new TSonorkUTSLinkEx( id , link_flags) );
				(*R)->net.last_request_clock.Set( timeslot_clock );
				link_table.entries++;
				return *R;
			}
		}
	}

	return NULL;
}
void
 TSonorkUTS::Disconnect(SONORK_UTS_LINK_ID id, bool kill_socket)
{
	TSonorkUTSLinkEx*	LINK;
	LINK=GetActiveLink( id , false );
	if( LINK != NULL )
	{
		DoLinkError(
			   LINK
			,  SONORK_RESULT_FORCED_TERMINATION
			,  GSS_NETCXLOST
			,  0
			,  kill_socket
			,  SONORK_UTS_WARNING_NONE);
	}
}
void
 TSonorkUTS::Disconnect(const TSonorkId& userId, bool kill_socket)
{
	TSonorkUTSLinkEx*   LINK;
	TSonorkListIterator I;

	InitEnumLink(I);

	while( (LINK=EnumNextLink(I)) != NULL )
	{
		if( LINK->descriptor.locus.userId != userId )continue;
		if( LINK->IsCondemned() ) continue;
		DoLinkError(
			   LINK
			,  SONORK_RESULT_FORCED_TERMINATION
			,  GSS_NETCXLOST
			,  0
			,  kill_socket
			,  SONORK_UTS_WARNING_NONE);
	}
}

SOCKET
 TSonorkUTS::CreateSocket(UINT *p_err_code, bool blocking)
{
	SOCKET sk;
	UINT err_code;
	sk=socket(PF_INET,SOCK_STREAM,0);
	if(sk==INVALID_SOCKET)
	{
		err_code=WSAGetLastError();
ep_02:
		if(*p_err_code)
			*p_err_code=err_code;
		return INVALID_SOCKET;
	}
	err_code=Sonork_Net_SetBlocking(sk,blocking);
	if(err_code)
	{
	//Migs
		Sonork_Net_KillSocket(sk);
		goto ep_02;
	}
	return sk;
}
void
    TSonorkUTS::Shutdown()
{
	UINT 		     	index;
	TSonorkUTSLinkEx**	dLINK;
	TSonorkErrorInfo ERR(SONORK_RESULT_FORCED_TERMINATION,"Shutdown",0,true);

	for(index=0,dLINK=link_table.ptr
	;index<link_table.max_entries
	;index++,dLINK++)
	{
		if(*dLINK==NULL)continue;
		DoLinkError(*dLINK
			, SONORK_RESULT_FORCED_TERMINATION
			, GSS_USRCANCEL
			, 0
			, true
			, SONORK_UTS_WARNING_NONE);
		DestroyLink((*dLINK)->Id());
	}
	del_queue.Clear();
	cts_queue.Clear();
	write_queue.Clear();
}




BOOL
	TSonorkUTS::CancelCallback(SONORK_UTS_LINK_ID id,void*param, SONORK_DWORD2* pTag )
{
	TSonorkUTSLinkEx*	LINK;
	TSonorkUTSOEntry*	E;
	TSonorkUTSOEntry*	foundE=NULL;
	UINT	entries=0;

//	sonork_printf("Uts::CancelCallback(%08x , %08x)",id,param);
	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		TSonorkListIterator I;
//		sonork_printf("Uts::CancelCallback:Link Found");

		LINK->net.akn_queue.BeginEnum(I);
		while( (E=LINK->net.akn_queue.EnumNext(I)) != NULL )
		{
			if(E->param == param)
			{
				foundE=E;
				E->callback = NULL;
				entries++;
			}
		}
		LINK->net.akn_queue.EndEnum(I);

		E=LINK->net.o_entry;
		if(E)
			if(E->param == param)
			{
				foundE=E;
				E->callback = NULL;
				entries++;
			}
		LINK->net.o_queue.BeginEnum(I);
		while( (E=LINK->net.o_queue.EnumNext(I)) != NULL )
		{
			if(E->param == param)
			{
				foundE=E;
				E->callback = NULL;
				entries++;
			}
		}
		LINK->net.o_queue.EndEnum(I);

	}
//	else		sonork_printf("Uts::CancelCallback:Link NOT Found");
	if(foundE && pTag)
		{
			pTag->Set(foundE->tag);
		}
//	sonork_printf("Uts::CancelCallback: %u callbacks found",entries);
	return entries;
}

bool
	TSonorkUTS::Authorize(SONORK_UTS_LINK_ID id,TSonorkError&ERR)
{
	TSonorkUTSLinkEx*	LINK;

	//SONORK_Trace(SONORK_TRACE_EVENT_DEBUG
	//		,"Uts::Authorize(%x) %s.%u",id,ERR.ResultName(),ERR.Code());

	LINK=GetActiveLink( id , false );
	if(LINK)
	{
		return SendLoginAkn(LINK,ERR);
	}
	else
	{
		return false;
	}
}
bool
	TSonorkUTS::SendLoginAkn(TSonorkUTSLinkEx*LINK,TSonorkError& ERR)
{
	if( !LINK->IsCondemned() && LINK->Status()==SONORK_NETIO_STATUS_AUTHORIZING )
	{
		bool					rv;
		TSonorkUTSLoginPacket		PACKET;
		TSonorkUTSLoginPacket::AKN*	AKN;

		PACKET.login_data.SetDataSize(sizeof(TSonorkUTSLoginPacket::AKN), true);
		AKN=(TSonorkUTSLoginPacket::AKN*)PACKET.login_data.Buffer();
		PACKET.header.version	=Version();
		PACKET.header.os_info 	=0;//MAKE_SONORK_OS_INFO(0);
		SONORK_ZeroMem(PACKET.header.reserved,sizeof(PACKET.header.reserved));

		AKN->link_flags	= (LINK->Flags()&SONORK_UTS_LINK_FM_BEHAVIOUR_NET);
		AKN->result		= ERR.Result();
		AKN->err_code   = ERR.Code();
		if(ERR.Result() == SONORK_RESULT_OK)
		{
			AKN->link_id	= LINK->Id();
			memcpy(&AKN->descriptor,&MainLink()->descriptor,sizeof(TSonorkUTSDescriptor));
		}
		else
		{
			SONORK_ZeroMem(&AKN->descriptor,sizeof(TSonorkUTSDescriptor));
			AKN->link_id	= 0;
		}
		AKN->phys_addr.Set(LINK->phys_addr);
		AKN->login_flags	= 0;
		SONORK_ZeroMem(AKN->reserved,sizeof(AKN->reserved));
		PACKET.header.signature=PACKET.login_data.GenerateSignature();
		LINK->CryptContext().GetCryptInfo(&PACKET.crypt_info);
		LINK->CryptContext().Encrypt( &PACKET.login_data );
		PACKET.login_data.SetDataType(SONORK_ATOM_SONORK_UTS_LOGIN_AKN_1);
/*
		sonork_printf("Uts[%08x] F:%08x SendLoginAkn (MUST AKN: %s)"
			, LINK->Id()
			, LINK->Flags()
			, LINK->Flags()&SONORK_UTS_LINK_F_MUST_AKN?"YES":"NO");
*/			
		{
			TSonorkTcpPacket *tcp_packet;
			TSonorkUTSPacket *P;
			UINT		A_size,P_size;
			A_size = PACKET.CODEC_Size();
			tcp_packet = Sonork_AllocTcpPacket( A_size + sizeof(TSonorkUTSPacket) );

			P = (TSonorkUTSPacket*)tcp_packet->DataPtr();
			P_size = P->E_Atom( A_size , 0 , Version() , &PACKET );
			tcp_packet->SetDataSize(P_size);
			tcp_packet->HFlag()=SONORK_NETIO_HFLAG_TYPE_LOGIN;
			// SendTcpPacket takes ownership of <tcp_packet>
			// so it should not be freed
			rv = (SendTcpPacket(LINK,tcp_packet,NULL,NULL,NULL,false) == SONORK_RESULT_OK);
		}
		if(ERR.Result()!=SONORK_RESULT_INVALID_ENCRYPTION)
		{
			SetStatus(LINK
				,ERR.Result()==SONORK_RESULT_OK
					?SONORK_NETIO_STATUS_CONNECTED
					:SONORK_NETIO_STATUS_DISCONNECTING
				,ERR);
		}
		return rv;
	}
	return false;
}
void
	TSonorkUTS::ProcessLoginAkn(
		TSonorkUTSLinkEx*	LINK
		,TSonorkUTSPacket*	P
		,UINT 			P_size)
{

	if( LINK->Status()==SONORK_NETIO_STATUS_AUTHORIZING && LINK->Outgoing())
	{
		TSonorkError  				ERR;
		SONORK_RESULT				result;
		TSonorkUTSLoginPacket		PACKET;
		TSonorkUTSLoginPacket::AKN*	AKN;
		char					tmp[64];
		DWORD					signature;
		LINK->phys_addr.GetStr(tmp);
		for(;;)
		{
			result = P->D_Atom(P_size,&PACKET);
			if( result != SONORK_RESULT_OK
			||  PACKET.login_data.Type() != SONORK_ATOM_SONORK_UTS_LOGIN_AKN_1)
			{
				ERR.SetSys(result
				, GSS_BADCODEC
				, SONORK_MODULE_LINE);
				break;
			}
			if(!LINK->CryptContext().SetCryptInfo(&PACKET.crypt_info))
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_BADCRYPTTYPE
				, SONORK_MODULE_LINE);
				break;
			}
			result = LINK->CryptContext().Uncrypt( &PACKET.login_data );
			if( result != SONORK_RESULT_OK )
			{
				ERR.SetSys(result
				, GSS_CRYPTFAIL
				, SONORK_MODULE_LINE);
				break;
			}

			if( PACKET.login_data.DataSize() < sizeof(TSonorkUTSLoginPacket::AKN))
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_PCKTOOSMALL
				, SONORK_MODULE_LINE);
				break;
			}

			if( PACKET.header.version > (DWORD)Version() )
			{
				ERR.SetSys(SONORK_RESULT_INVALID_VERSION
					, GSS_OPNOTSUPPORT
					, SONORK_MODULE_LINE);
				break;
			}
			signature = PACKET.login_data.GenerateSignature();
			if( signature != PACKET.header.signature )
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_BADCRYPTSIGNATURE
				, SONORK_MODULE_LINE);
				break;
			}
			AKN=(TSonorkUTSLoginPacket::AKN*)PACKET.login_data.Buffer();
			memcpy(&LINK->descriptor,&AKN->descriptor,sizeof(TSonorkUTSDescriptor));
			LINK->flags&=~SONORK_UTS_LINK_FM_BEHAVIOUR_NET;
			LINK->flags|=AKN->link_flags&SONORK_UTS_LINK_FM_BEHAVIOUR_NET;
			ERR.SetOk();
/*
			sonork_printf("Uts[%08x] ProcessLoginAkn F:%08x AF:%08x (MUST AKN %s)"
				, LINK->Id()
				, LINK->Flags()
				, AKN->link_flags
				, LINK->Flags()&SONORK_UTS_LINK_F_MUST_AKN?"YES":"NO");
*/
			break;
		}
		SetStatus(LINK,ERR.Result()==SONORK_RESULT_OK
					?SONORK_NETIO_STATUS_CONNECTED
					:SONORK_NETIO_STATUS_DISCONNECTING
					,ERR);
	}
}

bool
	TSonorkUTS::SendLoginReq( TSonorkUTSLinkEx* LINK)
{
	if ( LINK->Outgoing() && MainLink() !=NULL )
	{
		TSonorkUTSLoginPacket		PACKET;
		TSonorkUTSLoginPacket::REQ*	REQ;

		SetStatus(LINK
			,SONORK_NETIO_STATUS_AUTHORIZING
			,TSonorkErrorOk());

		PACKET.login_data.SetDataSize(sizeof(TSonorkUTSLoginPacket::REQ), true);
		REQ=(TSonorkUTSLoginPacket::REQ*)PACKET.login_data.Buffer();
		PACKET.header.version	= Version();
		PACKET.header.os_info 	= 0;//MAKE_SONORK_OS_INFO(0);
		SONORK_ZeroMem(PACKET.header.reserved,sizeof(PACKET.header.reserved));

		memcpy(&REQ->descriptor,&MainLink()->descriptor,sizeof(TSonorkUTSDescriptor));

		REQ->phys_addr.Set(MainLink()->phys_addr);
		REQ->login_flags	= 0;
		REQ->link_flags		= LINK->Flags()&SONORK_UTS_LINK_FM_BEHAVIOUR_NET;

		SONORK_ZeroMem(REQ->reserved,sizeof(REQ->reserved));
		EncodeSidPin(REQ->pin,REQ->pin_type,LINK);
		PACKET.header.signature=PACKET.login_data.GenerateSignature();
		LINK->CryptContext().GetCryptInfo(&PACKET.crypt_info);
		LINK->CryptContext().Encrypt( &PACKET.login_data );
		PACKET.login_data.SetDataType(SONORK_ATOM_SONORK_UTS_LOGIN_REQ_1);
		{
			TSonorkTcpPacket	*tcp_packet;
			TSonorkUTSPacket	*P;
			UINT 			A_size,P_size;
			A_size 		= PACKET.CODEC_Size();
			tcp_packet = Sonork_AllocTcpPacket( A_size + sizeof(TSonorkUTSPacket) );

			P = (TSonorkUTSPacket*)tcp_packet->DataPtr();
			P_size = P->E_Atom(A_size,0,Version(),&PACKET);
			tcp_packet->SetDataSize(P_size);
			tcp_packet->HFlag()=SONORK_NETIO_HFLAG_TYPE_CONNECT;
			SendTcpPacket(LINK,tcp_packet,NULL,NULL,NULL,false);
		}
	}
	return false;
}

void
	TSonorkUTS::ProcessLoginReq(
		TSonorkUTSLinkEx*	LINK
		,TSonorkUTSPacket*	P
		,UINT 			P_size)
{
	if( LINK ->Status()==SONORK_NETIO_STATUS_AUTHORIZING
	&&  LINK ->Incomming()
	&&  MainLink() != NULL)
	{
		TSonorkError  				ERR;
		SONORK_RESULT				result;
		bool 					send_akn=true;
		DWORD					signature;
		TSonorkUTSLoginPacket		PACKET;
		TSonorkUTSLoginPacket::REQ*	REQ;

//		LINK->phys_addr.GetStr(l_phys_addr);
		for(;;)
		{
			result = P->D_Atom(P_size,&PACKET);
			if( result != SONORK_RESULT_OK
			||  PACKET.login_data.Type() != SONORK_ATOM_SONORK_UTS_LOGIN_REQ_1)
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_BADCODEC
				, SONORK_MODULE_LINE);
				break;
			}
			if(!LINK->CryptContext().SetCryptInfo(&PACKET.crypt_info))
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_BADCRYPTTYPE
				, SONORK_MODULE_LINE);
				break;
			}
			result = LINK->CryptContext().Uncrypt( &PACKET.login_data );
			if( result != SONORK_RESULT_OK )
			{
				ERR.SetSys(result
				, GSS_CRYPTFAIL
				, SONORK_MODULE_LINE);
				break;
			}
			if( PACKET.login_data.DataSize() < sizeof(TSonorkUTSLoginPacket::REQ))
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_PCKTOOSMALL
				, SONORK_MODULE_LINE);
				break;
			}
			signature=PACKET.login_data.GenerateSignature();
			if( signature != PACKET.header.signature )
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				, GSS_BADCRYPTSIGNATURE
				, SONORK_MODULE_LINE);
				break;
			}

			REQ = (TSonorkUTSLoginPacket::REQ*)PACKET.login_data.Buffer();

			/*
			char 					l_phys_addr[64]
							,	r_phys_addr[64];
			REQ->phys_addr.GetStr(r_phys_addr);
			sonork_printf(
				"Uts[%08x] F:%08x REQ Login:%08x Link:%08x"
				,LINK->Id()
				,LINK->Flags()
				,REQ->link_flags
				,REQ->login_flags);

			sonork_printf("   A{L'%s' R'%s'} S%x:%x %u.%u@%x.%x,P:%x.%x"
				,l_phys_addr
				,r_phys_addr
				,REQ->descriptor.serviceId
				,REQ->descriptor.flags
				,REQ->descriptor.locus.userId.v[0]
				,REQ->descriptor.locus.userId.v[1]
				,REQ->descriptor.locus.sid.v[0]
				,REQ->descriptor.locus.sid.v[1]
				,REQ->pin.v[0].v[0]
				,REQ->pin.v[0].v[1]);
			*/

			if( REQ->phys_addr.Type() != SONORK_PHYS_ADDR_TCP_1
			 && REQ->phys_addr.Type() != SONORK_PHYS_ADDR_NONE)
			{
				ERR.SetSys(SONORK_RESULT_ACCESS_DENIED
					, GSS_BADNETPROTOCOL
					, SONORK_MODULE_LINE);
				break;
			}
			if( VerifyLinkInfo(LINK->Id()
				,&REQ->descriptor
				,LINK->Status()
				,ERR) != SONORK_RESULT_OK )
			{
				break;
			}
			else
			{
				TSonorkUTSEvent event( SONORK_UTS_EVENT_LOGIN , LINK , &ERR );
				memcpy(&LINK->descriptor
					,&REQ->descriptor
					,sizeof(TSonorkUTSDescriptor));
				LINK->flags&=~SONORK_UTS_LINK_FM_BEHAVIOUR_NET;
				LINK->flags|=(REQ->link_flags&SONORK_UTS_LINK_FM_BEHAVIOUR_NET);
				event.D.login.flags	= REQ->login_flags;
				event.D.login.pin.Set( REQ->pin );
				event.D.login.pin_type 	= (SONORK_PIN_TYPE)REQ->pin_type;
				event.D.login.authorization=SONORK_UTS_AUTHORIZATION_NONE;

				InvokeEventHandler(&event);
				for(;;)
				{
					if(event.GetLoginAuthorization()==SONORK_UTS_AUTHORIZATION_PENDING)
					{
						ERR.SetOk();
						send_akn=false;
						break;
					}
					if(event.GetLoginAuthorization()==SONORK_UTS_AUTHORIZATION_ACCEPT)
    	            {
        	            if(ERR.Result()==SONORK_RESULT_OK)
							break;
					}
					if(event.GetLoginAuthorization()==SONORK_UTS_AUTHORIZATION_DENY)
					{
        	            if(ERR.Result()!=SONORK_RESULT_OK)
							break;
	                }
					ERR.SetSys(SONORK_RESULT_INTERNAL_ERROR
						, GSS_INTERNALERROR
						, SONORK_MODULE_LINE);
					break;
				}
			}
			break;
		}
		if(send_akn)
		{
			SendLoginAkn( LINK , ERR);
		}
	}
}


// VerifyLinkInfo
// Checks for duplicate connections.
SONORK_RESULT
	TSonorkUTS::VerifyLinkInfo(
				SONORK_UTS_LINK_ID 		cLinkId
			, 	const TSonorkUTSDescriptor* 	cDescriptor
			, 	SONORK_NETIO_STATUS 		cStatus
			, 	TSonorkError&			ERR )
{
	if( !MainLink() )
	{
		ERR.SetSys(SONORK_RESULT_INTERNAL_ERROR
			, GSS_NOTINIT
			, SONORK_MODULE_LINE);
		return ERR.Result();
	}
	if( cDescriptor->flags&(SONORK_UTS_APP_F_SINGLE_USER_INSTANCE|SONORK_UTS_APP_F_SINGLE_GLOBAL_INSTANCE) )
	{
		UINT 		 index;
		TSonorkUTSLinkEx**ptrLINK;
		TSonorkUTSLinkEx* tstLINK;
		BOOL		 single_user;

		single_user=(cDescriptor->flags&SONORK_UTS_APP_F_SINGLE_USER_INSTANCE);

		for(index=1,ptrLINK=link_table.ptr+1
			;index<link_table.max_entries
			;index++,ptrLINK++)
		{
			if(!*ptrLINK)continue;
			tstLINK=*ptrLINK;


			if( tstLINK->Id() == cLinkId
				|| tstLINK->Status() <= SONORK_NETIO_STATUS_DISCONNECTING )
				continue;

			if( cDescriptor->serviceId!=tstLINK->descriptor.serviceId )
				continue;

			if( single_user)
			{
				if(!(cDescriptor->locus.userId==tstLINK->descriptor.locus.userId))
				{
					continue;
				}
			}

			// There is another link with the same attributes.
			ERR.SetSys(SONORK_RESULT_NOT_ACCEPTED
				, GSS_DUPDATA
				, SONORK_MODULE_LINE);

			if( cLinkId!=SONORK_INVALID_LINK_ID
				&&
				cStatus >= SONORK_NETIO_STATUS_AUTHORIZING )
			{
				// If the attributes being tested belong to a valid link
				// and the link is not in an early connect stage,
				// we check if it should override the previous link
				if( tstLINK->Status() >= SONORK_NETIO_STATUS_AUTHORIZING )
				{
					// The previous link is already authorizing:
					//  Reject the new link, keep the old one.
					return ERR.Result();
				}
				else
				{
					// The other link is not authorizing yet and the new link is
					// already authorizing: kill the old link, keep the new one
					SetStatus(tstLINK
						, SONORK_NETIO_STATUS_DISCONNECTING
						, ERR);
				}
			}
			else
			{
				// The attributes being tested do not belong to a valid link
				// or the link is in an early connect stage, the previous
				// existing link is kept, the new one is refused.
				return ERR.Result();
			}
		}
	}
	ERR.SetOk();
	return ERR.Result();
}

void
	TSonorkUTS::TS_Read( TSonorkUTSLinkEx*LINK , UINT recv_size )
{
	BYTE *src=RecvBuffer();
	BYTE *ptr;
	UINT 				header_bytes, data_bytes;
	TSonorkTcpPacket	*tcp_packet;

	if( LINK->IsWriteEnabled() )
	{
		// Don't set the last_request_clock is Write is disabled:
		// It means the LINK is blocking write and we want to
		// control how long the write is taking (and kill the
		// link if it takes too long) in TS_Monitor()
		LINK->net.last_request_clock.Set( timeslot_clock );
	}
	while(recv_size)
	{
		if(LINK->net.i_offset<SONORK_TCP_HEADER_SIZE)
		{
			ptr=((BYTE*)&LINK->net.i_header) + LINK->net.i_offset;
			header_bytes=SONORK_TCP_HEADER_SIZE - LINK->net.i_offset;
			if(header_bytes>recv_size)
				header_bytes=recv_size;
			memcpy(ptr,src,header_bytes);
			src+=header_bytes;
			recv_size-=header_bytes;
			LINK->net.i_offset+=header_bytes;
			if(LINK->net.i_offset>=SONORK_TCP_HEADER_SIZE)
			{
				LINK->net.i_header.Normalize();
				if(LINK->net.i_header.DataSize()>SONORK_UTS_MAX_PACKET_SIZE)
				{
					DoLinkError(LINK
						, SONORK_RESULT_PROTOCOL_ERROR
						, GSS_BADNETPROTOCOL
						, 0
						, true
						, SONORK_UTS_WARNING_PROTOCOL);
					break;
				}
				assert(LINK->net.i_packet==NULL);
				LINK->net.i_packet=Sonork_AllocTcpPacket(LINK->net.i_header.DataSize());
				memcpy(LINK->net.i_packet->pHeader(),&LINK->net.i_header,SONORK_TCP_HEADER_SIZE);
			}
			else
			{
				break;
			}
		}
		if( LINK->net.i_packet!=NULL )
		{
			if( recv_size )
			{
				if(recv_size>LINK->net.i_header.RemainingSize(LINK->net.i_offset))
					data_bytes=LINK->net.i_header.RemainingSize(LINK->net.i_offset);
				else
					data_bytes=recv_size;
				LINK->net.i_packet->AppendBuffer(LINK->net.i_offset,src,data_bytes);
				src+=data_bytes;
				recv_size-=data_bytes;
				LINK->net.i_offset+=data_bytes;
			}
			if(LINK->net.i_header.TxComplete(LINK->net.i_offset))
				ProcessInPacket(LINK);
		}
	}
	if( LINK->Flags() & SONORK_UTS_LINK_F_MUST_AKN  )
	{
		while( LINK->net.pending_akns )
		{
			if( LINK->net.pending_akns > SONORK_UDPIO_HFLAG_MAX_AKN_COUNT )
				recv_size = SONORK_UDPIO_HFLAG_MAX_AKN_COUNT ;
			else
				recv_size = LINK->net.pending_akns;
			tcp_packet = Sonork_AllocTcpPacket( 0 );
			tcp_packet->HFlag()=(WORD)(
				SONORK_NETIO_HFLAG_TYPE_NONE
				|SONORK_UDPIO_HFLAG_AKN_COUNT(recv_size));
			SendTcpPacket(LINK,tcp_packet,NULL,NULL,NULL,true);
			LINK->net.pending_akns-=recv_size;
		}
	}
}

SONORK_UTS_LINK_ID
 TSonorkUTS::ConnectToUts(
		  TSonorkError& 		ERR
		, const TSonorkUTSDescriptor*	descriptor
		, const TSonorkPhysAddr& 	phys_addr
		, UINT 				link_flags)
{
	TSonorkUTSLinkEx* 	LINK;

	link_flags&=SONORK_UTS_LINK_FM_CONNECT;

	// Force use SOCKS (if available)
	link_flags&=~SONORK_UTS_LINK_F_NO_SOCKS;

	if(VerifyLinkInfo( SONORK_INVALID_LINK_ID
		, descriptor
		, SONORK_NETIO_STATUS_DISCONNECTED
		, ERR )!=SONORK_RESULT_OK)
		return SONORK_INVALID_LINK_ID;

	LINK=Connect(ERR,phys_addr,link_flags);
	if(LINK)
	{
		memcpy(&LINK->descriptor,descriptor,sizeof(TSonorkUTSDescriptor));
		if(!(LINK->Flags()&SONORK_UTS_LINK_F_NO_ENCRYPTION))
		{
			LINK->CryptContext().SetSimple(
				LINK->descriptor.locus.sid.v[1]
				+LINK->descriptor.locus.userId.v[1]
				+27
				+SONORK_Random(0x0f00)
				+LINK->Id());
		}
		SetStatus(LINK
			,SONORK_NETIO_STATUS_CONNECTING
			,ERR);
		return LINK->Id();
	}
	return SONORK_INVALID_LINK_ID;
}

TSonorkUTSLinkEx*
 TSonorkUTS::Connect(
		TSonorkError& 		ERR
	, 	const TSonorkPhysAddr& 	target_phys_addr
	, 	UINT 	    		link_flags)
{
	UINT 		err_code;
	TSonorkUTSLinkEx*R;

	if(!MainLink())
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION
			, GSS_NOTINIT
			, SONORK_MODULE_LINE);
	else
	if(target_phys_addr.Type()!=SONORK_PHYS_ADDR_TCP_1)
	{
		ERR.SetSys(SONORK_RESULT_INVALID_PARAMETER
			, GSS_BADADDR
			, SONORK_MODULE_LINE);
	}
	else
	{
		link_flags&=~SONORK_UTS_LINK_FM_TYPE;
		if((R=AllocLink(SONORK_UTS_LINK_TYPE_OUT_CONNECT|link_flags))==NULL)
			ERR.SetSys(SONORK_RESULT_OUT_OF_RESOURCES
				, GSS_NETCXERR
				, SONORK_MODULE_LINE);
		else
			ERR.SetOk();
	}
	if(ERR.Result()!=SONORK_RESULT_OK)
		return NULL;

	R->net.sk=CreateSocket(&err_code,false);
	if(R->Socket()==INVALID_SOCKET)
	{
		DestroyLink(R->Id());
		ERR.SetSys(SONORK_RESULT_NETWORK_ERROR
			,GSS_NETERR
			,err_code);
		return NULL;
	}
	Sonork_Net_SetBufferSize(R->Socket(),SONORK_UTS_SEND_BUFFER_SIZE,-1);

	if(		SocksEnabled()
		&& 	target_phys_addr.data.inet1.addr.sin_addr.s_addr!=0x0100007f
		&& 	!(link_flags & SONORK_UTS_LINK_F_NO_SOCKS))
	{
		// If using socks (and not connecting to the local host),
		// don't connect to the target address,
		// connect to the socks server instead, the target address
		// will be later passed on to the socks server

		R->flags|=SONORK_UTS_LINK_F_SOCKS_ENABLED;
		R->phys_addr.Set(SocksInfo().physAddr);
	}
	else
	{
		R->flags&=~SONORK_UTS_LINK_F_SOCKS_ENABLED;
		R->phys_addr.Set(target_phys_addr);
	}

	err_code=connect(R->Socket()
		,(sockaddr*)R->phys_addr.Inet1()
		,sizeof(*R->phys_addr.Inet1()));

	if(R->IsUsingSocks())
	{
		// If using socks, we're not connecting to the target address,
		// we're connecting to the socks server: Eestore the address
		// to the target adress.
		R->phys_addr.Set(target_phys_addr);
	}
	if(err_code==(UINT)SOCKET_ERROR)
	{
		err_code=WSAGetLastError();
//oliver
		if(!IS_SONORK_SOCKERR_WOULDBLOCK(err_code))
		{
			DestroyLink(R->Id());
			ERR.SetSys(SONORK_RESULT_NETWORK_ERROR
				,GSS_NETCXERR
				,err_code);
			return NULL;
		}
	}
	SetFdSetDirty();
	return R;
}


TSonorkUTSLinkEx*	TSonorkUTS::EnumNextLink(TSonorkListIterator& iterator)
{
	DWORD 	index;
	TSonorkUTSLinkEx**dLINK;
	index=iterator.d.index;
	dLINK=link_table.ptr+index;
	while(index<link_table.max_entries)
	{
		if(*dLINK)
		if( !(*dLINK)->IsCondemned() )
		{
			iterator.d.index=++index;
			return *dLINK;
		}
		index++;
		dLINK++;
	}
	iterator.d.index=index;
	return NULL;
}

void    TSonorkUTS::EnableCallbacks(bool v)
{
	if(v)
		uts_flags&=~SONORK_UTS_SERVER_F_NO_CALLBACK;
	else
		uts_flags|=SONORK_UTS_SERVER_F_NO_CALLBACK;
}

TSonorkUTSLinkEx::TSonorkUTSLinkEx( SONORK_UTS_LINK_ID p_id , DWORD p_flags)
{
	id		= p_id;
	status	= SONORK_NETIO_STATUS_DISCONNECTED;
	flags  	= p_flags;


	net.i_packet	=NULL;
	net.o_entry	=NULL;
	net.pending_akns = net.i_offset=net.o_offset=0;
	net.sk=INVALID_SOCKET;
	SONORK_ZeroMem(&net.crypt_context,sizeof(net.crypt_context));
	phys_addr.Clear();
	data = NULL;
	descriptor.serviceId 	=0;
	descriptor.flags	=0;
	descriptor.locus.Clear();

}

TSonorkUTSLinkEx::~TSonorkUTSLinkEx()
{
	net.crypt_context.Clear();
	assert( net.o_queue.Items() == 0 );
	assert( net.akn_queue.Items() == 0 );
	assert( net.o_entry == NULL );
//Migs
	if(Socket()!=INVALID_SOCKET)
		Sonork_Net_KillSocket(Socket());

	if(net.i_packet)
		Sonork_FreeTcpPacket(net.i_packet);
}
bool
		TSonorkUTS::SocksEnabled()
{
	return socks_info.Enabled();
}
void
	TSonorkUTS::SetSocksV4(TSonorkPhysAddr& physAddr)
{
	if( physAddr.Type() == SONORK_PHYS_ADDR_TCP_1 )
	{
		socks_info.physAddr.Set(physAddr);
		socks_info.version=4;
	}
	else
		socks_info.version=0;
}


SONORK_UTS_LINK_ID TSonorkUTS::FindLink(const TSonorkId& userId
	,DWORD service_id_1
	,DWORD instance_1
	,DWORD service_id_2
	,DWORD instance_2
	)
{
	TSonorkUTSLink*L;
	TSonorkListIterator I;
	InitEnumLink(I);
	while( (L=EnumNextLink(I)) != NULL )
	{
		if(L->descriptor.serviceId == service_id_1)
		{
			if(instance_1!=0 && L->descriptor.instance!=instance_1)
				continue;
		}
		else
		if(L->descriptor.serviceId == service_id_2)
		{
			if(instance_2!=0 && L->descriptor.instance!=instance_2)
				continue;
		}
		else
			continue;

		if(L->descriptor.locus.userId != userId )
			continue;
			
		return L->Id();
	}
	return SONORK_INVALID_LINK_ID;
}

TSonorkUTSOEntry::TSonorkUTSOEntry(TSonorkTcpPacket*pPacket
		, void*				pParam
		, lpfnSonorkUTSCallback 	pCallback
		, const SONORK_DWORD2*		pTag)
{
	packet 	= pPacket;
	param   = pParam;
	callback= pCallback;
	if(pTag)
		tag.Set(*pTag);
	else
		tag.Clear();
	assert( packet != NULL );
}
TSonorkUTSOEntry::~TSonorkUTSOEntry()
{
	FreeTcpPacket();
}
void TSonorkUTSOEntry::FreeTcpPacket()
{
	if( packet )
	{
		Sonork_FreeTcpPacket( packet );
		packet = NULL;
	}
}

/*
SONORK_RESULT	TSonorkUTS::SendCtrlMsg(TSonorkError&ERR
	, SONORK_UTS_LINK_ID 	id
	, DWORD 		cmd
	, BYTE 			version
	, const TSonorkCtrlMsg*	msg
	, const BYTE*		data
	, UINT 			data_size)
{
	TSonorkUTSLinkEx* LINK;

	LINK=GetActiveLinkErr(id , ERR );
	if(LINK != NULL)
	{
		UINT		P_size,A_size;
		TSonorkUTSPacket	*P;
		A_size= sizeof(TSonorkCtrlMsg) + data_size + 32;
		P = SONORK_AllocUtsPacket( A_size );
		P_size = TGP_E_CtrlMsg(P, A_size ,cmd,version,msg,data,data_size);
		SendUtsPacket(ERR,LINK,P,P_size,NULL,NULL,NULL);
		SONORK_FreeUtsPacket(P);
	}
	return ERR.Result();
}
*/

