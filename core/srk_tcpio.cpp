#include "srk_defs.h"
#pragma hdrstop
#include "srk_tcpio.h"

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

int TSonorkTcpEngine::Listen(DWORD host, WORD port)
{
	int rv;
	rv=_Bind(host, port);
	if(!rv)rv=_Listen(5);
	return rv;
}

bool TSonorkTcpEngine::Accept(UINT select_msecs, SOCKET *new_sk,sockaddr_in*sender_address, bool blocking)
{
	assert(Flags()&SONORK_NETIO_F_LISTEN);

	if( _Select(select_msecs) > 0 )
	{
		if( ReadEvent() )
			if( !_Accept(new_sk,sender_address,blocking) )
				return true;
	}
	return false;
}

int TSonorkTcpEngine::Connect(const TSonorkPhysAddr& cx_addr)
{
	DWORD rv;
	if(SocksEnabled())
	{
		if(  (rv=_Connect(socks_info.physAddr, 0 , 0 )) == 0 )
			remote_phys_addr.Set(cx_addr);
	}
	else
	{
		rv=_Connect(cx_addr, 0 , 0);
	}
	return rv;
}

bool
 TSonorkTcpEngine::TCP_Recv(UINT select_msecs)
{
	if(_Select(select_msecs)<=0)return false;

	if(ExceptEvent())
	{
		if(Status()==SONORK_NETIO_STATUS_CONNECTING||Status()==SONORK_NETIO_STATUS_CONNECTED)
		{
			SetStatus(SONORK_NETIO_STATUS_DISCONNECTED);
		}
	}

	if(WriteEvent())
	{
		netio_flags|=SONORK_NETIO_F_WRITE;
		EnableSelectWrite(false);
		if(Status()==SONORK_NETIO_STATUS_CONNECTING)
		{
			if(SocksEnabled())
			{
				ClearWriteEvent();
				SendSocksConnectRequest();
			}
			else
			{
				SetStatus(SONORK_NETIO_STATUS_CONNECTED);
				DoWrite();
			}
		}
		else
			DoWrite();
	}

	if(ReadEvent())
	{
		if(!_Recv())
		{
			if(Status()==SONORK_NETIO_STATUS_CONNECTING)
			{
				if(SocksEnabled())
				{
					ClearReadEvent();
					RecvSocksRequest();
				}
				return false;
			}
			return true;
		}
	}
	return false;
}

///////////////////////////////////////

TSonorkRawTcpEngine::TSonorkRawTcpEngine(UINT buffer_size)
:TSonorkTcpEngine(buffer_size)
{
	o_packet=NULL;
	o_offset=0;
}

UINT
 TSonorkRawTcpEngine::Recv(UINT select_msecs)
{
	if(TCP_Recv(select_msecs))
		return RecvBytes();
	return 0;
}
void
 TSonorkRawTcpEngine::DoWrite()
{
	UINT		wr_bytes;
	int		err_code;


	if(!o_packet)
	{
		o_packet=o_queue.RemoveFirst();
		o_offset=0;
	}

	while(o_packet && (Flags()&SONORK_NETIO_F_WRITE))
	{

		err_code=_Send((const char*)o_packet->BufferEof(o_offset)
				,o_packet->RemainingSize(o_offset)
				,wr_bytes);

		o_offset+=wr_bytes;
		if(err_code)
		{
			if(IS_SONORK_SOCKERR_WOULDBLOCK(err_code))
			{
				netio_flags&=~SONORK_NETIO_F_WRITE;
				EnableSelectWrite(true);
				break;
			}
			break;
		}
		if(o_packet->TxComplete(o_offset))
		{
			Sonork_FreeRawTcpPacket(o_packet);
			o_packet=o_queue.RemoveFirst();
			o_offset=0;
		}
	}
}

DWORD
 TSonorkRawTcpEngine::Send(const void*data,UINT data_size)
{
	TSonorkRawTcpPacket *P;

	P=Sonork_AllocRawTcpPacket(data_size);
	memcpy(P->Buffer(),data,data_size);

	if(!o_packet)
		o_packet=P;
	else
		o_queue.Add(P);

	if(Flags()&SONORK_NETIO_F_WRITE)
		DoWrite();

	return 0;
}
void
 TSonorkRawTcpEngine::Shutdown()
{
	TSonorkTcpEngine::Shutdown();

	if(o_packet)
	{
		SONORK_MEM_DELETE(o_packet);
	}

	while((o_packet=o_queue.RemoveFirst())!=NULL)
		Sonork_FreeRawTcpPacket(o_packet);
}

