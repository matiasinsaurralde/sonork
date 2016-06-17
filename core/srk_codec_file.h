#if !defined(SONORK_CODEC_FILE_H)
#define SONORK_CODEC_FILE_H

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


#if SONORK_CODEC_LEVEL<=5
#	error SONORK_CODEC_LEVEL must be > 5
#endif

#include "srk_codec_io.h"
#include "srk_file_io.h"

struct TSonorkCodecFileHeader
{
	DWORD	signature;
	DWORD	version;
	DWORD	items;
	DWORD	crypt_code;
	TSonorkId	owner;
	DWORD	reserved[4];
}__SONORK_PACKED;

class TSonorkCodecFileStream
{
private:
enum SONORK_CODEC_FS_IO_MODE
{
	SONORK_CODEC_FS_IO_NONE
,	SONORK_CODEC_FS_IO_READ
,	SONORK_CODEC_FS_IO_WRITE
};
	TSonorkCodecFileHeader  	header;
	SONORK_FILE_HANDLE		handle;
	DWORD				index;
	SONORK_CODEC_FS_IO_MODE		io_mode;

	SONORK_RESULT	WriteHeader(int*);
public:
	TSonorkCodecFileStream();
	~TSonorkCodecFileStream();
	SONORK_RESULT	Open(SONORK_C_CSTR path,bool reset, int*err_code=NULL);
	void		Close();

	BOOL		IsOpen()	const { return handle!=SONORK_INVALID_FILE_HANDLE;}
	DWORD		Items() 	const { return header.items; }
	DWORD		Index()		const { return index;}
	DWORD		Version()   	const { return header.version;}
	DWORD		CryptCode()	const { return header.crypt_code;}

	SONORK_RESULT	BeginWrite(const TSonorkId&,DWORD crypt_code,int*err_code=NULL);
	SONORK_RESULT	WriteNext(const TSonorkCodecAtom*,int*err_code=NULL);
	SONORK_RESULT	EndWrite(int*err_code=NULL);

	SONORK_RESULT	BeginRead(const TSonorkId&,int*err_code=NULL);
	SONORK_RESULT	ReadNext(TSonorkCodecAtom*,int*err_code=NULL);
	SONORK_RESULT	EndRead();

};
SONORK_RESULT 	CODEC_WriteFile	(SONORK_C_CSTR file_name, const TSonorkCodecAtom*,DWORD& encode_size,int *p_err_code,DWORD crypt_code=0);
SONORK_RESULT 	CODEC_ReadFile	(SONORK_C_CSTR file_name,TSonorkCodecAtom*,int *p_err_code,DWORD crypt_code=0);
SONORK_RESULT 	CODEC_WriteFileStream(SONORK_FILE_HANDLE handle, const TSonorkCodecAtom*,DWORD &encode_size,int *p_err_code,DWORD crypt_code=0);
SONORK_RESULT 	CODEC_ReadFileStream(SONORK_FILE_HANDLE handle,TSonorkCodecAtom*,DWORD encode_size,int *p_err_code,DWORD crypt_code=0);
SONORK_RESULT 	CODEC_WriteFileStreamDW(SONORK_FILE_HANDLE handle, const TSonorkCodecAtom*,DWORD& encode_size,int *p_err_code,DWORD crypt_code=0);
SONORK_RESULT 	CODEC_ReadFileStreamDW(SONORK_FILE_HANDLE handle,TSonorkCodecAtom*,DWORD& encode_size,int *p_err_code,DWORD crypt_code=0);

#endif