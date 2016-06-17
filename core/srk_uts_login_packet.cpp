#include "srk_defs.h"
#pragma hdrstop
#include "srk_uts.h"
#include "srk_uts_login_packet.h"
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




DWORD
 TSonorkUTSLoginPacket::CODEC_DataSize() const
{
	return 	sizeof(DWORD)
		+	sizeof(header)
		+	::CODEC_Size(&login_data)
		+	::CODEC_Size(&crypt_info)
		+	::CODEC_Size(&ext_data);
}

void
 TSonorkUTSLoginPacket::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW( sizeof(header) );
	CODEC.WriteDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header ) );
	CODEC.Write(&login_data);
	CODEC.Write(&crypt_info);
	CODEC.Write(&ext_data);
}

void
 TSonorkUTSLoginPacket::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD	aux;
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN( (DWORD*)&header, SIZEOF_IN_DWORDS( header) );
	CODEC.Skip((aux&0xfff) - sizeof(header));
	CODEC.Read(&login_data);
	CODEC.Read(&crypt_info);
	CODEC.Read(&ext_data);
}

void
 TSonorkUTSLoginPacket::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	login_data.Clear();
	crypt_info.Clear();
	ext_data.Clear();
}

