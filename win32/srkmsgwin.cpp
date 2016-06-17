#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkmsgwin.h"
#include "srkmsgfilter.h"
#include "srkwinctrl.h"
#include "srkfiletxgui.h"
#include "srkinputwin.h"
#include "srkuserdatawin.h"
#include "srkultraminwin.h"
#include "srk_url_codec.h"
#include "srk_file_io.h"

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

#define SPACING					1
#define SONORK_MSG_WIN_MIN_INPUT_HEIGHT		70
#define SONORK_MSG_WIN_NOR_INPUT_HEIGHT		80
#define SONORK_MSG_WIN_MAX_INPUT_HEIGHT		140
#define SONORK_MSG_WIN_SEPARATOR_HEIGHT		4
#define SONORK_MSG_WIN_SEPARATOR_PADDING	3
#define SONORK_MSG_WIN_BUTTON_SPACING		1
#define MIN_WIDTH				(154)//+SKIN_ICON_SW)
#define MIN_HEIGHT				220

#define TOOL_BAR_ID				800
#define	CHECK_UTS_MESSAGE_COUNT			16
#define SONORK_CONSOLE_BUTTON_REPLY_TEMPLATE	SONORK_CONSOLE_BUTTON_01
#define DEFAULT_SCAN_LINES			8

#define SENDING_MSG				SONORK_WIN_F_USER_01
#define	PROCESS_QUERY_REPLY			SONORK_WIN_F_USER_02
#define	REPLY_TEMPLATE				SONORK_WIN_F_USER_03
#define RESIZING_INPUT				SONORK_WIN_F_USER_04
#define DIGEST_MODE				SONORK_WIN_F_USER_05
#define HAS_FOCUSED_A_MESSAGE			SONORK_WIN_F_USER_06
#define FOCUS_UNREAD_ON_ACTIVATE		SONORK_WIN_F_USER_07
#define REALIGNED				SONORK_WIN_F_USER_08
#define SHOWING_SID_MSG				SONORK_WIN_F_USER_09
#define NEW_SID_MSG				SONORK_WIN_F_USER_10

#define TBSTYLE_FLAT            		0x0800
#define TBSTYLE_LIST            		0x1000
#define TBSTYLE_CUSTOMERASE     		0x2000


enum TOOL_BAR_BUTTON
{
  TOOL_BAR_FIRST	=	1000
, TOOL_BAR_TEMPLATES	=	TOOL_BAR_FIRST
, TOOL_BAR_SEND_FILE
, TOOL_BAR_SEARCH
, TOOL_BAR_STAY_ON_TOP
, TOOL_BAR_SCROLL_LOCK
, TOOL_BAR_USER_INFO
, TOOL_BAR_USER_APPS
, TOOL_BAR_NEXT_UNREAD
, TOOL_BAR_ULTRA_MINIMIZE
, TOOL_BAR_LAST		=	TOOL_BAR_ULTRA_MINIMIZE
};
#define TOOL_BAR_BUTTONS	(TOOL_BAR_LAST - TOOL_BAR_FIRST + 1)

struct SaveMsgFileDialogHookData
{
	UINT		action;
	bool		enable_open_file;
};
static int  msg_win_sb_offset[MSG_WIN_SB_SECTIONS]=
		{88,88+110,88+110+26,-1};


void
 TSonorkMsgWin::OnCancelMode()
{
	SetResizingInput(false);
}
void
 TSonorkMsgWin::OnActivate(DWORD flags, BOOL )
{
	if( flags==WA_INACTIVE )
	{
		HideSidMsg();
		SonorkApp.CancelPostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)input.hwnd);
	}
	else
	{
		if( !IsIconic() )
		{
			if(IsEnabled())
			{
				PostPoke(SONORK_MSG_WIN_POKE_FOCUS_INPUT_WIN,0);
				if( TestWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE)
				&& user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)
				&& !IsToolBarButtonChecked(TOOL_BAR_SCROLL_LOCK))
				{
					FocusNextUnreadMsg( false );
				}
				else
				{
					ShowSidMsg( true , false );
				}
			}
			ClearWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);
		}
	}
}

void 	TSonorkMsgWin::OnMouseMove(UINT ,int ,int y)
{
//	RECT 	rect;
	int 	dy;
	bool	release;
	if( TestWinUsrFlag(RESIZING_INPUT) )
	{
		dy = y - sonork_win_move_origin.y;
		if( dy )
		{
			input.height -= dy;
			if( input.height <  SONORK_MSG_WIN_MIN_INPUT_HEIGHT )
			{
				input.height = SONORK_MSG_WIN_MIN_INPUT_HEIGHT;
				release=true;
			}
			else
			if( input.height > SONORK_MSG_WIN_MAX_INPUT_HEIGHT)
			{
				input.height = SONORK_MSG_WIN_MAX_INPUT_HEIGHT;
				release=true;
			}
			else
				release=false;
			RealignControls();

			if(release)
				SetResizingInput(false);
			else
				sonork_win_move_origin.y = y;
		}

	}
}
void 	TSonorkMsgWin::OnLButtonUp(UINT ,int ,int )
{
	SetResizingInput( false );
}


void	TSonorkMsgWin::SetResizingInput( BOOL v )
{
	if( !ChangeWinUsrFlag(RESIZING_INPUT,v) )return;

	if(v)
	{
		SetCapture( Handle() );
		SetCursor( sonork_skin.Cursor(SKIN_CURSOR_SIZE_NS) );
		HideCaret( input.hwnd );
		GetCursorPos(&sonork_win_move_origin);
		ScreenToClient(&sonork_win_move_origin);
	}
	else
	{
		ReleaseCapture();
		SetCursor( sonork_skin.Cursor(SKIN_CURSOR_ARROW) );
		ShowCaret( input.hwnd );
		SonorkApp.ProfileCtrlValue(SONORK_PCV_INPUT_HEIGHT) = input.height;
	}
}

LRESULT
	TSonorkMsgWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	TSonorkClipData *	CD;
	switch( event )
	{
		case SONORK_DRAG_DROP_QUERY:
					SonorkApp.ProcessDropQuery((TSonorkDropQuery*)lParam
					,SONORK_DROP_ACCEPT_F_FILES
					|SONORK_DROP_ACCEPT_F_URL
					|SONORK_DROP_ACCEPT_F_TEXT
					|SONORK_DROP_ACCEPT_F_SONORKCLIPDATA);
		break;

		case SONORK_DRAG_DROP_UPDATE:
		{
			TSonorkDropMsg*M=(TSonorkDropMsg*)lParam;
			*M->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE;
		}
		break;
		case SONORK_DRAG_DROP_CANCEL:
		case SONORK_DRAG_DROP_INIT:
			break;
		case SONORK_DRAG_DROP_EXECUTE:
		{
			CD = SonorkApp.ProcessDropExecute((TSonorkDropExecute*)lParam);
			if( CD == NULL )break;

			if( !SonorkApp.ProcessUserDropData(user_data,CD,false) )
			{
				SetCtrlDropText( IDC_MSG_INPUT , CD , "\r\n");
			}
			CD->Release();
		}
		break;
	}
	return 0;
}

void	TSonorkMsgWin::PostToolbarCmd(TOOL_BAR_BUTTON tb)
{
	PostMessage(WM_COMMAND,MAKEWPARAM(tb,BN_CLICKED),(LPARAM)toolbar.hwnd);
}
DWORD SONORK_CALLBACK
	TSonorkMsgWin::ConsoleCallback(void*			pTag
					,SONORK_CONSOLE_EVENT 	gcEvent
					,DWORD			pIndex
					,void*			pData)
{
	TSonorkMsgWin *_this = (TSonorkMsgWin*)pTag;
	switch(gcEvent)
	{
		case SONORK_CONSOLE_EVENT_HISTORY_EVENT:
			return _this->OnHistoryWinEvent(
				(TSonorkHistoryWinEvent*)pData);


		case SONORK_CONSOLE_EVENT_PROCESS:
			_this->OpenMsgWithFlags
			(
				pIndex?	SONORK_CONSOLE_TBF_PROCESS
					:SONORK_CONSOLE_TBF_DELETE
			);
			break;

		case SONORK_CONSOLE_EVENT_CLOSE_VIEW:
			_this->CloseMsg();
			return true;	// we've processed it

		case SONORK_CONSOLE_EVENT_VKEY:
			return _this->ProcessVKey(pIndex , (DWORD)pData);

		case SONORK_CONSOLE_EVENT_TOOLBAR_TOOL_TIP:
			if(pIndex == SONORK_CONSOLE_BUTTON_REPLY_TEMPLATE)
			{
				TSonorkWinToolTipText *pTTT=(TSonorkWinToolTipText*)pData;
				wsprintf(pTTT->str
						,"%s (%s)"
						,SonorkApp.LangString(GLS_OP_REPLY)
						,SonorkApp.LangString(GLS_LB_TMPLTS)
						);
			}
			break;

		case SONORK_CONSOLE_EVENT_TOOLBAR_NOTIFY:
			{
				TBNOTIFY *pT=(TBNOTIFY*)pData;
				if(pT->hdr.code==BN_CLICKED)
				{
					_this->ShowMtplMenu(true);
				}
				else
				if(pT->hdr.code==TBN_DROPDOWN)
				{
					_this->ShowMtplMenu(true);
					return TBDDRET_DEFAULT;
				}
			}
			break;
		case SONORK_CONSOLE_EVENT_INPUT_RESIZED:
			SonorkApp.ProfileCtrlValue(SONORK_PCV_INPUT_HEIGHT) = pIndex;
			break;
		case SONORK_CONSOLE_EVENT_EXPORT:
			return _this->OnConsoleExport(
				 (TSonorkConsoleExportEvent*)pData);
	}
	return 0L;
}

LRESULT
 TSonorkMsgWin::ProcessVKey( UINT vKey , DWORD vKeyFlags)
{
	BOOL ignore_esc;
	if( vKeyFlags&SONORK_CONSOLE_VKEY_WM_CHAR )
	{
		if((ignore_esc=TestWinUsrFlag(SHOWING_SID_MSG))!=0)
			HideSidMsg();
		switch( vKey )
		{
			case 10:
			case VK_RETURN:
				if( vKey == 10 || (vKeyFlags&SONORK_CONSOLE_VKEY_CONTROL) )
				{
					if(SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_SEND_WITH_ENTER) )
						break;
				}
				else
				if(vKey == VK_RETURN )
				{
					if( !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_SEND_WITH_ENTER) )
						break;
				}
				PostMessage(WM_COMMAND
					,MAKEWPARAM(IDC_MSG_SEND,BN_CLICKED)
					,(LPARAM)btn.send);
				return true;

			case VK_ESCAPE:
				if(!ignore_esc)
				{
					if( context.console->GetOutputMode() == SONORK_CONSOLE_OUTPUT_HISTORY )
					{
						OpenMsgWithFlags( 0 );
					}
					else
					{
						CloseMsg();
					}
				}
				return true;

			case VK_SPACE:
				if( !(vKeyFlags&SONORK_CONSOLE_VKEY_CONTROL) )
					return false;

				PostPoke(SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD , false);
				return true;

			case 5: // CTRL+E  - Next (E)vent
				PostPoke(SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD, true );
				return true;

			case 6: // CTRL+F (Find)
				PostToolbarCmd(TOOL_BAR_SEARCH);
				return true;

			case 14: // CTRL+N (N)ext Window
				SonorkApp.FocusNextWindow(this);
				return true;

			case 17: // CTRL+Q (Query)
				PostMessage(WM_COMMAND
					,MAKEWPARAM(IDC_MSG_QUERY,BN_CLICKED)
					,(LPARAM)btn.query);
				return true;

			case 18: // CTRL+R (Reply)
				PostMessage(WM_COMMAND
					,MAKEWPARAM(IDC_MSG_REPLY,BN_CLICKED)
					,(LPARAM)btn.reply);
				return true;

			case 21: // CTRL+U (Send File)
				PostToolbarCmd(TOOL_BAR_SEND_FILE);
				return true;

			case 23: // CTRL+W (W)here is reply
				PostPoke(SONORK_MSG_WIN_POKE_FIND_LINKED,true);
//				PostToolbarCmd(TOOL_BAR_FIND_LINKED);
				return true;


				/*
			case 127: // CTRL+BACKSPACE
				if( !(vKeyFlags&SONORK_CONSOLE_VKEY_CONTROL) )
					return false;
				ShowWindow(SW_MINIMIZE);
				return true;
				*/

		}

	}
	else
	{
		switch( vKey )
		{

			// Ignore VK_ESCAPE for WM_SYSKEYDOWN,WM_KEYDOWN
			case VK_ESCAPE:
				return true;


			case VK_SPACE:
				if( (vKeyFlags&SONORK_CONSOLE_VKEY_CONTROL)
				&&   GetKeyState(VK_SHIFT)&0x8000)
				{

					PostPoke(SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD , true );
				}
				return true;

			case VK_F11:
				CmdScrollLock( true );
				return true;

			case VK_F6:
			case VK_F12:
				PostPoke(SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD , true );
				return true;
		}
		return false;
	}
	return false;
}

