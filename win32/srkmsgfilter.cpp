#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srkmsgfilter.h"

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

#define POKE_CONTINUE_FILTER		SONORK_WIN_POKE_01
#define PROGRESS_SIZE(n)		((n)>>4)
#define LINES_PER_CONTINUE		512
#define SECS_IN_A_DAY			((double)(60*60*24))
#define MAX_RANGE_VALUE			999999

DWORD SONORK_CALLBACK
	TSonorkMsgFilterWin::ConsoleCallback(void*	pTag
					,SONORK_CONSOLE_EVENT 	gcEvent
					,DWORD		pIndex
					,void*		pData)
{
	TSonorkMsgFilterWin*_this = (TSonorkMsgFilterWin*)pTag;
	switch(gcEvent)
	{
		case SONORK_CONSOLE_EVENT_HISTORY_EVENT:
			return _this->OnHistoryWinEvent((TSonorkHistoryWinEvent*)pData);


		case SONORK_CONSOLE_EVENT_PROCESS:
			_this->CmdProcess();
			break;

		case SONORK_CONSOLE_EVENT_CLOSE_VIEW:
			_this->context.console->CloseView();
			return true;	// we've processed it

		case SONORK_CONSOLE_EVENT_VKEY:
			return _this->ProcessVKey(pIndex , (DWORD)pData);

		case SONORK_CONSOLE_EVENT_TOOLBAR_TOOL_TIP:
		case SONORK_CONSOLE_EVENT_TOOLBAR_NOTIFY:
			break;
			
		case SONORK_CONSOLE_EVENT_EXPORT:
			return _this->context.msg_win->OnConsoleExport(
				(TSonorkConsoleExportEvent*)pData);
	}
	return 0L;
}
LRESULT
	TSonorkMsgFilterWin::ProcessVKey( UINT vKey , DWORD vKeyFlags)
{
	if( vKey == VK_ESCAPE )
	{
	
		if( vKeyFlags&SONORK_CONSOLE_VKEY_WM_CHAR )
		{
			if( context.console->GetOutputMode() == SONORK_CONSOLE_OUTPUT_HISTORY )
				OpenMsg( false );
			else
				context.console->CloseView();
		}
		return true;
	}
	return false;
}

DWORD TSonorkMsgFilterWin::OnHistoryWinEvent(TSonorkHistoryWinEvent*E)
{
	TSonorkCCacheEntry *CL;
	DWORD	line_no;
	switch( E->Event() )
	{
		case SONORK_HIST_WIN_EVENT_LINE_PAINT:
			context.msg_win->OnHistoryWin_LinePaint( E->Line(), E->PaintContext() );
			break;
			
		case SONORK_HIST_WIN_EVENT_GET_TEXT:
			context.msg_win->OnHistoryWin_GetText( context.tmp_msg
				, E->Line()
				, E->GetTextType()
				, E->GetTextData() );
			break;

		case SONORK_HIST_WIN_EVENT_LINE_CLICK:
			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_FOCUS_CHANGED )
			{
				line_no = E->LineNo();
				CL = context.cache->Get(line_no , NULL , NULL );
				if( CL != NULL )
				{
					if(GetOutputMode() == SONORK_CONSOLE_OUTPUT_VIEW
					|| 	(E->ClickFlags()&SONORK_HIST_WIN_FLAG_RICON_CLICK ) )
						OpenMsg( false );
				}
				else
				{
					// Don't let UpdateMsgInfoText() try to read the line again
					line_no=SONORK_INVALID_INDEX;
				}
				UpdateMsgInfoText( line_no , CL );
			}
			else
			{
				if( E->LineNo() != context.console->FocusedLine() )
					break;

				if( E->ClickFlags() & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_RICON_CLICK) )
				{
					OpenMsg( E->ClickFlags() & SONORK_HIST_WIN_FLAG_DOUBLE_CLICK );
				}
			}
			if( E->ClickFlags() & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
			{
				TrackPopupMenu(SonorkApp.MFilPopupMenu()
						, TPM_LEFTALIGN  | TPM_LEFTBUTTON
						,E->ClickPoint().x
						,E->ClickPoint().y
						,0
						,Handle()
						,NULL);
			}
		break;

		case SONORK_HIST_WIN_EVENT_LINE_DRAG:
			SonorkApp.DoMsgDrag(context.console,E->LineNo());
		break;

	}
	return 0L;
}


