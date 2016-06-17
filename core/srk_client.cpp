#include "srk_defs.h"
#pragma hdrstop
#include "srk_client.h"
#include "srk_event_handler.h"
#include "srk_crypt.h"
#include "srk_services.h"
#include "srk_svr_login_packet.h"
#include "srk_codec_io.h"
#include "srk_sys_string.h"

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

extern void _ERRNotConnected(TSonorkError& ERR);
extern void _ERREngineBusy(TSonorkError& ERR);

//-----------------------------------------------------------------
// Constructor
//  <app> is a pointer to the application that controls the engine
//  <tag> is an arbitrary value defined by the application, not used by GU

TSonorkClient::TSonorkClient(TSonorkEventHandler*handler , void  *tag)
{
	app.handler		=handler;
	app.tag			=tag;

	server.protocol	=SONORK_NETIO_PROTOCOL_NONE;
	server.phys_addr.Clear();

	control.userId.Clear();
	control.userAddr.OldClear();
	control.sidPin.Clear();

	control.socks_info.Clear();


	control.cur_task_id		=0;
	control.flags			=0;
	control.tcp			=NULL;
	control.udp			=NULL;
	control.status			=SONORK_NETIO_STATUS_DISCONNECTED;
	control.select_wait_msecs	=0UL;
	control.handler_depth		=0;
	control.global_task.function 	=SONORK_FUNCTION_NONE;

}

TSonorkClient::~TSonorkClient()
{
	EnableCallbacks(false);
	DestroyNetIO();
}

//////////////////////
// Connect
//  Public - API
//  Starts the network system and
//  and initiates the connection phase.
// The network address of the server is
//  read from the configuration file.
// The login flags should usually be zero
//  but for special cases, take a look at
//  LOGIN FLAGS in the gu_defs.h header.

SONORK_RESULT
 TSonorkClient::Connect(TSonorkError&ERR,TSonorkPhysAddr&	server_phys_addr)
{
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(Running())
	{
		_ERREngineBusy(ERR);
	}
	else
	{
		server.phys_addr.Set(server_phys_addr);
		if(CreateNetIO(ERR)==SONORK_RESULT_OK)
		{
			control.timeslot_clock.LoadCurrent();
			control.idle_clock.Set(TimeSlotClock());
			control.monitor_clock.Set(TimeSlotClock());
			if( Protocol() == SONORK_NETIO_PROTOCOL_UDP )
			{
				UDP_Connect(ERR);
				UDP_ResetPckTime();
			}
			else
				TCP_Connect(ERR);

			if(ERR.Result() == SONORK_RESULT_OK)
			{
				control.max_idle_secs = SONORK_CLIENT_DEFAULT_MAX_IDLE_SECS;
				SetStatus(SONORK_NETIO_STATUS_CONNECTING
					,&ERR
					,0
					,"Connect()");
			}
		}
	}
	control.mutex.Unlock();
	return ERR.Result();
}

//////////////////////
// UDP_Connect
//  Private - Internal
//  Initiates the UDP connection phase
SONORK_RESULT TSonorkClient::UDP_Connect(TSonorkError& ERR)
{
	int err_code;
	err_code=UDP_NetIO().Connect( server.phys_addr
		, (1024*16)+512
		, (1024*24)+512);

	if(!err_code)
	{
		UDP_SendConnectRequest();
		ERR.SetOk();
	}
	else
	{
		ERR.SetSys( SONORK_RESULT_NETWORK_ERROR
				, GSS_NETCXERR
				, err_code);
	}
	return ERR.Result();
}

//////////////////////
// TCP_Connect
//  Private - Internal
//  Initiates the TCP connection phase
SONORK_RESULT TSonorkClient::TCP_Connect(TSonorkError&ERR)
{
	int err_code;

	TCP_NetIO().SocksInfo().Set(control.socks_info);

	err_code=TCP_NetIO().Connect(server.phys_addr);
	if(!err_code)
	{
		ERR.SetOk();
	}
	else
	{
		fprintf(stderr,"TSonorkClient::TCP_Connect()=%u\n",err_code);
		ERR.SetSys( SONORK_RESULT_NETWORK_ERROR
			, GSS_NETCXERR
			, err_code);
	}
	return ERR.Result();
}

//////////////////////
// Disconnect
//  Public - API
//  Stops the engine and releases all network resources.
//  The event callback is always invoked unless EnableCallbacks is false
void TSonorkClient::Disconnect()
{
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(Status()>SONORK_NETIO_STATUS_DISCONNECTED)
	{
		TSonorkErrorOk okERR;
		SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
			,&okERR
			,0
			,"Disconnect()");
	}
	control.mutex.Unlock();
}

//////////////////////
//  Public - API
//  This is the core of the class: It should be
//  called permanently in order to keep the client working.
//  It processes outgoing/incomming packets and calls the
//  owner application's event/request callback handlers.
//  Typically, a call to TimeSlot will take from 1 to 500 msecs
//   plus any time taken up by the owner application handlers.
//  It is not recursive, so if the owner application calls TimeSlot()
//  from within one of its callback handlers, the call will be ignored
//  and have no effect.
void
 TSonorkClient::TCP_ConnectionLost()
{
	// NetIO has disconnected. Set status to DISCONNECTED
	TSonorkError ERR;
	SONORK_RESULT	_result;
	_result = Status() == SONORK_NETIO_STATUS_CONNECTED
				?SONORK_RESULT_FORCED_TERMINATION
				:SONORK_RESULT_NETWORK_ERROR;
	ERR.SetSys(_result,GSS_NETCXLOST,SONORK_MODULE_LINE);
	SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
		,&ERR
		,SONORK_CLIENT_DISCONNECT_F_UNEXPECTED
		,"TCP_ConnectionLost()");
}

bool TSonorkClient::TimeSlot()
{
	DWORD 	i;
	bool activity_detected=false;
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	control.timeslot_clock.LoadCurrent();
	if( Running() && !InHandler() )
	{
		//activity_detected = Guts()->TimeSlot(0);
		if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
		{
			// UDP mode
			if(UDP_NetIO().Recv(control.select_wait_msecs)>0)
			{
				activity_detected = true;
				i=0;
				UDP_Recv(UDP_NetIO().Buffer()
					,UDP_NetIO().RecvBytes());
				while(UDP_NetIO().Recv(control.select_wait_msecs)>0)
				{
					UDP_Recv(UDP_NetIO().Buffer()
						,UDP_NetIO().RecvBytes());
					if(++i>10)break;
				}
		   }
		   if(Status()>=SONORK_NETIO_STATUS_CONNECTED)
				UDP_Send();
		   else
		   if(Status()>=SONORK_NETIO_STATUS_CONNECTING)
				UDP_ConnectingTimeSlot();

		}
		else
		{
			// TCP mode
			// Processes up to 10 incomming packets
			i=0;
			while((TCP_Control()->i_packet=TCP_NetIO().Recv(control.select_wait_msecs))!=NULL)
			{
				//TCP_ProcessInPacket will process the i_packet and delete it.
				TCP_ProcessInPacket();
				if(++i>10)break;
			}
			if(i>0)activity_detected = true;

		   // Check the status
			if(TCP_NetIO().Status()==SONORK_NETIO_STATUS_DISCONNECTED)
			{
				TCP_ConnectionLost();
			}
			else
			if(Status()>=SONORK_NETIO_STATUS_CONNECTED)
			{
				if( TCP_Send() )
					activity_detected = true;
			}
		}
		if(TimeSlotClock().IntervalSecsAfter( MonitorClock() ) >= SONORK_CLIENT_DEFAULT_MONITOR_CHECK_SECS)
		{
			if(Status()>=SONORK_NETIO_STATUS_CONNECTED)
				DoMonitor();

/*
#if defined(SONORK_LINUX_BUILD)
			sonork_printf("TimeSlotClock=%lf  - MonitorClock=%lf = %lf"
				,TimeSlotClock().GetClockValue()
				,control.monitor_clock.GetClockValue()
				,TimeSlotClock().GetClockValue() - control.monitor_clock.GetClockValue() );
#endif
*/
			control.monitor_clock.Set( TimeSlotClock() );
		}
	}
	control.mutex.Unlock();

	return activity_detected;
}


//////////////////////
// UDP_Recv
//  Private - Internal
//  Processes the incomming packets and calls the
//  owner application's event/request callback handlers.
void
 TSonorkClient::UDP_Recv(BYTE*raw_data, DWORD recv_size)
{
	DWORD 						ex_size,akn_count;
	SONORK_UDP_AKN_RESULT			akn_result;
	TSonorkUdpPacketHeader 			*hdr;

	if( recv_size < sizeof(TSonorkUdpPacketHeader) )
	{
		// hmmm. This should never happen, we've probably established
		// a connection to something else than a Sonork server.
		return;
	}
	// An entry packet starts with a HFLAG.
	// An HFLAG describes the type of packet.

	hdr=(TSonorkUdpPacketHeader*)raw_data;
	hdr->Normalize();

	if( hdr->Type() == SONORK_NETIO_HFLAG_TYPE_PACKET )
	{
		TSonorkUdpPacketEntry	*entry=(TSonorkUdpPacketEntry*)hdr;

		// A DATA packet: Are we running?
		if(Status()<SONORK_NETIO_STATUS_CONNECTED)
			return ;

		// Compare the session id (SID) with the one we obtained when
		// we established the connection.
		if( entry->SID() == control.userAddr.sid.SessionId() )
		{
			// Ok, a packet with our SID on it, process it.

			// Does the packet have included some AKNs on reply to the
			// entries we've sent?
			akn_count=entry->AknCount();
			if(akn_count)
			{
				// Yes: The server is telling us that it has succesfully
				// received the entries it lists after the packet prefix.

				// So, now make sure the size of the packet can hold the
				// amount of AKNs the server claims it has sent.
				ex_size=sizeof(*entry)+entry->AknSize();
				if(recv_size>=ex_size)
				{
					// Packet size check ok.
					// Process every QID (Paket entry ID): ProcessAkn()
					// compares the QID with the list of QIDs waiting for an AKN,
					// and marks those to indicate transmission complete.
					UDP_ProcessAkn(entry->AknPtr(),akn_count);
				}
			}

			// Does the packet contain an entry for an incomming packet?
			if(entry->QID()!=SONORK_UDP_PACKET_INVALID_QID)
			{

				//SONORK_Trace("%x.ENTRY",entry->QID());
				// If the QID was succesfully processed add it to the outgoing queue
				// from where Send() obtains the list of QIDs it should send as AKNs
				// to the server, telling it we have received the entry.
				akn_result=UDP_ProcessPacketEntry(entry,recv_size);
				if(akn_result>SONORK_UDP_AKN_RESULT_NOP)
				{
					UDP_OutAknQueue().Add(entry->QID()
						|(akn_result>=SONORK_UDP_AKN_RESULT_ALL
							?SONORK_UDP_PACKET_QID_F_AKN_COMPLETE:0));
				}
			}
		}
		else
			sonork_printf("UDP packet for another SID received");

	}
	else
	if(hdr->Type()==SONORK_NETIO_HFLAG_TYPE_LOGIN)
	{
		// Are we connecting?
		if(Status()<SONORK_NETIO_STATUS_CONNECTED)
		{
			ProcessLoginAkn(hdr->DataPtr()
				, recv_size - sizeof(TSonorkUdpPacketHeader));

			//UDP_Control()->max_akn_timeout_msecs	= udp_login_akn->max_akn_msecs;
			//UDP_Control()->max_pck_timeout_msecs	= udp_login_akn->max_pck_msecs;
			//UDP_Control()->max_akn_send_msecs		= udp_login_akn->max_akn_delay_msecs;
		}

	}
	else
	if(hdr->Type()==SONORK_NETIO_HFLAG_TYPE_LOGOUT)
	{
		sonork_printf("UDP::LOGOUT packet (SID=%x)",hdr->SID());
		// It has the LOGOUT flag, which means the server is terminating
		// the connection.
		// Check if the session id (SID) matches ours.
		if( hdr->SID()==control.userAddr.sid.SessionId() )
		{
			TSonorkError ERR;
			control.flags|=SONORK_CLIENT_F_SERVER_DISCONNECT;
			ERR.SetSys(SONORK_RESULT_FORCED_TERMINATION
				,GSS_NETCXSVRKILL
				,SONORK_MODULE_LINE);
			// Yes, SID matches ok. Too bad, we've been disconnected.
			SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
				,&ERR
				,SONORK_CLIENT_DISCONNECT_F_UNEXPECTED
				|SONORK_CLIENT_DISCONNECT_F_KILLED
				,"UDP_Recv( HFLAG_TYPE_LOGOUT )"
				);
		}
	}
}

