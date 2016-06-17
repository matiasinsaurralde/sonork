#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkgrpmsgwin.h"
#include "srkfiletxgui.h"

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

#define TOOL_BAR_ID			500
enum SONORK_GRPMSGWIN_TOOLBAR_BUTTON
{
	SONORK_GRPMSGWIN_TB_BASE	= 100
,	SONORK_GRPMSGWIN_TB_CLEAR	= SONORK_GRPMSGWIN_TB_BASE
,	SONORK_GRPMSGWIN_TB_QUERY
,	SONORK_GRPMSGWIN_TB_LIMIT
};
#define TOOL_BAR_BUTTONS	(SONORK_GRPMSGWIN_TB_LIMIT-SONORK_GRPMSGWIN_TB_BASE)
#define TAB_BUTTON_WIDTH	(SKIN_ICON_SW*3)
#define TAB_BUTTON_HEIGHT	(SKIN_ICON_SH+4)

#define SEND_ITEM_F_PROCESSED		0x000001
#define SEND_ITEM_F_SUCCESS		0x000002
#define SEND_ITEM_F_FAIL		0x000004
#define SEND_ITEM_F_DISABLED		0x000008

#define POKE_SEND_NEXT		SONORK_WIN_POKE_01
#define POKE_SEND_RESULT	SONORK_WIN_POKE_02
#define POKE_SEND_END		SONORK_WIN_POKE_03
#define IS_MAIN_VIEW		SONORK_WIN_F_USER_01

#define MIN_WIDTH	400
#define MIN_HEIGHT	280
static const char *szGrpMsgWinSysName="GrpMsgWin";
TSonorkGrpMsgWin::TSonorkGrpMsgWin(bool is_main_view)
	:TSonorkTaskWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_GRPMSG
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
	send.pCurItem = NULL;
	send.state = STATE_IDLE;
	if(is_main_view)SetWinUsrFlag(IS_MAIN_VIEW);
	SetEventMask(
		is_main_view
			?SONORK_APP_EM_CX_STATUS_AWARE
			|SONORK_APP_EM_MSG_PROCESSOR
			|SONORK_APP_EM_MAIN_VIEW_AWARE
			:SONORK_APP_EM_CX_STATUS_AWARE
			|SONORK_APP_EM_MSG_PROCESSOR);
}
void
 TSonorkGrpMsgWin::SetCompress(BOOL set)
{
	SetCtrlChecked(IDC_GRPMSG_FILE_COMPRESS,set);
}

void
 TSonorkGrpMsgWin::AddFile(SONORK_C_CSTR str)
{
	DWORD aux;
	if(ListBox_FindStringExact(t_file.list,-1,str)!=LB_ERR)
		return ;
	aux = GetFileAttributes(str);
	if(aux==(DWORD)-1 || (aux&FILE_ATTRIBUTE_DIRECTORY))
		return;
	ListBox_AddString(t_file.list,str);
	SetTab(TAB_FILE);
}
void
TSonorkGrpMsgWin::ProcessDrop(TSonorkClipData*CD)
{
	TSonorkListIterator I;
	TSonorkShortString  *S;
	const TSonorkShortStringQueue  *Q;
	if( CD->DataType() == SONORK_CLIP_DATA_FILE_NAME)
	{
		AddFile(CD->GetCStr());
	}
	else
	if( CD->DataType() == SONORK_CLIP_DATA_FILE_LIST)
	{
		Q=CD->GetShortStringQueue();
		Q->BeginEnum(I);
		while((S=Q->EnumNext(I))!=NULL)
			AddFile(S->CStr());
		Q->EndEnum(I);
	}
	else
	{
		SetCtrlDropText( IDC_GRPMSG_INPUT , CD , "\r\n");
		SetTab(TAB_TEXT);
		return;
	}
}

