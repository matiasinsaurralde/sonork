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

#include "srk_services.h"
#include "srk_uts.h"
#include "srk_winregkey.h"
#include "srk_cfg_names.h"
#include "srk_file_io.h"
#include "srkmainwin.h"
#include "srksmailwin.h"
#include "srkwappwin.h"
#include "srkconsole.h"
#include "srkchatwin.h"
#include "srktrackerwin.h"
#include "srklistview.h"


// ----------------------------------------------------------------------------
//  Variables (local and globals declared in guwin32app.h)
// ----------------------------------------------------------------------------


SONORK_C_CSTR	szSonorkOpenSaveDirKey="dir.%s";
SONORK_C_CSTR	szSonorkOpenSaveDir_Export="Export";
SONORK_C_CSTR	szSonorkOpenSaveDir_ClipPath="ClipPath";
SONORK_C_CSTR	szSONORK="Sonork";
SONORK_C_CSTR	szWinRunKey="SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
SONORK_C_CSTR	szInternetSonorkHost="server.sonork.com";
SONORK_C_CSTR	szStartMode="StartMode";
SONORK_C_CSTR	szInternet="Internet";
SONORK_C_CSTR	szIntranet="Intranet";
SONORK_C_CSTR	szDefault="default";

SONORK_C_CSTR 	szSonorkClientSetup="SonorkClientSetup";
SONORK_C_CSTR 	szWappBase="WappBase";
SONORK_C_CSTR 	szSonorkDatePickFormat="yyy/MM/dd";
SONORK_C_CSTR 	szSonorkTimePickFormat="HH:mm";
SONORK_C_CSTR 	szSonorkDateTimePickFormat= "yyy/MM/dd HH:mm";
SONORK_C_CSTR 	szAuthExcludeList= "AuthExcludeList";
SONORK_C_CSTR 	szPrivateDirClip= "clip";
SONORK_C_CSTR 	szPrivateDirUser= "user";
SONORK_C_CSTR 	szSonorkId	="SonorkId";
SONORK_C_CSTR 	szGuId		="GuID";



// ----------------------------------------------------------------------------
// Local functions declarations
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------

#define SHOULD_BROADCAST_OLD_CTRLMSG(n)	\
	((n)>=SONORK_OLD_CTRLMSG_CMD_QUERY_REQUEST \
	&&(n)<=SONORK_OLD_CTRLMSG_CMD_EXEC_RESULT)


void
 TSonorkWin32App::OnOldCtrlMsg(
	  const TSonorkUserHandle&	handle
	, const TSonorkCtrlMsg*		msg
	, const void*			data
	, DWORD				data_size)
{
	UINT	service_id;
	DWORD	rf;
	service_id = msg->OldServiceId();
	switch( service_id )
	{
		case SONORK_SERVICE_ID_EXTERNAL_APP:
			rf=OnOldCtrlMsg_ExtApp(handle,msg,data,data_size);
		break;

		case SONORK_SERVICE_ID_SONORK_CHAT:
			rf=TSonorkChatWin::OldCtrlMsgHandler(handle,msg,data,data_size);
		break;

		default:
			rf=SONORK_APP_OLD_CTRLMSG_RF_DEFAULT;
		break;
	}
	if(	SHOULD_BROADCAST_OLD_CTRLMSG(msg->OldCmd())
	&&	(rf&SONORK_APP_OLD_CTRLMSG_RF_MAY_BROADCAST)	)
	{
		TSonorkAppEventOldCtrlMsg E;
		E.handle		=&handle;
		E.msg			=msg;
		E.data			=(const BYTE*)data;
		E.data_size		=data_size;
		BroadcastAppEvent(SONORK_APP_EVENT_OLD_CTRL_MSG
			,SONORK_APP_EM_CTRL_MSG_CONSUMER
			,service_id
			,&E);
	}
	if(	(msg->OldCmd() == SONORK_OLD_CTRLMSG_CMD_QUERY_REQUEST)
	&&	(rf&SONORK_APP_OLD_CTRLMSG_RF_MAY_END_QUERY)	)
	{
		ReplyOldCtrlMsg(&handle
			,msg
			,SONORK_OLD_CTRLMSG_CMD_QUERY_END
			,0);
	}
}



// ----------------------------------------------------------------------------
// LaunchAppByCmd first attempts to interpret <menu_cmd> as an internal app
// and if it is not, it tries to launch the external app using LaunchExtApp.

SONORK_RESULT
	TSonorkWin32App::LaunchAppByCmd(UINT menu_cmd
		, const TSonorkExtUserData*UD
		, bool*is_a_valid_menu_cmd)
{
	bool valid=true;
	SONORK_RESULT	result=SONORK_RESULT_OK;
	TSonorkWin *W;
	switch(menu_cmd)
	{
		case CM_APP_CHAT:
			TSonorkChatWin::Init((TSonorkExtUserData*)UD);
			break;

		case CM_APP_TRACK:
			if( UD != NULL )
				TSonorkTrackerWin::Init(UD, "");
			break;

		case CM_APP_EMAIL:
			W = new TSonorkSmailWin(win32.main_win,UD);
			if( W->Create() )
				W->ShowWindow(SW_SHOW);
			else
				delete W;
			break;

		case CM_APP_CLIPBOARD:
			RunSysDialog(SONORK_SYS_DIALOG_CLIP);
			break;

		case CM_APP_SNAP_SHOT:
			RunSysDialog(SONORK_SYS_DIALOG_SNAP_SHOT);
			/*
			if( UD != NULL )
			{
				W = new TSonorkSnapshotWin((TSonorkExtUserData*)UD);
				if( W->Create() )
					W->ShowWindow(SW_SHOW);
				else
					delete W;
			}
			*/
			break;
		case CM_APP_REMINDER:
			RunSysDialog(SONORK_SYS_DIALOG_REMIND_LIST);
			break;

		case CM_APP_SYS_CONSOLE:
			ShowSysConsole();
			break;

		default:
			if( menu_cmd >=SONORK_APP_CM_EAPP_BASE && menu_cmd<SONORK_APP_CM_EAPP_LIMIT )
			{
				result = LaunchExtApp(
					ext_apps.PeekNo(menu_cmd - SONORK_APP_CM_EAPP_BASE)
					,UD,true);
			}
			else
			{
				valid=false;
				result=SONORK_RESULT_INVALID_PARAMETER;
			}
			break;
	}
	if(is_a_valid_menu_cmd)*is_a_valid_menu_cmd=valid;
	return result;
}