//////////////////////
// TCP_Send
//  Private. Called by TimeSlot when a connection has been established.
//  It processes outgoing packets.
bool
 TSonorkClient::TCP_Send()
{
	if(control.requests_queue.Items())
	{
		if( control.requests_table.CurEntries() < control.requests_table.MaxEntries() )
		{
			TSonorkClientRequest *request;
			request=control.requests_queue.RemoveFirst();
			assert(request!=NULL);

			request->idle_clk.Set( TimeSlotClock() );
			control.requests_table.AddEntry( request );
			TCP_NetIO().SendPacket( request->ReleaseTcpPacket() );
		}
		return true;
	}
	return false;
}

//////////////////////
// UDP_Send
//  Private. Called by TimeSlot when a connection has been established.
// 'It processes outgoing packets. Returns false if idle, true if something
//  is being transmitted
bool
 TSonorkClient::UDP_Send()
{
	DWORD		entries
			,	entry_no
			,	entries_sent
			,	max_entries_to_send;
	SONORK_UDP_PACKET_ENTRY_FLAGS
			*entry_flags;
	bool	akn_timeout_exceeded;
	bool	activity_detected;

	// Check if we're currently trasmitting a packet
	if( !UDP_OutPacket() )
	{
		// If not, check if there is one in the outgoing queue
		if(control.requests_queue.Items())
		{
		// Check if the request table (where we hold all requests
		// that have been sent to the server but have not finished yet)
		// is not full
			if(control.requests_table.CurEntries() < control.requests_table.MaxEntries())
			{
				TSonorkClientRequest *request;
				request=control.requests_queue.RemoveFirst();
				assert( request!=NULL );

				request->idle_clk.Set( TimeSlotClock() );
				control.requests_table.AddEntry( request );

				UDP_Control()->o_packet = request->ReleaseUdpPacket();

				UDP_ResetPckTime();
				UDP_ResetAknTime();
				/*
				entry_no=UDP_PckTimeMsecs();
				SONORK_Trace("%08u:TASK[%04x] TX START UID=%08x, AGE=%u/%u"
					,TimeSlotClock().tv
					,request->TaskId()
					,UDP_OutPacket()->UID()
					,entry_no
					,PckTimeoutMsecs()
					);
				*/
			}
		}

		if(UDP_OutPacket())
		{
		// Well, excuuuuuussssee meee for using a 'goto'
		// the code is complex as it is, trying to do
		// this without a goto just increased the complexity
		// (which, by the way, is the reason for not using gotos)
		// so stop crying: The goto's label is just a few lines below
			goto _SEND_O_PACKET_;
		}
		activity_detected = false;
	}
	else
	{
		entry_no = UDP_PckTimeMsecs();
		/*SONORK_Trace("%08u:UID [%08x] TX CONTINUE AGE=%u/%u"
			,TimeSlotClock().tv
			,UDP_OutPacket()->UID()
			,entry_no
			,PckTimeoutMsecs());
		*/
		// We're currently transmitting a packet, check for time out
		if( entry_no > PckTimeoutMsecs() )
		{
			// Packet timed out
			TSonorkError ERR;
			UDP_OutPacketTxTimeout();
			ERR.SetSys(SONORK_RESULT_NETWORK_ERROR,GSS_NETCXLOST,SONORK_MODULE_LINE);
			SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
				,&ERR
				,SONORK_CLIENT_DISCONNECT_F_UNEXPECTED
				|SONORK_CLIENT_DISCONNECT_F_TIMEOUT
				,"UDP_Send( delay>PckTimeoutMsecs() )");
			return false;
		}
		else
		{

_SEND_O_PACKET_:
			activity_detected = true;

			// Check how many entries the packet has been divided in.
			// The entries are portions of the full packet, whose
			//  sizes do not exceed the UDP limit imposed by the
			//  internet routers.
			// Each entry has a QID (global_task number) compose of
			//  the packet's UID (global_task ID) and the entry sequential number
			entries	=UDP_OutPacket()->Entries();

			// Test if the AKNwoledge timeout has been exceeded.
			//  Check for timeout should be done before sending the entries,
			//  because we reset the AKN timeout each time an entry is sent
			akn_timeout_exceeded = UDP_AknTimeMsecs() >= AknTimeoutMsecs() ;


			// max_entries_to_send controls how many entries we send at once.
			//  If the first entry (header of the packet) has not been aknowleged
			//   we send a small amount in order to increase the probabilities of
			//   the first entry reaching the server. This is necesary because the
			//   server does not process entries until it receives the header.
			//  Once the header has been received, we can send a large amount
			//   because any of them reaching will be processed, regardless of the order
			//   in which they arrive.
			max_entries_to_send = MAX_ENTRIES_PER_TIMESLOT;
			//UDP_OutPacket()->TestEntryFlag(0,SONORK_UDP_ENTRY_F_SENT)
			//?MAX_ENTRIES_PER_TIMESLOT:2;

			for(entry_flags=UDP_OutPacket()->EntryTable()
			,entries_sent=entry_no=0
			;entry_no<entries
			;entry_no++,entry_flags++)
			{
				//  This entry has already been sent?
				if(!(*entry_flags&SONORK_UDP_ENTRY_F_SENT))
				{
				//  No, it has not: send it now.
					if(!UDP_SendEntry( entry_no , false ))
						break;
					else
					{
						if(++entries_sent>max_entries_to_send)
							break;
					}
				}
				else
				{
					//  Yes, it has been sent: Check if it has been aknowledged by the server
					if(!(*entry_flags&SONORK_UDP_ENTRY_F_AKN))
					{
						//  No, it has not been AKNd, check if the timeout for AKN has expired.
						if( akn_timeout_exceeded )
						{
							//  AKN timeout has expired: The packet has
							//   probably been lost, resend it.
							*entry_flags|=SONORK_UDP_ENTRY_F_RESEND;
							*entry_flags&=~SONORK_UDP_ENTRY_F_SENT;
							if(!UDP_SendEntry( entry_no , true ))
								break;
							else
							if(++entries_sent>max_entries_to_send)
								break;
						}
					}
				}
			}
			if(entries_sent)
			{
				//SONORK_Trace("%08u:_ AKN TIMER RESET_",TimeSlotClock().tv);
				UDP_ResetAknTime();
			}
		}
	}

	//  Check if we have to send AKNs in response to incomming packets.
	//  The OutAknQueue() is fed by the Recv() function that processes
	//  the incomming packets.
	if( UDP_OutAknQueue().Items() != 0)
	{
		UDP_SendAkn();
		if( UDP_OutAknQueue().Items()!=0 )
			activity_detected=true;
	}
	return activity_detected;
}
bool
 TSonorkClient::ProcessLoginAkn(BYTE *data,DWORD data_size)
{
	TSonorkError 		   	ERR;
	TSonorkSvrLoginAknPacket	AKN;
	DWORD			   	flags=SONORK_CLIENT_DISCONNECT_F_UNEXPECTED;

	ERR.SetResult(AKN.CODEC_ReadMem(data,data_size));
	if( ERR.Result() != SONORK_RESULT_OK )
	{
		ERR.SetSysExtInfo(GSS_BADCODEC,SONORK_MODULE_LINE);
		flags|=SONORK_CLIENT_DISCONNECT_F_PROTOCOL;
	}
	else
	if(AKN.header.result==SONORK_RESULT_OK)
	{
		ERR.SetResult( CryptContext().Uncrypt( &AKN.sid_info ) );
		if( ERR.Result()!= SONORK_RESULT_OK)
		{
			ERR.SetSysExtInfo(GSS_BADCRYPTTYPE,SONORK_MODULE_LINE);
			flags|=SONORK_CLIENT_DISCONNECT_F_PROTOCOL;
		}
		else
		if( AKN.sid_info.DataSize() < sizeof(TSonorkSvrSidInfo) )
		{
			ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
				,GSS_PCKTOOSMALL
				,SONORK_MODULE_LINE);
			flags|=SONORK_CLIENT_DISCONNECT_F_PROTOCOL;
		}
		else
		{
			TSonorkSvrSidInfo *SI;
			DWORD test_signature;
			SI = (TSonorkSvrSidInfo *)AKN.sid_info.Buffer();
			test_signature=Sonork_GenerateSignature( AKN.sid_info.Buffer(), AKN.sid_info.DataSize() );
			if( test_signature != AKN.header.signature )
			{
				ERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
					,GSS_BADCRYPTSIGNATURE
					,SONORK_MODULE_LINE);
				flags|=SONORK_CLIENT_DISCONNECT_F_PROTOCOL;
			}
			else
			{
				control.userId.Set( AKN.header.userId );
				control.userAddr.OldSet( SI->addr );
				control.sidPin.Set(SI->sidPin);

				control.max_idle_secs 	  = AKN.header.maxIdleSecs;
				control.akn_timeout_msecs = AKN.header.maxAknMsecs;
				control.pck_timeout_msecs = AKN.header.maxPckMsecs;
				server.version		  = AKN.header.serverVersion;
				if(CryptContext().CryptEngine()==SONORK_CRYPT_ENGINE_SIMPLE)
					CryptContext().SetSimple(SI->sidPin);
				ERR.SetOk();
				SetStatus(SONORK_NETIO_STATUS_CONNECTED
					, &ERR
					, AKN.header.loginAknFlags
					, "ProcessLoginAkn( *true* )");
				return true;
			}

		}
	}
	else
	{
		flags|=SONORK_CLIENT_DISCONNECT_F_DENIED;
		ERR.Set((SONORK_RESULT)AKN.header.result
			,"!GL!0000 Server response"
			,SONORK_MODULE_LINE
			,false);
	}
	SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
		,&ERR
		,flags
		,"ProcessLoginAkn( *false* )");
	return false;
}

