#if !defined(SRKWIN32APP_H)
#define SRKWIN32APP_H

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

#if !defined(SONORK_APP_BUILD)
#	error Header is only for the Application build
#endif

#include "srkwin32app_defs.h"
#include "srkbicho_defs.h"
#include "srkwin.h"
#include "srk_data_types.h"
#include "srk_codec_io.h"
#include "srk_app_base.h"
#include "srk_atom_db.h"
#include "srk_language.h"
#include "srk_uts.h"

// ----------------------------------------------------------------------------
// Types declared but not defined in this header

// Windows
class	TSonorkMainWin;
class	TSonorkSlideWin;
class	TSonorkMainInfoWin;
class	TSonorkSysConsoleWin;
class	TSonorkHistoryWin;
class	TSonorkConsole;
class   TSonorkWaitWin;
class	TSonorkUltraMinWin;
struct 	TSonorkListView;
struct	TSonorkTreeView;
struct 	TSonorkListViewColumn;
struct  TSonorkChildDialogNotify;

// Services
struct  TSonorkServiceEntry;
struct  TSonorkServiceQueryEntry;
struct  TSonorkServiceCircuitEntry;

// External App
struct TSonorkExtApp;
struct TSonorkExtAppLoadInfo;
struct TSonorkExtAppLoadSection;
struct TSonorkExtAppPendingReq;

// Win32App
struct	TSonorkWin32AppInitData;

// Compression
class	TSonorkZip;
class	TSonorkZipStream;

// IPC
class	TSonorkWin32IpcServer;
struct	TSonorkIpcEntry;
struct  TSonorkIpcFileBlock;
// Enums
enum SONORK_WAIT_WIN_EVENT;

// ----------------------------------------------------------------------------
// Global strings

// Sonork stores the last folder where a file was open/saved
//  as a profile key named using the <szSonorkOpenSaveDirKey> format
//  so that next time the open/save dialog is used, it starts
//  at the last location. The path is stored as a TSonorkShortString
//  in the configuration file, under the currently open profile
//  user key using ReadProfileItem()/WriteProfileItem().
// The key name is "dir.%s" and "%s" should be any one of the
//  szSonorkOpenSaveDir_xxxxx strings.
// This is used by TSonorkWin32App::GetSavePath() and TSonorkWin32App::GetLoadPath() 
extern	SONORK_C_CSTR	szSonorkOpenSaveDirKey;		// "dir.%s"
extern	SONORK_C_CSTR	szSonorkOpenSaveDir_Export;	// "Export"
extern	SONORK_C_CSTR	szSonorkOpenSaveDir_ClipPath;	// "ClipPath"

extern	SONORK_C_CSTR	szSONORK;
extern	SONORK_C_CSTR	szSonorkId;
extern	SONORK_C_CSTR	szGuId;
extern	SONORK_C_CSTR	szWinRunKey;
extern	SONORK_C_CSTR	szStartMode;
extern	SONORK_C_CSTR	szInternet;
extern	SONORK_C_CSTR	szIntranet;
extern  SONORK_C_CSTR	szDefault;
extern 	SONORK_C_CSTR 	szSonorkClientSetup;
extern 	SONORK_C_CSTR 	szWappBase;



#define	SONORK_APP_TIMER_MSECS			50
#define	SONORK_APP_TIMER_SKIP			2

#if	SONORK_APP_TIMER_SKIP < 1
#	error SONORK_APP_TIMER_SKIP must be equal or larger than 1
#endif



#define	SONORK_WIN_TYPE_SONORK_MSG	   SONORK_WIN_TYPE_1
#define	SONORK_WIN_TYPE_USER_DATA	   SONORK_WIN_TYPE_2
#define	SONORK_WIN_TYPE_AUTH_REQ	   SONORK_WIN_TYPE_3
#define	SONORK_WIN_TYPE_MY_INFO		   SONORK_WIN_TYPE_4
#define SONORK_WIN_TYPE_IPC_FILE_TX	   SONORK_WIN_TYPE_5
#define SONORK_WIN_TYPE_CLIP		   SONORK_WIN_TYPE_6
#define SONORK_WIN_TYPE_SONORK_CHAT	   SONORK_WIN_TYPE_7

#define	WM_SONORK_ASYNC_NET_NAME_RESOLVE	(WM_SONORK_APP_01)
#define WM_SONORK_TRAY_MESSAGE			(WM_SONORK_APP_02)
#define WM_SONORK_SERVICE			(WM_SONORK_APP_03)
#define WM_SONORK_IPC				(WM_SONORK_APP_04)




// ----------------------------------------------------------------------------
//  NAME RESOLUTION
// ----------------------------------------------------------------------------


#define SONORK_APP_CX_RESOLVE_1ST		SONORK_APP_CX_RESOLVE_SOCKS

#define SONORK_APP_CX_RESOLVE_LAST		SONORK_APP_CX_RESOLVE_GU


struct TSonorkAppNetResolveData
{
	HANDLE			handle;
	HWND			owner_hwnd;
	bool			pseudo;
	TSonorkShortString	host;
	TSonorkPhysAddr		physAddr;
	BYTE			hostent[MAXGETHOSTSTRUCT];
	DWORD			tag;
};

struct TSonorkAppNetResolveResult
{
	TSonorkShortString*	host;
	TSonorkPhysAddr*	physAddr;
	TSonorkError		ERR;
	DWORD			tag;
};


struct TSonorkMsgHandleEx
:public TSonorkMsgHandle
{
	TSonorkError		ERR;
}__SONORK_PACKED;

DeclareSonorkQueueClass(TSonorkExtApp);
DeclareSonorkQueueClass(TSonorkAppTask);
DeclareSonorkQueueClass(TSonorkAppNetResolveData);


// ----------------------------------------------------------------------------
//  APPLICATION SERVICES
// ----------------------------------------------------------------------------


struct TSonorkServiceEntry;
struct TSonorkAppServiceEvent;

// ----------------------------------------------------------------------------
// Callback for the Application Services

typedef DWORD (SONORK_CALLBACK TSonorkAppServiceCallback)(
		 SONORK_DWORD2&			handler_tag
		,SONORK_APP_SERVICE_EVENT	event_id
		,SONORK_DWORD2*			event_tag
		,TSonorkAppServiceEvent*	event_data
		);
typedef TSonorkAppServiceCallback* TSonorkAppServiceCallbackPtr;

// ----------------------------------------------------------------------------
// Callback for the loading user list results

typedef void (SONORK_CALLBACK TSonorkAppSearchUserCallback)(
		 const TSonorkListView*		TV
		,TSonorkUserDataNotes*		UDN);
typedef TSonorkAppSearchUserCallback* TSonorkAppSearchUserCallbackPtr;
// ----------------------------------------------------------------------------

struct TSonorkServiceQueryEntry
{
	TSonorkServiceEntry*	entry;
	SONORK_DWORD2		tag;
	TSonorkId		user_id;
	DWORD			query_id;
	DWORD			flags;
	DWORD			cur_msecs;
	DWORD			max_msecs;
};

StartSonorkQueueClass(TSonorkServiceQueryQueue,TSonorkServiceQueryEntry);
EndSonorkQueueClass;

// ----------------------------------------------------------------------------

struct TSonorkServiceCircuitEntry
{
	TSonorkServiceEntry*	entry;
	SONORK_DWORD2		tag;
	TSonorkServiceCircuit	handle;
	DWORD			flags;
	DWORD			cur_msecs;
	DWORD			max_msecs;

};

StartSonorkQueueClass(TSonorkServiceCircuitQueue,TSonorkServiceCircuitEntry);
EndSonorkQueueClass;

// ----------------------------------------------------------------------------

struct
 TSonorkServiceEntry
{
	TSonorkServiceInstanceInfo	sii;
	DWORD				flags;
	SKIN_ICON			icon;
	TSonorkAppServiceCallbackPtr	cb_ptr;
	SONORK_DWORD2			cb_tag;
};

DeclareSonorkQueueClass(TSonorkServiceEntry);

// ----------------------------------------------------------------------------


struct TSonorkAppServiceEvent
{
	struct _MSG
	{
friend TSonorkWin32App;
friend TSonorkWin32IpcServer;
	protected:
		TSonorkServiceHandle	sender;
		const TSonorkCtrlMsg*	msg;
		const void*		data;
		DWORD			size;

