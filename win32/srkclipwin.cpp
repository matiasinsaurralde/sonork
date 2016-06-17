#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkclipwin.h"
#include "srkinputwin.h"
#include "srk_codec_file.h"
#include "srk_url_codec.h"
#include "srk_crypt.h"

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


#define IN_DRAG_LOOP			SONORK_WIN_F_USER_01
#define NOT_FIRST_LOAD			SONORK_WIN_F_USER_02
#define DATA_UNSAVED			SONORK_WIN_F_USER_03
#define ALIGNED				SONORK_WIN_F_USER_04
#define POKE_LOAD			SONORK_WIN_POKE_01
#define MAX_CLIP_FILE_NAME_SIZE		64
#define MAX_TEXT_LENGTH			128
#define	MAX_CLIPBOARD_ITEMS 		(1024*4)
#define CLIP_CONSOLE_TEXT_SIZE		512
#define CLIP_CONSOLE_CACHE_SIZE		112
#define TOOL_BAR_ID			1000
#define TOOL_BAR_BUTTONS		(TOOL_BAR_BUTTON_LIMIT - TOOL_BAR_BUTTON_BASE)

enum TOOL_BAR_BUTTON
{
	TOOL_BAR_BUTTON_BASE
,	TOOL_BAR_BUTTON_LOAD	= TOOL_BAR_BUTTON_BASE
,	TOOL_BAR_BUTTON_SAVE
,	TOOL_BAR_BUTTON_ADD
,	TOOL_BAR_BUTTON_STAY_ON_TOP
, 	TOOL_BAR_BUTTON_LIMIT
};

TSonorkClipWin::TSonorkClipWin()
	:TSonorkWin(NULL
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_CLIP
	|SONORK_WIN_DIALOG
	|IDD_BLANK_CAPSTATUS
	,SONORK_WIN_SF_NO_WIN_PARENT)
{
}

bool
 TSonorkClipWin::OnCreate()
{
	TSonorkTempBuffer 	path(SONORK_MAX_PATH);
	char			name[64];
	SIZE			size;
	bool			db_reset;
	HWND			history_hwnd;
	static TSonorkWinToolBarButton	tool_bar_buttons[TOOL_BAR_BUTTONS]=
	{
		{	TOOL_BAR_BUTTON_LOAD
			, SKIN_ICON_FILE_UPLOAD
			, GLS_OP_OPEN
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
,		{	TOOL_BAR_BUTTON_SAVE
			, SKIN_ICON_FILE_DOWNLOAD
			, GLS_OP_STORE
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
,		{	TOOL_BAR_BUTTON_ADD			| SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_ADD
			, GLS_OP_ADD
			, TBSTATE_ENABLED
			, TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE }
,		{	TOOL_BAR_BUTTON_STAY_ON_TOP | SONORK_WIN_TOOLBAR_PREFIX_SEPARATOR
			, SKIN_ICON_PIN
			, GLS_OP_STAYTOP
			, TBSTATE_ENABLED
			, TBSTYLE_CHECK|TBSTYLE_AUTOSIZE  }
	};

	dta	= new TSonorkClipData;


	SetCaptionIcon( SKIN_HICON_NOTES );
	SetWindowText( GLS_LB_CLPBRD );
	wsprintf( name,"~%u.%u_%s(%s)"
		, SonorkApp.ProfileUserId().v[0]
		, SonorkApp.ProfileUserId().v[1]
		, "clip"
		, szSonorkAppMode);
	SonorkApp.GetDirPath(path.CStr(),SONORK_APP_DIR_TEMP,name);
	db_reset=true;
	db.Open( SonorkApp.ProfileUserId() , path.CStr() , db_reset);
	SonorkApp.GetTempPath(path.CStr(), name ,NULL);
	cache = new TSonorkCCache(CLIP_CONSOLE_TEXT_SIZE
				, CLIP_CONSOLE_CACHE_SIZE
				, CCacheCallback
				, this);
	cache->Open( path.CStr() );
	cache->Clear( true );


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
		, tool_bar_buttons
		, &size);
	toolbar.height = size.cy;

	status.hwnd 	= GetDlgItem( IDC_BLANK_STATUS );
	status.height   = GetCtrlHeight( IDC_BLANK_STATUS );

	console=new TSonorkConsole(this
			,cache
			,ConsoleCallback
			,this
			,0);

	if( !console->Create() )
	{
		delete console;
		delete cache;
		return false;
	}
	history_hwnd = console->HistoryWin()->Handle();
	console->SetMarginsEx(SKIN_ICON_SW+1,1);
	console->SetPaddingEx(1,2);
	console->EnableLineDrag(true);

	drop_target = new TSonorkDropTarget( Handle()  , history_hwnd);

	console->SetDefColors(SKIN_COLOR_CLIP);
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_CLIP,true,"ClipWin");
	if( !TestWinUsrFlag(ALIGNED) )
		RealignControls();

	if(!SonorkApp.ProfileRunFlags().Test(SONORK_PRF_CLIPBOARD_INITIALIZED))
	{
		SonorkApp.ProfileRunFlags().Set(SONORK_PRF_CLIPBOARD_INITIALIZED);
		DL_SetDefaultPath();
	}
	else
	{
		// Read the current path from the
		// szSonorkOpenSaveDir_ClipPath key
		if(SonorkApp.ReadProfileItem(
			szSonorkOpenSaveDir_ClipPath
			,&dataListPath)!=SONORK_RESULT_OK)
			DL_SetDefaultPath();
	}

	::SetWindowLong(history_hwnd
		, GWL_STYLE
		, ::GetWindowLong(history_hwnd, GWL_STYLE)
		  |WS_TABSTOP);
	SendPoke(POKE_LOAD,0);
	SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND , (LPARAM)history_hwnd);
	return true;
}
void TSonorkClipWin::DL_SetDefaultPath()
{
	TSonorkShortString tmp;
	char 	dir_name[48];

	tmp.SetBufferSize(SONORK_MAX_PATH);
	wsprintf(dir_name,"%u.%u\\%s"	,SonorkApp.ProfileUserId().v[0]
					,SonorkApp.ProfileUserId().v[1]
					,szPrivateDirClip);

	SonorkApp.GetDirPath(tmp.Buffer(),SONORK_APP_DIR_DATA,dir_name);

	wsprintf(dir_name
		,szSonorkOpenSaveDirKey
		,szSonorkOpenSaveDir_ClipPath);
	SonorkApp.WriteProfileItem(dir_name,&tmp);

	SonorkApp.CreateDirIfNotExists( tmp.CStr() );
	strcat( tmp.Buffer() , "\\default.clp");
	dataListPath.Set(tmp);
}

