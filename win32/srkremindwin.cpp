#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkremindwin.h"
#include "srk_codec_file.h"
#include "srk_task_atom.h"

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

#define REMIND_LIST_MOVING_VIEW_ITEM	SONORK_WIN_F_USER_01
#define REMIND_LIST_DATA_UNSAVED	SONORK_WIN_F_USER_02
#define REMIND_LIST_LOADING		SONORK_WIN_F_USER_03
#define REMIND_LIST_IGNORE_EXPAND	SONORK_WIN_F_USER_04

#define REMIND_LIST_POKE_OPEN_ITEM	SONORK_WIN_POKE_01
#define REMIND_LIST_POKE_OPEN_DATE	SONORK_WIN_POKE_02

#define REMIND_EDIT_LIST_DISABLED	SONORK_WIN_F_USER_01
#define REMIND_EDIT_EXISTING		SONORK_WIN_F_USER_02

#define REMIND_CONSOLE_TEXT_SIZE	480
#define REMIND_CONSOLE_CACHE_SIZE	112
#define TOOL_BAR_ID			500
#define TOOL_BAR_BUTTONS		(TOOL_BAR_BUTTON_LIMIT - TOOL_BAR_BUTTON_BASE)

enum TOOL_BAR_BUTTON
{
	TOOL_BAR_BUTTON_BASE
,	TOOL_BAR_BUTTON_ADD		= TOOL_BAR_BUTTON_BASE
,	TOOL_BAR_BUTTON_PIN
, 	TOOL_BAR_BUTTON_LIMIT
};


struct TSonorkRemindViewItem
:public TSonorkTreeItem
{
	union{
		TSonorkTime	 	time;
		TSonorkRemindData*	remind;
	}data;
	int		boldChildren;

	TSonorkRemindViewItem( const TSonorkTime& time );
	TSonorkRemindViewItem( TSonorkRemindData*);
	~TSonorkRemindViewItem();

	TSonorkTime&
		Time()
		{ return data.time;}
		
	DWORD
		SearchId() const
		{ return 0;}

	TSonorkRemindData*
		RemindData()
		{ return data.remind;}

	SONORK_C_CSTR
		GetLabel(BOOL expanded,SKIN_ICON&) const;

	int
		GetBoldValue() const;
		
	void
		IncBoldCount(int dx)
		{
			boldChildren+=dx;
		}
		
	DWORD	GetStateFlags() const
		{ return GetBoldValue()?TVIS_BOLD:0; }
};




// ---------------------------------------------------------------------------
// TSonorkRemindAlarmWin

TSonorkRemindAlarmWin::TSonorkRemindAlarmWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_ALARM
	,SONORK_WIN_SF_NO_WIN_PARENT
	)
{
}
bool
 TSonorkRemindAlarmWin::OnCommand(UINT id,HWND , UINT code)
{

	if( code == BN_CLICKED )
	{
		if( id == IDC_ALARM_PENDING )
		{
			code=GetCtrlChecked(IDC_ALARM_PENDING);
			SetCtrlEnabled(IDL_ALARM_POSTPONE,code);
			SetCtrlEnabled(IDC_ALARM_MINUTES,code);
			SetCtrlEnabled(IDL_ALARM_MINUTES,code);
		}
		else
		if( id == IDOK )
		{
			code = GetCtrlUint(IDC_ALARM_MINUTES);
			header.flags&=~(SONORK_REMIND_F_ALARM|SONORK_REMIND_F_PENDING);

			if( GetCtrlChecked(IDC_ALARM_PENDING) )
			{
				header.flags|=SONORK_REMIND_F_PENDING;

				if( code != 0 )
				{
					header.flags|=SONORK_REMIND_F_ALARM;
					header.alarm_time.Set( SonorkApp.CurrentTime() );
					header.SetAlarmDelayMins( code );
				}
			}
			SonorkApp.SaveAlarmItemHeader(&header,false);
			Destroy(id);
			return true;
		}
	}
	return false;
}
LRESULT
 TSonorkRemindAlarmWin::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg   == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORSTATIC)
	if( lParam == (LPARAM)GetDlgItem(IDC_ALARM_MINUTES)
	||  lParam == (LPARAM)GetDlgItem(IDC_ALARM_NOTES))
		return sonork_skin.OnCtlColorMsgView(wParam);
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);
}
void TSonorkRemindAlarmWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDL_ALARM_POSTPONE	, GLS_OP_PPONE	}
	,	{IDL_ALARM_MINUTES	, GLS_LB_MINS	}
	,	{IDC_ALARM_PENDING	, GLS_LB_PENDING	}
	,	{0			, GLS_NULL   	}
	};
	LoadLangEntries(gls_table,true);
}

