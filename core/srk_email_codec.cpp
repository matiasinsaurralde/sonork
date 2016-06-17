#include "srk_defs.h"
#pragma hdrstop
#include "srk_email_codec.h"
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


#if SONORK_CODEC_LEVEL>5

// ----------------------------------------------------------------------------
// TSonorkEmailAccount
// ----------------------------------------------------------------------------

void
 TSonorkEmailAccount::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	for(int i=0;i<SONORK_EMAIL_ACCOUNT_STRINGS;i++)
		str[i].Clear();
}
void
 TSonorkEmailAccount::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
	CODEC.WriteDW(SONORK_EMAIL_ACCOUNT_STRINGS);
	for(int i=0;i<SONORK_EMAIL_ACCOUNT_STRINGS;i++)
		CODEC.Write(&str[i]);
}
void
 TSonorkEmailAccount::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD i,aux;
	if( CODEC.DataType() != CODEC_DataType() )
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		return;
	}
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&header , SIZEOF_IN_DWORDS(header) );
	if( aux>sizeof(header))
		CODEC.Skip(aux - sizeof(header));
	CODEC.ReadDW(&aux);
	for(i=0 ; CODEC.CodecOk() && i<aux && i<SONORK_EMAIL_ACCOUNT_STRINGS ; i++ )
		CODEC.Read(&str[i]);
}

DWORD
 TSonorkEmailAccount::CODEC_DataSize()	const
{
	DWORD rv;
	rv = sizeof(DWORD)*2 + sizeof(header);
	for(int i=0;i<SONORK_EMAIL_ACCOUNT_STRINGS;i++)
		rv+=::CODEC_Size(&str[i]);
	return rv;

}


// ----------------------------------------------------------------------------
// TSonorkEmailExceptQueue
// ----------------------------------------------------------------------------

BOOL
 TSonorkEmailExceptQueue::Contains( SONORK_C_CSTR from, SONORK_C_CSTR to , SONORK_C_CSTR subject)
{
	BOOL rv=false;
	TSonorkEmailExcept*E;
	TSonorkListIterator I;

	BeginEnum(I);
	while(( E=EnumNext(I)) != NULL )
	{
		if(E->Contains(from,to,subject))
		{
			rv=true;
			break;
		}
	}
	EndEnum(I);


	return rv;

}


// ----------------------------------------------------------------------------
// TSonorkEmailExcept
// ----------------------------------------------------------------------------

BOOL
 TSonorkEmailExcept::Contains( SONORK_C_CSTR from, SONORK_C_CSTR to , SONORK_C_CSTR subject)
{
	TSonorkListIterator I;
	TSonorkEmailExceptEntry *E;
	SONORK_C_CSTR		t_str;
	UINT			conditions,matches;
	BOOL			testB;
	conditions = matches = 0;

	queue.BeginEnum(I);
	while( (E=queue.EnumNext(I))!=NULL )
	{
		if(!*E->ExceptStr())
			continue;
		switch(E->ExceptField() )
		{
			case SONORK_EMAIL_EXCEPT_FIELD_TO:
				t_str=to;
				break;
			case SONORK_EMAIL_EXCEPT_FIELD_FROM:
				t_str=from;
				break;
			case SONORK_EMAIL_EXCEPT_FIELD_SUBJECT:
				t_str=subject;
				break;
			default:
				continue;
		}
		switch( E->ExceptOp()&~SONORK_EMAIL_EXCEPT_OP_NEGATIVE  )
		{
			case SONORK_EMAIL_EXCEPT_OP_CONTAINS:
				testB = SONORK_StrStr( t_str , E->ExceptStr() ) != NULL ;
				break;

			default:
				continue;
		}
		conditions++;
		if(E->ExceptOp() & SONORK_EMAIL_EXCEPT_OP_NEGATIVE )
		{
			if(!testB)matches++;
		}
		else
		{
			if(testB)matches++;
		}
	}
	queue.EndEnum(I);
	return conditions > 0 && matches == conditions;
}

void
 TSonorkEmailExcept::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	name.Clear();
	queue.Clear();
}

void
 TSonorkEmailExcept::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	TSonorkListIterator I;
	TSonorkEmailExceptEntry *E;


	CODEC.WriteDW(0);
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&name);

	CODEC.WriteDW(queue.Items());
	queue.BeginEnum(I);
	while((E=queue.EnumNext(I))!=NULL && CODEC.CodecOk() )
		CODEC.Write(E);
	queue.EndEnum(I);
}

void
 TSonorkEmailExcept::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD 	aux;
	TSonorkEmailExceptEntry *E;

	if( CODEC.DataType() != CODEC_DataType())
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		return;
	}

	CODEC.ReadDW(&aux);
	if((aux&0xff)!=0)return;
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Skip((aux&0xfff) - sizeof(header));
	CODEC.Read(&name);

	CODEC.ReadDW(&aux);
	while(aux-- && CODEC.CodecOk() )
	{
		E=new TSonorkEmailExceptEntry;
		CODEC.Read(E);
		queue.Add(E);
	}
}

DWORD
 TSonorkEmailExcept::CODEC_DataSize()	const
{
	return sizeof(DWORD)*3
			+ sizeof(header)
			+ ::CODEC_Size(&name)
			+ queue.SumAtomsCodecSize();
}


// ----------------------------------------------------------------------------
// TSonorkEmailExceptEntry
// ----------------------------------------------------------------------------


void
 TSonorkEmailExceptEntry::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	string.Clear();
}

void
 TSonorkEmailExceptEntry::CODEC_WriteDataMem	(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(0);
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&string);
}

void
 TSonorkEmailExceptEntry::CODEC_ReadDataMem	(TSonorkCodecReader& CODEC)
{
	DWORD 	aux;
	if( CODEC.DataType() != CODEC_DataType())
	{
		CODEC.SetBadCodecError( __FILE__ , SONORK_MODULE_LINE );
		return;
	}
	CODEC.ReadDW(&aux);
	if((aux&0xff)==0)
	{
		CODEC.ReadDW(&aux);
		CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
		CODEC.Skip((aux&0xfff) - sizeof(header));
		CODEC.Read(&string);
	}
}

DWORD
 TSonorkEmailExceptEntry::CODEC_DataSize()	const
{
	return sizeof(DWORD)*2
			+ sizeof(header)
			+ ::CODEC_Size(&string);
}
#endif