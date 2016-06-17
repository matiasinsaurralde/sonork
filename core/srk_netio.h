#if !defined(SONORK_NETIO_H)
#define SONORK_NETIO_H


#include "srk_data_lists.h"

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




enum SONORK_NETIO_PROTOCOL
{
  SONORK_NETIO_PROTOCOL_NONE
, SONORK_NETIO_PROTOCOL_UDP
, SONORK_NETIO_PROTOCOL_TCP
};

enum SONORK_NETIO_STATUS
{
  SONORK_NETIO_STATUS_DISCONNECTED
, SONORK_NETIO_STATUS_DISCONNECTING
, SONORK_NETIO_STATUS_CONNECT_REQ
, SONORK_NETIO_STATUS_CONNECTING
, SONORK_NETIO_STATUS_AUTHORIZING
, SONORK_NETIO_STATUS_CONNECTED
, SONORK_NETIO_STATUS_LISTENING
, SONORK_NETIO_STATUS_ITEMS
};

#define SONORK_NETIO_F_CX		0x00200
#define SONORK_NETIO_F_LISTEN	    	0x00400
#define SONORK_NETIO_F_BIND		0x00800
#define SONORK_NETIO_F_WRITE		0x01000
#define SONORK_NETIO_F_BLOCKED		0x02000

typedef WORD 		SONORK_NETIO_PACKET_HFLAG;
typedef DWORD 		SONORK_NETIO_PACKET_SIZE;

#define SONORK_NETIO_HFLAG_TYPE_MASK		 	0x0f00
#define SONORK_NETIO_HFLAG_FLAG_MASK		 	0xf000
#define SONORK_NETIO_HFLAG_TYPE_NONE  		   	0x0000
#define SONORK_NETIO_HFLAG_TYPE_CONNECT			0x0100
#define SONORK_NETIO_HFLAG_TYPE_LOGIN			0x0200
#define SONORK_NETIO_HFLAG_TYPE_LOGOUT		   	0x0300
#define SONORK_NETIO_HFLAG_TYPE_PACKET		   	0x0400
#define SONORK_NETIO_HFLAG_TYPE_RAW_DATA	   	0x0500


#define SONORK_NETIO_HFLAG_PACKET_TYPE(h)		((h)&SONORK_NETIO_HFLAG_TYPE_MASK)
#define SONORK_NETIO_HFLAG_PACKET_FLAGS(h)		((h)&SONORK_NETIO_HFLAG_FLAG_MASK)
#define SONORK_NETIO_HFLAG_FLAG_NOT_CRYPTED	   	0x1000

#define SONORK_UDPIO_HFLAG_AKN_COUNT_MASK  		0x0007
#define SONORK_UDPIO_HFLAG_MAX_AKN_COUNT   		7
#define SONORK_UDPIO_HFLAG_AKN_COUNT(h)			((h)&SONORK_UDPIO_HFLAG_AKN_COUNT_MASK)

struct SONORK_NETIO_SOCKS_DATA
{
	TSonorkPhysAddr		phys_addr;
	DWORD			version;	// SOCKS V4 or V5
};



class TSonorkNetIO
{
#define SONORK_NET_EVENT_FLAG_READ		0x000001
#define SONORK_NET_EVENT_FLAG_WRITE		0x000002
#define SONORK_NET_EVENT_FLAG_EXCEPTION	0x000004
private:
	SONORK_NETIO_PROTOCOL 	protocol;
	SOCKET 			sk;
	struct {
		BYTE	*	ptr;
		UINT		size;
	}buffer;
	UINT			event_flags;
	SONORK_NETIO_STATUS	netio_status;
	fd_set*			active_r_fd_set;
	fd_set*			active_w_fd_set;
	fd_set*			active_e_fd_set;
	fd_set*			work_r_fd_set;
	fd_set*			work_w_fd_set;
	fd_set*			work_e_fd_set;

	void
		DestroySocket();

#if defined(SONORK_SERVER_BUILD)
public:
#endif
	SOCKET
		Socket() const
		{ return sk;}

protected:

	TSonorkPhysAddr	local_phys_addr;
	TSonorkPhysAddr remote_phys_addr;
	DWORD		netio_flags;
	UINT		recv_bytes;

