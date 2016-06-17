#include "srk_defs.h"
#pragma hdrstop
#include "srk_stream_lr.h"

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



void TSonorkStreamLineReader::Clear()
{
	data.SetBlockSize(256);
	data.Clear();
	queue.Clear();
}
bool
	TSonorkStreamLineReader::Append(SONORK_C_CSTR data_ptr,UINT data_sz)
{
	UINT old_len, new_len, scan_len;
	TSonorkShortString	*str;
	SONORK_C_CSTR	bol, eol;

	if( data.DataSize() > max_line_length )
	{
		return false;
	}
	bol = data_ptr;
	scan_len=0;
	while(scan_len<data_sz)
	{
//		TRACE_DEBUG("Check @ %u/%u",scan_len,data_sz);
		if(*bol=='\n')
		{
			bol++;
			if(++scan_len==data_sz)
				break;
		}
		for(eol=bol ; scan_len < data_sz ; eol++,scan_len++)
			if(*eol=='\r')
				break;

		if( scan_len == data_sz )
			break;		// Not found

		old_len  = data.DataSize();
		new_len = (eol - bol);
		str = new TSonorkShortString();
		str->SetBufferSize( old_len + new_len + 1 );
		if(old_len)
		{
			memcpy(str->Buffer() , data.Buffer() , old_len );
			data.Clear();
		}
		memcpy(str->Buffer() + old_len , bol , new_len );
		*(str->Buffer() + old_len + new_len )=0;
		queue.Add(str);
		bol		= eol;
		if(++scan_len<data_sz && *bol=='\r')
			bol++;
	}
	new_len = (bol - data_ptr);
//	TRACE_DEBUG("data.Append SZ:%u - CL:%u = %u bytes",data_sz,new_len,data_sz-new_len);
	assert(new_len<=data_sz);
	data.Append(data_ptr + new_len , data_sz-new_len );
	return true;
}
