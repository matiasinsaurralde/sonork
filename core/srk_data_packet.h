#if !defined(SRK_DATA_PACKET_H)
#define SRK_DATA_PACKET_H
#include "srk_data_types.h"
#if defined(SONORK_SERVER_BUILD) || defined(SONORK_MONITOR_BUILD)
# include "srk_monitor.h"
#endif

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


enum SONORK_FUNCTION
{
  SONORK_FUNCTION_NONE
, SONORK_FUNCTION_PING
, SONORK_FUNCTION_SET_SID
, SONORK_FUNCTION_REQ_AUTH
, SONORK_FUNCTION_AKN_AUTH
, SONORK_FUNCTION_SET_AUTH
, SONORK_FUNCTION_DEL_AUTH
, SONORK_FUNCTION_USER_LIST
, SONORK_FUNCTION_PUT_MSG
, SONORK_FUNCTION_GET_MSG
, SONORK_FUNCTION_DEL_MSG
, SONORK_FUNCTION_PUT_DATA
, SONORK_FUNCTION_REQ_SID_TASK
, SONORK_FUNCTION_AKN_SID_TASK
, SONORK_FUNCTION_SET_USER_DATA
, SONORK_FUNCTION_GET_USER_DATA
, SONORK_FUNCTION_IDENTIFY_USER
, SONORK_FUNCTION_IDENTIFY_obsolete	// NOT USED
, SONORK_FUNCTION_CTRL_MSG
, SONORK_FUNCTION_SEARCH_USER_OLD
, SONORK_FUNCTION_ADD_SERVICE
, SONORK_FUNCTION_LIST_SERVICES
, SONORK_FUNCTION_DEL_SERVICE
, SONORK_FUNCTION_CREATE_USER
, SONORK_FUNCTION_SET_GROUP
, SONORK_FUNCTION_SET_PASS
, SONORK_FUNCTION_WAPP_LIST
, SONORK_FUNCTION_WAPP_URL
, SONORK_FUNCTION_NEWS_LIST
, SONORK_FUNCTION_GET_SYS_MSGS
, SONORK_FUNCTION_ADD_USERVER
, SONORK_FUNCTION_SET_USERVER
, SONORK_FUNCTION_DEL_USERVER
, SONORK_FUNCTION_LIST_USERVERS
, SONORK_FUNCTION_SEARCH_USER_NEW
, SONORK_FUNCTION_PUT_MSG_FROM
, SONORK_FUNCTION_LIST_TRACKERS
, SONORK_FUNCTION_REGISTER_TRACKER
, SONORK_FUNCTION_MONITOR_COMMAND
, SONORK_FUNCTIONS
};
enum SONORK_SERVER_CLIENT_EVENT
{
  SONORK_SERVER_CLIENT_EVENT_NONE
, SONORK_SERVER_CLIENT_EVENT_OLD_MSG_NOTIFICATION
, SONORK_SERVER_CLIENT_EVENT_MSG_DELIVERY
, SONORK_SERVER_CLIENT_EVENT_OLD_SID_NOTIFICATION
, SONORK_SERVER_CLIENT_EVENT_RESERVED_1
, SONORK_SERVER_CLIENT_EVENT_CTRL_MSG
, SONORK_SERVER_CLIENT_EVENT_SID_NOTIFICATION
, SONORK_SERVER_CLIENT_EVENT_MSG_NOTIFICATION
};

enum SONORK_FUNCTION_SEARCH_USER_FLAGS
{
  // Old flags
  // if NEW flag is NOT set, flags should be interpreted using these OF flags
  SONORK_FUNC_OF_SEARCH_USER_ONLINE_ONLY	= 0x00010000
, SONORK_FUNC_OF_SEARCH_USER_EXCLUDE_BUSY	= 0x00020000
, SONORK_FUNC_OF_SEARCH_USER_FRIENDLY_ONLY	= 0x00040000
// if NEW flag is set, flags should be interpreted using these flags
, SONORK_FUNC_F_SEARCH_USER_NEW			= 0x10000000
, SONORK_FUNC_NM_SEARCH_USER_METHOD		= 0x000000ff
// The METHOD_USER_INFO: Packet's user data should be matched
, SONORK_FUNC_NF_SEARCH_USER_METHOD_USER_INFO	= 0x00000000
// The METHOD_CALL_SIGN: Packet's user alias should be matched
//  agains list of call signs
, SONORK_FUNC_NF_SEARCH_USER_METHOD_CALL_SIGN	= 0x00000001
, SONORK_FUNC_NM_SEARCH_USER_SID_MODE		= 0x0000ff00
, SONORK_FUNC_NF_SEARCH_USER_SID_MODE_ALL	= 0x00000000
, SONORK_FUNC_NF_SEARCH_USER_SID_MODE_NOT_BUSY	= 0x00000100
, SONORK_FUNC_NF_SEARCH_USER_SID_MODE_FRIENDLY	= 0x00000200
, SONORK_FUNC_NM_SEARCH_USER_STATE		= 0x00030000
, SONORK_FUNC_NF_SEARCH_USER_STATE_ANY		= 0x00030000
, SONORK_FUNC_NF_SEARCH_USER_STATE_ONLINE	= 0x00010000
, SONORK_FUNC_NF_SEARCH_USER_STATE_OFFLINE	= 0x00020000
};