void
 TSonorkMsgFilterWin::UpdateMsgInfoText(DWORD line_no, TSonorkCCacheEntry* pEntry)
{
	DWORD 			sel_items;
	double			tDelta;
	char 			tmp1[104],tmp2[24];
	SYSTEMTIME		si;
	SKIN_HICON	hicon;
	DWORD			days;

	sel_items = context.console->Selection().Items();
	if( line_no != SONORK_INVALID_INDEX )
	{
		if(pEntry == NULL)
			pEntry = context.console->Get(line_no , NULL);
		if(pEntry != NULL)
		{
			pEntry->time.GetTime(&si);
			wsprintf(tmp2
				,"%04u/%02u/%02u %02u:%02u"
				,si.wYear
				,si.wMonth
				,si.wDay
				,si.wHour
				,si.wMinute);
			if( sel_items == 0)
			{
				SonorkApp.CurrentTime().DiffTime(pEntry->time,&tDelta);
				days = (DWORD)(tDelta/SECS_IN_A_DAY);
				wsprintf(tmp1
					,"%u/%u - %s: %u"
					,line_no+1
					,context.console->Lines()
					,age_label
					,days);
			}
		}
		else
			*tmp1=*tmp2=0;
	}
	else
	{
		*tmp1=*tmp2=0;
	}

	SetMsgWinStatus(status.hwnd
		, MSG_WIN_SB_DATE
		, tmp2
		, SKIN_HICON_NONE);


	if( sel_items )
	{
		SonorkApp.LangSprintf(tmp1 , GLS_MS_NITEMSEL, sel_items );
		hicon = SKIN_HICON_INFO;
	}
	else
	{
		hicon = SKIN_HICON_NONE;
	}
	SetMsgWinStatus(status.hwnd
		, MSG_WIN_SB_HINT
		, tmp1
		, hicon);

}

bool
 TSonorkMsgFilterWin::CmdProcess()
{
	UINT type=context.console->GetToolBarUserFlags();
	if( type == SONORK_MSG_WIN_PROCESS_URL )
	{
		SonorkApp.OpenUrl( this , context.cur_msg.ToCStr());
		return true;
	}
	else
	if( type == SONORK_MSG_WIN_PROCESS_FILE )
	{
		if(CmdOpenFile())
			return true;
	}
	return false;
}

bool
  TSonorkMsgFilterWin::CmdOpenFile()
{
	TSonorkCCacheEntry 	*	CL;
	TSonorkShortString		path;
	DWORD				line_no;
	CL = context.console->GetFocused( NULL , &line_no );
	if( CL == NULL )
		return false;
	if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_DELETED) )
		return false;
	if( (CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING )
	&& !(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_PROCESSED ))
		return false;
	return SonorkApp.OpenCCacheFile( this , CL , &context.file_info);
}

void
 TSonorkMsgFilterWin::OpenMsg( BOOL dbl_clicked )
{
	DWORD process_flags;
	if( context.msg_win->LoadCurrentMsg(context,process_flags,status.hwnd) != NULL )
	{
		if( dbl_clicked && (process_flags&SONORK_CONSOLE_TBF_PROCESS) )
		{
			if( CmdProcess() )
			{
				context.console->CloseView();
				return;
			}
			process_flags = 0;
		}
		context.console->OpenView( process_flags );
		if(IsActive())
		{
			SonorkApp.PostAppCommand( SONORK_APP_COMMAND_FOCUS_HWND , (LPARAM)context.console->OutputHandle() );
		}
	}
}



