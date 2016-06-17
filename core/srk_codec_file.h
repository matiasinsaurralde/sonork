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


#include "srk_codec_io.h"
#include "srk_file_io.h"

















































SONORK_RESULT 	CODEC_WriteFileStream(SONORK_FILE_HANDLE handle, const TSonorkCodecAtom*,DWORD &encode_size,int *p_err_code,DWORD crypt_code=0);

SONORK_RESULT 	CODEC_WriteFileStreamDW(SONORK_FILE_HANDLE handle, const TSonorkCodecAtom*,DWORD& encode_size,int *p_err_code,DWORD crypt_code=0);


