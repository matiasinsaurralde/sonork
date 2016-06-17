#include "srk_defs.h"
#pragma hdrstop
#include "srk_codec_file.h"
#include "srk_crypt.h"
#define SONORK_CODEC_FILE_VERSION	1

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




TSonorkCodecFileStream::TSonorkCodecFileStream()
{
	handle	= SONORK_INVALID_FILE_HANDLE;
	io_mode	= SONORK_CODEC_FS_IO_NONE;
}
TSonorkCodecFileStream::~TSonorkCodecFileStream()
{
	Close();
}
SONORK_RESULT	TSonorkCodecFileStream::Open(SONORK_C_CSTR path,bool reset, int*err_code)
{
	Close();
	handle = SONORK_IO_OpenFile(path
		,reset
			?SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS
			:SONORK_OPEN_FILE_METHOD_OPEN_ALWAYS
		,err_code);
	return IsOpen()?SONORK_RESULT_OK:SONORK_RESULT_STORAGE_ERROR;
}

SONORK_RESULT	TSonorkCodecFileStream::BeginRead(const TSonorkId&owner,int*p_err_code)
{
	SONORK_RESULT	result;
	int			err_code;
	if(io_mode != SONORK_CODEC_FS_IO_NONE || !IsOpen())
		return SONORK_RESULT_INVALID_OPERATION;
	if((err_code = SONORK_IO_SetFilePointer(handle,0))==0)
	if((err_code = SONORK_IO_ReadFile(handle,&header,sizeof(header)))==0)
	{
		if(header.signature!=SONORK_FILE_SIGNATURE )
			result=SONORK_RESULT_CODEC_ERROR;
		else
		if(header.version >SONORK_CODEC_FILE_VERSION)
			result=SONORK_RESULT_INVALID_VERSION;
		else
		if(header.owner != owner )
			result=SONORK_RESULT_ACCESS_DENIED;
		else
		{
			index   = 0;
			io_mode = SONORK_CODEC_FS_IO_READ;
			result  = SONORK_RESULT_OK;
		}
	}
	if(p_err_code)*p_err_code=err_code;
	return result;

}
SONORK_RESULT	TSonorkCodecFileStream::ReadNext(TSonorkCodecAtom*A,int*p_err_code)
{
	DWORD codec_size;
	if(io_mode != SONORK_CODEC_FS_IO_READ)
		return SONORK_RESULT_INVALID_OPERATION;
	else
	if( index >= header.items )
		return SONORK_RESULT_NO_DATA;
	else
		return
			CODEC_ReadFileStreamDW(handle
				, A
				, codec_size
				, p_err_code
				, header.crypt_code);

}
SONORK_RESULT	TSonorkCodecFileStream::EndRead()
{
	if(io_mode != SONORK_CODEC_FS_IO_READ)
		return SONORK_RESULT_INVALID_OPERATION;
	io_mode = SONORK_CODEC_FS_IO_NONE;
	return SONORK_RESULT_OK;
}

SONORK_RESULT	TSonorkCodecFileStream::BeginWrite(const TSonorkId& owner, DWORD crypt_code,int*p_err_code)
{
	SONORK_RESULT	result;
	if(io_mode != SONORK_CODEC_FS_IO_NONE || !IsOpen())
		return SONORK_RESULT_INVALID_OPERATION;

	header.signature 	= SONORK_FILE_SIGNATURE;
	header.crypt_code   	= crypt_code;
	header.version		= SONORK_CODEC_FILE_VERSION;
	header.items		= 0;
	header.owner.Set( owner );
	SONORK_ZeroMem(&header.reserved,sizeof(header.reserved));
	if( (result = WriteHeader(p_err_code)) == SONORK_RESULT_OK )
	{
		index   = 0;
		io_mode = SONORK_CODEC_FS_IO_WRITE;
		result  = SONORK_RESULT_OK;
	}
	return result;
}

SONORK_RESULT	TSonorkCodecFileStream::WriteNext(const TSonorkCodecAtom*A,int*p_err_code)
{
	DWORD	codec_size;
	SONORK_RESULT	result;
	if(io_mode != SONORK_CODEC_FS_IO_WRITE)
		return SONORK_RESULT_INVALID_OPERATION;
	else
	{

		if((result=CODEC_WriteFileStreamDW(handle
				, A
				, codec_size
				, p_err_code
				, header.crypt_code)) == SONORK_RESULT_OK)
		{
			index++;
		}
		return result;
	}
}
SONORK_RESULT	TSonorkCodecFileStream::EndWrite(int*p_err_code)
{
	if(io_mode != SONORK_CODEC_FS_IO_WRITE)
		return SONORK_RESULT_INVALID_OPERATION;
	io_mode	= SONORK_CODEC_FS_IO_NONE;
	header.items		= index;
	return WriteHeader(p_err_code);
}
SONORK_RESULT TSonorkCodecFileStream::WriteHeader(int *p_err_code)
{
	int err_code;
	SONORK_RESULT result;
	for(;;)
	{
		if((err_code = SONORK_IO_SetFilePointer(handle,0))==0)
			if((err_code = SONORK_IO_WriteFile(handle,&header,sizeof(header))) == 0)
			{
				result=SONORK_RESULT_OK;
				break;
			}
		result=SONORK_RESULT_STORAGE_ERROR;
		if(p_err_code)*p_err_code=err_code;
		break;
	}
	return result;
}