void TSonorkClient::TCP_ProcessInPacket()
{
	assert(TCP_InPacket()!=NULL);
	if(TCP_InPacket()->Type()==SONORK_NETIO_HFLAG_TYPE_PACKET)
	{
		if(Status()>=SONORK_NETIO_STATUS_CONNECTED)
		{
			ProcessInDataPacket();
			// RETURN: The packet should NOT be deleted as ProcessInDataPacket does that.
			return;
		}
	}
	else
	if(TCP_InPacket()->Type()==SONORK_NETIO_HFLAG_TYPE_LOGIN)
	{
		if(Status()==SONORK_NETIO_STATUS_AUTHORIZING)
			ProcessLoginAkn( TCP_InPacket()->DataPtr() , TCP_InPacket()->DataSize() );
	}
	else
	if(TCP_InPacket()->Type()==SONORK_NETIO_HFLAG_TYPE_CONNECT)
	{
		TSonorkTcpConnectAknPacket *CX=(TSonorkTcpConnectAknPacket*)TCP_InPacket()->DataPtr();
		// Are we connecting?
		if(Status()==SONORK_NETIO_STATUS_CONNECTING)
		{
			if(CX->signature==SONORK_SERVER_SIGNATURE)
			{
				TSonorkError aknERR;
				if(CX->result==SONORK_RESULT_OK)
				{
					aknERR.SetOk();
					SetStatus(SONORK_NETIO_STATUS_AUTHORIZING
						,&aknERR
						,0
						,"TCP_ProcessInPacket(HFLAG_TYPE_CONNECT)");
					TCP_SendConnectRequest();
				}
				else
				{
					aknERR.Set((SONORK_RESULT)CX->result,CX->message,SONORK_MODULE_LINE,false);
					SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
						,&aknERR
						,SONORK_CLIENT_DISCONNECT_F_UNEXPECTED
						|SONORK_CLIENT_DISCONNECT_F_DENIED
						,"TCP_ProcessInPacket(HFLAG_TYPE_CONNECT)");
				}
			}
		}
	}
	//  else IGNORE: we don't recognize the packet.
	Sonork_FreeTcpPacket(TCP_Control()->i_packet);
	TCP_Control()->i_packet=NULL;
}


//////////////////////
// Request
//  Public. Called to initiate a request.
//  Request() returns immediately, it does not wait until the packet
//  is transmitted.
//  It puts a *copy* of the packet in the outgoing queue (O_QUEUE) and
//  puts a reference to it in the O_TABLE.
//  <handler> is the application's callback handler for this request,
//  <handler_param> is an arbitrary pointer that will be passed to the
//  handler along with the event information.
//  If no space is available to hold this request (i.e. there are too
//  many pending requests) it will return SONORK_RESULT_OF_OUT_RESOURCES.
//  If the packet is successfully stored, it's TASK_ID will be returned
//  in the <data_packet> parameter.
//  The application can later call CancelRequest() with the packet's
//  TASK_ID to cancel it.
// NOTE: The owner application should always call CancelRequest to
//   indicate that the handler is no longer valid. This is critical when
//   the handler is a method of a class because the class' instance may be
//   deleted before the packet transmission is complete, so invoking the
//   handler may cause an exception.
SONORK_RESULT
 TSonorkClient::Request(
			 TSonorkError&		      	ERR
			,TSonorkDataPacket	      	*data_packet
			,UINT 			      	 data_packet_size
			,lpfnSonorkRequestHandler	 h_callback
			,void			      	*h_param
			,const SONORK_DWORD2		*h_tag
			,DWORD				*p_new_task_id
			)
{
	SONORK_FUNCTION	function;
	void				*DATA;
	UINT				 DATA_size;
	BOOL				 delete_data;
	DWORD				new_task_id;
	SONORK_RESULT			enc_result;

	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(!Running()||(Protocol()!=SONORK_NETIO_PROTOCOL_UDP&&Protocol()!=SONORK_NETIO_PROTOCOL_TCP))
	{
		ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_NOTCXTED,SONORK_MODULE_LINE);
	}
	else
	if(control.requests_queue.Items() >= SONORK_CLIENT_MAX_PENDING_TASKS)
	{
		ERR.SetSys(SONORK_RESULT_OUT_OF_RESOURCES, GSS_ENGBUSY,SONORK_MODULE_LINE);
	}
	else
	if(data_packet_size<sizeof(TSonorkDataPacket))
	{
		ERR.SetSys(SONORK_RESULT_CODEC_ERROR,GSS_PCKTOOSMALL,SONORK_MODULE_LINE);
	}
	else
	{

		function=data_packet->Function();
		if(++control.cur_task_id>SONORK_UDP_PACKET_UID_MAX_VALUE)
			control.cur_task_id=1;
		new_task_id=control.cur_task_id;
		data_packet->ReqTask().v[TSonorkTask::V_CLIENT]=new_task_id;

		data_packet->NormalizeHeader();

		enc_result=CryptContext().Encrypt(data_packet,data_packet_size,&DATA,&DATA_size,&delete_data);
		if(enc_result==SONORK_RESULT_OK)
		{
			TSonorkClientRequest*	request;
			SONORK_MEM_NEW(
				request=new TSonorkClientRequest(
					new_task_id
				, 	function
				, 	h_callback
				, 	h_param
                ,	h_tag
				)
			);
			if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
			{
				TSonorkUdpPacket* udp_packet;
				SONORK_MEM_NEW(
					udp_packet=new TSonorkUdpPacket(
						 SONORK_UDP_PACKET_VALUE_TO_UID(new_task_id)
						,DATA
						,DATA_size)
				);
				request->SetNetPacket(udp_packet);
			}
			else
			{
				TSonorkTcpPacket		 *tcp_packet;
				tcp_packet=Sonork_AllocTcpPacket(DATA_size);
				tcp_packet->HFlag()	=SONORK_WORD(SONORK_NETIO_HFLAG_TYPE_PACKET);
				memcpy(tcp_packet->DataPtr(),DATA,DATA_size);
				request->SetNetPacket(tcp_packet);
			}

			control.requests_queue.Add(request);
			if(p_new_task_id)*p_new_task_id=new_task_id;
			ERR.SetOk();
		}
		else
			ERR.SetSys(enc_result,GSS_CRYPTFAIL,SONORK_MODULE_LINE);
		if(delete_data&&DATA!=NULL)
			SONORK_MEM_FREE(DATA);
	}
	control.mutex.Unlock();
	return ERR.Result();
}
//////////////////////
// RequestStatus()
//  Public. Returns the status of a request initiated by Request()
//  Returns SONORK_CLIENT_REQUEST_STATUS_INVALID if the request has not
//  been made or has already been processed and no longer exists.
//  Returns SONORK_CLIENT_REQUEST_PENDING if either the request
//  is waiting to be transmitted or it has been transmitted and
//  is waiting for the server's response to it.
//  <handler_param> should match the one passed when creating the request
SONORK_CLIENT_REQUEST_STATUS
	 TSonorkClient::RequestStatus(DWORD task_id)
{
    TSonorkClientRequest   			*request;
    SONORK_CLIENT_REQUEST_STATUS	status;

	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
    if(!Running())
    	status=SONORK_CLIENT_REQUEST_STATUS_INVALID;
    else
    {
        // Get the O_PACKET entry from the O_TABLE
		request=control.requests_table.GetEntry(task_id);
        if( request != NULL )
        {
            if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
            {
                if(UDP_OutPacket()!=NULL)
                {
                    if( UDP_OutPacket()->UID() == request->TaskId())
                    {
                        status=SONORK_CLIENT_REQUEST_STATUS_TRANSMITTING;
                    }
                }
                else
                    status=SONORK_CLIENT_REQUEST_STATUS_PENDING;
            }
            else
				status=SONORK_CLIENT_REQUEST_STATUS_PENDING;
        }
        else
        {
            request=control.requests_queue.Peek(task_id);
            if( request != NULL )
                status=SONORK_CLIENT_REQUEST_STATUS_PENDING;
            else
                status=SONORK_CLIENT_REQUEST_STATUS_INVALID;
        }
    }
	control.mutex.Unlock();
	return status;
}

//////////////////////
// CancelRequest
//  Public. Called to cancel a request. See notes on Request()
//  <handler_param> should match the one passed when creating the request
// NOTE: The application's handler is NOT invoked when the
//  request is cancelled by the application itself.
// On return p_function and p_tag containt the request's function and tag
// Both may be null if function and tag is not required.
bool TSonorkClient::CancelRequest(DWORD task_id
	, SONORK_FUNCTION*p_function
	, SONORK_DWORD2*p_tag)
{
    TSonorkClientRequest   *request;
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );

    // Get the request entry from the output table
    request=control.requests_table.GetEntry( task_id );
    if( request != NULL )
    {
        // The packet has been sent!
        // Get the UDP_PACKET associated to the O_PACKET

        if( Protocol() == SONORK_NETIO_PROTOCOL_UDP )
        {
            if( UDP_OutPacket()!=NULL )
            {
                // Check if it is the packet currently being transmitted.
                if( task_id == UDP_OutPacket()->UIDValue() )
                {
                    // Yes: Stop transmitting current packet.
                    SONORK_MEM_DELETE(UDP_OutPacket());
                    UDP_Control()->o_packet=NULL;
                }
            }
        }

        // Remove reference from the O_TABLE
        control.requests_table.DelEntry( request->TaskId() );
    }
    else
    {
	    // Not in the output table, so try the output queue
	    request=control.requests_queue.Remove(task_id);
	}
    if( request != NULL )
    {
        if(request->Canceled())
        {
        	// It has already been canceled.
	        request=NULL;
		}
        else
        {
        	// Load return values
            if( p_function != NULL )
            	*p_function = request->Function();

			if( p_tag != NULL )
            	p_tag->Set( request->Tag() );

            if( InHandler() && control.active_request == request )
            {
                // Cannot delete the request:
                // It is the request for which we invoked the callback.
                // (we're being called from within the callback)
                // Set the handler to NULL with Cancel() to mark that
                //   the request as deleted.
                // The engine always checks this value when returning
                //  from the callback to see if the request was deleted
                //  while inside the callback.
                request->Cancel();

                // Don't tell the application we've got one task less:
                //   this request is still being processed, the engine
                //   will take care of that as soon as it finds out that
                //   the request has been cancelled.
            }
            else
            {
				// It is safe to delete, get rid of it.
                SONORK_MEM_DELETE(request);

                /*
                	// Tell the application we've got one task less.
					g_event.info.type					=SONORK_CLIENT_EVENT_WORK_TASKS;
				    g_event.info.data.work.tasks		=PendingRequests();
				    InvokeEventHandler();
				*/                    
            }

        }
    }
    control.mutex.Unlock();
	return request!=NULL;
}

//////////////////////
// OutPacketTxTimeout
//  Private. Called by Send() when it could not transmit a packet within
//  the allowed period. If deletes all references to the packet and
//  calls the request callback handler with result SONORK_RESULT_TIMEOUT

void TSonorkClient::UDP_OutPacketTxTimeout()
{
    DWORD task_id;
    TSonorkClientRequest* request;

	assert(UDP_OutPacket()!=NULL);
	task_id=UDP_OutPacket()->UIDValue();
	SONORK_MEM_DELETE(UDP_OutPacket());

	UDP_Control()->o_packet = NULL;
	request = control.requests_table.GetEntry(task_id);

	if( request != NULL)
	{
		TSonorkError ERR;
		sonork_printf("Client:: request %08x (%s) has timed-out"
			,request->TaskId()
			,SONORK_FunctionName( request->Function() ) );

		ERR.SetSys(SONORK_RESULT_TIMEOUT,GSS_REQTIMEOUT,SONORK_MODULE_LINE);
		RequestEnd(request,ERR);
    }
}
//////////////////////
// OutPacketTxComplete
//  Private. Called when an outgoing packet has been completely
//  transmitted.
//  It just deletes the packet to release memory.
void TSonorkClient::UDP_OutPacketTxComplete()
{
	assert( UDP_OutPacket() != NULL);
	SONORK_MEM_DELETE(UDP_OutPacket());
	UDP_Control()->o_packet = NULL;
}

