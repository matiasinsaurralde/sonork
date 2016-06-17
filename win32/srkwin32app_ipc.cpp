#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srk_ipc_defs.h"

//-----------------------------------------------------------------------------

// Private of server
#define SONORK_IPC_HEADER_FULL_SIZE	(4096)
#define SONORK_IPC_HEADER_DATA_SIZE	(sizeof(TSonorkIpcFileHeader))
#define SONORK_IPC_BLOCK_FULL_SIZE	(8192)
#define SONORK_IPC_BLOCK_DATA_SIZE	(SONORK_IPC_BLOCK_FULL_SIZE-sizeof(TSonorkIpcFileBlockHeader))
#define SONORK_IPC_MAX_DATA_SIZE	65536
#define SONORK_IPC_MESSAGE_TIMEOUT	60000	//1500
#define SONORK_IPC_COOKIE_MASK		0xffffff00
#define SONORK_IPC_COOKIE_SHIFT		8
#define SONORK_IPC_ENTRY_INDEX_MASK	0x000000ff
#define SONORK_IPC_SHARED_SLOTS		64
#define SONORK_IPC_SLOT_INDEX_MASK	0x00000fff
//-----------------------------------------------------------------------------

enum SONORK_IPC_ENTRY_CTRL_FLAGS
{
  SONORK_IPC_ECF_LINKED			= 0x00010000
, SONORK_IPC_ECF_CONDEMNED		= 0x00020000
, SONORK_IPC_ECF_INVALID		= 0x00040000
};

//-----------------------------------------------------------------------------

struct TSonorkIpcEntry
{
	DWORD	ipcId;
	DWORD	serviceId;
	DWORD	ctrlFlags;
	DWORD	eventMask;
	HWND	clientHwnd;
	DWORD	clientMsg;
	DWORD	sharedSlotHandle;
	struct {
		HANDLE			handle;
		BYTE*			ptr;
		TSonorkIpcFileBlock*	req;
		TSonorkIpcFileBlock*	evt;
	}dta;

};

struct TSonorkIpcFullFileHeader
:public TSonorkIpcFileHeader
{
	TSonorkIpcSharedSlot slot[SONORK_IPC_SHARED_SLOTS];
};

//-----------------------------------------------------------------------------

class TSonorkWin32IpcServer
{
friend TSonorkWin32App;

	struct
	{
		HANDLE				mutex;
		HANDLE				handle;
		TSonorkIpcFullFileHeader*	header;
		TSonorkIpcFileBlock*		block;
	}dta;


	struct
	{
		TSonorkIpcEntry*	entry;
		TSonorkIpcFileBlock*	reqBlock;
		TSonorkIpcFileBlock*	evtBlock;
	}req;

	struct
	{
		TSonorkIpcEntry*	entry;
		SONORK_IPC_FUNCTION	ipcFunction;
		union
		{
			struct
			{
				SONORK_APP_EVENT		event;
				union
				{
					void*			raw;
					TSonorkMsg*		msg;
				}data;
			}appEvent;

			struct
			{
				SONORK_APP_SERVICE_EVENT	event;
				union
				{
					TSonorkUserServer*	userver;
				}data;
			}svcEvent;
		};
		BYTE*	dataPtr;
		DWORD	dataSize;
	}disp;


	struct {
		DWORD		size,active;
		TSonorkIpcEntry	*entry;
	}table;

	SONORK_RESULT
		IpcReq_RegisterService();

	SONORK_RESULT
		IpcReq_UnregisterService();

	SONORK_RESULT
		IpcReq_PokeServiceData();

	SONORK_RESULT
		IpcReq_StartServiceQuery();

	SONORK_RESULT
		IpcReq_ReplyServiceQuery();

	SONORK_RESULT
		IpcReq_AcceptServiceCircuit();

	SONORK_RESULT
		IpcReq_GetServiceCircuit();

	SONORK_RESULT
		IpcReq_OpenServiceCircuit();

	SONORK_RESULT
		IpcReq_CloseServiceCircuit();

	SONORK_RESULT
		IpcReq_Link(DWORD);

	SONORK_RESULT
		IpcReq_GetUserData();

	SONORK_RESULT
		IpcReq_GetUserHandle();

	SONORK_RESULT
		IpcReq_GetEventData();

	SONORK_RESULT
		IpcReq_SetServerData();
		
	SONORK_RESULT
		IpcReq_Unlink();

	SONORK_RESULT
		IpcReq_AllocSharedSlot();

	SONORK_RESULT
		IpcReq_FreeSharedSlot();

	SONORK_RESULT
		IpcReq_EnumUserGroups();

	SONORK_RESULT
		IpcReq_EnumUserList();

	SONORK_RESULT
		IpcReq_PrepareMsg();

	SONORK_RESULT
		IpcReq_SendMsg();
                
	SONORK_RESULT
		IpcReq_SendFiles();

	SONORK_RESULT
		IpcReq_RecvFile();

	TSonorkIpcEntry*
		GetIpcEntry(DWORD);

	TSonorkIpcEntry*
		AllocIpcEntry();

	void
		CondemnIpcEntry(TSonorkIpcEntry*);

	static void
		ClearIpcEntry(DWORD id);


	static DWORD SONORK_CALLBACK
		IpcServiceCallback(
		 SONORK_DWORD2&			handler_tag
		,SONORK_APP_SERVICE_EVENT	event_id
		,SONORK_DWORD2*			event_tag
		,TSonorkAppServiceEvent*	event_data
		);

	static BOOL
		IpcDispatchMsg(TSonorkIpcEntry* E
			, DWORD			function
			, LPARAM 		lParam
			, DWORD& 		result);

	static BOOL
		IpcPutDataAtom(TSonorkIpcEntry*
			, DWORD function
			, TSonorkIpcFileBlock*
			, const TSonorkCodecAtom*);

	static BOOL
		IpcPutDataRaw(TSonorkIpcEntry*
			, DWORD function
			, TSonorkIpcFileBlock*
			, const void*data
			, DWORD dataSize);
	// use GET_REQ for ReqBlock and GET_EVT for req.evtBlock
	SONORK_RESULT
		IpcGetDataByCallback(
			  SONORK_IPC_FUNCTION
			, BYTE**	dataPtr
			, DWORD&	dataSize
			, BOOL& 	mustFreePtr);

};


//-----------------------------------------------------------------------------


TSonorkWin32IpcServer
	ipcServer;

//-----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_AcceptServiceCircuit()
{
	if( req.reqBlock->params.v[2] < sizeof(TSonorkServiceCircuit))
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	return SonorkApp.Service_AcceptCircuit(
			  req.reqBlock->params.v[0]
			, req.reqBlock->params.v[1]
			, (TSonorkServiceCircuit*)&req.reqBlock->params.v[6]);

}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_RecvFile()
{
	SONORK_RESULT		result;
	TSonorkShortString	path;
	TSonorkId		userId;
	userId.Set(req.reqBlock->params.d[0]);
	if( req.reqBlock->params.v[8] < sizeof(TSonorkCCacheMark))
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	req.reqBlock->data[SONORK_IPC_BLOCK_DATA_SIZE-1]=0;
	path.Set((char*)req.reqBlock->data);
	result = SonorkApp.RecvFile(userId
		 , (TSonorkCCacheMark*)&req.reqBlock->params.v[16]
		 , path);
	if( result == SONORK_RESULT_OK )
	{
		lstrcpyn((char*)req.reqBlock->data
			, path.CStr()
			,SONORK_IPC_BLOCK_DATA_SIZE-1);
	}
	return result;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_SendFiles()
{
	SONORK_RESULT		result;
	BYTE*			dataPtr;
	DWORD			dataSize;
	DWORD			fileCount;
	BOOL			mustFreePtr;
        TSonorkDynString	dString;
	TSonorkShortString*	sString;
        TSonorkShortStringQueue*sStringQueue;
        TSonorkExtUserData*	pUD;

        fileCount = req.reqBlock->params.v[0];
        pUD = SonorkApp.UserList().Get(req.reqBlock->params.d[4]);
	if( pUD==NULL || fileCount==0 )
        	return SONORK_RESULT_INVALID_PARAMETER;

	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);

	if( result == SONORK_RESULT_OK )
	{
        	TSonorkCodecReader	CODEC(dataPtr,dataSize,SONORK_ATOM_DYN_STRING);

		SONORK_MEM_NEW( sStringQueue = new TSonorkShortStringQueue );
                while(fileCount-- )
                {
			CODEC.Read(&dString);
			if( CODEC.CodecOk() )
			{
                        	sString = new TSonorkShortString;
                                sString->Set( dString.ToCStr() );
                                sStringQueue->Add( sString );
			}
                }
                result = CODEC.Result();

		if(mustFreePtr)
                {
			SONORK_MEM_FREE(dataPtr);
		}
                
                if( result==SONORK_RESULT_OK )
                {
                	SonorkApp.SendFiles(pUD,sStringQueue,true);
		}
                else
                {
                	SONORK_MEM_DELETE( sStringQueue );
                }

	}
	return result;


}

