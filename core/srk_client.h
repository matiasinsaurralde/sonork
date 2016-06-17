#if !defined(SRK_CLIENT_H)
#define SRK_CLIENT_H
#include "srk_udpio.h"
#include "srk_tcpio.h"
#include "srk_client_defs.h"
#include "srk_crypt_context.h"

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


#if defined(SONORK_IPC_BUILD)
#	error MODULE IS NOT FOR IPC MODULES
#endif

class 	TSonorkEventHandler;
struct	TSonorkSvrLoginReqPacket;
StartSonorkQueueClass( TSonorkClientRequestQueue , TSonorkClientRequest )

   TSonorkClientRequest* Peek(DWORD);
   TSonorkClientRequest* Remove(DWORD);

EndSonorkQueueClass;

class TSonorkClientRequestTable
{
	UINT 			table_size
    			,	allocated_entries;

	TSonorkClientRequest		**table_ptr;
public:
	TSonorkClientRequestTable();
	~TSonorkClientRequestTable();

	UINT
		CurEntries()
		const { return allocated_entries;}

	UINT
		MaxEntries()
		const { return table_size;}

	bool
		AddEntry(TSonorkClientRequest	*);

	TSonorkClientRequest*
		GetEntry(DWORD) const;

	bool
		DelEntry(DWORD);
	void
		InitEnum(UINT*i)
		const { *i=0;}

	TSonorkClientRequest*
		EnumNext(UINT*i) const;
};

class TSonorkClient
{
	union TSonorkClientSendBuffer
	{
		TSonorkUdpPacketHeader		header;
		TSonorkUdpPacketEntry		entry;
		BYTE 			 	padding[SONORK_UDP_PACKET_FULL_ENTRY_SIZE+8];
	};

	struct TSonorkClientUdpControl
	{
		TSonorkUdpEngine		net_io;
		SONORK_UDP_PACKET_QID		last_i_packet_uid;
		TSonorkUdpPacket		*i_packet;
		TSonorkUdpPacket 		*o_packet;
		TSonorkUdpAknQueue    		a_queue;
		TSonorkClock			akn_time_clock;
		TSonorkClock			pck_time_clock;
		TSonorkClientSendBuffer 	send_buffer;
	};

	struct TSonorkClientTcpControl
	{
		TSonorkPacketTcpEngine		net_io;
		TSonorkTcpPacket*     		i_packet;
	};


	struct TSonorkClientControl
	{
		DWORD				flags;
		SONORK_NETIO_STATUS		status;
		TSonorkLock			mutex;
		TSonorkHostInfo			socks_info;

		TSonorkId			userId;
		TSonorkOldUserAddr		userAddr;
		TSonorkPin			sidPin;
		
		TSonorkClientUdpControl*	udp;
		TSonorkClientTcpControl*  	tcp;
		UINT				select_wait_msecs;
		UINT				akn_timeout_msecs;
		UINT				pck_timeout_msecs;
		UINT				max_idle_secs;
		TSonorkClientRequestTable	requests_table;
		TSonorkClientRequestQueue	requests_queue;
		DWORD				cur_task_id;


		UINT				handler_depth;
		TSonorkClientRequest*		active_request;
		TSonorkClock			idle_clock;
		TSonorkClock			monitor_clock;
		TSonorkClock			timeslot_clock;

		struct _UNIQUE{
			SONORK_FUNCTION		function;
			SONORK_DWORD2		tag;
		}global_task;

	}control;


	struct _APP{
		TSonorkEventHandler		*handler;
		void				*tag;
		TSonorkClientEvent		event;
	}app;


	struct _SERVER
	{
		DWORD				version;
		SONORK_NETIO_PROTOCOL		protocol;
		TSonorkPhysAddr			phys_addr;
		TSonorkCryptContext		crypt_context;
	}server;

// ----------------------------------

	SONORK_RESULT
		CreateNetIO(TSonorkError&);

	void
		DestroyNetIO();

// ----------------------------------

	void
		RequestEnd(TSonorkClientRequest*,TSonorkError&);

	void
		ProcessInDataPacket();

	void
		ConsumeEventPacket(TSonorkDataPacket*,UINT);

	void
		ConsumeRequestPacket(TSonorkDataPacket*,UINT);

	void
		ProcessMsg(TSonorkMsg*);

	bool
		ProcessSysMsg(TSonorkMsg*);


// ----------------------------------

	DWORD
		SelfRequest(TSonorkError&
				,TSonorkDataPacket*P
				,UINT P_size
				,const SONORK_DWORD2*tag=NULL);

