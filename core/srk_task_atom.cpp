#include "srk_defs.h"
#pragma hdrstop
#include "srk_task_atom.h"
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

// ---------------------------------------------------------------------------
// TSonorkRemindData

void 	TSonorkRemindData::Clear()
{
	memset(&header,0,sizeof(header));
	title.Clear();
	notes.Clear();
	values.Clear();
}

void 	TSonorkRemindData::CODEC_WriteDataMem	(TSonorkCodecWriter& CODEC) const
{
//  Version note #1:
//	V1.04.06 wrote a zero as first DWORD
//	CODEC.WriteDW(0);		// VN#1
//	V1.05.00 and above write the header size as low word of first DWORD
	CODEC.WriteDW(sizeof(header));	// VN#1

	CODEC.WriteDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&title);
	CODEC.Write(&notes);
	CODEC.Write(&values);

}
void 	TSonorkRemindData::CODEC_ReadDataMem	(TSonorkCodecReader& CODEC)
{
	DWORD ver_size;
	CODEC.ReadDW(&ver_size);
	CODEC.ReadDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
//	V1.04.06 wrote a zero as first DWORD
	if(ver_size != 0)
	{
		ver_size&=0xfff;
		CODEC.Skip(ver_size - sizeof(header));
	}
	CODEC.Read(&title);
	CODEC.Read(&notes);
	CODEC.Read(&values);
}
DWORD	TSonorkRemindData::CODEC_DataSize() const
{
	return
		sizeof(header)
		+	sizeof(DWORD)
		+	title.CODEC_Size()
		+	notes.CODEC_Size()
		+	values.CODEC_Size();
}

// ---------------------------------------------------------------------------
// TSonorkTaskData
void 	TSonorkTaskData::CODEC_Clear()
{
	memset(&header,0,sizeof(header));
	notes.Clear();
	values.Clear();
}

void	TSonorkTaskData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
	CODEC.Write(&notes);
	CODEC.Write(&values);
}
void	TSonorkTaskData::CODEC_ReadDataMem(TSonorkCodecReader&CODEC)
{
	CODEC.ReadDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
	CODEC.Read(&notes);
	CODEC.Read(&values);
}
DWORD	TSonorkTaskData::CODEC_DataSize() const
{
	return sizeof(header)
		+	notes.CODEC_Size()
		+	values.CODEC_Size();
}
