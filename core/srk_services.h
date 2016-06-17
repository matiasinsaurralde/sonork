#if !defined(SRK_SERVICES_H)
#define SRK_SERVICES_H
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




#define SONORK_IPC_PACKET_F_ERROR		0x10000000
#define SONORK_IPC_PACKET_F_INVALID		0x20000000
#define SONORK_IPC_PACKET_F_RAW			0x40000000

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,4)
#endif

struct TSrkIpcPacketHeader
{
	DWORD		ipc_function;
	DWORD		ipc_version;		// Set to zero
	DWORD		ipc_flags;

	BOOL	IsError()	const {	return ipc_flags&SONORK_IPC_PACKET_F_ERROR;}
	BYTE*	DataPtr(){	return ((BYTE*)this)+sizeof(*this);	}
}__SONORK_PACKED;


struct TSrkIpcPacket
{
	TSrkIpcPacketHeader	header;

	UINT	Function()	const { return header.ipc_function;}
	UINT	Version()	const { return header.ipc_version&0xff;}
	BOOL	IsError()	const {	return header.ipc_flags&SONORK_IPC_PACKET_F_ERROR;}

	BYTE*	DataPtr(){ return ((BYTE*)&header)+sizeof(header);	}

// D_size is the size of the data size (as passed to GuAllocIpcPacket)
// P_size is the size of the while packet, as received by the tcp engine
	DWORD	E_Req(DWORD	D_size, UINT function, const TSonorkCodecAtom*);
	bool	D_Req(DWORD	P_size, TSonorkCodecAtom*);
	DWORD	E_Akn(DWORD	D_size, UINT function, const TSonorkCodecAtom*);
	bool    D_Akn(DWORD	P_size, TSonorkCodecAtom*);	// should NOT call if IsError() is set
	DWORD	E_Error(DWORD D_size, UINT function, const TSonorkError&);
	bool	D_Error(DWORD P_size, TSonorkError&);	// should call this if IsError() is set

}__SONORK_PACKED;



// ======================================================
// SRK_SERVICE_AUTHS

enum SRK_SERVICE_AUTHS_ATOM_TYPES
{
  SONORK_AUTHS_ATOM_BASE	=SONORK_SERVICE_ATOMS_BASE
, SONORK_AUTHS_ATOM_REQ		=SONORK_AUTHS_ATOM_BASE
, SONORK_AUTHS_ATOM_ADV
, SONORK_AUTHS_ATOM_AKN
, SONORK_AUTHS_ATOM_ADD
, SONORK_AUTHS_ATOM_DEL
, SONORK_AUTHS_ATOM_LIMIT	=SONORK_AUTHS_ATOM_DEL
};
struct TSonorkAuthsAtom1
:public TSonorkCodecAtom
{
public:
	TSonorkAuth2		auth;

// ----------------
// CODEC

public:

	void	CODEC_Clear()
			{auth.Clear();}

	DWORD	CODEC_DataSize()	const
			{	return sizeof(DWORD) + sizeof(auth);	}

private:

	void    CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void    CODEC_ReadDataMem	(TSonorkCodecReader&);
};


struct TSonorkAuthsAtom2
:public TSonorkCodecAtom
{

public:
	TSonorkAuth2			auth;
	TSonorkUserData			user_data;

// ----------------
// CODEC

public:

	void 	CODEC_Clear();

private:
	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;
	void	CODEC_ReadDataMem(TSonorkCodecReader&);
	DWORD	CODEC_DataSize() const;
};

struct TSonorkAuthsReq
:public TSonorkAuthsAtom1
{

// ----------------
// CODEC

public:

	SONORK_ATOM_TYPE
		CODEC_DataType() const {return (SONORK_ATOM_TYPE)SONORK_AUTHS_ATOM_REQ;}
};


struct TSonorkAuthsAkn
:public TSonorkAuthsAtom1
{
// ----------------
// CODEC

public:

	SONORK_ATOM_TYPE
		CODEC_DataType() const {return (SONORK_ATOM_TYPE)SONORK_AUTHS_ATOM_AKN;}
};


struct TSonorkAuthsDel
:public TSonorkAuthsAtom1
{

// ----------------
// CODEC

public:

	SONORK_ATOM_TYPE
		CODEC_DataType() const {return (SONORK_ATOM_TYPE)SONORK_AUTHS_ATOM_DEL;}
};
struct TSonorkAuthsAdd
:public TSonorkAuthsAtom2
{

// ----------------
// CODEC

public:

	SONORK_ATOM_TYPE
		CODEC_DataType() const {return (SONORK_ATOM_TYPE)SONORK_AUTHS_ATOM_ADD;}
};

