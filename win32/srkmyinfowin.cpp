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

#include "srkmyinfowin.h"
#include "srkdialogwin.h"
#include "srkinputwin.h"
#include "srkslidewin.h"
#include "srkprofileswin.h"
#include "srknetcfgwin.h"
#include "srk_winregkey.h"
#include "srk_cfg_names.h"
#include "srkusearchwin.h"
#include "srk_zip.h"


extern BOOL
 SaveAsBitmap(HDC sDC, const char*file_name, const RECT& rect);
//extern HBITMAP  Sonork_LoadBitmap(HDC tDC, const char*file_name);


// --------------------------------------------------------------------------

#define PREFS_LIST_LOADED		SONORK_WIN_F_USER_01
#define PREFS_LIST_MODIFIED		SONORK_WIN_F_USER_02
#define AUTH_LIST_LOADED		SONORK_WIN_F_USER_03
#define AUTH_LIST_MODIFIED 		SONORK_WIN_F_USER_04
#define EMAIL_LIST_LOADED		SONORK_WIN_F_USER_05
#define EMAIL_LIST_MODIFIED		SONORK_WIN_F_USER_06
#define SKIN_MODIFIED			SONORK_WIN_F_USER_07
#define TRACKER_TREE_LOADED		SONORK_WIN_F_USER_08
#define TRACKER_SUBS_LOADED             SONORK_WIN_F_USER_09
#define TRACKER_SUBS_MODIFIED		SONORK_WIN_F_USER_10
#define TRACKER_LOADED_FROM_CACHE	SONORK_WIN_F_USER_11
#define PIC_LOADED  			SONORK_WIN_F_USER_12
#define PIC_MODIFIED			SONORK_WIN_F_USER_13
#define POKE_SELECT_PROFILE		SONORK_WIN_POKE_01
#define POKE_EDIT_COLOR			SONORK_WIN_POKE_02
#define POKE_EDIT_AUTH			SONORK_WIN_POKE_03
#define POKE_EDIT_EMAIL_ACC		SONORK_WIN_POKE_04
#define POKE_EDIT_EMAIL_EXC		SONORK_WIN_POKE_05
#define POKE_EDIT_TRACKER		SONORK_WIN_POKE_06
// --------------------------------------------------------------------------
// 		COMMANDS
// --------------------------------------------------------------------------

#define MSG_VIEW_MAP_COLORS	12
#define MAIN_MAP_COLORS		5
#define CLIP_MAP_COLORS		3
#define SYSCON_MAP_COLORS	5
#define CHAT_MAP_COLORS		4
#define NO_SELECTED_COLOR	((DWORD)-1)
TMyInfoSkinMap sm_msg_view[MSG_VIEW_MAP_COLORS]=
{{SKIN_COLOR_MSG_VIEW	,SKIN_CI_BG	, GLS_CL_BG}	//0
,{SKIN_COLOR_MSG_VIEW	,SKIN_CI_SP	, GLS_CL_SP}	//1
,{SKIN_COLOR_MSG_IN_NEW	,SKIN_CI_BG	, GLS_CL_MSGIN}	//2
,{SKIN_COLOR_MSG_IN_NEW	,SKIN_CI_FG	, GLS_CL_MSGIN}	//3
,{SKIN_COLOR_MSG_IN_OLD	,SKIN_CI_BG	, GLS_CL_MSGIO}	//4
,{SKIN_COLOR_MSG_IN_OLD	,SKIN_CI_FG	, GLS_CL_MSGIO}	//5
,{SKIN_COLOR_MSG_OUT	,SKIN_CI_BG	, GLS_CL_MSGO}	//6
,{SKIN_COLOR_MSG_OUT	,SKIN_CI_FG	, GLS_CL_MSGO}	//7
,{SKIN_COLOR_MSG_FOCUS	,SKIN_CI_BG	, GLS_CL_FOC}	//8
,{SKIN_COLOR_MSG_FOCUS	,SKIN_CI_FG	, GLS_CL_FOC}	//9
,{SKIN_COLOR_MSG_SELECT	,SKIN_CI_BG	, GLS_CL_SEL}	//10
,{SKIN_COLOR_MSG_SELECT	,SKIN_CI_FG	, GLS_CL_SEL}	//11
};
TMyInfoSkinMap sm_main[MAIN_MAP_COLORS]=
{{SKIN_COLOR_MAIN	,SKIN_CI_BG		, GLS_CL_BG}	//0
,{SKIN_COLOR_MAIN	,SKIN_CI_FG		, GLS_CL_FG}	//1
,{SKIN_COLOR_MAIN_EXT	,SKIN_CI_MAIN_OFFLINE	, GLS_CL_MOFF}	//2
,{SKIN_COLOR_MAIN_EXT	,SKIN_CI_MAIN_ONLINE	, GLS_CL_MON}	//3
,{SKIN_COLOR_MAIN_EXT	,SKIN_CI_MAIN_EVENT	, GLS_CL_MEVT}	//4
};
TMyInfoSkinMap sm_clip[CLIP_MAP_COLORS]=
{{SKIN_COLOR_CLIP	,SKIN_CI_BG	,GLS_CL_BG}		// 0
,{SKIN_COLOR_CLIP	,SKIN_CI_FG	,GLS_CL_FG}		// 1
,{SKIN_COLOR_CLIP	,SKIN_CI_SP	,GLS_CL_SP}		// 2
};
TMyInfoSkinMap sm_syscon[SYSCON_MAP_COLORS]=
{{SKIN_COLOR_SYSCON	,SKIN_CI_BG	,GLS_CL_BG}		// 0
,{SKIN_COLOR_SYSCON	,SKIN_CI_FG	,GLS_CL_FG}		// 1
,{SKIN_COLOR_SYSCON_NEW	,SKIN_CI_BG	,GLS_CL_MSGIN}		// 2
,{SKIN_COLOR_SYSCON_NEW	,SKIN_CI_FG	,GLS_CL_MSGIN}		// 3
,{SKIN_COLOR_SYSCON	,SKIN_CI_SP	,GLS_CL_SP}		// 4
};
TMyInfoSkinMap sm_chat[CHAT_MAP_COLORS]=
{{SKIN_COLOR_CHAT	,SKIN_CI_BG		, GLS_CL_BG}	// 0
,{SKIN_COLOR_CHAT	,SKIN_CI_SP		, GLS_CL_SP}	// 1
,{SKIN_COLOR_CHAT_EXT	,SKIN_CI_CHAT_SYS	, GLS_CL_CSYS} 	// 2
,{SKIN_COLOR_CHAT_EXT	,SKIN_CI_CHAT_ACTION	, GLS_CL_CACT} 	// 3
};

enum SKIN_STYLE
{
	SKIN_STYLE_LINE_1
,	SKIN_STYLE_LINE_2
,	SKIN_STYLE_LINE_3
,	SKIN_STYLE_MSG
,	SKIN_STYLE_MAIN
//,	SKIN_STYLE_CHAT
};

#define SPACING			6
#define COLOR_HEIGHT		16
#define COLOR_WIDTH		16
#define HPADDING		8
#define VPADDING		8
#define SELECTION_COLOR         0xff33ff
#define IsRefreshableTab(n)	(n==TAB_INFO||n==TAB_AUTH||n==TAB_TRACK)
// ----------------------------------------------------------------------------

class TSonorkTrackerSubsWin
:public TSonorkWin
{

	TSonorkTrackerRoom*	room;
	TSonorkTrackerData*	data;
	HWND			help_cb;
	BOOL 			app_detector;
	bool
		OnCreate();

	bool
		OnCommand(UINT id,HWND hwnd, UINT notify_code);

	bool
		CmdSave();
public:
	TSonorkTrackerSubsWin(TSonorkWin*parent
		, TSonorkTrackerRoom*
		, TSonorkTrackerData*
		);

};


// ----------------------------------------------------------------------------
void
  TSonorkMyInfoWin::Pk_EditTracker( LPARAM lParam )
{
	UINT	id;
	TSonorkTrackerItemPtrs	IP;
	TSonorkTrackerRoom*  	room;
	TSonorkTrackerData*  	cur_data;
	TSonorkTrackerData*  	old_data;
	int	e_count, b_count;

	if((IP.lParam = lParam) == 0 || !TestWinUsrFlag(TRACKER_SUBS_LOADED))
		return;

	if( IP.item->Type() != SONORK_TREE_ITEM_TRACKER_ROOM_SUBS )
	{
		// If the user clicks on the "OPEN" button when
		// a group is selected, expand the group node.
		// (we can't edit a group node)
		if( IP.item->Type() == SONORK_TREE_ITEM_TRACKER_GROUP)
		{
			track_tree.ExpandItemLevels(IP.item, TVE_EXPAND);
		}
		return;
	}

	old_data= IP.subs->data;
	room	= IP.subs->room;
	b_count = IP.subs->GetBoldCount();
	e_count = IP.subs->GetEventCount();
	if( old_data == NULL )
	{
		SONORK_MEM_NEW( cur_data = new TSonorkTrackerData );
		cur_data->CODEC_Clear();
		cur_data->header.id.Set(IP.subs->room->header.id);
		cur_data->header.flags.Set(IP.subs->room->header.flags);
		cur_data->header.flags.Clear(SONORK_TRACKER_DATA_F0_ACTIVE);
	}
	else
	{
		cur_data = old_data;
	}

	id = TSonorkTrackerSubsWin(this
		,room
		,cur_data).Execute();

	if( id == IDOK )
	{
		IP.subs->data = cur_data ;
	}
	else
	{
		if( old_data==NULL || id == IDC_TRKSUBS_DEL)
		{
			IP.subs->data = NULL;
		}

	}
	if( IP.subs->data == NULL )
	{
		SONORK_MEM_DELETE( cur_data );
	}

	track_tree.UpdateItemAttributes(
		  IP.ptr
		, IP.ptr->GetBoldCount()  - b_count
		, IP.ptr->GetEventCount() - e_count
		, 0
		);
	SetWinUsrFlag(TRACKER_SUBS_MODIFIED);
}
// ----------------------------------------------------------------------------

void
  TSonorkMyInfoWin::LoadTrackerTree(BOOL auto_load)
{
	BOOL task_pending;
	SONORK_DWORD2	tag;
	if( auto_load )
	{
		if( TestWinUsrFlag(TRACKER_TREE_LOADED) )
			return;
	}
	if(!MayStartTask(NULL,SONORK_WIN_TASK_SONORK))
		return;
	tag.v[0] = SONORK_FUNCTION_LIST_TRACKERS;
	if( TestWinUsrFlag(TRACKER_SUBS_MODIFIED) )
		SaveTrackerSubs();
		
	MultiClearWinUsrFlags(
		 TRACKER_SUBS_LOADED
		|TRACKER_TREE_LOADED
		|TRACKER_LOADED_FROM_CACHE);
	if(TSonorkUSearchWin::StartTrackerTreeTask(
		  this
		, taskERR
		, track_tree
		, auto_load
		  ?SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
		  :SONORK_TREE_ITEM_NONE
		, task_pending
		, &tag
		  ))
	{
		if( task_pending )
			return;
		// Task not pending: Was loaded from cache
		SetWinUsrFlag( TRACKER_LOADED_FROM_CACHE );
		PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT
				, SONORK_FUNCTION_LIST_TRACKERS);
	}
}

// ----------------------------------------------------------------------------

void
  TSonorkMyInfoWin::LoadTrackerSubs()
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
			IP.ptr = track_tree.FindItem(SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
				, data->header.id.v[0]);
			if( IP.item )
			if( IP.subs->data == NULL )
			{
				IP.subs->data = data ;
				track_tree.UpdateItemAttributes(
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
	ClearWinUsrFlag(TRACKER_SUBS_MODIFIED);
	SetWinUsrFlag(TRACKER_SUBS_LOADED);
}

// ----------------------------------------------------------------------------

UINT
 TSonorkMyInfoWin::SaveTrackerSubsItem(TSonorkAtomDb& db,HTREEITEM hItem)
{
	UINT cc=0;
	TSonorkTrackerItemPtrs	IP;

	for(IP.ptr = track_tree.GetChild(hItem)
	   ;IP.ptr != NULL
	   ;IP.ptr = track_tree.GetNextSibling(IP.ptr))
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
			cc+=SaveTrackerSubsItem( db , IP.ptr->hItem() );
	}
	return cc;
}
void
 TSonorkMyInfoWin::SaveTrackerSubs()
{
	TSonorkAtomDb		db;
	if(!TestWinUsrFlag(TRACKER_SUBS_LOADED) )
		return;

	if(!SonorkApp.OpenAppDb(SONORK_APP_DB_TRACKER_SUBS,db,true))
		return;


	if(SaveTrackerSubsItem(db,TVI_ROOT) != 0)
		SonorkApp.RequestSynchTrackers( true );

	db.Close();
	ClearWinUsrFlag(TRACKER_SUBS_MODIFIED);

}
// ----------------------------------------------------------------------------

void
 TSonorkMyInfoWin::RebuildSkinPreview()
{
	SKIN_INDEX	skin_index;
	SKIN_STYLE	skin_style;
	DWORD		i,h,j;
	HBRUSH		bgBrush;
	TMyInfoSkinMap*	cPtr;
	static LOGBRUSH null_brush={BS_HATCHED,0x808080,HS_BDIAGONAL};
	if( skin.memDC==NULL )
		return;

	SetBkMode(skin.memDC,TRANSPARENT);
	SelectObject(skin.memDC,sonork_skin.Font(SKIN_FONT_BOLD));
	skin_index = (SKIN_INDEX)ComboBox_GetCurSel(skin.list);
	switch(skin_index)
	{
		case SKIN_MSG_VIEW:
			skin.colors=MSG_VIEW_MAP_COLORS;
			skin.map = sm_msg_view;
			skin_style = SKIN_STYLE_MSG;
			break;
		case SKIN_MAIN:
			skin.colors=MAIN_MAP_COLORS;
			skin.map = sm_main;
			skin_style = SKIN_STYLE_MAIN;
			break;
		case SKIN_CLIP:
			skin.colors=CLIP_MAP_COLORS;
			skin.map = sm_clip;
			skin_style = SKIN_STYLE_LINE_1;
			break;
		case SKIN_CHAT:
			skin.colors=CHAT_MAP_COLORS;
			skin.map = sm_chat;
			skin_style = SKIN_STYLE_LINE_2;
			break;
		case SKIN_SYSCON:
			skin.colors=SYSCON_MAP_COLORS;
			skin.map = sm_syscon;
			skin_style = SKIN_STYLE_LINE_3;
			break;
		default:
			::FillRect(skin.memDC,&skin.viewRect,GetSysColorBrush(COLOR_3DFACE));
			return;
	}
	bgBrush = CreateSolidBrush(sonork_skin.Color(skin.map->color,skin.map->index));
	skin.itemRect=skin.viewRect;
	skin.itemRect.top+=COLOR_HEIGHT;

	DrawItemBlock(0);
	skin.itemRect.top  += VPADDING;
	skin.itemRect.left = HPADDING ;
	skin.itemRect.right= skin.viewRect.right-HPADDING;

	if( skin_style == SKIN_STYLE_MAIN )
	{
		h = (skin.viewRect.bottom-COLOR_HEIGHT-VPADDING*2)/4;
		for(i=1;i<5;i++)
		{
			skin.itemRect.bottom	= skin.itemRect.top + h;
			DrawItemText(i);
			skin.itemRect.top	= skin.itemRect.bottom;
		}
	}
	else
	if( skin_style == SKIN_STYLE_MSG )
	{
		h = (skin.viewRect.bottom-COLOR_HEIGHT-VPADDING*2)/5-SPACING;

		for(i=0,j=2;i<5;i++)
		{
			skin.itemRect.bottom	= skin.itemRect.top + h;
			DrawItemBlock(j++);
			DrawItemText(j++);

			skin.itemRect.top	= skin.itemRect.bottom;
			skin.itemRect.bottom	= skin.itemRect.top + SPACING;
			DrawItemBlock(1);

			skin.itemRect.top	= skin.itemRect.bottom;
		}

	}
	else
	if( skin_style == SKIN_STYLE_LINE_1 )
	{
		h = (skin.viewRect.bottom-COLOR_HEIGHT-VPADDING*2)/2-SPACING;

		for(i=0;i<2;i++)
		{
			skin.itemRect.bottom	= skin.itemRect.top + h;

			DrawItemText(1);
			skin.itemRect.top	= skin.itemRect.bottom;
			skin.itemRect.bottom	= skin.itemRect.top + SPACING;
			DrawItemBlock(2);

			skin.itemRect.top	= skin.itemRect.bottom;
		}
	}
	else
	if( skin_style == SKIN_STYLE_LINE_2 )
	{
		h = (skin.viewRect.bottom-COLOR_HEIGHT-VPADDING*2)/2-SPACING;

		for(i=0,j=2;i<2;i++)
		{
			skin.itemRect.bottom	= skin.itemRect.top + h;
			DrawItemBlock(0);
			DrawItemText(j++);

			skin.itemRect.top	= skin.itemRect.bottom;
			skin.itemRect.bottom	= skin.itemRect.top + SPACING;
			DrawItemBlock(1);

			skin.itemRect.top	= skin.itemRect.bottom;
		}
	}
	else
	if( skin_style == SKIN_STYLE_LINE_3 )
	{
		h = (skin.viewRect.bottom-COLOR_HEIGHT-VPADDING*2)/2-SPACING;

		for(i=0,j=0;i<2;i++)
		{
			skin.itemRect.bottom	= skin.itemRect.top + h;
			DrawItemBlock(j++);
			DrawItemText(j++);

			skin.itemRect.top	= skin.itemRect.bottom;
			skin.itemRect.bottom	= skin.itemRect.top + SPACING;
			DrawItemBlock(4);

			skin.itemRect.top	= skin.itemRect.bottom;
		}
	}
	DeleteObject(bgBrush);
	skin.itemRect.left=0;
	skin.itemRect.top=0;
	skin.itemRect.bottom=COLOR_HEIGHT;
	for(cPtr=skin.map,i=0;i<skin.colors;i++,cPtr++)
	{
		skin.itemRect.right=skin.itemRect.left+COLOR_WIDTH;
		DrawItemBlock(i);
		skin.itemRect.left	= skin.itemRect.right;
	}
	MoveToEx(skin.memDC,0,COLOR_HEIGHT,NULL);
	LineTo(skin.memDC,skin.viewRect.right,COLOR_HEIGHT);
	bgBrush=CreateBrushIndirect(&null_brush);
	SetBkColor(skin.memDC,GetSysColor(COLOR_3DFACE));
	skin.itemRect.right=skin.viewRect.right;
	FillRect(skin.memDC,&skin.itemRect,bgBrush);
	DeleteObject(bgBrush);
	::InvalidateRect(skin.view.Handle(),NULL,false);

	page_win[TAB_SKIN]->SetCtrlText(IDL_MINFO_SKIN_COLOR
		,skin.selected_color == NO_SELECTED_COLOR
			?GLS_NULL
			:(skin.map+skin.selected_color)->gls);

}