void TSonorkClient::RequestEnd(TSonorkClientRequest*request, TSonorkError&ERR)
{
	DWORD task_id;

	assert(request != NULL);

	task_id	=request->TaskId();

	control.requests_table.DelEntry(task_id);
	if(! request->Canceled() )
	{
		TSonorkClientRequestEvent
			ev(SONORK_CLIENT_REQUEST_EVENT_END);
		ev.pERR	=&ERR;
		InvokeRequestHandler(request,&ev);
	}
	SONORK_MEM_DELETE(request);

}



//////////////////////
// SetConnected
//  Private. Updates the control flags to reflect the current connection status
//  and, if necesary, notifies the owner application by calling its
//  event handler.
void
 TSonorkClient::SetStatus(SONORK_NETIO_STATUS new_status,TSonorkError* statusERR,DWORD sFlags, SONORK_C_CSTR pCaller )
{
	UINT	index;
	SONORK_NETIO_STATUS old_status;

	TSonorkClientRequest	*request;

	old_status = control.status;
	if(new_status==old_status)
		return;
	control.status=new_status;
	if(new_status==SONORK_NETIO_STATUS_DISCONNECTED)
	{
		{
			TSonorkError requestERR;

			if(old_status>=SONORK_NETIO_STATUS_CONNECTED)
				sFlags|=SONORK_CLIENT_DISCONNECT_F_LOST;

			requestERR.SetSys(SONORK_RESULT_NETWORK_ERROR,GSS_NETCXLOST,SONORK_MODULE_LINE);

			//ClearGutsRevCxQueue(requestERR);

			control.requests_table.InitEnum(&index);

			while((request=control.requests_table.EnumNext(&index))!=NULL)
				RequestEnd(request,requestERR);

			while((request=control.requests_queue.RemoveFirst())!=NULL)
				RequestEnd(request,requestERR);
		}

		if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
		{
			if(old_status>=SONORK_NETIO_STATUS_CONNECTED
			&&
			!(control.flags&SONORK_CLIENT_F_SERVER_DISCONNECT))
			{
				UDP_SendDisconnectRequest();
				SONORK_Sleep(150);
				UDP_SendDisconnectRequest();
				SONORK_Sleep(150);
			}
			UDP_NetIO().Shutdown();
		}
		else
		if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
		{
			TCP_NetIO().Shutdown();
		}
		control.flags&=~SONORK_CLIENT_FM_INTERNAL;
		control.global_task.function = SONORK_FUNCTION_NONE;
		control.userAddr.OldClear();
		control.sidPin.Clear();
	}
	else
	if(new_status==SONORK_NETIO_STATUS_CONNECTED)
	{
		control.idle_clock.Set(TimeSlotClock());
		control.monitor_clock.Set(TimeSlotClock());
	}
	app.event.type			= SONORK_CLIENT_EVENT_STATUS_CHANGE;
	app.event.data.status.status	= Status();
	app.event.data.status.flags	= sFlags;
	app.event.pERR		= statusERR;
	InvokeEventHandler();

	// The login_info is only available during connection:
	//  After connecting (or, if connect failed, after the disconnection)
	//   login_info is deleted.
}




//////////////////////
// ConnectingTimeSlot
//  Private. Called by TimeSlot during the connection process
//  it just resends connection requests every X period and
//  checks for timeout.

#define SONORK_DEFAULT_CX_MSECS	20000
void TSonorkClient::UDP_ConnectingTimeSlot()
{
	assert(Protocol()==SONORK_NETIO_PROTOCOL_UDP);
	if( UDP_PckTimeMsecs() > SONORK_DEFAULT_CX_MSECS )
	{
		TSonorkError ERR;
		ERR.SetSys(SONORK_RESULT_TIMEOUT,GSS_REQTIMEOUT,SONORK_MODULE_LINE);
		SetStatus(SONORK_NETIO_STATUS_DISCONNECTED
			,&ERR
			,SONORK_CLIENT_DISCONNECT_F_UNEXPECTED
			|SONORK_CLIENT_DISCONNECT_F_TIMEOUT
			,"UDP_ConnectingTimeSlot( timeout )");
    }
}


//////////////////////
// SendConnectRequest
//  Private. Sends the first packet necesary to create a session on the
//  server. When this packet is answered by the server, we know
//  we've been assigned a session, however at that time we
//  are still NOT authenticated.
//  The owner application will be notified by a call to its event handler
//  and it should send a LOGIN packet in order to authenticate and be able
//  to send other types of packets.


UINT
 TSonorkClient::LoadConnectRequest(BYTE*data,DWORD data_size)
{
	TSonorkSvrLoginReqPacket REQ;


	REQ.Clear();

	app.event.type			=SONORK_CLIENT_EVENT_LOAD_LOGIN_REQ;
	app.event.data.login_req.packet	=&REQ;

	InvokeEventHandler();

	assert( REQ.Prepared() );

	//REQ.header.signature=REQ.password.GenerateSignature();

	CryptContext().Encrypt(&REQ.password);
	CryptContext().GetCryptInfo(&REQ.cryptInfo);

	REQ.CODEC_WriteMem( data , data_size ); // Modifies data_size
	return data_size;
}
void TSonorkClient::UDP_SendConnectRequest()
{
	assert(Protocol()==SONORK_NETIO_PROTOCOL_UDP);
	UINT D_size;

	D_size = LoadConnectRequest(UDP_SendBuffer().header.DataPtr() , SONORK_UDP_PACKET_FULL_ENTRY_SIZE - sizeof(TSonorkUdpPacketHeader));

	UDP_SendBuffer().header.hflag=SONORK_WORD( SONORK_NETIO_HFLAG_TYPE_LOGIN);
	if(!UDP_NetIO().Send(&UDP_SendBuffer(),D_size + sizeof(TSonorkUdpPacketHeader)))
		UDP_ResetAknTime();
}
void TSonorkClient::TCP_SendConnectRequest()
{
	BYTE* data;
	UINT  sz;
	assert(Protocol()==SONORK_NETIO_PROTOCOL_TCP);

	data = SONORK_MEM_ALLOC(BYTE,SONORK_UDP_PACKET_FULL_ENTRY_SIZE);
	
	sz = LoadConnectRequest(data, SONORK_UDP_PACKET_FULL_ENTRY_SIZE);
	TCP_NetIO().SendPacket(SONORK_NETIO_HFLAG_TYPE_LOGIN, data, sz );
	SONORK_MEM_FREE(data);

}

//////////////////////
// SendDisconnectRequest
//  Private. Sends a packet telling the server we are closing the connection.
//  The owner application will be notified by a call to its event handler
//  and it should not issue any other calls until it has established
//  a new connection.

void TSonorkClient::UDP_SendDisconnectRequest()
{
	TSonorkSvrLogoutReqPacket *D;
	UDP_SendBuffer().header.hflag=SONORK_NETIO_HFLAG_TYPE_LOGOUT;
	UDP_SendBuffer().header.sid  =control.userAddr.sid.SessionId();
	D=(TSonorkSvrLogoutReqPacket *)UDP_SendBuffer().header.DataPtr();
	SONORK_ZeroMem(D->reserved,sizeof(D->reserved));

	UDP_NetIO().Send(&UDP_SendBuffer()
		,sizeof(TSonorkUdpPacketHeader)+sizeof(TSonorkSvrLogoutReqPacket)
		);

}



//////////////////////
// ProcessPacketEntry
//  Private. Processes an incomming entry whose sequential number is not zero(0)
//  (i.e. the first entry). The First entry contains an additional DWORD
//  which specifies the full size of the packet, the non zero entries don't.
SONORK_UDP_AKN_RESULT
 TSonorkClient::UDP_ProcessPacketEntry(TSonorkUdpPacketEntry*E,DWORD E_size)
{
	SONORK_UDP_AKN_RESULT akn_result;


	if( E->PacketSize() > SONORK_PACKET_MAX_FULL_SIZE )
	{
		return SONORK_UDP_AKN_RESULT_INVALID;
	}

// LastInPacketUID() stores the UID of the last packet we have received.
// See ProcessPacketHeader() for more information.
	if(UDP_LastInPacketUID()==E->UID())
	{
		akn_result=SONORK_UDP_AKN_RESULT_ALL;
//		sonork_printf("TSonorkClient::ProcessPacketEntry[%x]: (LAST UID)",E->UID());
	}
	else
	{
		// Do we have information for this packet?
		if(UDP_InPacket())
		{

		// Yes, let the packet tell us the result.
			akn_result=UDP_InPacket()->ProcessEntry(E,E_size);

			if( akn_result <= SONORK_UDP_AKN_RESULT_NOP )
				return akn_result;	// Nothing else to do

		// yeah, yeah.. why don't you rewrite this without a goto
		// and see if it gets any clearer or smaller, hugh?
			if( akn_result >= SONORK_UDP_AKN_RESULT_ADD )
				goto ENTRY_PROCESSED;

		// At this point the result should be SONORK_UDP_AKN_RESULT_NEW
		// if its not, there's a solar magnetic storm affecting this PC
			assert(akn_result==SONORK_UDP_AKN_RESULT_NEW);

		// The previous packet has finished ( but not necesarily
		// successfully InPacketTxEnd() will check packet->TxComplete() )
			ProcessInDataPacket();

		}
		// No information for this packet, its a new one,
		// akn_result will be loaded by constructor
		SONORK_MEM_NEW(UDP_Control()->i_packet=new TSonorkUdpPacket(E,E_size,&akn_result));

//* Here's the big fat goto label, wasn't that difficult, was it? *

ENTRY_PROCESSED:

		if(akn_result>=SONORK_UDP_AKN_RESULT_ALL)
		{
		// Processing complete: Pass the new arrived packet to the application
		// and after doing so, delete it to release memory.
			ProcessInDataPacket();
		}
		else
		if(akn_result==SONORK_UDP_AKN_RESULT_INVALID)
		{
			SONORK_MEM_DELETE(UDP_Control()->i_packet);
			UDP_Control()->i_packet=NULL;
		}
	}
	return akn_result ;

}


