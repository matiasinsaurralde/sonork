#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srksysconwin.h"
#include "srkmainwin.h"
#include "srkusearchwin.h"
#include "srkgrpmsgwin.h"
#include "srkclipwin.h"
#include "srkremindwin.h"
#include "srkeappcfgwin.h"
#include "srkslidewin.h"
#include "srksnapshotwin.h"
#include "srkmyinfowin.h"
#include "srkmaininfowin.h"
#include "srkinputwin.h"

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





#define SONORK_UI_EVENT_TTL_INFO_SHORT		5000
#define SONORK_UI_EVENT_TTL_INFO_LONG		8000
#define SONORK_UI_EVENT_TTL_SLIDE		8000

#define MENU_ICON_X_L_OFFSET	2
#define MENU_ICON_X_R_OFFSET	4
#define MENU_ICON_X_PADDING	0
#define MENU_ICON_Y_SPACE	1
#define MENU_SEPARATOR_HEIGHT	8
#define SEPARATOR_COLOR		COLOR_GRAYTEXT


// ----------------------------------------------------------------------------

void
 static ConvertCmSeparatorToOwnerDraw(HMENU hmenu);


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::RebuildMenus()
{
	char			tmp[128];
	static TSonorkWinGlsEntry gls_main[]=
	{
	  {CM_CFGNETWORK
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_LB_NETCFG}
	, {CM_TELL_A_FRIEND
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_OP_TELLF}
	, {CM_CONNECT
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_OP_CONNECT	}
	, {CM_DISCONNECT
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_OP_DISCONNECT}
	, {CM_ADD_USER_CTX_GLOBAL
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_OP_ADDUSR	  }
	, {CM_REFRESH_USERS
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_OP_RFRUSRLST}
	, {CM_DBMAINT_USERS
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_DBMNT	  }
	, {CM_MY_INFO
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_LB_MYINFO}
	, {CM_PREFERENCES
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_OP_PREFS}
	, {CM_VISIBILITY
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	,GLS_UM_VISIBILITY}
	, {CM_QUIT
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_LB_QUIT}
	, {CM_APP_CLIPBOARD
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_LB_CLPBRD}
	, {CM_APP_EMAIL
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_EM_SND}
	, {CM_APP_CHAT
		|SONORK_WIN_CTF_OWNER_DRAW  	,GLS_CR_NAME}
	, {CM_APP_REMINDER
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_LB_RMINDLST}
	, {CM_APP_SYS_CONSOLE
		|SONORK_WIN_CTF_OWNER_DRAW	,GLS_LB_SYSCON}
	, {CM_MAIN_POPUP_FILE
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_LB_CX		}
	, {CM_MAIN_POPUP_USERS
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_LB_USRS}
	, {CM_ABOUT
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	, GLS_LB_HABOUT}
	, {0, GLS_NULL}
	};

	static TSonorkWinGlsEntry gls_user_popup[]=
	{
	  {CM_USER_MSG
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	, GLS_LB_MSGS}
	, {CM_USER_INFO
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	, GLS_LB_USRINFO}
	, {CM_DELETE
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS  	, GLS_OP_DELUSR}
	, {CM_AUTH_BUSY				, GLS_UA_BUSY}
	, {CM_AUTH_NBUSY			, GLS_UA_NBUSY}
	, {CM_AUTH_FRIENDLY			, GLS_UA_FRIENDLY}
	, {CM_AUTH_NFRIENDLY			, GLS_UA_NFRIENDLY}
	, {CM_AUTH_NCX				, GLS_UA_NCX}
	, {CM_AUTH_NADDR			, GLS_UA_NADDR}
	, {CM_AUTH_NEMAIL			, GLS_UA_NEMAIL}
	, {CM_AUTH_UTS_ENABLED			, GLS_OP_AUTO_UTS}

	, {CM_VISIBILITY_01			, GLS_VG_01	}
	, {CM_VISIBILITY_02			, GLS_VG_02	}
	, {CM_VISIBILITY_03			, GLS_VG_03	}
	, {CM_VISIBILITY_04			, GLS_VG_04	}
	, {CM_VISIBILITY_05			, GLS_VG_05	}
	, {CM_VISIBILITY_06			, GLS_VG_06	}
	, {CM_VISIBILITY_07			, GLS_VG_07	}
	, {CM_USER_CONTEXT_POPUP_APPS
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_LB_APPS }
	, {CM_USER_CONTEXT_POPUP_VISIB
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_UA_GRPS}
	, {CM_USER_CONTEXT_POPUP_AUTH
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_LB_AUTHS}
	, {CM_USER_CONTEXT_POPUP_MTPL
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_BYPOSITION	,GLS_LB_TMPLTS}
	, {0			    		, GLS_NULL}
	};

	static TSonorkWinGlsEntry gls_msgs_popup[]=
	{
	  {CM_MSGS_TOGGLE_SEL		, GLS_MW_TOGSEL}
	, {CM_MSGS_CLEAR_SEL		, GLS_MW_CLRSEL}
	, {CM_DELETE			, GLS_MW_DELSEL}
	, {CM_DELETE_PREVIOUS		, GLS_MW_DELPRV}
	, {CM_DELETE_ALL		, GLS_MW_DELALL}
	, {CM_MSGS_MARK_ALL_READ	, GLS_MW_ALLREAD}
	, {CM_MSGS_SEL_THREAD		, GLS_MW_THRSEL}
	, {CM_PROTECT			, GLS_MW_PROT}
	, {CM_CLIPBOARD_COPY		, GLS_OP_COPYWC}
	, {CM_MSGS_SINGLE_LINE_MODE	, GLS_MW_SLINE}
	, {CM_EXPORT			, GLS_OP_EXPORT }
	, {CM_MSGS_LOCATE_MAIN		, GLS_MW_LOCMAIN}
	, {0				, GLS_NULL}
	};

	static TSonorkWinGlsEntry gls_ugrp_popup[]=
	{
	  {CM_ADD_USER_CTX_GLOBAL
		|SONORK_WIN_CTF_OWNER_DRAW  , GLS_OP_ADDUSR}
	, {CM_ADD_GROUP
		|SONORK_WIN_CTF_OWNER_DRAW  , GLS_OP_ADDGRP}
	, {CM_RENAME
		|SONORK_WIN_CTF_OWNER_DRAW  , GLS_OP_RENAM}
	, {CM_DELETE
		|SONORK_WIN_CTF_OWNER_DRAW  , GLS_OP_DELGRP}
	, {CM_GROUP_MSG
		|SONORK_WIN_CTF_OWNER_DRAW  , GLS_LB_GRPMSG}
	, {0				, GLS_NULL}
	};
	static TSonorkWinGlsEntry gls_usel_popup[]=
	{
	  {CM_GROUP_MSG			, GLS_LB_GRPMSG}
	, {0				, GLS_NULL}
	};
	static TSonorkWinGlsEntry gls_eapp_popup[]=
	{
	  {CM_EAPP_CONFIG
		|SONORK_WIN_CTF_OWNER_DRAW
		|SONORK_WIN_CTF_ELLIPSIS	, GLS_OP_CFG}
	, {CM_EAPP_RELOAD
		|SONORK_WIN_CTF_OWNER_DRAW  	, GLS_OP_REFRESH}
	, {0						, GLS_NULL}
	};
	static TSonorkWinGlsEntry gls_tray_menu[]=
	{
	  {CM_QUIT				, GLS_LB_QUIT}
	, {CM_TRAY_POPUP_MODE|SONORK_WIN_CTF_BYPOSITION, GLS_LB_UMODE}
	, {CM_APP_SYS_CONSOLE			, GLS_LB_SYSCON}
	, {CM_TRAY_SID_MODE_ONLINE		, GLS_UM_READY}
//	, {CM_TRAY_SID_MODE_BUSY		, user's sex-dependant
//	, {CM_TRAY_SID_MODE_AT_WORK		, user's sex-dependant
	, {CM_TRAY_SID_MODE_FRIENDLY		, GLS_UM_FRIENDLY}
	, {CM_TRAY_SID_MODE_AWAY		, GLS_UM_AWAY}
	, {CM_TRAY_SID_MODE_INVISIBLE		, GLS_UM_INVISIBLE}
//	, {CM_TRAY_SID_MODE_DISCONNECTED	, user's sex-dependant
	, {0					, GLS_NULL}
	};
	static TSonorkWinGlsEntry gls_chat_user[]=
	{
	  {CM_USER_INFO			, GLS_LB_USRINFO}
	, {CM_AUTHORIZE			, GLS_CR_OPER}
	, {CM_DISCONNECT		, GLS_OP_DISCONNECT}
	, {0				, GLS_NULL}
	};

	TSonorkWin::SetMenuText( MainMenu(), gls_main );
	LangSprintf(tmp
		,GLS_OP_CHGMDX
		,IntranetMode()?szInternet:szIntranet);
	TSonorkWin::SetMenuText(MainMenu()
		,CM_SWITCHMODE
		|SONORK_WIN_CTF_ELLIPSIS
		|SONORK_WIN_CTF_ELLIPSIS
		,tmp);

	TSonorkWin::SetMenuText( menus.user_popup , gls_user_popup);
	TSonorkWin::SetMenuText( menus.msgs_popup , gls_msgs_popup);
	TSonorkWin::SetMenuText( menus.mfil_popup , gls_msgs_popup);// same as msgs_popup
	TSonorkWin::SetMenuText( menus.ugrp_popup , gls_ugrp_popup);
	TSonorkWin::SetMenuText( menus.usel_popup , gls_usel_popup);
	TSonorkWin::SetMenuText( menus.clip_popup , gls_msgs_popup);// same as msgs_popup
	TSonorkWin::SetMenuText( menus.chat_view  , gls_msgs_popup);// same as msgs_popup
	TSonorkWin::SetMenuText( menus.chat_user  , gls_chat_user);// same as msgs_popup
	TSonorkWin::SetMenuText( menus.eapp_popup , gls_eapp_popup);
	TSonorkWin::SetMenuText( menus.tray_icon , gls_tray_menu);

	ConvertCmSeparatorToOwnerDraw(MainMenu());
	ConvertCmSeparatorToOwnerDraw(UserMenu());

	DrawMenuBar(win32.main_win->Handle());
	LoadExtAppMenu();
	RebuildTrayMenu();
}

