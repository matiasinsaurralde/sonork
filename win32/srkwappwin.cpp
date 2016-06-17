#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkwappwin.h"
#include "srkuserdatawin.h"
#include "srktextwin.h"
#include "srkwin32app_mfc.h"
#include "srk_string_loader.h"
//#include "srkasynctaskwin.h"
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

#define TARGET_WIDTH			200
#define	TOOLBAR_WIDTH			200
#define MARGIN				1
#define TOOL_BAR_ID			500
#define TOOLBAR_BASE_ID	    		1000
#define STATUS_ICON_ID			IDC_MSGTPL_ICON

// We use IN_TARGET_HWND flag
// to avoid pre-translating the keyboard messages
// while the cursor inside the target combo box
#define IN_TARGET_HWND			SONORK_WIN_F_USER_01
#define REPLY_MODE			SONORK_WIN_F_USER_02
#define WORKING		   		SONORK_WIN_F_USER_03


#define POKE_NAVIGATE			SONORK_WIN_POKE_01

#define MIN_WIDTH			(TARGET_WIDTH + TOOLBAR_WIDTH + 16)
#define MIN_HEIGHT			250
#define MAX_FMT_SIZE			8
#define	MAX_VAR_SIZE    		32
#define MAX_FIL_SIZE			24
#define PROCESS_BUFFER_SIZE		10240

enum TOOL_BAR_COMMAND
{
	TOOL_BAR_BACK		=	TOOLBAR_BASE_ID
,	TOOL_BAR_FORWARD
,	TOOL_BAR_STOP
,	TOOL_BAR_HOME
,	TOOL_BAR_LOG
,	TOOL_BAR_QUERY
,	TOOL_BAR_ADD_USER
,	TOOL_BAR_LAST
};
#define TOOL_BAR_BUTTONS	(TOOL_BAR_LAST - TOOLBAR_BASE_ID)

const char 	*szSonorkUrlPrefix="srk:";
#define 	lnSonorkUrlPrefix   4
static const char *SonorkMainFormName="Srk:Form1";
static const char *SonorkFormField[]=
{"To"
,"PostURL"
,"Title"
,"Test"
,"Query"
,"AddUser"
,"NoStore"
,"NoUts"
,"Auto"
,NULL
};

// ---------------------------------------------------------------------------
// Message Loading (declaration)
// ---------------------------------------------------------------------------

static UINT Load_Msg_Text(
		 TSonorkStringLoader&	SL
		,SONORK_C_CSTR 			pSrc
		,TSonorkUrlParams& 		params);

static UINT LoadFormValues(
		TSonorkStringLoader&	SL
		,TSonorkUrlParams& 		params);

static BOOL Load_Msg_ReplyFields(
		TSonorkStringLoader&	SL
		,SONORK_C_CSTR 			pSrc
		,TSonorkUrlParams& 		params);

static BOOL Load_Msg_Archive(
		 TSonorkStringLoader&	SL
		,SONORK_C_CSTR 			pSrc
		,TSonorkUrlParams& 		params);


// ---------------------------------------------------------------------------
// Constructor/Destructors
// ---------------------------------------------------------------------------

TSonorkWappWin::TSonorkWappWin(const TSonorkWappData*WD,const TSonorkExtUserData*UD, const TSonorkCCacheMark*cMark)
	:TSonorkTaskWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_MSGTPL
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
	wapp.url.Clear();
	wapp.type 	= WD->AppType();
	wapp.app_id 	= WD->AppId2();
	wapp.url_id	= WD->UrlId();
	task.reply_mark.Clear();
	ClearTarget();
	SetReplyMark(UD,cMark);
	SetEventMask(	SONORK_APP_EM_WAPP_PROCESSOR | SONORK_APP_EM_MSG_PROCESSOR);

}

TSonorkWappWin::TSonorkWappWin(SONORK_C_CSTR url,const TSonorkExtUserData*UD)
	:TSonorkTaskWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_MSGTPL
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
	wapp.type = SONORK_WAPP_TYPE_NONE;
	wapp.url.Set(url);
	wapp.app_id =
	wapp.url_id = 0;
	task.reply_mark.Clear();
	SetTarget_User( UD , SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID );
	SetEventMask(	SONORK_APP_EM_WAPP_PROCESSOR | SONORK_APP_EM_MSG_PROCESSOR);
}

void TSonorkWappWin::SetReplyMark(const TSonorkExtUserData *UD, const TSonorkCCacheMark* pMark)
{
	if( pMark != NULL )
	{
		SetWinUsrFlag(REPLY_MODE);
		task.reply_mark.Set(*pMark);
	}
	else
	{
		task.reply_mark.Clear();
		ClearWinUsrFlag(REPLY_MODE);
	}
	SetTarget_User(UD , SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID );
	UpdateInterface(SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION);
}


// ---------------------------------------------------------------------------

bool
 TSonorkWappWin::OnCreate()
{
	int target_height;
	browser.status  = SONORK_WAPP_WIN_NAVIGATE_IDLE;
	target_height	= GetCtrlHeight(IDL_MSGTPL_TARGET);
	top_height	= GetCtrlHeight(IDC_MSGTPL_TOPFRAME);
	status_height 	= GetCtrlHeight(IDC_MSGTPL_STATUS);
	status_hwnd	= GetDlgItem(IDC_MSGTPL_STATUS);
	target.hwnd	= GetDlgItem(IDL_MSGTPL_TARGET);

	SetupToolBar();
	SetCtrlRect(IDL_MSGTPL_TARGET
		, MARGIN
		, MARGIN
		, TARGET_WIDTH
		, target_height);
	::SetWindowPos(toolbar.hwnd
		,NULL
		,MARGIN+TARGET_WIDTH+MARGIN*2
		,MARGIN
		,TOOLBAR_WIDTH
		,toolbar.size.cy
		,SWP_NOZORDER|SWP_NOACTIVATE);
	if((browser.window = SrkMfc_CreateWindow(this,SRK_MFC_WIN_BROWSER))!=NULL)
	{
		HWND tHwnd;
		browser.hwnd = browser.window->Handle();
		//browser.wc_dlg.AssignCtrl(this,Browser_Handle(),IDC_MSGTPL_BROWSER);
		tHwnd = (HWND)ExecBrowserCmd(SRK_MFC_BROWSER_CM_GET_BROWSER_CTRL,0);
		if(tHwnd)
			browser.wc_ctl.AssignCtrl(this,tHwnd,IDC_MSGTPL_BROWSER);
	}

	SonorkApp.TransferWinStartInfo( this , true, "MsgTplWin", NULL);
	ClearMainForm();
	// ClearMainForm()
	// already updated the STATUS_BAR and CONTROLS
	// we update the other items
	UpdateInterface(
		 SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION
		|SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET
		);
	return true;
}

// -----------------------------------------------

void TSonorkWappWin::OnAfterCreate()
{
	if( !Browser_Created() )
	{
		MessageBox("Cannot start inline Browser\n"
			"Either Internet Explorer 4.0 or higher is not installed\n"
			"or the application has not been properly set up."
			,szSONORK
			,MB_OK|MB_ICONSTOP);
		Destroy(IDCANCEL);
	}
	else
	{
		RealignControls();
		::ShowWindow(Browser_Handle(), SW_SHOW);
		ShowWindow(SW_SHOW);
		SetForegroundWindow(Handle());
		GetWappUrl();
	}
}

// -----------------------------------------------

void TSonorkWappWin::OnBeforeDestroy()
{
	SonorkApp.TransferWinStartInfo( this , false, "MsgTplWin", NULL);
	if(Browser_Created())
	{
		browser.wc_ctl.ReleaseCtrl();
		SrkMfc_DestroyWindow(browser.window);
		browser.hwnd=NULL;
		browser.window=NULL;
	}
}


// ---------------------------------------------------------------------------
// Url Processing
// ---------------------------------------------------------------------------

bool
	TSonorkWappWin::PreProcessUrl(const char *szUrl,const char*data,UINT data_size)
{
	TSonorkWappUrl*	wapp_url;
	if( !strnicmp(szUrl,szSonorkUrlPrefix,lnSonorkUrlPrefix) )
	{
		wapp_url=new TSonorkWappUrl;
		wapp_url->LoadUrl( szUrl );
		if( data_size )
			wapp_url->params.LoadQueryString( data );
		PostPoke(POKE_NAVIGATE,(LPARAM)wapp_url);
		return true;
	}
	return false;	// Let current url pass
}