SONORK_C_CSTR TSonorkClipWin::GetCleanFileName(SONORK_C_STR pBuffer)
{
	const char		*fname;
	char			*ext;
	fname = SONORK_IO_GetFileName( dataListPath.CStr() );
	lstrcpyn(pBuffer,fname,MAX_CLIP_FILE_NAME_SIZE);
	ext = (char*)SONORK_IO_GetFileExt( pBuffer );
	if(*ext=='.')*ext=0;
	return pBuffer;
}

void	TSonorkClipWin::UpdateCaption()
{
	TSonorkTempBuffer 	tmp(MAX_CLIP_FILE_NAME_SIZE+128);
	char			name[MAX_CLIP_FILE_NAME_SIZE];
	wsprintf(tmp.CStr()
		,"%s:%.64s"
		, SonorkApp.LangString(GLS_LB_CLPBRD)
		, GetCleanFileName( name ) );
	SetWindowText( tmp.CStr() );
}


BOOL	TSonorkClipWin::OnQueryClose()
{
	if(TestWinUsrFlag(DATA_UNSAVED))
	{
		UINT id;
		id =MessageBox(GLS_QU_SAVEXIT,GLS_LB_CLPBRD,MB_YESNOCANCEL|MB_ICONQUESTION);
		if(id==IDCANCEL)
			return true;
		if(id==IDYES)
			DL_Save();
	}
	return false;
}


