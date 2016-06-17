#if !defined(SRKFILETXENG_H)
#define SRKFILETXENG_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL).

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	SSCL for more details.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).
*/



#include "srk_services.h"

enum SONORK_FILE_TX_FLAGS
{
  SONORK_FILE_TX_F_TEMPORAL		=0x00001
, SONORK_FILE_TX_F_DELETE_FILE		=0x00002
, SONORK_FILE_TX_F_SEND_COMPRESS	=0x00004
, SONORK_FILE_TX_F_FREE_CALLBACK_PARAM	=0x01000
, SONORK_FILE_TX_F_NO_PROGRESS_REPORT	=0x00010
, SONORK_FILE_TX_F_INVISIBLE		=0x00020
, SONORK_FILE_TX_FM_ACTION		=0xf0000
, SONORK_FILE_TX_FV_OPEN_FOLDER		=0x10000
, SONORK_FILE_TX_FV_OPEN_FILE		=0x20000
, SONORK_FILE_TX_FM_IPC_ALLOWED		=(SONORK_FILE_TX_F_DELETE_FILE|SONORK_FILE_TX_F_NO_PROGRESS_REPORT|SONORK_FILE_TX_F_INVISIBLE)
};

enum SONORK_FILE_TX_EVENT
{
  SONORK_FILE_TX_EVENT_NONE
, SONORK_FILE_TX_EVENT_PHASE
, SONORK_FILE_TX_EVENT_PROGRESS
, SONORK_FILE_TX_EVENT_TARGET_RESULT
, SONORK_FILE_TX_EVENT_ERROR
};

enum SONORK_FILE_TX_MODE
{
  SONORK_FILE_TX_MODE_RECV
, SONORK_FILE_TX_MODE_SEND
, SONORK_FILE_TX_MODE_DELETE
};

enum SONORK_FILE_TX_PHASE
{
  SONORK_FILE_TX_PHASE_NONE	=	0
, SONORK_FILE_TX_PHASE_RESOLVING
, SONORK_FILE_TX_PHASE_CONNECTING
, SONORK_FILE_TX_PHASE_COMPRESSING
, SONORK_FILE_TX_PHASE_AUTHORIZING
, SONORK_FILE_TX_PHASE_REQUESTING
, SONORK_FILE_TX_PHASE_SENDING
, SONORK_FILE_TX_PHASE_RECEIVING
, SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM
, SONORK_FILE_TX_PHASE_EXPANDING
, SONORK_FILE_TX_PHASE_WAIT_RECV_CONFIRM
, SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE
, SONORK_FILE_TX_PHASE_FINISHED
, SONORK_FILE_TX_PHASE_HALTED_BY_ERROR

};

enum SONORK_FILE_TX_ERROR
{
  SONORK_FILE_TX_ERROR_UNKNOWN
, SONORK_FILE_TX_ERROR_RESOLVE
, SONORK_FILE_TX_ERROR_AUTHORIZE
, SONORK_FILE_TX_ERROR_REQUEST
, SONORK_FILE_TX_ERROR_FILE_OPEN
, SONORK_FILE_TX_ERROR_FILE_IO
, SONORK_FILE_TX_ERROR_NET_IO
, SONORK_FILE_TX_ERROR_CONFIRM
, SONORK_FILE_TX_ERROR_TERMINATED
};


struct TSonorkFileTxEngEvent
{
	struct _PHASE
	{
		SONORK_FILE_TX_PHASE	phase;
	};
	struct _PHASE_SIZE
	:public _PHASE
	{
		DWORD	offset;
		DWORD	size;
	};
	struct _PHASE_REQ_SEND
	:public _PHASE_SIZE
	{
		union{
			TSonorkId*		user;	// old version
			SONORK_DWORD2List*	list;	// new versions
		}target;
	};

	struct _ERROR
	:public _PHASE
	{
		SONORK_FILE_TX_ERROR	tx_err;
		TSonorkError*   	pERR;
	};

	struct _PROGRESS
	:public _PHASE
	{
		DWORD	offset;
	};


	SONORK_FILE_TX_EVENT		event;

	union {
		_PHASE		phase;
		_PHASE_SIZE	phase_size;
		_PHASE_REQ_SEND phase_req_send;
		_PHASE_SIZE	phase_req_recv;
		_PHASE_SIZE	phase_compress;
		_PHASE_SIZE	phase_expand;
		_ERROR		error;
		_PROGRESS	progress;
		TSonorkDataServerPutFileTargetResult*
				target_result;
	};
};


#if defined(SONORK_APP_BUILD)

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

typedef void (SONORK_CALLBACK SonorkFileTxEngHandler)(void *param,struct TSonorkFileTxEngEvent*);
typedef 	SonorkFileTxEngHandler* pSonorkFileTxEngHandler;