	static void SONORK_CALLBACK
		SelfHandler(void *param,struct TSonorkClientRequestEvent*);

	void
		SelfHandleDataPacket(TSonorkClientRequestEvent*);

	void
		SelfHandleGroups(TSonorkDataPacket*,UINT);

	void
		SelfHandleRequestEnd(TSonorkClientRequestEvent*);

// ----------------------------------

	void
		DoMonitor();

	void
		PingServer(UINT);

	bool
		ProcessLoginAkn(BYTE*,DWORD); // true if connected

	UINT
		LoadConnectRequest(BYTE*,DWORD);

// ----------------------------------

	void
		InvokeRequestHandler(TSonorkClientRequest*,TSonorkClientRequestEvent*);
		
	void
		InvokeEventHandler();

// ----------------------------------
// UDP-specific

	SONORK_RESULT
		UDP_Connect(TSonorkError&);

	TSonorkClientUdpControl*
		UDP_Control()
		{ return control.udp;}

	TSonorkUdpEngine&
		UDP_NetIO()
		{ return control.udp->net_io;	}

	SONORK_UDP_PACKET_QID
		UDP_LastInPacketUID()	const
		{ return control.udp->last_i_packet_uid;}

	TSonorkUdpPacket*
		UDP_InPacket()
		{ return control.udp->i_packet;}

	TSonorkUdpPacket*
		UDP_OutPacket()
		{ return control.udp->o_packet;}

	void
		UDP_SendConnectRequest();

	void
		UDP_SendDisconnectRequest();

	void
		UDP_OutPacketTxComplete();

	void
		UDP_OutPacketTxTimeout();

	void
		UDP_ResetAknTime()
		{ control.udp->akn_time_clock.Set(control.timeslot_clock);}

	void
		UDP_ResetPckTime()
		{ control.udp->pck_time_clock.Set(control.timeslot_clock); }

	TSonorkUdpAknQueue&
		UDP_OutAknQueue()
		{ return control.udp->a_queue;}

	UINT
		UDP_PendingOutAkns()	const
		{ return control.udp->a_queue.Items();}

	TSonorkClientSendBuffer&
		UDP_SendBuffer()
		{ return control.udp->send_buffer;}

	void
		UDP_ProcessAkn(SONORK_UDP_PACKET_QID*,DWORD count);

	void
		UDP_SendAkn();

	bool
		UDP_SendEntry(DWORD entry_no, bool resend);

	SONORK_UDP_AKN_RESULT
		UDP_ProcessPacketEntry(TSonorkUdpPacketEntry*,DWORD recv_size);

	bool
		UDP_Send();

	void
		UDP_Recv(BYTE*data,DWORD size);

	void
		UDP_ConnectingTimeSlot();


	TSonorkClock&
		UDP_AknTimeClock() const
		{ return control.udp->akn_time_clock;}

	TSonorkClock&
		UDP_PckTimeClock() const
		{ return control.udp->pck_time_clock;}

	UINT
		UDP_AknTimeMsecs() const
		{ return control.timeslot_clock.IntervalMsecsAfter(control.udp->akn_time_clock);	}

	UINT
		UDP_PckTimeMsecs() const
		{ return control.timeslot_clock.IntervalMsecsAfter(control.udp->pck_time_clock);	}

// ----------------------------------
// TCP-specific

	TSonorkClientTcpControl*
		TCP_Control()
		{ return control.tcp;}

	TSonorkTcpPacket*
		TCP_InPacket()
		{ return TCP_Control()->i_packet;}

	TSonorkPacketTcpEngine&
		TCP_NetIO()
		{ return TCP_Control()->net_io;	}

	SONORK_RESULT
		TCP_Connect(TSonorkError&);

	void
		TCP_SendConnectRequest();

	void
		TCP_ProcessInPacket();

	bool
		TCP_Send();

	void
		TCP_ConnectionLost();


// ----------------------------------
	bool
		MayStartGlobalTask(TSonorkError&) const;

	void
		StartGlobalTask(TSonorkError&,SONORK_FUNCTION);

	void
		OnGlobalTaskStarted( SONORK_FUNCTION , SONORK_DWORD2*);

	void
		OnGlobalTaskEnded( TSonorkError* ERR );

// ----------------------------------

	void
		SetStatus(SONORK_NETIO_STATUS,TSonorkError*,DWORD flags, SONORK_C_CSTR pCaller);


// ----------------------------------

public:
	TSonorkClient( TSonorkEventHandler*,void * tag );
	~TSonorkClient();

// ----------------------------------

// Information methods