void
 TSonorkMsgWin::OnHistoryWin_GetText(
		TSonorkMsg&			msg
	,	const TSonorkCCacheEntry*	CL
	, 	SONORK_HISTORY_WIN_TEXT_TYPE
	, 	TSonorkDynData*			DD)
{
	if( SonorkApp.GetMsg(CL->dat_index ,&msg) == SONORK_RESULT_OK )
		DD->AppendStr(msg.text.ToCStr(),false);
}

void
 TSonorkMsgWin::OnHistoryWin_LinePaint(const TSonorkCCacheEntry*CL, TSonorkHistoryWinPaintCtx*CTX)
{
	DWORD 	conFlags=CL->tag.v[SONORK_CCACHE_TAG_FLAGS];
	DWORD&  ctxFlags=CTX->flags;
//	SKIN_ICON	l_icon;
//	SKIN_ICON 	r_icon;
//	SKIN_ICON 	m_icon;
//	HBRUSH		brush;
	SKIN_COLOR	l_color;
	CTX->l_icon = GetMsgIcon( conFlags );

	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED ) // Line is focused
	{
		l_color = SKIN_COLOR_MSG_FOCUS;
	}
	else
	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_SELECTED ) // Line is selected
	{
		l_color = SKIN_COLOR_MSG_SELECT;
	}
	else
	{
		if( conFlags & SONORK_APP_CCF_INCOMMING )
		{
			if( conFlags & SONORK_APP_CCF_UNREAD )
			{
				l_color = SKIN_COLOR_MSG_IN_NEW;
			}
			else
			{
				l_color = SKIN_COLOR_MSG_IN_OLD;
			}
		}
		else
		{
			l_color = SKIN_COLOR_MSG_OUT;
		}
	}
	CTX->SetLineColor(sonork_skin.Color(l_color,SKIN_CI_BG));
	CTX->SetTextColor(sonork_skin.Color(l_color,SKIN_CI_FG));

	switch( (CL->tag.v[SONORK_CCACHE_TAG_INDEX]&SONORK_SERVICE_TYPE_MASK) )
	{
		case SONORK_SERVICE_TYPE_SONORK_FILE:
			if( conFlags & SONORK_APP_CCF_DELETED)
			{
				CTX->r_icon = SKIN_ICON_MORE_FILE_DELETE;
			}
			else
			{
				CTX->r_icon = SKIN_ICON_MORE_FILE;
			}
			break;


		case SONORK_SERVICE_TYPE_URL:
			CTX->r_icon = SKIN_ICON_MORE_URL;
			break;

		case SONORK_SERVICE_TYPE_EMAIL:
			CTX->r_icon = SKIN_ICON_EMAIL;
			break;


		case SONORK_SERVICE_TYPE_NONE:
		default:
			break;
	}
/*	::FillRect(CTX->hDC(),CTX->LeftRect(),brush);
	sonork_skin.DrawIcon(CTX->hDC()
		,l_icon
		,CTX->LeftRect()->left+1
		,CTX->TextRect()->top );
*/
	ctxFlags|=SONORK_HIST_WIN_PAINT_F_HOT_LICON;
	if( conFlags & SONORK_APP_CCF_PROTECTED)
	{
		CTX->e_icon=SKIN_ICON_PROTECTED_MSG;
		/*
		sonork_skin.DrawIcon(CTX->hDC()
			,SKIN_ICON_PROTECTED_MSG
			,CTX->LeftRect()->left+1
			,CTX->TextRect()->top );
		*/
	}
}

DWORD
 TSonorkMsgWin::OnHistoryWinEvent(TSonorkHistoryWinEvent*E)
{
	switch( E->Event() )
	{
		case SONORK_HIST_WIN_EVENT_LINE_PAINT:
			OnHistoryWin_LinePaint( E->Line(), E->PaintContext() );
		break;

		case SONORK_HIST_WIN_EVENT_SEL_CHANGE:
			UpdateStatusBar( SONORK_MSG_WIN_F_UPDATE_SB_SELECTION
				, NULL);
		break;

		case SONORK_HIST_WIN_EVENT_HINT_CLICK:
			HideSidMsg();
		break;

		case SONORK_HIST_WIN_EVENT_GET_TEXT:
			OnHistoryWin_GetText(
				  context.tmp_msg
				, E->Line()
				, E->GetTextType()
				, E->GetTextData() );
		break;

		case SONORK_HIST_WIN_EVENT_HINT_PAINT:
			OnHistoryWin_HintPaint( E->HintDC(), E->HintRect());
			break;

		case SONORK_HIST_WIN_EVENT_LINE_CLICK:
			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED )
			{
				OnHistoryWin_FocusChanged();
			}
			else
			{
			// Don't search for linked line if focus has changed
			// because it does not look right. If we do that, the
			// user will click on the line (at the icon) but
			//  the focus will go elsewhere (to the linked line).
				if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_LICON_CLICK) )
				{
					PostPoke(SONORK_MSG_WIN_POKE_FIND_LINKED,false);
					break;
				}
			}

			if(context.console->GetOutputMode() == SONORK_CONSOLE_OUTPUT_HISTORY )
			{
				if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
				{
					OpenMsgWithFlags(
						E->ClickFlags()&SONORK_HIST_WIN_FLAG_DOUBLE_CLICK
						?SONORK_CONSOLE_TBF_PROCESS
						:0);
					break;
				}

				if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
				{
					TrackPopupMenu(SonorkApp.MsgsPopupMenu()
							, TPM_LEFTALIGN  | TPM_LEFTBUTTON
							,E->ClickPoint().x
							,E->ClickPoint().y
							,0
							,Handle()
							,NULL);
				}
			}
		break;

		case SONORK_HIST_WIN_EVENT_LINE_DRAG:
			SonorkApp.DoMsgDrag( context.console , E->LineNo() );
		break;

		case SONORK_HIST_WIN_EVENT_LINE_DELETE:
			return OnHistoryWin_LineDelete( E->Line() );
	}
	return 0L;
}
void
 TSonorkMsgWin::OnHistoryWin_FocusChanged()
{
	TSonorkCCacheEntry *CL;
	DWORD sb_flags,line_no;
	line_no = context.console->FocusedLine();
	if( TestWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE) )
		ClearWinUsrFlag( FOCUS_UNREAD_ON_ACTIVATE );
	SetWinUsrFlag( HAS_FOCUSED_A_MESSAGE );
	sb_flags= SONORK_MSG_WIN_F_UPDATE_SB_FOCUS
		 |SONORK_MSG_WIN_F_UPDATE_SB_LINE_COUNT
		 |SONORK_MSG_WIN_F_UPDATE_SB_SELECTION
		 |SONORK_MSG_WIN_F_UPDATE_SB_MSG_INFO;

	CL = context.cache->Get( line_no , NULL , NULL );
	if( CL != NULL )
	{
		if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS]
			& (SONORK_APP_CCF_UNREAD|SONORK_APP_CCF_READ_ON_SELECT))
			== (SONORK_APP_CCF_UNREAD|SONORK_APP_CCF_READ_ON_SELECT) )
		{
			MarkLineAsRead( line_no , CL );
		}
	}
	else
	{
		// Remove SONORK_MSG_WIN_F_UPDATE_ST_MSG_INFO
		// To prevent UpdateMsgInfoText() from trying to read
		// the line again (we've just failed)
		sb_flags&=~SONORK_MSG_WIN_F_UPDATE_SB_MSG_INFO;
	}
	
	// We update the StatusBar() BEFORE Opening the message
	// because OpenMsg() uses the status bar to report any errors

	UpdateStatusBar( sb_flags , CL );
	if(context.console->GetOutputMode() == SONORK_CONSOLE_OUTPUT_VIEW && CL != NULL )
	{
		OpenMsgWithFlags( 0 );
	}

}
DWORD
 TSonorkMsgWin::OnHistoryWin_LineDelete( const TSonorkCCacheEntry*CE )
{
	if(CE->tag.v[SONORK_CCACHE_TAG_FLAGS]&(SONORK_APP_CCF_UNREAD|SONORK_APP_CCF_PROTECTED))
		return 0;
	SonorkApp.DelMsg( CE->dat_index );
	if( CE->ext_index != SONORK_INVALID_INDEX )
		SonorkApp.DelExtData( CE->ext_index );
	return SONORK_HIST_WIN_DELETE_F_ALLOW;
}

void
 TSonorkMsgWin::CloseMsg()
{
	context.console->CloseView();
	FocusInputWin();
}

TSonorkMsgWin::TSonorkMsgWin(TSonorkExtUserData*UD,TSonorkCCache*MC)
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_SONORK_MSG
	|SONORK_WIN_DIALOG
	|IDD_MSG
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
	user_data=UD;
	context.cache	= MC;
	context.console = new TSonorkConsole( this , MC, ConsoleCallback, this , 0);
	context.msg_win	= this;
	SetEventMask(SONORK_APP_EM_CX_STATUS_AWARE
		|SONORK_APP_EM_PROFILE_AWARE
		|SONORK_APP_EM_USER_LIST_AWARE
		|SONORK_APP_EM_MSG_CONSUMER
		|SONORK_APP_EM_MSG_PROCESSOR
		|SONORK_APP_EM_CTRL_MSG_CONSUMER
		|SONORK_APP_EM_SKIN_AWARE);

	//search.flags=0;
	sent_messages_counter=0;
	if( UD->IsSystemUser() )
		SetWinUsrFlag(DIGEST_MODE);
}
TSonorkMsgWin::~TSonorkMsgWin()
{
	if(context.cache != NULL)
	{
		SonorkApp.ReleaseSharedMsgCache( context.cache );
		context.cache=NULL;
	}
}
void
 TSonorkMsgWin::UpdateUserInterface(DWORD flags)
{
	char			tmp[128];
	UINT			hicon;
	GLS_INDEX		gls_index;
	hicon = (UINT)SonorkApp.GetUserModeHicon(user_data, &gls_index);
	if(flags&SONORK_MSG_WIN_F_UPDATE_UI_ICON)
	{
		if( user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) != 0 )
			hicon =  (UINT)sonork_skin.Hicon(SKIN_HICON_EVENT);
		SetCaptionIcon( (HICON)hicon );
	}
	if(flags&SONORK_MSG_WIN_F_UPDATE_UI_CAPTION)
	{
		wsprintf(tmp,"%-.32s (%-.32s,%u.%u)"
			,user_data->display_alias.CStr()
			,SonorkApp.LangString(gls_index)
			,user_data->userId.v[0]
			,user_data->userId.v[1]);
		SetWindowText( tmp );
	}
	if( flags & (SONORK_MSG_WIN_F_UPDATE_UI_CAPTION|SONORK_MSG_WIN_F_UPDATE_UI_ICON))
		if(ultra_min.win)
			ultra_min.win->InvalidateRect(NULL,false);
	if( flags & (SONORK_MSG_WIN_F_UPDATE_UI_SID_TEXT))
	{
		if( user_data->sid_text.Length() )
		{
			hicon=SKIN_HICON_NOTES;
		}
		else
		{
			hicon=SKIN_HICON_NONE;
		}
		::SendMessage(status.hwnd
			,SB_SETICON
			,(MSG_WIN_SB_SID_MSG&0xf)
			,(LPARAM)sonork_skin.Hicon((SKIN_HICON)hicon));
	}
}
void
 TSonorkMsgWin::SetupConsole(TSonorkConsole* pConsole)
{
	pConsole->SetMaxScanLines( DEFAULT_SCAN_LINES );
	//pConsole->SetMargins(SKIN_ICON_SW*2+2,1);
//	pConsole->SetMargins(SKIN_ICON_SW+2,1);
	pConsole->SetPaddingEx(3,1);
	pConsole->EnableLineDrag(true);
	pConsole->SetDefColors( SKIN_COLOR_MSG_VIEW );
}

void
 TSonorkMsgWin::UpdateStatusBar(DWORD flags, TSonorkCCacheEntry* pEntry)
{
	DWORD 		aux;
	char 		tmp[80];
	SYSTEMTIME	si;
	SKIN_HICON	hicon;
	DWORD		line_no=context.console->FocusedLine();

//	TRACE_DEBUG( "UPDATE_STATUS %u,%08x,%02x" , line_no,pEntry,flags );
	if( (aux = user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT))!=0)
	{
		SonorkApp.LangSprintf(tmp,GLS_IF_EVTC,aux);
		hicon = SKIN_HICON_EVENT;
	}
	else
	if( (aux = context.console->Selection().Items() ) != 0)
	{
		SonorkApp.LangSprintf(tmp , GLS_MS_NITEMSEL, aux );
		hicon = SKIN_HICON_INFO;
	}
	else
	{
		wsprintf(tmp,"%u/%u",line_no + 1,context.console->Lines());
		hicon = SKIN_HICON_NONE;
	}

	SetMsgWinStatus(status.hwnd
		, MSG_WIN_SB_HINT
		, tmp
		, hicon);

	if( flags & SONORK_MSG_WIN_F_UPDATE_SB_MSG_INFO	)
	{
		if(pEntry == NULL)
			pEntry = context.console->Get(line_no , NULL);
		if(pEntry != NULL)
		{
			pEntry->time.GetTime(&si);
			wsprintf(tmp
				,"%04u/%02u/%02u %02u:%02u"
				,si.wYear
				,si.wMonth
				,si.wDay
				,si.wHour
				,si.wMinute);

		}
		else
			*tmp=0;
		SetMsgWinStatus(status.hwnd
			, MSG_WIN_SB_DATE
			, tmp
			, SKIN_HICON_NONE);
	}
}