//-----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_OpenServiceCircuit()
{
	SONORK_RESULT		result;
	BYTE*			dataPtr;
	DWORD			dataSize;
	BOOL			mustFreePtr;

	if( req.reqBlock->params.v[8] < sizeof(TSonorkServiceCircuit))
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);
	if( result == SONORK_RESULT_OK )
	{
		result=SonorkApp.Service_OpenCircuit(
			  req.reqBlock->params.v[0]
			, (TSonorkServiceCircuit*)(&req.reqBlock->params.v[10])
			, req.reqBlock->params.v[3]
			, req.reqBlock->params.v[4]
			, req.reqBlock->params.v[5]
			, dataPtr
			, dataSize
			, (SONORK_DWORD2*)(&req.reqBlock->params.v[6]));
		if(mustFreePtr)
			SONORK_MEM_FREE(dataPtr);
	}
	return result;
}

//-----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_ReplyServiceQuery()
{
	BYTE*			dataPtr;
	DWORD			dataSize;
	SONORK_RESULT		result;
	BOOL			mustFreePtr;

	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);
	if( result == SONORK_RESULT_OK )
	{
		result=	SonorkApp.Service_Reply(
			  (SONORK_CTRLMSG_CMD)req.reqBlock->params.v[2]
			, req.reqBlock->params.v[3]
			, req.reqBlock->params.v[4]
			, dataPtr
			, dataSize);
		if(mustFreePtr)
			SONORK_MEM_FREE(dataPtr);

	}
	return result;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_CloseServiceCircuit()
{
	return	SonorkApp.Service_CloseCircuit(req.reqBlock->params.v[0]
			, req.reqBlock->params.v[1]
			, (SONORK_RESULT)req.reqBlock->params.v[2]);
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_GetServiceCircuit()
{
	if( req.reqBlock->params.v[2] < sizeof(TSonorkServiceCircuit))
	{
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	return SonorkApp.Service_GetCircuitHandle(
			  req.reqBlock->params.v[0]
			, (TSonorkServiceCircuit*)&req.reqBlock->params.v[6]);

}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_PokeServiceData()
{
	BYTE*			dataPtr;
	DWORD			dataSize;
	SONORK_RESULT		result;
	BOOL			mustFreePtr;

	if( req.reqBlock->params.v[8] < sizeof(TSonorkServiceHandle))
	{
		return SONORK_RESULT_INVALID_PARAMETER;

	}
	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);
	if( result == SONORK_RESULT_OK )
	{
		SonorkApp.win32.run_flags|=SONORK_WAPP_RF_IN_IPC_HANDLER;
		result	=SonorkApp.Service_SendPokeData(
				  req.reqBlock->params.v[0]
				, (TSonorkServiceHandle*)&req.reqBlock->params.v[10]
				, (SONORK_CTRLMSG_CMD)req.reqBlock->params.v[2]
				, req.reqBlock->params.v[3]
				, req.reqBlock->params.v[4]
				, dataPtr
				, dataSize);
		SonorkApp.win32.run_flags&=~SONORK_WAPP_RF_IN_IPC_HANDLER;
		if( mustFreePtr )
			SONORK_MEM_FREE( dataPtr );
	}
	return result;
}
// ----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_StartServiceQuery()
{
	BYTE*			dataPtr;
	DWORD			dataSize;
	SONORK_RESULT		result;
	BOOL			mustFreePtr;

	if( req.reqBlock->params.v[8] < sizeof(TSonorkServiceInstanceInfo))
	{
		return SONORK_RESULT_INVALID_PARAMETER;

	}
	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);
	if( result == SONORK_RESULT_OK )
	{
		SonorkApp.win32.run_flags|=SONORK_WAPP_RF_IN_IPC_HANDLER;
		result	=SonorkApp.Service_StartQuery(
				  req.reqBlock->params.v[0]
				, (TSonorkServiceQuery*)&req.reqBlock->params.v[10]
				, (SONORK_CTRLMSG_CMD)req.reqBlock->params.v[2]
				, req.reqBlock->params.v[3]
				, req.reqBlock->params.v[4]
				, req.reqBlock->params.v[5]
				, dataPtr
				, dataSize
				, (SONORK_DWORD2*)&req.reqBlock->params.v[6]);
		SonorkApp.win32.run_flags&=~SONORK_WAPP_RF_IN_IPC_HANDLER;
		if( mustFreePtr )
			SONORK_MEM_FREE( dataPtr );
	}
	return result;

}
//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_RegisterService()
{
	SONORK_APP_SERVICE_TYPE		svc_type;
	SKIN_ICON			svc_icon;
	SONORK_DWORD2			cb_tag;
	SONORK_RESULT			svc_result;

	if( req.reqBlock->dataSize < sizeof(TSonorkServiceInstanceInfo) )
	{
		return SONORK_RESULT_INVALID_PARAMETER;

	}
	svc_type	=(SONORK_APP_SERVICE_TYPE)req.reqBlock->params.v[0];
	cb_tag.v[0]	=req.entry->ipcId;
	cb_tag.v[1]	=req.reqBlock->params.v[1];
	svc_icon	=(SKIN_ICON)req.reqBlock->params.v[2];
	SonorkApp.win32.run_flags|=SONORK_WAPP_RF_IN_IPC_HANDLER;
	svc_result	=SonorkApp.Service_Register(
				  svc_type
				, (TSonorkServiceInstanceInfo*)req.reqBlock->data
				, svc_icon
				, IpcServiceCallback
				, &cb_tag);
	SonorkApp.win32.run_flags&=~SONORK_WAPP_RF_IN_IPC_HANDLER;
	return svc_result;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_UnregisterService()
{
	SONORK_RESULT			svc_result;
	SonorkApp.win32.run_flags|=SONORK_WAPP_RF_IN_IPC_HANDLER;
	svc_result=	SonorkApp.Service_Unregister(req.reqBlock->params.v[0]
			, req.reqBlock->params.v[1]
			, (SONORK_SERVICE_ID)req.reqBlock->params.v[2]);
	SonorkApp.win32.run_flags&=~SONORK_WAPP_RF_IN_IPC_HANDLER;
	return svc_result;
}

// ----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_PrepareMsg()
{
	TSonorkMsgHandle	msgHandle;
	TSonorkServiceId	serviceId;

	if(req.reqBlock->dataSize < sizeof(TSonorkMsgHeader) )
		return SONORK_RESULT_INVALID_PARAMETER;

	serviceId.Set(req.entry->serviceId,req.entry->ipcId);
	SonorkApp.PrepareMsg(msgHandle
		, (TSonorkMsgHeader*)req.reqBlock->data
		, NULL			// no text
		, req.reqBlock->params.v[0]
		, req.reqBlock->params.v[1]
		, req.reqBlock->params.v[2]
		, req.reqBlock->params.v[5]
		, &serviceId
		);
	req.reqBlock->params.v[0] = sizeof(TSonorkMsgHandle);
	req.reqBlock->params.v[1] = 0;
	memcpy(&req.reqBlock->params.v[4],&msgHandle,sizeof(TSonorkMsgHandle));
	return SONORK_RESULT_OK;

}

// ----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_SendMsg()
{
	TSonorkMsgHandle*	msgHandlePtr;
	TSonorkUserHandle*	userHandle;
	TSonorkMsgTarget*	msgTarget;
	TSonorkMsg		msg;
	BYTE*			dataPtr;
	DWORD			dataSize;
	SONORK_RESULT		result;
	BOOL			mustFreePtr;

	if( req.reqBlock->params.v[0] < sizeof(TSonorkMsgHandle)
	||  req.reqBlock->params.v[1] < sizeof(TSonorkUserHandle))
		return SONORK_RESULT_INVALID_PARAMETER;

	dataPtr = (BYTE*)( &req.reqBlock->params.v[10] );

	msgHandlePtr = (TSonorkMsgHandle*)dataPtr;
	dataPtr+= sizeof(TSonorkMsgHandle);

	userHandle = (TSonorkUserHandle*)dataPtr;
	if( req.reqBlock->params.v[2] == 0)
	{
		msgTarget=NULL;
	}
	else
	{
		if( req.reqBlock->params.v[2] < sizeof(TSonorkMsgTarget))
			return SONORK_RESULT_INVALID_PARAMETER;

		dataPtr+= sizeof(TSonorkUserHandle);
		msgTarget=(TSonorkMsgTarget*)dataPtr;
	}
	result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_REQ_DATA
			, &dataPtr
			, dataSize
			, mustFreePtr);
	if( result == SONORK_RESULT_OK )
	{
		result = msg.CODEC_ReadMem(dataPtr,dataSize);
		if(mustFreePtr)
			SONORK_MEM_FREE(dataPtr);

		if( result == SONORK_RESULT_OK )
		{
			TSonorkMsgHandleEx handleEx;
			memcpy(&handleEx,msgHandlePtr,sizeof(TSonorkMsgHandle));
			result = SonorkApp.SendMsgLocus(handleEx
					, NULL
					, userHandle
					, userHandle->utsLinkId
					, &msg
					, msgTarget);
			memcpy(msgHandlePtr,&handleEx,sizeof(TSonorkMsgHandle));
		}
	}
	return (SONORK_RESULT)result;
}