bool
 TSonorkRemindAlarmWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if(S->CtlID == IDC_ALARM_ICON)
	{
		sonork_skin.DrawSign(S->hDC, SKIN_SIGN_WORKING, 0,0,ILD_NORMAL);
		return true;
	}
	return false;
}
bool
 TSonorkRemindAlarmWin::OnCreate()
{
	static char *mins[]={"1","5","10","15","30","45","60",NULL};
	int i;
	SonorkApp.OnSysDialogRun(this
		,SONORK_SYS_DIALOG_REMIND_ALARM
		,true
		,NULL);
	SonorkApp.SetRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS );
	SetCtrlSize(IDC_ALARM_ICON,SKIN_SIGN_SW,SKIN_SIGN_SH);
	SetCtrlChecked(IDC_ALARM_PENDING,true);
	minHwnd = GetDlgItem( IDC_ALARM_MINUTES );
	for(i=0;mins[i]!=NULL;i++)
		ComboBox_AddString(minHwnd, mins[i]);
	ComboBox_SetCurSel(minHwnd,-1);
	return true;
}
void
 TSonorkRemindAlarmWin::OnBeforeDestroy()
{
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_REMIND_ALARM,false,NULL);
	SonorkApp.ClearRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS );
	SonorkApp.RequestCheckAlarms();
}


// ---------------------------------------------------------------------------
// TSonorkRemindListWin