bool
 TSonorkMsgWin::OnCreate()
{
static	UINT tt_ctrls[3]={IDC_MSG_SEND
			, IDC_MSG_REPLY
			, IDC_MSG_QUERY
			};
	BOOL	interactive_mode=!TestWinUsrFlag(DIGEST_MODE);
	RECT	rect;
	ultra_min.x=-1;
	ultra_min.win=NULL;

// Toolbar ----------------------


	SetupToolBar();

// Input window ----------------------

	input.height = SonorkApp.ProfileCtrlValue(SONORK_PCV_INPUT_HEIGHT) ;
	if( input.height < SONORK_MSG_WIN_MIN_INPUT_HEIGHT
	||  input.height > SONORK_MSG_WIN_MAX_INPUT_HEIGHT)
		input.height = SonorkApp.ProfileCtrlValue(SONORK_PCV_INPUT_HEIGHT) = SONORK_MSG_WIN_NOR_INPUT_HEIGHT;



// Logo ----------------------

	logo.hwnd	= GetDlgItem(IDC_MSG_LOGO);
	rect.left=rect.top=0;
	rect.right=SKIN_SMALL_LOGO_SW;
	rect.bottom=SKIN_SMALL_LOGO_SH;
	::AdjustWindowRectEx(&rect
		,::GetWindowLong(logo.hwnd,GWL_STYLE)
		,false
		,::GetWindowLong(logo.hwnd,GWL_EXSTYLE));
	logo.min_width	= rect.right-rect.left;
	logo.height	= rect.bottom-rect.top;
	input.hwnd	= GetDlgItem(IDC_MSG_INPUT);
	SetEditCtrlMaxLength(IDC_MSG_INPUT,SONORK_APP_MAX_MSG_TEXT_LENGTH);
	input.drop_target.AssignCtrl( Handle(), input.hwnd );
	::SendMessage(input.hwnd
		,WM_SETFONT
		,(WPARAM)sonork_skin.Font(SKIN_FONT_LARGE)
		,MAKELPARAM(1,0));

// Status --------

	status.hwnd	=GetDlgItem(IDC_MSG_STATUS);
	status.height	= GetCtrlHeight(IDC_MSG_STATUS);
	InitMsgWinStatus( status.hwnd );

// Buttons --------

	btn.send	= GetDlgItem(IDC_MSG_SEND);
	btn.reply	= GetDlgItem(IDC_MSG_REPLY);
	btn.query	= GetDlgItem(IDC_MSG_QUERY);
	separator	= GetDlgItem(IDC_MSG_SEPARATOR);
	btn.size.cx 	= GetCtrlWidth(IDC_MSG_REPLY);
	btn.size.cy 	= toolbar.size.cy;


// Hide controls not used when not in interactive mode --------

	SetCtrlVisible(IDC_MSG_SEND,interactive_mode);
	SetCtrlVisible(IDC_MSG_REPLY,interactive_mode);
	SetCtrlVisible(IDC_MSG_QUERY,interactive_mode);
	SetCtrlVisible(IDC_MSG_INPUT,interactive_mode);
	SetCtrlVisible(IDC_MSG_LOGO,interactive_mode);

	if( interactive_mode )
	{
		AddToolTipCtrl(3 , tt_ctrls);
		//AddToolTipCtrl( logo.hwnd );
	}


// Console & History Min -----------------

	context.console->Create();
	SetupConsole( context.console );
	input.ctrl.AssignCtrl(context.console,input.hwnd,IDC_MSG_INPUT);

	context.console->SetToolBarIconAndStyle(SONORK_CONSOLE_BUTTON_REPLY_TEMPLATE
				,SKIN_ICON_REPLY_TEMPLATE
				,TBSTYLE_BUTTON);

	UpdateUserInterface(SONORK_MSG_WIN_F_UPDATE_UI_CAPTION
				|SONORK_MSG_WIN_F_UPDATE_UI_ICON
				|SONORK_MSG_WIN_F_UPDATE_UI_SID_TEXT);
	UpdateMtplButtons();

	// LoadWinStartInfo sets the the initial window position
	SonorkApp.TransferWinStartInfo( this , true , "MsgWin", &user_data->userId);

	if( !TestWinUsrFlag(REALIGNED) )
		RealignControls();


	return true;
}

void
 TSonorkMsgWin::OnAfterCreate()
{

	// -----------------------
	// IMPORTANT:
	//  The resource dialog for this window is DISABLED so we
	//  manually enable the window after finishing initialization.
	//  The reason for having it DISABLED during creating is because
	//  windows always automatically activates enabled POPUP-DIALOGS
	//  and we don't want always to be activated automatically
	// (for example, when the user is already typing on another message
	//  window, we don't want to popup in front of what's he/she's doing).
	// The enabling and initialization is done in
	// the OnPoke() handler for POKE_OPEN which must be
	// posted to this window after creating it.
}