LRESULT
 TSonorkGrpMsgWin::OnPoke(SONORK_WIN_POKE poke,LPARAM)
{
	TSonorkExtUserData *	UD;
	TSonorkUserLocus1	locus;
	SONORK_UTS_LINK_ID	uts_link_id;
	TSonorkTempBuffer	buffer(SONORK_USER_ALIAS_MAX_SIZE+144);
	switch( poke )
	{
		case POKE_SEND_NEXT:
			send.pCurItem = send.queue.EnumNext( send.iterator );
			::InvalidateRect( users.hwnd , NULL , false);
			if( send.pCurItem == NULL )
			{
				PostPoke( POKE_SEND_END , 0);
				break;
			}
			send.process_count++;
			send.pCurItem->flags|=SEND_ITEM_F_PROCESSED;
			if(send.pCurItem->flags&SEND_ITEM_F_DISABLED)
			{
				PostPoke( POKE_SEND_NEXT , 0);
				break;
			}
			SonorkApp.LangSprintf(buffer.CStr()
				,GLS_MS_GRPSNDCUR
				,send.process_count
				,send.queue.Items()
				,send.pCurItem->alias.CStr()
				);
			TSonorkWin::SetStatus(status.hwnd
				, buffer.CStr()
				, SKIN_HICON_BUSY);

			UD=SonorkApp.UserList().Get(send.pCurItem->user_id);
			if(UD!=NULL)
			{
				UD->GetLocus1(&locus);
				uts_link_id=UD->UtsLinkId();
			}
			else
			{
				locus.userId.Set(send.pCurItem->user_id);
				locus.sid.Clear();
				uts_link_id=SONORK_INVALID_LINK_ID;

			}

			SonorkApp.SendMsgLocus(send.handle
					, this
					, &locus
					, uts_link_id
					,&send.msg
					);

			if(send.handle.ERR.Result() == SONORK_RESULT_OK)
				break;
//		break ommited, fall through to POKE_SEND_RESULT

		case POKE_SEND_RESULT:
			assert( send.pCurItem != NULL);

			if( send.handle.ERR.Result() == SONORK_RESULT_OK )
			{
				send.success_count++;
				send.pCurItem->flags|=SEND_ITEM_F_SUCCESS;
			}
			else
			{
				send.pCurItem->flags|=SEND_ITEM_F_FAIL;
			}
			PostPoke( POKE_SEND_NEXT , 0);
		break;

		case POKE_SEND_END:
			send.state = STATE_COMPLETE;
			send.queue.EndEnum(send.iterator);
			SonorkApp.LangSprintf(buffer.CStr()
				,GLS_MS_GRPSNDRES
				,send.success_count
				,send.queue.Items()
				);
			TSonorkWin::SetStatus(status.hwnd
				, buffer.CStr()
				, SKIN_HICON_INFO);
			UpdateToolBar();
			MessageBox(buffer.CStr()
				, GLS_LB_GRPMSG
				, MB_OK|MB_ICONINFORMATION);
			CmdClear( false ); 
			break;
	}
	return 0L;
}
void TSonorkGrpMsgWin::CmdSend()
{
	TSonorkShortString S;
	TSonorkShortStringQueue*queue;
	TSonorkFileTxGui *W;
	int	 i,mi;
	DWORD	usr_flags, pc_flags;

	if( !send.queue.Items() || send.state != STATE_IDLE)
			return;

	SonorkApp.ProfileCtrlFlags().SetClear(SONORK_PCF_GRPMSG_SAVUSR
		,GetCtrlChecked( IDC_GRPMSG_SAVUSR));
	SonorkApp.ProfileCtrlFlags().SetClear(SONORK_PCF_GRPMSG_SAVSYS
		,GetCtrlChecked( IDC_GRPMSG_SAVSYS));

	if(!MayStartTask(&send.handle.ERR))
	{
		TaskErrorBox(GLS_TK_SNDMSG,&send.handle.ERR);
		return;
	}
	if( tab.index == TAB_FILE )
	{
		SONORK_DWORD2List	dw2list;
		mi=ListBox_GetCount(t_file.list);
		if(mi<1)return;
		send.queue.BeginEnum(send.iterator);
		while((send.pCurItem=send.queue.EnumNext(send.iterator))!=NULL)
		{
			if(!(send.pCurItem->flags&SEND_ITEM_F_DISABLED))
				dw2list.AddItem(&send.pCurItem->user_id);
		}
		send.queue.EndEnum(send.iterator);
		if(dw2list.Items())
		{
			S.SetBufferSize(SONORK_MAX_PATH);

			SONORK_MEM_NEW( queue = new TSonorkShortStringQueue );
			for(i=0;i<mi;i++)
			{
				ListBox_GetString(t_file.list,i,S.Buffer());
				queue->Add( S.CStr() );
			}
			wsprintf(S.Buffer(),"%u %s"
				, dw2list.Items()
				, SonorkApp.LangString(dw2list.Items()==1?GLS_LB_USR:GLS_LB_USRS));
			W=new TSonorkFileTxGui(dw2list.ItemPtr(0)
				,dw2list.Items()
				,S.CStr()
				,queue
				,GetCtrlChecked(IDC_GRPMSG_FILE_COMPRESS)
					?SONORK_FILE_TX_F_SEND_COMPRESS
					:0);
			W->Create();
		}
		Destroy(IDOK);
	}
	else
	{
		GetCtrlText( IDC_GRPMSG_INPUT , S );
		if( !S.Length() )return;
		send.msg.SetStr( 0 , S.CStr() );
		usr_flags=
			(ToolBar_GetButtonState(toolbar.hwnd,SONORK_GRPMSGWIN_TB_QUERY)
				&TBSTATE_CHECKED)
				?SONORK_MSG_UF_GROUP|SONORK_MSG_UF_QUERY
				:SONORK_MSG_UF_GROUP;
		pc_flags=SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_GRPMSG_SAVUSR)
				?SONORK_APP_MPF_SILENT
				:SONORK_APP_MPF_SILENT|SONORK_APP_MPF_DONT_STORE;
		SonorkApp.PrepareMsg(send.handle
				,&send.msg
				,0		// sys_flags
				,usr_flags   	// usr_flags
				,pc_flags	// pc_flags
				,0		// reply_track_no
				,0
				);

		send.pCurItem = NULL;
		send.queue.BeginEnum( send.iterator );
		send.state = STATE_SENDING;

		UpdateToolBar();
		EnableInterface( false );
		send.process_count = send.success_count=0;
		PostPoke( POKE_SEND_NEXT , 0);
	}
}
void TSonorkGrpMsgWin::EnableInterface( BOOL v )
{
	SetCtrlEnabled( IDC_GRPMSG_INPUT 	, v);
	SetCtrlEnabled( IDC_GRPMSG_SAVUSR	, v);
	SetCtrlEnabled( IDC_GRPMSG_SAVSYS	, v);
	SetCtrlEnabled( IDC_GRPMSG_TOPFRAME	, v);
}