bool
 TSonorkRemindListWin::AddItem(const TSonorkRemindData* pRD)
{
	TSonorkRemindViewItem*	VI;
	TSonorkRemindData*	nRD;

	if( !TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
	{
		VI = GetRemindItem( TVI_ROOT, &pRD->header );
		if( VI == NULL )
		{
			nRD = new TSonorkRemindData;
			nRD->CODEC_Copy(pRD);
			AddRemindItem( nRD );
		}
		else
		{
			SetWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
			view.tree.DelItem( VI , false );
			VI->RemindData()->CODEC_Copy(pRD);
			AddRemindItem( VI );
			ClearWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
		}
		return true;
	}
	return false;

}

bool
 TSonorkRemindListWin::SetItemHeader(const TSonorkRemindDataHeader*HDR, bool delete_it)
{
	TSonorkRemindViewItem*	VI;

	if( !TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
	{
		VI = GetRemindItem( TVI_ROOT, HDR );
		if( VI != NULL )
		{
			if( delete_it )
			{
				view.tree.DelItem( VI , false );
			}
			else
			{
				SetWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
				view.tree.DelItem( VI , false );
				memcpy(&VI->RemindData()->header
					,HDR
					,sizeof(TSonorkRemindDataHeader));
				AddRemindItem( VI );
				ClearWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
			}
			return true;
		}
	}
	return false;

}

bool
 TSonorkRemindListWin::MayCheckAlarms()	// Returns true if should be invoked soon again
{
	if( SonorkApp.TestRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS ) )
		return false;


	if( TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
		return false;

	if(TestWinUsrFlag(REMIND_LIST_DATA_UNSAVED))
		DL_Save();
	return true;
}

void
 TSonorkRemindListWin::CmAddItem(const TSonorkTime* pTime)
{
	TSonorkRemindViewItem*	VI;
	TSonorkRemindData *RD;

	RD=new TSonorkRemindData;
	RD->Clear();
	RD->header.owner.Set(SonorkApp.ProfileUserId());
	RD->header.key.v[0] = SonorkApp.GenTrackingNo(SonorkApp.ProfileUserId());
	if(pTime)
		RD->wRemindTime().Set( *pTime );
	else
		RD->wRemindTime().SetTime_Local();

	if(TSonorkRemindWin(this,RD,true).Execute() != IDOK)
	{
		delete RD;
	}
	else
	{
		VI = AddRemindItem( RD );
		view.tree.SelectItem( VI );
	}

}
/*
void
 TSonorkRemindListWin::UpdateAfterEditItem(TSonorkRemindViewItem*VI)
{
	HTREEITEM 	oParent,nParent;

	oParent = view.tree.GetParent(VI);

	SetWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
	view.tree.DelItem(VI);
	AddRemindItem(VI);
	ClearWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
	newBoldCount = VI->GetBoldCount;

	nParent = view.tree.GetParent(VI);
	if(view.tree.GetChild( oParent ) == NULL)
	{
		view.tree.DelItem( oParent );
		view.tree.ExpandItemLevels(nParent,TVE_EXPAND,0);
	}
}
*/
void
 TSonorkRemindListWin::CmEditItem(TSonorkRemindViewItem*VI)
{
	UINT	id;
	TSonorkRemindData	tmp;
	if(VI==NULL)return;
	if(VI->Type() == REMIND_VIEW_ITEM_DATA)
	{
		tmp.CODEC_Copy( VI->RemindData() );
		id = TSonorkRemindWin(this
			,&tmp
			,false).Execute();
		if( id ==IDOK )
		{
			SetWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
			view.tree.DelItem( VI , false );
			VI->RemindData()->CODEC_Copy(&tmp);
			AddRemindItem( VI );
			ClearWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM);
			view.tree.SelectItem(VI);
		}
		else
		if(id==IDC_REMIND_DEL)
		{
			view.tree.DelItem(VI , false );
		}
	}
	else
	if(VI->Type() == REMIND_VIEW_ITEM_TIME)
	{
		CmAddItem( &VI->Time() );
	}
}


TSonorkRemindViewItem*
 TSonorkRemindListWin::AddRemindItem(TSonorkRemindData* RD)
{
	TSonorkRemindViewItem* nItem;
	nItem = new TSonorkRemindViewItem( RD );
	AddRemindItem( nItem );
	return nItem;
}

void
 TSonorkRemindListWin::AddRemindItem(TSonorkRemindViewItem*VI)
{
	TSonorkRemindViewItem* pItem;
	assert( VI->Type() == REMIND_VIEW_ITEM_DATA );
	pItem = GetDateItem( VI->RemindData()->RemindTime() );
	if( pItem == NULL )
	{
		pItem = AddTimeItem( VI->RemindData()->RemindTime() );
	}
	_AddViewItem(pItem,VI);


}

TSonorkRemindViewItem*
 TSonorkRemindListWin::AddTimeItem( const TSonorkTime& time )
{
	TSonorkRemindViewItem* nItem;
	nItem = new TSonorkRemindViewItem( time );
	_AddViewItem( NULL , nItem );
	return nItem;
}

void
 TSonorkRemindListWin::_AddViewItem(TSonorkRemindViewItem*parent,TSonorkRemindViewItem*item)
{
	view.tree.AddItem( parent , item , false );
	SetWinUsrFlag(REMIND_LIST_DATA_UNSAVED);
}

TSonorkRemindViewItem*
 TSonorkRemindListWin::GetDateItem(const TSonorkTime& time )
{
	TSonorkRemindViewItem*	VI;
	for( VI = (TSonorkRemindViewItem*)view.tree.GetChild( TVI_ROOT)
	;    VI != NULL
	;    VI = (TSonorkRemindViewItem*)view.tree.GetNextSibling(VI) )
	{
		if( VI->Type()==REMIND_VIEW_ITEM_TIME )
		{
			if( VI->Time().RelDate( time ) == SONORK_TIME_EQUAL)
				return VI;
		}
	}
	return NULL;
}
TSonorkRemindViewItem*
 TSonorkRemindListWin::GetRemindItem(HTREEITEM pItem
		, const TSonorkRemindDataHeader* HDR )
{
	TSonorkRemindViewItem*	VI, *tVI;
	for( VI = (TSonorkRemindViewItem*)view.tree.GetChild( pItem)
	; 	 VI != NULL
	;	 VI = (TSonorkRemindViewItem*)view.tree.GetNextSibling( VI ) )
	{
		if( VI->Type()==REMIND_VIEW_ITEM_TIME )
		{
			if( (tVI = GetRemindItem(VI->hitem,HDR)) != NULL )
				return tVI;
		}
		else
		if( VI->Type()==REMIND_VIEW_ITEM_DATA )
		{
			if( VI->RemindData()->equ(HDR) )
				return VI;
		}
	}
	return NULL;
}


void
 TSonorkRemindListWin::OnSize(UINT sz_type)
{
	if( sz_type == SIZE_RESTORED )
		RealignControls();
}
#define SPACING 	2
void
 TSonorkRemindListWin::RealignControls()
{
	const int 	midH=Height() - status.height;
	const int 	fullW=Width();
	int         leftW;
	int 		top,conH;

	::SetWindowPos( toolbar.hwnd
		, NULL
		, 0
		, 1
		, fullW
		, toolbar.height
		, SWP_NOACTIVATE|SWP_NOZORDER);

	top		= toolbar.height + 1;
	leftW	= fullW - mcal.width - SPACING*3 ;
	conH	= midH  - top;

	::SetWindowPos( view.tree.Handle()
		, NULL
		, SPACING
		, top
		, leftW
		, conH
		, SWP_NOACTIVATE|SWP_NOZORDER);

	::SetWindowPos( mcal.ctrl.Handle()
		, NULL
		, leftW + SPACING
		, top
		, mcal.width
		, conH
		, SWP_NOACTIVATE|SWP_NOZORDER);

	::SetWindowPos( status.hwnd
		, NULL
		, 0
		, midH
		, fullW
		, status.height
		, SWP_NOACTIVATE|SWP_NOZORDER);
}



TSonorkRemindListWin::TSonorkRemindListWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_REMINDLIST
	,SONORK_WIN_SF_NO_WIN_PARENT
	)
{
}


