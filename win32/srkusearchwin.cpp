#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkusearchwin.h"
#include "srktrackerwin.h"
#include "srkmyinfowin.h"

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

#define MIN_ITEMS_IN_CACHE		5
#define UWF_TRACKER_TREE_LOADED		SONORK_WIN_F_USER_01
#define UWF_TRACKER_USING_CACHE		SONORK_WIN_F_USER_02
#define UWF_LOCAL_NOTES_OK		SONORK_WIN_F_USER_03
#define UWF_REQUEST_AUTH		SONORK_WIN_F_USER_04
#define POKE_LIST_MEMBERS		SONORK_WIN_POKE_01
enum STATUS_BAR_SECTION
{
  STATUS_BAR_SECTION_TASK
, STATUS_BAR_SECTION_ADD_USER
, STATUS_BAR_SECTIONS
};
static int  status_bar_section_offset[STATUS_BAR_SECTIONS]=
{	 210
	,-1
};
void
 TSonorkUSearchWin::SetSelectedUser( TSonorkUserDataNotes*UDN )
{
	if(UDN)
	{
		BOOL b=UDN->user_data.userId!=SonorkApp.ProfileUserId();
		SetCtrlText(IDL_USEARCH_USER_ALIAS,UDN->user_data.alias.CStr());
		SetCtrlText(IDC_USEARCH_USER_TEXT,UDN->notes.ToCStr());
		SetCtrlEnabled(IDC_USEARCH_CALL_USER
			,  UDN->user_data.IsOnline()
			&& UDN->user_data.addr.sidFlags.TrackerEnabled()
			&& b);
		SetCtrlEnabled(IDC_USEARCH_USER_INFO , b);
		SetCtrlEnabled(IDC_USEARCH_ADD_USER  , b);
	}
	else
		ClearSelectedUser();
}

void
 TSonorkUSearchWin::SetSelectedMember(TSonorkTrackerMember* M)
{
	if( M )
	{
		BOOL b=M->user_data.userId!=SonorkApp.ProfileUserId();
		SetCtrlText(IDL_USEARCH_USER_ALIAS,M->user_data.alias.CStr());
		SetCtrlText(IDC_USEARCH_USER_TEXT,M->track_data.text.CStr());
		SetCtrlEnabled(IDC_USEARCH_CALL_USER
			,  M->user_data.IsOnline()
			&& M->user_data.addr.sidFlags.TrackerEnabled()
			&& b);
		SetCtrlEnabled(IDC_USEARCH_USER_INFO , b);
		SetCtrlEnabled(IDC_USEARCH_ADD_USER  , b);
	}
	else
		ClearSelectedUser();

}
void
 TSonorkUSearchWin::ClearSelectedUser()
{
	SetCtrlEnabled(IDC_USEARCH_CALL_USER,false);
	SetCtrlEnabled(IDC_USEARCH_USER_INFO,false);
	SetCtrlEnabled(IDC_USEARCH_ADD_USER,false);
	SetCtrlText(IDL_USEARCH_USER_ALIAS,GLS_IF_NUSEL);
	ClrCtrlText(IDC_USEARCH_USER_TEXT);
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::LoadTrackerList(const TSonorkTrackerRoom* room)
{
	BOOL task_pending;
	ClearRoomItems();
	current_task = LOAD_ROOM_LIST;
	if(StartTrackerListTask(
		  this
		, taskERR
		, track.list
		, room->header.id
		, task_pending
		  ))
	{
		if( task_pending )
			return;
		PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT,0);
	}
}
// ----------------------------------------------------------------------------
void
 TSonorkUSearchWin::ClearRoomItems()
{
	SetWinUsrFlag(SONORK_WIN_F_UPDATING);
	track.list.DelAllItems();
	SetSelectedMember(NULL);
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);

}
// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::LoadTrackerTree(BOOL auto_load)
{
	BOOL task_pending;

	if( auto_load )
	{
		if( TestWinUsrFlag(UWF_TRACKER_TREE_LOADED)
		|| !MayStartTask(NULL,SONORK_WIN_TASK_SONORK))
		return;
	}

	
	current_task = LOAD_ROOM_TREE;
	ClearRoomItems();
	ClearWinUsrFlag(UWF_TRACKER_USING_CACHE);


	if(StartTrackerTreeTask(
		  this
		, taskERR
		, track.tree
		, auto_load
		  ?SONORK_TREE_ITEM_TRACKER_ROOM
		  :SONORK_TREE_ITEM_NONE
		, task_pending
		, NULL
		  ))
	{
		if( task_pending )
			return;

		if(taskERR.Result() == SONORK_RESULT_OK )
			SetWinUsrFlag(UWF_TRACKER_USING_CACHE);

		PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT,0);
	}
}
// ----------------------------------------------------------------------------

LRESULT
  TSonorkUSearchWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch( wParam )
	{
		case POKE_LIST_MEMBERS:
			if( !IsTaskPending() && lParam != 0)
			{
				TSonorkTrackerRoomItem* VI;
				VI=(TSonorkTrackerRoomItem*)lParam;
				if(!VI->IsRoom() )
					break;
				LoadTrackerList(VI->room);
			}
			break;

		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			if(taskERR.Result() == SONORK_RESULT_NO_DATA)
				taskERR.SetOk();
			else
			if(taskERR.Result() != SONORK_RESULT_OK)
			{
				TaskErrorBox(
					current_task==ADD_USER
					?GLS_TK_ADDUSR
					:GLS_TK_GETINFO
					, &taskERR);
				break;
			}
			if( current_task == LOAD_ROOM_TREE )
			{
				SetWinUsrFlag(UWF_TRACKER_TREE_LOADED);
				AfterTrackerTreeLoad(
					 track.tree
					,TestWinUsrFlag(UWF_TRACKER_USING_CACHE)
					,true);
			}
			else
			if( current_task == LOAD_ROOM_LIST )
			{
				if(track.list.GetCount())
					track.list.SelectItem(0);
				UpdateUsersFoundStatus(track.list.GetCount());
			}
			else
			if( current_task == SEARCH_USERS )
			{
				if(users.list.GetCount())
					users.list.SelectItem(0);
				UpdateUsersFoundStatus(users.list.GetCount());
			}
			else
			if( current_task == ADD_USER )
			{
				ErrorBox(GLS_AUTHREQSENT,NULL,SKIN_SIGN_USERS);
			}

			break;
	}

	return 0;

}