bool
 TSonorkGrpMsgWin::OnDrawItem( DRAWITEMSTRUCT* S )
{
	int x,y;
	SKIN_ICON ic;
	SONORK_C_CSTR		pStr;
	TSonorkGrpMsgItem*	pItem;


	if(TestWinSysFlag(SONORK_WIN_SF_DESTROYING))
		return false;

	if( S->CtlID == IDC_GRPMSG_USERS )
	{
		if(S->itemID>=send.queue.Items())
			return true;
		pItem = (TSonorkGrpMsgItem*)ListBox_GetItemData(users.hwnd,S->itemID);
		if(pItem==NULL)
		{
			pStr = "??";
		}
		else
		{
			pStr = pItem->alias.CStr();
		}
		x=strlen(pStr);
		::SetBkMode(S->hDC,TRANSPARENT);
		::SetTextColor(S->hDC
			, sonork_skin.Color(SKIN_COLOR_DIALOG,SKIN_CI_FG));
		::FillRect(S->hDC
			, &S->rcItem
			, sonork_skin.Brush(SKIN_BRUSH_DIALOG));
		S->rcItem.left+=SKIN_ICON_SW+2;
		DrawText(S->hDC
			,pStr
			,x
			,&S->rcItem
			,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);

		if(pItem == send.pCurItem )
		{
			ic = SKIN_ICON_OUTGOING_PENDING;
		}
		else
		if(pItem->flags&SEND_ITEM_F_DISABLED)
		{
			ic=SKIN_ICON_CANCEL;
		}
		else
		if(pItem->flags&SEND_ITEM_F_PROCESSED)
		{
			ic = pItem->flags&SEND_ITEM_F_SUCCESS
				?SKIN_ICON_OUTGOING
				:SKIN_ICON_OUTGOING_ERROR;
		}
		else
		{
			ic = SKIN_ICON_BULLET;
		}
		sonork_skin.DrawIcon(S->hDC
			, ic
			, S->rcItem.left-(SKIN_ICON_SW+2)
			, S->rcItem.top
			,ILD_NORMAL);
		return true;
	}
	else
	if( S->CtlID == IDC_GRPMSG_TAB )
	{
		::FillRect(S->hDC
			,&S->rcItem
			, sonork_skin.Brush(SKIN_BRUSH_DIALOG));
		x = S->rcItem.left + ((S->rcItem.right-S->rcItem.left) - SKIN_ICON_SW)/2;
		y = S->rcItem.top  + ((S->rcItem.bottom-S->rcItem.top) - SKIN_ICON_SH)/2;
		switch(S->itemID)
		{
			case 0:
			ic=SKIN_ICON_NOTES;
			break;

			case 1:
			default:
			ic=SKIN_ICON_FILE;
			break;

		}
		sonork_skin.DrawIcon(S->hDC
			, ic
			, x
			, y
			,ILD_NORMAL);

		return true;
	}
	else
	if( S->CtlID == IDOK )
	{
		if(S->itemState&ODS_SELECTED)
		{
			x = 0;
			y = DFCS_BUTTONPUSH|DFCS_PUSHED;
		}
		else
		{
			y = DFCS_BUTTONPUSH;
			x = -1;
		}
		DrawFrameControl(S->hDC
			, &S->rcItem
			, DFC_BUTTON
			, y);

		sonork_skin.DrawIcon(S->hDC
			, SKIN_ICON_SEND_MSG
			, S->rcItem.left + x + ( (S->rcItem.right - S->rcItem.left) - SKIN_ICON_SW )/2
			, S->rcItem.top  + x + ( (S->rcItem.bottom - S->rcItem.top) - SKIN_ICON_SH )/2
			, S->itemState&ODS_DISABLED?ILD_BLEND25:ILD_NORMAL);
	}
	return false;


}
UINT
 TSonorkGrpMsgWin::ClearSendList()
{
	UINT pItems;
	assert( send.state != STATE_SENDING );
	pItems = send.queue.Items();
	ListBox_Clear( users.hwnd );
	while((send.pCurItem=send.queue.RemoveFirst()) != NULL )
		SONORK_MEM_DELETE(send.pCurItem);
	return pItems;
}
void
 TSonorkGrpMsgWin::LoadSendListFromMainView()
{
	TSonorkListIterator 	I;
	TSonorkId*		pGuId;
	SONORK_DWORD2List	dw2list;
	TSonorkExtUserData *	UD;

	assert( TestWinUsrFlag(IS_MAIN_VIEW));

	ClearSendList();
	SonorkApp.GetMainViewSelection( &dw2list );

	dw2list.InitEnum( I );
	while( (pGuId=(TSonorkId*)dw2list.EnumNext(I)) != NULL )
	{
		if((UD=SonorkApp.UserList().Get(*pGuId))==NULL)
			continue;
		AddUser(UD->userId,UD->display_alias.CStr());
	}
	AfterAddUsers();
}
void
 TSonorkGrpMsgWin::AddUser(const TSonorkId& user_id, SONORK_C_CSTR alias)
{
	TSonorkGrpMsgItem*	pItem;
	SONORK_MEM_NEW(pItem=new TSonorkGrpMsgItem);
	pItem->flags=0;
	pItem->user_id.Set( user_id );
	pItem->alias.Set( alias);
	ListBox_AddString(users.hwnd, (const char*)pItem );
	send.queue.Add(pItem);
}