SONORK_C_CSTR	SONORK_FunctionName	(SONORK_FUNCTION r);

#if defined(USE_PRAGMA_PUSH)
#pragma	pack(push,1)
#endif
#define SONORK_MAX_DELMSG_IDS				8


#if !defined(SONORK_IPC_BUILD)

// TYPE (HIGH NIBBLE OF PACKET_CTRL)
#define SONORK_PCTRL_TYPE_MASK	      		0xf000
#define SONORK_PCTRL_VERSION_MASK   	  	0x000f
#define SONORK_PCTRL_SUBFUNCTION_MASK		0x00f0
#define SONORK_PCTRL_MARK_NO_DATA   		0x0100	// packet contains NO data, don't try to decode
#define SONORK_PCTRL_MARK_1	    		0x0200
#define SONORK_PCTRL_MARK_2	    		0x0400
#define SONORK_PCTRL_MARK_ERROR	    		0x0800
#define SONORK_PCTRL_MARK_MASK	    		0x0f00


// Flags for SONORK_FUNCTION_GET_MSG
#define SONORK_PUTMSG_CTRL_F_REPLYING		0x00000001
#define SONORK_GETMSG_CTRL_F_NO_DELETE		0x00010000
#define SONORK_GETMSG_CTRL_F_MARK_READ		0x00020000

// Flags for SONORK_FUNCTION_GET_USER_DATA
#define SONORK_GETUSERDATA_F_USE_PUBLIC		0x00000001
#define SONORK_GETUSERDATA_F_USE_PRIVATE	0x00000002
#define SONORK_GETUSERDATA_F_OWNER  		0x10000000


// Version control: SONORK_DATA_PACKET_CURRENT_VERSION (GUDPCVs)
//  Defines the current version of the function requested, if not
// 	defined here, the version should be GUDPCV_NONE (0)
#define	GUDPCV_NONE					0
#define GUDPCV_USER_LIST				1
#define GUDPCV_WAPP_LIST				0
#define GUDPCV_SET_PASS					0
#define GUDPCV_CREATE_USER				1
#define GUDPCV_ADD_SERVICE				0
#define GUDPCV_LIST_SERVICES				1
#define GUDPCV_SEARCH_USER				0
#define GUDPCV_REQ_AUTH					0
#define GUDPCV_SET_GROUP				0
#define GUDPCV_PING					0
#define GUDPCV_CTRL_MSG					0
#define GUDPCV_PUT_MSG					0
#define GUDPCV_GET_MSG					0
#define GUDPCV_DEL_MSG					0
#define GUDPCV_AKN_AUTH					0
#define GUDPCV_SET_AUTH					0
#define GUDPCV_DEL_AUTH					0
#define GUDPCV_IDENTIFY_PLUGIN				0
#define GUDPCV_IDENTIFY_USER				0
#define GUDPCV_SET_SID					1
#define GUDPCV_SET_USER_DATA				0
#define GUDPCV_GET_USER_DATA				0
#define GUDPCV_GET_WAPP					0
#define GUDPCV_WAPP_URL					0
#define GUDPCV_NEWS_LIST				0
#define GUDPCV_GET_SYS_MSGS				0
#define GUDPCV_ADD_USERVER				0
#define GUDPCV_LIST_USERVERS				0
#define GUDPCV_SET_USERVER				0
#define GUDPCV_DEL_USERVER				0
#define GUDPCV_PUT_MSG_FROM				0
#define GUDPCV_LIST_TRACKERS				0
#define GUDPCV_REGISTER_TRACKER				0