SONORK_RESULT
 TSonorkWin32App::LaunchWebAppByCmd(TSonorkWin*W,UINT menu_cmd, const TSonorkExtUserData*UD, const TSonorkCCacheMark*cMark)
{
	if( menu_cmd == CM_WAPP_OPEN )
	{
		TSonorkWappWin*gWW;
		TSonorkTempBuffer       url(SONORK_MAX_PATH);
		TSonorkShortString 	path;

		if(!GetLoadPath(W->Handle()
					, path
					, NULL
					, GLS_OP_OPEN
					, "Wapp"
					, ".htm"
					, OFN_EXPLORER
					  | OFN_LONGNAMES
					  | OFN_NOCHANGEDIR
					  | OFN_PATHMUSTEXIST
					))
			return SONORK_RESULT_USER_TERMINATION;
		sprintf(url.CStr(),"file://%s",path.CStr());
		gWW = new TSonorkWappWin(url.CStr(),UD);
		gWW->Create();
		gWW->ShowWindow(SW_SHOW);
		return SONORK_RESULT_OK;
	}
	else
	{
		TSonorkAppViewItem*VI;
		VI = win32.main_win->GetAppViewItem(menu_cmd);
		if(VI!=NULL)
			if(VI->AppType()==SONORK_APP_TYPE_WEB)
				return LaunchWebApp(W,((TSonorkWebAppViewItem*)VI)->WappData(),UD,cMark);
		return SONORK_RESULT_INVALID_PARAMETER;
	}
}


SONORK_RESULT
 TSonorkWin32App::LaunchWebApp(TSonorkWin*,const TSonorkWappData*WD, const TSonorkExtUserData*UD, const TSonorkCCacheMark*cMark)
{
	if(WD!=NULL)
	{
		TSonorkAppEventStartWapp E;
		E.consumed 	= false;
		E.wapp_data	= WD;
		E.user_data	= UD;
		E.reply_mark= cMark;

		BroadcastAppEvent(SONORK_APP_EVENT_START_WAPP
				,SONORK_APP_EM_WAPP_PROCESSOR
				,0
				,&E);
		if(!E.consumed)
		{
			TSonorkWappWin*gWW;
			gWW = new TSonorkWappWin(WD,UD,cMark);
			gWW->Create();
			gWW->ShowWindow(SW_SHOW);
		}
		return SONORK_RESULT_OK;
	}
	return SONORK_RESULT_INVALID_PARAMETER;
}


void
 TSonorkWin32App::ToggleProfileCtrlFlag(SONORK_PROFILE_CTRL_FLAG f)
{
	if(ProfileCtrlFlags().Test(f))
		ProfileCtrlFlags().Clear(f);
	else
		ProfileCtrlFlags().Set(f);
	SaveCurrentProfile(SONORK_APP_BASE_SPF_SAVE_CTRL_DATA);
}