//////////////////////
// SendEntry
//  Private. Sends a packet with an entry to the server.
//  If there are pending AKNs, they will also be sent along
//  with outgoing packet data.
bool
 TSonorkClient::UDP_SendEntry(DWORD entry_no, bool /*is_resending*/)
{
	DWORD 	data_size,akn_count;
	void  	*packet_data;
	SONORK_UDP_PACKET_QID	*qid_ptr;

	assert(UDP_OutPacket()!=NULL);
	packet_data		=UDP_OutPacket()->EntryPtr(entry_no);
	assert(packet_data!=NULL);
	data_size		=UDP_OutPacket()->EntrySize(entry_no);
	assert(data_size!=0);


	UDP_SendBuffer().entry.qid=SONORK_DWORD(UidEntryToQid(UDP_OutPacket()->UID(),entry_no));
	UDP_SendBuffer().entry.sid=SONORK_DWORD(control.userAddr.sid.SessionId());

	for(akn_count=0
	,qid_ptr=UDP_SendBuffer().entry.AknPtr()
	;akn_count < SONORK_UDPIO_HFLAG_MAX_AKN_COUNT && UDP_OutAknQueue().Items()
	;akn_count++,qid_ptr++)
	{
		*qid_ptr=SONORK_DWORD(UDP_OutAknQueue().RemoveFirst());
	}
	UDP_SendBuffer().entry.hflag		=SONORK_WORD(SONORK_NETIO_HFLAG_TYPE_PACKET|akn_count);
	UDP_SendBuffer().entry.packet_size	=SONORK_DWORD(UDP_OutPacket()->PacketSize());
	UDP_SendBuffer().entry.entry_size	=SONORK_DWORD(UDP_OutPacket()->PacketEntrySize());

	memcpy(UDP_SendBuffer().entry.DataPtr(),packet_data,data_size);

	/*sonork_printf("%s_entry(QID=%08x SZ=%u AC=%u) %u/%u"
				,resend?"RESEND":"send"
				,UDP_SendBuffer().entry.qid
				,data_size
				,akn_count
				,UDP_AknTimeMsecs()
				,AknTimeoutMsecs());
	*/				

	if(!UDP_NetIO().Send(
		&UDP_SendBuffer()
		,UDP_SendBuffer().entry.FullSize(data_size))
		)
	{
		UDP_OutPacket()->SetEntryFlag(entry_no,SONORK_UDP_ENTRY_F_SENT);
		SONORK_Sleep(5);
		return true;
	}
	return false;
}

//////////////////////
// SendAkn
//  Private. Sends a packet with only AKNs to the server.
//  AKNs will also be sent along with outgoing packet data (entries)
//  But if we are currently not trasmitting any packet we need SendAkn()
//  to deliver the AKNS.
void TSonorkClient::UDP_SendAkn()
{
   DWORD  					akn_count;
   SONORK_UDP_PACKET_QID* 	qid_ptr;

    for(akn_count=0
    ,qid_ptr=UDP_SendBuffer().entry.AknPtr()
	;akn_count<SONORK_UDPIO_HFLAG_MAX_AKN_COUNT&&UDP_OutAknQueue().Items()
	;akn_count++,qid_ptr++)
	{
		*qid_ptr=UDP_OutAknQueue().RemoveFirst();
	}
	UDP_SendBuffer().entry.hflag	=(SONORK_NETIO_PACKET_HFLAG)(SONORK_NETIO_HFLAG_TYPE_PACKET|akn_count);
	UDP_SendBuffer().entry.qid		=SONORK_DWORD(SONORK_UDP_PACKET_INVALID_QID);
	UDP_SendBuffer().entry.sid		=SONORK_DWORD(control.userAddr.sid.SessionId());

	UDP_NetIO().Send(&UDP_SendBuffer(),UDP_SendBuffer().entry.FullSize(0));
}





//-----------------------------------------------------------------
//////////////////////
// ProcessAKN
//  Private. Processes an incomming AKN.
//  The server sends an AKN for each entry it has received.
void TSonorkClient::UDP_ProcessAkn(SONORK_UDP_PACKET_QID* qid_ptr,DWORD akn_count)
{
	SONORK_UDP_PACKET_QID	qid;
	SONORK_UDP_AKN_RESULT	akn_result;

	if(UDP_OutPacket())
	{
		UINT	processed_akns=0;
		while(akn_count--)
		{
			qid=*qid_ptr;
			//sonork_printf("QID %08x AKN'd at %u ms",qid,UDP_AknTimeMsecs());

			akn_result=UDP_OutPacket()->ProcessAkn(qid);
			if(akn_result != SONORK_UDP_AKN_RESULT_INVALID)
			{
				processed_akns++;
			}
			else
			{
				sonork_printf("QID %08x is invalid or old",qid);
			}
			if(akn_result>=SONORK_UDP_AKN_RESULT_ALL)
			{
				// Transmission complete: Delete the outgoing packet to release memory.
				//SONORK_Trace("OUT TX COMPLETE [%x]",qid);
				UDP_OutPacketTxComplete();
				break;
			}
			qid_ptr++;
		}
		if( processed_akns > 0 )
		{
			UDP_ResetAknTime();
			UDP_ResetPckTime();
		}
	}
	else
	{
		while(akn_count--)
		{
			sonork_printf("No packet for QID %08x AKN'd at %u ms"
				,*qid_ptr++
				,UDP_AknTimeMsecs());
		}
	}
}
//////////////////////
// ProcessInDataPacket
//  Private. Called when an incomming data packet has been completely received.
//  If the packet is an EVENT, it invokes the application's event handler.
//  If the packet is an AKN, it searches the O_TABLE for the packet's
//  task id and invokes the application handler associated to it.
//  After invoking the handler, it deletes the incomming packet to release memory.
void
 TSonorkClient::ProcessInDataPacket()
{
	bool	complete;
	void	*RAW_packet;
	UINT	RAW_packet_size;
	TSonorkDataPacket	*DATA_packet;
	UINT			 DATA_size;
	BOOL			 del_DATA_packet;

	if(!Ready())return;
	if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
	{
		assert(UDP_InPacket()!=NULL);
		complete=UDP_InPacket()->TxComplete();
		RAW_packet=UDP_InPacket()->Buffer();
		RAW_packet_size=UDP_InPacket()->PacketSize();
		UDP_Control()->last_i_packet_uid=UDP_InPacket()->UID();

	}
	else
	if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
	{
		assert(TCP_InPacket()!=NULL);
		complete=true;
		RAW_packet	=TCP_InPacket()->DataPtr();
		RAW_packet_size	=TCP_InPacket()->DataSize();
	}
	else
		assert(Protocol()==SONORK_NETIO_PROTOCOL_TCP||Protocol()==SONORK_NETIO_PROTOCOL_UDP);

	if(complete)
	{
		CryptContext().Uncrypt(RAW_packet
			,RAW_packet_size
			,(void**)&DATA_packet
			,&DATA_size
			,&del_DATA_packet);
		if( DATA_packet!=NULL )
		{
			if(DATA_size>=sizeof(TSonorkDataPacket))
			{

				DATA_packet->NormalizeHeader();
				if(DATA_packet->PacketType()==SONORK_PACKET_TYPE_EVT)
				{
					ConsumeEventPacket(DATA_packet,DATA_size);
				}
				else
				if(DATA_packet->PacketType()==SONORK_PACKET_TYPE_AKN)
				{
					ConsumeRequestPacket(DATA_packet,DATA_size);
				}
			}
			if(del_DATA_packet)
			{
				SONORK_MEM_FREE(DATA_packet);
			}
		}

	}
	else
	{
		sonork_printf("InPacketTxEnd:PACKET TX NOT COMPLETE!");
		////////////////////////////////////
		// SOME CODE TO HANDLE UNCOMPLETE
		// TRANSMISSION OF UDP PACKETS
	}
	if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
	{
		SONORK_MEM_DELETE(UDP_InPacket());
		UDP_Control()->i_packet=NULL;
	}
	else
	{
		Sonork_FreeTcpPacket(TCP_InPacket());
		TCP_Control()->i_packet=NULL;
	}
}

//-----------------------------------------------------------------

void TSonorkClient::ConsumeEventPacket(TSonorkDataPacket*P,UINT P_size)
{
	TSonorkError 		ERR;
	TSonorkListIterator	index;
	switch(P->Function())
	{
		case  SONORK_SERVER_CLIENT_EVENT_OLD_MSG_NOTIFICATION:
		{
			TSonorkOldMsgNotificationList	list;
			if(P->D_OldMsgNotify_E(P_size,&list))
			{
				TSonorkOldMsgNotification* 	info;
				TSonorkDataPacket*		RP;
				UINT				RP_size,RP_alloc;
				list.InitEnum(index);
				RP_alloc=sizeof(TSonorkOldMsgNotification)+64;
				while( (info=list.EnumNext(index))!=NULL)
				{
					RP=SONORK_AllocDataPacket(RP_alloc);
					RP_size=RP->E_GetMsg_R(RP_alloc,&info->msg_id,0);
					SelfRequest(ERR,RP,RP_size);
				}
			}
		}
		break;

		case  SONORK_SERVER_CLIENT_EVENT_MSG_DELIVERY:
		{
			TSonorkObjId	msg_id;
			TSonorkMsg	msg;
			if(P->D_MsgDelivery_E(P_size,&msg_id,&msg))
			{
				TSonorkDataPacket*	RP;
				UINT			RP_size,RP_alloc;
				RP_alloc=sizeof(TSonorkObjId)+32;
				RP=SONORK_AllocDataPacket(RP_alloc);
				RP_size=RP->E_DelMsg_R(RP_alloc,1,&msg_id);
				SelfRequest(ERR,RP,RP_size);
				ProcessMsg( &msg );
			}
		}
		break;

		case SONORK_SERVER_CLIENT_EVENT_OLD_SID_NOTIFICATION:
		{
			TSonorkOldSidNotificationList 	list;
			TSonorkDynString		text;
			TSonorkOldSidNotification*	N;
			if(P->D_OldSid_E(P_size,&list))
			{
				text.Clear();
				list.InitEnum(index);
				while( (N=list.EnumNext(index))!=NULL)
				{
					app.event.type	 =SONORK_CLIENT_EVENT_USER_SID;
					app.event.user_id.Set(N->locus.userId);
					app.event.data.sid.locus.Set(N->locus);
					app.event.data.sid.user_serial.Set(N->userSerial);
					app.event.data.sid.version.Clear();
					app.event.data.sid.text=&text;
					InvokeEventHandler();
				}
			}

		}
		break;

		case SONORK_SERVER_CLIENT_EVENT_SID_NOTIFICATION:
		{
			TSonorkSidNotificationQueue 	queue;
			TSonorkSidNotificationAtom*	N;
			if(P->D_Sid_E(P_size,&queue))
			{
				while( (N=queue.RemoveFirst())!=NULL)
				{
					app.event.type	 =SONORK_CLIENT_EVENT_USER_SID;
					app.event.user_id.Set(N->locus.userId);
					app.event.data.sid.locus.Set(N->locus);
					app.event.data.sid.user_serial.Set(N->header.user_serial);
					app.event.data.sid.version.Set(N->header.version);
					app.event.data.sid.text=&N->text;
					InvokeEventHandler();
				}
			}
			else
			sonork_printf("D_Sid_E CODEC ERROR!");

		}
		break;

		case SONORK_SERVER_CLIENT_EVENT_CTRL_MSG:
		{
			// Someone has sent a TSonorkCtrlMsg
			// read it into the ctrl_msg buffer and process it
			// we make g_event.info.data.ctrl.msg temporatily point to
			// the buffer to avoid passing it as a parameter.
			TSonorkCtrlMsg 	    msg;
			TSonorkUserLocus1   sender;
			TSonorkDynData	    dyn_data;
			if(P->D_CtrlMsg_E(P_size
			, &sender
			, &msg
			, &dyn_data))
			{
				app.event.type			=SONORK_CLIENT_EVENT_CTRL_MSG;
				app.event.data.ctrl.msg		=&msg;
				app.event.data.ctrl.sender 	=&sender;
				app.event.data.ctrl.flags	=0;
				app.event.data.ctrl.data	=&dyn_data;
				InvokeEventHandler();
			}
		}
		break;

	}
}
//-----------------------------------------------------------------