// SUB FUNCTIONS
// The subfunction value depends of the packet function;
// for example: When the server answers to a USER_LIST
// request, it'll answer with two types of packets, both of them
// with function=USER_LIST but with different subfunctions:
// Type 1 has SUBFUNC_USER_LIST_GROUPS: Group data
// Type 2 has SUBFUNC_USER_LIST_USERS: 	User data
// For compatibility, the receptor should ALWAYS check the
// subfunction and ignore packets with unknown subfunctions.

// ALL functions
#define SONORK_SUBFUNC_NONE				0x00

// USER_LIST
#define	SONORK_SUBFUNC_USER_LIST_USERS			0x00
#define	SONORK_SUBFUNC_USER_LIST_GROUPS			0x10

// WAPP_LIST
#define	SONORK_SUBFUNC_WAPP_LIST_APPS			0x00
#define	SONORK_SUBFUNC_WAPP_LIST_WAPP_GROUPS		0x10
#define	SONORK_SUBFUNC_WAPP_LIST_MTPL_GROUPS		0x20

// SET_GROUP
#define	SONORK_SUBFUNC_SET_GROUP_SET			0x00
#define	SONORK_SUBFUNC_SET_GROUP_ADD			0x10
#define	SONORK_SUBFUNC_SET_GROUP_DEL			0x20

// NEWS_LIST
#define	SONORK_SUBFUNC_NEWS_LIST_GROUPS			0x00
#define	SONORK_SUBFUNC_NEWS_LIST_SOURCE			0x10

// LIST_TRACKERS
#define	SONORK_SUBFUNC_LIST_TRACKERS_REQ_MEMBERS	0x00
#define	SONORK_SUBFUNC_LIST_TRACKERS_REQ_ROOMS		0x10

#define	SONORK_SUBFUNC_LIST_TRACKERS_AKN_MEMBERS	0x00
#define	SONORK_SUBFUNC_LIST_TRACKERS_AKN_ROOMS		0x10
#define	SONORK_SUBFUNC_LIST_TRACKERS_AKN_GROUPS		0x20

// TRACKER_REGISTER
#define	SONORK_SUBFUNC_REGISTER_TRACKER_RESET		0x00
#define	SONORK_SUBFUNC_REGISTER_TRACKER_APPEND		0x10


// Version control: SONORK_DATA_PACKET_LEAST_VERSION (SDPLVs)
//  Specify the first version that supports a subfunction of a function.
//  The server checks the ReqVersion() of the packet and if the value
//  is equal or higher to the GUDPLV of a given task, it executes it.
//  For example:
//   First GU's did not support user groups and sent the USER_LIST
//    request with ReqVersion() = 0, so the server does not
//    reply with SUBFUNC_USER_LIST_GROUPS packets, only with
//    SUBFUNC_USER_LIST_USERS packets since the client does
//    not know how to handle the SUBFUNC_USER_LIST_GROUPS packets.

// GROUP_SUPPORT:
//  The client supports extended GROUPS subfunction of
//  the USER_LIST function. (The server should send the
//  users group before sending the user list). If not
//  supported, the server sends only the user list.
#define SDPLV_SUBFUNC_USER_LIST_GROUPS		1

// CREATE_USER_EXT_DATA_1
// The client sends extended data for CREATE_USER
#define SDPLV_CREATE_USER_EXT_DATA_1		1

// SET_SID_MSG
// The client sent the additional String for SET_SID
#define SDPLV_SET_SID_MSG			1

// LIST_SERVICES_NEW_1
// The client send the LIST_SERVICES packet
// encoded in the new way
#define SDPLC_LIST_SERVICES_NEW_1		1

#define SONORK_SERVER_SIGNATURE		0x3d494753

enum SONORK_DATA_PACKET_MSG_TARGET_TYPE
{
	 SONORK_DATA_PACKET_MSG_TARGET_LOCUS
	,SONORK_DATA_PACKET_MSG_TARGET_GROUP
};


struct TSonorkDataPacketMsgTarget
{
	DWORD				type;
	union
	{
		TSonorkUserLocus1	locus;
		DWORD	  		group_tag;
	}data;
}__SONORK_PACKED;


// TSonorkDataPacket.packet_control

enum SONORK_PACKET_TYPE
{
  SONORK_PACKET_TYPE_REQ	=	0x0000
, SONORK_PACKET_TYPE_AKN	=	0x1000
, SONORK_PACKET_TYPE_EVT	=	0x2000
};

struct SONORK_REQ
{
	TSonorkTask		task;
}__SONORK_PACKED;

struct SONORK_AKN
{
	TSonorkTask		task;
}__SONORK_PACKED;