// -----------------------------------------------



// -----------------------------------------------


// ---------------------------------------------------------------------------
// Wapp resolution
// ---------------------------------------------------------------------------

void	TSonorkWappWin::GetWappUrl()
{
	SONORK_DWORD2		tag;
	UINT			P_size;
	TSonorkDataPacket*	P;
#define A_size		( 64 )
	if( wapp.type == SONORK_WAPP_TYPE_NONE )
	{
		wapp.pin.Clear();
		task.handle.ERR.SetOk();
		PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, SONORK_FUNCTION_WAPP_URL);
	}
	else
	{

		P=SONORK_AllocDataPacket( A_size );
		P_size = P->E_WappUrl_R(A_size
					,wapp.app_id
					,wapp.url_id);
		tag.v[0] = SONORK_FUNCTION_WAPP_URL;

		StartSonorkTask(task.handle.ERR,P,P_size,SONORK_TASKWIN_DEFAULT_FLAGS,GLS_TK_TASK,&tag);
		SONORK_FreeDataPacket( P );
		if(task.handle.ERR.Result() != SONORK_RESULT_OK)
			Destroy(IDCANCEL);
		else
			SetWorking(SONORK_FUNCTION_WAPP_URL);
	}

}
// -----------------------------------------------

void	TSonorkWappWin::OnPoke_WappUrl_Result()
{
	if( task.handle.ERR.Result() != SONORK_RESULT_OK )
	{
		TaskErrorBox(GLS_TK_TASK,&task.handle.ERR);
		Destroy(IDCANCEL);
	}
	else
	{
		Navigate_Home( true );
	}

}



// ---------------------------------------------------------------------------
// OnPoke
// ---------------------------------------------------------------------------

LRESULT TSonorkWappWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM lParam)
{
	switch(wParam)
	{
		case POKE_NAVIGATE:
			OnPoke_Navigate((TSonorkWappUrl*)lParam);
			break;

		case SONORK_WIN_POKE_SONORK_TASK_RESULT:
			SetWorking(SONORK_FUNCTION_NONE);
			switch(lParam)
			{
				case SONORK_FUNCTION_PUT_MSG:
					OnPoke_SendMsg_Result();
					break;
				case SONORK_FUNCTION_WAPP_URL:
					OnPoke_WappUrl_Result();
					break;
			}
			break;
	}
	return 0L;
}

// ---------------------------------------------------------------------------
// Sonork task handlers
// ---------------------------------------------------------------------------


// -----------------------------------------------

void TSonorkWappWin::OnSonorkTaskData(const SONORK_DWORD2&, TSonorkDataPacket*P, UINT P_size)
{
	if(P->Function() == SONORK_FUNCTION_WAPP_URL)
	{
		if( P->SubFunction() == SONORK_SUBFUNC_NONE )
		{
			TSonorkId		user_id;
			DWORD		app_pin;
			if(P->D_WappUrl_A(P_size,wapp.url,app_pin))
			{
				user_id.Set(wapp.app_id,wapp.url_id);
				SonorkApp.EncodePin64(
					wapp.pin
					,user_id
					,SONORK_SERVICE_ID_WAPP
					,app_pin);
			}
		}
	}

}

// -----------------------------------------------

void TSonorkWappWin::OnTaskEnd(SONORK_WIN_TASK_TYPE, const SONORK_DWORD2& tag, const TSonorkError*ERR)
{
	task.handle.ERR.Set(*ERR);
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, tag.v[0]);
}



// ---------------------------------------------------------------------------
// User interface
// ---------------------------------------------------------------------------

void TSonorkWappWin::UpdateInterface( DWORD update_flags )
{
	char	tmp[128];
	int 	l;


	if(Handle() == NULL)
		return;

	if( update_flags & SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION )
	{
		if(*page.title.CStr())
			lstrcpyn(tmp,page.title.CStr(),120);
		else
			strcpy(tmp,SonorkApp.LangString(GLS_LB_WAPPS));
		l=strlen(tmp);
		if( ReplyMode() )
			wsprintf(tmp+l," (%s)",SonorkApp.LangString(GLS_LB_REPLY));
		SetWindowText(tmp);
		SetCaptionIcon( ReplyMode()
			?SKIN_HICON_REPLY_TEMPLATE
			:SKIN_HICON_TEMPLATE);
	}

	if( update_flags & SONORK_WAPP_WIN_UPDATE_INTERFACE_STATUS )
	{
		if( browser.status != SONORK_WAPP_WIN_NAVIGATE_IDLE )
		{
			if(browser.status == SONORK_WAPP_WIN_NAVIGATE_REQUESTING )
			{
				TSonorkWin::SetStatus(status_hwnd
					,GLS_MS_REQTING
					,SKIN_HICON_BUSY);
			}
			else
			{
				TSonorkWin::SetStatus(status_hwnd
					,GLS_MS_RCVING
					,SKIN_HICON_FILE_DOWNLOAD);
			}
		}
		else
		if( !TestWinUsrFlag( WORKING) )
		{
			// if WORKING, SetWorking() should
			// have set the status bar text
			TSonorkWin::SetStatus(status_hwnd
				,GLS_MS_READY	//page.title.CStr()
				,SKIN_HICON_NONE);
				//ReplyMode()?SKIN_HICON_REPLY_TEMPLATE:SKIN_HICON_TEMPLATE);
		}
	}
	if( update_flags & SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET )
	{
		SetCtrlText(IDL_MSGTPL_TARGET , target.str.CStr() );
	}

	/*
	if( update_flags & SONORK_WAPP_WIN_UPDATE_INTERFACE_CONTROLS )
	{
		SetCtrlEnabled(IDC_MSGTPL_TARGET
			,	!(main_form.form_flags&SONORK_WAPP_WIN_FORM_F_NO_TARGET)
				&& !ReplyMode()
			);
		SetToolBarState(TOOL_BAR_LOG
			,tb_flags
			|((main_form.pc_flags&SONORK_APP_MPF_NO_STORE)?0:TBSTATE_CHECKED));

		SetToolBarState(TOOL_BAR_ADD_USER
			,tb_flags
			|((main_form.pc_flags&SONORK_APP_MPF_ADD_USER)?TBSTATE_CHECKED:0));

		if(ReplyMode())
			tb_flags=0;
		else
		if( main_form.usr_flags&SONORK_MSG_UF_QUERY )
			tb_flags|=TBSTATE_CHECKED;
		SetToolBarState(TOOL_BAR_QUERY,tb_flags);
	}
	*/
}

// -----------------------------------------------

bool TSonorkWappWin::OnCommand(UINT id,HWND hwnd, UINT notify_code)
{
	if(notify_code==BN_CLICKED)
	{
		if( hwnd == toolbar.hwnd )	// ToolBar
		{
			switch(id)
			{
				case TOOL_BAR_STOP:
					ExecBrowserCmd_Navigate_Stop();
					break;

				case TOOL_BAR_HOME:
					Navigate_Home( true );
					break;

				case TOOL_BAR_FORWARD:
					ExecBrowserCmd_Navigate_History(1);
					break;

				case TOOL_BAR_BACK:
					ExecBrowserCmd_Navigate_History(-1);
					break;

			}
		}
		else
			return false;
		return true;
	}
	else
	if( id == IDL_MSGTPL_TARGET )
	{
		if(notify_code==EN_KILLFOCUS)
		{
			ClearWinUsrFlag(IN_TARGET_HWND);
		}
		else
		if(notify_code==EN_SETFOCUS)
		{
			SetWinUsrFlag(IN_TARGET_HWND);
		}
	}
	return false;
}

// -----------------------------------------------

BOOL	TSonorkWappWin::GetToolBarChecked(UINT id)
{
	return ToolBar_GetButtonState(toolbar.hwnd,id)&TBSTATE_CHECKED;
}

// -----------------------------------------------

void TSonorkWappWin::SetToolBarChecked(UINT id ,BOOL checked)
{
	ToolBar_SetButtonState(toolbar.hwnd
	,id
	,checked?TBSTATE_ENABLED|TBSTATE_CHECKED:TBSTATE_ENABLED
	);
}

// -----------------------------------------------

