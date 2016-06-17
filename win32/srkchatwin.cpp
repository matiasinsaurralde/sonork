#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop

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

#include "srkchatwin.h"
#include "srkultraminwin.h"
#include "srkdialogwin.h"
#include "srkfiletxgui.h"
#include "srkgrpmsgwin.h"
#include "srkinputwin.h"
#include "srk_uts.h"
#include "srk_url_codec.h"

#define CR_SPACING			2
#define CR_INPUT_HEIGHT			64
#define CR_SEPARATOR_HEIGHT		4
#define CR_SEPARATOR_PADDING		4
#define CR_REPLY_BUTTON_HEIGHT		26
/*
#define CR_LEFT_MARGIN			20
#define CR_RIGHT_MARGIN			2
*/
#define CR_TEXT_PADDING			4
#define CR_TEXT_SPACING			1
#define CR_TRACKING_NO_PREFIX_MASK	0x3fff
#define CR_TRACKING_NO_SEQUENCE_MASK	0x3ffff
#define CR_TRACKING_NO_PREFIX_SHIFT     18
#define CR_TIMER_MSECS 			250
#define CR_MAX_NICK_SIZE		24
#define CR_MAX_TEXT_SIZE		1496
#define CR_MAX_ROOM_NAME_SIZE		24
#define CR_MAX_ROOM_TOPIC_SIZE		320
#define CR_MAX_LINE_SIZE		(CR_MAX_NICK_SIZE+CR_MAX_TEXT_SIZE+16)
#define CR_CACHE_SIZE			110
#define CR_QUERY_TIMER_MSECS		1000
#define CR_MAX_IDLE_SECS		3600
#define CR_EXPORT_UPDATE_MSECS		600000
#define CR_PACKET_VERSION_1		1
#define CR_CURRENT_PACKET_VERSION	CR_PACKET_VERSION_1
#define CR_CCACHE_TAG_FLAG		0
#define CR_CCACHE_TAG_ATTR		1

#define CR_F_QUERY_SAVE			SONORK_WIN_F_USER_01
#define CR_F_NO_JOIN_EVENTS		SONORK_WIN_F_USER_02
#define CR_F_SOUND			SONORK_WIN_F_USER_03
#define CR_F_NO_REPLY_MODE		SONORK_WIN_F_USER_04
#define CR_F_EVENT_PENDING              SONORK_WIN_F_USER_05
#define CR_POKE_SEND			SONORK_WIN_POKE_01
#define CR_POKE_OPEN_USER_UNDER_CURSOR	SONORK_WIN_POKE_02
#define CR_POKE_DISCONNECT_USER		SONORK_WIN_POKE_03
#define CR_POKE_COLOR_CHANGED		SONORK_WIN_POKE_04
#define CR_POKE_FIND_LINKED		SONORK_WIN_POKE_05

#define CR_INIT_USER_AVAILABLE		SONORK_WIN_F_USER_01
#define CR_INIT_POKE_START_CLIENT	SONORK_WIN_POKE_01
#define CR_NICK_FROM_TEXT_SEPARATOR_STR	"> "
#define CR_NICK_FROM_TEXT_SEPARATOR_LEN 2

enum SONORK_CHAT_CMD
{
  SONORK_CHAT_CMD_NONE
, SONORK_CHAT_CMD_JOIN_REQ	       	=100
, SONORK_CHAT_CMD_SERVER_DATA	       	=101
, SONORK_CHAT_CMD_JOIN_AKN	       	=102

, SONORK_CHAT_CMD_TEXT		       	=200

, SONORK_CHAT_CMD_USER_JOINED	       	=300
, SONORK_CHAT_CMD_USER_UPDATE
, SONORK_CHAT_CMD_USER_LEFT

, SONORK_CHAT_CMD_SET_USER_FLAGS
, SONORK_CHAT_CMD_SET_SERVER_DATA

, SONORK_CHAT_CMD_USER_LIST_STARTS	=400
, SONORK_CHAT_CMD_USER_LIST_ENDS


};


#define TOOL_BAR_ID			500

#define TOOL_BAR_BUTTONS		4

enum TOOL_BAR_BUTTON

{

	TOOL_BAR_BUTTON_SAVE		= 1000

,	TOOL_BAR_BUTTON_CONFIG

,	TOOL_BAR_BUTTON_SCROLL_LOCK

, 	TOOL_BAR_BUTTON_ULTRA_MINIMIZE

};



// ----------------------------------------------------------------------------

// TSonorkChatWin

// ----------------------------------------------------------------------------


void

 TSonorkChatWin::OnActivate(DWORD flags , BOOL )

{

	if( flags==WA_INACTIVE )

	{

		SonorkApp.CancelPostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)inputHwnd);

	}

	else

	{

		if( !IsUltraMinimized() && !IsIconic() )

		{

			ClearWinUsrFlag(CR_F_EVENT_PENDING);

			SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND

				, (LPARAM)inputHwnd);

		}


	}

}



// ----------------------------------------------------------------------------

// UTS handlers