// ----------------------------------------------------------------------------

TSonorkUSearchWin::TSonorkUSearchWin(
	  DWORD 	group_no
	, SONORK_C_CSTR group_name
	, BOOL 		request_auth)
	:TSonorkTaskWin(NULL
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|IDD_USEARCH
		,SONORK_WIN_SF_NO_WIN_PARENT)
{
	SetAddGroup(group_no,group_name,request_auth);
}
void
 TSonorkUSearchWin::SetAddGroup(
	  DWORD 	group_no
	, SONORK_C_CSTR group_name
	, BOOL 		request_auth)
{
	addGroupNo = group_no;
	addGroupName.Set(group_name);
	UpdateAddGroupStatus(request_auth);
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::UpdateAddGroupStatus(BOOL request_auth)
{
	char tmp[128];
	int l;
	if(request_auth)
		SetWinUsrFlag(UWF_REQUEST_AUTH);
	else
		ClearWinUsrFlag(UWF_REQUEST_AUTH);
	if(Handle())
	{
		l=wsprintf(tmp,"%s: %s"
			,SonorkApp.LangString(GLS_OP_ADD)
			,SonorkApp.LangString(
				 TestWinUsrFlag(UWF_REQUEST_AUTH)
				?GLS_LB_AUTHUSRS:GLS_LB_RUSRS));
		if( addGroupNo != 0)
		{
			wsprintf(tmp+l,", %s: %s"
				,SonorkApp.LangString(GLS_LB_GRP)
				,addGroupName.CStr());
		}
		::SendMessage(status_bar
			,SB_SETTEXT
			,STATUS_BAR_SECTION_ADD_USER
			,(LPARAM)tmp);
	}

}

// ----------------------------------------------------------------------------

void
  TSonorkUSearchWin::UpdateUsersFoundStatus(UINT n)
{
	char 		tmp[128];
	if(n==0)
		strcpy(tmp,SonorkApp.LangString(GLS_MS_USR_NFOUND));
	else
		SonorkApp.LangSprintf(tmp,GLS_MS_USR_FOUND,n);
	::SendMessage(status_bar
		,SB_SETTEXT
		,STATUS_BAR_SECTION_TASK
		,(LPARAM)tmp);
	::SendMessage(status_bar
		,SB_SETICON
		,STATUS_BAR_SECTION_TASK
		,(LPARAM)sonork_skin.Hicon(SKIN_HICON_INFO));
}

// ----------------------------------------------------------------------------

static void
 LoadCombo(HWND hwnd,GLS_INDEX* gls, int items)
{
	for(int i=0;i<items;i++,gls++)
	{
		ComboBox_AddString(hwnd,SonorkApp.LangString(*gls));
	}
	ComboBox_SetCurSel(hwnd,0);
}

// ----------------------------------------------------------------------------

bool
 TSonorkUSearchWin::OnCreate()
{
	union {
		TC_ITEM 	tc_item;
	const	TSonorkLangCodeRecord* langREC;
	}D;
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDL_USEARCH_USER_ALIAS	|SONORK_WIN_CTF_BOLD
		,	GLS_IF_NUSEL	}
	,	{IDC_USEARCH_USER_TEXT
		, 	GLS_NULL	}
	,	{IDC_USEARCH_REFRESH
		,	GLS_OP_REFRESH	}
	,	{IDC_USEARCH_USER_INFO
		,	GLS_LB_INFO	}
	,	{IDC_USEARCH_CALL_USER
		,	GLS_TR_CALL	}
	,	{IDC_USEARCH_ADD_USER
			, GLS_OP_ADDUSR	}
	,	{IDC_USEARCH_OFFLINE
		, 	GLS_LB_OFLUSRS	}
	,	{IDCANCEL
		,	GLS_OP_CLOSE	}
	,	{-1
		,	GLS_OP_FNDUSRS}
	,	{0,GLS_NULL}
	};
	static GLS_INDEX
		gls_sex[3]={GLS_LB_EVRYBODY , GLS_LB_SEXF , GLS_LB_SEXM};

	static GLS_INDEX

		gls_type[6]={GLS_LB_NAME , GLS_LB_ALIAS, GLS_LB_EMAIL,  GLS_LB_CALSGN, GLS_LB_SRKID, GLS_LB_NOTES};

	static GLS_INDEX
		gls_mode[3]={GLS_CR_NBUSY , GLS_CR_ONFRI , GLS_LB_EVRYBODY };


	static TSonorkListViewColumn
		user_list_cols[]=
	{       {GLS_LB_SRKID	, 70}
	,	{GLS_LB_ALIAS 	, 96}
	,	{GLS_LB_NAME	, 128}
	,	{GLS_LB_LANG	, 96}
	,	{GLS_NULL	, SKIN_ICON_SW}
	,	{GLS_NULL	, SKIN_ICON_SW}
	};

	SetWinUsrFlag(SONORK_WIN_F_UPDATING);
	LoadLangEntries(gls_table,true);

	track.tree.SetHandle( GetDlgItem(IDC_USEARCH_ROOMS ) );
	track.list.SetHandle( GetDlgItem(IDC_USEARCH_MEMBERS )
		, true , true);

	SetupTrackerListView( track.list );

	users.list.SetHandle( GetDlgItem(IDC_USEARCH_USERS)
		, true
		, false);
	users.list.SetStyle(
		  LVS_REPORT
		  |LVS_SHAREIMAGELISTS
		  |LVS_SINGLESEL
		  |LVS_SHOWSELALWAYS
		,LVS_TYPEMASK
		  |LVS_SHAREIMAGELISTS
		  |LVS_SINGLESEL
		  |LVS_SHOWSELALWAYS
		  |LVS_EDITLABELS
		  |LVS_NOSCROLL
		  |LVS_SORTASCENDING
		  |LVS_SORTDESCENDING

		,LVS_EX_FULLROWSELECT
		 |LVS_EX_SUBITEMIMAGES
		,LVS_EX_FULLROWSELECT
		 |LVS_EX_GRIDLINES
		 |LVS_EX_INFOTIP
		 |LVS_EX_HEADERDRAGDROP
		 |LVS_EX_MULTIWORKAREAS
		 |LVS_EX_FLATSB
		 |LVS_EX_SUBITEMIMAGES
		 |LVS_EX_CHECKBOXES
		 |LVS_EX_REGIONAL
		 |LVS_EX_ONECLICKACTIVATE);
	users.list.AddColumns(6,user_list_cols);
	status_bar	= GetDlgItem(IDC_USEARCH_STATUS);

	search.type	= GetDlgItem(IDC_USEARCH_FIELD_TYPE);
	search.text	= GetDlgItem(IDC_USEARCH_FIELD_TEXT);
	search.sex   	= GetDlgItem(IDC_USEARCH_SEX);

	search.mode	= GetDlgItem(IDC_USEARCH_SID_MODE);

	search.lang	= GetDlgItem(IDC_USEARCH_COUNTRY);

	LoadCombo(search.type,gls_type,6);

	LoadCombo(search.sex,gls_sex,3);

	LoadCombo(search.mode,gls_mode,3);



	ComboBox_AddString(search.lang,"*");

	D.langREC=SonorkApp.LanguageCodeTable().Table();

	for(int i=0;i<SonorkApp.LanguageCodeTable().Items();i++,D.langREC++)

		ComboBox_AddString(search.lang,D.langREC->name);

	ComboBox_SetCurSel(search.lang,0);

	tab.hwnd = GetDlgItem(IDC_USEARCH_TAB);

	tab.page = PAGE_USERS;

	D.tc_item.mask=TCIF_TEXT;
	D.tc_item.lParam=0;
	D.tc_item.pszText=(char*)SonorkApp.LangString(GLS_LB_USRS);
	TabCtrl_InsertItem(tab.hwnd,0,&D.tc_item);
	D.tc_item.pszText=(char*)SonorkApp.LangString(GLS_TR_ROOMS);
	TabCtrl_InsertItem(tab.hwnd,1,&D.tc_item);

	::SetWindowPos(tab.hwnd
		,HWND_BOTTOM
		,0,0,0,0
		,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	SonorkApp.OnSysDialogRun(this
		,SONORK_SYS_DIALOG_USER_SEARCH
		,true
		,"AddUserWin");

	EnableDisableItems();
	SetTab( PAGE_USERS , true , true );
	ClearSelectedUser();
	::SendMessage(status_bar
		, SB_SETPARTS
		, STATUS_BAR_SECTIONS
		, (LPARAM)status_bar_section_offset);

	UpdateAddGroupStatus(TestWinUsrFlag(UWF_REQUEST_AUTH));
	SetCtrlChecked(IDC_USEARCH_OFFLINE,true);
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);


	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::OnBeforeDestroy()
{
	track.tree.DelAllItems();
	track.list.DelAllItems();
	users.list.DelAllItems();

	SonorkApp.OnSysDialogRun(this
		,SONORK_SYS_DIALOG_USER_SEARCH
		,false
		,"AddUserWin");
}