struct SONORK_EVT
{
	SONORK_DWORD2		param;
}__SONORK_PACKED;


class TSonorkCodecIO;

struct TSonorkDataPacketHeader
{
	WORD			function;
	WORD			packet_ctrl;
	union {
		SONORK_REQ	req;
		SONORK_AKN	akn;
		SONORK_EVT	evt;
	}packet_info __SONORK_PACKED;
} __SONORK_PACKED;

struct TSonorkDataPacket
{
friend struct IAmNoOnesFriend;		// Dummy: To avoid compiler warnings
private:
	TSonorkDataPacket(){}	// Dummy , just to force the use of
	~TSonorkDataPacket(){}  // SONORK_AllocDataPacket and SONORK_FreeDataPacket()

	TSonorkDataPacketHeader		header;


	UINT  _E_R(SONORK_FUNCTION,DWORD subfunc_version);

	SONORK_RESULT E_Error(TSonorkCodecWriter&, const TSonorkError&E);

	void InitHeader(SONORK_FUNCTION,DWORD subfunc_ver_type);
	void InitHeader(SONORK_SERVER_CLIENT_EVENT,DWORD subfunc_ver);

public:
	void			NormalizeHeader();

	SONORK_FUNCTION
		Function()	const
		{ return (SONORK_FUNCTION)(header.function&0x7f);}

	DWORD
		SubFunction()	const
		{ return (DWORD)(header.packet_ctrl&SONORK_PCTRL_SUBFUNCTION_MASK);}

	SONORK_PACKET_TYPE
		PacketType()	const
		{ return (SONORK_PACKET_TYPE)(header.packet_ctrl&SONORK_PCTRL_TYPE_MASK);}

	DWORD
		PacketMarks()	const
		{ return (DWORD)(header.packet_ctrl & SONORK_PCTRL_MARK_MASK); }

	void	SetErrorMark()
		{ header.packet_ctrl |= SONORK_PCTRL_MARK_ERROR;}

	void	SetNoDataMark()
		{ header.packet_ctrl |= SONORK_PCTRL_MARK_NO_DATA;}

	void	SetMark1()
		{ header.packet_ctrl |= SONORK_PCTRL_MARK_1;}

	void	SetMark2()
		{ header.packet_ctrl |= SONORK_PCTRL_MARK_2;}

	BOOL	TestMarks(DWORD v) const
		{ return header.packet_ctrl&(v&SONORK_PCTRL_MARK_MASK);}

	BOOL	IsErrorMarkSet() const
		{ return header.packet_ctrl&SONORK_PCTRL_MARK_ERROR;}

	BOOL	IsNoDataMarkSet() const
		{ return header.packet_ctrl&SONORK_PCTRL_MARK_NO_DATA;}

	BOOL	IsMark1Set() const
		{ return header.packet_ctrl & SONORK_PCTRL_MARK_1;}

	BOOL	IsMark2Set() const
		{ return header.packet_ctrl & SONORK_PCTRL_MARK_2;}

	void	ClearErrorMark()
		{ header.packet_ctrl &= ~SONORK_PCTRL_MARK_ERROR;}

	void	ClearNoDataMark()
		{ header.packet_ctrl &= ~SONORK_PCTRL_MARK_NO_DATA;}

	void	ClearMark1()
		{ header.packet_ctrl &= ~SONORK_PCTRL_MARK_1;}

	void	ClearMark2()
		{ header.packet_ctrl &= ~SONORK_PCTRL_MARK_2;}


	BYTE*	DataPtr(UINT pos=0)
		{ return ((BYTE*)&header)+sizeof(header)+pos;}

	SONORK_REQ&
		Req()
		{ return header.packet_info.req;}

	SONORK_AKN&
		Akn()
		{ return header.packet_info.akn;}

	SONORK_EVT&
		Evt()
		{ return header.packet_info.evt;}

	UINT	EvtVersion()	const
		{ return (header.packet_ctrl&SONORK_PCTRL_VERSION_MASK); }

	TSonorkTask&
		ReqTask()
		{ return header.packet_info.req.task;}

	UINT	ReqVersion()	const
		{ return (header.packet_ctrl&SONORK_PCTRL_VERSION_MASK); }

	TSonorkTask&
		AknTask()
		{ return header.packet_info.akn.task;}

	SONORK_RESULT
		AknResult();

	BOOL	IsAknEnd() const
		{ return header.packet_info.req.task.Server()==SONORK_INVALID_TASK_ID;}


#if defined(SONORK_CLIENT_BUILD)



