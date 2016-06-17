#include "srk_defs.h"
#pragma hdrstop
#include "srk_services.h"
#include "srk_codec_io.h"

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
// SONORK_SERVICE_AUTHS

// TSonorkAuthsAtom1
void
 TSonorkAuthsAtom1::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW(sizeof(auth));
	CODEC.WriteDWN((DWORD*)&auth , SIZEOF_IN_DWORDS(auth) );
}

void
 TSonorkAuthsAtom1::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	if( CODEC.DataType() != CODEC_DataType() )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		return;
	}
	CODEC.Skip(sizeof(DWORD));
	CODEC.ReadDWN((DWORD*)&auth , SIZEOF_IN_DWORDS(auth) );
}

// TSonorkAuthsAtom2

void
 TSonorkAuthsAtom2::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC)	const
{
	CODEC.WriteDW(sizeof(auth));
	CODEC.WriteDWN((DWORD*)&auth , SIZEOF_IN_DWORDS(auth) );
	CODEC.Write(&user_data);
}

void
 TSonorkAuthsAtom2::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD aux;
	if( CODEC.DataType() != CODEC_DataType() )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		return;
	}
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&auth , SIZEOF_IN_DWORDS(auth) );
	CODEC.Skip((aux&0xfff) - sizeof(auth));
	CODEC.Read(&user_data);
}

DWORD
 TSonorkAuthsAtom2::CODEC_DataSize() const
{
	return  sizeof(DWORD)
		+	::CODEC_Size(&auth)
		+  	::CODEC_Size(&user_data);
}
void
 TSonorkAuthsAtom2::CODEC_Clear()
{
	auth.Clear();
	user_data.Clear();
}


// ----------------------------------------------------------------------------
// TSonorkDataServerLoginReq

void
 TSonorkDataServerLoginReq::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
}

DWORD
 TSonorkDataServerLoginReq::CODEC_DataSize()	const
{
	return sizeof(DWORD) + sizeof(header);
}

void
 TSonorkDataServerLoginReq::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
}

void
 TSonorkDataServerLoginReq::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_LOGIN_REQ )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.Skip(sizeof(DWORD));
		CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	}
}



// ----------------------------------------------------------------------------
// TSonorkFileInfo

void
 TSonorkFileInfo::Set(const TSonorkFileInfo&O)
{
	locus.Set(O.locus);
	name.Set(O.name);
	memcpy(&attr,&O.attr,sizeof(attr));
	data.Set(O.data);
}

void
 TSonorkFileInfo::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW(sizeof(locus) + sizeof(attr));
	CODEC.WriteDWN((DWORD*)&locus , SIZEOF_IN_DWORDS(locus)+SIZEOF_IN_DWORDS(attr) );
	CODEC.Write(&name);
	CODEC.Write(&data);
}

void
 TSonorkFileInfo::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD	aux;
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&locus
		, SIZEOF_IN_DWORDS(locus)+SIZEOF_IN_DWORDS(attr) );
	CODEC.Skip( (aux&0xfff) - ( sizeof(locus) + sizeof(attr) ) );
	CODEC.Read(&name);
	CODEC.Read(&data);
}

DWORD
 TSonorkFileInfo::CODEC_DataSize() const
{
	return		sizeof(DWORD)
			+ sizeof(locus)
			+ sizeof(attr)
			+ ::CODEC_Size(&name)
			+ ::CODEC_Size(&data);
}

void
 TSonorkFileInfo::Clear()
{
	SONORK_ZeroMem(&locus,sizeof(locus)+sizeof(attr));
	name.Clear();
	data.Clear();
};

// ----------------------------------------------------------------------------
// TSonorkDataServerOldPutFileReq

void
 TSonorkDataServerPutFileTargetResult::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	ERR.SetOk();
}

DWORD
  TSonorkDataServerPutFileTargetResult::CODEC_DataSize() const
{
	return sizeof(DWORD) + sizeof(header) + ::CODEC_Size(&ERR);
}
void
 TSonorkDataServerPutFileTargetResult::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&ERR);
}

