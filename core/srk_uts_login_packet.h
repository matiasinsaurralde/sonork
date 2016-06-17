#if !defined(SONORK_UTS_LOGIN_PACKET_H)
#define SONORK_UTS_LOGIN_PACKET_H

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


#include "srk_defs.h"
#include "srk_crypt.h"

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

struct TSonorkUTSLoginPacket
:public TSonorkCodecAtom
{
public:

	struct HEADER
	{
		DWORD			signature;
		DWORD			version;
		DWORD			os_info;
		DWORD			reserved[16];
	}__SONORK_PACKED;

	struct REQ
	{
		TSonorkUTSDescriptor	descriptor;
		TSonorkPhysAddr			phys_addr;
		DWORD 	 				login_flags;
		SONORK_DWORD4			pin;
		DWORD					pin_type;
		DWORD					link_flags;
		DWORD					reserved[8];
	}__SONORK_PACKED;

	struct AKN
	{
		TSonorkUTSDescriptor	descriptor;
		TSonorkPhysAddr			phys_addr;
		DWORD 	 				login_flags;
		DWORD					link_id;
		DWORD					result;
		DWORD					err_code;
		DWORD					link_flags;
		DWORD					reserved[9];
	}__SONORK_PACKED;

	HEADER     				header;
	TSonorkDynData			login_data;
	TSonorkCryptInfo		crypt_info;
	TSonorkDynData			ext_data;

	void Clear();

// -----------
// CODEC

public:
	void 	CODEC_Clear()
			{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType() const
			{ return SONORK_ATOM_SONORK_UTS_LOGIN_PACKET_1; }

private:

	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem	(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;


}  __SONORK_PACKED;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

#endif