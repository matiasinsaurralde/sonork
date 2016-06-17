#include "srk_defs.h"
#pragma hdrstop
#include "srk_netio.h"

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

// ----------------------------------------------------------------------------
// TSonorkNetIO
// ----------------------------------------------------------------------------
void	TSonorkNetIO::SetRecvBufferSize(UINT sz)
{
	if( sz != buffer.size )
	{
		SONORK_MEM_FREE(buffer.ptr);
		buffer.size=sz;
		buffer.ptr=SONORK_MEM_ALLOC(BYTE,buffer.size);
	}
}
int TSonorkNetIO::_Connect(const TSonorkPhysAddr& addr, long send_sz, long recv_sz)
{
	int		rv;
#if defined(SONORK_WIN32_BUILD)
	int 		addr_len;
#elif defined(SONORK_LINUX_BUILD)
	socklen_t 	addr_len;
#endif

	if(sk==INVALID_SOCKET)
	{
		rv=CreateSocket(false , send_sz, recv_sz);
		if(rv)return rv;
	}
	addr_len=sizeof(*remote_phys_addr.Inet1());
	remote_phys_addr.Set(addr);
	if(::connect(Socket(),(sockaddr*)remote_phys_addr.Inet1(),addr_len))
	{
		rv=WSAGetLastError();
		if(IS_SONORK_SOCKERR_WOULDBLOCK(rv))
		{
			rv=0;
			SetStatus(SONORK_NETIO_STATUS_CONNECTING);
		}
	}
	else
	{
		rv=0;
		getsockname(Socket(),(sockaddr*)local_phys_addr.Inet1(),&addr_len);
		SetStatus(SONORK_NETIO_STATUS_CONNECTED);
	}
	if( ! rv )
	{
		EnableSelectWrite( Protocol() == SONORK_NETIO_PROTOCOL_TCP );
		EnableSelectRead( true );
		EnableSelectExcept( true );
	}
	return rv;
}

int TSonorkNetIO::_Send(const void*data,UINT bytes,UINT&bytes_sent)
{
	bytes_sent=(UINT)::send(Socket(),(const char*)data,bytes,0);
	if(bytes_sent==SOCKET_ERROR)
	{
		bytes_sent=0;
		return WSAGetLastError();
	}
	return 0;
}

//---------------------------------------------------------------------
// _Recv
// reads the data available and places it into _RecvBuffer()
//---------------------------------------------------------------------
UINT TSonorkNetIO::_Recv()
{
	DWORD code;
	recv_bytes=::recv(Socket(),(char*)Buffer(),BufferSize(),0);
	if(recv_bytes==(UINT)SOCKET_ERROR)
	{
		recv_bytes=0;
L0:
		code=WSAGetLastError();
		if(!IS_SONORK_SOCKERR_WOULDBLOCK(code))
		{
			SetStatus(SONORK_NETIO_STATUS_DISCONNECTED);
		}
		return code;
	}
	else
	if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
	{
		if(recv_bytes==0)
		{
			goto L0;
		}
	}
    return 0;
}

int
 TSonorkNetIO::_Bind(DWORD host, WORD port)
{
	int rv;
	local_phys_addr.header.type		= SONORK_PHYS_ADDR_TCP_1;
	local_phys_addr.Inet1()->sin_port   	= htons(port);
	local_phys_addr.Inet1()->sin_family 	= AF_INET;
	local_phys_addr.Inet1()->sin_addr.s_addr = host;
	if(Socket()==INVALID_SOCKET)
	{
		rv=CreateSocket();
		if(rv)
		{
			sonork_printf("NET I/O: Cannot create socket (Error %u)\n",rv);
			return rv;
		}
	}
	if(bind(Socket()
		, (sockaddr*)local_phys_addr.Inet1()
		, sizeof (*local_phys_addr.Inet1())) != 0)
	{
		rv=WSAGetLastError();
		sonork_printf("NET I/O: Cannot bind socket (Error %u)\n",rv);
		return rv;
	}
	netio_flags|=SONORK_NETIO_F_BIND;
	return 0;
}