	public:

	const	TSonorkServiceHandle&
			Sender() const
			{ return sender; }

	const	TSonorkId&
			SenderUserId() const
			{ return sender.userId;}
			
		SONORK_CTRLMSG_CMD
			Cmd() const
			{ return msg->Cmd(); }

		DWORD
			CmdFlags() const
			{ return msg->CmdFlags(); }

		DWORD
			QueryId() const
			{ return msg->QueryId(); }

		DWORD
			UserFlags() const
			{ return msg->UserFlags(); }

		DWORD
			UserParam() const
			{ return msg->UserParam(); }

		DWORD
			CircuitId() const
			{ return msg->CircuitId(); }

		const BYTE*
			Data() const
			{ return (const BYTE*)data; }

		DWORD
			DataSize() const
			{ return size;}

		void
			GetQueryHandle(TSonorkServiceSystem& query) const
			{
				query.TSonorkServiceHandle::Set(sender);
				query.systemId=QueryId();
			}
		void
			GetCircuitHandle(TSonorkServiceSystem& circuit) const
			{
				circuit.TSonorkServiceHandle::Set(sender);
				circuit.systemId=CircuitId();
			}

	};

	struct _CREQ
	:public _MSG
	{
		DWORD	accept_timeout;
		DWORD	accept_instance;
	};

	struct _REG
	{
		TSonorkServiceInstanceInfo*sii;

	};

	struct _CHNDL
	{
		TSonorkServiceCircuit*	handle;

		DWORD
			CircuitId() const
			{ return handle->CircuitId(); }

		void
			GetCircuitHandle(TSonorkServiceSystem& circuit) const
			{ circuit.Set(*handle);	}
	};

	struct _STR
	{
		TSonorkShortString*	str;
	};

	struct _BOOL
	{
		DWORD		value;
	};

	struct _RES
	{
		DWORD		id;
		SONORK_RESULT	result;

		DWORD
			QueryId() const
			{ return id; }

		DWORD
			CircuitId() const
			{ return id; }

		SONORK_RESULT
			Result() const
			{ return result;}

		SONORK_C_CSTR
			ResultName() const
			{ return SONORK_ResultName( result ); }
	};

	struct _SVR
	:public _MSG  
	{
		TSonorkUserServer*	data;
		BOOL
			IsRemoteQuery() const
			{ return msg != NULL; }

	};

	union {
		_MSG	cur_msg;
		_MSG	query_req;
		_MSG	query_akn;
		_RES	query_end;
		_CHNDL	circuit_open;
		_CHNDL	circuit_update;
		_CREQ	circuit_req;
		_MSG	circuit_data;
		_RES	circuit_close;
		_MSG	poke_data;
		_BOOL	busy;
		_BOOL	init;
		_BOOL	exported;
		_STR	get_name;
		_SVR	get_server;
	};
};

// ----------------------------------------------------------------------------

extern HINSTANCE sonork_app_instance;

struct TSonorkDesktopSize
{
	POINT					origin;
	POINT					limit;
	SIZE					size;
};

enum SONORK_SYS_DIALOG
{
  SONORK_SYS_DIALOG_INVALID	=-1
, SONORK_SYS_DIALOG_SYS_CONSOLE	= 0
, SONORK_SYS_DIALOG_MY_INFO
, SONORK_SYS_DIALOG_USER_SEARCH
, SONORK_SYS_DIALOG_GRP_MSG
, SONORK_SYS_DIALOG_CLIP
, SONORK_SYS_DIALOG_REMIND_LIST
, SONORK_SYS_DIALOG_REMIND_ALARM
, SONORK_SYS_DIALOG_EXT_APP_CONFIG
, SONORK_SYS_DIALOG_SERVICES
, SONORK_SYS_DIALOG_SNAP_SHOT
, SONORK_SYS_DIALOGS
};