void
 TSonorkMsgWin::UpdateMtplButtons()
{
	BOOL interactive;
	BOOL templates_enabled;
	interactive	 =!TestWinUsrFlag(DIGEST_MODE);
	templates_enabled=GetMenuItemCount(SonorkApp.UserMtplMenu())!=0&&interactive;
	context.console->EnableToolBar(SONORK_CONSOLE_BUTTON_REPLY_TEMPLATE,templates_enabled);
	SetToolBarButtonEnabled(TOOL_BAR_TEMPLATES,templates_enabled);
	SetToolBarButtonEnabled(TOOL_BAR_USER_APPS,interactive);
	SetToolBarButtonEnabled(TOOL_BAR_SEND_FILE,interactive);
}
void
 TSonorkMsgWin::OnBeforeDestroy()
{
	if(ultra_min.win)ultra_min.win->Destroy();
	input.ctrl.ReleaseCtrl();
	input.drop_target.Enable( false );
	SonorkApp.TransferWinStartInfo( this , false , "MsgWin", &user_data->userId);
	context.console->Destroy();
	context.cache->Flush();
}
void
 TSonorkMsgWin::OnDestroy()
{
	if(context.cache != NULL)
	{
		SonorkApp.ReleaseSharedMsgCache(context.cache );
		context.cache=NULL;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::OnMove()
{}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::OnSize(UINT size_type)
{
	if(size_type==SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
	{
		RealignControls();
	}
}

// ----------------------------------------------------------------------------

bool
 TSonorkMsgWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MIN_WIDTH;
	MMI->ptMinTrackSize.y=MIN_HEIGHT;
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::FocusInputWin()
{
	if( IsActive()&&IsEnabled()&&!IsIconic()&&!IsUltraMinimized() )
	{
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)input.hwnd);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::RealignControls()
{
	HDWP defer_handle;
	const int H = (int)Height() ;
	const int W = (int)Width();
	int	console_W;
	int	input_W;
	int	btn_x,x,y,h,reply_cy;

	SetWinUsrFlag(REALIGNED);
	console_W	= W - SPACING*2;

	defer_handle = BeginDeferWindowPos( 8 );



	if( TestWinUsrFlag(DIGEST_MODE) )
	{
		input_W	= W - SPACING*2;
		h 	= H -
			( status.height
			+ SPACING*4
			+ SONORK_MSG_WIN_SEPARATOR_PADDING*2
			+ toolbar.size.cy
			);
	}
	else
	{
		input_W	= W - btn.size.cx - SPACING * 4;
		btn_x 	= input_W + SPACING*4;
		h 	= H -
			( input.height
			+ status.height
			+ SONORK_MSG_WIN_SEPARATOR_HEIGHT
			+ SONORK_MSG_WIN_SEPARATOR_PADDING*2
			+ SPACING*4
			+ toolbar.size.cy
			);
	}

	defer_handle = DeferWindowPos(defer_handle
		,context.console->Handle()
		,NULL
		,SPACING
		,SPACING
		,console_W
		,h
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y = h + SPACING;

	defer_handle = DeferWindowPos(defer_handle
		,status.hwnd
		,NULL
		,0
		,H - status.height
		,W
		,status.height
		,SWP_NOZORDER|SWP_NOACTIVATE);
	y+=SONORK_MSG_WIN_SEPARATOR_PADDING;
	defer_handle = DeferWindowPos(defer_handle
		,separator
		,NULL
		,SPACING
		,y
		,console_W
		,SONORK_MSG_WIN_SEPARATOR_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);
	y+=SONORK_MSG_WIN_SEPARATOR_HEIGHT+SONORK_MSG_WIN_SEPARATOR_PADDING;

	if( !TestWinUsrFlag(DIGEST_MODE) )
	{
		x=btn_x-toolbar.size.cx-SPACING;// * 3;
		defer_handle =	DeferWindowPos(defer_handle
			,toolbar.hwnd
			,NULL
			,x
			,y
			,toolbar.size.cx
			,toolbar.size.cy
			,SWP_NOZORDER|SWP_NOACTIVATE);
		if(x<logo.min_width+SPACING*2)
		{
			h=logo.min_width;
		}
		else
			h=x-SPACING*2;
		x-=h;

		defer_handle =	DeferWindowPos(defer_handle
			,logo.hwnd
			,NULL
			,x
			,y
			,h
			,toolbar.size.cy-1
			,SWP_NOZORDER|SWP_NOACTIVATE);

		defer_handle = DeferWindowPos(defer_handle
			,input.hwnd
			,NULL
			,SPACING
			,y + toolbar.size.cy + SPACING
			,input_W
			,input.height+SPACING*2
			,SWP_NOZORDER|SWP_NOACTIVATE);


		defer_handle = DeferWindowPos(defer_handle
			,btn.query
			,NULL
			,btn_x
			,y
			,btn.size.cx
			,btn.size.cy
			,SWP_NOZORDER|SWP_NOACTIVATE);

		y+= btn.size.cy	+ SONORK_MSG_WIN_BUTTON_SPACING ;

		h = H - y - status.height - SPACING;
		reply_cy = h*2/5;
		if(reply_cy<btn.size.cy)
			reply_cy = btn.size.cy;

		defer_handle = DeferWindowPos(defer_handle
			,btn.reply
			,NULL
			,btn_x
			,y
			,btn.size.cx
			,reply_cy
			,SWP_NOZORDER|SWP_NOACTIVATE);

		y+= reply_cy	+ SONORK_MSG_WIN_BUTTON_SPACING ;

		defer_handle = DeferWindowPos(defer_handle
			,btn.send
			,NULL
			,btn_x
			,y
			,btn.size.cx
			,h-reply_cy
			//,input.height + toolbar.size.cy -  (reply_cy + btn.size.cy + (SONORK_MSG_WIN_BUTTON_SPACING * 2))
			,SWP_NOZORDER|SWP_NOACTIVATE);
	}
	else
	{
		defer_handle =	DeferWindowPos(defer_handle
			,toolbar.hwnd
			,NULL
			,SPACING
			,y
			,toolbar.size.cx
			,toolbar.size.cy
			,SWP_NOZORDER|SWP_NOACTIVATE);
	}


	 EndDeferWindowPos(defer_handle);
	 ::InvalidateRect(separator,NULL,false);
	 ::InvalidateRect(logo.hwnd,NULL,false);
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::CmdSendMessage( SEND_MODE send_mode )
{
	DWORD 			reply_tracking_no;
	DWORD 			line_no;
	TSonorkMsgHandleEx	handle;
	TSonorkShortString 	str;

	if( TestWinUsrFlag(DIGEST_MODE) )
		return ;

	ClearWinUsrFlag(PROCESS_QUERY_REPLY);
	send_ctx.mark.Clear();
	reply_tracking_no = 0;
	if( send_mode == SEND_MODE_REPLY )
	{
		TSonorkCCacheEntry *CL;
		if((CL = context.console->GetFocused(NULL,&line_no))!=NULL)
		{
			send_ctx.mark.Set(line_no,CL);
			reply_tracking_no = send_ctx.mark.tracking_no.v[0];
			if(!(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_PROCESSED))
				SetWinUsrFlag(PROCESS_QUERY_REPLY);
		}
	}

	GetCtrlText( IDC_MSG_INPUT , str );// GetCtrlText trims spaces on both side
	if( str.Length() > 0 )
	{
		if(user_data->addr.sidFlags.SidMode() == SONORK_SID_MODE_BUSY )
		{
			if(MessageBox(GLS_MS_UBUSY,GLS_OP_SNDMSG,MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)
				return;
		}


		context.tmp_msg.SetStr( 0 , str.CStr() );
		SonorkApp.PrepareMsg(handle
				,&context.tmp_msg
				,0				 // sys_flags
				,send_mode == SEND_MODE_QUERY
					?SONORK_MSG_UF_QUERY	// usr_flags
					:0
				,0				// pc_flags
				,reply_tracking_no
				,NULL
				);
		if(SonorkApp.SendMsgUser(handle
			,this
			,user_data
			,&context.tmp_msg
			)==SONORK_RESULT_OK)
		{
			PostPoke( SONORK_MSG_WIN_POKE_AFTER_SEND , true );
		}
		return;
	}
	else
		PostPoke( SONORK_MSG_WIN_POKE_AFTER_SEND , false );
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkMsgWin::IsToolBarButtonChecked(TOOL_BAR_BUTTON b)
{
	return ToolBar_GetButtonState(toolbar.hwnd,b) & TBSTATE_CHECKED;

}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::SetToolBarButtonEnabled(TOOL_BAR_BUTTON b,BOOL e)
{
	SetToolBarButtonState(b,e?TBSTATE_ENABLED:0);
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::SetToolBarButtonState(TOOL_BAR_BUTTON b,UINT state)
{
	ToolBar_SetButtonState(toolbar.hwnd,b,state);
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::OnToolTipText(UINT id, HWND  , TSonorkWinToolTipText&TTT )
{
	SONORK_C_CSTR	ctrl = SonorkApp.LangString(GLS_LB_KBCTL);
	SONORK_C_CSTR	str1,str2;

	if( id == (UINT)btn.send )
	{
		str1 = SonorkApp.LangString(GLS_OP_SNDMSG);
		str2 = SonorkApp.LangString(GLS_LB_KBRET);
		if(SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_SEND_WITH_ENTER))
			wsprintf(TTT.str,"%s (%s)",str1,str2);
		else
			wsprintf(TTT.str,"%s (%s+%s)",str1,ctrl,str2);
	}
	else
	if( id == (UINT)btn.reply )
		wsprintf(TTT.str
			,"%s (%s+R)"

			,SonorkApp.LangString(GLS_OP_RPLMSG)
			,ctrl
			);
	else
	if( id == (UINT)btn.query )
		wsprintf(TTT.str
			,"%s (%s+Q)"
			,SonorkApp.LangString(GLS_LB_QUERYMSG)
			,ctrl
			);
	else
	switch(id)
	{

		case TOOL_BAR_USER_INFO:
			TTT.gls = GLS_LB_USRINFO;
			break;

		case TOOL_BAR_ULTRA_MINIMIZE:
			TTT.gls = GLS_OP_UMINSND;
			break;

		case TOOL_BAR_TEMPLATES:
			TTT.gls = GLS_LB_TMPLTS;
			break;

		case TOOL_BAR_USER_APPS:
			TTT.gls = GLS_LB_APPS;
			break;

		case TOOL_BAR_NEXT_UNREAD:
			sprintf(TTT.str,"%s (%s+%s)"
				,SonorkApp.LangString(GLS_OP_FSTNRDMSG)
				,ctrl
				,SonorkApp.LangString(GLS_LB_KBSPC));
			break;

		case TOOL_BAR_SEND_FILE:
			sprintf(TTT.str,"%s (%s+U)",SonorkApp.LangString(GLS_OP_PUTFILE),ctrl);
			break;

		case TOOL_BAR_SCROLL_LOCK:
			sprintf(TTT.str,"%s (F11)",SonorkApp.LangString(GLS_OP_PINMSG));
			break;

		case TOOL_BAR_STAY_ON_TOP:
			TTT.gls = GLS_OP_STAYTOP;
			break;


		case TOOL_BAR_SEARCH:
			sprintf(TTT.str,"%s (%s+F)",SonorkApp.LangString(GLS_OP_SEARCH),ctrl);
			break;
	}
}

bool
 TSonorkMsgWin::OnAppEventMsg_Store( TSonorkAppEventMsg* E)
{
	TSonorkMsg *msg;
	bool	i_am_the_fg_window;
	DWORD	cc_flags;
	DWORD	pcFlags;

	msg = E->msg;
	// Check if the event has not been consumed, it is a message
	// that may be handled by the application and the message belongs to our user
	if(    (E->pcFlags & SONORK_APP_MPF_PROCESSED)
		|| !(E->header->userId == user_data->userId))
		return false;

	if( !IS_SONORK_APP_SERVICE_ID(E->header->target.ServiceId()) )
		return false;

	pcFlags = (E->pcFlags|=SONORK_APP_MPF_PROCESSED);

	if( pcFlags&SONORK_APP_MPF_DONT_STORE )
	{
		// return true, but don't store
		return true;
	}

	i_am_the_fg_window = !IsIconic() && IsActive() && !IsUltraMinimized();
	cc_flags  = E->ccFlags;
	if( cc_flags & SONORK_APP_CCF_INCOMMING )
	{
		// Incomming message
		cc_flags|=SONORK_APP_CCF_UNREAD;	// Unread by default
		if( i_am_the_fg_window)
		{
			pcFlags|=SONORK_APP_MPF_SOFT_SOUND;
			if(	GetOutputMode() == SONORK_CONSOLE_OUTPUT_HISTORY
				&& msg->DataServiceType()==SONORK_SERVICE_TYPE_NONE)
			{
				// If I'm the FG window and the message is a short text,
				// enable the AUTO_READ_SHORT flag because the user can read it
				// right from the history window without having to open it.
				// The AUTO_READ_SHORT flag automatically clears the
				// UNREAD flag if the LONG flag is not set.
				// The LONG flag is set for those messages that are
				// cutted to fit the maximum line length in HISTORY_MODE
				pcFlags|=SONORK_APP_MPF_AUTO_READ_SHORT;
			}
		}
	}
	else
	{
		// Outgoing, mark as read by default
		cc_flags &=~(SONORK_APP_CCF_UNREAD);
		if( IsToolBarButtonChecked(TOOL_BAR_ULTRA_MINIMIZE)
		&& user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)==0)
			UltraMinimize();
	}
	if( SonorkApp.StoreMsg( context.cache
			, msg
			, pcFlags
			, cc_flags
			, user_data
			, E->mark) == SONORK_RESULT_OK)
	{
		E->pcFlags|=SONORK_APP_MPF_STORED;
		context.console->AfterAdd();

		PostPoke(SONORK_MSG_WIN_POKE_NEW_MSG
			,cc_flags & SONORK_APP_CCF_INCOMMING);
	}
	UpdateStatusBar( SONORK_MSG_WIN_F_UPDATE_SB_LINE_COUNT , NULL );
	return true;
}

bool
 TSonorkMsgWin::OnAppEventMsg_Sent( TSonorkAppEventMsg* E)
{
	if(    !(E->pcFlags & SONORK_APP_MPF_PROCESSED)
		&& E->header->userId == user_data->userId)
	{
		E->pcFlags |= SONORK_APP_MPF_PROCESSED;
		if( E->pcFlags&SONORK_APP_MPF_STORED )
		{
			if(SonorkApp.SetMsgTag(user_data
					, context.cache
					, &E->mark
					, E->ccFlags
					, SONORK_APP_CCF_UNSENT|SONORK_APP_CCF_ERROR
					, NULL))
			{
				E->pcFlags |= SONORK_APP_MPF_SUCCESS;
				context.console->PaintViewLine(E->mark.line_no);
			}
		}
		else
			E->pcFlags |= SONORK_APP_MPF_SUCCESS;
		return true;
	}
	return false;
}

bool
 TSonorkMsgWin::OnAppEventMsgProcessed(TSonorkAppEventMsgProcessed* E)
{
	if(!(E->pcFlags & SONORK_APP_MPF_PROCESSED)
	&& E->userId == user_data->userId)
	{
		E->pcFlags |= SONORK_APP_MPF_PROCESSED;
		if(E->pcFlags & SONORK_APP_MPF_STORED)
		{
			if(SonorkApp.SetMsgTag(user_data
					, context.cache
					, E->markPtr
					, E->ccFlags
					, E->ccMask
					, E->extIndex))
			{
				E->pcFlags |= SONORK_APP_MPF_SUCCESS;
				context.console->PaintViewLine(E->markPtr->line_no);
			}
		}
		else
			E->pcFlags |= SONORK_APP_MPF_SUCCESS;
		return true;
	}
	return false;
}


bool
 TSonorkMsgWin::OnAppEvent(UINT event, UINT param,void*data)
{
	union {
		void*				ptr;
		TSonorkAppEventSetUser*		set_user;
		TSonorkAppEventUserSid*		user_sid;
		TSonorkAppEventMsg*		msg;
		TSonorkAppEventMsgProcessed*	msgProcessed;
		TSonorkId*			userId;
	}E;
	DWORD	aux;
	E.ptr=(void*)data;
	switch( event )
	{
		case SONORK_APP_EVENT_OPEN_PROFILE:
			if( param == false)Destroy();
			return true;

		case SONORK_APP_EVENT_USER_MTPL_CHANGE:
			UpdateMtplButtons();
			return true;

		case SONORK_APP_EVENT_SKIN_COLOR_CHANGED:
			::InvalidateRect(input.hwnd,NULL,true);
			if(ultra_min.win)
				ultra_min.win->InvalidateRect(NULL,false);
			return true;

		case SONORK_APP_EVENT_SONORK_MSG_RCVD:
		case SONORK_APP_EVENT_SONORK_MSG_SENDING:
			return OnAppEventMsg_Store( E.msg);

		case SONORK_APP_EVENT_SONORK_MSG_SENT:
			return OnAppEventMsg_Sent(E.msg);

		case SONORK_APP_EVENT_SONORK_MSG_PROCESSED:
			return OnAppEventMsgProcessed(E.msgProcessed);

		case SONORK_APP_EVENT_SET_USER:
			if(E.set_user->user_data->userId == user_data->userId )
			{
				if(param & SONORK_APP_EVENT_SET_USER_F_DISPLAY_ALIAS )
					aux = SONORK_MSG_WIN_F_UPDATE_UI_CAPTION;
				else
					aux = 0;
				if(param & SONORK_APP_EVENT_SET_USER_F_MSG_COUNT )
				{
					// Update Icon only where msg_count
					// transtions to/from 0 (0->1 or 1->0)
					if(user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)<=1)
						aux |= SONORK_MSG_WIN_F_UPDATE_UI_ICON
						    |  SONORK_MSG_WIN_F_UPDATE_UI_MSG_COUNT;
					UpdateStatusBar(0,NULL);
				}
				if( aux )
				{
					UpdateUserInterface(aux);
				}
			}
			return true;
		case SONORK_APP_EVENT_USER_SID:
			if( param == SONORK_APP_EVENT_USER_SID_LOCAL)
			{
				if(E.user_sid->local.userData->userId == user_data->userId )
				{
					UpdateUserInterface(
						 SONORK_MSG_WIN_F_UPDATE_UI_CAPTION
						|SONORK_MSG_WIN_F_UPDATE_UI_ICON
						|SONORK_MSG_WIN_F_UPDATE_UI_SID_TEXT);
					SonorkApp.UTS_ConnectByUserData( user_data , false );
				}
			}
			break;
		case SONORK_APP_EVENT_DEL_USER:
			{
				if( *E.userId == user_data->userId )
				{
					Destroy(IDCANCEL);
				}
			}
			return true;

		case SONORK_APP_EVENT_FLUSH_BUFFERS:
			context.cache->Flush(false,true);
			return true;

	}
	return false;
}


LRESULT
 TSonorkMsgWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	UINT 	aux1,aux2;
	HWND	other_win_handle;
	switch( wParam )
	{
		case  SONORK_WIN_POKE_ULTRA_MIN_DESTROY:
			if(!TestWinSysFlag(SONORK_WIN_SF_DESTROYING))
			{
				SetWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);
				ultra_min.x = ultra_min.win->Left();
				ultra_min.y = ultra_min.win->Top();
				ultra_min.win=NULL;
				SonorkApp.PostAppCommand(
					 SONORK_APP_COMMAND_FOREGROUND_HWND
					,(LPARAM)Handle());
				
			}
			break;
			
		case SONORK_WIN_POKE_ULTRA_MIN_PAINT:
			UltraMinimizedPaint((TSonorkUltraMinPaint*)lParam);
			break;


		case SONORK_MSG_WIN_POKE_AFTER_SEND:
			if(lParam)
			{
				SetCtrlText( IDC_MSG_INPUT,(char*)NULL);
				if(!IsToolBarButtonChecked(TOOL_BAR_SCROLL_LOCK))
					context.console->CloseView();
				if( TestWinUsrFlag(PROCESS_QUERY_REPLY) )
				{
					if(SonorkApp.SetMsgTag(user_data
								,context.cache
								,&send_ctx.mark
								,SONORK_APP_CCF_PROCESSED
								,SONORK_APP_CCF_PROCESSED|SONORK_APP_CCF_UNREAD
								,NULL))
					{
						context.console->PaintViewLine( send_ctx.mark.line_no );
					}
				}
				context.tmp_msg.Clear();
				send_ctx.mark.Clear();
				if( ++sent_messages_counter > CHECK_UTS_MESSAGE_COUNT )
				{
					SonorkApp.UTS_ConnectByUserData( user_data , false );
					sent_messages_counter=0;
				}
			}
			if( IsToolBarButtonChecked(TOOL_BAR_ULTRA_MINIMIZE)
			&& user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)==0)
			{
				UltraMinimize();
				break;
			}
			FocusInputWin();
		break;


		case SONORK_MSG_WIN_POKE_OPEN_INIT:
			// lParam = <SONORK_MSG_WIN_OPEN_MODE>

			other_win_handle=NULL;
			if( lParam != SONORK_MSG_WIN_OPEN_FOREGROUND )
                        {
                        	// if not FOREGROUND, try to keep current
                                // Sonork window in foreground
                                if( sonork_active_win != NULL )
                                if( sonork_active_win->WinType() != SONORK_WIN_TYPE_MAIN )
                                {
                                        // Must save <other_win_handle> because
                                        // ShowWindow() might change <sonork_active_win>
                                        other_win_handle=sonork_active_win->Handle();
                                }
			}

			ShowWindow(lParam==SONORK_MSG_WIN_OPEN_MINIMIZED
				?SW_SHOWMINNOACTIVE
				:SW_SHOW);

#define START_LINE_NO	aux1
			if( !user_data->RunFlags().Test(SONORK_URF_CONSOLE_INIT)
			|| user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT)>context.console->Lines() )
			{

				// FILE RESCAN
				//  If the CONSOLE_INIT run flag is not yet set
				//  we rescan the last section of the message
				//  file looking for unread messages; we do this
				//  only once after the profile is open to make
				//  sure (or at least sure-er) that the user's
				//  unread messages counter is synchronized with
				//  the contents of the message file.
				// Also, everytime the message file is open,
				//  we update the message information bar (time, ID)
				//  because normally we update the message information bar
				//  everytime the history window generates EVENT_SEL_CHANGED,
				//  but this event is not generated when the file is first open
				//  even though there could be a selected line.
				user_data->RunFlags().Set(SONORK_URF_CONSOLE_INIT);

				//  16 context.cache reads (About 1024 lines)
				if(context.cache->Lines()>TSonorkWin32App::MSG_CONSOLE_CACHE_SIZE*16)
					START_LINE_NO=context.cache->Lines()-(TSonorkWin32App::MSG_CONSOLE_CACHE_SIZE*4);
				else
					START_LINE_NO=0;

				RescanMsgFile(START_LINE_NO);
			}

#undef START_LINE_NO
                        if( lParam!=SONORK_MSG_WIN_OPEN_MINIMIZED )
			{
				if(!FocusNextUnreadMsg(false))
					ShowSidMsg(true,false);
			}
			if( !TestWinUsrFlag(HAS_FOCUSED_A_MESSAGE) )
				OnHistoryWin_FocusChanged();
			EnableWindow(true);
			if( lParam!=SONORK_MSG_WIN_OPEN_MINIMIZED )
			{
				if(!other_win_handle)
				{
					PostPoke(SONORK_MSG_WIN_POKE_FOCUS_INPUT_WIN,0);
				}
				else
				{
					// Keep the old window active
					SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOREGROUND_HWND
						, (LPARAM)other_win_handle);
				}
				SonorkApp.UTS_ConnectByUserData( user_data , false );
			}

		break;



		case SONORK_MSG_WIN_POKE_NEW_MSG:
#define CAN_FOCUS_NEW_MSG		aux2

			CAN_FOCUS_NEW_MSG=false;
			ClearWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);

			if( lParam ) // if (lParam) then it is an Incomming message
			{
				if( SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_POPUP_MSG_WIN) )
				{
					// Profile flags indicate that we should
					// auto-open new messages.
					// Must save <other_win_handle> because
					// ShowWindow() / SetForegound()
					// might change <sonork_active_win>

					other_win_handle=NULL;
					if( !IsActive() )
					if( sonork_active_win != NULL)
					if( sonork_active_win->WinType() != SONORK_WIN_TYPE_MAIN )
						other_win_handle=sonork_active_win->Handle();

					if( other_win_handle != NULL )
					{
						// Another Sonork window is
						// active: Just restore to
						// background or blink

						if( IsIconic() )
						{
							// Don't Blinking
							// and don't focus,
							// just restore to background
							ShowWindow(SW_SHOWNOACTIVATE);
							SetActiveWindow(other_win_handle);
							SetWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);
							break;
						}

					}
					else
					{
//						TRACE_DEBUG(" : NONE_OTHER_ACTIVE");
						// No another sonork window is active:
						// we must popup
						if( !IsUltraMinimized() )
						{
							if( IsIconic() )
							{
								CAN_FOCUS_NEW_MSG=true;
								ShowWindow(SW_RESTORE);
								PostPoke(SONORK_MSG_WIN_POKE_FOCUS_INPUT_WIN,true);
							}
							else
							if( !IsActive() )
							{
								CAN_FOCUS_NEW_MSG=true;
								::SetForegroundWindow(Handle());
							}
						}
						// Set lParam to 0 so next
						// piece of code does not blink.
						lParam=0;
					}
				}
				else
				{
					lParam = ( IsIconic() || !IsActive() );
				}

				if( lParam  )
				{
					SetWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);
					BlinkWindow();
					CAN_FOCUS_NEW_MSG=false;
				}

			}

			// Outgoing, or incomming with lParam==0

			if( !IsToolBarButtonChecked(TOOL_BAR_SCROLL_LOCK) )
			{
				if( CAN_FOCUS_NEW_MSG )
				{
					FocusNextUnreadMsg(false);
				}
				else
				{
					context.console->MakeLastLineVisible();
				}
			}