int TSonorkNetIO::_Listen(int back_log)
{
	if(Protocol()!=SONORK_NETIO_PROTOCOL_TCP)
		return 0;
	if(Socket()==INVALID_SOCKET)
	{
		int rv=CreateSocket();
		if(rv)return rv;
	}
	if(listen(Socket(),back_log)!=0)
	{
		return WSAGetLastError();
	}
	netio_flags|=SONORK_NETIO_F_LISTEN;
	SetStatus(SONORK_NETIO_STATUS_LISTENING);
	return 0;
}

int TSonorkNetIO::_Accept(SOCKET *new_sk,sockaddr_in*sender_address, bool blocking)
{
	int err_code;
#if defined(SONORK_WIN32_BUILD)
	int addr_len;
#else
	socklen_t	addr_len;
#endif

	addr_len = sizeof(*sender_address);
	*new_sk=accept(Socket(),(sockaddr*)sender_address,&addr_len);
 	if(*new_sk==INVALID_SOCKET)
    {
    	return WSAGetLastError();
    }
    err_code=Sonork_Net_SetBlocking(*new_sk,blocking);
    if(err_code)
    {
		Sonork_Net_KillSocket(*new_sk);
		return err_code;
    }
    return 0;
}


void
 TSonorkNetIO::EnableSelectRead(bool s)
{
	FD_ZERO(active_r_fd_set);
	if(s)FD_SET(Socket(),active_r_fd_set);

}

void
 TSonorkNetIO::EnableSelectWrite(bool s)
{
	FD_ZERO(active_w_fd_set);
	if(s)FD_SET(Socket(),active_w_fd_set);
}

void
 TSonorkNetIO::EnableSelectExcept(bool s)
{
	FD_ZERO(active_e_fd_set);
	if(s)FD_SET(Socket(),active_e_fd_set);
}