void	TSonorkWappWin::OnToolTipText(UINT id, HWND  , TSonorkWinToolTipText&TTT )
{
	switch(id)
	{
		case TOOL_BAR_BACK:
			TTT.gls = GLS_NV_BCK;
			break;

		case TOOL_BAR_FORWARD:
			TTT.gls = GLS_NV_FWD;
			break;

		case TOOL_BAR_HOME:
			TTT.gls = GLS_NV_HOME;
			break;

		case TOOL_BAR_STOP:
			TTT.gls = GLS_OP_STOP;
			break;

		case TOOL_BAR_LOG:
			TTT.gls = GLS_OP_SAVMSGCPY;
			break;

		case TOOL_BAR_QUERY:
			TTT.gls = GLS_LB_QUERYMSG;
			break;

		case TOOL_BAR_ADD_USER:
			TTT.gls = GLS_NV_ADDUSRIFNE;
			break;

	}
}

// -----------------------------------------------

LRESULT TSonorkWappWin::OnCtlWinMsg(TSonorkWinCtrl*WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return WC->DefaultProcessing(uMsg,wParam,lParam);
}

// -----------------------------------------------

void
	TSonorkWappWin::SetWorking( SONORK_FUNCTION f )
{
	BOOL set=f!=SONORK_FUNCTION_NONE;
	GLS_INDEX	gls;
	if( set )
	{
		SetWinUsrFlag( WORKING );
		if( f == SONORK_FUNCTION_PUT_MSG )
		{
			gls = GLS_MS_SNDINGMSG;
		}
		else
		{
			gls = GLS_MS_PWAIT;
		}
		TSonorkWin::SetStatus(status_hwnd, gls , SKIN_HICON_BUSY);
	}
	else
	{
		ClearWinUsrFlag( WORKING );
		UpdateInterface( SONORK_WAPP_WIN_UPDATE_INTERFACE_STATUS );
	}
	::EnableWindow(Browser_Handle()	, !set);
	::EnableWindow(toolbar.hwnd	, !set);
}

// -----------------------------------------------

// ---------------------------------------------------------------------------
// Target
// ---------------------------------------------------------------------------

void
 TSonorkWappWin::SetTarget_User(const TSonorkExtUserData *UD, DWORD flags)
{
	char tmp[128];
	if( UD == NULL )
	{
		if( flags & SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID )
			ClearTarget();
	}
	else
	{
		sprintf(tmp,"%u.%u %-.24s"
			,UD->userId.v[0]
			,UD->userId.v[1]
			,UD->display_alias.CStr());
		target.str.Set(tmp);
		UpdateInterface( SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET );
	}
}

// -----------------------------------------------

void TSonorkWappWin::SetTarget_ID(const TSonorkId& user_id
			, const char *name
			, DWORD flags)
{
	char tmp[128];
	if( user_id.IsZero() )
	{
		if( flags & SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID )
		{
			ClearTarget();
		}
	}
	else
	{
		if( !(flags&SONORK_WAPP_WIN_SET_TARGET_NO_NAME_LOOKUP) )
		{
			if( name == NULL || (flags&SONORK_WAPP_WIN_SET_TARGET_FORCE_NAME_LOOKUP))
			{
				TSonorkExtUserData *UD;
				UD=SonorkApp.GetUser(user_id);
				if(UD!=NULL)
					name = UD->display_alias.CStr();
			}
		}
		user_id.GetStr(tmp);
		if(name)
		if(*name)
		{
			sprintf(tmp+strlen(tmp)," %-.24s",name);
		}
		target.str.Set(tmp);
		UpdateInterface( SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET );
	}
}

// -----------------------------------------------

void TSonorkWappWin::ClearTarget()
{
	target.str.Clear();
	UpdateInterface( SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET );
}

// -----------------------------------------------

/*void TSonorkWappWin::SetTarget_Str(SONORK_C_CSTR pStr, DWORD flags)
{
	if(pStr)
		if(*pStr)
		{
			int  len=strlen(pStr);
			if( len >= 3 )
			{
				char *p;
				TSonorkId user_id;
				TSonorkTempBuffer buff(strlen(pStr)+2);
				strcpy(buff.CStr(),pStr);
				p=strchr(buff.CStr(),'.');
				if(p)
				if((p-buff.CStr())<=10)
				{
					p=strchr(p+1,' ');
					if(p)
						*p++=0;
					user_id.SetStr(buff.CStr());
					SetTarget(user_id,ignore_name?NULL:p);
					return;
				}
			}
		}
	ClearTarget();
}
*/


// -----------------------------------------------
/*
void TSonorkWappWin::SetFormTarget(SONORK_C_CSTR new_target)
{
	char cur_target[64];
	GetCtrlText(IDC_MSGTPL_TARGET
		,cur_target
		,sizeof(cur_target));

	if(cur_target[0]==0||*new_target=='+'||*new_target=='=')
	{
		SetTarget_Str( new_target+1 , false );
		if(*new_target=='=')
			form.form_flags|=FORM_CF_NO_TARGET;
	}
	else
	{

		AddTarget( new_target+1 , true );
	}

}


// -----------------------------------------------

void TSonorkWappWin::AddTarget(SONORK_C_CSTR t_name, bool force_select_target)
{
	if( ComboBox_FindString(target_hwnd,-1,t_name) == CB_ERR)
	{
		ComboBox_AddString(target_hwnd,t_name);
	}
	if( ComboBox_GetCount( target_hwnd) == 1 || force_select_target )
	{
		SetCtrlText( IDC_MSGTPL_TARGET , t_name );
	}
}
*/

// -----------------------------------------------

LRESULT TSonorkWappWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
	switch(event)
	{
		case SONORK_DRAG_DROP_QUERY:
		{
			SonorkApp.ProcessDropQuery((TSonorkDropQuery*)lParam
			,SONORK_DROP_ACCEPT_F_SONORKCLIPDATA
			);

		}
		break;
		case SONORK_DRAG_DROP_UPDATE:
		{
			TSonorkDropMsg*M=(TSonorkDropMsg*)lParam;
			*M->drop_effect|=DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE;
		}
		break;

		case SONORK_DRAG_DROP_CANCEL:
		break;

		case SONORK_DRAG_DROP_EXECUTE:
		{
			TSonorkClipData*   CD;
			CD = SonorkApp.ProcessDropExecute((TSonorkDropExecute*)lParam);
			if( CD != NULL )
			{
				if( CD->DataType() == SONORK_CLIP_DATA_USER_ID)
				{
					SetTarget_ID( CD->GetUserId()
					, NULL
					, SONORK_WAPP_WIN_SET_TARGET_FORCE_NAME_LOOKUP
					| SONORK_WAPP_WIN_SET_TARGET_CLEAR_IF_INVALID
					);
				}
				CD->Release() ;
			}
		}
		break;
	}
	return 0;
}


void TSonorkWappWin::RealignControls()
{
	HDWP defer_handle;
	int  top,height;

	top = top_height;
	height = Height()- (status_height + top);
	defer_handle =BeginDeferWindowPos( 3 );
	if( Browser_Created() )
		defer_handle = DeferWindowPos(defer_handle
			,Browser_Handle()
			//,Browser_Control()
			,NULL
			,0
			,top
			,Width()
			,height
			,SWP_NOZORDER|SWP_NOACTIVATE);

	defer_handle = DeferWindowPos(defer_handle
		,status_hwnd
		,NULL
		,0
		,Height()-status_height
		,Width()
		,status_height
		,SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(defer_handle);
}
bool	TSonorkWappWin::OnDrawItem(DRAWITEMSTRUCT* )
{
	return false;
}
void	TSonorkWappWin::SetupToolBar()
{
	static TSonorkWinToolBarButton btn_info[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_BACK
			, SKIN_ICON_LEFT_ARROW
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }

	,	{	TOOL_BAR_FORWARD
			, SKIN_ICON_RIGHT_ARROW
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }

	,	{	TOOL_BAR_STOP
			, SKIN_ICON_STOP
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }
	,	{	TOOL_BAR_HOME
			, SKIN_ICON_URL
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON }

	,	{	TOOL_BAR_LOG
			, SKIN_ICON_FILE_DOWNLOAD
			, GLS_NULL
			, 0
			, TBSTYLE_CHECK }

	,	{	TOOL_BAR_QUERY
			, SKIN_ICON_QUERY_MSG
			, GLS_NULL
			, 0
			, TBSTYLE_CHECK }

	,	{	TOOL_BAR_ADD_USER
			, SKIN_ICON_ADD_USER
			, GLS_NULL
			, 0
			, TBSTYLE_CHECK }
	};
	toolbar.hwnd=TSonorkWin::CreateToolBar(
			Handle()
		,	TOOL_BAR_ID
		,	 WS_VISIBLE
			| TBSTYLE_TOOLTIPS
			| TBSTYLE_FLAT
			| CCS_NOPARENTALIGN
			| CCS_NODIVIDER
			| CCS_NORESIZE
		, 	TOOL_BAR_BUTTONS
		, 	btn_info
		, 	&toolbar.size);
}