	UINT
		E_AtomQueue_R(UINT 		sz
			, SONORK_FUNCTION
			, DWORD 		subfunc_ver_type
			, TSonorkAtomQueue& 	set);

// ----------------------------------------------------------------------------
// User Data & Search

	UINT
		E_SetUserData_R(UINT sz, TSonorkUserDataNotes&,UINT udn_flags);

	bool
		D_SetUserData_A(UINT sz, TSonorkSerial&);

	UINT
		E_GetUserData_R(UINT 			sz
				,const 	TSonorkId& 	user_id
				,SONORK_USER_INFO_LEVEL level
				,DWORD 			get_flags
				,DWORD			auth_pin
				,DWORD			reserved);
	bool
		D_GetUserData_A(UINT sz,TSonorkUserDataNotes&);

	UINT
		E_SetPass_R(UINT sz, const TSonorkShortString& pass);

	UINT
		E_SearchUser_R_NEW(UINT sz
			, const TSonorkUserDataNotes&
			, DWORD udn_flags
			, DWORD flags
			, DWORD max_count=50);

	UINT
		E_SearchUser_R_OLD(UINT sz
			, const TSonorkId&
			, const TSonorkShortString& alias
			, const TSonorkShortString& name
			, const TSonorkShortString& email
			, DWORD max_count=50);
	bool
		D_SearchUser_A(UINT size, TSonorkUserDataNotesQueue& list);

	UINT
		E_CreateUser1_R(UINT sz,DWORD referrer_id,const TSonorkUserDataNotes&,const TSonorkDynData&);

	bool
		D_CreateUser_A(UINT sz,TSonorkId&);

// ----------------------------------------------------------------------------
// User List & Sids


// ----------------------------------------------------------------------------
// Groups

	UINT
		E_SetGroup_R2(UINT sz, DWORD subfunc, const void *UG);

	bool
		D_SetGroup_A2(UINT sz, void*UG);

	UINT
		E_AddGroup_R(UINT sz, TSonorkGroup*UG)
		{ return E_SetGroup_R2(sz,SONORK_SUBFUNC_SET_GROUP_ADD,UG); }

	UINT
		E_SetGroup_R(UINT sz, TSonorkGroup*UG)
		{ return E_SetGroup_R2(sz,SONORK_SUBFUNC_SET_GROUP_SET,UG); }

	UINT
		E_DelGroup_R(UINT sz, SONORK_GROUP_TYPE, DWORD no);

	bool
		D_AddGroup_A(UINT sz, TSonorkGroup*UG)
		{ return D_SetGroup_A2(sz,UG); }

	bool
		D_SetGroup_A(UINT sz, TSonorkGroup*UG)
		{ return D_SetGroup_A2(sz,UG); }

	bool
		D_DelGroup_A(UINT sz, SONORK_GROUP_TYPE*, DWORD* grp);

// ----------------------------------------------------------------------------
// Authorizations

	UINT
		E_ReqAuth_R(UINT sz,const TSonorkId&,const TSonorkAuth2&auth,const TSonorkText&);

	bool
		D_ReqAuth_A(UINT sz,TSonorkId&);

	UINT
		E_AknAuth_R(UINT sz,const TSonorkId&,const TSonorkAuth2&);

	bool
		D_AknAuth_A(UINT sz,TSonorkId&,TSonorkAuth2&);

	UINT
		E_SetAuth_R(UINT sz,const TSonorkId&,const TSonorkAuth1&);

	bool
		D_SetAuth_A(UINT sz,TSonorkId&,TSonorkAuth2&);

	UINT
		E_DelAuth_R(UINT sz,const TSonorkId&);

	bool
		D_DelAuth_A(UINT sz,TSonorkId&);

// ----------------------------------------------------------------------------
// Services

	UINT
		E_AddService_R(UINT sz,const TSonorkServiceData&);

	bool
		D_AddService_A(UINT sz,TSonorkServiceLocus1& service_locus);

	UINT
		E_NewListServices_R(UINT sz
			,SONORK_SERVICE_TYPE		service_type
			,DWORD				service_instance
			,DWORD				min_version=0
			,DWORD 				max_count=8
			,SONORK_SERVICE_STATE		min_state=SONORK_SERVICE_STATE_AVAILABLE
			,SONORK_SERVICE_PRIORITY        min_prio=SONORK_SERVICE_PRIORITY_LOWEST
			);
	UINT
		E_OldListServices_R(UINT sz
			,SONORK_SERVICE_TYPE		service_type
			,DWORD 				max_count=8
			);
	bool
		D_ListServices_A(UINT sz,TSonorkServiceDataQueue&);

// ----------------------------------------------------------------------------
// User servers