// ----------------------------------------------------------------------------

void
 TSonorkWin32App::RebuildTrayMenu()
{
	GLS_INDEX	busy;
	GLS_INDEX	work;
	GLS_INDEX	dcx;
	SONORK_SEX	sex;

	if( IsProfileOpen() )
	{
		sex = ProfileUser().InfoFlags().GetSex();
	}
	else
	{
		sex = SONORK_SEX_NA;
	}
	if( sex == SONORK_SEX_F )
	{
		busy = GLS_UM_BUSYF;
		work = GLS_UM_WORKF;
		dcx  = GLS_UM_DCXF;
	}
	else
	{
		busy = GLS_UM_BUSYM;
		work = GLS_UM_WORKM;
		dcx  = GLS_UM_DCXM;
	}
	TSonorkWin::SetMenuText( menus.tray_icon , CM_TRAY_SID_MODE_BUSY , busy );
	TSonorkWin::SetMenuText( menus.tray_icon , CM_TRAY_SID_MODE_AT_WORK , work );
	TSonorkWin::SetMenuText( menus.tray_icon , CM_DISCONNECT , dcx );
}


bool
 TSonorkWin32App::MenuMeasureItem( MEASUREITEMSTRUCT* DS)
{
	DWORD		id,data;
	GLS_INDEX	gls;
	SONORK_C_CSTR	str;
	HDC		tDC;
	SIZE		size;
	if( DS->CtlType!= ODT_MENU )
		return false;
	id 	= DS->itemID;
	if(id==CM_SEPARATOR)
	{
		DS->itemWidth	= SKIN_ICON_SW
				+ MENU_ICON_X_L_OFFSET
				+ MENU_ICON_X_R_OFFSET
				+ MENU_ICON_X_PADDING*2;
		DS->itemHeight	=MENU_SEPARATOR_HEIGHT;
	}
	else
	{
		data	= DS->itemData;
		gls	=(GLS_INDEX)(data&SONORK_WIN_CTRL_TEXT_ID);
		str	=LangString(gls);
		tDC=::GetDC(win32.work_win);
		SelectObject(tDC,GetStockObject(DEFAULT_GUI_FONT));
		GetTextExtentPoint32(tDC,str,strlen(str),&size);
		::ReleaseDC(win32.work_win,tDC);

		if(size.cy<SKIN_ICON_SH+MENU_ICON_Y_SPACE*2)
		{
			size.cy=SKIN_ICON_SH+MENU_ICON_Y_SPACE*2;
		}
		DS->itemWidth	=size.cx
				+ SKIN_ICON_SW
				+ MENU_ICON_X_L_OFFSET
				+ MENU_ICON_X_R_OFFSET
				+ MENU_ICON_X_PADDING*2;
		DS->itemHeight	=size.cy;
	}
	return true;
}
struct PopupMenuIconXlat
{
	UINT		pos;
	SKIN_ICON       icon;
};
#define	MAIN_MENU_POPUPS	2
#define USER_MENU_POPUPS	4
bool
 TSonorkWin32App::MenuDrawItem( DRAWITEMSTRUCT* DS)
{
	DWORD		id,data,flags;
	GLS_INDEX	gls;
	SONORK_C_CSTR	str;
	HDC		tDC;
	SKIN_ICON 	ic;
	int		cBg,cFg;
	RECT&		rcItem=DS->rcItem;
	static PopupMenuIconXlat
			mainMenuIconXlat[MAIN_MENU_POPUPS]=
			{{CM_MAIN_POPUP_FILE,SKIN_ICON_CONNECTING}
			,{CM_MAIN_POPUP_USERS,SKIN_ICON_USERS}};
	static PopupMenuIconXlat
			userMenuIconXlat[USER_MENU_POPUPS]=
			{{CM_USER_CONTEXT_POPUP_APPS,SKIN_ICON_APP}
			,{CM_USER_CONTEXT_POPUP_VISIB,SKIN_ICON_VIS_PRIVATE}
			,{CM_USER_CONTEXT_POPUP_AUTH,SKIN_ICON_SECURITY_OFF}
			,{CM_USER_CONTEXT_POPUP_MTPL,SKIN_ICON_USER_TEMPLATE}};



	if( DS->CtlType!= ODT_MENU )
		return false;
	tDC	= DS->hDC;
	id 	= DS->itemID;
	if(id==CM_SEPARATOR)
	{
		id=rcItem.top+MENU_SEPARATOR_HEIGHT/2;
		rcItem.top=id-1;
		rcItem.bottom=id+1;
		::FillRect(tDC
			, &rcItem
			, GetSysColorBrush(SEPARATOR_COLOR));
	}
	else
	{
		SaveDC(tDC);
		SelectObject(tDC,GetStockObject(DEFAULT_GUI_FONT));
		SetBkMode(tDC,TRANSPARENT);

		data	= DS->itemData;
		flags	= (data&SONORK_WIN_CTRL_TEXT_FLAGS);
		gls	= (GLS_INDEX)(data&SONORK_WIN_CTRL_TEXT_ID);
		str	= LangString(gls);
		ic=SKIN_ICON_NONE;
		if( flags & SONORK_WIN_CTF_BYPOSITION)
		{
			if( DS->hwndItem == (HWND)MainMenu() )
			{
				for(int i=0;i<MAIN_MENU_POPUPS;i++)
				if(GetSubMenu(MainMenu(),mainMenuIconXlat[i].pos) == (HMENU)id)
				{
					ic=mainMenuIconXlat[i].icon;
					break;
				}
			}
			else
			if( DS->hwndItem == (HWND)UserMenu())
			{
				for(int i=0;i<USER_MENU_POPUPS;i++)
				if(GetSubMenu(UserMenu(),userMenuIconXlat[i].pos) == (HMENU)id)
				{
					ic=userMenuIconXlat[i].icon;
					break;
				}
			}

		}
		else
		switch(id)
		{
			case CM_CONNECT:
				ic=SKIN_ICON_CONNECTED;
				break;

			case CM_DISCONNECT:
				ic=SKIN_ICON_DISCONNECTED;
				break;

			case CM_CFGNETWORK:
			case CM_EAPP_CONFIG:
			case CM_DBMAINT_USERS:
				ic=SKIN_ICON_TOOLS;
				break;

			case CM_ADD_USER_CTX_GLOBAL:
			case CM_ADD_USER_CTX_USER:
				ic=SKIN_ICON_ADD_USER;
				break;

			case CM_RENAME:
			case CM_REFRESH_USERS:
			case CM_EAPP_RELOAD:
				ic=SKIN_ICON_REFRESH;
				break;

//			case CM_USER_AUTH:
//				if(gls == GLS_OP_AUTHUSR)
//					ic=SKIN_ICON_ADD_USER;
//				else
//					ic=SKIN_ICON_SECURITY_OFF;
//				break;

			case CM_USER_INFO:
			case CM_MY_INFO:
				ic=SKIN_ICON_USER_INFO;
				break;

			case CM_PREFERENCES:
				ic=SKIN_ICON_TOOLS;
				break;

			case CM_VISIBILITY:
				ic=SKIN_ICON_VIS_PRIVATE;
				break;

			case CM_APP_CLIPBOARD:
				ic=SKIN_ICON_NOTES;
				break;

			case CM_APP_REMINDER:
				ic=SKIN_ICON_TIME;
				break;

			case CM_APP_CHAT:
				ic=SKIN_ICON_CHAT;
				break;

			case CM_APP_EMAIL:
				ic=SKIN_ICON_EMAIL;
				break;

			case CM_APP_SYS_CONSOLE:
				ic=SKIN_ICON_PANEL;
				break;

			case CM_ABOUT:
				ic=SKIN_ICON_SONORK;
				break;

			case CM_QUIT:
				ic=SKIN_ICON_CANCEL;
				break;

			case CM_GROUP_MSG:
			case CM_USER_MSG:
				ic=SKIN_ICON_EVENT;
				break;

			case CM_ADD_GROUP:
				ic=SKIN_ICON_ADD;
				break;

			case CM_DELETE:
				if( DS->hwndItem == (HWND)UserMenu())
					ic=SKIN_ICON_DEL_USER;
				else
					ic=SKIN_ICON_DEL;
				break;
		}
		if(DS->itemState&ODS_DISABLED)
		{
			cBg=COLOR_MENU;
			cFg=COLOR_GRAYTEXT;
			id=ILD_BLEND25;
		}
		else
		{
			id=ILD_NORMAL;
			if(DS->itemState&ODS_SELECTED)
			{
				cBg=COLOR_HIGHLIGHT;
				cFg=COLOR_HIGHLIGHTTEXT;
			}
			else
			{
				cBg=COLOR_MENU;
				cFg=COLOR_MENUTEXT;
			}
		}
		SetTextColor(tDC,GetSysColor(cFg));
		FillRect(tDC
			,&rcItem
			,GetSysColorBrush(cBg));
		if( gls!=GLS_NULL)
		{
			rcItem.left+=SKIN_ICON_SW
			+ MENU_ICON_X_L_OFFSET
			+ MENU_ICON_X_R_OFFSET
			+ MENU_ICON_X_PADDING*2;

			DrawText(tDC
				,str
				,strlen(str)
				,&rcItem
				,DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		}

		if(ic!=SKIN_ICON_NONE)
		{
			sonork_skin.DrawIcon(tDC
				, ic
				, MENU_ICON_X_L_OFFSET
				  +MENU_ICON_X_PADDING
				, (rcItem.top+rcItem.bottom-SKIN_ICON_SH)/2
				,id);
		}
		RestoreDC(tDC,-1);
	}
	return true;
}


// ----------------------------------------------------------------------------

void
 TSonorkWin32App::Set_UI_Event(SONORK_UI_EVENT_TYPE event_type
	, SONORK_C_CSTR str
	, DWORD event_flags
	, const void *event_data)
{
	enum HINT_COLOR
	{
		HINT_COLOR_NONE
	,	HINT_COLOR_MSG
	,	HINT_COLOR_ALERT
	,	HINT_COLOR_SUCCESS
	,	HINT_COLOR_ERROR
	};
	DWORD			fg_color;
	DWORD			bg_color;
	SKIN_ICON		event_icon;
	GLS_INDEX		event_gls;
	SONORK_APP_SOUND	event_sound;
	SONORK_SEQUENCE		event_sequence;
	HINT_COLOR		hint_color;
	static char		buffer[512];
	DWORD			info_ttl=SONORK_UI_EVENT_TTL_INFO_SHORT;
	// Load caller's flags, we may choose to ignore
	// these flags for certain events.
	if(TestRunFlag(SONORK_WAPP_RF_APP_TERMINATING))
		return;
	switch( event_type )
	{
		case SONORK_UI_EVENT_NONE:
		case SONORK_UI_EVENT_SEL_COUNT:
		case SONORK_UI_EVENT_EVENT_COUNT:
//			win32.info_win->ClearEvent();
			return;

		case SONORK_UI_EVENT_SONORK_CONNECT:
		case SONORK_UI_EVENT_SONORK_DISCONNECT:
			event_flags|=(SONORK_UI_EVENT_F_SET_INFO
				     |SONORK_UI_EVENT_F_AUTO_DISMISS
				     |SONORK_UI_EVENT_F_SET_SLIDER);
			if(event_type==SONORK_UI_EVENT_SONORK_CONNECT)
			{
				hint_color	=HINT_COLOR_SUCCESS;
				event_icon	=SKIN_ICON_CONNECTED;
				event_sequence	=SONORK_SEQUENCE_CALL;
				event_sound	=SONORK_APP_SOUND_CONNECT;
			}
			else
			{
				hint_color	=HINT_COLOR_ERROR;
				event_icon	=SKIN_ICON_DISCONNECTED;
				event_sequence	=SONORK_SEQUENCE_ERROR;

			}
			break;

		case SONORK_UI_EVENT_ERROR:
			event_flags|=(SONORK_UI_EVENT_F_SET_INFO
				     |SONORK_UI_EVENT_F_AUTO_DISMISS);
			event_flags&=~(SONORK_UI_EVENT_F_SET_SLIDER);
			hint_color	=HINT_COLOR_ERROR;
			if( event_flags & SONORK_UI_EVENT_F_WARNING )
			{
				event_icon	=SKIN_ICON_ALERT;
			}
			else
			{
				event_icon	=SKIN_ICON_ERROR;
			}

			break;

#define pUD	((const TSonorkExtUserData*)event_data)
		case SONORK_UI_EVENT_USER_CONNECT:
		case SONORK_UI_EVENT_USER_DISCONNECT:
			win32.info_win->UserId().Set(pUD->userId);
			win32.slide_win->UserId().Set(pUD->userId);
			hint_color =HINT_COLOR_ALERT;
			if( event_type == SONORK_UI_EVENT_USER_CONNECT )
			{
				event_icon = SKIN_ICON_CONNECTED;
				event_gls  = GLS_IF_USRCX;
				info_ttl   = SONORK_UI_EVENT_TTL_INFO_LONG;
				if(pUD->TestCtrlFlag(SONORK_APP_UCF_NO_SLIDER_CX_DISPLAY))
					event_flags=SONORK_UI_EVENT_F_SET_INFO
						   |SONORK_UI_EVENT_F_AUTO_DISMISS
						   |SONORK_UI_EVENT_F_LOG;
				else
					event_flags=SONORK_UI_EVENT_F_SET_INFO
						   |SONORK_UI_EVENT_F_AUTO_DISMISS
						   |SONORK_UI_EVENT_F_LOG
						   |SONORK_UI_EVENT_F_SET_SLIDER;
				OnlSound(pUD);
			}
			else
			{
				event_icon = SKIN_ICON_DISCONNECTED;
				event_gls  = GLS_IF_USRDCX;
				event_flags=SONORK_UI_EVENT_F_SET_INFO
					   |SONORK_UI_EVENT_F_AUTO_DISMISS
					   |SONORK_UI_EVENT_F_LOG;
			}
			if( pUD->TestCtrlFlag(SONORK_APP_UCF_NO_STATUS_CX_DISPLAY) )
				event_flags&=~SONORK_UI_EVENT_F_SET_INFO;
			LangSprintf(buffer , event_gls ,pUD->display_alias.CStr());
			str=buffer;
#undef pUD
		break;


		case SONORK_UI_EVENT_TASK_START:
			event_flags|=SONORK_UI_EVENT_F_SET_INFO;
			event_flags&=~(SONORK_UI_EVENT_F_AUTO_DISMISS
				       |SONORK_UI_EVENT_F_SET_SLIDER
				       |SONORK_UI_EVENT_F_LOG
				       |SONORK_UI_EVENT_F_SOUND);

			hint_color	=HINT_COLOR_ALERT;
			event_icon 	=SKIN_ICON_BUSY;
			event_sequence	=SONORK_SEQUENCE_WORK;
			break;

		case SONORK_UI_EVENT_TASK_END:

#define pERR	((const TSonorkError*)event_data)
			event_flags|=SONORK_UI_EVENT_F_SET_INFO|SONORK_UI_EVENT_F_AUTO_DISMISS;
			event_flags&=~(SONORK_UI_EVENT_F_SET_SLIDER);
			assert( pERR != NULL );
			if( pERR->Result() == SONORK_RESULT_OK )
			{
				if( event_flags & SONORK_UI_EVENT_F_BICHO)
				{
					SetBichoSequenceIf(SONORK_SEQUENCE_WORK,SONORK_SEQUENCE_IDLE);
					// Don't set the Bicho, we've already done that
					event_flags&=~(SONORK_UI_EVENT_F_BICHO);
				}
				if( event_flags&SONORK_UI_EVENT_F_CLEAR_IF_NO_ERROR)
				{
					win32.info_win->ClearEvent( false );
					return;
				}
				hint_color	=HINT_COLOR_SUCCESS;
				event_icon 	=SKIN_ICON_INFO;
				event_gls	=GLS_IF_TKOK;
			}
			else
			{
				hint_color	=HINT_COLOR_ERROR;
				event_gls	=GLS_IF_TKERR;
				event_sequence	=SONORK_SEQUENCE_ERROR;
				event_sound	=SONORK_APP_SOUND_ERROR;
				event_icon 	=SKIN_ICON_ERROR;
			}
			if( str==NULL )
				str=LangString(event_gls);
#undef pERR
			break;


#define pUD	((const TSonorkExtUserData*)event_data)
		case SONORK_UI_EVENT_ADD_USER:
		case SONORK_UI_EVENT_DEL_USER:
			win32.info_win->UserId().Set(pUD->userId);
			win32.slide_win->UserId().Set(pUD->userId);
			info_ttl=SONORK_UI_EVENT_TTL_INFO_LONG;
			event_flags|=(SONORK_UI_EVENT_F_LOG
					|SONORK_UI_EVENT_F_SET_INFO
					|SONORK_UI_EVENT_F_AUTO_DISMISS
					|SONORK_UI_EVENT_F_SET_SLIDER);

			event_flags&=~( SONORK_UI_EVENT_F_LOG_AS_UNREAD
				       |SONORK_UI_EVENT_F_LOG_AUTO_OPEN
				       |SONORK_UI_EVENT_F_SOUND
				       |SONORK_UI_EVENT_F_BICHO);

			if( event_type == SONORK_UI_EVENT_ADD_USER )
			{
				event_icon = SKIN_ICON_ADD_USER;
				event_gls  = GLS_SM_USRADD;
				event_flags|= SONORK_UI_EVENT_F_SOUND;
				event_sound= SONORK_APP_SOUND_NOTICE;
			}
			else
			{
				event_icon = SKIN_ICON_DEL_USER;
				event_gls  = GLS_SM_USRDEL;
			}
			LangSprintf(buffer , event_gls ,pUD->display_alias.CStr());
			str=buffer;
#undef pUD
		break;

		case SONORK_UI_EVENT_UTS_LINKED:
		case SONORK_UI_EVENT_UTS_UNLINKED:
			event_flags|=	 SONORK_UI_EVENT_F_LOG
					|SONORK_UI_EVENT_F_AUTO_DISMISS;
			event_flags&=~(SONORK_UI_EVENT_F_SET_SLIDER
				       |SONORK_UI_EVENT_F_LOG_AS_UNREAD
				       |SONORK_UI_EVENT_F_LOG_AUTO_OPEN
				       |SONORK_UI_EVENT_F_SET_INFO);
			hint_color = HINT_COLOR_NONE;
			if( event_type == SONORK_UI_EVENT_UTS_LINKED )
			{
				event_icon = SKIN_ICON_SECURITY_ON;
				event_gls  = GLS_SM_PEERON;
			}
			else
			{
				event_icon = SKIN_ICON_SECURITY_OFF;
				event_gls  = GLS_SM_PEEROFF;
			}
			LangSprintf(buffer , event_gls ,str);
			str=buffer;
		break;

		case SONORK_UI_EVENT_DEBUG:
			event_icon 	= SKIN_ICON_DEBUG;
			event_sound	= SONORK_APP_SOUND_NOTICE;
			event_sequence	= SONORK_SEQUENCE_CALL;
			break;

		case SONORK_UI_EVENT_INCOMMING_EMAIL:
		case SONORK_UI_EVENT_SYS_MSG:

			if( event_type == SONORK_UI_EVENT_INCOMMING_EMAIL)
			{
				event_icon = SKIN_ICON_EMAIL;
			}
			else
			{
				event_icon = SKIN_ICON_SONORK;
			}
			event_flags|=SONORK_UI_EVENT_F_SET_SLIDER
				|SONORK_UI_EVENT_F_SET_INFO
				|SONORK_UI_EVENT_F_AUTO_DISMISS;
//			event_flags&=~(SONORK_UI_EVENT_F_SET_INFO);
			event_sound	= SONORK_APP_SOUND_NOTICE;
			event_sequence	= SONORK_SEQUENCE_EMAIL;
			hint_color	= HINT_COLOR_MSG;

		break;

#define pUD	((const TSonorkExtUserData*)event_data)
		case SONORK_UI_EVENT_USER_MSG:
			win32.slide_win->UserId().Set(pUD->userId);
			event_flags|=SONORK_UI_EVENT_F_SET_SLIDER;
			event_flags&=~(SONORK_UI_EVENT_F_SET_INFO
					|SONORK_UI_EVENT_F_SOUND
					|SONORK_UI_EVENT_F_BICHO
					|SONORK_UI_EVENT_F_LOG);
			event_icon 	= SKIN_ICON_EVENT;
			info_ttl	= SONORK_UI_EVENT_TTL_INFO_LONG;
			hint_color	= HINT_COLOR_MSG;
			sprintf(buffer,
				"%s: %-.128s"
				,pUD->display_alias.CStr()
				,str);
			str=buffer;
#undef pUD
		break;
	}
	if( event_flags&SONORK_UI_EVENT_F_SOUND )
	{
		AppSound( event_sound );
	}
	if( hint_color==HINT_COLOR_ALERT )
	{
		fg_color = 0x000000;
		bg_color = 0xF0FFFF;
	}
	else
	if( hint_color==HINT_COLOR_MSG )
	{
		fg_color = sonork_skin.Color(SKIN_COLOR_MSG_VIEW,SKIN_CI_FG);
		bg_color = sonork_skin.Color(SKIN_COLOR_MSG_VIEW,SKIN_CI_BG);
	}
	else
	if( hint_color==HINT_COLOR_SUCCESS)
	{
		fg_color = 0xFFFFFF;
		bg_color = 0x40C040;
	}
	else
	if( hint_color==HINT_COLOR_ERROR )
	{
		if( event_flags & SONORK_UI_EVENT_F_WARNING )
		{
			fg_color = 0x000000;
			bg_color = 0x80D0FF;

		}
		else
		{
			fg_color = 0xFFFFFF;
			bg_color = 0x000080;
		}
	}
	else
	{
		fg_color = sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_FG);
		bg_color = sonork_skin.Color(SKIN_COLOR_HINT,SKIN_CI_BG);
	}
	if( event_flags&SONORK_UI_EVENT_F_SET_INFO )
	{
		win32.info_win->SetEvent(
			  event_type
			, str
			, event_icon
			, fg_color
			, bg_color
			, (event_flags&SONORK_UI_EVENT_F_AUTO_DISMISS)
				?info_ttl
				:SONORK_UI_EVENT_TTL_FOREVER
			);
	}

	if( event_flags&SONORK_UI_EVENT_F_SET_SLIDER )
	{
		if( ProfileCtrlValue(SONORK_PCV_SLIDER_POS) != SONORK_SLIDER_WIN_DISABLED)
		if( !win32.main_win->IsActive() || win32.main_win->IsIconic() )
		{
			win32.slide_win->SetEvent(
				  event_type
				, str
				, event_icon
				, fg_color
				, bg_color
				, SONORK_UI_EVENT_TTL_SLIDE
				);
		}
	}
	if( event_flags&SONORK_UI_EVENT_F_BICHO )
	{
		SetBichoSequence( event_sequence );
	}

	if( event_flags&SONORK_UI_EVENT_F_LOG )
	{
		SysLog( event_type
			, event_icon
			, str
			, event_flags&SONORK_UI_EVENT_F_LOG_AS_UNREAD
			, event_flags&SONORK_UI_EVENT_F_LOG_AUTO_OPEN
			);
	}
}

