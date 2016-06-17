#include "srk_defs.h"
#pragma hdrstop
#include "srk_svr_login_packet.h"
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




void
 TSonorkSvrLoginReqPacket::Prepare2(const TSonorkUserData*UD
			,SONORK_C_CSTR	pass
			,DWORD 		login_flags
			,const TSonorkVersion& version)
{
	SONORK_ZeroMem(&header,sizeof(header));
	if(UD==NULL || (login_flags&SONORK_LOGIN_RF_GUEST))
	{
		login_flags|=SONORK_LOGIN_RF_GUEST;
	}
	else
	{
		login_flags&=~SONORK_LOGIN_RF_GUEST;
		header.region.Set(UD->Region());
		header.userId.Set(UD->userId);
	}
	password.SetStr( pass );
	header.oldVersion  	= SONORK_CLIENT_VERSION_NUMBER;
	header.old_os_info  	= 0;//MAKE_SONORK_OS_INFO(0);
	header.loginFlags 	= login_flags;
	header.version.Set( version );
	extData.Clear();
	cryptInfo.Clear();
	header.signature=password.GenerateSignature();
}

DWORD
 TSonorkSvrLoginReqPacket::CODEC_DataSize() const
{
	return  sizeof(DWORD)
		+	sizeof(header)
		+	::CODEC_Size(&password)
		+	::CODEC_Size(&cryptInfo)
		+	::CODEC_Size(&extData);
}

void
 TSonorkSvrLoginReqPacket::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header ) );
	CODEC.Write(&password);
	CODEC.Write(&cryptInfo);
	CODEC.Write(&extData);

}

void
 TSonorkSvrLoginReqPacket::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	if( CODEC.DataType() != CODEC_DataType() )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW(&aux);
		aux&=0xfff;
		CODEC.ReadDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header) );
		CODEC.Skip(aux - sizeof(header));
		CODEC.Read(&password);
		CODEC.Read(&cryptInfo);
		CODEC.Read(&extData);
	}
}

void
 TSonorkSvrLoginReqPacket::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	password.Clear();
	cryptInfo.Clear();
	extData.Clear();
}




DWORD TSonorkSvrLoginAknPacket::CODEC_DataSize() const
{
	return  sizeof(DWORD)
		+	sizeof(header)
		+	::CODEC_Size(&sid_info)
		+	::CODEC_Size(&crypt_info)
		+	::CODEC_Size(&ext_data);
}
/*
UINT 	TSonorkSvrLoginAknPacket::CODEC_MinDataSize() const
{
	return 	sizeof(header)
		+	sid_info.CODEC_MinSize()
		+	crypt_info.CODEC_MinSize()
		+	ext_data.CODEC_MinSize();
}
*/

void	TSonorkSvrLoginAknPacket::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header ) );
	CODEC.Write(&sid_info);
	CODEC.Write(&crypt_info);
	CODEC.Write(&ext_data);
}
void	TSonorkSvrLoginAknPacket::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	if( CODEC.DataType() != CODEC_DataType() )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	else
	{
		CODEC.ReadDW(&aux);
		aux&=0xfff;
		CODEC.ReadDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header) );
		CODEC.Skip(aux - sizeof(header));
		CODEC.Read(&sid_info);
		CODEC.Read(&crypt_info);
		CODEC.Read(&ext_data);
	}
}
void TSonorkSvrLoginAknPacket::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	sid_info.Clear();
	crypt_info.Clear();
	ext_data.Clear();
}