void
 TSonorkGrpMsgWin::AfterAddUsers()
{
	char tmp[80];
	if( send.queue.Items() )
	{
		SonorkApp.LangSprintf(tmp,GLS_IF_USEL,send.queue.Items());
		TSonorkWin::SetStatus(status.hwnd, tmp, SKIN_HICON_INFO);
	}
	else
		TSonorkWin::SetStatus(status.hwnd
			, GLS_IF_NUSEL
			, SKIN_HICON_ALERT);
	UpdateToolBar();
}

bool	TSonorkGrpMsgWin::OnAppEvent(UINT event, UINT , void*data)
{
	TSonorkAppEventMsg	*E;
	if( event == SONORK_APP_EVENT_MAIN_VIEW_SELECTION )
	{
		if(TestWinUsrFlag(IS_MAIN_VIEW) && send.state == STATE_IDLE)
			LoadSendListFromMainView();
		return true;
	}
	else
	if( event == SONORK_APP_EVENT_SONORK_MSG_SENT )
	{
		E=(TSonorkAppEventMsg*)data;
		if( E->taskId == send.handle.taskId )
		{
			send.handle.taskId=0;
			send.handle.ERR.Set(*E->ERR);
			PostPoke( POKE_SEND_RESULT , 0);
		}
	}
	return false;
}


void TSonorkGrpMsgWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDC_GRPMSG_SAVUSR	,	GLS_OP_SMCUSR	}
	,	{IDC_GRPMSG_SAVSYS	,	GLS_OP_SMCSYS	}
	,	{IDC_GRPMSG_TOPFRAME	, 	GLS_OP_SAVMSGCPY}
	,	{IDC_GRPMSG_FILE_BROWSE	, 	GLS_OP_BROWSE}
	,	{-1			,	GLS_LB_GRPMSG	}
	,	{0			,	GLS_NULL		}
	};
	LoadLangEntries( gls_table, true );
}

bool
 TSonorkGrpMsgWin::OnCreate()
{
	UINT aux;
	POINT pt;
	tab.index = TAB_TEXT;

	aux=IDOK;
	AddToolTipCtrl(1,&aux);
	send_btn.hwnd		= GetDlgItem(IDOK);
	send_btn.width		= GetCtrlWidth(IDOK);
	t_file.list		= GetDlgItem( IDC_GRPMSG_FILE_LIST );
	t_file.browse		= GetDlgItem( IDC_GRPMSG_FILE_BROWSE );
	GetCtrlSize(IDC_GRPMSG_FILE_BROWSE , &t_file.browse_sz);


	inputHwnd	= GetDlgItem(IDC_GRPMSG_INPUT);
	users.hwnd	= GetDlgItem(IDC_GRPMSG_USERS);
	users.width	= GetCtrlWidth(IDC_GRPMSG_USERS);
	status.hwnd 	= GetDlgItem(IDC_GRPMSG_STATUS);
	status.height 	= GetCtrlHeight(IDC_GRPMSG_STATUS);
	tab.hwnd        = GetDlgItem(IDC_GRPMSG_TAB);
	GetCtrlPoint(IDC_GRPMSG_TAB,&pt);
	tab.top			= pt.y;
	SetupToolBar();
	{
		TC_ITEM tcHitem;
		tab.hwnd = GetDlgItem(IDC_GRPMSG_TAB );

		aux = ::GetWindowLong(tab.hwnd,GWL_STYLE);

		aux|=TCS_SINGLELINE  ;//TCS_RIGHT|
		aux&=~(WS_BORDER);
		::SetWindowLong(tab.hwnd,GWL_STYLE,aux);
		TabCtrl_SetItemSize(tab.hwnd
			,TAB_BUTTON_WIDTH
			,TAB_BUTTON_HEIGHT);

		TabCtrl_SetImageList(tab.hwnd,sonork_skin.Icons());

		tcHitem.mask=TCIF_TEXT|TCIF_IMAGE;
		tcHitem.lParam=0;

		tcHitem.iImage =SKIN_ICON_NOTES;
		tcHitem.pszText=(char*)SonorkApp.LangString(GLS_LB_TXT);
		TabCtrl_InsertItem(tab.hwnd,0,&tcHitem);

		tcHitem.iImage =SKIN_ICON_FILE;
		tcHitem.pszText=(char*)SonorkApp.LangString(GLS_LB_FILE);
		TabCtrl_InsertItem(tab.hwnd,1,&tcHitem);
	}
	::SendMessage(inputHwnd ,WM_SETFONT,(WPARAM)sonork_skin.Font(SKIN_FONT_LARGE),0);

	SonorkApp.OnSysDialogRun(this
		,TestWinUsrFlag(IS_MAIN_VIEW)?SONORK_SYS_DIALOG_GRP_MSG:SONORK_SYS_DIALOG_INVALID
		,true
		,szGrpMsgWinSysName);
	RealignControls();
	if(TestWinUsrFlag(IS_MAIN_VIEW) )
		LoadSendListFromMainView();
	SetEditCtrlMaxLength(IDC_GRPMSG_INPUT,SONORK_APP_MAX_MSG_TEXT_LENGTH);
	SetCtrlChecked( IDC_GRPMSG_SAVUSR, SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_GRPMSG_SAVUSR));
	SetCtrlChecked( IDC_GRPMSG_SAVSYS, SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_GRPMSG_SAVSYS));
	SetCtrlChecked( IDC_GRPMSG_FILE_COMPRESS, !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_COMPRESS_FILES));

	::SetWindowPos(tab.hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	inputDropTarget.AssignCtrl( Handle(), inputHwnd );
	t_file.drop_target.AssignCtrl( Handle(), t_file.list );
	
	_SetTab(TAB_TEXT,true,true);
	return true;

}
void
 TSonorkGrpMsgWin::OnBeforeDestroy()
{
	if(send.state ==STATE_SENDING)
	{
		send.queue.EndEnum( send.iterator );
	}
	send.state=STATE_IDLE;	// All operations aborted
	ClearSendList();
	inputDropTarget.Enable( false );
	t_file.drop_target.Enable( false );
	SonorkApp.OnSysDialogRun(this
		,TestWinUsrFlag(IS_MAIN_VIEW)?SONORK_SYS_DIALOG_GRP_MSG:SONORK_SYS_DIALOG_INVALID
		,false
		,szGrpMsgWinSysName);
}
void
 TSonorkGrpMsgWin::_SetTab(TAB_INDEX b, bool update_button, bool forced)
{
	if(tab.index != b || forced)
	{
		bool is_file;
		tab.index = b;
		is_file = tab.index == TAB_FILE;

		SetCtrlVisible( IDC_GRPMSG_FILE_LIST 	, is_file );
		SetCtrlVisible( IDC_GRPMSG_FILE_BROWSE	, is_file );
		SetCtrlVisible( IDC_GRPMSG_FILE_COMPRESS, is_file );
		SetCtrlVisible( IDC_GRPMSG_INPUT, !is_file );
		if(update_button)
			TabCtrl_SetCurSel(tab.hwnd,tab.index);
		UpdateToolBar();
	}
}
LRESULT
 TSonorkGrpMsgWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	TSonorkClipData *	CD;
	switch( event )
	{
		case SONORK_DRAG_DROP_QUERY:
			SonorkApp.ProcessDropQuery((TSonorkDropQuery*)lParam
				,SONORK_DROP_ACCEPT_F_URL
				|SONORK_DROP_ACCEPT_F_TEXT
				|SONORK_DROP_ACCEPT_F_FILES
				|SONORK_DROP_ACCEPT_F_SONORKCLIPDATA);
		break;

		case SONORK_DRAG_DROP_UPDATE:
		{
			TSonorkDropMsg*M=(TSonorkDropMsg*)lParam;
			*M->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_MOVE|DROPEFFECT_LINK;
		}
		break;
		case SONORK_DRAG_DROP_CANCEL:
		case SONORK_DRAG_DROP_INIT:
			break;
		case SONORK_DRAG_DROP_EXECUTE:
		{
			CD = SonorkApp.ProcessDropExecute((TSonorkDropExecute*)lParam);
			if( CD == NULL )break;
			ProcessDrop( CD );
			CD->Release();

		}
		break;
	}
	return 0;
}