void
 TSonorkWin32App::Clear_UI_Event( SONORK_UI_EVENT_TYPE event_type )
{
	TSonorkCCacheEntry* CL;
	DWORD line_no,aux;
	if(TestRunFlag(SONORK_WAPP_RF_APP_TERMINATING))
		return;

	if( event_type == SONORK_UI_EVENT_INCOMMING_EMAIL )
	{
		aux	= 0;
		line_no = AppRunValue(SONORK_ARV_FIRST_UNREAD_SYS_MSG);
		for(;;)
		{
			// GetNext returns last line number in <line_no> if none is found
			CL = console.sys->GetNext(line_no
				, SONORK_APP_CCF_UNREAD
				, SONORK_APP_CCF_UNREAD);
			if( CL != NULL )
			{
				if( CL->ext_index == (DWORD)event_type
				||  event_type == SONORK_UI_EVENT_NONE)
				{
					CL->tag.v[SONORK_CCACHE_TAG_FLAGS]&=~SONORK_APP_CCF_UNREAD;
					IncCounter(SONORK_APP_COUNTER_SYS_CONSOLE,-1);
					console.sys->Set(line_no,CL->tag,NULL);
					aux++;
				}
				line_no++;
			}
			else
				break;
		}
		if( aux && SysConsoleWin() != NULL)
		{
			SysConsoleWin()->InvalidateRect(NULL,false);
		}

	}

}

