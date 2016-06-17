#if !defined(SONORK_UTS_H)
#define SONORK_UTS_H

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


#include "srk_tcpio.h"
#include "srk_uts_packet.h"
#include "srk_crypt_context.h"



#define SONORK_UTS_SERVER_F_NO_CALLBACK       	0x0001
#define SONORK_UTS_SERVER_F_FD_SET_DIRTY      	0x0002
#define SONORK_UTS_SERVER_F_QUEUE_DIRTY       	0x0004


#define SONORK_UTS_KERNEL_APP_FLAGS   			(SONORK_UTS_APP_F_SINGLE_USER_INSTANCE)



enum SONORK_UTS_EVENT
{
  SONORK_UTS_EVENT_STATUS
, SONORK_UTS_EVENT_ACCEPT
, SONORK_UTS_EVENT_SID_PIN
, SONORK_UTS_EVENT_CLEAR_TO_SEND
, SONORK_UTS_EVENT_ERROR
, SONORK_UTS_EVENT_LOGIN
, SONORK_UTS_EVENT_WARNING
, SONORK_UTS_EVENT_DATA
};

enum SONORK_UTS_WARNING
{
  SONORK_UTS_WARNING_NONE
, SONORK_UTS_WARNING_SOCKS_DENIED
, SONORK_UTS_WARNING_AUTH_TIMEOUT
, SONORK_UTS_WARNING_PROTOCOL
};


enum SONORK_UTS_LINK_TYPE{
  SONORK_UTS_LINK_TYPE_NONE		=0x00
, SONORK_UTS_LINK_TYPE_LISTEN		=0x01
, SONORK_UTS_LINK_TYPE_IN_CONNECT  	=0x02
, SONORK_UTS_LINK_TYPE_OUT_CONNECT  	=0x03
};

enum SONORK_UTS_AUTHORIZATION{
    SONORK_UTS_AUTHORIZATION_NONE
,   SONORK_UTS_AUTHORIZATION_ACCEPT
,   SONORK_UTS_AUTHORIZATION_DENY
,   SONORK_UTS_AUTHORIZATION_PENDING
};


#define SONORK_UTS_APP_F_SINGLE_GLOBAL_INSTANCE   0x00004
#define SONORK_UTS_APP_F_SINGLE_USER_INSTANCE     0x00008



#define SONORK_UTS_LINK_F_NO_ENCRYPTION		0x00000100	// Both Startup() and Connect()
#define SONORK_UTS_LINK_F_NO_LISTEN		0x00000200	// Only for Startup()
#define SONORK_UTS_LINK_F_NO_SOCKS		0x00000400	// Only for Connect()
#define SONORK_UTS_LINK_F_MUST_AKN		0x00000800
#define SONORK_UTS_LINK_F_WRITE_ENABLED       	0x00001000
#define SONORK_UTS_LINK_F_IN_WRITE_QUEUE	0x00002000
#define SONORK_UTS_LINK_F_CLEAR_TO_SEND        	0x00004000
#define SONORK_UTS_LINK_F_WARNED  	        0x00010000
#define SONORK_UTS_LINK_F_BLOCKING  	       	0x00020000
#define SONORK_UTS_LINK_F_CONDEMNED	        0x00040000
#define SONORK_UTS_LINK_F_SOCKS_ENABLED        	0x00100000
#define SONORK_UTS_LINK_F_SOCKS_REQ_SENT        0x00200000
#define SONORK_UTS_LINK_FM_TYPE		        0x0000000f

// Flags that define behaviour of link
#define SONORK_UTS_LINK_FM_BEHAVIOUR_FULL		(SONORK_UTS_LINK_F_MUST_AKN|SONORK_UTS_LINK_F_NO_ENCRYPTION)

// Flags that define behaviour of link and are commong to the other end
#define SONORK_UTS_LINK_FM_BEHAVIOUR_NET		(SONORK_UTS_LINK_F_MUST_AKN)

// Flags allowed for Connect()
#define SONORK_UTS_LINK_FM_CONNECT				(SONORK_UTS_LINK_F_NO_SOCKS|SONORK_UTS_LINK_FM_BEHAVIOUR_FULL)

// Flags allowed for Startup()
#define SONORK_UTS_LINK_FM_STARTUP				(SONORK_UTS_LINK_F_NO_LISTEN|SONORK_UTS_LINK_FM_BEHAVIOUR_FULL)