class TSonorkWin32App
:public TSonorkAppBase
{
friend	TSonorkWin32IpcServer;
public:
	enum
	{
		OLD_MSG_CONSOLE_TEXT_SIZE		=256
	,	OLD_MSG_CONSOLE_CACHE_SIZE		=64
	,	MSG_CONSOLE_TEXT_SIZE			=512
	,	MSG_CONSOLE_CACHE_SIZE			=100
	,	SYS_CONSOLE_TEXT_SIZE			=192
	,	SYS_CONSOLE_CACHE_SIZE			=70
	};

private:
	// Put all this classes' data into a structure to
	// clearly diferenciate from the super classes' data.
	struct _Win32
	{
		UINT				run_flags;
		UINT				cfg_flags;
		SONORK_APP_CX_STATUS		cx_status;
		HWND				work_win;	// Invisible application window handle
		TSonorkMainWin*			main_win;	// User interface window
		TSonorkMainInfoWin*		info_win;	// Created by TSonorkMainWin
		TSonorkSlideWin*		slide_win;
		TSonorkBicho*			bicho;      // Created by TSonorkMainWin
		TSonorkUTS*			uts_server;
		TSonorkWinQueue			win_list;
		TSonorkAppNetResolveDataQueue	resolve_queue;
		TSonorkClientServerProfile	server_profile;
		TSonorkAppTaskQueue		task_queue;
		TSonorkShortString		root_dir;
		TSonorkShortString		data_dir;
		TSonorkShortString		temp_dir;
		TSonorkWin* 			sys_dialog[SONORK_SYS_DIALOGS];

		TSonorkZip*			zip;

		char				cfg_file[SONORK_APP_CONFIG_FILE_NAME_SIZE];
		char				cfg_key[SONORK_APP_CONFIG_FILE_NAME_SIZE+16];
		UINT				serial_no;
		TSonorkTime			cur_time;
		SONORK_SID_MODE			saved_auto_away_sid_mode;
	}win32;

	struct _REFERRER{
		DWORD			id;
		TSonorkShortString    name;
		TSonorkShortString	url;
	}referrer;

	struct _SERVICE_INSTANCE
	{
		DWORD		sonork;
		DWORD		ext_app;
		DWORD		chat;
		DWORD		tracker;
	}service_instance;

	struct _Menus{
		HMENU					global;
		HMENU					main;
		HMENU					user_popup;
		HMENU					user_visib;
		HMENU					user_auth;
		HMENU					ugrp_popup;
		HMENU					msgs_popup;
		HMENU					mfil_popup;
		HMENU					clip_popup;
		HMENU					usel_popup;
		HMENU					chat_view;
		HMENU					chat_user;
		HMENU					eapp_popup;
		HMENU					user_apps;
		HMENU					user_mtpl;
		HMENU					tray_icon;
	}menus;


	TSonorkExtAppQueue		ext_apps;


	int	counter[SONORK_APP_COUNTERS];

	TSonorkDesktopSize	desktop_size;

	struct _Lang
	{
		TSonorkShortString		name;
		TSonorkLanguageTable		table;
		TSonorkLangCodeTable		country;
		TSonorkLangCodeTable		language;
	}lang;


	struct _Console{
		struct _DTA
		{
			TSonorkMsg	msg;
		}dta;
		struct _TMP
		{
			TSonorkId	userId;
			DWORD		refCount;
			TSonorkCCache	*ptr;
		}tmp;
		TSonorkCCache	*sys;
	}console;


	struct _Db
	{
		TSonorkAtomDb	msg;
		TSonorkAtomDb	ext;
		TSonorkAtomDb	sys;
	}db;


	SONORK_DWORD2List	auth_exclude_list;

#define SONORK_APP_SERVICE_CACHE_SIZE	2
	struct _SvcCache
	{
		TSonorkClock		clk[SONORK_APP_SERVICE_CACHE_SIZE];
		TSonorkServiceData	data[SONORK_APP_SERVICE_CACHE_SIZE];
	}svc_cache;

	TSonorkCCache	*GrabTmpMsgCache(const TSonorkId&user_id);
	void		ReleaseTmpMsgCache(TSonorkCCache*);
	


static	LRESULT	CALLBACK			// Invisible application window procedure
		WinProc(HWND hwnd,UINT uMsg ,WPARAM wParam ,LPARAM lParam);
		
static	void SONORK_CALLBACK
		SonorkClientRequestHandler(void*,struct TSonorkClientRequestEvent*);

static	void SONORK_CALLBACK
		SonorkUtsRequestHandler(void *param, const SONORK_DWORD2& packet_tag, const TSonorkUTSLink*, const TSonorkError*);

static	void SONORK_CALLBACK
		SonorkUtsEventHandler(void*,struct TSonorkUTSEvent*);

static	BOOL SONORK_CALLBACK
		MsgCCacheCallback(void*,TSonorkCCacheEntry*,char*,UINT size);

static	BOOL SONORK_CALLBACK
		SysCCacheCallback(void*,TSonorkCCacheEntry*,char*,UINT size);

	// StartSonorkRequests takes ownership of <P> and deletes it
	SONORK_RESULT
		StartSonorkRequest(TSonorkDataPacket*P,UINT P_size,const SONORK_DWORD2* tag,TSonorkError*);

	void		WmSonorkAsyncNetNameResolve(WPARAM wParam ,LPARAM lParam);
	void 		WmSonorkAppCommand(WPARAM wParam ,LPARAM lParam);
	void		WmSonorkService(WPARAM wParam ,LPARAM lParam);
static	LRESULT		WmSonorkIpc(WPARAM wParam ,LPARAM lParam);
	void		WmSonorkTrayMessage(WPARAM wParam ,LPARAM lParam);
	void 		WmSonorkPoke(WPARAM wParam ,LPARAM lParam);
	void		WmSettingChange(WPARAM wParam ,LPARAM lParam);
	BOOL		WmCopyData(HWND senderHwnd, const COPYDATASTRUCT*CD);



	void		SetCxStatus(SONORK_APP_CX_STATUS,const TSonorkError&);
	void		OnCxNetNameResolve(TSonorkAppNetResolveResult*);



// ----------------------------------------------------------------------------
// Initialization, main loop and cleanup

private:
	bool
		Init(const char*cmd_line);

	void
		Init_MainWin();

	bool
		Init_Lang(SONORK_C_CSTR, BOOL update_user_reg_key);

	bool
		Init_User(TSonorkWin32AppInitData*CFG);

	bool
		Init_Services();

	bool
		Init_Ipc();

	void
		Main();

	void
		Exit();

	void
		Exit_Services();

	void
		Exit_Ipc();
// ----------------------------------------------------------------------------
// Services

public:

	// Built-in services instances

	DWORD
		SonorkServiceInstance() const
		{ return service_instance.sonork;}

	DWORD
		ExtAppServiceInstance() const
		{ return service_instance.ext_app;}

	DWORD
		ChatServiceInstance() const
		{ return service_instance.chat;}

	DWORD
		TrackerServiceInstance() const
		{ return service_instance.tracker;}

	static SONORK_C_CSTR
		Service_GetDescription(
			  const TSonorkServiceInstanceInfo&	sii
			, SONORK_C_CSTR*        type=NULL
			, SKIN_ICON*		icon=NULL);

	SONORK_RESULT
		Service_Register(
		  SONORK_APP_SERVICE_TYPE	svc_type
		, TSonorkServiceInstanceInfo*   svc_info
		, SKIN_ICON			svc_icon
		, TSonorkAppServiceCallbackPtr	cb_ptr
		, SONORK_DWORD2* 		cb_tag);

	SONORK_RESULT
		Service_SetCallback(
		  DWORD 			instance_id
		, TSonorkAppServiceCallbackPtr	cb_ptr
		, SONORK_DWORD2* 		cb_tag);

	// When unregistering a server, the <service_id> is needed only
	// if the server is a locator (has <instance_id>==0)
	SONORK_RESULT
		Service_Unregister(DWORD instance_id
			, BOOL invoke_query_callbacks
			, SONORK_SERVICE_ID service_id=SONORK_SERVICE_ID_NONE);

	SONORK_RESULT
		Service_ExportService(DWORD instance_id, BOOL exported);

	SONORK_RESULT
		Service_SendPokeData(
			  DWORD			source_instance
			, TSonorkServiceHandle*	target_server
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const void*		data_ptr
			, DWORD			data_size);


	SONORK_RESULT
		Service_SendPokeData(
			  DWORD			source_instance
			, TSonorkServiceHandle*	target_server
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom*A);

	SONORK_RESULT
		Service_SendCircuitData(
			  DWORD			source_instance
			, TSonorkServiceCircuit*target_server
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const void*		data_ptr
			, DWORD			data_size);


	SONORK_RESULT
		Service_SendCircuitData(
			  DWORD			source_instance
			, TSonorkServiceCircuit*target_server
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom*A);

	SONORK_RESULT
		Service_StartQuery(
			  DWORD			source_instance
			, TSonorkServiceQuery*	query_handle
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		user_param
			, DWORD 		user_flags
			, DWORD			timeout_msecs
			, const void*		data_ptr
			, DWORD			data_size

			, SONORK_DWORD2*	query_tag);


	SONORK_RESULT
		Service_StartQuery(
			  DWORD			source_instance
			, TSonorkServiceQuery*	query_handle
			, SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		user_param
			, DWORD 		user_flags
			, DWORD			timeout_msecs
			, const TSonorkCodecAtom*A
			, SONORK_DWORD2*	query_tag);

	SONORK_RESULT

		Service_OpenCircuit(
			  DWORD			source_instance
			, TSonorkServiceCircuit*circuit
			, DWORD 		user_param
			, DWORD 		user_flags
			, DWORD			timeout_msecs
			, const void*		data_ptr
			, DWORD			data_size

			, SONORK_DWORD2*	circuit_tag);


	SONORK_RESULT
		Service_OpenCircuit(
			  DWORD			source_instance
			, TSonorkServiceCircuit*circuit
			, DWORD 		user_param
			, DWORD 		user_flags
			, DWORD			timeout_msecs
			, const TSonorkCodecAtom*A
			, SONORK_DWORD2*	circuit_tag);

	// Must load circuit->systemId with the desired
	// circuit_id before calling.
	SONORK_RESULT
		Service_AcceptCircuit(
			  DWORD			old_source_instance
			, DWORD			new_source_instance
			, TSonorkServiceCircuit*circuit);

	// Must load circuit->systemId with the desired
	// circuit_id before calling.
	SONORK_RESULT

		Service_GetCircuitHandle(
			  DWORD source_instance
			, TSonorkServiceCircuit*);


	SONORK_RESULT

		Service_CloseCircuit(DWORD	source_instance
			, DWORD			circuit_id
			, SONORK_RESULT		result);

	SONORK_RESULT
		Service_Reply(
			  SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const void*		data_ptr
			, DWORD			data_size);


	SONORK_RESULT
		Service_Reply(
			  SONORK_CTRLMSG_CMD 	cmd
			, DWORD 		cmd_param
			, DWORD 		cmd_flags
			, const TSonorkCodecAtom*A);


	void

		Service_BeginEnum(TSonorkListIterator&);


	const TSonorkServiceEntry*

		Service_EnumNext(TSonorkListIterator&

			,TSonorkShortString*name);	// <name> may be NULL


	void

		Service_EndEnum(TSonorkListIterator&);


private:
	// Build in service locators

	static DWORD SONORK_CALLBACK
		ServiceCallback_Sonork(SONORK_DWORD2&	handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			event_tag

			,TSonorkAppServiceEvent*	E);

	void
		SonorkService_QueryPic(TSonorkAppServiceEvent*);

	void
		SonorkService_QueryServices(TSonorkAppServiceEvent*);

	static DWORD SONORK_CALLBACK
		ServiceCallback_ExtApp(SONORK_DWORD2&	handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			event_tag

			,TSonorkAppServiceEvent*	E);


	void
		Service_OnSonorkDisconnect();

	void
		Service_OnUserNotAvaiable(const TSonorkId&);

	void
		Service_OnUserUtsLink(const TSonorkId&, DWORD uts_link_id);

	void
		Service_CondemnQuery(TSonorkServiceQueryEntry*query
			,SONORK_RESULT
			,bool invoke_handler
			,bool clear_item);

	void
		Service_CondemnCircuit(TSonorkServiceCircuitEntry*query
			,SONORK_RESULT
			,bool invoke_handler
			,bool clear_item);

	void
		Service_Cleanup(SONORK_RESULT, bool invoke_handlers);

	void
		Service_ProcessCtrlMsg(
			  const TSonorkUserLocus1* sender
			, DWORD uts_link_id
			, const TSonorkCtrlMsg*msg
			, const void*data
			, DWORD data_size);
			
	void
		Service_ProcessSystemMsg();

	void
		Service_ProcessSystemMsg_OpenCircuit();

	void
		Service_ProcessSystemMsg_UtsLink();

	void
		Service_ProcessDataMsg();

	void
		Service_ProcessQueryMsg();

	void
		Service_ProcessCircuitMsg();

	SONORK_RESULT
		Service_SendCtrlMsg(
			  TSonorkServiceEntry*	source_entry
			, TSonorkServiceHandle*	target_handle
			, TSonorkCtrlMsg*	msg
			, const void*		data_ptr
			, DWORD			data_size

			, DWORD			task_type);


	SONORK_RESULT
		Service_SendCtrlMsg(
			  TSonorkServiceEntry*	source_entry
			, TSonorkServiceHandle*	target_handle
			, TSonorkCtrlMsg*	msg
			, const TSonorkCodecAtom*A);

static	TSonorkServiceEntry*
		Service_GetEntry(SONORK_SERVICE_ID , DWORD instance_id,BOOL registered_only);

static	TSonorkServiceQueryEntry*
		Service_GetQuery(DWORD instance_id,DWORD system_id);

static	TSonorkServiceCircuitEntry*
		Service_GetCircuit(DWORD instance_id,DWORD system_id);

	DWORD
		Service_DispatchEvent(
			  TSonorkServiceEntry*		E
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		event_tag
			, TSonorkAppServiceEvent*	event_data
			);

	SONORK_RESULT
		Service_ExportService(TSonorkServiceEntry* E);

	SONORK_RESULT
		Service_CancelExport(TSonorkServiceEntry* E);

	void
		Service_Unregister(TSonorkServiceEntry* E,BOOL invoke_query_callbacks);

	void
		Service_LockEntry(TSonorkServiceEntry* E, BOOL);

	void
		Service_ReplyCurMsg(SONORK_CTRLMSG_CMD
			, DWORD sending_instance
			, DWORD sys_flags
			, DWORD msg_param);

// ----------------------------------------------------------------------------
// Engine Handlers:
//  overload the virtual methods of TSonorkAppBase

private:

	void 	OnSonorkStatusChange(SONORK_NETIO_STATUS,const TSonorkError*,DWORD flags);
	void		OnSonorkConnect(const TSonorkError*,DWORD login_flags);
	void		OnSonorkDisconnect(const TSonorkError*,DWORD status_flags);
	void 	OnSonorkGlobalTask(SONORK_FUNCTION,const TSonorkError*);
	void 	OnSonorkUserProfileOpen(bool open);
	void 	OnSonorkUserAuth(TSonorkAuthReqData*);
	void	OnSonorkAddUser(TSonorkExtUserData*, DWORD msg_sys_flags);
	void 	OnSonorkSetUser(TSonorkExtUserData*,const TSonorkAuth2*,const TSonorkDynString*);
	void 	OnSonorkUserSid(TSonorkExtUserData*,const TSonorkUserLocus3&old_locus);
	void 			ProcessUserSidChange(TSonorkExtUserData*,const TSonorkUserLocus3*old_locus, BOOL update_ui);
	void	OnSonorkDelUser( const TSonorkId&, TSonorkExtUserData* );
	void	OnSonorkCtrlMsg(const TSonorkUserLocus1*,const TSonorkCtrlMsg*,TSonorkDynData*);
	void		OnOldCtrlMsg(const TSonorkUserHandle& sender,const TSonorkCtrlMsg*msg,const void*data, DWORD data_size);
	DWORD		OnOldCtrlMsg_ExtApp(const TSonorkUserHandle&,const TSonorkCtrlMsg*,const void*,DWORD);

	void	OnSonorkMsg(TSonorkMsg*);

	// returns modified pcFlags
	DWORD
		ConsumeMsgEvent(SONORK_APP_EVENT
				, TSonorkMsg*
				, DWORD  pcFlags
				, DWORD  ccFlags
				, TSonorkCCacheMark*
				, DWORD  handleId
				, DWORD  taskId
				);
	SONORK_LOGIN_MODE
		OnSonorkLoadLoginRequest(DWORD&);

	void 	OnSonorkAddWapp( const TSonorkWappData*WD );
	void 	OnSonorkSysMsg(DWORD index , TSonorkSysMsg* sm);


// ----------------------------------------------------------------------------
// Work win's timer

private:

	void	WmTimer();
	void	TimerTask_Blink(UINT);
	void	TimerTask_Monitor(UINT);
	void	TimerTask_Services(UINT);
	void	TimerTask_CheckCxStatus();
	void	TimerTask_CheckSysMsgs();
	void	TimerTask_SynchTrackers();
	UINT	TimerTask_CheckAlarms();// Returns delay for next check in SECONDS

// ----------------------------------------------------------------------------
// Reminders and alarms

public:

	void	AddAlarmItem(const struct TSonorkRemindData*);
	void	SaveAlarmItemHeader(const struct TSonorkRemindDataHeader*, bool delete_item);
	void	RequestCheckAlarms(UINT secs=0);
	void	RequestCheckEmailAccounts(UINT secs=0);
	void	RequestSynchTrackers(bool);
	void	CancelCheckEmailAccounts();


private:

// ----------------------------------------------------------------------------
// User Interface

	// Tray icon
	BOOL
		TrayMessage(DWORD dwMessage, HICON hIcon, SONORK_C_CSTR pszTip);

	HICON
		GetTrayIcon();

	// RebuildMenus: rebuilds all menus,
	// must be called after the language changes
	// It also calls RebuildTrayMenu().
	void
		RebuildMenus();

	 // RebuildTrayMenu() is included in but separate from RebuildMenus
	 // because the menu text is sensitive to the profile's "sex" setting
	void
		RebuildTrayMenu();


// ----------------------------------------------------------------------------
// Extern applications support

public:
	void
		GetExtAppsIniFile(SONORK_C_STR);
	void
		LoadExtApps();
	void
		LoadExtAppMenu();
	void
		ClearExtAppMenu();

	const TSonorkExtApp*
		GetExtApp(SONORK_C_CSTR app_name,SONORK_EXT_APP_CONTEXT,SONORK_C_STR view_name) const;

	// UpdateUserExtAppMenu:
	//  Enables/Disables items for the user menu
	//  depending on the connection state and sex of the user
	void
		UpdateUserExtAppMenu(const TSonorkUserData*UD);

	SONORK_RESULT
		LaunchExtApp(const TSonorkExtApp*,const TSonorkExtUserData*UD, bool init_mode);

private:

	SONORK_RESULT
		LaunchExtApp_Remote(
			 const TSonorkExtApp*		EA
			,const TSonorkExtUserData*	UD
			,TSonorkExtAppPendingReq*	pending_req);


private:

	void
		LoadExtAppInfo(TSonorkExtAppLoadInfo&);

	TSonorkExtApp*
		LoadExtAppInfo_Reg(TSonorkExtAppLoadSection*);

	TSonorkExtApp*
		LoadExtAppInfo_Exe(TSonorkExtAppLoadSection*);

	static UINT
		LoadExtAppInfo_Flags( char* buffer );

	bool
		ParseExtAppParams(const TSonorkExtApp*EA,SONORK_C_STR params ,const TSonorkExtUserData*UD);

	static SONORK_EXT_APP_PARAM_TYPE
		ParseExtAppParams_Type( SONORK_C_CSTR );

	bool
		ParseExtAppParams_MakeTempFile(SONORK_C_STR,const TSonorkExtApp*,SONORK_C_STR,const TSonorkExtUserData*);

	void
		ExpandExtAppVar(SONORK_C_CSTR s_ptr
			, SONORK_C_STR t_ptr
			, UINT tgt_size
			, const TSonorkExtUserData*UD);

	void
		ClearExtApps();

	void
		ClearWappList();

	static void SONORK_CALLBACK
		LaunchExtApp_RemoteAkn_Waiter(TSonorkWaitWin*WW
			, LPARAM cb_param
			, LPVOID cb_data
			, SONORK_WAIT_WIN_EVENT event
			, LPARAM event_param);

	static void SONORK_CALLBACK
		LaunchExtApp_UserResponse_Waiter(TSonorkWaitWin*WW
			, LPARAM cb_param
			, LPVOID cb_data
			, SONORK_WAIT_WIN_EVENT event
			, LPARAM event_param);

// ----------------------------------------------------------------------------
// Session synchronization

private:
	// ---------------
	//  Asks the engine to synchronize the local sid in UserSidFlags()
	//  with the remote sid kept in the server and by the engine
	//  in User().Address() (also syncs User().Region())

	SONORK_RESULT
		DoSyncSid(TSonorkError& ERR);

public:

	SONORK_RESULT
		SetSidFlags(TSonorkSidFlags&);

	SONORK_RESULT
		SetSidMode(SONORK_SID_MODE);

	SONORK_RESULT
		CancelAutoAwaySidMode();


public:

	TSonorkWin32App();
	~TSonorkWin32App();

	HINSTANCE	Instance()		const	{	return sonork_app_instance;	}
	UINT		RunFlags()	    	const	{	return win32.run_flags;	}
	UINT		CfgFlags()		const	{	return win32.cfg_flags;	}
	WNDPROC		GetWinProc()    	const	{	return WinProc;			}
	HWND		AppWinHandle()		const	{	return win32.work_win;}
	TSonorkMainWin*	MainWin()		const	{ 	return win32.main_win;}
	HWND		MainWinHandle()		const	{	return ((TSonorkWin*)win32.main_win)->Handle();}
	TSonorkSysConsoleWin*
			SysConsoleWin()	const
			{ return (TSonorkSysConsoleWin*)(win32.sys_dialog[SONORK_SYS_DIALOG_SYS_CONSOLE]); }

	TSonorkZip*	ZipEngine()
			{ return win32.zip; }

	HMENU		MainMenu()		const	{ return menus.main; }
	HMENU		UserMtplMenu()		const	{ return menus.user_mtpl;}
	HMENU		UserAppsMenu()		const	{ return menus.user_apps;}
	HMENU		MsgsPopupMenu()		const	{ return menus.msgs_popup;}
	HMENU		MFilPopupMenu() 	const	{ return menus.mfil_popup;}
	HMENU		UserMenu()		const	{ return menus.user_popup;}
	HMENU		UserVisib()		const	{ return menus.user_visib;}
	HMENU		UserAuth()		const	{ return menus.user_auth;}
	HMENU		UserGroupMenu() 	const	{ return menus.ugrp_popup;}
	HMENU		UserSelectionMenu() 	const	{ return menus.usel_popup;}
	HMENU		ClipPopupMenu()		const	{ return menus.clip_popup;}
	HMENU		ChatViewMenu()		const	{ return menus.chat_view;}
	HMENU		ChatUserMenu()		const	{ return menus.chat_user;}
	HMENU		EappPopupMenu()		const	{ return menus.eapp_popup;}
	HMENU		TrayIconMenu()		const	{ return menus.tray_icon; }
	
const TSonorkTime&	CurrentTime()		const	{ return win32.cur_time;}
const TSonorkClock& 	CurrentClock()		const	{ return sonork.TimeSlotClock();}
	SONORK_DWORD2List& AuthExcludeList()		{ return auth_exclude_list; }


	DWORD
		ReferrerId() const
		{ return referrer.id;}

	SONORK_C_CSTR
		ReferrerName() const
		{ return referrer.name.CStr(); }

	SONORK_C_CSTR
		ReferrerUrl() const
		{ return referrer.url.CStr(); }

	BOOL
		TestRunFlag(SONORK_WIN32_APP_RUN_FLAG f)	const
		{	return win32.run_flags&f; 	}

	void
		SetRunFlag(SONORK_WIN32_APP_RUN_FLAG f)
		{	win32.run_flags|=f; 	}

	void
		ClearRunFlag(SONORK_WIN32_APP_RUN_FLAG f)
		{	win32.run_flags&=~f; 	}

	BOOL
		TestCfgFlag(SONORK_WIN32_APP_CFG_FLAG f)	const
		{	return win32.cfg_flags&f; 	}

	void
		SetCfgFlag(SONORK_WIN32_APP_CFG_FLAG f)
		{	win32.cfg_flags|=f; 	}

	void
		ClearCfgFlag(SONORK_WIN32_APP_CFG_FLAG f)
		{	win32.cfg_flags&=~f; 	}

	void	ToggleProfileCtrlFlag(SONORK_PROFILE_CTRL_FLAG);

	void  	Run(const char *cmd_line);

	void	OnSonorkAddGroup( const TSonorkGroup* );
	void	OnSonorkSetGroup( const TSonorkGroup* );
	void	OnSonorkDelGroup( SONORK_GROUP_TYPE, DWORD );


// ----------------------------------------------------------------------------
// Connection state

	// CxStatus() Holds the current connection phase
	//  (The App connection phase starts by reading configuration and
	//    resolving names before connecting the GU engine)
	SONORK_APP_CX_STATUS
		CxStatus() const
		{	return win32.cx_status;}

	bool
		CxConnecting() const
		{
			return win32.cx_status>SONORK_APP_CX_STATUS_IDLE && win32.cx_status<SONORK_APP_CX_STATUS_READY;
		}

	bool
		CxActiveOrPending() const
		{
			return win32.cx_status>SONORK_APP_CX_STATUS_IDLE || TestRunFlag(SONORK_WAPP_RF_CX_PENDING);
		}

	bool	CxActiveOrReady() const
		{
			return win32.cx_status>SONORK_APP_CX_STATUS_IDLE;
		}
	bool	CxReady() 	const	// true if connected and ready
		{
			return win32.cx_status>=SONORK_APP_CX_STATUS_READY;
		}

	// IsPseudoCxStatus()
	//  Returns true if the current status don't modify wich functions
	//  are available to the user: They are merely transition states
	bool
		IsPseudoCxStatus()		const;

	bool
		GlobalTaskPending()		const
		{ return sonork.Busy();	}

	// MayStartGlobalTask(): true if connected and no tasks pending
	bool
		MayStartGlobalTask()	const;

	BOOL
		IntranetMode()			const
		{ return TestCfgFlag(SONORK_WAPP_CF_INTRANET_MODE); }

	SONORK_C_CSTR
		ConfigKeyName()			const
		{ return win32.cfg_key; }

	SONORK_C_CSTR
		ConfigFileName() 		const
		{ return win32.cfg_file; }


	SONORK_C_CSTR

		ConfigName()			const

		{ return win32.cfg_key + 8; }	// NB!! Asumming cfg_key is prefixed with 'CfgFile\\'




// -------------------------------------------------------------------------
// Engine

	// Initiates connection sequence;
	//  on non-fatal error, sets cx_status >= ACTIVE
	SONORK_RESULT
		Connect(TSonorkError&);

	// Disconnects/cancels connect sequence;
	// Sets cx_status to IDLE
	void
		Disconnect();


	SONORK_RESULT
		RefreshUserList(TSonorkError*);

const	TSonorkHostInfo&
		SonorkSocksInfo() const
		{ return sonork.SocksInfo(); }

	SONORK_RESULT
		StartSonorkRequest(TSonorkError&ERR
		,TSonorkDataPacket*P
		,UINT P_size
		,lpfnSonorkRequestHandler h_ptr
		,void*h_param
		,const SONORK_DWORD2*tag
		,DWORD*task_id)
		{
			return sonork.Request(ERR,P,P_size,h_ptr,h_param,tag,task_id);
		}

	bool
		CancelSonorkRequest(DWORD task_id,SONORK_FUNCTION*pFunc,TSonorkTag*pTag)
		{
			return sonork.CancelRequest(task_id,pFunc,pTag);
		}



	// LaunchAppByCmd first attempts to interpret <menu_cmd> as an internal app
	// and if it is not, it tries to launch the external app using LaunchExtApp.
	SONORK_RESULT
		LaunchAppByCmd(UINT menu_cmd,const TSonorkExtUserData*UD,bool*is_a_valid_menu_cmd);// Set UD to NULL for MAIN context

	SONORK_RESULT
		LaunchWebApp(TSonorkWin*,const TSonorkWappData*, const TSonorkExtUserData*, const TSonorkCCacheMark*);

	SONORK_RESULT
		LaunchWebAppByCmd(TSonorkWin*,UINT menu_cmd, const TSonorkExtUserData*, const TSonorkCCacheMark*);


// -------------------------------------------------------------------------
// Name resolution

	HANDLE
		AsyncResolve(TSonorkError&
			,HWND
			,SONORK_PHYS_ADDR_TYPE
			,const char *host
			,DWORD port
			,DWORD tag=0);

	void
		CancelAsyncResolve(HWND);

// -------------------------------------------------------------------------
// App messages and commands

	void
		PostAppCommand(SONORK_APP_COMMAND, LPARAM lParam);

	void
		CancelPostAppCommand(SONORK_APP_COMMAND, LPARAM lParam);
	// BroadcastAppEvent sends the event to all windows
	// that have one of the <mask> in the sys flags.
	// The event is received by the OnAppEvent() method and
	// should return true if the windows processes it and no further
	// processing should take place or it should return false so that
	// the default event handler kicks in.
	// if <mask> is zero, the event is broadcasted to all windows
	void
		BroadcastAppEvent(SONORK_APP_EVENT,UINT mask,UINT param,void*data);

	void
		BroadcastAppEvent_SetUser(TSonorkExtUserData*,UINT set_user_flags);

	void
		OnMainWinActivated();

	void
		OnMainWinDestroying();

// -------------------------------------------------------------------------
// User Interface

	SKIN_ICON
		GetUserModeViewInfo(SONORK_SID_MODE,const TSonorkUserInfoFlags&,SONORK_C_CSTR*) const;

// --------------------
// Menu drawing
	bool
		MenuMeasureItem(  MEASUREITEMSTRUCT* DS);

	bool
		MenuDrawItem( DRAWITEMSTRUCT* DS);
		
// --------------------
// Hint & Info Wins
public:
	TSonorkSlideWin*
		UI_SlideWin()
		{	return win32.slide_win;}
		
	void
		Set_UI_Event(SONORK_UI_EVENT_TYPE , SONORK_C_CSTR, DWORD flags, const void *data=NULL);

	void
		Set_UI_Event(SONORK_UI_EVENT_TYPE , GLS_INDEX , DWORD flags, const void *data=NULL);

	void
		Set_UI_Event_UserConnect(const TSonorkExtUserData*UD,bool connect)
		{
			Set_UI_Event(connect
				?SONORK_UI_EVENT_USER_CONNECT
				:SONORK_UI_EVENT_USER_DISCONNECT
				,csNULL	,0 ,UD);
		}

		// Set pERR to NULL when task starts
		// set pERR to result when task ends
	void
		Set_UI_Event_TaskStart(SONORK_C_CSTR str, DWORD flags);
	void
		Set_UI_Event_TaskStart(GLS_INDEX, DWORD flags);

	// TaskEnd: if str is NULL, default message is used
	void
		Set_UI_Event_TaskEnd(const TSonorkError*, SONORK_C_CSTR str , DWORD flags);

	void
		Clear_UI_Event( SONORK_UI_EVENT_TYPE );

private:
	void
		SetTrayIconTip( SONORK_C_CSTR pszTip ); // set szTip to NULL to leave unchanged

	void	UpdateTrayIcon()
		{ SetTrayIconTip(NULL);}

	void
		SysLog(   SONORK_UI_EVENT_TYPE
			, SKIN_ICON
			, SONORK_C_CSTR
			, BOOL mark_as_unread
			, BOOL force_open );

public:

	void
		UpdateDesktopSize();	// Updates desktop size & origin

const	POINT&
		DesktopOrigin() const
		{	return desktop_size.origin;	}

const	POINT&
		DesktopLimit() const
		{	return desktop_size.limit;	}

const	SIZE&
		DesktopSize() const
		{	return desktop_size.size;	}


	void
		ResetMainView();

	UINT
		GetMainViewSelection(SONORK_DWORD2List*);

	void
		ShowLoginDialog();

	void
		ShowSetupDialog();

	// These GetxxWin may return NULL
	TSonorkWin*
		GetUserWin(const TSonorkId&,SONORK_WIN_TYPE);

	TSonorkWin*
		GetActiveWin(const TSonorkId&,SONORK_WIN_TYPE);

	class TSonorkMsgWin*
		GetUserMsgWin(const TSonorkId&);

	class TSonorkAuthReqWin*
		GetUserAuthReqWin(const TSonorkId&);

	class TSonorkUserDataWin*
		GetUserDataWin(const TSonorkId&user_id);

	void
		DelAuthReqViewItem(const TSonorkId&);
		
	void
		OpenMsgWindow(TSonorkExtUserData*UD, SONORK_MSG_WIN_OPEN_MODE mode);

	void
		OpenUserDataWin(
			  const TSonorkId&
			, const TSonorkUserData*  user	// may be null
			, const TSonorkDynString* notes	// may be null
			, TSonorkWin*owner		// may be NULL
			, UINT tab=0 );

	void
		OpenUserDataWin(
			const TSonorkExtUserData*UD
			, TSonorkWin*owner		// may be NULL
			, UINT tab=0);

	void
		OpenUserDataWin(
			  const TSonorkUserData*	UD
			, const TSonorkDynString*     	notes	// may be null
			, TSonorkWin*owner		// may be NULL
			, UINT 				tab=0);

	void
		OpenUserDataWin(
			  const TSonorkUserDataNotes*UDN
			, TSonorkWin*owner		// may be NULL
			, UINT tab=0);

	void
		OpenUserAuthReqWin(const TSonorkId& user_id);

	void
		ProcessUserProfileData(TSonorkDataPacket*P, UINT P_size);

	//  CmdAddUser:if <task_win> is NULL, the main window is used
	//   as the task win (However, only one main window request may
	//   be active at a given time)
	//  Returns <false> if the request was not be executed
	//  (because <task_win> is already busy or because the user canceled)
	//  Returns <true> if the requested was executed, and ERR
	//  will contains any errors that ocurred while executing.
	// (true does not imply that the requested completed successfuly)
	bool
		CmdAddUser(class TSonorkTaskWin* task_win
				,TSonorkError& ERR
				,const TSonorkId&,DWORD group_no);

	static bool
		AskAddUserText(TSonorkWin*,TSonorkText& text);
		

// -------------------------------------------------------------------------
// Language table

	TSonorkLanguageTable&
		LangTable()
		{	return lang.table;	}

	DWORD
		LangCode()
		{	return lang.table.LangCode();}

	SONORK_C_CSTR
		LangName()
		{	return lang.name.CStr();}

	TSonorkLangCodeTable&
		CountryCodeTable()
		{ return lang.country;}

	TSonorkLangCodeTable&
		LanguageCodeTable()
		{ return lang.language;}

	void
		LangTranslate(TSonorkError&ERR)
		{ lang.table.Translate(ERR); }

	SONORK_C_CSTR
		LangString(GLS_INDEX index)	const
		{	return lang.table.AppStr(index); }

	SONORK_C_CSTR
		SysString(SONORK_SYS_STRING index)	const
		{	return lang.table.SysStr(index);}

	// LangLoad() Only name, no path or extension
	// returns TRUE if changed, returns FALSE if the language
	// is already loaded or an error occurs. Caller should always check
	// ERR.Result() when return value is FALSE.

	BOOL
		LangLoad( TSonorkError&ERR
			, SONORK_C_CSTR file_name
			, BOOL update_user_reg_key);

	void
		EnumLanguagesIntoComboBox(HWND);// handle to a combo box

	BOOL
		LangLoadCodeTable( TSonorkError&ERR, TSonorkLangCodeTable&, SONORK_C_CSTR file_name );

	int
		LangSprintf(char*tgt,GLS_INDEX fmt,...);


public:


// -------------------------------------------------------------------------
// Messsage Sending

	void
		PrepareMsg(TSonorkMsgHandle&
			, TSonorkMsg*           msg
			, DWORD 		sysFlags
			, DWORD 		usrFlags
			, DWORD 		pcFlags
			, DWORD 		replyTrackingNo
			, TSonorkServiceId*	sourceService
			);
	void
		PrepareMsg(TSonorkMsgHandle&	handle
			, TSonorkMsgHeader*	header
			, TSonorkText*		text
			, DWORD 		sysFlags
			, DWORD 		usrFlags
			, DWORD 		pcFlags
			, DWORD 		replyTrackingNo
			, TSonorkServiceId*	sourceService
			);


	SONORK_RESULT
		SendMsgUser(TSonorkMsgHandleEx&
			, TSonorkWin*
			, const TSonorkExtUserData*
			, TSonorkMsg*           msg
			, const TSonorkMsgTarget*target=NULL
			);

	SONORK_RESULT
		SendMsgLocus(TSonorkMsgHandleEx&
			, TSonorkWin*
			, TSonorkUserLocus1*
			, SONORK_UTS_LINK_ID
			, TSonorkMsg*           msg
			, const TSonorkMsgTarget*target=NULL
			);

	// NB!: SendFiles() takes ownership of <queue> and DELETES it
	void
		SendFiles(TSonorkExtUserData*,TSonorkShortStringQueue*queue, BOOL temporal=false);

	void
		SendFile(TSonorkExtUserData*,SONORK_C_CSTR, BOOL temporal=false);

	void
		SendUrl(TSonorkExtUserData*,SONORK_C_CSTR);


	SONORK_RESULT
		ReplyOldCtrlMsg(const TSonorkUserHandle*	handle
				,const TSonorkCtrlMsg*	msg
				,SONORK_OLD_CTRLMSG_CMD	msg_cmd
				,DWORD			msg_flags
				);
	SONORK_RESULT
		SendCtrlMsg(const TSonorkUserHandle*	handle
				,TSonorkCtrlMsg*        msg
				,const void*		data
				,UINT 			data_size
				,SONORK_DWORD2*		req_tag
				);

	SONORK_RESULT
		SendCtrlMsg(const TSonorkUserHandle*	handle
				,TSonorkCtrlMsg*        msg
				,const TSonorkCodecAtom*A
				,SONORK_DWORD2*		req_tag
				);

private:
	void
		OnAppTaskResult(TSonorkAppTask*
			, const TSonorkError*
			, bool remove_from_queue);
	void
		OnAppTaskResult_SendMsg(TSonorkAppTask*,const TSonorkError*);

	void
		OnAppTaskResult_ExportService(TSonorkAppTask_ExportService*
			,const TSonorkError*);

	void
		OnAppTaskResult_SendServiceMsg(TSonorkAppTask_SendServiceMsg*
			,const TSonorkError*);

// -------------------------------------------------------------------------
// Events/Counters

public:

	int	GetCounter(SONORK_APP_COUNTER c) const
		{ return counter[c]; }

	bool
		OpenNextEvent(bool show_main_win_if_none_found, bool force_main_window_visible=false);

	void
		RecalcCounter(SONORK_APP_COUNTER);

	void
		SetCounter(SONORK_APP_COUNTER c,int);

	void
		IncCounter(SONORK_APP_COUNTER c,int v)
		{ SetCounter( c, GetCounter(c) + v); }

	void
		SetUnreadMsgCount(TSonorkExtUserData*,int);

	void
		IncUnreadMsgCount(TSonorkExtUserData*,int);


// -------------------------------------------------------------------------
// Sound

	void
		WavSound( const char* );

	void
		AppSound(SONORK_APP_SOUND);

	void
		MsgSound(const TSonorkExtUserData*,BOOL soft);

	void
		OnlSound(const TSonorkExtUserData*);


// -------------------------------------------------------------------------
// Drag/drop

	void
		ProcessDropQuery(struct TSonorkDropQuery*, UINT flags);

	struct TSonorkClipData*
		ProcessDropExecute(struct TSonorkDropExecute*);

	bool
		ProcessUserDropData(TSonorkExtUserData*,TSonorkClipData*, bool delete_clip_data);

	bool
		LoadSonorkClipData(TSonorkClipData*,const TSonorkMsg*, TSonorkCCacheEntry*CL=NULL);

	void
		DoMsgDrag(TSonorkConsole*,DWORD line_no);

	SONORK_RESULT
		RecvFile( const TSonorkId& 	userId
			, TSonorkCCacheMark* 	mark
			, TSonorkShortString& 	path);

// -------------------------------------------------------------------------
// Misc

	int
		ShellOpenFile(TSonorkWin* , SONORK_C_CSTR , BOOL check_file_exists);

	int
		ExecFile(TSonorkWin* , SONORK_C_CSTR , BOOL expand_environ);

	bool
		OpenCCacheFile(TSonorkWin* pWin , TSonorkCCacheEntry* pCE , struct TSonorkFileInfo* file_info);

	void
		OpenUrl(TSonorkWin*hw,SONORK_C_CSTR);

	void
		OpenMailReader(TSonorkWin*);

	BOOL
		AmIVisibleToUser(const TSonorkExtUserData*UD) const;

	DWORD
		GetSerialNo();

	void
		LoadViewLine(SONORK_C_STR buffer
			 ,UINT	buffer_size
			, SONORK_C_CSTR source
			, BOOL strip_dups
			, bool *line_was_cut=NULL);

// -------------------------------------------------------------------------
// File/Directories 

	static BOOL
		CreateDirIfNotExists(SONORK_C_CSTR);


	static BOOL
		CheckFileAttr(SONORK_C_CSTR, DWORD file_attr_flags, DWORD file_attr_mask);

// -------------------------------------------------------------------------
// Keyboard state

	static BOOL
		IsControlKeyDown();

	static BOOL
		IsSelectKeyDown();


// -------------------------------------------------------------------------
// SID and User View

	// NormalizeSidMode
	//  returns one of the basic "clean" SID_MODEs given any SID_MODE.
static	SONORK_SID_MODE
		NormalizeSidMode(SONORK_SID_MODE);

static SKIN_ICON
		GetUserModeIcon(const TSonorkUserData*UD);

static SKIN_ICON
		GetUserInfoIcon(const TSonorkUserInfoFlags&);

static SKIN_ICON
		GetUserModeViewInfo(SONORK_SID_MODE,const TSonorkUserInfoFlags&,GLS_INDEX*);

static HICON
		GetUserModeHicon(const TSonorkExtUserData*UD,GLS_INDEX*);



// -------------------------------------------------------------------------
// Storage & files

// --------------------
// Directories/Paths

	TSonorkShortString&
		GetRootDir()
		{ return win32.root_dir;	}

	SONORK_RESULT
		GetDirPath(TSonorkShortString&,SONORK_APP_DIR dir,SONORK_C_CSTR subdir);

	SONORK_RESULT
		GetDirPath(SONORK_C_STR	, SONORK_APP_DIR dir,SONORK_C_CSTR subdir);

	SONORK_RESULT
		GetTempPath(SONORK_C_STR , SONORK_C_CSTR prefix, SONORK_C_CSTR sufix=NULL, DWORD un=0);

	SONORK_RESULT

		GetTempPath(TSonorkShortString&,SONORK_C_CSTR prefix, SONORK_C_CSTR sufix=NULL, DWORD un=0);


#define MKTIMESTR_DATE	0x1
#define MKTIMESTR_TIME	0x2
	static bool

		MakeTimeStr( const TSonorkTime& time ,char *buffer, UINT flags);


// --------------------
// Databases

	bool
		OpenAppDb(SONORK_APP_DB , TSonorkAtomDb&, bool write_mode);

	bool
		LoadEmailAccounts(class TSonorkEmailAccountQueue*);


	SONORK_RESULT
		StoreMsg(TSonorkCCache*	C
			,TSonorkMsg*		msg
			,DWORD 			pc_flags
			,DWORD			cc_flags
			,TSonorkExtUserData*UD
			,TSonorkCCacheMark&	REF);
	SONORK_RESULT
		GetMsg(DWORD index, TSonorkMsg*Body);

	SONORK_RESULT
		DelMsg(DWORD index);

	BOOL
		IsMsgLocked(const TSonorkId&, const TSonorkCCacheMark&);

	bool
		MarkMsgProcessed(const TSonorkId&,TSonorkCCacheMark&,DWORD cc_flags,DWORD cc_mask,DWORD*ext_index);

	bool
		SetMsgTag(TSonorkExtUserData*,TSonorkCCache*C,TSonorkCCacheMark*,DWORD flags, DWORD mask, DWORD*ext_index);

	bool
		SetMsgTag(TSonorkExtUserData*,TSonorkCCache*C,DWORD line_no, TSonorkCCacheEntry *, DWORD cc_flags,DWORD cc_mask,DWORD*ext_index);

	SONORK_RESULT
		AddExtData(DWORD*index,const TSonorkCodecAtom*,TSonorkTag*);

	SONORK_RESULT
		AddExtData(DWORD*index,const TSonorkShortString*,TSonorkTag*);

	SONORK_RESULT
		GetExtData(DWORD index,TSonorkCodecAtom*,TSonorkTag*);

	SONORK_RESULT
		GetExtData(DWORD index,TSonorkShortString*,TSonorkTag*);

	SONORK_RESULT
		DelExtData(DWORD index);

// --------------------
// Message Cache

	void
		MakeMsgCachePath(SONORK_C_STR,const TSonorkId&);

	void
		MakeProfileFilePath(SONORK_C_STR, SONORK_C_CSTR name_ext);

	TSonorkCCache*
		GrabSharedMsgCache(const TSonorkId&user_id);

	void
		ReleaseSharedMsgCache(TSonorkCCache*);

	TSonorkCCache*
		CreateMsgCache(
		  DWORD max_text_size	= MSG_CONSOLE_TEXT_SIZE
		, DWORD cache_size	= MSG_CONSOLE_CACHE_SIZE);

// --------------------
// Server profile

	SONORK_RESULT

		LoadServerProfile(SONORK_C_CSTR name,TSonorkClientServerProfile&,bool may_load_default,TSonorkShortString*loaded_profile_name=NULL);


	SONORK_RESULT

		SaveServerProfile(SONORK_C_CSTR name,const TSonorkClientServerProfile&SP)

		{ return _SaveServerProfile(name,SP); }


const	TSonorkClientServerProfile&

		CurrentServerProfile()	const

		{ return win32.server_profile; }



// -------------------------------------------------------------------------
// Uts (User Transfer System)

	TSonorkUTS*
		UTS_Server()
		{	return win32.uts_server;}

	bool	UTS_Enabled() const
		{	return win32.uts_server!=NULL && CxReady(); }

	SONORK_RESULT
		UTS_ConnectByLocus(TSonorkUserLocus1*,TSonorkPhysAddr*,TSonorkError*pERR=NULL);

	SONORK_RESULT
		UTS_ConnectByUserData(TSonorkExtUserData*UD,bool forced,TSonorkError*pERR=NULL);

	void
		UTS_SetLink(const TSonorkId&,SONORK_UTS_LINK_ID,TSonorkExtUserData*UD , BOOL update_ui);

	SONORK_UTS_LINK_ID
		UTS_GetLink(const TSonorkId&);
private:

	// Uts server event handlers
	void
		UTS_Event_SidPin(TSonorkUTSEvent*);

	void
		UTS_Event_Login(TSonorkUTSEvent*);

	void
		UTS_Event_Status(TSonorkUTSEvent*);

	void
		UTS_Event_Msg(TSonorkUTSEvent*,struct TSonorkUTSPacket*P,UINT P_size);

	void
		UTS_Event_CtrlMsg(TSonorkUTSEvent*,struct TSonorkUTSPacket*P,UINT P_size);

	void
		UTS_Event_Warning(TSonorkUTSEvent*);

public:

	BOOL
		UTS_MayActAsServer()	const;

	struct GetLoadSaveDialog
	{
		UINT		id;
		LPOFNHOOKPROC   hook;
		DWORD		data;
	};

	BOOL
		GetSavePath(HWND
			, TSonorkShortString&   path
			, SONORK_C_CSTR 	file_name
			, GLS_INDEX 	 	title
			, SONORK_C_CSTR 	key=NULL
			, SONORK_C_CSTR 	ext=NULL
			, UINT			flags=OFN_EXPLORER
						| OFN_LONGNAMES
						| OFN_NOCHANGEDIR
						| OFN_NOREADONLYRETURN
						| OFN_OVERWRITEPROMPT
						| OFN_PATHMUSTEXIST
			, GetLoadSaveDialog*	GLSD=NULL);

	BOOL
		GetLoadPath(HWND
			, TSonorkShortString&   path
			, SONORK_C_CSTR 	file_name
			, GLS_INDEX 	 	title
			, SONORK_C_CSTR 	key=NULL
			, SONORK_C_CSTR 	ext=NULL
			, UINT			flags=OFN_EXPLORER
						| OFN_LONGNAMES
						| OFN_NOCHANGEDIR
						| OFN_PATHMUSTEXIST
						| OFN_FILEMUSTEXIST
			, GetLoadSaveDialog*	GLSD=NULL);

	BOOL
		GetFileNameForUpload(TSonorkWin*,TSonorkShortString&);

// -------------------------------------------------------------------------
// IPC

private:
	static void
		ClearIpcEntry(DWORD id);

	void
		BroadcastAppEventToIpc(SONORK_APP_EVENT,UINT mask,UINT param,void*data);

// -------------------------------------------------------------------------
// Service cache

public:
	// GetServiceInfoCache:
	//	Caller should delete returned pointer
	TSonorkServiceData*
		GetServiceDataFromCache(
				SONORK_SERVICE_TYPE     service_type
			, 	DWORD			service_instance
			,	DWORD			min_version);

	void
		ReportServiceDataUsageToChache(const TSonorkServiceData*, bool successful);


// -------------------------------------------------------------------------
// Main View

	void
		LoadMainView();
	void
		ClearMainView();

	void
		SetupSearchUserList(const TSonorkListView*);

	void
		ProcessSearchUserTaskData(const TSonorkListView*
				,TSonorkDataPacket*	P
				,UINT			P_size
				,bool 			online_search
				,TSonorkAppSearchUserCallbackPtr ptr=NULL);

#define SONORK_APP_SWITCH_MODE_F_QUERY		0x0001
#define SONORK_APP_SWITCH_MODE_F_NEWWIN		0x0002
	void		SwitchMode(const char*, UINT flags);


// -------------------------------------------------------------------------
// Dialogs

	TSonorkWin*
		RunSysDialog(SONORK_SYS_DIALOG);
	TSonorkWin*
		GetSysDialog(SONORK_SYS_DIALOG type);
	void
		OnSysDialogRun(TSonorkWin*,SONORK_SYS_DIALOG,bool open,SONORK_C_CSTR szTransferKeyName);
	void
		ShowExistingSysDialog(TSonorkWin*);
	void
		ShowNewSysDialog(TSonorkWin*);
	void
		ShowSysConsole();
	void
		ShowUserDbMaintenance();
	void
		FocusNextWindow(TSonorkWin*caller);

	SONORK_RESULT
		TransferWinStartInfo(TSonorkWin*,BOOL load, const char*win_type_name,const TSonorkId*user_id=NULL);


// -------------------------------------------------------------------------
// Bicho

	void
		SetBichoSequence(SONORK_SEQUENCE, bool restart_if_already_set=false);

	BOOL
		SetBichoSequenceIf(SONORK_SEQUENCE if_seq,SONORK_SEQUENCE set_seq);

	BOOL
		SetBichoSequenceIfNot(SONORK_SEQUENCE if_seq,SONORK_SEQUENCE set_seq);

	void
		SetBichoSequenceError(bool sound);

	SONORK_SEQUENCE
		GetBichoSequence()	const;// { return bicho.GetSequence();}

};

extern TSonorkWin32App SonorkApp;




extern  SONORK_C_CSTR szSonorkAppMode;
extern  SONORK_C_CSTR szSonorkDatePickFormat;
extern  SONORK_C_CSTR szSonorkTimePickFormat;
extern  SONORK_C_CSTR szSonorkDateTimePickFormat;
extern	SONORK_C_CSTR szAuthExcludeList;
extern	SONORK_C_CSTR szPrivateDirClip;
extern	SONORK_C_CSTR szPrivateDirUser;



struct TSonorkSkinCodecAtom

:public TSonorkCodecAtom
{
	TSonorkSkin	*skin;

// -----------------------
// CODEC

public:
	TSonorkSkinCodecAtom(TSonorkSkin*pSkin){ skin=pSkin;}

	void	CODEC_Clear();

	SONORK_ATOM_TYPE
		CODEC_DataType()	const
		{ 	return SONORK_ATOM_GENERIC; 	}

private:

	void
		CODEC_WriteDataMem(TSonorkCodecWriter&) const;

	void
		CODEC_ReadDataMem(TSonorkCodecReader&);

	DWORD
		CODEC_DataSize()	const 	;
};



#endif
