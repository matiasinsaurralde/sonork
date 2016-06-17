#if !defined(SRKWIN32APP_DEFS_H)
#define SRKWIN32APP_DEFS_H

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

#include "srkwin_defs.h"

// ----------------------------------------------------------------------------

struct TSonorkExtUserData;
struct TSonorkMsg;
struct TSonorkWappData;
struct TSonorkError;

// ----------------------------------------------------------------------------

#define SONORK_APP_VERSION_MAJOR	SONORK_CLIENT_VERSION_MAJOR
#define SONORK_APP_VERSION_MINOR	SONORK_CLIENT_VERSION_MINOR
#define SONORK_APP_VERSION_PATCH	SONORK_CLIENT_VERSION_PATCH
#define SONORK_APP_VERSION_BUILD	SONORK_CLIENT_VERSION_BUILD
#define SONORK_APP_VERSION_NUMBER	SONORK_CLIENT_VERSION_NUMBER

// -------------------------------------------------------------------------
// App flags & values

#define SONORK_APP_MAX_MSG_TEMPLATES		 128
#define SONORK_APP_MAX_EXT_APPS			 128
#define SONORK_APP_MAX_WEB_APPS			 128
#define SONORK_APP_CONFIG_FILE_NAME_SIZE    	 16
#define SONORK_APP_MAX_UTS_CONNECTIONS		 32
#define SONORK_APP_HTML_ENGINE_VERSION		 1
#define SONORK_APP_MAX_MSG_TEXT_LENGTH		 (12288*2)

// ----------------------------------------------------------------------------

#define SONORK_SECS_FOR_USER_RESPONSE		60
#define SONORK_SECS_FOR_CIRCUIT_CONNECTION	20
#define SONORK_SECS_FOR_CIRCUIT_ACCEPTANCE	5
#define SONORK_SECS_FOR_PROTOCOL_INITIATION	20
#define SONORK_SECS_FOR_QUERY_REPLY    		(SONORK_SECS_FOR_CIRCUIT_CONNECTION)
#define SONORK_SECS_FOR_APP_TO_START		3
#define SONORK_MSECS_FOR_CIRCUIT_CONNECTION	(SONORK_SECS_FOR_CIRCUIT_CONNECTION*1000)
#define SONORK_MSECS_FOR_PROTOCOL_INITIATION	(SONORK_SECS_FOR_PROTOCOL_INITIATION*1000)
#define SONORK_MSECS_FOR_QUERY_REPLY		(SONORK_SECS_FOR_QUERY_REPLY*1000)
#define SONORK_MSECS_FOR_CIRCUIT_ACCEPTANCE	(SONORK_SECS_FOR_CIRCUIT_ACCEPTANCE*1000)

#define SONORK_APP_VISIBILITY_GROUPS		7

#define SONORK_UI_EVENT_TTL_FOREVER		(0x3fffffff)

// ----------------------------------------------------------------------------

#define SONORK_USER_PIC_MAX_BYTES		32767
#define SONORK_USER_PIC_SW			104
#define SONORK_USER_PIC_SH			130

// ----------------------------------------------------------------------------

#define IS_SONORK_APP_SERVICE_ID(n)	((n)==SONORK_SERVICE_ID_NONE || (n)==SONORK_SERVICE_ID_SONORK)
// ----------------------------------------------------------------------------

enum SONORK_WIN32_APP_RUN_FLAG
{
  SONORK_WAPP_RF_ALLOW_INTERNET_MODE		= 0x00000001
//, SONORK_WAPP_RF_ALLOW_INTRANET_MODE		= 0x00000002
, SONORK_WAPP_RF_IN_IPC_HANDLER			= 0x00000100
, SONORK_WAPP_RF_BLINK_ON			= 0x00001000
, SONORK_WAPP_RF_DISABLE_ALARMS			= 0x00002000
, SONORK_WAPP_RF_TRACKER_SYNCH_PENDING		= 0x00004000
, SONORK_WAPP_RF_TRACKER_CACHE_LOADED		= 0x00008000
, SONORK_WAPP_RF_MSG_TEMPLATES_LOADED		= 0x00010000
, SONORK_WAPP_RF_UNINSTALL			= 0x00020000
, SONORK_WAPP_RF_REFRESHING_LIST		= 0x00040000
, SONORK_WAPP_RF_CX_PENDING			= 0x00100000
, SONORK_WAPP_RF_CONNECT_NOW			= 0x00200000
, SONORK_WAPP_RF_SELF_DISCONNECT		= 0x00400000
, SONORK_WAPP_RF_NO_AUTO_LOGIN			= 0x00800000
, SONORK_WAPP_RF_NO_PROFILE_EVENTS		= 0x01000000
, SONORK_WAPP_RF_SETUP_INIT			= 0x02000000
, SONORK_WAPP_RF_SHOW_LOGIN			= 0x04000000
, SONORK_WAPP_RF_APP_INITIALIZED		= 0x10000000
, SONORK_WAPP_RF_APP_TERMINATING		= 0x20000000
, SONORK_WAPP_RF_APP_TERMINATED			= 0x40000000
};

// ----------------------------------------------------------------------------

enum SONORK_WIN32_APP_CFG_FLAG
{
  SONORK_WAPP_CF_NO_CLOSE			= 0x00000001
, SONORK_WAPP_CF_NO_UTS				= 0x00000002
, SONORK_WAPP_CF_NO_INVISIBLE			= 0x00000004
, SONORK_WAPP_CF_NO_VISIBILITY			= 0x00000008
, SONORK_WAPP_CF_NO_USER_SELECT			= 0x00000010
, SONORK_WAPP_CF_NO_NET_CONFIG			= 0x00000020
, SONORK_WAPP_CF_NO_DISCONNECT			= 0x00000100
, SONORK_WAPP_CF_POPUP_MSG_WIN			= 0x00000200
, SONORK_WAPP_CF_NO_FILE_SEND			= 0x00001000
, SONORK_WAPP_CF_NO_FILE_RECV			= 0x00002000
, SONORK_WAPP_CF_INTRANET_MODE			= 0x02000000
};

// ----------------------------------------------------------------------------