TSonorkMsgFilterWin::TSonorkMsgFilterWin(TSonorkMsgWin*owner)
	:TSonorkWin(owner
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_MSGFILTER
	,0)
{
	context.msg_win	= owner;
	context.cache		= NULL;
	i_console 	= owner->Console();
	i_cache		= i_console->Cache();

}
void
 TSonorkMsgFilterWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{	{IDL_MSGFILTER_INCTEXT	,	GLS_LB_CNTNS	}
	,	{IDL_MSGFILTER_NOTTEXT	,	GLS_LB_NCNTNS	}
	,	{IDL_MSGFILTER_LENGTH	,	GLS_LB_LEN		}
	,	{IDCANCEL				, 	GLS_OP_CLOSE	}
	,	{IDOK					,	GLS_OP_SEARCH	}
	,	{IDC_MSGFILTER_SENT		,	GLS_LB_SENT		}
	,	{IDC_MSGFILTER_RECVD	,	GLS_LB_RECVD	}
	,	{IDC_MSGFILTER_QUERY	,	GLS_LB_QUERY	}
	,	{IDC_MSGFILTER_REPLY	,	GLS_LB_REPLY	}
	,	{IDC_MSGFILTER_FILES	,	GLS_LB_FILE		}
	,	{IDC_MSGFILTER_PLAINTEXT,	GLS_LB_TXT		}
	,	{IDC_MSGFILTER_EMAIL	,	GLS_LB_EMAIL	}
	,	{IDC_MSGFILTER_OTHER	,	GLS_LB_OTHR		}
	,	{IDC_MSGFILTER_PROTECTED,	GLS_LB_PROTD	}
	,	{0						,	GLS_NULL		}
	};
	LoadLangEntries( gls_table, true );

	wsprintf(age_label,"%s(%s)",SonorkApp.LangString(GLS_LB_AGE),SonorkApp.LangString(GLS_LB_DAYS));
	SetCtrlText( IDL_MSGFILTER_AGE, age_label );
}

bool
 TSonorkMsgFilterWin::OnCreate()
{
	union {
		RECT rect;
		char str[96];
	}D;

	status.hwnd 	= GetDlgItem( IDC_MSGFILTER_STATUS );
	status.height   = GetCtrlHeight( IDC_MSGFILTER_STATUS );
	InitMsgWinStatus( status.hwnd );

	wsprintf(D.str
		,"%s:%-.32s"
		,SonorkApp.LangString(GLS_OP_SEARCH)
		,context.msg_win->UserData()->display_alias.CStr());
	SetWindowText( D.str );

	sprintf(D.str
		,"mff%x%x"
		,context.msg_win->UserData()->userId.v[0]
		,context.msg_win->UserData()->userId.v[1]);
	SonorkApp.GetTempPath(cache_path,D.str,NULL,(DWORD)this);
	
	context.cache= SonorkApp.CreateMsgCache();
	context.cache->Open( cache_path.CStr() );
	context.cache->Clear(true);

	context.console = new TSonorkConsole( this , context.cache, ConsoleCallback, this , 0);
	context.console->Create();
	context.msg_win->SetupConsole( context.console );

	GetCtrlRect(IDC_MSGFILTER_TOPFRAME,&D.rect);
	top_frame_size.cx = D.rect.right - D.rect.left + 4;
	top_frame_size.cy = D.rect.bottom- D.rect.top + 4;
	RealignControls();

	SetCtrlChecked(IDC_MSGFILTER_SENT,true);
	SetCtrlChecked(IDC_MSGFILTER_RECVD,true);
	SetCtrlChecked(IDC_MSGFILTER_PLAINTEXT,true);
	SetCtrlChecked(IDC_MSGFILTER_OTHER,true);
	SetCtrlUint(IDC_MSGFILTER_AGEMAX,30);
 	return true;
}
bool	TSonorkMsgFilterWin::OnMinMaxInfo(MINMAXINFO*MMI)
{
	MMI->ptMinTrackSize.x=top_frame_size.cx + 2;
	MMI->ptMinTrackSize.y=top_frame_size.cy + 80;
	return true;
}
void	TSonorkMsgFilterWin::RealignControls()
{
	const int sH = Height() - status.height;
	context.console->MoveWindow(
		 2
		,top_frame_size.cy
		,Width() - 4
		,sH - top_frame_size.cy - 2
		,true);
	::MoveWindow(status.hwnd
		,0
		,sH
		,Width()
		,status.height
		,true);
}
void	TSonorkMsgFilterWin::OnBeforeDestroy()
{
	char tmp[SONORK_MAX_PATH];
	context.console->Destroy();
	delete context.cache;
	wsprintf(tmp,"%s.dat",cache_path.CStr());
	DeleteFile(tmp);
	wsprintf(tmp,"%s.idx",cache_path.CStr());
	DeleteFile(tmp);
}