void	TSonorkClipWin::OnInitMenu(HMENU hmenu)
{
	if( hmenu == SonorkApp.ClipPopupMenu() )
	{
		BOOL selection;
		UINT enable;
		char	tmp[96];
		selection=console->Selection().Active();
		enable=(selection?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND;
		EnableMenuItem(hmenu,CM_MSGS_CLEAR_SEL,enable);
		wsprintf(tmp,"%s (%s)"
			,SonorkApp.LangString(GLS_OP_EXPORT)
			,SonorkApp.LangString(selection?GLS_LB_SELTD:GLS_LB_ALL));
		TSonorkWin::SetMenuText(hmenu, CM_EXPORT, tmp);

		CheckMenuItem(hmenu,CM_MSGS_SINGLE_LINE_MODE
			,MF_BYCOMMAND|(console->GetMaxScanLines()==1?MF_CHECKED:MF_UNCHECKED));
	}
}

void
 TSonorkClipWin::CmdAdd()
{
	TSonorkInputWin W(this);
	W.flags=SONORK_INPUT_F_LONG_TEXT;
	W.help.SetBufferSize(96);
	W.sign=SKIN_SIGN_FILE;
	wsprintf(W.help.Buffer()
		,"%s (%s)"
		,SonorkApp.LangString(GLS_OP_ADD)
		,SonorkApp.LangString(GLS_LB_CLPBRD));
	W.prompt.Set(SonorkApp.LangString(GLS_LB_TXT));
	if(W.Execute() == IDOK)
	if(*W.input.CStr()!=0)
	{
		dta->SetCStr(
			SONORK_IsUrl( W.input.CStr() )
			?SONORK_CLIP_DATA_URL
			:SONORK_CLIP_DATA_TEXT
			, W.input.CStr() );
		DL_Add(dta);
	}
}

bool
	TSonorkClipWin::OnCommand(UINT id,HWND hwnd, UINT code)
{
	DWORD 	aux;
	if( hwnd == toolbar.hwnd )	// ToolBar
	{
		if(code != BN_CLICKED)
			return false;
		switch( id )
		{

//			case TOOL_BAR_BUTTON_CLEAR:DL_Clear();CDB.SetStatus_RecordCount();break;
			case TOOL_BAR_BUTTON_ADD:
				CmdAdd();
			break;
			
			case TOOL_BAR_BUTTON_STAY_ON_TOP:
				aux = ::SendMessage(toolbar.hwnd
					,TB_GETSTATE
					,TOOL_BAR_BUTTON_STAY_ON_TOP
					,0)&TBSTATE_CHECKED;
				SetStayOnTop(aux);
			break;

			case TOOL_BAR_BUTTON_SAVE:
			{
				if( SonorkApp.IsControlKeyDown() )
				{
					if(!SonorkApp.GetSavePath(Handle()
						, dataListPath
						, dataListName.CStr()
						, GLS_OP_STORE
						, szSonorkOpenSaveDir_ClipPath
						, "clp"
						,OFN_EXPLORER
						 | OFN_LONGNAMES
						 | OFN_NOCHANGEDIR
						 | OFN_NOREADONLYRETURN
						 | OFN_HIDEREADONLY
						 | OFN_PATHMUSTEXIST))
						break;
				}
				DL_Save();
			}
			break;
			case TOOL_BAR_BUTTON_LOAD:
			{
				if( SonorkApp.IsControlKeyDown() )
					DL_SetDefaultPath();
				else
				if(!SonorkApp.GetLoadPath(Handle()
					, dataListPath
					, dataListName.CStr()
					, GLS_OP_LOAD
					, szSonorkOpenSaveDir_ClipPath
					, "clp"
					, OFN_EXPLORER
					  | OFN_LONGNAMES
					  | OFN_NOCHANGEDIR
					  | OFN_PATHMUSTEXIST
					  | OFN_HIDEREADONLY
					))
					break;
				DL_Load();
			}
			break;
			default:
				return false;
		}
		return true;
	}
	else
	if( hwnd == NULL ) // Menu
	{
		switch( id )
		{
			case CM_MSGS_TOGGLE_SEL:
				console->SelectToggleFocused();
				break;

			case CM_MSGS_CLEAR_SEL:
				console->ClearSelection();
				break;

			case CM_DELETE:
				console->DelSelectedLines();
				SetWinUsrFlag(DATA_UNSAVED);
				break;

			case CM_CLIPBOARD_COPY:
				console->CopyToClipboard(0);
				break;

			case CM_EXPORT:
				DoExport();
				break;

			case CM_MSGS_SINGLE_LINE_MODE:
				console->SetMaxScanLines(
					console->GetMaxScanLines() == 1
					?SONORK_HIST_WIN_DEFAULT_MAX_SCAN_LINES
					:1);
				break;

			default:
				return false;

		}
		return true;
	}
	return false;
}


LRESULT	TSonorkClipWin::OnDragDrop(SONORK_DRAG_DROP_EVENT event,LPARAM lParam)
{
//	DWORD aux;
	switch( event )
	{
		case SONORK_DRAG_DROP_INIT:
			//SetDropTarget(NULL);
			break;
		case SONORK_DRAG_DROP_QUERY:
		if(!TestWinUsrFlag(IN_DRAG_LOOP))
//		if(  ListView_GetItemCount(datalist.hwnd) < MAX_CLIPBOARD_ITEMS)
		if(  console->Lines() < MAX_CLIPBOARD_ITEMS)
		{
			SonorkApp.ProcessDropQuery(
				(TSonorkDropQuery*)lParam
				,	(
					 SONORK_DROP_ACCEPT_F_SONORKCLIPDATA
					|SONORK_DROP_ACCEPT_F_FILES
					|SONORK_DROP_ACCEPT_F_URL
					|SONORK_DROP_ACCEPT_F_TEXT
					)
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
			//SetDropTarget( NULL );
		break;
		case SONORK_DRAG_DROP_EXECUTE:
		{
			TSonorkDropExecute*E=(TSonorkDropExecute*)lParam;
			TSonorkClipData	*CD;

			CD = SonorkApp.ProcessDropExecute( E );
			if( CD )
			{
				TSonorkListIterator I;
				TSonorkClipData	*nCD;
				if(CD->DataType() == SONORK_CLIP_DATA_LIST)
				{
					const TSonorkClipDataQueue *Q;
					Q = CD->GetSonorkClipDataQueue();
					Q->BeginEnum(I);
					while( (nCD=Q->EnumNext(I)) != NULL )
						DL_Add(nCD);
					Q->EndEnum(I);
					//if(ListView_GetItemCount(datalist.hwnd)>=MAX_CLIPBOARD_ITEMS)
					if( console->Lines() >= MAX_CLIPBOARD_ITEMS)
						break;
				}
				else
				if(CD->DataType() == SONORK_CLIP_DATA_FILE_LIST)
				{
					const TSonorkShortStringQueue *Q;
					TSonorkShortString	*S;
					Q = CD->GetShortStringQueue();
					Q->BeginEnum(I);
					while( (S=Q->EnumNext(I)) != NULL )
					{
//						nCD = new TSonorkClipData();
						dta->SetCStr( SONORK_CLIP_DATA_FILE_NAME , S->CStr() );
						DL_Add(dta);
						//nCD->Release();

						//if(ListView_GetItemCount(datalist.hwnd)>=MAX_CLIPBOARD_ITEMS)
						if(console->Lines()>=MAX_CLIPBOARD_ITEMS)
							break;
					}
					Q->EndEnum(I);
				}
				else
					DL_Add(CD);
				CD->Release();
				SetStatus_RecordCount();
				//if( (aux = ListView_GetItemCount(datalist.hwnd)) > 0)
				//	ListView_EnsureVisible(datalist.hwnd,aux-1,false);
				console->MakeLastLineVisible();

			}
			//SonorkApp.ProcessUserDropData(view_item->UserData(),DD,true);
			//SetDropTarget( NULL );
		}
		break;
	}
	return 0L;

}
LRESULT	TSonorkClipWin::OnPoke(SONORK_WIN_POKE poke,LPARAM )
{
	if( poke == POKE_LOAD)
		DL_Load();
	return 0L;
}


void
 TSonorkClipWin::OnBeforeDestroy()
{
	DL_Clear();
	SonorkApp.OnSysDialogRun(this,SONORK_SYS_DIALOG_CLIP,false,"ClipWin");
	console->Destroy();
	delete cache;
	dta->Release();
	dta=NULL;
}

SONORK_RESULT TSonorkClipWin::DL_GetByDbIndex(DWORD db_index,TSonorkClipData*DD)
{
	return db.Get(db_index,DD);
}
SONORK_RESULT TSonorkClipWin::DL_Add(TSonorkClipData*DD)
{
	TSonorkCCacheEntry	CE;
	SONORK_RESULT 	result;
	CE.Clear();
	result = db.Add(DD,&CE.dat_index) ;
	if( result == SONORK_RESULT_OK )
	{
		CE.tag.v[SONORK_CCACHE_TAG_INDEX] = DD->DataType();
		console->Add(CE);
		SetWinUsrFlag(DATA_UNSAVED);
	}
	return result;
}
void	TSonorkClipWin::DL_Clear()
{
	console->Clear();
	db.Clear();
	SetWinUsrFlag(DATA_UNSAVED);
}



void	TSonorkClipWin::DL_Save()
{
	DWORD			ci,mi;
	TSonorkCCacheEntry*	CE;
	TSonorkCodecFileStream SF;

	SF.Open(dataListPath.CStr(),true);
	if( SF.IsOpen() )
	{
		TSonorkWin::SetStatus_PleaseWait(status.hwnd);
		::UpdateWindow(status.hwnd);
		mi = console->Lines();
		if( mi > MAX_CLIPBOARD_ITEMS)
			mi=MAX_CLIPBOARD_ITEMS;

		SF.BeginWrite(  SonorkApp.ProfileUserId(), ( (((DWORD)this)^mi) & 0x0fffffff )+1   );
		for(ci=0 ; ci<mi ; ci++ )
		{
			if( (CE=console->Get( ci , NULL ))==NULL)
				continue;
			if( db.Get( CE->dat_index, dta) != SONORK_RESULT_OK)
				continue;
			if( SF.WriteNext( dta ) != SONORK_RESULT_OK )
				break;
		}
		SF.EndWrite();
		SF.Close();
		TSonorkWin::SetStatus(status.hwnd,GLS_MS_SAVED,SKIN_HICON_FILE_DOWNLOAD);
		SetAuxTimer(2000);
		SonorkApp.WriteProfileItem(szSonorkOpenSaveDir_ClipPath
			,&dataListPath);
	}
	UpdateCaption();
	ClearWinUsrFlag(DATA_UNSAVED);
}

bool	TSonorkClipWin::DL_Load()
{
	TSonorkCCacheEntry	CE;
	DWORD			ci,mi;
	bool			error_found=true;
	TSonorkCodecFileStream SF;


	DL_Clear();

	dataListName.Set( SONORK_IO_GetFileName(dataListPath.CStr()) );
	SF.Open(dataListPath.CStr() , false );
	if( SF.IsOpen() )
	{
		CE.Clear();
		TSonorkWin::SetStatus_PleaseWait(status.hwnd);
		::UpdateWindow(status.hwnd);

		if( SF.BeginRead(SonorkApp.ProfileUserId()) == SONORK_RESULT_OK )
		{
			error_found=false;
			mi=SF.Items();
			if(mi>MAX_CLIPBOARD_ITEMS)
				mi=MAX_CLIPBOARD_ITEMS;
			for(ci=0 ; ci<mi ;ci++)
			{
				if(SF.ReadNext(dta) != SONORK_RESULT_OK)
					break;
				if(db.Add(dta,&CE.dat_index) == SONORK_RESULT_OK )
				{
					CE.tag.v[SONORK_CCACHE_TAG_INDEX] = dta->DataType();
					cache->Add(CE,NULL);
					continue;
				}
				error_found=true;
			}
			if(!error_found)
				SonorkApp.WriteProfileItem(
					szSonorkOpenSaveDir_ClipPath
					,&dataListPath);
			SF.EndRead();
		}
		SF.Close();
		console->AfterAdd();
	}
	if(TestWinUsrFlag(NOT_FIRST_LOAD))
	{
		SetStatus_RecordCount(SKIN_HICON_FILE_UPLOAD);
	}
	else
	{
		SetWinUsrFlag(NOT_FIRST_LOAD);
		TSonorkWin::SetStatus(status.hwnd,GLS_MS_CLPHLP,SKIN_HICON_INFO);
	}

	UpdateCaption();
	ClearWinUsrFlag(DATA_UNSAVED);

	return !error_found;
}
void
 TSonorkClipWin::OnTimer(UINT id)
{
	if( id == SONORK_WIN_AUX_TIMER_ID )
	{
		if( console->Selection().Items() )
			SetStatus_SelectCount();
		else
			SetStatus_RecordCount();
		KillAuxTimer();
	}
}


void
 TSonorkClipWin::GetCDText(TSonorkShortString& str)
{
	DWORD aux = dta->GetTextFormatMinSize();
	str.SetBufferSize( aux );
	GetCDText( str.Buffer(),aux);
}
void
 TSonorkClipWin::GetCDText(SONORK_C_STR str,UINT str_size)
{
	TSonorkId 	  	user_id;
	TSonorkExtUserData	*UD;
	if( dta->DataType() != SONORK_CLIP_DATA_USER_ID )
		dta->GetTextFormat(str,str_size);
	else
	{
		user_id.Set(dta->GetUserId());
		UD=SonorkApp.GetUser(user_id);
		if(UD!=NULL)
			wsprintf(str
				,"%u.%u %-.24s"
				,user_id.v[0]
				,user_id.v[1]
				,UD->display_alias.CStr());
		else
			user_id.GetStr(str);
	}
}

BOOL SONORK_CALLBACK
 TSonorkClipWin::CCacheCallback(void*tag
				,TSonorkCCacheEntry*pE
				,char*str
				,UINT str_size)
{
	TSonorkClipWin *_this = (TSonorkClipWin*)tag;
	if( _this->db.Get( pE->dat_index, _this->dta ) == SONORK_RESULT_OK)
	{
		_this->GetCDText(str,str_size);
	}
	else
	{
		lstrcpyn(str,"<error>",str_size);
	}
	return false;
}
DWORD SONORK_CALLBACK
 TSonorkClipWin::ConsoleCallback(void*		tag
				,SONORK_CONSOLE_EVENT 	gcEvent
				,DWORD			//pIndex
				,void*			pData)
{
	TSonorkClipWin *_this = (TSonorkClipWin*)tag;
	if( gcEvent == SONORK_CONSOLE_EVENT_HISTORY_EVENT )
	{
		return _this->OnHistoryWin_Event( (TSonorkHistoryWinEvent*)pData );
	}
	else
	if( gcEvent == SONORK_CONSOLE_EVENT_EXPORT )
	{
		return _this->OnConsole_Export((TSonorkConsoleExportEvent*)pData);
	}
	return 0;
}



struct _ExportTag
{
	TSonorkShortString	str;
	char			fg_color[16];
	char			bg_color[16];
};

void
 TSonorkClipWin::DoExport()
{
	TSonorkShortString 	path;
	_ExportTag		ET;

	TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_CLIP,SKIN_CI_FG), ET.fg_color);
	TSonorkConsole::WinToRGB(sonork_skin.Color(SKIN_COLOR_CLIP,SKIN_CI_BG), ET.bg_color);
	console->CloseView();
	path.SetBufferSize(SONORK_MAX_PATH);
	ET.str.SetBufferSize(SONORK_MAX_PATH);

	strcpy(path.Buffer() , GetCleanFileName(ET.str.Buffer()) );

	if(console->Export(path.Buffer()
		,SONORK_CONSOLE_EXPORT_F_ASK_PATH
		,&ET
		,NULL))
	{
		SonorkApp.ShellOpenFile( console->HistoryWin() , path.CStr() , false );
	}
}

DWORD
 TSonorkClipWin::OnConsole_Export(TSonorkConsoleExportEvent* EV)
{
	_ExportTag*		ET;

	ET = (_ExportTag*)EV->tag;

	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_LINE)
	{
		if(db.Get(EV->data.line->dat_index,dta) != SONORK_RESULT_OK )
			return 0L;
		GetCDText(ET->str);
	}
	else
	if( EV->section == SONORK_CONSOLE_EXPORT_SECTION_START )
	{
		ET->str.SetBufferSize(SONORK_MAX_PATH);
		GetCleanFileName(ET->str.Buffer());
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
					SONORK_HtmlPuts( EV->file, ET->str.Buffer() );
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
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:

				fprintf(EV->file,
					" <TR>\n"
					"  <TD VALIGN=TOP BGCOLOR=#%s><font color=#%s>"
					,ET->bg_color
					,ET->fg_color);
				SONORK_HtmlPuts(EV->file, ET->str.CStr() );
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
					fprintf(EV->file,"%u.%u: %s (%u/%u/%u %u:%u)\n"
						,SonorkApp.ProfileUserId().v[0]
						,SonorkApp.ProfileUserId().v[1]
						,ET->str.Buffer()
						,EV->st.wYear
						,EV->st.wMonth
						,EV->st.wDay
						,EV->st.wHour
						,EV->st.wMinute);
					TSonorkConsole::SepLine(EV->file,8);
				}
				break;
			case SONORK_CONSOLE_EXPORT_SECTION_COMMENTS:
				break;

			case SONORK_CONSOLE_EXPORT_SECTION_LINE:
				fputs(ET->str.CStr() , EV->file);
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

void
	TSonorkClipWin::OpenFocused( BOOL )
{
	TSonorkCCacheEntry *CL;
	TSonorkShortString	text;
	DWORD			line_no;
	SONORK_RESULT		result;
	CL = console->GetFocused( NULL , &line_no );
	if( CL != NULL )
	{
		result = db.Get( CL->dat_index , dta );
		if( result == SONORK_RESULT_OK )
		{
			if(dta->DataType() == SONORK_CLIP_DATA_FILE_NAME )
			{
				char *cut_point;
				const char *path;

				path = dta->GetCStr();
				if( !SonorkApp.IsControlKeyDown() )
				{
					// Unless CTRL key is down, we open the folder, not the file
					// Remove the file name after folder path
					//  SONORK_IO_GetFileName does NOT include the bar (\\)
					//  before the file name
					text.Set( path );
					path = text.CStr();
					cut_point = (char*)SONORK_IO_GetFileName(path);
					*cut_point=0;
				}
				SonorkApp.ShellOpenFile(console->HistoryWin() , path , true);
			}
			else
			{
				dta->GetTextFormat( &text );
				console->SetOutputText( text.CStr() );
				console->OpenView( 0);
			}
			return;
		}
	}
	console->CloseView();
}

void
 TSonorkClipWin::OnHistoryWin_LinePaint(const TSonorkCCacheEntry*CL, TSonorkHistoryWinPaintCtx*CTX)
{
	DWORD&	ctxFlags=CTX->flags;
	SKIN_ICON	l_icon;
//	HBRUSH		brush;
	SKIN_COLOR	l_color;
	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_FOCUSED ) // Line is focused
	{
		l_color = SKIN_COLOR_MSG_FOCUS;
	}
	else
	if( ctxFlags & SONORK_HIST_WIN_PAINT_F_LINE_SELECTED ) // Line is focused
	{
		l_color = SKIN_COLOR_MSG_SELECT;
	}
	else
	{
		l_color = SKIN_COLOR_CLIP;
	}
	CTX->SetLineColor(sonork_skin.Color(l_color,SKIN_CI_BG));
	CTX->SetTextColor(sonork_skin.Color(l_color,SKIN_CI_FG));

	switch( CL->tag.v[SONORK_CCACHE_TAG_INDEX] )
	{
		case SONORK_CLIP_DATA_USER_ID:
			l_icon = SKIN_ICON_USERS;
		break;

		case SONORK_CLIP_DATA_TEXT:
			l_icon = SKIN_ICON_NOTES;
		break;

		case SONORK_CLIP_DATA_URL:
			l_icon = SKIN_ICON_URL;
		break;

		case SONORK_CLIP_DATA_FILE_NAME:
			l_icon = SKIN_ICON_FILE;
		break;

		case SONORK_CLIP_DATA_FILE_LIST:
		default:
			l_icon = SKIN_ICON_NONE;
			break;
	}
//	CTX->r_icon = SKIN_ICON_NONE;
	CTX->l_icon = l_icon;
	ctxFlags|=SONORK_HIST_WIN_PAINT_F_HOT_LICON;
	/*
	::FillRect(CTX->hDC(),CTX->LeftRect(),brush);
	sonork_skin.DrawIcon(CTX->hDC()
		,l_icon
		,CTX->LeftRect()->left+1
		,CTX->LeftRect()->top);
	*/
}

