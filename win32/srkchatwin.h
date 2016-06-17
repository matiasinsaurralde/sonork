#if !defined(SRKCHATWIN_H)
#define SRKCHATWIN_H

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

#if !defined(SRKTASKWIN_H)
# include "srktaskwin.h"
#endif

#if !defined(SRKWAITWIN_H)
# include "srkwaitwin.h"
#endif

#if !defined(SRKCONSOLE_H)
# include "srkconsole.h"
#endif

#if !defined(SRKLISTVIEW_H)
# include "srklistview.h"
#endif



#define SONORK_CHAT_VERSION_NUMBER		MAKE_VERSION_NUMBER(1,0,0,0)

#define SONORK_CHAT_NICK_DATA_ATOM		SONORK_SERVICE_ATOM_001
#define SONORK_CHAT_ROOM_DATA_ATOM		SONORK_SERVICE_ATOM_002
#define SONORK_CHAT_SERVER_DATA_ATOM		SONORK_SERVICE_ATOM_003
#define SONORK_CHAT_TEXT_DATA_ATOM		SONORK_SERVICE_ATOM_004
#define SONORK_CHAT_INVITE_DATA_ATOM		SONORK_SERVICE_ATOM_005


#define SONORK_CHAT_TEXT_FLAG_SYSTEM		SONORK_CCLF_01
#define SONORK_CHAT_TEXT_FLAG_URL		SONORK_CCLF_02


enum SONORK_CHAT_NICK_FLAGS
{
  SONORK_CHAT_NICK_F_CONNECTED	= 0x00001
, SONORK_CHAT_NICK_F_JOINED	= 0x00002
, SONORK_CHAT_NICK_F_OPERATOR	= 0x00004
, SONORK_CHAT_NICK_F_SERVER	= 0x00008
};

#define SONORK_CHAT_TEXT_TAG_FLAG		0
#define SONORK_CHAT_TEXT_TAG_ATTR		1

enum 	SONORK_CHAT_CMD;

enum SONORK_CHAT_OLD_CTRLMSG_PARAM0
{
	SONORK_CHAT_OLD_CTRLMSG_PARAM0_LIST_ROOMS	=	1
,	SONORK_CHAT_OLD_CTRLMSG_PARAM0_INVITE
};

class TSonorkChatWin;

// --------------------------------------------------------------------------

struct TSonorkChatNickData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD		nickId;
		DWORD		nick_flags;
		DWORD		reserved[2];
	}__SONORK_PACKED;
	HEADER			header;
	TSonorkShortString	nick;
	TSonorkUserData		user_data;

	DWORD
		NickId()	const
		{ return header.nickId;}

	DWORD
		NickFlags()	const
		{ return header.nick_flags;}

//----------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_CHAT_NICK_DATA_ATOM; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;

}__SONORK_PACKED;

struct TSonorkChatJoinAkn1
:public TSonorkChatNickData::HEADER
{
	DWORD	tracking_no_prefix;
	DWORD	reserved[5];
};

// --------------------------------------------------------------------------

struct TSonorkChatTextData
:public TSonorkCodecAtom
{

	struct HEADER_V1
	{
		TSonorkTag	tag;
		DWORD		sender_nick_id;
	}__SONORK_PACKED;

	struct HEADER_V2
	:public HEADER_V1
	{
		DWORD		target_nick_id;
		SONORK_DWORD2	tracking_no;
	}__SONORK_PACKED;

	HEADER_V2		header;
	TSonorkDynString	text;
	TSonorkDynData		data;

	DWORD
		TextFlags()
		const { return header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG];}
		
	DWORD
		TextAttrs()
		const { return header.tag.v[SONORK_CHAT_TEXT_TAG_ATTR];}
//----------
// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_CHAT_TEXT_DATA_ATOM; 	}

private:

	void
			CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;

}__SONORK_PACKED;

// --------------------------------------------------------------------------


// --------------------------------------------------------------------------

enum SONORK_CHAT_SERVER_FLAGS
{
	SONORK_CHAT_SERVER_F_PRIVATE	= 0x0010000
,	SONORK_CHAT_SERVER_F_PUBLIC	= 0x0020000
};

