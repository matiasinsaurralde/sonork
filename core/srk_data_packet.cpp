#include "srk_defs.h"
#pragma hdrstop
#include "srk_data_packet.h"
#include "srk_codec_io.h"

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

#define DECODE_AKN_SECTION_STARTS(psize) \
			TSonorkPacketReader CODEC(DataPtr(),(psize)-sizeof(header));\
			if( !(header.packet_ctrl & (SONORK_PCTRL_MARK_ERROR|SONORK_PCTRL_MARK_NO_DATA)) )\
			{

#define DECODE_AKN_SECTION_ENDS	\
			}\
			return CODEC.CodecOk()


#define DECODE_REQ_SECTION_STARTS(sz)	TSonorkPacketReader CODEC(DataPtr(),(sz)-sizeof(header))

#define DECODE_REQ_SECTION_ENDS		return CODEC.CodecOk()


#define DECODE_EVT_SECTION_STARTS(n)	DECODE_AKN_SECTION_STARTS(n)
#define DECODE_EVT_SECTION_ENDS		DECODE_AKN_SECTION_ENDS

#define ENCODE_REQ_SECTION_STARTS(func,psize,subfunc_ver)\
			InitHeader(func,SONORK_PACKET_TYPE_REQ|(subfunc_ver));\
			TSonorkPacketWriter CODEC(DataPtr(),psize);\
			{

#define ENCODE_EVT_SECTION_STARTS(evt,psize,subfunc_ver)\
			InitHeader(evt,subfunc_ver);\
			TSonorkPacketWriter  CODEC(DataPtr(),psize);\
			{\


#define ENCODE_AKN_SECTION_STARTS(func,psize,subfunc_ver)\
			InitHeader(func,SONORK_PACKET_TYPE_AKN|(subfunc_ver));\
			TSonorkPacketWriter  CODEC(DataPtr(),psize);\
			{


#define ENCODE_SECTION_ENDS \
			}\
			return CODEC.CodecOk()?sizeof(header)+CODEC.Size():0

inline void TSonorkDataPacket::InitHeader(SONORK_FUNCTION pFunction,DWORD subfunc_ver_type)
{
	header.function=(WORD)pFunction;
	header.packet_ctrl=(WORD)(subfunc_ver_type&(SONORK_PCTRL_TYPE_MASK|SONORK_PCTRL_VERSION_MASK|SONORK_PCTRL_SUBFUNCTION_MASK) );
}

inline void TSonorkDataPacket::InitHeader(SONORK_SERVER_CLIENT_EVENT pEvent,DWORD subfunc_ver)
{
	header.function=(WORD)pEvent;
	header.packet_ctrl=(WORD)
					( (subfunc_ver&(SONORK_PCTRL_VERSION_MASK|SONORK_PCTRL_SUBFUNCTION_MASK) )
					| SONORK_PACKET_TYPE_EVT );
}



// ----------------------------------------------------------------------------
// SysMsg

#if defined(SONORK_SERVER_BUILD)
bool TSonorkDataPacket::D_GetSysMsgs_R(UINT sz, TSonorkTime& time, TSonorkRegion& region)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&time);
		CODEC.ReadDW2(&region);
	DECODE_REQ_SECTION_ENDS;

}

#endif

#if defined(SONORK_CLIENT_BUILD)
UINT TSonorkDataPacket::E_GetSysMsgs_R(UINT sz, const TSonorkTime& time, const TSonorkRegion& region)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_GET_SYS_MSGS,sz,GUDPCV_GET_SYS_MSGS);
		CODEC.WriteDW2(&time);
		CODEC.WriteDW2(&region);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_GetSysMsgs_A(UINT sz, TSonorkSysMsgQueue& queue)
{
	DWORD items;
	TSonorkSysMsg*DS;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while(CODEC.CodecOk() && items-- )
		{
			DS = new TSonorkSysMsg;
			CODEC.Read(DS);
			if( CODEC.CodecOk() )
				queue.Add(DS);
			else
				SONORK_MEM_DELETE( DS );
		}
	DECODE_AKN_SECTION_ENDS;
}
#endif

// ----------------------------------------------------------------------------
// GetGuWapp

#if defined(SONORK_SERVER_BUILD)

bool TSonorkDataPacket::D_WappUrl_R(UINT sz,DWORD*app_no,DWORD* app_id)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(app_no);
		CODEC.ReadDW(app_id);
	DECODE_REQ_SECTION_ENDS;
}
UINT TSonorkDataPacket::E_WappUrl_A(UINT sz
	,const TSonorkShortString& url
	,DWORD app_pin)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_WAPP_URL,sz,GUDPCV_WAPP_URL);
		CODEC.Write(&url);
		CODEC.WriteDW(app_pin);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT TSonorkDataPacket::E_WappUrl_R(UINT sz,DWORD app_id,DWORD url_id)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_WAPP_URL,sz,GUDPCV_GET_WAPP);
		CODEC.WriteDW(app_id);
		CODEC.WriteDW(url_id);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_WappUrl_A(UINT sz,TSonorkShortString& url,DWORD& app_pin)
{
   DECODE_AKN_SECTION_STARTS(sz);
		CODEC.Read(&url);
		CODEC.ReadDW(&app_pin);
   DECODE_AKN_SECTION_ENDS;
}

#endif

// ----------------------------------------------------------------------------
// SetPass

#if defined(SONORK_SERVER_BUILD)
bool TSonorkDataPacket::D_SetPass_R(UINT sz,TSonorkShortString& pass)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.Read(&pass);
	DECODE_REQ_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT TSonorkDataPacket::E_SetPass_R(UINT sz, const TSonorkShortString& pass)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_PASS,sz,GUDPCV_SET_PASS);
		CODEC.Write(&pass);
	ENCODE_SECTION_ENDS;
}

#endif


// ----------------------------------------------------------------------------
// CreateUser

#if defined(SONORK_SERVER_BUILD)
bool TSonorkDataPacket::D_CreateUser0_R(UINT sz,TSonorkUserDataNotes&UDN)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadUDN(&UDN);
	DECODE_REQ_SECTION_ENDS;
}
bool TSonorkDataPacket::D_CreateUser1_R(UINT sz
	,DWORD&				referrer_id
	,TSonorkUserDataNotes& 		UDN
	,TSonorkDynData& 		data)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(&referrer_id);
		CODEC.ReadUDN(&UDN);
		CODEC.Read(&data);
	DECODE_REQ_SECTION_ENDS;
}
UINT TSonorkDataPacket::E_CreateUser_A(UINT sz
	,const TSonorkId& user_id)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_CREATE_USER,sz,0);
		CODEC.WriteDW2(&user_id);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT TSonorkDataPacket::E_CreateUser1_R(UINT sz
	,DWORD 						referrer_id
	,const TSonorkUserDataNotes& 	UDN
	,const TSonorkDynData& 			data)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_CREATE_USER,sz,1);
		CODEC.WriteDW(referrer_id);
		CODEC.WriteUDN(&UDN,SONORK_UDN_ENCODE_NOTES|SONORK_UDN_ENCODE_PASSWORD);
		CODEC.Write(&data);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_CreateUser_A(UINT sz,TSonorkId& user_id)
{
   DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
   DECODE_AKN_SECTION_ENDS;
}
#endif