	void
		EnableSelectRead(bool s);

	void
		EnableSelectWrite(bool s);

	void
		EnableSelectExcept(bool s);

	int
		CreateSocket(bool blocking=false, long send_sz=-1, long recv_sz=-1);


	TSonorkNetIO(SONORK_NETIO_PROTOCOL,UINT recv_buffer_size);

	virtual
		~TSonorkNetIO();
		
	void
		SetStatus(SONORK_NETIO_STATUS);

	int
		_Send(const void*data,UINT bytes,UINT&bytes_sent);

	UINT
		_Recv();

	int
		_Bind(DWORD host, WORD port);

	int
		_Listen(int backlog=5);

	int
		_Select(DWORD msecs);

	int
		_Accept(SOCKET *new_sk,sockaddr_in*sender_address, bool blocking);

	void
		ClearReadEvent()
		{event_flags&=~SONORK_NET_EVENT_FLAG_READ;}

	void
		ClearWriteEvent()
		{event_flags&=~SONORK_NET_EVENT_FLAG_WRITE;}

	void
		ClearExceptEvent()
		{event_flags&=~SONORK_NET_EVENT_FLAG_EXCEPTION;}

public:
	bool
		SocketCreated() const
		{ return Socket() != INVALID_SOCKET; } 

	bool
		SetBlocking(SOCKET sk,bool set);

	DWORD	Flags()	const
		{ return netio_flags;}

	SONORK_NETIO_STATUS
		Status() const
		{ return netio_status;}

	SONORK_NETIO_PROTOCOL
		Protocol() const
		{ return protocol;}


	BYTE*
		Buffer()
		{return buffer.ptr;}

	UINT
		BufferSize() const
		{ return buffer.size;}

	UINT	RecvBytes() const
		{ return recv_bytes;}

const	TSonorkPhysAddr&
		LocalPhysAddress() const
		{ return local_phys_addr;}

const	TSonorkPhysAddr&
		RemotePhysAddress() const
		{ return remote_phys_addr;}

	void
		SetRecvBufferSize(UINT sz);

	int
		SetSocketBufferSize(long send_size,long recv_size);

	BOOL
		ReadEvent() const
		{ return event_flags&SONORK_NET_EVENT_FLAG_READ;}

	BOOL
		WriteEvent() const
		{ return event_flags&SONORK_NET_EVENT_FLAG_WRITE;}

	BOOL
		ExceptEvent() const
		{ return event_flags&SONORK_NET_EVENT_FLAG_EXCEPTION;}

	int
		_Connect(const TSonorkPhysAddr&, long send_sz, long recv_sz);

virtual	void
		Shutdown();
};

BOOL
	Sonork_Net_Start(BYTE hv, BYTE lv);

void
	Sonork_Net_Stop();
int
	Sonork_Net_SetBlocking(SOCKET sk, bool blocking);
int
	Sonork_Net_SetBufferSize(SOCKET sk,long send_size,long recv_size);

SONORK_C_STR
	Sonork_Net_StatusName(SONORK_NETIO_STATUS);

#if defined(SONORK_WIN32_BUILD)
#	define SONORK_MsecsToSelectTimeval(n)		((n)*1000)
#	define Sonork_Net_KillSocket(S)			closesocket(S)
#elif defined(SONORK_LINUX_BUILD)
#	define SONORK_MsecsToSelectTimeval(n)		((n)*1000)
#	define Sonork_Net_KillSocket(S)			{shutdown(S,1);close(S);}
#endif


#if defined(SONORK_WIN32_BUILD)
#define IS_SONORK_SOCKERR_WOULDBLOCK(n)	((n)==WSAEWOULDBLOCK)
#define IS_SONORK_SOCKERR_CONNRESET(n)	((n)==WSAECONNRESET)
#define IS_SONORK_SOCKERR_CONNREFUSED	((n)==WSAECONNREFUSED)
#else
#define IS_SONORK_SOCKERR_WOULDBLOCK(n)	((n)==EWOULDBLOCK||(n)==EINPROGRESS)
#define IS_SONORK_SOCKERR_CONNRESET(n)	((n)==ECONNRESET)
#define IS_SONORK_SOCKERR_CONNREFUSED(n)	((n)==ECONNREFUSED)
#endif

#endif


