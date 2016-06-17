#include "srk_defs.h"
#pragma hdrstop
#include "srk_crypt.h"
#include "srk_crypt_context.h"
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



#define MAX_DUMP_LINES	3

DWORD
 TSonorkCryptInfo::CODEC_DataSize() const
{
	return    sizeof(DWORD)
			+ sizeof(header)
			+ data.CODEC_Size();
}
void
 TSonorkCryptInfo::Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	data.Clear();
}

void
 TSonorkCryptInfo::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&data);
}
void
 TSonorkCryptInfo::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	aux&=0xfff;
	CODEC.Skip(aux - sizeof(header));
	CODEC.Read(&data);
}


void Dump(const char*title,void *pPtr,UINT sz);

UINT
 TSonorkCryptContext::MaxEncryptedSize(UINT input_size) const
{
	switch(CryptEngine())
	{
		case SONORK_CRYPT_ENGINE_SIMPLE:
		case SONORK_CRYPT_ENGINE_NONE:
			return input_size;
			
		case SONORK_CRYPT_ENGINE_RIJNDAEL:
			return input_size+32;
		default:
			return 0;
	}
}
UINT
 TSonorkCryptContext::MaxUncryptedSize(UINT input_size) const
{
	switch(CryptEngine())
	{
		case SONORK_CRYPT_ENGINE_NONE:
		case SONORK_CRYPT_ENGINE_SIMPLE:
			return input_size;
		case SONORK_CRYPT_ENGINE_RIJNDAEL:
			return input_size+32;
		default:
			return 0;
	}
}

SONORK_RESULT
 TSonorkCryptContext::Encrypt(TSonorkDynData* DD) const
{
	if( !Enabled() || !DD->DataSize() )return SONORK_RESULT_OK;
	if(SingleBufferEncryptionEnabled())
	{
		return SingleBufferEncrypt(DD->wBuffer(),DD->DataSize());
	}
	else
	{

		UINT 		o_size;
		BYTE*		o_buffer;
		SONORK_RESULT	result;
		
		o_size 		=MaxEncryptedSize( DD->DataSize() );
		o_buffer	=SONORK_MEM_ALLOC( BYTE , o_size );
		result = DoubleBufferEncrypt(
					DD->wBuffer()
				,	DD->DataSize()
				,	o_buffer
				,	&o_size);
		DD->Assign(o_buffer,o_size); // discards current data
		return result;
	}
}

SONORK_RESULT
 TSonorkCryptContext::Encrypt(void*input_buffer
				,UINT input_buffer_size
				,void**output_buffer
				,UINT*output_buffer_size
				,BOOL*new_output_buffer) const
{
	if(!Enabled())
	{
		*new_output_buffer=false;
		*output_buffer=input_buffer;
		*output_buffer_size=input_buffer_size;
		return SONORK_RESULT_OK;
	}
	else
	if(SingleBufferEncryptionEnabled())
	{
		*new_output_buffer=false;
		*output_buffer=input_buffer;
		*output_buffer_size=input_buffer_size;
		return SingleBufferEncrypt(input_buffer,input_buffer_size);
	}
	else
	{
		*new_output_buffer=true;
		*output_buffer_size=MaxEncryptedSize(input_buffer_size);
		*output_buffer=SONORK_MEM_ALLOC(void,*output_buffer_size);
		return DoubleBufferEncrypt(
				input_buffer
			,	input_buffer_size
			,	*output_buffer
			,	output_buffer_size);
	}
}
SONORK_RESULT
 TSonorkCryptContext::Uncrypt(TSonorkDynData* DD) const
{
	if( !Enabled() || !DD->DataSize() )
		return SONORK_RESULT_OK;
	if(SingleBufferEncryptionEnabled())
	{
		return SingleBufferUncrypt(DD->wBuffer(),DD->DataSize());
	}
	else
	{

		UINT 		o_size;
		BYTE*		o_buffer;
		SONORK_RESULT	result;

		o_size 		=MaxUncryptedSize( DD->DataSize() );
		o_buffer	=SONORK_MEM_ALLOC( BYTE , o_size );
		result = DoubleBufferUncrypt(
					DD->Buffer()
				,	DD->DataSize()
				,	o_buffer
				,	&o_size);
		DD->Assign(o_buffer,o_size); // discards current data
		return result;
	}
}

SONORK_RESULT
 TSonorkCryptContext::Uncrypt(void*input_buffer
				,UINT input_buffer_size
				,void**output_buffer
				,UINT*output_buffer_size
				,BOOL*new_output_buffer) const
{
	if(SingleBufferEncryptionEnabled())
	{
		*new_output_buffer=false;
		*output_buffer=input_buffer;
		*output_buffer_size=input_buffer_size;
		return SingleBufferUncrypt(input_buffer,input_buffer_size);
	}
    else
    {
        *new_output_buffer=true;
        *output_buffer_size=MaxUncryptedSize(input_buffer_size);
  		*output_buffer=SONORK_MEM_ALLOC(void,*output_buffer_size);
		return DoubleBufferUncrypt(
        		input_buffer
        	,	input_buffer_size
            ,	*output_buffer
            ,	output_buffer_size);
    }
}