LRESULT
TSonorkMyInfoWin::OnCtlWinMsg(TSonorkWinCtrl* WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	DWORD x,y,sc;
	while( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_MOUSEMOVE )
	{
		x=LOWORD(lParam);
		y=HIWORD(lParam);
		if(y < COLOR_HEIGHT)
		if(x < skin.colors * COLOR_WIDTH)

		{
			sc=x/COLOR_WIDTH;
			SetCursor(sonork_skin.Cursor(SKIN_CURSOR_HAND));
			if( skin.selected_color != sc )
			{
				skin.selected_color = sc;
				RebuildSkinPreview();
			}
			if(uMsg == WM_LBUTTONDOWN)
				PostPoke(POKE_EDIT_COLOR,sc);
			return 0L;
		}
		if(skin.selected_color != NO_SELECTED_COLOR)
		{
			skin.selected_color = NO_SELECTED_COLOR;
			RebuildSkinPreview();
		}
		break;
	}
	return WC->DefaultProcessing(uMsg,wParam,lParam);

}
void
 TSonorkMyInfoWin::EditSkinColor(DWORD c)
{
	CHOOSECOLOR CC;
	COLORREF	custom[16];
	TMyInfoSkinMap*	cPtr;
	if(c>=skin.colors || skin.map==NULL)
		return;
	CC.lStructSize	= sizeof(CC);
	CC.hwndOwner	= Handle();
	CC.hInstance	= NULL;
	cPtr=skin.map+c;
	CC.rgbResult	= sonork_skin.Color(cPtr->color,cPtr->index);
	CC.lpCustColors	= custom;
	CC.Flags		= CC_RGBINIT|CC_FULLOPEN;//CC_PREVENTFULLOPEN|
	skin.selected_color=NO_SELECTED_COLOR;
	RebuildSkinPreview();
	if(ChooseColor(&CC))
	{
		SetWinUsrFlag(SKIN_MODIFIED);
		sonork_skin.SetColor(cPtr->color,cPtr->index , CC.rgbResult);
		RebuildSkinPreview();
		SonorkApp.BroadcastAppEvent(SONORK_APP_EVENT_SKIN_COLOR_CHANGED
			,SONORK_APP_EM_SKIN_AWARE
			,0,0);

	}

}
void
 TSonorkMyInfoWin::DrawItemBlock(DWORD color_no)
{
	HBRUSH	itemBrush;
	DWORD	d_color;

//	d_color = color_no == skin.selected_color
//		?SELECTION_COLOR
//		:sonork_skin.Color((skin.map+color_no)->color,(skin.map+color_no)->index);
	d_color = sonork_skin.Color((skin.map+color_no)->color,(skin.map+color_no)->index);

	itemBrush = CreateSolidBrush(d_color);//;
	FillRect(skin.memDC,&skin.itemRect,itemBrush);
	DeleteObject(itemBrush);
	if( color_no == skin.selected_color )
	{
		DrawEdge(skin.memDC
			,&skin.itemRect
			,EDGE_BUMP
			,BF_BOTTOMLEFT|BF_TOPRIGHT);
			//(HBRUSH)GetSysColorBrush(COLOR_GRAYTEXT));
		//FrameRect(skin.memDC,&skin.itemRect,(HBRUSH)GetSysColorBrush(COLOR_GRAYTEXT));
	}
}
void
 TSonorkMyInfoWin::DrawItemText(DWORD color_no)
{
	const char *str;
	RECT	t_rect;
	TMyInfoSkinMap* M=skin.map + color_no;
	if( color_no == skin.selected_color )
	{
		t_rect=skin.itemRect;
		t_rect.top=skin.itemRect.top
				+ (skin.itemRect.bottom - skin.itemRect.top)/2
				- 10;
		t_rect.bottom=t_rect.top+20;
		t_rect.left+=16;
		t_rect.right-=16;
		DrawEdge(skin.memDC
			,&t_rect
			,EDGE_BUMP
			,BF_BOTTOMLEFT|BF_TOPRIGHT);
	}
	str=SonorkApp.LangString(M->gls);
	SetTextColor( skin.memDC, sonork_skin.Color(M->color,M->index) );
	::DrawText(skin.memDC
		, str
		, strlen(str)
		, &skin.itemRect
		, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
}

void
 TSonorkMyInfoWin::CmdLoad()
{
	LoadLogin(true,true,false);
	LoadInfo();
	LoadNotes();
	LoadPic(NULL);

	OnPrefItemSelected();
}

void
 TSonorkMyInfoWin::CmdStore(bool connect, bool destroy)
{
	int aux;
	char tmp[64],s1[8],s2[8],s3[8];
	const TSonorkLangCodeRecord	*REC;
	TSonorkUserInfo3*UI;
	TSonorkWin	*page;
	TSonorkError ERR;
	UINT			A_size,P_size;
	TSonorkDataPacket	*P;
	SONORK_DWORD2	tag;

	SaveLogin();

	if( TestWinUsrFlag(AUTH_LIST_MODIFIED  ) )
		SaveAuthList();

	if( TestWinUsrFlag(EMAIL_LIST_MODIFIED  ) )
		SaveEmailList();

	if( TestWinUsrFlag(SKIN_MODIFIED) )
		SonorkApp.WriteProfileItem("Skin",&TSonorkSkinCodecAtom(&sonork_skin));
		
	if( TestWinUsrFlag(PIC_MODIFIED))
		SavePic();


	if(SonorkApp.MayStartGlobalTask() && !connect)
	{
		UDN.user_data.Set(SonorkApp.ProfileUser());
		if(UDN.user_data.GetUserInfoLevel()>=SONORK_USER_INFO_LEVEL_1)
		{

			page_win[TAB_PIC]->GetCtrlText(IDC_MINFO_PIC_NOTES
				,UDN.notes);

			page = page_win[TAB_INFO];
			UI = UDN.user_data.wUserInfo();
			page->GetCtrlText(IDC_MINFO_ALIAS,UDN.user_data.alias);
			page->GetCtrlText(IDC_MINFO_NAME,UDN.user_data.name);
			page->GetCtrlText(IDC_MINFO_EMAIL,UDN.user_data.email);

			aux = ComboBox_GetCurSel( sex_list );
			if(aux == 1)
				aux = SONORK_SEX_F;
			else
			if(aux == 2)
				aux = SONORK_SEX_NA;
			else
				aux = SONORK_SEX_M;
			UI->infoFlags.SetSex((SONORK_SEX)aux);

			page->GetCtrlText(IDC_MINFO_BDATE_YR,s1,6);
			page->GetCtrlText(IDC_MINFO_BDATE_MO,s2,6);
			page->GetCtrlText(IDC_MINFO_BDATE_DA,s3,6);
			wsprintf(tmp,"%s/%s/%s",s1,s2,s3);

			UI->bornTime.SetDate( tmp );

			aux = ComboBox_GetCurSel( country_list );
			REC = SonorkApp.CountryCodeTable().GetRecordByIndex( aux );
			if( REC != NULL )
				UI->region.SetCountry( REC->code );
			UI->pubAuthFlags.Clear();
			UI->pubAuthFlags.SetUserInfoLevel( SONORK_USER_INFO_LEVEL_1 );
			UI->privAuthFlags.Set(UI->pubAuthFlags);


			page = page_win[TAB_AUTH];
			UI->pubAuthFlags.SetClear(
				SONORK_AUTH_F0_HIDE_PUBLIC
				,!page->GetCtrlChecked(IDC_MINFO_AUTH_PUB_UINFO));
			UI->pubAuthFlags.SetClear(
				SONORK_AUTH_F1_HIDE_EMAIL
				,page->GetCtrlChecked(IDC_MINFO_AUTH_PUB_HIDE_EMAIL));
			UI->pubAuthFlags.SetClear(
				SONORK_AUTH_F1_HIDE_ADDR
				,page->GetCtrlChecked(IDC_MINFO_AUTH_PUB_HIDE_NETADDR));
			UI->pubAuthPin = page->GetCtrlUint(IDC_MINFO_AUTH_PUB_PIN);

			tag.v[0] = SONORK_FUNCTION_SET_USER_DATA;

			sonork_codec_io_v104_compatibility_mode =
				SonorkApp.ServerVersionNumber()
				<
				MAKE_VERSION_NUMBER(1,5,0,7);

			A_size = CODEC_Size(&UDN,SONORK_UDN_ENCODE_NOTES)+64;
			P = SONORK_AllocDataPacket( A_size );
			P_size = P->E_SetUserData_R(A_size,UDN,SONORK_UDN_ENCODE_NOTES);
			StartSonorkTask(taskERR
				,P
				,P_size
				,0
				,GLS_TK_PUTINFO
				,&tag);
			SONORK_FreeDataPacket( P );

			sonork_codec_io_v104_compatibility_mode = false;
			return;
		}
	}
	if(connect)
		SonorkApp.Connect(ERR);
	if(destroy)
		Destroy(IDOK);
}
void
 TSonorkMyInfoWin::CmdRefresh()
{
	if( tab_ctrl.page == TAB_TRACK )
	{
		LoadTrackerTree( false );

	}
	else
	{
		UINT			A_size,P_size;
		TSonorkDataPacket	*P;
		SONORK_DWORD2		tag;
		A_size = 64;

		tag.v[0]=SONORK_FUNCTION_GET_USER_DATA;
		P = SONORK_AllocDataPacket( A_size );
		P_size = P->E_GetUserData_R(A_size
				,SonorkApp.ProfileUserId()
				,SONORK_USER_INFO_MAX_LEVEL
				,0
				,0
				,0);
		StartSonorkTask(taskERR
			,P
			,P_size
			,SONORK_TASKWIN_DEFAULT_FLAGS
			,GLS_TK_GETINFO
			,&tag);
		SONORK_FreeDataPacket( P );
	}

}


// --------------------------------------------------------------------------
// 		FORM LOADING
// --------------------------------------------------------------------------

void
 TSonorkMyInfoWin::LoadTimeZone()
{
	char tmp[48];
	page_win[TAB_INFO]->SetCtrlText(IDC_MINFO_TZ
		, SonorkApp.ProfileRegion().GetTimeZoneStr(tmp)
	);
}

void
 TSonorkMyInfoWin::SelectProfile()
{
	if(!SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_USER_SELECT))
	{
		TSonorkProfilesWin W(this);

		if(SonorkApp.IsProfileOpen())
		{
			CmdStore(false,false);
		}

		if(W.DoExecute())
		{
			Destroy();
			SonorkApp.ShowSetupDialog();
		}
		else
			CmdLoad();

	}
}

void
 TSonorkMyInfoWin::SaveLogin()
{
	char tmp[48];
	if(!SonorkApp.IsProfileOpen())
		return;

	SonorkApp.ProfileCtrlFlags().SetClear(
		SONORK_PCF_NO_SAVE_PASS
		,!page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_SAVE_PASS));

	SonorkApp.ProfileCtrlFlags().SetClear(
		SONORK_PCF_PASS_PROTECT
		,page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_PROTECT));

	if( page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_DEFAULT) )
		startup_user_id.Set(SonorkApp.ProfileUserId());
	else
		startup_user_id.Clear();

	SonorkApp.ProfileCtrlFlags().SetClear(
		SONORK_PCF_NO_AUTO_CONNECT
		,!page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_AUTO_CX));

	SonorkApp.ProfileCtrlFlags().SetClear(
		SONORK_PCF_USING_PROXY
		,page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_PROXY));

	page_win[TAB_LOGIN]->GetCtrlText(IDC_MINFO_LOGIN_PASS
		,SonorkApp.wProfilePassword());

	SonorkApp.SaveCurrentProfile(
		SONORK_APP_BASE_SPF_SAVE_CTRL_DATA
		|SONORK_APP_BASE_SPF_SAVE_PASSWORD);
	startup_user_id.GetStr(tmp);

	usrModeKey.SetValue(szGuId,tmp);
	appModeKey.SetValue(szGuId,tmp);

	appRootKey.SetValue(szLang,SonorkApp.LangName());
	// NOTE: We don't write the 'Lang' value to the usrRootKey because
	// that is done automatically by SonorkApp.LangLoad().
}
void
 TSonorkMyInfoWin::UpdateLoginCheckboxes()
{
	BOOL     is_protected
		,is_save_pass
		,is_startup_user
		,is_auto_connect
		,is_connected
		,is_profile_open;


	if((is_profile_open=SonorkApp.IsProfileOpen()) != false)
	{
		is_connected	=SonorkApp.CxActiveOrReady();
		is_startup_user =page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_DEFAULT);
		is_save_pass	=page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_SAVE_PASS);
		is_protected	=page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_PROTECT);
		is_auto_connect	=page_win[TAB_LOGIN]->GetCtrlChecked(IDC_MINFO_LOGIN_AUTO_CX);
	}
	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_SAVE_PASS
		, is_profile_open);
		
	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_PROTECT
		, is_profile_open&&is_save_pass);

	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_DEFAULT
		, is_profile_open&&!is_protected && is_save_pass);

	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_AUTO_CX
		, is_profile_open&&!is_protected && is_save_pass && is_startup_user);

	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_PROXY
		 , !is_connected);

	page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_PROTECT
		, is_profile_open&&is_protected && is_save_pass);

	page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_AUTO_CX
		, is_profile_open&&!is_protected && is_auto_connect && is_startup_user);

	page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_DEFAULT
		,    is_profile_open
		  && is_startup_user
		  && is_save_pass
		  && !is_protected
		);
}
void
 TSonorkMyInfoWin::LoadLogin(bool user
			, bool net_profile
			, bool cx_change)
{
	BOOL	is_connected
		, is_profile_open;
	is_connected	= SonorkApp.CxActiveOrReady();
	is_profile_open	= SonorkApp.IsProfileOpen();
	if( user )
	{
		if( is_profile_open )
		{
			page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_SAVE_PASS
				,!SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_SAVE_PASS));

			page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_PROTECT
				,SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_PASS_PROTECT));

			page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_DEFAULT
				,SonorkApp.ProfileUserId() == startup_user_id);

			page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_AUTO_CX
				,!SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_NO_AUTO_CONNECT));

			page_win[TAB_LOGIN]->SetCtrlChecked(IDC_MINFO_LOGIN_PROXY
				,SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_USING_PROXY));

			page_win[TAB_LOGIN]->SetCtrlText(IDC_MINFO_LOGIN_USER
				,SonorkApp.ProfileUserAlias().CStr());

			page_win[TAB_LOGIN]->SetCtrlText(IDC_MINFO_LOGIN_PASS
				,SonorkApp.ProfilePassword().CStr());
		}
		else
		{
			page_win[TAB_LOGIN]->ClrCtrlText(IDC_MINFO_LOGIN_USER);
			page_win[TAB_LOGIN]->ClrCtrlText(IDC_MINFO_LOGIN_PASS);
		}
	}

	if( cx_change || user )
	{
		UpdateLoginCheckboxes();

		page_win[TAB_LOGIN]->SetCtrlEnabled(
			IDC_MINFO_LOGIN_CHANGE_USER
			, !is_connected );

		page_win[TAB_LOGIN]->SetCtrlEnabled(
			IDC_MINFO_LOGIN_CONNECT
			, !is_connected && is_profile_open);

		page_win[TAB_LOGIN]->SetCtrlEnabled(
			IDB_MINFO_LOGIN_NET_PROFILE
			, !is_connected );

		page_win[TAB_LOGIN]->SetCtrlEnabled(
			IDC_MINFO_LOGIN_CHANGE_PASS
			, is_connected);
	}
	
	if( net_profile )
	{
		TSonorkClientServerProfile SP;
		char pname[32];
		char tmp[64];
		SonorkApp.LoadServerProfile(SonorkApp.ProfileServerProfile().CStr()
			, SP
			, true
			, &SonorkApp.wProfileServerProfile());
		lstrcpyn(pname,SonorkApp.ProfileServerProfile().CStr(),24);
		SP.sonork.Host().CutAt(24);
		sprintf(tmp
			,"%s (%s)"
			,pname
			,SP.sonork.Host().CStr());
		page_win[TAB_LOGIN]->SetCtrlText(IDC_MINFO_LOGIN_NET_PROFILE,tmp);
	}

}
void
 TSonorkMyInfoWin::OnLangSelected()
{
	TSonorkError 	ERR;
	char	tmp[64];
	int index;

	index = ComboBox_GetCurSel(lang_list);
	cc_no_update++;
	if( index!=-1 )
	{
		ComboBox_GetString(lang_list,index,tmp);
		if(SonorkApp.LangLoad( ERR, tmp , true ))
		{
			SaveLogin();
		}
		else
		if(ERR.Result() != SONORK_RESULT_OK)
		{
			ErrorBox("Cannot load/No se puede cargar",&ERR);
			index=-1;
		}
	}
	if( index == -1 )
	{
		index = ComboBox_FindStringExact(lang_list
				,-1
				,SonorkApp.LangName());
		ComboBox_SetCurSel(lang_list,index);
	}
	cc_no_update--;
}
void
 TSonorkMyInfoWin::LoadInfo()
{
	TSonorkWin*	page;
	char 		tmp[80];
	TSonorkTime  	bdate;
	int		index;

	cc_no_update++;

	page = page_win[TAB_INFO];

	page->SetCtrlText(IDC_MINFO_ALIAS,SonorkApp.ProfileUser().alias.CStr());
	page->SetCtrlText(IDC_MINFO_NAME,SonorkApp.ProfileUser().name.CStr());
	page->SetCtrlText(IDC_MINFO_EMAIL,SonorkApp.ProfileUser().email.CStr());

	bdate.Set(SonorkApp.ProfileUser().BornTime());
	wsprintf(tmp,"%04u%c%02u%c%02u"
		,bdate.Year()
		,0
		,bdate.Month()
		,0
		,bdate.Day());

	page->SetCtrlText(IDC_MINFO_BDATE_YR,tmp);
	page->SetCtrlText(IDC_MINFO_BDATE_MO,tmp+5);
	page->SetCtrlText(IDC_MINFO_BDATE_DA,tmp+8);

	index = SonorkApp.ProfileUser().InfoFlags().GetSex();
	if( index== SONORK_SEX_F)
		index=1;
	else
	if( index== SONORK_SEX_M)
		index=0;
	else
		index=2;
	ComboBox_SetCurSel(sex_list,index);

	LoadTimeZone();
	if(SonorkApp.CountryCodeTable().GetRecordByCode(
		SonorkApp.ProfileRegion().GetCountry()
		, &index)==NULL)
		index = LB_ERR;
	ComboBox_SetCurSel( country_list, index );

	index = ComboBox_FindStringExact(lang_list,-1,SonorkApp.LangName());
	ComboBox_SetCurSel( lang_list , index );


	page = page_win[TAB_AUTH];
	page->SetCtrlUint(IDC_MINFO_AUTH_PUB_PIN
		,SonorkApp.ProfileUser().PublicAuthPin());
	page->SetCtrlChecked(IDC_MINFO_AUTH_PUB_UINFO
		, SonorkApp.ProfileUser().PublicAuthFlags().IsPublic()
		);
	page->SetCtrlChecked(IDC_MINFO_AUTH_PUB_HIDE_EMAIL
		, SonorkApp.ProfileUser().PublicAuthFlags().HideEmail()
		);
	page->SetCtrlChecked(IDC_MINFO_AUTH_PUB_HIDE_NETADDR
		, SonorkApp.ProfileUser().PublicAuthFlags().HideAddr()
		);

	cc_no_update--;

}