struct TSonorkChatServerData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		TSonorkUTSDescriptor	descriptor;
		TSonorkPhysAddr		physAddr;
		DWORD			nickId;	// Nick id of server
		DWORD			version;
		DWORD			userCount;
		DWORD			flags;
		DWORD			reserved[2];
	} __SONORK_PACKED;
	HEADER			header;
	TSonorkDynString	room_name;
	TSonorkDynString	room_topic;
	TSonorkDynData		data;

	DWORD
		NickId()	const
		{
			return header.nickId;
		}

	DWORD
		ServerFlags()	const
		{
			return header.flags;
		}

	void
		SaveInto(TSonorkUserServer*);

	void

		LoadFrom(const TSonorkUserServer*);
//----------

// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_CHAT_SERVER_DATA_ATOM; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;

}__SONORK_PACKED;


// --------------------------------------------------------------------------

struct TSonorkChatInviteData
:public TSonorkCodecAtom
{
	struct HEADER
	{
		DWORD	reserved[16];
	} __SONORK_PACKED;
	HEADER			header;
	TSonorkChatNickData	nick_data;
	TSonorkChatServerData 	server_data;
	TSonorkText             text;

//----------

// CODEC

public:

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
			CODEC_DataType()	const
			{ 	return SONORK_CHAT_INVITE_DATA_ATOM; 	}

private:

	void
			CODEC_WriteDataMem	(TSonorkCodecWriter&) const;

	void
			CODEC_ReadDataMem	(TSonorkCodecReader&);

	DWORD
			CODEC_DataSize()	const 	;

}__SONORK_PACKED;


// --------------------------------------------------------------------------
enum CR_INIT_PAGE
{
		CR_INIT_CLIENT=0
	,	CR_INIT_SERVER
	,	CR_INIT_PAGES
};
enum CR_QUERY_MODE
{
	CR_QUERY_NONE
,	CR_QUERY_USER
,	CR_QUERY_GLOBAL
};

class TSonorkChatSetWin
:public TSonorkWin
{
	BOOL is_server,is_operator;
	TSonorkChatServerData	*server;
	DWORD * room_color;
	DWORD	nick_flags;
	bool	OnCreate();
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	UpdateCheckboxes();
	void	CmdChooseColor();
public:
	TSonorkChatSetWin(TSonorkWin*parent
		,TSonorkChatServerData*
		,DWORD nick_flags
		,DWORD *room_color);
};
class TSonorkChatInitWin
:public TSonorkTaskWin
{

	struct {
		HWND		hwnd;
		CR_INIT_PAGE	page;
		TSonorkWin*	win[CR_INIT_PAGES];
	}tab;

	HWND			status_hwnd;
	TSonorkExtUserData*	context_user;
     	CR_QUERY_MODE		query_mode;
	DWORD			service_instance;
	struct {
		UINT			query_id;
		UINT			query_msecs;
		TSonorkListView		view;
	}client;

	struct{
		HWND			room;
	}server;

	bool	OnCreate();
	void	OnAfterCreate();
	void	OnBeforeDestroy();
	void	OnTimer(UINT);

	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT	OnChildDialogNotify(struct TSonorkChildDialogNotify*);

	static DWORD SONORK_CALLBACK
		ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);

	void	OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size);
	void	OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&, const TSonorkError*);


	void	UpdateInterface(bool update_cbs);
	bool	OnAppEvent(UINT event, UINT param,void*data);
	void		OnAppEvent_UserSid(TSonorkAppEventLocalUserSid*);
	void		OnAppEvent_UserDel(const TSonorkId*);
	bool		OnAppEvent_OldCtrlMsg(TSonorkAppEventOldCtrlMsg*);
	void	GetDefaultRoomName(char*);

	bool
		CmdStartServer();
		
	bool
		CmdStartClient();

	void
		SetPage(CR_INIT_PAGE pg , bool manual_set);
	void
		SERVER_UpdateCheckboxes();

	void
		CLIENT_StartQuery();
	void
		CLIENT_EndQuery();
	// AddServerData
	// Adds a server entry to the client list
	void
		CLIENT_AddServer(const TSonorkId&,TSonorkChatServerData*);
	void
		CLIENT_ClearServers();
public:
	TSonorkChatInitWin(TSonorkExtUserData*);
};