// --------------------------------------------------------------------------
// Info and Hint
// --------------------------------------------------------------------------


void
 TSonorkWin32App::Set_UI_Event(SONORK_UI_EVENT_TYPE event_type
	, GLS_INDEX gls
	, DWORD event_flags
	, const void *event_data)
{
	Set_UI_Event(event_type,LangString(gls),event_flags,event_data);
}


void
 TSonorkWin32App::Set_UI_Event_TaskStart(SONORK_C_CSTR str, DWORD flags)
{
	Set_UI_Event(SONORK_UI_EVENT_TASK_START,str,flags);
}
void
 TSonorkWin32App::Set_UI_Event_TaskStart(GLS_INDEX gls, DWORD flags)
{
	Set_UI_Event(SONORK_UI_EVENT_TASK_START,LangString(gls),flags);
}
	// TaskEnd: if str is NULL, default message is used
void
 TSonorkWin32App::Set_UI_Event_TaskEnd(const TSonorkError*ERR, SONORK_C_CSTR str , DWORD flags)
{
	Set_UI_Event(SONORK_UI_EVENT_TASK_END,str,flags,(void*)ERR);
}


void
 TSonorkWin32App::ShowSysConsole()
{
	RunSysDialog( SONORK_SYS_DIALOG_SYS_CONSOLE );

}