void TSonorkClient::ConsumeRequestPacket(TSonorkDataPacket*DATA_packet,UINT DATA_size)
{
	TSonorkClientRequest   		*request;
	TSonorkTask					gu_task;
	gu_task.Set(DATA_packet->AknTask());

	request=control.requests_table.GetEntry(gu_task.Client());
	if( request )
	{
		TSonorkError ERR;

		// Save the packet that we're processing
		// so that CancelRequest() does not delete it
		// while we are still processing it
		request->idle_clk.Set( TimeSlotClock() );
		//SONORK_Trace("TSonorkClient::ConsumeRequestPacket(TID=%x)",gu_task.Client());

		control.active_request = request;
		if( request->Function() == DATA_packet->Function())
		{
			control.idle_clock.Set(TimeSlotClock());
			if(!DATA_packet->TestMarks(SONORK_PCTRL_MARK_ERROR|SONORK_PCTRL_MARK_NO_DATA))
			{
				TSonorkClientRequestEvent ev(SONORK_CLIENT_REQUEST_EVENT_PACKET);

				ev.packet			=DATA_packet;
				ev.packet_size		=DATA_size;
				InvokeRequestHandler( request , &ev);
				ERR.SetOk();
			}
			else
			{
				DATA_packet->D_Error_A(DATA_size,ERR);
				ERR.SetLocal(false);
			}
		}
		else
		{
			// Cancel the request so that the callback is not invoked
			// and it is deleted
			request->Cancel();

			ERR.SetSys(SONORK_RESULT_INTERNAL_ERROR,GSS_SYNCHFAIL,SONORK_MODULE_LINE);
		}
        
		if(gu_task.Server()==SONORK_INVALID_TASK_ID || request->Canceled())
			RequestEnd(request , ERR);
		control.active_request=NULL;
	}
	else
		sonork_printf("O_PACKET not found for task %u",gu_task.Client());

}

//-----------------------------------------------------------------
// InvokeEventHandler
//  Private. Invokes the client application's event callback .
//  Sets the HANDLER flag to prevent recusive calls.

void TSonorkClient::InvokeEventHandler()
{
	//assert(handler_callback!=NULL);
	if(!(control.flags&SONORK_CLIENT_F_NO_CALLBACK))
	{
		//SONORK_Trace("InvokeHandler(%x) Event=%u",handler,event.info.Type());

		control.handler_depth++;
		control.mutex.Unlock();	// Mutex is set at Entry point of TimeSlot()

		app.handler->ProcessSonorkEvent(app.tag,&app.event);
		//(*g_event.handler.callback)(g_event.handler.param,&g_event.info);

		control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );	// Lock it again
		control.handler_depth--;
	}
}

void TSonorkClient::InvokeRequestHandler(TSonorkClientRequest*request, TSonorkClientRequestEvent* event)
{
	assert( !request->Canceled() );
	if(!(control.flags&SONORK_CLIENT_F_NO_CALLBACK))
	{
		//SONORK_Trace("InvokeHandler(%x) Event=%u",handler,event.info.Type());

		control.handler_depth++;
        event->request = request;
		control.mutex.Unlock();	// Mutex is set at Entry point of TimeSlot()

		(*request->HandlerCallback())(request->HandlerParam(),event);

		control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );	// Lock it again
		control.handler_depth--;
	}
}


//-------------------------
// TSonorkClientRequestTable


TSonorkClientRequestTable::TSonorkClientRequestTable()
{
    allocated_entries=0;
    table_size=SONORK_CLIENT_MAX_ACTIVE_TASKS;
    SONORK_MEM_NEW(table_ptr=new TSonorkClientRequest*[table_size]);
	SONORK_ZeroMem(table_ptr,sizeof(TSonorkClientRequest*)*table_size);
}

TSonorkClientRequestTable::~TSonorkClientRequestTable()
{
	SONORK_MEM_DELETE_ARRAY(table_ptr);
}

bool
	TSonorkClientRequestTable::AddEntry(TSonorkClientRequest	*R)
{
	UINT entry_no;
	TSonorkClientRequest**dPtr;

	for(entry_no=0,dPtr=table_ptr;entry_no<table_size;entry_no++,dPtr++)
		if(!*dPtr)break;

	if(entry_no==table_size)return false;
	*dPtr=R;
	allocated_entries++;
	return true;
}

TSonorkClientRequest	*
 TSonorkClientRequestTable::GetEntry(DWORD task_id) const
{
	UINT entry_no;
	TSonorkClientRequest**dPtr;
	for(entry_no=0,dPtr=table_ptr;entry_no<table_size;entry_no++,dPtr++)
		if(*dPtr)
		if((*dPtr)->TaskId()==task_id)
			return *dPtr;
	return NULL;
}

bool
 TSonorkClientRequestTable::DelEntry(DWORD task_id)
 {
	DWORD entry_no;
	TSonorkClientRequest**dPtr;
	for(entry_no=0,dPtr=table_ptr;entry_no<table_size;entry_no++,dPtr++)
		if(*dPtr)
		if((*dPtr)->TaskId()==task_id)
		break;
	if(entry_no<table_size)
	{
		allocated_entries--;
		*dPtr=0;
		return true;
	}
	return false;
}
/*bool  TSonorkClientRequestTable::IsZero()
{
   TSonorkClientRequest** dPtr;
    UINT i;
    for(dPtr=table_ptr,i=0;i<table_size;i++,dPtr++)
	if(*dPtr)return false;
   return true;
}
*/

TSonorkClientRequest*
 TSonorkClientRequestTable::EnumNext(UINT*i) const
{
	UINT entry_no=*i;
	TSonorkClientRequest** dPtr;
	if(entry_no<table_size)
	{
		for(dPtr=table_ptr+entry_no;entry_no<table_size;entry_no++,dPtr++)
		{
			if(*dPtr)
			{
				*i=entry_no+1;
				return *dPtr;
			}

		}
		*i=entry_no;
	}
	return NULL;
}
SONORK_RESULT
	TSonorkClient::CreateNetIO(TSonorkError&ERR)
{
	DestroyNetIO();

	CryptContext().SetSimple( SONORK_Random(0xf000) );

	control.akn_timeout_msecs=SRK_DEFAULT_AKN_MSECS;
	control.pck_timeout_msecs=SRK_DEFAULT_PCK_MSECS;
	ERR.SetOk();
	if(server.phys_addr.Type() == SONORK_PHYS_ADDR_UDP_1)
	{
		server.protocol=SONORK_NETIO_PROTOCOL_UDP;
		SONORK_MEM_NEW(control.udp=new TSonorkClientUdpControl);
		control.udp->i_packet=NULL;
		control.udp->o_packet=NULL;
		control.udp->last_i_packet_uid=SONORK_UDP_PACKET_INVALID_QID;
	}
	else
	if(server.phys_addr.Type() == SONORK_PHYS_ADDR_TCP_1)
	{
		server.protocol=SONORK_NETIO_PROTOCOL_TCP;
		SONORK_MEM_NEW(control.tcp=new TSonorkClientTcpControl);
		TCP_Control()->i_packet=NULL;
	}
	else
	{
		ERR.SetSys(SONORK_RESULT_INVALID_PARAMETER,GSS_BADNETPROTOCOL,SONORK_MODULE_LINE);
	}
	if(ERR.Result()!=SONORK_RESULT_OK)
		DestroyNetIO();
	return ERR.Result();
}
void  TSonorkClient::DestroyNetIO()
{
	if(server.protocol==SONORK_NETIO_PROTOCOL_UDP)
	{
		UDP_NetIO().Shutdown();
		if(UDP_InPacket())
			SONORK_MEM_DELETE(UDP_InPacket());
		if(UDP_OutPacket())
			SONORK_MEM_DELETE(UDP_OutPacket());

		SONORK_MEM_DELETE(control.udp);
		control.udp=NULL;
	}
	else
	if(server.protocol==SONORK_NETIO_PROTOCOL_TCP)
	{
		TCP_NetIO().Shutdown();
		SONORK_MEM_DELETE(control.tcp);
		control.tcp=NULL;
	}
	server.protocol=SONORK_NETIO_PROTOCOL_NONE;
}
TSonorkClientRequest::TSonorkClientRequest(DWORD p_task_id
	,SONORK_FUNCTION 		p_function
	,lpfnSonorkRequestHandler	h_callback
	,void*					h_param
	,const SONORK_DWORD2*		h_tag)
{
	task_id	=p_task_id;
	function=p_function;
	handler.callback	=h_callback;
	handler.param		=h_param;
	if(h_tag)
		handler.tag.Set(*h_tag);
	else
		handler.tag.Clear();

	pck.udp=NULL;
	pck.tcp=NULL;

}
TSonorkClientRequest::~TSonorkClientRequest()
{
	if(pck.udp != NULL)
	{
		SONORK_MEM_DELETE(pck.udp);
	}
	else
	if(pck.tcp != NULL)
	{
		Sonork_FreeTcpPacket( pck.tcp );
	}
}

void TSonorkClientRequest::SetNetPacket(TSonorkTcpPacket *p)
{
	assert( pck.tcp==NULL && pck.udp==NULL);
	pck.tcp=p;
}
void TSonorkClientRequest::SetNetPacket(TSonorkUdpPacket *p)
{
	assert( pck.tcp==NULL && pck.udp==NULL);
	pck.udp=p;
}

TSonorkUdpPacket *TSonorkClientRequest::ReleaseUdpPacket()
{
	TSonorkUdpPacket *P;
	assert( pck.udp!=NULL );
	P=pck.udp;
	pck.udp=NULL;
	return P;
}

TSonorkTcpPacket *TSonorkClientRequest::ReleaseTcpPacket()
{
	TSonorkTcpPacket *P;
	assert( pck.tcp!=NULL );
	P=pck.tcp;
	pck.tcp=NULL;
	return P;
}

TSonorkClientRequest* TSonorkClientRequestQueue::Peek(DWORD t)
{
	TSonorkClientRequest* R;
	TSonorkListIterator  I;

	BeginEnum(I);
	while( (R=EnumNext(I))!=NULL )
		if( R->TaskId() == t )
			break;
	EndEnum(I);
	return R;

}
TSonorkClientRequest* TSonorkClientRequestQueue::Remove(DWORD t)
{
	TSonorkClientRequest* R=Peek(t);
    if(R)w_Remove(R);
    return R;
}
void  TSonorkClient::EnableCallbacks(bool v)
{
    if(v)
		control.flags&=~SONORK_CLIENT_F_NO_CALLBACK;
    else
        control.flags|=SONORK_CLIENT_F_NO_CALLBACK;
}



void TSonorkClient::DoMonitor()
{
	UINT idle_secs = TimeSlotClock().IntervalSecsAfter( IdleClock() );

/*
#if defined(SONORK_LINUX_BUILD)
	sonork_printf("TimeSlotClock=%lf  - IdleClock=%lf = %lf (%u)"
		,TimeSlotClock().GetClockValue()
		,IdleClock().GetClockValue()
		,TimeSlotClock().GetClockValue() - IdleClock().GetClockValue()
		,idle_secs);
#endif
*/
	if( control.requests_table.CurEntries() )
	{
		TSonorkClientRequest	*request;
		UINT				index,to;
		UINT    			req_timeout_secs;

		req_timeout_secs =   ReqTimeoutMsecs() / 1000;
		control.requests_table.InitEnum(&index);
		while((request=control.requests_table.EnumNext(&index))!=NULL)
		{
			to=TimeSlotClock().IntervalSecsAfter( request->idle_clk );
			if(to>(req_timeout_secs/2)+1)
				sonork_printf("TSonorkClient::DoMonitor:TID=%x TO=%u/%u",request->TaskId(),to,req_timeout_secs);
			if( to > req_timeout_secs )
			{
				TSonorkError ERR;
				ERR.SetSys(SONORK_RESULT_TIMEOUT
						,GSS_REQTIMEOUT
						,SONORK_MODULE_LINE);
				RequestEnd(request,ERR);
			}
		}
	}
//	sonork_printf("IDLE_SECS = %03u / %03u",idle_secs,control.max_idle_secs);
	if( idle_secs >= control.max_idle_secs )
	{
		app.event.data.monitor.idle=true;
		PingServer(idle_secs);
		control.idle_clock.Set( TimeSlotClock() );
	}
	else
	{
		app.event.data.monitor.idle=false;
		//CheckGutsRevCxQueue();
	}
	app.event.type=SONORK_CLIENT_EVENT_MONITOR;
	InvokeEventHandler();
}