// -----------------------------------------------

void TSonorkWappWin::SetNavigateStatus( SONORK_WAPP_WIN_NAVIGATE_STATUS ns)
{
	TRACE_DEBUG("SetNavigateStatus(%u->%u)"
		, browser.status
		, ns);
	if(browser.status != ns)
	{
		browser.status = ns;
		UpdateInterface( SONORK_WAPP_WIN_UPDATE_INTERFACE_STATUS );
		if( ns == SONORK_WAPP_WIN_NAVIGATE_DOWNLOADING )
		{
//			TRACE_DEBUG(" ** CLEAR FORM ** ");
			ClearMainForm();
		}
	}

}


// -----------------------------------------------
// Main Form

void
 TSonorkWappWin::LoadMainForm( IUnknown*II , SONORK_C_CSTR szAction)
 {
	TSonorkWappUrl		action;
	TSonorkWappForm 	form;
	TSonorkShortString	aux_str;
	char			tmp[64];
	int			i;
	action.LoadUrl( szAction );

	action.params.Clear();
	for(i=0;SonorkFormField[i]!=NULL;i++)
	{
		sprintf(tmp,"Srk%s",SonorkFormField[i]);
		if(ExecBrowserCmd_GetFormElement(II,tmp,&aux_str))
		{
//			TRACE_DEBUG("%s=%s",tmp,aux_str.CStr());
			action.params.Add(tmp,aux_str.CStr());
		}
	}
	ParseForm( &form , &action.params, NULL );

	page.title.Set(form.title);

	if(action.IsSonorkExtension())
	{
		target.str.Set(form.target);

		UpdateInterface(
			 SONORK_WAPP_WIN_UPDATE_INTERFACE_STATUS
			|SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION
			|SONORK_WAPP_WIN_UPDATE_INTERFACE_TARGET);
		SetToolBarChecked(TOOL_BAR_LOG
			, !(form.pc_flags&SONORK_APP_MPF_DONT_STORE ));
		SetToolBarChecked(TOOL_BAR_ADD_USER
			, (form.pc_flags&SONORK_APP_MPF_ADD_USER) );
		SetToolBarChecked(TOOL_BAR_QUERY
			, form.usr_flags&SONORK_MSG_UF_QUERY);
		if(action.params.GetValueNum("SrkAuto",0) != 0)
		{
			PostPoke(POKE_NAVIGATE,
			(LPARAM)new TSonorkWappUrl(
				SONORK_WAPP_URL_SUBMIT,SonorkMainFormName));
		}
	}
	else
	{
		UpdateInterface(SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION);
	}
}

// -----------------------------------------------

void TSonorkWappWin::ClearMainForm()
{
	page.title.Clear();
	UpdateInterface(SONORK_WAPP_WIN_UPDATE_INTERFACE_CAPTION);
}

void
 TSonorkWappWin::ParseForm( TSonorkWappForm*form
	,TSonorkUrlParams* 	params
	,TSonorkWappUrl*	post_url)
{
	SONORK_C_CSTR		aux_ptr;
	int			aux_int;
	form->pc_flags 	=
	form->usr_flags =
	form->form_flags=0;

	aux_ptr=params->GetValueStr("SrkTo");
	if( aux_ptr != NULL )
	{
		if( *aux_ptr>='0' && *aux_ptr<='9' )
		{
			form->target.Set( aux_ptr );
		}
		else
		{
			if( target.str.Length() )
				form->target.Set( target.str );
			else
				form->target.Set( aux_ptr+1 );
		}
	}
	else
		form->target.Set(target.str);

	aux_ptr=params->GetValueStr("SrkTitle");
	if( aux_ptr != NULL )
		form->title.Set( aux_ptr );
	else
		form->title.Clear();

	if( post_url != NULL )
	{
		aux_ptr=params->GetValueStr("SrkPostURL");
		post_url->LoadUrl( aux_ptr );
		if(post_url->type == SONORK_WAPP_URL_HTML)
		{
			if( *aux_ptr=='.' || strstr(aux_ptr,"://")==NULL )
			{
				if(!ExecBrowserCmd_CombineURL(post_url->str
					, NULL
					, aux_ptr))
					post_url->Clear();
			}
		}
	}
	if( !GetToolBarChecked(TOOL_BAR_LOG) )
		form->pc_flags|=SONORK_APP_MPF_DONT_STORE;

	if(GetToolBarChecked(TOOL_BAR_ADD_USER))
		form->pc_flags|=SONORK_APP_MPF_ADD_USER;

	if(GetToolBarChecked(TOOL_BAR_QUERY))
		form->usr_flags |= SONORK_MSG_UF_QUERY;

	if( params->GetValueNum("SrkTest",0) )
	{
		form->form_flags |= SONORK_WAPP_WIN_FORM_F_TEST_MODE;
	}

	aux_int = params->GetValueNum("SrkQuery",-1);
	if(aux_int==1)
		form->usr_flags |= SONORK_MSG_UF_QUERY;
	else
	if(aux_int==0)
		form->usr_flags &= ~SONORK_MSG_UF_QUERY;

	aux_int = params->GetValueNum("SrkAddUser",-1);
	if(aux_int==1)
		form->pc_flags |= SONORK_APP_MPF_ADD_USER;
	else
	if(aux_int==0)
		form->pc_flags &= ~SONORK_APP_MPF_ADD_USER;


	aux_int = params->GetValueNum("SrkNoStore",-1);
	if(aux_int==1)
		form->pc_flags |= SONORK_APP_MPF_DONT_STORE;
	else
	if(aux_int==0)
		form->pc_flags &= ~SONORK_APP_MPF_DONT_STORE;

	aux_int = params->GetValueNum("SrkNoUts",-1);
	if(aux_int==1)
		form->pc_flags |= SONORK_APP_MPF_NO_UTS;
	else
	if(aux_int==0)
		form->pc_flags &= ~SONORK_APP_MPF_NO_UTS;
}

// -----------------------------------------------



// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------

bool 	TSonorkWappWin::OnAppEvent(UINT event, UINT , void*data)
{
	if( event == SONORK_APP_EVENT_START_WAPP )
	{
		TSonorkAppEventStartWapp *E=(TSonorkAppEventStartWapp *)data;

		if(E->wapp_data->AppId2() == wapp.app_id)
		if(E->wapp_data->UrlId() == wapp.url_id)
		{
			E->consumed=true;
			SetReplyMark(E->user_data,E->reply_mark);
			Navigate_Home( true );
			ShowWindow(SW_RESTORE);
			SetForegroundWindow(Handle());
			return true;
		}
	}
	else
	if( event == SONORK_APP_EVENT_SONORK_MSG_SENT )
	{
		TSonorkAppEventMsg*E=(TSonorkAppEventMsg*)data;
		TRACE_DEBUG("SONORK_APP_EVENT_SONORK_MSG_SENT %u/%u"
			,E->taskId
			,task.handle.taskId);
		if( E->taskId == task.handle.taskId )
		{
			task.handle.taskId=0;
			task.handle.ERR.Set( *E->ERR );
			PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, SONORK_FUNCTION_PUT_MSG);
		}
	}
	return false;

}

// -----------------------------------------------

BOOL	TSonorkWappWin::ReplyMode()	const
{
	return TestWinUsrFlag(REPLY_MODE);
}

// -----------------------------------------------

void TSonorkWappWin::OnSize(UINT size_type)
{
	if(size_type == SIZE_RESTORED || size_type == SIZE_MAXIMIZED)
		RealignControls();
}

// -----------------------------------------------