void
 TSonorkMyInfoWin::LoadNotes()
{
	TSonorkDynString	notes;
	SonorkApp.ReadProfileItem("Notes",&notes);
	page_win[TAB_PIC]->SetCtrlText(IDC_MINFO_PIC_NOTES
				,notes.ToCStr());
}

void
 TSonorkMyInfoWin::LoadAuthList()
{
	TSonorkWin*	   	page;
	DWORD			aux;
	TSonorkListIterator	I;
	SONORK_DWORD2*		pDW2;
	char 			tmp[32];

	page = page_win[TAB_AUTH];
	aux =  SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_IGNORE_NON_AUTHED_MSGS);
	page->SetCtrlChecked(IDC_MINFO_AUTH_ACCEPT , !aux );
	page->SetCtrlChecked(IDC_MINFO_AUTH_IGNORE , aux );
	ListBox_Clear( auth_list );
	SonorkApp.AuthExcludeList().InitEnum(I);
	while( (pDW2=SonorkApp.AuthExcludeList().EnumNext(I)) != NULL )
	{
		pDW2->GetStr(tmp);
		ListBox_AddString(auth_list,tmp);
	}
	ClearWinUsrFlag( AUTH_LIST_MODIFIED );
	SetWinUsrFlag(AUTH_LIST_LOADED);
}

void	TSonorkMyInfoWin::LoadEmailList()
{
	TSonorkEmailAccount* 		email_acc;
	TSonorkEmailExcept*  		email_exc=NULL;
	TSonorkAtomDb			db;
	TSonorkEmailAccountQueue 	acc_queue;
	DWORD				i,mi;
	char				tmp[64];
	TSonorkShortString		str;
	TSonorkRegKey			RK;
	const char			*ptr;

#define MINS	8
	static UINT	mins[MINS]={5,6,7,8,9,10,15,20};

	// Abort any e-mail checking in progress
	// because the user might edit the e-mail accounts which
	// are held in memory while the checking is in progress.
	SonorkApp.CancelCheckEmailAccounts();

	i =  SonorkApp.ProfileCtrlFlags().Test(SONORK_PCF_IGNORE_EMAILS);
	page_win[TAB_EMAIL]->SetCtrlChecked(IDC_MINFO_EMAIL_ALERT , !i );
	page_win[TAB_EMAIL]->SetCtrlChecked(IDC_MINFO_EMAIL_IGNORE , i );

	for(i=0;i<MINS;i++)
		ComboBox_AddString(email_chk_list,ultoa(mins[i],tmp,10));
	i = ComboBox_FindStringExact(email_chk_list
			,-1
			,ultoa(SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_CHECK_MINS),tmp,10));
	ComboBox_SetCurSel( email_chk_list , i );

	if( RK.Open(HKEY_LOCAL_MACHINE
		, "Software\\Clients\\Mail"
		, false
		, KEY_READ) == ERROR_SUCCESS)
	{
		for(i=0;RK.EnumKey(i,tmp,sizeof(tmp) ) == ERROR_SUCCESS;i++)
			ComboBox_AddString(email_rdr_list,tmp);

		if(SonorkApp.ReadProfileItem("MailClient",&str) == SONORK_RESULT_OK)
		{
			ptr=str.CStr();
		}
		else
			ptr=NULL;

		if( ptr == NULL )
		{
			if( RK.QueryValue(NULL,tmp, sizeof(tmp)) == ERROR_SUCCESS )
				ptr=tmp;
		}
		if( ptr!=NULL )
			i = ComboBox_FindStringExact(email_rdr_list,-1,ptr);
		else
			i = (DWORD)-1;
		ComboBox_SetCurSel(email_rdr_list,i);
		RK.Close();
	}


	// SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID)
	//  holds the UID for the default account
	// If the user deleted the default account, we make
	//  sure the profile value is cleared (we also
	//  clear the profile value when a list item is
	//  deleted, see the LVN_DELETEITEM handler)
	SonorkApp.LoadEmailAccounts(&acc_queue);
	if( !acc_queue.Items() )
		SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID)=0;

	while((email_acc = acc_queue.RemoveFirst()) != NULL )
		AddEmailAccount(email_acc);

	if( SonorkApp.OpenAppDb(SONORK_APP_DB_EMAIL_EXCEPT , db , false ) )
	{
		mi = db.Items();
		for(i=0;i<mi;i++)
		{
			if(email_exc == NULL )
				SONORK_MEM_NEW( email_exc = new TSonorkEmailExcept );
			if(db.Get(i,email_exc)!=SONORK_RESULT_OK)continue;
			AddEmailException(email_exc);
			email_exc=NULL;
		}
		if( email_exc!=NULL )
			SONORK_MEM_DELETE(email_exc);
		db.Close();
	}
	ClearWinUsrFlag( EMAIL_LIST_MODIFIED );

	SetWinUsrFlag( EMAIL_LIST_LOADED );
}

void	TSonorkMyInfoWin::AddEmailAccount(TSonorkEmailAccount *acc)
{
	if( acc->UID() == 0)
		acc->SetUID( SonorkApp.GenSelfTrackingNo() );
	email_acc.AddItem(acc->Name(),SKIN_ICON_EMAIL,(LPARAM)acc);
}
void	TSonorkMyInfoWin::AddEmailException(TSonorkEmailExcept *exc)
{
	email_exc.AddItem(exc->Name(),SKIN_ICON_TOOLS,(LPARAM)exc);
}

void	TSonorkMyInfoWin::SaveEmailList()
{
	TSonorkEmailAccount* 	EA;
	TSonorkEmailExcept*  	EE;
	TSonorkAtomDb		db;
	int			ci,mi;
	TSonorkShortString	str;

	page_win[TAB_EMAIL]->GetCtrlText(IDC_MINFO_EMAIL_READER,str);
	SonorkApp.WriteProfileItem("MailClient",&str);

	if( SonorkApp.OpenAppDb(SONORK_APP_DB_EMAIL_ACCOUNT , db , true ) )
	{
		mi = email_acc.GetCount();
		for(ci=0;ci<mi;ci++)
		{
			EA=(TSonorkEmailAccount*)email_acc.GetItem(ci);
			if(EA != NULL)
				db.Add(EA);
		}
		if(db.Items())SonorkApp.RequestCheckEmailAccounts();
		db.Close();
	}

	if( SonorkApp.OpenAppDb(SONORK_APP_DB_EMAIL_EXCEPT , db , true ) )
	{
		mi = email_exc.GetCount();
		for(ci=0;ci<mi;ci++)
		{
			EE = (TSonorkEmailExcept*)email_exc.GetItem(ci);
			if(EE != NULL)
				db.Add(EE);
		}
		db.Close();
	}
	ClearWinUsrFlag( EMAIL_LIST_MODIFIED );
}

void
 TSonorkMyInfoWin::SaveAuthList()
{
	int	ci,mi;
	TSonorkId	gu_id;
	char	tmp[64];

	mi = ListBox_GetCount(auth_list);
	SonorkApp.AuthExcludeList().Clear();
	SonorkApp.AuthExcludeList().PrepareFor( mi );
	for(ci=0;ci<mi;ci++)
	{
		ListBox_GetString(auth_list,ci,tmp);
		if( gu_id.SetStr( tmp ) )
			SonorkApp.AuthExcludeList().AddItem( &gu_id );
	}
	SonorkApp.WriteProfileItem(szAuthExcludeList
		,&TSonorkSimpleDataListAtom(&SonorkApp.AuthExcludeList(),SONORK_ATOM_SONORK_ID,true));
	ClearWinUsrFlag( AUTH_LIST_MODIFIED );
}

// --------------------------------------------------------------------------
// 	ADITIONAL EDIT
// --------------------------------------------------------------------------

void TSonorkMyInfoWin::Pk_EditAuth( EDIT_OP op )
{
	TSonorkId	gu_id;
	TSonorkId*	gu_id_ptr;
	char		tmp[64];
	int		index;
	if( op == EDIT_ADD )
	{
		TSonorkInputWin W( this );
		W.flags=0;
		W.help.Set(SonorkApp.LangString(GLS_LB_AUTHS));
		W.prompt.Set(szSonorkId);
		W.sign = SKIN_SIGN_SECURITY;
		if(W.Execute() == IDOK)
		if( gu_id.SetStr(W.input.CStr()) )
		if( SonorkApp.AuthExcludeList().Get(gu_id) == NULL )
		{
			SetWinUsrFlag( AUTH_LIST_MODIFIED );
			SonorkApp.AuthExcludeList().AddItem( &gu_id );
			ListBox_AddString(auth_list,W.input.CStr());
		}
	}
	else
	{
		index = ListBox_GetCurSel(auth_list);
		if(index==LB_ERR)return;
		ListBox_GetString(auth_list, index, tmp);
		if( !gu_id.SetStr( tmp ) )return;
		if((gu_id_ptr = (TSonorkId*)SonorkApp.AuthExcludeList().Get(gu_id))!=NULL)
			gu_id_ptr->Clear();
		ListBox_DelString(auth_list, index );
		SetWinUsrFlag( AUTH_LIST_MODIFIED );
	}

}
void TSonorkMyInfoWin::Pk_EditEmailAcc( EDIT_OP op )
{
	TSonorkEmailAccount*	acc;
	int	index;
	if( op == EDIT_ADD )
	{
		acc=new TSonorkEmailAccount;
		acc->Clear();
		acc->header.uid = SonorkApp.GenSelfTrackingNo();
		acc->str[SONORK_EMAIL_ACCOUNT_STR_RETURN_ADDRESS].Set(
			SonorkApp.ProfileUser().email);
		acc->str[SONORK_EMAIL_ACCOUNT_STR_REAL_NAME].Set(
			SonorkApp.ProfileUser().name);
		// SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID)
		//  holds the UID for the default account, if it is
		//  zero, we don't have a default account so we
		//  make this (new) one the default account.
		if( SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) == 0)
			SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) =acc->UID();

		if(TSonorkEmailAccountWin(this,acc).Execute()==IDOK)
			if( *acc->Name() )
			{
				SetWinUsrFlag( EMAIL_LIST_MODIFIED );
				AddEmailAccount(acc);
				return;
			}
		if( SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) == acc->UID())
			SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) =0;
		delete acc;
		return;
	}
	acc = (TSonorkEmailAccount*)email_acc.GetSelectedItem(&index);
	if( acc == NULL )return;

	if( op == EDIT_SET )
	{
		if( TSonorkEmailAccountWin(this,acc).Execute()!=IDOK )
			return;
		email_acc.SetItemText(index,0,acc->Name());
	}
	else
	if( op == EDIT_DEL )
	{
		email_acc.DelItem( index );
	}
	SetWinUsrFlag( EMAIL_LIST_MODIFIED );
}

void TSonorkMyInfoWin::Pk_EditEmailExc( EDIT_OP op )
{
	TSonorkEmailExcept*	exc;
	int			index;
	if( op == EDIT_ADD )
	{
		exc=new TSonorkEmailExcept;
		if(TSonorkEmailExceptWin(this,exc).Execute()==IDOK)
		if( exc->name.Length() )
		{
			SetWinUsrFlag( EMAIL_LIST_MODIFIED );
			AddEmailException(exc);
		}
		else
			delete exc;
		return;
	}
	exc = (TSonorkEmailExcept*)email_exc.GetSelectedItem(&index);
	if( exc == NULL )return;

	if( op == EDIT_SET )
	{
		if( TSonorkEmailExceptWin(this,exc).Execute()!=IDOK )
			return;
		email_exc.SetItemText(index,0,exc->name.CStr());
	}
	else
	if( op == EDIT_DEL )
	{
		email_exc.DelItem( index );
	}
	SetWinUsrFlag( EMAIL_LIST_MODIFIED );

}


// --------------------------------------------------------------------------
// Sonork TASK HANDLERS
// --------------------------------------------------------------------------