bool
 TSonorkRemindListWin::OnCommand(UINT id,HWND hwnd, UINT code)
{
	DWORD 	aux;
	if( hwnd == toolbar.hwnd )	// ToolBar
	{
		if(code != BN_CLICKED)
			return false;
		switch( id )
		{
			case TOOL_BAR_BUTTON_ADD:
				CmAddItem( NULL );
			break;

			case TOOL_BAR_BUTTON_PIN:
				aux = ToolBar_GetButtonState(toolbar.hwnd
						,TOOL_BAR_BUTTON_PIN) & TBSTATE_CHECKED;
				SetStayOnTop(aux);
			break;
			
			default:
				return false;
		}
		return true;
	}
	return false;
}

LRESULT
 TSonorkRemindListWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	TSonorkRemindViewItem*	VI;
	TSonorkTime		T;
	POINT			pt;
	if( N->hdr.idFrom == IDC_REMINDLIST_VIEW )
	{
		switch(N->hdr.code)
		{
		case NM_DBLCLK:
			::GetCursorPos(&pt);
			VI=(TSonorkRemindViewItem*)
				view.tree.HitTest(pt.x,pt.y
					,TVHT_ONITEM|TVHT_ONITEMINDENT|TVHT_ONITEMRIGHT);

			if( VI)
			{
				SetWinUsrFlag( REMIND_LIST_IGNORE_EXPAND );
				PostPoke(REMIND_LIST_POKE_OPEN_ITEM,(LPARAM)VI);
			}
		break;


		case TVN_ITEMEXPANDING:
			if( TestWinUsrFlag( REMIND_LIST_IGNORE_EXPAND ) )
			{
				ClearWinUsrFlag( REMIND_LIST_IGNORE_EXPAND );
				return true;
			}
			break;

		case TVN_ITEMEXPANDED:
			if(N->tview.itemNew.hItem != view.autoOpenHitem )
				view.autoOpenHitem=NULL;
			break;

		case TVN_DELETEITEM:
			VI=(TSonorkRemindViewItem*)N->tview.itemOld.lParam;
			if( view.autoOpenHitem == N->tview.itemOld.hItem)
				view.autoOpenHitem=NULL;

			if( !TestWinUsrFlag(REMIND_LIST_MOVING_VIEW_ITEM) )
				delete VI;
			else
				VI->hitem=NULL;
			SetWinUsrFlag(REMIND_LIST_DATA_UNSAVED);
			break;

		case NM_CUSTOMDRAW:
			return TSonorkTreeItem::OnCustomDraw( (NMTVCUSTOMDRAW*)N);

		case TVN_GETDISPINFO:		// asking for presentation
			return TSonorkTreeItem::OnGetDispInfo((TV_DISPINFO*)N);
		}

	}
	else
	if( N->hdr.idFrom == IDC_REMINDLIST_MONTHCAL )
	{
		if( N->hdr.code == MCN_SELECT )
		{
			T.SetTime(&N->selchange.stSelStart);
			if(view.autoOpenHitem!=NULL)
			{
				view.tree.ExpandItemLevels(
					view.autoOpenHitem
					, TVE_COLLAPSE
					, 0);
			}
			VI = GetDateItem(T);
			if(VI)
			{
				view.autoOpenHitem =VI->hitem;
				view.tree.ExpandItemLevels(
					view.autoOpenHitem
					, TVE_EXPAND
					, 0);
			}
			else
				view.autoOpenHitem = NULL;
			view.tree.SelectItem(view.autoOpenHitem);
		}
	}
	return 0;
}
LRESULT
 TSonorkRemindListWin::OnCtlWinMsg(TSonorkWinCtrl*WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg == WM_LBUTTONDBLCLK  )
	{
		PostPoke(REMIND_LIST_POKE_OPEN_DATE,0);
		return 0L;
	}
	return WC->DefaultProcessing(uMsg,wParam,lParam);
}

void
 TSonorkRemindListWin::LoadLabels()
{
	SetWindowText( GLS_LB_RMINDLST ) ;
}

void
 TSonorkRemindListWin::MakeFilePath(SONORK_C_STR path)
{
	char fname[64];
	sprintf(fname,"%u.%u_remind(%s)"
				,SonorkApp.ProfileUserId().v[0]
				,SonorkApp.ProfileUserId().v[1]
				,szSonorkAppMode);
	SonorkApp.GetDirPath(path,SONORK_APP_DIR_DATA,fname);
}
void
 TSonorkRemindListWin::DL_Clear()
{
	view.tree.DelAllItems();
	SetWinUsrFlag(REMIND_LIST_DATA_UNSAVED);
}

