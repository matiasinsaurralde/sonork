#include "srk_defs.h"
#pragma hdrstop
#include "srk_uts_packet.h"
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


#define ENCODE_SECTION_STARTS\
		TSonorkPacketWriter CODEC(DataPtr(),pSz); \
		header.cmd   =(WORD)pCmd;header.flags =(WORD)(pVersion&SONORK_UTS_PM_VERSION);

#define ENCODE_SECTION_ENDS \
		 if( CODEC.CodecOk() )return sizeof(TSonorkUTSPacket)+CODEC.Size();\
		 return 0

#define DECODE_SECTION_STARTS	\
		TSonorkPacketReader CODEC(TGP->DataPtr(),pSz);	\

#define DECODE_SECTION_ENDS	\
		if( CODEC.CodecOk() )return true\
		return false

// ----------------------------------------------------------------------------

DWORD
 TSonorkUTSPacket::E_CtrlMsg(DWORD pSz,DWORD pCmd,BYTE pVersion,const TSonorkCtrlMsg*msg,const void*data,DWORD data_size)
{
	ENCODE_SECTION_STARTS;
		CODEC.WriteCM1(msg);
		CODEC.WriteDW(data_size);
		if(data_size != 0)
			CODEC.WriteRaw( data , data_size );
	ENCODE_SECTION_ENDS;
}

// ----------------------------------------------------------------------------

BYTE*
 TSonorkUTSPacket::D_CtrlMsg(DWORD pSz,TSonorkCtrlMsg*msg,DWORD* data_size) const
{
	DWORD packet_data_size = DataSize(pSz);
	TSonorkPacketReader CODEC(DataPtr(),packet_data_size);
	CODEC.ReadCM1(msg);
	CODEC.ReadDW(data_size);
	if( packet_data_size - CODEC.Offset() < *data_size )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
	}
	if( !CODEC.CodecOk() )
	{
		*data_size=0;
		return NULL;
	}
	return 	DataPtr() + CODEC.Offset();
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkUTSPacket::E_Atom(DWORD pSz,DWORD pCmd,BYTE pVersion,const TSonorkCodecAtom*A)
{
	ENCODE_SECTION_STARTS;
		CODEC.WriteAtom	(A);
	ENCODE_SECTION_ENDS;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkUTSPacket::E_Data(DWORD pSz,DWORD pCmd,BYTE pVersion,const void*data,DWORD data_size)
{
	ENCODE_SECTION_STARTS;
		CODEC.WriteRaw(data,data_size);
	ENCODE_SECTION_ENDS;
}

// ----------------------------------------------------------------------------

BYTE*
 TSonorkUTSPacket::D_Data(DWORD P_size, DWORD& data_size) const
{
	data_size = DataSize(P_size);
	return DataPtr();
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkUTSPacket::D_Atom(DWORD P_size,TSonorkCodecAtom*A) const
{
	DWORD data_size = DataSize(P_size);
	return A->CODEC_ReadMemNoSize(DataPtr(),data_size);
}

// ----------------------------------------------------------------------------

TSonorkUTSPacket*SONORK_AllocUtsPacket(DWORD data_size)
{
	assert( data_size < SONORK_UTS_MAX_PACKET_SIZE+256 );
	return SONORK_MEM_ALLOC( TSonorkUTSPacket, sizeof(TSonorkUTSPacket) + data_size + sizeof(DWORD) );
}

// ----------------------------------------------------------------------------

void SONORK_FreeUtsPacket(TSonorkUTSPacket*P)
{
	SONORK_MEM_FREE(P);
}

// ----------------------------------------------------------------------------


/*

DWORD		TGP_E_Atom(TSonorkUTSPacket*, DWORD P_size, DWORD cmd,BYTE version,const class TSonorkCodecAtom*A);
DWORD		TGP_E_Data(TSonorkUTSPacket*, DWORD P_size, DWORD cmd,BYTE version,const void*,DWORD	 );
SONORK_RESULT	TGP_D_Atom(TSonorkUTSPacket*P, DWORD P_size, TSonorkCodecAtom*A);
BYTE*		TGP_D_Data(TSonorkUTSPacket*P, DWORD P_size, DWORD& data_size);
DWORD		TGP_E_CtrlMsg(TSonorkUTSPacket*, DWORD P_size, DWORD cmd,BYTE version,const class TSonorkCtrlMsg*,const BYTE*data,DWORD data_size);
BYTE*		TGP_D_CtrlMsg(TSonorkUTSPacket*, DWORD P_size, TSonorkCtrlMsg*,DWORD*data_size);



//-----------------------------------

inline DWORD TGP_E_AtomData(TSonorkUTSPacket*P,DWORD sz,WORD cmd,BYTE version,const TSonorkCodecAtom*A)
{
	return TGP_E_Atom(P,sz,cmd,version,A);
}

inline DWORD TGP_E_RawData(TSonorkUTSPacket*P,DWORD sz,WORD cmd,BYTE version,const void*data,DWORD data_size)
{
	return TGP_E_Data(P,sz,cmd,version,data,data_size);
}
*/