// ----------------------------------------------------------------------------
void
 TSonorkUSearchWin::SearchOnlineUsers()
{
	UINT P_size,A_size;
	TSonorkDataPacket *	P;
	TSonorkUserDataNotes	UDN;
	int			i;
	char			str[48];
const	TSonorkLangCodeRecord* langREC;
	DWORD			search_flags=
				SONORK_FUNC_F_SEARCH_USER_NEW
				|SONORK_FUNC_NF_SEARCH_USER_STATE_ONLINE;

	if( IsTaskPending() )
		return;
	/*
	TSonorkWin*W;
	if( !TestWinUsrFlag( UWF_LOCAL_NOTES_OK ) )
	{
		SonorkApp.ReadProfileItem("Notes",&UDN.notes);
		if(UDN.notes.Length()<2)
		{
			MessageBox(GLS_TR_NOSCH
				,GLS_OP_FNDONLUSRS
				,MB_OK|MB_ICONWARNING);

			W=SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_MY_INFO);
			W->PostPoke(SONORK_WIN_POKE_SET_TAB
				, TSonorkMyInfoWin::TAB_PIC);
			return;
		}
		UDN.notes.Clear();
		SetWinUsrFlag(UWF_LOCAL_NOTES_OK);
	}
	*/

	SetWinUsrFlag(SONORK_WIN_F_UPDATING);
	SetSelectedUser( NULL );
	users.list.DelAllItems();
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);

	UDN.Clear();
	UDN.user_data.userId.Clear();
	UDN.user_data.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1,false);
	UDN.user_data.Clear(false);

	GetCtrlText(IDC_USEARCH_FIELD_TEXT,str,sizeof(str));


	i = ComboBox_GetCurSel(search.type);
	switch(i)
	{

		case 0:	// Name
			UDN.user_data.name.Set(str);
			break;

		case 1: // alias
		case 3: // call sign
			UDN.user_data.alias.Set(str);
			break;

		case 2: // email
			UDN.user_data.email.Set(str);
			break;

		case 4: // user id
			if(!UDN.user_data.userId.SetStr(str))
			{
				MessageBox(GLS_MS_INVGUID,szSONORK,MB_OK|MB_ICONINFORMATION);
				return;
			}
			break;

		case 5: // Notes
			UDN.notes.Set(str);
			break;
		default:
			return;
	}

	search_flags|=(i==3)	// 3=Call sign
			?SONORK_FUNC_NF_SEARCH_USER_METHOD_CALL_SIGN
			:SONORK_FUNC_NF_SEARCH_USER_METHOD_USER_INFO;

	i = ComboBox_GetCurSel(search.sex);
	if(i==1)
		i=SONORK_SEX_F;
	else
	if(i==2)
		i=SONORK_SEX_M;
	else
		i=SONORK_SEX_NA;

	UDN.user_data.wUserInfo()->infoFlags.SetSex((SONORK_SEX)i);



	if(GetCtrlChecked(IDC_USEARCH_OFFLINE))
		search_flags|=SONORK_FUNC_NF_SEARCH_USER_STATE_OFFLINE;

	// None of the checkboxes set, act as is both were checked
	if(!(search_flags&SONORK_FUNC_NM_SEARCH_USER_STATE))
	{
		search_flags|=SONORK_FUNC_NF_SEARCH_USER_STATE_ANY;
	}

	i = ComboBox_GetCurSel(search.mode);
	if(i==0)
		search_flags|=SONORK_FUNC_NF_SEARCH_USER_SID_MODE_NOT_BUSY;
	else
	if(i==1)
		search_flags|=SONORK_FUNC_NF_SEARCH_USER_SID_MODE_FRIENDLY;


	str[0]='*';
	i = ComboBox_GetCurSel(search.lang);
	if( i!=CB_ERR )
		ComboBox_GetString(search.lang,i,str);
	if(*str!='*')
	{
		langREC=SonorkApp.LanguageCodeTable().Table();
		for(i=0;i<SonorkApp.LanguageCodeTable().Items();i++,langREC++)

		{

			if(!strcmp(langREC->name,str))

			{

				UDN.user_data.wUserInfo()->region.SetLanguage(langREC->code);

				break;

			}

		}

	}


	current_task = SEARCH_USERS;
	A_size = CODEC_Size(&UDN,SONORK_UDN_ENCODE_NOTES)+64;
	P=SONORK_AllocDataPacket(A_size);
	P_size=P->E_SearchUser_R_NEW(A_size
		,UDN
		,SONORK_UDN_ENCODE_NOTES
		,search_flags
		,512);
	StartSonorkTask(taskERR
			, P
			, P_size
			, 0
			, GLS_TK_SEARCH
			, NULL);
	SONORK_FreeDataPacket( P );
}