void
 TSonorkWin32App::SysLog( SONORK_UI_EVENT_TYPE	ev_type
			, SKIN_ICON		ev_icon
			, SONORK_C_CSTR		str
			, BOOL 		mark_as_unread
			, BOOL 		force_open )
{
	TSonorkCCacheEntry 		CE;
	TSonorkCodecLCStringAtom	A;

	if( TestRunFlag( SONORK_WAPP_RF_APP_TERMINATING ) )
		return;
	CE.time.Set( CurrentTime() );
	CE.tag.v[SONORK_CCACHE_TAG_INDEX]=ev_icon;
	CE.tag.v[SONORK_CCACHE_TAG_FLAGS]=mark_as_unread?SONORK_APP_CCF_UNREAD:0;
	CE.ext_index 	= ev_type;
	A.ptr		= (char*)str;
	A.length_or_size= strlen(str);
	if(db.sys.Add(&A,&CE.dat_index) == SONORK_RESULT_OK )
	{
		if( mark_as_unread )
		{
			IncCounter( SONORK_APP_COUNTER_SYS_CONSOLE , 1 );
		}

		if( SysConsoleWin() )
		{
			SysConsoleWin()->Add( CE , NULL );
			SysConsoleWin()->MakeLastLineVisible();
		}
		else
		{
			console.sys->Add( CE , NULL );
		}
		if( force_open )
			ShowSysConsole();
	}
}