#define SONORK_UTS_LINK_ID_FM_INDEX				0x00fff
#define SONORK_UTS_LINK_ID_FM_COOKIE				(~SONORK_UTS_LINK_ID_FM_INDEX)
#define SONORK_UTS_LINK_ID_FS_COOKIE				12


// Don't use this definition, use Uts::Version() instead
#define _SONORK_UTS_VERSION_						1

struct TSonorkUTSLink
{
friend class TSonorkUTS;

protected:
	SONORK_UTS_LINK_ID	id;
	DWORD			flags;
	SONORK_NETIO_STATUS	status;
	TSonorkPhysAddr		phys_addr;

public:

	TSonorkUTSDescriptor   	descriptor;
	DWORD			data;


private:

	void
		SetStatus(SONORK_NETIO_STATUS s)
		{ status=s;  }

public:
	SONORK_UTS_LINK_ID
		Id()
		const	{ return id;}

	DWORD
		Flags()
		const { return flags;}

	SONORK_SERVICE_ID
		ServiceId()
		const { return descriptor.ServiceId(); }

	SONORK_NETIO_STATUS
		Status()
		const { return status;}

	SONORK_UTS_LINK_TYPE
		Type()
		const { return (SONORK_UTS_LINK_TYPE)(flags&SONORK_UTS_LINK_FM_TYPE);}

const	TSonorkPhysAddr&
		PhysAddr()
		const	{ return phys_addr;}

	BOOL 	Incomming()
		const	{ return Type()==SONORK_UTS_LINK_TYPE_IN_CONNECT;}

	BOOL	Outgoing()
		const { return Type()==SONORK_UTS_LINK_TYPE_OUT_CONNECT;}

	BOOL	IsInWriteQueue()
		const { return flags&SONORK_UTS_LINK_F_IN_WRITE_QUEUE;}

	BOOL	IsWriteEnabled()
		const { return flags&SONORK_UTS_LINK_F_WRITE_ENABLED;}

	BOOL    IsClearToSend()
		const { return flags&SONORK_UTS_LINK_F_CLEAR_TO_SEND;}

	BOOL 	IsCondemned()
		const { return flags&SONORK_UTS_LINK_F_CONDEMNED;}

	BOOL	IsBlocking()
		const { return flags&SONORK_UTS_LINK_F_BLOCKING;}

	BOOL	IsUsingSocks()
		const { return flags&SONORK_UTS_LINK_F_SOCKS_ENABLED;}

	DWORD
		GetData()
		const { return data; }

	void
		SetData(DWORD v)
		{ data=v;}
};


struct TSonorkUTSEvent
{
friend class TSonorkUTS;

private:
	SONORK_UTS_EVENT	event;
	TSonorkUTSLink*		link;

	union
	{

		struct{
			TSonorkUTSLink*		link;
		}acc;

		struct {
			SONORK_UTS_AUTHORIZATION	authorization;
			SONORK_DWORD4			pin;
			SONORK_PIN_TYPE			pin_type;
			DWORD				flags;
		}login;

		struct
		{
			TSonorkUTSPacket*	packet;
			UINT			packet_size;
			UINT			packet_cmd;
		}data;

		struct {
			SONORK_UTS_WARNING	warning;
		}warn;
	}D;

	TSonorkError* 	error_info;
	
	TSonorkUTSEvent(SONORK_UTS_EVENT t, TSonorkUTSLink*L,TSonorkError *err=NULL)
	{
		event=t;
		link=L;
		error_info=err;
	}

public:

// EventType(), Link() and derived
//	All Events
	SONORK_UTS_EVENT
		Event()	const
		{ return event;}

	TSonorkUTSLink	 *
		Link() const
		{ return link;}

	SONORK_UTS_LINK_ID
		LinkId() const
		{ return link->Id();}

	DWORD
		LinkFlags() const
		{ return link->Flags();}

	SONORK_NETIO_STATUS
		LinkStatus() const
		{ return link->Status();}

	SONORK_UTS_LINK_TYPE
		LinkType() const
		{ return link->Type();}

	DWORD
		LinkData() const
		{ return link->GetData();}

