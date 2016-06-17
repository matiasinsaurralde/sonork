#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkfiletxeng.h"
#include "srk_crypt.h"
#include "srk_file_io.h"
#include "srk_zip.h"

#define	POKE_DATASERVER_RESOLUTION_RESULT	SONORK_WIN_POKE_01
#define POKE_CONTINUE				SONORK_WIN_POKE_02

#define SONORK_FILE_TX_COMPRESSION_LEVEL	9

#define SONORK_FILE_TX_COMPRESSION_FLAGS \
		(SONORK_FILE_INFO_COMPRESS_METHOD_ZIP \
		|SONORK_FILE_INFO_COMPRESS_TYPE_NONE  \
		|SONORK_FILE_TX_COMPRESSION_LEVEL)

#define SONORK_FILE_TX_COMPRESSION_TYPE(n)	\
		((n)&(SONORK_FILE_INFO_F_COMPRESS_METHOD|SONORK_FILE_INFO_F_COMPRESS_TYPE))


#define DONT_REPORT_EVENT	((TSonorkFileTxEngEvent*)NULL)
#define	REPORT_PHASE_EVENT	((TSonorkFileTxEngEvent*)1)

#define MULT_TIMER_MSECS			100
#define TX_FILE_YIELD_COUNT			16
#define MAX_PENDING_PACKETS			8

static SONORK_C_CSTR
 GetPhaseName(SONORK_FILE_TX_PHASE phase);


// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::OnBeforeDestroy()
{

	Terminate(NULL);
	CloseTxFile("OnBeforeDestroy");
	SetPhase(SONORK_FILE_TX_PHASE_NONE
			,DONT_REPORT_EVENT
			,"OnBeforeDestroy"
			);
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::Terminate(TSonorkError* pERR)
{
	SONORK_FILE_TX_PHASE	end_phase;
	if( file.ZH )
	{
		SonorkApp.ZipEngine()->Close(file.ZH);
		file.ZH=NULL;
	}
	if(tx_phase >= SONORK_FILE_TX_PHASE_NONE
	&& tx_phase < SONORK_FILE_TX_PHASE_FINISHED)
	{
		end_phase = tx_phase;
		if( pERR!=NULL )
			taskERR.Set(*pERR);
		else
			taskERR.SetSys(SONORK_RESULT_FORCED_TERMINATION
				,GSS_USRCANCEL
				,SONORK_MODULE_LINE);
		SetError( SONORK_FILE_TX_ERROR_TERMINATED , &taskERR );
		
		if(end_phase >=	SONORK_FILE_TX_PHASE_COMPRESSING
		&& end_phase <=  SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM
		&& tx_mode == SONORK_FILE_TX_MODE_SEND
		&& file.compress_flags != 0)
		{
			//TRACE_DEBUG("DELETE: %s",file.work_path.CStr());
			DeleteFile(file.work_path.CStr());
		}

	}
	cb.ptr = NULL;
	Destroy(IDOK);
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::InvokeEventHandler(TSonorkFileTxEngEvent* E)
{
	if(cb.ptr != NULL)
	{
		cb.ptr(cb.dat,E);
	}
}
// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendKeepAlive()
{
	TSrkIpcPacket*P;
	UINT P_size;
	if(++file.keep_alive>40)
	{
		P=SONORK_AllocIpcPacket( 0 );
		P_size = P->E_Req(0,SONORK_DATA_SERVER_FUNCTION_KEEP_ALIVE,NULL);
		tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
		SONORK_FreeIpcPacket( P );
		file.keep_alive=0;
	}
}

void
  TSonorkFileTxEng::SendRecvComplete(SONORK_C_CSTR pCaller)
{
	TSrkIpcPacket*P;
	UINT P_size;
	SetPhase(SONORK_FILE_TX_PHASE_WAIT_RECV_CONFIRM
		,REPORT_PHASE_EVENT
		,pCaller);
	P=SONORK_AllocIpcPacket( 0 );
	P_size = P->E_Req(0,SONORK_DATA_SERVER_FUNCTION_CLOSE_FILE,NULL);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeIpcPacket( P );
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::RecvFileData(BYTE*i_data,UINT i_bytes)
{
	int code;
	void*o_data;
	UINT o_bytes;
	BOOL must_free_o_data;
	TSonorkFileTxEngEvent	E;
	file.crypt.Uncrypt(i_data
			,i_bytes
			,&o_data
			,&o_bytes
			,&must_free_o_data);

	if( (code=SONORK_IO_WriteFile(file.handle,o_data,o_bytes)) != 0 )
	{
		SetIOError( SONORK_FILE_TX_ERROR_FILE_IO , GSS_FILEWRERR , code);
	}
	else
	{
		file.offset+=o_bytes;
		ReportProgress(file.offset);

		if(file.offset>=file.info.attr.stor_size.v[0])
		{
			if( IsNewProtocol() )
			{
				if( file.compress_flags!=0 )
				{
					E.phase_expand.offset=0;
					E.phase_expand.size  =file.info.attr.orig_size.v[0];
					SetPhase(SONORK_FILE_TX_PHASE_EXPANDING
						, &E	// Yes, report event to callback
						, "RecvFileData(compress)"
						);
				}
				else
				{
					SendRecvComplete("RecvFileData(NEW)");
				}
			}
			else
			{
				SetPhase(SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE
				, REPORT_PHASE_EVENT
				, "RecvFileData(OLD)");
			}
		}
	}
	if( must_free_o_data )
		SONORK_MEM_FREE( o_data );
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendFileData()
{
	UINT code;
	void*o_data;
	UINT i_bytes,o_bytes;
	BOOL must_free_o_data;

	if(tcp.PendingPackets() >= MAX_PENDING_PACKETS)
		return;
//	assert( send.buffer != NULL );

	while(file.offset<file.info.attr.stor_size.v[0]
		&& tcp.PendingPackets() < MAX_PENDING_PACKETS)
	{
		i_bytes=tcp.BufferSize();//SEND_BUFFER_SIZE;
		if(file.offset+i_bytes>file.info.attr.stor_size.v[0])
			i_bytes=file.info.attr.stor_size.v[0]-file.offset;
		code = SONORK_IO_ReadFile(file.handle
			,tcp.Buffer()
			,i_bytes);
		if( code != 0)
		{
			SetIOError( SONORK_FILE_TX_ERROR_FILE_IO
				, GSS_FILERDERR
				, code);
			return;
		}
		file.offset+=i_bytes;
		file.crypt.Encrypt(tcp.Buffer()
			,i_bytes
			,&o_data
			,&o_bytes
			,&must_free_o_data);

		tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_RAW_DATA
			,o_data
			,o_bytes);
		if(must_free_o_data)
			SONORK_MEM_FREE( o_data );
		if( ShouldYield() )
		{
			// Don't use all the time, let other windows work,
			// we'll restart when OnTimer() is invoked..
			break;
		}
	}
	ReportProgress(file.offset);

	if(file.offset>=file.info.attr.stor_size.v[0])
	{
		SetPhase( SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM
			, REPORT_PHASE_EVENT
			, "SendFileData"
			);
	}

}

// ----------------------------------------------------------------------------

bool
 TSonorkFileTxEng::OpenTxFile( SONORK_C_CSTR path
		, BOOL 		write_mode
		, DWORD*        p_size
		, TSonorkTime* 	p_time
		, SONORK_C_CSTR	pCaller)
{
	FILETIME gmt_time,local_time;
	DWORD	file_size;
	//TRACE_DEBUG("OpenTxFile << %s",pCaller);
	//TRACE_DEBUG("           : '%s'",path);
	CloseTxFile("OpenTxFile 1");
	if(tx_mode== SONORK_FILE_TX_MODE_DELETE)
		return false;

	file.handle = CreateFile(path
				,write_mode?GENERIC_WRITE:GENERIC_READ
				,write_mode?0:FILE_SHARE_READ
				,NULL
				,write_mode?CREATE_ALWAYS:OPEN_EXISTING
				,FILE_ATTRIBUTE_NORMAL
				,INVALID_HANDLE_VALUE);
	if( file.handle != SONORK_INVALID_FILE_HANDLE
		&& write_mode == false
		&& (p_size != NULL || p_time!=NULL) )
	{
		if(SONORK_IO_GetFileSize(file.handle,&file_size) != 0)
			CloseTxFile("OpenTxFile 2");
		else
		{
			if( p_size != NULL)
			{
				*p_size = file_size;
			}
			if( p_time != NULL )
			{
				GetFileTime(file.handle,NULL,NULL,&gmt_time);
				FileTimeToLocalFileTime(&gmt_time,&local_time);
				p_time->SetTime( &local_time );
			}
			/*
			file.info.attr.phys_size.v[0]=file_size;
			file.info.attr.phys_size.v[1]=0;
			file.info.attr.file_size.Set(file.info.attr.phys_size);
			file.info.attr.modify_time.SetTime( &local_time );
			*/

		}
	}
	if( file.handle == SONORK_INVALID_FILE_HANDLE )
	{
		TSonorkTempBuffer tmp(SONORK_MAX_PATH+128);
		SonorkApp.LangSprintf(tmp.CStr()
			, GLS_MS_NOOPENFILE
			, path);
		taskERR.Set(SONORK_RESULT_STORAGE_ERROR
			,tmp.CStr()
			,GetLastError()
			,true);
		SetError(SONORK_FILE_TX_ERROR_FILE_OPEN
			,&taskERR);
		return false;
	}
	return true;
}


// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::CloseTxFile(SONORK_C_CSTR pCaller)
{
	if( file.handle != SONORK_INVALID_FILE_HANDLE )
	{
		//TRACE_DEBUG("--CloseTxFile() << %s",pCaller);
		SONORK_IO_CloseFile(file.handle);
		file.handle=SONORK_INVALID_FILE_HANDLE;
		// FIX THIS!!
		/*
		if( (tx_flags & SONORK_FILE_TX_F_DELETE_FILE)
		&&   tx_mode == SONORK_FILE_TX_MODE_SEND)
		{
			DeleteFile( file.open_path.CStr() );
		}
		*/
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::ReportProgress(DWORD offset)
{
	TSonorkFileTxEngEvent E;
	if(tx_flags&SONORK_FILE_TX_F_NO_PROGRESS_REPORT)
		return;

	E.event = SONORK_FILE_TX_EVENT_PROGRESS;
	E.progress.phase  = tx_phase;
	E.progress.offset = offset;
	InvokeEventHandler( &E );
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SetCodecError(
	  SONORK_FILE_TX_ERROR 	tx_err
	, TSonorkError& 	ERR
	, UINT 			line)
{
	ERR.SetSys(SONORK_RESULT_CODEC_ERROR,GSS_BADCODEC,line);
	SetError(tx_err,&ERR);
}

// ----------------------------------------------------------------------------
// Set Error:
//  Something happened: Set the phase to ERROR
//  and let the callback know about it.
void
 TSonorkFileTxEng::SetError( SONORK_FILE_TX_ERROR tx_err, TSonorkError*pERR )
{
	TSonorkFileTxEngEvent	E;

	// Report the phase in which the error ocurred.
	// Save it before invoked SetPhase() which will change our current phase
	E.event 	= SONORK_FILE_TX_EVENT_ERROR;
	E.error.phase 	= tx_phase;
	E.error.tx_err	= tx_err;
	E.error.pERR  	= pERR;

	// Don't report event to callback: The EVENT_ERROR
	// has extra parameters. (HALTER_BY_ERROR is never
	// reported to the callback, only EVENT_ERROR)

	SetPhase( SONORK_FILE_TX_PHASE_HALTED_BY_ERROR
		, DONT_REPORT_EVENT	// No, don't report event to callback
		, "SetError"
		);

	// Report the event to the callback
	InvokeEventHandler( &E );

}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SetIOError(SONORK_FILE_TX_ERROR tx_err, SONORK_SYS_STRING gss,int code)
{
	taskERR.SetSys(SONORK_RESULT_STORAGE_ERROR,gss,code);
	SetError(tx_err,&taskERR);
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&)
{
	SetPhase(SONORK_FILE_TX_PHASE_RESOLVING
		,REPORT_PHASE_EVENT
		,"OnTaskStart");
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if(P->Function() == SONORK_FUNCTION_LIST_SERVICES)
	if(P->SubFunction() == 0)
	{
		TSonorkServiceData *SI;
		TSonorkServiceDataQueue queue;
		if(P->D_ListServices_A(P_size,queue))
			data_server=queue.RemoveFirst();

		// Delete any remainig items (we only asked for one, but just in case)
		while((SI=queue.RemoveFirst())!=NULL)
			SONORK_MEM_DELETE(SI);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&, const TSonorkError*pERR)
{
	taskERR.Set(*pERR);
	PostPoke(POKE_DATASERVER_RESOLUTION_RESULT , 0 );
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SetPhase(SONORK_FILE_TX_PHASE new_phase
	, TSonorkFileTxEngEvent	*pE
	, SONORK_C_CSTR pCaller)
{
	TSonorkFileTxEngEvent	tmpE;

	//TRACE_DEBUG("TSonorkFileTxEng::SetPhase(%s->%s) << %s"
	//	,GetPhaseName(tx_phase)
	//	,GetPhaseName(new_phase)
	//	,pCaller);
	if( tx_phase == new_phase )
	{
		TRACE_DEBUG("TSonorkFileTxEng:: **** SAME PHASE ****");
		return;
	}

	tx_phase=new_phase;
	if( tx_phase == SONORK_FILE_TX_PHASE_CONNECTING )
	{
		SetAuxTimer( tx_mode==SONORK_FILE_TX_MODE_DELETE
			?MULT_TIMER_MSECS*2
			:MULT_TIMER_MSECS);
	}
	else
	if( tx_phase == SONORK_FILE_TX_PHASE_REQUESTING
	||  tx_phase == SONORK_FILE_TX_PHASE_COMPRESSING
	||  tx_phase == SONORK_FILE_TX_PHASE_EXPANDING)
	{
		file.offset=0;
		file.keep_alive=0;
	}
	else
	if( tx_phase >= SONORK_FILE_TX_PHASE_FINISHED )
	{
		KillAuxTimer();
		if(tcp.Status()>SONORK_NETIO_STATUS_DISCONNECTED)
			tcp.Shutdown();
	}

	if(  tx_phase == SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM
	||   tx_phase >= SONORK_FILE_TX_PHASE_EXPANDING)
	{
		char tmp[128];
		wsprintf(tmp,"SetPhase << %s",pCaller);
		CloseTxFile(tmp);
		if( tx_phase == SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM
		&& file.compress_flags!= 0)
		{
//			TRACE_DEBUG("DELETE: '%s'",file.work_path.CStr());
			DeleteFile(file.work_path.CStr());
			

		}
	}

	if( pE != DONT_REPORT_EVENT )
	{
		if( pE == REPORT_PHASE_EVENT )
			pE=&tmpE;
		pE->event = SONORK_FILE_TX_EVENT_PHASE;
		pE->phase.phase = tx_phase;
		InvokeEventHandler( pE );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::Continue() const
{
//	TRACE_DEBUG("TSonorkFileTxEng::Continue");
	PostPoke(POKE_CONTINUE,0);
}

bool
 IsCompressed(const char *extension_with_dot)
{
	const char *compressed_exension[]={"zip","rar","lzh","gzip","arj",NULL};
	if(extension_with_dot != NULL )
	if(*extension_with_dot++=='.')
	for(int i=0;compressed_exension[i]!=NULL;i++)
		if(!stricmp(extension_with_dot ,compressed_exension[i]))
			return true;
	return false;
}
// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::DoSendNextFile()
{
	TSonorkShortString *S;
	TSonorkFileTxEngEvent	E;

	assert( send.queue != NULL );
	S=send.queue->RemoveFirst();
	if( !S )
	{
		SetPhase( SONORK_FILE_TX_PHASE_FINISHED
		, REPORT_PHASE_EVENT
		, "DoSendNextFile");
	}
	else
	{
		file.real_path.Set(S->CStr());
		SONORK_MEM_DELETE(S);
		if(!OpenTxFile(file.real_path.CStr()
			, false		// read mode
			, &file.info.attr.orig_size.v[0]
			, &file.info.attr.modify_time
			, "DoSendNextFile(real)"
			 ))
			return;
		file.info.attr.orig_size.v[1]=0;



		// Load file name
		file.info.name.Set(
			SONORK_IO_GetFileName( file.real_path.Buffer() )
		);

		file.compress_flags = 0;
		if( tx_flags & SONORK_FILE_TX_F_SEND_COMPRESS )
		if( IsNewProtocol() )
		{
			// Don't zip already zipped files
			if( !IsCompressed(SONORK_IO_GetFileExt(file.info.name.CStr())) )
			{
				file.compress_flags=SONORK_FILE_TX_COMPRESSION_FLAGS;
			}
		}

		if( file.compress_flags != 0 )
		{
			CloseTxFile("DoSendNextFile(Compress)");
			// Don't know STORED size yet
			file.info.attr.stor_size.Clear();

			// Work path = Temp Path
			SonorkApp.GetTempPath(file.work_path
				, "filetx"
				, ".zip"
				, (DWORD)this);
			E.phase_compress.offset = 0;
			E.phase_compress.size   = file.info.attr.orig_size.v[0];
			SetPhase(SONORK_FILE_TX_PHASE_COMPRESSING
				,&E
				,"DoSendNextFile(compress)");
		}
		else
		{
			// Not compressed: stor_size=orig_size
			file.info.attr.stor_size.Set(file.info.attr.orig_size);
			// Work path = Real path
			file.work_path.Set( file.real_path );
			SendPutFileRequest();
		}
	}

}
// ----------------------------------------------------------------------------
// SendPutFileRequest()
//  We set the phase to REQUESTING without reporting it to the callback,
//  load the file information and then call SendPutFileRequest_New or
//  SendPutFileRequest_Old depending on the version of the data server.
// Those functions will then report the SONORK_FILE_TX_EVENT_PHASE event loaded
//  by this function into <E> with phase = SONORK_FILE_TX_PHASE_REQUESTING.
// Uppon reception of the event, the callback should load
//  <E.phase_req_send.target.list> if this->IsNewProtocol() is true,
//  or
//  <E.phase_req_send.target.user> if this->IsNewProtocol() is false.

void
 TSonorkFileTxEng::SendPutFileRequest()
{
	TSonorkFileTxEngEvent	E;


	// Set encryption type
	file.crypt.SetSimple( ((DWORD)this+(DWORD)Parent()) | 1 );


	file.info.locus.Clear();	// Don't know until file is aknowledged
	if(tx_flags&SONORK_FILE_TX_F_TEMPORAL)
		file.info.locus.attrFlags |= SONORK_FILE_ATTR_TEMPORARY;
	file.info.attr.compress_flags=file.compress_flags;


	SONORK_ZeroMem(file.info.attr.reserved,sizeof(file.info.attr.reserved));

	// Preload the event data common to both
	// SendPutFileRequest_New and SendPutFileRequest_Old

	E.phase_req_send.offset	     = 0;
	E.phase_req_send.size	     = file.info.attr.stor_size.v[0];

	if( IsNewProtocol() )
		SendPutFileRequest_New(E);
	else
		SendPutFileRequest_Old(E);
}

// ----------------------------------------------------------------------------
// SendPutFileRequest_New()
//  Sends a new put file request to the data server.
//  It asks the callback to load a list of TSonorkId.

void
 TSonorkFileTxEng::SendPutFileRequest_New(TSonorkFileTxEngEvent& E)
{
	TSonorkDataServerNewPutFileReq REQ;
	UINT 			D_size,P_size;
	TSrkIpcPacket	*P;

	// Make sure request is initialized
	REQ.CODEC_Clear();

	// Load the <file_info> pre-loaded by SendPutFileRequest()
	// into the request.
	REQ.file_info.Set(file.info);

	// Load the tracking no
	REQ.header.trackingNo.Set( SendTrackingNo() );

	// The callback should load a list of TSonorkIds
	E.phase_req_send.target.list = &REQ.target_list;
	SetPhase( SONORK_FILE_TX_PHASE_REQUESTING
		, &E
		, "SendPutFileRequest(NEW)"
		);

	// Load the Encryption context.
	file.crypt.GetCryptInfo(&REQ.crypt_info);

	// Add compressed extension if the file is compressed
	if(file.compress_flags != 0)
		REQ.file_info.name.Append(".zip");

//	TRACE_DEBUG("REQ='%s' (ORIG=%u STOR=%u) CF=%x"
//		, REQ.file_info.name.CStr()
//		, REQ.file_info.attr.orig_size.v[0]
//		, REQ.file_info.attr.stor_size.v[0]
//		, file.compress_flags);
	// Create and send the packet
	D_size=REQ.CODEC_Size();
	P=SONORK_AllocIpcPacket( D_size );
	P_size = P->E_Req(D_size,SONORK_DATA_SERVER_FUNCTION_PUT_FILE_NEW,&REQ);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeIpcPacket( P );
}

// ----------------------------------------------------------------------------
// SendPutFileRequest_Old()
//  Sends a old put file request to the data server.
//  It asks the callback to load one TSonorkId.

void
 TSonorkFileTxEng::SendPutFileRequest_Old(TSonorkFileTxEngEvent& E)
{
	TSonorkDataServerOldPutFileReq REQ;
	UINT 		D_size,P_size;
	TSrkIpcPacket	*P;

	// Make sure request is initialized
	REQ.CODEC_Clear();

	// Load the <file_info> pre-loaded by SendPutFileRequest()
	// into the request.
	REQ.file_info.Set(file.info);

	// The callback should load only one TSonorkId
	E.phase_req_send.target.user = &REQ.header.target;
	SetPhase( SONORK_FILE_TX_PHASE_REQUESTING
		, &E
		, "SendPutFileRequest(NEW)"
		);

	// Load the Encryption context.
	// It MUST be <simple> for OLD method.
	assert( file.crypt.CryptEngine() == SONORK_CRYPT_ENGINE_SIMPLE );
	REQ.header.code = file.crypt.SimpleCode();

	// Add compressed extension if the file is compressed
	if(file.compress_flags != 0)
		REQ.file_info.name.Append(".zip");

	// Create and send the packet
	D_size=REQ.CODEC_Size();
	P=SONORK_AllocIpcPacket( D_size );
	P_size = P->E_Req(D_size,SONORK_DATA_SERVER_FUNCTION_PUT_FILE_OLD,&REQ);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeIpcPacket( P );
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendGetFileRequest()
{
	TSonorkFileTxEngEvent	E;

	E.phase_req_recv.offset	= 0;
	E.phase_req_recv.size	= file.info.attr.stor_size.v[0];

	SetPhase( SONORK_FILE_TX_PHASE_REQUESTING
		, &E	// Don't report: We need to send size & offset
		, "SendGetFileRequest");
	file.crypt.SetSimple( ((DWORD)this+(DWORD)Parent()) | 1 );

	if( IsNewProtocol() )
		SendGetFileRequest_New();
	else
		SendGetFileRequest_Old();
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendGetFileRequest_New()
{
	TSonorkDataServerNewGetFileReq REQ;
	UINT D_size,P_size;
	TSrkIpcPacket	*P;


	SONORK_ZeroMem(REQ.header.reserved,sizeof(REQ.header.reserved));
	REQ.header.flags =
		(tx_mode==SONORK_FILE_TX_MODE_DELETE)
			?SONORK_DATA_SERVER_GET_FILE_F_DELETE
			:0;
	REQ.locus.Set( file.info.locus );
	file.crypt.GetCryptInfo(&REQ.crypt_info);

	D_size=REQ.CODEC_Size();
	P=SONORK_AllocIpcPacket( D_size );
	P_size = P->E_Req(D_size,SONORK_DATA_SERVER_FUNCTION_GET_FILE_NEW,&REQ);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeIpcPacket( P );

}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendGetFileRequest_Old()
{
	TSonorkDataServerOldGetFileReq REQ;
	UINT D_size,P_size;
	TSrkIpcPacket	*P;

	assert( file.crypt.CryptEngine() == SONORK_CRYPT_ENGINE_SIMPLE );

	SONORK_ZeroMem(REQ.header.reserved,sizeof(REQ.header.reserved));
	REQ.header.flags =
		(tx_mode==SONORK_FILE_TX_MODE_DELETE)
			?SONORK_DATA_SERVER_GET_FILE_F_DELETE
			:0;
	REQ.locus.Set( file.info.locus );
	REQ.header.code = file.crypt.SimpleCode();

	D_size=REQ.CODEC_Size();
	P=SONORK_AllocIpcPacket( D_size );
	P_size = P->E_Req(D_size,SONORK_DATA_SERVER_FUNCTION_GET_FILE_OLD,&REQ);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET , P, P_size);
	SONORK_FreeIpcPacket( P );

}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::SendLoginRequest()
{
	TSonorkDataServerLoginReq REQ;
	UINT D_size,P_size;
	TSrkIpcPacket	*P;


	SonorkApp.ProfileUser().GetLocus1(&REQ.header.locus);
	REQ.header.pin_type	= SONORK_PIN_TYPE_64;
	SonorkApp.EncodePin64(REQ.header.pin
		, data_server->header.locus.userId
		, SONORK_SERVICE_ID_DATA_SERVER
		, 0);
	REQ.header.server_no 		= data_server->header.ServiceInstance();
	REQ.header.service_type 	= SONORK_SERVICE_TYPE_SONORK_FILE;
	REQ.header.login_flags		= 0;
	SONORK_ZeroMem(REQ.header.reserved,sizeof(REQ.header.reserved));

	D_size = REQ.CODEC_Size()+32;
	P=SONORK_AllocIpcPacket( D_size );
	P_size = P->E_Req(D_size,SONORK_DATA_SERVER_FUNCTION_LOGIN_REQ,&REQ);
	tcp.SendPacket(SONORK_NETIO_HFLAG_TYPE_PACKET,P,P_size);
	SONORK_FreeIpcPacket(P);

	SetPhase( SONORK_FILE_TX_PHASE_AUTHORIZING
		, REPORT_PHASE_EVENT
		, "SendLoginRequest");
}
BOOL
 TSonorkFileTxEng::InitExpandFile()
{
	TSonorkShortString	file_name;
	assert( file.compress_flags != 0);
	GetUncompressedSize(file.info,file_name);
	file.ZH = SonorkApp.ZipEngine()->OpenForInflate(
			  file.work_path.CStr()
			, 4096);
	if(file.ZH == NULL)
		return false;
	return SonorkApp.ZipEngine()->InitInflateFileByName(
		  file.ZH
		, file_name.CStr()	// zip_name
		, file.real_path.CStr()
		);
}
void
 TSonorkFileTxEng::DoExpandFile()
{
	if(file.ZH == NULL )
	{
		if(!InitExpandFile())
			goto expand_failure;
	}

	while( !ShouldYield() )
	{
		if(!SonorkApp.ZipEngine()->DoInflateFile(file.ZH) )
		{
			goto expand_failure;
		}
		if( file.ZH->OperationComplete() )
		{
			ReportProgress( file.ZH->DataOffset() );
//			TRACE_DEBUG("EXPAND_COMPLETE WF=%s", file.work_path.CStr());
			goto expansion_end;
		}
	}
	ReportProgress( file.ZH->DataOffset() );
	SendKeepAlive();
	return;

expand_failure:

//	TRACE_DEBUG("EXPAND_FAILED WF=%s", file.work_path.CStr());
	// Expansion failed: Assume real path
	// is the temporal zipped file
	file.compress_flags = 0;
	file.real_path.Set( file.work_path );

expansion_end:
	if( file.ZH )
	{
		SonorkApp.ZipEngine()->Close(file.ZH);
		file.ZH=NULL;
	}
	if( file.compress_flags != 0 )
	{
		TRACE_DEBUG("DELETE: '%s'"
			,file.work_path.CStr());
		DeleteFile( file.work_path.CStr() );
	}
	SendRecvComplete("DoExpandFile");
}
BOOL
 TSonorkFileTxEng::InitCompressFile()
{
	assert( file.compress_flags != 0);
	file.ZH = SonorkApp.ZipEngine()->OpenForDeflate(
		  file.work_path.CStr()
		, false
		, 4096);
	if(file.ZH == NULL)
		return false;
	return SonorkApp.ZipEngine()->InitDeflateFile(
		  file.ZH
		, file.real_path.CStr()
		, SONORK_FILE_TX_COMPRESSION_LEVEL
		, file.info.name.CStr()	// zip_name
		);
}

void
 TSonorkFileTxEng::DoCompressFile()
{
	if(file.ZH == NULL )
		if(!InitCompressFile())
			goto compress_failure;
	while( !ShouldYield() )
	{
		if(!SonorkApp.ZipEngine()->DoDeflateFile(file.ZH) )
			goto compress_failure;
		if( file.ZH->OperationComplete() )
		{
			ReportProgress( file.ZH->DataOffset() );
//			TRACE_DEBUG("COMPRESS_COMPLETE WF=%s", file.work_path.CStr());
			goto compress_end;
		}
	}
	ReportProgress( file.ZH->DataOffset() );
	SendKeepAlive();

	return;

compress_failure:
//	TRACE_DEBUG("COMPRESS_FAILURE WF=%s", file.work_path.CStr());
	// Compression failed: Assume temporal file
	// is the real file
	file.compress_flags = 0;
	file.work_path.Set( file.real_path );

compress_end:
	if( file.ZH )
	{
		SonorkApp.ZipEngine()->Close(file.ZH);
		file.ZH=NULL;
	}
	if(!OpenTxFile(file.work_path.CStr()
		, false		// read mode
		, &file.info.attr.stor_size.v[0]	// Get PhysSize
		, NULL
		, "DoCompress"
		 ))
		return;
	file.info.attr.stor_size.v[1]=0;
	SendPutFileRequest();

}

// ----------------------------------------------------------------------------
// OnTimer()
// Keeps the tcp engine running while we're sending receiving.
// Note that tcp.Recv() is used for both receiving AND sending data:
//  It must be called even if we're not interested in receiving anything
//  and, when it returns a packet, the caller MUST delete it.
void
 TSonorkFileTxEng::OnTimer(UINT)
{
	TSonorkTcpPacket *P;

	// Check if valid mode.
	if( tx_phase < SONORK_FILE_TX_PHASE_CONNECTING )
	{
		KillAuxTimer();
		return;
	}
	// Notes on the Yielding mechanism.
	// ClearYield() sets <yield_count> to 0
	// we increment <yield_count> everytime we get or send a packet
	// by calling ShouldYield(). When <yield_count> exceeds the
	// allowed value, ShouldYield() will return <true> indicating we
	// should return and wait for the next time OnTimer() is invoked.
	// This yielding mechanism is to avoid "freezing" the application
	// when the underlying network is very fast (for example, in a LAN)
	// because tcp.Recv() will receive packets all the time and
	// the while((P=tcp.Recv(0))!=NULL) will never stop until the
	// whole file is received (which could potentially be of serveral MB)
	// Same mechanism is used in SendFileData() which only breaks
	// out when the socket is blocked: If the network is very fast, the
	// socket will never block and the application would freeze until the
	// whole file is transmitted, hence ShouldYield() is also used there.
	ClearYield();

	// Is there a packet pending? (msecs=0 to poll, don't wait)
	while((P=tcp.Recv(0))!=NULL)
	{
		// Process and & delete the pcket
		ProcessPacket(P);
		Sonork_FreeTcpPacket(P);
		if(ShouldYield())
		{
			// Too much work done, wait for next OnTimer()
			return;
		}
	}
	
	if( tx_phase == SONORK_FILE_TX_PHASE_EXPANDING )
	{
		DoExpandFile();
		return;	// Don't care if we're still connected
	}
	else
	if( tx_phase == SONORK_FILE_TX_PHASE_COMPRESSING )
	{
		DoCompressFile();
		// Don't return: Check if we're still connected
	}

	if( tcp.Status() == SONORK_NETIO_STATUS_CONNECTED )
	{
		// We're connected
		if( tx_phase == SONORK_FILE_TX_PHASE_SENDING )
		{
			SendFileData();
		}
		else
		if( tx_phase == SONORK_FILE_TX_PHASE_CONNECTING )
		{
			// if tx_phase == SONORK_FILE_TX_PHASE_CONNECTING
			// we where waiting for the tcp engine to connect.
			// Ok, send the login request and set our phase
			// to SONORK_FILE_TX_PHASE_AUTHORIZING.
			// The AKN packet will be processed by ProcessPacket()
			// Let the application's service cache know that we did
			// connect to the server, so the entry is still good.
			// (See below: Notes about the application service cache)
			SonorkApp.ReportServiceDataUsageToChache(data_server,true);
			SendLoginRequest();
		}
	}
	else
	if( tcp.Status() == SONORK_NETIO_STATUS_DISCONNECTED )
	{
		// We've disconnected
		if( tx_phase < SONORK_FILE_TX_PHASE_FINISHED )
		{
			// Hey!, we're not finished here.
			taskERR.SetSys(SONORK_RESULT_NETWORK_ERROR
				,GSS_NETCXLOST
				,0);
			SetError(SONORK_FILE_TX_ERROR_NET_IO,&taskERR);
		}
		if( tx_phase <= SONORK_FILE_TX_PHASE_CONNECTING )
		{
			// * Notes about the application service cache *
			// If we could not even connect, let the application
			// Service Cache know that this data server entry is
			// of no good and it should be deleted.
			// The Service Cache holds the list of service servers
			// used lately, so that we don't have to go through the
			// process of requesting the address to the Sonork server
			// and things go faster.
			// But when we could not even connect, we must delete it
			// so that we re-resolve the server.
			// Normally service servers do not change their addresses
			// but it could happen if the service server is restarted.
			SonorkApp.ReportServiceDataUsageToChache(data_server,false);
		}

	}
}

// ----------------------------------------------------------------------------
// ProcessPacket
// We process packets received from the data server here
// invoked by OnTimer() when tcp.Recv() returns a packet
// ProcessPacket() does not own the packet and should not delete it.

void
 TSonorkFileTxEng::ProcessPacket(TSonorkTcpPacket*tP)
{
	TSrkIpcPacket	* iP;
	UINT		iP_size;

	// A TSonorkTcpPacket has 2 attributes: type and size
	// When the SonorkServer sends an TSrkIpcPacket it sets
	// type to SONORK_NETIO_HFLAG_TYPE_PACKET
	// when it is sending raw file data it sets type to
	// SONORK_NETIO_HFLAG_TYPE_RAW_DATA

	// A raw data packet: The data server is sending us the data
	// for a file we requested.
	if( tP->Type() == SONORK_NETIO_HFLAG_TYPE_RAW_DATA )
	{
		// Are we in receive mode?
		if( tx_phase == SONORK_FILE_TX_PHASE_RECEIVING )
		{
			// Yes: Process the data
			RecvFileData(tP->DataPtr(),tP->DataSize());
		}
		return;
	}
	// Is it an TSrkIpcPacket?
	if( tP->Type() != SONORK_NETIO_HFLAG_TYPE_PACKET )
		return;	// No: Don't know how to handle, so ignore

	// TSonorkTcpPacket's data is a TSrkIpcPacket
	iP=(TSrkIpcPacket*)tP->DataPtr();
	iP_size=tP->DataSize();
	if(iP_size < sizeof(TSrkIpcPacket))
	{
		// Size is less than the header of a TSrkIpcPacket
		// this should never happen!: We've probably connected
		// to something that is NOT a dataserver.
		SetCodecError(
			 SONORK_FILE_TX_ERROR_NET_IO
			,taskERR
			,GSS_PCKTOOSMALL);
		return;
	}
	// When the data server is sending us an error instead
	// of the AKN for our request, it sets the ERROR flag in
	// the TSonorkTcpPacket. This flag means that the packet
	// contains an TSonorkError instead of the normal data
	// it would have contained for the AKN of the request.
	if( iP->IsError() )
	{
		// Extract the error
		if(!iP->D_Error(iP_size,taskERR))
		{
			// Geez, we could not even decode the error..
			// what is going on?. Set a default error:
			// We've probably connected to something
			// that is NOT a dataserver.
			taskERR.SetSys(SONORK_RESULT_CODEC_ERROR
				,GSS_BADCODEC
				,SONORK_MODULE_LINE);
		}
	}
	else
		taskERR.SetOk();
	switch(tx_phase)
	{

		case SONORK_FILE_TX_PHASE_AUTHORIZING:
			TxPhase_Authorizing_Packet(iP,iP_size);
			break;

		case SONORK_FILE_TX_PHASE_REQUESTING:
			TxPhase_Requesting_Packet(iP,iP_size);
			break;

		case SONORK_FILE_TX_PHASE_WAIT_RECV_CONFIRM:
			TxPhase_WaitRecvConfirm_Packet(iP);
			break;

		case SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM:
			TxPhase_WaitSendConfirm_Packet(iP,iP_size);
			break;

		case SONORK_FILE_TX_PHASE_RECEIVING:
			// We've received a TSonorkIpcPacket while RECEIVING.
			// Normally the server sends only RAW packets, so
			// something bad must have happened on the server's
			// side while sending the data.
			if( taskERR.Result()==SONORK_RESULT_OK )
			{
				// Why did the server send an "OK" error
				// in middle of the transmission?: The
				// server is losing it.
				taskERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
					,GSS_BADNETPROTOCOL
					,SONORK_MODULE_LINE);
			}
			SetError(SONORK_FILE_TX_ERROR_NET_IO,&taskERR);
			return;
	}

	if( taskERR.Result()!=SONORK_RESULT_OK
	&&  tx_phase!=SONORK_FILE_TX_PHASE_HALTED_BY_ERROR )
	{
		// An error ocurred, but none of the Packet handling
		// methods catched it. (Otherwise our current phase
		// would have been SONORK_FILE_TX_PHASE_HALTED_BY_ERROR)
		// So we generate the UNKNOWN error which will also change
		// our phase to SONORK_FILE_TX_PHASE_HALTED_BY_ERROR.
		SetError(SONORK_FILE_TX_ERROR_UNKNOWN , &taskERR);
	}
}

// ----------------------------------------------------------------------------
// TxPhase_Authorizing_Packet
//  Process packets we expect when in AUTHORIZING phase:
//  The only packet we know how to process is
//    SONORK_DATA_SERVER_FUNCTION_LOGIN_REQ
//  All the rest are ignored.

void
 TSonorkFileTxEng::TxPhase_Authorizing_Packet(
	 TSrkIpcPacket*	P
	,UINT 		)
{
	if( P->Function() != SONORK_DATA_SERVER_FUNCTION_LOGIN_REQ )
	{
		// We do not understand the packet, assume ok and ignore
		taskERR.SetOk();
		return;
	}

	if( taskERR.Result() != SONORK_RESULT_OK )
	{
		// The server denied our request
		SetError(SONORK_FILE_TX_ERROR_AUTHORIZE,&taskERR);
		return;
	}

	// The server accepted our request: Start transmission
	// Both SendGetFileRequest() and DoSendNextFile()
	// set our mode to SONORK_FILE_TX_PHASE_REQUESTING
	switch( tx_mode )
	{
		case SONORK_FILE_TX_MODE_RECV:
		case SONORK_FILE_TX_MODE_DELETE:
			SendGetFileRequest();
			break;

		case SONORK_FILE_TX_MODE_SEND:
			DoSendNextFile();
			break;
	}
}

// ----------------------------------------------------------------------------
// TxPhase_Requesting_Packet
//  Process packets we expect when in REQUESTING phase,
//  the allowed packets in this mode are:
//  for OLD and NEW data servers
//	SONORK_DATA_SERVER_FUNCTION_GET_FILE_OLD
//	SONORK_DATA_SERVER_FUNCTION_PUT_FILE_OLD
//  for NEW data server:
//      SONORK_DATA_SERVER_FUNCTION_GET_FILE_NEW
//      SONORK_DATA_SERVER_FUNCTION_PUT_FILE_NEW
//  All the rest are ignored.

void
 TSonorkFileTxEng::TxPhase_Requesting_Packet(
	 TSrkIpcPacket*	P
	,UINT 		P_size)
{
	switch( tx_mode )
	{
		case SONORK_FILE_TX_MODE_RECV :
		case SONORK_FILE_TX_MODE_DELETE:
		if( P->Function() != SONORK_DATA_SERVER_FUNCTION_GET_FILE_OLD
		&&  P->Function() != SONORK_DATA_SERVER_FUNCTION_GET_FILE_NEW)
		{
			// We do not understand the packet, assume ok and ignore
			taskERR.SetOk();
			return;
		}

		// Valid packet: Is it an error?
		if(taskERR.Result() != SONORK_RESULT_OK)
		{
			// Yes: Halt by error (below)
			break;
		}

		if( tx_mode == SONORK_FILE_TX_MODE_DELETE)
		{
			// Packet AKN of a GET_FILE request with
			// the DELETE flag set, means the operation
			// has completed.
			SetPhase(SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE
				, REPORT_PHASE_EVENT
				, IsNewProtocol()
				?"Req:GET_FILE_NEW"
				:"Req:GET_FILE_OLD"
			);
			return;
		}
		if( !P->D_Akn(P_size,&file.info) )
		{
			taskERR.SetSys(SONORK_RESULT_CODEC_ERROR
				,GSS_BADCODEC
				,SONORK_MODULE_LINE);
			// Halt by error (below)
			break;
		}
		if( SONORK_FILE_TX_COMPRESSION_TYPE(file.info.attr.compress_flags)
			==
		    SONORK_FILE_TX_COMPRESSION_TYPE(SONORK_FILE_TX_COMPRESSION_FLAGS))
		{
			file.compress_flags=file.info.attr.compress_flags;
			SonorkApp.GetTempPath(file.work_path
				, "filetx"
				, ".zip"
				, (DWORD)this);
		}
		else
		{
			file.compress_flags=0;
			file.work_path.Set(file.real_path);
		}

		if(!OpenTxFile(file.work_path.CStr()
				, true		// write mode
				, NULL,NULL	// No size nor time needed
				, IsNewProtocol()
					?"Req:GET_FILE_NEW"
					:"Req:GET_FILE_OLD"
				))
			return; // Don't set error: OpenTxFile() already does that

		SetPhase(SONORK_FILE_TX_PHASE_RECEIVING
		, REPORT_PHASE_EVENT
		, IsNewProtocol()
			?"Req:GET_FILE_NEW"
			:"Req:GET_FILE_OLD"
			);
		return;	// return, _not_ break

		case SONORK_FILE_TX_MODE_SEND:
		if( P->Function() == SONORK_DATA_SERVER_FUNCTION_PUT_FILE_OLD
		||  P->Function() == SONORK_DATA_SERVER_FUNCTION_PUT_FILE_NEW)
		{
			// Valid packet: Is it an error?
			if(taskERR.Result() != SONORK_RESULT_OK)
			{
				// Yes: Halt by error
				break;
			}
			// Set our phase to SENDING and report to callback.
			// This will do nothing right now, but next time
			// OnTimer() is invoked it will make OnTimer()
			// call SendFileData() until the phase is changed
			// again.
			SetPhase(SONORK_FILE_TX_PHASE_SENDING
			, REPORT_PHASE_EVENT
			,  IsNewProtocol()
				?"Req:PUT_FILE_NEW"
				:"Req:PUT_FILE_OLD"
			);
		}
		else
		{
			// We do not understand the packet, assume ok and ignore
			taskERR.SetOk();
		}
		return;	// return (don't set error)
	}

	// Set error to SONORK_FILE_TX_ERROR_REQUEST
	// and phase to SONORK_FILE_TX_PHASE_HALTED_BY_ERROR

	SetError(SONORK_FILE_TX_ERROR_REQUEST, &taskERR);
}

// ----------------------------------------------------------------------------
// TxPhase_WaitRecvConfirm_Packet
// We've downloaded the file ok, and after doing that, we sent a CLOSE_FILE
// packet to the server to let it know we've got the file OK. Wait for the
// AKN of the server.

void
 TSonorkFileTxEng::TxPhase_WaitRecvConfirm_Packet(TSrkIpcPacket*P)
{
	if(tx_mode!=SONORK_FILE_TX_MODE_RECV || !IsNewProtocol() )
	{
		// Dear Server,
		//  Why did you send us this packet?

		taskERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
			,GSS_BADNETPROTOCOL
			,SONORK_MODULE_LINE);
		SetError(SONORK_FILE_TX_ERROR_NET_IO,&taskERR);
		return;
	}
	
	if( P->Function() != SONORK_DATA_SERVER_FUNCTION_CLOSE_FILE )
	{
		// We do not understand the packet, assume ok and ignore
		taskERR.SetOk();
		return;
	}

//	TRACE_DEBUG("RECVD : CLOSE FILE = %s",taskERR.ResultName());
	SetPhase(SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE
		,REPORT_PHASE_EVENT
		,"Wait_RECV_Confirm");
}

// ----------------------------------------------------------------------------
// TxPhase_WaitSendConfirm_Packet
// We've transmited the whole current file and we're waiting for the
// server to let us know when it has processed it.
// The NEW method will send us <N> TARGET_RESULT packets, where <N>
// is the number of users we've specified as target for the file.
// After the result of all targets have been received, it will send
// the final PUT_FILE_OLD or PUT_FILE_NEW packet with the result of
// the whole operation.

void
 TSonorkFileTxEng::TxPhase_WaitSendConfirm_Packet(
	 TSrkIpcPacket*	P
	,UINT 		P_size)
{
	TSonorkFileTxEngEvent	E;
	TSonorkDataServerPutFileTargetResult	target_result;

	if( tx_mode!=SONORK_FILE_TX_MODE_SEND )
	{
		// Dear Server,
		//  Why did you send us this packet?

		taskERR.SetSys(SONORK_RESULT_PROTOCOL_ERROR
			,GSS_BADNETPROTOCOL
			,SONORK_MODULE_LINE);
		SetError(SONORK_FILE_TX_ERROR_NET_IO,&taskERR);
		return;
	}
	switch(P->Function())
	{

	case SONORK_DATA_SERVER_FUNCTION_PUT_FILE_OLD:
	case SONORK_DATA_SERVER_FUNCTION_PUT_FILE_NEW:

		if(taskERR.Result() != SONORK_RESULT_OK)
			break;

		if( P->D_Akn(P_size,&TSonorkCodecDW(&file.info.locus,sizeof(file.info.locus),(SONORK_ATOM_TYPE)SRK_DATA_SERVER_ATOM_FILE_LOCUS)) )
		{
			SetPhase( SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE
			, REPORT_PHASE_EVENT
			, "Wait_SEND_Confirm");
			return;
		}
		taskERR.SetSys(SONORK_RESULT_CODEC_ERROR,GSS_BADCODEC,SONORK_MODULE_LINE);
		break;


	case SONORK_DATA_SERVER_FUNCTION_PUT_FILE_TARGET_RESULT:

		if(taskERR.Result() != SONORK_RESULT_OK)
			break;

		if( P->D_Akn(P_size,&target_result) )
		{
//			TRACE_DEBUG("TARGET_RESULT(%u.%u) = %s (ID=%x.%x)"
//			, target_result.header.sonork_id.v[0]
//			, target_result.header.sonork_id.v[1]
//			, target_result.ERR.ResultName()
//			, target_result.header.msg_id.v[0]
//			, target_result.header.msg_id.v[1]);

			E.event = SONORK_FILE_TX_EVENT_TARGET_RESULT;
			E.target_result = &target_result;
			InvokeEventHandler( &E );
			return;
		}
		taskERR.SetSys(SONORK_RESULT_CODEC_ERROR,GSS_BADCODEC,SONORK_MODULE_LINE);
		break;

	default:
		// We do not understand the packet, assume ok and ignore
		taskERR.SetOk();
		return;
	}
	SetError(SONORK_FILE_TX_ERROR_CONFIRM, &taskERR);
}

// ----------------------------------------------------------------------------
// OnPoke
//  We post ourselves "pokes" from the handlers
//  in order to execute the poked task outside the handler.
//  Why? because we can't display message boxes from within the handlers

LRESULT
 TSonorkFileTxEng::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	int net_err;

	switch(wParam)
	{
	case POKE_CONTINUE:
		if( tx_phase == SONORK_FILE_TX_PHASE_NONE )
		{
			StartResolution();
		}
		else
		if( tx_phase == SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE )
		{
			if( tx_mode == SONORK_FILE_TX_MODE_SEND )
			{
				// Sending
				DoSendNextFile();
			}
			else
			{
				// Receiving
				SetPhase(SONORK_FILE_TX_PHASE_FINISHED
				,REPORT_PHASE_EVENT
				,"Poke:CONTINUE");
			}
		}
		// else: IGNORE
	break;

	case POKE_DATASERVER_RESOLUTION_RESULT:
		// Data server resolution has ended.
		// The resolution starts when we process POKE_CONTINUE (above)
		// and phase is still SONORK_FILE_TX_PHASE_NONE (we're starting)
		if( data_server != NULL )
		{
			// Ok!, we've got the data server we requested
			// (or "a" data server, in send mode)

			// Load the SOCKS information from the application
			// so that the engine uses SOCKS if applicable.
			tcp.SocksInfo().Set(SonorkApp.SonorkSocksInfo());

			// Now, check if the data server is an old or new one
			protocol =
				data_server->ServiceVersionNumber()
					>=SONORK_DATA_SERVER_NEW_PROTOCOL_VERSION
				?PROTOCOL_NEW
				:PROTOCOL_OLD;
			// Initiate connection phase.
			// The engine is not connected when Connect() returns,
			// it just means that it has STARTED connecting.
			// Whe will know when it has linked because
			// its state will change to SONORK_NETIO_STATUS_CONNECTED.
			// We do the state checking in OnTimer()

			net_err= tcp.Connect(data_server->physAddr) ;
			if(net_err == 0)
			{
				// Ok, connection phase started.
				SetPhase(SONORK_FILE_TX_PHASE_CONNECTING
				, REPORT_PHASE_EVENT
				, "POKE:RESOLVE_RESULT");
				break;
			}
			// Something is wrong (probably an invalid address)
			taskERR.SetSys(SONORK_RESULT_NETWORK_ERROR
				,GSS_NETCXERR
				,net_err);
		}
		else
		{
			// We did not get any servers. If the task result is OK
			// it means that the resolution completed fine, but no
			// data servers where found.
			if(taskERR.Result() == SONORK_RESULT_OK)
			{
				taskERR.SetSys(SONORK_RESULT_NO_DATA,GSS_NODATA,0);
			}
		}
		SetError(SONORK_FILE_TX_ERROR_RESOLVE
				,&taskERR);
	break;

	case SONORK_WIN_POKE_DESTROY:
		Destroy(lParam);
	break;


	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::StartResolution()
{
	UINT 	P_size;
	TSonorkDataPacket *P;
	DWORD   service_instance;

	ClearResolution();
	if(	(tx_mode==SONORK_FILE_TX_MODE_RECV && SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_FILE_RECV))
	||	(tx_mode==SONORK_FILE_TX_MODE_SEND && SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_FILE_SEND)))
	{
		taskERR.SetSys(SONORK_RESULT_ACCESS_DENIED
			,GSS_NOPERMIT
			,SONORK_MODULE_LINE);
	}
	else
	{
		service_instance=tx_mode==SONORK_FILE_TX_MODE_SEND
				?0
				:file.info.locus.serverNo;

		if( (data_server = SonorkApp.GetServiceDataFromCache(
			 SONORK_SERVICE_TYPE_SONORK_FILE
			, service_instance
			, 0
			)) != NULL )
		{
			taskERR.SetOk();
			PostPoke(POKE_DATASERVER_RESOLUTION_RESULT , 0 );
			return;
		}

#define A_size	128
		P=SONORK_AllocDataPacket(A_size);
		if( SonorkApp.ServerVersionNumber()<MAKE_VERSION_NUMBER(1,5,0,0))
		{
			P_size = P->E_OldListServices_R(A_size
				,SONORK_SERVICE_TYPE_SONORK_FILE
				,2);	// Max Count

		}
		else
		{
			P_size = P->E_NewListServices_R(A_size
				,SONORK_SERVICE_TYPE_SONORK_FILE
				,service_instance
				,0	// Min version
				,2);	// Max count
		}
#undef A_size
		StartSonorkTask(taskERR
			,P
			,P_size
			,SONORK_TASKWIN_F_NO_ERROR_BOX
			,GLS_TK_CXDATASVR
			,(SONORK_DWORD2*)NULL);

		SONORK_FreeDataPacket( P );
		if(taskERR.Result() == SONORK_RESULT_OK)
		{
			// Task has been initiated
			return;
		}
	}
	PostPoke(POKE_DATASERVER_RESOLUTION_RESULT , 0);
}

// ----------------------------------------------------------------------------

void
 TSonorkFileTxEng::ClearResolution()
{
	if(data_server)
	{
		SONORK_MEM_DELETE(data_server);
		data_server=NULL;
	}
}

// ----------------------------------------------------------------------------

TSonorkFileTxEng::TSonorkFileTxEng(
	  TSonorkWin*			parent
	, TSonorkShortStringQueue*	queue
	, pSonorkFileTxEngHandler 	cb_ptr
	, void*				cb_dat
	, DWORD 			p_tx_flags)
	:TSonorkTaskWin(parent
		,SONORK_WIN_CLASS_NORMAL|SONORK_WIN_TYPE_NONE
		, parent!=NULL?0:SONORK_WIN_SF_NO_WIN_PARENT)
{
	SetEventMask(SONORK_APP_EM_ENUMERABLE);

	tx_phase 	= SONORK_FILE_TX_PHASE_NONE;
	tx_flags	= p_tx_flags;
	tx_mode		= SONORK_FILE_TX_MODE_SEND;

	file.handle	= SONORK_INVALID_FILE_HANDLE;
	file.ZH		= NULL;
//	send.buffer 	= SONORK_MEM_ALLOC(BYTE,SEND_BUFFER_SIZE);
	send.queue  	= queue;

	// All files sent will have the same tracking no.
	// But given that a copy will be sent to different users
	// we won't confuse them.
	send.tracking_no.v[0]=SonorkApp.GenSelfTrackingNo();
	send.tracking_no.v[1]=0;

	data_server	= NULL;
	cb.ptr		= cb_ptr;
	cb.dat		= cb_dat;


}
TSonorkFileTxEng::TSonorkFileTxEng(
	  TSonorkWin*			parent
	, const TSonorkFileInfo& 	p_file_info
	, SONORK_C_CSTR			p_download_path
	, pSonorkFileTxEngHandler 	cb_ptr
	, void*				cb_dat
	, DWORD 			p_tx_flags)
	:TSonorkTaskWin(parent,SONORK_WIN_CLASS_NORMAL|SONORK_WIN_TYPE_NONE, parent!=NULL?0:SONORK_WIN_SF_NO_WIN_PARENT)
{


	tx_phase 	= SONORK_FILE_TX_PHASE_NONE;
	tx_flags	= p_tx_flags;
	tx_mode		= tx_flags&SONORK_FILE_TX_F_DELETE_FILE?SONORK_FILE_TX_MODE_DELETE:SONORK_FILE_TX_MODE_RECV;

	file.handle	= SONORK_INVALID_FILE_HANDLE;
	file.ZH		= NULL;
	file.info.Set(p_file_info);	// File we are going to retrieve
	file.real_path.Set( p_download_path );

//	send.buffer = NULL;
	send.queue  = NULL;
	send.tracking_no.Clear();

	data_server	= NULL;
	cb.ptr		= cb_ptr;
	cb.dat		= cb_dat;
}

TSonorkFileTxEng::~TSonorkFileTxEng()
{
//	if(send.buffer != NULL){SONORK_MEM_FREE( send.buffer );}

	if(send.queue  != NULL)
	{
		SONORK_MEM_FREE( send.queue );
	}

	ClearResolution();
	
	if((tx_flags & SONORK_FILE_TX_F_FREE_CALLBACK_PARAM)
		&& cb.dat!=NULL)
	{
		SONORK_MEM_FREE( cb.dat );
	}		
}

// ----------------------------------------------------------------------------
// ClearYield(): See notes in OnTimer() about the yielding mechanism

void
 TSonorkFileTxEng::ClearYield()
{
	yield_count=0;
}

// ----------------------------------------------------------------------------
// ShouldYield(): See notes in OnTimer() about the yielding mechanism

bool
 TSonorkFileTxEng::ShouldYield()
{
	return ++yield_count > TX_FILE_YIELD_COUNT;
}
// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkFileTxEng::GetUncompressedSize(const TSonorkFileInfo& fi,TSonorkShortString& name)
{
	SONORK_C_STR	str;
	name.Set(fi.name);
	if(fi.attr.compress_flags == SONORK_FILE_TX_COMPRESSION_FLAGS )
	{
		 // SONORK_IO_GetFileExt DOES include the dot
		str = (SONORK_C_STR)SONORK_IO_GetFileExt(name.CStr());
		*str=0;
	}

	return name.CStr();
}

static SONORK_C_CSTR
 GetPhaseName(SONORK_FILE_TX_PHASE phase)
{
switch(phase)
{
case SONORK_FILE_TX_PHASE_NONE:return "NONE";
case SONORK_FILE_TX_PHASE_RESOLVING:return "Resolv";
case SONORK_FILE_TX_PHASE_CONNECTING:return "Cxt";
case SONORK_FILE_TX_PHASE_COMPRESSING:return "COMPRESS";
case SONORK_FILE_TX_PHASE_AUTHORIZING:return "Auth";
case SONORK_FILE_TX_PHASE_REQUESTING:return "Req";
case SONORK_FILE_TX_PHASE_SENDING:return "Send";
case SONORK_FILE_TX_PHASE_RECEIVING:return "Recv";
case SONORK_FILE_TX_PHASE_WAIT_SEND_CONFIRM:return "Conf-Send";
case SONORK_FILE_TX_PHASE_WAIT_RECV_CONFIRM:return "Conf-Recv";
case SONORK_FILE_TX_PHASE_EXPANDING:return "EXPAND";
case SONORK_FILE_TX_PHASE_FILE_TX_COMPLETE:return "Complete";
case SONORK_FILE_TX_PHASE_FINISHED:return "FINISHED";
case SONORK_FILE_TX_PHASE_HALTED_BY_ERROR:return "HALTED";
default:
	return "???";
}
}
