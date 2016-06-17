#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkconsole.h"
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

#define SPACING						1
#define TOOL_BAR_ID					301

// ----------------------------------------------------------------------------

LRESULT
 TSonorkConsole::GenExportEvent(TSonorkConsoleExportEvent*pEV)
{
	return cb.ptr(cb.tag
			,SONORK_CONSOLE_EVENT_EXPORT
			,0
			,(void*)pEV);
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::Export(
	 SONORK_C_STR	pPath		// must be at least SONORK_MAX_PATH
	,DWORD 		exp_flags
	,void*		pExportTag
	,SONORK_CONSOLE_EXPORT_FORMAT* 	pExportFormat)
{
	TSonorkShortString 			str;
	TSonorkConsoleExportEvent 	EV;
	DWORD					ci,mi;
	TSonorkListIterator			I;

	GetLocalTime(&EV.st);
	EV.flags	= exp_flags;
	if(!pExportFormat)
		EV.flags|=SONORK_CONSOLE_EXPORT_F_ASK_PATH;
	else
		EV.format = *pExportFormat;

	if( EV.flags & SONORK_CONSOLE_EXPORT_F_ADD_TIME_SUFFIX)
	{
		str.SetBufferSize(64);
		wsprintf(str.Buffer()
			,"_%04u%02u%02u_%02u%02u"
			,EV.st.wYear
			,EV.st.wMonth
			,EV.st.wDay
			,EV.st.wHour
			,EV.st.wMinute);
		strcat(pPath,str.CStr());
	}
	if( EV.flags & SONORK_CONSOLE_EXPORT_F_ASK_PATH )
		EV.format 	= GetExportPath( str , "ExportPath" , pPath );

	if(EV.format == SONORK_CONSOLE_EXPORT_NONE)
		return false;

	lstrcpyn(pPath,str.CStr(),SONORK_MAX_PATH);
	EV.file = fopen(pPath,"wt");
	if(!EV.file)
		return false;

	if( SelectionActive() )
		EV.flags|=SONORK_CONSOLE_EXPORT_F_SELECTION;
	else
		EV.flags&=~SONORK_CONSOLE_EXPORT_F_SELECTION;

	EV.tag = pExportTag;
	if(EV.flags&SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS)
	{
		if(!GetExportComments(str, (EV.flags&SONORK_CONSOLE_EXPORT_F_SELECTION)) )
		{
			fclose(EV.file);
			return false;
		}
	}

	SetHintModeLang(GLS_MS_PWAIT,true);

	EV.section 	= SONORK_CONSOLE_EXPORT_SECTION_START;
	EV.data.ptr = NULL;
	GenExportEvent(&EV);

	EV.section 			= SONORK_CONSOLE_EXPORT_SECTION_COMMENTS;
	if(EV.flags&SONORK_CONSOLE_EXPORT_F_ASK_COMMENTS)
	{
		EV.data.comments	= str.CStr();
	}
	else
		EV.data.comments	= "";
	GenExportEvent(&EV);

	EV.section 			= SONORK_CONSOLE_EXPORT_SECTION_LINE;
	if( !(EV.flags & SONORK_CONSOLE_EXPORT_F_SELECTION) )
	{
		mi=cache->Lines();
		for(ci=0 ; ci<mi ; ci++)
		{
			if((EV.data.line = cache->Get( ci , NULL , NULL ))!=NULL)
				GenExportEvent(&EV);
		}
	}
	else
	{
		SortSelection();
		InitEnumSelection(I);
		while( (ci=EnumNextSelection(I)) != SONORK_INVALID_INDEX)
		{
			if((EV.data.line = cache->Get( ci , NULL , NULL ))!=NULL)
				GenExportEvent(&EV);
		}
	}
	EV.section 	= SONORK_CONSOLE_EXPORT_SECTION_END;
	EV.data.ptr = NULL;
	GenExportEvent(&EV);
	fclose(EV.file);
	ClearHintMode();
	return true;
}

// ----------------------------------------------------------------------------

SONORK_CONSOLE_EXPORT_FORMAT
 TSonorkConsole::GetExportPath(TSonorkShortString& path, SONORK_C_CSTR szDirKeyName, SONORK_C_CSTR szFileName)
{
	OPENFILENAME 	OFN;
	char			config_key_name[48];
	TSonorkShortString	default_dir;
	SONORK_CONSOLE_EXPORT_FORMAT format;

	format=SONORK_CONSOLE_EXPORT_HTML;

	if( szDirKeyName != NULL )
	{
		wsprintf(config_key_name,szSonorkOpenSaveDirKey,szDirKeyName);
		SonorkApp.ReadProfileItem(config_key_name, &default_dir );
	}

	path.SetBufferSize(SONORK_MAX_PATH+2);
	strcpy( path.Buffer(), szFileName );
	OFN.lStructSize 	= sizeof(OFN);
	OFN.hwndOwner       	= Handle();
	OFN.hInstance		= SonorkApp.Instance();//NULL;
	OFN.lpstrFilter		= "Html\x0*.htm;*.html\x0Text\x0*.txt\x0\x0";
	OFN.lpstrCustomFilter = NULL;
	OFN.nMaxCustFilter	= 0;
	OFN.nFilterIndex	= 1;
	OFN.lpstrFile		= path.Buffer();
	OFN.nMaxFile		= SONORK_MAX_PATH;
	OFN.lpstrFileTitle	= NULL;
	OFN.nMaxFileTitle	= 0;
	OFN.lpstrInitialDir	= default_dir.CStr();
	OFN.lpstrTitle		= SonorkApp.LangString(GLS_OP_EXPORT);
	OFN.Flags			= OFN_EXPLORER
						| OFN_LONGNAMES
						| OFN_NOCHANGEDIR
						| OFN_NOREADONLYRETURN
						| OFN_OVERWRITEPROMPT
						| OFN_PATHMUSTEXIST;
	OFN.nFileOffset		=
	OFN.nFileExtension	= 0;
	OFN.lpstrDefExt		= "htm";
	OFN.lCustData		= NULL;
	OFN.lpfnHook		= NULL;
	OFN.lpTemplateName	= 0;
	if(!GetSaveFileName(&OFN))
		return SONORK_CONSOLE_EXPORT_NONE;
	if(!OFN.nFileExtension)
		strcat(path.Buffer(),"htm");
	else
	if(!strnicmp(path.CStr()+OFN.nFileExtension,"txt",3))
		format = SONORK_CONSOLE_EXPORT_TEXT;
	else
		strcpy(path.Buffer()+OFN.nFileExtension,"htm");

	if( szDirKeyName != NULL )
	{
		default_dir.Set( path );
		*(default_dir.Buffer() + OFN.nFileOffset)=0;
		SonorkApp.WriteProfileItem(config_key_name, &default_dir );
	}
	return format;
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::GetExportComments(TSonorkShortString& S, BOOL selection)
{
	TSonorkInputWin W(this);
	UINT	aux;
	W.flags=SONORK_INPUT_F_LONG_TEXT;
	W.help.SetBufferSize(96);
	W.sign=SKIN_SIGN_FILE;
	wsprintf(W.help.Buffer()
		,"%s\n(%s)"
		,SonorkApp.LangString(GLS_OP_EXPORT)
		,SonorkApp.LangString(selection?GLS_LB_SELTD:GLS_LB_ALL));
	W.prompt.Set(SonorkApp.LangString(GLS_LB_COMMENTS));
	aux = W.Execute();
	if(aux == IDOK)
	{
		S.Set( W.input.CStr() );
		return true;
	}
	return false;

}

// ----------------------------------------------------------------------------

TSonorkConsole::TSonorkConsole(TSonorkWin*parent
	, TSonorkCCache* 			pcache
	, TSonorkConsoleCallbackPtr cb_ptr
	, void *				cb_tag
	, UINT 					sys_flags)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_CONSOLE
	,sys_flags|SONORK_WIN_SF_NO_CLOSE)
{
	output_mode		=SONORK_CONSOLE_OUTPUT_NONE;
	toolbar.flags		=0;
	cache			=pcache;
//	i_height		=SONORK_CONSOLE_DEFAULT_INPUT_HEIGHT;
	cb.ptr			=cb_ptr;
	cb.tag			=cb_tag;
	ignoreVKey = 0;

}

// ----------------------------------------------------------------------------

TSonorkConsole::~TSonorkConsole()
{}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkConsole::OnCtlColor( UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg   == WM_CTLCOLORSTATIC )
	if( lParam == (LPARAM)output.hwnd )
		return sonork_skin.OnCtlColorMsgView(wParam);
	return TSonorkWin::OnCtlColor(uMsg,wParam,lParam);

}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::Clear()
{
	cache->Clear(true);
	history_win->AfterClear();
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::OnCreate()
{
	DWORD aux;
	assert( cache != NULL );

	output.hwnd = GetDlgItem(IDC_CONSOLE_OUTPUT);
	output.ctrl.AssignCtrl(this, IDC_CONSOLE_OUTPUT);
	aux = ::GetWindowLong( output.hwnd, GWL_EXSTYLE );
	aux&=~(WS_EX_WINDOWEDGE|WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME);
	::SetWindowLong( output.hwnd, GWL_EXSTYLE , aux);
	aux = ::GetWindowLong( output.hwnd, GWL_STYLE );
	aux|=WS_BORDER;
	::SetWindowLong( output.hwnd, GWL_STYLE , aux);


	// Tool bar
	SetupToolBar();
	history_win = new TSonorkHistoryWin(
			this
		,	cbHistoryEvent
		,	this
		, 	cache );
	history_win->Create();
	SetOutputMode(SONORK_CONSOLE_OUTPUT_HISTORY,0);
	::SendMessage(output.hwnd
		,WM_SETFONT
		,(WPARAM)sonork_skin.Font(SKIN_FONT_LARGE)
		,0);
	return true;
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::OnAfterCreate()
{
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::OnBeforeDestroy()
{
	output.ctrl.ReleaseCtrl();
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::OpenView(DWORD flags)
{
	SetOutputMode(SONORK_CONSOLE_OUTPUT_VIEW , flags );
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::CloseView()
{
	SetOutputMode(SONORK_CONSOLE_OUTPUT_HISTORY
		, toolbar.flags	// Leave toolbar flags as they are
		);
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SetToolBarFlags(UINT new_flags)
{
	if((toolbar.flags&SONORK_CONSOLE_TBM_BUTTONS)!=(new_flags&SONORK_CONSOLE_TBM_BUTTONS))
	{
		EnableToolBar(SONORK_CONSOLE_BUTTON_PROCESS
			,new_flags&SONORK_CONSOLE_TBF_PROCESS);
		if((new_flags&(SONORK_CONSOLE_TBF_PROCESS|SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS))
		 == (SONORK_CONSOLE_TBF_PROCESS|SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS ) )
		{
			if( !(toolbar.flags&SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS) )
			{
				SetToolBarIconAndStyle(SONORK_CONSOLE_BUTTON_PROCESS
					, SKIN_ICON_OK_HIGHLIGHTED
					, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE);
			}
		}
		else
		{
			new_flags&=~SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS;
			if( toolbar.flags&SONORK_CONSOLE_TBF_DEFAULT_IS_PROCESS )
			{
				SetToolBarIconAndStyle(SONORK_CONSOLE_BUTTON_PROCESS
					, SKIN_ICON_OK
					, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE);
			}
		}
		EnableToolBar(SONORK_CONSOLE_BUTTON_DELETE  ,new_flags&SONORK_CONSOLE_TBF_DELETE);
		EnableToolBar(SONORK_CONSOLE_BUTTON_01	,new_flags&SONORK_CONSOLE_TBF_BUTTON_01);
		EnableToolBar(SONORK_CONSOLE_BUTTON_02  ,new_flags&SONORK_CONSOLE_TBF_BUTTON_02);
		EnableToolBar(SONORK_CONSOLE_BUTTON_03	,new_flags&SONORK_CONSOLE_TBF_BUTTON_03);
	}
	toolbar.flags = new_flags;
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SetOutputMode(SONORK_CONSOLE_OUTPUT_MODE new_mode, DWORD flags)
{
	SetToolBarFlags( flags );
	if(output_mode == new_mode)
		return;
	output_mode=new_mode;
	switch( new_mode )
	{
		case SONORK_CONSOLE_OUTPUT_HISTORY:
			::ShowWindow(output.hwnd,SW_HIDE);
			::ShowWindow(toolbar.hwnd,SW_HIDE);
			history_win->ShowWindow(SW_SHOW);
			break;

		case SONORK_CONSOLE_OUTPUT_VIEW:
			history_win->ShowWindow(SW_HIDE);
			::ShowWindow(output.hwnd,SW_SHOW);
			::ShowWindow(toolbar.hwnd,SW_SHOW);
			break;

		default:
			return;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::OnSize(UINT size_type)
{
	if(size_type==SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
		RealignControls("");
}


// ----------------------------------------------------------------------------

void
 TSonorkConsole::RealignControls(SONORK_C_CSTR )
{
	HDWP defer_handle;
	const int H = (int)Height() ;
	const int W = (int)Width();
	const int sW = W;
	int	output_height;
	int y;

	if( !history_win )return;
	defer_handle = BeginDeferWindowPos( 4 );


	output_height = H
				-toolbar.size.cy
				-SPACING;

	y = 0;
	defer_handle = DeferWindowPos(defer_handle
		,history_win->Handle()
		,NULL
		,0
		,y
		,sW
		,output_height + toolbar.size.cy + SPACING
		,SWP_NOZORDER|SWP_NOACTIVATE);


	defer_handle = DeferWindowPos(defer_handle
		,output.hwnd
		,NULL
		,0
		,y
		,sW
		,output_height - SPACING
		,SWP_NOZORDER|SWP_NOACTIVATE);

	y+= output_height + SPACING;

	defer_handle = DeferWindowPos(defer_handle
		,toolbar.hwnd
		,NULL
		,W-toolbar.size.cx
		,y
		,toolbar.size.cx
		,toolbar.size.cy
		,SWP_NOZORDER|SWP_NOACTIVATE);
			
	 EndDeferWindowPos(defer_handle);
}

// ----------------------------------------------------------------------------

bool
TSonorkConsole::OnDrawItem(DRAWITEMSTRUCT*)
{
	return false;
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SetToolBarIconAndStyle(SONORK_CONSOLE_BUTTON b,SKIN_ICON icon,DWORD style)
{
	TBBUTTONINFO BI;
	BI.cbSize  = sizeof(TBBUTTONINFO);
	BI.dwMask  = TBIF_IMAGE|TBIF_STYLE;
	BI.iImage  = icon;
	BI.fsStyle = (BYTE)(style|TBSTYLE_AUTOSIZE) ;
	::SendMessage(toolbar.hwnd,TB_SETBUTTONINFO,b,(LPARAM)&BI);
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::EnableToolBar(SONORK_CONSOLE_BUTTON b,BOOL e)
{
	SetToolBarState(b,e?TBSTATE_ENABLED:0);
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SetToolBarState(SONORK_CONSOLE_BUTTON b,UINT state)
{
	ToolBar_SetButtonState(toolbar.hwnd,b,state);
}

// ----------------------------------------------------------------------------

UINT
 TSonorkConsole::GetToolBarState(SONORK_CONSOLE_BUTTON b)
{
	return ToolBar_GetButtonState(toolbar.hwnd,b);
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::OnToolTipText(UINT id, HWND , TSonorkWinToolTipText&TTT )
{
	switch(id)
	{
		case SONORK_CONSOLE_BUTTON_01:
		case SONORK_CONSOLE_BUTTON_02:
		case SONORK_CONSOLE_BUTTON_03:
			cb.ptr(cb.tag
				,SONORK_CONSOLE_EVENT_TOOLBAR_TOOL_TIP
				,id
				,(void*)&TTT);
			break;
		case SONORK_CONSOLE_BUTTON_PROCESS:
			TTT.gls = GLS_OP_EXEOPEN;
			break;
		case SONORK_CONSOLE_BUTTON_DELETE:
			TTT.gls = GLS_OP_EXEDEL;
			break;
		case SONORK_CONSOLE_BUTTON_CLOSE:
			sprintf(TTT.str,"%s (ESC)",SonorkApp.LangString(GLS_OP_CLSMSG));
		break;
		case SONORK_CONSOLE_BUTTON_PREVIOUS:
			TTT.gls = GLS_OP_PRVMSG;
			break;
		case SONORK_CONSOLE_BUTTON_NEXT:
			TTT.gls = GLS_OP_NXTMSG;
			break;
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::On_GcLineDrag(DWORD line_no)
{
	TSonorkDropSourceData 	*SD;
	TSonorkCCacheEntry 		*CL;
	TSonorkClipData         *sCD;
	SONORK_C_CSTR			line_text;
	DWORD				aux=0;

	SD = new TSonorkDropSourceData;
	sCD = SD->GetClipData();
	if( history_win->SelectionActive() )
	{
		TSonorkListIterator I;
		TSonorkClipData    *nCD;
		sCD->SetSonorkClipDataQueue();
		history_win->SortSelection();
		history_win->InitEnumSelection(I);
		while((line_no = history_win->EnumNextSelection(I))!=SONORK_INVALID_INDEX)
		{
			CL = cache->Get( line_no , &line_text , NULL );
			if( CL == NULL )continue;
			nCD = new TSonorkClipData;
			nCD->SetCStr( SONORK_CLIP_DATA_TEXT , line_text );
			if(sCD->AddSonorkClipData( nCD ))
				aux++;
			nCD->Release();
		}
	}
	else
	{
		CL = cache->Get( line_no , &line_text , NULL );
		if( CL != NULL )
			sCD->SetCStr( SONORK_CLIP_DATA_TEXT , line_text );
					aux=1;
	}
	if( aux > 0 )
	{
		aux=0;
		SetWinUsrFlag(SONORK_CONSOLE_WF_IN_DRAG_LOOP);
		SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
		ClearWinUsrFlag(SONORK_CONSOLE_WF_IN_DRAG_LOOP);
	}
	SD->Release();

}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkConsole::OnNotify(WPARAM,TSonorkWinNotify*N)
{
	if( N->hdr.hwndFrom == toolbar.hwnd )
	if( N->hdr.code	  == TBN_DROPDOWN )
	{
		if(N->tbn.iItem>=SONORK_CONSOLE_BUTTON_FIRST_CUSTOM
			&& N->tbn.iItem<=SONORK_CONSOLE_BUTTON_LAST_CUSTOM)
		{
			return cb.ptr(cb.tag
					,SONORK_CONSOLE_EVENT_TOOLBAR_NOTIFY
					,N->tbn.iItem
					,&N->hdr);
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::OnCommand(UINT id,HWND , UINT code)
{
	if( code != BN_CLICKED)
		return false;
	if(id>=SONORK_CONSOLE_BUTTON_FIRST&&id<=SONORK_CONSOLE_BUTTON_LAST)
	{
		switch(id)
		{
			case SONORK_CONSOLE_BUTTON_01:
			case SONORK_CONSOLE_BUTTON_02:
			case SONORK_CONSOLE_BUTTON_03:
			{
				TBNOTIFY 		tBN;
				tBN.iItem 		= id;
				tBN.hdr.code    = code;
				cb.ptr(cb.tag
					,SONORK_CONSOLE_EVENT_TOOLBAR_NOTIFY
					,id
					,(void*)&tBN);
			}
			break;

			case SONORK_CONSOLE_BUTTON_PROCESS:
			case SONORK_CONSOLE_BUTTON_DELETE:
				cb.ptr(cb.tag
					,SONORK_CONSOLE_EVENT_PROCESS
					,id == SONORK_CONSOLE_BUTTON_PROCESS
					,NULL);
				break;

			case SONORK_CONSOLE_BUTTON_CLOSE:
				if(GetOutputMode() == SONORK_CONSOLE_OUTPUT_VIEW)
				{
					if(!cb.ptr(cb.tag
						,SONORK_CONSOLE_EVENT_CLOSE_VIEW
						,0
						,NULL))
					{
						CloseView();
					}
				}
				break;
				

			case SONORK_CONSOLE_BUTTON_PREVIOUS:
			case SONORK_CONSOLE_BUTTON_NEXT:
				ScrollLine(
					id==SONORK_CONSOLE_BUTTON_PREVIOUS?-1:1
				);
				break;

			default:
				return false;
		}
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------

DWORD SONORK_CALLBACK
 TSonorkConsole::cbHistoryEvent(void*param,struct TSonorkHistoryWinEvent*E)
{
	TSonorkConsole *_this = (TSonorkConsole*)param;
	return _this->cb.ptr(_this->cb.tag ,SONORK_CONSOLE_EVENT_HISTORY_EVENT,0,E);
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::MarkLineAsRead(DWORD line_no, TSonorkCCacheEntry *CL)
{
	if( CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_UNREAD )
	{
		CL->tag.v[SONORK_CCACHE_TAG_FLAGS] &=~SONORK_APP_CCF_UNREAD;
		history_win->Set(line_no , CL->tag , NULL);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkConsole::OnCtlWinMsg(TSonorkWinCtrl*WC,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if( uMsg == WM_GETDLGCODE )
	{
		if( lParam )
		{
			MSG *msg = (MSG*)lParam;
			if( msg->message == WM_KEYDOWN)
			{
				if(msg->wParam == VK_ESCAPE)
				{
					return DLGC_WANTMESSAGE;
				}
			}
		}
	}
	else
	if(	(uMsg >= WM_KEYFIRST && uMsg <=WM_KEYLAST) )
	{
		switch(uMsg)
		{
			case WM_CHAR:
				if( ProcessVKey(wParam , SONORK_CONSOLE_VKEY_WM_CHAR) )
					return 0;
				break;

			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				if( ignoreVKey == 0 || ignoreVKey == wParam )
				{
					if( ProcessVKey(wParam , 0) )
					{
						ignoreVKey = wParam;
						return 0;
					}
				}
				break;

			case WM_SYSKEYUP:
			case WM_KEYUP:
				if(ignoreVKey == wParam)
				{
					//TRACE_DEBUG("IGNORING KEY %x",wParam);
					ignoreVKey=0;
					return 0;
				}
				break;

			default:
				break;


		}
	}
	else
	if( uMsg == WM_SETFOCUS )
	{
		// Must reset ignore vkey when getting focus
		// (because we could have lost it while IgnoreVKey was non zero)
		ignoreVKey = 0;
		TRACE_DEBUG("WM_SETFOCUS");

	}
	return WC->DefaultProcessing(uMsg,wParam,lParam);
}

// ----------------------------------------------------------------------------

bool
 TSonorkConsole::ProcessVKey(DWORD vKey, DWORD vKeyFlags)
{
	BOOL ctrl_down;
	bool rv;
	if((ctrl_down = (GetKeyState(VK_CONTROL)&0x80000))!=0)
		vKeyFlags|=SONORK_CONSOLE_VKEY_CONTROL;

	for(;;)
	{
		if( !(vKeyFlags&SONORK_CONSOLE_VKEY_WM_CHAR) )
		{
			if(vKey==VK_UP 	|| vKey==VK_DOWN
			|| vKey==VK_PRIOR	|| vKey == VK_NEXT)
			{
				if( ctrl_down  )
				{
					if(vKey==VK_UP || vKey==VK_DOWN)
						ScrollLine(vKey == VK_UP?-1:1);
					else
						ScrollPage(vKey == VK_PRIOR?-1:1);
					rv = true;
				}
				else
					rv = false;
				break;
			}
			else
			if( !(vKeyFlags&SONORK_CONSOLE_VKEY_WM_CHAR) )
			{
				if( vKey == VK_ESCAPE )
				{
					TRACE_DEBUG("CONSOLE_VK_ESCAPE");
					if(output_mode == SONORK_CONSOLE_OUTPUT_HISTORY
					&& history_win->SelectionActive())
					{
						history_win->ClearSelection();
						rv = true;
						break;
					}
				}
			}
		}
		rv = cb.ptr(cb.tag,SONORK_CONSOLE_EVENT_VKEY,vKey,(void*)vKeyFlags )!=0;
		break;
	}
	//TRACE_DEBUG("PROC(KEY=%x, FLG=%x)=%u",vKey,vKeyFlags,rv);
	return rv;
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::ScrollLine( int dir )
{
	(dir>=0?history_win->FocusLineDown():history_win->FocusLineUp());
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::ScrollPage( int dir )
{
	if(output_mode != SONORK_CONSOLE_OUTPUT_HISTORY)return ;
	if( dir >= 0)
		history_win->FocusPageDown();
	else
		history_win->FocusPageUp();
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SetupToolBar()
{
	static TSonorkWinToolBarButton	btn_info[SONORK_CONSOLE_BUTTONS]=
	{
		{	SONORK_CONSOLE_BUTTON_03
			, SKIN_ICON_NONE
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE  }
	,	{	SONORK_CONSOLE_BUTTON_02
			, SKIN_ICON_NONE
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_01
			, SKIN_ICON_NONE
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_DELETE
			, SKIN_ICON_CANCEL
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_PROCESS
			, SKIN_ICON_OK
			, GLS_NULL
			, 0
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_PREVIOUS      | SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_UP_ARROW
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_NEXT
			, SKIN_ICON_DOWN_ARROW
			, GLS_NULL
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	,	{	SONORK_CONSOLE_BUTTON_CLOSE     	| SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_CLOSE
			, GLS_OP_CLOSE
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON |TBSTYLE_AUTOSIZE }
	};


	toolbar.hwnd=TSonorkWin::CreateToolBar(Handle()
			,TOOL_BAR_ID
			,	 WS_VISIBLE
				| TBSTYLE_TOOLTIPS
				| TBSTYLE_FLAT
				| TBSTYLE_LIST
				| CCS_NOPARENTALIGN
				| CCS_NODIVIDER
				| CCS_NORESIZE
			, SONORK_CONSOLE_BUTTONS
			, btn_info
			, &toolbar.size);
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::DelPreviousLines(DWORD line_no)
{
	CloseView();
	SetHintModeLang(GLS_MS_PWAIT,true);
	history_win->DelPreviousLines(line_no);
	ClearHintMode();
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::DelAllLines()
{
	CloseView();
	SetHintModeLang(GLS_MS_PWAIT,true);
	history_win->DelAllLines();
	ClearHintMode();
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::DelSelectedLines()
{
	CloseView();
	history_win->DelSelectedLines();
}

// ----------------------------------------------------------------------------

void
 TSonorkConsole::SepLine( FILE *file , UINT mi)
{
	for(UINT i=0;i<mi;i++)fprintf(file,"-----");
	fputc('\n',file);
}

// ----------------------------------------------------------------------------

SONORK_C_CSTR
 TSonorkConsole::WinToRGB( DWORD win_bgr, char *buffer)
{
	DWORD rgb ;
	rgb = ( (win_bgr&0x0000ff)<<16 )
		| ( (win_bgr&0xff0000)>>16 )
		| ( (win_bgr&0x00ff00)     );
	wsprintf(buffer,"%06x",rgb);
	return buffer;
}