void
 TSonorkUSearchWin::OnCommand_Refresh()
{
	if( tab.page == PAGE_ROOMS )
	{
		LoadTrackerTree( false );
	}
	else
	{
		SearchOnlineUsers();
	}
}

void
 TSonorkUSearchWin::OnCommand_User(UINT id, LPARAM lParam)
{
	union {
		LPARAM			lParam;
		TSonorkTrackerMember*	member;
		TSonorkUserDataNotes*	UDN;
	}D;
	TSonorkTempBuffer	tmp(256);
	D.lParam=lParam;
	TSonorkUserData*	UD;
	TSonorkExtUserData*	localUD;
	if( id == IDC_USEARCH_USER_INFO )
	{
		if( tab.page == PAGE_ROOMS )
		{
			SonorkApp.OpenUserDataWin(&D.member->user_data
					, NULL
					, this
					);
		}
		else
		{
			SonorkApp.OpenUserDataWin(
				  D.UDN
				, this);

		}
	}
	else
	if( id == IDC_USEARCH_CALL_USER )
	{
		if( tab.page == PAGE_ROOMS )
		{
			UD =&D.member->user_data;
		}
		else
		{
			UD =&D.UDN->user_data;
		}
		if( !UD->addr.sidFlags.TrackerEnabled() )
		{
			SonorkApp.LangSprintf(tmp.CStr()
				, GLS_TR_USRNENA
				, UD->alias.CStr());
			MessageBox(tmp,GLS_TR_NAME, MB_OK|MB_ICONSTOP);
			return;
		}
		TSonorkTrackerWin::Init(UD , "");
	}
	else
	if( id == IDC_USEARCH_ADD_USER )
	{

		TSonorkAuth2    auth;
		if( tab.page == PAGE_ROOMS )
		{
			UD =&D.member->user_data;
		}
		else
		{
			UD =&D.UDN->user_data;
		}
		if( TestWinUsrFlag(UWF_REQUEST_AUTH) )
		{
			current_task = ADD_USER;
			SonorkApp.CmdAddUser(this,taskERR,UD->userId,addGroupNo);
			return;
		}
		localUD=SonorkApp.UserList().Get(UD->userId);
		if( localUD != NULL )
		{
			SonorkApp.OpenUserDataWin(localUD,0);
			return;
		}
		auth.flags.Clear();
		auth.flags.SetUserInfoLevel(SONORK_USER_INFO_LEVEL_1);
		SonorkApp.AddRemoteUser(UD->userId
			,UD->alias.CStr()
			,&auth);
		MessageBox(GLS_AUTHREQSENT,csNULL,MB_OK|MB_ICONINFORMATION);
	}
}