// ----------------------------------------------------------------------------
// IpcReq_GetEventData()
//-----------------------------------------------------------------------------
// This function may be called only from within the IPC handler
//  while it is processing an event with extended data such as a message.
// When the application delivers an event with extended data it sets
// the following fields of the <ipc.disp> structure:
// * <entry> is the entry being invoked by IpcDispatchMsg()
// * <function> to the event being delivered
// So, in this way, by looking at <disp.entry> we know which IPC is
//   currently handling the message and by looking at <disp.function/event>
//   we know what type of data to deliver.
// The server will always clear <ipc.disp.function> once the event has been delivered


SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_GetEventData()
{
	DWORD	dataOffset,bufferSize;
	// Check that the requesting IPC is the one currently receiving the
	// event.
	if( disp.entry != req.entry )
		return SONORK_RESULT_INVALID_OPERATION;

	switch(disp.ipcFunction)
	{
	case SONORK_IPC_FUNCTION_APP_EVENT:
		if( disp.appEvent.event != SONORK_APP_EVENT_SONORK_MSG_RCVD
		&&  disp.appEvent.event != SONORK_APP_EVENT_SONORK_MSG_SENDING)
		{
			return SONORK_RESULT_INVALID_OPERATION;
		}
		IpcPutDataAtom(disp.entry
			, SONORK_IPC_FUNCTION_PUT_EVT_DATA
			, req.evtBlock
			, disp.appEvent.data.msg
			);
		break;

	case SONORK_IPC_FUNCTION_SERVICE_EVENT:

		dataOffset=req.evtBlock->params.v[0];	// 0 = data offset
		bufferSize=req.evtBlock->params.v[1];	// 1 = buffer size
		if(disp.dataSize < SONORK_IPC_BLOCK_DATA_SIZE
		|| bufferSize < SONORK_IPC_BLOCK_DATA_SIZE
		|| dataOffset >= bufferSize
		|| dataOffset >= disp.dataSize
		)
		{
			return SONORK_RESULT_INVALID_OPERATION;
		}
		// Send only up to what fits
		if( bufferSize > disp.dataSize )
			bufferSize=disp.dataSize;

		IpcPutDataRaw(disp.entry
			, SONORK_IPC_FUNCTION_PUT_EVT_DATA
			, req.evtBlock
			, disp.dataPtr+dataOffset
			, bufferSize - dataOffset  
			);
		break;

		default:
			return SONORK_RESULT_INVALID_OPERATION;
	}
	return SONORK_RESULT_OK;
}


