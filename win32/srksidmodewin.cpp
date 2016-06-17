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

#include "srkdialogwin.h"
#include "srksidmodewin.h"

#define TAB_MARGIN		1
/*
#define UWF_TRACKER_TREE_LOADED	SONORK_WIN_F_USER_01
#define UWF_TRACKERS_MODIFIED	SONORK_WIN_F_USER_02
#define POKE_LOADED_OK		SONORK_WIN_POKE_01
#define POKE_EDIT_ITEM 		SONORK_WIN_POKE_02
*/







// ----------------------------------------------------------------------------

LRESULT
 TSonorkSidModeWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch( wParam )
	{

		case SONORK_WIN_POKE_CHILD_NOTIFY:
			return OnChildDialogNotify((TSonorkChildDialogNotify*)lParam);

		case SONORK_WIN_POKE_CHILD_DRAW_ITEM:
			return OnDrawItem(
				&((TSonorkChildDialogNotify*)lParam)->N->draw
				);

		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			break;
	}

	return 0;

}

// ----------------------------------------------------------------------------

static struct TMsgXlat
{
	SONORK_SID_MODE	sid_mode;
	UINT		ctrl_id;
	GLS_INDEX	gls;
}msg_xlat[3]=
{{SONORK_SID_MODE_BUSY		, IDC_SID_MSG_BUSY	, GLS_UM_BUSYM}
,{SONORK_SID_MODE_AT_WORK	, IDC_SID_MSG_WORK	, GLS_UM_WORKM}
,{SONORK_SID_MODE_AWAY		, IDC_SID_MSG_AWAY	, GLS_UM_AWAY}};

// ----------------------------------------------------------------------------