// ----------------------------------------------------------------------------
bool
 TSonorkUSearchWin::OnCommand(UINT id,HWND , UINT code)
{
	LPARAM lParam;
	if( code == BN_CLICKED )
	{
		switch(id)
		{
		case IDCANCEL:
		case IDOK:
			Destroy(id);
			break;

		case IDC_USEARCH_REFRESH:
			OnCommand_Refresh();
			break;
		case IDC_USEARCH_USER_INFO:
		case IDC_USEARCH_CALL_USER:
		case IDC_USEARCH_ADD_USER:
			if( tab.page == PAGE_ROOMS )
				lParam = track.list.GetSelectedItem();
			else
				lParam = users.list.GetSelectedItem();
			if(lParam==0)
			{
				ErrorBox( GLS_IF_NUSEL );
				break;
			}

			OnCommand_User( id , lParam );
			break;

		default:
			return false;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::SetTab( PAGE p, bool forced, bool update_tab)
{
	static page_users_items[]=
	{IDC_USEARCH_SID_MODE
	,IDC_USEARCH_SEX	, IDC_USEARCH_COUNTRY
	,IDC_USEARCH_OFFLINE
	,IDC_USEARCH_FIELD_TYPE	, IDC_USEARCH_FIELD_TEXT
	,IDC_USEARCH_USERS
	,0};
	static page_room_items[]=
	{IDC_USEARCH_ROOMS	, IDC_USEARCH_MEMBERS,0};
	int i;
	bool	show;
	if( p!=tab.page || forced )
	{
		tab.page = p;
		show = p==PAGE_ROOMS;
		for(i=0;page_room_items[i]!=0;i++)
			SetCtrlVisible(page_room_items[i],show);
		show = p==PAGE_USERS;
		for(i=0;page_users_items[i]!=0;i++)
			SetCtrlVisible(page_users_items[i],show);
		if( update_tab )
			TabCtrl_SetCurSel(tab.hwnd , tab.page);
		if(p==PAGE_ROOMS)
		{
			SetSelectedMember(
				(TSonorkTrackerMember*)track.list.GetSelectedItem());
			LoadTrackerTree(true);
		}
		else
		{
			SetSelectedUser(
				(TSonorkUserDataNotes*)users.list.GetSelectedItem());

		}
		SetCtrlText(IDC_USEARCH_REFRESH,
			p==PAGE_ROOMS
			?GLS_OP_REFRESH
			:GLS_OP_SEARCH);
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::EnableDisableItems()
{
	BOOL may_execute;
	may_execute=MayStartTask(NULL,SONORK_WIN_TASK_SONORK);
	SetCtrlEnabled(IDC_USEARCH_REFRESH,may_execute);
}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&)
{
	EnableDisableItems();
	::SendMessage(status_bar
			,SB_SETTEXT
			,STATUS_BAR_SECTION_TASK
			,(LPARAM)SonorkApp.LangString(GLS_MS_PWAIT));
	::SendMessage(status_bar
			,SB_SETICON
			,STATUS_BAR_SECTION_TASK
			,(LPARAM)sonork_skin.Hicon(SKIN_HICON_BUSY));


}

void
 TSonorkUSearchWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if( P->Function() == SONORK_FUNCTION_LIST_TRACKERS )
	{
	   if(current_task == LOAD_ROOM_TREE)
		ProcessTrackerTreeTaskData(track.tree
			, SONORK_TREE_ITEM_TRACKER_ROOM
			, P
			, P_size);
	   else
	   if(current_task == LOAD_ROOM_LIST)
		ProcessTrackerListTaskData(track.list
			, P
			, P_size);

	}
	else
	if(P->Function() == SONORK_FUNCTION_SEARCH_USER_NEW)
	{
		if(current_task == SEARCH_USERS)
		{
			SonorkApp.ProcessSearchUserTaskData(&users.list
				,P
				,P_size
				,true
				,SearchUserCallback);
		}
	}
}

void
 TSonorkUSearchWin::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&,const TSonorkError*pERR)
{
	taskERR.Set( *pERR);
	::SendMessage(status_bar
			,SB_SETTEXT
			,STATUS_BAR_SECTION_TASK
			,(LPARAM)"");
	::SendMessage(status_bar
			,SB_SETICON
			,STATUS_BAR_SECTION_TASK
			,(LPARAM)sonork_skin.Hicon(SKIN_HICON_NONE));
	EnableDisableItems();
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT , 0);

}
void
 TSonorkUSearchWin::DoDrag(const TSonorkUserData& UD)
{
	TSonorkDropSourceData*	SD;
	TSonorkClipData*	CD;
	DWORD 			aux;

	SD = new TSonorkDropSourceData;
	
	CD=SD->GetClipData();
	CD->SetUserData( &UD );
	SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);

	SD->Release();
}
// ----------------------------------------------------------------------------