#define COMPRESS_CHECKBOX_HEIGHT	16

void
 TSonorkGrpMsgWin::RealignControls()
{
	const UINT tH = Height() ;
	const UINT tW = Width();
	int		tab_x,tab_w,tab_h;
	int		lst_y,lst_x,lst_w,lst_h;
	SIZE	border;
	HDWP 	defer_handle;

	border.cx	= GetSystemMetrics(SM_CXBORDER);
	border.cy	= GetSystemMetrics(SM_CYBORDER);

	tab_x		= users.width + border.cx*4;
	tab_w	 	= tW - tab_x - border.cx*2;
	tab_h 		= tH - tab.top - status.height-border.cy;


	lst_y		= tab.top + 32;
	lst_x		= tab_x + border.cx*4;
	lst_w		= tab_w - border.cx*8;
	lst_h		= tab_h - toolbar.size.cy -  border.cy*6 - 36;

	defer_handle = BeginDeferWindowPos( 10 );

	defer_handle = DeferWindowPos(defer_handle
		,users.hwnd
		,NULL
		,border.cx
		,border.cy
		,users.width
		,tH - status.height - border.cy*2
		,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,tab.hwnd
		,NULL
		,tab_x
		,tab.top
		,tab_w
		,tab_h
		,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,inputHwnd
		,NULL
		,lst_x
		,lst_y
		,lst_w
		,lst_h
		,SWP_NOZORDER|SWP_NOACTIVATE);

	lst_h-=(t_file.browse_sz.cy+COMPRESS_CHECKBOX_HEIGHT+border.cy*5);
	defer_handle = DeferWindowPos(defer_handle
		,t_file.list
		,NULL
		,lst_x
		,lst_y
		,lst_w
		,lst_h
		,SWP_NOZORDER|SWP_NOACTIVATE);

	lst_y+=lst_h + (border.cy*2);
	defer_handle = DeferWindowPos(defer_handle
		,GetDlgItem(IDC_GRPMSG_FILE_COMPRESS)
		,NULL
		,lst_x
		,lst_y
		,lst_w
		,COMPRESS_CHECKBOX_HEIGHT
		,SWP_NOZORDER|SWP_NOACTIVATE);

	lst_y+=COMPRESS_CHECKBOX_HEIGHT + (border.cy*2);

	defer_handle = DeferWindowPos(defer_handle
		,t_file.browse
		,NULL
		,lst_x + (lst_w - t_file.browse_sz.cx)/2
		,lst_y 
		,t_file.browse_sz.cx
		,t_file.browse_sz.cy
		,SWP_NOZORDER|SWP_NOACTIVATE);


	lst_x=tab_x 	+ tab_w - send_btn.width - border.cx*4;
	lst_y=tab.top 	+ tab_h - toolbar.size.cy - border.cy*4;
	defer_handle = DeferWindowPos(defer_handle
		, send_btn.hwnd
		, NULL
		, lst_x
		, lst_y
		, send_btn.width
		, toolbar.size.cy
		,SWP_NOZORDER|SWP_NOACTIVATE);


	defer_handle = DeferWindowPos(defer_handle
		,toolbar.hwnd
		,NULL
		,lst_x - toolbar.size.cx - border.cx*2
		,lst_y
		,toolbar.size.cx 
		,toolbar.size.cy
		,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,status.hwnd
		,NULL
		,0
		,tH - status.height
		,tW
		,status.height
		,SWP_NOZORDER|SWP_NOACTIVATE);



	EndDeferWindowPos( defer_handle );
}
void
 TSonorkGrpMsgWin::UpdateToolBar()
{
	DWORD	cur_status,new_status;
	UINT	items=send.queue.Items();

	SetCtrlEnabled(IDOK,items != 0 && send.state==STATE_IDLE);
	for(UINT i=SONORK_GRPMSGWIN_TB_BASE
		;i<SONORK_GRPMSGWIN_TB_LIMIT
		;i++)
	{
		new_status=TBSTATE_ENABLED;
		if( i == SONORK_GRPMSGWIN_TB_CLEAR )
		{
			if(send.state>STATE_IDLE&&send.state>STATE_COMPLETE)
				new_status = 0;
		}
		else
		if( items == 0 || send.state!=STATE_IDLE )
			new_status = 0;
		else
		if( i == SONORK_GRPMSGWIN_TB_QUERY )
		{
			if( tab.index != TAB_TEXT )
				new_status = 0;
		}
		cur_status = ToolBar_GetButtonState( toolbar.hwnd , i);
		cur_status&=~TBSTATE_ENABLED;
		ToolBar_SetButtonState( toolbar.hwnd
			, i
			, cur_status|new_status);
	}

}