void
 TSonorkMyInfoWin::OnTaskStart(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2&tag)
{
	GLS_INDEX gls_index;
	if( tag.v[0] == SONORK_FUNCTION_SET_USER_DATA )
		gls_index = GLS_MS_STORING;
	else
		gls_index = GLS_MS_RFRESHING;
	TSonorkWin::SetStatus(status_bar,gls_index,SKIN_HICON_BUSY);
	page_win[TAB_TRACK]->SetCtrlEnabled( IDC_MINFO_TRACK_EDIT , false );
	page_win[TAB_TRACK]->SetCtrlEnabled( IDC_MINFO_TRACK_REFRESH , false );
	SetCtrlEnabled( IDC_MYINFO_REFRESH, false );
	SetCtrlEnabled( IDC_MYINFO_REFRESH, false );
	SetCtrlEnabled( IDOK, false );
}
void
 TSonorkMyInfoWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if( P->Function() == SONORK_FUNCTION_LIST_TRACKERS )
	{
		TSonorkUSearchWin::ProcessTrackerTreeTaskData(track_tree
				, SONORK_TREE_ITEM_TRACKER_ROOM_SUBS
				, P
				, P_size);
	}
	else
	if( P->Function() == SONORK_FUNCTION_SET_USER_DATA )
	{
		if( P->SubFunction() == 0)
		if( P->D_SetUserData_A(P_size, UDN.user_data.wSerial()) )
		if( UDN.user_data.GetUserInfoLevel() >= SONORK_USER_INFO_LEVEL_3 )
		{
			SonorkApp.BroadcastAppEvent(SONORK_APP_EVENT_SET_PROFILE
				,SONORK_APP_EM_PROFILE_AWARE
				,0
				,&UDN);
		}
	}
	else
	if( P->Function() == SONORK_FUNCTION_GET_USER_DATA )
	{
		SonorkApp.ProcessUserProfileData(P,P_size);
	}
}
void	TSonorkMyInfoWin::OnTaskEnd(SONORK_WIN_TASK_TYPE,const SONORK_DWORD2& tag, const TSonorkError*pERR)
{
	// We post ourselves a message: Can't diplay modal dialogs
	// 	from within a GuTask handler... we'd halt the whole GU engine
	taskERR.Set( *pERR );
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, tag.v[0]);
}





// --------------------------------------------------------------------------
// 		CONSTRUCTOR / CREATION / DESTRUCTION
// --------------------------------------------------------------------------

TSonorkMyInfoWin::TSonorkMyInfoWin(TSonorkWin*P,TAB tab)
	:TSonorkTaskWin(P
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_MY_INFO
	|SONORK_WIN_DIALOG
	|IDD_MYINFO
	,0)
{
	SetEventMask(SONORK_APP_EM_PROFILE_AWARE
		|SONORK_APP_EM_CX_STATUS_AWARE
		|SONORK_APP_EM_SKIN_AWARE);
	tab_ctrl.page=tab;
}

void
 TSonorkMyInfoWin::ShowCxOnlyMessageIfNotReady()
{
	if( !SonorkApp.CxReady() )
	{
		if(tab_ctrl.page <= TAB_AUTH || tab_ctrl.page == TAB_TRACK)
		{
			TSonorkWin::SetStatus(status_bar,GLS_MS_CXONLY,SKIN_HICON_ALERT);
			return;
		}
	}
	TSonorkWin::SetStatus_None(status_bar);

}