///////////////////////////////////////

TSonorkPacketTcpEngine::TSonorkPacketTcpEngine(UINT buffer_size)
:TSonorkTcpEngine(buffer_size)
{
	o_packet=NULL;
    i_packet=NULL;
    o_offset=i_offset=0;
}
TSonorkPacketTcpEngine::~TSonorkPacketTcpEngine()
{
	if(i_packet)Sonork_FreeTcpPacket(i_packet);
    if(o_packet)Sonork_FreeTcpPacket(o_packet);
}

TSonorkTcpPacket*
 TSonorkPacketTcpEngine::Recv(UINT select_msecs, UINT max_loops)
{
	BYTE *src,*ptr;
	UINT recv_size,header_bytes, data_bytes,loop_no=0;
	do
	{
		if(!TCP_Recv(select_msecs))
			break;
		if((recv_size=RecvBytes())==0)
			break;
		src=Buffer();
		do
		{
			if(i_offset<SONORK_TCP_HEADER_SIZE)
			{
				ptr=((BYTE*)&i_header) + i_offset;
				header_bytes=SONORK_TCP_HEADER_SIZE-i_offset;
				if(header_bytes>recv_size)
					header_bytes=recv_size;

				memcpy(ptr,src,header_bytes);
				src+=header_bytes;
				recv_size-=header_bytes;
				i_offset+=header_bytes;
				if(i_offset>=SONORK_TCP_HEADER_SIZE)
				{
					i_header.Normalize();
					if(i_header.size > SONORK_PACKET_MAX_FULL_SIZE)
					{
						SetStatus(SONORK_NETIO_STATUS_DISCONNECTED);
						return NULL;
					}
					assert(i_packet==NULL);
					i_packet=Sonork_AllocTcpPacket( i_header.size );
					memcpy(i_packet->pHeader(),&i_header,SONORK_TCP_HEADER_SIZE);
				}
				else
				{
					break;
				}
			}
			if(!recv_size)
				break;
			assert(i_packet!=NULL);
			if(recv_size>i_header.RemainingSize(i_offset))
				data_bytes=i_header.RemainingSize(i_offset);
			else
				data_bytes=recv_size;
			i_packet->AppendBuffer(i_offset,src,data_bytes);
			src+=data_bytes;
			recv_size-=data_bytes;
			i_offset+=data_bytes;
			if(i_header.TxComplete(i_offset) )
			{
				i_queue.Add(i_packet);
				i_packet=NULL;
				i_offset=0;
			}
		}while(recv_size);
	}while(++loop_no<max_loops);
	return i_queue.RemoveFirst();
}

void TSonorkPacketTcpEngine::Shutdown()
{
	TSonorkTcpEngine::Shutdown();

    if(o_packet)SONORK_MEM_DELETE(o_packet);
	while((o_packet=o_queue.RemoveFirst())!=NULL)
	Sonork_FreeTcpPacket(o_packet);
}
bool TSonorkPacketTcpEngine::GetNextOutputPacket()
{
	o_packet=o_queue.RemoveFirst();
	o_offset=0;
    if(o_packet)
    {
	memcpy(&o_header,o_packet->pHeader(),sizeof(TSonorkTcpPacketHeader));
		o_packet->NormalizeHeader();
	return true;
    }
	return false;
}
void TSonorkPacketTcpEngine::DoWrite()
{
	UINT	wr_bytes;
    int		err_code;
    TSonorkTcpPacket *P;
	if(!o_packet)
    	if(!GetNextOutputPacket())
        	return;

    while(o_packet && (Flags()&SONORK_NETIO_F_WRITE))
    {
	P=o_packet;
	err_code=_Send((const char*)P->pHeader()+o_offset
			, o_header.RemainingSize(o_offset)
			, wr_bytes);
		o_offset+=wr_bytes;
		if(err_code)
		{
			if(IS_SONORK_SOCKERR_WOULDBLOCK(err_code))
			{
				netio_flags&=~SONORK_NETIO_F_WRITE;
				EnableSelectWrite(true);
				break;
		}
		break;
        }
        if(o_header.TxComplete(o_offset))
        {
	    	Sonork_FreeTcpPacket(P);
			if(!GetNextOutputPacket())
    	    	return;
        }
    }
}