void TSonorkClient::PingServer( UINT )
{
	TSonorkError ERR;
	UINT P_size;
	TSonorkDataPacket *P;
	P=SONORK_AllocDataPacket(64);
	P_size=P->E_Ping_R();
	SelfRequest(ERR,P,P_size);
}
bool
	TSonorkClient::UsingSocks() const
{
	if(Status() > SONORK_NETIO_STATUS_DISCONNECTED)
	{
		if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
			return control.tcp->net_io.SocksEnabled();
	}
	return false;

}

void
 TSonorkClient::RegisterTrackers(
			 TSonorkError&		  ERR
			,TSonorkTrackerDataQueue& queue)
{
	UINT	A_size,P_size;
	TSonorkTrackerData *data;
	TSonorkDataPacket *P;

	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(MayStartGlobalTask(ERR))
	{
		A_size = queue.SumAtomsCodecSize() + 32;
		while( A_size >= 65536 )
		{
			sonork_printf("REGISTER_TRACKERS: SZ=%u (TOO LARGE)",A_size);
			assert( (data = queue.RemoveFirst()) != NULL );
			SONORK_MEM_DELETE( data );
			A_size = queue.SumAtomsCodecSize() + 32;
		}
		sonork_printf("REGISTER_TRACKERS: SZ=%u",A_size);
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_RegisterTracker_R(A_size
			, false
			, queue);
		SelfRequest(ERR,P,P_size);
		if( ERR.Result() == SONORK_RESULT_OK )
			OnGlobalTaskStarted( SONORK_FUNCTION_REGISTER_TRACKER , NULL);
	}
	control.mutex.Unlock();
	queue.Clear();

}

void
 TSonorkClient::SyncSid(TSonorkError&ERR
			, const TSonorkSidFlags&	sid_flags
			, const TSonorkPhysAddr&	phys_addr
			, const TSonorkRegion&		region
			, const TSonorkDynString&	msg)
{
	UINT 	P_size,A_size;
	TSonorkDataPacket *P;
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(MayStartGlobalTask(ERR))
	{
		A_size=msg.CODEC_Size()+64;
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_SetSid_R(A_size
			,sid_flags
			,region
			,phys_addr
			,msg);
		SelfRequest(ERR,P,P_size);
		if( ERR.Result() == SONORK_RESULT_OK )
			OnGlobalTaskStarted( SONORK_FUNCTION_SET_SID , NULL);
	}
	control.mutex.Unlock();
}
void
	TSonorkClient::RefreshWappList(TSonorkError&ERR)
{
	StartGlobalTask(ERR,SONORK_FUNCTION_WAPP_LIST);
}
void
	TSonorkClient::RefreshUserList(TSonorkError&ERR)
{
	StartGlobalTask(ERR,SONORK_FUNCTION_USER_LIST);
}
SONORK_RESULT
	TSonorkClient::GetSysMsgs(TSonorkError& ERR,const TSonorkTime& pTime, const TSonorkRegion& pRegion)
{
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(MayStartGlobalTask(ERR))
	{
		TSonorkDataPacket *P;
		UINT 			P_size;
#define A_size	64
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_GetSysMsgs_R(A_size , pTime, pRegion);
		SelfRequest(ERR,P,P_size);
#undef 	A_size
		if( ERR.Result() == SONORK_RESULT_OK )
			OnGlobalTaskStarted( SONORK_FUNCTION_GET_SYS_MSGS , NULL);
	}
	control.mutex.Unlock();
	return ERR.Result();
}



void
 TSonorkClient::RefreshUser(TSonorkError&ERR,const TSonorkId&user_id,const TSonorkAuth2&auth)
{
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(Status()<SONORK_NETIO_STATUS_CONNECTED)
		_ERRNotConnected(ERR);
	else
	{
		UINT P_size,A_size;
		TSonorkDataPacket *P;
//		sonork_printf("Refreshing user (%u.%u)",user_id.v[0],user_id.v[1]);
		A_size=64;
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_GetUserData_R(A_size
				,user_id
				,SONORK_USER_INFO_LEVEL_1
				,0
				,auth.pin
				,0
				);
		SelfRequest(ERR,P,P_size,&user_id);
	}
	control.mutex.Unlock();
}

// --------------------------------------------
// SELF REQUESTS
//  These are requests that are handled by the engine internl handler
//  There are two types of self requests/tasks: Global and Local
//  Global tasks generate start/end GLOBAL_TASK events
//   and only one can be active at a given time. They
//   may generate other [derived] events while executing.
//  Local tasks are silent and don't generate start/end events,
//   but they can result in other events being fired
//   (i.e. RefreshUser can generate a USER_SET event)
//   also, any amount of local tasks may be active at a given time

DWORD TSonorkClient::SelfRequest(TSonorkError&ERR
	,TSonorkDataPacket*P
	,UINT P_size
	,const SONORK_DWORD2*p_tag)
{
	DWORD task_id;
	Request(ERR,P,P_size,SelfHandler,this,p_tag,&task_id);
	SONORK_FreeDataPacket(P);
	return task_id;
}
void
 SONORK_CALLBACK TSonorkClient::SelfHandler(void *param , TSonorkClientRequestEvent *E)
{
	TSonorkClient *_GU=(TSonorkClient*)param;
	// mutex is locked at TimeSlot() and unlocked
	// before calling any event and then relocked
	// after returning from the event.

	// Because the SelfHandler an event handler and it may
	// invoke events itself, we should relock the mutex.

	_GU->control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );

	if( E->EventType() == SONORK_CLIENT_REQUEST_EVENT_PACKET )
		_GU->SelfHandleDataPacket(E);
	else
	if( E->EventType() == SONORK_CLIENT_REQUEST_EVENT_END )
		_GU->SelfHandleRequestEnd(E);

	_GU->control.mutex.Unlock();
}

void
 TSonorkClient::SelfHandleDataPacket(TSonorkClientRequestEvent*E)
{
	TSonorkDataPacket*P;
	UINT 	P_size;
	union
	{
		TSonorkUserDataNotes*	udn;
		TSonorkWappData*	wapp;
		TSonorkRegion		region;
	}PTR;
	P=E->Packet();
	P_size=E->PacketSize();

	switch( P->Function() )
	{
		case SONORK_FUNCTION_GET_MSG:
		if( P->SubFunction() == SONORK_SUBFUNC_NONE )
		{
			TSonorkMsg msg;
			if(P->D_GetMsg_A(P_size,&msg))
				ProcessMsg(&msg);
		}
		break;

		case SONORK_FUNCTION_SET_SID:
		if( P->SubFunction() == SONORK_SUBFUNC_NONE )
		{
			P->D_SetSid_A( P_size
				,	control.userAddr.sidFlags
				,	PTR.region	// dummy: Ignore
				,	control.userAddr.physAddr
				);
		}
		break;

		case SONORK_FUNCTION_USER_LIST:
		if( P->SubFunction() == SONORK_SUBFUNC_USER_LIST_USERS)
		{
			TSonorkUserDataNotesQueue	queue;
			P->D_UserListUsers_A(P_size,queue);
			while( (PTR.udn=queue.RemoveFirst())!=NULL )
			{
				if(PTR.udn->user_data.GetUserInfoLevel()
					>= SONORK_USER_INFO_LEVEL_1)
				{
					app.event.type	   		= SONORK_CLIENT_EVENT_USER_ADD;
					app.event.data.user_set.flags	= SONORK_MSG_SF_SELF_TRIGGERED;
					app.event.user_data		= &PTR.udn->user_data;
					app.event.data.user_set.notes	= &PTR.udn->notes;
					app.event.data.user_set.auth	= &PTR.udn->auth;
					app.event.user_id.Set( PTR.udn->user_data.userId );
					InvokeEventHandler();
				}
				else
					sonork_printf("------USER_LIST_PACKET: USER(%u.%u) LEVEL=%u--------"
						,PTR.udn->user_data.userId.v[0]
						,PTR.udn->user_data.userId.v[1]
						,PTR.udn->user_data.GetUserInfoLevel()
						);
				SONORK_MEM_DELETE( PTR.udn );
			}
		}
		else
		if( P->SubFunction() == SONORK_SUBFUNC_USER_LIST_GROUPS)
		{
			SelfHandleGroups(P,P_size);
		}
		break;

		case SONORK_FUNCTION_GET_SYS_MSGS:
		if( P->SubFunction() == SONORK_SUBFUNC_NONE)
		{
			TSonorkSysMsgQueue	queue;
			P->D_GetSysMsgs_A(P_size,queue);

			app.event.data.sys_msg.index = 0;
			while( (app.event.data.sys_msg.ptr=queue.RemoveFirst())!=NULL  )
			{
				app.event.type	   = SONORK_CLIENT_EVENT_SYS_MSG;
				InvokeEventHandler();
				SONORK_MEM_DELETE( (void*)app.event.data.sys_msg.ptr );
				app.event.data.sys_msg.index++;
			}
		}
		break;

		case SONORK_FUNCTION_WAPP_LIST:
		if( P->SubFunction() == SONORK_SUBFUNC_WAPP_LIST_APPS)
		{
			TSonorkWappDataQueue		queue;
			P->D_ListWapps_A(P_size,queue);
			while( (app.event.data.wapp=queue.RemoveFirst())!=NULL )
			{
				app.event.type	  = SONORK_CLIENT_EVENT_WAPP_ADD;
				InvokeEventHandler();
				SONORK_MEM_DELETE(app.event.data.wapp);
			}
		}
		else
		if( P->SubFunction() == SONORK_SUBFUNC_WAPP_LIST_WAPP_GROUPS)
		{
			SelfHandleGroups(P , P_size );
		}
		else
		if( P->SubFunction() == SONORK_SUBFUNC_WAPP_LIST_MTPL_GROUPS)
		{
			SelfHandleGroups(P , P_size );
		}
		break;

		case SONORK_FUNCTION_GET_USER_DATA:

		if( P->SubFunction() == SONORK_SUBFUNC_NONE )
		{
			TSonorkUserDataNotes  ND;
			if(P->D_GetUserData_A(P_size,ND))
			{
				if(ND.user_data.GetUserInfoLevel()>=SONORK_USER_INFO_LEVEL_1)
				{
					app.event.type	 				= SONORK_CLIENT_EVENT_USER_SET;
					app.event.user_data 			= &ND.user_data;
					app.event.data.user_set.notes	= &ND.notes;
					app.event.data.user_set.auth	= &ND.auth;
					InvokeEventHandler();
				}
				else
				{
					sonork_printf("TSonorkClient::GET_USER_DATA,UserInfoLevel() < SONORK_USER_INFO_LEVEL_1");
				}
			}
			else
			{
				sonork_printf("TSonorkClient::GET_USER_DATA, DECODE_FAILURE");
			}
		}
		break;


	}
}
void
 TSonorkClient::SelfHandleGroups(TSonorkDataPacket*P,UINT P_size)
{
	TSonorkGroup				*entry;
	TSonorkGroupQueue			queue;
	if( P->D_UserGroups_A(P_size,queue) )
	{
		while( (entry=queue.RemoveFirst())!=NULL )
		{
			app.event.type	 		= SONORK_CLIENT_EVENT_USER_GROUP_ADD;
			app.event.data.group.ptr 	= entry;
			InvokeEventHandler();
			SONORK_MEM_DELETE(entry);
		}
	}
}
void TSonorkClient::SelfHandleRequestEnd(TSonorkClientRequestEvent* E)
{
	switch( E->RequestFunction() )
	{
		case SONORK_FUNCTION_GET_USER_DATA:
		{
			app.event.type	 			= SONORK_CLIENT_EVENT_USER_SYNCH_END;
			app.event.user_id.Set(E->RequestTag());
			app.event.pERR			= E->ErrorInfo();
			InvokeEventHandler();
		}
		break;

	}
	if( E->RequestFunction() == GlobalTaskFunction() )
		OnGlobalTaskEnded( E->ErrorInfo() );
}



