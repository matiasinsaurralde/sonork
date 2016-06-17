#include "srk_defs.h"
#pragma hdrstop
#include "srk_pairvalue.h"
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
// TSonorkPairValueQueueAtom

void	TSonorkPairValueQueue::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	TSonorkListIterator I;
	TSonorkPairValue *	PV;
	DWORD 			items=Items();

	CODEC.WriteDW( 0 );		// Version
	CODEC.WriteDW( items );	// Item Count

	if(items)
	{
		BeginEnum(I);
		while( (PV=EnumNext(I)) != NULL && CODEC.CodecOk() )
			PV->CODEC_WriteDataMem( CODEC );
		EndEnum(I);
	}
}
void	TSonorkPairValueQueue::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD 	items;
	TSonorkPairValue *	PV;

	CODEC.ReadDW( &items );		// Read Version
	if( (items&0xf) == 0 )
	{
		CODEC.ReadDW( &items );	// Item Count
		if( CODEC.CodecOk() )
		{
			while( items--  )
			{
				SONORK_MEM_NEW( PV=new TSonorkPairValue );
				PV->CODEC_ReadDataMem( CODEC );
				if( !CODEC.CodecOk() )
				{
					SONORK_MEM_DELETE( PV );
					break;
				}
				Add( PV );
			}
		}
	}
}

DWORD	TSonorkPairValueQueue::CODEC_DataSize()	const
{
	TSonorkListIterator I;
	TSonorkPairValue *	PV;
	DWORD			size;

	size=sizeof(DWORD)*2;
	if( Items() )
	{
		BeginEnum(I);
		while( (PV=EnumNext(I)) != NULL )
			size+=PV->CODEC_Size();
		EndEnum(I);
	}
	return size;
}


// ---------------------------------------------------------------------------
// TSonorkPairValueQueue

TSonorkPairValueQueue::TSonorkPairValueQueue()
{}

TSonorkPairValueQueue::~TSonorkPairValueQueue()
{
	Clear();
}
bool TSonorkPairValueQueue::Remove(SONORK_C_CSTR name)
{
	TSonorkPairValue *PV;
	if( (PV=Get(name)) != NULL)
	{
		return Remove(PV);
	}
	return false;

}

TSonorkPairValue *TSonorkPairValueQueue::GetType(SONORK_C_CSTR name, SONORK_PAIR_VALUE_TYPE type)
{
	TSonorkPairValue *PV;
	if( (PV=Get(name)) != NULL)
	{
		if(type != SONORK_PAIR_VALUE_ANY)
		{
			if(PV->Type() != type)
				return NULL;
		}
	}
	return PV;
}
TSonorkPairValue *TSonorkPairValueQueue::GetIndexed(SONORK_C_CSTR name,UINT idx,SONORK_PAIR_VALUE_TYPE type)
{
	char tmp[64];
	sprintf(tmp,"%s%u",name,idx);
	return GetType(tmp,type);
}

TSonorkPairValue *TSonorkPairValueQueue::Get(SONORK_C_CSTR name)
{
	TSonorkPairValue *PV;
	TSonorkListIterator I;

	BeginEnum(I);
	while( (PV=EnumNext(I)) != NULL )
	{
		if( !SONORK_StrNoCaseCmp(PV->Name(),name) )
			break;

	}
	EndEnum(I);
 	return PV;
}
void TSonorkPairValueQueue::Clear()
{
	TSonorkPairValue *PV;

	while( (PV=(TSonorkPairValue*)w_RemoveFirst()) != NULL )
	{
		SONORK_MEM_DELETE( PV );
	}
	_Clear();

}


// ---------------------------------------------------------------------------
// TSonorkPairValue

TSonorkPairValue::TSonorkPairValue()
{
	type	=SONORK_PAIR_VALUE_NONE;
	value.dw=0;
}
TSonorkPairValue::TSonorkPairValue(SONORK_C_CSTR p_name, SONORK_PAIR_VALUE_TYPE p_type)
{
	type	=SONORK_PAIR_VALUE_NONE;
	name.Set( p_name );
	value.dw=0;
	SetType(p_type);
}
TSonorkPairValue::~TSonorkPairValue()
{
	Clear();
}
void TSonorkPairValue::Clear()
{
	SetType(SONORK_PAIR_VALUE_NONE);
}
void TSonorkPairValue::SetType(SONORK_PAIR_VALUE_TYPE newType)
{
	if( type == newType )
		return;
	switch( type )
	{
		case SONORK_PAIR_VALUE_NONE:
		case SONORK_PAIR_VALUE_DWORD:
		case SONORK_PAIR_VALUE_DWORD2:
		default:
			break;
		case SONORK_PAIR_VALUE_SHORT_STRING:
			assert( value.s_str != NULL);
			SONORK_MEM_DELETE( value.s_str);
			break;
		case SONORK_PAIR_VALUE_DYN_DATA:
			assert( value.d_data != NULL);
			SONORK_MEM_DELETE( value.d_data );
			break;
	}
	type = newType;
	switch( type )
	{
		default:
			type=SONORK_PAIR_VALUE_NONE;
		// break ommited, fall to case SONORK_PAIR_VALUE_NONE
		case SONORK_PAIR_VALUE_NONE:
		case SONORK_PAIR_VALUE_DWORD:
			value.dw = 0;
			break;

		case SONORK_PAIR_VALUE_DWORD2:
			value.dw2.Clear();
			break;

		case SONORK_PAIR_VALUE_SHORT_STRING:
			SONORK_MEM_NEW( value.s_str = new TSonorkShortString );
			break;

		case SONORK_PAIR_VALUE_DYN_DATA:
			SONORK_MEM_NEW( value.d_data = new TSonorkDynData );
			break;
	}
}
TSonorkShortString* TSonorkPairValue::SetStr(SONORK_C_CSTR str)
{
	SetType( SONORK_PAIR_VALUE_SHORT_STRING );
	value.s_str->Set( str );
	return value.s_str;
}
const	TSonorkShortString* TSonorkPairValue::GetStr() const
{
	if( type == SONORK_PAIR_VALUE_SHORT_STRING )
		return value.s_str;
	return NULL;
}