SONORK_RESULT
 TSonorkCryptContext::DoubleBufferEncrypt(
			  const void *
			, UINT
			, void *
			, UINT*) const
{
	return SONORK_RESULT_NOT_SUPPORTED;
}
SONORK_RESULT
 TSonorkCryptContext::DoubleBufferUncrypt(
			  const void *
			, UINT
			, void *
			, UINT*) const
{
	return SONORK_RESULT_NOT_SUPPORTED;
}

SONORK_RESULT
 TSonorkCryptContext::SingleBufferEncrypt(void *buffer,UINT buffer_size) const
{
	switch(CryptEngine())
	{
		case SONORK_CRYPT_ENGINE_NONE:
			return SONORK_RESULT_OK;
		case SONORK_CRYPT_ENGINE_SIMPLE:
			Sonork_SimpleEncrypt(buffer,buffer_size,SimpleCode());
			return SONORK_RESULT_OK;
		default:
			return SONORK_RESULT_INVALID_OPERATION;
	}
}
SONORK_RESULT
 TSonorkCryptContext::SingleBufferUncrypt(void *buffer,UINT buffer_size) const
{
	switch(CryptEngine())
	{
		case SONORK_CRYPT_ENGINE_NONE:
			return SONORK_RESULT_OK;

		case SONORK_CRYPT_ENGINE_SIMPLE:
			Sonork_SimpleUncrypt(buffer,buffer_size,SimpleCode());
		return SONORK_RESULT_OK;

		default:
			return SONORK_RESULT_INVALID_OPERATION;
	}
}
void
 TSonorkCryptContext::SetSimple(const TSonorkPin& pin)
{
	SetSimple(
    	(	( (pin.v[0]>> 8) * (pin.v[1]>>24) )
    	    ^
	        ( (pin.v[0]>>24) * (pin.v[1]>> 8) )
        )
        &0xffffffff);
}

void TSonorkCryptContext::SetSimple(DWORD salt)
{
	SetCryptInfo(NULL);
    engine=SONORK_CRYPT_ENGINE_SIMPLE;
    data=(void*)salt;
    data_size=0;

}

const char *TSonorkCryptContext::CryptName() const
{
	switch(CryptEngine())
	{
	
	case SONORK_CRYPT_ENGINE_NONE:
		return "None";
	case SONORK_CRYPT_ENGINE_SIMPLE:
		return "Simple";
	case SONORK_CRYPT_ENGINE_RIJNDAEL:
		return "Rijndael";
	default:
		return "??";

	}
}
void TSonorkCryptContext::Assign(TSonorkCryptContext&O)
{
	engine=O.engine;
    data=O.data;
    data_size=O.data_size;
    O.engine=SONORK_CRYPT_ENGINE_NONE;
    O.data=NULL;
	O.data_size=0;
}
void
 TSonorkCryptContext::GetCryptInfo(TSonorkCryptInfo*CI)
{
	CI->header.engine=(WORD)engine;
	CI->header.flags=0;
	switch(engine)
	{
		case SONORK_CRYPT_ENGINE_SIMPLE:
			CI->header.param = SimpleCode();
			CI->data.Clear();
		break;

		default:
			CI->header.param=0;
			CI->data.Clear();
		break;
	}

}
bool TSonorkCryptContext::SetCryptInfo(TSonorkCryptInfo*CI)
{
	if(data)
	{
		// First release out current data
		switch(engine)
		{
			case SONORK_CRYPT_ENGINE_NONE:
			case SONORK_CRYPT_ENGINE_SIMPLE:
			case SONORK_CRYPT_ENGINE_RIJNDAEL:
			default:
				// Nothing to be deleted
				break;
		}
		data=NULL;
	}
	if(CI)
	{
		engine=CI->Engine();
		switch(engine)
		{
			case SONORK_CRYPT_ENGINE_SIMPLE:
				data=(void*)CI->header.param;
				data_size=0;
				break;

			case SONORK_CRYPT_ENGINE_NONE:
				break;
			default:
				engine=SONORK_CRYPT_ENGINE_NONE;
				return false;
		}
	}
	else
	{
		engine=SONORK_CRYPT_ENGINE_NONE;
	}
	return true;
}

/*#if defined(__CONSOLE__)
void Dump(const char*title,void *pPtr,UINT sz)
{
	char *ptr=(char*)pPtr;
    char hex[80],ch[20];
	char *t,c;
	UINT col;
#if defined(SONORK_CLIENT_BUILD)
	SONORK_Trace(SONORK_TRACE_EVENT_DEBUG,"%s (%u)",title,sz);

#else
	printf("%s (%u)\n",title,sz);
#endif
    if(sz>16*MAX_DUMP_LINES)sz=16*MAX_DUMP_LINES;
    if(ptr)
    for(UINT i=0;i<sz;)
    {
    	t=hex;
		col=0;
    	while(col<16&&i<sz)
		{
			c=*ptr;
			if(c<' '||c>126)c='.';
			ch[col]=c;
            t+=sprintf(t,"%02.02x ",*ptr&0xff);
            ptr++;
            col++;
			i++;
        }
        ch[col]=0;
#if defined(SONORK_CLIENT_BUILD)
	SONORK_Trace(SONORK_TRACE_EVENT_DEBUG,"%-48.48s %s",hex,ch);
#else
	printf("%-48.48s %s\n",hex,ch);
#endif

    }
}
#else
void Dump(const char*,void *,UINT ){}
#endif
*/