struct TSonorkAuthsAdv
:public TSonorkAuthsAtom2
{

// ----------------
// CODEC

public:

	SONORK_ATOM_TYPE
		CODEC_DataType() const {return (SONORK_ATOM_TYPE)SONORK_AUTHS_ATOM_ADV;}
};


// ======================================================
//	SRK_SERVICE_DATA_SERVER

#define SONORK_DATA_SERVER_NEW_PROTOCOL_VERSION	MAKE_VERSION_NUMBER(1,5,0,5)
#define SONORK_MAIN_SERVER_NEW_PROTOCOL_VERSION	MAKE_VERSION_NUMBER(1,5,0,5)

enum SRK_SERVICE_DATA_SERVER_ATOM_TYPES
{
  SRK_DATA_SERVER_ATOM_LOGIN_REQ   =SONORK_SERVICE_ATOMS_BASE
, SRK_DATA_SERVER_ATOM_FILE_LOCUS
, SRK_DATA_SERVER_ATOM_FILE_INFO
, SRK_DATA_SERVER_ATOM_PUT_FILE_OLD
, SRK_DATA_SERVER_ATOM_GET_FILE_OLD
, SRK_DATA_SERVER_ATOM_PUT_FILE_NEW
, SRK_DATA_SERVER_ATOM_GET_FILE_NEW
, SRK_DATA_SERVER_ATOM_TARGET_LIST
, SRK_DATA_SERVER_ATOM_FILE_HEADER
, SRK_DATA_SERVER_ATOM_PUT_FILE_TARGET_RESULT
};
enum SONORK_DATA_SERVER_FUNCTION
{
  SONORK_DATA_SERVER_FUNCTION_NONE
, SONORK_DATA_SERVER_FUNCTION_LOGIN_REQ
, SONORK_DATA_SERVER_FUNCTION_PUT_FILE_OLD
, SONORK_DATA_SERVER_FUNCTION_GET_FILE_OLD
, SONORK_DATA_SERVER_FUNCTION_PUT_FILE_NEW
, SONORK_DATA_SERVER_FUNCTION_GET_FILE_NEW
, SONORK_DATA_SERVER_FUNCTION_PUT_FILE_TARGET_RESULT
, SONORK_DATA_SERVER_FUNCTION_CLOSE_FILE
, SONORK_DATA_SERVER_FUNCTION_KEEP_ALIVE
};


struct TSonorkDataServerLoginReq
:public TSonorkCodecAtom
{
	struct HEADER{
		TSonorkUserLocus1	locus;
		SONORK_DWORD4		pin;
		DWORD			pin_type;
		DWORD			server_no;
		DWORD			service_type;
		DWORD			login_flags;
		DWORD			requested_version;
		DWORD			reserved[10];
	};
	HEADER	header;

	// ----------------
// CODEC

public:
	DWORD
		ServerNo() const
		{ return header.server_no; }

	SONORK_SERVICE_TYPE
		ServiceType() const
		{ return (SONORK_SERVICE_TYPE)header.service_type;}

const	SONORK_DWORD4&
		Pin()	const
		{ return header.pin; }

	SONORK_PIN_TYPE
		PinType() const
		{ return (SONORK_PIN_TYPE)header.pin_type; }

	void
		CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const
		{ return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_LOGIN_REQ;}

private:


	DWORD
		CODEC_DataSize()	const;

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

};

struct TSonorkFileLocus
{
	TSonorkId	      creator;
	DWORD		      serverNo;
	DWORD		      _zero;		// unused, set to zero
	TSonorkObjId	      fileId;
	DWORD		      attrFlags;
	DWORD		      reserved[7];

	bool	IsSameId( const TSonorkFileLocus& O ) const
		{ return creator==O.creator && fileId == O.fileId; }
	void
		Set(const TSonorkFileLocus& O)
		{ memcpy(this,&O,sizeof(*this)); }

	void
		Clear()
		{ SONORK_ZeroMem(this,sizeof(*this));}
}__SONORK_PACKED;

enum SONORK_FILE_INFO_COMPRESS_FLAGS
{
   SONORK_FILE_INFO_F_COMPRESS_METHOD	=	0x00ff0000
,  SONORK_FILE_INFO_F_COMPRESS_TYPE  	=	0x0000ff00
,  SONORK_FILE_INFO_F_COMPRESS_LEVEL	= 	0x000000ff
};

enum SONORK_FILE_INFO_COMPRESS_METHOD
{
  SONORK_FILE_INFO_COMPRESS_METHOD_NONE	=	0x00000000
, SONORK_FILE_INFO_COMPRESS_METHOD_ZIP	=	0x00010000
};