TSonorkSidModeWin::TSonorkSidModeWin(TSonorkWin*parent)
	:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|IDD_SID_MODE
		,0)
{
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_vis[]=
	{	{IDC_SID_VIS_F1	,	GLS_VG_01	}
	,	{IDC_SID_VIS_F2	,	GLS_VG_02	}
	,	{IDC_SID_VIS_F3	,	GLS_VG_03	}
	,	{IDC_SID_VIS_F4	,	GLS_VG_04	}
	,	{IDC_SID_VIS_F5	,	GLS_VG_05	}
	,	{IDC_SID_VIS_F6	,	GLS_VG_06	}
	,	{IDC_SID_VIS_F7	,	GLS_VG_07	}
	,	{IDC_SID_VIS_PRIVATE   |SONORK_WIN_CTF_BOLD
		,	GLS_SD_PRI	}
	,	{IDC_SID_VIS_PUBLIC	|SONORK_WIN_CTF_BOLD
		,	GLS_SD_PUB	}
	,	{IDC_SID_VIS_FRIENDLY
		,	GLS_SD_FRI	}
	,	{IDC_SID_VIS_INVITE    |SONORK_WIN_CTF_BOLD
		,	GLS_SD_TRK	}
	,	{IDC_SID_VIS_PUB_INVITE
		,	GLS_SD_AUTH	}
	,	{IDC_SID_VIS_HELP
		,	GLS_SD_HLP	}
	,	{0			,	GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_msg[]=
	{	{IDL_SID_MSG_BUSY	|SONORK_WIN_CTF_BOLD
		,	GLS_UM_BUSYM	}
	,	{IDL_SID_MSG_AWAY	|SONORK_WIN_CTF_BOLD
		,	GLS_UM_AWAY	}
	,	{IDL_SID_MSG_WORK	|SONORK_WIN_CTF_BOLD
		,	GLS_UM_WORKM	}
	,	{0			,	GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_table[]=
	{	{-1	, GLS_UM_VISIBILITY}
	,	{0	, GLS_NULL}
	};
	LoadLangEntries(gls_table,true);
	page[TAB_VIS]->LoadLangEntries(gls_vis, true);
	page[TAB_MSG]->LoadLangEntries(gls_msg, true);
}

// ----------------------------------------------------------------------------

bool
 TSonorkSidModeWin::OnCreate()
{
	RECT	rect;
	SIZE	sz;
	UINT	height;
	UINT	cx_border;
	TSonorkDynString	STR;
	int	i;
	union {
		TC_ITEM 	tc_item;
		char		tmp[64];
	}D;
	static UINT tab_dlg_id[TABS]=
	{
		IDD_SID_VIS
	,	IDD_SID_MSG
	};
	static GLS_INDEX gls_tab[TABS]=
	{
		GLS_LB_VISTY
	,	GLS_LB_SIDMSG
	};


	tab.hwnd=GetDlgItem(IDC_SID_MODE_TAB);
	TabCtrl_GetItemRect(tab.hwnd,0,&rect);
	height = (rect.bottom  - rect.top) + 1;
	::GetWindowRect(tab.hwnd,&rect);
	ScreenToClient(&rect);

	cx_border=GetSystemMetrics(SM_CXBORDER);
	sz.cx=cx_border + TAB_MARGIN;
	sz.cy=GetSystemMetrics(SM_CYBORDER) + TAB_MARGIN;
	rect.left 	+= sz.cx;
	rect.top  	+= height + sz.cy + TAB_MARGIN;
	rect.right	-= sz.cx;
	rect.bottom	-= sz.cy;


	D.tc_item.mask=TCIF_TEXT|TCIF_IMAGE;
	D.tc_item.lParam=0;
	D.tc_item.iImage=-1;
	for(i=0;i<TABS;i++)
	{
		D.tc_item.pszText=(char*)SonorkApp.LangString(gls_tab[i]);
		TabCtrl_InsertItem(tab.hwnd,i,&D.tc_item);
		page[i] = new TSonorkChildDialogWin(this
				,tab_dlg_id[i]
				,SONORK_WIN_SF_NO_CLOSE);
		TSonorkWin::CenterWin(page[i],rect,SONORK_CENTER_WIN_F_CREATE);
	}
	::SetWindowPos(tab.hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

	form_sid_flags.Set(SonorkApp.ProfileSidFlags());

	user_list 	= page[TAB_VIS]->GetDlgItem(IDC_SID_VIS_USERS);
	priv_check  	= page[TAB_VIS]->GetDlgItem(IDC_SID_VIS_PRIVATE);
	pub_check  	= page[TAB_VIS]->GetDlgItem(IDC_SID_VIS_PUBLIC);

	Transfer_Sid2Form();
	// ----------------------------------------------------
	// LOADING


	SetTab(TAB_VIS, true, true );

	for(i=0;i<3;i++)
	{
		wsprintf(D.tmp,"SidMsg%02u",msg_xlat[i].sid_mode);
		SonorkApp.ReadProfileItem(D.tmp,&STR);
		if(STR.Length()==0)
			STR.Set(SonorkApp.LangString(msg_xlat[i].gls));
		page[TAB_MSG]->SetCtrlText(msg_xlat[i].ctrl_id,STR.ToCStr());
		page[TAB_MSG]->SetEditCtrlMaxLength(msg_xlat[i].ctrl_id
				,SONORK_SID_MSG_MAX_SIZE);
	}
	return true;
}
void
 TSonorkSidModeWin::OnBeforeDestroy()
{
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::SetTab(TAB tb , bool update_tab, bool forced )
{
	int i;
	if( tb != tab.page || forced)
	{
		if( tb<0 || tb>=TABS)return;
		tab.page = tb;
		for(i=0;i<TABS;i++)
			page[i]->ShowWindow(i==tb?SW_SHOW:SW_HIDE);
		if( update_tab )
			TabCtrl_SetCurSel(tab.hwnd , tb );

	}
}


// ----------------------------------------------------------------------------

LRESULT
 TSonorkSidModeWin::OnChildDialogNotify(TSonorkChildDialogNotify*childN)
{
	TSonorkWinNotify*N;
	UINT	dialogId;
	UINT	itemId;
	UINT	code;

	N	= childN->N;
	itemId	= N->hdr.idFrom;
	code	= N->hdr.code;
	dialogId= childN->dialog_id;


	if( dialogId == IDD_SID_VIS )
	{
		if( code == BN_CLICKED )
		{
			switch( itemId )
			{

			case IDC_SID_VIS_PUBLIC:
			case IDC_SID_VIS_PRIVATE:
			case IDC_SID_VIS_INVITE:
			case IDC_SID_VIS_PUB_INVITE:
			case IDC_SID_VIS_FRIENDLY:
				// break ommited
			case IDC_SID_VIS_F1:
			case IDC_SID_VIS_F2:
			case IDC_SID_VIS_F3:
			case IDC_SID_VIS_F4:
			case IDC_SID_VIS_F5:
			case IDC_SID_VIS_F6:
			case IDC_SID_VIS_F7:
				Transfer_Form2Sid( itemId!=IDC_SID_VIS_PUBLIC , true );
				break;
			}
		}
		return true;
	}

	return 0;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkSidModeWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{

	if( N->hdr.hwndFrom == tab.hwnd)
	{
		if( N->hdr.code == TCN_SELCHANGE )
		{
			SetTab((TAB)TabCtrl_GetCurSel(tab.hwnd)
				, false
				, false );
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::EnableDisableCheckboxes()
{
	BOOL bBool;

	bBool=!form_sid_flags.IsPrivate();
	page[TAB_VIS]->SetCtrlEnabled(IDC_SID_VIS_PUBLIC,bBool);
	bBool=bBool&&!SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NOT_PUBLIC_SID);
	page[TAB_VIS]->SetCtrlEnabled(IDC_SID_VIS_FRIENDLY,bBool);

	bBool=!SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_INVITATIONS);
	page[TAB_VIS]->SetCtrlEnabled(IDC_SID_VIS_PUB_INVITE,bBool);
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::EnableDisablePrivateMask()
{
	BOOL enable;
	UINT  id=IDC_SID_VIS_F1;
	enable = form_sid_flags.IsPrivate();
	for(int i=0;i<SONORK_APP_VISIBILITY_GROUPS;i++)
	{
		page[TAB_VIS]->SetCtrlEnabled(id,enable);
		id++;
	}

}

// ----------------------------------------------------------------------------

bool TSonorkSidModeWin::OnCommand(UINT id,HWND , UINT code)
{
	char tmp[64];
	TSonorkDynString	STR;
	int	i;
	if( code == BN_CLICKED )
	{
		switch( id )
		{
			case IDOK:
				for(i=0;i<3;i++)
				{
					wsprintf(tmp,"SidMsg%02u",msg_xlat[i].sid_mode);
					page[TAB_MSG]->GetCtrlText(msg_xlat[i].ctrl_id,STR);
					STR.CutAt(SONORK_SID_MSG_MAX_SIZE);

					SonorkApp.WriteProfileItem(tmp,&STR);
				}
				Transfer_Form2Sid( true , false );
				SonorkApp.SetSidFlags( form_sid_flags );


			case IDCANCEL:
				Destroy(id);
				break;
		}
		return true;
	}
	return false;

}


// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::LoadUserList()
{
	UINT users=0;
	char	tmp[80];
	ListBox_Clear(user_list);
	if( form_sid_flags.IsPrivate() )
	{
		TSonorkListIterator	I;
		TSonorkExtUserData *UD;
		UINT			private_mask;

		private_mask	= form_sid_flags.PrivateMask();

		SonorkApp.UserList().BeginEnum(I);
		while( (UD=SonorkApp.UserList().EnumNext(I)) != NULL)
		{
			if( UD->UserType() != SONORK_USER_TYPE_AUTHORIZED )
				continue;
			if( !(UD->ctrl_data.auth.PrivateMask() & private_mask ))
				continue;
			users++;
			ListBox_AddString(user_list,UD->display_alias.CStr());
		}
		SonorkApp.UserList().EndEnum(I);
	}
	else
	{
		sprintf(tmp,"(%s)",SonorkApp.LangString(GLS_LB_EVRYBODY));
		ListBox_AddString(user_list,tmp);
		users=1;
	}
	if( users == 0 )
	{
		sprintf(tmp,"(%s)",SonorkApp.LangString(GLS_LB_NOBODY));
		ListBox_AddString(user_list,tmp);
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::Transfer_Form2Sid(bool private_mask , bool update_ui )
{
	int i;
	UINT	flag=SONORK_SID_F0_PRIVATE_1;
	UINT 	id=IDC_SID_VIS_F1;

	form_sid_flags.ClearPrivate();
	form_sid_flags.ClearPublic();
	form_sid_flags.DisableTracker();

	SonorkApp.ProfileCtrlFlags().Set(SONORK_PCF_NOT_PUBLIC_SID);
	SonorkApp.ProfileCtrlFlags().Clear(SONORK_PCF_PUBLIC_SID_WHEN_FRIENDLY);
	SonorkApp.ProfileCtrlFlags().Clear(SONORK_PCF_NO_INVITATIONS);
	SonorkApp.ProfileCtrlFlags().Clear(SONORK_PCF_NO_PUBLIC_INVITATIONS);

	if( page[TAB_VIS]->GetCtrlChecked(IDC_SID_VIS_PRIVATE) )
		form_sid_flags.SetPrivate();

	if( !page[TAB_VIS]->GetCtrlChecked(IDC_SID_VIS_INVITE) )
		SonorkApp.ProfileCtrlFlags().Set(SONORK_PCF_NO_INVITATIONS);

	if( page[TAB_VIS]->GetCtrlChecked(IDC_SID_VIS_PUB_INVITE) )
		SonorkApp.ProfileCtrlFlags().Set(SONORK_PCF_NO_PUBLIC_INVITATIONS);

	if( page[TAB_VIS]->GetCtrlChecked(IDC_SID_VIS_PUBLIC) )
		SonorkApp.ProfileCtrlFlags().Clear(SONORK_PCF_NOT_PUBLIC_SID);

	if( page[TAB_VIS]->GetCtrlChecked(IDC_SID_VIS_FRIENDLY) )
		SonorkApp.ProfileCtrlFlags().Set(SONORK_PCF_PUBLIC_SID_WHEN_FRIENDLY);


	if( update_ui )
		EnableDisableCheckboxes();

	if( private_mask )
	{
		for(i=0;i<SONORK_APP_VISIBILITY_GROUPS;i++)
		{
			if( page[TAB_VIS]->GetCtrlChecked(id) )
				form_sid_flags.Set((SONORK_SID_FLAG0)flag);
			else
				form_sid_flags.Clear((SONORK_SID_FLAG0)flag);
			flag<<=1;
			id++;
		}
		if( update_ui )
		{
			EnableDisablePrivateMask();
			LoadUserList();
		}
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::Transfer_Sid2Form()
{
	UINT 			flag;
	UINT  			id;

	page[TAB_VIS]->SetCtrlChecked(IDC_SID_VIS_PRIVATE
		, form_sid_flags.IsPrivate());

	page[TAB_VIS]->SetCtrlChecked(IDC_SID_VIS_PUBLIC
		, !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NOT_PUBLIC_SID));

	page[TAB_VIS]->SetCtrlChecked(IDC_SID_VIS_FRIENDLY
		, SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_PUBLIC_SID_WHEN_FRIENDLY));

	page[TAB_VIS]->SetCtrlChecked(IDC_SID_VIS_INVITE
		, !SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_INVITATIONS));

	page[TAB_VIS]->SetCtrlChecked(IDC_SID_VIS_PUB_INVITE
		, SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_PUBLIC_INVITATIONS));

	flag=SONORK_SID_F0_PRIVATE_1;
	id=IDC_SID_VIS_F1;
	for(int i=0;i<SONORK_APP_VISIBILITY_GROUPS;i++)
	{
		page[TAB_VIS]->SetCtrlChecked(id,form_sid_flags.Test((SONORK_SID_FLAG0)flag));
		flag<<=1;
		id++;
	}
	EnableDisableCheckboxes();
	EnableDisablePrivateMask();
	LoadUserList();

}



/*

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::CmdEditItem( TSonorkTrackerTreeItem* VI)
{
	UINT	id;
	TSonorkTrackerItemPtrs	IP;
	TSonorkTrackerRoom*  	room;
	TSonorkTrackerData*  	data;
	bool	is_new_data;
	int	e_count, b_count;
	if( VI->Type() != SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
	|| !TestWinUsrFlag(UWF_TRACKER_TREE_LOADED))
		return;
	IP.item=VI;
	data	= IP.subs->data;
	room	= IP.subs->room;
	b_count = IP.subs->GetBoldCount();
	e_count = IP.subs->GetEventCount();
	if( data == NULL )
	{
		is_new_data = true;
		SONORK_MEM_NEW( data = new TSonorkTrackerData );
		data->CODEC_Clear();
		data->header.id.Set(IP.subs->room->header.id);
		data->header.flags.Set(IP.subs->room->header.flags);
		data->header.flags.Set(SONORK_TRACKER_DATA_F0_ACTIVE);
	}
	else
		is_new_data = false;
		
	id = TSonorkTrackerSubsWin(this
		,room
		,data).Execute();

	if( id == IDOK )
	{
		IP.subs->data = data ;
	}
	else
	{
		if(id == IDC_TRKSUBS_DEL )
		{
			IP.subs->data = NULL;
		}
		if( is_new_data || id == IDC_TRKSUBS_DEL)
			SONORK_MEM_DELETE( data );

	}
	tree.UpdateItemAttributes(
		  IP.ptr
		, IP.ptr->GetBoldCount()  - b_count
		, IP.ptr->GetEventCount() - e_count
		, 0
		);
	SetWinUsrFlag(UWF_TRACKERS_MODIFIED);

}



// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::LoadSubscriptions()
{
	TSonorkAtomDb		db;
	TSonorkTrackerData*	data;
	UINT			ci,mi;
	TSonorkTrackerItemPtrs	IP;

	if(!SonorkApp.OpenAppDb(SONORK_APP_DB_TRACKER_SUBS,db,false))
		return;
	if((mi=db.Items())!=0)
	{
		SONORK_MEM_NEW( data = new TSonorkTrackerData );
		for(ci=0;ci<mi;ci++)
		if(db.Get(ci,data,NULL) == SONORK_RESULT_OK)
		{
			IP.ptr = tree.FindItem(SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
				, data->header.id.v[0]);
			if( IP.item )
			if( IP.subs->data == NULL )
			{
				IP.subs->data = data ;
				tree.UpdateItemAttributes(
					  IP.ptr
					, IP.subs->GetBoldCount()
					, IP.subs->GetEventCount()
					, 0
					);
				SONORK_MEM_NEW( data = new TSonorkTrackerData );
			}
		}
		SONORK_MEM_DELETE( data );
	}
	db.Close();
	ClearWinUsrFlag(UWF_TRACKERS_MODIFIED);
}

// ----------------------------------------------------------------------------

UINT
 TSonorkSidModeWin::SaveSubscriptionItems(TSonorkAtomDb& db,HTREEITEM hItem)
{
	UINT cc=0;
	TSonorkTrackerItemPtrs	IP;

	for(IP.ptr = tree.GetChild(hItem)
	   ;IP.ptr != NULL
	   ;IP.ptr = tree.GetNextSibling(IP.ptr))
	{
		if( IP.ptr->Type() == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS )
		{
			if(IP.subs->data != NULL )
			{
				db.Add( IP.subs->data );
				if( IP.subs->data->header.flags.Test(SONORK_TRACKER_DATA_F0_ACTIVE) )
					cc++;
			}
		}
		else
			cc+=SaveSubscriptionItems( db , IP.ptr->hItem() );
	}
	return cc;
}
void
 TSonorkSidModeWin::SaveSubscriptions()
{
	TSonorkAtomDb		db;
	if(!TestWinUsrFlag(UWF_TRACKER_TREE_LOADED)
	|| !TestWinUsrFlag(UWF_TRACKERS_MODIFIED))
		return;

	if(!SonorkApp.OpenAppDb(SONORK_APP_DB_TRACKER_SUBS,db,true))
		return;
	

	if(SaveSubscriptionItems(db,TVI_ROOT) != 0)
		SonorkApp.RequestSynchTrackers( true );

	db.Close();
	ClearWinUsrFlag(UWF_TRACKERS_MODIFIED);

}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::LoadTrackerTree(BOOL auto_load)
{
	BOOL task_pending;
	if( auto_load )
	{
		if( TestWinUsrFlag(UWF_TRACKER_TREE_LOADED)
		|| !MayStartTask(NULL,SONORK_WIN_TASK_SONORK))
		return;
	}

	if(TSonorkTrackViewWin::StartTrackerTreeTask(
		  this
		, taskERR
		, tree
		, auto_load
		  ?SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
		  :SONORK_TREE_ITEM_NONE
		, task_pending
		  ))
	{
		if( task_pending )
			return;
		if(taskERR.Result() == SONORK_RESULT_OK )
			PostPoke(POKE_LOADED_OK
				, false // Don't save to cache
				);
		else
			PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT
				, SONORK_FUNCTION_LIST_TRACKERS);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&)
{}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if( P->Function() == SONORK_FUNCTION_LIST_TRACKERS )
		TSonorkTrackViewWin::ProcessTrackerTreeTaskData(tree
				, SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
				, P
				, P_size);
}

// ----------------------------------------------------------------------------

void
 TSonorkSidModeWin::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&,const TSonorkError*)
{
	PostPoke( POKE_LOADED_OK
		, true // Save result to cache
		);
}

		case POKE_LOADED_OK:
			SetWinUsrFlag(UWF_TRACKER_TREE_LOADED);
			LoadSubscriptions();
			tree.AfterMassOp();
			if( lParam )
				TSonorkTrackViewWin::SaveTrackerListToCache(tree);
			break;
		case POKE_EDIT_ITEM:
			CmdEditItem( (TSonorkTrackerTreeItem*)lParam );
			break;

				tree.DelAllItems();

*/