void
 TSonorkMyInfoWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDOK			,	GLS_OP_STORE	}
	,	{IDC_MYINFO_REFRESH	,	GLS_OP_REFRESH	}
	,	{-1			,	GLS_LB_MYINFO	}
	,	{0			,	GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_login[]=
	{	{IDC_MINFO_LOGIN_SAVE_PASS
			, GLS_LOGIN_SAVPWD}
	,	{IDC_MINFO_LOGIN_PROTECT
			, GLS_LOGIN_BLKPWD}
	,	{IDC_MINFO_LOGIN_DEFAULT
			, GLS_LOGIN_DEFAULT}
	,	{IDC_MINFO_LOGIN_AUTO_CX
			, GLS_LOGIN_AUTOCX}
	,	{IDC_MINFO_LOGIN_PROXY
			, GLS_LOGIN_PROXY	}
	,	{IDG_MINFO_LOGIN_USER 		//|SONORK_WIN_CTF_BOLD
			, GLS_NULL              }
	,	{IDL_MINFO_LOGIN_NET_PROFILE	|SONORK_WIN_CTF_BOLD
			, GLS_LB_SVR_HOST	}
	,	{IDB_MINFO_LOGIN_NET_PROFILE
			, GLS_LB_NETCFG		}
	,	{IDL_MINFO_LOGIN_USER   	|SONORK_WIN_CTF_BOLD
			, GLS_LB_USR		}
	,	{IDL_MINFO_LOGIN_PASS		|SONORK_WIN_CTF_BOLD
			, GLS_LB_PWD		}
	,	{IDC_MINFO_LOGIN_CHANGE_PASS
			, GLS_OP_CHGPWD		}
	,	{IDC_MINFO_LOGIN_CHANGE_USER
			, GLS_LB_SELPROF	}
	,	{IDG_MINFO_LOGIN_OPTIONS 	|SONORK_WIN_CTF_BOLD
			, GLS_LB_OPTS		}
	,	{IDC_MINFO_LOGIN_CONNECT 	|SONORK_WIN_CTF_BOLD
			, GLS_OP_CONNECT	}
	,	{0	,	GLS_NULL 	}
	};

	static TSonorkWinGlsEntry gls_info[]=
	{	{IDL_MINFO_ALIAS	|SONORK_WIN_CTF_BOLD
			, GLS_LB_ALIAS		}
	,	{IDL_MINFO_NAME		|SONORK_WIN_CTF_BOLD
			, GLS_LB_NAME		}
	,	{IDL_MINFO_EMAIL	|SONORK_WIN_CTF_BOLD
			, GLS_LB_EMAIL		}
	,	{IDL_MINFO_COUNTRY	|SONORK_WIN_CTF_BOLD
			, GLS_LB_COUNTRY	}
	,	{IDL_MINFO_LANGUAGE	|SONORK_WIN_CTF_BOLD
			, GLS_LB_LANG		}
	,	{IDL_MINFO_BDATE	|SONORK_WIN_CTF_BOLD
			, GLS_LB_BDATE		}
	,	{IDL_MINFO_SEX		|SONORK_WIN_CTF_BOLD
			, GLS_LB_SEX		}
	,	{IDG_MINFO_UINFO
			, GLS_LB_USRINFO	}
	,	{IDC_MINFO_PROFILES
			, GLS_LB_PROFILES	}
	,	{0	, GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_auth[]=
	{	{IDL_MINFO_AUTH_HELP	|SONORK_WIN_CTF_BOLD
			, GLS_MI_AUTHLP	}
	,	{IDL_MINFO_AUTH_LIST	|SONORK_WIN_CTF_BOLD
			, GLS_MI_AUTHLST	}
	,	{IDC_MINFO_AUTH_ACCEPT
			, GLS_OP_ACCEPT	}
	,	{IDC_MINFO_AUTH_IGNORE	
			, GLS_OP_IGNORE	}
	,	{IDC_MINFO_AUTH_ADD
			, GLS_OP_ADD	}
	,	{IDC_MINFO_AUTH_DEL
			, GLS_OP_DEL	}
	,	{IDG_MINFO_AUTH_PUB     |SONORK_WIN_CTF_BOLD
			, GLS_LB_PUBLST	}
	,	{IDC_MINFO_AUTH_PUB_UINFO
			, GLS_UA_UINFO}
	,	{IDC_MINFO_AUTH_PUB_HIDE_EMAIL
			, GLS_UA_NEMAIL}
	,	{IDC_MINFO_AUTH_PUB_HIDE_NETADDR
			, GLS_UA_NADDR}
	,	{0	, GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_email[]=
	{	{IDC_MINFO_EMAIL_ADD_ACCOUNT
			, GLS_OP_ADD		}
	,	{IDC_MINFO_EMAIL_DEL_ACCOUNT
			, GLS_OP_DEL		}
	,	{IDC_MINFO_EMAIL_ADD_EXCEPT
			, GLS_OP_ADD		}
	,	{IDC_MINFO_EMAIL_DEL_EXCEPT
			, GLS_OP_DEL		}
	,	{IDG_MINFO_EMAIL_ACCOUNTS|SONORK_WIN_CTF_BOLD
			, GLS_EM_ACCS		}
	,	{IDL_MINFO_EMAIL_READER	//|SONORK_WIN_CTF_BOLD
			, GLS_EM_RDR		}
	,	{IDG_MINFO_EMAIL_ALERTS	|SONORK_WIN_CTF_BOLD
			, GLS_EM_ACTION	}
	,	{IDC_MINFO_EMAIL_IGNORE	//|SONORK_WIN_CTF_BOLD
			, GLS_OP_IGNORE	}
	,	{IDC_MINFO_EMAIL_ALERT	//|SONORK_WIN_CTF_BOLD
			, GLS_OP_ALERT	}
	,	{IDL_MINFO_EMAIL_EXCEPT	//|SONORK_WIN_CTF_BOLD
			, GLS_EM_EXCLST	}
	,	{IDL_MINFO_EMAIL_CHECK_MINS	//|SONORK_WIN_CTF_BOLD
			, GLS_LB_MINS		}
	,	{IDL_MINFO_EMAIL_CHECK_EVERY	//|SONORK_WIN_CTF_BOLD
			, GLS_EM_CHKEV	}
	,	{0								,	GLS_NULL		}
	};
	static TSonorkWinGlsEntry gls_prefs[]=
	{	{IDG_MINFO_PREFS_LABEL	|SONORK_WIN_CTF_BOLD
			, GLS_NULL	}
	,	{IDC_MINFO_PREFS_ENABLED|SONORK_WIN_CTF_BOLD
			, GLS_LB_ENA	}
	,	{IDC_MINFO_PREFS_DISABLED|SONORK_WIN_CTF_BOLD
			, GLS_LB_DIS	}
	,	{0	, GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_pic[]=
	{
		{IDG_MINFO_PIC_NOTES
			, GLS_LB_NOTES		}
	,	{IDC_MINFO_PIC_NOTES_HELP
			, GLS_MI_LNHLP		}
	,	{IDC_MINFO_PIC_BROWSE
			, GLS_OP_BROWSE		}
	,	{0	, GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_skin[]=
	{
		{IDL_MINFO_SKIN_COLOR	|SONORK_WIN_CTF_BOLD
			, GLS_NULL	}
	,	{IDC_MINFO_SKIN_256
			, GLS_CL_LO	}
	,	{IDC_MINFO_SKIN_DEFAULT
			, GLS_CL_HI	}
	,	{IDC_MINFO_SKIN_UNDO
			, GLS_OP_UNDO	}
	,	{0	, GLS_NULL	}
	};
	static TSonorkWinGlsEntry gls_track[]=
	{
		{IDC_MINFO_TRACK_REFRESH|SONORK_WIN_CTF_BOLD
			, GLS_OP_REFRESH	}
	,	{IDC_MINFO_TRACK_EDIT	|SONORK_WIN_CTF_BOLD
			, GLS_OP_OPEN	}
	,	{IDL_MINFO_TRACK_NOCX
			, GLS_MS_CXONLY	}
	,	{0	, GLS_NULL	}
	};
	static GLS_INDEX tab_gls[TABS]=
	{
		GLS_LB_PROFILES
	,	GLS_LB_INFO
	,	GLS_LB_NOTES
	,	GLS_LB_AUTHS
	,	GLS_OP_PREFS
	,	GLS_LB_EMAIL
	, 	GLS_TR_ROOMS
	,	GLS_LB_COLOR
	};

	static GLS_INDEX gls_sex[3]=
	{
		GLS_LB_SEXM
	,	GLS_LB_SEXF
	,	GLS_LB_SEXN
	};
	int i,index;
	union {
		char	str[128];
		TC_ITEM tc_item;
	}D;
	// ----------------------------------------------------
	// LANGUAGE LABELS AND FONTS SETTINGS

	cc_no_update++;
	this->LoadLangEntries(gls_table,false);
	page_win[TAB_LOGIN]->LoadLangEntries(gls_login,false);
	page_win[TAB_INFO]->LoadLangEntries(gls_info,false);
	page_win[TAB_PIC]->LoadLangEntries(gls_pic,false);
	page_win[TAB_AUTH]->LoadLangEntries(gls_auth,false);
	page_win[TAB_PREFS]->LoadLangEntries(gls_prefs,false);
	page_win[TAB_EMAIL]->LoadLangEntries(gls_email,false);
	page_win[TAB_TRACK]->LoadLangEntries(gls_track,false);
	page_win[TAB_SKIN]->LoadLangEntries(gls_skin,false);


	index = ComboBox_GetCurSel(sex_list);
	ComboBox_Clear(sex_list);
	for(i=0;i<3;i++)
		ComboBox_AddString(sex_list,SonorkApp.LangString(gls_sex[i]));
	ComboBox_SetCurSel(sex_list,index);

	wsprintf(D.str
		,"(%s/%s/%s)"
		,SonorkApp.LangString(GLS_LB_DYR)
		,SonorkApp.LangString(GLS_LB_DMO)
		,SonorkApp.LangString(GLS_LB_DDA));

	page_win[TAB_INFO]->SetCtrlText(IDL_MINFO_BDATE_FORMAT,D.str);


	D.tc_item.mask=TCIF_TEXT|TCIF_IMAGE;
	D.tc_item.lParam=0;
	D.tc_item.iImage=-1;
	for(i=0;i<TABS;i++)
	{
		D.tc_item.pszText=(char*)SonorkApp.LangString(tab_gls[i]);
		TabCtrl_SetItem(tab_ctrl.hwnd,i,&D.tc_item);
	}
	cc_no_update--;
}
BOOL
 TSonorkMyInfoWin::OnQueryClose()
{
	return IsTaskPending();
}

bool
 TSonorkMyInfoWin::OnCreate()
{
	RECT	rect;
	SIZE	sz;
	UINT	aux;
	UINT	cx_border;
	int	i;
	union {
		TC_ITEM 	tc_item;
		LV_ITEM 	lv_item;
		const TSonorkLangCodeRecord	*REC;
		char		tmp[64];
		HDC		dc;
	}D;
//	char tmp[48];
	const char *key_name;


	static UINT tab_dlg_id[TABS]=
	{
		IDD_MINFO_LOGIN
	,	IDD_MINFO_INFO
	, 	IDD_MINFO_PIC
	,	IDD_MINFO_AUTH
	,	IDD_MINFO_PREFS
	,	IDD_MINFO_EMAIL
	, 	IDD_MINFO_TRACK
	, 	IDD_MINFO_SKIN
	};
	static GLS_INDEX skin_list_gls[SKIN_INDEXES]=
	{
		GLS_LB_MSGS
	,	GLS_CL_MAIN
	,	GLS_LB_CLPBRD
	,	GLS_LB_SYSCON
	,	GLS_CR_NAME
	};

	// Start with cc_no_update counter set to 1 to
	// prevent our handlers from processing
	// events caused by the loading of the forms
	cc_no_update=1;

	// ----------------------------------------------------
	// STATUS BAR

	status_bar=GetDlgItem(IDC_MYINFO_STATUS_BAR);

// ----------------------------------------------------
// TAB CONTROL

#define MARGIN	2

	tab_ctrl.hwnd=GetDlgItem(IDC_MYINFO_TAB);
	TabCtrl_GetItemRect(tab_ctrl.hwnd,0,&rect);
	aux = (rect.bottom  - rect.top) + 1;
	::GetWindowRect(tab_ctrl.hwnd,&rect);
	ScreenToClient(&rect);

	cx_border=GetSystemMetrics(SM_CXBORDER);
	sz.cx=cx_border + MARGIN;
	sz.cy=GetSystemMetrics(SM_CYBORDER) + MARGIN;
	rect.left 	+= sz.cx;
	rect.top  	+= aux + sz.cy + MARGIN;
	rect.right	-= sz.cx;
	rect.bottom	-= sz.cy;

	D.tc_item.mask=TCIF_TEXT|TCIF_IMAGE;
	D.tc_item.lParam=0;
	D.tc_item.iImage=-1;
	for(i=0;i<TABS;i++)
	{
		D.tc_item.pszText="";
		TabCtrl_InsertItem(tab_ctrl.hwnd,i,&D.tc_item);
		page_win[i] = new TSonorkChildDialogWin(this
				,tab_dlg_id[i]
				,SONORK_WIN_SF_NO_CLOSE);
		TSonorkWin::CenterWin(page_win[i],rect,SONORK_CENTER_WIN_F_CREATE);
	}


	// ----------------------------------------------------
	// LOGIN

	page_win[TAB_LOGIN]->SetCtrlEnabled(IDC_MINFO_LOGIN_CHANGE_USER
		,!SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_USER_SELECT));

	page_win[TAB_LOGIN]->SetCtrlEnabled(IDB_MINFO_LOGIN_NET_PROFILE
		,!SonorkApp.TestCfgFlag(SONORK_WAPP_CF_NO_NET_CONFIG));

	// Set NO_AUTO_LOGIN flag so that the application
	// does not try to automatically login while we're displaying
	// this form.
	SonorkApp.SetRunFlag( SONORK_WAPP_RF_NO_AUTO_LOGIN );

	// Clear the CX_PENDING flag which indicates that
	SonorkApp.ClearRunFlag( SONORK_WAPP_RF_CX_PENDING );

	key_name = SonorkApp.ConfigKeyName();
	if(appRootKey.Open(HKEY_LOCAL_MACHINE
			,szSrkClientRegKeyRoot
			,true
			,KEY_ALL_ACCESS)==SONORK_RESULT_OK)
	{
		appModeKey.Open(appRootKey
				,key_name
				,true
				,KEY_ALL_ACCESS);
	}

	if(usrRootKey.Open(HKEY_CURRENT_USER
			,szSrkClientRegKeyRoot
			,true
			,KEY_ALL_ACCESS)==ERROR_SUCCESS)
	{
		usrModeKey.Open(usrRootKey
				,key_name
				,true
				,KEY_ALL_ACCESS);
	}
	if(usrModeKey.QueryValue(szGuId,D.tmp,sizeof(D.tmp)) != ERROR_SUCCESS )
		D.tmp[0]=0;
	startup_user_id.SetStr( D.tmp );



	// ----------------------------------------------------
	// USER INFO

	sex_list	= page_win[TAB_INFO]->GetDlgItem( IDC_MINFO_SEX );
	country_list 	= page_win[TAB_INFO]->GetDlgItem( IDC_MINFO_COUNTRY );
	lang_list 	= page_win[TAB_INFO]->GetDlgItem( IDC_MINFO_LANGUAGE );

	for(i=0,D.REC = SonorkApp.CountryCodeTable().Table()
		;i<SonorkApp.CountryCodeTable().Items()
		;i++,D.REC++)
		ComboBox_AddString(country_list,D.REC->name);

	SonorkApp.EnumLanguagesIntoComboBox( lang_list );

	// ----------------------------------------------------
	// NOTES


	picHwnd=page_win[TAB_PIC]->GetDlgItem(IDC_MINFO_PIC);
	::GetClientRect(picHwnd,&rect);
	picBmRect.left   = (rect.right - SONORK_USER_PIC_SW)/2-2;
	picBmRect.top	   = (rect.bottom - SONORK_USER_PIC_SH)/2-2;
	picBmRect.bottom = picBmRect.top+SONORK_USER_PIC_SH+4;
	picBmRect.right  = picBmRect.left+SONORK_USER_PIC_SW+4;

	picBm.InitHwnd( picHwnd );

	// ----------------------------------------------------
	// AUTHORIZATIONS

	auth_list = page_win[TAB_AUTH]->GetDlgItem( IDC_MINFO_AUTH_LIST );
	::GetClientRect( auth_list, &rect );
	::SendMessage( auth_list, LB_SETCOLUMNWIDTH , rect.right/3 , 0);



	// ----------------------------------------------------
	// PREFERENCES

	cur_pref_item = MINFO_PREF_INVALID;
	prefs_list.SetHandle(
		  page_win[TAB_PREFS]->GetDlgItem( IDC_MINFO_PREFS_LIST )
		, false
		, false);
	prefs_list.SetImageList(NULL,sonork_skin.Icons());
	prefs_combo= page_win[TAB_PREFS]->GetDlgItem( IDC_MINFO_PREFS_COMBO );

	prefs_list.AddColumn(0,""
			,page_win[TAB_PREFS]->GetCtrlWidth(IDC_MINFO_PREFS_LIST)
			-GetSystemMetrics(SM_CXHSCROLL)
			-cx_border*2-2);

	for( 	i=0
		;i<MINFO_PREFERENCES
		;i++ )
	{
		if( GetPreferenceSetting((MINFO_PREFERENCE)i) )
			aux = INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_CHECKED+1);
		else
			aux = INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_UNCHECKED+1);
		prefs_list.AddItemState(
			  GetPreferenceLabel((MINFO_PREFERENCE)i)
			, aux
			, LVIS_STATEIMAGEMASK
			, i);
	}

	// ----------------------------------------------------
	// EMAIL

	email_chk_list = page_win[TAB_EMAIL]->GetDlgItem( IDC_MINFO_EMAIL_CHECK_MINS );
	email_rdr_list = page_win[TAB_EMAIL]->GetDlgItem( IDC_MINFO_EMAIL_READER );
	email_acc.SetHandle(
		  page_win[TAB_EMAIL]->GetDlgItem( IDC_MINFO_EMAIL_ACCOUNT_LIST )
		, true
		, false);
	email_exc.SetHandle(
		page_win[TAB_EMAIL]->GetDlgItem( IDC_MINFO_EMAIL_EXCEPT_LIST )
		, true
		, false);

	email_acc.AddColumn(0,"",page_win[TAB_EMAIL]->GetCtrlWidth(IDC_MINFO_EMAIL_ACCOUNT_LIST) - 2);
	email_exc.AddColumn(0,"",page_win[TAB_EMAIL]->GetCtrlWidth(IDC_MINFO_EMAIL_EXCEPT_LIST) - 2);


	// ----------------------------------------------------
	// TRACKERS
	track_tree.SetHandle(page_win[TAB_TRACK]->GetDlgItem(IDC_MINFO_TRACK_LIST));
	

	// ----------------------------------------------------
	// SKIN
	skin.view.AssignCtrl(this
		,page_win[TAB_SKIN]->GetDlgItem( IDB_MINFO_SKIN_PREVIEW )
		, IDB_MINFO_SKIN_PREVIEW);
	::GetClientRect( skin.view.Handle() ,&skin.viewRect);
	HDC scrDC=::GetDC(skin.view.Handle());
	skin.memDC=CreateCompatibleDC( scrDC );
	skin.memBM=CreateCompatibleBitmap(scrDC,skin.viewRect.right,skin.viewRect.bottom);
	SelectObject(skin.memDC,skin.memBM);
	::ReleaseDC(skin.view.Handle(),scrDC);

	skin.list = page_win[TAB_SKIN]->GetDlgItem( IDC_MINFO_SKIN_LIST );
	skin.selected_color=NO_SELECTED_COLOR;
	for(i=0;i<SKIN_INDEXES;i++)
		ComboBox_AddString(skin.list,SonorkApp.LangString(skin_list_gls[i]));
	ComboBox_SetCurSel( skin.list , 0);
	RebuildSkinPreview();

	// ----------------------------------------------------
	// LOADING


	SetCurrentTab(
		SonorkApp.IsProfileOpen()
		?tab_ctrl.page
		:TAB_LOGIN
		, true
		, true );


	::SetWindowPos(tab_ctrl.hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_MY_INFO,true,"MyInfoWin");

	// Set cc_no_update to 0
	cc_no_update--;

	return true;
}
void
 TSonorkMyInfoWin::OnAfterCreate()
{
	CmdLoad();
	UpdateCxItems();
	if(!SonorkApp.IsProfileOpen())
		SelectProfile();

}
void
 TSonorkMyInfoWin::BrowsePic()
{
	TSonorkShortString	path;
	if( !SonorkApp.IsProfileOpen() || !picBm.Initialized())
		return;
	if(!SonorkApp.GetLoadPath(Handle()
		, path
		, NULL
		, GLS_OP_LOAD
		, "PicPath"
		, "bmp"
		, OFN_EXPLORER
		  | OFN_LONGNAMES
		  | OFN_NOCHANGEDIR
		  | OFN_PATHMUSTEXIST
		  | OFN_FILEMUSTEXIST
		))
		return;
	if( LoadPic(path.CStr()) )
	{
		SetWinUsrFlag(PIC_MODIFIED);
	}


}


void
 TSonorkMyInfoWin::SavePic()
{
	TSonorkTempBuffer	bmp_path(SONORK_MAX_PATH);
	TSonorkTempBuffer	zip_path(SONORK_MAX_PATH);
	TSonorkZipStream* 	ZH;
	int			i;
	BOOL			saved_ok;


	if(!SonorkApp.IsProfileOpen() || !TestWinUsrFlag(PIC_LOADED) )
		return;

	SonorkApp.MakeProfileFilePath(bmp_path,"picBmp");
	if( !picBm.Save( bmp_path ) )
		return;
		
	SonorkApp.MakeProfileFilePath(zip_path,"pic.zip");
	ZH = SonorkApp.ZipEngine()->OpenForDeflate( zip_path , false , 4096);
	if(ZH == NULL)
		return;
	saved_ok=false;
	if(SonorkApp.ZipEngine()->InitDeflateFile(
			  ZH
			, bmp_path
			, 9
			, "picBmp"	// zip_name
			) )
	{
		for(i=0;i<64;i++)	// 64*4096 bytes should be enough
		{
			if(!SonorkApp.ZipEngine()->DoDeflateFile(ZH) )
				break;
			if( ZH->OperationComplete() )
			{
				saved_ok=true;
				break;
			}
		}
	}
	SonorkApp.ZipEngine()->Close(ZH);
	if(!saved_ok)
	{
		DeleteFile(zip_path);
	}
	else
	{
		ClearWinUsrFlag(PIC_MODIFIED);
	}
}


bool
 TSonorkMyInfoWin::LoadPic(SONORK_C_CSTR p_path)
{
	TSonorkTempBuffer	path(SONORK_MAX_PATH);
	
	MultiClearWinUsrFlags( PIC_LOADED|PIC_MODIFIED );

	if( !picBm.Initialized() )
		return false;
	if(SonorkApp.IsProfileOpen())
	{
		if(!p_path)
			SonorkApp.MakeProfileFilePath(path,"picBmp");
		else
			strcpy(path.CStr(),p_path);

		if( picBm.LoadFile(
				path.CStr()
				, SONORK_USER_PIC_SW
				, SONORK_USER_PIC_SH) )
		{
			SetWinUsrFlag( PIC_LOADED );
		}
	}
	if( !picBm.Loaded() )
	{
		picBm.CreateBlank(SONORK_USER_PIC_SW
				, SONORK_USER_PIC_SH
				, GetSysColorBrush(COLOR_3DDKSHADOW) );
	}
	::InvalidateRect(picHwnd,NULL,true);
	return TestWinUsrFlag( PIC_LOADED ) != 0;
}
void
 TSonorkMyInfoWin::OnBeforeDestroy()
{
	if( TestWinUsrFlag(AUTH_LIST_MODIFIED  ) )
		SaveAuthList();

	if( TestWinUsrFlag(EMAIL_LIST_MODIFIED  ) )
		SaveEmailList();

	if( TestWinUsrFlag(TRACKER_SUBS_MODIFIED  ) )
		SaveTrackerSubs();
	track_tree.DelAllItems();
	email_acc.DelAllItems();
	email_exc.DelAllItems();
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_MY_INFO,false,"MyInfoWin");
	DeleteDC(skin.memDC);
	DeleteObject(skin.memBM);
	skin.memDC=NULL;
}
// --------------------------------------------------------------------------
// 		EVENT HANDLERS
// --------------------------------------------------------------------------

bool
 TSonorkMyInfoWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		switch( id )
		{
			case IDOK:
				CmdStore(false,true);
				break;
				
			case IDC_MYINFO_REFRESH:
				CmdRefresh();
				break;
				
			default:
				return false;
		}
		return true;
	}
	return false;
}
LRESULT
 TSonorkMyInfoWin::OnChildDialogNotify(TSonorkChildDialogNotify*childN)
{

	TSonorkWinNotify*N =childN->N;
	union
	{
		LPARAM			lParam;
		TSonorkEmailAccount*	acc;
		TSonorkEmailExcept*	exc;
		TSonorkTreeItem*	tree;
	}P;
	int 	index;
	UINT	dialogId,itemId,code;


	dialogId= childN->dialog_id;
	itemId	= N->hdr.idFrom;
	code	= N->hdr.code;
	if(dialogId == IDD_MINFO_TRACK )
	{
		if( itemId == IDC_MINFO_TRACK_LIST )
		{
		switch( code )
		{
			case NM_CUSTOMDRAW:
				return TSonorkTreeItem::OnCustomDraw(&N->tdraw);

			case NM_CLICK:
			case NM_DBLCLK:
				::GetCursorPos(&N->pt);
				P.tree = track_tree.HitTest(
						 N->pt.x
						,N->pt.y
						,code==NM_CLICK
						?TVHT_ONITEMICON
						:TVHT_ONITEM|TVHT_ONITEMINDENT|TVHT_ONITEMRIGHT);
				if(P.tree!=NULL)
				{
					if( code == NM_DBLCLK )
					{
						PostPoke( POKE_EDIT_TRACKER
						, P.lParam );
					}
					else
					{
						track_tree.ExpandItemLevels(P.tree
							, TVE_TOGGLE);

					}
				}
				break;

			case TVN_GETDISPINFO:		// asking for presentation
				return TSonorkTreeItem::OnGetDispInfo(
					&N->tdispinfo
					);

			case TVN_DELETEITEM:       // a item is being deleted
			{
				P.lParam=N->tview.itemOld.lParam;
				if(P.lParam)
					SONORK_MEM_DELETE(P.tree);
			}
			break;
		}
		}
		else
		if(code==BN_CLICKED)
		{
			if( itemId == IDC_MINFO_TRACK_EDIT )
			{
				P.tree=track_tree.GetSelectedItem();
				if(P.tree!=NULL)
					PostPoke( POKE_EDIT_TRACKER
						, P.lParam );
			}
			else
			if( itemId == IDC_MINFO_TRACK_REFRESH )
			{
				LoadTrackerTree(false);
			}
		}

	}
	else
	if( dialogId == IDD_MINFO_EMAIL )
	{
		if( N->hdr.hwndFrom == email_acc.Handle()  )
		{
			if( code == LVN_DELETEITEM )
			{
				P.lParam = N->lview.lParam;
				if( P.lParam != NULL )
				{
					// SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID)
					//  holds the UID for the default account
					// If the user deleted the default account, we make
					//  sure the profile value is cleared.
					// We don't do this when the accounts are being
					//  automatically deleted because the window destroyed.
					if( !TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
					if( P.acc->UID() == SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID))
						SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID)=0;
					SONORK_MEM_DELETE(P.acc);
				}
			}
			else
			if( code == NM_DBLCLK)
			{
				PostPoke(POKE_EDIT_EMAIL_ACC ,EDIT_SET );
			}
		}
		else
		if( N->hdr.hwndFrom == email_exc.Handle() )
		{
			if( code == LVN_DELETEITEM )
			{
				P.lParam=N->lview.lParam;
				if(  P.lParam!= 0 )
				{
					SONORK_MEM_DELETE(P.exc);
				}
			}
			else
			if( code == NM_DBLCLK)
			{
				PostPoke(POKE_EDIT_EMAIL_EXC , EDIT_SET );
			}
		}
		else
		if( code == BN_CLICKED )
		{
			if( itemId == IDC_MINFO_EMAIL_ADD_ACCOUNT
			|| itemId==IDC_MINFO_EMAIL_DEL_ACCOUNT)
			{
				PostPoke(POKE_EDIT_EMAIL_ACC,
					itemId == IDC_MINFO_EMAIL_ADD_ACCOUNT
					? EDIT_ADD
					: EDIT_DEL
					);
			}
			else
			if( itemId == IDC_MINFO_EMAIL_ADD_EXCEPT
			|| itemId==IDC_MINFO_EMAIL_DEL_EXCEPT)
			{
				PostPoke(POKE_EDIT_EMAIL_EXC
					, itemId == IDC_MINFO_EMAIL_ADD_EXCEPT
					? EDIT_ADD
					: EDIT_DEL);
			}
			else
			if( itemId == IDC_MINFO_EMAIL_ALERT
			||  itemId == IDC_MINFO_EMAIL_IGNORE)
			{
				SonorkApp.ProfileCtrlFlags().SetClear(
					SONORK_PCF_IGNORE_EMAILS
					,page_win[TAB_EMAIL]->GetCtrlChecked(IDC_MINFO_EMAIL_IGNORE));
			}
		}
		else
		if( code == CBN_SELENDOK && TestWinUsrFlag(EMAIL_LIST_LOADED) )
		{
			if( N->hdr.hwndFrom == email_chk_list )
			{
				SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_CHECK_MINS)	=
					page_win[TAB_EMAIL]->GetCtrlUint( IDC_MINFO_EMAIL_CHECK_MINS );
			}
			else
			if( N->hdr.hwndFrom == email_rdr_list )
			{
				SetWinUsrFlag( EMAIL_LIST_MODIFIED );
			}
		}

	}
	else
	if( dialogId == IDD_MINFO_INFO )
	{
		if(cc_no_update)
			return true;

		if( itemId == IDC_MINFO_TZ_UDN
		   && code == UDN_DELTAPOS)
		{
			index = SonorkApp.ProfileRegion().GetTimeZone();
			index -= N->updown.iDelta;

			if( index<SONORK_REGION_TZ_MIN_VALUE
			 || index>SONORK_REGION_TZ_MAX_VALUE )
				return true;      // Prevent change
			SonorkApp.wProfileRegion().SetTimeZone(index);
			LoadTimeZone();
		}
		else
		if( itemId == IDC_MINFO_LANGUAGE && code == CBN_SELENDOK)
		{
			OnLangSelected();
		}
		else
		if( itemId == IDC_MINFO_PROFILES && code==BN_CLICKED)
		{
			SetCurrentTab(TAB_LOGIN , true, true );

		}
	}
	else
	if( dialogId == IDD_MINFO_AUTH )
	{
		if( code == BN_CLICKED )
		{
			if( itemId == IDC_MINFO_AUTH_ADD
			||  itemId==IDC_MINFO_AUTH_DEL)
			{
				PostPoke(POKE_EDIT_AUTH
					, itemId == IDC_MINFO_AUTH_ADD
					?EDIT_ADD
					:EDIT_DEL);

			}
			else
			if( itemId == IDC_MINFO_AUTH_IGNORE
			||  itemId == IDC_MINFO_AUTH_ACCEPT )
			{
				SonorkApp.ProfileCtrlFlags().SetClear(
					SONORK_PCF_IGNORE_NON_AUTHED_MSGS
					,page_win[TAB_AUTH]->GetCtrlChecked(IDC_MINFO_AUTH_IGNORE));
			}
		}
	}
	else
	if( dialogId == IDD_MINFO_PREFS )
	{
		if(itemId == IDC_MINFO_PREFS_LIST )
		{
			if(code == LVN_ITEMCHANGED)
			{
				if(N->lview.iItem != -1
				&& N->lview.uNewState&LVIS_FOCUSED)
				{
					OnPrefItemSelected();
				}
			}
			else
			if(code == NM_DBLCLK)
			{
				OnPrefItemToggle();
			}

		}
		else
		if(code==BN_CLICKED
		&& (itemId==IDC_MINFO_PREFS_ENABLED
		|| itemId==IDC_MINFO_PREFS_DISABLED))
		{
			OnPrefItemFlagSelected();
		}
		else
		if(code==CBN_SELENDOK && itemId==IDC_MINFO_PREFS_COMBO)
			OnPrefItemComboSelected();
		else
		if(code==EN_KILLFOCUS && itemId==IDC_MINFO_PREFS_TEXT)
			OnPrefItemTextKillFocus();

	}
	else
	if( dialogId == IDD_MINFO_SKIN )
	{
		if( cc_no_update )return 0;
		if( itemId==IDC_MINFO_SKIN_LIST)
		{
			if(code == CBN_SELENDOK )
			{
				skin.selected_color=NO_SELECTED_COLOR;
				RebuildSkinPreview();
			}
		}
		else
		if(code == BN_CLICKED)
		{
			if(itemId == IDC_MINFO_SKIN_DEFAULT
			||itemId == IDC_MINFO_SKIN_256)
			{
				sonork_skin.SetDefaultColors(itemId == IDC_MINFO_SKIN_256,true);
				SetWinUsrFlag(SKIN_MODIFIED);
			}
			else
			if(itemId == IDC_MINFO_SKIN_UNDO)
			{
				SonorkApp.ReadProfileItem("Skin",&TSonorkSkinCodecAtom(&sonork_skin));
				ClearWinUsrFlag(SKIN_MODIFIED);
			}
			else
				return 0;
			RebuildSkinPreview();
			SonorkApp.BroadcastAppEvent(SONORK_APP_EVENT_SKIN_COLOR_CHANGED
				,SONORK_APP_EM_SKIN_AWARE
				,0,0);
		}
	}
	else
	if( dialogId == IDD_MINFO_LOGIN )
	{
		if(code==BN_CLICKED)
		{
			switch(itemId)
			{
				case IDC_MINFO_LOGIN_PROXY:
				case IDC_MINFO_LOGIN_AUTO_CX:
				case IDC_MINFO_LOGIN_SAVE_PASS:
				case IDC_MINFO_LOGIN_PROTECT:
				case IDC_MINFO_LOGIN_DEFAULT:
					UpdateLoginCheckboxes();
				break;

				case IDC_MINFO_LOGIN_CONNECT:
					CmdStore(true,true);
					break;

				case IDC_MINFO_LOGIN_CHANGE_USER:
					SelectProfile();
				break;

				case IDB_MINFO_LOGIN_NET_PROFILE:
				{
					TSonorkNetCfgWin(this).Execute();
					LoadLogin(false,true,false);

				}
				break;

				case IDC_MINFO_LOGIN_CHANGE_PASS:
				{
					UINT	A_size,P_size;
					TSonorkDataPacket*	P;
					TSonorkChgPwdWin CP(this);
					if( CP.Execute() == IDOK )
					{
						sPwd.Set(CP.pass);
						A_size = CODEC_Size(&sPwd)+32;
						P = SONORK_AllocDataPacket( A_size );
						P_size = P->E_SetPass_R(A_size,sPwd);
						N->tag.v[0]=SONORK_FUNCTION_SET_PASS;
						StartSonorkTask(taskERR
							,P
							,P_size
							,SONORK_TASKWIN_DEFAULT_FLAGS
							,GLS_TK_PUTINFO
							,&N->tag);
						SONORK_FreeDataPacket( P );
					}
				}
				break;

				default:
					return false;
			}
			return true;
		}

	}
	else
	if( dialogId == IDD_MINFO_PIC )
	{
		if(code==BN_CLICKED && itemId == IDC_MINFO_PIC_BROWSE)
		{
			BrowsePic();
			return true;
		}

	}
	return 0;
}