LRESULT
 TSonorkUSearchWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	POINT	pt;
	union {
		LPARAM			lParam;
		TSonorkTreeItem*	tview;
		TSonorkTrackerMember*   member;
		TSonorkUserDataNotes*	user;
	}D;
	if( N->hdr.hwndFrom == track.tree.Handle() )
	{
		switch(N->hdr.code)
		{

		case NM_CUSTOMDRAW:
			return TSonorkTreeItem::OnCustomDraw( (NMTVCUSTOMDRAW*)N);

		case TVN_GETDISPINFO:		// asking for presentation
			return TSonorkTreeItem::OnGetDispInfo((TV_DISPINFO*)N);

		case TVN_DELETEITEM:       // a item is being deleted

			if((D.lParam = N->tview.itemOld.lParam) != 0)
			{
				SONORK_MEM_DELETE( D.tview );
			}
			break;

		case TVN_SELCHANGED:
			if( TestWinUsrFlag(SONORK_WIN_F_UPDATING )
			||  TestWinSysFlag(SONORK_WIN_SF_DESTROYING))
				break;

			TRACE_DEBUG("TVN_SELCHANGED %x"
				,N->tview.itemNew.lParam);
			if(!IsTaskPending())
				PostPoke( POKE_LIST_MEMBERS
				, N->tview.itemNew.lParam );

		break;

		case NM_CLICK:
		case NM_DBLCLK:
			if( TestWinUsrFlag(SONORK_WIN_F_UPDATING )
			||  TestWinSysFlag(SONORK_WIN_SF_DESTROYING))
				break;
			::GetCursorPos(&pt);
			D.tview = track.tree.HitTest(
					 pt.x
					,pt.y
					,N->hdr.code==NM_CLICK
					?TVHT_ONITEMICON
					:TVHT_ONITEM|TVHT_ONITEMINDENT|TVHT_ONITEMRIGHT);
			if(!D.tview)break;
			if(N->hdr.code==NM_CLICK)
			{
				track.tree.ExpandItemLevels(D.tview, TVE_TOGGLE);
			}
			else
			{
				if(!IsTaskPending() )
					PostPoke( POKE_LIST_MEMBERS, D.lParam );
			}
			break;

		}
	}
	else
	if( N->hdr.hwndFrom == track.list.Handle() )
	{
		switch(N->hdr.code)
		{

		case LVN_BEGINDRAG:
			D.lParam = track.list.GetItem(N->lview.iItem);
			if(D.lParam )
			{
				DoDrag( D.member->user_data );
			}
			break;

		case LVN_ITEMCHANGED:
			if( !TestWinUsrFlag(SONORK_WIN_F_UPDATING) )
			if( (N->lview.iItem != -1)
			&&  (N->lview.uNewState&LVIS_FOCUSED) )
			{
				SetSelectedMember((TSonorkTrackerMember*)(N->lview.lParam));
			}
			break;

		case LVN_DELETEITEM:
			if((D.lParam=N->lview.lParam) != 9)
			{
				SONORK_MEM_DELETE( D.member );
			}
			break;
		}
	}
	else
	if( N->hdr.hwndFrom == users.list.Handle() )
	{
		switch(N->hdr.code)
		{
		case LVN_BEGINDRAG:

			D.lParam = users.list.GetItem(N->lview.iItem);
			if(D.lParam )
			{
				DoDrag( D.user->user_data );
			}

		break;
		case LVN_ITEMCHANGED:
			if( !TestWinUsrFlag(SONORK_WIN_F_UPDATING) )
			if((N->lview.iItem != -1)
			&& (N->lview.uNewState&LVIS_FOCUSED) )
			{
				SetSelectedUser( (TSonorkUserDataNotes*)(N->lview.lParam) );
			}
			break;

		case LVN_DELETEITEM:
			if((D.lParam=N->lview.lParam) != 0)
			{
				SONORK_MEM_DELETE( D.user );
			}
			break;
		}
	}
	else
	if( N->hdr.hwndFrom == tab.hwnd )
	{
		if( N->hdr.code == TCN_SELCHANGE && !TestWinUsrFlag(SONORK_WIN_F_UPDATING) )
			SetTab((PAGE)TabCtrl_GetCurSel(tab.hwnd) , false , false );
	}

	return 0;
}


// ----------------------------------------------------------------------------
// StartTracker????Task
// Returns TRUE if task was initiated
// <task_pending> will be TRUE if the task is now executing
// or FALSE if already completed.
//
// if return value is FALSE (did not even try to execute the task):
//    indicates the server does not support Trackers
// if return value is TRUE (the function attempted to execute the task):
//   if <task_pending> is FALSE:
//        if ERR.Result() == OK
//          the task completed OK
//        otherwise the error should be reported to the user
//   if <task_pending> is TRUE:
//      ERR should be ignored (will always be OK)
BOOL
 TSonorkUSearchWin::StartTrackerTreeTask(TSonorkTaskWin* TW
			, TSonorkError&			ERR
			, const TSonorkTreeView&	TV
			, SONORK_TREE_ITEM_TYPE		room_item_type
			, BOOL& 			task_pending
			, SONORK_DWORD2*		tag
			)
{
	UINT 	P_size;
	TSonorkDataPacket *P;
	TSonorkTrackerId tracker_id;

	task_pending = false;

	if( !TW->MayStartTask(&ERR,SONORK_WIN_TASK_SONORK) )
		return true;

	if( SonorkApp.ServerVersionNumber()<MAKE_VERSION_NUMBER(1,5,0,7) )
	{
		ERR.SetOk();
		return false;
	}

	TV.DelAllItems();
	if( room_item_type != SONORK_TREE_ITEM_NONE )
	{
		if( LoadTrackerListFromCache(TV, room_item_type) )
		{
			ERR.SetOk();
			return true;
		}
	}

	tracker_id.Clear();
#define A_size	128
	P=SONORK_AllocDataPacket(A_size);
	P_size = P->E_ListTrackers_R(A_size
			, tracker_id
			, true
			, 512);
#undef A_size
	TW->StartSonorkTask(ERR
		,P
		,P_size
		,SONORK_TASKWIN_F_NO_ERROR_BOX
		,GLS_TK_GETINFO
		,tag);

	SONORK_FreeDataPacket( P );
	if(ERR.Result() == SONORK_RESULT_OK)
	{
		task_pending=true;
		InvalidateTrackerListCache();
	}
	return true;

}

// ----------------------------------------------------------------------------

BOOL
 TSonorkUSearchWin::StartTrackerListTask(TSonorkTaskWin* TW
			, TSonorkError&			ERR
			, const TSonorkListView&	LV
			, const TSonorkTrackerId&	tracker_id
			, BOOL& 			task_pending
			)
{
	UINT 	P_size;
	TSonorkDataPacket *P;

	task_pending = false;

	if( !TW->MayStartTask(&ERR,SONORK_WIN_TASK_SONORK) )
		return true;

	if( SonorkApp.ServerVersionNumber()<MAKE_VERSION_NUMBER(1,5,0,6) )
	{
		ERR.SetOk();
		return false;
	}

	LV.DelAllItems();
#define A_size	128
	P=SONORK_AllocDataPacket(A_size);
	P_size = P->E_ListTrackers_R(A_size
			, tracker_id
			, false
			, 512);
#undef A_size
	TW->StartSonorkTask(ERR
		,P
		,P_size
		,SONORK_TASKWIN_F_NO_ERROR_BOX
		,GLS_TK_GETINFO
		,(SONORK_DWORD2*)NULL);

	SONORK_FreeDataPacket( P );
	if(ERR.Result() == SONORK_RESULT_OK)
	{
		task_pending=true;
	}
	return true;

}

// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::ProcessTrackerTreeTaskData(
			  const TSonorkTreeView& TV
			, SONORK_TREE_ITEM_TYPE	room_item_type
			, TSonorkDataPacket*	P
			, UINT 			P_size)
{
	bool			is_group;
	TSonorkTrackerItemPtrs	IP;
	TSonorkTrackerRoom	*room;
	TSonorkTrackerRoomQueue queue;
	if( P->Function() != SONORK_FUNCTION_LIST_TRACKERS )
		return;
	if( P->SubFunction() == SONORK_SUBFUNC_LIST_TRACKERS_AKN_GROUPS )
	{
		is_group=true;
	}
	else
	if( P->SubFunction() == SONORK_SUBFUNC_LIST_TRACKERS_AKN_ROOMS )
	{
		is_group=false;
	}
	else
		return;

	P->D_ListTrackerRooms_A(P_size,queue);
	while( (room=queue.RemoveFirst()) != NULL )
	{
		if( is_group )
		{
			IP.item = new TSonorkTrackerRoomItem(
				  SONORK_TREE_ITEM_TRACKER_GROUP
				, room);
		}
		else
		if( room_item_type == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS )
		{
			IP.item = new TSonorkTrackerRoomSubsItem(room);
		}
		else
		if( room_item_type == SONORK_TREE_ITEM_TRACKER_ROOM )
		{
			IP.item = new TSonorkTrackerRoomItem(
				  SONORK_TREE_ITEM_TRACKER_ROOM
				, room);
		}
		else
			continue;
		TV.AddItem(
			  TV.FindItem(SONORK_TREE_ITEM_TRACKER_GROUP
				,room->header.id.v[1])
			, IP.item
			, true );
	}

}
// ----------------------------------------------------------------------------
void
 TSonorkUSearchWin::ProcessTrackerListTaskData(
			const TSonorkListView&	LV
			, TSonorkDataPacket*	P
			, UINT 			P_size)
{
	int			index;
	TSonorkTrackerMember	*member;
	TSonorkTrackerMemberQueue queue;
	DWORD			help_level;
	char			str[22];
	SONORK_C_CSTR		pszLevel;
	const TSonorkLangCodeRecord	*REC;
	TSonorkTempBuffer	tmp(256);
	if( P->Function() != SONORK_FUNCTION_LIST_TRACKERS
	|| P->SubFunction() != SONORK_SUBFUNC_LIST_TRACKERS_AKN_MEMBERS )
		return;

	P->D_ListTrackerMembers_A(P_size,queue);
	while( (member=queue.RemoveFirst()) != NULL )
	{
		member->user_data.alias.CutAt(SONORK_USER_ALIAS_MAX_SIZE);
		sprintf(tmp.CStr(),"%s %s"
			,member->user_data.userId.GetStr(str)
			,member->user_data.alias.CStr());
		index = LV.AddItem( tmp.CStr()
				, SonorkApp.GetUserModeIcon( &member->user_data )
				, (LPARAM)member);
		if( index != -1 )
		{
			help_level=member->HelpLevel();
			if(help_level!=0)
			{
				sprintf(tmp.CStr()
				, "%s (%u)"
				, SonorkApp.LangString(GLS_LB_YES)
				, help_level);
				pszLevel=tmp.CStr();
			}
			else
			{
				pszLevel=SonorkApp.LangString(GLS_LB_NO);

			}
			LV.SetItem( index
				, 1
				, pszLevel
				, SKIN_ICON_NONE);
			REC = SonorkApp.LanguageCodeTable().GetRecordByCode(
					member->user_data.Region().GetLanguage());
			LV.SetItemText(index,2,REC?REC->name:"");
			continue;
		}
		SONORK_MEM_DELETE(member);
	}

}
void
 TSonorkUSearchWin::SetupTrackerListView(const TSonorkListView& LV)
{
	LV.AddColumn( 0, GLS_LB_USR, 125);
	LV.AddColumn( 1, GLS_TR_HLPR, 45);
	LV.AddColumn( 2, GLS_LB_LANG, 60);
}


// ----------------------------------------------------------------------------

void
 TSonorkUSearchWin::InvalidateTrackerListCache()
{
	SonorkApp.ClearRunFlag(SONORK_WAPP_RF_TRACKER_CACHE_LOADED);
}
// ----------------------------------------------------------------------------
void
 TSonorkUSearchWin::SaveTrackerListToCache(const TSonorkTreeView& TV)
{
	TSonorkAtomDb		db;

	InvalidateTrackerListCache();

	if(SonorkApp.OpenAppDb(SONORK_APP_DB_TRACKER_CACHE,db,true))
	{
		SaveTrackerCacheItem(db,TV,TVI_ROOT);
		if(db.Items() > MIN_ITEMS_IN_CACHE)
			SonorkApp.SetRunFlag(SONORK_WAPP_RF_TRACKER_CACHE_LOADED);
		db.Close();

	}
}