	SONORK_SERVICE_ID
		LinkServiceId()	const
		{ return link->ServiceId();}

const   TSonorkPhysAddr&
		LinkPhysAddr()	const
		{ return link->PhysAddr();}

const   TSonorkUTSDescriptor*
		LinkDescriptor() const
		{ return &link->descriptor;}

const	TSonorkUserLocus1&
		LinkUserLocus()	const
		{ return link->descriptor.locus;}

const   TSonorkId&
		LinkUserId()  const
		{ return link->descriptor.locus.userId;}
	BOOL
		IncommingLink() const
		{ return link->Incomming();}

	BOOL
		OutgoingLink()	const
		{ return link->Outgoing();}


// AcceptLinkId(),AcceptLinkStatus(),AcceptLinkFlags()
//	SONORK_UTS_EVENT_ACCEPT only
	TSonorkUTSLink*
		AcceptLink() const
		{ return D.acc.link;}

	SONORK_UTS_LINK_ID
		AcceptLinkId() const
		{ return AcceptLink()->Id();}

	SONORK_NETIO_STATUS
		AcceptLinkStatus() const
		{ return AcceptLink()->Status();}

	DWORD	AcceptLinkFlags() const
		{ return AcceptLink()->Flags();}


// LoginSidPin()
//	SONORK_UTS_EVENT_SID_PIN, SONORK_UTS_EVENT_LOGIN
	SONORK_DWORD4&
		LoginPin()
		{ return D.login.pin;}

// LoginPinType(),LoginFlags()
// SONORK_UTS_EVENT_LOGIN
	SONORK_PIN_TYPE
		LoginPinType() const
		{ return D.login.pin_type;}

	DWORD
		LoginFlags() const
		{ return D.login.flags;}

// SetLoginPinType
//	SONORK_UTS_EVENT_SID_PIN

	void
		SetLoginPinType(SONORK_PIN_TYPE ptype)
		{ D.login.pin_type=ptype;}
// Authorization data
//	SONORK_UTS_EVENT_LOGIN only
	SONORK_UTS_AUTHORIZATION
		GetLoginAuthorization()	const
		{ return D.login.authorization;}

	void
		SetLoginAuthorizationDenied(TSonorkError&E)
		{D.login.authorization=SONORK_UTS_AUTHORIZATION_DENY;ErrorInfo()->Set(E);}

	void
		SetLoginAuthorizationPending()
		{D.login.authorization=SONORK_UTS_AUTHORIZATION_PENDING;}

	void
		SetLoginAuthorizationAccepted()
		{D.login.authorization=SONORK_UTS_AUTHORIZATION_ACCEPT;}

// Warning()
// 	SONORK_UTS_EVENT_WARNING

	SONORK_UTS_WARNING
		Warning() const
		{ return D.warn.warning;}

// ErrorInfo()
//	SONORK_UTS_EVENT_STATUS
//	SONORK_UTS_EVENT_ERROR
//	SONORK_UTS_EVENT_ACCEPT
//	SONORK_UTS_EVENT_LOGIN
// 	SONORK_UTS_EVENT_WARNING
	TSonorkError*
		ErrorInfo()	const
		{ return error_info;}

//	DataCmd(), DataPtr(), DataSize(), DataVersion()
//	SONORK_UTS_EVENT_DATA
	TSonorkUTSPacket*
		DataPacket() const
		{ return D.data.packet; }

	UINT
		DataPacketCmd()	 const
		{ return D.data.packet_cmd;}

	UINT
		DataPacketVersion() const
		{ return D.data.packet->Version();}

	UINT
		DataPacketSize() const
		{ return D.data.packet_size; }

const	BYTE*
		DataPtr() const
		{ return D.data.packet->DataPtr();}

	UINT	DataSize() const
		{ return D.data.packet->DataSize(D.data.packet_size);}

};



typedef void (SONORK_CALLBACK fnSonorkUTSCallback)(void *param, const SONORK_DWORD2& packet_tag, const TSonorkUTSLink*, const TSonorkError*);
typedef 	fnSonorkUTSCallback* lpfnSonorkUTSCallback;

struct TSonorkUTSOEntry
{
public:
	void			*param;
	lpfnSonorkUTSCallback   callback;
	SONORK_DWORD2		tag;
	TSonorkTcpPacket	*packet;

	TSonorkUTSOEntry(TSonorkTcpPacket*,void*param,lpfnSonorkUTSCallback callback,const SONORK_DWORD2*);
	~TSonorkUTSOEntry();
	void	FreeTcpPacket();

};

