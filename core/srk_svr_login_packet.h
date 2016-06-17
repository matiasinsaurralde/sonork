#if !defined(SONORK_SVR_LOGIN_PACKET_H)
#define SONORK_SVR_LOGIN_PACKET_H
#include "srk_data_types.h"
#include "srk_crypt_context.h"

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





#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

struct TSonorkSvrLoginReqPacket
:public TSonorkCodecAtom
{
public:


private:

	struct _HEADER
	{
		TSonorkId		userId;
		TSonorkRegion		region;
		DWORD 	 		loginFlags;
		DWORD			signature;
		// <old_version> is no longer used for V1.5
		// but it must still be loaded with
		// SONORK_CLIENT_VERSION
		DWORD			oldVersion;

		// <old_os_info> should be set to 0 for V1.5
		DWORD			old_os_info;	// RESERVED for V1.05
		DWORD       		extData;
		TSonorkVersion 	version;
		DWORD			reserved[12];
	}__SONORK_PACKED;

public:
	_HEADER     		header;
	TSonorkDynData		password;
	TSonorkCryptInfo	cryptInfo;
	TSonorkDynData		extData;

	void
		Clear();

	void
		SetLoginFlags(DWORD v)
		{header.loginFlags=v;}

	void
		SetLoginMode(UINT v)
		{
			header.loginFlags=
				(header.loginFlags&~SONORK_LOGIN_RF_MODE_MASK)
				|(v&SONORK_LOGIN_RF_MODE_MASK);
		}

	DWORD
		LoginMode()	const
		{return (header.loginFlags & SONORK_LOGIN_RF_MODE_MASK);}

	DWORD
		LoginFlags()	const
		{ return header.loginFlags; }

	TSonorkVersion&
		Version()
		{ return header.version;}

	void
		Prepare2( const 	TSonorkUserData*
			,SONORK_C_CSTR	pass
			,DWORD 	loginFlags
			,const TSonorkVersion& version
			);

	bool
		Prepared() const
		{ return header.version.VersionNumber()!=0; }
// ------------
// CODEC

public:

	void CODEC_Clear()
	{
		Clear();
	}

	SONORK_ATOM_TYPE
		CODEC_DataType() const
	{
		return SONORK_ATOM_SERVER_LOGIN_REQ_1;
	}

private:

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;

}  __SONORK_PACKED;

struct TSonorkSvrSidInfo
{
	TSonorkOldUserAddr	addr;
	TSonorkPin		sidPin;
	TSonorkSerial		userSerial;
}__SONORK_PACKED;

struct TSonorkSvrLoginAknPacket
:public TSonorkCodecAtom
{

	struct _HEADER
	{
		DWORD			result;
		TSonorkId		userId;
		DWORD			signature;
		DWORD			loginFlags_obsolete;
		DWORD			serverVersion;
		DWORD    		maxAknMsecs;
		DWORD			maxPckMsecs;
		DWORD			maxIdleSecs;
		DWORD			maxTaskSecs;
		DWORD			loginAknFlags;
		DWORD			loginReqFlags;// Accepted flags
		DWORD			reserved[16];
	}__SONORK_PACKED;

public:
	_HEADER			header;
	TSonorkDynData		sid_info;
	TSonorkCryptInfo	crypt_info;
	TSonorkDynData		ext_data;

	void 		Clear();

	SONORK_RESULT	Result() const
				{ return (SONORK_RESULT)header.result;	}

// ------------
// CODEC

public:

	void CODEC_Clear()
		{ Clear();  }

	SONORK_ATOM_TYPE
		CODEC_DataType() const
		{ return SONORK_ATOM_SERVER_LOGIN_AKN_1; }

private:

	DWORD	CODEC_DataSize() const;
	void	CODEC_WriteDataMem	(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem	(TSonorkCodecReader&);

}  __SONORK_PACKED;

struct TSonorkSvrLogoutReqPacket
{
	DWORD		reserved[32];
};


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif



#endif