class TSonorkFileTxEng
:public TSonorkTaskWin
{
private:
	enum PROTOCOL_VERSION
	{
	  PROTOCOL_OLD
	, PROTOCOL_NEW
	};

	TSonorkPacketTcpEngine		tcp;
	TSonorkServiceData * 		data_server;
	TSonorkError	  		taskERR;
	SONORK_FILE_TX_PHASE		tx_phase;
	SONORK_FILE_TX_MODE		tx_mode;
	DWORD		   		tx_flags;
	UINT		   		yield_count;
	PROTOCOL_VERSION		protocol;

	struct CB{
		pSonorkFileTxEngHandler	ptr;
		void*			dat;
	}cb;

	struct FILE
	{
		TSonorkFileInfo		info;
		UINT			offset;
		UINT			keep_alive;
		SONORK_FILE_HANDLE	handle;
		TSonorkShortString	real_path;
		TSonorkShortString	work_path;
		TSonorkCryptContext	crypt;
		TSonorkZipStream*	ZH;
		DWORD			compress_flags;
	}file;

	struct SND
	{
		TSonorkShortStringQueue*	queue;
		SONORK_DWORD2			tracking_no;
	}send;

	void	OnBeforeDestroy();

	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);

	void 	OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&);
	void 	OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);
	void 	OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&,const TSonorkError*);

	void	OnTimer(UINT);
	void	ProcessPacket(TSonorkTcpPacket*);
	void		TxPhase_Authorizing_Packet(TSrkIpcPacket*,UINT P_size);
	void		TxPhase_Requesting_Packet(TSrkIpcPacket*,UINT P_size);
	void		TxPhase_WaitSendConfirm_Packet(TSrkIpcPacket*,UINT P_size);
	void		TxPhase_WaitRecvConfirm_Packet(TSrkIpcPacket*);

	void	SetPhase(SONORK_FILE_TX_PHASE
		, TSonorkFileTxEngEvent*
		, SONORK_C_CSTR);
	void	SetError(SONORK_FILE_TX_ERROR,TSonorkError*);
	void	SetCodecError(SONORK_FILE_TX_ERROR , TSonorkError&,UINT line);
	void	SetIOError(SONORK_FILE_TX_ERROR , SONORK_SYS_STRING,int);
	void	ReportProgress(DWORD);

	void	StartResolution();
	void	ClearResolution();

	void	SendLoginRequest();
	bool	OpenTxFile(SONORK_C_CSTR
			, BOOL          write_mode
			, DWORD*        size
			, TSonorkTime*	mod_time
			, SONORK_C_CSTR	pCaller);
	void	CloseTxFile(SONORK_C_CSTR);

	void	GetLocalPath();
	void	SendGetFileRequest();
	void		SendGetFileRequest_Old();
	void		SendGetFileRequest_New();
	void	RecvFileData(BYTE*,UINT);
	void	SendKeepAlive();
	void	SendRecvComplete(SONORK_C_CSTR);

	void	DoExpandFile();
	BOOL	InitExpandFile();

	void	DoCompressFile();
	BOOL	InitCompressFile();

	void	SendPutFileRequest();
	void		SendPutFileRequest_Old(TSonorkFileTxEngEvent&);
	void		SendPutFileRequest_New(TSonorkFileTxEngEvent&);
	void	DoSendNextFile();
	void	SendFileData();



	void	ClearYield();
	bool	ShouldYield();
	void	Resume();

	void	InvokeEventHandler(TSonorkFileTxEngEvent*);

public:
	TSonorkFileTxEng(TSonorkWin*parent
		, TSonorkShortStringQueue*
		, pSonorkFileTxEngHandler
		, void*param
		, DWORD tx_flags		// See SONORK_FILE_TX_FLAGS
		);

	TSonorkFileTxEng(TSonorkWin*parent
		, const TSonorkFileInfo& 	file_info
		, const SONORK_C_CSTR		download_path
		, pSonorkFileTxEngHandler
		, void*		param
		, DWORD 	tx_flags	// See SONORK_FILE_TX_FLAGS
		);

	~TSonorkFileTxEng();

	void
		Terminate(TSonorkError*ERR=NULL);

	void
		Continue()	const;

	bool
		IsNewProtocol() const
		{ return protocol==PROTOCOL_NEW;}

	SONORK_FILE_TX_PHASE
		TxPhase()	const
		{ return tx_phase; }

	SONORK_FILE_TX_MODE
		TxMode()	const
		{ return tx_mode;}

	UINT
		TxFlags()	const
		{ return tx_flags;}


const	TSonorkShortString&
		FilePath() const
		{ return file.real_path; }

	bool
		FileIsRemote() const
		{ return tx_mode !=SONORK_FILE_TX_MODE_SEND;}

const	TSonorkFileInfo&
		FileInfo() const
		{ return file.info; }

const	TSonorkServiceData*
		DataServer() const
		{	return data_server;}

	void*
		CallbackParam()
		{	return cb.dat;}

const	TSonorkShortStringQueue*
		SendQueue() const
		{	return send.queue;}

const	SONORK_DWORD2&
		SendTrackingNo() const 
		{	return send.tracking_no;}

static SONORK_C_CSTR
		GetUncompressedSize(const TSonorkFileInfo&,TSonorkShortString&);
};
#endif

#endif