StartSonorkQueueClass ( TSonorkUTSOEntryQueue , TSonorkUTSOEntry );
	NoExtensions
EndSonorkQueueClass;

struct TSonorkUTSLinkEx
:public TSonorkUTSLink
{
friend class TSonorkUTS;

private:

	struct _NET
	{
		SOCKET				sk;
		UINT				o_offset;
		TSonorkTcpPacketHeader 		o_header;
		TSonorkUTSOEntry	 *	o_entry;
		TSonorkUTSOEntryQueue	 	o_queue;
		TSonorkUTSOEntryQueue	 	akn_queue;
		UINT				pending_akns;
		UINT				i_offset;
		TSonorkTcpPacketHeader 		i_header;
		TSonorkTcpPacket	 *	i_packet;
		TSonorkClock			last_request_clock;
		TSonorkCryptContext		crypt_context;
	};

	_NET				net;

	SOCKET
		Socket() const
		{ return net.sk;}


	TSonorkUTSLinkEx( SONORK_UTS_LINK_ID p_id , DWORD flags );
	~TSonorkUTSLinkEx();

public:

	TSonorkCryptContext&
		CryptContext()
		{ return net.crypt_context;}

	BOOL
		IsDataPending() const
		{ return net.o_entry!=NULL || net.o_queue.Items() != 0; }

};

StartSonorkQueueClass ( UtsLinkQueue , TSonorkUTSLinkEx );
	NoExtensions
EndSonorkQueueClass;


typedef void  (SONORK_CALLBACK fnUtsEventHandler)(void*handler_param,TSonorkUTSEvent*);
typedef		fnUtsEventHandler*	lpfnUtsEventHandler;



class TSonorkUTS
{
	struct {
		struct 	TSonorkUTSLinkEx	**ptr;
		UINT  	max_entries,entries;
		DWORD 	cookie;
	}link_table;

	TSonorkLock	mutex;
	UtsLinkQueue	del_queue;
	UtsLinkQueue	write_queue;
	UtsLinkQueue	cts_queue;
	char		buffer[SONORK_TCPIO_DEFAULT_BUFFER_SIZE];
	struct {
		TSonorkUTSLinkEx	*link;
		lpfnUtsEventHandler 	event_handler;
		void            	*handler_param;
	}main;
	fd_set*			active_w_fd_set;
	fd_set*			active_r_fd_set;
	fd_set*			active_e_fd_set;
	fd_set*			work_w_fd_set;
	fd_set*			work_r_fd_set;
	fd_set*			work_e_fd_set;

#if defined(SONORK_LINUX_BUILD)
	SOCKET			max_sk_in_active_set;
#endif

	UINT			sockets_in_active_set;
	UINT			monitor_interval_secs;
	UINT			max_link_idle_secs;
	UINT            	uts_flags;
	TSonorkClock		timeslot_clock;
	TSonorkClock		monitor_clock;
	TSonorkHostInfo		socks_info;

	TSonorkUTSLinkEx*	AllocLink(DWORD link_flags);
	BOOL			DestroyLink(SONORK_UTS_LINK_ID);
	BOOL			SetBlocking(TSonorkUTSLinkEx*L,bool);


	void			SetReadyToSend(TSonorkUTSLinkEx*L);
	void			SetWriteEnabled(TSonorkUTSLinkEx*L);
	void			SetCondemned(TSonorkUTSLinkEx*L );
	void			AddToWriteQueue(TSonorkUTSLinkEx*L);

	void			EndOutputEntryTx(TSonorkUTSLinkEx*, TSonorkUTSOEntry*, TSonorkError*);
	void			DoLinkError(TSonorkUTSLinkEx*R
					, SONORK_RESULT
					, SONORK_SYS_STRING
					, int code
					, bool kill_socket
					, SONORK_UTS_WARNING);
	int			TS_Select(DWORD msecs);
	void			TS_RebuildFdSet();
	void			TS_Accept(TSonorkUTSLinkEx*R);
	void			TS_Read(TSonorkUTSLinkEx*L,UINT size);
	void			TS_ClearQueues();
	void			TS_Write(TSonorkUTSLinkEx*L);
	BOOL			TS_GetNextOutputEntry(TSonorkUTSLinkEx*L);
	void   			TS_Monitor();
	SONORK_RESULT		VerifyLinkInfo(SONORK_UTS_LINK_ID id,const TSonorkUTSDescriptor*,SONORK_NETIO_STATUS,TSonorkError&);