class TSonorkChatWin
:public TSonorkWin
{
public:
	enum CHAT_MODE
	{
		CHAT_MODE_NONE
	,	CHAT_MODE_CLIENT
	,	CHAT_MODE_SERVER
	};

private:
	CHAT_MODE   		chat_mode;
	TSonorkAtomDb		db;
	TSonorkCCache*		cache;
	TSonorkConsole*		console;

	TSonorkListView		userListView;
	int					userListWidth;
	TSonorkDropTarget	userListDropTarget;
	int					userListDropIndex;


	HWND				inputHwnd;
	int					inputHeight;
	TSonorkWinCtrl		inputCtrl;
	TSonorkDropTarget	inputDropTarget;


	struct
	{
		HWND	hwnd;
		int	height;
	}toolbar;
	
	struct {
		int x,y;
		TSonorkUltraMinWin*	win;
	}ultra_min;
	
	DWORD			main_link_id;
	DWORD			menu_nick_id;
	DWORD			service_instance;
	TSonorkTime		last_sound_time;

	struct unROOM
	{
		DWORD			nickId;
		DWORD			nick_flags;
		DWORD			tracking_no_prefix;
		DWORD			tracking_no_sequence;
		COLORREF		color;
		TSonorkChatServerData	server;
		DWORD			last_update_msecs;
	}room;


	class TSonorkUTS*		guts;
	TSonorkError			gutsERR;
	TSonorkCCacheLinkContext	link_ctx;

	BOOL	OnQueryClose();

	bool	OnCreate();
	void	OnBeforeDestroy();
	void	OnTimer(UINT);
	bool 	OnCommand(UINT id,HWND hwnd, UINT notify_code);
	void	OnActivate(DWORD flags, BOOL minimized);
	LRESULT	OnNotify(WPARAM,TSonorkWinNotify*);
	LRESULT	OnPoke(SONORK_WIN_POKE,LPARAM);
	LRESULT OnDragDrop(SONORK_DRAG_DROP_EVENT,LPARAM);
	LRESULT OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool	OnDrawItem(DRAWITEMSTRUCT*);
	bool	OnMinMaxInfo(MINMAXINFO*);

	void 	OnSize(UINT);
	bool	OnAppEvent(UINT event, UINT param,void*data);
	bool		OnAppEvent_OldCtrlMsg(TSonorkAppEventOldCtrlMsg*);
	void		OnAppEvent_SetProfile();
//	bool		OnAppEvent_ServerRegistered(TSonorkAppTask_RegisterServer*);
	static	void SONORK_CALLBACK
		UtsEventHandler(void*,struct TSonorkUTSEvent*);
	void UtsEventLogin(TSonorkUTSEvent*E);
	void UtsEventStatus(TSonorkUTSEvent*E);
	void UtsEventData(TSonorkUTSEvent*E);
	void	ProcessDataInClientMode(SONORK_CHAT_CMD, const BYTE* , DWORD);
	void	ProcessDataInServerMode(TSonorkUTSLink* , SONORK_CHAT_CMD, const BYTE* , DWORD);

	SONORK_RESULT
		SendAtom(DWORD targetID,SONORK_CHAT_CMD,BYTE ver,const TSonorkCodecAtom*);

	SONORK_RESULT
		SendRaw(DWORD targetID,SONORK_CHAT_CMD,BYTE ver,const void*,DWORD);

	void
		BroadcastAtom(DWORD sourceID,SONORK_CHAT_CMD,BYTE ver,const TSonorkCodecAtom*);

	void
		BroadcastRaw(DWORD sourceID,SONORK_CHAT_CMD,BYTE ver,const void *,DWORD);

	void
		BroadcastPacket(DWORD sourceID,TSonorkUTSPacket*,UINT P_size);

	void
		SendUserList(DWORD);

	void Send_Clnt2Svr_JoinReq();
	void Send_Clnt2Svr_SetUserFlags(const TSonorkChatNickData*,DWORD flags,DWORD mask);

	void On_Clnt2Svr_JoinReq(TSonorkUTSLink*,const BYTE*,DWORD);
	void On_Clnt2Svr_Text(TSonorkUTSLink*,const BYTE*,DWORD);
	void On_Clnt2Svr_UserUpdate(TSonorkUTSLink*,const BYTE*,DWORD);
	void On_Clnt2Svr_SetUserFlags(TSonorkUTSLink*,const BYTE*,DWORD);
	void On_Clnt2Svr_SetServerData(TSonorkUTSLink*,const BYTE*,DWORD);

	void On_Svr2Clnt_JoinAkn(const BYTE*,DWORD);
	void On_Svr2Clnt_ServerData(const BYTE*,DWORD);
	void On_Svr2Clnt_UserJoined(const BYTE*,DWORD);
	void On_Svr2Clnt_UserUpdate(const BYTE*,DWORD);
	void On_Svr2Clnt_UserLeft(const BYTE*,DWORD);
	void On_Svr2Clnt_Text(const BYTE*,DWORD);


	void
		AddUser(const TSonorkChatNickData*);

	void
		AddUserText( TSonorkChatTextData* );

	void
		AddText(SONORK_C_CSTR
			, const SONORK_DWORD2& tag
			, const SONORK_DWORD2& tracking_no
			, DWORD nickId);

	void
		AddSysLine(GLS_INDEX);

	void
		AddSysLine(SONORK_C_CSTR);

	bool
		OnUserDataChanged(const TSonorkChatNickData*, bool copy_data , bool copy_flags);

	void
		AddSetUserViewItem(const TSonorkChatNickData*, int index);

	int
		GetUser(DWORD pId, TSonorkChatNickData**pND , bool by_nick_id);

	void
		DelUser(DWORD);

	void
		ClearUserList();

	void	RealignControls();
	void	CmdSave();
	void	CmdConfig();

	void	CmdAuthorizeUser(TSonorkChatNickData*);
	void	CmdDisconnectUser(const TSonorkChatNickData*);

	void	LoadLabels();

	static DWORD SONORK_CALLBACK
		ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);


	static	BOOL SONORK_CALLBACK
		CCacheCallback(void*
			,TSonorkCCacheEntry*
			,char*
			,UINT size);

	static DWORD SONORK_CALLBACK
		ConsoleCallback(void*
			,SONORK_CONSOLE_EVENT 	pEvent
			,DWORD				pIndex
			,void*				pData);


	static void SONORK_CALLBACK
		UserResponseWaitCallback(
			  TSonorkWaitWin*	WW
			, LPARAM 		cb_param
			, LPVOID 		cb_data
			, SONORK_WAIT_WIN_EVENT event
			, LPARAM  		event_param);


	DWORD
		OnHistory_Event(TSonorkHistoryWinEvent*E);

	void
		OnHistory_LinePaint(const TSonorkCCacheEntry*, TSonorkHistoryWinPaintCtx*);

	void
		OnHistory_LineDrag(DWORD );

	DWORD
		OnConsole_Export(TSonorkConsoleExportEvent*);

	void
		OnHistoryWin_GetText(DWORD,TSonorkDynData*DD);



	// Uts engine start/stop
	// <max_users> is only used if CHAT_MODE is SERVER
	bool
		StartRoom( CHAT_MODE  , DWORD max_users);

	void
		StopRoom();

	void
		SendInputText(BOOL reply);

	void
		Poke_OpenUser(LPARAM);

	void
		SendFiles(TSonorkDropTarget*,TSonorkClipData* );
		
	void
		SetUserListDropTarget(int );

	void
		OnServerSettingsChange(
			  BOOL update_ui
			, BOOL broadcast_to_room);

	void
		UltraMinimizedPaint(struct TSonorkUltraMinPaint*);

	void	SetHintMode(SONORK_C_CSTR str, bool update_window)
			{ console->SetHintMode(str,update_window); }