//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_AllocSharedSlot()
{
	DWORD	slotId;
	DWORD	slotParam;
	DWORD	openMethod;
	DWORD	foundSlot;
	TSonorkIpcSharedSlot	*slot;

	if( req.entry->sharedSlotHandle!=0)
		return SONORK_RESULT_OUT_OF_RESOURCES;

	slotId		= req.reqBlock->params.v[0];
	slotParam	= req.reqBlock->params.v[1];
	openMethod	= req.reqBlock->params.v[2];

	if( slotId==0 || slotParam == 0)
		return SONORK_RESULT_INVALID_PARAMETER;

	slot=dta.header->slot;
	for(foundSlot=0;foundSlot<SONORK_IPC_SHARED_SLOTS;foundSlot++,slot++)
	{
		if(slot->slotId==slotId && slot->slotParam ==slotParam)
			break;
	}

	if( foundSlot < SONORK_IPC_SHARED_SLOTS)
	{
		// Slot WAS found
		if( openMethod == SONORK_IPC_ALLOC_CREATE_NEW )
			return SONORK_RESULT_DUPLICATE_DATA;
	}
	else
	{
		// Slot was NOT found
		if( openMethod == SONORK_IPC_ALLOC_OPEN_EXISTING )
			return SONORK_RESULT_NO_DATA;

		for(foundSlot=0;foundSlot<SONORK_IPC_SHARED_SLOTS;foundSlot++,slot++)
		{
			if(slot->slotId==0
			&& slot->slotParam==0)
				break;
		}
		if( foundSlot == SONORK_IPC_SHARED_SLOTS)
		{
			// No free slot
			return SONORK_RESULT_OUT_OF_RESOURCES;
		}
	}

	req.reqBlock->params.v[0] = req.entry->sharedSlotHandle = 0x10000000|foundSlot;
	req.reqBlock->params.v[1] = foundSlot*sizeof(TSonorkIpcSharedSlot);
	slot->refCount+=1;

	return SONORK_RESULT_OK;

}
//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_FreeSharedSlot()
{
	DWORD	slotIndex;
	DWORD	slotHandle;
	TSonorkIpcSharedSlot	*slot;

	slotHandle=req.entry->sharedSlotHandle;

	if( slotHandle==0)
		return SONORK_RESULT_NO_DATA;

	if( slotHandle != req.reqBlock->params.v[0])
		return SONORK_RESULT_INVALID_HANDLE;

	slotIndex	= slotHandle&SONORK_IPC_SLOT_INDEX_MASK;
	slot		= (dta.header->slot+slotIndex);

	if( (slot->slotId==0 && slot->slotParam==0) || slot->refCount==0)
		return SONORK_RESULT_INTERNAL_ERROR;

	if( (--slot->refCount) == 0)
	{
		memset(slot,0,sizeof(TSonorkIpcSharedSlot));
	}
	req.entry->sharedSlotHandle = 0;
	return SONORK_RESULT_OK;

}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_EnumUserGroups()
{
	TSonorkGroup *UG;
	TSonorkGroupQueue	Q;
	SonorkApp.LoadUserGroups(Q);
	while((UG=Q.RemoveFirst()) != NULL )
	{
		if(UG->GroupNo() > 0 && UG->GroupNo()<SONORK_MAX_USER_GROUPS)
		{
			IpcPutDataAtom(req.entry
				, SONORK_IPC_FUNCTION_PUT_REQ_DATA
				, req.reqBlock
				, UG);
		}
		SONORK_MEM_DELETE(UG);
	}
	return SONORK_RESULT_OK;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_EnumUserList()
{
	TSrkExtUserDataAtom	atom(NULL);
	TSonorkListIterator	I;
	SonorkApp.UserList().BeginEnum(I);
	while( (atom.UD=SonorkApp.UserList().EnumNext(I)) != NULL)
	{
		IpcPutDataAtom(req.entry
			, SONORK_IPC_FUNCTION_PUT_REQ_DATA
			, req.reqBlock
			, &atom);
	}
	SonorkApp.UserList().EndEnum(I);
	return SONORK_RESULT_OK;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_GetUserData()
{
	TSrkExtUserDataAtom	atom(NULL);
	DWORD	dataSize;

	atom.UD = SonorkApp.UserList().Get( req.reqBlock->params.d[0] );
	if( atom.UD == NULL )
		return SONORK_RESULT_NO_DATA;

	req.reqBlock->dataSize = dataSize = atom.CODEC_Size();

	if( dataSize > SONORK_IPC_BLOCK_DATA_SIZE )
		return SONORK_RESULT_INTERNAL_ERROR;

	// Data fits in a single block, no need for temporal buffer
	return atom.CODEC_WriteMem(req.reqBlock->data,dataSize);
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_GetUserHandle()
{
	TSonorkExtUserData 	*userData;
	TSonorkUserHandle	*userHandle;

	if( req.reqBlock->dataSize < sizeof(TSonorkUserHandle) )
		return SONORK_RESULT_INVALID_HANDLE;

	userHandle = (TSonorkUserHandle*)req.reqBlock->data;

	userData = SonorkApp.UserList().Get( userHandle->userId );
	if( userData == NULL )
		return SONORK_RESULT_NO_DATA;

	userHandle->sid.Set( userData->addr.sid );
	userHandle->utsLinkId = userData->UtsLinkId();

	return SONORK_RESULT_OK;

}
//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_Link(DWORD serviceId)
{
	TSonorkIpcEntry*E;
	char	tmp[128];

	if( serviceId < SONORK_SERVICE_ID_INTERNAL_LIMIT )
		return SONORK_RESULT_INVALID_PARAMETER;
		
	E=AllocIpcEntry();
	if(E==NULL)
		return SONORK_RESULT_OUT_OF_RESOURCES;
	E->serviceId  = serviceId;
	E->clientHwnd = (HWND)dta.header->link.req.clientHwnd;
	E->clientMsg = dta.header->link.req.clientMsg;
	E->eventMask = dta.header->link.req.eventMask;
	if( !IsWindow(E->clientHwnd) )
	{
		TRACE_DEBUG(" Not a valid window (%x)",E->clientHwnd);
		return SONORK_RESULT_INVALID_PARAMETER;
	}
	if( E->clientMsg < WM_APP || E->clientMsg > WM_APP+100)
	{
		TRACE_DEBUG(" Not a valid Msg(%u)",E->clientMsg);
		return SONORK_RESULT_INVALID_PARAMETER;
	}

	sprintf(tmp
		,"sonorkipcdata%s%08x"
		,SonorkApp.ConfigName()
		,E->ipcId);
	SONORK_StrToLower(tmp);

	E->dta.handle = CreateFileMapping(
			 (HANDLE)-1
			,NULL
			,PAGE_READWRITE//|SEC_COMMIT
			,0
			,SONORK_IPC_BLOCK_FULL_SIZE*2
			,tmp);
	if( E->dta.handle == NULL)
		return SONORK_RESULT_STORAGE_ERROR;

	E->dta.ptr = (BYTE*)MapViewOfFile(
			 E->dta.handle
			,FILE_MAP_ALL_ACCESS
			,0,0,0);
	if(E->dta.ptr == NULL )
	{
		CloseHandle(E->dta.handle);
		E->dta.handle=NULL;
		return SONORK_RESULT_STORAGE_ERROR;
	}
	E->dta.req = (TSonorkIpcFileBlock*)(E->dta.ptr + SONORK_IPC_BLOCK_FULL_SIZE*0);
	E->dta.evt = (TSonorkIpcFileBlock*)(E->dta.ptr + SONORK_IPC_BLOCK_FULL_SIZE*1);
	memset(E->dta.ptr,0,SONORK_IPC_BLOCK_FULL_SIZE*2);

	E->ctrlFlags=SONORK_IPC_ECF_LINKED;
	table.active++;
	dta.header->link.akn.ipcId   = E->ipcId;
	dta.header->link.akn.ipcFlags= E->ctrlFlags;
	return SONORK_RESULT_OK;
}

//-----------------------------------------------------------------------------

SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_Unlink()
{
	CondemnIpcEntry(req.entry);
	return SONORK_RESULT_OK;
}




//-----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcReq_SetServerData()
{
	BYTE*			dataPtr;
	DWORD			dataSize;
	SONORK_RESULT		result;
	BOOL			mustFreePtr;

	// Check that the requesting IPC is the one currently receiving the event
	// and that it is the GET_SERVER_DATA service event
	if( disp.entry != req.entry
	||  disp.ipcFunction!=SONORK_IPC_FUNCTION_SERVICE_EVENT
	||  disp.svcEvent.event != SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA)
	{
		result = SONORK_RESULT_INVALID_OPERATION;
		TRACE_DEBUG("ipc.SET_SERVER_DATA:INVALID_OPERATION");
	}
	else
	{
		result = IpcGetDataByCallback(SONORK_IPC_FUNCTION_GET_EVT_DATA
				, &dataPtr
				, dataSize
				, mustFreePtr);
		if( result == SONORK_RESULT_OK )
		{
			result = disp.svcEvent.data.userver->CODEC_ReadMem(dataPtr,dataSize);
			if(mustFreePtr)
			{
				SONORK_MEM_FREE(dataPtr);
			}
			if( result == SONORK_RESULT_OK)
			{
				TRACE_DEBUG("ipc.SET_SERVER_DATA:%u '%s'"
					, dataSize
					, disp.svcEvent.data.userver->name.CStr());
			}
			else
			{
				TRACE_DEBUG("ipc.SET_SERVER_DATA:%u CODEC_ERROR"
					, dataSize);
			}

		}
		else
		{
			TRACE_DEBUG("ipc.SET_SERVER_DATA:GetData(%u)=%s"
				, dataSize
				, SONORK_ResultName(result));
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

extern TSonorkServiceEntry* Service_DispatchEvent_Entry;

//-----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
  TSonorkWin32IpcServer::IpcServiceCallback(
		 SONORK_DWORD2&			handler_tag
		,SONORK_APP_SERVICE_EVENT	eventId
		,SONORK_DWORD2*			eventTag
		,TSonorkAppServiceEvent*	eventData)
{
	DWORD	result;


	ipcServer.disp.ipcFunction=SONORK_IPC_FUNCTION_SERVICE_EVENT;
	ipcServer.disp.svcEvent.event	=eventId;

	switch(eventId)
	{
		default:
		case SONORK_APP_SERVICE_EVENT_NONE:
			return 0;

		case SONORK_APP_SERVICE_EVENT_INITIALIZE:
			return 0;

		case SONORK_APP_SERVICE_EVENT_EXPORT:
		case SONORK_APP_SERVICE_EVENT_BUSY:
			ipcServer.dta.block->params.v[4]=eventData->busy.value;
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_SERVICE_EVENT_GET_NAME:
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA:

			ipcServer.disp.svcEvent.data.userver=eventData->get_server.data;
			if((ipcServer.dta.block->params.v[4]=eventData->get_server.IsRemoteQuery()) == false)
			{
				ipcServer.disp.dataSize=0;
				break;
			}
			goto load_msg_hdr;

		case SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ:
			ipcServer.dta.block->params.v[4] = eventData->circuit_req.accept_timeout;
			ipcServer.dta.block->params.v[5] = eventData->circuit_req.accept_instance;
			goto load_msg_hdr;

		case SONORK_APP_SERVICE_EVENT_POKE_DATA:
		case SONORK_APP_SERVICE_EVENT_CIRCUIT_DATA:
		case SONORK_APP_SERVICE_EVENT_QUERY_REQ:
		case SONORK_APP_SERVICE_EVENT_QUERY_AKN:
load_msg_hdr:
			ipcServer.dta.block->params.v[8]=sizeof(DWORD)*12;
			ipcServer.dta.block->params.v[9]=sizeof(TSonorkServiceHandle);
			ipcServer.dta.block->params.v[10]=sizeof(TSonorkCtrlMsg);
			ipcServer.dta.block->params.v[11]=0;


			ipcServer.disp.dataPtr=(BYTE*)&ipcServer.dta.block->params.v[12];
			memcpy(ipcServer.disp.dataPtr
				,&eventData->cur_msg.sender
				,sizeof(TSonorkServiceHandle));

			ipcServer.disp.dataPtr+=sizeof(TSonorkServiceHandle);
			memcpy(ipcServer.disp.dataPtr
				,eventData->cur_msg.msg
				,sizeof(TSonorkCtrlMsg));

			ipcServer.disp.dataPtr=(BYTE*)eventData->cur_msg.data;
			ipcServer.disp.dataSize=eventData->cur_msg.size;
			break;

		case SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN:
		case SONORK_APP_SERVICE_EVENT_CIRCUIT_UPDATE:
			ipcServer.disp.dataPtr =(BYTE*)eventData->circuit_open.handle;
			ipcServer.disp.dataSize=sizeof(TSonorkServiceQuery);
			break;

		case SONORK_APP_SERVICE_EVENT_QUERY_END:
		case SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED:
			ipcServer.dta.block->params.v[4]=eventData->circuit_close.id;
			ipcServer.dta.block->params.v[5]=eventData->circuit_close.result;
			ipcServer.disp.dataSize=0;
			break;
	}

	ipcServer.dta.block->dataSize=ipcServer.disp.dataSize;
	ipcServer.dta.block->params.v[0]=handler_tag.v[1];
	ipcServer.dta.block->params.v[1]=Service_DispatchEvent_Entry->sii.ServiceInstance();
	if( eventTag )
	{
		ipcServer.dta.block->params.v[2]=eventTag->v[0];
		ipcServer.dta.block->params.v[3]=eventTag->v[1];
	}
	else
	{
		ipcServer.dta.block->params.v[2]=
		ipcServer.dta.block->params.v[3]=0;
	}
	
	if( ipcServer.disp.dataSize != 0 && ipcServer.disp.dataPtr != NULL)
	{
		memcpy( ipcServer.dta.block->data
			, ipcServer.disp.dataPtr
			, ipcServer.disp.dataSize > SONORK_IPC_BLOCK_DATA_SIZE
				?SONORK_IPC_BLOCK_DATA_SIZE
				:ipcServer.disp.dataSize
			);
	}

	ipcServer.disp.entry=ipcServer.GetIpcEntry(handler_tag.v[0]);
	if(!ipcServer.disp.entry)return 0;

	if(!IpcDispatchMsg(
		  ipcServer.disp.entry
		, SONORK_IPC_FUNCTION_SERVICE_EVENT
		, eventId
		, result))
	{
		result=0;
	}
	ipcServer.disp.entry=NULL;
	
	if( !result )
		return 0;

	switch(eventId)
	{
		default:
			break;
		case SONORK_APP_SERVICE_EVENT_INITIALIZE:
			return 0;

		case SONORK_APP_SERVICE_EVENT_GET_NAME:
			if( ipcServer.dta.block->dataSize<1||ipcServer.dta.block->dataSize>SONORK_IPC_BLOCK_DATA_SIZE-1)
			{
				result=0;
			}
			ipcServer.dta.block->data[64]=0;
			eventData->get_name.str->Set((char*)ipcServer.dta.block->data);
			break;
		case SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA:
			TRACE_DEBUG("%08x GET_SERVER_DATA = %u '%s'"
				,handler_tag.v[0]
				,result
				,eventData->get_server.data->name.CStr()
				);
			break;

		case SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ:
			eventData->circuit_req.accept_timeout=ipcServer.dta.block->params.v[4];
			eventData->circuit_req.accept_instance=ipcServer.dta.block->params.v[5];
			return 0;
	}
	return result;
}


//-----------------------------------------------------------------------------
TSonorkIpcEntry	*
 TSonorkWin32IpcServer::GetIpcEntry(DWORD id)
{
	DWORD	index;
	TSonorkIpcEntry	*E;

	if( id )
	{
		index=(id&SONORK_IPC_ENTRY_INDEX_MASK);
		if(index<table.size)
		{
			E=(table.entry+index);
			if( E->ipcId == id )
				return E;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------

TSonorkIpcEntry	*
 TSonorkWin32IpcServer::AllocIpcEntry()
{
	DWORD	index;
	DWORD	cookie;
	TSonorkIpcEntry	*E;

	for(E=table.entry,index=0;index<table.size;index++,E++)
	{
		if( E->ctrlFlags == 0)
		{
			cookie=SonorkApp.GenSelfTrackingNo()&SONORK_IPC_COOKIE_MASK;
			if(!cookie)
				cookie=(1<<SONORK_IPC_COOKIE_SHIFT);
			E->ipcId=index|cookie;
			return E;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------

void
 TSonorkWin32IpcServer::CondemnIpcEntry( TSonorkIpcEntry * E )
{
	union{
		DWORD	dummyResult;
		TSonorkListIterator	I;
	}D;
	DWORD	ipcId;
	TSonorkServiceEntry*svcE;
	if(!(E->ctrlFlags&SONORK_IPC_ECF_CONDEMNED))
	{
		ipcId=E->ipcId;
		TRACE_DEBUG("- IpcCondemn(%x)",ipcId);
		table.active--;
		E->ctrlFlags&=~SONORK_IPC_ECF_LINKED;
		E->ctrlFlags|=SONORK_IPC_ECF_CONDEMNED;
		if(!(E->ctrlFlags&SONORK_IPC_ECF_INVALID))
		{
			IpcDispatchMsg(
				  E
				, SONORK_IPC_FUNCTION_UNLINK
				, ipcId
				, D.dummyResult);
		}
		SonorkApp.Service_BeginEnum(D.I);
		while((svcE=(TSonorkServiceEntry*)SonorkApp.Service_EnumNext(D.I,NULL))!=NULL)
		{
			if(svcE->flags&SONORK_APP_SERVICE_EF_IPC)
			if(svcE->cb_tag.v[0]==ipcId)
			{
				SonorkApp.Service_Unregister(svcE,false);
			}
		}
		SonorkApp.Service_EndEnum(D.I);
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_CLEAR_IPC_ENTRY,ipcId);
	}
}

// ----------------------------------------------------------------------------
SONORK_RESULT
  TSonorkWin32IpcServer::IpcGetDataByCallback(
			  SONORK_IPC_FUNCTION	function
			, BYTE**		dataPtr
			, DWORD&		dataSize
			, BOOL& 		mustFreePtr)
{

	DWORD	offset,blockSize;
	DWORD	unusedDispResult;
	BYTE*	ptr;
	TSonorkIpcFileBlock*	block;

	block = function == SONORK_IPC_FUNCTION_GET_REQ_DATA
			? req.reqBlock
			: req.evtBlock;

	dataSize=block->dataSize;
	if(dataSize <=  SONORK_IPC_BLOCK_DATA_SIZE )
	{
		*dataPtr = block->data;
		mustFreePtr=false;
		return SONORK_RESULT_OK;
	}
	if( dataSize > SONORK_IPC_MAX_DATA_SIZE)
	{
		*dataPtr=NULL;
		mustFreePtr=false;
		return SONORK_RESULT_QUOTA_EXCEEDED;

	}
	ptr = SONORK_MEM_ALLOC(BYTE,dataSize);
	offset   =0;
	blockSize=SONORK_IPC_BLOCK_DATA_SIZE;
	for(;;)
	{
		memcpy(  ptr+offset
			,block->data
			,blockSize);
		offset+=blockSize;
		if( offset >= dataSize )
		{
			*dataPtr=ptr;
			mustFreePtr=true;
			return SONORK_RESULT_OK;
		}

		if(!IpcDispatchMsg(req.entry
			, function
			, offset
			, unusedDispResult))
		{
			break;
		}
		if( offset + blockSize > dataSize )
			blockSize=dataSize-offset;
	}
	*dataPtr=NULL;
	SONORK_MEM_FREE(ptr);
	mustFreePtr=false;
	return SONORK_RESULT_INTERNAL_ERROR;
}

//-----------------------------------------------------------------------------

BOOL
 TSonorkWin32IpcServer::IpcPutDataAtom(
		  TSonorkIpcEntry* 		entry
		, DWORD				function
		, TSonorkIpcFileBlock*   	block
		, const TSonorkCodecAtom*	atom )
{
	BYTE		*temp_buffer;
	const BYTE	*sptr;
	DWORD		dummyResult;
	DWORD		dataSize , blockOffset, blockSize;

	block->dataSize = dataSize = atom->CODEC_Size();

	if( dataSize <= SONORK_IPC_BLOCK_DATA_SIZE )
	{
		// Data fits in a single block, no need for temporal buffer
		atom->CODEC_WriteMem(block->data,dataSize);
		return IpcDispatchMsg(entry
				, function
				, dataSize
				, dummyResult);
	}
	else
	{
		temp_buffer = SONORK_MEM_ALLOC(BYTE,dataSize);

		blockSize=SONORK_IPC_BLOCK_DATA_SIZE;
		atom->CODEC_WriteMem(temp_buffer,dataSize);

		for(  blockOffset = 0		    , sptr=temp_buffer
		    ; blockOffset < dataSize
		    ; blockOffset += blockSize	    , sptr+=blockSize)
		{
			if(blockOffset + blockSize > dataSize)
				blockSize = dataSize - blockOffset;
			memcpy(block->data , sptr , blockSize);
			if(!IpcDispatchMsg(entry
				, function
				, blockSize
				, dummyResult))
				break;
		}
		SONORK_MEM_FREE(temp_buffer);
		return blockOffset == dataSize;
	}

}

//-----------------------------------------------------------------------------

BOOL
 TSonorkWin32IpcServer::IpcPutDataRaw(
		  TSonorkIpcEntry* 		entry
		, DWORD				function
		, TSonorkIpcFileBlock*   	block
		, const void* 			data
		, DWORD 			dataSize)
{
	const BYTE	*sptr;
	DWORD		dummyResult,blockOffset,blockSize;
	block->dataSize  = dataSize;

	if( dataSize <= SONORK_IPC_BLOCK_DATA_SIZE )
	{
		// Data fits in a single block, no need for temporal buffer
		memcpy(block->data,data,dataSize);
		return IpcDispatchMsg(entry
			, function
			, dataSize
			, dummyResult);
	}
	else
	{
		blockSize=SONORK_IPC_BLOCK_DATA_SIZE;

		for(  blockOffset = 0		, sptr=(const BYTE*)data
		    ; blockOffset < dataSize
		    ; blockOffset += blockSize	, sptr+=blockSize)
		{
			if(blockOffset + blockSize > dataSize)
				blockSize = dataSize - blockOffset;

			memcpy(block->data ,sptr ,blockSize);
			if(!IpcDispatchMsg(entry
				, function
				, blockSize
				, dummyResult))
				break;
		}
		return blockOffset == dataSize;
	}
}

//-----------------------------------------------------------------------------

BOOL
  TSonorkWin32IpcServer::IpcDispatchMsg(TSonorkIpcEntry* E
			, DWORD			function
			, LPARAM 		lParam
			, DWORD& 		result)
{
	if(!(E->ctrlFlags&SONORK_IPC_ECF_INVALID))
	{
		TRACE_DEBUG("ipc.%08x! Dsp(%u) > %u"
			, E->ipcId
			, function
			, lParam);

		if(SendMessageTimeout(
			  E->clientHwnd
			, E->clientMsg
			, function
			, lParam
			, SMTO_NORMAL|SMTO_ABORTIFHUNG
			, SONORK_IPC_MESSAGE_TIMEOUT
			, &result))
		{
			TRACE_DEBUG("ipc.%08x! Dsp(%u) < %u"
				, E->ipcId
				, function
				, result);
			return true;
		}
		TRACE_DEBUG("FAILED (%u)",GetLastError());
		E->ctrlFlags|=SONORK_IPC_ECF_INVALID;
		ipcServer.CondemnIpcEntry( E );
	}
	return false;
}
//-----------------------------------------------------------------------------

LRESULT
 TSonorkWin32App::WmSonorkIpc(WPARAM wParam ,LPARAM lParam)
{
	TSonorkIpcEntry	*E;
	SONORK_RESULT	result;
	E=ipcServer.GetIpcEntry(lParam);
	TRACE_DEBUG("ipc.%08x> Req(%u) = %x",lParam,wParam,E);
	if(E!=NULL)
	{
		if( !(E->ctrlFlags&SONORK_IPC_ECF_LINKED) )
		{
			result=SONORK_RESULT_ACCESS_DENIED;
		}
		else
		if( ipcServer.req.entry != NULL )
		{
			result=SONORK_RESULT_SERVICE_BUSY;
		}
		else
		{

		ipcServer.req.entry = E;
		ipcServer.req.reqBlock = E->dta.req;
		ipcServer.req.evtBlock = E->dta.evt;

		switch(wParam)
		{
		case SONORK_IPC_FUNCTION_NONE:
			result=SONORK_RESULT_OK;
			break;

		case SONORK_IPC_FUNCTION_REGISTER_SERVICE:
			result=ipcServer.IpcReq_RegisterService();
			break;
			
		case SONORK_IPC_FUNCTION_UNREGISTER_SERVICE:
			result=ipcServer.IpcReq_UnregisterService();
			break;

		case SONORK_IPC_FUNCTION_START_SERVICE_QUERY:
			result=ipcServer.IpcReq_StartServiceQuery();
			break;

		case SONORK_IPC_FUNCTION_REPLY_SERVICE_QUERY:
			result=ipcServer.IpcReq_ReplyServiceQuery();
			break;

		case SONORK_IPC_FUNCTION_ACCEPT_SERVICE_CIRCUIT:
			result=ipcServer.IpcReq_AcceptServiceCircuit();
			break;

		case SONORK_IPC_FUNCTION_GET_SERVICE_CIRCUIT:
			result=ipcServer.IpcReq_GetServiceCircuit();
			break;

		case SONORK_IPC_FUNCTION_OPEN_SERVICE_CIRCUIT:
			result=ipcServer.IpcReq_OpenServiceCircuit();
			break;

		case SONORK_IPC_FUNCTION_CLOSE_SERVICE_CIRCUIT:
			result=ipcServer.IpcReq_CloseServiceCircuit();
			break;

		case SONORK_IPC_FUNCTION_POKE_SERVICE_DATA:
			result=ipcServer.IpcReq_PokeServiceData();
			break;

		case SONORK_IPC_FUNCTION_SET_EVENT_MASK:
			E->eventMask = ipcServer.req.reqBlock->params.v[0];
			result=SONORK_RESULT_OK;
			break;

		case SONORK_IPC_FUNCTION_PREPARE_MSG:
			result=ipcServer.IpcReq_PrepareMsg();
			break;

		case SONORK_IPC_FUNCTION_SEND_MSG:
			result=ipcServer.IpcReq_SendMsg();
			break;

                case SONORK_IPC_FUNCTION_SEND_FILES:
			result=ipcServer.IpcReq_SendFiles();
			break;

                case SONORK_IPC_FUNCTION_RECV_FILE:
			result=ipcServer.IpcReq_RecvFile();
			break;

		case SONORK_IPC_FUNCTION_GET_USER_DATA:
			result=ipcServer.IpcReq_GetUserData();
			break;

		case SONORK_IPC_FUNCTION_GET_USER_HANDLE:
			result=ipcServer.IpcReq_GetUserHandle();
			break;

		case SONORK_IPC_FUNCTION_GET_EVENT_DATA:
			result=ipcServer.IpcReq_GetEventData();
			break;
			
		case SONORK_IPC_FUNCTION_SET_SERVER_DATA:
			result=ipcServer.IpcReq_SetServerData();
			break;
			
		case SONORK_IPC_FUNCTION_ALLOC_SHARED_SLOT:
			result=ipcServer.IpcReq_AllocSharedSlot();
			break;

		case SONORK_IPC_FUNCTION_FREE_SHARED_SLOT:
			result=ipcServer.IpcReq_FreeSharedSlot();
			break;

		case SONORK_IPC_FUNCTION_UNLINK:
			result=ipcServer.IpcReq_Unlink();
			break;

		case SONORK_IPC_FUNCTION_ENUM_USER_GROUPS:
			result=ipcServer.IpcReq_EnumUserGroups();
			break;

		case SONORK_IPC_FUNCTION_ENUM_USER_LIST:
			result=ipcServer.IpcReq_EnumUserList();
			break;

		default:
			result=SONORK_RESULT_NOT_SUPPORTED;
			break;
		}

		ipcServer.req.entry=NULL;
		ipcServer.req.reqBlock =
		ipcServer.req.evtBlock = NULL;

		}
	}
	else
	if( wParam == SONORK_IPC_FUNCTION_LINK )
	{
		result=ipcServer.IpcReq_Link(lParam);
	}
	else
		result=SONORK_RESULT_INVALID_HANDLE;
	TRACE_DEBUG("ipc.%08x< Akn(%u) = %s",lParam,wParam,SONORK_ResultName(result));
	return result;
}

//-----------------------------------------------------------------------------

bool
 TSonorkWin32App::Init_Ipc()
{
	char tmp[128];
	BYTE	*ptr;
	memset(&ipcServer,0,sizeof(ipcServer));
	for(;;)
	{
		sprintf(tmp,"sonorkipclock%s",ConfigName());
		SONORK_StrToLower(tmp);
		ipcServer.dta.mutex  = CreateMutex(NULL,false,tmp);
		if(ipcServer.dta.mutex==NULL)
			break;
		sprintf(tmp,"sonorkipcdata%s",ConfigName());
		SONORK_StrToLower(tmp);
		ipcServer.dta.handle = CreateFileMapping(
				 (HANDLE)-1//appIpcArea.mmf_handle
				,NULL
				,PAGE_READWRITE//|SEC_COMMIT
				,0
				,SONORK_IPC_HEADER_FULL_SIZE+SONORK_IPC_BLOCK_FULL_SIZE
				,tmp);
		if( ipcServer.dta.handle == NULL)
			break;

		ptr = (BYTE*)MapViewOfFile(
			 ipcServer.dta.handle
			,FILE_MAP_ALL_ACCESS
			,0,0,0);
		if(ptr == NULL )
			break;

		ipcServer.dta.header =  (TSonorkIpcFullFileHeader*)(ptr);
		ipcServer.dta.block  =  (TSonorkIpcFileBlock*)(ptr+SONORK_IPC_HEADER_FULL_SIZE);

		memset(ipcServer.dta.header
			,0
			,SONORK_IPC_HEADER_FULL_SIZE+SONORK_IPC_BLOCK_FULL_SIZE);
		ipcServer.dta.header->headerFullSize 	= SONORK_IPC_HEADER_FULL_SIZE;
		ipcServer.dta.header->headerDataSize 	= SONORK_IPC_HEADER_DATA_SIZE;

		ipcServer.dta.header->sharedSlotsOffset	= SONORK_IPC_HEADER_DATA_SIZE;
		ipcServer.dta.header->sharedSlotsSize	= sizeof(TSonorkIpcSharedSlot)*SONORK_IPC_SHARED_SLOTS;

		ipcServer.dta.header->blockOffset  	= SONORK_IPC_HEADER_FULL_SIZE;
		ipcServer.dta.header->blockFullSize  	= SONORK_IPC_BLOCK_FULL_SIZE;
		ipcServer.dta.header->blockDataSize  	= SONORK_IPC_BLOCK_DATA_SIZE;
		ipcServer.dta.header->maxDataSize	= SONORK_IPC_MAX_DATA_SIZE;
		ipcServer.dta.header->serverHwnd	= (DWORD)win32.work_win;
		ipcServer.dta.header->serverMsg 	= WM_SONORK_IPC;
		ipcServer.dta.header->appVersionNumber= SONORK_CLIENT_VERSION_NUMBER;

		ipcServer.table.size = 16;
		SONORK_MEM_NEW(ipcServer.table.entry = new TSonorkIpcEntry[ipcServer.table.size] );
		SONORK_ZeroMem(ipcServer.table.entry , sizeof(TSonorkIpcEntry)*ipcServer.table.size);
		TRACE_DEBUG("IPC Started");
		return true;
	}
	Exit_Ipc();
	return false;
}

//-----------------------------------------------------------------------------

void
 TSonorkWin32App::Exit_Ipc()
{
	if( ipcServer.table.entry != NULL )
	{
		SONORK_MEM_DELETE_ARRAY( ipcServer.table.entry );
		ipcServer.table.entry=NULL;
	}
	if( ipcServer.dta.header != NULL )
	{
		UnmapViewOfFile(ipcServer.dta.header);
	}
	if( ipcServer.dta.handle != NULL )
	{
		CloseHandle(ipcServer.dta.handle);
		ipcServer.dta.handle=NULL;
	}
	if( ipcServer.dta.mutex != NULL )
	{
		CloseHandle(ipcServer.dta.mutex);
		ipcServer.dta.mutex=NULL;
	}
	memset(&ipcServer.dta,0,sizeof(ipcServer.dta));
	TRACE_DEBUG("IPC Stopped");
}
//-----------------------------------------------------------------------------

void
 TSonorkWin32App::ClearIpcEntry(DWORD id)
{
	TSonorkIpcEntry*E;
	DWORD	slotIndex;
	DWORD	slotHandle;
	TSonorkIpcSharedSlot	*slot;

	E=ipcServer.GetIpcEntry(id);
	TRACE_DEBUG("%06x IPC_ClearEntry HWND=%x PTR=%x",id,E);
	if(E==NULL)return;
	if(E->ctrlFlags==0)return;

	slotHandle=E->sharedSlotHandle;

	if( slotHandle!=0)
	{
		slotIndex	= slotHandle&SONORK_IPC_SLOT_INDEX_MASK;
		slot		= (ipcServer.dta.header->slot+slotIndex);
		if( slot->refCount )
		{
			if( (--slot->refCount) == 0)
			{
				memset(slot,0,sizeof(TSonorkIpcSharedSlot));
			}
		}
	}

	if( E->dta.handle != NULL )
	{
		if( E->dta.ptr != NULL )
		{
			UnmapViewOfFile(E->dta.ptr);
		}
		CloseHandle(E->dta.handle);
	}
	memset(E,0,sizeof(*E));
}


//-----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::BroadcastAppEventToIpc(SONORK_APP_EVENT eventId
		,UINT eventMask
		,UINT eventParam
		,void*eventData)
{
	UINT	slot,disp_count;
	DWORD	dummyResult;

	if( ipcServer.dta.header==NULL )
		return; // Ipc did not initialize or no IPC entries active

	ipcServer.disp.ipcFunction=SONORK_IPC_FUNCTION_APP_EVENT;
	ipcServer.disp.appEvent.event=eventId;

	switch( eventId )
	{
		default:
		case SONORK_APP_EVENT_OLD_CTRL_MSG:
		case SONORK_APP_EVENT_NONE:
		case SONORK_APP_EVENT_DESKTOP_SIZE_CHANGE:
		case SONORK_APP_EVENT_START_WAPP:
		case SONORK_APP_EVENT_USER_MTPL_CHANGE:
		case SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK:
		case SONORK_APP_EVENT_SYS_DIALOG:
			// Not mapped to IPC
			return;

		case SONORK_APP_EVENT_SHUTDOWN:
		case SONORK_APP_EVENT_FLUSH_BUFFERS:
		case SONORK_APP_EVENT_MAINTENANCE:
		case SONORK_APP_EVENT_SET_PROFILE:
		case SONORK_APP_EVENT_SKIN_COLOR_CHANGED:
		case SONORK_APP_EVENT_SET_LANGUAGE:
			// No parameters, no data
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_OPEN_PROFILE:
			if(IsProfileOpen())
				ipcServer.dta.header->userId.Set(ProfileUserId());
			else
				ipcServer.dta.header->userId.Clear();
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_CX_STATUS:
			ipcServer.dta.header->appStatus=CxStatus();
			ipcServer.disp.dataSize=0;
			break;


		case SONORK_APP_EVENT_SID_CHANGED:

			ipcServer.dta.block->params.d[4].Set(SonorkApp.ProfileSidFlags().v[0]);
			ipcServer.dta.block->params.d[5].Set(SonorkApp.ProfileSidFlags().v[1]);
			ipcServer.disp.dataPtr=(BYTE*)&profile_user.addr;
			ipcServer.disp.dataSize=sizeof(TSonorkNewUserAddr);
			break;

		case SONORK_APP_EVENT_USER_SID:
			if( eventParam == SONORK_APP_EVENT_USER_SID_LOCAL )
			{
#define auxE	((TSonorkAppEventLocalUserSid*)eventData)
				ipcServer.dta.block->params.d[4].Set(auxE->userData->userId);
				ipcServer.dta.block->params.d[5].v[0]= auxE->oldSidMode;
				ipcServer.dta.block->params.d[5].v[1]= auxE->onlineDir;
				memcpy(&ipcServer.dta.block->params.d[6]
					,&auxE->userData->addr
					,sizeof(auxE->userData->addr));
#undef  auxE
			}
			else
			if( eventParam == SONORK_APP_EVENT_USER_SID_REMOTE )
			{
#define auxE	((TSonorkAppEventRemoteUserSid*)eventData)
				ipcServer.dta.block->params.d[4].Set(auxE->userId);
				ipcServer.dta.block->params.d[5].Set(auxE->sid);
				ipcServer.dta.block->params.d[6].Set(auxE->sidFlags.v[0]);
				ipcServer.dta.block->params.d[7].Set(auxE->sidFlags.v[1]);
#undef  auxE

			}
			else
				return;
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_USER_UTS:
			ipcServer.dta.block->params.d[4].Set( *(TSonorkId*)eventData );
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_SET_USER:
			ipcServer.dta.block->params.d[4].Set(
				((TSonorkAppEventSetUser*)eventData)->user_id);
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_DEL_USER:
			ipcServer.dta.block->params.d[4].Set(*(TSonorkId*)eventData);
			ipcServer.disp.dataSize=0;
			break;

		case SONORK_APP_EVENT_SONORK_MSG_RCVD:
		case SONORK_APP_EVENT_SONORK_MSG_SENDING:
		case SONORK_APP_EVENT_SONORK_MSG_SENT:

			// Note: <d> are 2 dwords, so d[4] = v[8]
			ipcServer.dta.block->params.v[8]=
				((TSonorkAppEventMsg*)eventData)->pcFlags;

			ipcServer.dta.block->params.v[9]=
				((TSonorkAppEventMsg*)eventData)->ccFlags;

			ipcServer.dta.block->params.v[10]=
				((TSonorkAppEventMsg*)eventData)->handleId;

			ipcServer.dta.block->params.v[11]=
				((TSonorkAppEventMsg*)eventData)->taskId;

			ipcServer.dta.block->params.v[13]=0;

			memcpy(&ipcServer.dta.block->params.v[16]
				, &((TSonorkAppEventMsg*)eventData)->mark
				, sizeof(TSonorkCCacheMark));

			if( eventId == SONORK_APP_EVENT_SONORK_MSG_SENT )
			{
				// No extended data
				ipcServer.dta.block->params.v[12]=
					((TSonorkAppEventMsg*)eventData)->ERR->result;
			}
			else
			{
				// The extended data is the message
				ipcServer.dta.block->params.v[12]=0;
				ipcServer.disp.appEvent.data.msg = ((TSonorkAppEventMsg*)eventData)->msg;
			}
			ipcServer.disp.dataPtr =(BYTE*)((TSonorkAppEventMsg*)eventData)->header;
			ipcServer.disp.dataSize=sizeof(TSonorkMsgHeader);

			break;

		case SONORK_APP_EVENT_SONORK_MSG_PROCESSED:
			// Note: <d> are 2 dwords, so d[4] = v[8]
			ipcServer.dta.block->params.v[8]=
                        	((TSonorkAppEventMsgProcessed*)eventData)->userId.v[0];
			ipcServer.dta.block->params.v[9]=
                        	((TSonorkAppEventMsgProcessed*)eventData)->userId.v[1];
			ipcServer.dta.block->params.v[10]=
				((TSonorkAppEventMsgProcessed*)eventData)->pcFlags;
			ipcServer.dta.block->params.v[11]=
				((TSonorkAppEventMsgProcessed*)eventData)->ccFlags;
			ipcServer.dta.block->params.v[12]=
				((TSonorkAppEventMsgProcessed*)eventData)->ccMask;
			if( ((TSonorkAppEventMsgProcessed*)eventData)->extIndex )
			{
				ipcServer.dta.block->params.v[13]=1;
				ipcServer.dta.block->params.v[14]= *((TSonorkAppEventMsgProcessed*)eventData)->extIndex ;
			}
			else
			{
				ipcServer.dta.block->params.v[13]=0;
			}
			ipcServer.dta.block->params.v[15]=0;

			memcpy(&ipcServer.dta.block->params.v[16]
				, ((TSonorkAppEventMsgProcessed*)eventData)->markPtr
				, sizeof(TSonorkCCacheMark));

			ipcServer.disp.dataSize=0;
			break;



		case SONORK_APP_EVENT_MAIN_VIEW_SELECTION:
			return;
	}
	if(ipcServer.table.active==0)
		return;
	ipcServer.dta.header->eventSerial+=1;
	ipcServer.dta.block->params.v[0]=eventParam;
	ipcServer.dta.block->params.v[1]=
	ipcServer.dta.block->params.v[2]=
	ipcServer.dta.block->params.v[3]=0;
	ipcServer.dta.block->dataSize=ipcServer.disp.dataSize;
	disp_count=slot=0;

	if( ipcServer.disp.dataSize != 0 && ipcServer.disp.dataPtr != NULL)
	{
		memcpy( ipcServer.dta.block->data
			, ipcServer.disp.dataPtr
			, ipcServer.disp.dataSize > SONORK_IPC_BLOCK_DATA_SIZE
				?SONORK_IPC_BLOCK_DATA_SIZE
				:ipcServer.disp.dataSize
			);
	}

	for(ipcServer.disp.entry=ipcServer.table.entry
	     ;slot<ipcServer.table.size && disp_count<ipcServer.table.active
	     ;ipcServer.disp.entry++,slot++)
	{

		if(!(ipcServer.disp.entry->ctrlFlags&SONORK_IPC_ECF_LINKED))
			continue;
		disp_count++;
		if( !(eventMask&ipcServer.disp.entry->eventMask) )
			continue;
		ipcServer.IpcDispatchMsg(
			  ipcServer.disp.entry
			, SONORK_IPC_FUNCTION_APP_EVENT
			, eventId
			, dummyResult);

	}
	ipcServer.disp.entry=NULL;
}