	DWORD
		Flags()		const
		{ return control.flags;}

	SONORK_NETIO_STATUS
		Status()	const
		{ return control.status;}
	bool
		Running()	const
		{ return Status()>SONORK_NETIO_STATUS_DISCONNECTED;}

	bool	Ready()		const
		{ return Status()>=SONORK_NETIO_STATUS_CONNECTED;}

	SONORK_NETIO_PROTOCOL
		Protocol()	const
		{ return server.protocol;}

	TSonorkEventHandler*
		AppHandler() const
		{ return app.handler;}

	void*	AppTag()	const
		{ return app.tag;}

// ----------------------------------

	UINT
		AknTimeoutMsecs() const
		{ return control.akn_timeout_msecs; }

	UINT
		PckTimeoutMsecs() const
		{ return control.pck_timeout_msecs; }

	UINT
		ReqTimeoutMsecs() const
		{ return control.pck_timeout_msecs*3; }

// ----------------------------------

	TSonorkCryptContext&
		CryptContext()
		{ return server.crypt_context;}

// ----------------------------------

	UINT
		InHandler()	const
		{ return control.handler_depth > 0;}

	DWORD
		ClientVersionNumber()	const
		{ return SONORK_CLIENT_VERSION_NUMBER;	}

	// ServerVersion() is only valid when connected
	DWORD
		ServerVersionNumber() const
		{ return server.version; }


	UINT
		PendingRequests() const
		{
			return    control.requests_queue.Items()+control.requests_table.CurEntries();
		}

	UINT
		MaxPendingRequests();

// ----------------------------------
// Connection establishment

	SONORK_RESULT
		Connect(TSonorkError& ERR
			,TSonorkPhysAddr& phys_addr);

	void
		Disconnect();


// ----------------------------------
// main loop
	bool
		TimeSlot();


// ----------------------------------
// Requests

	SONORK_RESULT
		Request(
			 TSonorkError&			ERR
			,TSonorkDataPacket*		p
			,UINT 				p_size
			,lpfnSonorkRequestHandler	h_callback
			,void			  	*h_param
			,const SONORK_DWORD2		*h_tag=NULL
			,DWORD				*task_id=NULL
			);

	SONORK_CLIENT_REQUEST_STATUS
		RequestStatus(DWORD);

// On return p_function and p_tag containt the request's function and tag
// Both may be null if function and tag is not required.
	bool
		CancelRequest(DWORD, SONORK_FUNCTION*p_function,SONORK_DWORD2*p_tag);

// ----------------------------------
// Predefined requests
// Immediate tasks (no task start/end report)

	SONORK_RESULT
		GetSysMsgs(TSonorkError&,const TSonorkTime&, const TSonorkRegion&);


// ----------------------------------
// Global tasks
//  only one of these may be active at the same time
//  Busy() and GlobalTaskFunction() give status of these tasks
//  the GLOBAL_TASK is fired when the function starts and when it ends

	SONORK_FUNCTION
		GlobalTaskFunction() const
		{return control.global_task.function;}

	bool
		Busy()		const
		{ return control.global_task.function!=SONORK_FUNCTION_NONE;}

	void
		SyncSid(TSonorkError&
				,const TSonorkSidFlags&
				,const TSonorkPhysAddr&
				,const TSonorkRegion&
				,const TSonorkDynString&);

	void
		RefreshUser(	TSonorkError&
				,const TSonorkId&
				,const TSonorkAuth2&);

	void
		RegisterTrackers(
			 TSonorkError&		  ERR
			,TSonorkTrackerDataQueue& queue);
			
	void
		RefreshUserList(TSonorkError&);

	void
		RefreshWappList(TSonorkError&);

// ----------------------------------
// Utility

	void    EnableCallbacks(bool);


const	TSonorkHostInfo&
		SocksInfo()	 const
		{ return control.socks_info; }

	TSonorkHostInfo&
		wSocksInfo()
		{ return control.socks_info; }

	bool
		UsingSocks()	const;

	const TSonorkId&
		UserId() const
		{ return control.userId; }

	const TSonorkOldUserAddr&
		UserAddress() const
		{ return control.userAddr; }

	const TSonorkPin&
		SidPin() const
		{ return control.sidPin; }

	const TSonorkClock&
		TimeSlotClock()	const
		{ return control.timeslot_clock;}

	const TSonorkClock&
		MonitorClock()  const
		{ return control.monitor_clock;}

	const TSonorkClock&
		IdleClock()		const
		{ return control.idle_clock;}


};



#endif