void TSonorkCodecFileStream::Close()
{
	if( IsOpen() )
	{
		SONORK_IO_CloseFile(handle);
		handle	= SONORK_INVALID_FILE_HANDLE;
		io_mode	= SONORK_CODEC_FS_IO_NONE;
	}
}




SONORK_RESULT 	CODEC_WriteFileStreamDW(SONORK_FILE_HANDLE handle, const TSonorkCodecAtom*A,DWORD& codec_size,int *p_err_code,DWORD crypt_code)
{
	int 		err_code;
	SONORK_RESULT	result;
	BYTE*		buffer;
	codec_size = A->CODEC_Size();
	if( (err_code = SONORK_IO_WriteFile(handle,&codec_size,sizeof(DWORD))) == 0 )
	{
		buffer 			= SONORK_MEM_ALLOC(BYTE,codec_size);
		result			= A->CODEC_WriteMem(buffer,codec_size);
		if(result==SONORK_RESULT_OK)
		{
			if(crypt_code)Sonork_SimpleEncrypt(buffer,codec_size,crypt_code);
			if((err_code=SONORK_IO_WriteFile(handle,buffer,codec_size))!=0)
			{
				result=SONORK_RESULT_STORAGE_ERROR;
			}
		}
		SONORK_MEM_FREE( buffer );
	}
	else
	{
		result=SONORK_RESULT_STORAGE_ERROR;
	}
	if(p_err_code)*p_err_code=err_code;
	return result;

}
SONORK_RESULT 	CODEC_ReadFileStreamDW(SONORK_FILE_HANDLE handle,TSonorkCodecAtom*A , DWORD& codec_size , int *p_err_code , DWORD crypt_code)
{
	SONORK_RESULT 	result;
	BYTE  		*buffer;
	int 		err_code;

	codec_size=0;
	if( (err_code = SONORK_IO_ReadFile(handle,&codec_size,sizeof(DWORD))) == 0 )
	{
		if( codec_size < sizeof(DWORD))
			result = SONORK_RESULT_CODEC_ERROR;
		else
		{
			buffer=SONORK_MEM_ALLOC(BYTE,codec_size);
			if( (err_code=SONORK_IO_ReadFile(handle,buffer,codec_size)) == 0 )
			{
				if(crypt_code)Sonork_SimpleUncrypt(buffer,codec_size,crypt_code);
				result=A->CODEC_ReadMem(buffer,codec_size);
			}
			else
			{
				result=SONORK_RESULT_STORAGE_ERROR;
			}
			SONORK_MEM_FREE(buffer);
		}
	}
	else
		result=SONORK_RESULT_STORAGE_ERROR;
	if(p_err_code)*p_err_code=err_code;
	return result;
}

SONORK_RESULT
CODEC_WriteFileStream(
		SONORK_FILE_HANDLE 		handle
	, 	const TSonorkCodecAtom*	A
	,	DWORD&				codec_size
	,	int *				p_err_code
	,	DWORD 				crypt_code)
{
	SONORK_RESULT 		result;
	BYTE*			buffer;
	int 			err_code;

	codec_size		= A->CODEC_Size();
	buffer 			= SONORK_MEM_ALLOC(BYTE,codec_size);
	result			= A->CODEC_WriteMem(buffer,codec_size);
	if(result==SONORK_RESULT_OK)
	{
		if(crypt_code)Sonork_SimpleEncrypt(buffer,codec_size,crypt_code);

		if((err_code=SONORK_IO_WriteFile(handle,buffer,codec_size))!=0)
		{
			result=SONORK_RESULT_STORAGE_ERROR;
			if(p_err_code)*p_err_code=err_code;
		}
	}
	SONORK_MEM_FREE( buffer );
	return result;
}

SONORK_RESULT 	CODEC_ReadFileStream(
		SONORK_FILE_HANDLE 	handle
	,	TSonorkCodecAtom*	A
	,	DWORD			codec_size
	,	int*			p_err_code
	,	DWORD 			crypt_code)
{
	SONORK_RESULT 	result;
	BYTE  		*buffer;
	int 		err_code;

	if(codec_size<sizeof(DWORD))
		result=SONORK_RESULT_CODEC_ERROR;
	else
	{
		buffer=SONORK_MEM_ALLOC(BYTE,codec_size);
		if((err_code=SONORK_IO_ReadFile(handle,buffer,codec_size))==0)
		{
			if(crypt_code)Sonork_SimpleUncrypt(buffer,codec_size,crypt_code);
			result=A->CODEC_ReadMem(buffer,codec_size);
		}
		else
		{
			if(p_err_code)*p_err_code=err_code;
			result=SONORK_RESULT_STORAGE_ERROR;
		}
		SONORK_MEM_FREE(buffer);
	}
	return result;
}