DWORD TSonorkPacketTcpEngine::SendPacket(TSonorkTcpPacket*P)
{
	o_queue.Add(P);
	if(Flags()&SONORK_NETIO_F_WRITE)DoWrite();
	return 0;
}
DWORD TSonorkPacketTcpEngine::SendPacket(SONORK_NETIO_PACKET_HFLAG hf,const void*data,DWORD data_size)
{
	TSonorkTcpPacket*P;
    P=Sonork_AllocTcpPacket(data_size);
    memcpy(P->DataPtr(),data,data_size);
    P->HFlag()=hf;
    return SendPacket(P);
}



void  TSonorkTcpEngine::SetSocksV4(TSonorkPhysAddr& addr)
{
	if(addr.Type() == SONORK_PHYS_ADDR_TCP_1)
    {
		socks_info.physAddr.Set(addr);
		socks_info.version  = 4;
	}
	else
	{
		socks_info.Clear();
	}
}



void  TSonorkTcpEngine::SendSocksConnectRequest()
{
	int 	error;
	BYTE 	socks_request[ SOCKS_V4_CONNECT_REQUEST_BUFFER_SIZE ];
	UINT	bytes,bytes_sent;

	if(!SocksEnabled())
		error=10;
	else
	if(Protocol()!=SONORK_NETIO_PROTOCOL_TCP)
		error=20;
	else
	{


		bytes = (UINT)BuildSocksV4ConnectRequest(
				  socks_request
				, RemotePhysAddress() );


		if(SocksInfo().physAddr.Inet1()->sin_addr.s_addr == 0)
			error = 30;
		else
			error=_Send(socks_request,bytes,bytes_sent);
	}
	if(error)
		SetStatus(SONORK_NETIO_STATUS_DISCONNECTED);
}

// ----------------------------------------------------------------------------

void
 TSonorkTcpEngine::RecvSocksRequest()
{
	int error;

	error = ParseSocksV4ConnectResponse(Buffer(), RecvBytes());
	SetStatus(error != 0 ? SONORK_NETIO_STATUS_DISCONNECTED : SONORK_NETIO_STATUS_CONNECTED);
}



TSonorkTcpPacket *
 Sonork_AllocTcpPacket(UINT data_size)
{

	TSonorkTcpPacket*P=SONORK_MEM_ALLOC(TSonorkTcpPacket,sizeof(TSonorkTcpPacket) + data_size);
	P->header.size=data_size;
	return P;
}
TSonorkRawTcpPacket *Sonork_AllocRawTcpPacket(UINT data_size)
{
	TSonorkRawTcpPacket *P=SONORK_MEM_ALLOC(TSonorkRawTcpPacket,sizeof(TSonorkRawTcpPacket) + data_size);
	P->size=data_size;
	return P;
}



#define SONORK_NETIO_SOCKS_COMMAND_CONNECT	1
#define SONORK_NETIO_SOCKS_COMMAND_BIND		2

#define SOCKS_V4_CONNECT_REQUEST_LEN		9
#define SOCKS_V4_CONNECT_REPLY_LEN		8
int
 BuildSocksV4ConnectRequest( BYTE*buffer , const TSonorkPhysAddr&phys_addr )	// Returns size
{
	*(buffer + 0)=(BYTE)4;
	*(buffer + 1)=SONORK_NETIO_SOCKS_COMMAND_CONNECT;
	// 2,3 = port
	*((WORD*)(buffer + 2))=phys_addr.data.inet1.addr.sin_port;
	// 4,5,6,7 = IP
	*((DWORD*)(buffer + 4))=phys_addr.data.inet1.addr.sin_addr.s_addr;
	*(buffer + 8)=*(buffer + 9)=0;
	return SOCKS_V4_CONNECT_REQUEST_LEN;
}
int ParseSocksV4ConnectResponse(BYTE*buffer,UINT sz)	// returns error code (0 if ok)
{
	int error;
	if( sz < SOCKS_V4_CONNECT_REPLY_LEN)
	{
		error = 1;
	}
	else
	{
		error = *(buffer + 1);
		if(error==90) // request granted
		{
			error=0;
		}
	}
	return error;
}




void
 TSonorkTcpPacketHeader::Normalize()
{
#if defined(SONORK_BYTE_REVERSE)
	hflag=SONORK_REV_WORD(hflag);
	size =SONORK_REV_DWORD(size);
#endif
}

