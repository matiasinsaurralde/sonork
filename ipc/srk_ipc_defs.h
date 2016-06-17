#if !defined(SRK_IPC_DEFS_H)
#define SRK_IPC_DEFS_H
#if !defined(SRK_DEFS_H)
# include "srk_defs.h"
#endif

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL) Version 1.

	You should have received a copy of the SSCL along with this program;
	if not, write to sscl@sonork.com.

	You should NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL), doing so will indicate your agreement
	to the the terms which may be differ for each version of the software.

	This comment section, indicating the existence and requirement of
	acceptance of the SSCL may not be removed from the source code.
*/

#define SONORK_IPC_PARAMETERS_DWORDS	64
#define SONORK_IPC_PARAMETERS_BYTES	(SONORK_IPC_PARAMETERS_DWORDS*sizeof(DWORD))
enum SONORK_IPC_FUNCTION
{
  SONORK_IPC_FUNCTION_NONE
, SONORK_IPC_FUNCTION_PUT_REQ_DATA		= 1	// SERVER -> CLIENT
, SONORK_IPC_FUNCTION_PUT_EVT_DATA		= 2
, SONORK_IPC_FUNCTION_GET_REQ_DATA		= 3	// SERVER <- CLIENT
, SONORK_IPC_FUNCTION_GET_EVT_DATA		= 4
, SONORK_IPC_FUNCTION_APP_EVENT			= 10
, SONORK_IPC_FUNCTION_SERVICE_EVENT		= 12
, SONORK_IPC_FUNCTION_LINK			= 100
, SONORK_IPC_FUNCTION_UNLINK			= 101
, SONORK_IPC_FUNCTION_SET_EVENT_MASK		= 200
, SONORK_IPC_FUNCTION_GET_EVENT_DATA		= 202
, SONORK_IPC_FUNCTION_ALLOC_SHARED_SLOT		= 250
, SONORK_IPC_FUNCTION_FREE_SHARED_SLOT		= 252
, SONORK_IPC_FUNCTION_PREPARE_MSG		= 300
, SONORK_IPC_FUNCTION_SEND_MSG			= 302
, SONORK_IPC_FUNCTION_SEND_FILES		= 304
, SONORK_IPC_FUNCTION_RECV_FILE			= 306
, SONORK_IPC_FUNCTION_ENUM_USER_GROUPS		= 400
, SONORK_IPC_FUNCTION_ENUM_USER_LIST		= 402
, SONORK_IPC_FUNCTION_GET_USER_DATA		= 410
, SONORK_IPC_FUNCTION_GET_USER_HANDLE		= 412
, SONORK_IPC_FUNCTION_REGISTER_SERVICE		= 500
, SONORK_IPC_FUNCTION_UNREGISTER_SERVICE	= 502
, SONORK_IPC_FUNCTION_EXPORT_SERVICE		= 504
, SONORK_IPC_FUNCTION_SET_SERVER_DATA		= 506
, SONORK_IPC_FUNCTION_POKE_SERVICE_DATA		= 510
, SONORK_IPC_FUNCTION_START_SERVICE_QUERY	= 520
, SONORK_IPC_FUNCTION_REPLY_SERVICE_QUERY	= 522
, SONORK_IPC_FUNCTION_ACCEPT_SERVICE_CIRCUIT 	= 530
, SONORK_IPC_FUNCTION_GET_SERVICE_CIRCUIT	= 532
, SONORK_IPC_FUNCTION_OPEN_SERVICE_CIRCUIT	= 534
, SONORK_IPC_FUNCTION_CLOSE_SERVICE_CIRCUIT	= 536
};

enum SONORK_IPC_ALLOC_METHOD
{
  SONORK_IPC_ALLOC_OPEN_EXISTING	= 0	// Must previously exist
, SONORK_IPC_ALLOC_CREATE_NEW		= 1	// Must create, fails if exists
, SONORK_IPC_ALLOC_OPEN_ALWAYS		= 2	// If exists: open, if not: create
};

enum SONORK_IPC_SHARED_SLOT_INDEX
{
  SONORK_IPC_SHARED_SLOT_01		= 0
, SONORK_IPC_SHARED_SLOT_02		= 1
, SONORK_IPC_SHARED_SLOT_03		= 2
, SONORK_IPC_SHARED_SLOT_04		= 3
, SONORK_IPC_SHARED_SLOT_INDEXES
};


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif

union TSonorkIpcLinkData
{
	struct {
		DWORD		clientHwnd;
		DWORD		clientMsg;
		DWORD		eventMask;
	}req;

	struct {
		DWORD		ipcId;
		DWORD		ipcFlags;
	}akn;

}__SONORK_PACKED;


struct TSonorkIpcFileHeader
{
	DWORD		headerFullSize;
	DWORD		headerDataSize;
	DWORD		sharedSlotsOffset;
	DWORD		sharedSlotsSize;
	DWORD		blockOffset;
	DWORD		blockDataSize;
	DWORD		blockFullSize;
	DWORD		maxDataSize;
	DWORD		serverHwnd;
	DWORD		serverMsg;
	DWORD		eventSerial;
	TSonorkId	userId;
	char		alias[SONORK_USER_ALIAS_MAX_SIZE];
	DWORD		appStatus;
	DWORD		appVersionNumber;
	TSonorkIpcLinkData
			link;       // See remarks below
}__SONORK_PACKED;

// <link> is used only during Linking
// The client must lock the mutex to use it

struct TSonorkIpcFileBlockHeader
{
	DWORD	dataSize;
	DWORD	reserved[3];
	union
	{
		BYTE		b[SONORK_IPC_PARAMETERS_BYTES];
		DWORD		v[SONORK_IPC_PARAMETERS_DWORDS];
		SONORK_DWORD2	d[SONORK_IPC_PARAMETERS_DWORDS/2];
	}params;
}__SONORK_PACKED;

struct TSonorkIpcFileBlock
:public TSonorkIpcFileBlockHeader
{
	BYTE		data[4];	// Dummy
}__SONORK_PACKED;

struct TSonorkIpcSharedSlot
{
	DWORD		slotId;
	DWORD		slotParam;
	DWORD		refCount;
	DWORD		reserved;
	DWORD		data[SONORK_IPC_SHARED_SLOT_INDEXES];
}__SONORK_PACKED;
#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif

#endif
