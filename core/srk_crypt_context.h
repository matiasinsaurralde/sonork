#if !defined(SONORK_CRYPT_CONTEXT_H)
#define SONORK_CRYPT_CONTEXT_H

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

union SONORK_CRYPT_DATA
{
	void *raw;
};


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

struct TSonorkCryptInfo
:public TSonorkCodecAtom
{
private:
	struct HEADER{
		DWORD		    engine;
		DWORD		    flags;
		DWORD		    param;
		DWORD		    reserved[13];
	}__SONORK_PACKED;


public:
	HEADER			header;
	TSonorkDynData		data;

	SONORK_CRYPT_ENGINE
		Engine() const
		{return (SONORK_CRYPT_ENGINE)header.engine;}

	DWORD
		Flags()	const
		{return header.engine;}

	DWORD
		Param()	const
		{return header.param;}

	void Clear();

// -------
// CODEC

public:
	void 	CODEC_Clear()
			{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType() const
			{   return SONORK_ATOM_CRYPT_INFO_1; }

private:

	DWORD		CODEC_DataSize() const;
	
	void		CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void		CODEC_ReadDataMem	(TSonorkCodecReader&);


}__SONORK_PACKED;

#if defined(USE_PRAGMA_PUSH)
#pragma	pack( pop )
#endif

struct TSonorkCryptContext
{
private:
	SONORK_CRYPT_ENGINE 	engine;
	void			*data;
	UINT 			data_size;
public:
	TSonorkCryptContext()
		{engine=SONORK_CRYPT_ENGINE_NONE;}

	~TSonorkCryptContext()
		{Clear();}
	DWORD
		SimpleCode() const
		{ return (DWORD)data;}

	SONORK_CRYPT_ENGINE
		CryptEngine()	const
		{ return engine;}

	void
		*CryptData()
		{ return data;}

	UINT
		CryptDataSize()	const
		{ return data_size;}

	bool
		Enabled() const
		{ return CryptEngine()!=SONORK_CRYPT_ENGINE_NONE;}
    // Loads/Unloads a Context from/to a CRYPT_INFO structure
	bool
		SetCryptInfo(TSonorkCryptInfo*);

	void
		GetCryptInfo(TSonorkCryptInfo*);

	void	Clear()
		{SetCryptInfo(NULL);}


    // Initializes a simple encryption context
	void
		SetSimple(DWORD salt);

	void
		SetSimple(const TSonorkPin& pin);

	const char*
		CryptName() const;

	UINT
		MaxEncryptedSize(UINT input_buffer_size) const;
	UINT
		MaxUncryptedSize(UINT input_buffer_size) const;


    // Routines for pre-allocated buffers,
    // output_buffer_size is input/output
    //   it should be loaded with the output buffer size
    //   and will return with the data size
	SONORK_RESULT
		DoubleBufferEncrypt(
			const void *input_buffer
			, UINT input_buffer_size
			, void *output_buffer
			, UINT*output_buffer_size ) const;
	SONORK_RESULT
		DoubleBufferUncrypt(
			const void *input_buffer
			, UINT input_buffer_size
			, void *output_buffer
			, UINT*output_buffer_size) const;

    // IsSingleBuffer() indicates that the engine may encrypt/uncrypt
    // using a single read/write buffer and that the input size is always
    // the same as the output size.
    // Test before calling SingleBufferEncrypt() or SingleBufferUncrypt()
	bool
		SingleBufferEncryptionEnabled() const
	{
		return CryptEngine()<=SONORK_CRYPT_ENGINE_SINGLE_BUFFERED;
	}

    // Routines that encrypt/uncrypt on the same buffer, should ONLY be called
    // when IsSingleBuffer() is true

	SONORK_RESULT
		SingleBufferEncrypt(void *buffer, UINT buffer_size) const;

	SONORK_RESULT
		SingleBufferUncrypt(void *buffer, UINT buffer_size) const;


    // Routines that will encrypt using a single if possible
    // On return:
    //  If Single Buffering encryption is possible
    //    output_buffer will be the same as input_buffer
    //    output_buffer_size will be the same as input_buffer_size
    //    and new_output_buffer will be set to FALSE
    // If Double buffering encryption was used,
    //    output_buffer will point to the new [encrypted/decrypted] buffer
    //    output_buffer_size will hold the size of the new buffer
    //    and new_output_buffer will be set to TRUE
    // NOTE: if Double buffering encryption is used (new_output_buffer is TRUE)
    //    and output_buffer is not null, the caller should
    //    free this new buffer using SONORK_MEM_FREE(),even when
    //    the result is not SONORK_RESULT_OK

	SONORK_RESULT
		Encrypt(void*input_buffer
			,UINT input_buffer_size
			,void**output_buffer
			,UINT*output_buffer_size
			,BOOL*new_output_buffer) const;

	SONORK_RESULT
		Uncrypt(void*input_buffer
			,UINT input_buffer_size
			,void**output_buffer
			,UINT*output_buffer_size
			,BOOL*new_output_buffer) const;


	SONORK_RESULT
		Encrypt(TSonorkDynData*) const;

	SONORK_RESULT
		Uncrypt(TSonorkDynData*) const;
		
	// Assign will load the data of another [source] context and clear the source context.
	void
		Assign(TSonorkCryptContext&);


};


#endif