public:
	TSonorkChatWin();
	~TSonorkChatWin();
	SONORK_C_CSTR	RoomName()
		{	return room.server.room_name.CStr(); }

	static TSonorkChatWin*
		CreateClient(const TSonorkChatServerData*);

	static TSonorkChatWin*
		CreateClient(const TSonorkUserServer*);

	static TSonorkChatWin*
		CreateServer(SONORK_C_CSTR,SONORK_C_CSTR title,DWORD max_users);

	bool
		Invite(const TSonorkUserData*UD, SONORK_C_CSTR text);

	bool
		SetPublic(BOOL set);

	void
		SetPrivate(BOOL set);

	void
		UpdateUI();

	void
		OpenLine(DWORD line_no);
		
		// NB!: LoadLocalNickData
		//  user data is extracted from
		//  the application profile and
		//  not from the user list.
	void
		LoadLocalNickData(TSonorkChatNickData*);

// New V1.5 service locator messages
	static DWORD SONORK_CALLBACK
		LocatorServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data);

// For compatibility with V1.04: Handles OLD ctrl messages
	static DWORD
		OldCtrlMsgHandler(
			  const TSonorkUserHandle& 	handle

			, const TSonorkCtrlMsg*		reqMsg
			, const void*			data
			, DWORD	 			data_size);

	static void
		Init(TSonorkExtUserData*UD);	// UD may be NULL

	BOOL
		IsUltraMinimized() const
		{ return ultra_min.win != NULL; }

	void
		UltraMinimize();

	void
		DoSound( bool forced );

};





#endif