	UINT
		E_AddUserServer_R(UINT sz,const TSonorkUserServer&);

	UINT
		E_SetUserServer_R(UINT sz,const TSonorkUserServer&);

	UINT
		E_ListUserServers_R(UINT sz,const TSonorkUserServer& data, DWORD max_count=256);

	bool
		D_ListUserServers_A(UINT sz,TSonorkUserServerQueue&);

	UINT
		E_DelUserServer_R(UINT sz,const TSonorkSid&,DWORD server_no);

// ----------------------------------------------------------------------------
// Messages

	UINT
		E_PutMsg_R(UINT sz,const TSonorkUserLocus1*locus,const TSonorkMsg*msg);

	UINT
		E_PutMsg_From_R(UINT sz, const TSonorkId* target,const TSonorkMsg*msg);

	bool
		D_PutMsg_A(UINT sz,TSonorkObjId *msg_id);

	bool
		D_OldMsgNotify_E(UINT P_size,TSonorkOldMsgNotificationList*);

	bool
		D_MsgDelivery_E(UINT P_size,TSonorkObjId *msg_id, TSonorkMsg*);

// ----------------------------------------------------------------------------
// Control Messages

	UINT
		E_CtrlMsg_R(UINT A_size
			, const TSonorkUserLocus1*
			, const TSonorkCtrlMsg*
			, const void*data
			, DWORD data_size);

	bool
		D_CtrlMsg_A(UINT P_size, TSonorkError&ERR)
		{ return D_Error_A(P_size,ERR);}

	bool
		D_CtrlMsg_E(UINT P_size, TSonorkUserLocus1*, TSonorkCtrlMsg*,TSonorkDynData*);

	bool
		D_OldSid_E(UINT size,TSonorkOldSidNotificationList*);

	bool
		D_Sid_E(UINT size,TSonorkSidNotificationQueue*);


	UINT
		E_UserList_R();
		
	UINT
		E_WappList_R();

	bool
		D_UserListUsers_A(UINT sz,TSonorkUserDataNotesQueue& set);

	bool
		D_UserGroups_A(UINT sz,TSonorkGroupQueue& set);

	bool
		D_ListWapps_A(UINT sz , TSonorkWappDataQueue& set);

	UINT
		E_IdentifyUser_R(UINT sz
				,const TSonorkUserLocus1&
				,const SONORK_DWORD4& 		pin
				,SONORK_PIN_TYPE			pin_type
				,SONORK_SERVICE_ID			service_id
				,DWORD					service_pin
				);
	bool
		D_IdentifyUser_A(UINT sz,TSonorkUserData&,DWORD&id_flags);

	UINT
		E_GetSysMsgs_R(UINT sz, const TSonorkTime&, const TSonorkRegion&);

	bool
		D_GetSysMsgs_A(UINT sz, TSonorkSysMsgQueue&);

	UINT
		E_SetSid_R(UINT sz
			, const TSonorkSidFlags&
			, const TSonorkRegion&
			, const TSonorkPhysAddr& service_address
			, const TSonorkDynString& msg);

	bool
		D_SetSid_A(UINT sz
			, TSonorkSidFlags&
			, TSonorkRegion&
			, TSonorkPhysAddr& service_address);

	UINT
		E_GetMsg_R(UINT size,TSonorkObjId *msg_id,DWORD ctrl_flags);

	bool
		D_GetMsg_A(UINT size,TSonorkMsg*);

	UINT
		E_WappUrl_R(UINT sz,DWORD app_id,DWORD url_id);

	bool
		D_WappUrl_A(UINT sz,TSonorkShortString& url,DWORD& app_pin);

// ----------------------------------------------------------------------------
// Ping & SysMsgs

	UINT
		E_DelMsg_R(UINT size,DWORD count, TSonorkObjId *msg_id);

	UINT
		E_Ping_R();


// ----------------------------------------------------------------------------
// Trackers

	UINT
		E_ListTrackers_R(UINT sz
			, const TSonorkTrackerId&
			, BOOL  rooms
			, DWORD max_count=256);

	UINT
		E_RegisterTracker_R(UINT sz
			, BOOL append
			, TSonorkTrackerDataQueue& queue);

	bool
		D_ListTrackerRooms_A(UINT sz
			,TSonorkTrackerRoomQueue&);