// ----------------------------------------------------------------------------
// User servers

// ----------------------------------------------------------------------------
// AddUserServer

#if defined(SONORK_CLIENT_BUILD) 
UINT
  TSonorkDataPacket::E_AddUserServer_R(UINT sz,const TSonorkUserServer& data)
{
 ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_ADD_USERVER, sz , GUDPCV_ADD_USERVER);
	CODEC.Write(&data);
 ENCODE_SECTION_ENDS;
}

UINT
  TSonorkDataPacket::E_SetUserServer_R(UINT sz,const TSonorkUserServer& data)
{
 ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_USERVER, sz , GUDPCV_SET_USERVER);
	CODEC.Write(&data);
 ENCODE_SECTION_ENDS;
}
#endif

#if defined(SONORK_SERVER_BUILD)

bool
 TSonorkDataPacket::D_AddUserServer_R(UINT sz,TSonorkUserServer& data)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.Read(&data);
	DECODE_REQ_SECTION_ENDS;
}
bool
 TSonorkDataPacket::D_SetUserServer_R(UINT sz,TSonorkUserServer& data)
{
	return D_AddUserServer_R(sz,data);
}
#endif


#if defined(SONORK_CLIENT_BUILD) 
UINT
  TSonorkDataPacket::E_ListUserServers_R(UINT sz,const TSonorkUserServer& data, DWORD max_count)
{
 ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_LIST_USERVERS,sz,GUDPCV_LIST_USERVERS);
	CODEC.WriteDW(max_count);
	CODEC.WriteDW(0);
	CODEC.Write(&data);
 ENCODE_SECTION_ENDS;
}
#endif

#if defined(SONORK_SERVER_BUILD)
bool
 TSonorkDataPacket::D_ListUserServers_R(UINT sz, TSonorkUserServer& data, DWORD& flags, DWORD& max_count)
{
 DECODE_REQ_SECTION_STARTS(sz);
	CODEC.ReadDW(&max_count);
	CODEC.ReadDW(&flags);
	CODEC.Read(&data);
 DECODE_REQ_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_ListUserServers_A(UINT sz,TSonorkUserServerQueue& queue)
{
	TSonorkUserServer *data;
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_LIST_USERVERS,sz, GUDPCV_NONE);
		CODEC.WriteDW(queue.Items());
		while( (data = queue.RemoveFirst()) != NULL )
		{
			CODEC.Write(data);
			SONORK_MEM_DELETE(data);
		}
	ENCODE_SECTION_ENDS;
}

#endif

#if defined(SONORK_CLIENT_BUILD) 
bool
   TSonorkDataPacket::D_ListUserServers_A(UINT sz,TSonorkUserServerQueue& queue)
{
	DWORD 		items;
	TSonorkUserServer *data;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while( items-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(data = new TSonorkUserServer);
			CODEC.Read(data);

			if( CODEC.CodecOk() )
				queue.Add(data);
			else
				SONORK_MEM_DELETE(data);
		}
	DECODE_AKN_SECTION_ENDS;
}
#endif


#if defined(SONORK_CLIENT_BUILD) 

UINT
  TSonorkDataPacket::E_DelUserServer_R(UINT sz,const TSonorkSid& sid,DWORD server_no)
{
 ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_DEL_USERVER, sz , 0);
	CODEC.WriteDW2(&sid);
	CODEC.WriteDW(server_no);
	CODEC.WriteDW(0);
 ENCODE_SECTION_ENDS;
}
#endif

#if defined(SONORK_SERVER_BUILD)
bool
 TSonorkDataPacket::D_DelUserServer_R(UINT sz,TSonorkSid& sid,DWORD& server_no)
{
 DECODE_REQ_SECTION_STARTS(sz);
	CODEC.ReadDW2(&sid);
	CODEC.ReadDW(&server_no);
 DECODE_REQ_SECTION_ENDS;
}
#endif

// ----------------------------------------------------------------------------
// AddService

#if defined(SONORK_SERVER_BUILD)

bool
 TSonorkDataPacket::D_AddService_R(UINT sz,TSonorkServiceData& data)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.Read(&data);
	DECODE_REQ_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_AddService_A(UINT sz
			,const TSonorkServiceLocus1& 	locus)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_ADD_SERVICE,sz, GUDPCV_NONE);
		CODEC.WriteSL1(&locus);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT TSonorkDataPacket::E_AddService_R(UINT sz,const TSonorkServiceData& data)
{
 ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_ADD_SERVICE, sz , GUDPCV_ADD_SERVICE);
	CODEC.Write(&data);
 ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_AddService_A(UINT 	sz
			,TSonorkServiceLocus1& 	locus)
{
   DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadSL1(&locus);
   DECODE_AKN_SECTION_ENDS;
}
#endif


// ----------------------------------------------------------------------------
// ListServices

#if defined(SONORK_SERVER_BUILD)

bool TSonorkDataPacket::D_ListServices_R(UINT sz
	,TSonorkServiceInstanceInfo&	info
	,TSonorkServiceState&		state
	, DWORD& 			max_count)
{

	TSonorkOldServiceServerExt	oldS;
	DECODE_REQ_SECTION_STARTS(sz);
	if( ReqVersion()< GUDPCV_LIST_SERVICES )
	{
		CODEC.ReadDWN((DWORD*)&oldS , SIZEOF_IN_DWORDS(oldS) );
		CODEC.ReadDW(&max_count);

		info.SetInstanceInfo(
			  SONORK_SERVICE_ID_NONE
			, oldS.ServiceType()
			, 0
			, 0
			, 0);
		state.SetState(
			SONORK_SERVICE_STATE_AVAILABLE
		,	SONORK_SERVICE_PRIORITY_LOWEST);
	}
	else
	{
		CODEC.ReadDW(&max_count);
		CODEC.ReadDWN((DWORD*)&info,SIZEOF_IN_DWORDS(info));
		CODEC.ReadDW2(&state);

	}
	DECODE_REQ_SECTION_ENDS;
}
UINT TSonorkDataPacket::E_ListServices_A(UINT sz, TSonorkServiceDataQueue& list)
{
	TSonorkServiceData *pData;
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_LIST_SERVICES,sz,GUDPCV_NONE);
		CODEC.WriteDW(list.Items());
		while( (pData = list.RemoveFirst()) != NULL )
		{
			CODEC.Write(pData);
			SONORK_MEM_DELETE(pData);
		}
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