// ----------------------------------------------------------------------------
// 	Directories/Paths
// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::GetDirPath(TSonorkShortString&target,SONORK_APP_DIR dir,SONORK_C_CSTR subdir)
{
	TSonorkTempBuffer 	tmp( SONORK_MAX_PATH+8 );
	SONORK_RESULT		result;
	result=GetDirPath(tmp.CStr(),dir,subdir);
	target.Set(tmp.CStr());
	return result;
}
SONORK_RESULT	TSonorkWin32App::GetDirPath(SONORK_C_STR target,SONORK_APP_DIR dir,SONORK_C_CSTR subdir)
{
	const char *base;
	const char *root;
	switch(dir)
	{
		case SONORK_APP_DIR_ROOT:
			base="";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_DATA:
			base="";
			root=win32.data_dir.CStr();
			break;
		case SONORK_APP_DIR_TEMP:
			base="";
			root=win32.temp_dir.CStr();
			break;
		case SONORK_APP_DIR_APPS:
			base="apps\\";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_USERS:
			base="users\\";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_BIN:
			base="bin\\";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_SOUND:
			base="sounds\\";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_SKIN:
			base="skins\\";
			root=win32.root_dir.CStr();
			break;
		case SONORK_APP_DIR_MSGTPL:
			base="msgtpl\\";
			root=win32.root_dir.CStr();
			break;
		default:
			*target=0;
			return SONORK_RESULT_INVALID_PARAMETER;
	}
	if(SONORK_IsEmptyStr(subdir))
		sprintf(target,"%s\\%s",root,base);
	else
		sprintf(target,"%s\\%s%s"
			,root
			,base
			,subdir);

	return SONORK_RESULT_OK;
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::GetTempPath(SONORK_C_STR target,SONORK_C_CSTR prefix, SONORK_C_CSTR sufix, DWORD un)
{
	char tmp[128];

	sprintf(tmp,"~%u.%u~%s%x(%s)%s"
		,ProfileUserId().v[0]
		,ProfileUserId().v[1]
		,prefix?prefix:""
		,(un==(DWORD)-1)?GetSerialNo():un
		,szSonorkAppMode
		,sufix?sufix:"");

	return GetDirPath(target,SONORK_APP_DIR_TEMP,tmp);
}

// ----------------------------------------------------------------------------

SONORK_RESULT	TSonorkWin32App::GetTempPath(TSonorkShortString& target,SONORK_C_CSTR prefix, SONORK_C_CSTR sufix, DWORD un)
{
	TSonorkTempBuffer tmp(SONORK_MAX_PATH+128);
	SONORK_RESULT	result;
	if((result=GetTempPath(tmp.CStr(),prefix,sufix,un))==SONORK_RESULT_OK)
		target.Set(tmp.CStr());
	else
		target.Set("");
	return result;
}


// ----------------------------------------------------------------------------


BOOL
 TSonorkWin32App::GetFileNameForUpload(TSonorkWin*W,TSonorkShortString& file_path)
{
	OPENFILENAME OFN;
	TSonorkShortString dir_path;

	ReadProfileItem("UldPath", &dir_path);

	file_path.SetBufferSize(SONORK_MAX_PATH+2);
	*file_path.Buffer()=0;
	OFN.lStructSize 	= sizeof(OFN);
	OFN.hwndOwner       	= W->Handle();
	OFN.hInstance		= NULL;//GuApp.Instance();
	OFN.lpstrFilter		= NULL;
	OFN.lpstrCustomFilter 	= NULL;
	OFN.nMaxCustFilter	= 0;
	OFN.nFilterIndex	= 0;
	OFN.lpstrFile		= file_path.Buffer();
	OFN.nMaxFile		= SONORK_MAX_PATH;
	OFN.lpstrFileTitle	= NULL;
	OFN.nMaxFileTitle	= 0;
	OFN.lpstrInitialDir	= dir_path.CStr();
	OFN.lpstrTitle		= LangString(GLS_OP_PUTFILE);
	OFN.Flags			= OFN_EXPLORER
						| OFN_LONGNAMES
						| OFN_NOCHANGEDIR
						| OFN_FILEMUSTEXIST
						| OFN_READONLY
						| OFN_PATHMUSTEXIST;
	OFN.nFileOffset		=
	OFN.nFileExtension	= 0;
	OFN.lpstrDefExt		= NULL;
	OFN.lCustData		= 0;
	OFN.lpfnHook		= 0;
	OFN.lpTemplateName	= NULL;
	if( GetOpenFileName(&OFN) )
	{
		dir_path.Set(file_path);
		*(dir_path.Buffer() + OFN.nFileOffset)=0;
		WriteProfileItem("UldPath", &dir_path);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------

BOOL
 TSonorkWin32App::GetLoadPath(HWND ownerHwnd
	, TSonorkShortString&	path
	, SONORK_C_CSTR 	file_name
	, GLS_INDEX 		title_gls
	, SONORK_C_CSTR 	key
	, SONORK_C_CSTR 	ext
	, UINT			flags
	, GetLoadSaveDialog* 	GLSD
	)
{
	OPENFILENAME 	OFN;
	TSonorkShortString	default_dir;
	LPCTSTR			dialog_id;
	LPOFNHOOKPROC		dialog_hook;
	DWORD			dialog_data;
	char			filter[64];
	char			key_name[48];
	int			l;
	if(key!=NULL)
	{
		wsprintf(key_name,szSonorkOpenSaveDirKey,key);
		ReadProfileItem(key_name, &default_dir );
	}
	if(GLSD != NULL )
	{
		flags|=OFN_ENABLETEMPLATE;
		dialog_id 	= MAKEINTRESOURCE(GLSD->id);
		dialog_data = GLSD->data;
		if( (dialog_hook = GLSD->hook)!=NULL)
		{
			flags|=OFN_ENABLEHOOK;
		}
	}
	path.SetBufferSize(SONORK_MAX_PATH+2);
	if(file_name)
		strcpy( path.Buffer(), file_name );
	else
		*path.Buffer()=0;
	if( ext )
	{
		l=wsprintf(filter,"%s",ext)+1;
		l+=wsprintf(filter+l,"*.%s",ext)+1;
		*(filter+l)=0;


	}
	OFN.lStructSize 	= sizeof(OFN);
	OFN.hwndOwner       = ownerHwnd;
	OFN.hInstance		= NULL;
	OFN.lpstrFilter		= ext?filter:NULL;
	OFN.lpstrCustomFilter = NULL;
	OFN.nMaxCustFilter	= 0;
	OFN.nFilterIndex	= 0;
	OFN.lpstrFile		= path.Buffer();
	OFN.nMaxFile		= SONORK_MAX_PATH;
	OFN.lpstrFileTitle	= NULL;
	OFN.nMaxFileTitle	= 0;
	OFN.lpstrInitialDir	= key!=NULL?default_dir.CStr():NULL;
	OFN.lpstrTitle		= LangString( title_gls );
	OFN.Flags	     	= flags;
	OFN.nFileOffset		=
	OFN.nFileExtension	= 0;
	OFN.lpstrDefExt		= ext;
	OFN.lCustData		= dialog_data;
	OFN.lpfnHook		= dialog_hook;
	OFN.lpTemplateName	= dialog_id;
	if(IsWindow(ownerHwnd))
	::UpdateWindow(ownerHwnd);
	if(!GetOpenFileName(&OFN))
		return false;
	if( key!=NULL )
	{
		default_dir.Set( path );
		*(default_dir.Buffer() + OFN.nFileOffset)=0;
		WriteProfileItem(key_name, &default_dir );
	}
	return true;
}

// ----------------------------------------------------------------------------

BOOL	TSonorkWin32App::GetSavePath(HWND owner
	, TSonorkShortString&	path
	, SONORK_C_CSTR 	file_name
	, GLS_INDEX 		title_gls
	, SONORK_C_CSTR 	key
	, SONORK_C_CSTR 	ext
	, UINT			flags
	, GetLoadSaveDialog* 	GLSD
	)
{
	OPENFILENAME 	OFN;
	TSonorkShortString 	default_dir;
	LPCTSTR			dialog_id;
	LPOFNHOOKPROC		dialog_hook;
	DWORD			dialog_data;
	char			key_name[48];
	if(key!=NULL)
	{
		sprintf(key_name , szSonorkOpenSaveDirKey , key );
		ReadProfileItem(key_name, &default_dir );
	}
	if(GLSD != NULL )
	{
		flags |= OFN_ENABLETEMPLATE;
		dialog_id = MAKEINTRESOURCE(GLSD->id);
		dialog_data = GLSD->data;
		if( (dialog_hook = GLSD->hook)!=NULL)
		{
			flags|=OFN_ENABLEHOOK;
		}
	}
	path.SetBufferSize(SONORK_MAX_PATH+2);
	strcpy( path.Buffer(), file_name );
	OFN.lStructSize 	= sizeof(OFN);
	OFN.hwndOwner       	= owner;
	OFN.hInstance		= Instance();//NULL;
	OFN.lpstrFilter		= NULL;
	OFN.lpstrCustomFilter = NULL;
	OFN.nMaxCustFilter	= 0;
	OFN.nFilterIndex	= 0;
	OFN.lpstrFile		= path.Buffer();
	OFN.nMaxFile		= SONORK_MAX_PATH;
	OFN.lpstrFileTitle	= NULL;
	OFN.nMaxFileTitle	= 0;
	OFN.lpstrInitialDir	= key!=NULL?default_dir.CStr():NULL;
	OFN.lpstrTitle		= LangString( title_gls );
	OFN.Flags		= flags; //flags;
	OFN.nFileOffset		=
	OFN.nFileExtension	= 0;
	OFN.lpstrDefExt		= ext;
	OFN.lCustData		= dialog_data;
	OFN.lpfnHook		= dialog_hook;
	OFN.lpTemplateName	= dialog_id;//NULL;
	if(!GetSaveFileName(&OFN))
		return false;
	if( key!=NULL )
	{
		default_dir.Set( path );
		*(default_dir.Buffer() + OFN.nFileOffset)=0;
		WriteProfileItem(key_name, &default_dir );
	}
	return true;
}

// ----------------------------------------------------------------------------

int
	TSonorkWin32App::ExecFile(TSonorkWin* hint_win, SONORK_C_CSTR file, BOOL expand_vars )
{
	const char *ptr;
	char	*tmp;
	int		ec;
	int		rv;
	STARTUPINFO			start_info;
	PROCESS_INFORMATION	proc_info;

	if(!hint_win)
		hint_win=win32.main_win;

	hint_win->SetHintModeLang(GLS_MS_PWAIT,true);


	ptr = file;
	if( expand_vars  )
	{
		ec = strlen(file) + 256;
		tmp = SONORK_MEM_ALLOC( char, ec );
		if( ExpandEnvironmentStrings(file,tmp,ec) )
			ptr = tmp;
	}
	else
		tmp=NULL;


	start_info.cb = sizeof(start_info);
	GetStartupInfo(&start_info);
	start_info.dwFlags|=STARTF_USESHOWWINDOW;
	start_info.wShowWindow = SW_SHOWNORMAL;
	if(CreateProcess(
			NULL
		,	(char*)ptr
		, 	NULL
		,	NULL
		,	false
		,	0
		,	NULL
		, 	NULL
		,	&start_info
		,	&proc_info))
	{
		SetBichoSequence(SONORK_SEQUENCE_OPEN_WINDOW);
		CloseHandle( proc_info.hThread );
		WaitForInputIdle( proc_info.hProcess , 1000 );
		CloseHandle( proc_info.hProcess );
		rv = 0;
	}
	else
	{
		TSonorkTempBuffer B(SONORK_MAX_PATH);
		LangSprintf(B.CStr()
			,GLS_MS_NOOPENFILE
			,ptr );
		Set_UI_Event(SONORK_UI_EVENT_ERROR
			,B.CStr()
			,SONORK_UI_EVENT_F_BICHO
			|SONORK_UI_EVENT_F_SOUND
			|SONORK_UI_EVENT_F_NO_LOG);
		//SetBichoSequenceError(true);
		//SysLogV( SONORK_UI_EVENT_ERROR , 0 , GLS_MS_NOOPENFILE, ptr);
		rv = GetLastError();
	}


	hint_win->ClearHintMode();
	if(tmp != NULL )
		SONORK_MEM_FREE( tmp );
	return rv;
}

// ----------------------------------------------------------------------------

int
 TSonorkWin32App::ShellOpenFile( TSonorkWin* hint_win	, SONORK_C_CSTR file, BOOL check_file_exists)
{
	int ec;
	if(!hint_win)
		hint_win=win32.main_win;
	if( check_file_exists )
	{
		if( !CheckFileAttr(file, 0 , 0))
		{
			hint_win->MessageBox(GLS_MS_FDNEXIST,szSONORK,MB_OK);
			return 2;
		}
	}
	hint_win->SetHintModeLang(GLS_MS_PWAIT,true);
	ec=(int)::ShellExecute(
		 hint_win->Handle()
		,"open"
		, file
		, NULL
		, NULL
		, SW_SHOWNORMAL
		);
	hint_win->ClearHintMode();
	if(ec<=32)
		TRACE_DEBUG("ShellOpenFile(%s) = %d",file,ec);
	return ec;
}


// ----------------------------------------------------------------------------


DWORD	TSonorkWin32App::GetSerialNo()
{
	if(++win32.serial_no>100)win32.serial_no=0;
	return win32.serial_no;
}

void
  TSonorkWin32App::LoadViewLine(SONORK_C_STR buffer,UINT buffer_size, SONORK_C_CSTR source, BOOL strip_dups, bool *p_line_was_cut)
{
	static SONORK_C_CSTR _no_repeat_chars_	="=-+*/\\¿?¡!\"<> \t";
	SONORK_C_CSTR	src;
	SONORK_C_STR	tgt;
	SONORK_C_STR	last_cuttable_pos;
	SONORK_C_CHAR	source_char,last_copied_char,last_no_repeat_char;
	UINT		tgt_length;
	UINT		src_length;

	src_length =
	tgt_length = 0;
	tgt = buffer;
	src = source;

	assert( buffer_size > 8 );

	buffer_size -- ;
	last_cuttable_pos = NULL;
	last_no_repeat_char	=
	last_copied_char	= 0;
	// Load only buffer size chars and scan at most 1000 chars
	while( tgt_length < buffer_size && src_length < 1000)
	{
		source_char = *src++;
		if(!source_char)break;
		src_length++;
		if( strip_dups )
		{
			if( source_char>0 && source_char<' ')	// ignore non-printable chars
			{
				if(source_char=='\r' || source_char=='\n')
				{
					if(last_no_repeat_char==' ')
						continue;
					last_no_repeat_char = source_char=' ';
				}
				else
				{
					last_no_repeat_char = ' ';
					continue;
				}
			}
			else
			if( strchr(_no_repeat_chars_,source_char) )
			{
				if( last_no_repeat_char == source_char )
					continue;
				last_no_repeat_char=source_char;
			}
			else
			{
				last_no_repeat_char=0;
			}
		}
		else
		{
			if( source_char>0 && source_char<' ')	// ignore non-printable chars
			{
				if(source_char=='\r' || source_char=='\n')
				{
					if(last_copied_char!=' ')
						source_char=' ';
					else
						continue;
				}
				else
					continue;
			}
		}
		*tgt++ = last_copied_char = source_char;
		tgt_length ++;
		if(last_copied_char>0 && last_copied_char <=' ')
			last_cuttable_pos=tgt;
	}
	*tgt = 0;
	if( source_char != 0 && !(last_copied_char>0 && last_copied_char<=' '))
	{
		// last_copied_char was not a space
		if(last_cuttable_pos != NULL)
		{
			source_char = 1;	// Force 'line_was_cut' flag
			*last_cuttable_pos=0;
		}
	}
	if( p_line_was_cut )
		*p_line_was_cut = (source_char!=0);
}


// -------------------------------------------------------------------------
// File/Directories

BOOL
 TSonorkWin32App::CreateDirIfNotExists(SONORK_C_CSTR path)

{

	if(!TSonorkWin32App::CheckFileAttr(path , FILE_ATTRIBUTE_DIRECTORY , FILE_ATTRIBUTE_DIRECTORY))

	{

		return CreateDirectory(path,NULL);

	}

	return true;

}

BOOL
 TSonorkWin32App::CheckFileAttr(SONORK_C_CSTR path, DWORD file_attr_flags, DWORD file_attr_mask)
{
	DWORD attr;
	attr = GetFileAttributes( path );
	if( attr == (DWORD)-1)
		return false;
	if(file_attr_mask)
	{
		return (attr & file_attr_mask) &  file_attr_flags;
	}
	return true;

}

// ----------------------------------------------------------------------------
// Keyboard state

BOOL
 TSonorkWin32App::IsControlKeyDown()
{
	return GetKeyState(VK_CONTROL) & 0x8000;
}

BOOL
 TSonorkWin32App::IsSelectKeyDown()
{
	return GetKeyState(VK_SHIFT) & 0x8000;
}

// ----------------------------------------------------------------------------

bool
 TSonorkWin32App::MakeTimeStr( const TSonorkTime& time ,char *buffer, UINT flags)
{

	SYSTEMTIME	st;

	*buffer=0;

	if( !time.GetTime( &st) )
		return false;

	{
		char tmp[80];
		if( flags & MKTIMESTR_DATE )
		{
			if(!GetDateFormat(
				  LOCALE_USER_DEFAULT
				, DATE_SHORTDATE
				, &st
				, NULL
				, tmp
				, sizeof(tmp)
			   ))
			{
				return false;
			}
			strcpy(buffer,tmp);
		}
		if(flags & MKTIMESTR_TIME)
		{
			if(GetTimeFormat(
				  LOCALE_USER_DEFAULT
				, TIME_NOSECONDS
				, &st
				, NULL
				, tmp
				, sizeof(tmp)
			   ))
			{
				if( flags & MKTIMESTR_DATE )
					strcat(buffer," ");
				strcat(buffer,tmp);
			}
		}
	}
	return true;

}



// ----------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::TransferWinStartInfo(TSonorkWin*W,BOOL load, const char*win_type_name,const TSonorkId*user_id)
{
	char key_name[64];
	SONORK_RESULT result;
	TSonorkWinStartInfo SI;
	TSonorkCodecRawAtom ATOM(&SI,sizeof(SI));
	wsprintf(key_name,"%sSI",win_type_name);
	if( load )
	{
		if( user_id != NULL )
			result = ReadUserItem(*user_id,key_name,&ATOM);
		else
			result = ReadProfileItem(key_name, &ATOM);
		if( result == SONORK_RESULT_OK)
			W->TransferStartInfo(&SI, true);
	}
	else
	{
		if( !W->IsIconic() )
		{
			W->TransferStartInfo(&SI, false);
			if( user_id != NULL )
				result = WriteUserItem(*user_id,key_name,&ATOM);
			else
				result = WriteProfileItem(key_name, &ATOM);
		}
	}
	return result;
}

void
 TSonorkWin32App::SetupSearchUserList( const TSonorkListView*LV )
{
	static TSonorkListViewColumn cols[]=
	{       {GLS_LB_SRKID	, 70}
	,	{GLS_LB_ALIAS 	, 80}
	,	{GLS_LB_NAME	, 120}
	,	{GLS_LB_NOTES	, 1500}
	};
	LV->AddColumns(4,cols);
}
#define SEARCH_RESULT_NOTES_MAX_SIZE	800

void
 TSonorkWin32App::ProcessSearchUserTaskData(
	const TSonorkListView*	LV
	,TSonorkDataPacket*	P
	,UINT			P_size
	,bool 			online_search
	,TSonorkAppSearchUserCallbackPtr cb)
{
	if((P->Function() == SONORK_FUNCTION_SEARCH_USER_NEW
	|| P->Function() == SONORK_FUNCTION_SEARCH_USER_OLD)
	&& P->SubFunction() ==  SONORK_SUBFUNC_NONE )
	{
		TSonorkUserDataNotes 	*DN;
		TSonorkTempBuffer	tmp(SEARCH_RESULT_NOTES_MAX_SIZE);
		int			index;
		TSonorkUserDataNotesQueue 	queue;
		
		P->D_SearchUser_A(P_size,queue);
		while( (DN=queue.RemoveFirst())!=NULL)
		{
			if(DN->user_data.GetUserInfoLevel()>=SONORK_USER_INFO_LEVEL_1)
			{
				if(!online_search)DN->user_data.addr.Clear();
				if( cb )
				{
					cb(LV,DN);
				}
				else
				{
					index = LV->AddItem(
						  DN->user_data.userId.GetStr(tmp.CStr())
						, GetUserModeIcon( &DN->user_data )
						, (LPARAM)DN);
					LV->SetItemText(index,1,DN->user_data.alias.CStr());
					LV->SetItemText(index,2,DN->user_data.name.CStr());
					LoadViewLine(tmp.CStr()
						, SEARCH_RESULT_NOTES_MAX_SIZE
						, DN->notes.ToCStr()
						, false
						, NULL);
					LV->SetItemText(index,3,tmp.CStr());
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::ReplyOldCtrlMsg(
		 const TSonorkUserHandle*	handle
		,const TSonorkCtrlMsg*		msg
		,SONORK_OLD_CTRLMSG_CMD	    	cmd
		,DWORD				sysFlags)
{
	TSonorkCtrlMsg	akn;
	memcpy(&akn,msg,sizeof(TSonorkCtrlMsg));
	akn.sysFlags	= sysFlags;
	akn.cmdFlags 	= cmd;
	return SendCtrlMsg(handle,&akn,NULL,0) ;
}

// ----------------------------------------------------------------------------


SONORK_RESULT
 TSonorkWin32App::SendCtrlMsg(
		 const TSonorkUserHandle*	handle
		,TSonorkCtrlMsg*		msg
		,const TSonorkCodecAtom	*	A
		,SONORK_DWORD2*			req_tag
		)
{
	TSonorkDynData	DD;
	if(A)A->CODEC_Write(&DD);

	return SendCtrlMsg(handle
			, msg
			, DD.Buffer()
			, DD.DataSize()
			, req_tag);
}

// ----------------------------------------------------------------------------

SONORK_RESULT
 TSonorkWin32App::SendCtrlMsg(
		 const TSonorkUserHandle*	handle
		,TSonorkCtrlMsg*		msg
		,const void*			data
		,UINT 				data_size
		,SONORK_DWORD2*			req_tag
		)
{
	TSonorkError ERR;
	UINT A_size,P_size;
	union {
		TSonorkDataPacket	*sonork;
		TSonorkUTSPacket	*uts;
	}P;

	if(handle->userId.IsZero() || handle->sid.IsZero() )
	{
		ERR.SetSys(SONORK_RESULT_INVALID_PARAMETER, GSS_BADADDR, 0);
	}
	else
	{

		A_size=   sizeof(TSonorkUserLocus1)
			+ sizeof(TSonorkCtrlMsg)
			+ data_size
			+ 32;
		if( win32.uts_server != NULL && handle->utsLinkId != 0)
		{
			P.uts = SONORK_AllocUtsPacket( A_size );

			P_size = P.uts->E_CtrlMsg(A_size
					,SONORK_UTS_CMD_CTRL_MSG
					,0
					,msg
					,data
					,data_size);
			win32.uts_server->SendPacket(ERR
				, handle->utsLinkId
				, P.uts
				, P_size
				, this
				, SonorkUtsRequestHandler
				, req_tag);
			SONORK_FreeUtsPacket(P.uts);
			if(ERR.Result()	== SONORK_RESULT_OK )
			{
				return SONORK_RESULT_OK;
			}
			// Send Ctrl Msg via UTS failed
		}
		P.sonork = SONORK_AllocDataPacket( A_size );
		P_size=P.sonork->E_CtrlMsg_R(A_size
			, handle
			, msg
			, data
			, data_size);

		StartSonorkRequest(P.sonork
			,P_size
			,req_tag
			,&ERR);

	}
	if(ERR.Result() != SONORK_RESULT_OK)
		TRACE_DEBUG("SendCtrlMsg FAILED(%s,%u)",ERR.ResultName(),ERR.Code());
	return ERR.Result();
}


// ----------------------------------------------------------------------------
// Folder locations
// ----------------------------------------------------------------------------

void
 TSonorkWin32App::GetExtAppsIniFile(SONORK_C_STR str)
{
	GetDirPath(str,SONORK_APP_DIR_APPS,"app.ini");
}


SONORK_RESULT
 TSonorkWin32App::LoadServerProfile(
		 SONORK_C_CSTR 			requested_profile_name
		,TSonorkClientServerProfile& 	SP
		,bool				may_load_default_profile
		,TSonorkShortString*		loaded_profile_name)
{
	SONORK_RESULT 		result;
	TSonorkShortString	profile_name;
	TSonorkRegKey		svr_key;
	char			tmp[256];

	profile_name.Set(requested_profile_name);
	if(!*profile_name.CStr())
		profile_name.Set(szDefault);
	for(;;)
	{
		result =_LoadServerProfile(profile_name.CStr(), SP);
		if(result == SONORK_RESULT_OK
			&& SP.sonork.host.Length()>1
			&& SP.sonork.ctrl_data.udp_port!=0
			&& SP.sonork.ctrl_data.tcp_port!=0)
		{
			SP.sonork.Enable();
			break;
		}

		if(SONORK_StrNoCaseCmp(profile_name.CStr(),szDefault) )
		{
			// If load failed and not the default profile,
			// try to load default profile (if allowed)
			profile_name.Set(szDefault);
			break;
		}
		SP.Clear();
		if(!may_load_default_profile)
			break;
		if( IntranetMode() )
		{
			result = SONORK_RESULT_CONFIGURATION_ERROR;
			sprintf(tmp,"%s\\Servers\\default",szSrkClientRegKeyRoot);
			if(svr_key.Open(HKEY_LOCAL_MACHINE,tmp,false,KEY_READ) != ERROR_SUCCESS)
				break;
			if(svr_key.QueryValue("Host",tmp,80)!=ERROR_SUCCESS)
				break;
			if(svr_key.QueryValue("TcpPort",&SP.sonork.ctrl_data.tcp_port)!=ERROR_SUCCESS)
				break;
			if(svr_key.QueryValue("UdpPort",&SP.sonork.ctrl_data.udp_port)!=ERROR_SUCCESS)
				break;
			SP.sonork.host.Set( tmp );
			if(svr_key.QueryValue("Protocol",tmp,8)!=ERROR_SUCCESS)
				*tmp=0;

			SP.sonork.ctrl_data.addr_type=
				!stricmp(tmp,"TCP")?SONORK_PHYS_ADDR_TCP_1:SONORK_PHYS_ADDR_UDP_1;
		}
		else
		{
			SP.sonork.host.Set( szInternetSonorkHost );
			SP.sonork.ctrl_data.udp_port	=1503;
			SP.sonork.ctrl_data.tcp_port	=1504;
			SP.sonork.ctrl_data.addr_type	=SONORK_PHYS_ADDR_UDP_1;
		}

		if(	SP.sonork.host.Length()>1
			&& SP.sonork.ctrl_data.udp_port!=0
			&& SP.sonork.ctrl_data.tcp_port!=0)
		{
			SP.sonork.Enable();
			SaveServerProfile(szDefault, SP);
			result = SONORK_RESULT_OK;
		}
		else
		{
			result = SONORK_RESULT_CONFIGURATION_ERROR;
		}
		break;	// exit loop
	}
	if(loaded_profile_name!=NULL )
		loaded_profile_name->Set(profile_name);
	return result;
}


bool
 TSonorkWin32App::LoadSonorkClipData(TSonorkClipData*CD
	,const TSonorkMsg*MSG
	,TSonorkCCacheEntry*CL)
{
	SONORK_C_STR		buffer=NULL;
	SONORK_CLIP_DATA_TYPE	data_type=SONORK_CLIP_DATA_NONE;
	SONORK_C_CSTR		data_text;
	TSonorkExtUserData	*UD;
	SONORK_C_STR		ptr;
	TSonorkTag		tag;

	data_text = MSG->CStr();
	switch( MSG->DataServiceType() )
	{
		case SONORK_SERVICE_TYPE_SONORK_FILE:
		if( CL != NULL )
		{
			if(!(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_DELETED))
			if(CL->ext_index!=SONORK_INVALID_INDEX)
			{
				TSonorkShortString 		aux_ss;
				if(GetExtData(CL->ext_index,&aux_ss,&tag)==SONORK_RESULT_OK)
				{
					if(tag.v[SONORK_DB_TAG_FLAGS]&SONORK_EXT_DB_FILE_PATH_FULL)
					{
						// Ext data is the full path
						data_type=SONORK_CLIP_DATA_FILE_NAME;
						data_text= buffer = SONORK_CStr(aux_ss.CStr());
					}
					else
					{
						TSonorkFileInfo FI;
						// Old version stored only the directory in the EXT data:
						// We must read the FileInfo and append the file
						// name to the string (this was changed because
						// users can change the file name before storing
						// and thus the result of appending the original
						// file name to the folder could be incorrect)
						if( FI.CODEC_Read(&MSG->data) == SONORK_RESULT_OK )
						{
							data_type=SONORK_CLIP_DATA_FILE_NAME;
							data_text= buffer = SONORK_MEM_ALLOC(char
								,aux_ss.Length()+FI.name.Length() + 16);
							wsprintf(buffer,"%s%s"
								,aux_ss.CStr()
								,FI.name.CStr());
						}

					}
				}
			}
		}
		break;

		case SONORK_SERVICE_TYPE_URL:
			data_type=SONORK_CLIP_DATA_URL;
		break;
		case SONORK_SERVICE_TYPE_NONE:
		default:
			{
				ptr = buffer = SONORK_MEM_ALLOC(char,strlen(data_text) + 64);
				UD = GetUser(MSG->header.userId );
				if( UD != NULL )
					ptr+=wsprintf(ptr,"%-.16s",UD->display_alias.CStr());
				ptr+=wsprintf(ptr,"(%u.%u)"
					,MSG->header.userId.v[0]
					,MSG->header.userId.v[1]);

				if( !CL )
					*ptr++=':';
				else
					if(CL->tag.v[SONORK_CCACHE_TAG_FLAGS] & SONORK_APP_CCF_INCOMMING)
						*ptr++='>';
					else
						*ptr++='<';
				strcpy(ptr , data_text );
				data_type=SONORK_CLIP_DATA_TEXT;
				data_text= buffer;
			}
		break;
	}
	if( data_type != SONORK_CLIP_DATA_NONE )
		CD->SetCStr( data_type , data_text );
	if(buffer!=NULL)SONORK_MEM_FREE(buffer);
	return data_type != SONORK_CLIP_DATA_NONE;

}

void
 TSonorkWin32App::OpenMailReader(TSonorkWin*hw)
{
	char				mail_client[64];
	char				key_name[128];
	TSonorkRegKey		tRK;
	TSonorkRegKey		sRK;
	TSonorkTempBuffer	buffer(SONORK_MAX_PATH);

	Clear_UI_Event(SONORK_UI_EVENT_INCOMMING_EMAIL);

	if( tRK.Open(HKEY_LOCAL_MACHINE
		, "Software\\Clients\\Mail"
		, false
		, KEY_READ) != ERROR_SUCCESS)
			return;

	if(ReadProfileItem("MailClient",mail_client,sizeof(mail_client)) != SONORK_RESULT_OK)
		mail_client[0]=0;

	if( strlen(mail_client) < 2 )
		if( tRK.QueryValue(NULL,mail_client, sizeof(mail_client)) != ERROR_SUCCESS )
			return;

	sprintf(key_name,"%s\\Shell\\Open\\command",mail_client);
	TRACE_DEBUG("KeyName=%s",key_name);
	if( sRK.Open(tRK,key_name,false,KEY_READ) == ERROR_SUCCESS )
	{
		if( sRK.QueryValue(NULL,buffer.CStr(),SONORK_MAX_PATH) == ERROR_SUCCESS)
		{
			TRACE_DEBUG("KeyValue=%s",buffer.CStr());
			ExecFile( hw , buffer.CStr() , true );
		}
		else
			TRACE_DEBUG("Cannot read command");
	}
	else
		TRACE_DEBUG("Cannot open key");
}
void
 TSonorkWin32App::OpenUrl(TSonorkWin* hw,SONORK_C_CSTR pStr)
{
	int l=pStr?strlen(pStr)+1:0;
	if( l>0 )
	{
		TSonorkTempBuffer url( l+2 );

		SONORK_StrCopyCut(url.CStr(),l, pStr  );

		ShellOpenFile(hw , url.CStr() , false);
	}
}
bool
 TSonorkWin32App::OpenCCacheFile(TSonorkWin* hw
		, TSonorkCCacheEntry* pCE
		, TSonorkFileInfo* file_info)
{
	TSonorkTag		ext_tag;
	TSonorkShortString	ss_path;
	const char *open_path;
	const char *ext_path;
	BOOL				open_the_file;
	if(GetExtData(pCE->ExtIndex(),&ss_path,&ext_tag)==SONORK_RESULT_OK)
	{
		TSonorkTempBuffer t_path(SONORK_MAX_PATH);

		// If control key is down, we open the file,
		// if not, we open the folder
		open_the_file = TSonorkWin32App::IsControlKeyDown();

		ext_path = ss_path.CStr();
		if(ext_tag.v[SONORK_DB_TAG_FLAGS]&SONORK_EXT_DB_FILE_PATH_FULL)
		{
			// Ver. 1.04.06 and above store the full path in the EXT data
			if( open_the_file ) // Control key is pressed?
			{
				open_path = ext_path;
			}
			else
			{
				char *cut_point;
				cut_point = t_path.CStr();
				strcpy(cut_point , ext_path );
				open_path = cut_point;

				// Remove the file name after folder path
				//  SONORK_IO_GetFileName does NOT include the bar (\\)
				//  before the file name
				cut_point = (char*)SONORK_IO_GetFileName(cut_point);
				*cut_point=0;
			}

		}
		else
		{
			// Ver. 1.04.05 and before store the folder's path in the EXT data
			if( open_the_file ) // Control key is pressed?
			{
				wsprintf(t_path.CStr(),"%s%s",ext_path
					,file_info->name.CStr());
				open_path = t_path.CStr();
			}
			else
			{
				open_path = ext_path;
			}
		}
		ShellOpenFile( hw , open_path , true);
	}
	return false;
}



void
 TSonorkWin32App::DoMsgDrag(TSonorkConsole* console, DWORD line_no)
{
	TSonorkDropSourceData 	*SD;
	TSonorkMsg		drag_msg;
	TSonorkCCacheEntry 	*CL;
	TSonorkClipData         *sCD;
	TSonorkCCache		*cache;
	DWORD			aux=0;

	cache = console->Cache();
	SD = new TSonorkDropSourceData;
	sCD = SD->GetClipData();
	if( console->SelectionActive() )
	{
		TSonorkListIterator I;
		TSonorkClipData    *nCD;
		sCD->SetSonorkClipDataQueue();
		console->SortSelection();
		console->InitEnumSelection(I);
		while((line_no = console->EnumNextSelection(I))!=SONORK_INVALID_INDEX)
		{
			CL = cache->Get(line_no , NULL , NULL );
			if( CL == NULL )continue;
			if( GetMsg(CL->dat_index ,&drag_msg) == SONORK_RESULT_OK )
			{
				nCD = new TSonorkClipData;
				if(LoadSonorkClipData(nCD,&drag_msg,CL))
					if(sCD->AddSonorkClipData( nCD ))
						aux++;

				nCD->Release();
			}
		}
	}
	else
	{
		CL = cache->Get(line_no , NULL , NULL );
		if( CL != NULL )
			if( GetMsg(CL->dat_index,&drag_msg) == SONORK_RESULT_OK )
				if(LoadSonorkClipData(sCD,&drag_msg,CL))
					aux=1;
	}
	if( aux > 0 )
	{
		aux=0;
		SONORK_DoDragDrop(SD,DROPEFFECT_COPY,&aux);
	}
	SD->Release();
}





// ----------------------------------------------------------------------------
// SID and User View
// ----------------------------------------------------------------------------

HICON
 TSonorkWin32App::GetUserModeHicon(const TSonorkExtUserData*UD,GLS_INDEX*p_gls_index)
{
	SKIN_HICON		icon;
	SONORK_SEX		sex;
	GLS_INDEX		gls_index;

	sex = ((TSonorkExtUserData*)UD)->InfoFlags().GetSex();
	switch(NormalizeSidMode(UD->addr.sidFlags.SidMode()))
	{
		default:
		case SONORK_SID_MODE_DISCONNECTED:

			if( sex == SONORK_SEX_F)
			{
				gls_index=GLS_UM_DCXF;
				icon=SKIN_HICON_SEX_FEMALE;
			}
			else
			{
				gls_index=GLS_UM_DCXM;
				icon=SKIN_HICON_SEX_MALE;
			}
			break;

		case SONORK_SID_MODE_ONLINE:
			gls_index=GLS_UM_READY;
			if( sex == SONORK_SEX_F )
			{
				icon=SKIN_HICON_MODE_F_ONLINE;
			}
			else
			{
				icon = SKIN_HICON_MODE_M_ONLINE;
			}
			break;

		case SONORK_SID_MODE_BUSY:
			if( sex == SONORK_SEX_F )
			{
				gls_index=GLS_UM_BUSYF;
				icon=SKIN_HICON_MODE_F_BUSY;
			}
			else
			{
				gls_index = GLS_UM_BUSYM;
				icon=SKIN_HICON_MODE_M_BUSY;
			}
			break;

		case SONORK_SID_MODE_AT_WORK:
			if( sex == SONORK_SEX_F )
			{
				gls_index=GLS_UM_WORKF;
				icon=SKIN_HICON_MODE_F_AT_WORK;
			}
			else
			{
				gls_index=GLS_UM_WORKM;
				icon=SKIN_HICON_MODE_M_AT_WORK;
			}
			break;

		case SONORK_SID_MODE_FRIENDLY:
			gls_index=GLS_UM_FRIENDLY;
			if( sex == SONORK_SEX_F )
			{
				icon=SKIN_HICON_MODE_F_FRIENDLY;
			}
			else
			{
				icon=SKIN_HICON_MODE_M_FRIENDLY;
			}
			break;

		case SONORK_SID_MODE_AWAY:
			gls_index=GLS_UM_AWAY;
			if( sex == SONORK_SEX_F )
			{
				icon=SKIN_HICON_MODE_F_AWAY;
			}
			else
			{
				icon=SKIN_HICON_MODE_M_AWAY;
			}
			break;
	}
	if( p_gls_index != NULL )
		*p_gls_index=gls_index;

	return sonork_skin.Hicon((SKIN_HICON)icon);
}

// ----------------------------------------------------------------------------

SKIN_ICON
 TSonorkWin32App::GetUserModeIcon(const TSonorkUserData*UD )
{
	return GetUserModeViewInfo(
				 UD->addr.sidFlags.SidMode()
				,UD->InfoFlags()
				,(GLS_INDEX*)NULL);
}

// ----------------------------------------------------------------------------

SKIN_ICON
 TSonorkWin32App::GetUserInfoIcon(const TSonorkUserInfoFlags&info_flags)
{
	UINT s;
	if(info_flags.Test(SONORK_UINFO_F1_ROBOT))
		return SKIN_ICON_ROBOT;
	s = info_flags.GetSex();
	if( s == SONORK_SEX_M)return SKIN_ICON_SEX_MALE;
	if( s == SONORK_SEX_F)return SKIN_ICON_SEX_FEMALE;
	return SKIN_ICON_BULLET;
}

// ----------------------------------------------------------------------------

SKIN_ICON
 TSonorkWin32App::GetUserModeViewInfo(SONORK_SID_MODE mode
	,const TSonorkUserInfoFlags& 	info_flags
	,GLS_INDEX* 			p_gls_index)
{
	SKIN_ICON	icon;
	SONORK_SEX	sex;
	GLS_INDEX	gls_index;

	sex = info_flags.GetSex();
	switch( NormalizeSidMode(mode) )
	{
		default:
		case SONORK_SID_MODE_DISCONNECTED:
			if( sex == SONORK_SEX_F)
			{
				gls_index = GLS_UM_DCXF;
				icon = SKIN_ICON_SEX_FEMALE;
			}
			else
			{
				gls_index= GLS_UM_DCXM ;
				icon = SKIN_ICON_SEX_MALE;
			}
			break;

		case SONORK_SID_MODE_ONLINE:
			gls_index=GLS_UM_READY;
			if( sex == SONORK_SEX_F)
			{
				icon=SKIN_ICON_MODE_F_ONLINE;
			}
			else
			{
				icon=SKIN_ICON_MODE_M_ONLINE;
			}
			break;

		case SONORK_SID_MODE_BUSY:
			if( sex == SONORK_SEX_F)
			{
				gls_index=GLS_UM_BUSYF;
				icon=SKIN_ICON_MODE_F_BUSY;
			}
			else
			{
				gls_index=GLS_UM_BUSYM;
				icon=SKIN_ICON_MODE_M_BUSY;
			}
			break;

		case SONORK_SID_MODE_AT_WORK:
			if( sex == SONORK_SEX_F)
			{
				gls_index=GLS_UM_WORKF;
				icon=SKIN_ICON_MODE_F_AT_WORK;
			}
			else
			{
				gls_index=GLS_UM_WORKM;
				icon=SKIN_ICON_MODE_M_AT_WORK;
			}
			break;

		case SONORK_SID_MODE_FRIENDLY:
			gls_index=GLS_UM_FRIENDLY;
			if( sex == SONORK_SEX_F)
			{
				icon=SKIN_ICON_MODE_F_FRIENDLY;
			}
			else
			{
				icon=SKIN_ICON_MODE_M_FRIENDLY;
			}
			break;

		case SONORK_SID_MODE_AWAY:
			gls_index=GLS_UM_AWAY;
			if( sex == SONORK_SEX_F)
			{
				icon=SKIN_ICON_MODE_F_AWAY;
			}
			else
			{
				icon=SKIN_ICON_MODE_M_AWAY;
			}
			break;

		case SONORK_SID_MODE_INVISIBLE:
			gls_index=GLS_UM_INVISIBLE;
			if( sex == SONORK_SEX_F)
			{
				icon=SKIN_ICON_MODE_F_INVISIBLE;
			}
			else
			{
				icon=SKIN_ICON_MODE_M_INVISIBLE;
			}
			break;
	}
	if( p_gls_index ) *p_gls_index = gls_index;
	return icon;
}

// ----------------------------------------------------------------------------
// NormalizeSidMode
//  returns one of the basic "clean" SID_MODEs given any SID_MODE.
// Examples:
//   All 3 : SID_MODE_AWAY, SID_MODE_AWAY_PHONE and SID_MODE_AWAY_AUTO
//     will return the basic SID_MODE_AWAY.
//  and SID_MODE_BUSY, SID_MODE_BUSY_01 and SID_MODE_BUSY_02
//     will return the basic SID_MODE_BUSY.

SONORK_SID_MODE
  TSonorkWin32App::NormalizeSidMode( SONORK_SID_MODE sm)
{
	switch(sm)
	{
		default:
		case SONORK_SID_MODE_ONLINE:
		case SONORK_SID_MODE_ONLINE_02:
		case SONORK_SID_MODE_ONLINE_03:
			return SONORK_SID_MODE_ONLINE;

		case SONORK_SID_MODE_BUSY:
		case SONORK_SID_MODE_BUSY_02:
		case SONORK_SID_MODE_BUSY_03:
			return SONORK_SID_MODE_BUSY;

		case SONORK_SID_MODE_AT_WORK:
		case SONORK_SID_MODE_AT_WORK_02:
		case SONORK_SID_MODE_AT_WORK_03:
			return SONORK_SID_MODE_AT_WORK;

 		case SONORK_SID_MODE_FRIENDLY:
		case SONORK_SID_MODE_FRIENDLY_02:
		case SONORK_SID_MODE_FRIENDLY_03:
			return SONORK_SID_MODE_FRIENDLY;

		case SONORK_SID_MODE_AWAY:
		case SONORK_SID_MODE_AWAY_AUTO:
		case SONORK_SID_MODE_AWAY_HOLD:
		case SONORK_SID_MODE_AWAY_PHONE:
		case SONORK_SID_MODE_AWAY_02:
		case SONORK_SID_MODE_AWAY_03:
			return SONORK_SID_MODE_AWAY;

		case SONORK_SID_MODE_INVISIBLE:
		case SONORK_SID_MODE_INVISIBLE_02:
		case SONORK_SID_MODE_INVISIBLE_03:
			return SONORK_SID_MODE_INVISIBLE;

		case SONORK_SID_MODE_DISCONNECTED:
			return SONORK_SID_MODE_DISCONNECTED;
	}
}
int
  TSonorkWin32App::LangSprintf(char*tgt,GLS_INDEX fmt,...)
{
	int len;
	va_list argptr;
	va_start(argptr, fmt);
		len = vsprintf(tgt, LangString(fmt), argptr);
	va_end(argptr);
	return len;
}

// ----------------------------------------------------------------------------
// 		HELPERS & ETC
// ----------------------------------------------------------------------------

HINSTANCE
 SonorkApp_Instance()
{
	return SonorkApp.Instance();
}

SONORK_C_CSTR
 SonorkApp_LangString(GLS_INDEX index)
{
	return SonorkApp.LangString(index);
}

void
 SonorkApp_ReportWinCreate(TSonorkWin*W,bool create)
{
	SonorkApp.PostAppCommand(
		create	?SONORK_APP_COMMAND_WIN_CREATED
			:SONORK_APP_COMMAND_WIN_DESTROYED
			,(LPARAM)W);
}



// ----------------------------------------------------------------------------
// TSonorkSkinCodecAtom
// ----------------------------------------------------------------------------

void
 TSonorkSkinCodecAtom::CODEC_Clear()
{
	skin->SetDefaultColors(false,true);
}


void
 TSonorkSkinCodecAtom::CODEC_WriteDataMem(TSonorkCodecWriter&CODEC) const
{
	UINT i;
	CODEC.WriteDW(SKIN_COLORS|(SKIN_COLOR_INDEXES<<12));
	CODEC.WriteDW(0);
	for(i=0;i<SKIN_COLORS;i++)
		CODEC.WriteDWN(skin->color[i],SKIN_COLOR_INDEXES);
}

void
 TSonorkSkinCodecAtom::CODEC_ReadDataMem(TSonorkCodecReader& CODEC)
{
	DWORD	c,codec_indexes,codec_colors;

	CODEC.ReadDW(&codec_colors);
	CODEC.Skip(sizeof(DWORD));

	codec_indexes=(codec_colors>>12)&0xf;
	codec_colors&=0xfff;
	if(codec_colors>SKIN_COLORS)
		codec_colors=SKIN_COLORS;
	for(c=0;c<codec_colors;c++)
	{
		CODEC.ReadDWN(skin->color[c],SKIN_COLOR_INDEXES);
		CODEC.Skip((codec_indexes - SKIN_COLOR_INDEXES)*sizeof(DWORD));
	}
	skin->CreateColorBrushes();
}

DWORD
 TSonorkSkinCodecAtom::CODEC_DataSize()	const
{
	return 		sizeof(DWORD)*2
		+       sizeof(DWORD)*SKIN_COLORS*SKIN_COLOR_INDEXES;
}