void
 TSonorkRemindListWin::DL_Load()
{
	TSonorkRemindData*	RD;
	TSonorkAtomDb		DB;
	UINT			ci,mi;

	DL_Clear();
	SetWinUsrFlag(REMIND_LIST_LOADING);
	if( SonorkApp.OpenAppDb( SONORK_APP_DB_REMIND , DB, false) )
	{
		
		mi=DB.Items();

		RD = new TSonorkRemindData;

		for(ci=0;ci<mi;ci++)
			if(DB.Get(ci,RD,NULL) == SONORK_RESULT_OK)
			{
				AddRemindItem(RD);
				RD=new TSonorkRemindData;
			}

		delete RD;

		DB.Close();

	}
	MultiClearWinUsrFlags(REMIND_LIST_DATA_UNSAVED|REMIND_LIST_LOADING);
}
void
 TSonorkRemindListWin::_SaveItems(TSonorkAtomDb*DB,HTREEITEM pItem)
{
	TSonorkRemindViewItem*	VI;
	for( VI = (TSonorkRemindViewItem*)view.tree.GetChild( pItem)
	; 	 VI != NULL
	;	 VI = (TSonorkRemindViewItem*)view.tree.GetNextSibling(VI ) )
	{
		if( VI->Type()==REMIND_VIEW_ITEM_DATA )
		{
			DB->Add( VI->RemindData() );
		}
		else
		{
			_SaveItems(DB,VI->hitem);
		}
	}
}

void
 TSonorkRemindListWin::DL_Save()
{
	TSonorkAtomDb		DB;

	if( SonorkApp.OpenAppDb( SONORK_APP_DB_REMIND , DB, true) )
	{
		_SaveItems(&DB,TVI_ROOT);
		DB.Close();
	}
	ClearWinUsrFlag(REMIND_LIST_DATA_UNSAVED);
	view.tree.SortChildren(TVI_ROOT,false);
}


bool
 TSonorkRemindListWin::OnCreate()
{
	static TSonorkWinToolBarButton	btn_info[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_BUTTON_ADD
			, SKIN_ICON_ADD
			, GLS_OP_ADD
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
,		{	TOOL_BAR_BUTTON_PIN
			, SKIN_ICON_PIN
			, GLS_OP_STAYTOP
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE }
	};
	SIZE size;
	LONG	cs;

	mcal.ctrl.AssignCtrl(this,IDC_REMINDLIST_MONTHCAL);
	mcal.width	= GetCtrlWidth(IDC_REMINDLIST_MONTHCAL);
	cs = ::GetClassLong( mcal.ctrl.Handle() , GCL_STYLE );
	::SetClassLong( mcal.ctrl.Handle() , GCL_STYLE  ,cs|CS_DBLCLKS);

	view.tree.SetHandle(GetDlgItem(IDC_REMINDLIST_VIEW));
	view.tree.SetStyle(TVS_LINESATROOT,TVS_HASLINES|TVS_LINESATROOT|WS_BORDER|TVS_INFOTIP);
	view.autoOpenHitem = NULL;



	SetCaptionIcon( SKIN_HICON_TIME ) ;

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
	toolbar.height = size.cy;
	status.hwnd		= GetDlgItem( IDC_REMINDLIST_STATUS );
	status.height	= GetCtrlHeight( IDC_REMINDLIST_STATUS );

	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_REMIND_LIST,true,"RemindList");
	RealignControls();
	return true;
}
void
 TSonorkRemindListWin::OnAfterCreate()
{
	UINT				expandCount=0;
	TSonorkRemindViewItem*	VI;
	TSonorkRemindViewItem*	bestItem;
	DL_Load();
	bestItem = NULL;
	for( VI = (TSonorkRemindViewItem*)view.tree.GetChild( TVI_ROOT)
	;    VI != NULL
	;    VI = (TSonorkRemindViewItem*)view.tree.GetNextSibling( VI ) )
	{
		if( VI->Type()==REMIND_VIEW_ITEM_TIME )
		{
			if( VI->Time().RelDate( SonorkApp.CurrentTime() ) <= SONORK_TIME_EQUAL)
			{
				bestItem=VI;
			}
			else
			{
				if(expandCount<5)
				{
					view.tree.ExpandItemLevels(VI,TVE_EXPAND ,0);
					expandCount++;
				}
			}
		}
	}
	if(bestItem)
	{
		view.autoOpenHitem=bestItem->hitem;
		view.tree.ExpandItemLevels(bestItem,TVE_EXPAND ,0);
		view.tree.SelectItem(view.autoOpenHitem);
	}
	else
		view.autoOpenHitem=NULL;
	TSonorkWin::SetStatus(status.hwnd,GLS_MS_REMHLP,SKIN_HICON_INFO);
}

void
 TSonorkRemindListWin::OnBeforeDestroy()
{
	DL_Save();
	DL_Clear();
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_REMIND_LIST,false,"RemindList");
	mcal.ctrl.ReleaseCtrl();
	SonorkApp.RequestCheckAlarms();
}