bool
 TSonorkGrpMsgWin::OnCommand(UINT id,HWND hwnd , UINT code )
{
	int index;
	TSonorkGrpMsgItem*	pItem;
	RECT	rect;
	if( hwnd == users.hwnd )
	{
		//&& !TestWinUsrFlag(IS_MAIN_VIEW)
		if( code == LBN_SELCHANGE )
		{
			index = ListBox_GetCurSel(hwnd);
			if(index>=0 && index<(int)send.queue.Items() )
			{
				pItem = (TSonorkGrpMsgItem*)ListBox_GetItemData(hwnd,index);
				if(pItem!=NULL)
				{
					pItem->flags^=SEND_ITEM_F_DISABLED;
					::SendMessage(hwnd,LB_GETITEMRECT,index,(LPARAM)&rect);
					::InvalidateRect(hwnd,&rect,false);
				}
			}
		}
		return true;
	}

	if( code == BN_CLICKED )
	{
		if(send.state>STATE_IDLE&&send.state<STATE_COMPLETE)
			return true;
		if( id == IDOK )
		{
			CmdSend();
			return true;
		}
		else
		if( id == IDC_GRPMSG_FILE_BROWSE )
		{
			CmdFileBrowse();
			return true;
		}
		else
		if( hwnd == toolbar.hwnd )
		{
			if( id == SONORK_GRPMSGWIN_TB_CLEAR )
			{
				CmdClear( true );
				return true;
			}
		}
	}
	return false;
}
void
 TSonorkGrpMsgWin::CmdFileBrowse()
{
	TSonorkShortString file_path;
	if( SonorkApp.GetFileNameForUpload(this,file_path) )
		ListBox_AddString(t_file.list,file_path.CStr() );
}
void
 TSonorkGrpMsgWin::CmdClear( bool user_click )
{
	if(send.state == STATE_SENDING)
		return;

	if(send.state == STATE_COMPLETE)
	{
		send.state = STATE_IDLE;
		EnableInterface( true );
		if( TestWinUsrFlag(IS_MAIN_VIEW) )
		{
			LoadSendListFromMainView();
		}
		else
		if( user_click )
		{
			Destroy(IDOK);
			return;
		}
	}
	SetCtrlText(IDC_GRPMSG_INPUT,csNULL);
	if( tab.index == TAB_TEXT)
	{
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND,(LPARAM)inputHwnd );
	}
	ListBox_Clear(t_file.list);
}
void
 TSonorkGrpMsgWin::OnSize(UINT sz_type)
{
	if(sz_type == SIZE_RESTORED)
	{
		RealignControls();
	}
}