	bool
		D_ListTrackerMembers_A(UINT sz
			,TSonorkTrackerMemberQueue& queue);

#endif

#if defined(SONORK_SERVER_BUILD)

# if defined(SONORK_MONITOR_BUILD)
	UINT
		E_MonitorCommand_R(UINT 	sz
			, const TSonorkMonitorCmdAtom* A);
# endif
	bool
		D_MonitorCommand_R(UINT sz
			, TSonorkMonitorCmdAtom* A);
		
// ----------------------------------------------------------------------------
// User Data & Search

	bool
		D_SetUserData_R(UINT sz, TSonorkUserDataNotes&);

	UINT
		E_SetUserData_A(UINT sz, const TSonorkSerial&);

	bool
		D_GetUserData_R(UINT 	sz
			,TSonorkId& 	user_id
			,DWORD&		level
			,DWORD&		flags
			,DWORD&		pin
			,DWORD&		reserved);

	UINT
		E_GetUserData_A(UINT sz,const TSonorkUserDataNotes&,UINT enc_flags);

	bool
		D_SetPass_R(UINT sz,TSonorkShortString& pass);

	bool
		D_SearchUser_R_NEW(UINT sz
		, TSonorkUserDataNotes&
		, DWORD& search_flags
		, DWORD& max_count);

	bool
		D_SearchUser_R_OLD(UINT sz, TSonorkId&
			, TSonorkShortString& alias
			, TSonorkShortString& name
			, TSonorkShortString& email
			, DWORD& max_count);

	UINT
		E_SearchUser_A(UINT size
				, TSonorkUserDataNotesQueue& list
				, UINT enc_flags
				, SONORK_FUNCTION p_func);

	bool
		D_CreateUser0_R(UINT sz,TSonorkUserDataNotes&);

	bool
		D_CreateUser1_R(UINT sz,DWORD&referrer_id,TSonorkUserDataNotes&,TSonorkDynData&);

	UINT
		E_CreateUser_A(UINT sz,const TSonorkId&);

// ----------------------------------------------------------------------------
// Groups

	bool
		D_SetGroup_R2(UINT sz, TSonorkGroup&UG);

	UINT
		E_SetGroup_A2(UINT sz, DWORD subfunc, const TSonorkGroup&UG);

// ----------------------------------------------------------------------------
// Authorizations

	bool
		D_ReqAuth_R(UINT sz,TSonorkId&,TSonorkAuth2&auth,TSonorkText&);

	UINT
		E_ReqAuth_A(UINT sz,const TSonorkId&);

	bool
		D_AknAuth_R(UINT sz,TSonorkId&,TSonorkAuth2&);

	UINT
		E_AknAuth_A(UINT sz,const TSonorkId&,const TSonorkAuth2&);

	bool
		D_SetAuth_R(UINT sz,TSonorkId&,TSonorkAuth1&);

	UINT
		E_SetAuth_A(UINT sz,const TSonorkId&,const TSonorkAuth2&);

	bool
		D_DelAuth_R(UINT sz,TSonorkId&);

	UINT
		E_DelAuth_A(UINT sz,const TSonorkId&);

// ----------------------------------------------------------------------------
// Services

	bool
		D_AddService_R(UINT sz,TSonorkServiceData&);
	UINT
		E_AddService_A(UINT sz,const TSonorkServiceLocus1& svc_locus);

	bool
		D_ListServices_R(UINT sz
			, TSonorkServiceInstanceInfo&
			, TSonorkServiceState&
			, DWORD& max_count);

	UINT
		E_ListServices_A(UINT sz,TSonorkServiceDataQueue&);

// ----------------------------------------------------------------------------
// User servers

	bool
		D_AddUserServer_R(UINT sz,TSonorkUserServer&);

	bool
		D_SetUserServer_R(UINT sz,TSonorkUserServer&);

	bool
		D_ListUserServers_R(UINT sz, TSonorkUserServer& data, DWORD& flags, DWORD& max_count);

	UINT
		E_ListUserServers_A(UINT sz,TSonorkUserServerQueue&);

	bool
		D_DelUserServer_R(UINT sz,TSonorkSid&,DWORD&);


// ----------------------------------------------------------------------------
// Messages

	bool D_PutMsg_R(UINT sz
		, TSonorkDataPacketMsgTarget& 	target
		, TSonorkMsg& 		msg);

	bool D_PutMsg_From_R(UINT sz
		, TSonorkDataPacketMsgTarget&	target
		, TSonorkMsg&		msg);