DWORD
 TSonorkClipWin::OnHistoryWin_Event( TSonorkHistoryWinEvent* E )
{
	switch( E->Event() )
	{
		case SONORK_HIST_WIN_EVENT_LINE_PAINT:
			OnHistoryWin_LinePaint(E->Line(), E->PaintContext());
			break;
			
		case SONORK_HIST_WIN_EVENT_GET_TEXT:
			OnHistoryWin_GetText(E->Line(),E->GetTextData());
			break;

		case SONORK_HIST_WIN_EVENT_SEL_CHANGE:
			if( console->Selection().Items() )
				SetStatus_SelectCount();
			else
				SetStatus_RecordCount();
			break;

		case SONORK_HIST_WIN_EVENT_LINE_CLICK:

			if( console->Selection().Items() )
				SetStatus_SelectCount();
			else
				SetStatus_RecordCount();

			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED )
			{
				if(console->GetOutputMode() == SONORK_CONSOLE_OUTPUT_VIEW
				|| (E->ClickFlags()&(SONORK_HIST_WIN_FLAG_LICON_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) ) )
				{
					OpenFocused( false );
					break;
				}
			}
			else
			{
				if( E->LineNo() != console->FocusedLine() )
					break;

				if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_LICON_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
				{
					OpenFocused(E->ClickFlags() & SONORK_HIST_WIN_FLAG_DOUBLE_CLICK);
					break;
				}
			}

			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
			{
				TrackPopupMenu(SonorkApp.ClipPopupMenu()
						, TPM_LEFTALIGN  | TPM_LEFTBUTTON
						,E->ClickPoint().x
						,E->ClickPoint().y
						,0
						,Handle()
						,NULL);
			}
		break;

	case SONORK_HIST_WIN_EVENT_LINE_DRAG:
		OnHistoryWin_LineDrag( E->LineNo() );
		break;

	case SONORK_HIST_WIN_EVENT_LINE_DELETE:
		db.Del( E->Line()->dat_index );
		return SONORK_HIST_WIN_DELETE_F_ALLOW;
	}
	return 0;
}
void
 TSonorkClipWin::OnHistoryWin_GetText(const TSonorkCCacheEntry*CL ,TSonorkDynData*DD)
{
	TSonorkShortString	str;
	if(db.Get(CL->dat_index,dta) == SONORK_RESULT_OK )
	{
		GetCDText( str );
		DD->AppendStr(str.CStr() , false);
	}

}
void
 TSonorkClipWin::OnHistoryWin_LineDrag( DWORD line_no )
{
	TSonorkDropSourceData 	*SD;
	TSonorkCCacheEntry 	*CL;
	TSonorkClipData         *sCD;
	DWORD			aux=0;

	SD = new TSonorkDropSourceData;
	sCD = SD->GetClipData();
	if( console->SelectionActive() )
	{
		TSonorkListIterator I;
		TSonorkClipData		*nCD;
		sCD->SetSonorkClipDataQueue();
		console->SortSelection();
		console->InitEnumSelection(I);
		while((line_no = console->EnumNextSelection(I))!=SONORK_INVALID_INDEX)
		{
			CL = cache->Get(line_no , NULL , NULL );
			if( CL == NULL )continue;

			nCD = new TSonorkClipData;

			if( db.Get(CL->dat_index ,nCD) == SONORK_RESULT_OK )
				if(sCD->AddSonorkClipData( nCD ))
					aux++;

			nCD->Release();
		}
	}
	else
	{
		CL = cache->Get(line_no , NULL, NULL);
		if( CL != NULL )
		{
			if( db.Get(CL->dat_index ,sCD) == SONORK_RESULT_OK )
				aux++;
		}
	}
	if( aux > 0 )
	{
		aux=0;
		SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
	}
	SD->Release();
}
void TSonorkClipWin::OnSize(UINT sz_type)
{
	if( sz_type == SIZE_RESTORED )
		RealignControls();
}
#define SPACING 	2
void
 TSonorkClipWin::RealignControls()
{
	const int 	midH	=Height() - status.height;
	const int 	fullW	=Width();

	SetWinUsrFlag(ALIGNED);
	::SetWindowPos( toolbar.hwnd
		, NULL
		, 0
		, 1
		, fullW-2
		, toolbar.height
		, SWP_NOACTIVATE|SWP_NOZORDER);

	::SetWindowPos( console->Handle()
		, NULL
		, SPACING
		, toolbar.height + 1
		, fullW - SPACING*2
		, midH  - toolbar.height - 1
		, SWP_NOACTIVATE|SWP_NOZORDER);

	::SetWindowPos( status.hwnd
		, NULL
		, 0
		, midH
		, fullW
		, status.height
		, SWP_NOACTIVATE|SWP_NOZORDER);
}

void
 TSonorkClipWin::SetStatus_Count(GLS_INDEX gls, DWORD count, SKIN_HICON hicon)
{
	char tmp[128];
	SonorkApp.LangSprintf(tmp , gls , count );
	TSonorkWin::SetStatus(status.hwnd,tmp,hicon);
}
void
 TSonorkClipWin::SetStatus_SelectCount()
{
	SetStatus_Count(GLS_MS_NITEMSEL
		, console->Selection().Items()
		, SKIN_HICON_INFO);
}
void
 TSonorkClipWin::SetStatus_RecordCount(SKIN_HICON hicon)
{
	SetStatus_Count(GLS_MS_NITEMS
		, console->Lines()
		, hicon);
}