void TSonorkGrpMsgWin::SetupToolBar()
{
	static TSonorkWinToolBarButton	btn_info[TOOL_BAR_BUTTONS]=
	{
		{   SONORK_GRPMSGWIN_TB_CLEAR
			, SKIN_ICON_CLEAR
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON }

	,	{	SONORK_GRPMSGWIN_TB_QUERY	+	SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_QUERY_MSG
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK }
	};

	toolbar.hwnd=TSonorkWin::CreateToolBar(
		Handle()
		,TOOL_BAR_ID
		,	 WS_VISIBLE
			| TBSTYLE_TOOLTIPS
			| TBSTYLE_FLAT
			| CCS_NOPARENTALIGN
			| CCS_NODIVIDER
			| CCS_NORESIZE
		, TOOL_BAR_BUTTONS
		, btn_info
		, &toolbar.size);

}
LRESULT	TSonorkGrpMsgWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if( N->hdr.hwndFrom == tab.hwnd )
	{
		if(N->hdr.code == TCN_SELCHANGE)
			_SetTab((TAB_INDEX)TabCtrl_GetCurSel(tab.hwnd) , false, false);
	}
	return 0L;
}

LRESULT TSonorkGrpMsgWin::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg   == WM_CTLCOLORLISTBOX )
	if( lParam == (LPARAM)users.hwnd )
	{
		return sonork_skin.OnCtlColorDialog( wParam);
	}
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);

}
void
 TSonorkGrpMsgWin::OnToolTipText(UINT id, HWND , TSonorkWinToolTipText&TTT )
{
	if( id == (UINT)send_btn.hwnd )
	{
		TTT.gls = GLS_OP_SNDMSG;
	}
	else
	switch(id)
	{
		case SONORK_GRPMSGWIN_TB_CLEAR:
			TTT.gls = GLS_OP_RESET;
			break;
		case SONORK_GRPMSGWIN_TB_QUERY:
			TTT.gls = GLS_LB_QUERYMSG;
			break;
	}
}
bool
 TSonorkGrpMsgWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MIN_WIDTH;
	MMI->ptMinTrackSize.y=MIN_HEIGHT;
	return true;

}
