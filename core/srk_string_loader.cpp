#include "srk_defs.h"
#pragma hdrstop
#include "srk_string_loader.h"

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



void TSonorkStringLoader::Open(char *buffer,UINT buffer_size)
{
	cur_ptr	= buffer;
	cur_len 	= 0;
	if(buffer_size>0)
		max_len	= buffer_size-1;
	else
		max_len = 0;
}
void TSonorkStringLoader::Add(char c)
{
	if(cur_len<max_len)
	{
		*cur_ptr++=c;
		cur_len++;
	}
}

void 	TSonorkStringLoader::AddEncode(char c)	// Prefixes '\\' for special chars
{
	if(c<' '||c=='\\')
	{
		bool prefix=true;
		switch(c)
		{
			case '\\':
				break;

			case '\n':
				c='n';
				break;

			case '\r':
				if(flags&TGSL_IGNORE_CR)
					return;
				c='r';
				break;

			case '\t':
				c='t';
				break;

			case '\b':
				c='b';
				break;
				
			default:
				prefix=false;
				break;
		}
		if(prefix)Add('\\');
	}
	Add(c);

}
char TSonorkStringLoader::GetDecodeChar(char c)
{
	switch(c)
	{
		case 'n':
			c='\n';
			break;
			
		case 'r':
			c = (char)((flags&TGSL_IGNORE_CR)?0:'\r');
			break;

		case 't':
			c='\t';
			break;

		case 'b':
			c='\b';
			break;
			
		default:	// any other '\<x>' will be decoded to '<x>'
			break;
	}
	return c;
}
char TSonorkStringLoader::AddDecode(char c)
{
	if((c=GetDecodeChar(c))!=0)
		Add(c);
	return c;
}
void 	TSonorkStringLoader::AddStr(const char* str)
{
	while(*str)Add(*str++);
}
void 	TSonorkStringLoader::AddStrEncode(const char* str)
{
	while(*str)AddEncode(*str++);

}
void 	TSonorkStringLoader::AddStrDecode(const char* str)
{
	char c;
	while(*str)
	{
		c=*str++;
		if(c=='\\')
		{
			if(!*str)break;
			AddDecode(*str++);
		}
		else
			Add(c);
	}
}

void 	TSonorkStringLoader::Push(TSonorkStringLoaderMark& undo)
{
	undo.len 	= cur_len;
	undo.ptr 	= cur_ptr;
	undo.flags  = flags;
}
void 	TSonorkStringLoader::Pop(TSonorkStringLoaderMark& undo, BOOL position)
{
	if(position)
	{
		cur_len = undo.len;
		cur_ptr	= undo.ptr;
	}
	flags	= undo.flags;
}


UINT TSonorkStringLoader::Close()
{
	*cur_ptr=0;
	cur_ptr=NULL;
	return cur_len;
}