bool
 TSonorkMyInfoWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID ==  IDC_MYINFO_ICON )
	{
		::FillRect(S->hDC,&S->rcItem,(HBRUSH)GetSysColorBrush(COLOR_3DFACE) );
		sonork_skin.DrawSign(S->hDC,SKIN_SIGN_QUERY,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

LRESULT
 TSonorkMyInfoWin::OnChildDialogDraw(TSonorkChildDialogNotify* CN)
{
	DRAWITEMSTRUCT*S=(DRAWITEMSTRUCT*)CN->N;

	if( CN->dialog_id == IDD_MINFO_SKIN &&
		S->CtlID == IDB_MINFO_SKIN_PREVIEW )
	{
		::BitBlt(S->hDC,0,0
			,skin.viewRect.right,skin.viewRect.bottom
			, skin.memDC
			, 0, 0, SRCCOPY);
		return true;
	}
	else
	if( CN->dialog_id == IDD_MINFO_PIC &&
		S->CtlID == IDC_MINFO_PIC )
	{
		::FillRect(S->hDC
			, &S->rcItem
			, GetSysColorBrush(COLOR_3DFACE));
		if( picBm.Loaded() )
		{
			::DrawEdge(
			   S->hDC
				,&picBmRect
				, EDGE_RAISED
				, BF_RECT);
			picBm.BitBlt(S->hDC
				, picBmRect.left+2
				, picBmRect.top +2 );
		}
	}
	return 0;
}

bool
 TSonorkMyInfoWin::OnAppEvent(UINT event, UINT , void* )
{
	switch( event )
	{
		case SONORK_APP_EVENT_CX_STATUS:

			UpdateCxItems();
			LoadLogin(false,false,true);
			return true;

		case SONORK_APP_EVENT_SET_LANGUAGE:
			LoadLabels();
			TSonorkWin::SetStatus_None(status_bar);
			return true;

		case SONORK_APP_EVENT_SKIN_COLOR_CHANGED:
			track_tree.UpdateSkin();
			prefs_list.UpdateSkin();
			email_acc.UpdateSkin();
			email_exc.UpdateSkin();
			return true;

	}
	return false;
}
LRESULT
 TSonorkMyInfoWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch( wParam )
	{
		case POKE_EDIT_COLOR:
			EditSkinColor( lParam );
			break;

		case POKE_EDIT_AUTH:
			Pk_EditAuth((EDIT_OP)lParam);
			break;

		case POKE_EDIT_EMAIL_ACC:
			Pk_EditEmailAcc((EDIT_OP)lParam);
			break;

		case POKE_EDIT_EMAIL_EXC:
			Pk_EditEmailExc((EDIT_OP)lParam);
			break;
			
		case POKE_EDIT_TRACKER:
			Pk_EditTracker(lParam);
			break;

		case SONORK_WIN_POKE_SET_TAB:
			SetCurrentTab((TAB)lParam, true , false );
			break;



		case SONORK_WIN_POKE_CHILD_NOTIFY:
			return OnChildDialogNotify((TSonorkChildDialogNotify*)lParam);

		case SONORK_WIN_POKE_CHILD_DRAW_ITEM:
			return OnChildDialogDraw((TSonorkChildDialogNotify*)lParam);


		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			OnTaskResult((SONORK_FUNCTION)lParam);
		break;
	}
	return 0;
}
void
 TSonorkMyInfoWin::OnTaskResult(SONORK_FUNCTION func)
{
	GLS_INDEX 	gls_status=GLS_NULL
		,	gls_task=GLS_TK_TASK;
	SKIN_HICON	hicon;

	SetCtrlEnabled( IDC_MYINFO_REFRESH, SonorkApp.CxReady() );
	SetCtrlEnabled( IDOK, true );
	page_win[TAB_TRACK]->SetCtrlEnabled( IDC_MINFO_TRACK_EDIT , true );
	page_win[TAB_TRACK]->SetCtrlEnabled( IDC_MINFO_TRACK_REFRESH , true );


	if( func == SONORK_FUNCTION_GET_USER_DATA
	||  func == SONORK_FUNCTION_SET_USER_DATA)
	{
		// Load info, regardless of result of task
		LoadInfo();
		LoadNotes();
	}
	switch( func )
	{
		case SONORK_FUNCTION_GET_USER_DATA:
			gls_task  =GLS_TK_GETINFO;
			gls_status= GLS_MS_RFRESHED;
			break;
			
		case SONORK_FUNCTION_SET_USER_DATA:
			gls_task  =GLS_TK_PUTINFO;
			gls_status= GLS_MS_SAVED;
			if(taskERR.Result() == SONORK_RESULT_OK )
			{
				Destroy(IDOK);
				return;
			}
			break;

		case SONORK_FUNCTION_SET_PASS:
			gls_task  =GLS_TK_PUTINFO;
			gls_status= GLS_MS_SAVED;

			if(taskERR.Result() == SONORK_RESULT_OK )
			{
				page_win[TAB_LOGIN]->SetCtrlText(IDC_MINFO_LOGIN_PASS,sPwd.CStr());
				SonorkApp.wProfilePassword().Set( sPwd );
				SonorkApp.SaveCurrentProfile( SONORK_APP_BASE_SPF_SAVE_ALL );
			}
			break;

		case SONORK_FUNCTION_LIST_TRACKERS:
			gls_task  =GLS_TK_GETINFO;
			gls_status= GLS_MS_RFRESHED;
			if(taskERR.Result() == SONORK_RESULT_OK )
			{
				SetWinUsrFlag(TRACKER_TREE_LOADED);
				LoadTrackerSubs();
				TSonorkUSearchWin::AfterTrackerTreeLoad(
					 track_tree
					,TestWinUsrFlag(TRACKER_LOADED_FROM_CACHE)
					,true);
			}
			break;
		default:
			break;
	}


	if(taskERR.Result() != SONORK_RESULT_OK )
	{
		hicon = SKIN_HICON_ERROR;
		gls_status=GLS_IF_TKERR;
	}
	else
		hicon = SKIN_HICON_INFO;

	TSonorkWin::SetStatus(status_bar,gls_status,hicon);
	if( taskERR.Result() != SONORK_RESULT_OK )
		TaskErrorBox(gls_task,&taskERR);
}

LRESULT
 TSonorkMyInfoWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if( cc_no_update )return 0;

	if( N->hdr.hwndFrom == tab_ctrl.hwnd && N->hdr.code == TCN_SELCHANGE )
	{
		SetCurrentTab((TAB)TabCtrl_GetCurSel(tab_ctrl.hwnd) , false , false );
	}
	return 0;
}

BOOL CALLBACK
 TSonorkMyInfoWin::CxItemsEnable( HWND hwnd, LPARAM lParam )
{
	TSonorkMyInfoWin*_this = (TSonorkMyInfoWin*)lParam;
	HWND 	parent_hwnd;
	UINT 	id;
	BOOL	enable;

	parent_hwnd = ::GetParent(hwnd);
	id =::GetWindowLong(hwnd,GWL_ID);
	if( parent_hwnd == _this->page_win[TAB_INFO]->Handle() )
	{
		switch(id)
		{
				case IDC_MINFO_ALIAS:
				case IDL_MINFO_ALIAS:
				case IDC_MINFO_EMAIL:
				case IDL_MINFO_EMAIL:
				case IDC_MINFO_NAME:
				case IDL_MINFO_NAME:
				case IDL_MINFO_BDATE:
				case IDL_MINFO_BDATE_FORMAT:
				case IDC_MINFO_BDATE_YR:
				case IDC_MINFO_BDATE_MO:
				case IDC_MINFO_BDATE_DA:
				case IDC_MINFO_TZ:
				case IDC_MINFO_TZ_UDN:
				case IDL_MINFO_COUNTRY:
				case IDC_MINFO_COUNTRY:
				case IDL_MINFO_SEX:
				case IDC_MINFO_SEX:
					enable=SonorkApp.CxReady();
				break;
				default:
					return true;
		}
	}
	else
	if( parent_hwnd == _this->page_win[TAB_AUTH]->Handle() )
	{
		switch(id)
		{
			case IDC_MINFO_AUTH_PUB_UINFO:
			case IDC_MINFO_AUTH_PUB_HIDE_EMAIL:
			case IDC_MINFO_AUTH_PUB_HIDE_NETADDR:
			case IDC_MINFO_AUTH_PUB_PIN:
			case IDG_MINFO_AUTH_PUB_PIN:
				enable=SonorkApp.CxReady();
			break;
			default:
				return true;
		}
	}
	else
	if( parent_hwnd == _this->page_win[TAB_PIC]->Handle() )
	{
		switch(id)
		{
			case IDC_MINFO_PIC_NOTES:
			case IDC_MINFO_PIC_NOTES_HELP:
				enable=SonorkApp.CxReady();
			break;
			default:
				return true;
		}
	}

	::EnableWindow( hwnd, enable );
	return true;
}

void
 TSonorkMyInfoWin::UpdateCxItems()
{
	static UINT track_cx_only[3]={IDC_MINFO_TRACK_LIST
		,IDC_MINFO_TRACK_EDIT
		,IDC_MINFO_TRACK_REFRESH};
	// Enable the controls,  we don't enable/disable the dialogs
	//  themselvs because the appearance of the controls don't change if we
	//  do that (they don't get grayed out)

	ShowCxOnlyMessageIfNotReady();

	EnumChildWindows(page_win[TAB_INFO]->Handle()
		, CxItemsEnable
		, (LPARAM)this);
	EnumChildWindows(page_win[TAB_PIC]->Handle()
		, CxItemsEnable
		, (LPARAM)this);
	EnumChildWindows(page_win[TAB_AUTH]->Handle()
		, CxItemsEnable
		, (LPARAM)this);
	SetCtrlEnabled( IDC_MYINFO_REFRESH
		, SonorkApp.CxReady()
		&& IsRefreshableTab(tab_ctrl.page)
		);
	if( SonorkApp.CxReady() && tab_ctrl.page==TAB_TRACK)
	{
		LoadTrackerTree( true );
	}
	for(int i=0;i<3;i++)
	{
		page_win[TAB_TRACK]->SetCtrlVisible( track_cx_only[i] , SonorkApp.CxReady() );
	}
	page_win[TAB_TRACK]->SetCtrlVisible( IDL_MINFO_TRACK_NOCX , !SonorkApp.CxReady() );

}

void
 TSonorkMyInfoWin::SetCurrentTab(TAB tb , bool update_tab, bool forced )
{
	int i;
	if( tb != tab_ctrl.page || forced)
	{
		// Update page
		if( tb<0 || tb>=TABS)return;
		tab_ctrl.page = tb;
		for(i=0;i<TABS;i++)
			page_win[i]->ShowWindow(i==tb?SW_SHOW:SW_HIDE);
		ShowCxOnlyMessageIfNotReady();
		if( update_tab )
			TabCtrl_SetCurSel(tab_ctrl.hwnd , tb );
		if(tb == TAB_AUTH)
		{
			if(!TestWinUsrFlag(AUTH_LIST_LOADED))
				LoadAuthList();
		}
		else
		if(tb== TAB_EMAIL)
		{
			if(!TestWinUsrFlag(EMAIL_LIST_LOADED))
				LoadEmailList();
		}
		else
		if(tb==TAB_TRACK)
		{
			SonorkApp.RequestSynchTrackers( false );
			LoadTrackerTree( true );
		}

		SetCtrlEnabled( IDC_MYINFO_REFRESH
			, SonorkApp.CxReady() && IsRefreshableTab(tab_ctrl.page)
		);

	}
}


// --------------------------------------------------------------------------
// TSonorkChgPwdWin
// --------------------------------------------------------------------------

TSonorkChgPwdWin::TSonorkChgPwdWin(TSonorkWin*parent)
:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_CHGPWD
	,0)
{}
bool 	TSonorkChgPwdWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		if( id == IDOK )
		{
			TSonorkShortString tmp;
			GetCtrlText(IDC_CHGPWD1,tmp);
			GetCtrlText(IDC_CHGPWD2,pass);
			if(tmp.Length()<2 || strcmp(tmp.CStr(),pass.CStr()) )
			{
				MessageBox(GLS_PWDHLP,GLS_OP_CHGPWD,MB_ICONSTOP|MB_OK);
				return true;
			}
		}
		if( id==IDOK||id==IDCANCEL )
			EndDialog(id);
	}
	return false;
}
bool
 TSonorkChgPwdWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if( S->CtlID == IDL_CHGPWD_SIGN )
	{
		sonork_skin.DrawSign(S->hDC,SKIN_SIGN_SECURITY,S->rcItem.left,S->rcItem.top);
		return true;
	}
	return false;
}