// Must support old servers
UINT
 TSonorkDataPacket::E_OldListServices_R(UINT sz
		,SONORK_SERVICE_TYPE		service_type
		,DWORD 				max_count
		)
{
	TSonorkOldServiceServerExt	oldS;
	oldS.SetInstance(0,0);
	oldS.SetService(service_type, 0);
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_LIST_SERVICES,sz,0);
		CODEC.WriteDWN((const DWORD*)&oldS , SIZEOF_IN_DWORDS(oldS) );
		CODEC.WriteDW(max_count);
	ENCODE_SECTION_ENDS;
}
UINT
 TSonorkDataPacket::E_NewListServices_R(UINT sz
		, SONORK_SERVICE_TYPE		service_type
		, DWORD				service_instance
		, DWORD				min_version
		, DWORD 			max_count
		, SONORK_SERVICE_STATE		min_state
		, SONORK_SERVICE_PRIORITY       min_prio)
{
	TSonorkServiceInstanceInfo      info;
	TSonorkServiceState		state;
	info.SetInstanceInfo(
			  SONORK_SERVICE_ID_NONE
			, service_type
			, 0
			, service_instance
			, min_version);
	state.Set(min_state,min_prio);

	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_LIST_SERVICES,sz,GUDPCV_LIST_SERVICES);
		CODEC.WriteDW(max_count);
		CODEC.WriteDWN((DWORD*)&info,SIZEOF_IN_DWORDS(info));
		CODEC.WriteDW2(&state);
	ENCODE_SECTION_ENDS;

}



bool TSonorkDataPacket::D_ListServices_A(UINT sz, TSonorkServiceDataQueue& queue)
{
	DWORD 		items;
	TSonorkServiceData*data;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while( items-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(data = new TSonorkServiceData);
			CODEC.Read(data);

			if( CODEC.CodecOk() )
				queue.Add(data);
			else
				SONORK_MEM_DELETE(data);
		}
	DECODE_AKN_SECTION_ENDS;
}
#endif


// ----------------------------------------------------------------------------
// SearchUser

#if defined(SONORK_CLIENT_BUILD)
UINT
 TSonorkDataPacket::E_SearchUser_R_NEW(UINT sz
			, const TSonorkUserDataNotes&	UDN
			, DWORD				udn_flags
			, DWORD				search_flags
			, DWORD 			max_count)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SEARCH_USER_NEW,sz,GUDPCV_SEARCH_USER);
		CODEC.WriteDW(max_count);
		CODEC.WriteDW(search_flags);
		CODEC.WriteUDN(&UDN,udn_flags);
	ENCODE_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_SearchUser_R_OLD(UINT sz
			, const TSonorkId&		user_id
			, const TSonorkShortString&	alias
			, const TSonorkShortString&	name
			, const TSonorkShortString&	email
			, DWORD 			max_count)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SEARCH_USER_OLD,sz,GUDPCV_SEARCH_USER);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteDW(max_count);
		CODEC.Write(&alias);
		CODEC.Write(&name);
		CODEC.Write(&email);
	ENCODE_SECTION_ENDS;
}


#endif

#if defined(SONORK_SERVER_BUILD)

bool TSonorkDataPacket::D_SearchUser_R_NEW(UINT sz
			, TSonorkUserDataNotes&	UDN
			, DWORD& search_flags
			, DWORD& max_count)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(&max_count);
		CODEC.ReadDW(&search_flags);
		CODEC.ReadUDN(&UDN);
	DECODE_REQ_SECTION_ENDS;
}

bool TSonorkDataPacket::D_SearchUser_R_OLD(UINT sz
			, TSonorkId& user_id
			, TSonorkShortString& alias
			, TSonorkShortString& name
			, TSonorkShortString& email
			, DWORD& max_count)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadDW(&max_count);
		CODEC.Read(&alias);
		CODEC.Read(&name);
		CODEC.Read(&email);
	DECODE_REQ_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_SearchUser_A(UINT sz
	, TSonorkUserDataNotesQueue& list
	, UINT enc_flags
	, SONORK_FUNCTION p_func)
{
	TSonorkUserDataNotes *DN;
	ENCODE_AKN_SECTION_STARTS(p_func,sz,0);
		CODEC.WriteDW(list.Items());
		while( (DN = list.RemoveFirst()) != NULL)
		{
			CODEC.WriteUDN(DN , enc_flags);
			SONORK_MEM_DELETE(DN);
		}
	ENCODE_SECTION_ENDS;
}

#endif

#if defined(SONORK_CLIENT_BUILD)


bool TSonorkDataPacket::D_SearchUser_A(
	UINT 			sz
	,TSonorkUserDataNotesQueue& list)
{
	DWORD 				items;
	TSonorkUserDataNotes   *DN;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while( items-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(DN = new TSonorkUserDataNotes);
			// If one of the two next lines fail, ON_DECODE_AKN_FAILURE_STARTS
			// will be executed, where we delete <DN>
			CODEC.ReadUDN( DN );
			if( CODEC.CodecOk() )
				list.Add(DN);
			else
				SONORK_MEM_DELETE( DN );
		}
	DECODE_AKN_SECTION_ENDS;
}
#endif



// ----------------------------------------------------------------------------
// CtrlMsg

#if defined(SONORK_SERVER_BUILD)

// Ctrl msg event
UINT TSonorkDataPacket::E_CtrlMsg_E(
 	 UINT 			sz
	,const TSonorkUserLocus1*	locus
	,const void*		data
	,UINT 			data_size)
{
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_CTRL_MSG,sz,GUDPCV_NONE);
		CODEC.WriteUL1(locus);
		CODEC.WriteRaw(data,data_size);
   ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT
 TSonorkDataPacket::E_CtrlMsg_R(
	 UINT 				sz
	,const TSonorkUserLocus1*	locus
	,const TSonorkCtrlMsg* 		msg
	,const void*			data
	,DWORD				data_size)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_CTRL_MSG,sz,GUDPCV_CTRL_MSG);
		CODEC.WriteUL1(locus);
		CODEC.WriteCM1(msg);
		CODEC.WriteDW(data_size);
		if(data_size != 0)
			CODEC.WriteRaw( data , data_size );
	ENCODE_SECTION_ENDS;

}

bool TSonorkDataPacket::D_CtrlMsg_E(
	 UINT 			sz
	,TSonorkUserLocus1*	locus
	,TSonorkCtrlMsg* 	msg
	,TSonorkDynData*	data)
{
	DWORD data_size;
	DECODE_EVT_SECTION_STARTS(sz);
		CODEC.ReadUL1(locus);
		CODEC.ReadCM1(msg);
		CODEC.ReadDW(&data_size);
		data->SetDataSize(data_size,true);
		if(data_size != 0)
			CODEC.ReadRaw(data->wBuffer(),data_size);
	DECODE_EVT_SECTION_ENDS;
}

#endif

// ----------------------------------------------------------------------------
// IdentifyUser

#if defined(SONORK_SERVER_BUILD)

bool
	TSonorkDataPacket::D_IdentifyUser_R(
	UINT 				sz
	,TSonorkUserLocus1&		locus
	,SONORK_DWORD4&			pin
	,DWORD&				pin_type
	,DWORD&				service_id
	,DWORD&				service_pin)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadUL1(&locus);
		CODEC.ReadDW(&service_id);
		CODEC.ReadDW(&service_pin);
		CODEC.ReadDW(&pin_type);
		CODEC.ReadDW4(&pin);
	DECODE_REQ_SECTION_ENDS;
}

