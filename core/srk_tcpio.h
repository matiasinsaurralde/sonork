#if !defined(SONORK_TCPIO_H)
#define SONORK_TCPIO_H

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

#include "srk_data_lists.h"
#include "srk_netio.h"


#if !defined(SONORK_TCPIO_DEFAULT_BUFFER_SIZE)
#if defined(SONORK_SERVER_BUILD)
#	define SONORK_TCPIO_DEFAULT_BUFFER_SIZE		(4096*2)
#else
#	define SONORK_TCPIO_DEFAULT_BUFFER_SIZE		4096
#endif
#endif

#if defined(USE_PRAGMA_PUSH)
#	pragma	pack(push,1)
#endif

// ----------------------------------------------------------------------------

struct TSonorkTcpConnectAknPacket
{
	char 			message[80];
	WORD			result;
	DWORD			signature;
	DWORD			server_flags;
	DWORD			server_version;
	TSonorkOldUserAddr	addr;
} __SONORK_PACKED ;

// ----------------------------------------------------------------------------

struct TSonorkTcpPacketHeader
{
	SONORK_NETIO_PACKET_HFLAG	hflag;
	DWORD				size;

	DWORD
		Type()		const
		{ return SONORK_NETIO_HFLAG_PACKET_TYPE(hflag); }

	DWORD
		Flags()		const
		{ return SONORK_NETIO_HFLAG_PACKET_FLAGS(hflag); }

	DWORD
		AknCount()	const
		{ return SONORK_UDPIO_HFLAG_AKN_COUNT(hflag);}

	UINT
		FullSize()  	const
		{ return sizeof(*this)+size; }

	UINT
		DataSize()	const
		{ return size; }

	UINT
		RemainingSize(UINT offset) const
		{ return FullSize()-offset; }

	bool
		TxComplete(UINT offset)	const
		{ return RemainingSize(offset) == 0; }

	void
		Normalize();

} __SONORK_PACKED ;


#define SONORK_TCP_HEADER_SIZE	sizeof(TSonorkTcpPacketHeader)

// ----------------------------------------------------------------------------

struct TSonorkTcpPacket
{
friend TSonorkTcpPacket *Sonork_AllocTcpPacket(UINT data_size);
private:
	TSonorkTcpPacketHeader	header;
public:
	SONORK_NETIO_PACKET_HFLAG&
		HFlag()
		{ return header.hflag; }
	DWORD
		Type()	const
		{ return header.Type(); }

	DWORD
		Flags()	 const
		{ return header.Flags(); }

	DWORD
		AknCount() const
		{ return header.AknCount(); }

	void
		NormalizeHeader()
		{ header.Normalize();	}
	UINT
		FullSize() const
		{ return sizeof(header)+header.size;}

	UINT
		DataSize() const
		{ return header.size; }

	BYTE  *
		pHeader()
		{ return (BYTE*)&header; }

	BYTE  *
		DataPtr()
		{ return ((BYTE*)&header)+sizeof(header); }

	void
		SetDataSize(UINT sz)
		{ header.size=(DWORD)sz; }

	void
		AppendBuffer(UINT offset,const void *data, DWORD data_size)
		{ memcpy( pHeader()+offset , data , data_size);	}
		

} __SONORK_PACKED ;

#if defined(USE_PRAGMA_PUSH)
#	pragma	pack(pop)
#endif

// ----------------------------------------------------------------------------

DeclareSonorkQueueClass( TSonorkTcpPacket );

// ----------------------------------------------------------------------------

class TSonorkTcpEngine
:public TSonorkNetIO
{
protected:

	TSonorkHostInfo	socks_info;

	void
		SendSocksConnectRequest();

	void
		RecvSocksRequest();

	virtual void
		DoWrite(){};

	bool
		TCP_Recv(UINT select_msecs);
public:
	TSonorkTcpEngine(UINT buffer_size = SONORK_TCPIO_DEFAULT_BUFFER_SIZE)
		:TSonorkNetIO(SONORK_NETIO_PROTOCOL_TCP,buffer_size)
	{
		socks_info.Clear();
	};

	TSonorkHostInfo&
		SocksInfo()
		{ return socks_info; }

	bool
		SocksEnabled() const
		{ return socks_info.Enabled();}

	void
		SetSocksV4(TSonorkPhysAddr&);

	int
		Connect( const TSonorkPhysAddr& addr );

	int
		Listen(DWORD host, WORD port);

	bool
		Accept(UINT select_msecs,SOCKET*new_sk,sockaddr_in*sender_address, bool blocking=false);
};