void
  TSonorkDataServerPutFileTargetResult::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD	aux;
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_PUT_FILE_TARGET_RESULT )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW( &aux );
		CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) );
		CODEC.Skip((aux&0xfff) - sizeof(header));
		CODEC.Read(&ERR);
	}
}

// ----------------------------------------------------------------------------
// TSonorkDataServerOldPutFileReq

TSonorkDataServerNewPutFileReq::TSonorkDataServerNewPutFileReq()
{
	target_atom.SetSource(&target_list
			,(SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_TARGET_LIST
			,true);
}
void
 TSonorkDataServerNewPutFileReq::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&file_info);
	CODEC.Write(&target_atom);
	CODEC.Write(&crypt_info);
}

void
 TSonorkDataServerNewPutFileReq::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD	aux;
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_PUT_FILE_NEW )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW( &aux );
		CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) );
		CODEC.Skip((aux&0xfff) - sizeof(header));
		CODEC.Read(&file_info);
		CODEC.Read(&target_atom);
		CODEC.Read(&crypt_info);
	}
}
DWORD
 TSonorkDataServerNewPutFileReq::CODEC_DataSize() const
{
	return sizeof(DWORD)
		+ sizeof(header)
		+ ::CODEC_Size(&file_info)
		+ ::CODEC_Size(&target_atom)
		+ ::CODEC_Size(&crypt_info);
}


void
 TSonorkDataServerNewPutFileReq::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	file_info.CODEC_Clear();
	target_list.Clear();
};

// ----------------------------------------------------------------------------
// TSonorkDataServerOldPutFileReq

void
 TSonorkDataServerOldPutFileReq::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&file_info);
}

void
 TSonorkDataServerOldPutFileReq::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD	aux;
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_PUT_FILE_OLD )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW( &aux );
		CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) );
		CODEC.Skip((aux&0xfff) - sizeof(header));
		CODEC.Read(&file_info);
	}
}
DWORD
 TSonorkDataServerOldPutFileReq::CODEC_DataSize() const
{
	return sizeof(DWORD) + sizeof(header) + ::CODEC_Size(&file_info);
}


void
 TSonorkDataServerOldPutFileReq::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	file_info.CODEC_Clear();
};

// ----------------------------------------------------------------------------
// TSonorkDataServerOldGetFileReq

void
 TSonorkDataServerNewGetFileReq::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW( sizeof(header) );
	CODEC.WriteDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) + SIZEOF_IN_DWORDS(locus) );
	CODEC.Write(&crypt_info);
}
void
 TSonorkDataServerNewGetFileReq::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD	aux;
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_GET_FILE_NEW )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW(&aux);
		CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header)  );
		CODEC.Skip( (aux&0xfff) - sizeof(header)  );
		CODEC.ReadDWN((DWORD*)&locus, SIZEOF_IN_DWORDS(locus)  );
		CODEC.Read(&crypt_info);
	}
}
DWORD
 TSonorkDataServerNewGetFileReq::CODEC_DataSize() const
{
	return sizeof(DWORD)
		+ sizeof( header )
		+ sizeof( locus )
		+ ::CODEC_Size(&crypt_info);
}
void
 TSonorkDataServerNewGetFileReq::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header) + sizeof(locus));
	crypt_info.Clear();
};

// ----------------------------------------------------------------------------
// TSonorkDataServerOldGetFileReq

void
 TSonorkDataServerOldGetFileReq::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	CODEC.WriteDW( sizeof(header) + sizeof(locus) );
	CODEC.WriteDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) + SIZEOF_IN_DWORDS(locus) );
}
void
 TSonorkDataServerOldGetFileReq::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	DWORD	aux;
	if( CODEC.DataType() != SRK_DATA_SERVER_ATOM_GET_FILE_OLD )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW(&aux);
		CODEC.ReadDWN((DWORD*)&header, SIZEOF_IN_DWORDS(header) + SIZEOF_IN_DWORDS(locus) );
		CODEC.Skip( (aux&0xfff) - (sizeof(header) + sizeof(locus)) );
	}
}
DWORD
 TSonorkDataServerOldGetFileReq::CODEC_DataSize() const
{
	return sizeof(DWORD) + sizeof( header ) + sizeof( locus );
}
void
 TSonorkDataServerOldGetFileReq::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header) + sizeof(locus));

};