TSonorkShortString* TSonorkPairValue::wGetStr()
{
	if( type == SONORK_PAIR_VALUE_SHORT_STRING )
		return value.s_str;
	return NULL;
}
SONORK_C_CSTR		TSonorkPairValue::CStr() const
{
	if( type == SONORK_PAIR_VALUE_SHORT_STRING )
		return value.s_str->CStr();
	return "";
}
DWORD	TSonorkPairValue::DW() const
{
	if( type == SONORK_PAIR_VALUE_DWORD )
		return value.dw;
	return 0;
}
DWORD TSonorkPairValue::SetDW(DWORD v)
{
	SetType(SONORK_PAIR_VALUE_DWORD);
	value.dw = v;
	return value.dw;

}
SONORK_DWORD2& TSonorkPairValue::SetDW2(const SONORK_DWORD2& v)
{
	SetType(SONORK_PAIR_VALUE_DWORD2);
	value.dw2.Set(v);
	return value.dw2;
}
TSonorkDynData*		TSonorkPairValue::SetDynData(const void*ptr,UINT psz,SONORK_ATOM_TYPE ptype)
{
	SetType(SONORK_PAIR_VALUE_DYN_DATA);
	if(ptr&&psz)
	{
		value.d_data->SetRaw(ptr,psz,ptype);
	}
	else
	{
		value.d_data->Clear();
		value.d_data->SetDataType(ptype);
	}
	return value.d_data;
}
TSonorkDynData*		TSonorkPairValue::SetDynDataAtom(TSonorkCodecAtom*A)
{
	SetType(SONORK_PAIR_VALUE_DYN_DATA);
	if( A )
	{
		A->CODEC_Write( value.d_data );
	}
	else
	{
		value.d_data->Clear();
	}
	return value.d_data;
}

bool	TSonorkPairValue::GetDW2(SONORK_DWORD2& v) const
{
	if( type == SONORK_PAIR_VALUE_DWORD2 )
	{
		v.Set( value.dw2);
		return true;
	}
	v.Clear();
	return false;
}

UINT	TSonorkPairValue::CODEC_Size() const
{
	UINT size;
	size=sizeof(DWORD) + ::CODEC_Size(&name);
	switch( type )
	{
		case SONORK_PAIR_VALUE_NONE:
		default:
			break;

		case SONORK_PAIR_VALUE_DWORD:
			size+=sizeof(DWORD);
			break;

		case SONORK_PAIR_VALUE_DWORD2:
			size+=sizeof(SONORK_DWORD2);
			break;

		case SONORK_PAIR_VALUE_SHORT_STRING:
			size+=::CODEC_Size( value.s_str);
			break;

		case SONORK_PAIR_VALUE_DYN_DATA:
			size+=::CODEC_Size( value.d_data);
			break;
	}
	return size;
}
void	TSonorkPairValue::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(type&0xff);
	CODEC.Write(&name);
	switch( type )
	{
		case SONORK_PAIR_VALUE_NONE:
		default:
			break;

		case SONORK_PAIR_VALUE_DWORD:
			CODEC.WriteDW(value.dw);
			break;

		case SONORK_PAIR_VALUE_DWORD2:
			CODEC.WriteDW2(&value.dw2);
			break;

		case SONORK_PAIR_VALUE_SHORT_STRING:
			CODEC.Write(value.s_str);
			break;

		case SONORK_PAIR_VALUE_DYN_DATA:
			CODEC.Write(value.d_data);
			break;
	}
}
void	TSonorkPairValue::CODEC_ReadDataMem	(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	CODEC.Read(&name);
	if( CODEC.CodecOk() )
	{
		SetType( (SONORK_PAIR_VALUE_TYPE)(aux&0xff) );
		switch( type )
		{
			case SONORK_PAIR_VALUE_NONE:
			default:
				break;

			case SONORK_PAIR_VALUE_DWORD:
				CODEC.ReadDW(&value.dw);
			break;

			case SONORK_PAIR_VALUE_DWORD2:
				CODEC.ReadDW2(&value.dw2);
			break;

			case SONORK_PAIR_VALUE_SHORT_STRING:
				CODEC.Read(value.s_str);
			break;

		case SONORK_PAIR_VALUE_DYN_DATA:
				CODEC.Read(value.d_data);
			break;
		}
	}
}