void 	TSonorkMsgFilterWin::StartFilter()
{
	DWORD 	t_flags=0;
	char	v[16];
	if(GetCtrlChecked(IDC_MSGFILTER_SENT))
		t_flags|=SONORK_MFF_SENT;
	if(GetCtrlChecked(IDC_MSGFILTER_RECVD))
		t_flags|=SONORK_MFF_RECVD;
	if(GetCtrlChecked(IDC_MSGFILTER_PROTECTED))
		t_flags|=SONORK_MFF_PROTECTED;
	if(GetCtrlChecked(IDC_MSGFILTER_REPLY))
		t_flags|=SONORK_MFF_REPLY;
	if(GetCtrlChecked(IDC_MSGFILTER_PLAINTEXT))
		t_flags|=SONORK_MFF_PLAIN_TEXT;
	if(GetCtrlChecked(IDC_MSGFILTER_URLS))
		t_flags|=SONORK_MFF_URL;
	if(GetCtrlChecked(IDC_MSGFILTER_FILES))
		t_flags|=SONORK_MFF_FILE;
	if(GetCtrlChecked(IDC_MSGFILTER_OTHER))
		t_flags|=SONORK_MFF_OTHER;
	if(GetCtrlChecked(IDC_MSGFILTER_QUERY))
		t_flags|=SONORK_MFF_QUERY;
	if(GetCtrlChecked(IDC_MSGFILTER_EMAIL))
		t_flags|=SONORK_MFF_EMAIL;

	GetCtrlText(IDC_MSGFILTER_LENGTHMIN,v,16);
	if(*v)t_flags|=SONORK_MFF_LENGTH;
	filter.length.min = strtoul(v,NULL,10);

	GetCtrlText(IDC_MSGFILTER_LENGTHMAX,v,16);
	if(*v)
	{
		t_flags|=SONORK_MFF_LENGTH;
		filter.length.max = strtoul(v,NULL,10);
	}
	else
		filter.length.max = MAX_RANGE_VALUE;

	GetCtrlText(IDC_MSGFILTER_AGEMIN,v,16);
	if(*v)t_flags|=SONORK_MFF_AGE;
	filter.age.min = strtoul(v,NULL,10);
	GetCtrlText(IDC_MSGFILTER_AGEMAX,v,16);
	if(*v)
	{
		t_flags|=SONORK_MFF_AGE;
		filter.age.max = strtoul(v,NULL,10);
	}
	else
		filter.age.max = MAX_RANGE_VALUE;

	GetCtrlText(IDC_MSGFILTER_INCTEXT,filter.inc_text);
	if(filter.inc_text.Length())t_flags|=SONORK_MFF_INCTEXT;

	GetCtrlText(IDC_MSGFILTER_NOTTEXT,filter.not_text);
	if(filter.not_text.Length())t_flags|=SONORK_MFF_NOTTEXT;

	filter.cur_line	= 0;
	filter.end_line = i_console->Lines();
	filter.flags	= t_flags;
	i_range.min = i_range.max = 0;
	SetHintModeLang(GLS_MS_PWAIT,true);
	context.console->Clear();
	PostPoke(POKE_CONTINUE_FILTER,0);
}