// --------------------------------------
// GLOBAL TASK CONTROL
// Global tasks are special requests which are handled
//  internally by the engine.
// Only one global task can be active at a given time
// A Global Task generates GLOBAL_TASK events when started
//  and finished... the actual [raw] data is NOT passed to
//  the application.
// A Global Task may generate other events while executing
//  (i.e. While refreshing the user list, it will generate
//        USER_ADD events for each user received)

bool
 TSonorkClient::MayStartGlobalTask(TSonorkError&ERR) const
{
	if(Status()<SONORK_NETIO_STATUS_CONNECTED)
		_ERRNotConnected(ERR);
	if(Busy() )
		_ERREngineBusy(ERR);
	else
		return true;
	return false;

}

void
 TSonorkClient::StartGlobalTask(TSonorkError&ERR,SONORK_FUNCTION function)
{
	assert( function == SONORK_FUNCTION_USER_LIST
		||  function == SONORK_FUNCTION_WAPP_LIST);
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );
	if(MayStartGlobalTask(ERR))
	{
		UINT P_size;
		TSonorkDataPacket *P;
		P=SONORK_AllocDataPacket(0);
		if(function == SONORK_FUNCTION_USER_LIST)
			P_size=P->E_UserList_R();
		else
			P_size=P->E_WappList_R();
		SelfRequest(ERR,P,P_size);
		if( ERR.Result() == SONORK_RESULT_OK )
			OnGlobalTaskStarted( function , NULL);
	}
	control.mutex.Unlock();
}

void
 TSonorkClient::OnGlobalTaskStarted( SONORK_FUNCTION function, SONORK_DWORD2*tag)
{
	assert( function != SONORK_FUNCTION_NONE );
	assert( control.global_task.function == SONORK_FUNCTION_NONE );


	app.event.data.task.function	=control.global_task.function = function;
	if( tag != NULL )
		control.global_task.tag.Set( *tag );
	else
		control.global_task.tag.Clear();

	app.event.type	 	=SONORK_CLIENT_EVENT_GLOBAL_TASK;
	app.event.pERR		=NULL;	// pERR==NULL: Task is starting

	InvokeEventHandler();
}
void
 TSonorkClient::OnGlobalTaskEnded(TSonorkError*pERR)
{
	if( control.global_task.function == SONORK_FUNCTION_NONE )
		return;

	assert( pERR != NULL );

	control.global_task.tag.Clear();

	app.event.data.task.function	=control.global_task.function;
	control.global_task.function 	=SONORK_FUNCTION_NONE;

	app.event.type	 	=SONORK_CLIENT_EVENT_GLOBAL_TASK;
	app.event.pERR		=pERR; // pERR!=NULL: Task is ending

	InvokeEventHandler();
}



void
 _ERRNotConnected(TSonorkError& ERR)
{
	ERR.SetSys(SONORK_RESULT_INVALID_OPERATION,GSS_NOTCXTED,SONORK_MODULE_LINE);
}
void
 _ERREngineBusy(TSonorkError& ERR)
{
	ERR.SetSys(SONORK_RESULT_INVALID_OPERATION, GSS_ENGBUSY,SONORK_MODULE_LINE);
}

UINT
 TSonorkClient::MaxPendingRequests()
{
	return SONORK_CLIENT_MAX_PENDING_TASKS+SONORK_CLIENT_MAX_ACTIVE_TASKS;
}


void
 TSonorkClient::ProcessMsg(TSonorkMsg*msg)
{
	if( msg->SysFlags() & SONORK_MSG_SF_SERVER_ORIGINATED )
	{
		if(ProcessSysMsg(msg))
			return;
	}
	app.event.type	 		=SONORK_CLIENT_EVENT_MSG;
	app.event.pERR			=NULL;
	app.event.data.msg.ptr		=msg;
	InvokeEventHandler();
}

bool
 TSonorkClient::ProcessSysMsg(TSonorkMsg*msg)
{

	if( 	msg->DataServiceId() 	!= SONORK_SERVICE_ID_SONORK
	||	msg->DataServiceType()	!= SONORK_SERVICE_TYPE_AUTHS)
		return false;

	switch( msg->ExtDataType() )
	{

//  SONORK_MSG_DATA_AUTH_ADV
//   Authorization Advice: The server is telling us that
//    a request (AUTH_REQ) to modify out user list has been issued.
//   This system message is sent to both the source and the target.
//   The source GuId of the message contains the GuId of the
//    user that requested it, so if the source GuId matches
//    ours, it means that we've requested to add another user,
//    if not, it means that another user requested to add us to their
//    user list, in either case the final result is the same:
//     both users are added to each others user's list.
//   The authorization request can be accepted only by the target by
//    replying at any time with a Aknowledge (AUTH_AKN) and can be
//    denied or deleted by both the target and the sender by sending
//    and AUTH_DEL message.
		case SONORK_AUTHS_ATOM_ADV:

			//SONORK_Trace("TSonorkClient::ProcessSysMsg(AUTH_ADV)");
			{
				TSonorkAuthsAdv 	auth_adv;
				TSonorkAuthReqData	auth_req;
				if(auth_adv.CODEC_Read(msg->ExtDataPtr()) != SONORK_RESULT_OK)
				{
					// This should never happen unless the msg was
					// an invalid packet that could not be decoded.
					break;
				}
				if(auth_adv.user_data.GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1)
				{
					// This should never happen unless the msg was
					// an invalid packet that could not be decoded.
					break;
				}

				app.event.pERR	=NULL;
				app.event.user_id.Clear();
				app.event.user_data=NULL;
				app.event.data.auth_req = &auth_req;

				auth_req.user_data.Set(auth_adv.user_data);
				auth_req.notes.Set(msg->Text().String());

				auth_req.header.requestor_id.Set( msg->UserId() );
				auth_req.header.time.Set( msg->Time() );
				auth_req.header.auth.Set( auth_adv.auth );


				app.event.type		=SONORK_CLIENT_EVENT_USER_AUTH;
				InvokeEventHandler();
			}
			break;

//  SONORK_MSG_DATA_AUTH_ADD
//   Authorization add: The server is telling us that
//   a user has been added to our user list. This message is sent to
//   both users (the one that requested with AUTH_REQ and the one
//   that accepted with AUTH_AKN).
//   The source GuId of the message contains the GuId of the
//    user that has been added.
//   This message implies that any pending authorization request
//    has also been deleted.
		case SONORK_AUTHS_ATOM_ADD:
			//SONORK_Trace("TSonorkClient::ProcessSysMsg(AUTH_ADD)");
			{
				TSonorkAuthsAdd 	auth_add;
				if( auth_add.CODEC_Read(msg->ExtDataPtr()) != SONORK_RESULT_OK )
				{
					// This should never happen unless the msg was
					// an invalid packet that could not be decoded.
					break;
				}
				if(auth_add.user_data.GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1)
				{
					// This should never happen unless the msg was
					// an invalid packet that could not be decoded.
					break;
				}
				else
				{
					app.event.user_id.Set( msg->UserId() );
					app.event.user_data=&auth_add.user_data;
					app.event.pERR	=NULL;
					app.event.data.user_set.flags=msg->SysFlags();
					app.event.data.user_set.auth =&auth_add.auth;
					app.event.data.user_set.notes=&msg->Text().String();
					app.event.type		=SONORK_CLIENT_EVENT_USER_ADD;
					InvokeEventHandler();

				}
			}
			break;

//  SONORK_MSG_DATA_AUTH_DEL
//   Authorization del: The server is telling us that
//   a user has been deleted from our user list.
//   The source GuId of the message contains the GuId of the
//    user that has been deleted.
//   This message implies that any pending authorization request
//    has also been deleted.

		case SONORK_AUTHS_ATOM_DEL:
			{
				//SONORK_Trace("TSonorkClient::ProcessSysMsg(AUTH_DEL)");
				app.event.user_id.Set( msg->UserId() );
				app.event.user_data	=NULL;
				app.event.pERR		=NULL;
				app.event.type		=SONORK_CLIENT_EVENT_USER_DEL;
				InvokeEventHandler();
			}
			break;


//	SONORK_MSG_DATA_AUTH_REQ - Authorization Request
//	SONORK_MSG_DATA_AUTH_AKN - Authorization Aknowledge
//	These are messages WE send to the server and are
//  put here just to comment on them, the server
//  will never send us these messages.
//  AUTH_REQ we send to the server to ask an addition of another user.
//	AUTH_AKN we send to the server to to accept an AUTH_ADV (Authorization advice).

		case SONORK_AUTHS_ATOM_AKN:
		case SONORK_AUTHS_ATOM_REQ:
		default:
			break;
	}
	return true;

}

/*
SONORK_RESULT
	TSonorkClient::PutCtrlMsg(
		TSonorkError&		ERR
		,const TSonorkUserLocus1*	locus
		,const TSonorkCtrlMsg*	msg
		,const BYTE*	data
		,UINT			data_size)
{

	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );

	if(Status()<SONORK_NETIO_STATUS_CONNECTED)
		_ERRNotConnected(ERR);
	else
	{
		TSonorkDataPacket	*P;
		UINT P_size, A_size;
		A_size=sizeof(TSonorkUserLocus1) + sizeof(TSonorkCtrlMsg) + data_size + 32;
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_CtrlMsg_R(A_size,locus, msg, data,data_size);
		SelfRequest(ERR,P,P_size);
	}
	control.mutex.Unlock();
	return ERR.Result();
}

SONORK_RESULT TSonorkClient::PutMsg(TSonorkError&ERR,const TSonorkUserLocus1*locus,TSonorkMsg* msg)
{
	control.mutex.Lock( __FILE__ , SONORK_MODULE_LINE );

	if(Status()<SONORK_NETIO_STATUS_CONNECTED)
		_ERRNotConnected(ERR);
	else
	{
		TSonorkDataPacket	*P;
		UINT P_size, A_size;

		A_size=::CODEC_Size(locus) + ::CODEC_Size(msg) + 32;

		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_PutMsg_R(A_size , locus , msg);
		SelfRequest(ERR,P,P_size);
	}
	control.mutex.Unlock();
	return ERR.Result();
}
*/