void	TSonorkChgPwdWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDL_CHGPWD1		,	GLS_LB_PWD	}
	,	{IDL_CHGPWD2		,	GLS_LB_PWD	}
	,	{IDL_CHGPWD_HELP	,	GLS_PWDHLP }
	,	{-1					,	GLS_OP_CHGPWD}
	,	{0					,	GLS_NULL	}
	};
	LoadLangEntries(gls_table,true);
	pass.Set(SonorkApp.ProfilePassword());
	SetCtrlText(IDC_CHGPWD1,pass.CStr());
	SetCtrlText(IDC_CHGPWD2,pass.CStr());
	SetEditCtrlMaxLength(IDC_CHGPWD1,12);
	SetEditCtrlMaxLength(IDC_CHGPWD2,12);
}


// --------------------------------------------------------------------------
// TSonorkEmailAccountWin
// --------------------------------------------------------------------------
#define ACCOUNT_STRINGS	8
static UINT	AccCtlStr[ACCOUNT_STRINGS][2]=
{
	{IDC_EMAILACC_ACC_NAME	,SONORK_EMAIL_ACCOUNT_STR_NAME}
,	{IDC_EMAILACC_REAL_NAME	,SONORK_EMAIL_ACCOUNT_STR_REAL_NAME}
,	{IDC_EMAILACC_RET_ADDR	,SONORK_EMAIL_ACCOUNT_STR_RETURN_ADDRESS}
,	{IDC_EMAILACC_LOGIN_NAME,SONORK_EMAIL_ACCOUNT_STR_LOGIN_NAME}
,	{IDC_EMAILACC_LOGIN_PASS,SONORK_EMAIL_ACCOUNT_STR_LOGIN_PASS}
,	{IDC_EMAILACC_I_SERVER	,SONORK_EMAIL_ACCOUNT_STR_INCOMMING_SERVER}
,	{IDC_EMAILACC_O_SERVER	,SONORK_EMAIL_ACCOUNT_STR_OUTGOING_SERVER}
,	{IDC_EMAILACC_UID		,SONORK_EMAIL_ACCOUNT_STR_LAST_UIDL}
};

TSonorkEmailAccountWin::TSonorkEmailAccountWin(TSonorkWin*parent,TSonorkEmailAccount* ptr)
:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_EMAILACC
	,0)
{
	acc=ptr;
}
void
 TSonorkEmailAccountWin::LoadLabels()
{
	char tmp[128];
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDG_EMAILACC_NAME	,	GLS_LB_NAME	}
	,	{IDG_EMAILACC_OUTGOING	,	GLS_EM_OUT	}
	,	{IDG_EMAILACC_INCOMMING	,	GLS_EM_IN	}
	,	{IDL_EMAILACC_REAL_NAME	,	GLS_LB_NAME	}
	,	{IDL_EMAILACC_RET_ADDR	,	GLS_EM_RPLADDR}
	,	{IDC_EMAILACC_CHECK_MAIL, 	GLS_EM_CHK	}
	,	{IDL_EMAILACC_LOGIN_NAME, 	GLS_LB_USR	}
	,	{IDL_EMAILACC_LOGIN_PASS, 	GLS_LB_PWD	}
	,	{IDL_EMAILACC_UID	, 	GLS_LB_LSTMSG}
	,	{IDC_EMAILACC_RESET	, 	GLS_OP_RESET}
	,	{IDC_EMAILACC_DEFAULT	,	GLS_EM_DEFLT}
	,	{-1						,	GLS_EM_ACC	}
	,	{0						,	GLS_NULL	}
	};
	LoadLangEntries(gls_table,true);
	wsprintf(tmp,"%s (%s)",SonorkApp.LangString(GLS_LB_SVR_HOST),"smtp");
	SetCtrlText( IDL_EMAILACC_O_SERVER , tmp);
	wsprintf(tmp,"%s (%s)",SonorkApp.LangString(GLS_LB_SVR_HOST),"pop3");
	SetCtrlText( IDL_EMAILACC_I_SERVER , tmp);


}

bool
	TSonorkEmailAccountWin::OnCommand(UINT id,HWND , UINT code)
{
	BOOL cm;
	if( code == BN_CLICKED )
	{
		if(id == IDC_EMAILACC_RESET)
		{
			acc->SetLastMsgNo(0);
			ClrCtrlText(IDC_EMAILACC_UID);
			return true;
		}
		cm = GetCtrlChecked(IDC_EMAILACC_CHECK_MAIL);

		if(id == IDC_EMAILACC_CHECK_MAIL)
		{

			EnableCheckMailItems( cm  , true);
			return true;
		}

		if(id == IDOK)
		{
			acc->header.flags=0;
			if( cm )
				acc->header.flags|=SONORK_EMAIL_ACCOUNT_F_CHECK_MAIL;

			if( GetCtrlChecked(IDC_EMAILACC_DEFAULT) )
				SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) = acc->UID();
			else
			if(SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) == acc->UID())
				SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) = 0;

			for(int i=0;i<ACCOUNT_STRINGS;i++)
				GetCtrlText(AccCtlStr[i][0],acc->str[AccCtlStr[i][1]]);
		}
		else
		if(id!=IDCANCEL)
			return false;

		EndDialog(id);
		return true;
	}
	return false;
}

bool
	TSonorkEmailAccountWin::OnCreate()
{
	BOOL cm;
	SetCaptionIcon( SKIN_HICON_EMAIL );
	for(int i=0;i<ACCOUNT_STRINGS;i++)
		SetCtrlText(AccCtlStr[i][0],acc->str[AccCtlStr[i][1]].CStr());
	SetCtrlChecked( IDC_EMAILACC_DEFAULT
		, SonorkApp.ProfileCtrlValue(SONORK_PCV_EMAIL_ACCOUNT_UID) == acc->UID());

	cm =  acc->header.flags &  SONORK_EMAIL_ACCOUNT_F_CHECK_MAIL;
	SetCtrlChecked(IDC_EMAILACC_CHECK_MAIL, cm);
	EnableCheckMailItems( cm , false);
	return true;
}
void
	TSonorkEmailAccountWin::EnableCheckMailItems( BOOL e , bool auto_complete)
{
#define CHECK_MAIL_ITEMS	9
	static UINT cm_item[CHECK_MAIL_ITEMS]=
	{
		IDL_EMAILACC_LOGIN_NAME
	,	IDC_EMAILACC_LOGIN_NAME
	,	IDL_EMAILACC_LOGIN_PASS
	,	IDC_EMAILACC_LOGIN_PASS
	,	IDL_EMAILACC_I_SERVER
	,	IDC_EMAILACC_I_SERVER
	,	IDL_EMAILACC_UID
	,	IDC_EMAILACC_UID
	,	IDC_EMAILACC_RESET
	};
	TSonorkShortString 	str;
	SONORK_C_STR		ptr;

	for(int i=0;i<CHECK_MAIL_ITEMS;i++)
		SetCtrlEnabled(cm_item[i] , e);

	if( e && auto_complete )
	{
		GetCtrlText(IDC_EMAILACC_I_SERVER , str );
		if(*str.CStr() == 0 )
		{
			GetCtrlText(IDC_EMAILACC_O_SERVER , str );
			SetCtrlText(IDC_EMAILACC_I_SERVER , str.CStr() );
		}
		GetCtrlText(IDC_EMAILACC_LOGIN_NAME , str );
		if(*str.CStr() == 0 )
		{
			GetCtrlText(IDC_EMAILACC_RET_ADDR , str );
			ptr = strchr(str.Buffer(),'@');
			if(ptr)
			{
				*ptr=0;
				SetCtrlText(IDC_EMAILACC_LOGIN_NAME, str.CStr() );
			}
		}
	}
}


// --------------------------------------------------------------------------
// TSonorkEmailExceptWin
// --------------------------------------------------------------------------

#define EMAIL_EXC_BOXES		3
#define EMAIL_EXC_FIELDS	4
#define EMAIL_EXC_OPS		3
struct EmailExceptField{
	SONORK_EMAIL_EXCEPT_FIELD	field;
	SONORK_C_CSTR				str;
};
struct EmailExceptOp{
	SONORK_EMAIL_EXCEPT_OP		op;
	GLS_INDEX				gls;
};
EmailExceptField email_exc_field[EMAIL_EXC_FIELDS]=
{
	{SONORK_EMAIL_EXCEPT_FIELD_NONE		, ""}
,	{SONORK_EMAIL_EXCEPT_FIELD_TO 		, "To"}
,	{SONORK_EMAIL_EXCEPT_FIELD_FROM 	, "From"}
,	{SONORK_EMAIL_EXCEPT_FIELD_SUBJECT  , "Subject"}
};

EmailExceptOp email_exc_op[EMAIL_EXC_OPS]=
{
	{SONORK_EMAIL_EXCEPT_OP_NONE		 	, GLS_NULL}
,	{SONORK_EMAIL_EXCEPT_OP_CONTAINS 		, GLS_LB_CNTNS}
,	{SONORK_EMAIL_EXCEPT_OP_NOT_CONTAINS 	, GLS_LB_NCNTNS}
};
static SONORK_C_CSTR	EmailExceptFieldStr(SONORK_EMAIL_EXCEPT_FIELD f);
static SONORK_C_CSTR	EmailExceptOpStr(SONORK_EMAIL_EXCEPT_OP o);

TSonorkEmailExceptWin::TSonorkEmailExceptWin(TSonorkWin*parent,TSonorkEmailExcept*ptr )
:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_EMAILEXC
	,0)
{
	exc=ptr;
}
void
 TSonorkEmailExceptWin::LoadLabels()
{
	SetCtrlText(IDL_EMAIL_EXC_NAME , GLS_LB_NAME);
	SetWindowText(GLS_LB_EXCEPTN);
}

bool
 TSonorkEmailExceptWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		if(id == IDOK)
		{
			SaveForm();
		}
		else
		if(id!=IDCANCEL)
			return false;

		EndDialog(id);
		return true;
	}
	return false;
}

bool
 TSonorkEmailExceptWin::OnCreate()
{
	int index,fno,cb_index;
	HWND hWnd;

	SetCaptionIcon( SKIN_HICON_EMAIL );

	for(index=0;index<EMAIL_EXC_BOXES;index++)
	{
		hWnd = GetDlgItem(IDC_EMAIL_EXC_FIELD_1 + index);
		for(fno=0
			;fno < EMAIL_EXC_FIELDS
			;fno++)
		{
			cb_index = ComboBox_AddString(hWnd,email_exc_field[fno].str);
			ComboBox_SetItemData(hWnd,cb_index ,email_exc_field[fno].field );
		}
		ComboBox_SetCurSel(hWnd,0);
		hWnd = GetDlgItem(IDC_EMAIL_EXC_OP_1 + index);
		for(fno=0
			;fno < EMAIL_EXC_OPS
			;fno++)
		{
			cb_index = ComboBox_AddString(hWnd,SonorkApp.LangString(email_exc_op[fno].gls));
			ComboBox_SetItemData(hWnd,cb_index ,email_exc_op[fno].op );
		}
		ComboBox_SetCurSel(hWnd,0);
	}
	LoadForm();
	return true;
}

void
 TSonorkEmailExceptWin::LoadForm()
{
	int index,cb_index;
	TSonorkEmailExceptEntry *E;
	TSonorkListIterator		IT;
	HWND				hWnd;

	SetCtrlText(IDC_EMAIL_EXC_NAME , exc->name.CStr());
	exc->queue.BeginEnum(IT);
	for(index=0
		;(E=exc->queue.EnumNext(IT))!=NULL
		&& index<EMAIL_EXC_BOXES
		;index++)
	{
		if(E->ExceptField() != SONORK_EMAIL_EXCEPT_FIELD_NONE
		&& E->ExceptOp() != SONORK_EMAIL_EXCEPT_OP_NONE
		&& E->string.Length() )
		{
			hWnd = GetDlgItem(IDC_EMAIL_EXC_FIELD_1 + index);
			cb_index = ComboBox_FindStringExact(hWnd
					,-1
					,EmailExceptFieldStr(E->ExceptField()) );
			ComboBox_SetCurSel( hWnd, cb_index);

			hWnd = GetDlgItem(IDC_EMAIL_EXC_OP_1 + index);
			cb_index = ComboBox_FindStringExact(hWnd
					,-1
					,EmailExceptOpStr(E->ExceptOp()) );
			ComboBox_SetCurSel( hWnd, cb_index);

			SetCtrlText( IDC_EMAIL_EXC_STR_1 + index
				, E->string.CStr());
		}
		else
			E->Clear();
	}
	exc->queue.EndEnum(IT);
}
void
	TSonorkEmailExceptWin::SaveForm()
{
	int index,cb_index;
	SONORK_EMAIL_EXCEPT_FIELD	f;
	SONORK_EMAIL_EXCEPT_OP		op;
	TSonorkShortString			str;
	HWND					hWnd;
	TSonorkEmailExceptEntry *E;

	GetCtrlText(IDC_EMAIL_EXC_NAME , exc->name);
	exc->queue.Clear();
	for(index=0
		;index<EMAIL_EXC_BOXES
		;index++)
	{
		hWnd = GetDlgItem(IDC_EMAIL_EXC_FIELD_1 + index);
		cb_index=ComboBox_GetCurSel( hWnd );
		if(cb_index == CB_ERR)continue;
		f = (SONORK_EMAIL_EXCEPT_FIELD)ComboBox_GetItemData(hWnd, cb_index);
		if( f == SONORK_EMAIL_EXCEPT_FIELD_NONE)continue;

		hWnd = GetDlgItem(IDC_EMAIL_EXC_OP_1 + index);
		cb_index=ComboBox_GetCurSel( hWnd );
		if(cb_index == CB_ERR)continue;
		op = (SONORK_EMAIL_EXCEPT_OP)ComboBox_GetItemData(hWnd, cb_index);
		if( op == SONORK_EMAIL_EXCEPT_OP_NONE)continue;

		GetCtrlText(IDC_EMAIL_EXC_STR_1 + index , str );
		if( str.Length() == 0)continue;
		E = new TSonorkEmailExceptEntry;
		E->Clear();
		E->SetExceptField( f );
		E->SetExceptOp( op );
		E->string.Set( str );
		exc->queue.Add( E );
	}

}


static SONORK_C_CSTR	EmailExceptFieldStr(SONORK_EMAIL_EXCEPT_FIELD f)
{
	for(int i=0;i<EMAIL_EXC_FIELDS;i++)
	{
		if(email_exc_field[i].field == f)
			return email_exc_field[i].str;
	}
	return "";
}

static SONORK_C_CSTR	EmailExceptOpStr(SONORK_EMAIL_EXCEPT_OP o)
{
	for(int i=0;i<EMAIL_EXC_OPS;i++)
	{
		if(email_exc_op[i].op == o)
			return SonorkApp.LangString(email_exc_op[i].gls);
	}
	return  "";
}

// --------------------------------------------------------------------------
// PREFERENCES
void LoadCombo(HWND combo, GLS_INDEX*ptr,UINT items,UINT selected)
{
	ComboBox_Clear(combo);
	while(items--)
	{
		ComboBox_AddString(combo,SonorkApp.LangString(*ptr));
		ptr++;
	}
	ComboBox_SetCurSel(combo,selected);
}
void
 TSonorkMyInfoWin::OnPrefItemSelected()
{
#define PREF_SHOW_NONE		-1
#define PREF_SHOW_FLAGS		0
#define PREF_SHOW_TEXT		1
#define PREF_SHOW_COMBO		2
	UINT			pref_val;
	int			pref_show;
	TSonorkWin*		W;
	MINFO_PREFERENCE	new_pref_item;
	static GLS_INDEX gls_start_mode[3]=
		{GLS_OP_ASK,GLS_IM_INTRA,GLS_IM_EXTRA};
	static GLS_INDEX gls_slider_pos[5]=
		{GLS_LB_DIS,GLS_LBP_TL,GLS_LBP_TR,GLS_LBP_BR,GLS_LBP_BL};


	if( cc_no_update )
		return;
		
	cc_no_update++;

	prefs_list.GetSelectedItem( &pref_show );
	if(pref_show!=-1)
		new_pref_item=(MINFO_PREFERENCE)pref_show;
	else
		new_pref_item=MINFO_PREF_INVALID;



	W=page_win[TAB_PREFS];

	if(new_pref_item!=MINFO_PREF_INVALID)
	{
		W->SetCtrlText( IDG_MINFO_PREFS_LABEL,GetPreferenceLabel(new_pref_item));
		pref_val = GetPreferenceSetting(new_pref_item);

		switch(new_pref_item)
		{

//			case MINFO_PREF_AUTO_AWAY_MINS:
//				pref_show=PREF_SHOW_TEXT;
//				W->SetCtrlUint(IDC_MINFO_PREFS_TEXT,pref_val);
//			break;
			case MINFO_PREF_QUERY_START_MODE:
				pref_show=PREF_SHOW_COMBO;
				LoadCombo(prefs_combo,gls_start_mode,3,pref_val);
			break;

			case MINFO_PREF_SLIDER_POS:
				pref_show=PREF_SHOW_COMBO;
				LoadCombo(prefs_combo,gls_slider_pos,5,pref_val);
			break;

			default:
				W->SetCtrlChecked(IDC_MINFO_PREFS_ENABLED , pref_val!=0);
				W->SetCtrlChecked(IDC_MINFO_PREFS_DISABLED ,pref_val==0);
				pref_show=PREF_SHOW_FLAGS;
			break;


		}
	}
	else
		pref_show=PREF_SHOW_NONE;

	W->SetCtrlVisible(IDC_MINFO_PREFS_TEXT	   , pref_show==PREF_SHOW_TEXT);
	W->SetCtrlVisible(IDC_MINFO_PREFS_COMBO	   , pref_show==PREF_SHOW_COMBO);
	W->SetCtrlVisible(IDC_MINFO_PREFS_ENABLED  , pref_show==PREF_SHOW_FLAGS);
	W->SetCtrlVisible(IDC_MINFO_PREFS_DISABLED , pref_show==PREF_SHOW_FLAGS);

	cur_pref_item=new_pref_item;
	cc_no_update--;

}
void
 TSonorkMyInfoWin::SetPrefListItemFlag(MINFO_PREFERENCE mp,BOOL bBool)
{
	UINT state;
	if( bBool )
		state = INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_CHECKED+1);
	else
		state = INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_UNCHECKED+1);
	prefs_list.SetItemState( mp , state , LVIS_STATEIMAGEMASK );
}