void 	TSonorkMsgFilterWin::ContinueFilter()
{
	UINT cur_cc;
	DWORD			aux;
	TSonorkCCacheEntry*	iEntry;
	TSonorkMsg			CLmsg;
	double  		t_delta;
	DWORD			days;
	for(cur_cc=0
		;cur_cc<LINES_PER_CONTINUE
		&&filter.cur_line<filter.end_line
		;cur_cc++
		,filter.cur_line++)
	{
		iEntry = i_cache->Get(filter.cur_line
					, NULL
					, NULL
					, SONORK_CCACHE_SD_FORWARD);
		if(iEntry==NULL)continue;

		if( iEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING )
		{
			if(!(filter.flags & SONORK_MFF_RECVD))
				continue;
		}
		else
		{
			if(!(filter.flags & SONORK_MFF_SENT))
				continue;
		}

		aux = (iEntry->tag.v[SONORK_CCACHE_TAG_INDEX]&SONORK_SERVICE_TYPE_MASK);
		if( aux == SONORK_SERVICE_TYPE_NONE )
		{
			if(!(filter.flags & SONORK_MFF_PLAIN_TEXT))
				continue;
		}
		else
		if( aux == SONORK_SERVICE_TYPE_SONORK_FILE)
		{
			if(!(filter.flags & SONORK_MFF_FILE))
				continue;
		}
		else
		if( aux == SONORK_SERVICE_TYPE_URL)
		{
			if(!(filter.flags & SONORK_MFF_URL))
				continue;
		}
		else
		if( aux == SONORK_SERVICE_TYPE_EMAIL )
		{
			if(!(filter.flags & SONORK_MFF_EMAIL))
				continue;
		}
		else
		{
			if(!(filter.flags & SONORK_MFF_OTHER))
				continue;
		}
		if( filter.flags & SONORK_MFF_PROTECTED )
		{
			if(!(iEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_PROTECTED))
				continue;
		}
		if( filter.flags & SONORK_MFF_REPLY )
		{
			if(!(iEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & (SONORK_APP_CCF_THREAD_START|SONORK_APP_CCF_IN_THREAD)))
				continue;
		}
		if( filter.flags & SONORK_MFF_QUERY )
		{
			if(!(iEntry->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_QUERY))
				continue;
		}

		if(filter.flags & SONORK_MFF_AGE)
		{
			if(SonorkApp.CurrentTime().DiffTime(iEntry->time,&t_delta))
			{
				days = (DWORD)(t_delta/SECS_IN_A_DAY);
				if(days<filter.age.min||days>filter.age.max)
					continue;
			}
		}

		if(filter.flags & (SONORK_MFF_INCTEXT|SONORK_MFF_NOTTEXT|SONORK_MFF_LENGTH))
		{
			if(SonorkApp.GetMsg(iEntry->dat_index,&CLmsg)!=SONORK_RESULT_OK)
				continue;
			CLmsg.ToCStr();
			if(filter.flags & SONORK_MFF_LENGTH)
			{
				aux = CLmsg.TextLength() ;
				if(aux<filter.length.min || aux>filter.length.max)
					continue;
			}
			if(filter.flags & SONORK_MFF_INCTEXT)
			{
				if( !SONORK_StrStr(CLmsg.CStr(),filter.inc_text.CStr()) )
					continue;

			}
			if(filter.flags & SONORK_MFF_NOTTEXT)
			{
				if( SONORK_StrStr(CLmsg.CStr(),filter.not_text.CStr()) )
					continue;
			}
		}
		if( !context.cache->Lines() )
			i_range.min = filter.cur_line;
		context.cache->Add( *iEntry , NULL );
		i_range.max = filter.cur_line;
	}
	if(filter.cur_line>=filter.end_line)
	{
		context.console->AfterAdd();
		if( !(context.console->SetFocusedLine(0, false)&SONORK_VIEW_SET_FOCUS_RF_EVENT_GENERATED) )
			UpdateMsgInfoText(SONORK_INVALID_INDEX,NULL);
		ClearHintMode();
	}
	else
	{
//	::SendMessage(progress_hwnd,PBM_SETPOS,PROGRESS_SIZE(filter.cur_line),0);
		PostPoke(POKE_CONTINUE_FILTER,0);
	}
}