// E_IdentifyUser_A encodes <user_data>
//  ONLY if SONORK_IDENTIFY_MATCH_OK(<id_flags>) is true
//  See gu_defs.h for definition of SONORK_IDENTIFY_MATCH_OK.

UINT
	TSonorkDataPacket::E_IdentifyUser_A(
	UINT 				sz
	,const TSonorkUserData&	UD
	,DWORD				id_flags)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_IDENTIFY_USER,sz,GUDPCV_NONE);
		CODEC.WriteDW(id_flags);
		if( SONORK_IDENTIFY_MATCH_OK(id_flags) )
			CODEC.Write(&UD);

	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)


UINT
	TSonorkDataPacket::E_IdentifyUser_R(
	UINT					sz
	,const TSonorkUserLocus1&	locus
	,const SONORK_DWORD4& 		pin
	,SONORK_PIN_TYPE			pin_type
	,SONORK_SERVICE_ID			service_id
	,DWORD					service_pin
	)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_IDENTIFY_USER,sz,GUDPCV_IDENTIFY_USER);
		CODEC.WriteUL1(&locus);
		CODEC.WriteDW(service_id);
		CODEC.WriteDW(service_pin);
		CODEC.WriteDW(pin_type);
		CODEC.WriteDW4(&pin);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_IdentifyUser_A(
	UINT 			sz
	,TSonorkUserData&	UD
	,DWORD&			id_flags)
{
	DECODE_AKN_SECTION_STARTS(sz);

		CODEC.ReadDW(&id_flags);
		if( SONORK_IDENTIFY_MATCH_OK(id_flags) )
			CODEC.Read(&UD);
		else
			UD.Clear();

	DECODE_AKN_SECTION_ENDS;
}

#endif

// ----------------------------------------------------------------------------
// SetSid

#if defined(SONORK_SERVER_BUILD)

bool	 TSonorkDataPacket::D_SetSid_R(UINT size
		,TSonorkSidFlags& 	sid_flags
		,TSonorkRegion&		region
		,TSonorkPhysAddr&	service_address
		,TSonorkDynString&	sid_msg)
{
	DECODE_REQ_SECTION_STARTS(size);
		CODEC.ReadDW4(&sid_flags);
		CODEC.ReadDW2(&region);
		CODEC.ReadPA1(&service_address);
		if( ReqVersion() >= SDPLV_SET_SID_MSG )
			CODEC.Read(&sid_msg);
		else
			sid_msg.Clear();
	DECODE_REQ_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_SetSid_A(UINT sz
	, const TSonorkSidFlags&	sid_flags
	, const TSonorkRegion&		region
	, const TSonorkPhysAddr&	phys_addr)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_SET_SID,sz,GUDPCV_SET_SID);
		CODEC.WriteDW4(&sid_flags);
		CODEC.WriteDW2(&region);
		CODEC.WritePA1(&phys_addr);
	ENCODE_SECTION_ENDS;

}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT TSonorkDataPacket::E_SetSid_R(UINT sz
	,const TSonorkSidFlags& 	sid_flags
	,const TSonorkRegion&		region
	,const TSonorkPhysAddr&		service_address
	,const TSonorkDynString& 	sid_msg)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_SID,sz,GUDPCV_SET_SID);
		CODEC.WriteDW4(&sid_flags);
		CODEC.WriteDW2(&region);
		CODEC.WritePA1(&service_address);
		CODEC.Write(&sid_msg);
	ENCODE_SECTION_ENDS;
}

bool	 TSonorkDataPacket::D_SetSid_A(UINT size
	, TSonorkSidFlags& 	sid_flags
	, TSonorkRegion& 	region
	, TSonorkPhysAddr&	phys_addr)
{
	DECODE_AKN_SECTION_STARTS(size);
		CODEC.ReadDW4(&sid_flags);
		CODEC.ReadDW2(&region);
		CODEC.ReadPA1(&phys_addr);
	DECODE_AKN_SECTION_ENDS;
}
#endif

// ----------------------------------------------------------------------------
// SetUserData

#if defined(SONORK_SERVER_BUILD)
bool	TSonorkDataPacket::D_SetUserData_R(UINT sz, TSonorkUserDataNotes&UDN)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadUDN(&UDN);
	DECODE_REQ_SECTION_ENDS;
}
UINT	TSonorkDataPacket::E_SetUserData_A(UINT sz
	, const TSonorkSerial& serial)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_SET_USER_DATA,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&serial);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)


UINT	TSonorkDataPacket::E_SetUserData_R(UINT sz,TSonorkUserDataNotes&UDN, UINT udn_flags)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_USER_DATA,sz,GUDPCV_SET_USER_DATA);
		CODEC.WriteUDN(&UDN, udn_flags);
	ENCODE_SECTION_ENDS;
}


bool	TSonorkDataPacket::D_SetUserData_A(
		UINT 				sz
		,TSonorkSerial&			serial)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&serial);
	DECODE_AKN_SECTION_ENDS;
}

#endif

// ----------------------------------------------------------------------------
// GetUserData

#if defined(SONORK_SERVER_BUILD)
bool TSonorkDataPacket::D_GetUserData_R(
	 UINT 				sz
	,TSonorkId&			user_id
	,DWORD&				level
	,DWORD&				flags
	,DWORD&				pin
	,DWORD&				reserved)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadDW(&level);
		CODEC.ReadDW(&flags);
		CODEC.ReadDW(&pin);
		CODEC.ReadDW(&reserved);
	DECODE_REQ_SECTION_ENDS;

}

UINT  TSonorkDataPacket::E_GetUserData_A(
	 UINT 						sz
	,const TSonorkUserDataNotes&	ND
	,UINT						enc_flags)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_GET_USER_DATA,sz,GUDPCV_NONE);
		CODEC.WriteUDN(&ND,enc_flags);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)


UINT	TSonorkDataPacket::E_GetUserData_R(
	 UINT					sz
	,const TSonorkId&			user_id
	,SONORK_USER_INFO_LEVEL		level
	,DWORD					flags
	,DWORD					pin
	,DWORD					reserved)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_GET_USER_DATA,sz,GUDPCV_GET_USER_DATA);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteDW(level);
		CODEC.WriteDW(flags);
		CODEC.WriteDW(pin);
		CODEC.WriteDW(reserved);
	ENCODE_SECTION_ENDS;
}


bool TSonorkDataPacket::D_GetUserData_A(
	 UINT 				sz
	,TSonorkUserDataNotes&	ND)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadUDN(&ND);
	DECODE_AKN_SECTION_ENDS;
}

#endif


SONORK_RESULT	TSonorkDataPacket::AknResult()
{
	if( IsErrorMarkSet() )
	{
		WORD *R=(WORD*)DataPtr( sizeof(DWORD) );
		return (SONORK_RESULT)SONORK_WORD(*R);
	}
	else
		return SONORK_RESULT_OK;
}