void TSonorkWappWin::MarkMsg(DWORD )
{
/*
	TRACE_DEBUG("TSonorkWappWin::MarkGuMsg(%x,%u{%x.%x,%x.%x})"
		,flags
		,form.msg_mark.line_no
		,form.msg_mark.tracking_no.v[0]
		,form.msg_mark.tracking_no.v[1]
		,form.msg_mark.time.v[0]
		,form.msg_mark.time.v[1]);
	SonorkApp.MarkMsgProcessed(
	 form.target.locus.user_id
	,form.msg_mark
	,flags
	,SONORK_APP_CCF_UNREAD|SONORK_APP_CCF_PROCESSED
	,NULL);
*/
}

// -----------------------------------------------

bool	TSonorkWappWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=MIN_WIDTH;
	MMI->ptMinTrackSize.y=MIN_HEIGHT;
	return true;
}

// -----------------------------------------------

BOOL	TSonorkWappWin::PreTranslateMessage(MSG*pMsg)
{
	if( !TestWinUsrFlag(IN_TARGET_HWND) && Browser_Created() )
		if(browser.window->PreTranslateMessage(pMsg))
			return true;

	return TSonorkWin::PreTranslateMessage(pMsg);
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void TSonorkWappWin::Navigate_Home( bool immediate )
{
	if( wapp.url.Length() )
	{
		TSonorkWappUrl *wurl;
		SONORK_MEM_NEW( wurl = new TSonorkWappUrl );
		wurl->type = SONORK_WAPP_URL_HTML;
		PrepareSonorkQueryString( wurl->str , wapp.url.CStr() );
		if( immediate )
		{
			SendPoke( POKE_NAVIGATE , (LPARAM)wurl);
		}
		else
		{
			PostPoke( POKE_NAVIGATE , (LPARAM)wurl);
		}
	}
}

// -----------------------------------------------

void TSonorkWappWin::Navigate_WappUrl( const TSonorkWappUrl& pUrl, bool immediate )
{
	TSonorkWappUrl *wurl;
	SONORK_MEM_NEW( wurl = new TSonorkWappUrl );
	wurl->Set( pUrl );
	if( immediate )
	{
		SendPoke( POKE_NAVIGATE , (LPARAM)wurl);
	}
	else
	{
		PostPoke( POKE_NAVIGATE , (LPARAM)wurl);
	}

}

// -----------------------------------------------

void	TSonorkWappWin::OnPoke_Navigate(struct TSonorkWappUrl*wapp_url)
{
	SONORK_C_CSTR szUrl=wapp_url->CStr();

	switch(wapp_url->Type())
	{

		case SONORK_WAPP_URL_SEND_MSG:
			OnPoke_Navigate_SendMsg( wapp_url );
		break;

		case SONORK_WAPP_URL_ADD_USER:
			OnPoke_Navigate_AddUser( wapp_url );
		break;

		case SONORK_WAPP_URL_USER_INFO:
			OnPoke_Navigate_UserInfo( wapp_url );
		break;

		case SONORK_WAPP_URL_GOTO:
			OnPoke_Navigate_Goto( wapp_url );
		break;

		case SONORK_WAPP_URL_HTML:
			ExecBrowserCmd_Navigate_Url( szUrl );
		break;

		case SONORK_WAPP_URL_SUBMIT:
			ExecBrowserCmd_Navigate(TSrkMfcBrowserCmNavigate::CMD_SUBMIT
				,wapp_url->CStr()
				,0);
		break;

		default:
		case SONORK_WAPP_URL_INVALID:
			break;
	}
	SONORK_MEM_DELETE( wapp_url );
}

void
 TSonorkWappWin::OnPoke_Navigate_SendMsg( TSonorkWappUrl* action )
{
	TSonorkListIterator 	I;
	TSonorkUrlNamedValue* 	NV;
	TSonorkMsgHandleEx	handle;
	TSonorkMsg 		msg;
	TSonorkWappForm		form;
	TRACE_DEBUG("__________OnPoke_Navigate_SendMsg___________");
	action->params.BeginEnum(I);
	while( (NV=action->params.EnumNext(I)) != NULL )
		TRACE_DEBUG("%s='%s'",NV->Name(),NV->Value());
	action->params.EndEnum(I);
	ParseForm( &form , &action->params, &task.post_url);
	Load_Msg( msg , action->params);


	if( form.form_flags & SONORK_WAPP_WIN_FORM_F_TEST_MODE )
	{
		OnPoke_Navigate_SendMsg_Test(&form,action,&msg);
		task.handle.ERR.SetOk();
	}
	else
	{
		TSonorkExtUserData*UD;
		DWORD			reply_tracking_no;
		TSonorkUserLocus2	locus;
		DWORD			uts_link_id;

		if( !locus.userId.SetStr( form.target.CStr() ))
			task.handle.ERR.SetSys(SONORK_RESULT_INVALID_PARAMETER,GSS_BADGUID,SONORK_MODULE_LINE);
		else
		{
			UD=SonorkApp.GetUser( locus.userId );
			if(UD!=NULL)
			{
				UD->GetLocus1(&locus);
				uts_link_id = UD->UtsLinkId();
			}
			else
			{
				locus.sid.Clear();
				uts_link_id = SONORK_INVALID_LINK_ID;
			}
			if(ReplyMode())
			{
				form.usr_flags|=SONORK_MSG_UF_REPLY;
				reply_tracking_no= task.reply_mark.tracking_no.v[0];
			}
			else
			{
				form.usr_flags&=~SONORK_MSG_UF_REPLY;
				reply_tracking_no	= 0;
			}

			SonorkApp.PrepareMsg(
				handle
				, &msg
				, 0				// sys_flags
				, form.usr_flags		// usr_flags
				, form.pc_flags
				  |SONORK_APP_MPF_CLEAR_SERVICE	// pc_flags
				  |SONORK_APP_MPF_SILENT
				, reply_tracking_no		
				, NULL				// source Service
				);
			SonorkApp.SendMsgLocus(
				  handle
				, this
				, &locus
				, uts_link_id
				, &msg
				);
			if(task.handle.ERR.Result() == SONORK_RESULT_OK)
			{
				task.handle.taskId=handle.taskId;
				SetWorking( SONORK_FUNCTION_PUT_MSG );
				return;
			}
		}
	}
	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, SONORK_FUNCTION_PUT_MSG);
}

// -----------------------------------------------
void TSonorkWappWin::OnPoke_SendMsg_Result()
{
	TRACE_DEBUG("Send_Msg_Result() PostUrl=%u %s"
		,task.post_url.type
		,task.post_url.CStr());
	if( task.handle.ERR.Result() != SONORK_RESULT_OK )
	{
		TaskErrorBox(GLS_TK_SNDMSG,&task.handle.ERR);
	}
	else
	{
		if( ReplyMode() )
			MarkMsg(SONORK_APP_CCF_PROCESSED);
		Navigate_WappUrl( task.post_url , true );
	}
	task.post_url.Clear();
}

// http://idg.uol.com.br/pcw/testes/internet/0039.html
void
 TSonorkWappWin::OnPoke_Navigate_SendMsg_Test(
	 TSonorkWappForm* form
	,TSonorkWappUrl*  //wapp_url
	,TSonorkMsg* msg)
{
	int l;
	char *ptr;
	TSonorkTextWin TW(this);
	l=strlen( msg->CStr() )
		+ strlen( msg->data.CStr() )
		+ strlen( task.post_url.CStr() )
		+ 128;
	TW.title.Set(SonorkApp.LangString(GLS_OP_SNDMSG));
	ptr = TW.text.SetBufferSize(l);

	strcpy(ptr,"TARGET\r\n");
	strcat(ptr,form->target.CStr());
	strcat(ptr,"\r\nTEXT\r\n");
	strcat(ptr,msg->CStr());
	strcat(ptr,"\r\nPOST\r\n");
	strcat(ptr,task.post_url.CStr());
	strcat(ptr,"\r\nDATA\r\n");
	strcat(ptr,msg->data.CStr());
	TW.Execute();

}
void
 TSonorkWappWin::OnPoke_Navigate_Goto( TSonorkWappUrl* wapp_url )
 {
	SONORK_C_CSTR	pStr;
	pStr=wapp_url->params.GetValueStr("ID");
	if(!pStr)return;
	if( !stricmp(pStr,"back") )
	{
		ExecBrowserCmd_Navigate_History( -1 );
	}
	else
	if( !stricmp(pStr,"forward") )
	{
		ExecBrowserCmd_Navigate_History( 1 );
	}
	else
	if( !stricmp(pStr,"home") )
	{
		Navigate_Home( true );
	}
	else
	if( !stricmp(pStr,"close") )
	{
		Destroy(IDOK);
	}
}