	UINT
		E_PutMsg_A(UINT sz,SONORK_FUNCTION f,const TSonorkObjId& msg_id);

	bool
		D_PutData_R(UINT sz,TSonorkUserLocus1	*locus	,TSonorkDynData&);


	UINT
		E_CtrlMsg_A(UINT A_size, const TSonorkError&ERR)
		{ return E_Error_A(A_size,SONORK_FUNCTION_CTRL_MSG,ERR);}


	// Ctrl msg event
	UINT
		E_CtrlMsg_E(UINT A_size, const TSonorkUserLocus1*, const void*data, UINT data_size);

	// Old Sid event
	UINT
		E_OldSid_E(UINT size,UINT items,TSonorkOldSidNotification*);

	// New Sid event
	UINT
		E_Sid_E(UINT size,const TSonorkSidNotificationAtom&);

	// New Sid event
	UINT
		E_Sid_E(UINT size,TSonorkSidNotificationQueue&);

	// Msg notify event
	UINT
		E_MsgNotify_E(UINT A_size
			,UINT items
			, TSonorkOldMsgNotification*);

	// Msg delivery event
	UINT
		E_MsgDelivery_E(UINT sz,TSonorkObjId *msg_id,TSonorkMsg*);

	UINT
		E_UserListUsers_A(UINT sz
				, TSonorkUserDataNotesQueue& set
				, UINT enc_flags);

	UINT
		E_AtomQueue_A(UINT 		sz
			, SONORK_FUNCTION
			, DWORD 		subfunc_ver_type
			, TSonorkAtomQueue& 	set);

	bool
		D_IdentifyUser_R(UINT 		sz
			,TSonorkUserLocus1& 	locus
			,SONORK_DWORD4& 	pin
			,DWORD&			pin_type
			,DWORD&			service_id
			,DWORD&			service_pin);

	UINT
		E_IdentifyUser_A(UINT sz
			,const TSonorkUserData&	user_data
			,DWORD 			id_flags);

	bool
		D_SetSid_R(UINT sz
			, TSonorkSidFlags&	flags
			, TSonorkRegion&        region
			, TSonorkPhysAddr& 	service_address
			, TSonorkDynString&	sid_msg);

	UINT
		E_SetSid_A(UINT sz
			, const TSonorkSidFlags&
			, const TSonorkRegion&
			, const TSonorkPhysAddr& service_address );

	bool
		D_GetMsg_R(UINT size,TSonorkObjId *msg_id,DWORD*ctrl_flags);

	UINT
		E_GetMsg_A(UINT size,const TSonorkMsg*);

	bool
		D_DelMsg_R(UINT size,DWORD& count, TSonorkObjId *msg_id);

	bool
		D_WappUrl_R(UINT sz,DWORD*app_no,DWORD*app_id);

	UINT
		E_WappUrl_A(UINT sz,const TSonorkShortString& url,DWORD app_pin);

// ----------------------------------------------------------------------------
// Ping & SysMsgs

	UINT
		E_Ping_A();

	bool
		D_GetSysMsgs_R(UINT sz, TSonorkTime&, TSonorkRegion&);

// ----------------------------------------------------------------------------
// Trackers

	bool
		D_ListTrackers_R(UINT sz
			, TSonorkTrackerId&
			, DWORD& max_count);
	bool
		D_RegisterTracker_R(UINT sz
			,TSonorkTrackerDataQueue& queue);

#endif


	UINT
		E_Data_E(UINT A_size,TSonorkDynData&);

	bool
		D_Data_E(UINT P_size,TSonorkDynData&);

	UINT
		E_Error_A(UINT A_size, SONORK_FUNCTION, const TSonorkError&E);

	bool
		D_Error_A(UINT P_size, TSonorkError&E);


// NOTE: The methods that have the "sys_" Prefix (i.e. sys_E_Ping_R)
// are special functions that should be invoked using their
// corresponding methods in TSonorkClient, these type of packets should
// not be used DIRECTLY by the host application because TSonorkClient
// does extra maintenance work on them.


}__SONORK_PACKED;


struct TSonorkDataPacket 		*SONORK_AllocDataPacket		(DWORD data_size);
inline void   				 SONORK_FreeDataPacket		(TSonorkDataPacket*P){SONORK_MEM_FREE(P);}


#if defined(USE_PRAGMA_PUSH)
#pragma	pack(pop)
#endif


#endif	//if !defined(SONORK_IPC_BUILD)

#endif