void
 TSonorkMyInfoWin::OnPrefItemToggle()
{
	DWORD value;
	switch(cur_pref_item)
	{
		case MINFO_PREF_INVALID:
//		case MINFO_PREF_AUTO_AWAY_MINS:
		case MINFO_PREF_QUERY_START_MODE:
			return;
	}
	value = page_win[TAB_PREFS]->GetCtrlChecked(IDC_MINFO_PREFS_ENABLED );
	page_win[TAB_PREFS]->SetCtrlChecked(IDC_MINFO_PREFS_ENABLED , !value);
	page_win[TAB_PREFS]->SetCtrlChecked(IDC_MINFO_PREFS_DISABLED, value);
	OnPrefItemFlagSelected();
}
void
 TSonorkMyInfoWin::OnPrefItemFlagSelected()
{
	DWORD value;


	switch(cur_pref_item)
	{
		case MINFO_PREF_INVALID:
//		case MINFO_PREF_AUTO_AWAY_MINS:
		case MINFO_PREF_QUERY_START_MODE:
			return;
	}
	value = page_win[TAB_PREFS]->GetCtrlChecked(IDC_MINFO_PREFS_ENABLED );
	value = SetPreferenceSetting(cur_pref_item , value);
	SetPrefListItemFlag(cur_pref_item,value);

}

void
 TSonorkMyInfoWin::OnPrefItemComboSelected()
{
	UINT			value;

	value=ComboBox_GetCurSel(prefs_combo);
	switch(cur_pref_item)
	{
		case MINFO_PREF_QUERY_START_MODE:
			if(value>=SONORK_APP_START_MODES)
				value=SONORK_APP_START_MODE_ASK;
			break;
		case MINFO_PREF_SLIDER_POS:
		break;

		default:
			return;
	}

	cc_no_update++;

	value = SetPreferenceSetting(cur_pref_item , value);
	SetPrefListItemFlag(cur_pref_item,value);

	cc_no_update--;

}
void
 TSonorkMyInfoWin::OnPrefItemTextKillFocus()
{
/*
	UINT			value;


	switch(cur_pref_item)
	{
		case MINFO_PREF_AUTO_AWAY_MINS:
			break;
		default:
			return;
	}
	value=page_win[TAB_PREFS]->GetCtrlUint(IDC_MINFO_PREFS_TEXT);
	if(value>=60)
		value=60;

	SetWinUsrFlag(SONORK_WIN_F_UPDATING);

	value = SetPreferenceSetting(cur_pref_item , value);
	SetPrefListItemFlag(cur_pref_item,value);

	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);
*/

}


enum MINFO_PREF_FLAG
{
	MINFO_PREF_F_REVERSED	= 	0x00001
};
struct MinfoPrefEntry
{
	GLS_INDEX	gls;
	UINT		code;
	UINT		flags;
};
static MinfoPrefEntry pref_entry[MINFO_PREFERENCES]=
{
  {GLS_OP_HIDEMIN
	, SONORK_PCF_NO_HIDE_ON_MINIMIZE
	, MINFO_PREF_F_REVERSED}
, {GLS_OP_SHTTIPS
	, SONORK_PCF_NO_TOOL_TIPS
	, MINFO_PREF_F_REVERSED}
, {GLS_OP_RE_SND
	, SONORK_PCF_NO_MSG_REMINDER
	, MINFO_PREF_F_REVERSED}
, {GLS_OP_BLNKTRAY
	, SONORK_PCF_NO_TRAY_BLINK
	, MINFO_PREF_F_REVERSED}
, {GLS_OP_SNDWENTER
	, SONORK_PCF_SEND_WITH_ENTER
	, 0}
, {GLS_OP_ASHOWRMSG
	, SONORK_PCF_POPUP_MSG_WIN
	, 0}
, {GLS_OP_AUTOAUTH
	, SONORK_PCF_AUTO_AUTH
	, 0}
, {GLS_OP_ASHSIDMSG
	, SONORK_PCF_NO_AUTO_SHOW_SID_MSG
	, MINFO_PREF_F_REVERSED}
, {GLS_LB_INITWW
	, 0
	, 0}
, {GLS_LB_INITMD
	, 0
	, 0}
//, {GLS_OP_AAWAY
//	, 0
//	, 0}
, {GLS_OP_SHSLIDER
	, 0
	, 0}
, {GLS_OP_ACMPRS
	, SONORK_PCF_NO_COMPRESS_FILES
	, MINFO_PREF_F_REVERSED}
, {GLS_OP_CNFRMFILSND
	, SONORK_PCF_CONFIRM_FILE_SEND
	, 0}
, {GLS_OP_PCSPKR
	, 0
	, 0}
, { GLS_OP_MUT_SND
	, SONORK_PCF_MUTE_SOUND
	, 0}

};

SONORK_C_CSTR
 TSonorkMyInfoWin::GetPreferenceLabel(MINFO_PREFERENCE ei)
{
	return SonorkApp.LangString(pref_entry[ei].gls);
}
DWORD
 TSonorkMyInfoWin::SetPreferenceSetting(MINFO_PREFERENCE ei,DWORD value)
{
	DWORD			aux;
	TSonorkRegKey 		regKEY;
	TSonorkTempBuffer buffer(SONORK_MAX_PATH);
	MinfoPrefEntry*E;

	assert(ei>=0 && ei<MINFO_PREFERENCES);
	E=pref_entry+ei;
	switch(ei)
	{
		case MINFO_PREF_HIDE_ON_MINIMIZE:
		case MINFO_PREF_SHOW_TOOL_TIPS:
		case MINFO_PREF_MSG_REMINDER:
		case MINFO_PREF_BLINK_TRAY_ICON:
		case MINFO_PREF_SEND_WITH_ENTER:
		case MINFO_PREF_POPUP_MSG_WIN:
		case MINFO_PREF_AUTO_AUTH:
		case MINFO_PREF_COMPRESS_FILES:
		case MINFO_PREF_CONFIRM_FILE_SEND:
		case MINFO_PREF_SHOW_SID_MSG:
		case MINFO_PREF_MUTE_SOUND:
		aux=value;
		if( E->flags & MINFO_PREF_F_REVERSED )
			aux = !aux;
		SonorkApp.ProfileCtrlFlags().SetClear( E->code , aux);
		break;

		case MINFO_PREF_START_WITH_WINDOWS:

		for(;;)
		{
			if(regKEY.Open(HKEY_LOCAL_MACHINE
				,szWinRunKey
				,false
				,KEY_ALL_ACCESS )==ERROR_SUCCESS)
			{
				if( value )
				{
					SonorkApp.GetDirPath(buffer.CStr()
						,SONORK_APP_DIR_ROOT
						,szSrkClientImage);
					strcat(buffer.CStr()," -auto");
					if( regKEY.SetValue(szSONORK
						,buffer.CStr()) == ERROR_SUCCESS)
					{
						break;
					}
				}
				else
				{
					if(regKEY.DelValue(szSONORK)==ERROR_SUCCESS)
					{
						break;
					}
				}
				regKEY.Close();

			}
			value=GetPreferenceSetting(MINFO_PREF_START_WITH_WINDOWS);
			break;
		}
		break;

		case MINFO_PREF_QUERY_START_MODE:
			for(;;)
			{
				if(regKEY.Open(HKEY_CURRENT_USER
					,szSrkClientRegKeyRoot
					,false
					,KEY_ALL_ACCESS)==ERROR_SUCCESS)
				{
					if(regKEY.SetValue(szStartMode,value)==ERROR_SUCCESS)
						break;
					regKEY.Close();
				}
				// Failed to set the setting, read again
				value = GetPreferenceSetting(MINFO_PREF_QUERY_START_MODE);
				break;
			}
		break;

//		case MINFO_PREF_AUTO_AWAY_MINS:
//			SonorkApp.ProfileCtrlValue(SONORK_PCV_AUTO_AWAY_MINS)=value;
//		break;

		case MINFO_PREF_SLIDER_POS:
			SonorkApp.ProfileCtrlValue(SONORK_PCV_SLIDER_POS)=value;
			SonorkApp.UI_SlideWin()->SetEvent(
			value>0?SONORK_UI_EVENT_SYS_MSG:SONORK_UI_EVENT_NONE
				, SonorkApp.LangString(GLS_OP_SHSLIDER)
				, SKIN_ICON_SONORK
				, 0x800000
				, 0x00ffff
				, 3000);
		break;

		case MINFO_PREF_PC_SPEAKER:
			SonorkApp.AppCtrlFlags().SetClear( SONORK_ACF_USE_PC_SPEAKER , value );
		break;

		default:
			return 0;

	}
	return value;
}

DWORD
 TSonorkMyInfoWin::GetPreferenceSetting(MINFO_PREFERENCE ei)
{
	DWORD		rv;
	DWORD		aux;
	TSonorkRegKey 	regKEY;
	MinfoPrefEntry*E;

	assert(ei>=0 && ei<MINFO_PREFERENCES);
	E=pref_entry+ei;

	switch(ei)
	{
		case MINFO_PREF_HIDE_ON_MINIMIZE:
		case MINFO_PREF_SHOW_TOOL_TIPS:
		case MINFO_PREF_MSG_REMINDER:
		case MINFO_PREF_BLINK_TRAY_ICON:
		case MINFO_PREF_SEND_WITH_ENTER:
		case MINFO_PREF_POPUP_MSG_WIN:
		case MINFO_PREF_AUTO_AUTH:
		case MINFO_PREF_COMPRESS_FILES:
		case MINFO_PREF_CONFIRM_FILE_SEND:
		case MINFO_PREF_SHOW_SID_MSG:
		case MINFO_PREF_MUTE_SOUND:
		rv = SonorkApp.ProfileCtrlFlags().Test( E->code );
		if( E->flags & MINFO_PREF_F_REVERSED )
			return !rv;
		break;

		case MINFO_PREF_START_WITH_WINDOWS:
			if(regKEY.Open(HKEY_LOCAL_MACHINE
				,szWinRunKey
				,false
				,KEY_READ )==ERROR_SUCCESS)
			{
				if( regKEY.QueryValue(szSONORK,NULL,0) == ERROR_SUCCESS)
				{
					rv=1;
					break;
				}
			}
			rv=0;
		break;

		case MINFO_PREF_QUERY_START_MODE:
			if(regKEY.Open(HKEY_CURRENT_USER
				,szSrkClientRegKeyRoot
				,false
				,KEY_READ)==ERROR_SUCCESS)
			{
				aux=regKEY.QueryValue(szStartMode,&rv);
				if( aux==ERROR_SUCCESS )
					break;
			}
			rv= SONORK_APP_START_MODE_ASK;
		break;
//		case MINFO_PREF_AUTO_AWAY_MINS:
//			rv=SonorkApp.ProfileCtrlValue(SONORK_PCV_AUTO_AWAY_MINS);
//		break;
			//_GLS(OP_PCSPKR)

		case MINFO_PREF_SLIDER_POS:
			rv = SonorkApp.ProfileCtrlValue(SONORK_PCV_SLIDER_POS);
		break;

		case MINFO_PREF_PC_SPEAKER:
			rv = SonorkApp.AppCtrlFlags().Test( SONORK_ACF_USE_PC_SPEAKER );
		break;


		default:
		rv = 0;
		break;

	}
	return rv;
}

// ---------------------------------------------------------------------------

TSonorkTrackerSubsWin::TSonorkTrackerSubsWin(TSonorkWin*parent
		, TSonorkTrackerRoom* proom
		, TSonorkTrackerData* pdata
		)
		:TSonorkWin(parent
		,SONORK_WIN_CLASS_NORMAL
		|SONORK_WIN_TYPE_NONE
		|SONORK_WIN_DIALOG
		|IDD_TRKSUBS
		,0)
{
	room=proom;
	data=pdata;
}

// ---------------------------------------------------------------------------

bool
 TSonorkTrackerSubsWin::OnCreate()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{-1
		,	GLS_TR_ROOM	}
	,	{IDG_TRKSUBS_ROOM	|SONORK_WIN_CTF_BOLD
		,	GLS_NULL	}
	,	{IDG_TRKSUBS_SUBS	|SONORK_WIN_CTF_BOLD
		,	GLS_LB_MYINFO	}
	,	{IDC_TRKSUBS_SUBS_ON
		,	GLS_TR_ACT	}
	,	{IDG_TRKSUBS_SUBS_TEXT  |SONORK_WIN_CTF_BOLD
		,	GLS_TR_MYTXT	}
	,	{IDG_TRKSUBS_SUBS_LEVEL	|SONORK_WIN_CTF_BOLD
		,	GLS_TR_MYHLP	}
	,	{IDC_TRKSUBS_DEL
		,	GLS_OP_DEL	}
	,	{0,GLS_NULL}};

	SetWinUsrFlag(SONORK_WIN_F_UPDATING);

	LoadLangEntries( gls_table, true );
	SetCtrlText(IDG_TRKSUBS_ROOM,room->name.CStr() );
	SetCtrlText(IDC_TRKSUBS_ROOM_TEXT,room->text.CStr() );
	SetCtrlText(IDC_TRKSUBS_SUBS_TEXT,data->text.CStr() );
	SetCtrlChecked(IDC_TRKSUBS_SUBS_ON
		,data->IsActive());
	help_cb=GetDlgItem(IDC_TRKSUBS_SUBS_LEVEL);

	if( (app_detector=room->IsAppDetector()) != false)
	{
		SetCtrlChecked(IDC_TRKSUBS_SUBS_AUTO
			, data->IsAppDetector());
	}
	SetCtrlEnabled(IDC_TRKSUBS_SUBS_AUTO, app_detector );
	for(int i=0;i<=5;i++)
	{
		ComboBox_AddString(help_cb
			, SonorkApp.LangString((GLS_INDEX)(GLS_TR_L0+i)));
	}
	ComboBox_SetCurSel(help_cb,data->HelpLevel());
	ClearWinUsrFlag(SONORK_WIN_F_UPDATING);

	return true;
}

// ---------------------------------------------------------------------------

bool
 TSonorkTrackerSubsWin::CmdSave()
{
	DWORD help_level;
	help_level=ComboBox_GetCurSel(help_cb);
	if(help_level>5)
		help_level=0;
	data->SetHelpLevel(help_level);
	data->header.flags.Clear(SONORK_TRACKER_DATA_F0_ACTIVE);
	data->header.flags.Clear(SONORK_TRACKER_DATA_F1_APP_DETECTOR);

	GetCtrlText(IDC_TRKSUBS_SUBS_TEXT , data->text );

	if(GetCtrlChecked(IDC_TRKSUBS_SUBS_ON))
	{
		if(data->text.Length() < 2 )
		{
			MessageBox(GLS_TR_NOTXT
				,GLS_TR_ROOM
				,MB_ICONSTOP|MB_OK);
			return false;
		}
		data->header.flags.Set(SONORK_TRACKER_DATA_F0_ACTIVE);
	}

	if( app_detector)
	if(GetCtrlChecked(IDC_TRKSUBS_SUBS_AUTO) )
		data->header.flags.Set(SONORK_TRACKER_DATA_F1_APP_DETECTOR);
	return true;
}

// ---------------------------------------------------------------------------

bool
 TSonorkTrackerSubsWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		switch(id)
		{
			case IDOK:
				if(!CmdSave())
					return true;

			case IDCANCEL:
			case IDC_TRKSUBS_DEL:
				EndDialog(id);
				return true;

		}
		return true;

	}
	return false;
}