// -----------------------------------------------
/*
void
 TSonorkWappWin::OnPoke_Navigate_SendMsg( SONORK_C_CSTR )
{
	if( !TestWinUsrFlag(SONORK_MAIN_FORM_LOADED ) )
	{
		taskERR.SetSys(SONORK_RESULT_INVALID_PARAMETER
			, GSS_BADCODEC
			, 0);
	}
	else
	if( ProcessSendMsg(form.params) )
	{
		if( !(form.form_flags & FORM_CF_TEST) )
		{
			if(MayStartTask(&taskERR))
			{
				SendMsg();
				return;
			}
		}
		else
		{
			taskERR.SetOk();
		}
	}

	PostPoke(SONORK_WIN_POKE_SONORK_TASK_RESULT, SONORK_FUNCTION_PUT_MSG);
}

*/

// -----------------------------------------------

void
 TSonorkWappWin::OnPoke_Navigate_AddUser( TSonorkWappUrl* wapp_url )
{
	TSonorkId		user_id;
	TSonorkError		ERR;

	if( user_id.SetStr( wapp_url->params.GetValueStr("ID") ) )
	{
		SonorkApp.CmdAddUser( this, ERR , user_id , 0);
	}
}

// -----------------------------------------------

void
 TSonorkWappWin::OnPoke_Navigate_UserInfo( TSonorkWappUrl* wapp_url  )
{
	TSonorkId user_id;
	if( user_id.SetStr( wapp_url->params.GetValueStr("ID") ) )
	{
		SonorkApp.OpenUserDataWin( user_id , NULL, NULL , NULL , 0);
	}
}

// -----------------------------------------------

void
	TSonorkWappWin::PrepareSonorkQueryString(TSonorkShortString& tUrl , SONORK_C_CSTR base_url)

{
	DWORD		aux1;
	UINT		alloc_size,base_size;
	TSonorkTempBuffer	tP1(64),tP2(64);
	char			*pBuff;

	base_size 	= strlen(base_url);
	alloc_size 	= (base_size + 640 + target.str.Length()*2);

	// Realloc buffer size if too small or too large
	tUrl.SetBufferSize( alloc_size );

	pBuff = tUrl.Buffer();
	memcpy(pBuff, base_url , base_size+1);
	pBuff+=base_size;

	if(strchr(base_url,'?'))
		*pBuff++='&';
	else
		*pBuff++='?';

	aux1=SonorkApp.ProfileUser().Region().GetLanguage();
	pBuff+=sprintf(pBuff
			,"AppMode=%u&%s&%s&Lang=%2.2s"
			,SonorkApp.IntranetMode()?SONORK_APP_START_MODE_INTRANET:SONORK_APP_START_MODE_INTERNET
			,SONORK_UrlParamEncode(&tP1, szSonorkId	,SonorkApp.ProfileUserId())
			,SONORK_UrlParamEncode(&tP2, szAlias,SonorkApp.ProfileUser().alias.CStr())
			,&aux1
			);
	pBuff+=wsprintf(pBuff
			,"&%s&%s"
			,SONORK_UrlParamEncode( &tP1, "InfoF"	,SonorkApp.ProfileUser().InfoFlags() )
			,SONORK_UrlParamEncode( &tP2, "Region"	,SonorkApp.ProfileUser().Region() )
			);

	pBuff+=wsprintf(pBuff
			,"&%s&%s"
			,SONORK_UrlParamEncode( &tP1, "InfoF"	,SonorkApp.ProfileUser().InfoFlags() )
			,SONORK_UrlParamEncode( &tP2, "Region"	,SonorkApp.ProfileUser().Region() )
			);

	pBuff+=sprintf(pBuff
			,"&SonorkVer=%02x%02x%02x%02x&%s"
			,SONORK_CLIENT_VERSION_MAJOR
			,SONORK_CLIENT_VERSION_MINOR
			,SONORK_CLIENT_VERSION_PATCH
			,SONORK_CLIENT_VERSION_BUILD
			,SONORK_UrlParamEncode( &tP1, "SidV", SonorkApp.ProfileUser().addr.sid )
		);
	pBuff+=sprintf(pBuff
			,"&%s&%s"
			,SONORK_UrlParamEncode( &tP2, "SidF1", SonorkApp.ProfileUser().addr.sidFlags.v[0])
			,SONORK_UrlParamEncode( &tP2, "SidF2", SonorkApp.ProfileUser().addr.sidFlags.v[1])
			);
	wsprintf(tP1.CStr()
		,"%u.%u"
		,wapp.app_id
		,wapp.url_id);
	pBuff+=sprintf(pBuff
			,"&%s"
			,SONORK_UrlParamEncode( &tP2, "WappId", tP1.CStr() )
		);

	wsprintf(tP1.CStr()
		,"%x:%x.%x.%x.%x"
		,SONORK_PIN_TYPE_64
		,wapp.pin.v[0].v[0]
		,wapp.pin.v[0].v[1]
		,wapp.pin.v[1].v[0]
		,wapp.pin.v[1].v[1]
		);
	pBuff+=sprintf(pBuff
			,"&%s"
			,SONORK_UrlParamEncode( &tP2, "SonorkPin", tP1.CStr() ));

	sprintf(pBuff
			,"&%s&Reply=%u"
			,SONORK_UrlParamEncode( &tP1 , "Tgt" , target.str.CStr() )
			,ReplyMode()?1:0
			);
/*
	TRACE_DEBUG("PREPARE L=%u A=%u T=%u"
		, tUrl.Length()
		, alloc_size
		, page.target.Length());
*/

}

// ---------------------------------------------------------------------------
// Browser commands
// ---------------------------------------------------------------------------

LRESULT
 TSonorkWappWin::ExecBrowserCmd(SRK_MFC_CMD cmd ,LPARAM lParam)
{
	LRESULT rv;
	if( Browser_Created() )
		rv = browser.window->Exec(cmd,lParam);
	else
		rv = E_FAIL;
	return rv;
}

// -----------------------------------------------

LRESULT
 TSonorkWappWin::ExecBrowserCmd_Navigate(DWORD cmd,SONORK_C_CSTR str,int i_param)
{
	TSrkMfcBrowserCmNavigate CMD;
	CMD.cmd		= cmd;
	CMD.str		= str;
	CMD.i_param	= i_param;
	return ExecBrowserCmd(SRK_MFC_BROWSER_CM_NAVIGATE,(LPARAM)&CMD);
}

// -----------------------------------------------

LRESULT
 TSonorkWappWin::ExecBrowserCmd_Navigate_Url(SONORK_C_CSTR pUrl)
{
	return ExecBrowserCmd_Navigate(TSrkMfcBrowserCmNavigate::CMD_URL,pUrl,0);
}

// -----------------------------------------------

LRESULT
 TSonorkWappWin::ExecBrowserCmd_Navigate_Stop()
{
	return ExecBrowserCmd_Navigate(TSrkMfcBrowserCmNavigate::CMD_STOP
		,NULL
		,0);
}

// -----------------------------------------------

LRESULT
 TSonorkWappWin::ExecBrowserCmd_Navigate_History(int step)
{
	return ExecBrowserCmd_Navigate(TSrkMfcBrowserCmNavigate::CMD_HISTORY
		,NULL
		,step);
}

// -----------------------------------------------

IUnknown*
 TSonorkWappWin::ExecBrowserCmd_GetFormInfo(SONORK_C_CSTR name, TSonorkShortString* action)
{
	TSrkMfcBrowserCmGetFormInfo FI;
	memset(&FI,0,sizeof(FI));
	FI.form			= NULL;
	FI.s_param		= name;
	FI.str[TSrkMfcBrowserCmGetFormInfo::STR_VALUE] 	= action;
	return (IUnknown*)ExecBrowserCmd(SRK_MFC_BROWSER_CM_GET_FORM_INFO,(LPARAM)&FI);
}

// -----------------------------------------------