void	SONORK_CALLBACK

 TSonorkChatWin::UtsEventHandler(void*param,struct TSonorkUTSEvent*E)
{
	TSonorkChatWin *_this=(TSonorkChatWin*)param;

	switch(E->Event())
	{
		case SONORK_UTS_EVENT_SID_PIN:
			SonorkApp.EncodePin64(
				 E->LoginPin()
				,E->LinkUserId()
				,E->LinkServiceId()	
				,0);
			E->SetLoginPinType( SONORK_PIN_TYPE_64 );
		break;

		case SONORK_UTS_EVENT_LOGIN:
			_this->UtsEventLogin(E);
		break;

		case SONORK_UTS_EVENT_STATUS:
			_this->UtsEventStatus(E);
		break;

		case SONORK_UTS_EVENT_DATA:
			_this->UtsEventData(E);
		break;

		case SONORK_UTS_EVENT_CLEAR_TO_SEND:
		default:
		break;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::UtsEventLogin(TSonorkUTSEvent*E)
{
	E->SetLoginAuthorizationAccepted();
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::UtsEventStatus(TSonorkUTSEvent*E)
{
	TSonorkUTSLink*LINK=E->Link();
	DWORD	nickId,nick_flags;
	char	tmp[182];
	if( E->LinkStatus() == SONORK_NETIO_STATUS_CONNECTED
	||  E->LinkStatus() == SONORK_NETIO_STATUS_DISCONNECTED )
	{
		// We keep LINK->Get/SetData() synchronized
		// with the TSonorkNickUserData in our list,
		// so we don't need to look it up everytime.
		nick_flags=LINK->GetData();
		if( chat_mode == CHAT_MODE_SERVER )
		{
			// Server mode

			if( LINK->Status() == SONORK_NETIO_STATUS_CONNECTED )
			{
				if( nick_flags &  SONORK_CHAT_NICK_F_CONNECTED)
					return;

				nick_flags=SONORK_CHAT_NICK_F_CONNECTED;
				room.server.header.userCount++;

			}
			else
			{
				nickId=LINK->Id();

				if( !(nick_flags&SONORK_CHAT_NICK_F_CONNECTED) )
					return;
				DelUser( nickId );
				if( nick_flags & SONORK_CHAT_NICK_F_JOINED )
				{
					BroadcastRaw(LINK->Id()
						,SONORK_CHAT_CMD_USER_LEFT
						,CR_PACKET_VERSION_1
						,&nickId
						,sizeof(nickId));
				}
				nick_flags=0;
				room.server.header.userCount--;
			}

			LINK->SetData( nick_flags );
		}
		else
		{
			// Client mode
			if( LINK->Id() != main_link_id )
				return;	// we're only interested on events of the main link

			if( LINK->Status() == SONORK_NETIO_STATUS_CONNECTED )
			{
				// We've connected (the socket): Send the join request
				Send_Clnt2Svr_JoinReq();
				AddSysLine(GLS_MS_CXWFREPLY);
			}
			else
			{
				// We've disconnected, let the user know
				// and clear the user list
				ClearUserList();
				SonorkApp.LangTranslate(*E->ErrorInfo());
				wsprintf(tmp
					,"%s (%s)"
					,SonorkApp.LangString(GLS_MS_DCXTED),E->ErrorInfo()->text.CStr() );
				AddSysLine(tmp);
			}
		}
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::UtsEventData(TSonorkUTSEvent*E)
{
	if( E->DataPacketVersion() > CR_CURRENT_PACKET_VERSION)
		return;

	if( chat_mode == CHAT_MODE_SERVER )
		ProcessDataInServerMode( E->Link()
			, (SONORK_CHAT_CMD)E->DataPacketCmd()
			, E->DataPtr()
			, E->DataSize() );
	else
		ProcessDataInClientMode(
			  (SONORK_CHAT_CMD)E->DataPacketCmd()
			, E->DataPtr()
			, E->DataSize() );

}

// ----------------------------------------------------------------------------
// SERVER MODE

void
 TSonorkChatWin::ProcessDataInServerMode(TSonorkUTSLink*L
		, SONORK_CHAT_CMD cmd
		, const BYTE*data
		, DWORD data_size)
{

	if( !(L->GetData()&SONORK_CHAT_NICK_F_CONNECTED) )
		return;

	switch( cmd )
	{

		case SONORK_CHAT_CMD_TEXT:
			On_Clnt2Svr_Text(L,data,data_size);
		break;

		case SONORK_CHAT_CMD_JOIN_REQ:
			On_Clnt2Svr_JoinReq(L,data,data_size);
		break;

		case SONORK_CHAT_CMD_USER_UPDATE:
			On_Clnt2Svr_UserUpdate(L,data,data_size);
		break;

		case SONORK_CHAT_CMD_SET_USER_FLAGS:
			On_Clnt2Svr_SetUserFlags(L,data,data_size);
		break;

		case SONORK_CHAT_CMD_SET_SERVER_DATA:
			On_Clnt2Svr_SetServerData(L,data,data_size);
		break;

		default:
#if defined(SONORK_DEBUG)
			TRACE_DEBUG("TSonorkChatWin(SRVR) Unknown Command (%u)", cmd );
#endif
		break;
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Clnt2Svr_SetServerData(TSonorkUTSLink* sourceL
	, const BYTE*data, DWORD data_size )
{
	TSonorkChatServerData	SD;
	if( (sourceL->GetData() & (SONORK_CHAT_NICK_F_JOINED|SONORK_CHAT_NICK_F_OPERATOR))
		!= (SONORK_CHAT_NICK_F_JOINED|SONORK_CHAT_NICK_F_OPERATOR) )
		return;
	if(SD.CODEC_ReadMem(data,data_size)!=SONORK_RESULT_OK)
		return;
	room.server.room_name.Set(SD.room_name);
	room.server.room_topic.Set(SD.room_topic);
	OnServerSettingsChange( true , true );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Clnt2Svr_SetUserFlags(TSonorkUTSLink* sourceL
	, const BYTE*data, DWORD data_size )
{
	TSonorkChatNickData *ND;
	TSonorkChatNickData::HEADER *HDR;
	if( (sourceL->GetData() & (SONORK_CHAT_NICK_F_JOINED|SONORK_CHAT_NICK_F_OPERATOR))
		!= (SONORK_CHAT_NICK_F_JOINED|SONORK_CHAT_NICK_F_OPERATOR)
	|| data_size<sizeof(TSonorkChatNickData::HEADER) )
	{
		return;
	}
	HDR=(TSonorkChatNickData::HEADER *)data;
	if( HDR->nickId == main_link_id )
	{
		// Cannot modify server link
		return;
	}

	if(GetUser(HDR->nickId,&ND,true)==-1)
		return;

	// Copy only flags that may be changed by operator
	if( !(HDR->nick_flags&SONORK_CHAT_NICK_F_CONNECTED))
	{
		PostPoke(CR_POKE_DISCONNECT_USER , HDR->nickId);
	}
	else
	{
		ND->header.nick_flags&=~SONORK_CHAT_NICK_F_OPERATOR;
		ND->header.nick_flags|=(HDR->nick_flags&SONORK_CHAT_NICK_F_OPERATOR);
		OnUserDataChanged( ND , false , false);
		BroadcastAtom( 	  0
				, SONORK_CHAT_CMD_USER_UPDATE
				, CR_PACKET_VERSION_1
				, ND );
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Clnt2Svr_UserUpdate(TSonorkUTSLink* sourceL
		, const BYTE*data, DWORD data_size )
{
	TSonorkChatNickData ND;

	if( !(sourceL->GetData() & SONORK_CHAT_NICK_F_JOINED) )
	{
		return;
	}

	if( ND.CODEC_ReadMem(data,data_size) != SONORK_RESULT_OK )
	{
		return;
	}

	ND.header.nickId	= sourceL->Id();

	if( ND.nick.Length()==0 )	// Don't accept zero length nicks
	{
		return;
	}

	OnUserDataChanged( &ND , true , false );

	BroadcastAtom( 	  ND.header.nickId
			, SONORK_CHAT_CMD_USER_UPDATE
			, CR_PACKET_VERSION_1
			, &ND );

}

// ----------------------------------------------------------------------------

void
	TSonorkChatWin::On_Clnt2Svr_JoinReq(TSonorkUTSLink* sourceL
		, const BYTE*data
		, DWORD data_size)
{
	TSonorkChatNickData		ND;
	TSonorkChatJoinAkn1		AKN;

	if( (sourceL->GetData() & SONORK_CHAT_NICK_F_JOINED) )
		return;

	if( ND.CODEC_ReadMem(data,data_size) != SONORK_RESULT_OK)
		return;

	ND.header.nickId	= sourceL->Id();

	if( ND.nick.Length()==0 )	// Don't accept zero length nicks
		return;

	
	// we keep the nick's flags and the guts link's flags synchronized
	ND.header.nick_flags=sourceL->GetData()|SONORK_CHAT_NICK_F_JOINED;
	ND.header.nick_flags&=~(SONORK_CHAT_NICK_F_OPERATOR
				|SONORK_CHAT_NICK_F_SERVER);
	sourceL->SetData(ND.header.nick_flags);

	AddUser( &ND );

	memcpy(&AKN,&ND.header,sizeof(ND.header));
	if(++room.tracking_no_prefix>CR_TRACKING_NO_PREFIX_MASK)
		room.tracking_no_prefix=1;

	AKN.tracking_no_prefix=room.tracking_no_prefix;
	SONORK_ZeroMem(AKN.reserved,sizeof(AKN.reserved));
	SendRaw(  ND.header.nickId
		, SONORK_CHAT_CMD_JOIN_AKN
		, CR_PACKET_VERSION_1
		, &AKN
		, sizeof(AKN));

	SendAtom( ND.header.nickId
		, SONORK_CHAT_CMD_SERVER_DATA
		, CR_PACKET_VERSION_1
		, &room.server);


	SendUserList(ND.header.nickId);

	BroadcastAtom(ND.header.nickId
		, SONORK_CHAT_CMD_USER_JOINED
		, CR_PACKET_VERSION_1
		, &ND );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Clnt2Svr_Text(TSonorkUTSLink* sourceL
			, const BYTE*data
			, DWORD data_size )
{
	TSonorkChatTextData TD;

	if( !(sourceL->GetData() & SONORK_CHAT_NICK_F_JOINED) )
		return;

	if( TD.CODEC_ReadMem(data,data_size)!=SONORK_RESULT_OK)
		return;

	// Make the nickId match the link's id
	TD.header.sender_nick_id=sourceL->Id();
	AddUserText( &TD );

	// Send the packet to the other users
	BroadcastAtom(TD.header.sender_nick_id
		,SONORK_CHAT_CMD_TEXT
		, CR_PACKET_VERSION_1
		,&TD);
}

// ----------------------------------------------------------------------------
// CLIENT MODE

void
 TSonorkChatWin::ProcessDataInClientMode(
		  SONORK_CHAT_CMD cmd
		, const BYTE*data
		, DWORD data_size)
{
	switch( cmd )
	{
		case SONORK_CHAT_CMD_TEXT:
			On_Svr2Clnt_Text(data,data_size);
		break;

		case SONORK_CHAT_CMD_SERVER_DATA:
			On_Svr2Clnt_ServerData(data,data_size);
		break;

		case SONORK_CHAT_CMD_JOIN_AKN:
			On_Svr2Clnt_JoinAkn(data,data_size);
		break;

		case SONORK_CHAT_CMD_USER_LIST_STARTS:
			SetWinUsrFlag(CR_F_NO_JOIN_EVENTS);
			ClearUserList();
		break;

		case SONORK_CHAT_CMD_USER_JOINED:
			On_Svr2Clnt_UserJoined(data,data_size);

		break;


		case SONORK_CHAT_CMD_USER_LEFT:
			On_Svr2Clnt_UserLeft(data,data_size);

		break;

		case SONORK_CHAT_CMD_USER_LIST_ENDS:
			ClearWinUsrFlag(CR_F_NO_JOIN_EVENTS);
			AddSysLine(GLS_MS_READY);
			break;

		case SONORK_CHAT_CMD_USER_UPDATE:
			On_Svr2Clnt_UserUpdate(data,data_size);
		break;

		default:

#if defined(SONORK_DEBUG)
			TRACE_DEBUG("TSonorkChatWin(CLNT) Unknown Command (%u)",cmd );
#endif
		break;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::Send_Clnt2Svr_JoinReq()
{
	TSonorkChatNickData ND;
	LoadLocalNickData( &ND );
	SendAtom(main_link_id
		, SONORK_CHAT_CMD_JOIN_REQ
		, CR_PACKET_VERSION_1
		, &ND );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_UserUpdate(const BYTE*data, DWORD data_size )
{
	TSonorkChatNickData ND;
	if( ND.CODEC_ReadMem(data,data_size) == SONORK_RESULT_OK )
	{
		OnUserDataChanged( &ND , true , true );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_Text(const BYTE*data, DWORD data_size )
{
	TSonorkChatTextData TD;
	if(TD.CODEC_ReadMem(data,data_size) == SONORK_RESULT_OK)
		AddUserText( &TD );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_ServerData(const BYTE*data, DWORD data_size)
{
	if( room.server.CODEC_ReadMem(data,data_size) != SONORK_RESULT_OK )
		return;
	OnServerSettingsChange( true , false );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_JoinAkn(const BYTE*data, DWORD data_size)
{
	TSonorkChatJoinAkn1*	AKN;
	if(data_size>=sizeof(DWORD))
	{
		AKN =(TSonorkChatJoinAkn1*)data;
		room.nickId = AKN->nickId;
		room.tracking_no_prefix=0;
		if(data_size==sizeof(DWORD))
		{
			// Very old Sonork chat version sent only the nick id
			// (a DWORD) instead of the full header:
			// Use default nick flags
			room.nick_flags = SONORK_CHAT_NICK_F_CONNECTED
					|SONORK_CHAT_NICK_F_JOINED;

		}
		else
		if(data_size>=sizeof(TSonorkChatNickData::HEADER))
		{
			room.nick_flags = AKN->nick_flags;
			if(data_size>=sizeof(TSonorkChatJoinAkn1))
			{
			// New Sonork chat versions send the AKN header
			// TSonorkChatJoinAkn1 which is an extension of
			// TSonorkChatNickData::HEADER and includes the
			// tracking number we should use when sending
			// messages.
				room.tracking_no_prefix=AKN->tracking_no_prefix;
			}
		}
		// If server did not send a tracking_number prefix
		// we derive one from our current session SID
		// (it will hopefully be unique)
		if(room.tracking_no_prefix==0)
		{
			AddSysLine(GLS_CR_NRPLY);
			SetWinUsrFlag(CR_F_NO_REPLY_MODE);
			SetCtrlEnabled(IDC_CHAT_REPLY,false);
			room.tracking_no_prefix =
				SonorkApp.ProfileSid().SessionId()|1;
		}
		room.tracking_no_prefix&=CR_TRACKING_NO_PREFIX_MASK;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_UserJoined(const BYTE*data, DWORD data_size)
{
	TSonorkChatNickData ND;
	if(ND.CODEC_ReadMem(data,data_size) == SONORK_RESULT_OK)
		AddUser( &ND );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::On_Svr2Clnt_UserLeft(const BYTE*data, DWORD data_size)
{
	if( data_size < sizeof(DWORD))
		return;
	DelUser( *(DWORD*)data );
}



// ----------------------------------------------------------------------------

// Creation/Destruction



TSonorkChatWin::TSonorkChatWin()
	:TSonorkWin(NULL
	, SONORK_WIN_CLASS_NORMAL
	 |SONORK_WIN_TYPE_SONORK_CHAT
	 |SONORK_WIN_DIALOG
	 |IDD_CHAT
	, SONORK_WIN_SF_NO_WIN_PARENT)
{
	chat_mode=CHAT_MODE_NONE;
	guts=NULL;
	cache = NULL;
	ultra_min.win = NULL;
	ultra_min.x=-1;
	SetEventMask(SONORK_APP_EM_CTRL_MSG_CONSUMER
	 |SONORK_APP_EM_PROFILE_AWARE
	 |SONORK_APP_EM_SKIN_AWARE);

	
}

// ----------------------------------------------------------------------------

TSonorkChatWin::~TSonorkChatWin()
{
	if(cache)
		delete cache;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::LoadLabels()
{
	::SendMessage(inputHwnd
		, WM_SETFONT
		, (WPARAM)sonork_skin.Font(SKIN_FONT_LARGE)
		, MAKELPARAM(1,0));
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::OnCreate()
{
	char			name[48];
	TSonorkTempBuffer 	path(SONORK_MAX_PATH);
	SIZE			size;
	bool			db_reset;
	static TSonorkWinToolBarButton	btn_info[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_BUTTON_SAVE
			, SKIN_ICON_FILE_DOWNLOAD
			, GLS_OP_STORE
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }

	,	{	TOOL_BAR_BUTTON_CONFIG
			, SKIN_ICON_TOOLS
			, GLS_OP_CFG
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }

	,	{	TOOL_BAR_BUTTON_SCROLL_LOCK
			, SKIN_ICON_NO_SCROLL
			, GLS_LB_SCRLCK
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE }
	,	{ 	TOOL_BAR_BUTTON_ULTRA_MINIMIZE
			, SKIN_ICON_CLOSE_UP
			, GLS_OP_UMIN
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE  }
	};

	toolbar.hwnd=TSonorkWin::CreateToolBar(Handle()
			,	TOOL_BAR_ID
			,	 WS_VISIBLE
				| TBSTYLE_TOOLTIPS
				| TBSTYLE_FLAT
				| TBSTYLE_LIST
				| CCS_NOPARENTALIGN
				| CCS_NODIVIDER
				| CCS_NORESIZE
			, TOOL_BAR_BUTTONS
			, btn_info
			, &size);
	if( SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_CHAT_SOUND))
		SetWinUsrFlag(CR_F_SOUND);
	toolbar.height = size.cy;
	link_ctx.Clear();
	menu_nick_id = main_link_id = room.nickId = room.nick_flags = 0;
	room.color    = SonorkApp.ProfileCtrlValue(SONORK_PCV_CHAT_COLOR);
	room.tracking_no_prefix=
	room.tracking_no_sequence=0;


	inputHwnd	= GetDlgItem( IDC_CHAT_INPUT );
	inputHeight	= CR_INPUT_HEIGHT;
	inputDropTarget.AssignCtrl( Handle(), inputHwnd);

	SetEditCtrlMaxLength(IDC_CHAT_INPUT,CR_MAX_TEXT_SIZE-1 );

	userListView.SetHandle( GetDlgItem( IDC_CHAT_USERS )
		, true
		, false );
	userListWidth = GetCtrlWidth( IDC_CHAT_USERS );
	userListDropTarget.AssignCtrl( Handle(), userListView.Handle() );
	userListDropIndex=-1;
	userListView.AddColumn( 0 , "" , userListWidth - 4);

	ShowScrollBar(userListView.Handle() , SB_HORZ , false);

	wsprintf(name,"~%u.%u~%x_chat"
		,SonorkApp.ProfileUserId().v[0]
		,SonorkApp.ProfileUserId().v[1]
		,this);
	SonorkApp.GetDirPath(path.CStr(),SONORK_APP_DIR_TEMP,name);
	db_reset=true;
	db.Open( SonorkApp.ProfileUserId() , path.CStr() , db_reset );


	SonorkApp.GetTempPath(path.CStr(),"chat",NULL,(DWORD)this);
	cache = new TSonorkCCache(CR_MAX_LINE_SIZE, CR_CACHE_SIZE , CCacheCallback, this);
	cache->Open( path.CStr() );
	cache->Clear( true );

	console = new TSonorkConsole(this,cache,ConsoleCallback,this,0);
	console->Create();

	inputCtrl.AssignCtrl(console,inputHwnd,IDC_MSG_INPUT);

	console->EnableSelect( true );
	console->EnableLineDrag( true );
//	console->SetMargins(CR_LEFT_MARGIN,CR_RIGHT_MARGIN);
	console->SetPaddingEx(CR_TEXT_PADDING,CR_TEXT_SPACING);
	console->SetDefColors(SKIN_COLOR_CHAT);

	RealignControls();
	console->ShowWindow(SW_SHOW);
	SetCaptionIcon( SKIN_HICON_CHAT );
	ShowWindow(SW_SHOW);
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::OnBeforeDestroy()
{
	inputCtrl.ReleaseCtrl();
	inputDropTarget.Enable(false);
	userListDropTarget.Enable(false);
	console->Destroy();
	cache->Close();
	db.Close();
	StopRoom();
	ClearUserList();
}

// ----------------------------------------------------------------------------
// Export


struct TSonorkChatWinExportTag
{
	TSonorkShortString	str;
	char			bg_color[16];
};

void
 TSonorkChatWin::CmdSave()
{
	TSonorkShortString 		path;
	TSonorkChatWinExportTag     	tag;

	TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_CHAT, SKIN_CI_BG) , tag.bg_color);

	console->ClearSelection();
	console->CloseView();
	path.SetBufferSize(SONORK_MAX_PATH);
	strcpy(path.Buffer(),"chat");
	console->Export(path.Buffer()
		,SONORK_CONSOLE_EXPORT_F_ASK_PATH
		|SONORK_CONSOLE_EXPORT_F_ADD_TIME_SUFFIX
		|SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS
		,&tag
		,NULL);
	ClearWinUsrFlag(CR_F_QUERY_SAVE);
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkChatWin::OnConsole_Export(TSonorkConsoleExportEvent* EV)
{
	char			tmp1[64];
	char           		fg_color[16];
	TSonorkChatWinExportTag	*tag;
	TSonorkCCacheEntry	*pEntry;
	SONORK_C_CSTR		line_str;

	tag = (TSonorkChatWinExportTag*)EV->tag;

	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_LINE)
	{
		pEntry =EV->data.line;
		if(db.Get(pEntry->dat_index
			,&TSonorkCodecShortStringAtom(&tag->str)) != SONORK_RESULT_OK )
			return 0L;
		TSonorkConsole::WinToRGB(pEntry->tag.v[SONORK_CHAT_TEXT_TAG_ATTR]&0xffffff , fg_color );
		line_str = tag->str.CStr();
	}
	else
	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_START )
	{
		GetWindowText(Handle(), tmp1 , CR_MAX_ROOM_NAME_SIZE);
		tmp1[CR_MAX_ROOM_NAME_SIZE-1]=0;
	}

	if( EV->format == SONORK_CONSOLE_EXPORT_HTML )
	{
		switch(EV->section)
		{
			case SONORK_CONSOLE_EXPORT_SECTION_START:
			{

				fputs("<HTML>\n<HEAD>\n<TITLE>",EV->file);
				fprintf(
					EV->file
					,"%u.%u : "
					,SonorkApp.ProfileUserId().v[0]
					,SonorkApp.ProfileUserId().v[1]
					);
				SONORK_HtmlPuts( EV->file, tmp1 );
				fprintf(EV->file," (%u/%u/%u %u:%u)</TITLE>\n"
					"</HEAD>\n"
					"<BODY>\n"
					"<BASEFONT FACE=\"Arial,Helvetica\">\n"
					,EV->st.wYear
					,EV->st.wMonth
					,EV->st.wDay
					,EV->st.wHour
					,EV->st.wMinute);
				fprintf(EV->file,
					"<CENTER>\n"
					"<TABLE BORDER=0 CELLSPACING=1 CELLPADDING=4>\n");
			}
			break;

			case SONORK_CONSOLE_EXPORT_SECTION_COMMENTS:
			if(*EV->data.comments)
			{
				fputs(" <TR>\n"
					 "  <TD COLSPAN=2 BGCOLOR=#F0F0E0><font size=+1><b>"
					 ,EV->file);
				SONORK_HtmlPuts(EV->file,EV->data.comments);
				fputs("</b></font></TD>\n </TR>\n",EV->file);
			}
			break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:

			fprintf(EV->file,"<TR><TD BGCOLOR=#%s><font color=#%s>"
				,tag->bg_color
				,fg_color);

			SONORK_HtmlPuts(EV->file, line_str );
			fputs("</TD></TR>\n",EV->file);

			break;

			case SONORK_CONSOLE_EXPORT_SECTION_END:
			fprintf(EV->file,
				"</TABLE>\n"
				"<br><br><FONT SIZE=-2>Sonork Export</FONT>"
				"</CENTER>\n"
				"</BODY>\n"
				"</HTML>\n");
			break;
		}
	}
	else
	if( EV->format == SONORK_CONSOLE_EXPORT_TEXT )
	{
		switch(EV->section)
		{
			case SONORK_CONSOLE_EXPORT_SECTION_START:
				{
					fprintf(EV->file,"%u.%u: %s (%u/%u/%u %u:%u)\n"
						,SonorkApp.ProfileUserId().v[0]
						,SonorkApp.ProfileUserId().v[1]
						,tmp1
						,EV->st.wYear
						,EV->st.wMonth
						,EV->st.wDay
						,EV->st.wHour
						,EV->st.wMinute);
					TSonorkConsole::SepLine(EV->file,8);
				}
				break;
			case SONORK_CONSOLE_EXPORT_SECTION_COMMENTS:
				if(*EV->data.comments)
				{
					fprintf(EV->file,"%s\n",EV->data.comments);
				}
				fprintf(EV->file,"\n");
				TSonorkConsole::SepLine(EV->file,8);
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:
				fputs(line_str , EV->file);
				fputc('\n',EV->file);
				TSonorkConsole::SepLine(EV->file,2);
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_END:
				fprintf(EV->file,"Sonork Export\n");
			break;
		}
	}

	return 0L;
}

// ----------------------------------------------------------------------------
// Console/History callbacks
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkChatWin::ConsoleCallback(void*			pTag
				,SONORK_CONSOLE_EVENT 	gcEvent
				,DWORD			pIndex
				,void*			pData)
{
	TSonorkChatWin*_this = (TSonorkChatWin*)pTag;
	switch(gcEvent)
	{
		case SONORK_CONSOLE_EVENT_HISTORY_EVENT:
			return _this->OnHistory_Event((TSonorkHistoryWinEvent*)pData);

		case SONORK_CONSOLE_EVENT_EXPORT:
			return _this->OnConsole_Export((TSonorkConsoleExportEvent*)pData);

		case SONORK_CONSOLE_EVENT_VKEY:
			if( pIndex == 10 || pIndex == VK_RETURN )
			{

				_this->SendPoke(CR_POKE_SEND,0);

				return true;

			}

			else

			if( pIndex == VK_ESCAPE )

			{

				_this->ClrCtrlText(IDC_CHAT_INPUT);

				return true;

			}

			break;

	}
	return 0L;
}

void
 TSonorkChatWin::OpenLine( DWORD line_no )
{
	TSonorkCCacheEntry * 	CL;
	SONORK_C_CSTR		pLineText;
	SONORK_C_CSTR		pUrlStart;

	if((CL = console->Get( line_no , &pLineText)) == NULL )
		return;
	if( CL->tag.v[SONORK_CHAT_TEXT_TAG_FLAG]&SONORK_CHAT_TEXT_FLAG_URL )
	{
		if((pUrlStart=strstr(pLineText,CR_NICK_FROM_TEXT_SEPARATOR_STR))==NULL)
			return;
		pUrlStart+=CR_NICK_FROM_TEXT_SEPARATOR_LEN;
		SonorkApp.OpenUrl(this , pUrlStart);
	}

}
//				OpenMsg(E->ClickFlags() & SONORK_HIST_WIN_FLAG_DOUBLE_CLICK);

// ----------------------------------------------------------------------------

DWORD
 TSonorkChatWin::OnHistory_Event(TSonorkHistoryWinEvent*E)
{
	switch( E->Event() )
	{
		case SONORK_HIST_WIN_EVENT_LINE_PAINT:
			OnHistory_LinePaint( E->Line(), E->PaintContext() );
		break;

		case SONORK_HIST_WIN_EVENT_GET_TEXT:
			OnHistoryWin_GetText(E->LineNo(),E->GetTextData());
		break;

		case SONORK_HIST_WIN_EVENT_SEL_CHANGE:
		break;

		case SONORK_HIST_WIN_EVENT_LINE_CLICK:
			if( !(E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED ))
			{
			// Don't search for linked line if focus has changed
			// because it does not look right. If we do that, the
			// user will click on the line (at the icon) but
			//  the focus will go elsewhere (to the linked line).
				if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_LICON_CLICK) )
				{
					PostPoke(CR_POKE_FIND_LINKED,false);
					break;
				}
			}
			if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
			{
				OpenLine(E->LineNo());
				break;
			}

			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
			{
				TrackPopupMenu(SonorkApp.ChatViewMenu()
						, TPM_LEFTALIGN  | TPM_LEFTBUTTON
						,E->ClickPoint().x
						,E->ClickPoint().y
						,0
						,Handle()
						,NULL);
			}
		break;

		case SONORK_HIST_WIN_EVENT_LINE_DRAG:
			OnHistory_LineDrag( E->LineNo() );
		break;

	}
	return 0L;
}

// ----------------------------------------------------------------------------

BOOL SONORK_CALLBACK
 TSonorkChatWin::CCacheCallback(void*tag,TSonorkCCacheEntry*pE,char*str,UINT str_size)
{
	TSonorkChatWin *_this = (TSonorkChatWin*)tag;
	TSonorkCodecLCStringAtom	A;
	A.ptr = str;
	A.length_or_size = str_size;
	if(_this->db.Get(pE->dat_index,&A) != SONORK_RESULT_OK )
		strcpy(str,"<error>");
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::OnHistoryWin_GetText(
	 DWORD		line_no
	,TSonorkDynData*DD)
{
	SONORK_C_CSTR	pszText;
	if(cache->Get( line_no, &pszText , NULL ))
		DD->AppendStr(pszText,false);
}


// ----------------------------------------------------------------------------

void
 TSonorkChatWin::OnHistory_LineDrag(DWORD line_no)
{
	TSonorkDropSourceData 	*SD;
	TSonorkClipData         *sCD;
	const TSonorkCCacheEntry*pE;
	SONORK_C_CSTR			pszText;
	DWORD				aux=0;
	TSonorkTempBuffer		buffer( CR_MAX_LINE_SIZE );

	SD = new TSonorkDropSourceData;
	sCD = SD->GetClipData();
	if( console->SelectionActive() )
	{
		TSonorkListIterator I;
		TSonorkClipData    *nCD;
		DWORD			line_no;
		sCD->SetSonorkClipDataQueue();
		console->SortSelection();
		console->InitEnumSelection(I);
		while((line_no = console->EnumNextSelection(I))!=SONORK_INVALID_INDEX)
		{
			pE = cache->Get(line_no , &pszText , NULL );
			if( pE==NULL )
				continue;
			nCD = new TSonorkClipData;
			nCD->SetCStr( SONORK_CLIP_DATA_TEXT , pszText );

			if(sCD->AddSonorkClipData( nCD ))
				aux++;
			nCD->Release();
		}
	}
	else
	{
		pE = cache->Get(line_no , &pszText , NULL );
		if( pE!=NULL )
		{
			sCD->SetCStr( SONORK_CLIP_DATA_TEXT , pszText );
			aux=1;
		}
	}
	if( aux > 0 )
	{
		aux=0;
		SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
	}
	SD->Release();
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::OnTimer(UINT)
{
	if( guts != NULL )guts->TimeSlot(0);
	room.last_update_msecs+=CR_TIMER_MSECS;
	if( chat_mode == CHAT_MODE_SERVER )
	if( room.last_update_msecs > CR_EXPORT_UPDATE_MSECS )
	{
		if(room.server.header.flags & SONORK_CHAT_SERVER_F_PUBLIC)
			SetPublic( true );
		else
			room.last_update_msecs=0;

	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::SendInputText( BOOL reply )
{
	TSonorkCCacheEntry *CL;
	TSonorkChatTextData TD;
	if(guts== NULL || room.nickId == 0)
		return;
	GetCtrlText( IDC_CHAT_INPUT , TD.text );
	TD.text.CutAt(CR_MAX_TEXT_SIZE);
	if( TD.text.Length() )
	{
		TD.header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]= 0;
		TD.header.tag.v[SONORK_CHAT_TEXT_TAG_ATTR]= (room.color&0xffffff);
		TD.header.sender_nick_id= room.nickId;

		TD.header.tracking_no.v[1]=0;
		if(reply)
		{
			// Replying to
			if((CL = console->GetFocused(NULL,NULL))!=NULL)
			{
				TD.header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]|=SONORK_CCLF_IN_THREAD;
				TD.header.tracking_no.v[1] = CL->tracking_no.v[0];
			}
		}

		if( SONORK_IsUrl( TD.text.CStr() ) )
			TD.header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]|=SONORK_CHAT_TEXT_FLAG_URL;

		if(++room.tracking_no_sequence>CR_TRACKING_NO_SEQUENCE_MASK)
			room.tracking_no_sequence=1;

		TD.header.tracking_no.v[0]=room.tracking_no_sequence;

		if( chat_mode == CHAT_MODE_SERVER )
		{
			BroadcastAtom(0
				, SONORK_CHAT_CMD_TEXT
				, CR_PACKET_VERSION_1
				, &TD );
		}
		else
		{
			TD.header.tracking_no.v[0]|=
				(room.tracking_no_prefix<<CR_TRACKING_NO_PREFIX_SHIFT);
			SendAtom(main_link_id
				, SONORK_CHAT_CMD_TEXT
				, CR_PACKET_VERSION_1
				, &TD );
		}
		AddUserText(&TD);
		SetCtrlText( IDC_CHAT_INPUT , (char*)NULL);
	}
	UltraMinimize();
}

// ----------------------------------------------------------------------------

bool

 TSonorkChatWin::OnCommand(UINT id,HWND hwnd, UINT code)
{
	TSonorkChatNickData	*ND;
	if( hwnd == toolbar.hwnd )
	{
		if(code==BN_CLICKED)
		switch(id)
		{
			case TOOL_BAR_BUTTON_SAVE:
				CmdSave();
			break;

			case TOOL_BAR_BUTTON_SCROLL_LOCK:
			if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_SCROLL_LOCK)& TBSTATE_CHECKED ))
				console->MakeLastLineVisible();
			break;
			
			case TOOL_BAR_BUTTON_ULTRA_MINIMIZE:
				UltraMinimize();
			break;

			case TOOL_BAR_BUTTON_CONFIG:
				CmdConfig();
			break;
			

			default:
				return false;
		}
		return true;
	}
	else
	if(code==BN_CLICKED && (id==IDOK || id==IDC_CHAT_REPLY))
	{
		SendInputText(id==IDC_CHAT_REPLY);
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)inputHwnd);
	}
	else
	if( hwnd == NULL )
	{
		switch(id)
		{
			case CM_MSGS_TOGGLE_SEL:
				console->SelectToggleFocused();
			break;

			case CM_MSGS_CLEAR_SEL:
				console->ClearSelection();
			break;

			case CM_CLIPBOARD_COPY:
				console->CopyToClipboard(0);
			break;


			case CM_EXPORT:
				CmdSave();
			break;
			
			case CM_MSGS_SINGLE_LINE_MODE:
				console->SetMaxScanLines(
					console->GetMaxScanLines() == 1
					?SONORK_HIST_WIN_DEFAULT_MAX_SCAN_LINES
					:1);
				break;

			default:
				if(GetUser(menu_nick_id,&ND,true)==-1)
					break;
				switch(id)
				{
					case CM_USER_INFO:
						SonorkApp.OpenUserDataWin(&ND->user_data,NULL,0);
						break;

					case CM_AUTHORIZE:
						CmdAuthorizeUser(ND);
						break;

					case CM_DISCONNECT:
						CmdDisconnectUser(ND);
						break;
				}
			break;
		}
		menu_nick_id=0;
	}
	return false;
}


// ----------------------------------------------------------------------------

void
 TSonorkChatWin::CmdConfig()
{
	DWORD	id;
	TSonorkChatServerData SD;
	SD.CODEC_Copy(&room.server);
	id = TSonorkChatSetWin(this,&SD,room.nick_flags,&room.color).Execute();
	if( id== IDOK )
	{
		if(chat_mode == CHAT_MODE_SERVER)
		{
			room.server.room_name.Set( SD.room_name);
			room.server.room_topic.Set( SD.room_topic);
			SetPrivate( SD.ServerFlags() & SONORK_CHAT_SERVER_F_PRIVATE );

			id=room.server.ServerFlags()&SONORK_CHAT_SERVER_F_PUBLIC;
			if((SD.ServerFlags()&SONORK_CHAT_SERVER_F_PUBLIC) != id || id!=0)
				SetPublic( SD.ServerFlags() & SONORK_CHAT_SERVER_F_PUBLIC );
			OnServerSettingsChange( true , true );
		}
		else
		if( room.nick_flags & SONORK_CHAT_NICK_F_OPERATOR )
		{
			SendAtom(main_link_id
				, SONORK_CHAT_CMD_SET_SERVER_DATA
				, CR_PACKET_VERSION_1
				, &SD );

		}
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::CmdAuthorizeUser(TSonorkChatNickData*ND)
{
	if(chat_mode == CHAT_MODE_SERVER)
	{
		if( ND->NickId() != main_link_id )
		{
			ND->header.nick_flags^=SONORK_CHAT_NICK_F_OPERATOR;
			OnUserDataChanged( ND , false , false );
			BroadcastAtom( 0
				, SONORK_CHAT_CMD_USER_UPDATE
				, CR_PACKET_VERSION_1
				, ND );
		}

	}
	else
	if( room.nick_flags&SONORK_CHAT_NICK_F_OPERATOR)
	{
		Send_Clnt2Svr_SetUserFlags(ND
			,ND->NickFlags()^SONORK_CHAT_NICK_F_OPERATOR
			,SONORK_CHAT_NICK_F_OPERATOR);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::CmdDisconnectUser(const TSonorkChatNickData*ND)
{
	if(chat_mode == CHAT_MODE_SERVER)
	{
		if( ND->NickId() != main_link_id )
		{
			PostPoke(CR_POKE_DISCONNECT_USER,ND->NickId());
		}
	}
	else
	if( room.nick_flags&SONORK_CHAT_NICK_F_OPERATOR)
	{
		Send_Clnt2Svr_SetUserFlags(ND
			,0
			,SONORK_CHAT_NICK_F_CONNECTED);
	}
}

// ----------------------------------------------------------------------------

void
  TSonorkChatWin::Send_Clnt2Svr_SetUserFlags(const TSonorkChatNickData*ND
	,DWORD flags
	,DWORD mask)
{
	TSonorkChatNickData::HEADER header;
	memcpy(&header,&ND->header,sizeof(header));
	header.nick_flags&=~mask;
	header.nick_flags|=flags;
	SendRaw(main_link_id
		, SONORK_CHAT_CMD_SET_USER_FLAGS
		, CR_PACKET_VERSION_1
		, &header
		, sizeof(header));
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::OnAppEvent(UINT event, UINT param,void*data)
{
	switch( event)
	{
		case SONORK_APP_EVENT_SET_PROFILE:
		case SONORK_APP_EVENT_SID_CHANGED:
			OnAppEvent_SetProfile();
			return true;

		case SONORK_APP_EVENT_OLD_CTRL_MSG:
			if( param == room.server.header.descriptor.serviceId )
				return OnAppEvent_OldCtrlMsg((TSonorkAppEventOldCtrlMsg*)data);
			break;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::OnAppEvent_SetProfile()
{
	TSonorkChatNickData ND;
	if(room.nickId == 0) // Not in a room
		return;

	LoadLocalNickData( &ND );
	if(!OnUserDataChanged( &ND , true , false ))
		return;
	if( chat_mode == CHAT_MODE_CLIENT )
		SendAtom( main_link_id
		, SONORK_CHAT_CMD_USER_UPDATE
		, CR_PACKET_VERSION_1
		, &ND );
	else
		BroadcastAtom( 0
		, SONORK_CHAT_CMD_USER_UPDATE
		, CR_PACKET_VERSION_1
		, &ND );
}

// ----------------------------------------------------------------------------
// OnAppEvent_OldCtrlMsg
// Processes old-style CTRL messages used before V1.5
//  Needed to send the of list rooms to an old Sonork versions.
// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::OnAppEvent_OldCtrlMsg(TSonorkAppEventOldCtrlMsg*E)
{
	const TSonorkCtrlMsg *rMsg	= E->msg;
	if( room.nickId == 0 )// Not in a room
		return false;
	if( rMsg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_QUERY_REQUEST )
	if( rMsg->body.o.param[ 0 ]== SONORK_CHAT_OLD_CTRLMSG_PARAM0_LIST_ROOMS )
	{
		TSonorkCtrlMsg		akn;

		akn.cmdFlags 		= SONORK_OLD_CTRLMSG_CMD_QUERY_RESULT;
		akn.sysFlags		= 0;
		akn.SetOldServiceId(room.server.header.descriptor.serviceId);
		akn.SetOldQueryId(rMsg->OldQueryId() );
		akn.body.o.param[ 0 ]	= rMsg->body.o.param[ 0 ];
		akn.body.o.param[ 1 ]	= rMsg->body.o.param[ 1 ];
		akn.body.o.param[ 2 ]	= 0;
		akn.body.o.param[ 3 ]	= 0;
		SonorkApp.SendCtrlMsg(E->handle
			, &akn
			, &room.server
			, NULL) ;
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------

void TSonorkChatWin::BroadcastRaw(DWORD sourceID
		, SONORK_CHAT_CMD 	cmd
		, BYTE			packet_ver
		, const void *  	data
		, DWORD			data_size)
{
#define A_size	data_size
	if( room.server.header.userCount > 1 || sourceID == 0 )
	{
		UINT		P_size;
		TSonorkUTSPacket	*P;
		P=SONORK_AllocUtsPacket( A_size );
		P_size = P->E_Data(
			  A_size
			, cmd
			, packet_ver
			, data
			, data_size);
		BroadcastPacket( sourceID , P , P_size );	// P is freed by BroadcastPacket
	}
#undef A_size
}

// ----------------------------------------------------------------------------

void
	TSonorkChatWin::BroadcastAtom(DWORD sourceID
		, SONORK_CHAT_CMD 	cmd
		, BYTE			packet_ver
		, const TSonorkCodecAtom*A)
{
	if( room.server.header.userCount > 1 || sourceID == 0 )
	{
		UINT		A_size, P_size;
		TSonorkUTSPacket	*P;
		A_size = A->CODEC_Size();
		P=SONORK_AllocUtsPacket( A_size );
		P_size = P->E_Atom(
			  A_size
			, cmd
			, packet_ver
			, A);
		BroadcastPacket( sourceID , P , P_size );	// P is freed by BroadcastPacket
	}
}

// ---------------------------------------------------------------------------
// BroadcastPacket
// NB!: The packet <P> is freed by BroadcastPacket
// does not send packet to sourceID if sourceID<>0

void
 TSonorkChatWin::BroadcastPacket(DWORD sourceID,TSonorkUTSPacket*P,UINT P_size)
{
	TSonorkListIterator I;
	TSonorkUTSLink*		targetL;

	guts->InitEnumLink(I);
	while( (targetL = guts->EnumNextLink(I)) != NULL )
	{
		if( targetL->GetData() == NULL )
			continue;

		if( targetL->Status() != SONORK_NETIO_STATUS_CONNECTED )
			continue;

		if( targetL->Id() == sourceID )
			continue;

		guts->SendPacket( gutsERR
			, targetL->Id()
			, P
			, P_size
			, NULL	// param
			, NULL	// callback
			, NULL	// tag
			);
	}
	SONORK_FreeUtsPacket(P);
}


// ---------------------------------------------------------------------------
// MISC (Paint/Align/Color)



LRESULT

 TSonorkChatWin::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)

{

	if( uMsg   == WM_CTLCOLOREDIT )

	if( lParam == (LPARAM)inputHwnd )
	{
		::SetTextColor((HDC)wParam
			, room.color);
		::SetBkColor((HDC)wParam
			, GetSysColor(COLOR_WINDOW));
		return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
	}
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);
}


// ---------------------------------------------------------------------------


void

 TSonorkChatWin::OnHistory_LinePaint(const TSonorkCCacheEntry*CL, TSonorkHistoryWinPaintCtx*CTX)

{
	DWORD 		cFlags=CL->tag.v[SONORK_CHAT_TEXT_TAG_FLAG];
	DWORD&		ctxFlags=CTX->flags;
//	HBRUSH		brush;
	COLORREF	textC;
//	SKIN_ICON	icon;

	//ctxFlags |= SONORK_HIST_WIN_PAINT_F_LEFT_PAINTED;
	ctxFlags &=~(SONORK_HIST_WIN_PAINT_F_HOT_LICON);
	if( cFlags&SONORK_CHAT_TEXT_FLAG_URL)
	{
		CTX->r_icon=SKIN_ICON_MORE_URL;
	}


	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED )
	{
		CTX->SetLineColor(sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_BG));
		textC = sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_FG);
	}
	else
	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_SELECTED )
	{
		CTX->SetLineColor(sonork_skin.Color(SKIN_COLOR_MSG_SELECT,SKIN_CI_BG));
		textC = sonork_skin.Color(SKIN_COLOR_MSG_SELECT,SKIN_CI_FG);
	}
	else
	{
		CTX->SetLineColor(sonork_skin.Color(SKIN_COLOR_CHAT,SKIN_CI_BG));
		textC = CL->tag.v[SONORK_CHAT_TEXT_TAG_ATTR]&0xffffff;
	}
	/*
	::FillRect(CTX->hDC(),CTX->LeftRect(),brush);
	*/
	if( cFlags & SONORK_CHAT_TEXT_FLAG_SYSTEM )
	{
		CTX->l_icon=SKIN_ICON_INFO;
		/*
		sonork_skin.DrawIcon(CTX->hDC()
			, SKIN_ICON_INFO
			, CTX->LeftRect()->left + (CR_LEFT_MARGIN - SKIN_ICON_SW)/2
			, CTX->TextRect()->top );
		*/
	}
	else
	{
		if( cFlags&SONORK_CCLF_IN_THREAD )
		{
			ctxFlags|=SONORK_HIST_WIN_PAINT_F_HOT_LICON;
			CTX->l_icon=cFlags&SONORK_CCLF_INCOMMING
					?SKIN_ICON_IN_REPLY
					:SKIN_ICON_OUT_REPLY;
		}
		else
		{
			CTX->l_icon=cFlags&SONORK_CCLF_INCOMMING
					?SKIN_ICON_INCOMMING
					:SKIN_ICON_OUTGOING;
		}
		/*
		sonork_skin.DrawIcon(CTX->hDC()
			, icon
			, CTX->LeftRect()->left + (CR_LEFT_MARGIN - SKIN_ICON_SW)/2
			, CTX->TextRect()->top );
		*/
	}
	CTX->SetTextColor( textC );

}

// ---------------------------------------------------------------------------

void TSonorkChatWin::OnSize(UINT size_type)
{
	if( size_type == SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
	{
		RealignControls();
	}
}

// ---------------------------------------------------------------------------

void
 TSonorkChatWin::RealignControls()
{
	const UINT H = Height() ;
	const UINT W = Width();
	int   y,listX,consoleW,consoleH;
	HDWP defer_handle;
	HWND	separator=GetDlgItem(IDC_CHAT_SEPARATOR);


	consoleW	= W - userListWidth - CR_SPACING * 4;
	consoleH	= H -
				(inputHeight
				+ CR_SEPARATOR_HEIGHT
				+ CR_SEPARATOR_PADDING*2
				+ CR_SPACING * 3
				+ toolbar.height
				+ 4 // bottom spacing
				);
	listX = consoleW + CR_SPACING*3;

	defer_handle = BeginDeferWindowPos( 7 );

	defer_handle = DeferWindowPos(defer_handle
		,toolbar.hwnd
		,NULL
		,CR_SPACING
		,CR_SPACING
		,W-CR_SPACING
		,toolbar.height
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y = CR_SPACING*2+toolbar.height;

	defer_handle = DeferWindowPos(defer_handle
		,userListView.Handle()
		,NULL
		,listX
		,y
		,userListWidth
		,consoleH
		,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,console->Handle()
		,NULL
		,CR_SPACING
		,y
		,consoleW
		,consoleH
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=consoleH+CR_SEPARATOR_PADDING;

	defer_handle = DeferWindowPos(defer_handle
		,separator
		,NULL
		,CR_SPACING
		,y
		,W-CR_SPACING*2
		,CR_SEPARATOR_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=CR_SEPARATOR_HEIGHT+CR_SEPARATOR_PADDING;

	defer_handle = DeferWindowPos(defer_handle
		,inputHwnd
		,NULL
		,CR_SPACING
		,y
		,consoleW
		,inputHeight
		,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,GetDlgItem(IDC_CHAT_REPLY)
		,NULL
		,listX
		,y
		,userListWidth
		,CR_REPLY_BUTTON_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+=CR_REPLY_BUTTON_HEIGHT+CR_SPACING;

	defer_handle = DeferWindowPos(defer_handle
		,GetDlgItem(IDOK)
		,NULL
		,listX
		,y
		,userListWidth
		,inputHeight - (CR_REPLY_BUTTON_HEIGHT+CR_SPACING)
		,SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(defer_handle);
	 ::InvalidateRect(separator,NULL,false);
}

// ---------------------------------------------------------------------------

bool
 TSonorkChatWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	DWORD		state;
	int		delta;
	SKIN_ICON	icon;
	if(S->CtlID == IDOK || S->CtlID==IDC_CHAT_REPLY)
	{
		icon = S->CtlID == IDOK
			?SKIN_ICON_SEND_MSG
			:SKIN_ICON_REPLY_MSG;
		if(S->itemState&ODS_SELECTED)
		{
			delta=1;
			state = DFCS_BUTTONPUSH|DFCS_PUSHED;
		}
		else
		{
			state = DFCS_BUTTONPUSH;
			delta = 0;
		}
		DrawFrameControl(S->hDC
			, &S->rcItem
			, DFC_BUTTON
			, state);

		sonork_skin.DrawIcon(S->hDC
			, icon
			, S->rcItem.left + delta + ( (S->rcItem.right - S->rcItem.left) - SKIN_ICON_SW )/2
			, S->rcItem.top  + delta + ( (S->rcItem.bottom - S->rcItem.top) - SKIN_ICON_SH )/2
			, S->itemState&ODS_DISABLED?ILD_BLEND25:ILD_NORMAL);
		return true;
	}
	return false;

}

// ---------------------------------------------------------------------------

#define A_size	32
void
 TSonorkChatWin::SendUserList(DWORD userID)
{
	UINT				P_size;
	int				item_no,items;
	TSonorkUTSPacket		*P;
	TSonorkChatNickData		*ND;

	P=SONORK_AllocUtsPacket( A_size );
	P_size = P->E_Data(
			  A_size
			, SONORK_CHAT_CMD_USER_LIST_STARTS
			, CR_PACKET_VERSION_1
			, 0
			, NULL);
	guts->SendPacket( gutsERR
		, userID
		, P
		, P_size
		, NULL	// param
		, NULL	// callback
		, NULL	// tag
		);

	items = userListView.GetCount();
	for(item_no=0; item_no<items;item_no++)
	{
		ND = (TSonorkChatNickData*)userListView.GetItem( item_no );
		if( ND == NULL ) continue;
		guts->SendPacket( gutsERR
				, userID
				, SONORK_CHAT_CMD_USER_JOINED
				, CR_PACKET_VERSION_1
				, ND
				, NULL	// param
				, NULL	// callback
				, NULL	// tag
				);
	}
	P_size = P->E_Data(
			  A_size
			, SONORK_CHAT_CMD_USER_LIST_ENDS
			, CR_PACKET_VERSION_1
			, NULL
			, 0);
	guts->SendPacket( gutsERR
		, userID
		, P
		, P_size
		, NULL	// param
		, NULL	// callback
		, NULL	// tag
		);
	SONORK_FreeUtsPacket(P);

}
#undef A_size

// ---------------------------------------------------------------------------

SONORK_RESULT
	TSonorkChatWin::SendAtom(DWORD linkID
		, SONORK_CHAT_CMD cmd
		, BYTE	packet_ver
		, const TSonorkCodecAtom* A)
{
	return guts->SendPacket(gutsERR
					, linkID
					, cmd
					, packet_ver
					, A
					, NULL	// param
					, NULL	// callback
					, NULL	// tag
					);
}
SONORK_RESULT
	TSonorkChatWin::SendRaw(DWORD linkID
	, SONORK_CHAT_CMD 	cmd
	, BYTE			packet_ver
	, const void *		data
	, DWORD			data_size)
{
	return guts->SendPacket(gutsERR
					, linkID
					, cmd
					, packet_ver
					, (const BYTE*)data
					, data_size
					, NULL	// param
					, NULL	// callback
					, NULL	// tag
					);
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkChatWin::OnQueryClose()
{
	if(TestWinUsrFlag(CR_F_QUERY_SAVE))
	{
		UINT id;
		id = MessageBox(GLS_QU_SAVEXIT
			,GLS_CR_NAME
			,MB_YESNOCANCEL|MB_ICONQUESTION);
		if(id==IDCANCEL)
			return true;
		StopRoom();
		if(id==IDYES)
			CmdSave();
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddText(SONORK_C_CSTR pStr
		, const SONORK_DWORD2& tag
		, const SONORK_DWORD2& tracking_no
		, DWORD nickId)
{
	TSonorkCCacheEntry		CL;
	TSonorkCodecLCStringAtom	A;

	A.ptr 			= (char*)pStr;
	A.length_or_size    	= strlen(pStr);

	db.Add(&A,&CL.dat_index);

	CL.tag.Set( tag );
	CL.tracking_no.Set(tracking_no);
	CL.time.SetTime_Local();
	CL.ext_index = nickId;


	console->Add( CL );
	if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_SCROLL_LOCK)& TBSTATE_CHECKED ))
		console->MakeLastLineVisible();
	if( tag.v[SONORK_CHAT_TEXT_TAG_FLAG]&(SONORK_CCLF_INCOMMING|SONORK_CHAT_TEXT_FLAG_SYSTEM) )
	if( !IsActive() || IsUltraMinimized() || IsIconic() )
	{
		if( last_sound_time!=SonorkApp.CurrentTime() )
		{
			if( TestWinUsrFlag(CR_F_SOUND) )
				SonorkApp.AppSound( SONORK_APP_SOUND_MSG_LO );
			last_sound_time=SonorkApp.CurrentTime();
		}
		if( !IsUltraMinimized() )
		{
			FlashWindow(Handle() , true);
			Sleep(50);
			FlashWindow(Handle() , true);
		}
		else
		{
			if(!TestWinUsrFlag(CR_F_EVENT_PENDING))
				ultra_min.win->InvalidateRect(NULL,false);
		}
		SetWinUsrFlag(CR_F_EVENT_PENDING);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddUserText(TSonorkChatTextData*TD)
{
	TSonorkChatNickData* ND;
	TSonorkTempBuffer buffer( CR_MAX_LINE_SIZE );
	TD->header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]&=~SONORK_CHAT_TEXT_FLAG_SYSTEM;

	if( TD->header.sender_nick_id == room.nickId )
	{
		TD->header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]&=~SONORK_CCLF_INCOMMING;
	}
	else
	{
		TD->header.tag.v[SONORK_CHAT_TEXT_TAG_FLAG]|=SONORK_CCLF_INCOMMING;
	}
	if( GetUser(TD->header.sender_nick_id , &ND , true) != -1 )
	{
		wsprintf(buffer.CStr()
			,"%s%s%s"
			,ND->user_data.alias.CStr()
			,CR_NICK_FROM_TEXT_SEPARATOR_STR
			,TD->text.ToCStr());

		AddText(  buffer.CStr()
			, TD->header.tag
			, TD->header.tracking_no
			, TD->header.sender_nick_id );
	}
	SetWinUsrFlag(CR_F_QUERY_SAVE);
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddSysLine(SONORK_C_CSTR str)
{
	TSonorkTempBuffer tmp(CR_MAX_TEXT_SIZE + 64);
	SONORK_DWORD2 tag;
	SONORK_DWORD2 tracking_no;
	tag.Set(SONORK_CHAT_TEXT_FLAG_SYSTEM,sonork_skin.Color(SKIN_COLOR_CHAT_EXT,SKIN_CI_CHAT_SYS));
	tracking_no.Clear();
	AddText(str
		,tag
		,tracking_no
		,SONORK_INVALID_INDEX);
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddSysLine( GLS_INDEX gls )
{
	AddSysLine(SonorkApp.LangString(gls));
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::SetUserListDropTarget( int new_index )
{
	if( userListDropIndex == new_index )
		return;

	if( userListDropIndex != -1 )
		userListView.SetDropTarget(userListDropIndex,false);

	userListDropIndex=new_index;

	if( userListDropIndex != -1 )
		userListView.SetDropTarget(userListDropIndex,true);

}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	union
	{
		LPARAM			lParam;
		TSonorkDropQuery*	query;
		TSonorkDropMsg*		msg;
		TSonorkDropExecute*	exec;
	}D;
	int			index;
	TSonorkClipData*   	CD;
	D.lParam=lParam;
	switch(event)
	{
		case SONORK_DRAG_DROP_QUERY:
			SonorkApp.ProcessDropQuery(D.query
			,SONORK_DROP_ACCEPT_F_SONORKCLIPDATA
			|SONORK_DROP_ACCEPT_F_FILES
			|SONORK_DROP_ACCEPT_F_URL
			|SONORK_DROP_ACCEPT_F_TEXT
			);
			SetUserListDropTarget(-1);
		break;
		case SONORK_DRAG_DROP_UPDATE:
			if(D.msg->target->CtrlHandle() == userListView.Handle()  )
			{
				userListView.HitTest(
					  D.msg->point->x
					, D.msg->point->y
					, LVHT_ONITEMICON|LVHT_ONITEMLABEL
					, &index);
				SetUserListDropTarget( index );
			}
			*D.msg->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE;
		break;

		case SONORK_DRAG_DROP_CANCEL:
			SetUserListDropTarget(-1);
		break;

		case SONORK_DRAG_DROP_EXECUTE:
			CD = SonorkApp.ProcessDropExecute(D.exec);
			if( CD != NULL )
			{
				if( CD->DataType() == SONORK_CLIP_DATA_USER_ID)
				{
					Invite(SonorkApp.GetUser( CD->GetUserId() )
						, NULL);
				}
				else
				if( CD->DataType() == SONORK_CLIP_DATA_USER_DATA)
				{
					Invite(CD->GetUserData() , NULL );

				}
				else
				if( CD->DataType() == SONORK_CLIP_DATA_FILE_NAME
				||  CD->DataType() == SONORK_CLIP_DATA_FILE_LIST)
				{
					SendFiles(D.exec->target,CD);
				}
				else
				{
					SetCtrlDropText( IDC_CHAT_INPUT , CD , "\r\n");
				}
				CD->Release() ;
			}
			SetUserListDropTarget( -1 );
		break;
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::SendFiles( TSonorkDropTarget* T, TSonorkClipData* CD )
{
	char	tmp[128];
	TSonorkShortStringQueue*queue;
	union{
		TSonorkFileTxGui *file;
		TSonorkGrpMsgWin *group;
	}W;
	TSonorkChatNickData*	ND;
	SONORK_DWORD2List	dw2list;
	int	 		item_no,items;
	if( T->CtrlHandle() == inputHwnd )
	{
		// All users
		W.group=NULL;
		items = userListView.GetCount();
		for(item_no=0;item_no<items;item_no++)
		{
			ND = (TSonorkChatNickData*)userListView.GetItem(item_no);
			if( ND == NULL )
				continue;
			if( ND->user_data.userId== SonorkApp.ProfileUserId())
				continue;

			if(W.group==NULL)
			{
				W.group=new TSonorkGrpMsgWin(false);
				if(!W.group->Create())
				{
					delete W.group;
					return;
				}
			}
			W.group->AddUser(
				 ND->user_data.userId
				,ND->user_data.alias.CStr()
				);
		}
		if(W.group != NULL )
		{
			W.group->AfterAddUsers();
			W.group->ProcessDrop(CD);
			W.group->ShowWindow(SW_SHOW);
		}
	}
	else
	{
		// Highlighted user
		if(userListDropIndex==-1)
			return;
		ND = (TSonorkChatNickData*)userListView.GetItem(userListDropIndex);
		if( ND == NULL )
			return;
		if( ND->user_data.userId== SonorkApp.ProfileUserId())
			return;
		dw2list.AddItem(&ND->user_data.userId);
		if( CD->DataType() == SONORK_CLIP_DATA_FILE_NAME )
		{
			SONORK_MEM_NEW( queue = new TSonorkShortStringQueue );
			queue->Add( CD->GetCStr() );
		}
		else
		if( CD->DataType() == SONORK_CLIP_DATA_FILE_LIST )
		{
			queue = (TSonorkShortStringQueue*)CD->ReleaseData();
		}
		else
			assert( false );
		wsprintf(tmp,"%u %s"
			, dw2list.Items()
			, SonorkApp.LangString(dw2list.Items()==1?GLS_LB_USR:GLS_LB_USRS));
		W.file=new TSonorkFileTxGui(dw2list.ItemPtr(0)
			,dw2list.Items()
			,tmp
			,queue
			,0);
		if(!W.file->Create())
			delete W.file;
	}

}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch(wParam)
	{
		case CR_POKE_SEND:
			SendInputText( lParam );
		break;
		case CR_POKE_DISCONNECT_USER:
			if( chat_mode == CHAT_MODE_SERVER )
				guts->Disconnect( lParam , false );
			break;

		case CR_POKE_COLOR_CHANGED:
			SonorkApp.ProfileCtrlValue(SONORK_PCV_CHAT_COLOR)=room.color;
			::InvalidateRect(inputHwnd,NULL,true);
			break;

		case CR_POKE_OPEN_USER_UNDER_CURSOR:
			Poke_OpenUser(lParam);
		break;

		case CR_POKE_FIND_LINKED:
			console->FocusLinked( false );
		break;

		case SONORK_WIN_POKE_DESTROY:
			Destroy();
		break;


		case  SONORK_WIN_POKE_ULTRA_MIN_DESTROY:
			ultra_min.x = ultra_min.win->Left();
			ultra_min.y = ultra_min.win->Top();
			ultra_min.win=NULL;

			console->MakeLastLineVisible();

			SonorkApp.PostAppCommand(
				 SONORK_APP_COMMAND_FOREGROUND_HWND
				,(LPARAM)Handle());
			break;

		case SONORK_WIN_POKE_ULTRA_MIN_PAINT:
			UltraMinimizedPaint((TSonorkUltraMinPaint*)lParam);
			break;

	}
	return 0L;
}
// ----------------------------------------------------------------------------


void

 TSonorkChatWin::UltraMinimizedPaint(TSonorkUltraMinPaint* P)
{
	if( TestWinUsrFlag(CR_F_EVENT_PENDING) )

	{
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_FG);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_BG);
	}
	else
	{
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MAIN_EXT
				,SKIN_CI_MAIN_ONLINE);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG);
	}
	P->icon = SKIN_ICON_CHAT;

	lstrcpyn(P->text,room.server.room_name.CStr(),sizeof(P->text));

}


// ----------------------------------------------------------------------------


void

 TSonorkChatWin::UltraMinimize()
{
	RECT	rect;
	if( !(ToolBar_GetButtonState(toolbar.hwnd,TOOL_BAR_BUTTON_ULTRA_MINIMIZE)& TBSTATE_CHECKED) )
		return;

	if(ultra_min.win != NULL)
		return;
	ShowWindow(SW_HIDE);
	if(ultra_min.x==-1)
	{
		GetWindowRect(&rect);
		ultra_min.x = rect.left;
		ultra_min.y = rect.top;
	}

	ultra_min.win=new TSonorkUltraMinWin(this);
	ultra_min.win->Show(ultra_min.x,ultra_min.y);
}


// ----------------------------------------------------------------------------

void
 TSonorkChatWin::Poke_OpenUser(LPARAM lParam)
{
	POINT	cursor_pos;
	TSonorkChatNickData	*ND;
	menu_nick_id = 0;
	GetCursorPos(&cursor_pos);
	ND = (TSonorkChatNickData*)userListView.HitTest(
			 cursor_pos.x
			,cursor_pos.y
			,LVHT_ONITEMICON|LVHT_ONITEMLABEL);
	if( ND==NULL )
		return;

	if( lParam )
	{
		SonorkApp.OpenUserDataWin(&ND->user_data,NULL,0);
	}
	else
	{
		menu_nick_id 	= ND->NickId();
		// lParam is true if the nick is NOT the host
		lParam		= menu_nick_id != room.server.NickId();
		EnableMenuItem(SonorkApp.ChatUserMenu()
			,CM_USER_INFO
			,menu_nick_id!=room.nickId // Disable for self
			?MF_BYCOMMAND|MF_ENABLED
			:MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(SonorkApp.ChatUserMenu()
			,CM_DISCONNECT
			,(room.nick_flags&SONORK_CHAT_NICK_F_OPERATOR)
			&& lParam
			?MF_BYCOMMAND|MF_ENABLED
			:MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(SonorkApp.ChatUserMenu()
			,CM_AUTHORIZE
			,(room.nick_flags&SONORK_CHAT_NICK_F_OPERATOR)
			&& lParam
			?MF_BYCOMMAND|MF_ENABLED
			:MF_BYCOMMAND|MF_GRAYED);
		CheckMenuItem(SonorkApp.ChatUserMenu()
			,CM_AUTHORIZE
			, (ND->NickFlags()&SONORK_CHAT_NICK_F_OPERATOR)
			?MF_BYCOMMAND|MF_CHECKED
			:MF_BYCOMMAND|MF_UNCHECKED);

		TrackPopupMenu(SonorkApp.ChatUserMenu()
				, TPM_LEFTALIGN  | TPM_LEFTBUTTON
				, cursor_pos.x
				, cursor_pos.y
				, 0
				, Handle()
				, NULL);
	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::SetPublic( BOOL set )
{
	if( chat_mode != CHAT_MODE_SERVER )
		return false;
	room.server.header.flags&=~SONORK_CHAT_SERVER_F_PRIVATE;
	room.last_update_msecs = 0;
	SonorkApp.Service_ExportService( room.server.header.descriptor.instance , set);
	return true;
}

// ----------------------------------------------------------------------------


void
 TSonorkChatWin::SetPrivate( BOOL set )
{
	if( chat_mode != CHAT_MODE_SERVER)
		return;
	if( set && !(room.server.header.flags&SONORK_CHAT_SERVER_F_PUBLIC))
		room.server.header.flags|=SONORK_CHAT_SERVER_F_PRIVATE;
	else
		room.server.header.flags&=~SONORK_CHAT_SERVER_F_PRIVATE;
}

// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkChatWin::ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*
			, TSonorkAppServiceEvent*	E)
{
	TSonorkCodecAtom	*A;

	TSonorkChatWin*		_this;

	SONORK_SERVICE_TYPE	svc_type;

	_this = (TSonorkChatWin*)handler_tag.v[0];

	switch( event_id )

	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:

		E->get_name.str->Set(_this->room.server.room_name.CStr());

		return 1;


	case SONORK_APP_SERVICE_EVENT_EXPORT:

		if( E->exported.value != 0 )

		{

			_this->room.server.header.flags|=SONORK_CHAT_SERVER_F_PUBLIC;

		}

		else

		{

			_this->room.server.header.flags&=~SONORK_CHAT_SERVER_F_PUBLIC;

		}

		_this->OnServerSettingsChange( true , true );

	break;


	case SONORK_APP_SERVICE_EVENT_BUSY:

		ToolBar_SetButtonState(_this->toolbar.hwnd

			,TOOL_BAR_BUTTON_CONFIG

			,E->exported.value != 0 ? 0 : TBSTATE_ENABLED );

	break;


	case SONORK_APP_SERVICE_EVENT_GET_SERVER_DATA:

		if( !E->get_server.IsRemoteQuery()

		|| !(_this->room.server.header.flags&SONORK_CHAT_SERVER_F_PRIVATE) )

		{

			_this->room.server.SaveInto(E->get_server.data);

			return true;

		}

	break;


	case SONORK_APP_SERVICE_EVENT_QUERY_REQ:

		// Remote side is requesting server data
		if(E->query_req.Cmd() == SONORK_CTRLMSG_CMD_GET_SERVER_DATA )
		if(_this->room.nickId != 0)
		if(!(_this->room.server.header.flags&SONORK_CHAT_SERVER_F_PRIVATE) )
		{
			TSonorkUserServer	userver;
			if(E->query_req.UserParam() == SONORK_SERVICE_TYPE_SONORK_CHAT )
			{
			// Server data is being requested in native format
				A=&_this->room.server;
				svc_type=SONORK_SERVICE_TYPE_SONORK_CHAT;
			}
			else
			{
			// Server data is being requested in generic
			// or unkown format: Send as generic.
				A=&userver;
				_this->room.server.SaveInto(&userver);
				svc_type=SONORK_SERVICE_TYPE_NONE;
			}

			SonorkApp.Service_Reply(
			  SONORK_CTRLMSG_CMD_GET_SERVER_DATA
			, svc_type		// usr_param
			, 0			// usr_flags
			, A);
		}
	break;

	}

	return 0;

}


// ----------------------------------------------------------------------------


void

 TSonorkChatWin::OnServerSettingsChange(BOOL update_ui, BOOL bcast_to_room)

{

	if(update_ui)

		UpdateUI();
	if( chat_mode == CHAT_MODE_SERVER && bcast_to_room)
	{
		BroadcastAtom( 0
		, SONORK_CHAT_CMD_SERVER_DATA
		, CR_PACKET_VERSION_1
		, &room.server);
	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::Invite(const TSonorkUserData*UD, SONORK_C_CSTR ptext)
{
	TSonorkChatInviteData	ID;
	TSonorkServiceQuery	handle;
	TSonorkTempBuffer 	tmp(CR_MAX_NICK_SIZE + 128);
	SONORK_RESULT		result;
	UINT 			aux;


	if( UD==NULL )return false;
	if( UD->GetUserInfoLevel() < SONORK_USER_INFO_LEVEL_1 )return false;
	if( UD->userId==SonorkApp.ProfileUserId()  )return false;

	SONORK_ZeroMem(&ID.header.reserved,sizeof(ID.header.reserved));
	LoadLocalNickData( &ID.nick_data );
	ID.server_data.CODEC_Copy(&room.server);
	ID.server_data.header.physAddr.GetStr(tmp.CStr());
	TRACE_DEBUG("INVITE TO '%s'",tmp.CStr());
	if( ptext == NULL )
	{
		TSonorkInputWin W(this);
		W.flags=SONORK_INPUT_F_LONG_TEXT;
		W.sign=SKIN_SIGN_USERS;
		W.help.Set(SonorkApp.LangString(GLS_CR_INVUSR0));
		W.prompt.Set(SonorkApp.LangString(GLS_LB_NOTES));
		aux = W.Execute();
		if(aux != IDOK)return false;
		ID.text.SetStr( 0 , W.input.CStr() );
	}
	else
		ID.text.SetStr( 0 , ptext );

	UD->GetUserHandle(&handle);
	if( UD->SidVersion().UsesOldCtrlMsg() )
	{
		TSonorkCtrlMsg		msg;
		msg.ClearOldParams();
		msg.sysFlags		= 0;
		msg.cmdFlags 		= SONORK_OLD_CTRLMSG_CMD_EXEC_START;
		msg.SetOldServiceId( room.server.header.descriptor.serviceId);
		msg.SetOldQueryId(0);
		msg.body.o.param[ 0 ]	= SONORK_CHAT_OLD_CTRLMSG_PARAM0_INVITE;

		result=SonorkApp.SendCtrlMsg(&handle
			,&msg
			,&ID
			,NULL);
	}
	else
	{
		handle.LoadTarget(SONORK_SERVICE_ID_SONORK_CHAT
			, SONORK_SERVICE_LOCATOR_INSTANCE(0));

		result=SonorkApp.Service_SendPokeData(
			  service_instance
			, &handle
			, SONORK_CTRLMSG_CMD_START
			, 0	// usr_param
			, 0	// usr_flags
			, &ID);
	}

	SonorkApp.LangSprintf(tmp.CStr()
		,GLS_CR_INVUSR1
		,UD->alias.CStr());
	AddSysLine(tmp.CStr());

	return result==SONORK_RESULT_OK;


}

// ----------------------------------------------------------------------------

TSonorkChatWin*
 TSonorkChatWin::CreateServer(SONORK_C_CSTR name, SONORK_C_CSTR topic, DWORD max_users)
{
	TSonorkChatWin*_this;
	_this = new TSonorkChatWin;
	if( !_this->Create() )
	{
		delete _this;
		return NULL;
	}

	if(  _this->StartRoom( CHAT_MODE_SERVER , max_users) )
	{
		TSonorkChatNickData ND;
		char	tmp[64];

		_this->room.nickId 	= _this->main_link_id ;
		_this->room.nick_flags = (SONORK_CHAT_NICK_F_CONNECTED
				|SONORK_CHAT_NICK_F_JOINED
				|SONORK_CHAT_NICK_F_OPERATOR
				|SONORK_CHAT_NICK_F_SERVER);
		_this->room.server.room_name.Set( name );
		_this->room.server.room_topic.Set( topic );
		_this->room.server.room_name.CutAt(CR_MAX_ROOM_NAME_SIZE);
		_this->room.server.room_topic.CutAt(CR_MAX_ROOM_TOPIC_SIZE);

		_this->room.server.header.nickId = _this->room.nickId;
		_this->room.server.header.version = SONORK_CHAT_VERSION_NUMBER;
		// Use the main link's address because that's how the rest
		// of the world sees us
		_this->room.server.header.physAddr.SetInet1(
			  SONORK_PHYS_ADDR_TCP_1
			, SonorkApp.ProfileUser().addr.physAddr.GetInet1Addr()
			, _this->guts->MainLink()->PhysAddr().GetInet1Port() );
		SonorkApp.ProfileUser().addr.physAddr.GetStr(tmp);
		_this->room.server.header.physAddr.GetStr(tmp);

		_this->LoadLocalNickData( &ND );

		//SetWinUsrFlag(CR_F_NO_JOIN_EVENTS);
		_this->AddUser( &ND );
		//ClearWinUsrFlag(CR_F_NO_JOIN_EVENTS);
		return _this;
	}
	_this->Destroy();
	return NULL;
}

// ----------------------------------------------------------------------------

TSonorkChatWin*
 TSonorkChatWin::CreateClient(const TSonorkUserServer* userver)
{
	TSonorkChatServerData SD;

	SD.LoadFrom(userver);
	return CreateClient(&SD);
}

TSonorkChatWin*
 TSonorkChatWin::CreateClient( const TSonorkChatServerData* pSD )
{
	TSonorkChatWin*_this;
	_this = new TSonorkChatWin;

	if(!_this->Create() )
	{
		delete _this;
		return NULL;
	}

	if(  _this->StartRoom( CHAT_MODE_CLIENT , 0 ) )
	{

		char tmp[256];
		int l;

		// Copy server data only after starting the room because
		// StartRoom() uses room.server.header.physAddr
		// as parameter to uts->Startup()
		// uts->Startup() returns the assigned address:port
		// used when in SERVER mode.

		_this->room.server.CODEC_Copy( pSD );

		_this->room.server.header.physAddr.GetStr(tmp);
		TRACE_DEBUG("CLIENT: ROOM IS '%s'",tmp);

		_this->main_link_id =
			_this->guts->ConnectToUts(
				  _this->gutsERR
				, &_this->room.server.header.descriptor
				, _this->room.server.header.physAddr
				, 0);
		if( _this->main_link_id == 0 )
		{
			_this->AddSysLine(GLS_MS_CXFAIL);
			l=sprintf(tmp,"%u.%u@%x.%x "
			,_this->room.server.header.descriptor.locus.userId.v[0]
			,_this->room.server.header.descriptor.locus.userId.v[1]
			,_this->room.server.header.descriptor.locus.sid.v[0]
			,_this->room.server.header.descriptor.locus.sid.v[1]);
			_this->room.server.header.physAddr.GetStr(tmp+l);
			_this->AddSysLine(tmp);
			// Let the user close the window
		}
		else
		{
			SonorkApp.ProfileUser().addr.physAddr.GetStr(tmp);
			_this->room.server.header.physAddr.GetStr(tmp);
			_this->AddSysLine(GLS_MS_CXTING);
			return _this;
		}
		return NULL;
	}
	_this->Destroy();
	return NULL;
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::StartRoom( CHAT_MODE cm , DWORD max_users )
{
	UINT		link_flags;
	SONORK_DWORD2	sii_tag;
	TSonorkServiceInstanceInfo	sii;
	if( Handle() == NULL )
		return false;

	// We load the server data /descriptor with our local data because
	// this is what UTS expects: Uts will copy the data and later use
	// it for loging on.
	// If we're starting in CLIENT mode, the server data will be overwritten
	// with what the server sends us.
	// If we're starting in SERVER mode, the server data will remain unchanged.


	SonorkApp.ProfileUser().GetLocus1(&room.server.header.descriptor.locus);
	room.server.header.descriptor.serviceId= SONORK_SERVICE_ID_SONORK_CHAT;
	room.server.header.descriptor.flags	= SONORK_UTS_APP_F_SINGLE_USER_INSTANCE;
	room.server.header.userCount	= 0;
	room.server.header.flags	= 0;

	sii.SetInstanceInfo(
			  SONORK_SERVICE_ID_SONORK_CHAT
			, SONORK_SERVICE_TYPE_SONORK_CHAT
			, 0	// hflags
			, 0	// instance: Don't know yet, set to 0
			, SONORK_CHAT_VERSION_NUMBER);


	sii_tag.v[0]=(DWORD)this;
	sii_tag.v[1]=0;
	if(SonorkApp.Service_Register(
		  cm == CHAT_MODE_CLIENT
			?SONORK_APP_SERVICE_TYPE_CLIENT
			:SONORK_APP_SERVICE_TYPE_SERVER
		, &sii
		, SKIN_ICON_CHAT
		, ServiceCallback
		, &sii_tag)!=SONORK_RESULT_OK)
			return false;

	service_instance = sii.ServiceInstance();
	if( cm == CHAT_MODE_CLIENT )
	{
		link_flags  = SONORK_UTS_LINK_F_NO_LISTEN;
		room.server.header.descriptor.instance= 0;
	}
	else
	{
		link_flags  = 0;
		room.server.header.descriptor.instance= service_instance;
	}

	chat_mode = cm;
	SONORK_MEM_NEW( guts = new TSonorkUTS(max_users) );



	main_link_id = guts->Startup(gutsERR
				, room.server.header.descriptor
				, UtsEventHandler
				, this
				, link_flags
				, INADDR_ANY
				, SonorkApp.CurrentServerProfile().nat.range.v[0]
				, SonorkApp.CurrentServerProfile().nat.range.v[1]
				);
	if( main_link_id != 0 )
	{
		guts->SocksInfo().Set( SonorkApp.SonorkSocksInfo() );
		SetAuxTimer(CR_TIMER_MSECS);
		guts->SetMaxLinkIdleSecs(CR_MAX_IDLE_SECS);
	}
	else
	{
		// Failed to start!
		StopRoom();
	}
	return main_link_id != 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::StopRoom()
{
	if( chat_mode != CHAT_MODE_NONE )
	{
		SonorkApp.Service_Unregister(
			  service_instance
			, false);


		room.nickId=room.nick_flags = main_link_id = 0 ;
		if( guts != NULL )
		{
			guts->Shutdown();
			SONORK_MEM_DELETE(guts);
			guts=NULL;
		}
		KillAuxTimer();
		chat_mode=CHAT_MODE_NONE;
	}
}

// ----------------------------------------------------------------------------

int
 TSonorkChatWin::GetUser( DWORD pId, TSonorkChatNickData**pND , bool by_nick_id)
{
	int 			items;
	LV_ITEM			lv_item;
	if( by_nick_id )
	{
		items 				= userListView.GetCount();
		lv_item.iSubItem	= 0;
		lv_item.mask 		= LVIF_PARAM;
		for(lv_item.iItem=0;lv_item.iItem<items;lv_item.iItem++)
		{
			ListView_GetItem(userListView.Handle(),&lv_item);
			if( (*pND = (TSonorkChatNickData*)lv_item.lParam)!= NULL )
			if( (*pND)->NickId() == pId )
				return lv_item.iItem;
		}
	}
	else
	{
		*pND = (TSonorkChatNickData*)userListView.GetItem( pId );
		if( *pND != NULL )
			return pId;
	}
	return -1;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddSetUserViewItem(const TSonorkChatNickData* pND, int index)
{
	TSonorkTempBuffer 	tmp(128);
	LV_ITEM			lv_item;
	wsprintf(tmp.CStr()
		, "%-.20s (%u.%u)"
		, pND->nick.CStr()
		, pND->user_data.userId.v[0]
		, pND->user_data.userId.v[1]
		);
	lv_item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lv_item.iSubItem=0;
	lv_item.pszText=tmp.CStr();
	lv_item.iImage =SonorkApp.GetUserModeIcon( &pND->user_data );
	lv_item.lParam =(LPARAM)pND;
	if( index == -1 )
	{
		lv_item.iItem  =0;
		ListView_InsertItem(userListView.Handle(),&lv_item);
	}
	else
	{
		lv_item.iItem  =index;
		ListView_SetItem(userListView.Handle(),&lv_item);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::AddUser(const TSonorkChatNickData*pND)
{
	TSonorkChatNickData* nND;

	SONORK_MEM_NEW( nND = new TSonorkChatNickData );

	nND->CODEC_Copy(pND);
	AddSetUserViewItem( nND , -1 );

	if( !TestWinUsrFlag(CR_F_NO_JOIN_EVENTS) )
	{
		TSonorkTempBuffer tmp(CR_MAX_NICK_SIZE + 128);
		SonorkApp.LangSprintf(tmp.CStr(),GLS_CR_ACJOIN,nND->nick.CStr());
		AddSysLine(tmp.CStr());
	}

}

// ----------------------------------------------------------------------------

bool
  TSonorkChatWin::OnUserDataChanged( const TSonorkChatNickData*pND
	, bool copy_data
	, bool copy_flags)
{
	TSonorkChatNickData* 	eND;
	int 			index;
	DWORD			saved_nick_flags;
	index = GetUser( pND->NickId() , &eND , true );
	if(  index != -1 )
	{
		if( copy_data )
		{
			saved_nick_flags = eND->NickFlags();
			eND->CODEC_Copy(pND);
			if( !copy_flags )
			{
				eND->header.nick_flags = saved_nick_flags;
			}
		}
		else
		{
			assert( eND == pND );
		}
		AddSetUserViewItem( eND , index );
		if(eND->NickId() == room.nickId)
		{
			room.nick_flags = eND->NickFlags();
			UpdateUI();
		}
		else
		if(chat_mode==CHAT_MODE_SERVER && guts!=NULL)
		{
			guts->SetLinkData( eND->NickId() , eND->NickFlags() );
		}

		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::DelUser(DWORD link_id)
{
	int index;
	TSonorkChatNickData* pND;
	index = GetUser( link_id , &pND , true );
	if(  index != -1 )
	{
		if(!TestWinUsrFlag(CR_F_NO_JOIN_EVENTS))
		{
			TSonorkTempBuffer tmp(CR_MAX_NICK_SIZE + 128);
			SonorkApp.LangSprintf(tmp.CStr(),GLS_CR_ACLEAVE,pND->nick.CStr());
			AddSysLine(tmp.CStr());
		}
		userListView.DelItem( index );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::ClearUserList()
{
	userListView.DelAllItems();
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	TSonorkChatNickData	*ND;
	if( N->hdr.hwndFrom==userListView.Handle()  )
	{
		if( N->hdr.code == LVN_DELETEITEM  )
		{
			ND = (TSonorkChatNickData*)(N->lview.lParam);
			if( userListDropIndex == N->lview.iItem)
				userListDropIndex=-1;
			if(ND)
			{
				SONORK_MEM_DELETE(ND);
			}
		}
		else
		if( N->hdr.code == NM_DBLCLK  || N->hdr.code==NM_RCLICK)
		{
			PostPoke(CR_POKE_OPEN_USER_UNDER_CURSOR
			,N->hdr.code == NM_DBLCLK);
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::LoadLocalNickData( TSonorkChatNickData*ND )
{
	SONORK_ZeroMem(
		  ND->header.reserved
		, sizeof(ND->header.reserved));

	ND->header.nickId	=room.nickId;
	ND->header.nick_flags	=room.nick_flags;
	ND->user_data.Set( SonorkApp.ProfileUser() );
	ND->nick.Set( SonorkApp.ProfileUser().alias );

}

// ----------------------------------------------------------------------------

bool
 TSonorkChatWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=320;
	MMI->ptMinTrackSize.y=240;
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatWin::UpdateUI()
{
#define CAPTION_SIZE	256
	char	tmp[CAPTION_SIZE];
	int	l;
	lstrcpyn(tmp, room.server.room_name.ToCStr() , CR_MAX_ROOM_NAME_SIZE);
	l=strlen(tmp);
	l+=sprintf(tmp+l
		,"|%cp%ci%co|"
		,room.server.header.flags&SONORK_CHAT_SERVER_F_PUBLIC?'+':'-'
		,room.server.header.flags&SONORK_CHAT_SERVER_F_PRIVATE?'+':'-'
		,room.nick_flags&SONORK_CHAT_NICK_F_OPERATOR?'+':'-'
		);
	lstrcpyn(tmp+l,room.server.room_topic.ToCStr() ,CAPTION_SIZE-l);
	SetWindowText(tmp);

}



// ----------------------------------------------------------------------------

// TSonorkChatInitWin

// ----------------------------------------------------------------------------


TSonorkChatInitWin::TSonorkChatInitWin(TSonorkExtUserData*UD)
	:TSonorkTaskWin(NULL
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|IDD_CHATINIT
		,SONORK_WIN_SF_NO_WIN_PARENT)
{
	context_user=UD;
	SetEventMask(UD!=NULL
		?SONORK_APP_EM_MSG_CONSUMER|SONORK_APP_EM_USER_LIST_AWARE
		:SONORK_APP_EM_MSG_CONSUMER);
};

// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkChatInitWin::ServiceCallback(
			  SONORK_DWORD2&		handler_tag
			, SONORK_APP_SERVICE_EVENT	event_id
			, SONORK_DWORD2*		//event_tag
			, TSonorkAppServiceEvent*	E)
{
	TSonorkChatServerData	*cr_server;

	TSonorkUserServer	*userver;

	TSonorkChatInitWin*_this = (TSonorkChatInitWin*)handler_tag.v[0];

	SONORK_RESULT	result;

	switch( event_id )

	{


	case SONORK_APP_SERVICE_EVENT_GET_NAME:

		E->get_name.str->Set("Initializator");
		return 1;


	case SONORK_APP_SERVICE_EVENT_QUERY_AKN:

		if( E->query_akn.UserParam() == SONORK_SERVICE_TYPE_SONORK_CHAT
		||  E->query_akn.UserParam() == SONORK_SERVICE_TYPE_NONE)
		{
			SONORK_MEM_NEW(cr_server=new TSonorkChatServerData);
			if( E->query_akn.UserParam() == SONORK_SERVICE_TYPE_SONORK_CHAT )
			{
				// Remote replied in native format
				result=cr_server->CODEC_ReadMemNoSize(
					 E->query_akn.Data()
					,E->query_akn.DataSize());
			}
			else
			{
				// Remote replied in generic format
				SONORK_MEM_NEW(userver=new TSonorkUserServer);
				result=userver->CODEC_ReadMemNoSize(
					 E->query_akn.Data()
					,E->query_akn.DataSize());
				if( result == SONORK_RESULT_OK )
					cr_server->LoadFrom( userver );
				SONORK_MEM_DELETE( userver );
			}
			if( result == SONORK_RESULT_OK)
			{
				_this->CLIENT_AddServer(_this->context_user->userId,cr_server);
			}
			else
			{
				SONORK_MEM_DELETE(cr_server);
			}
		}

		break;

	case SONORK_APP_SERVICE_EVENT_QUERY_END:

		if( _this->client.query_id == E->query_end.QueryId() )

			_this->CLIENT_EndQuery();

		break;

	}

	return 0;

}


void
 TSonorkChatInitWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if(P->Function()==SONORK_FUNCTION_LIST_USERVERS)
	if(P->SubFunction()==0)
	{
		TSonorkUserServer*		userver;
		TSonorkChatServerData*		chat_server;
		TSonorkUserServerQueue		queue;
		if(P->D_ListUserServers_A(P_size,queue))
		{
			while((userver=queue.RemoveFirst())!=NULL)
			{
				SONORK_MEM_NEW(chat_server=new TSonorkChatServerData);
				chat_server->LoadFrom(userver);
				CLIENT_AddServer(userver->UserId() , chat_server);
				SONORK_MEM_DELETE(userver);
			}

		}

	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&, const TSonorkError*)
{
	CLIENT_EndQuery();
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatInitWin::OnCreate()
{
	RECT	rect;
	SIZE	sz;
	UINT	height;
	UINT	cx_border;
	int	i;
	SONORK_DWORD2	sii_tag;
	TSonorkServiceInstanceInfo	sii;
	union {
		char 		str[128];
		TC_ITEM 	tc_item;
		LV_COLUMN 	lv_col;
		LV_ITEM 	lv_item;
		const TSonorkLangCodeRecord	*REC;
	}D;
	static TSonorkWinGlsEntry gls_server[]=
	{
		{IDL_CRSERVER_ROOM	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_NAME	}
	,	{IDL_CRSERVER_TOPIC	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_TITLE	}
	,	{IDL_CRSERVER_MAX_USERS	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_MAXUSR	}
	,	{IDG_CRSERVER_OPTS	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_OPTS	}
	,	{IDC_CRSERVER_INVITE    //|SONORK_WIN_CTF_BOLD
		,	GLS_CR_INVUSR0	}
	,	{IDC_CRSERVER_PRIVATE	//|SONORK_WIN_CTF_BOLD
		,	GLS_CR_PRIV	}
	,	{IDC_CRSERVER_PUBLIC
		,	GLS_CR_PUB	}
	,	{IDL_CRSERVER_INVHLP
		,	GLS_CR_NSVR	}
	,	{0
		,	GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_client[]=
	{
		{IDC_CRCLIENT_SEARCH
		, GLS_OP_SEARCH	}
	,	{IDG_CRCLIENT_OPTS	|SONORK_WIN_CTF_BOLD
		, GLS_LB_OPTS	}
	,	{IDL_CRCLIENT_TEXT
		, GLS_LB_TXT	}
	,	{IDC_CRCLIENT_UQUERY
		, GLS_CR_SCHUSR0	}
	,	{IDC_CRCLIENT_GQUERY
		, GLS_CR_SCHGLB		}
	,	{0
		, GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_table[]=
	{
		{-1, GLS_CR_NAME}
	,	{0,GLS_NULL}
	};
	
	static TSonorkListViewColumn
		client_columns[]=
	{
		{GLS_LB_SRKID	, 70}
	,	{GLS_LB_NAME	, 80}
	,	{GLS_LB_TITLE	, 500}
	};

	static GLS_INDEX page_title[CR_INIT_PAGES]=
	{
		GLS_CR_CMODE
	,	GLS_CR_SMODE
	};

	static UINT page_dlg_id[CR_INIT_PAGES]=
	{
		IDD_CRCLIENT
	,	IDD_CRSERVER
	};

	// Set UPDATING flag to prevent our handlers from processing
	// events caused by the loading of the forms
	SetWinUsrFlag(SONORK_WIN_F_UPDATING);

	SetCaptionIcon(  SKIN_HICON_CHAT );
	query_mode=CR_QUERY_NONE;

	// ----------------------------------------------------
	// STATUS BAR

	status_hwnd=GetDlgItem(IDC_CHATINIT_STATUS);

	// ----------------------------------------------------
	// TAB CONTROLS

	tab.hwnd=GetDlgItem(IDC_CHATINIT_TAB);
	//TabCtrl_SetImageList(tab_ctrl.hwnd,GuSkin_Icons());

	D.tc_item.mask=TCIF_TEXT|TCIF_IMAGE;

	D.tc_item.lParam=0;
	D.tc_item.iImage=-1;
	for(i=0;i<CR_INIT_PAGES;i++)
	{
		D.tc_item.pszText=(char*)SonorkApp.LangString(page_title[i]);
		TabCtrl_InsertItem(tab.hwnd,i,&D.tc_item);
		tab.win[i] = new TSonorkChildDialogWin(this
			,page_dlg_id[i]
			,SONORK_WIN_SF_NO_CLOSE);
	}
#define MARGIN	2

	// ----------------------------------------------------
	// TAB SIZE & DIALOGS POSITIONING

	TabCtrl_GetItemRect(tab.hwnd,0,&rect);
	height = (rect.bottom  - rect.top) + 1;
	::GetWindowRect(tab.hwnd,&rect);
	ScreenToClient(&rect);

	cx_border=GetSystemMetrics(SM_CXBORDER);
	sz.cx=cx_border + MARGIN;
	sz.cy=GetSystemMetrics(SM_CYBORDER) + MARGIN;
	rect.left 	+= sz.cx;
	rect.top  	+= height + sz.cy + MARGIN;
	rect.right	-= sz.cx;
	rect.bottom	-= sz.cy;

	for(i=0;i<CR_INIT_PAGES;i++)
		TSonorkWin::CenterWin(tab.win[i],rect,SONORK_CENTER_WIN_F_CREATE);

	// ----------------------------------------------------
	// LANGUAGE LABELS AND FONTS SETTINGS

	LoadLangEntries(gls_table,true);
	tab.win[CR_INIT_CLIENT]->LoadLangEntries(gls_client,false);
	tab.win[CR_INIT_SERVER]->LoadLangEntries(gls_server,false);



	// ----------------------------------------------------
	// CLIENT

	client.query_id	= 0;
	client.view.SetHandle( tab.win[CR_INIT_CLIENT]->GetDlgItem( IDC_CRCLIENT_ROOMS )
		, true
		, true);
	client.view.AddColumns(3,client_columns);


	// ----------------------------------------------------
	// SERVER
	
	GetDefaultRoomName(D.str);
	server.room = tab.win[CR_INIT_SERVER]->GetDlgItem( IDC_CRSERVER_ROOM );
	::SetWindowText(server.room,D.str);

	// Clear UPDATING flag that prevents our handlers from
	// processing events caused by the loading of the forms
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);


	// ----------------------------------------------------
	// LOADING

	if(context_user!=NULL)
	{
		SonorkApp.LangSprintf(D.str
			,GLS_CR_INVUSR1
			,context_user->display_alias.CStr());
		tab.win[CR_INIT_SERVER]->SetCtrlText(IDC_CRSERVER_INVITE,D.str);

		SonorkApp.LangSprintf(D.str
			,GLS_CR_SCHUSR1
			,context_user->display_alias.CStr());
		tab.win[CR_INIT_CLIENT]->SetCtrlText(IDC_CRCLIENT_UQUERY,D.str);
	}
	tab.win[CR_INIT_SERVER]->SetCtrlChecked(IDC_CRSERVER_PUBLIC,true);

	::SetWindowPos(tab.hwnd
		,HWND_BOTTOM
		,0,0,0,0
		,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);


	// ----------------------------------------------------
	// HOOK INTO APP SERVICE SYSTEM
	sii.SetInstanceInfo(
			  SONORK_SERVICE_ID_SONORK_CHAT
			, SONORK_SERVICE_TYPE_SONORK_CHAT
			, 0	// hflags
			, 0	// instance: Don't know yet, set to 0
			, SONORK_CHAT_VERSION_NUMBER);


	sii_tag.v[0]=(DWORD)this;
	sii_tag.v[1]=0;
	// Register as CLIENT: We won't be providing any services,
	// we only need to query other servers.
	if(SonorkApp.Service_Register(
		  SONORK_APP_SERVICE_TYPE_CLIENT
		, &sii
		, SKIN_ICON_CHAT
		, ServiceCallback
		, &sii_tag)!=SONORK_RESULT_OK)
			return false;

	service_instance = sii.ServiceInstance();

	return true;

}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnAfterCreate()
{
	SetPage( CR_INIT_CLIENT , true );
	TSonorkWin::SetStatus(status_hwnd,GLS_CR_INVHLP,SKIN_HICON_INFO);
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnBeforeDestroy()
{
	SonorkApp.Service_Unregister(service_instance,true);
	CLIENT_ClearServers();
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::SetPage(CR_INIT_PAGE pg , bool manual_set)
{
	if( tab.page != pg || manual_set )
	{
		tab.page = pg;
		for(int i=0;i<CR_INIT_PAGES;i++)
			tab.win[i]->ShowWindow(i==tab.page?SW_SHOW:SW_HIDE);
		if( manual_set )
			TabCtrl_SetCurSel(tab.hwnd , tab.page );
		SetCtrlText(IDOK
		 , tab.page == CR_INIT_CLIENT
		 ? GLS_OP_JOIN
		 : GLS_OP_CREATE);
		if( tab.page == CR_INIT_SERVER )
		{
			 SetCtrlEnabled(IDOK,SonorkApp.UTS_MayActAsServer() );
		}
		else
		{
			SetCtrlEnabled(IDOK,true);
		}

		// update_checkboxes is manually set
		UpdateInterface(manual_set);
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::UpdateInterface(bool user_update )
{
	BOOL	user_available;
	BOOL	may_query;

	ClearWinUsrFlag(CR_INIT_USER_AVAILABLE);
	user_available=false;
	if(context_user != NULL )
	{
		if(context_user->IsOnline())
		{
			if( user_update )
			{
				SonorkApp.UTS_ConnectByUserData( context_user , false );
			}
			SetWinUsrFlag(CR_INIT_USER_AVAILABLE);
			user_available=true;
		}
	}

	if( tab.page == CR_INIT_CLIENT )
	{
		may_query=query_mode==CR_QUERY_NONE;
		tab.win[CR_INIT_CLIENT]->SetCtrlEnabled(IDC_CRCLIENT_UQUERY
			,user_available&&may_query);
		tab.win[CR_INIT_CLIENT]->SetCtrlEnabled(IDC_CRCLIENT_GQUERY
			,may_query);
		tab.win[CR_INIT_CLIENT]->SetCtrlEnabled(IDC_CRCLIENT_SEARCH
			,may_query);
		tab.win[CR_INIT_CLIENT]->SetCtrlEnabled(IDC_CRCLIENT_TEXT
			,may_query);
		tab.win[CR_INIT_CLIENT]->SetCtrlEnabled(IDL_CRCLIENT_TEXT
			,may_query);
	}
	else
	{
		tab.win[CR_INIT_SERVER]->SetCtrlEnabled(IDC_CRSERVER_INVITE,user_available);
	}
	if( user_update )
	{
		SERVER_UpdateCheckboxes();
		tab.win[CR_INIT_SERVER]->SetCtrlChecked(IDC_CRSERVER_INVITE,user_available);
		tab.win[CR_INIT_CLIENT]->SetCtrlChecked(IDC_CRCLIENT_UQUERY,user_available);
		tab.win[CR_INIT_CLIENT]->SetCtrlChecked(IDC_CRCLIENT_GQUERY,!user_available);

		if( user_available )
			CLIENT_StartQuery();
		else
		if( query_mode == CR_QUERY_USER )
			CLIENT_EndQuery();

	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::SERVER_UpdateCheckboxes()
{
	tab.win[CR_INIT_SERVER]->SetCtrlEnabled(IDC_CRSERVER_PRIVATE,!tab.win[CR_INIT_SERVER]->GetCtrlChecked(IDC_CRSERVER_PUBLIC));
	tab.win[CR_INIT_SERVER]->SetCtrlEnabled(IDC_CRSERVER_PUBLIC,!tab.win[CR_INIT_SERVER]->GetCtrlChecked(IDC_CRSERVER_PRIVATE));
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnTimer(UINT id)
{
	if( query_mode==CR_QUERY_USER && id == SONORK_WIN_AUX_TIMER_ID )
	{
		client.query_msecs+=CR_QUERY_TIMER_MSECS;
		if(client.query_msecs > SONORK_MSECS_FOR_QUERY_REPLY )
			CLIENT_EndQuery();
	}
	else
		KillTimer(id);
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::CLIENT_StartQuery()
{
	if( query_mode!=CR_QUERY_NONE )
		return;

	if( tab.win[CR_INIT_CLIENT]->GetCtrlChecked(IDC_CRCLIENT_UQUERY) )
	{
		if(!TestWinUsrFlag(CR_INIT_USER_AVAILABLE) )
			return;
	}

	CLIENT_ClearServers();


	if( tab.win[CR_INIT_CLIENT]->GetCtrlChecked(IDC_CRCLIENT_UQUERY) )
	{
		TSonorkCtrlMsg		msg;
		TSonorkServiceQuery	handle;
		
		context_user->GetUserHandle(&handle);

		client.query_msecs	= 0;
		if( context_user->SidVersion().UsesOldCtrlMsg())
		{
			client.query_id 	= SonorkApp.GenTrackingNo(context_user->userId);
			msg.ClearOldParams();
			msg.cmdFlags 		= SONORK_OLD_CTRLMSG_CMD_QUERY_REQUEST;
			msg.sysFlags		= 0;
			msg.SetOldServiceId(SONORK_SERVICE_ID_SONORK_CHAT);
			msg.SetOldQueryId( client.query_id );
			msg.body.o.param[ 0 ]	= SONORK_CHAT_OLD_CTRLMSG_PARAM0_LIST_ROOMS;

			if( SonorkApp.SendCtrlMsg(
				&handle
				,&msg
				,NULL
				,0) == SONORK_RESULT_OK)
			{
				if(SetAuxTimer( CR_QUERY_TIMER_MSECS ))
				{
					query_mode=CR_QUERY_USER;
				}
				else
					MessageBox("SetAuxTimer failed",szSONORK,MB_OK);
			}
		}
		else
		{
			TSonorkCtrlGetServerDataReq req;
			req.sizeof_this=sizeof(req);
			req.sonork_version.Set(SonorkApp.Version());
			req.sender_version=SONORK_CHAT_VERSION_NUMBER;

			handle.LoadTarget(SONORK_SERVICE_ID_SONORK_CHAT
				, SONORK_SERVICE_SERVER_INSTANCE(0) );

			if( SonorkApp.Service_StartQuery(
				  service_instance
				, &handle
				, SONORK_CTRLMSG_CMD_GET_SERVER_DATA
				, SONORK_SERVICE_TYPE_SONORK_CHAT// usr_param
				, 0	// usr_flags
				, SONORK_MSECS_FOR_QUERY_REPLY
				, &req
				, sizeof(req)
				, NULL	// query_tag
				)== SONORK_RESULT_OK)
			{
				client.query_id = handle.QueryId();
				query_mode	= CR_QUERY_USER;
			}
		}
	}
	else
	{
		UINT P_size,A_size;
		TSonorkDataPacket *	P;
		TSonorkUserServer   userver;
		TSonorkError		ERR;

		userver.Clear();

		tab.win[CR_INIT_CLIENT]->GetCtrlText(IDC_CRCLIENT_TEXT,userver.name);
		userver.SetInstanceInfo(
			  SONORK_SERVICE_ID_SONORK_CHAT
			, SONORK_SERVICE_TYPE_SONORK_CHAT
			, 0	// hflags
			, 0	// instance
			, 0	// version
		);
		userver.header.SetState(SONORK_SERVICE_STATE_READY);
		A_size=userver.CODEC_Size()+64;
		P=SONORK_AllocDataPacket(A_size);
		P_size=P->E_ListUserServers_R(A_size,userver,512);
		StartSonorkTask(ERR
				, P
				, P_size
				, 0
				, GLS_TK_GETINFO
				, NULL);
		SONORK_FreeDataPacket( P );
		if(ERR.Result()==SONORK_RESULT_OK)
		{
			query_mode=CR_QUERY_GLOBAL;
		}
	}

	if( query_mode != CR_QUERY_NONE )
	{
	 	TSonorkWin::SetStatus(status_hwnd
		,GLS_MS_RFRESHING
		,SKIN_HICON_BUSY);
		UpdateInterface(false);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::CLIENT_EndQuery()
{
	if( query_mode == CR_QUERY_NONE )
		return;
	if( query_mode == CR_QUERY_USER )
	{
		KillAuxTimer();
		client.query_id=0;
	}
	query_mode=CR_QUERY_NONE;
	TSonorkWin::SetStatus_None(status_hwnd);
	UpdateInterface( false );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::CLIENT_AddServer(const TSonorkId& user_id,TSonorkChatServerData*SD)
{
	char		tmp[48];
	int		index;

	index = client.view.AddItem(
			   user_id.GetStr(tmp)
			,  SKIN_ICON_CHAT
			,  (LPARAM)SD);

	client.view.SetItemText(index,1,SD->room_name.CStr());
	client.view.SetItemText(index,2,SD->room_topic.CStr());

}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::CLIENT_ClearServers()
{
	client.view.DelAllItems();
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnAppEvent_UserSid(TSonorkAppEventLocalUserSid* userSid)
{
	if( context_user != NULL )
		if(userSid->userData->userId == context_user->userId )
			UpdateInterface( true );
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::OnAppEvent_UserDel(const TSonorkId* userId)
{
	if( context_user != NULL )
		if(*userId == context_user->userId )
			Destroy(IDCANCEL);
}

// ----------------------------------------------------------------------------
// OnAppEvent_OldCtrlMsg
// Processes old-style CTRL messages used before V1.5
//  Needed to list rooms in an old Sonork versions.
// ----------------------------------------------------------------------------

bool
 TSonorkChatInitWin::OnAppEvent_OldCtrlMsg(TSonorkAppEventOldCtrlMsg* E)
{
	const TSonorkCtrlMsg *	reqMsg;
	const BYTE*		data;
	DWORD			data_size;
	TSonorkChatServerData* 	SD;

	reqMsg	= E->msg;

	if( reqMsg->body.o.param[ 0 ] != SONORK_CHAT_OLD_CTRLMSG_PARAM0_LIST_ROOMS )
		return false;

	if( client.query_id != reqMsg->OldQueryId()
	||  client.query_id == 0)
		return false;

	client.query_msecs = 0;

	if( reqMsg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_QUERY_RESULT )
	{
		data		= E->data;
		data_size 	= E->data_size;
		SONORK_MEM_NEW(SD=new TSonorkChatServerData);
		if( SD->CODEC_ReadMem(data,data_size) ==SONORK_RESULT_OK )
		{
			CLIENT_AddServer(context_user->userId,SD);
			return true;
		}
		SONORK_MEM_DELETE(SD);
	}
	else
	if( reqMsg->OldCmd()	== SONORK_OLD_CTRLMSG_CMD_QUERY_END )
	{
		CLIENT_EndQuery();
	}
	return true;
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatInitWin::OnAppEvent(UINT event, UINT param,void*data)
{
	switch( event )
	{
		case SONORK_APP_EVENT_USER_SID:
			if( param == SONORK_APP_EVENT_USER_SID_LOCAL )
			{
				OnAppEvent_UserSid((TSonorkAppEventLocalUserSid*)data );
			}
			return true;

		case SONORK_APP_EVENT_DEL_USER:
			OnAppEvent_UserDel((TSonorkId*)data);
			return true;

		case SONORK_APP_EVENT_OLD_CTRL_MSG:
			if(param == SONORK_SERVICE_ID_SONORK_CHAT )
				return OnAppEvent_OldCtrlMsg((TSonorkAppEventOldCtrlMsg*)data);
			break;

	}
	return false;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatInitWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
#define	CN	((TSonorkChildDialogNotify*)lParam)
#define S 	(CN->N->draw)
	if( wParam == SONORK_WIN_POKE_CHILD_NOTIFY)
	{
		return OnChildDialogNotify(CN);
	}
	else
	if( wParam == SONORK_WIN_POKE_CHILD_DRAW_ITEM )
	{
		if( S.CtlID == IDI_CRSERVER_ALERT )
		{
			sonork_skin.DrawSign(S.hDC
				,SKIN_SIGN_TOOLS
				,S.rcItem.left
				,S.rcItem.top);
			return true;
		}
		return false;
#undef S
#undef CN
	}
	else
	if( wParam == CR_INIT_POKE_START_CLIENT)
		if(CmdStartClient())
			Destroy(IDOK);
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkChatInitWin::GetDefaultRoomName(char *name)
{
	wsprintf(name
		,"%.20s-%02u"
		,SonorkApp.ProfileUser().alias.CStr()
		,SonorkApp.GetSerialNo()
		);
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatInitWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if( N->hdr.hwndFrom == tab.hwnd && N->hdr.code == TCN_SELCHANGE )
	{
		SetPage((CR_INIT_PAGE)TabCtrl_GetCurSel(tab.hwnd) , false);
	}
	return 0;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkChatInitWin::OnChildDialogNotify(TSonorkChildDialogNotify*childN)
{
	TSonorkWinNotify*	N;
	TSonorkChatServerData*SD;
	N = childN->N;

	if( childN->dialog_id == IDD_CRCLIENT )
	{
		if( N->hdr.hwndFrom == client.view.Handle() )
		{
			if( N->hdr.code == LVN_DELETEITEM )
			{
				SD = (TSonorkChatServerData*)(N->lview.lParam);
				if(SD)SONORK_MEM_DELETE(SD);
			}
			else
			if( N->hdr.code == NM_DBLCLK)
			{
				PostPoke(CR_INIT_POKE_START_CLIENT,0);
			}
		}
		else
		if( N->hdr.code == BN_CLICKED )
		{
			if(N->hdr.idFrom==IDC_CRCLIENT_SEARCH )
			{
				CLIENT_StartQuery();
			}
		}

	}
	else
	if( childN->dialog_id == IDD_CRSERVER )
	{
		if( N->hdr.code == BN_CLICKED )
		{
			if(N->hdr.idFrom==IDC_CRSERVER_PUBLIC|| N->hdr.idFrom==IDC_CRSERVER_PRIVATE)
				SERVER_UpdateCheckboxes();
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatInitWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		if( id == IDOK )
		{
			if( tab.page == CR_INIT_CLIENT )
			{
				if(!CmdStartClient())
					return true;
			}
			else
			{
				if(!CmdStartServer())
					return true;
			}
		}
		Destroy(IDOK);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

bool
 TSonorkChatInitWin::CmdStartServer()
{

	TSonorkChatWin*		W;

	char 			name[CR_MAX_ROOM_NAME_SIZE];

	TSonorkShortString	topic;

	DWORD			max_users;

	if( !SonorkApp.UTS_MayActAsServer() )
		return false;
	tab.win[CR_INIT_SERVER]->GetCtrlText(IDC_CRSERVER_ROOM
		,name
		,sizeof(name));
	if(*name==0)
		GetDefaultRoomName(name);
	tab.win[CR_INIT_SERVER]->GetCtrlText(IDC_CRSERVER_TOPIC,topic);

	max_users=tab.win[CR_INIT_SERVER]->GetCtrlUint(IDC_CRSERVER_MAX_USERS);
	if(max_users<4)
		max_users=4;
	else
	if(max_users>512)
		max_users=512;
	W=TSonorkChatWin::CreateServer(name,topic.CStr(),max_users);
	if(!W)
		return false;

	if(tab.win[CR_INIT_SERVER]->GetCtrlChecked(IDC_CRSERVER_PUBLIC))
		W->SetPublic( true );
	else
	if(tab.win[CR_INIT_SERVER]->GetCtrlChecked(IDC_CRSERVER_PRIVATE))
		W->SetPrivate( true );

	if(TestWinUsrFlag(CR_INIT_USER_AVAILABLE)
	 && tab.win[CR_INIT_SERVER]->GetCtrlChecked(IDC_CRSERVER_INVITE))
		W->Invite(context_user, "");
	W->UpdateUI();
	return true;
}



// -----------------------------------------------------------------------

bool

 TSonorkChatInitWin::CmdStartClient()
{

	TSonorkChatServerData*	SD;

	SD = (TSonorkChatServerData*)client.view.GetSelectedItem();
	if( SD != NULL )
	{
		if(TSonorkChatWin::CreateClient(SD))
			return true;
	}
	return false;
}







// ----------------------------------------------------------------------------
// CODEC ATOMS

// ----------------------------------------------------------------------------


// --------------------
// TSonorkChatNickData


void
 TSonorkChatNickData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	nick.Clear();
	user_data.Clear();
}

void
 TSonorkChatNickData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&nick);
	CODEC.Write(&user_data);
}

void
 TSonorkChatNickData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	aux&=0xfff;
	CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Skip( aux  - sizeof(header));
	CODEC.Read(&nick);
	CODEC.Read(&user_data);
}
DWORD
 TSonorkChatNickData::CODEC_DataSize()	const
{

	return sizeof(DWORD)
			+ 	sizeof(header)
			+	::CODEC_Size(&nick)
			+	::CODEC_Size(&user_data);
}

// --------------------
// TSonorkChatTextData

void
 TSonorkChatTextData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	text.Clear();
	data.Clear();
}

void
 TSonorkChatTextData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&text);
	CODEC.Write(&data);
}

void
 TSonorkChatTextData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW(&aux);
	aux&=0xfff;
	if( aux < sizeof(header))
	{
		CODEC.ReadDWN((DWORD*)&header,SIZE_IN_DWORDS(aux));
	}
	else
	{
		CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
		CODEC.Skip( aux - sizeof(header));
	}

	CODEC.Read(&text);
	CODEC.Read(&data);
}

DWORD
 TSonorkChatTextData::CODEC_DataSize()	const
{
	return sizeof(DWORD)
		+ sizeof(header)
		+ ::CODEC_Size(&text)
		+ ::CODEC_Size(&data);
}

// --------------------
// TSonorkChatServerData

// ----------------------------------------------------------------------------

void
 TSonorkChatServerData::LoadFrom(const TSonorkUserServer* userver)
{
	header.physAddr.Set(userver->physAddr);
	room_name.Set(userver->name);
	room_topic.Set(userver->text);
	data.Set(userver->data);
	header.descriptor.serviceId=userver->ServiceId();
	header.descriptor.instance =userver->UtsInstance();
	header.descriptor.flags	=userver->UtsFlags();
	header.version		=userver->ServiceVersionNumber();
	header.nickId		=userver->Param(0);
	header.userCount	=userver->Param(1);
	header.flags		=userver->Param(2);
}

void
 TSonorkChatServerData::SaveInto(TSonorkUserServer* userver)
{
	userver->CODEC_Clear();
	userver->SetInstanceInfo(
		  SONORK_SERVICE_ID_SONORK_CHAT
		, SONORK_SERVICE_TYPE_SONORK_CHAT
		, 0	// hflags
		, header.descriptor.instance
		, SONORK_CHAT_VERSION_NUMBER	// version
	);
	userver->SetState(SONORK_SERVICE_STATE_READY);
	userver->name.Set(room_name);
	userver->text.Set(room_topic);
	userver->data.Set(data);
	userver->SetUtsFlags(header.descriptor.flags);
	userver->SetParam(0,header.nickId);
	userver->SetParam(1,header.userCount);
	userver->SetParam(2,header.flags);
	userver->physAddr.Set(header.physAddr);
}

void

 TSonorkChatServerData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	room_name.Clear();
	room_topic.Clear();
	data.Clear();
}

void
 TSonorkChatServerData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header));
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&room_name);
	CODEC.Write(&room_topic);
	CODEC.Write(&data);
}

void
 TSonorkChatServerData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD aux;
	CODEC.ReadDW( &aux );
	aux&=0xfff;
	CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Skip( aux - sizeof(header));
	CODEC.Read(&room_name);
	CODEC.Read(&room_topic);
	CODEC.Read(&data);
}

DWORD
 TSonorkChatServerData::CODEC_DataSize()	const
{
	return sizeof(DWORD)
		+ sizeof(header)
		+ ::CODEC_Size(&room_name)
		+ ::CODEC_Size(&room_topic)
		+ ::CODEC_Size(&data);
};


// --------------------
// TSonorkChatInviteData

enum SONORK_CHAT_INVITE_CODEC_FLAGS
{
	SONORK_CHAT_INVITE_CODEC_F_HAS_TEXT	= 0x10000
};

void

 TSonorkChatInviteData::CODEC_Clear()
{
	SONORK_ZeroMem(&header,sizeof(header));
	nick_data.CODEC_Clear();
	server_data.CODEC_Clear();
	text.Clear();
}

void
 TSonorkChatInviteData::CODEC_WriteDataMem(TSonorkCodecWriter& CODEC) const
{
	CODEC.WriteDW(sizeof(header)|SONORK_CHAT_INVITE_CODEC_F_HAS_TEXT);
	CODEC.WriteDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Write(&nick_data);
	CODEC.Write(&server_data);
	CODEC.Write(&text);
}
void
 TSonorkChatInviteData::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD	flags,sz;
	CODEC.ReadDW(&flags);
	sz=flags&0xfff;
	CODEC.ReadDWN((DWORD*)&header,SIZEOF_IN_DWORDS(header));
	CODEC.Skip( sz - sizeof(header) );
	CODEC.Read(&nick_data);
	CODEC.Read(&server_data);
	if( flags & SONORK_CHAT_INVITE_CODEC_F_HAS_TEXT )
		CODEC.Read(&text);
}

DWORD
 TSonorkChatInviteData::CODEC_DataSize()	const
{
	return sizeof(DWORD)
		+  sizeof(HEADER)
		+ ::CODEC_Size(&nick_data)
		+ ::CODEC_Size(&server_data)
		+ ::CODEC_Size(&text);
}






// ----------------------------------------------------------------------------

// TSonorkChatSetWin

// ----------------------------------------------------------------------------


TSonorkChatSetWin::TSonorkChatSetWin(TSonorkWin*parent

		,TSonorkChatServerData* p_server
		,DWORD p_nick_flags
		,DWORD*p_room_color)
	:TSonorkWin(parent
	, SONORK_WIN_CLASS_NORMAL
	| SONORK_WIN_TYPE_NONE
	| SONORK_WIN_DIALOG
	| IDD_CHATSET
	, 0
	)
{
	server 		= p_server;
	nick_flags      = p_nick_flags;
	room_color	= p_room_color;
}
bool
 TSonorkChatSetWin::OnCreate()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDL_CHATSET_NAME	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_NAME	}
	,	{IDL_CHATSET_TOPIC	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_TITLE	}
	,	{IDG_CHATSET_OPTS	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_OPTS	}
	,	{IDC_CHATSET_PRIVATE
		,	GLS_CR_PRIV	}
	,	{IDC_CHATSET_PUBLIC
		,	GLS_CR_PUB	}
	,	{IDC_CHATSET_COLORS
		,	GLS_LB_COLOR	}
	,	{IDC_CHATSET_SOUND
		,	GLS_OP_SOUND	}
	,	{-1
		,	GLS_CR_NAME	}
	,	{0
		,	GLS_NULL	}
	};
	is_server	=nick_flags&SONORK_CHAT_NICK_F_SERVER;
	is_operator     =(nick_flags&SONORK_CHAT_NICK_F_OPERATOR)||is_server;
	LoadLangEntries(gls_table,true);
	SetCtrlText(IDC_CHATSET_NAME,server->room_name.CStr());
	SetCtrlText(IDC_CHATSET_TOPIC,server->room_topic.CStr());
	SetCtrlEnabled(IDC_CHATSET_NAME,is_operator);
	SetCtrlEnabled(IDC_CHATSET_TOPIC,is_operator);
	SetCtrlChecked(IDC_CHATSET_PUBLIC,server->ServerFlags()&SONORK_CHAT_SERVER_F_PUBLIC);
	SetCtrlChecked(IDC_CHATSET_PRIVATE,server->ServerFlags()&SONORK_CHAT_SERVER_F_PRIVATE);
	SetCtrlChecked(IDC_CHATSET_SOUND,Parent()->TestWinUsrFlag(CR_F_SOUND));
	UpdateCheckboxes();
	return true;
}

bool
 TSonorkChatSetWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code==BN_CLICKED )
	{
		if(id == IDOK )
		{
			if(is_operator)
			{
				GetCtrlText( IDC_CHATSET_TOPIC, server->room_topic );
				server->room_topic.CutAt(CR_MAX_ROOM_TOPIC_SIZE);
				GetCtrlText( IDC_CHATSET_NAME , server->room_name );
				server->room_name.CutAt(CR_MAX_ROOM_NAME_SIZE);
				if(is_server)
				{
					server->header.flags&=~(SONORK_CHAT_SERVER_F_PUBLIC
							|SONORK_CHAT_SERVER_F_PRIVATE);
					if(GetCtrlChecked(IDC_CHATSET_PUBLIC))
						server->header.flags|=SONORK_CHAT_SERVER_F_PUBLIC;
					else
					if(GetCtrlChecked(IDC_CHATSET_PRIVATE))
						server->header.flags|=SONORK_CHAT_SERVER_F_PRIVATE;
				}
			}
			Destroy(IDOK);
		}
		else
		if( id==IDC_CHATSET_PUBLIC || id==IDC_CHATSET_PRIVATE )
			UpdateCheckboxes();
		else
		if( id==IDC_CHATSET_COLORS )
			CmdChooseColor();
		else
		if( id==IDC_CHATSET_SOUND )
		{
			code = GetCtrlChecked(IDC_CHATSET_SOUND);
			Parent()->ChangeWinUsrFlag(CR_F_SOUND,code);
			SonorkApp.ProfileCtrlFlags().SetClear(SONORK_PCF_CHAT_SOUND,code);
			if(code)
				SonorkApp.AppSound( SONORK_APP_SOUND_MSG_LO );

		}
		else
			return false;
		return true;
	}
	return false;
}
void

 TSonorkChatSetWin::UpdateCheckboxes()

{

	BOOL public_mode=GetCtrlChecked(IDC_CHATSET_PUBLIC);

	BOOL private_mode=GetCtrlChecked(IDC_CHATSET_PRIVATE);

	SetCtrlEnabled(IDC_CHATSET_PUBLIC,is_server&&!private_mode);

	SetCtrlEnabled(IDC_CHATSET_PRIVATE,is_server&&!public_mode);
}
void

 TSonorkChatSetWin::CmdChooseColor()

{

	CHOOSECOLOR CC;

	COLORREF	custom[16];
	CC.lStructSize	= sizeof(CC);
	CC.hwndOwner	= Handle();
	CC.hInstance	= NULL;
	CC.rgbResult	= *room_color;
	CC.lpCustColors	= custom;
	CC.Flags		= CC_RGBINIT;//CC_PREVENTFULLOPEN|
	if(ChooseColor(&CC))
	{
		*room_color = CC.rgbResult;
		Parent()->PostPoke(CR_POKE_COLOR_CHANGED,0);
	}

}


// ----------------------------------------------------------------------------
// Global handlers

// ----------------------------------------------------------------------------


void
 TSonorkChatWin::Init(TSonorkExtUserData*UD)
{
	TSonorkChatInitWin*W;
	W=new TSonorkChatInitWin(UD);
	W->Create();
}

// ----------------------------------------------------------------------------
// TSonorkChatWin::UserResponseWaitCallback

// Waits for the user to accept or deny an invitation

// It is passed as the callback function to a TSonorkWaitWin

// ----------------------------------------------------------------------------


void SONORK_CALLBACK
 TSonorkChatWin::UserResponseWaitCallback(TSonorkWaitWin*WW
		, LPARAM // cb_param
		, LPVOID cb_data
		, SONORK_WAIT_WIN_EVENT event
		, LPARAM  event_param)
{
	char		*tmp;
#define ID ((TSonorkChatInviteData*)cb_data)
	switch( event )
	{
		case SONORK_WAIT_WIN_EVENT_CREATE:
			SONORK_MEM_NEW( tmp = new char[ID->CODEC_Size() + 128] );
			sprintf(tmp
			, "%s: %s\r\n"
			  "%s: %s\r\n"
			  "%s: %s"
			  , SonorkApp.LangString(GLS_LB_NOTES)
			  , ID->text.ToCStr()
			  , SonorkApp.LangString(GLS_LB_NAME)
			  , ID->server_data.room_name.ToCStr()
			  , SonorkApp.LangString(GLS_LB_TITLE)
			  , ID->server_data.room_topic.ToCStr());
			WW->SetCtrlText( IDC_WAIT_INFO , tmp);
			SonorkApp.LangSprintf(tmp
				, GLS_CR_INVMSG
				, ID->nick_data.user_data.alias.CStr()
				, ID->nick_data.user_data.userId.v[0]
				, ID->nick_data.user_data.userId.v[1]);
			WW->SetCtrlText( IDC_WAIT_LABEL , tmp );
			WW->SetWindowText( GLS_OP_STARTAPP); 
			WW->SetCaptionIcon( SKIN_HICON_CHAT );

			SONORK_MEM_DELETE_ARRAY( tmp );
			*((DWORD*)event_param)=
				   SONORK_SECS_FOR_USER_RESPONSE
				  |SONORK_WAIT_WIN_F_NO_CHECKBOX
				  |SONORK_WAIT_WIN_F_IS_NOTICE;
			break;

		case SONORK_WAIT_WIN_EVENT_RESULT:
			if( event_param == SONORK_WAIT_WIN_F_RESULT_ACCEPT )
			{
				TSonorkChatWin::CreateClient(&ID->server_data);
			}
			break;

		case SONORK_WAIT_WIN_EVENT_DESTROY:
			SONORK_MEM_DELETE( ID );
			break;
	}
#undef ID
}



// ----------------------------------------------------------------------------

// TSonorkChatWin::OldCtrlMsgHandler
// Processes old-style CTRL messages used before V1.5
//  Needed to process invitations from old versions
// ----------------------------------------------------------------------------


DWORD

 TSonorkChatWin::OldCtrlMsgHandler(

		  const TSonorkUserHandle& 	handle

		, const TSonorkCtrlMsg*		reqMsg
		, const void*			data
		, DWORD	 			data_size)
{

	TSonorkChatInviteData	*ID;

	if( reqMsg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_EXEC_START )

	{
		if(reqMsg->body.o.param[0] == SONORK_CHAT_OLD_CTRLMSG_PARAM0_INVITE )
		{
			if( SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_INVITATIONS) )
				return 0;


			if( SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_PUBLIC_INVITATIONS) )

			{

				if(SonorkApp.UserList().Get( handle.userId ) == NULL )

					return 0;

			}


			SONORK_MEM_NEW(ID = new TSonorkChatInviteData);
			if(ID->CODEC_ReadMem( (const BYTE*)data , data_size ) == SONORK_RESULT_OK )
			{
				ID->nick_data.user_data.userId.Set( handle.userId );
				if(Sonork_StartWaitWin(
					  TSonorkChatWin::UserResponseWaitCallback
					, 0
					, ID))
				{
					return 0;
				}
			}
			SONORK_MEM_DELETE(ID);
		}
		return 0;
	}
	return SONORK_APP_OLD_CTRLMSG_RF_DEFAULT;
}



// ----------------------------------------------------------------------------

// TSonorkChatWin::LocatorServiceCallback
// Service callback for the chat locator:
//  Needed to process invitations
// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkChatWin::LocatorServiceCallback(
			 SONORK_DWORD2&			//handler_tag
			,SONORK_APP_SERVICE_EVENT	event_id
			,SONORK_DWORD2*			//event_tag

			,TSonorkAppServiceEvent*	E)

{
	TSonorkChatInviteData	*ID;


	switch(event_id)

	{

	case SONORK_APP_SERVICE_EVENT_GET_NAME:

		E->get_name.str->Set("Chat locator");
		return 1;

	case SONORK_APP_SERVICE_EVENT_POKE_DATA:

		if( E->poke_data.Cmd() != SONORK_CTRLMSG_CMD_START )

			break;


		if( SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_INVITATIONS) )

			break;


		if( SonorkApp.ProfileCtrlFlags().Test( SONORK_PCF_NO_PUBLIC_INVITATIONS) )

		{

			if(SonorkApp.UserList().Get( E->query_req.SenderUserId() ) == NULL )

				break;

		}


		SONORK_MEM_NEW(ID = new TSonorkChatInviteData);

		if(ID->CODEC_ReadMemNoSize(
			E->poke_data.Data(),E->poke_data.DataSize()) == SONORK_RESULT_OK )
		{

			ID->nick_data.user_data.userId.Set( E->query_req.SenderUserId() );
			if(Sonork_StartWaitWin(
				  TSonorkChatWin::UserResponseWaitCallback
				, 0
				, ID))
			{
				break;
			}
		}
		SONORK_MEM_DELETE(ID);
		break;


	}

	return 0;

}