enum SONORK_APP_COMMAND
{
  SONORK_APP_COMMAND_TERMINATE
, SONORK_APP_COMMAND_REFRESH
, SONORK_APP_COMMAND_FOREGROUND_HWND
, SONORK_APP_COMMAND_FOCUS_HWND
, SONORK_APP_COMMAND_HALTED				=100	// Internal use only!!
, SONORK_APP_COMMAND_WIN_CREATED        		// Internal use only!!
, SONORK_APP_COMMAND_WIN_DESTROYED    			// Internal use only!!
, SONORK_APP_COMMAND_LOGIN_FATAL_ERROR			// Internal use only!!
, SONORK_APP_COMMAND_SWITCH_MODE
, SONORK_APP_COMMAND_CLEAR_IPC_ENTRY
, SONORK_APP_COMMAND_RELOAD_EXT_APPS
, SONORK_APP_COMMAND_PSEUDO_TASK_RESULT

};

// ----------------------------------------------------------------------------

enum SONORK_APP_START_MODE
{
  SONORK_APP_START_MODE_CANCEL		= -1
, SONORK_APP_START_MODE_ASK		= 0
, SONORK_APP_START_MODE_INTRANET
, SONORK_APP_START_MODE_INTERNET
, SONORK_APP_START_MODES
};

// ----------------------------------------------------------------------------

enum SONORK_APP_CFG_ITEM
{
  SONORK_APP_CFG_NONE
, SONORK_APP_CFG_CLIENT_SERVER_PROFILE
};

// ----------------------------------------------------------------------------