BOOL
 TSonorkWappWin::ExecBrowserCmd_GetFormElement(IUnknown*form
				, SONORK_C_CSTR		 name
				, TSonorkShortString*	value)
{
	TSrkMfcBrowserCmGetFormInfo FI;
	memset(&FI,0,sizeof(FI));
//	TRACE_DEBUG("Browser_GetFormElement(%s)",name);
	FI.form			= form;
	FI.s_param		= name;
	FI.str[TSrkMfcBrowserCmGetFormInfo::STR_VALUE] 	= value;
	return ExecBrowserCmd(SRK_MFC_BROWSER_CM_GET_FORM_ELEMENT,(LPARAM)&FI)!=0;
}

// -----------------------------------------------

BOOL
 TSonorkWappWin::ExecBrowserCmd_CombineURL(TSonorkShortString& oURL
	, SONORK_C_CSTR bURL
	, SONORK_C_CSTR aURL)
{
	TSrkMfcBrowserCmCombineUrl CU;
	LRESULT		rv;
	CU.oURL 	= &oURL;
	CU.iURL[0]	= bURL; // set <bURL> to NULL to combine with current
	CU.iURL[1]	= aURL;

	rv = ExecBrowserCmd(SRK_MFC_BROWSER_CM_COMBINE_URL,(LPARAM)&CU);
	if(rv!=S_OK)
	{
		TRACE_DEBUG("CombineURL failed: err=%d",rv);
		return false;
	}
	return true;
}


// ---------------------------------------------------------------------------
// Browser events
// ---------------------------------------------------------------------------

LRESULT
 TSonorkWappWin::OnMfcEvent(SRK_MFC_EVENT event,LPARAM lParam)
{
	if( TestWinSysFlag(SONORK_WIN_SF_DESTROYING) )
		return 0L;

	switch(event)
	{
		case SRK_MFC_BROWSER_EV_BEFORE_NAVIGATE:
			BrowserEvent_OnBeforeNavigate((TSrkMfcBrowserEvBeforeNavigate*)lParam);
			break;

		case SRK_MFC_BROWSER_EV_DOWNLOAD_BEGIN:
		case SRK_MFC_BROWSER_EV_DOWNLOAD_COMPLETE:
			BrowserEvent_OnDownload( event == SRK_MFC_BROWSER_EV_DOWNLOAD_BEGIN);
			break;

		case SRK_MFC_BROWSER_EV_PROGRESS_CHANGE:
			BrowserEvent_OnProgressChange(
				  ((TSrkMfcBrowserEvProgressChange*)lParam)->cur_v
				, ((TSrkMfcBrowserEvProgressChange*)lParam)->max_v
				);
			break;

		case SRK_MFC_BROWSER_EV_NAVIGATE_COMPLETE:
			BrowserEvent_OnNavigateComplete( (SONORK_C_CSTR)lParam );
			break;

		case SRK_MFC_BROWSER_EV_DOCUMENT_COMPLETE:
			BrowserEvent_OnDocumentComplete( (SONORK_C_CSTR)lParam );
			break;

		case SRK_MFC_BROWSER_EV_TITLE_CHANGE:
			BrowserEvent_OnTitleChange( (SONORK_C_CSTR)lParam );
			break;
	}
	return 0;
}

// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnBeforeNavigate(TSrkMfcBrowserEvBeforeNavigate*ev)
{
	bool cancel_url;
	SONORK_WAPP_WIN_NAVIGATE_STATUS  ns;
	cancel_url = PreProcessUrl(ev->url
			,ev->post_data
			,ev->post_data_size);
/*	TRACE_DEBUG("BEFORE_NAV (%u,%-.32s): %s"
		,browser.status
		,ev->url
		,cancel_url?"**CANCEL**":"OK");
*/
	if(cancel_url)
	{
		*ev->cancel=true;
		ns = SONORK_WAPP_WIN_NAVIGATE_IDLE;
	}
	else
	{
		ns = SONORK_WAPP_WIN_NAVIGATE_REQUESTING;
	}
	SetNavigateStatus( ns );
}

// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnProgressChange(LONG cur_v, LONG max_v)
{
//	TRACE_DEBUG("PROGRESS (%u,%d,%d)",browser.status,cur_v,max_v);
	if( browser.status == SONORK_WAPP_WIN_NAVIGATE_REQUESTING )
	{
		if( cur_v==-1 )
		{
			// FAILED
			TRACE_DEBUG("**FAILED**",browser.status,cur_v,max_v);
			SetNavigateStatus( SONORK_WAPP_WIN_NAVIGATE_IDLE );
			ClearMainForm();
		}
	}
}

// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnNavigateComplete(SONORK_C_CSTR )
{
	if( browser.status == SONORK_WAPP_WIN_NAVIGATE_REQUESTING )
	{
//		TRACE_DEBUG("DOWNLOAD_START (%-.45s)",pUrl);
		SetNavigateStatus( SONORK_WAPP_WIN_NAVIGATE_DOWNLOADING );
	}
}

// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnDocumentComplete(SONORK_C_CSTR )
{
	IUnknown*		II;
	TSonorkShortString 	value;
	/*
	TRACE_DEBUG("DOC_COMPLETE %u %-.32s"
		, browser.status
		, url);
	*/
	if( browser.status ==  SONORK_WAPP_WIN_NAVIGATE_DOWNLOADING )
	{
		II  = ExecBrowserCmd_GetFormInfo(SonorkMainFormName,&value);
		if(II)
		{
			LoadMainForm( II , value.CStr() );
			II->Release();
		}
	}
	SetNavigateStatus( SONORK_WAPP_WIN_NAVIGATE_IDLE );

}
// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnDownload(BOOL )
{
}

// -----------------------------------------------

void
 TSonorkWappWin::BrowserEvent_OnTitleChange(SONORK_C_CSTR )
{
}



// ---------------------------------------------------------------------------
// TSonorkWappUrl
// ---------------------------------------------------------------------------
TSonorkWappUrl::TSonorkWappUrl(SONORK_WAPP_URL_TYPE t,SONORK_C_CSTR pStr)
{
	type = t;
	str.Set(pStr);
}
void
	TSonorkWappUrl::LoadUrl(SONORK_C_CSTR pStr)
{
// ---------------------------------------------------------
// V 1.04.07 extensions added to this function
// Please refer to code of V1.04.06, function TGWappUrl::LoadGuCmd()
// if backward compatibility is to be maintained!
// V1.04.06 was NOT an official release of the Wapp system
// ---------------------------------------------------------
	if( pStr == NULL )
	{
		type 	= SONORK_WAPP_URL_VOID;
	}
	else
	{
		for(;;)
		{
			if(strnicmp(pStr,szSonorkUrlPrefix,lnSonorkUrlPrefix))
				break;

			pStr+=lnSonorkUrlPrefix;
			str.Clear();

			if(!strnicmp(pStr,"Goto",4))
			{
				type 	= SONORK_WAPP_URL_GOTO;
				pStr+=4;
			}
			else
			if(!strnicmp(pStr,"SendMsg",7))
			{
				type 	= SONORK_WAPP_URL_SEND_MSG;
				pStr+=7;
			}
			else
			if(!strnicmp(pStr,"AddUser",7))
			{
				type 	= SONORK_WAPP_URL_ADD_USER;
				pStr+=7;
			}
			else
			if(!strnicmp(pStr,"UserInfo",8))
			{
				type 	= SONORK_WAPP_URL_USER_INFO;
				pStr+=8;
			}
			else
			{
				type 	= SONORK_WAPP_URL_VOID;
				pStr	= "";
			}
			if(*pStr=='?')
			{
				params.LoadQueryString( ++pStr );
			}
			return ;
		}
		type 	= SONORK_WAPP_URL_HTML;
	}
	str.Set( pStr );
	params.Clear();

}

// -----------------------------------------------
void TSonorkWappUrl::Set(const TSonorkWappUrl&O)
{
	type=O.type;
	str.Set(O.str);
	params.Set(O.params);
}

// -----------------------------------------------

void TSonorkWappUrl::Clear()
{
	type=SONORK_WAPP_URL_INVALID;
	str.Clear();
	params.Clear();
}

// ---------------------------------------------------------------------------
// Message Loading
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// Msg Processing
// ---------------------------------------------------------------------------