LRESULT
 TSonorkRemindListWin::OnPoke(SONORK_WIN_POKE poke,LPARAM lParam)
{
	TSonorkTime			T;
	MCHITTESTINFO	HT;
	if( poke == REMIND_LIST_POKE_OPEN_ITEM )
	{
		CmEditItem((TSonorkRemindViewItem*)lParam);
		ClearWinUsrFlag( REMIND_LIST_IGNORE_EXPAND );
	}
	else
	if( poke == REMIND_LIST_POKE_OPEN_DATE )
	{
		HT.cbSize=sizeof(MCHITTESTINFO);
		::GetCursorPos(&HT.pt);
		::ScreenToClient(mcal.ctrl.Handle(), &HT.pt);
		MonthCal_HitTest(mcal.ctrl.Handle(),&HT);
		if( (HT.uHit & MCHT_CALENDARDATE) == MCHT_CALENDARDATE )
		{
			T.SetTime( &HT.st );
			CmAddItem( &T );
		}
	}
	return 0L;
}









// --------------------------------------------------------
// TSonorkRemindWin

TSonorkRemindWin::TSonorkRemindWin(TSonorkWin*parent,TSonorkRemindData*RD,bool new_item)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_REMIND
	,0
	)
{
	remind=RD;
	ChangeWinUsrFlag(REMIND_EDIT_EXISTING,!new_item);
}

void
 TSonorkRemindWin::LoadForm()
{
	union
	{
		SYSTEMTIME	si;
		LVITEM		lv;
	}D;
	DWORD		tFlag,sFlag;


	// RemindTime() Can't have a zero date
	if( remind->RemindTime().IsZero() )
		remind->wRemindTime().SetTime_Local();
	remind->wRemindTime().ClearTime();


	remind->RemindTime().GetTime(&D.si);
	// Set the date picker (It cannot have a GTD_NONE)
	if(!::SendMessage(hDatePicker,DTM_SETSYSTEMTIME,GDT_VALID,(LPARAM)&D.si))
		TRACE_DEBUG("DTM_SETSYSTEMTIME (hDate) failed");

	// Set the alarm picker (It can have GTD_NONE)
	if( !remind->header.IsAlarmOn() || !remind->AlarmTime().GetTime(&D.si) )
	{
		// Not a valid date, clear alarm time
		// and set alarm picker to "No date"
		tFlag=GDT_NONE;
		remind->wAlarmTime().Clear();
		remind->header.flags&=~SONORK_REMIND_F_ALARM;
	}
	else
	{
		tFlag=GDT_VALID;
		remind->wAlarmTime().v[TSonorkTime::V_DATE]
			=remind->RemindTime().v[TSonorkTime::V_DATE];
		remind->header.flags|=SONORK_REMIND_F_ALARM;
	}
	::SendMessage(hAlarmPicker
		,DTM_SETSYSTEMTIME
		,tFlag
		,(LPARAM)&D.si);

	SetCtrlText(IDC_REMIND_TITLE,remind->title.CStr());
	SetCtrlText( IDC_REMIND_NOTES, remind->notes.CStr() );

	SetCtrlChecked( IDC_REMIND_RECURRENT	, remind->IsRecurrent());
	SetCtrlChecked( IDC_REMIND_PENDING 	, remind->IsPending());
	SetCtrlEnabled( IDC_REMIND_ALARM	, remind->IsPending());


	D.lv.mask 	= LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
	D.lv.iSubItem	=
	D.lv.lParam  	=
	D.lv.iImage		= 0;
	D.lv.stateMask	= LVIS_STATEIMAGEMASK ;

	sFlag = remind->header.alarm_mod;
	for( D.lv.iItem=0 , tFlag = SONORK_REMIND_ALARM_MOD_DAY_01
		;D.lv.iItem<7
		;D.lv.iItem++ , tFlag<<=1)
	{
		D.lv.state	=(tFlag&sFlag)
					?INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_CHECKED+1)
					:INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_UNCHECKED+1);
		D.lv.pszText	=(char*)SonorkApp.LangString((GLS_INDEX)(GLS_DY01+D.lv.iItem));
		ListView_InsertItem(hListView,&D.lv);
	}
	UpdateRecurrentFields();
}


void	TSonorkRemindWin::SaveForm()
{
	UINT		code;
	SYSTEMTIME	st;
	GetCtrlText( IDC_REMIND_TITLE, remind->title );
	GetCtrlText( IDC_REMIND_NOTES, remind->notes );

	// Save alarm settings
	// Note that the alarm days flags are processed in the
	// NM_CLICK of the list view.
	remind->header.flags&=
		~(SONORK_REMIND_F_ALARM|SONORK_REMIND_F_RECURRENT|SONORK_REMIND_F_PENDING);

	// RemindTime() Can't have a zero date
	if( remind->RemindTime().IsZero() )
		remind->wRemindTime().SetTime_Local();

	if(GetCtrlChecked( IDC_REMIND_PENDING ))
	{
		remind->header.flags|=SONORK_REMIND_F_PENDING;
		code = ::SendMessage( hAlarmPicker
			, DTM_GETSYSTEMTIME
			, 0
			, (LPARAM)&st);
		if( code ==  GDT_VALID )
		{
			remind->wAlarmTime().SetTime( &st );
			remind->header.flags|=SONORK_REMIND_F_ALARM;
		}
	}
	// We set the alarm time to the same date as the reminder
	remind->wAlarmTime().v[TSonorkTime::V_DATE]
		=remind->RemindTime().v[TSonorkTime::V_DATE];
}