enum SONORK_APP_DIR
{
  SONORK_APP_DIR_ROOT
, SONORK_APP_DIR_DATA
, SONORK_APP_DIR_TEMP
, SONORK_APP_DIR_USERS
, SONORK_APP_DIR_APPS
, SONORK_APP_DIR_BIN
, SONORK_APP_DIR_MSGTPL
, SONORK_APP_DIR_SOUND
, SONORK_APP_DIR_SKIN
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SOUND

{
  SONORK_APP_SOUND_NONE
, SONORK_APP_SOUND_ERROR
, SONORK_APP_SOUND_NOTICE
, SONORK_APP_SOUND_CONNECT
, SONORK_APP_SOUND_REMIND
, SONORK_APP_SOUND_ONLINE
, SONORK_APP_SOUND_MSG_LO
, SONORK_APP_SOUND_MSG_HI
, SONORK_APP_SOUND_TRACKER
};

// ----------------------------------------------------------------------------

enum SONORK_APP_COUNTER
{
  SONORK_APP_COUNTER_EVENTS
, SONORK_APP_COUNTER_UNREAD_MSGS
, SONORK_APP_COUNTER_PENDING_AUTHS
, SONORK_APP_COUNTER_SYS_CONSOLE
, SONORK_APP_COUNTERS
};


// ----------------------------------------------------------------------------
// Application tasks:
//  When the application executes a request on Sonork, and the request
//  handler is the application itself and not a window. The application
//  stores the pending task's information in a <TSonorkAppTask> structure
//  and places this structure in the <win32.task_queue>.
// ----------------------------------------------------------------------------

enum SONORK_APP_TASK_TYPE
{
  SONORK_APP_TASK_NONE
, SONORK_APP_TASK_SEND_MSG
, SONORK_APP_TASK_SEND_SERVICE_MSG
, SONORK_APP_TASK_EXPORT_SERVICE
};

// ----------------------------------------------------------------------------

enum SONORK_APP_TASK_EXPORT_OP
{
  SONORK_APP_TASK_EXPORT_OP_ADD
, SONORK_APP_TASK_EXPORT_OP_SET
, SONORK_APP_TASK_EXPORT_OP_DEL
, SONORK_APP_TASK_EXPORT_OP_KILL// Same as DEL, but entry is no longer valid
};

// ----------------------------------------------------------------------------


enum SONORK_UI_EVENT_TYPE
{
  SONORK_UI_EVENT_NONE
, SONORK_UI_EVENT_SONORK_CONNECT
, SONORK_UI_EVENT_SONORK_DISCONNECT
, SONORK_UI_EVENT_WARNING
, SONORK_UI_EVENT_ERROR
, SONORK_UI_EVENT_USER_CONNECT
, SONORK_UI_EVENT_USER_DISCONNECT
, SONORK_UI_EVENT_TASK_START
, SONORK_UI_EVENT_TASK_END
, SONORK_UI_EVENT_ADD_USER
, SONORK_UI_EVENT_DEL_USER
, SONORK_UI_EVENT_UTS_LINKED
, SONORK_UI_EVENT_UTS_UNLINKED
, SONORK_UI_EVENT_EVENT_COUNT
, SONORK_UI_EVENT_SEL_COUNT
, SONORK_UI_EVENT_DEBUG
, SONORK_UI_EVENT_INCOMMING_EMAIL
, SONORK_UI_EVENT_SYS_MSG
, SONORK_UI_EVENT_USER_MSG
};

// ----------------------------------------------------------------------------

enum SONORK_UI_EVENT_FLAGS
{
  SONORK_UI_EVENT_F_BICHO		= 0x00000001
, SONORK_UI_EVENT_F_NO_BICHO      	= 0x00000000	// For clarity
, SONORK_UI_EVENT_F_SOUND		= 0x00000002
, SONORK_UI_EVENT_F_NO_SOUND		= 0x00000000    // For clarity
, SONORK_UI_EVENT_F_LOG			= 0x00000004
, SONORK_UI_EVENT_F_NO_LOG		= 0x00000000	// For clarity
, SONORK_UI_EVENT_F_LOG_AS_UNREAD	= 0x00000010    // only if F_LOG is set
, SONORK_UI_EVENT_F_LOG_AUTO_OPEN	= 0x00000020	// only if F_LOG is set
, SONORK_UI_EVENT_F_CLEAR_IF_NO_ERROR	= 0x00000100	// for TASK_END event
, SONORK_UI_EVENT_F_WARNING		= 0x00000200
, SONORK_UI_EVENT_INTERNAL_MASK		= 0xfff00000	// Used internally by Set_UI_Event
, SONORK_UI_EVENT_F_SET_INFO		= 0x00100000
, SONORK_UI_EVENT_F_AUTO_DISMISS	= 0x00200000
, SONORK_UI_EVENT_F_SET_SLIDER		= 0x00400000
};

// ----------------------------------------------------------------------------

enum SONORK_APP_DB
{
  SONORK_APP_DB_REMIND
, SONORK_APP_DB_EMAIL_ACCOUNT
, SONORK_APP_DB_EMAIL_EXCEPT
, SONORK_APP_DB_TRACKER_SUBS
, SONORK_APP_DB_TRACKER_CACHE
};

// ----------------------------------------------------------------------------
// Special menu commands used globaly by all menus

enum SONORK_APP_MENU_COMMAND
{
  SONORK_APP_CM_EAPP_BASE	= 5000
, SONORK_APP_CM_EAPP_LIMIT	= (SONORK_APP_CM_EAPP_BASE+SONORK_APP_MAX_EXT_APPS)
, SONORK_APP_CM_WAPP_BASE
, SONORK_APP_CM_WAPP_LIMIT	= (SONORK_APP_CM_WAPP_BASE+SONORK_APP_MAX_WEB_APPS)
, SONORK_APP_CM_MTPL_BASE
, SONORK_APP_CM_MTPL_LIMIT	= (SONORK_APP_CM_MTPL_BASE+SONORK_APP_MAX_WEB_APPS)
, SONORK_APP_CM_LIMIT
};

// ----------------------------------------------------------------------------
// Sonork application connection status

enum SONORK_APP_CX_STATUS
{
  SONORK_APP_CX_STATUS_IDLE		= 0
, SONORK_APP_CX_STATUS_NAME_RESOLVE
, SONORK_APP_CX_STATUS_LINKING
, SONORK_APP_CX_STATUS_LINKED
, SONORK_APP_CX_STATUS_REFRESHING_LIST
, SONORK_APP_CX_STATUS_DOWNLOADING_WAPPS
, SONORK_APP_CX_STATUS_REGISTERING_SID
, SONORK_APP_CX_STATUS_CONNECTING_FIRST	=SONORK_APP_CX_STATUS_NAME_RESOLVE
, SONORK_APP_CX_STATUS_CONNECTING_LAST	=SONORK_APP_CX_STATUS_REGISTERING_SID
, SONORK_APP_CX_STATUS_READY		 // All post-connect initilization finished
, SONORK_APP_CX_STATUS_READY_AND_GO	// Post connect time elapsed (*)
};
// SONORK_APP_CX_STATUS_READY_AND_GO: We leave a small time window after connecting
//  before starting to make sounds, etc..
//  This is to avoid saturating the user with the notification sounds of the
//   already connected users and server-stored messages.

// ----------------------------------------------------------------------------
// SERVICES
// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_ENTRY_FLAGS
{
  SONORK_APP_SERVICE_EF_REGISTERED	= 0x00000001
, SONORK_APP_SERVICE_EF_EXPORTED	= 0x00000010
, SONORK_APP_SERVICE_EF_LOCKED		= 0x00000020
, SONORK_APP_SERVICE_EF_IPC		= 0x00000040
, SONORK_APP_SERVICE_EF_HIDDEN		= 0x10000000
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_QUERY_FLAGS
{
  SONORK_APP_SERVICE_QF_ACTIVE		= 0x00000001
, SONORK_APP_SERVICE_QF_CLEARED		= 0x00000100
, SONORK_APP_SERVICE_QF_REMOVED		= 0x00000200
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_CIRCUIT_FLAGS
{
  SONORK_APP_SERVICE_CF_ACTIVE		= 0x00000001
, SONORK_APP_SERVICE_CF_PENDING		= 0x00000002
, SONORK_APP_SERVICE_CF_ACCEPTED	= 0x00000004
, SONORK_APP_SERVICE_CF_REMOTE_DEAD	= 0x00000008
, SONORK_APP_SERVICE_CF_OPEN_BY_LOCAL	= 0x00000010
, SONORK_APP_SERVICE_CF_OPEN_BY_REMOTE	= 0x00000020
, SONORK_APP_SERVICE_CF_CLEARED		= 0x00000100
, SONORK_APP_SERVICE_CF_REMOVED		= 0x00000200
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_TYPE
{
  SONORK_APP_SERVICE_TYPE_NONE
, SONORK_APP_SERVICE_TYPE_CLIENT
, SONORK_APP_SERVICE_TYPE_LOCATOR
, SONORK_APP_SERVICE_TYPE_SERVER
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_EVENT
{
  SONORK_APP_SERVICE_EVENT_NONE
, SONORK_APP_SERVICE_EVENT_INITIALIZE
, SONORK_APP_SERVICE_EVENT_EXPORT
, SONORK_APP_SERVICE_EVENT_BUSY
, SONORK_APP_SERVICE_EVENT_GET_NAME		= 100
, SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA
, SONORK_APP_SERVICE_EVENT_POKE_DATA            = 200
, SONORK_APP_SERVICE_EVENT_QUERY_REQ		= 300
, SONORK_APP_SERVICE_EVENT_QUERY_AKN
, SONORK_APP_SERVICE_EVENT_QUERY_END
, SONORK_APP_SERVICE_EVENT_CIRCUIT_REQ		= 400
, SONORK_APP_SERVICE_EVENT_CIRCUIT_OPEN
, SONORK_APP_SERVICE_EVENT_CIRCUIT_DATA
, SONORK_APP_SERVICE_EVENT_CIRCUIT_UPDATE
, SONORK_APP_SERVICE_EVENT_CIRCUIT_CLOSED
};

// ----------------------------------------------------------------------------

enum SONORK_APP_SERVICE_CIRCUIT_REQ_RESULT
{
  SONORK_APP_SERVICE_CIRCUIT_NOT_ACCEPTED
, SONORK_APP_SERVICE_CIRCUIT_ACCEPT_PENDING
, SONORK_APP_SERVICE_CIRCUIT_ACCEPTED
};

// ----------------------------------------------------------------------------

// Sonork APP-specific control messages
#define SONORK_APP_CTRLMSG_QUERY_PIC		SONORK_CTRLMSG_CMD_01
#define SONORK_APP_CTRLMSG_QUERY_SERVICES	SONORK_CTRLMSG_CMD_02

// ----------------------------------------------------------------------------
// External applications

#define SONORK_APP_EA_CONTEXT_MASK		0x00000007
#define SONORK_APP_EAF_NON_ZERO_IP		0x00000100
#define SONORK_APP_EAF_CONNECTED		0x00000200
#define SONORK_APP_EAF_SEND_REQ			0x01000000
#define SONORK_APP_EAF_WAIT_AKN			0x02000000

enum SONORK_APP_TYPE
{
  SONORK_APP_TYPE_INTERNAL
, SONORK_APP_TYPE_EXTERNAL
, SONORK_APP_TYPE_WEB
};
enum SONORK_EXT_APP_CONTEXT
{
  SONORK_EXT_APP_MAIN_CONTEXT
, SONORK_EXT_APP_USER_CONTEXT
, SONORK_EXT_APP_CTRL_CONTEXT
, SONORK_EXT_APP_CONTEXTS
};
enum SONORK_EXT_APP_TYPE
{
  SONORK_EXT_APP_TYPE_REG
, SONORK_EXT_APP_TYPE_EXE
};

// SONORK_EXT_APP_PARAM_TYPE
// Defines the type of parameters for an application listed in the app.ini file.

enum SONORK_EXT_APP_PARAM_TYPE
{
  SONORK_EXT_APP_PARAM_INVALID
, SONORK_EXT_APP_PARAM_RELATIVE
, SONORK_EXT_APP_PARAM_ABSOLUTE
, SONORK_EXT_APP_PARAM_TEMPORAL
};

// -------------------------------------------------------------------------
// Event mask Determine what type of events are dispatched to a window/IPC.

enum SONORK_APP_EVENT_MASK
{
  SONORK_APP_EM_CX_STATUS_AWARE		=0x00000001
, SONORK_APP_EM_PROFILE_AWARE		=0x00000002
, SONORK_APP_EM_USER_LIST_AWARE		=0x00000004
, SONORK_APP_EM_MSG_CONSUMER		=0x00000010
, SONORK_APP_EM_MSG_PROCESSOR		=0x00000020
, SONORK_APP_EM_CTRL_MSG_CONSUMER	=0x00000040
, SONORK_APP_EM_SYS_WINDOWS_AWARE	=0x00000100
, SONORK_APP_EM_MAIN_VIEW_AWARE		=0x00000200
, SONORK_APP_EM_SKIN_AWARE		=0x00000400
, SONORK_APP_EM_WAPP_PROCESSOR		=0x00001000
#if defined(SONORK_APP_BUILD)
, SONORK_APP_EM_ENUMERABLE		=0x10000000
#endif
};
// SONORK_APP_EM_ENUMERABLE:
//  Window is placed in the application's main window queue,
//  but no events are sent to it if not other flags are set.

#define SONORK_WIN_POKE_NET_RESOLVE_RESULT	SONORK_WIN_POKE_APP_01
#define SONORK_WIN_POKE_DESTROY			SONORK_WIN_POKE_APP_02
#define SONORK_WIN_POKE_SET_TAB			SONORK_WIN_POKE_APP_03
#define SONORK_WIN_POKE_SONORK_TASK_RESULT	SONORK_WIN_POKE_APP_04
#define SONORK_WIN_POKE_CHILD_NOTIFY		SONORK_WIN_POKE_APP_05
#define SONORK_WIN_POKE_CHILD_DRAW_ITEM		SONORK_WIN_POKE_APP_06
#define SONORK_WIN_POKE_ULTRA_MIN_DESTROY	SONORK_WIN_POKE_APP_08
#define SONORK_WIN_POKE_ULTRA_MIN_PAINT		SONORK_WIN_POKE_APP_09

// SONORK_EXT_DB_FLAGS
//  Used for the SONORK_DB_TAG_FLAGS tag of the EXT database
//  Note that some flags are data-dependant: And they mean
//  different things for different types of data

// SONORK_EXT_DB_FILE_PATH_xxxxx
//  These flags are used when storing the original  location
//   of a file sent/received.

//  SONORK_EXT_DB_FILE_PATH_FULL
//   If this flag is set, the data is the full path to the file. If it is
//   not set, the data is the path to the folder, without the file name.
//   Versions 1.04.05 and before did not use this flag
//   Versions 1.04.06 and after always set this flag
#define SONORK_EXT_DB_FILE_PATH_FULL		0x00010000



// SONORK_APP_ATOM_DB_VALUES
//  These values are mapped into SONORK_ATOM_DB_VALUE
//  and are used for the application's MSG and EXT databases

#define SONORK_APP_ATOM_DB_VALUE_VERSION	SONORK_ATOM_DB_VALUE_1

// This is the value for SONORK_APP_ATOM_DB_VALUE_VERSION
// that will be used when creating databases
#define SONORK_APP_CURRENT_ATOM_DB_VERSION	1

// -------------------------------------------------------------------------
// User message console line flags

#include "srk_ccache.h"

enum SONORK_APP_CCACHE_FLAGS
{
  SONORK_APP_CCF_INCOMMING    	=SONORK_CCLF_INCOMMING 		//0x00000001
, SONORK_APP_CCF_UNREAD	    	=SONORK_CCLF_01			//0x00000002
, SONORK_APP_CCF_DENIED	    	=SONORK_CCLF_02			//0x00000004
, SONORK_APP_CCF_DELETED	=SONORK_CCLF_02			//0x00000004
, SONORK_APP_CCF_PROCESSED    	=SONORK_CCLF_03			//0x00000008
, SONORK_APP_CCF_ACCEPTED	=SONORK_CCLF_03			//0x00000008
, SONORK_APP_CCF_READ_ON_SELECT	=SONORK_CCLF_08			//0x00001000
, SONORK_APP_CCF_NO_READ_ON_OPEN=SONORK_CCLF_09			//0x00002000
, SONORK_APP_CCF_PROTECTED    	=SONORK_CCLF_10			//0x00004000
, SONORK_APP_CCF_UNSENT	    	=SONORK_CCLF_11			//0x00008000
, SONORK_APP_CCF_THREAD_START 	=SONORK_CCLF_THREAD_START	//0x00010000
, SONORK_APP_CCF_IN_THREAD 	=SONORK_CCLF_IN_THREAD		//0x00020000
, SONORK_APP_CCF_EXT_DATA	=SONORK_CCLF_EXT_DATA		//0x00040000
, SONORK_APP_CCF_QUERY	    	=SONORK_CCLF_12			//0x00100000
, SONORK_APP_CCF_ERROR	    	=SONORK_CCLF_13			//0x00200000
};


enum SONORK_APP_MSG_PROCESS_FLAGS
{

// SONORK_APP_MPF_PROCESSED
// The message has been processed/consumed
  SONORK_APP_MPF_PROCESSED		=0x00000001

// SONORK_APP_MPF_SUCCESS
// The message has been processed successfully (only if MPF_CONSUMED is also set)
, SONORK_APP_MPF_SUCCESS		=0x00000002

// SONORK_APP_MPF_DONT_SEND
// Message was already sent, don't re-send, just store (if aplicable)
, SONORK_APP_MPF_DONT_SEND		=0x00000004

// SONORK_APP_MPF_DONT_STORE
// Message should not be stored in local DB
, SONORK_APP_MPF_DONT_STORE		=0x00000010

// SONORK_APP_MPF_STORED
// The message has been stored and the <mark> member of the event is valid
,  SONORK_APP_MPF_STORED		=0x00000020

// SONORK_APP_MPF_SILENT
// Message should be handled silently, don't make sounds or advise user
, SONORK_APP_MPF_SILENT			=0x00000100

// SONORK_APP_MPF_SOFT_SOUND
// If incoming: Use soft sound instead of default user sound
, SONORK_APP_MPF_SOFT_SOUND		=0x00000200

// SONORK_APP_MPF_AUTO_READ_SHORT
// If Incomming: Message may be marked as read if it's a short message
// "short message" means the whole text fits on screen and there is no
// need to open the message in the history window to see the whole of it.
, SONORK_APP_MPF_AUTO_READ_SHORT	=0x00000400

// SONORK_APP_MPF_NO_STRIPPING
// Do no strip duplicates when storing the preview text on the message console
// (used for filenames, urls, etc.)

, SONORK_APP_MPF_NO_STRIPPING		=0x00000800
// SONORK_APP_MPF_NO_TIME_SET
// If incomming: Don't adjust GMT time to local time
// If outgoign: Don't load current (local) time
, SONORK_APP_MPF_NO_TIME_SET		=0x00001000
// SONORK_APP_MPF_UTS
// If incomming: Message arrived via UTS
// If outgoing: Message MUST go via UTS or fail
, SONORK_APP_MPF_UTS			=0x00002000
// SONORK_APP_MPF_UTS
// If outgoing: Message MUST NOT go via UTS
, SONORK_APP_MPF_NO_UTS			=0x00010000

// SONORK_APP_MPF_ADD_USER
// If user is not in the user list, add it to the contacts list
, SONORK_APP_MPF_ADD_USER		=0x00020000

// SONORK_APP_MPF_NO_SERVICE_PARSE
// Will not attempt to derive the data service type from the text
// (Transform message into URL, Prepare Msg only))
, SONORK_APP_MPF_NO_SERVICE_PARSE	=0x00040000

// SONORK_APP_MPF_CLEAR_SERVICE
// Will set the data service to NONE
, SONORK_APP_MPF_CLEAR_SERVICE		=0x00080000

// SONORK_APP_MPF_NO_TRACKING_NUMBER
// Will not load the tracking number for the message
, SONORK_APP_MPF_NO_TRACKING_NUMBER	=0x00100000

};

// -------------------------------------------------------------------------
// Profile & User flags/values

// Profile Configuration flags
#define SONORK_PCF_NO_HIDE_ON_MINIMIZE			SONORK_PCF_1
#define SONORK_PCF_POPUP_MSG_WIN      			SONORK_PCF_2
#define SONORK_PCF_SEND_WITH_ENTER    			SONORK_PCF_3
#define SONORK_PCF_MUTE_SOUND	      			SONORK_PCF_4
#define SONORK_PCF_NO_TOOL_TIPS	      			SONORK_PCF_5
#define SONORK_PCF_NO_MSG_REMINDER    			SONORK_PCF_6
#define SONORK_PCF_IGNORE_NON_AUTHED_MSGS		SONORK_PCF_7
#define SONORK_PCF_IGNORE_EMAILS      			SONORK_PCF_8
#define SONORK_PCF_CONFIRM_FILE_SEND			SONORK_PCF_9
#define SONORK_PCF_NO_TRAY_BLINK      			SONORK_PCF_10
#define SONORK_PCF_GRPMSG_SAVUSR      			SONORK_PCF_11
#define SONORK_PCF_GRPMSG_SAVSYS      			SONORK_PCF_12
#define SONORK_PCF_NOT_PUBLIC_SID			SONORK_PCF_13
#define SONORK_PCF_PUBLIC_SID_WHEN_FRIENDLY		SONORK_PCF_14
#define SONORK_PCF_CHAT_SOUND				SONORK_PCF_15
#define SONORK_PCF_NO_AUTO_SHOW_SID_MSG			SONORK_PCF_16
#define SONORK_PCF_NO_COMPRESS_FILES			SONORK_PCF_17
#define SONORK_PCF_NO_INVITATIONS			SONORK_PCF_18
#define SONORK_PCF_NO_PUBLIC_INVITATIONS		SONORK_PCF_19

// Profile Configuration values
#define SONORK_PCV_AUTO_AWAY_MINS		SONORK_PCV_1
#define SONORK_PCV_DOWNLOAD_FILE_ACTION		SONORK_PCV_2
#define	SONORK_PCV_CHAT_COLOR			SONORK_PCV_3
#define SONORK_PCV_INPUT_HEIGHT			SONORK_PCV_4
#define SONORK_PCV_SLIDER_POS			SONORK_PCV_5

// Profile Run flags
#define SONORK_PRF_CLIPBOARD_INITIALIZED	SONORK_PRF_1

// App Run values
#define SONORK_ARV_FIRST_UNREAD_SYS_MSG		SONORK_ARV_1

// App Ctrl Flags
#define SONORK_ACF_USE_PC_SPEAKER		SONORK_ACF_1


// TSonorkExtUserData::SONORK_USER_CTRL_FLAGs
#define SONORK_APP_UCF_NO_SLIDER_CX_DISPLAY		SONORK_UCF_2
#define SONORK_APP_UCF_NO_STATUS_CX_DISPLAY		SONORK_UCF_3




// -----------------------------------------------------------------------
//  APPLICATION EVENTS

// SONORK_APP_EVENT

//  These events are broadcasted using TSonorkWin32App::BroadcastAppEvent
//   and trigger the OnAppEvent() handler of every TSonorkWin that is in the
//   app event queue. (Only windows with certain SONORK_APP_WIN_DESCRIPTOR flags
//    are placed in the queue; see the SONORK_APP_WIN_DESCRIPTOR enum)

//  The OnAppEvent receives three parameters:
//		UINT event	: The event id (SONORK_APP_EVENT)
//		UINT param	: Event specific parameter
//		void*data	: Event specific data

//  Description of each event:

//  SONORK_APP_EVENT_SHUTDOWN
//    Last advise of the application shutting down, all windows should close NOW.
//    No parameters.
//    Event is non-maskable: It is received by all windows in the events list.

//  SONORK_APP_EVENT_CX_STATUS

//    The application has changed its CxStatus, SONORK_APP_CX_STATUS <param> holds
//    the previous status. Windows that cannot work while disconnected should
//    respond to this event and close when apropriate.
//    This event is received by windows with the SONORK_APP_EM_CX_STATUS_AWARE flag

// SONORK_APP_EVENT_SET_LANGUAGE
//    The language table has changed.
//    This event is non-maskable: It is received by all windows in the events list.

// SONORK_APP_EVENT_OPEN_PROFILE
//    An user profile has been open/closed.
//     if BOOL:<param> is FALSE, the profile has been closed, otherwise
//     it has been open.
//    Most windows cannot work until the user profile is open
//    because the profile determines which user is using the application.
//    This event is received by windows with the SONORK_APP_EM_PROFILE_AWARE flag.
//    Windows that need the profile open should close uppon receiving the
//     event with <param>=FALSE.

//  SONORK_APP_EVENT_SET_PROFILE
//   The user profile has been modified
//   <data> is a pointer to the TSonorkUserDataNotes structure with the
//   new profile information that has been passed to BroadcastAppEvent()
//   as <data> parameter. <param> is undefined.
//    This event is received by windows with the SONORK_APP_EM_PROFILE_AWARE flag.

//  SONORK_APP_EVENT_SID_CHANGED
//   Online mode has changed.
//   SONORK_SID_MODE:<param> is the new SID mode. <data> is not undefined.
//    This event is received by windows with the SONORK_APP_EM_PROFILE_AWARE flag.

//  SONORK_APP_EVENT_USER_SID
//   Online mode of a user has changed.
//   The SONORK_APP_EVENT_USER_SID_TYPE:<param> should be inspected to
//   see how to handle this message.
//   if <param> is SONORK_APP_EVENT_USER_SID_TYPE_LOCAL, <data>
//    should be interpreted as (TSonorkAppEventLocalUserSid*)
//   if <param> is SONORK_APP_EVENT_USER_SID_TYPE_REMOTE, <data>
//    should be interpreted as (TSonorkAppEventRemoteUserSid*)
//   if <param> is neither of above, the event must be ignored

//  SONORK_APP_EVENT_SET_USER
//    One of the users in the user list has changed its attributes
//    UINT <param> holds the flags of the attributes changed.
//			  See TSonorkAppEventSetUser::SET_FLAGS for the list of flags.
//    TSonorkAppEventSetUser* <data>  is a pointer to the data of the user.
//    Note that the TSonorkAppEventSetUser::<user_data> member may be
//    null if TSonorkAppEventSetUser::<user_id> was not found in the user list.
//    This event is received by windows with the SONORK_APP_EM_USER_LIST_AWARE flag

//  SONORK_APP_EVENT_DEL_USER
//    The application is about to delete an user.
//    TSonorkId* <data> is a pointer to the TSonorkId of the user.
//    <param> is undefined.
//    Windows that are currently working with this user's data should
//    immediately close, the user WILL be deleted as soon as the broadcast
//    of the event ends.
//    This event is received by windows with the SONORK_APP_EM_USER_LIST_AWARE flag

//  SONORK_APP_EVENT_SONORK_MSG_RCVD
//   A message has been received.
//   TSonorkAppEventMsg* <data> holds the event data. <param> is undefined.
//   The <ERR> field of <data> is NULL for this event
//   This event is received by windows with the SONORK_APP_EM_MSG_CONSUMER flag.
//   See TSonorkAppEventMsg for details on the event data.

//  SONORK_APP_EVENT_SONORK_MSG_SENDING
//   A message is being sent: It should be stored in the database.
//   TSonorkAppEventMsg* <data> holds the event data. <param> is undefined.
//   The <ERR> field of <data> is NULL for this event
//   This event is received by windows with the SONORK_APP_EM_MSG_CONSUMER flag.
//   See TSonorkAppEventMsg for details on the event data.

//  SONORK_APP_EVENT_SONORK_MSG_SENT
//   A message has been sent or failed to send: The status of the message
//    must be updated in the database.
//   TSonorkAppEventMsg* <data> holds the event data. <param> is undefined.
//   The <ERR> field of <data> is valid for this event:
//    if ERR->Result() is OK, the message was sent,
//    otherwise message delivery failed
//   This event is received by windows with either the SONORK_APP_EM_MSG_CONSUMER
//    or the SONORK_APP_EM_MSG_PROCESSOR flags.
//   See TSonorkAppEventMsg for details on the event data.

//  SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK
//   This event is triggered before starting a procedure that processes a
//    message (and that will probably generate a MSG_PROCESSED event)
//    to check if there exists another procedure/dialog already processing
//    the message
//   TSonorkAppEventMsgQueryLock* <data> holds the event data. <param> is undefined.
//   This event is received by windows with the SONORK_APP_EM_MSG_PROCESSOR flag.
//   See TSonorkAppEventMsgQueryLock for details on the event data.

// SONORK_APP_EVENT_SONORK_MSG_PROCESSED
//   This event is triggered after a console message line has been processed/changed
//   and the corresponding console file NEEDS to be updated.
//   TSonorkAppEventMsgProcessed* <data> holds the event data. <param> is undefined.
//   This event is received by windows with the SONORK_APP_EM_MSG_CONSUMER flag.
//   See TSonorkAppEventMsgProcessed for details on the event data.

// SONORK_APP_EVENT_USER_UTS
//   Event triggered when UTS link of an user changes.
//    TSonorkId* <data> is a pointer to the TSonorkId of the user.
//    <param> is the current UTS link which is zero if link has been closed.
//   This event is received by windows with the SONORK_APP_EM_USER_LIST_AWARE flag.

// SONORK_APP_EVENT_START_WAPP
//   Triggered when a Web Application (WAPP) needs to be started,
//   TSonorkAppEventStartWapp* <data> holds the event data and <wParam>
//    is nt used.
//    The WAPP window serving that application should restore itself
//    if hidden, minimized, and become the foreground window.
//  Received by windows with the SONORK_APP_EM_WAPP_PROCESSOR flag
//  See TSonorkAppEventStartWapp for details on the event data.

// SONORK_APP_EVENT_USER_MTPL_CHANGE
//   Triggered when a the message templates menu has been modified.
//   No parameters, the consumers can access GuApp::UserMtplMenu()
//   This event is non-maskable: It is received by all windows in the events list.

// SONORK_APP_EVENT_DESKTOP_SIZE_CHANGE
//   Triggered when app window receives WM_SETTINGCHANGE with SPI_GETWORKAREA
//   flag set.
//   This event is non-maskable: It is received by all windows in the events list.

// SONORK_APP_EVENT_MAIN_VIEW_SELECTION
//   User selection (highlighting users) in the main view has changed.
//    <param> is the new number of users selected. <data> is undefined.
//   This event is received by windows with the SONORK_APP_EM_MAIN_VIEW_AWARE flag.

//  SONORK_APP_EVENT_MAINTENANCE
//    All windows should close: System is going into "Maintenance" mode
//    <data> is the pointer to the TSonorkWin that requested Maintenance mode.
//   This event is non-maskable: It is received by all windows in the events list.

// SONORK_APP_EVENT_FLUSH_BUFFERS
//    The window should flush its buffers. This event is generated every X
//    minutes so that, if something fails, nothing is left unwritten in memory.
//   This event is non-maskable: It is received by all windows in the events list.

// ----------------------------------------------------------------------------

enum SONORK_APP_EVENT
{
  SONORK_APP_EVENT_NONE			= 0
, SONORK_APP_EVENT_SHUTDOWN		= 1
, SONORK_APP_EVENT_FLUSH_BUFFERS	= 5
, SONORK_APP_EVENT_MAINTENANCE		= 6
, SONORK_APP_EVENT_OLD_CTRL_MSG		= 7  // For V1.04 backwards support
, SONORK_APP_EVENT_OPEN_PROFILE		= 10
, SONORK_APP_EVENT_CX_STATUS		= 12
, SONORK_APP_EVENT_SET_PROFILE   	= 14 // User profile attributes have been modified
, SONORK_APP_EVENT_SID_CHANGED		= 16 // User online mode has changed
, SONORK_APP_EVENT_SET_LANGUAGE		= 18
, SONORK_APP_EVENT_SKIN_COLOR_CHANGED	= 20
, SONORK_APP_EVENT_SYS_DIALOG		= 22 // lower word of lParam contains dialog ID, high word contains true or false
, SONORK_APP_EVENT_USER_SID		= 50 // User changed its sid attributes
, SONORK_APP_EVENT_SET_USER		= 52 // User in local user list changed its attributes
, SONORK_APP_EVENT_DEL_USER		= 54
, SONORK_APP_EVENT_USER_UTS		= 53
, SONORK_APP_EVENT_SONORK_MSG_RCVD	= 60 // message has been received
, SONORK_APP_EVENT_SONORK_MSG_SENDING	= 62 // message has entered queue for sending
, SONORK_APP_EVENT_SONORK_MSG_SENT	= 64 // message has been sent or failed to send
, SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK= 66
, SONORK_APP_EVENT_SONORK_PROCESS_MSG	= 68
, SONORK_APP_EVENT_SONORK_MSG_PROCESSED = 70
, SONORK_APP_EVENT_START_WAPP		= 80
, SONORK_APP_EVENT_USER_MTPL_CHANGE	= 82
, SONORK_APP_EVENT_MAIN_VIEW_SELECTION	= 100
, SONORK_APP_EVENT_DESKTOP_SIZE_CHANGE	= 101
};

// ----------------------------------------------------------------------------

enum SONORK_APP_EVENT_USER_SID_TYPE
{
  SONORK_APP_EVENT_USER_SID_LOCAL 	=1
, SONORK_APP_EVENT_USER_SID_REMOTE      =2
};

// ----------------------------------------------------------------------------

enum SONORK_APP_EVENT_SET_USER_FLAGS
{
  SONORK_APP_EVENT_SET_USER_F_AUTH_FLAGS	=0x000001
, SONORK_APP_EVENT_SET_USER_F_DATA		=0x000004
, SONORK_APP_EVENT_SET_USER_F_R_NOTES		=0x000008
, SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS	=0x000010
, SONORK_APP_EVENT_SET_USER_F_L_NOTES		=0x000020
, SONORK_APP_EVENT_SET_USER_F_MSG_COUNT		=0x000100
, SONORK_APP_EVENT_SET_USER_F_EXT_DATA		=0x000400
, SONORK_APP_EVENT_SET_USER_F_REFRESH		=0x000800
, SONORK_APP_EVENT_SET_USER_F_ALL		=0x00ffff
, SONORK_APP_EVENT_SET_USER_F_ANY		=0x00ffff
};
// ----------------------------------------------------------------------------

struct TSonorkMsgHandle
{
friend class	TSonorkWin32App;
private:
	DWORD			pcFlags;
	TSonorkServiceId	sourceService;

public:
	DWORD			taskId;
	DWORD			handleId;
	TSonorkCCacheMark	mark;

const	DWORD
		ProcessFlags() const
		{ return pcFlags; }

}__SONORK_PACKED;


// ----------------------------------------------------------------------------
#if defined(SONORK_APP_BUILD)

// These structures should not be used by the IPC.
// The IPC server maps them to IPC-defined structures
// in order to be able to freely change these types without
// affecting compatiblity with existing IPC applications.

// ----------------------------------------------------------------------------

struct TSonorkAppEventMsg
{
//  SONORK_APP_EVENT_SONORK_MSG_RECV
//  SONORK_APP_EVENT_SONORK_MSG_SENDING
//  SONORK_APP_EVENT_SONORK_MSG_SENT
//   A message has been received/sent or is being sent.
//   TSonorkAppEventEventMsg* <data> holds the event data. <param> is undefined.
//   A window should only process the event if it "knows" how to process it
//    and ONLY if <consumed> is FALSE. After processing it, <consumed> should
//    be set to TRUE.
//   <pc_flags> are the proces flags
//   <cc_flags> are stored in the message console.
	DWORD			pcFlags;
	DWORD			ccFlags;

	// <HandleId> and <taskId> are valid only if event is
	// SONORK_APP_EVENT_SONORK_MSG_SENT or SONORK_APP_EVENT_SONORK_MSG_SENDING
	//  0 otherwise
	DWORD			handleId;
	DWORD			taskId;
	
	// <mark> is valid only if event is SONORK_APP_EVENT_SONORK_MSG_SENT
	// or after SONORK_APP_EVENT_SONORK_MSG_RCVD and/or
	//  SONORK_APP_EVENT_SONORK_MSG_SENDING is processed and stored
	//  (pcFlags has the SONORK_APP_MPF_STORED flag set)
	// it is undefined otherwise.
	TSonorkCCacheMark	mark;


	// <header> is valid for all events
const	TSonorkMsgHeader*	header;

	// <msg> is valid only if event is SONORK_APP_EVENT_SONORK_MSG_RECV
	// or SONORK_APP_EVENT_SONORK_MSG_SENDING, null otherwise
	TSonorkMsg*		msg;

	// <ERR> is valid only if event is SONORK_APP_EVENT_SONORK_MSG_SENT,
	// null otherwise
	const TSonorkError*	ERR;

};

// ----------------------------------------------------------------------------

struct TSonorkAppEventMsgQueryLock
{
	// SONORK_APP_EVENT_SONORK_MSG_QUERY_LOCK
	//  This event is triggered before starting a procedure that processes a
	//    message (and that will probably generate a MSG_PROCESSED event)
	//    to check if there exists another procedure/dialog already processing
	//  All windows with the SONORK_APP_EM_MSG_PROCESSOR flag set will receive
	//  the event.
	//  The window should set <locked> to <true> if the message
	//   being processed matches <pConLineRef>, if not, it should
	//   ignore the message
		TSonorkId		userId;
const	TSonorkCCacheMark*		markPtr;
		TSonorkWin*		owner;
};

//
// ----------------------------------------------------------------------------

struct TSonorkAppEventMsgProcessed
{
	// SONORK_APP_EVENT_SONORK_MSG_PROCESSED
	// This event is triggered after a message has been processed/changed
	//  and the corresponding console file needs to be updated.
	// A window should only process the event if it "knows" how to process it
	//  and ONLY if <consumed> is FALSE. After processing it,
	//  <consumed> should be set to TRUE.
	// The <user_id> and <index> denote the user's console and line number
	//  whose flags need to be modified.
	// <cc_flags> denote the new console flags and <cc_mask> the flags that
	// need to be changed: only those flags set in <mask> should be udpated.
	TSonorkId			userId;
	DWORD				pcFlags;
	DWORD				ccFlags;
	DWORD           		ccMask;
	TSonorkCCacheMark*		markPtr;
	DWORD*				extIndex;
};

// ----------------------------------------------------------------------------
// Used for old V1.04 support. Not used any more in V1.5, except in very few
//  cases when "talking" to an old version.
// Newer versions will depreciate old ctrl messages completely.

// ----------------------------------------------------------------------------

enum SONORK_APP_OLD_CTRLMSG_RESULT_FLAGS
{
  SONORK_APP_OLD_CTRLMSG_RF_MAY_BROADCAST	= 0x000001
, SONORK_APP_OLD_CTRLMSG_RF_MAY_END_QUERY	= 0x000002
, SONORK_APP_OLD_CTRLMSG_RF_DEFAULT		= 0x000003
};

struct TSonorkAppEventOldCtrlMsg
{
	const TSonorkUserHandle*	handle;
	const TSonorkCtrlMsg*		msg;
	const BYTE*			data;
	UINT				data_size;
};

// ----------------------------------------------------------------------------

struct TSonorkAppEventSetUser
{
	// Flags passed as wParam that can be tested
	// for the SONORK_APP_EVENT_SET_USER event
	// MSG_COUNT is a EXT_DATA member, so if MSG_COUNT is set,
	TSonorkId			user_id;
	TSonorkExtUserData*		user_data;
};

// ----------------------------------------------------------------------------
// SONORK_APP_EVENT_USER_SID structures & defs

struct TSonorkAppEventLocalUserSid
{
	// online_dir:
	// -1 if disconnected
	//  0 if changed mode (without disconnecting or connecting)
	//  1 if connected
	int                     onlineDir;
	SONORK_SID_MODE		oldSidMode;
	SONORK_SID_MODE		newSidMode;
	TSonorkExtUserData*	userData;
};

// ----------------------------------------------------------------------------

struct TSonorkAppEventRemoteUserSid
{
	// use <remote> struct if <lParam> of has SET_F_REMOTE flag
	TSonorkId			userId;
	TSonorkSid			sid;
	TSonorkSidFlags			sidFlags;

};

// ----------------------------------------------------------------------------
// TSonorkAppEventUserSid:
// Depending on the event's <lParam>, the  the <local> or <remote> members should be used
// if lParam == SONORK_APP_EVENT_USER_SID_LOCAL, use <local>
// if lParam == SONORK_APP_EVENT_USER_SID_REMOTE, use <remote>
// if neither, IGNORE the event
union TSonorkAppEventUserSid
{
	TSonorkAppEventLocalUserSid	local;
	TSonorkAppEventRemoteUserSid	remote;
};

// ----------------------------------------------------------------------------
struct TSonorkAppEventStartWapp
{
	// The Wapp window that processes this message should
	//  set <consumed> to true.
	bool		       	consumed;
const	TSonorkWappData*	wapp_data;
const	TSonorkExtUserData*	user_data;		// Cannot be <NULL> if replying.
const	TSonorkCCacheMark*	reply_mark;		// Set non-<NULL> if replying
};

struct TSonorkAppTask_SendMsg
{
	TSonorkMsgHeader		header;
	DWORD		   		pcFlags;
	DWORD				handleId;
	TSonorkCCacheMark		mark;
};

struct TSonorkAppTask_SendServiceMsg
{
	DWORD				instance;
	DWORD		   		systemId;
	DWORD				msgType;
};

struct TSonorkAppTask_ExportService
{
	SONORK_APP_TASK_EXPORT_OP	operation;
	TSonorkServiceInstanceInfo	service_info;
	BOOL				entry_dead;
};

struct TSonorkAppTask
{
	SONORK_APP_TASK_TYPE	type;
	union
	{
		TSonorkAppTask_SendMsg		send_msg;
		TSonorkAppTask_ExportService	export_service;
		TSonorkAppTask_SendServiceMsg	send_service_msg;
	};
	TSonorkAppTask(SONORK_APP_TASK_TYPE t){type=t;}
};


enum SONORK_MSG_WIN_OPEN_MODE
{
   SONORK_MSG_WIN_OPEN_MINIMIZED
,  SONORK_MSG_WIN_OPEN_AUTOMATIC
,  SONORK_MSG_WIN_OPEN_FOREGROUND
};


#endif	// #if defined(SONORK_APP_BUILD)


#endif