// ----------------------------------------------------------------------------

class TSonorkPacketTcpEngine
:public TSonorkTcpEngine
{
	UINT			i_offset;
	TSonorkTcpPacketHeader	i_header;
	TSonorkTcpPacket	*i_packet;
	TSonorkTcpPacketQueue	i_queue;
	UINT			o_offset;
	TSonorkTcpPacketHeader	o_header;
	TSonorkTcpPacket	*o_packet;
	TSonorkTcpPacketQueue	o_queue;

	void
		DoWrite();

	bool
		GetNextOutputPacket();

public:

	TSonorkPacketTcpEngine(UINT buffer_size = SONORK_TCPIO_DEFAULT_BUFFER_SIZE);

	virtual ~TSonorkPacketTcpEngine();

	UINT
		PendingPackets() const
		{ return o_queue.Items()+(o_packet!=NULL?1:0); }

	DWORD
		SendPacket(TSonorkTcpPacket *P);

	DWORD
		SendPacket(SONORK_NETIO_PACKET_HFLAG,const void*data,DWORD size);

	TSonorkTcpPacket*
		Recv(UINT select_msecs, UINT max_loops=2);	// Caller must delete packet!

	void
		Shutdown();
};

// ----------------------------------------------------------------------------

struct TSonorkRawTcpPacket
{
friend  TSonorkRawTcpPacket *Sonork_AllocRawTcpPacket(UINT);

private:
	UINT	size;

public:

	DWORD
		Size() const
		{ return size;}

	UINT
		RemainingSize(UINT offset) const
		{ return Size()-offset;}

	bool
		TxComplete(UINT offset) const
		{ return RemainingSize(offset)==0;}

	BYTE  *
		Buffer()
		{ return ((BYTE*)&size)+sizeof(size);}

	BYTE  *
		BufferEof(UINT offset)
		{ return Buffer()+offset;}

	void
		AppendBuffer(UINT offset,const void *data, DWORD data_size)
		{ memcpy(BufferEof(offset),data,data_size); }
};

DeclareSonorkQueueClass( TSonorkRawTcpPacket );

class TSonorkRawTcpEngine
:public TSonorkTcpEngine
{
	UINT				o_offset;
	TSonorkRawTcpPacket*		o_packet;
	TSonorkRawTcpPacketQueue  	o_queue;

	void
		DoWrite();
public:
	TSonorkRawTcpEngine(UINT buffer_size = SONORK_TCPIO_DEFAULT_BUFFER_SIZE);

	UINT
		PendingPackets() const
		{ return o_queue.Items()+o_packet!=NULL?1:0; }

	DWORD
		Send(const void*data,UINT size);

	UINT
		Recv(UINT select_msecs);// returns received bytes (0 if nothing available)


	void
		Shutdown();
};

// defined in SONORK_TCPIO

#define SOCKS_V4_CONNECT_REQUEST_BUFFER_SIZE	12
int	BuildSocksV4ConnectRequest(BYTE*buffer, const TSonorkPhysAddr&);// Returns size to send
int	ParseSocksV4ConnectResponse(BYTE*buffer,UINT sz);	 // Returns error code (0 if OK)

struct TSonorkTcpPacket 	*Sonork_AllocTcpPacket(UINT data_size);
struct TSonorkRawTcpPacket 	*Sonork_AllocRawTcpPacket(UINT data_size);
inline void   			 Sonork_FreeTcpPacket(TSonorkTcpPacket*P){SONORK_MEM_FREE(P);}
inline void   			 Sonork_FreeRawTcpPacket(TSonorkRawTcpPacket*P){SONORK_MEM_FREE(P);}


#endif