void	TSonorkMsgFilterWin::OnSize(UINT size_type)
{
	if(size_type==SIZE_RESTORED)
	{
		RealignControls();
	}
}


LRESULT	TSonorkMsgFilterWin::OnPoke(SONORK_WIN_POKE wParam,LPARAM)
{
	if(wParam == POKE_CONTINUE_FILTER)
		ContinueFilter();
	return 0;
}
/*SKIN_ICON
			TSonorkMsgFilterWin::On_GcLineColor(DWORD 	line_no
					,DWORD 			line_flags
					,TSonorkCCacheEntry*	CL
					,SKIN_ICON&   l_icon
					,COLORREF&		fg_color
					,COLORREF&		bg_color)
{
	return msg_win->On_GcLineColor(line_no,line_flags,CL,l_icon,fg_color,bg_color);
}

void	TSonorkMsgFilterWin::On_GcLineOpen(DWORD line_no)
{
	if( line_no == context.console->FocusedLine() )
		OpenFocusedMsg( false );
}
void	TSonorkMsgFilterWin::On_GcProcess(bool )
{
}

void	TSonorkMsgFilterWin::On_GcLineClick(DWORD line_no, UINT ev_flags, POINT& pt)
{
	if( line_no == context.console->FocusedLine() )
	if( ev_flags & (SONORK_HIST_WIN_FLAG_DOUBLE_CLICK|SONORK_HIST_WIN_FLAG_OPEN_CLICK) )
	{
		OpenFocusedMsg(ev_flags & SONORK_HIST_WIN_FLAG_DOUBLE_CLICK);
	}
	else
	if( ev_flags & SONORK_HIST_WIN_FLAG_RIGHT_CLICK )
	{
		TrackPopupMenu(SonorkApp.MFilPopupMenu()
				, TPM_LEFTALIGN  | TPM_LEFTBUTTON
				,pt.x
				,pt.y
				,0
				,Handle()
				,NULL);
	}
}
*/
/*
void TSonorkMsgFilterWin::LoadForm(TSonorkMsgFilter*filter)
{
	DWORD 	t_flags=filter->flags;
	SetCtrlChecked(IDC_MSGFILTER_SENT,t_flags&SONORK_MFF_SENT);
	SetCtrlChecked(IDC_MSGFILTER_RECVD,t_flags&SONORK_MFF_RECVD);
	SetCtrlChecked(IDC_MSGFILTER_PLAINTEXT,t_flags&SONORK_MFF_PLAIN_TEXT);
	SetCtrlChecked(IDC_MSGFILTER_URLS,t_flags&SONORK_MFF_URL);
	SetCtrlChecked(IDC_MSGFILTER_FILES,t_flags&SONORK_MFF_FILE);
	SetCtrlChecked(IDC_MSGFILTER_OTHER,t_flags&SONORK_MFF_OTHER);
	SetCtrlUint(IDC_MSGFILTER_LENGTHMIN,filter->length.min);
	SetCtrlUint(IDC_MSGFILTER_LENGTHMAX,filter->length.max);
	SetCtrlUint(IDC_MSGFILTER_AGEMIN,filter->age.min );
	SetCtrlUint(IDC_MSGFILTER_AGEMAX,filter->age.max );
	SetCtrlText(IDC_MSGFILTER_INCTEXT,filter->inc_text.CStr());
	SetCtrlText(IDC_MSGFILTER_NOTTEXT,filter->not_text.CStr());
}
*/
void	TSonorkMsgFilterWin::OnInitMenu(HMENU hmenu)
{
	if( hmenu == SonorkApp.MFilPopupMenu() )
	{
		UINT enable;
		BOOL selection;
		char	tmp[96];
		selection=context.console->Selection().Active();

		enable=(selection?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND;
		EnableMenuItem(hmenu,CM_DELETE,enable);
		EnableMenuItem(hmenu,CM_MSGS_CLEAR_SEL,enable);
		wsprintf(tmp,"%s (%s)"
			,SonorkApp.LangString(GLS_OP_EXPORT)
			,SonorkApp.LangString(selection?GLS_LB_SELTD:GLS_LB_ALL));
		TSonorkWin::SetMenuText(hmenu, CM_EXPORT, tmp);

//		CheckMenuItem(hmenu,CM_MSGS_SINGLE_LINE_MODE
//			,MF_BYCOMMAND|(context.console->GetSingleLineMode()?MF_CHECKED:MF_UNCHECKED));
	}

}
void	TSonorkMsgFilterWin::CmdDelSelected()
{
	if(context.console->SelectionActive())
	{
		TSonorkListIterator 	I;
		DWORD			line_no,scan_lines;
		TSonorkCCacheEntry *	CL;
		TSonorkCCacheMark	CM;
		scan_lines = SONORK_CCACHE_DEFAULT_MARKER_SCAN_LINES + (i_range.max - i_range.min);
		i_console->ClearSelection();
		context.console->InitEnumSelection(I);
		while( (line_no = context.console->EnumNextSelection(I)) != SONORK_INVALID_INDEX)
		{
			if((CL = context.cache->Get(line_no , NULL, NULL))!=NULL)
			{
				CM.Set(i_range.max,CL);
				if( i_cache->GetByMark(&CM, NULL , scan_lines) != NULL )
				{
					i_console->AddSelection(CM.line_no);
				}
			}
		}
		if(i_console->SelectionActive())
		{
			i_console->DelSelectedLines();
			StartFilter();
		}
	}

}
void TSonorkMsgFilterWin::CmdLocateMain()
{
	TSonorkCCacheEntry 	*pEntry;
	DWORD			line_no;
	TSonorkCCacheMark	CM;
	pEntry = context.console->GetFocused( NULL , &line_no );
	if( pEntry != NULL )
	{
		CM.Set(i_range.max,pEntry);
		if( i_cache->GetByMark(&CM
			, NULL
			, SONORK_CCACHE_DEFAULT_MARKER_SCAN_LINES + (i_range.max - i_range.min)) != NULL )
			i_console->SetFocusedLine(CM.line_no,false);
	}
}
bool	TSonorkMsgFilterWin::OnCommand(UINT id,HWND hwndSender, UINT notify_code)
{
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

			case CM_CLIPBOARD_COPY:
				context.console->CopyToClipboard(0);
				break;

			case CM_EXPORT:
				context.msg_win->CmdExport( context.console );
				break;

			case CM_DELETE:
				CmdDelSelected();
				break;

			case CM_PROTECT:
			case CM_MSGS_SEL_THREAD:
			case CM_MSGS_SINGLE_LINE_MODE:
//				context.console->SetSingleLineMode( !context.console->GetSingleLineMode() );
				break;

			case CM_MSGS_LOCATE_MAIN:
				CmdLocateMain();
				break;

			default:
				return false;
		}
		return true;
	}
	else
	if(notify_code==BN_CLICKED)
	{
		if(id==IDOK)
		{
			StartFilter();
			return true;
		}
	}
	return false;
}


bool
 TSonorkMsgFilterWin::OnDrawItem(DRAWITEMSTRUCT*S)
{
	if(S->CtlID == IDC_MSGFILTER_STATUS )
	{
		DrawMsgWinStatus(S);
		return true;
	}
	return false;
}