SONORK_RESULT  TSonorkDataPacket::E_Error(TSonorkCodecWriter&CODEC, const TSonorkError&E)
{
	if(E.Result()!=SONORK_RESULT_OK)
	{
		// Set ERROR and NO_DATA flag
		header.packet_ctrl|=(SONORK_PCTRL_MARK_ERROR|SONORK_PCTRL_MARK_NO_DATA);
		CODEC.Write(&E);
		return E.Result();
	}
	else
	{
		// Clear ERROR and leave NO_DATA flag as it is
		header.packet_ctrl&=~SONORK_PCTRL_MARK_ERROR;
		return SONORK_RESULT_OK;
	}
}



// ----------------------------------------------------------------------------
// Sid Event

#if defined(SONORK_SERVER_BUILD)

// Old Sid event
UINT
 TSonorkDataPacket::E_OldSid_E(UINT sz,UINT items, TSonorkOldSidNotification*list)
{
   TSonorkOldSidNotification *ptr;
   UINT  			i;
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_OLD_SID_NOTIFICATION,sz,GUDPCV_NONE);
		CODEC.WriteDW((DWORD)items);
		for(i=0,ptr=list;i<items;i++,ptr++)
			CODEC.WriteOSN1(ptr);

   ENCODE_SECTION_ENDS;

}
// New Sid event
UINT
 TSonorkDataPacket::E_Sid_E(UINT sz,const TSonorkSidNotificationAtom& N)
{
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_SID_NOTIFICATION,sz,GUDPCV_NONE);
	CODEC.WriteDW(1);
	CODEC.Write(&N);
   ENCODE_SECTION_ENDS;
}
UINT
 TSonorkDataPacket::E_Sid_E(UINT sz,TSonorkSidNotificationQueue&Q)
{
   TSonorkSidNotificationAtom* N;
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_SID_NOTIFICATION,sz,GUDPCV_NONE);
	CODEC.WriteDW(Q.Items());
	while( (N = Q.RemoveFirst()) != NULL )
	{
		CODEC.Write(N);
		SONORK_MEM_DELETE(N);
	}

   ENCODE_SECTION_ENDS;
}


#endif


#if defined(SONORK_CLIENT_BUILD)

bool  TSonorkDataPacket::D_OldSid_E(UINT size,TSonorkOldSidNotificationList *list)
{
	TSonorkOldSidNotification L;
	UINT	i;
	DWORD	items;
	DECODE_EVT_SECTION_STARTS(size);
		list->Clear();
		CODEC.ReadDW(&items);
		for(i=0;i<items;i++)
		{
			CODEC.ReadOSN1(&L);
			list->AddItem(&L);
		}
	DECODE_EVT_SECTION_ENDS;
}
bool  TSonorkDataPacket::D_Sid_E(UINT sz,TSonorkSidNotificationQueue* queue)
{
	DWORD items;
	TSonorkSidNotificationAtom*N;
	DECODE_EVT_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while(CODEC.CodecOk() && items-- )
		{
			N = new TSonorkSidNotificationAtom;
			CODEC.Read(N);
			if( CODEC.CodecOk() )
				queue->Add(N);
			else
				SONORK_MEM_DELETE( N );
		}
	DECODE_EVT_SECTION_ENDS;

}

#endif

// ----------------------------------------------------------------------------
// Msgs: Notify, Delivery, Put and Get

#if defined(SONORK_SERVER_BUILD)

UINT  TSonorkDataPacket::E_MsgNotify_E(
	 UINT 				sz
	,UINT 				items
	,TSonorkOldMsgNotification*	ptr)
{
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_OLD_MSG_NOTIFICATION
	,sz
	,GUDPCV_NONE);
	CODEC.WriteDW((DWORD)items);
	while(items--)
	{
		CODEC.WriteMN1(ptr);
		ptr++;
	}
   ENCODE_SECTION_ENDS;
}

UINT  TSonorkDataPacket::E_MsgDelivery_E(UINT sz,TSonorkObjId *msg_id,TSonorkMsg*msg)
{
   ENCODE_EVT_SECTION_STARTS(SONORK_SERVER_CLIENT_EVENT_MSG_DELIVERY,sz,GUDPCV_NONE);
		CODEC.WriteDW2(msg_id);
		CODEC.Write(msg);
   ENCODE_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_PutMsg_R(UINT sz,TSonorkDataPacketMsgTarget& target,TSonorkMsg& msg)
{
	DECODE_REQ_SECTION_STARTS(sz);
	CODEC.ReadDW(&target.type);
	if(target.type==SONORK_DATA_PACKET_MSG_TARGET_LOCUS)
	{
		CODEC.ReadUL1(&target.data.locus);
	}
	else
	if(target.type==SONORK_DATA_PACKET_MSG_TARGET_GROUP)
	{
		CODEC.ReadDW(&target.data.group_tag);
	}
	else
	{
		CODEC.SetError(SONORK_RESULT_INVALID_PARAMETER
		, __FILE__
		, SONORK_MODULE_LINE
		, 0xBAD00000);
	}
	CODEC.Read(&msg);
    DECODE_REQ_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_PutMsg_From_R(UINT sz
	,TSonorkDataPacketMsgTarget&	target
	,TSonorkMsg&			msg)
{
	return D_PutMsg_R(sz,target,msg);
}

UINT  TSonorkDataPacket::E_PutMsg_A(UINT sz
	,SONORK_FUNCTION	f
	,const TSonorkObjId&	msg_id)
{
	ENCODE_AKN_SECTION_STARTS(f,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&msg_id);
	ENCODE_SECTION_ENDS;
}

bool   TSonorkDataPacket::D_GetMsg_R(UINT sz,TSonorkObjId *msg_id,DWORD* ctrl_flags)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(ctrl_flags);
		CODEC.ReadDW2(msg_id);
	DECODE_REQ_SECTION_ENDS;
}

UINT  TSonorkDataPacket::E_GetMsg_A(UINT sz
		,const TSonorkMsg*		body)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_GET_MSG,sz,GUDPCV_NONE);
		CODEC.Write(body);
   ENCODE_SECTION_ENDS;
}