enum SONORK_FILE_INFO_COMPRESS_TYPE
{
  SONORK_FILE_INFO_COMPRESS_TYPE_NONE	=	0x00000000
};


struct TSonorkFileInfo
:public TSonorkCodecAtom
{
public:
	struct _ATTR{
		SONORK_DWORD2		orig_size;	// Original file size
		SONORK_DWORD2		stor_size;	// Stored or compress size
		DWORD			version_seq;
		DWORD			compress_flags;
		TSonorkTime   		modify_time;
		DWORD			reserved[16];
	}__SONORK_PACKED;

	TSonorkFileLocus  	locus;
	_ATTR			attr;
	TSonorkShortString	name;
	TSonorkDynData		data;

	void	Set(const TSonorkFileInfo&);

// ----------------
// CODEC

public:
	void	Clear();

	void	CODEC_Clear()
		{ Clear(); }

	SONORK_ATOM_TYPE
			CODEC_DataType() 	const
			{ return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_FILE_INFO;}

private:

	DWORD	CODEC_DataSize()	const;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);


};

struct TSonorkDataServerOldPutFileReq
:public TSonorkCodecAtom
{

public:
	struct _HEADER
	{
		TSonorkId	target;
		DWORD		code;
		DWORD		reserved[13];
	} __SONORK_PACKED;

	_HEADER		header;
	TSonorkFileInfo	file_info;

// ----------------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const
		{ return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_PUT_FILE_OLD;}

private:

	DWORD
		CODEC_DataSize()	const;

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

}__SONORK_PACKED;

enum SONORK_DATA_SERVER_PUT_FILE_FLAGS
{
 SONORK_DATA_SERVER_PUT_FILE_F_NO_SEND_MSG =	0x00008000
};

struct TSonorkDataServerNewPutFileReq
:public TSonorkCodecAtom
{

public:
	struct HEADER
	{
		DWORD		flags;
		SONORK_DWORD2	trackingNo;
		DWORD		reserved[5];
	} __SONORK_PACKED;

	HEADER				header;
	TSonorkFileInfo			file_info;
	SONORK_DWORD2List		target_list;
	TSonorkSimpleDataListAtom       target_atom;
	TSonorkCryptInfo		crypt_info;

// ----------------
// CODEC

public:
	TSonorkDataServerNewPutFileReq();

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const
		{ return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_PUT_FILE_NEW;}

private:

	DWORD	CODEC_DataSize()	const;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);

};

struct TSonorkDataServerPutFileTargetResult
:public TSonorkCodecAtom
{

public:
	struct HEADER
	{
		TSonorkId	sonork_id;
		TSonorkObjId	msg_id;
		DWORD		reserved[4];
	} __SONORK_PACKED;

	HEADER				header;
	TSonorkError			ERR;

// ----------------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const
		{ return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_PUT_FILE_TARGET_RESULT;}

private:

	DWORD	CODEC_DataSize()	const;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);

};
enum SONORK_DATA_SERVER_GET_FILE_FLAGS
{
 SONORK_DATA_SERVER_GET_FILE_F_DELETE	=	0x00008000
};

struct TSonorkDataServerOldGetFileReq
:public TSonorkCodecAtom
{

public:
	struct _HEADER
	{
		DWORD				flags;
		DWORD				code;
		DWORD				reserved[14];
	} __SONORK_PACKED;

	_HEADER			header;
	TSonorkFileLocus	locus;


// ----------------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const { return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_GET_FILE_OLD;}

private:

	DWORD	CODEC_DataSize()	const;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);
};

struct TSonorkDataServerNewGetFileReq
:public TSonorkCodecAtom
{

public:
	struct _HEADER
	{
		DWORD		flags;
		DWORD		reserved[3];
	} __SONORK_PACKED;

	_HEADER			header;
	TSonorkFileLocus	locus;
	TSonorkCryptInfo	crypt_info;


// ----------------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType() 	const { return (SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_GET_FILE_NEW;}

private:

	DWORD	CODEC_DataSize()	const;

	void	CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void	CODEC_ReadDataMem(TSonorkCodecReader&);
};


#define SONORK_AllocIpcPacket(data_sz)	SONORK_MEM_ALLOC(TSrkIpcPacket,sizeof(TSrkIpcPacket)+data_sz)
#define SONORK_FreeIpcPacket(P)			SONORK_MEM_FREE(P);

#if defined(USE_PRAGMA_PUSH)
#pragma	pack( pop )
#endif

#endif