int
 TSonorkNetIO::CreateSocket(bool blocking, long send_size,long recv_size )
{
	DWORD type;

	DestroySocket();
	if(Protocol()==SONORK_NETIO_PROTOCOL_UDP)
		type=SOCK_DGRAM;
	else
	if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
		type=SOCK_STREAM;
	else
		return -1;
	sk=socket(PF_INET,type,0);
	if(sk==INVALID_SOCKET)
		return WSAGetLastError();

	SetSocketBufferSize(send_size,recv_size);

	if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
	{
		int rv;
		rv=Sonork_Net_SetBlocking(sk,blocking);
		if(rv)
		{
			//Migs
			Sonork_Net_KillSocket(sk);
			sk=INVALID_SOCKET;
			return rv;
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// _Select
// tests the socket for events during <msecs>
// returns the number of sockets set (can be 0 , 1 or SOCKET_ERROR)
// that have events pending. The events are set in <event_flags>
//---------------------------------------------------------------------

int  TSonorkNetIO::_Select(DWORD msecs)
{
	if( Status() > SONORK_NETIO_STATUS_DISCONNECTED )
	{
		int rv;
		struct timeval tv={0,SONORK_MsecsToSelectTimeval(msecs)};

#if defined(SONORK_WIN32_BUILD)
		if((work_r_fd_set->fd_count=active_r_fd_set->fd_count)!=0)
			work_r_fd_set->fd_array[0]=active_r_fd_set->fd_array[0];

		if((work_w_fd_set->fd_count=active_w_fd_set->fd_count)!=0)
			work_w_fd_set->fd_array[0]=active_w_fd_set->fd_array[0];

		if((work_e_fd_set->fd_count=active_e_fd_set->fd_count)!=0)
			work_e_fd_set->fd_array[0]=active_e_fd_set->fd_array[0];
#else
		*work_r_fd_set=*active_r_fd_set;
		*work_w_fd_set=*active_w_fd_set;
		*work_e_fd_set=*active_e_fd_set;
#endif

		event_flags=0;
		rv=select(
#if defined(SONORK_WIN32_BUILD)
				0
#elif defined(SONORK_LINUX_BUILD)
				Socket()+1
#endif
				,work_r_fd_set
				,work_w_fd_set
				,work_e_fd_set,&tv);

		if(rv==SOCKET_ERROR)
			rv=0;
		else
		if(rv)
		{
			if(FD_ISSET(Socket(),work_r_fd_set))
				event_flags|=SONORK_NET_EVENT_FLAG_READ;

			if(FD_ISSET(Socket(),work_w_fd_set))
				event_flags|=SONORK_NET_EVENT_FLAG_WRITE;

			if(FD_ISSET(Socket(),work_e_fd_set))
				event_flags|=SONORK_NET_EVENT_FLAG_EXCEPTION;
		}
		return rv;
	}
	else
		return 0;		
}


int
 TSonorkNetIO::SetSocketBufferSize(long send_size,long recv_size)
{
	return Sonork_Net_SetBufferSize(Socket(),send_size,recv_size );
}

void
 TSonorkNetIO::DestroySocket()
{
	if( sk != INVALID_SOCKET)
	{
		Sonork_Net_KillSocket( sk );
		sk=INVALID_SOCKET;
		FD_ZERO(active_r_fd_set);
		FD_ZERO(active_w_fd_set);
		FD_ZERO(active_e_fd_set);
	}
}
void
 TSonorkNetIO::Shutdown()
{
	netio_flags=0;
	DestroySocket();
}


TSonorkNetIO::TSonorkNetIO( SONORK_NETIO_PROTOCOL prot,UINT buffer_size)
{
	UINT fd_set_size;
	protocol=prot;
	netio_flags=0;
	event_flags=0;
	buffer.size=buffer_size;
	buffer.ptr=SONORK_MEM_ALLOC(BYTE,buffer_size);
	sk=INVALID_SOCKET;
#if defined(SONORK_WIN32_BUILD)
	fd_set_size=(sizeof(u_int)*2)+(1*sizeof(SOCKET));
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
}

TSonorkNetIO::~TSonorkNetIO()
{
	Shutdown();
	SONORK_MEM_FREE(buffer.ptr);
	SONORK_MEM_FREE(active_r_fd_set);
	SONORK_MEM_FREE(active_w_fd_set);
	SONORK_MEM_FREE(active_e_fd_set);
	SONORK_MEM_FREE(work_r_fd_set);
	SONORK_MEM_FREE(work_w_fd_set);
	SONORK_MEM_FREE(work_e_fd_set);
}


void
 TSonorkNetIO::SetStatus(SONORK_NETIO_STATUS new_status)
{
	if(new_status==SONORK_NETIO_STATUS_DISCONNECTED)
	{
		EnableSelectRead(false);
		EnableSelectWrite(false);
		EnableSelectExcept(false);
		if(Socket()!=INVALID_SOCKET)
		{
			Sonork_Net_KillSocket(sk);
			sk=INVALID_SOCKET;
		}
	}
	else
	{
		EnableSelectRead(true);
		if(Protocol()==SONORK_NETIO_PROTOCOL_TCP)
		{
			EnableSelectExcept(new_status!=SONORK_NETIO_STATUS_LISTENING);
			if(new_status==SONORK_NETIO_STATUS_CONNECTING)
				EnableSelectWrite(true);
		}
	}
	netio_status = new_status;

}




// ----------------------------------------------------------------------------
// Support functions
// ----------------------------------------------------------------------------

int Sonork_Net_SetBufferSize(SOCKET sk, long send_size,long recv_size )
{
	long  result_val;
#if defined(SONORK_WIN32_BUILD)
	int 		opt_val_size;
#elif defined(SONORK_LINUX_BUILD)
	socklen_t 	opt_val_size;
#else
#error NO IMPLEMENTATION
#endif
//	sonork_printf("SetBufferSize(%x,%d,%d)",sk,send_size,recv_size);


	if( send_size >= 128)
	{
		opt_val_size=sizeof(result_val);
		if(getsockopt(sk,SOL_SOCKET,SO_SNDBUF,(char*)&result_val,&opt_val_size))
			return GetLastError();
//		sonork_printf(" CurrentSend=%d",result_val);

		opt_val_size=sizeof(send_size);
		setsockopt(sk,SOL_SOCKET,SO_SNDBUF,(char*)&send_size,opt_val_size);
	}

	if( recv_size >= 128 )
	{
		opt_val_size=sizeof(result_val);
		if(getsockopt(sk,SOL_SOCKET,SO_RCVBUF,(char*)&result_val,&opt_val_size))
			return GetLastError();
//		sonork_printf(" CurrentRecv=%d",result_val);

		opt_val_size=sizeof(recv_size);
		setsockopt (sk,SOL_SOCKET,SO_RCVBUF,(char*)&recv_size,opt_val_size);
	}
	return 0;
}

int
 Sonork_Net_SetBlocking(SOCKET sk, bool blocking)
{
//Oliver,Migs
#if defined(SONORK_WIN32_BUILD)
 DWORD non_blocking=blocking?0:1;
 if(ioctlsocket (sk,FIONBIO,&non_blocking))
 {
	return WSAGetLastError();
 }
#elif defined(SONORK_LINUX_BUILD)
	int flags, res;

	flags = fcntl(sk, F_GETFL, 0);
	if(blocking)
		res = fcntl(sk, F_SETFL, O_NONBLOCK | flags);
	else
		res = fcntl(sk, F_SETFL, (~O_NONBLOCK) & flags);	// O_NONBLOCK tiene que estar seteado
	if(res < 0)
	{
		return errno;
	}
#else
#error NO IMPLEMENTATION
#endif
 return 0;
}


BOOL Sonork_Net_Start(BYTE hv, BYTE lv)
{
//Migs
#if defined(SONORK_WIN32_BUILD)
    WSADATA wsadata;
    WORD version;
    BOOL rv;
    version=lv;
    version<<=8;
    version|=hv;
    rv=(WSAStartup(version,&wsadata)==0);
    return rv;
#else
	return 1;
#endif
}
void Sonork_Net_Stop()
{
//Migs
#if defined(SONORK_WIN32_BUILD)
	WSACleanup();
#endif    
}

SONORK_C_STR Sonork_Net_StatusName(SONORK_NETIO_STATUS s)
{
static SONORK_C_CSTR
	gu_netio_status_name[SONORK_NETIO_STATUS_ITEMS]=
        {
         "Disconnected"
		,"Disconnecting"
		,"Req-Connect"
        ,"Connecting"
        ,"Authorizing"
        ,"Connected"
        ,"Listening"
        };
    if(s>=SONORK_NETIO_STATUS_DISCONNECTED&&s<SONORK_NETIO_STATUS_ITEMS)
		return (SONORK_C_STR)gu_netio_status_name[s];
	else
		return "??";
}

// ----------------------------------------------------------------------------
// TSonorkPhysAddr
// ----------------------------------------------------------------------------

bool TSonorkPhysAddr::operator==(const TSonorkPhysAddr&O)
{
	if(Type()==SONORK_PHYS_ADDR_TCP_1||Type()==SONORK_PHYS_ADDR_UDP_1)
	if(Type()==O.Type())
	{
		return
		data.inet1.addr.sin_addr.s_addr==O.data.inet1.addr.sin_addr.s_addr
		&&
		data.inet1.addr.sin_port==O.data.inet1.addr.sin_port;
	}
//    return true;	// why was it true?
	return false;
}

bool TSonorkPhysAddr::IsEqual(sockaddr_in*src) const
{
   if(Type()!=SONORK_PHYS_ADDR_NONE)
   {

	return
		data.inet1.addr.sin_addr.s_addr==src->sin_addr.s_addr
		&&
		data.inet1.addr.sin_port==src->sin_port;
	}
   return false;
}

int TSonorkPhysAddr::SetInet1(SONORK_PHYS_ADDR_TYPE t,SONORK_C_CSTR p_host_name,WORD port)
{
	int err;
	struct hostent *HOST_ENTRY;
	char	host_name[128];
	char	*colon;
	if(t==SONORK_PHYS_ADDR_TCP_1||t==SONORK_PHYS_ADDR_UDP_1)
	{
		SONORK_StrCopy(host_name,sizeof(host_name)-1,p_host_name);
		colon = strchr(host_name,':');
		if( colon )
		{
			*colon++=0;
			port=(WORD)atol(colon);
		}
		// else <port> as passed by parameters is used 
		data.inet1.addr.sin_port=htons(port);
		data.inet1.addr.sin_family=AF_INET;
		header.type=(BYTE)t;
		if(*host_name>='0'&&*host_name<='9')
		{
			data.inet1.addr.sin_addr.s_addr =  inet_addr(host_name);
			return 0;
		}
		else
		{
			HOST_ENTRY=gethostbyname(host_name);
			if(!HOST_ENTRY)
				err=WSAGetLastError();
			else
			{
				data.inet1.addr.sin_addr.s_addr = *((unsigned long *)(*HOST_ENTRY->h_addr_list));
				return 0;
			}
		}
	}
	else
		err=0;
	header.type=(BYTE)SONORK_PHYS_ADDR_NONE;
	return err;
}


int TSonorkPhysAddr::SetInet1(SONORK_PHYS_ADDR_TYPE t,unsigned long addr,WORD port)
{
	if(t==SONORK_PHYS_ADDR_TCP_1||t==SONORK_PHYS_ADDR_UDP_1)
	{
		header.type 			=(BYTE)t;
		data.inet1.addr.sin_family	=AF_INET;
		data.inet1.addr.sin_port	=htons(port);
		data.inet1.addr.sin_addr.s_addr =addr;
		return 0;
	}
	header.type=(BYTE)SONORK_PHYS_ADDR_NONE;
	return 1;
}

int TSonorkPhysAddr::SetInet1(SONORK_PHYS_ADDR_TYPE t, const sockaddr_in*src)
{
	if(t==SONORK_PHYS_ADDR_TCP_1||t==SONORK_PHYS_ADDR_UDP_1)
	{
		header.type=(BYTE)t;
		memcpy(&data.inet1.addr,src,sizeof(data.inet1.addr));
    }
	else
    	header.type=(BYTE)SONORK_PHYS_ADDR_NONE;
    return 0;
}



SONORK_PHYS_ADDR_TYPE TSonorkPhysAddr::GetInet1(sockaddr_in*src)
{
	if(Type()==SONORK_PHYS_ADDR_TCP_1||Type()==SONORK_PHYS_ADDR_UDP_1)
    {
    	memcpy(src,&data.inet1.addr,sizeof(data.inet1.addr));
		return Type();
    }
    else
    {
		src->sin_addr.s_addr=0;
		src->sin_port=0;
		return SONORK_PHYS_ADDR_NONE;
	}
}
void	TSonorkPhysAddr::SetType(SONORK_PHYS_ADDR_TYPE t, UINT family)
{
	header.type=(BYTE)t;
	data.inet1.addr.sin_family=(WORD)family;

}

static const char *gu_addr_type_name[SONORK_PHYS_ADDR_TYPES]={"","tcp:","udp:"};

char*
 TSonorkPhysAddr::GetStr(char*str) const
{
	if(Type()==SONORK_PHYS_ADDR_TCP_1||Type()==SONORK_PHYS_ADDR_UDP_1)
	{

		sprintf(str
			,"%s%s:%u"
			,gu_addr_type_name[Type()]
			,inet_ntoa(data.inet1.addr.sin_addr)
			,ntohs(data.inet1.addr.sin_port));
	}
	else
		*str=0;
	return str;
}

void TSonorkPhysAddr::SetStr(SONORK_C_CSTR S)
{
	if(*((DWORD*)S)==*((DWORD*)gu_addr_type_name[SONORK_PHYS_ADDR_TCP_1]))
    	header.type=SONORK_PHYS_ADDR_TCP_1;
    else
	if(*((DWORD*)S)==*((DWORD*)gu_addr_type_name[SONORK_PHYS_ADDR_UDP_1]))
    	header.type=SONORK_PHYS_ADDR_UDP_1;
    else
	{
		header.type=SONORK_PHYS_ADDR_NONE;
		return;
	}

	char str[64],*c;
	strcpy(str,S+4);
	c=strchr(str,':');
	if(c)
	{
		*c++=0;
		data.inet1.addr.sin_addr.s_addr =  inet_addr(str);
		data.inet1.addr.sin_family=AF_INET;
		data.inet1.addr.sin_port=htons((WORD)atoi(c));
		return;
	}
	header.type=SONORK_PHYS_ADDR_NONE;
	return ;
}
#if defined(SONORK_LINUX_BUILD)
int		GetLastError(){return errno;}
int		WSAGetLastError(){return errno;}
#endif