#undef CAN_FOCUS_NEW_MSG
		break;

		case SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD:
			ClearWinUsrFlag(FOCUS_UNREAD_ON_ACTIVATE);
			HideSidMsg();
			if( IsIconic() )
			{
				ShowWindow(SW_RESTORE);
				SetForegroundWindow ( Handle() );
			}
			else
			if( IsUltraMinimized() )
			{
				ultra_min.win->Destroy();
			}
			else
			if( !IsActive() )
			{
				::SetForegroundWindow(Handle());
			}
			FocusNextUnreadMsg( lParam != 0);
			break;

		case SONORK_MSG_WIN_POKE_OPEN_SID_MSG:
			ShowSidMsg( !TestWinUsrFlag(SHOWING_SID_MSG) , true);
			break;

		case SONORK_MSG_WIN_POKE_FIND_LINKED:
			FindLinked();
			break;

		case SONORK_MSG_WIN_POKE_FOCUS_INPUT_WIN:
			FocusInputWin();
			break;

	}
	return 0;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkMsgWin::CmdProcess( DWORD process_flags )
{
	UINT type=context.console->GetToolBarUserFlags();
	if( type == SONORK_MSG_WIN_PROCESS_URL )
	{
		SonorkApp.OpenUrl(this , context.cur_msg.ToCStr() );
		return process_flags;
	}
	else
	if( type == SONORK_MSG_WIN_PROCESS_FILE )
	{
		return CmdProcessFile(process_flags);
	}
	else
	if( type == SONORK_MSG_WIN_PROCESS_MSG_TEMPLATE )
	{
		return CmdProcessMsgTemplate(process_flags);
	}
	return 0;

}

// ----------------------------------------------------------------------------

DWORD
 TSonorkMsgWin::CmdProcessFile( DWORD process_flags )
{
	TSonorkCCacheEntry 	*	CL;
	DWORD			 	line_no;
	UINT			 	flags;
	SONORK_C_CSTR		 	file_ext;
	char				tmp[64];
	TSonorkShortString		file_name;
	SaveMsgFileDialogHookData	HD;
	static const char *no_open_extensions[]=
	{"exe","com","vbs","bat","cmd","js","pif",NULL};
	CL = context.console->GetFocused( NULL , &line_no );
	if( CL == NULL )
		return 0;

	if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_DELETED) )
		return 0;

	if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING )
	&& !(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_PROCESSED ) )
	{
		TSonorkFileTxGui 		*W;
		TSonorkCCacheMark		mark;
		TSonorkShortString		ss_path;
		mark.Set( line_no, CL );
		if( SonorkApp.IsMsgLocked( user_data->userId , mark ) )
		{
			MessageBox(GLS_MS_ILOCKD,(char*)NULL,MB_OK|MB_ICONSTOP);
			return false;
		}
		if( process_flags& SONORK_CONSOLE_TBF_PROCESS )
		{
			TSonorkWin32App::GetLoadSaveDialog GLSD;

			TSonorkFileTxEng::GetUncompressedSize(context.file_info,file_name);

			GLSD.id 	= IDD_GETFILE;
			GLSD.hook	= SaveMsgFileDialogHook;
			GLSD.data	= (DWORD)&HD;
			HD.action	= SonorkApp.ProfileCtrlValue(SONORK_PCV_DOWNLOAD_FILE_ACTION);
			HD.enable_open_file=true;
			file_ext = SONORK_IO_GetFileExt(file_name.CStr());
			if( *file_ext=='.' )
			{
				file_ext++;
				for(UINT i=0;no_open_extensions[i]!=NULL;i++)
					if(!stricmp(file_ext,no_open_extensions[i]) )
					{
						HD.enable_open_file=false;
						break;
					}
			}
			wsprintf(tmp
				,"Dld%x%x"
				,user_data->userId.v[0]
				,user_data->userId.v[1]);
			if(!SonorkApp.GetSavePath(Handle()
				, ss_path
				, file_name.CStr()
				, GLS_OP_STORE
				, tmp
				, NULL
				,  OFN_EXPLORER
				 | OFN_LONGNAMES
				 | OFN_NOCHANGEDIR
				 | OFN_NOREADONLYRETURN
				 | OFN_HIDEREADONLY
				 | OFN_PATHMUSTEXIST
				 | OFN_OVERWRITEPROMPT
				,&GLSD))
				{
					if(HD.action == IDC_GETFILE_DELETE )
						flags=SONORK_FILE_TX_F_DELETE_FILE;
					else
						return process_flags;
				}
				else
				{
					if( HD.action == IDC_GETFILE_ACT_OPEN_FOLDER )
						flags =  SONORK_FILE_TX_FV_OPEN_FOLDER;
					else
					if( HD.action == IDC_GETFILE_ACT_OPEN_FILE )
						flags =  SONORK_FILE_TX_FV_OPEN_FILE;
					else
						flags =  0;
					SonorkApp.ProfileCtrlValue(SONORK_PCV_DOWNLOAD_FILE_ACTION) = HD.action;
				}
		}
		else
			flags=SONORK_FILE_TX_F_DELETE_FILE;

		W=new TSonorkFileTxGui(
			 user_data->display_alias.CStr()
			,ss_path.CStr()
			,mark
			,context.file_info
			,flags);
		if(!W->Create())
		{
			delete W;
			return 0;
		}
	}
	else
	{
		SonorkApp.OpenCCacheFile( this , CL , &context.file_info);
	}
	return process_flags;
}

// ----------------------------------------------------------------------------