bool   TSonorkDataPacket::D_DelMsg_R(UINT sz,DWORD& count, TSonorkObjId *msg_id)
{
	UINT i;
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(&count);
		if(count > SONORK_MAX_DELMSG_IDS )
			CODEC.SetBadCodecError(__FILE__ , SONORK_MODULE_LINE) ;
		else
		{
			for(i=0;i<count;i++)
			{
				CODEC.ReadDW2(msg_id);
				msg_id++;
			}
		}
	DECODE_REQ_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

bool
 TSonorkDataPacket::D_OldMsgNotify_E(
		UINT 				sz
	, 	TSonorkOldMsgNotificationList*	list)
{
	DWORD 					items;
	TSonorkOldMsgNotification		notification;
	DECODE_EVT_SECTION_STARTS(sz);
		list->Clear();
		CODEC.ReadDW(&items);
		while(items--)
		{
			CODEC.ReadMN1(&notification);
			list->AddItem(&notification);
		}
	DECODE_EVT_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_MsgDelivery_E(UINT sz,TSonorkObjId *msg_id,TSonorkMsg*msg)
{
	DECODE_EVT_SECTION_STARTS(sz);
		CODEC.ReadDW2(msg_id);
		CODEC.Read(msg);
	DECODE_EVT_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_PutMsg_From_R(UINT 	sz
			, const TSonorkId* 	target
			, const TSonorkMsg*	msg)
{
   TSonorkUserLocus1 locus;
   locus.userId.Set( *target );
   locus.sid.Clear();
   ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_PUT_MSG_FROM,sz,GUDPCV_PUT_MSG_FROM);
	CODEC.WriteDW(SONORK_DATA_PACKET_MSG_TARGET_LOCUS);
	CODEC.WriteUL1(&locus);
	CODEC.Write(msg);
   ENCODE_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_PutMsg_R(
			 UINT			sz
			,const TSonorkUserLocus1*locus
			,const TSonorkMsg*	msg)
{
   ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_PUT_MSG,sz,GUDPCV_PUT_MSG);
	  CODEC.WriteDW(SONORK_DATA_PACKET_MSG_TARGET_LOCUS);
	  CODEC.WriteUL1(locus);
	  CODEC.Write(msg);
   ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_PutMsg_A(UINT sz,TSonorkObjId *msg_id)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(msg_id);
	DECODE_AKN_SECTION_ENDS;
}

UINT  TSonorkDataPacket::E_GetMsg_R(UINT sz,TSonorkObjId *msg_id,DWORD ctrl_flags)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_GET_MSG,sz,GUDPCV_GET_MSG);
		CODEC.WriteDW(ctrl_flags);
		CODEC.WriteDW2(msg_id);
   ENCODE_SECTION_ENDS;
}

bool   TSonorkDataPacket::D_GetMsg_A(UINT sz,TSonorkMsg*body)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.Read(body);
	DECODE_AKN_SECTION_ENDS;
}

UINT  TSonorkDataPacket::E_DelMsg_R(UINT sz,DWORD count, TSonorkObjId *msg_id)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_DEL_MSG,sz,GUDPCV_DEL_MSG);
		CODEC.WriteDW(count);
		while(count--)
		{
			CODEC.WriteDW2(msg_id);
			msg_id++;
		}
   ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_SERVER_BUILD)

UINT
 TSonorkDataPacket::E_UserListUsers_A(UINT  sz
		,TSonorkUserDataNotesQueue& set
		,UINT enc_flags)
{
	TSonorkUserDataNotes *entry;
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_USER_LIST,sz,SONORK_SUBFUNC_USER_LIST_USERS);
		CODEC.WriteDW(set.Items());
		while( (entry = set.RemoveFirst()) != NULL)
		{
			CODEC.WriteUDN( entry , enc_flags );
			SONORK_MEM_DELETE(entry);
		}
	ENCODE_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_AtomQueue_A(UINT sz
		,SONORK_FUNCTION 	function
		,DWORD 			subfunc_ver_type
		,TSonorkAtomQueue& 	queue)
{
	TSonorkCodecAtom*	A;
	ENCODE_AKN_SECTION_STARTS(function,sz,subfunc_ver_type);
		CODEC.WriteDW(queue.Items());
		while( (A = queue.RemoveFirstAtom()) != NULL)
		{
			CODEC.Write( A );
			SONORK_MEM_DELETE( A );
		}
	ENCODE_SECTION_ENDS;
}


#endif


#if defined(SONORK_CLIENT_BUILD)


UINT
 TSonorkDataPacket::E_UserList_R()
{
	return _E_R( SONORK_FUNCTION_USER_LIST , GUDPCV_USER_LIST );
}

UINT
 TSonorkDataPacket::E_WappList_R()
{
	return _E_R( SONORK_FUNCTION_WAPP_LIST , GUDPCV_WAPP_LIST );
}

bool
 TSonorkDataPacket::D_ListWapps_A(UINT sz, TSonorkWappDataQueue& set)
{
	DWORD 		entries;
	TSonorkWappData	*entry;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&entries);
		while(entries-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(entry = new TSonorkWappData);
			CODEC.Read( entry );
			if( CODEC.CodecOk() )
				set.Add(entry);
			else
				SONORK_MEM_DELETE( entry );
		}
	DECODE_AKN_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_UserListUsers_A(UINT sz,TSonorkUserDataNotesQueue& set)
{
	DWORD 			entries;
	TSonorkUserDataNotes   *entry;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&entries);
		while(entries-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(entry = new TSonorkUserDataNotes);
			CODEC.ReadUDN( entry );
			if( CODEC.CodecOk() )
				set.Add(entry);
			else
			{
				SONORK_MEM_DELETE(entry);
			}
		}
	DECODE_AKN_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_UserGroups_A(UINT sz,TSonorkGroupQueue& set)
{
	DWORD 			entries;
	TSonorkGroup		*entry;
	DECODE_AKN_SECTION_STARTS(sz);

		CODEC.ReadDW(&entries);

		while(entries-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(entry = new TSonorkGroup);
			CODEC.Read( entry );
			if( CODEC.CodecOk() )
				set.AddSorted( entry );
			else
				SONORK_MEM_DELETE( entry );
		}

	DECODE_AKN_SECTION_ENDS;
}


#endif

#if defined(SONORK_SERVER_BUILD)