void
 TSonorkUSearchWin::SaveTrackerCacheItem(
	 TSonorkAtomDb& 	db
	,const TSonorkTreeView& TV
	,HTREEITEM 		hItem)
 {
	TSonorkTag		tag;
	TSonorkTrackerItemPtrs	IP;

	tag.v[1]=0;
	for(IP.ptr = TV.GetChild(hItem)
	   ;IP.ptr != NULL
	   ;IP.ptr = TV.GetNextSibling(IP.ptr))
	{
		if( IP.ptr->Type() == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
		||  IP.ptr->Type() == SONORK_TREE_ITEM_TRACKER_ROOM
		||  IP.ptr->Type() == SONORK_TREE_ITEM_TRACKER_GROUP)
		{
			tag.v[0]=IP.ptr->Type();
			db.Add( IP.room->room , NULL , &tag );
			SaveTrackerCacheItem( db , TV , IP.ptr->hItem() );
		}
	}

}
// ----------------------------------------------------------------------------
BOOL
 TSonorkUSearchWin::LoadTrackerListFromCache(
		const TSonorkTreeView& TV
	, 	SONORK_TREE_ITEM_TYPE	room_item_type)
{
	TSonorkAtomDb		db;
	TSonorkTrackerRoom*	room;
	TSonorkTrackerItemPtrs	IP;
	TSonorkTag		tag;
	UINT			ci,mi,li=0;


	if(!SonorkApp.TestRunFlag(SONORK_WAPP_RF_TRACKER_CACHE_LOADED))
		return false;

	if(!SonorkApp.OpenAppDb(SONORK_APP_DB_TRACKER_CACHE,db,false))
		goto no_data;

	if((mi=db.Items())<=MIN_ITEMS_IN_CACHE)
		goto no_data;

	SONORK_MEM_NEW( room = new TSonorkTrackerRoom );
	for(ci=0;ci<mi;ci++)
	{
		if(db.Get(ci,room,&tag) != SONORK_RESULT_OK)
			continue;
		if( tag.v[0] == SONORK_TREE_ITEM_TRACKER_GROUP )
		{
			IP.item = new TSonorkTrackerRoomItem(
				  SONORK_TREE_ITEM_TRACKER_GROUP
				, room);
		}
		else
		if( tag.v[0] == SONORK_TREE_ITEM_TRACKER_ROOM
		||  tag.v[0] == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS)
		{
			if( room_item_type == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS)
				IP.item = new TSonorkTrackerRoomSubsItem(room);
			else
			if( room_item_type == SONORK_TREE_ITEM_TRACKER_ROOM )
				IP.item = new TSonorkTrackerRoomItem(
					  SONORK_TREE_ITEM_TRACKER_ROOM
					, room);
			else
				continue;

		}
		else
			continue;
		TV.AddItem(
		  TV.FindItem(SONORK_TREE_ITEM_TRACKER_GROUP
			,room->header.id.v[1])
		, IP.item
		, true );
		li++;
		SONORK_MEM_NEW( room = new TSonorkTrackerRoom );
	}
	SONORK_MEM_DELETE( room );
	db.Close();

no_data:
	if( li <= MIN_ITEMS_IN_CACHE)
	{
		InvalidateTrackerListCache();
		return false;
	}
	return true;
}
void
 TSonorkUSearchWin::AfterTrackerTreeLoad(const TSonorkTreeView& TV
	, BOOL from_cache
	, BOOL )
{
	TV.AfterMassOp();
	if( !from_cache )
		SaveTrackerListToCache(TV);
	TV.ExpandItemLevels(TVI_ROOT,TVE_EXPAND,1);

}

void SONORK_CALLBACK
 TSonorkUSearchWin::SearchUserCallback(
		 const TSonorkListView*		LV
		,TSonorkUserDataNotes*		UDN)
{

	const TSonorkLangCodeRecord	*REC;

	int	index;

	char	tmp[32];


	index = LV->AddItem(
		  UDN->user_data.userId.GetStr(tmp)
		, SonorkApp.GetUserModeIcon( &UDN->user_data )
		, (LPARAM)UDN);
	LV->SetItemText(index
		,1
		,UDN->user_data.alias.CStr());
	LV->SetItemText(index
		,2
		,UDN->user_data.name.CStr());
	REC = SonorkApp.LanguageCodeTable().GetRecordByCode(
		UDN->user_data.Region().GetLanguage());

	LV->SetItemText(index,3,REC?REC->name:"");
	LV->SetItem(index,4,""
		,	UDN->user_data.addr.sidFlags.TrackerEnabled()
			?SKIN_ICON_TRACKER

			:SKIN_ICON_NONE);
	LV->SetItem(index,5,""

		,	UDN->notes.Length()>2
			?SKIN_ICON_NOTES

			:SKIN_ICON_NONE);
	/*

SonorkApp.LangString(

			UDN->user_data.addr.sidFlags.TrackerEnabled()

			?GLS_LB_YES

			:GLS_LB_NO)

	*/

}




// ----------------------------------------------------------------------------
// TSonorkTrackerRoomItem
// ----------------------------------------------------------------------------

TSonorkTrackerRoomItem::TSonorkTrackerRoomItem(
	 SONORK_TREE_ITEM_TYPE 	tp
	,TSonorkTrackerRoom* 	proom)
	:TSonorkTrackerTreeItem(tp)
{
	room = proom;
	e_count=b_count=0;
}

// ----------------------------------------------------------------------------

TSonorkTrackerRoomItem::~TSonorkTrackerRoomItem()
{
	SONORK_MEM_DELETE(room);
}


// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkTrackerRoomItem::GetLabel(BOOL expanded,SKIN_ICON& icon ) const
{
	if( Type() == SONORK_TREE_ITEM_TRACKER_GROUP )
	{
		icon = expanded?SKIN_ICON_OPEN_FOLDER:SKIN_ICON_FOLDER;
	}
	else
	if( Type() == SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
	||  Type() == SONORK_TREE_ITEM_TRACKER_ROOM)
	{
		icon = SKIN_ICON_TRACKER;
	}
	return room->name.CStr();
}

// ----------------------------------------------------------------------------
// TSonorkTrackerRoomSubsItem
// ----------------------------------------------------------------------------

TSonorkTrackerRoomSubsItem::TSonorkTrackerRoomSubsItem(TSonorkTrackerRoom* room)
:TSonorkTrackerRoomItem(SONORK_TREE_ITEM_TRACKER_ROOM_SUBS, room)
{
	data=NULL;
}

// ----------------------------------------------------------------------------

TSonorkTrackerRoomSubsItem::~TSonorkTrackerRoomSubsItem()
{
	if(data)SONORK_MEM_DELETE( data );
}


// ----------------------------------------------------------------------------

int
 TSonorkTrackerRoomSubsItem::GetBoldValue() const
{
	return data!=NULL?1:0;
}
// ----------------------------------------------------------------------------
int
 TSonorkTrackerRoomSubsItem::GetEventValue()	const
{
	return	data==NULL
		?0
		:(data->IsActive()
			?1
			:0
		);

}