DWORD
 TSonorkMsgWin::CmdProcessMsgTemplate(DWORD )
{
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::CmdSelectThread()
{
	DWORD line_no,cc,aux;
	TSonorkCCacheEntry *	CL;
	CL = context.console->GetFocused( NULL , &line_no );
	if( CL != NULL )
	{
		context.cache->ClearLinkContext();
		// Force first time go to top
		for(cc=0
			;(CL = context.cache->GetLinked(
				  line_no
				, CL
				, aux
				, NULL
				, cc==0	// First time round: Find top of thread
				?SONORK_CCACHE_GET_LINKED_TOP_OF_THREAD
				:SONORK_CCACHE_GET_LINKED_FORWARD_ONLY
				)) != NULL
			 && cc<SONORK_CCACHE_MAX_SELECTION_SIZE
			;cc++)
		{
			line_no = aux;
			if( context.console->Selection().Contains(line_no) )
			{
				//We've wrapped
				break;
			}
			context.console->AddSelection(line_no);
		}
	}
}



// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::FindLinked()
{
	if( context.console->FocusLinked(false) )
	{
		if(GetOutputMode()==SONORK_CONSOLE_OUTPUT_VIEW)
			OpenMsgWithFlags( 0 );
	}
	else
	{
		SetMsgWinStatus(status.hwnd
			, MSG_WIN_SB_HINT
			, SonorkApp.LangString(GLS_MS_NLNKMSG)
			, SKIN_HICON_INFO);
	}
}

// ----------------------------------------------------------------------------

void TSonorkMsgWin::CmdSearch()
{
	TSonorkMsgFilterWin *W=new TSonorkMsgFilterWin(this);
	if(!W->Create())
		delete W;
	else
	    W->ShowWindow(SW_SHOW);
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::CmdScrollLock( bool update_button )
{
	if( update_button )
	{
		DWORD aux;
		aux=!IsToolBarButtonChecked(TOOL_BAR_SCROLL_LOCK);
		SetToolBarButtonState(TOOL_BAR_SCROLL_LOCK,TBSTATE_ENABLED|(aux?TBSTATE_CHECKED:0));
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::CmdDeletePrevious()
{
	if(MessageBox(GLS_MW_DELPRV
		, GLS_LB_CONFIRM_OP
		, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)
		return;
	context.console->DelPreviousLines(context.console->FocusedLine());
}
void
 TSonorkMsgWin::CmdDeleteAll()
{
	if(MessageBox(GLS_MW_DELALL
		, GLS_LB_CONFIRM_OP
		, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)
		return;
	context.console->DelAllLines();
}

bool	TSonorkMsgWin::OnCommand(UINT id,HWND hwndSender, UINT code)
{
	DWORD 			aux;
	bool			is_a_valid_menu_cmd;
	TSonorkListIterator 	I;
	TSonorkCCacheEntry *CL;
	if( hwndSender == input.hwnd ) // Input
		return false;
	HideSidMsg();
	if( hwndSender == NULL ) // Menu
	{
		switch( id )
		{
			case CM_MSGS_TOGGLE_SEL:
				context.console->SelectToggleFocused();
				break;
				
			case CM_MSGS_CLEAR_SEL:
				context.console->ClearSelection();
				break;

			case CM_DELETE:
				context.console->DelSelectedLines();
				break;

			case CM_DELETE_PREVIOUS:
				CmdDeletePrevious();
				break;

			case CM_DELETE_ALL:
				CmdDeleteAll();
				break;

			case CM_PROTECT:
			if(!context.console->SelectionActive())
			{

				CL = context.console->GetFocused( NULL, &aux );

				if( CL != NULL )
				{
					CL->tag.v[SONORK_CCACHE_TAG_FLAGS]^=SONORK_APP_CCF_PROTECTED;
					context.console->Set(aux,CL->tag,NULL);
				}
			}

			else

			{

				context.console->InitEnumSelection(I);

				while( (aux = context.console->EnumNextSelection(I)) != SONORK_INVALID_INDEX)

				{

					CL = context.cache->Get(aux, NULL, NULL , SONORK_CCACHE_SD_RANDOM);

					if( CL != NULL )
					{
						CL->tag.v[SONORK_CCACHE_TAG_FLAGS]^=SONORK_APP_CCF_PROTECTED;

						context.console->Set(aux,CL->tag,NULL);

					}

				}

			}

			break;


			case CM_MSGS_SEL_THREAD:
			{
				if( !SonorkApp.IsControlKeyDown() )
					context.console->ClearSelection();
				CmdSelectThread();
			}
			break;

			case CM_CLIPBOARD_COPY:
				context.console->CopyToClipboard(0);
				break;

			case CM_EXPORT:
				CmdExport(context.console);
				break;

			case CM_MSGS_SINGLE_LINE_MODE:
				context.console->SetMaxScanLines(
					context.console->GetMaxScanLines() == 1
					?DEFAULT_SCAN_LINES
					:1);
				break;

			default:
				SonorkApp.LaunchAppByCmd( id , user_data , &is_a_valid_menu_cmd);
				if(is_a_valid_menu_cmd)
					break;
					
				if( id == CM_WAPP_OPEN || id >=SONORK_APP_CM_MTPL_BASE && id < SONORK_APP_CM_MTPL_LIMIT )
				{
					TSonorkCCacheMark 	mark,*pMark;
					if(TestWinUsrFlag(REPLY_TEMPLATE))
					{
						TSonorkCCacheEntry *CL;
						CL = context.console->GetFocused(NULL,&aux);
						if(CL == NULL)break;
						mark.Set(aux,CL);
						pMark=&mark;
					}
					else
						pMark=NULL;
					SonorkApp.LaunchWebAppByCmd(this,id,user_data,pMark);
				}
				return false;
		}
		return true;
	}
	else
	if( hwndSender == separator )
	{
		if(code == STN_CLICKED && !TestWinUsrFlag(DIGEST_MODE))
		{
			SetResizingInput( true );
		}
	}
	else
	if(code == BN_CLICKED)
	{
		if( id >= TOOL_BAR_FIRST && id<=TOOL_BAR_LAST )
			OnToolBarButton( id );
		else
		if( id == IDC_MSG_REPLY
		||	id == IDC_MSG_SEND
		||	id == IDC_MSG_QUERY)
			CmdSendMessage( (SEND_MODE)id );
		return true;
	}
	return false;
}
void TSonorkMsgWin::OnToolBarButton(UINT id)
{
	POINT	pt;
	HideSidMsg();
	switch(id)
	{
		case TOOL_BAR_SEARCH:
			CmdSearch();
			break;

		case TOOL_BAR_USER_APPS:
			SonorkApp.UpdateUserExtAppMenu( user_data );
			::GetCursorPos(&pt);
			TrackPopupMenu(SonorkApp.UserAppsMenu()
					, TPM_LEFTALIGN  | TPM_LEFTBUTTON
					,pt.x
					,pt.y
					,0
					,Handle()
					,NULL);
			break;

		case TOOL_BAR_USER_INFO:
			SonorkApp.OpenUserDataWin(user_data,NULL,TSonorkUserDataWin::TAB_INFO);
			break;

		case TOOL_BAR_ULTRA_MINIMIZE:
			if( IsToolBarButtonChecked(TOOL_BAR_ULTRA_MINIMIZE) )
				UltraMinimize();
			break;
			
		case TOOL_BAR_TEMPLATES:
			ShowMtplMenu(false);
			break;

		case TOOL_BAR_SEND_FILE:
			CmdSendFile();
			break;

		case TOOL_BAR_SCROLL_LOCK:
			CmdScrollLock( false );
			break;

		case TOOL_BAR_STAY_ON_TOP:
			SetStayOnTop(IsToolBarButtonChecked(TOOL_BAR_STAY_ON_TOP));
			break;


		case TOOL_BAR_NEXT_UNREAD:
			FocusNextUnreadMsg( true );
			break;
	}
}




void TSonorkMsgWin::OpenMsgWithFlags( DWORD p_flags )
{
	TSonorkCCacheEntry *	CL;
	DWORD			line_flags;
	CL= LoadCurrentMsg( context , line_flags , status.hwnd);
	;
	if( CL != NULL )
	{
		if( context.line_flags & SONORK_APP_CCF_UNREAD )
		if( !(context.line_flags & SONORK_APP_CCF_NO_READ_ON_OPEN)	)
			MarkLineAsRead(context.line_no,CL);

		p_flags&=line_flags;
		if( p_flags & (SONORK_CONSOLE_TBF_PROCESS|SONORK_CONSOLE_TBF_DELETE))
		{
			p_flags = CmdProcess( p_flags );
			if( (p_flags&(SONORK_CONSOLE_TBF_PROCESS|SONORK_CONSOLE_TBF_DELETE)) )
			{
				context.console->CloseView();
				return;
			}
			line_flags=p_flags;
		}
		context.console->OpenView( line_flags );
	}
}

TSonorkCCacheEntry *TSonorkMsgWin::LoadCurrentMsg(
		TSonorkMsgWinContext& 	CTX
	, 	DWORD&			process_flags
	, 	HWND			status_hwnd)
{
	TSonorkCCacheEntry *		CL;
	SONORK_RESULT	 		result;
	char		 		size_str[16],time_stamp[64],*ptr;

	CL = CTX.console->GetFocused( &CTX.default_text , &CTX.line_no );
	if( CL == NULL )
	{
		if(CTX.console->Lines())
		{
			SetMsgWinStatus(status_hwnd
			, MSG_WIN_SB_HINT
			, SonorkApp.SysString(GSS_FILERDERR)
			, SKIN_HICON_ALERT);
		}
		CTX.console->CloseView();
		return NULL;
	}
	result = SonorkApp.GetMsg( CL->dat_index , &CTX.cur_msg );
	if( result != SONORK_RESULT_OK )
	{
		SetMsgWinStatus(status_hwnd
			, MSG_WIN_SB_HINT
			, SonorkApp.SysString(GSS_FILERDERR)
			, SKIN_HICON_ALERT);

		CTX.console->CloseView();
		return NULL;
	}

	CTX.file_info.CODEC_Clear();
	CTX.line_flags		= CL->tag.v[SONORK_CCACHE_TAG_FLAGS];
	CTX.default_text 	= CTX.cur_msg.ToCStr();
	process_flags	 	= 0;
	switch( CTX.cur_msg.DataServiceType() )
	{
		case SONORK_SERVICE_TYPE_SONORK_FILE:
		{
			if( CTX.file_info.CODEC_Read(&CTX.cur_msg.ExtData()) == SONORK_RESULT_OK )
			{
				TSonorkTempBuffer buffer(CTX.file_info.name.Length() + 256 + SONORK_MAX_PATH);
				int len;
				TSonorkWin32App::MakeTimeStr( CTX.file_info.attr.modify_time
					,time_stamp
					,MKTIMESTR_DATE|MKTIMESTR_TIME);
				ptr = DottedValue(size_str
					,CTX.file_info.attr.orig_size.v[0]
					,NULL);
				len=sprintf(buffer.CStr()
					,"%s\r\n%s: %s bytes\r\n%s:%s"
					,CTX.file_info.name.CStr()
					,SonorkApp.LangString(GLS_LB_SIZE)
					,ptr
					,SonorkApp.LangString(GLS_LB_MODIFIED)
					,time_stamp);

				process_flags = SONORK_MSG_WIN_PROCESS_FILE;
				if( !(CTX.line_flags & (SONORK_APP_CCF_PROCESSED|SONORK_APP_CCF_DELETED)) )
				{
					if( CTX.line_flags & SONORK_APP_CCF_INCOMMING )
					{
						process_flags |=
							SONORK_CONSOLE_TBF_PROCESS
							|SONORK_CONSOLE_TBF_DELETE
							|SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS;
					}


				}
				else
				{
					if(!(CTX.line_flags & SONORK_APP_CCF_DELETED) )
						process_flags|= SONORK_CONSOLE_TBF_PROCESS;

					if( CTX.line_flags & SONORK_APP_CCF_INCOMMING )
					{
						len+=sprintf(buffer.CStr()+len,"\r\n");
						len+=SonorkApp.LangSprintf(buffer.CStr() + len
							,	GLS_MS_FILESTAT
							,	SonorkApp.LangString(
								(CTX.line_flags & SONORK_APP_CCF_DELETED)
								?GLS_WO_DELETED:GLS_WO_DOWNLOADED));
					}
				}
				if(!(CTX.line_flags & SONORK_APP_CCF_DELETED) && CL->ExtIndex()!=SONORK_INVALID_INDEX)
				if( (CTX.line_flags & SONORK_APP_CCF_PROCESSED)	|| !(CTX.line_flags & SONORK_APP_CCF_INCOMMING))
				{
					TSonorkShortString dld_path;
					if(SonorkApp.GetExtData(CL->ExtIndex()
						,&dld_path,NULL)==SONORK_RESULT_OK)
						sprintf(buffer.CStr()+len
						,"\r\n%s"
						,dld_path.CStr());
				}
				CTX.console->SetOutputText( buffer.CStr() );
				CTX.default_text=NULL;
			}
			else
			{
				SetMsgWinStatus(status_hwnd
					, MSG_WIN_SB_HINT
					, SonorkApp.SysString(GSS_FILERDERR)
					, SKIN_HICON_ALERT);

			}
		}
		break;
		case SONORK_SERVICE_TYPE_URL:
		{
			process_flags=SONORK_CONSOLE_TBF_PROCESS
					|SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS
					|SONORK_MSG_WIN_PROCESS_URL;
		}
		break;

		case SONORK_SERVICE_TYPE_NONE:
		default:
		break;
	}
	if(CTX.default_text)
		CTX.console->SetOutputText( CTX.default_text );
	CTX.console->SetToolBarFlags( process_flags );
	return CL;
}

void	TSonorkMsgWin::OnInitMenu(HMENU hmenu)
{
	if( hmenu == SonorkApp.MsgsPopupMenu() )
	{
		BOOL selection;
		UINT enable;
		TSonorkCCacheEntry *CL;
		char	tmp[96];
		selection=context.console->Selection().Active();
		enable=(selection?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND;
//		EnableMenuItem(hmenu,CM_DELETE,enable);
		EnableMenuItem(hmenu,CM_MSGS_CLEAR_SEL,enable);
		wsprintf(tmp,"%s (%s)"
			,SonorkApp.LangString(GLS_OP_EXPORT)
			,SonorkApp.LangString(selection?GLS_LB_SELTD:GLS_LB_ALL));
		TSonorkWin::SetMenuText(hmenu, CM_EXPORT, tmp);

		enable = MF_GRAYED;
		if((CL = context.console->GetFocused( NULL ))!=NULL)
			if(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & (SONORK_APP_CCF_THREAD_START|SONORK_APP_CCF_IN_THREAD))
				enable = MF_ENABLED;
		EnableMenuItem(hmenu,CM_MSGS_SEL_THREAD,MF_BYCOMMAND|enable);
		CheckMenuItem(hmenu,CM_MSGS_SINGLE_LINE_MODE
			,MF_BYCOMMAND|(context.console->GetMaxScanLines()==1?MF_CHECKED:MF_UNCHECKED));
	}
}




bool	TSonorkMsgWin::MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry *pEntry)
{
	if(SonorkApp.SetMsgTag(user_data
					,context.cache
					,line_no
					,pEntry
					,0
					,SONORK_APP_CCF_UNREAD
					,NULL))
	{
		context.console->PaintViewLine(line_no);
		return true;
	}
	return false;

}


void TSonorkMsgWin::RescanMsgFile(DWORD line_no)
{
	DWORD	first_unread,unread_count,lines;
	TSonorkCCacheEntry* CL;

	lines = context.cache->Lines();
	first_unread=unread_count = 0;

	if(lines)
	{
		if(lines > TSonorkWin32App::MSG_CONSOLE_CACHE_SIZE*4)
			SetHintModeLang(GLS_MS_PWAIT,true);
		for( ;;)
		{
			CL = context.cache->GetNext( line_no , SONORK_APP_CCF_UNREAD, SONORK_APP_CCF_UNREAD);
			if(!CL)break;
			if(unread_count == 0)
				first_unread = line_no;
			unread_count++;
			line_no++;
		}

		if(!unread_count)
			first_unread = lines-1;
		ClearHintMode();
	}
	SonorkApp.SetUnreadMsgCount( user_data , unread_count);
	user_data->CtrlValue(SONORK_UCV_FIRST_UNREAD)=first_unread;
}
BOOL
 TSonorkMsgWin::FocusNextUnreadMsg( BOOL next_window_if_not_found )
{
	DWORD line_no;
	TSonorkCCacheEntry* CL;

	ClearWinUsrFlag( FOCUS_UNREAD_ON_ACTIVATE );
	line_no = user_data->CtrlValue(SONORK_UCV_FIRST_UNREAD);

	CL = context.cache->GetNext( line_no
		,SONORK_APP_CCF_UNREAD
		,SONORK_APP_CCF_UNREAD);
	user_data->CtrlValue(SONORK_UCV_FIRST_UNREAD)=line_no;
	context.console->SetFocusedLine( line_no , false );

	if( CL==NULL)
	{
		if(next_window_if_not_found )
		{
			SonorkApp.OpenNextEvent( false );
		}
		return false;
	}
	return true;

}


void
 TSonorkMsgWin::CmdSendFile()
{
	TSonorkShortString file_path;

	if(TestWinUsrFlag(DIGEST_MODE))
		return ;

	::UpdateWindow(toolbar.hwnd);
	if( SonorkApp.GetFileNameForUpload(this,file_path) )
		SonorkApp.SendFile( user_data , file_path.CStr() );
}


void
 TSonorkMsgWin::SetupToolBar()
{
	static TSonorkWinToolBarButton	btn_info[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_USER_APPS
			, SKIN_ICON_APP
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON  }
	,	{ 	TOOL_BAR_TEMPLATES
			, SKIN_ICON_USER_TEMPLATE
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON  }
	,	{ 	TOOL_BAR_SEND_FILE	| SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_FILE_UPLOAD
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{ 	TOOL_BAR_USER_INFO
			, SKIN_ICON_USER_INFO
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{ 	TOOL_BAR_SEARCH   //		| SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_SEARCH
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
/*	,	{	TOOL_BAR_FIND_LINKED
			, SKIN_ICON_SEARCH_LINKED
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
*/
,		{	TOOL_BAR_STAY_ON_TOP        | SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_PIN
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK	}
	,	{ 	TOOL_BAR_ULTRA_MINIMIZE
			, SKIN_ICON_CLOSE_UP
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK  }
	,	{ 	TOOL_BAR_SCROLL_LOCK	| SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_NO_SCROLL
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK  }
	,	{ 	TOOL_BAR_NEXT_UNREAD
			, SKIN_ICON_EVENT
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	};

	toolbar.hwnd=
		TSonorkWin::CreateToolBar(
			Handle()
			,TOOL_BAR_ID
			,	 WS_VISIBLE
				| TBSTYLE_LIST
				| TBSTYLE_TOOLTIPS
				| TBSTYLE_FLAT
//				| WS_BORDER
				| CCS_NOPARENTALIGN
				| CCS_NODIVIDER
				| CCS_NORESIZE
			, TOOL_BAR_BUTTONS
			, btn_info
			, &toolbar.size);
}




UINT CALLBACK
 TSonorkMsgWin::SaveMsgFileDialogHook(
  HWND dlg_hwnd,      // handle to child dialog window
  UINT uMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
{
#define DLG_CONTROLS	5
	SaveMsgFileDialogHookData	*HD;
	UINT	i;
	HWND	tHwnd;
static struct
	{
		UINT 			id;
		GLS_INDEX       gls;
		BOOL			is_option;
	}ctrl[DLG_CONTROLS]
	=
	{{IDC_GETFILE_ACT_OPEN_FILE 	, GLS_DL_ADOFI	, true }
	,{IDC_GETFILE_ACT_OPEN_FOLDER 	, GLS_DL_ADOFO	, true }
	,{IDC_GETFILE_ACT_NOTHING	, GLS_DL_ADONO	, true }
	,{IDL_GETFILE_TITLE 		, GLS_DL_AFDLD	, false}
	,{IDC_GETFILE_DELETE 		, GLS_OP_DEL	, false}
	};

	if( uMsg == WM_COMMAND )
	{
		UINT id 	= LOWORD(wParam);
		UINT code 	= HIWORD(wParam);
		if( code == BN_CLICKED )
		{
			HD = (SaveMsgFileDialogHookData*)::GetWindowLong(dlg_hwnd,DWL_USER);
			if( HD!= NULL )
			switch( id )
			{
				case IDC_GETFILE_ACT_OPEN_FILE:
				case IDC_GETFILE_ACT_OPEN_FOLDER:
				case IDC_GETFILE_ACT_NOTHING:
				case IDC_GETFILE_DELETE:
					HD->action = id;
					if( id == IDC_GETFILE_DELETE )
					{
						HWND OFhwnd=GetParent(dlg_hwnd);
						::PostMessage(OFhwnd
							,WM_COMMAND
							,MAKEWPARAM(IDABORT,BN_CLICKED)
							,0);
						::PostMessage(OFhwnd
							,WM_COMMAND
							,MAKEWPARAM(IDCANCEL,BN_CLICKED)
							,(LPARAM)::GetDlgItem(OFhwnd,IDCANCEL));
					}
					break;
			}
		}
	}
	else
	if( uMsg == WM_DRAWITEM)
	{
		DRAWITEMSTRUCT*S=(DRAWITEMSTRUCT*)lParam;
		if( S->CtlID == IDC_GETFILE_ICON )
		{
			sonork_skin.DrawSign(S->hDC
				,SKIN_SIGN_FILE
				,S->rcItem.left,S->rcItem.top);
			return true;
		}
	}
	else
	if( uMsg == WM_INITDIALOG )
	{
		OPENFILENAME 	*OFN;
		OFN=(OPENFILENAME*)lParam;
		HD = (SaveMsgFileDialogHookData*)OFN->lCustData;
		::SetWindowLong(dlg_hwnd,DWL_USER,(DWORD)HD);

		for( i = 0 ; i < DLG_CONTROLS ; i++ )
		{
			if( HD->action == ctrl[i].id
			&&  ctrl[i].is_option)
				break;
		}
		if( i==DLG_CONTROLS)
			HD->action=IDC_GETFILE_ACT_NOTHING;

		if( !HD->enable_open_file )
			if(HD->action == IDC_GETFILE_ACT_OPEN_FILE)
				HD->action = IDC_GETFILE_ACT_OPEN_FOLDER;

		for( i = 0 ; i < DLG_CONTROLS ; i++ )
		{
			tHwnd=::GetDlgItem(dlg_hwnd,ctrl[i].id);
			if( ctrl[i].is_option)
			{
				::SendMessage(tHwnd
					,BM_SETCHECK
					,(WPARAM)(HD->action == ctrl[i].id)?BST_CHECKED:BST_UNCHECKED
					,0);
				if( ctrl[i].id == IDC_GETFILE_ACT_OPEN_FILE)
					::EnableWindow( tHwnd , HD->enable_open_file );
			}
			::SetWindowText(tHwnd, SonorkApp.LangString(ctrl[i].gls) );
		}
		
		return true;
	}
#undef	DLG_CONTROLS
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::TransferStartInfo(TSonorkWinStartInfo*SI, BOOL load)
{
	TSonorkWin::TransferStartInfo(SI,load);
	if( load )
	{
		context.console->SetMaxScanLines( SI->win_flags & 0x1?1:DEFAULT_SCAN_LINES);
	}
	else
	{
		SI->win_flags=context.console->GetMaxScanLines()==1?1:0;
	}
}

// ----------------------------------------------------------------------------

SKIN_ICON
 TSonorkMsgWin::GetMsgIcon( DWORD flags )
{
	if( flags & SONORK_APP_CCF_UNREAD)
	{
		if( flags & SONORK_APP_CCF_QUERY )
			return SKIN_ICON_QUERY_MSG;
		return SKIN_ICON_UNREAD_MSG;
	}
	else
	{
		if( flags & SONORK_APP_CCF_INCOMMING )
		{
			if( flags & (SONORK_APP_CCF_THREAD_START|SONORK_APP_CCF_IN_THREAD) )
			{
				if( flags & SONORK_APP_CCF_QUERY )
				{
					if(flags & SONORK_APP_CCF_PROCESSED)
						return SKIN_ICON_IN_QUERY;
					else
						return SKIN_ICON_IN_PENDING_QUERY;
				}
				return SKIN_ICON_IN_REPLY;
			}
			else
				return SKIN_ICON_INCOMMING;
		}
		else
		{
			if( flags & (SONORK_APP_CCF_ERROR|SONORK_APP_CCF_UNSENT) )
			{
				if( flags & (SONORK_APP_CCF_IN_THREAD|SONORK_APP_CCF_THREAD_START) )
				{
					return  ( flags & SONORK_APP_CCF_ERROR)
						?SKIN_ICON_OUT_REPLY_ERROR
						:SKIN_ICON_OUT_REPLY_PENDING;
				}
				else
				{
					return  ( flags & SONORK_APP_CCF_ERROR)
						?SKIN_ICON_OUTGOING_ERROR
						:SKIN_ICON_OUTGOING_PENDING;
				}
			}
			else
			if( flags & (SONORK_APP_CCF_IN_THREAD|SONORK_APP_CCF_THREAD_START) )
			{
				if( flags & SONORK_APP_CCF_QUERY )
					return SKIN_ICON_OUT_QUERY;
				return SKIN_ICON_OUT_REPLY;
			}
			else
				return SKIN_ICON_OUTGOING;
		}
	}
}


void
 TSonorkMsgWin::CmdExport(TSonorkConsole*pConsole)
{
	TSonorkShortString 	path;
	TSonorkTempBuffer	buffer(512);

	pConsole->CloseView();

	path.SetBufferSize(SONORK_MAX_PATH);
	lstrcpyn(path.Buffer(),user_data->display_alias.CStr(),SONORK_USER_ALIAS_MAX_SIZE);

	if(pConsole->Export(path.Buffer()
		,SONORK_CONSOLE_EXPORT_F_ASK_PATH
		 |SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS
		 |SONORK_CONSOLE_EXPORT_F_ADD_TIME_SUFFIX
		,buffer.CStr()
		,NULL))
	{
		SonorkApp.ShellOpenFile( this , path.CStr() , false);
	}

	context.tmp_msg.Clear();
}

DWORD
 TSonorkMsgWin::OnConsoleExport(TSonorkConsoleExportEvent* EV)
{
	SONORK_C_CSTR 	pAlias;
	UINT		aux;
	char		pBgColor[16];
	char		pFgColor[16];

	if( EV->format == SONORK_CONSOLE_EXPORT_HTML )
	{
		switch(EV->section)
		{
			case SONORK_CONSOLE_EXPORT_SECTION_START:
				{
					fputs("<HTML>\n<HEAD>\n<TITLE>",EV->file);
					SONORK_HtmlPuts(EV->file,user_data->display_alias.CStr());
					fputc('/',EV->file);
					SONORK_HtmlPuts(EV->file,SonorkApp.ProfileUser().alias.CStr());
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
				if(SonorkApp.GetMsg(EV->data.line->dat_index
					,&context.tmp_msg) != SONORK_RESULT_OK )
					break;
				TSonorkWin32App::MakeTimeStr( EV->data.line->time
					,(char*)EV->tag
					,MKTIMESTR_DATE|MKTIMESTR_TIME);

				if( EV->data.line->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING )
				{
					pAlias	= user_data->display_alias.CStr();
					TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_MSG_IN_OLD,SKIN_CI_BG), pBgColor);
					TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_MSG_IN_OLD,SKIN_CI_FG), pFgColor);
				}
				else
				{
					pAlias = SonorkApp.ProfileUser().alias.CStr();
					TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_MSG_OUT,SKIN_CI_BG), pBgColor);
					TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_MSG_OUT,SKIN_CI_FG), pFgColor);
				}
				fprintf(EV->file,
					" <TR>\n"
					"  <TD VALIGN=TOP BGCOLOR=#%s><font color=#%s><b>"
					,pBgColor
					,pFgColor);
				SONORK_HtmlPuts(EV->file,pAlias);
				fprintf(EV->file,
					"</b><br><font size=-2>%s</font></TD>\n"
					"  <TD BGCOLOR=#%s><font color=#%s>"
					,(char*)EV->tag
					,pBgColor
					,pFgColor);
				if(context.tmp_msg.DataServiceType() == SONORK_SERVICE_TYPE_URL )
				{
					aux=context.tmp_msg.TextLength() + 2;
					fprintf(EV->file,"<A HREF=\"");
					SONORK_StrCopyCut((char*)EV->tag
						,aux
						,context.tmp_msg.ToCStr()
						);
					fputs((char*)EV->tag , EV->file);
					fprintf(EV->file,"\">%s</A>",(char*)EV->tag);
				}
				else
					SONORK_HtmlPuts(EV->file
						, context.tmp_msg.ToCStr());
				fprintf(EV->file,"</TD>\n"
							" </TR>\n");
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
					fprintf(EV->file,"%s (%u/%u/%u %u:%u)\n"
						,user_data->display_alias.CStr()
						,EV->st.wYear
						,EV->st.wMonth
						,EV->st.wDay
						,EV->st.wHour
						,EV->st.wMinute);
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
				if(SonorkApp.GetMsg(EV->data.line->dat_index
					,&context.tmp_msg) != SONORK_RESULT_OK )
					break;

				TSonorkWin32App::MakeTimeStr( EV->data.line->time
					,(char*)EV->tag
					,MKTIMESTR_DATE|MKTIMESTR_TIME);

				if( EV->data.line->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING )
				{
					pAlias	= user_data->display_alias.CStr();
					aux	= '<';
				}
				else
				{
					pAlias	= SonorkApp.ProfileUser().alias.CStr();
					aux	= '>';
				}
				fprintf(EV->file,"%c %-16.16s \t%s\n"
				,aux
				,pAlias
				,(char*)EV->tag);
				fprintf(EV->file,"%s\n\n",context.tmp_msg.ToCStr());
				TSonorkConsole::SepLine(EV->file,8);

				break;
			case SONORK_CONSOLE_EXPORT_SECTION_END:
			fprintf(EV->file,"Sonork export\n");
			break;
		}
	}

	return 0L;
}


LRESULT
 TSonorkMsgWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	POINT pt;
	if( N->hdr.hwndFrom == toolbar.hwnd )
	{
		if( N->hdr.code == TBN_DROPDOWN )
		{
			OnToolBarButton( N->tbn.iItem );
			return TBDDRET_DEFAULT ;
		}
	}
	else
	if( N->hdr.hwndFrom == status.hwnd )
	{
		if( N->hdr.code == NM_CLICK )
		{
			GetCursorPos(&pt);
			::ScreenToClient(status.hwnd,&pt);
			pt.y=msg_win_sb_offset[(MSG_WIN_SB_SID_MSG&0xf)-1];
			if(pt.x>=pt.y)
			{
				PostPoke((pt.x<=pt.y+22||TestWinUsrFlag(SHOWING_SID_MSG))
					?SONORK_MSG_WIN_POKE_OPEN_SID_MSG
					:SONORK_MSG_WIN_POKE_OPEN_NEXT_UNREAD
					, false );
			}

		}
	}
	return 0;
}

void
 TSonorkMsgWin::ShowMtplMenu(BOOL reply)
{
	POINT pt;
	if( SonorkApp.UserMtplMenu() != NULL )
	{
		if(reply)
			SetWinUsrFlag(REPLY_TEMPLATE);
		else
			ClearWinUsrFlag(REPLY_TEMPLATE);
		::GetCursorPos(&pt);
		TrackPopupMenu(SonorkApp.UserMtplMenu()
				, TPM_LEFTALIGN  | TPM_LEFTBUTTON
				,pt.x
				,pt.y
				,0
				,Handle()
				,NULL);
	}
}
void
 TSonorkMsgWin::LoadLabels()
{

}
bool
 TSonorkMsgWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	SKIN_ICON icon;
	DWORD		state;
	int			delta;

	if(S->CtlID == IDC_MSG_STATUS )
	{
		DrawMsgWinStatus(S);
		return true;
	}
	
	if(S->CtlID == IDC_MSG_LOGO )
	{
		sonork_skin.DrawSmallLogo(S->hDC,&S->rcItem,true);
		return true;
	}

	if(S->CtlID == IDC_MSG_SEND )
	{
		icon = SKIN_ICON_SEND_MSG;
	}
	else
	if(S->CtlID == IDC_MSG_REPLY)
	{
		icon = SKIN_ICON_REPLY_MSG;
	}
	else
	if(S->CtlID == IDC_MSG_QUERY )
	{
		icon = SKIN_ICON_QUERY_MSG;
	}
	else
		return false;
	if(S->itemState&ODS_SELECTED)
	{
		delta = 0;
		state = DFCS_BUTTONPUSH|DFCS_PUSHED;
	}
	else
	{
		state = DFCS_BUTTONPUSH;
		delta = -1;
	}
	DrawFrameControl(S->hDC
		, &S->rcItem
		, DFC_BUTTON
		, state);
	
	sonork_skin.DrawIcon(S->hDC
		, icon
		, S->rcItem.left + delta + ( (S->rcItem.right - S->rcItem.left) - SKIN_ICON_SW )/2
		, S->rcItem.top  + delta + ( (S->rcItem.bottom - S->rcItem.top) - SKIN_ICON_SH )/2 );
//	S->rcItem.left += SKIN_ICON_SW+2;
//	::DrawText(S->hDC,str,len,&S->rcItem,DT_LEFT|DT_SINGLELINE|DT_VCENTER);

	return true;
}
LRESULT
 TSonorkMsgWin::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg   == WM_CTLCOLOREDIT )
	if( lParam == (LPARAM)input.hwnd )
		return sonork_skin.OnCtlColorMsgView( wParam );
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);

}

