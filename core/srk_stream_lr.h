#if !defined(SONORK_STREAM_LR_H)
#define SONORK_STREAM_LR_H

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

#include "srk_data_types.h"

struct TSonorkStreamLineReader
{
private:
	// max_line_length
	// is a threshold value, it does not mean lines
	// will not at most <max_line_length> chars, it means
	// that the reader will stop Appending data when
	// this value has been exceeded and Append() will return false
	UINT					max_line_length;
	TSonorkDynData			data;
	TSonorkShortStringQueue	queue;
public:
	TSonorkStreamLineReader(){max_line_length=0xffff;}
	bool	Append(SONORK_C_CSTR,UINT);
	void	SetMaxLineLength(UINT n){max_line_length=n;}
	TSonorkShortString*
		RemoveLine()
	{
		return queue.RemoveFirst();
	}

	void	Clear();
};

#endif