bool
 TSonorkDataPacket::D_SetGroup_R2(UINT sz, TSonorkGroup&UG)
{
	DECODE_REQ_SECTION_STARTS(sz);

		CODEC.Skip(sizeof(DWORD));
		if( SubFunction() == SONORK_SUBFUNC_SET_GROUP_DEL )
		{
			CODEC.ReadDW(&UG.header.group_type);
			CODEC.ReadDW(&UG.header.group_no);
		}
		else
			CODEC.Read(&UG);
	DECODE_REQ_SECTION_ENDS;
}
UINT
 TSonorkDataPacket::E_SetGroup_A2(UINT sz
		, DWORD subfunc
		, const TSonorkGroup& UG)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_SET_GROUP,sz,subfunc);

		CODEC.Skip(sizeof(DWORD));
		if( subfunc == SONORK_SUBFUNC_SET_GROUP_DEL )
		{
			CODEC.WriteDW(UG.header.group_type);
			CODEC.WriteDW(UG.header.group_no);
		}
		else
			CODEC.Write(&UG);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT
 TSonorkDataPacket::E_SetGroup_R2(UINT sz, DWORD subfunc, const void *UG)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_GROUP,sz,GUDPCV_SET_GROUP|subfunc);

		CODEC.Skip(sizeof(DWORD));
		if( SubFunction() == SONORK_SUBFUNC_SET_GROUP_DEL )
			CODEC.WriteDWN((DWORD*)UG,2);
		else
			CODEC.Write((const TSonorkGroup*)UG);
	ENCODE_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_SetGroup_A2(UINT sz, void *UG)
{
	DECODE_AKN_SECTION_STARTS(sz);

		CODEC.Skip(sizeof(DWORD));
		if( SubFunction() == SONORK_SUBFUNC_SET_GROUP_DEL )
			CODEC.ReadDWN((DWORD*)UG,2);
		else
			CODEC.Read((TSonorkGroup*)UG);
	DECODE_AKN_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_DelGroup_R(UINT sz, SONORK_GROUP_TYPE type, DWORD no)
{
	DWORD	tmp[2];
	tmp[0]=type;
	tmp[1]=no;
	return E_SetGroup_R2(sz,SONORK_SUBFUNC_SET_GROUP_DEL,tmp);
}
bool
	TSonorkDataPacket::D_DelGroup_A(UINT sz, SONORK_GROUP_TYPE*pType, DWORD* pNo)
{
	DWORD	tmp[2];
	if( D_SetGroup_A2(sz,tmp) )
	{
		*pType	= (SONORK_GROUP_TYPE)tmp[0];
		*pNo	= tmp[1];
		return true;
	}
	return false;
}

#endif




UINT  TSonorkDataPacket::_E_R(SONORK_FUNCTION func,DWORD subfunc_version)
{
	InitHeader(func,SONORK_PACKET_TYPE_REQ|subfunc_version);
	return sizeof(header);
}

UINT  TSonorkDataPacket::E_Error_A(UINT sz, SONORK_FUNCTION function, const TSonorkError&ERR)
{
	InitHeader(function,SONORK_PACKET_TYPE_AKN);
	TSonorkPacketWriter  CODEC(DataPtr(),sz);
	E_Error(CODEC,ERR);
	SetNoDataMark();
	return CODEC.CodecOk()?sizeof(TSonorkDataPacket)+CODEC.Size():0;
}
bool  TSonorkDataPacket::D_Error_A(UINT sz, TSonorkError&ERR)
{
	if( IsErrorMarkSet() )
	{
		TSonorkPacketReader CODEC(DataPtr(),sz);
		CODEC.Read(&ERR);
		if( !CODEC.CodecOk())
		{

			ERR.SetSys( CODEC.Result() , GSS_BADCODEC , SONORK_MODULE_LINE );
			return false;
		}
	}
	else
	{
		ERR.SetOk();
	}
	return true;
}


// ----------------------------------------------------------------------------
// Auths

#if defined(SONORK_SERVER_BUILD)

bool TSonorkDataPacket::D_ReqAuth_R(
	UINT 			sz
	,TSonorkId&			user_id
	,TSonorkAuth2&		auth
	,TSonorkText&		text)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadAU2(&auth);
		CODEC.Read(&text);
	DECODE_REQ_SECTION_ENDS;
}
UINT TSonorkDataPacket::E_ReqAuth_A(
	UINT 				sz
	,const TSonorkId&		user_id)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_REQ_AUTH,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&user_id);
	ENCODE_SECTION_ENDS;
}

bool	 TSonorkDataPacket::D_AknAuth_R(UINT sz,TSonorkId&user_id,TSonorkAuth2&auth)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadAU2(&auth);
	DECODE_REQ_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_AknAuth_A(
	UINT 				sz
	,const TSonorkId&		user_id
	,const TSonorkAuth2&	auth)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_AKN_AUTH,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteAU2(&auth);
	ENCODE_SECTION_ENDS;
}

bool	 TSonorkDataPacket::D_SetAuth_R(
	UINT 				sz
	,TSonorkId&				user_id
	,TSonorkAuth1&			auth)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadAU1(&auth);
	DECODE_REQ_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_SetAuth_A(
	UINT 				sz
	,const TSonorkId&		user_id
	,const TSonorkAuth2&	auth)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_SET_AUTH,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteAU2(&auth);
	ENCODE_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_DelAuth_R(
	 UINT 			sz
	,TSonorkId&		user_id)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
	DECODE_REQ_SECTION_ENDS;
}

UINT
 TSonorkDataPacket::E_DelAuth_A(
	UINT 				sz
	,const TSonorkId&		user_id)
{
	ENCODE_AKN_SECTION_STARTS(SONORK_FUNCTION_DEL_AUTH,sz,GUDPCV_NONE);
		CODEC.WriteDW2(&user_id);
	ENCODE_SECTION_ENDS;
}

#endif


#if defined(SONORK_CLIENT_BUILD)


UINT TSonorkDataPacket::E_ReqAuth_R(
	UINT 				sz
	,const TSonorkId&		user_id
	,const TSonorkAuth2&	auth
	,const TSonorkText&		text)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_REQ_AUTH,sz,GUDPCV_REQ_AUTH);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteAU2(&auth);
		CODEC.Write(&text);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_ReqAuth_A(
	UINT 				sz
	,TSonorkId&				user_id)
{
   DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
   DECODE_AKN_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_AknAuth_R(
	 UINT 				sz
	,const TSonorkId&		user_id
	,const TSonorkAuth2&	auth)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_AKN_AUTH,sz,GUDPCV_AKN_AUTH);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteAU2(&auth);
	ENCODE_SECTION_ENDS;
}

bool TSonorkDataPacket::D_AknAuth_A(
	 UINT 				sz
	,TSonorkId&				user_id
	,TSonorkAuth2&			auth)
{
   DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadAU2(&auth);
   DECODE_AKN_SECTION_ENDS;
}

UINT TSonorkDataPacket::E_SetAuth_R(
	UINT 				sz
	,const TSonorkId&		user_id
	,const TSonorkAuth1&	auth)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_SET_AUTH,sz,GUDPCV_SET_AUTH);
		CODEC.WriteDW2(&user_id);
		CODEC.WriteAU1(&auth);
	ENCODE_SECTION_ENDS;

}

bool	 TSonorkDataPacket::D_SetAuth_A(
	 UINT 				sz
	,TSonorkId&				user_id
	,TSonorkAuth2&			auth)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
		CODEC.ReadAU2(&auth);
	DECODE_AKN_SECTION_ENDS;
}


UINT TSonorkDataPacket::E_DelAuth_R(
	UINT 			sz
	,const TSonorkId&	user_id)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_DEL_AUTH,sz,GUDPCV_DEL_AUTH);
		CODEC.WriteDW2(&user_id);
	ENCODE_SECTION_ENDS;
}

bool	 TSonorkDataPacket::D_DelAuth_A(
	UINT 				sz
	,TSonorkId&		      	user_id)
{
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW2(&user_id);
	DECODE_AKN_SECTION_ENDS;
}

#endif

// ----------------------------------------------------------------------------
// Ping

#if defined(SONORK_SERVER_BUILD)

UINT
 TSonorkDataPacket::E_Ping_A()
{
	InitHeader(SONORK_FUNCTION_PING,SONORK_PACKET_TYPE_AKN|0);
	return sizeof(header);
}

#endif


#if defined(SONORK_CLIENT_BUILD)

UINT
 TSonorkDataPacket::E_Ping_R()
{
	return _E_R(SONORK_FUNCTION_PING , GUDPCV_PING );
}
#endif


#if defined(SONORK_CLIENT_BUILD)

// ----------------------------------------------------------------------------
// Trackers