void InitMsgWinStatus(HWND sb)
{

	::SendMessage(sb
		, SB_SETPARTS
		, MSG_WIN_SB_SECTIONS
		, (LPARAM)msg_win_sb_offset);
	::SetMsgWinStatus(sb
		, MSG_WIN_SB_LABEL
		, SonorkApp.LangString(GLS_MS_MSGDT)
		, SKIN_HICON_NONE);

}
void
	SetMsgWinStatus(HWND 		sb
		, MSG_WIN_SB_SECTION	section
		, SONORK_C_CSTR 	pStr
		, SKIN_HICON 		hIcon)

{
	::SendMessage(sb
		,SB_SETICON
		,section&0xf
		,(LPARAM)sonork_skin.Hicon(hIcon));
	::SendMessage(sb
		,SB_SETTEXT
		,section
		,(LPARAM)pStr);
}

void
 DrawMsgWinStatus(DRAWITEMSTRUCT*S)
{
	SetBkMode(S->hDC,TRANSPARENT);
	::SelectObject(S->hDC,sonork_skin.Font(SKIN_FONT_BOLD));
	DrawText(S->hDC
		,(SONORK_C_CSTR)S->itemData
		,-1
		,&S->rcItem
		,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX
		 );

}

void
 TSonorkMsgWin::BlinkWindow()
{
#define FLASHES		2
#define FLASH_DELAY     20
	if(last_flash_time!=SonorkApp.CurrentTime())
	{
		if(!IsUltraMinimized())
		{
			for(int i=0;i<FLASHES;i++)
			{
				FlashWindow(Handle(),true);
				Sleep(FLASH_DELAY);
				FlashWindow(Handle(),true);
				if(i<FLASHES-1)Sleep(FLASH_DELAY/2);
			}
		}
		last_flash_time.Set(SonorkApp.CurrentTime());
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::UltraMinimizedPaint(TSonorkUltraMinPaint* P)
{
	if( user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) != 0 )
	{
		P->icon = SKIN_ICON_EVENT;
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_FG);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_BG);
		wsprintf(P->text,"%s (%u)"
			,user_data->display_alias.CStr()
			,user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT));
	}
	else
	{
		P->icon = SonorkApp.GetUserModeIcon( user_data );
		P->fg_color = sonork_skin.Color(SKIN_COLOR_MAIN_EXT
				,user_data->IsOnline()
				?SKIN_CI_MAIN_ONLINE:SKIN_CI_MAIN_OFFLINE);
		P->bg_color = sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG);
		strcpy(P->text,user_data->display_alias.CStr());
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::UltraMinimize()
{
	RECT	rect;
	if(ultra_min.win)return;
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
 TSonorkMsgWin::ShowSidMsg(BOOL show, BOOL forced)
{
	char tmp[64];
	while(show)
	{
		if(!forced)
		{
			if(user_data->RunValue(SONORK_URV_CURRENT_SID_SEQ_NO)
			== user_data->RunValue(SONORK_URV_PROCESSED_SID_SEQ_NO))
			{
				break;
			}
			if(SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_AUTO_SHOW_SID_MSG))
			{
				break;
			}
		}
		user_data->RunValue(SONORK_URV_PROCESSED_SID_SEQ_NO)=
			user_data->RunValue(SONORK_URV_CURRENT_SID_SEQ_NO);

		if(!user_data->sid_text.Length())
			break;

		SetWinUsrFlag(SHOWING_SID_MSG);
		context.console->CloseView();
		context.console->SetCallbackHintMode();
		wsprintf(tmp,"%s (ESC)"
			,SonorkApp.LangString(GLS_OP_HIDE));


		SetMsgWinStatus(status.hwnd
			, MSG_WIN_SB_HINT
			, tmp
			, SKIN_HICON_INFO);
		return;
	}
	HideSidMsg();
}

