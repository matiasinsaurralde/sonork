#if !defined(SONORK_UTS_PACKET_H)
#define SONORK_UTS_PACKET_H

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


#define SONORK_UTS_MAX_PACKET_SIZE    			(SONORK_CODEC_ATOM_MAX_DATA_SIZE+sizeof(TSonorkUTSPacket))

#define SONORK_UTS_PM_VERSION 			0x000f


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

struct TSonorkUTSPacket
{

//-----------------------------------
	struct _HEADER {
		WORD	cmd;
		WORD	flags;
	}__SONORK_PACKED;

	_HEADER	header;

//-----------------------------------

	DWORD
		Cmd() const
		{ return header.cmd;}

	BYTE
		Version() const
		{ return (BYTE)(header.flags&SONORK_UTS_PM_VERSION);}

//-----------------------------------

	BYTE*
		DataPtr() const
		{ return ((BYTE*)&header)+sizeof(header);}

	DWORD
		DataSize(DWORD packet_size) const
		{ return packet_size-sizeof(header); }

	DWORD
		PacketSize(DWORD data_size) const
		{ return data_size+sizeof(header); }

//-----------------------------------
	DWORD
		E_Atom(DWORD P_size, DWORD cmd,BYTE version,const struct TSonorkCodecAtom*A);

	DWORD
		E_Data(DWORD P_size, DWORD cmd,BYTE version,const void*,DWORD);

	SONORK_RESULT
		D_Atom(DWORD P_size, TSonorkCodecAtom*A) const;

	BYTE*
		D_Data(DWORD P_size, DWORD& data_size) const;

	DWORD
		E_CtrlMsg(DWORD P_size, DWORD cmd,BYTE version,const struct TSonorkCtrlMsg*,const void*data,DWORD data_size);

	BYTE*
		D_CtrlMsg(DWORD P_size, TSonorkCtrlMsg*,DWORD*data_size) const;

}__SONORK_PACKED;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif


TSonorkUTSPacket*	SONORK_AllocUtsPacket(DWORD data_size);
void			SONORK_FreeUtsPacket(TSonorkUTSPacket*P);


#endif