void
	TSonorkRemindWin::UpdateRecurrentFields()
{
	BOOL isAlarmOn,isRecurrent;

	isAlarmOn = remind->IsAlarmOn()
			&& GetCtrlChecked(IDC_REMIND_PENDING);
	SetCtrlEnabled(IDC_REMIND_RECURRENT , isAlarmOn );
	isRecurrent = isAlarmOn && GetCtrlChecked(IDC_REMIND_RECURRENT);
	if( ChangeWinUsrFlag(REMIND_EDIT_LIST_DISABLED,!isRecurrent) )
	{
		ListView_SetTextColor(hListView
			,GetSysColor(isRecurrent?COLOR_WINDOWTEXT:COLOR_GRAYTEXT));
		::InvalidateRect(hListView,NULL,false);
	}
}

bool	TSonorkRemindWin::OnCommand(UINT id,HWND , UINT code)
{
	if( code == BN_CLICKED )
	{
		switch(id)
		{
			case IDOK:
				SaveForm();
				// break ommited, pass onto IDCANCEL
			case IDCANCEL:
			case IDC_REMIND_DEL:
				EndDialog(id);
				break;

			case IDC_REMIND_PENDING:
				SetCtrlEnabled( IDC_REMIND_ALARM, GetCtrlChecked(IDC_REMIND_PENDING));
				UpdateRecurrentFields();
				break;

			case IDC_REMIND_RECURRENT:
				UpdateRecurrentFields();
				break;

		}
		return true;
	}
	return false;
}
LRESULT	TSonorkRemindWin::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	union{
		SYSTEMTIME 		si;
		LV_HITTESTINFO 	ht;
	}D;
	RECT	rect;
	DWORD	flag,state;
	if(N->hdr.hwndFrom == hListView )
	{
		if(N->hdr.code == NM_CLICK )
		if(!TestWinUsrFlag(REMIND_EDIT_LIST_DISABLED) )
		{
			::GetCursorPos(&D.ht.pt);
			::ScreenToClient(hListView,&D.ht.pt);
			ListView_HitTest(hListView,&D.ht);
			if( D.ht.flags&(LVHT_ONITEM|LVHT_ONITEMSTATEICON) )
			{
				flag = SONORK_REMIND_ALARM_MOD_DAY_01 << D.ht.iItem;

				remind->header.alarm_mod^=flag;

				state=	(remind->header.alarm_mod&flag)
							?INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_CHECKED+1)
							:INDEXTOSTATEIMAGEMASK(SKIN_ICON_CB_UNCHECKED+1);
				ListView_GetItemRect(hListView,D.ht.iItem,&rect,LVIR_SELECTBOUNDS );
				::InvalidateRect(hListView,&rect,true);
				ListView_SetItemState(hListView
						,D.ht.iItem
						,state
						,LVIS_STATEIMAGEMASK);
			}
		}
	}
	else
	if(N->hdr.code == DTN_DATETIMECHANGE)
	{
		if( N->hdr.hwndFrom == hAlarmPicker )
		{
			if( N->datetime.dwFlags == GDT_NONE )
				remind->header.flags&=~SONORK_REMIND_F_ALARM;
			else
				remind->header.flags|=SONORK_REMIND_F_ALARM;
			UpdateRecurrentFields();
		}
		else
		if(N->hdr.hwndFrom == hDatePicker )
		{
			remind->wRemindTime().SetTime( &N->datetime.st );
		}
	}
	return 0L;
}
void	TSonorkRemindWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDL_REMIND_TITLE	,	GLS_LB_NAME   	}
	,	{IDL_REMIND_NOTES	,	GLS_LB_NOTES	}
	,	{IDL_REMIND_TIME	,	GLS_LB_DATE   	}
	,	{IDC_REMIND_DEL		,	GLS_OP_DEL    	}
	,	{IDC_REMIND_PENDING	,	GLS_LB_PENDING	}
	,	{IDG_REMIND_ALARM	,	GLS_LB_ALARM	}
	,	{IDOK		      	,	GLS_OP_STORE	}
	,	{-1		      	,	GLS_LB_RMINDER	}
	,	{0		      	,	GLS_NULL      	}
	};
	LoadLangEntries(gls_table,true);
	::SendMessage(hInput ,WM_SETFONT,(WPARAM)sonork_skin.Font(SKIN_FONT_LARGE)
		,MAKELPARAM(1,0));

}