// ----------------------------------------------------------------------------

void
 TSonorkMsgWin::HideSidMsg()
{
	if(TestWinUsrFlag(SHOWING_SID_MSG))
	{
		context.console->ClearHintMode();
		ClearWinUsrFlag(SHOWING_SID_MSG);
		UpdateStatusBar( SONORK_MSG_WIN_F_UPDATE_SB_SELECTION , NULL );

	}
}

// Note: HistoryWin automatically saves and restores the DC
// no there is no need to do it here (unless we destroy
// objects selected into the device before returning)
#define HINT_DT_FLAGS DT_NOPREFIX|DT_LEFT|DT_WORDBREAK
#define HINT_SPACING_X		8
#define HINT_SPACING_Y		8
#define HINT_SPACING_ICON	4
void
 TSonorkMsgWin::OnHistoryWin_HintPaint( HDC tDC, const RECT* fullRECT)
{
	RECT	textRECT;
	HBRUSH	brush;
	SONORK_C_CSTR	str;
	int	h;

	brush = CreateSolidBrush( sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_BG) );

	textRECT.left	= fullRECT->left+SKIN_ICON_SW+HINT_SPACING_X+HINT_SPACING_ICON;
	textRECT.right	= fullRECT->right-HINT_SPACING_X*2;
	textRECT.top=fullRECT->bottom/8+2;
	textRECT.bottom= fullRECT->bottom-HINT_SPACING_Y;
	

	SelectObject(tDC,sonork_skin.Font(SKIN_FONT_BOLD));
	::SetTextColor(tDC, sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_FG) );
	::SetBkMode(tDC,TRANSPARENT);
	::FillRect(tDC,fullRECT,brush);

	str=SonorkApp.LangString(GLS_LB_SIDMSG);

	h=::DrawText(tDC
		,str
		,strlen(str)
		,&textRECT
		,HINT_DT_FLAGS);
	sonork_skin.DrawIcon(tDC
		,SKIN_ICON_NOTES
		,textRECT.left - SKIN_ICON_SW-HINT_SPACING_ICON+2
		,textRECT.top+1
		,ILD_MASK
		);
	sonork_skin.DrawIcon(tDC
		,SKIN_ICON_NOTES
		,textRECT.left - SKIN_ICON_SW-HINT_SPACING_ICON
		,textRECT.top);
	if(textRECT.right-textRECT.left>250)
		textRECT.right=textRECT.left+250;

	textRECT.top+=h+4;
	SelectObject(tDC,sonork_skin.Font(SKIN_FONT_LARGE));
	::DrawText(tDC
		,user_data->sid_text.CStr()
		,user_data->sid_text.Length()
		,&textRECT
		,HINT_DT_FLAGS);
	DeleteObject(brush);
}


/*
	SKIN_ICON		icon;
	HBRUSH			brush;
	const char*str=user_data->display_alias.CStr();

	SetBkMode(tDC,TRANSPARENT);
	SelectObject(tDC,sonork_skin.Font(SKIN_FONT_MAIN_TREE));
	if( user_data->CtrlValue(SONORK_UCV_UNREAD_MSG_COUNT) != 0 )
	{
		SetTextColor(tDC,sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_FG));
		icon        = SKIN_ICON_EVENT;
		brush 	= CreateSolidBrush(sonork_skin.Color(SKIN_COLOR_MSG_IN_NEW, SKIN_CI_BG));
		::FillRect(tDC,fullRECT,brush);
		DeleteObject(brush);
	}
	else
	{
		icon = SonorkApp.GetUserModeIcon( user_data );
		SetTextColor(tDC,
			sonork_skin.Color(SKIN_COLOR_MAIN_EXT
			,user_data->IsOnline()
			?SKIN_CI_MAIN_ONLINE:SKIN_CI_MAIN_OFFLINE));
		::FillRect(tDC,fullRECT,sonork_skin.Brush(SKIN_BRUSH_MAIN_VIEW));
	}
	::DrawText(tDC
		,str
		,strlen(str)
		,(RECT*)textRECT
		,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
	::FrameRect(tDC,fullRECT,(HBRUSH)GetStockObject(BLACK_BRUSH));
	sonork_skin.DrawIcon(tDC
		,icon
		,1
		,((ULTRA_MIN_FULL_HEIGHT-SKIN_ICON_SH)>>1)
		);
*/