	void			ProcessInPacket(TSonorkUTSLinkEx*R);
	void			ProcessDataPacket(TSonorkUTSLinkEx*,TSonorkUTSPacket*,UINT size);
	void			ProcessDataAkn(TSonorkUTSLinkEx*,DWORD);
	void			ProcessLoginReq(TSonorkUTSLinkEx*R,TSonorkUTSPacket*,UINT size);
	void			ProcessLoginAkn(TSonorkUTSLinkEx*R,TSonorkUTSPacket*,UINT size);

	bool			SendLoginReq(TSonorkUTSLinkEx*R);
	bool			_SendLoginReq(TSonorkUTSLinkEx*R);
	void			EncryptLoginReq(TSonorkUTSLinkEx*R,struct TSonorkUTSLoginReq*REQ);
	SONORK_RESULT		UncryptLoginReq(TSonorkUTSLinkEx*R,struct TSonorkUTSLoginReq*REQ, TSonorkError&ERR);

	bool			SendLoginAkn(TSonorkUTSLinkEx*R,TSonorkError&);
	void			EncryptLoginAkn(TSonorkUTSLinkEx*R,struct TSonorkUTSLoginAkn*AKN);
	SONORK_RESULT		UncryptLoginAkn(TSonorkUTSLinkEx*R,struct TSonorkUTSLoginAkn*AKN, TSonorkError&ERR);

	void			TS_SendSocksRequest(TSonorkUTSLinkEx*);
	void			TS_RecvSocksRequest(TSonorkUTSLinkEx*,int);

	SONORK_RESULT
		SendUtsPacket(	TSonorkError&		ERR
				, TSonorkUTSLinkEx*	LINK
				, TSonorkUTSPacket*	P
				, UINT 			P_size
				, void *		param
				, lpfnSonorkUTSCallback callback
				, const SONORK_DWORD2*	tag);

	SONORK_RESULT
		SendTcpPacket(TSonorkUTSLinkEx*R
				,TSonorkTcpPacket*	tcp_packet
				, void *		param
				, lpfnSonorkUTSCallback callback
				, const SONORK_DWORD2*	tag
				, bool			high_priority);

	TSonorkUTSLinkEx**	GetLinkPtr(SONORK_UTS_LINK_ID);
	TSonorkUTSLinkEx	*	GetActiveLink(SONORK_UTS_LINK_ID, bool must_be_connected);
	// GetActiveLinkErr()
	// If Link is not valid or the Link is not in CONNECTED status,
	//  ERR will be  set, if Link IS valid, ERR is *NOT* modified.
	TSonorkUTSLinkEx	*	GetActiveLinkErr(SONORK_UTS_LINK_ID,TSonorkError&ERR);

	void	InvokeEventHandler(TSonorkUTSEvent*);

	void   	SetUtsFlag(UINT f)
		{ uts_flags|=f;}

	BOOL
		TestUtsFlag(UINT f) const
		{ return uts_flags&f;}

	SOCKET
		CreateSocket(UINT*error_code,bool blocking);

	void
		SetStatus(TSonorkUTSLinkEx*R,SONORK_NETIO_STATUS status,TSonorkError&);

	void
		SetClearToSend(TSonorkUTSLinkEx*R);

	int
		DoListen(TSonorkUTSLinkEx*,DWORD lo_port,DWORD hi_port);

	TSonorkUTSLinkEx*
		Connect(TSonorkError&,const TSonorkPhysAddr&, UINT flags);


	void	SetFdSetDirty()
		{
			uts_flags|=SONORK_UTS_SERVER_F_FD_SET_DIRTY;
		}

	void
		SetQueueDirty()
		{
			uts_flags|=SONORK_UTS_SERVER_F_QUEUE_DIRTY;
		}

	void
		EncodeSidPin(SONORK_DWORD4&pin, DWORD& pin_type,TSonorkUTSLinkEx*);

	BYTE   *
		RecvBuffer()
		{return (BYTE*)buffer;}

public:
	TSonorkUTS(DWORD max_links);
	~TSonorkUTS();

	bool
		TimeSlot(UINT msecs);