bool
	TSonorkWappWin::Load_Msg(TSonorkMsg& msg , TSonorkUrlParams& params)
{

	TSonorkTempBuffer		buffer(PROCESS_BUFFER_SIZE);
	TSonorkStringLoader		SL;
	TSonorkStringLoaderMark		SL_mark;
	SONORK_C_CSTR			pSrc;
	BOOL				pSuccess;

	pSrc = params.GetValueStr("SrkText");
	for(;;)
	{
		if(pSrc != NULL)
		{
			SL.Open(buffer.CStr(),buffer.GetSize());
			pSuccess=Load_Msg_Text(SL,pSrc,params);
			SL.Close();
			if(pSuccess)
			{
				msg.SetStr( 0 , buffer.CStr());
				break;
			}
		}
		task.handle.ERR.Set(SONORK_RESULT_INVALID_PARAMETER
			,"Form is invalid"
			,SONORK_MODULE_LINE,true);
		return false;
	}

	SL.Open(buffer.CStr(),buffer.GetSize());

	if((pSrc = params.GetValueStr("SrkReplyWapp"))!=NULL)
	if(*pSrc)
	{
		SL.AddStr("ReplyWapp=");
		SL.AddStr(pSrc);
		SL.AddStr("\r\n");
	}

	if((pSrc = params.GetValueStr("SrkReplyFields"))!=NULL)
	{
		SL.Push(SL_mark);
		pSuccess = Load_Msg_ReplyFields(SL,pSrc,params);
		SL.Pop(SL_mark,!pSuccess);
	}

	if((pSrc = params.GetValueStr("SrkArchive"))!=NULL)
	{
		SL.Push(SL_mark);
		pSuccess = Load_Msg_Archive(SL,pSrc,params);
		SL.Pop(SL_mark,!pSuccess);
	}

	SL.Push(SL_mark);
	LoadFormValues(SL,params);
	SL.Pop(SL_mark,false);

	SL.Close();
	msg.data.AppendStr( buffer.CStr() , true);

	if( msg.data.DataSize() )
		msg.data.SetDataType(SONORK_ATOM_MTPL_INFO);
	else
		msg.data.Clear();
	task.handle.ERR.SetOk();
	return true;
}

UINT Load_Msg_Text(
	 TSonorkStringLoader&	SL
	,SONORK_C_CSTR 		pSrc
	,TSonorkUrlParams& 	params)
{
	SONORK_C_CHAR   c;
	TSonorkStringLoader	VL;
	SONORK_C_CSTR		var_value;
	char			var_name[MAX_VAR_SIZE];

	while( *pSrc && !SL.Full() )
	{
		c=*pSrc++;
		if(VL.IsOpen())
		{
			if(c=='$')
			{
				VL.Close();
				var_value = params.GetValueStr(var_name);
				if(var_value)
					while( *var_value && !SL.Full() )
						SL.Add(*var_value++);
				continue;
			}
			else
				VL.Add(c);
			continue;
		}
		else
		if(c=='\\')
		{
			if(!*pSrc)break;
			SL.AddDecode(*pSrc++);
			continue;
		}
		else
		if(c=='$')
		{
			VL.Open(var_name,MAX_VAR_SIZE);
			continue;
		}
		SL.Add(c);
	}
	return SL.CurLen();
}

// -----------------------------------------------

UINT	LoadFormValues(
		TSonorkStringLoader&	SL
		,TSonorkUrlParams& 	params)
{
	TSonorkListIterator 	I;
	TSonorkUrlNamedValue* 	NV;
	TSonorkShortString	tmp_str;
	UINT			fields;
	fields=0;
	SL.AddStr("FieldData=");
	params.BeginEnum(I);
	while( (NV=params.EnumNext(I)) != NULL )
	{
		if(!NV->GetMark()||!*NV->Value())continue;
		if( fields > 0 )SL.Add('&');
		SL.AddStr(NV->Name());
		SL.Add('=');
		SL.AddStr( SONORK_UrlEncode(&tmp_str, NV->Value() ) );
		fields++;
	}
	params.EndEnum(I);
	SL.AddStr("\r\n");
	return fields;

}

// -----------------------------------------------

BOOL Load_Msg_ReplyFields(
		TSonorkStringLoader&	SL
		,SONORK_C_CSTR 			pSrc
		,TSonorkUrlParams& 		params)
{
	TSonorkStringLoader	VL;
	SONORK_C_CSTR		pComma;
	UINT			fields,len;
	TSonorkUrlNamedValue *	pNV;
	TSonorkShortString	tmp_str;
	char			pName[MAX_VAR_SIZE];

	SL.AddStr("ReplyFields=");
	fields=0;
	while(*pSrc)
	{
		pComma=strchr(pSrc,',');
		if(pComma)
			len =pComma-pSrc;
		else
			len =MAX_VAR_SIZE;
		if(len)
		{
			SONORK_StrCopy(pName
				,MAX_VAR_SIZE
				,pSrc
				,len);
			if(!*pName)break;
			pNV=params.GetValueNV(pName);
			if(pNV)
			if(*pNV->Value())
			{
				if(fields++)SL.Add('+');
				SL.AddStr( SONORK_UrlEncode(&tmp_str,pName) );
				pNV->SetMark();
			}
		}
		if(pComma)
			pSrc=pComma+1;
		else
			break;
	}
	SL.AddStr("\r\n");
	return fields>0;
}

// -----------------------------------------------
// LoadArchiveInfo:
// 	Archive format is <FORMAT_NAME>,<FILE_NAME>,<FIELD1>,<FIELD2>,...,<FIELDn>
// Currently, only supported format name is "CSV"

BOOL Load_Msg_Archive(
	 TSonorkStringLoader&	SL
	,SONORK_C_CSTR 		pSrc
	,TSonorkUrlParams& 	params)

{
	SONORK_C_CSTR		pVar;
	UINT			format_count,field_count;
	UINT			section_index;
	char			fmt_name[MAX_FMT_SIZE];
	char			var_name[MAX_VAR_SIZE];
	TSonorkUrlNamedValue 	*pNV;
	TSonorkStringLoaderMark	undo_mark;
	TSonorkShortString	tmp_str;

//  Input format is
//   {<FORMAT1>,<NAME1>,<field#1>,<field#2>..<field#N>}
//   {<FORMAT2>,<NAME2>,<field#1>,<field#2>..<field#N>}
//   ..
//   {<FORMAT3>,<NAME3>,<field#1>,<field#2>..<field#N>}
//  Output format is the same as input format

	SL.AddStr("Archive=");
	format_count=0;
	while(pSrc && !SL.Full() && format_count<3)
	{
		pSrc=strchr(pSrc,'{');	// Search for section init
		if(!pSrc)break;
		pSrc++;
		pVar=strchr(pSrc,',');	// Search for comma after <format name>
		if(!pVar)break;
		SONORK_StrCopy(fmt_name
			,MAX_FMT_SIZE
			,pSrc
			,pVar-pSrc);
		pSrc=++pVar;
		pVar=strchr(pSrc,',');  // Search for comma after <file name>
		if(!pVar)break;

		SONORK_StrCopy(var_name
			,MAX_FIL_SIZE
			,pSrc
			,pVar-pSrc);
		pSrc=++pVar;

		SL.Push(undo_mark);
		if(format_count)
			SL.Add('&');
		SL.AddStr(fmt_name);
		SL.Add('=');
		SL.AddStr( SONORK_UrlEncode(&tmp_str, var_name ) );
		for(field_count=0;;)
		{
			pVar=pSrc;//search for ',' or '}' after field name
			section_index=0;
			while(*pVar)
			{
				if(*pVar==',')
				{
					section_index=1;
					break;
				}
				if(*pVar=='}')
				{
					section_index=-1;
					break;
				}
				else
				if(*pVar=='{')	// Another section starting: Error
					break;
				pVar++;
			}
			if(!section_index)break;
			SONORK_StrCopy(var_name,MAX_VAR_SIZE,pSrc,pVar-pSrc);
			pVar++;
			if(!*var_name)break;
			pNV = params.GetValueNV(var_name);
			if(pNV)pNV->SetMark();
			SL.Add('+');
			SL.AddStr( SONORK_UrlEncode(&tmp_str, var_name ) );
			pSrc = pVar;
			field_count++;
			if(section_index==-1)
				break;
		}
		if(!field_count)
		{
			SL.Pop(undo_mark,true);
		}
		else
		{
			format_count++;
		}
	}
	SL.AddStr("\r\n");
	return format_count>0;
}