// ----------------------------------------------------------------------------
/*
SONORK_RESULT
 TSonorkWin32App::SendAddUserRequest(const TSonorkId& user_id,const TSonorkText& text)
{
	return win32.main_win->Task_AddUser(user_id,text);
}
*/

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::CmdAddUser(TSonorkTaskWin*TW,TSonorkError&ERR,const TSonorkId& user_id,DWORD group_no)
{
	UINT				A_size,P_size;
	TSonorkDataPacket	*	P;
	TSonorkText			text;
	TSonorkAuth2    		auth;
	TSonorkExtUserData*		pUD;

	if(user_id == SonorkApp.ProfileUserId() )
		return false;
	pUD=UserList().Get(user_id);
	if( pUD != NULL )
	if( pUD->UserType() == SONORK_USER_TYPE_AUTHORIZED )
	{
		
		OpenUserDataWin(pUD,0);
		return false;
	}

	if( TW == NULL )
		TW=win32.main_win;
	if( TW->IsTaskPending() )
		return false;
	if(!AskAddUserText(TW,text))
		return false;
	A_size = text.CODEC_Size() + 64;

	auth.tag = 0;
	auth.flags.Clear();
	auth.flags.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
	auth.pin = 0;
	auth.SetGroupNo(group_no);
	//SonorkApp.GenTrackingNo( UDN->user_data.user_id );


	P = SONORK_AllocDataPacket( A_size );
	P_size = P->E_ReqAuth_R(A_size
		,user_id
		,auth,text);
	TW->StartSonorkTask(ERR
		, P
		, P_size
		, 0
		, GLS_TK_ADDUSR
		, NULL);
	SONORK_FreeDataPacket( P );
	return true;
}


// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::AskAddUserText(TSonorkWin*parent,TSonorkText& text)
{
	TSonorkInputWin W(parent);
	UINT rv;
	W.flags=SONORK_INPUT_F_LONG_TEXT;
	W.sign=SKIN_SIGN_USERS;
	W.help.Set(SonorkApp.LangString(GLS_ADDUSRNOTESHLP));
	W.prompt.Set(SonorkApp.LangString(GLS_LB_NOTES));
	rv = W.Execute();
	if(rv == IDOK)
	{
		text.SetRegion(SonorkApp.ProfileRegion());
		text.SetStr( 0 , W.input.CStr() );
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

TSonorkWin*
 TSonorkWin32App::GetSysDialog(SONORK_SYS_DIALOG type)
{
	assert( type>SONORK_SYS_DIALOG_INVALID
		&&
		type<SONORK_SYS_DIALOGS);
	return win32.sys_dialog[type];
}
void
 TSonorkWin32App::ShowExistingSysDialog(TSonorkWin* W)
{
	if( W->IsIconic() || !W->IsVisible())
		W->ShowWindow(SW_RESTORE);
	PostAppCommand( SONORK_APP_COMMAND_FOREGROUND_HWND , (LPARAM)W->Handle() );
	SetBichoSequence(SONORK_SEQUENCE_ZOOM);
}
void
 TSonorkWin32App::ShowNewSysDialog(TSonorkWin*W)
{
	if(W->Create())
		W->ShowWindow(SW_SHOW);
	else
		delete W;
}

TSonorkWin*
 TSonorkWin32App::RunSysDialog(SONORK_SYS_DIALOG type)
{
	TSonorkWin *e_win;
	e_win = GetSysDialog(type);
	if( e_win != NULL )
	{
		ShowExistingSysDialog( e_win );
	}
	else
	{
		switch(type)
		{

			case SONORK_SYS_DIALOG_SYS_CONSOLE:
				e_win = new TSonorkSysConsoleWin( win32.main_win, console.sys);
				break;

			case SONORK_SYS_DIALOG_MY_INFO:
				e_win = new TSonorkMyInfoWin(win32.main_win);
				break;


			case SONORK_SYS_DIALOG_GRP_MSG:
				e_win = new TSonorkGrpMsgWin( true );
				break;

			case SONORK_SYS_DIALOG_CLIP:
				e_win = new TSonorkClipWin();
				break;

			case SONORK_SYS_DIALOG_REMIND_LIST:
				e_win = new TSonorkRemindListWin();
				break;

			case SONORK_SYS_DIALOG_REMIND_ALARM:
				e_win = new TSonorkRemindAlarmWin();
				break;

			case SONORK_SYS_DIALOG_EXT_APP_CONFIG:
				e_win = new TSonorkEappCfgWin();
				break;

			case SONORK_SYS_DIALOG_SERVICES:
				e_win = new TSonorkServicesWin();
				break;

			case SONORK_SYS_DIALOG_SNAP_SHOT:
				e_win = new TSonorkSnapshotWin();
				break;

		// Dialogs that require parameters cannot be
		// created here: Caller must create them and
		// afterwards follow same procedure as
		// this function
			case SONORK_SYS_DIALOG_USER_SEARCH:
			default:
				return NULL;
		}
		ShowNewSysDialog( e_win );
	}
	return e_win;
}

void TSonorkWin32App::OnSysDialogRun(TSonorkWin*W
	,SONORK_SYS_DIALOG		type
	,bool				open
	,SONORK_C_CSTR 			szTransferKeyName)
{
	TSonorkWin* sW = open?W:NULL;
	
	if(W == NULL)return;

	if(szTransferKeyName)
		TransferWinStartInfo( W , open, szTransferKeyName, NULL);

	if( type == SONORK_SYS_DIALOG_INVALID )
		return;

	assert( type>SONORK_SYS_DIALOG_INVALID
		&&
		type<SONORK_SYS_DIALOGS);

	win32.sys_dialog[type]=sW;
	BroadcastAppEvent(SONORK_APP_EVENT_SYS_DIALOG
		,SONORK_APP_EM_SYS_WINDOWS_AWARE
		,type
		,sW);
}

// ----------------------------------------------------------------------------

void
 static ConvertCmSeparatorToOwnerDraw(HMENU hmenu)
{
	int i,mi=GetMenuItemCount(hmenu);
	DWORD id;
	for(i=0;i<mi;i++)
	{
		id=GetMenuItemID(hmenu,i);
		if(id==(DWORD)-1)
		{
			ConvertCmSeparatorToOwnerDraw(GetSubMenu(hmenu,i));
		}
		else
		if(id==CM_SEPARATOR)
		{
			TSonorkWin::SetMenuText(hmenu
			, i|SONORK_WIN_CTF_OWNER_DRAW|SONORK_WIN_CTF_BYPOSITION
			, GLS_NULL);
		}
	}

}