UINT
 TSonorkDataPacket::E_ListTrackers_R(UINT sz
			, const TSonorkTrackerId& tracker_id
			, BOOL  rooms
			, DWORD max_count)
{
	ENCODE_REQ_SECTION_STARTS(
			 SONORK_FUNCTION_LIST_TRACKERS
			,sz
			,rooms
			 ?SONORK_SUBFUNC_LIST_TRACKERS_REQ_ROOMS|GUDPCV_LIST_TRACKERS
			 :SONORK_SUBFUNC_LIST_TRACKERS_REQ_MEMBERS|GUDPCV_LIST_TRACKERS);
		CODEC.WriteDW2(&tracker_id);
		CODEC.WriteDW(max_count);
	ENCODE_SECTION_ENDS;
}


UINT
 TSonorkDataPacket::E_RegisterTracker_R(UINT sz
	,BOOL append
	,TSonorkTrackerDataQueue& queue)
{
	return E_AtomQueue_R(sz
			, SONORK_FUNCTION_REGISTER_TRACKER
			, append?SONORK_SUBFUNC_REGISTER_TRACKER_APPEND|GUDPCV_REGISTER_TRACKER
				:SONORK_SUBFUNC_REGISTER_TRACKER_RESET|GUDPCV_REGISTER_TRACKER
			, queue);
}


bool
 TSonorkDataPacket::D_ListTrackerRooms_A(UINT sz,TSonorkTrackerRoomQueue& queue)
{
	DWORD 			entries;
	TSonorkTrackerRoom*	entry;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&entries);
		while(entries-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(entry = new TSonorkTrackerRoom);
			CODEC.Read( entry );
			if( CODEC.CodecOk() )
				queue.Add(entry);
			else
				SONORK_MEM_DELETE( entry );
		}
	DECODE_AKN_SECTION_ENDS;
}

bool
 TSonorkDataPacket::D_ListTrackerMembers_A(UINT sz
	,TSonorkTrackerMemberQueue& queue)
{
	DWORD 			entries;
	TSonorkTrackerMember*	entry;
	DECODE_AKN_SECTION_STARTS(sz);
		CODEC.ReadDW(&entries);
		while(entries-- && CODEC.CodecOk() )
		{
			SONORK_MEM_NEW(entry = new TSonorkTrackerMember);
			CODEC.Read( entry );
			if( CODEC.CodecOk() )
				queue.Add(entry);
			else
				SONORK_MEM_DELETE( entry );
		}
	DECODE_AKN_SECTION_ENDS;
}



UINT
 TSonorkDataPacket::E_AtomQueue_R(UINT 		sz
			, SONORK_FUNCTION	p_function
			, DWORD 		p_subfunc_ver_type
			, TSonorkAtomQueue& 	queue)
{
	TSonorkCodecAtom*	A;
	ENCODE_REQ_SECTION_STARTS(p_function,sz,p_subfunc_ver_type);
		CODEC.WriteDW(queue.Items());
		while( (A = queue.RemoveFirstAtom()) != NULL)
		{
			CODEC.Write( A );
			SONORK_MEM_DELETE( A );
		}
	ENCODE_SECTION_ENDS;
}

#endif

#if defined(SONORK_SERVER_BUILD)
bool
 TSonorkDataPacket::D_MonitorCommand_R(UINT sz
			, TSonorkMonitorCmdAtom* A)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.Read(A);
	DECODE_REQ_SECTION_ENDS;
}

#if defined(SONORK_MONITOR_BUILD)
UINT
 TSonorkDataPacket::E_MonitorCommand_R(UINT 	sz
			, const TSonorkMonitorCmdAtom* A)
{
	ENCODE_REQ_SECTION_STARTS(SONORK_FUNCTION_MONITOR_COMMAND
		,sz
		,0);
		CODEC.Write(A);
	ENCODE_SECTION_ENDS;
}
#endif

#endif


#if defined(SONORK_SERVER_BUILD)
// ----------------------------------------------------------------------------
// Trackers

bool
 TSonorkDataPacket::D_ListTrackers_R(UINT sz
			, TSonorkTrackerId&	tracker_id
			, DWORD& 		max_count)
{
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW2(&tracker_id);
		CODEC.ReadDW(&max_count);
	DECODE_REQ_SECTION_ENDS;
}


bool
 TSonorkDataPacket::D_RegisterTracker_R(UINT sz
			,TSonorkTrackerDataQueue& queue)
{
	TSonorkTrackerData*entry;
	DWORD		items;
	DECODE_REQ_SECTION_STARTS(sz);
		CODEC.ReadDW(&items);
		while(CODEC.CodecOk() && items-- )
		{
			entry = new TSonorkTrackerData;
			CODEC.Read(entry);
			if( CODEC.CodecOk() )
				queue.Add(entry);
			else
				SONORK_MEM_DELETE( entry );
		}

	DECODE_REQ_SECTION_ENDS;
}

#endif



TSonorkDataPacket*
 SONORK_AllocDataPacket(DWORD data_size)
{
	return SONORK_MEM_ALLOC(TSonorkDataPacket,sizeof(TSonorkDataPacket) + data_size + sizeof(DWORD) );
}

void
 TSonorkDataPacket::NormalizeHeader()
{
#if defined(SONORK_BYTE_REVERSE)
#	error SONORK_BYTE_REVERSE defined
#endif
}



#define R_CASE(n)	case SONORK_FUNCTION_##n: str=#n;break;
SONORK_C_CSTR SONORK_FunctionName(SONORK_FUNCTION f)
{
   SONORK_C_CSTR str;
   switch(f)
   {
	R_CASE(NONE)
	R_CASE(PING)
	R_CASE(SET_SID)
	R_CASE(USER_LIST)
	R_CASE(REQ_AUTH)
	R_CASE(AKN_AUTH)
	R_CASE(SET_AUTH)
	R_CASE(DEL_AUTH)
	R_CASE(PUT_MSG)
	R_CASE(GET_MSG)
	R_CASE(DEL_MSG)
	R_CASE(PUT_DATA)
	R_CASE(SET_USER_DATA)
	R_CASE(GET_USER_DATA)
	R_CASE(REQ_SID_TASK)
	R_CASE(AKN_SID_TASK)
	R_CASE(IDENTIFY_USER)
	R_CASE(CTRL_MSG)
	R_CASE(SEARCH_USER_OLD)
	R_CASE(SEARCH_USER_NEW)
	R_CASE(ADD_SERVICE)
	R_CASE(LIST_SERVICES)
	R_CASE(CREATE_USER)
	R_CASE(SET_GROUP)
	R_CASE(SET_PASS)
	R_CASE(WAPP_LIST)
	R_CASE(WAPP_URL)
	R_CASE(NEWS_LIST)
	R_CASE(GET_SYS_MSGS)
	R_CASE(ADD_USERVER)
	R_CASE(SET_USERVER)
	R_CASE(DEL_USERVER)
	R_CASE(LIST_USERVERS)
	R_CASE(PUT_MSG_FROM)
	R_CASE(LIST_TRACKERS)
	R_CASE(REGISTER_TRACKER)
	R_CASE(MONITOR_COMMAND)
	default:str="??";break;
   }
   return str;
}
#undef R_CASE