	SONORK_UTS_LINK_ID
		Startup(TSonorkError&		ERR
			,const TSonorkUTSDescriptor&	descriptor
			,lpfnUtsEventHandler		h_callback
			,void* 				h_param
			,UINT				flags
			,DWORD				address
			,DWORD				lo_port
			,DWORD				hi_port);

	SONORK_UTS_LINK_ID
		Listen(WORD port,sockaddr*addr,DWORD *error=NULL);

	SONORK_UTS_LINK_ID
		ConnectToUts(TSonorkError&ERR
			, const TSonorkUTSDescriptor*	descriptor
			, const TSonorkPhysAddr& 	phys_addr
			, UINT  link_flags);

	bool
		Authorize(SONORK_UTS_LINK_ID id,TSonorkError&);

	void
		Disconnect(SONORK_UTS_LINK_ID id, bool kill_socket);

	void
		Disconnect(const TSonorkId&, bool kill_socket);

	// Set size to -1 to leave buffer size as it is
	int					SetSocketBufferSize(SONORK_UTS_LINK_ID id,long send_size,long recv_size);

	SONORK_RESULT
		SendPacket(TSonorkError& ERR
			, SONORK_UTS_LINK_ID 	id
			, TSonorkUTSPacket*	P
			, UINT 			P_size
			, void *		param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag);

	SONORK_RESULT
		SendPacket(TSonorkError& ERR
			, SONORK_UTS_LINK_ID 	id
			, DWORD 		cmd
			, BYTE 			version
			, const BYTE*		data
			, UINT 			data_size
			, void *		param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag);

	SONORK_RESULT
		SendPacket(TSonorkError&	ERR
			, SONORK_UTS_LINK_ID 	id
			, DWORD 		cmd
			, BYTE 			version
			, const TSonorkCodecAtom*
			, void *		 param
			, lpfnSonorkUTSCallback callback
			, const SONORK_DWORD2*	tag);

/*
	SONORK_RESULT
		SendCtrlMsg(TSonorkError&,SONORK_UTS_LINK_ID id,DWORD cmd, BYTE version, const TSonorkCtrlMsg*,const BYTE*data,UINT data_size);
*/		

	BOOL
		CancelCallback(SONORK_UTS_LINK_ID id,void*param, SONORK_DWORD2*tag);

	void
		Shutdown();
		
	void
		EnableCallbacks(bool v);

	TSonorkUTSLinkEx*
		MainLink()
		{ return main.link;}

	SONORK_UTS_LINK_ID
		MainLinkId()
		{ return main.link?main.link->Id():SONORK_INVALID_LINK_ID;}

	SONORK_NETIO_STATUS
		MainLinkStatus()
		{ return main.link?main.link->Status():SONORK_NETIO_STATUS_DISCONNECTED;}

	TSonorkUTSDescriptor*
		MainLinkDescriptor()
		{ return main.link?&main.link->descriptor:NULL;}


	bool
		GetFlags(SONORK_UTS_LINK_ID id,DWORD*flags);
		
	bool
		GetLinkInfo(SONORK_UTS_LINK_ID id,SONORK_NETIO_STATUS*link_status,DWORD*link_flags=NULL,TSonorkUTSDescriptor*descriptor=NULL);

	bool
		GetPhysAddr(SONORK_UTS_LINK_ID id,TSonorkPhysAddr&);

	bool
		GetLinkData(SONORK_UTS_LINK_ID id,DWORD*data);

	bool
		SetLinkData(SONORK_UTS_LINK_ID id,DWORD data);
		
	bool
		ResetLinkTimeout(SONORK_UTS_LINK_ID id);

	void
		InitEnumLink(TSonorkListIterator& index){index.Reset();}
	TSonorkUTSLinkEx*
		EnumNextLink(TSonorkListIterator& index);

	SONORK_UTS_LINK_ID
		FindLink(const TSonorkId&
			,DWORD service_id_1
			,DWORD instance_1
			,DWORD service_id_2
			,DWORD instance_2
			);

	BYTE
		Version() const
		{ return _SONORK_UTS_VERSION_;	}

	TSonorkHostInfo&
		SocksInfo()
		{ return socks_info; }
	bool
		SocksEnabled();
	void
		SetSocksV4(TSonorkPhysAddr&);

	UINT
		GetMaxLinkIdleSecs() const
		{ return max_link_idle_secs; }

	void
		SetMaxLinkIdleSecs(UINT v)
		{ max_link_idle_secs=v; }
};



#endif