bool
	TSonorkRemindWin::OnCreate()
{
	union {
		LV_COLUMN 	col;
	}lv;
	LONG		cx;

	SonorkApp.SetRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS );

	hInput		= GetDlgItem(IDC_REMIND_NOTES);

	SetCtrlEnabled(IDC_REMIND_DEL,TestWinUsrFlag(REMIND_EDIT_EXISTING));
	hDatePicker = GetDlgItem(IDC_REMIND_DATE);
	hAlarmPicker= GetDlgItem(IDC_REMIND_ALARM);
	::SendMessage(hDatePicker,DTM_SETFORMAT,0,(LPARAM)szSonorkDatePickFormat);
	::SendMessage(hAlarmPicker,DTM_SETFORMAT,0,(LPARAM)szSonorkTimePickFormat);

	cx		= GetCtrlWidth(IDC_REMIND_LIST);
	hListView	= GetDlgItem(IDC_REMIND_LIST);
	ListView_SetImageList(hListView,sonork_skin.Icons(),LVSIL_STATE );


	lv.col.mask		= LVCF_TEXT	| LVCF_WIDTH |LVCF_SUBITEM |LVCF_FMT;
	lv.col.fmt		= LVCFMT_LEFT;
	lv.col.iSubItem = 0;

	lv.col.cx		= cx-4;
	lv.col.pszText	= (char*)SonorkApp.LangString(GLS_LB_DAYS);
	ListView_InsertColumn(hListView,0,&lv.col);

	LoadForm();
	return true;
}


void
	TSonorkRemindWin::OnBeforeDestroy()
{
	SonorkApp.ClearRunFlag( SONORK_WAPP_RF_DISABLE_ALARMS );
}
LRESULT TSonorkRemindWin::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg   == WM_CTLCOLOREDIT )
		return sonork_skin.OnCtlColorMsgView(wParam);
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);

}

LRESULT	TSonorkRemindWin::OnPoke(SONORK_WIN_POKE,LPARAM)
{	return 0L;}




// ---------------------------------------------------------------------------
// TSonorkRemindViewItem

TSonorkRemindViewItem::TSonorkRemindViewItem( const TSonorkTime& time )
:TSonorkTreeItem(REMIND_VIEW_ITEM_TIME)
{
	if( time.IsValid() )
	{
		data.time.Set(time);
		data.time.ClearTime();
	}
	else
		data.time.Clear();
	boldChildren	= 0;
}
TSonorkRemindViewItem::TSonorkRemindViewItem( TSonorkRemindData* RD)
:TSonorkTreeItem(REMIND_VIEW_ITEM_DATA)
{
	data.remind 	= RD;
	boldChildren	= 0;
}

TSonorkRemindViewItem::~TSonorkRemindViewItem()
{
	if( Type() == REMIND_VIEW_ITEM_DATA )
		delete data.remind;
}
SONORK_C_CSTR
 TSonorkRemindViewItem::GetLabel(BOOL ,SKIN_ICON& icon) const
{
	static char tmp[64];
	if(Type()==REMIND_VIEW_ITEM_DATA)
	{
		if( data.remind->IsAlarmOn() )
		{
			icon = SKIN_ICON_SOUND;
			TSonorkWin32App::MakeTimeStr( data.remind->AlarmTime() ,tmp, MKTIMESTR_TIME);
			wsprintf(tmp,"%s %-.32s",tmp,data.remind->title.CStr());
		}
		else
		{
			lstrcpyn(tmp,data.remind->title.CStr(),64);
			if( data.remind->IsPending() )
				icon = SKIN_ICON_CB_CHECKED;
			else
				icon = SKIN_ICON_CB_UNCHECKED;
		}

	}
	else
	if(Type()==REMIND_VIEW_ITEM_TIME)
	{
		if(!data.time.GetDate(tmp))
			strcpy(tmp,"<none>");

		if( boldChildren )
			icon = SKIN_ICON_CB_SEMI_CHECKED;
		else
			icon = SKIN_ICON_CB_UNCHECKED;
	}
	else
	{
		tmp[0]=0;
		icon = SKIN_ICON_CB_UNCHECKED;
	}
	return tmp;
}
int
 TSonorkRemindViewItem::GetBoldValue() const
{
	if( Type() == REMIND_VIEW_ITEM_TIME )
		return boldChildren;
	else
	if( Type() == REMIND_VIEW_ITEM_DATA )
	{
		return data.remind->IsAlarmOn()	|| data.remind->IsPending()?1:0;
	}
	return 0;

}
/*DWORD TSonorkRemindViewItem::GetStateFlags()
{
	if( type == REMIND_VIEW_ITEM_TIME )
	{
		if( data.time.RelDate( SonorkApp.CurrentTime() ) >= SONORK_TIME_EQUAL )
			return TVIS_BOLD;
	}
	else
	{
		if( data.ptr->RemindFlags().Test(SONORK_REMIND_F_ENABLED) )
			return TVIS_BOLD;

	}
	return 0;
}
*/