// ----------------------------------------------------------------------------
// SONORK_SERVICE_DATA_SERVER

DWORD
 TSrkIpcPacket::E_Req(DWORD D_size, UINT function, const TSonorkCodecAtom*A)
{
	header.ipc_function 	= function;
	header.ipc_version  	= 0;
	if(A)
	{
		if(A->CODEC_WriteMem(DataPtr(), D_size)!=SONORK_RESULT_OK )
		{
			header.ipc_flags = SONORK_IPC_PACKET_F_INVALID; //SONORK_PCK_F_NO_DATA
			D_size=0;
		}
		else
			header.ipc_flags = 0;
	}
	else
	{
		//header.ipc_flags = SONORK_PCK_F_NO_DATA;
		D_size=0;
	}
	return D_size + sizeof(header);
}


bool
 TSrkIpcPacket::D_Req( DWORD P_size, TSonorkCodecAtom*A )
{
	if( (header.ipc_flags & SONORK_IPC_PACKET_F_INVALID) ||  P_size < sizeof(header))
		return false;

	if(A == NULL )return true;
	if( P_size == sizeof(header) )
	{
		A->CODEC_Clear();
		return true;
	}
	else
	{
		P_size-=sizeof(header);
		if(A->CODEC_ReadMem( DataPtr(), P_size)==SONORK_RESULT_OK)
			return true;
	}
	return false;

}
DWORD
 TSrkIpcPacket::E_Akn(DWORD D_size, UINT function, const TSonorkCodecAtom*A)
{
	header.ipc_function 	= function;
	header.ipc_version  	= 0;
	if(A)
	{
		if(A->CODEC_WriteMem(DataPtr(), D_size)!=SONORK_RESULT_OK)
		{
			header.ipc_flags = SONORK_IPC_PACKET_F_INVALID; //SONORK_PCK_F_NO_DATA|
			D_size=0;
		}
		else
		{
			header.ipc_flags = 0;
		}
	}
	else
	{
		header.ipc_flags = 0;//SONORK_PCK_F_NO_DATA;
		D_size=0;
	}
	return D_size + sizeof(header);
}
bool
 TSrkIpcPacket::D_Akn(DWORD P_size, TSonorkCodecAtom*A)
{
	if( (header.ipc_flags & SONORK_IPC_PACKET_F_INVALID) ||  P_size < sizeof(header))
		return false;
	if( header.ipc_flags & SONORK_IPC_PACKET_F_ERROR )
		return false;

	if(A == NULL )return true;
	if( P_size == sizeof(header) )
	{
		A->CODEC_Clear();
		return true;
	}
	else
	{
		P_size-=sizeof(header);
		if(A->CODEC_ReadMem( DataPtr(), P_size)==SONORK_RESULT_OK)
			return true;
	}
	return false;
}
DWORD
 TSrkIpcPacket::E_Error(DWORD D_size, UINT function, const TSonorkError& ERR)
{
	header.ipc_function 	= function;
	header.ipc_version  	= 0;
	header.ipc_flags = SONORK_IPC_PACKET_F_ERROR;
	ERR.CODEC_WriteMem(DataPtr(),D_size);
	return D_size+sizeof(header);
}
bool
 TSrkIpcPacket::D_Error(DWORD P_size, TSonorkError&ERR)
{
	if( (header.ipc_flags & SONORK_IPC_PACKET_F_INVALID) ||  P_size < sizeof(header))
		return false;
	if( !(header.ipc_flags & SONORK_IPC_PACKET_F_ERROR ))
		return false;
	P_size-=sizeof(header);
	return ERR.CODEC_ReadMem( DataPtr(), P_size) == SONORK_RESULT_OK;

}


