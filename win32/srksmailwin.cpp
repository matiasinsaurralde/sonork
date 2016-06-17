#include "srkwin32app.h"
#include "sonork.rh"
#include "srkappstr.h"
#pragma hdrstop
#include "srksmailwin.h"
#include "srkmyinfowin.h"
#include "srk_file_io.h"

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

#define SENDING_MAIL		SONORK_WIN_F_USER_01
#define BUFFER_SIZE		1024
#define	ITEMS_SIZE		192
#define TIMER_MSECS		250

static SONORK_C_CSTR szMailerImage="gbmail.exe";
//
TSonorkSmailWin::TSonorkSmailWin(TSonorkWin*parent,const TSonorkExtUserData* UD)
	:TSonorkWin(parent
	,SONORK_WIN_CLASS_NORMAL
	|SONORK_WIN_TYPE_NONE
	|SONORK_WIN_DIALOG
	|IDD_SMAIL
	,0)
{
	SetEventMask(SONORK_APP_EM_USER_LIST_AWARE);
	user_data = UD;

}
void	TSonorkSmailWin::LoadLabels()
{
	static TSonorkWinGlsEntry gls_table[]=
	{
		{IDOK				,	GLS_EM_SND		}
	,	{IDC_SMAIL_STORE	, 	GLS_OP_SAVMSGCPY}
	,	{IDL_SMAIL_TO		, 	GLS_MSG_TO		}
	,	{IDL_SMAIL_CC		, 	GLS_MSG_CC		}
	,	{IDL_SMAIL_BCC		, 	GLS_MSG_BCC		}
	,	{IDL_SMAIL_FROM		, 	GLS_EM_SNDAS	}
	,	{IDL_SMAIL_SUBJECT	, 	GLS_MSG_SUBJ	}
	,	{IDL_SMAIL_BODY		, 	GLS_LB_TXT		}
	,	{IDC_SMAIL_INCLUDE_SIGNATURE,	GLS_EM_ISGN		}
	,	{-1					,	GLS_EM_SND		}
	,	{0					,	GLS_NULL		}
	};
	LoadLangEntries(gls_table,true);
}

bool
 TSonorkSmailWin::OnCreate()
{
	BOOL 	user_context;
	int		i;
	static UINT bold_items[]=
	{
		IDC_SMAIL_STORE
	,	IDL_SMAIL_TO
	,	IDL_SMAIL_CC
	,	IDL_SMAIL_BCC
	,	IDL_SMAIL_FROM
	,	IDL_SMAIL_SUBJECT
	,	IDL_SMAIL_BODY
	,	IDC_SMAIL_INCLUDE_SIGNATURE
	,	IDOK
	,	0
	};
	static UINT limit_text[4]=
	{
		IDL_SMAIL_TO
	,	IDL_SMAIL_CC
	,	IDL_SMAIL_BCC
	,	IDL_SMAIL_SUBJECT
	};
	for(i=0;i<4;i++)
		SetEditCtrlMaxLength(limit_text[i],ITEMS_SIZE);
	SetCtrlChecked(IDC_SMAIL_INCLUDE_SIGNATURE,true);
	SetEditCtrlMaxLength(IDC_SMAIL_BODY,SONORK_APP_MAX_MSG_TEXT_LENGTH);
	SetCaptionIcon( SKIN_HICON_EMAIL );
	SetCtrlsFont(bold_items,sonork_skin.Font(SKIN_FONT_BOLD) );
//	SonorkApp.TransferWinStartInfo( this , true , "Smail", NULL);
	user_context = user_data!=NULL;
	if( user_context )
	{
		SetCtrlText( IDC_SMAIL_TO , user_data->email.CStr() );
		SonorkApp.PostAppCommand(SONORK_APP_COMMAND_FOCUS_HWND , (LPARAM)GetDlgItem(IDC_SMAIL_SUBJECT));
	}
	SetCtrlChecked(IDC_SMAIL_STORE, user_context );
	SetCtrlEnabled(IDC_SMAIL_STORE, user_context );
	LoadAccounts();
	return true;
}
void	TSonorkSmailWin::OnAfterCreate()
{
	TSonorkWin *W;
	if( !acc_queue.Items() )
	{
		if(MessageBox(GLS_EM_NACCAVAIL , csNULL , MB_YESNO|MB_ICONQUESTION) == IDYES )
		{
			if((W = SonorkApp.RunSysDialog(SONORK_SYS_DIALOG_MY_INFO))!=NULL)
				W->PostPoke(SONORK_WIN_POKE_SET_TAB , TSonorkMyInfoWin::TAB_EMAIL);
		}
		Destroy(IDCANCEL);
	}
}
void	TSonorkSmailWin::LoadAccounts()
{
	TSonorkListIterator I;
	TSonorkEmailAccount *acc;
	HWND			hWnd;
	int				index;
	int				def_index=0;
	char			tmp[64];
	SonorkApp.LoadEmailAccounts(&acc_queue);
	hWnd = GetDlgItem(IDC_SMAIL_FROM);
	acc_queue.BeginEnum(I);
	while((acc=acc_queue.EnumNext(I))!=NULL)
	{
		wsprintf(tmp,"%-.24s (%-.24s)"
			,acc->Name()
			,acc->ReturnAddress());
		index = ComboBox_AddString(hWnd,tmp);
		ComboBox_SetItemData( hWnd, index , acc->UID());
		if( acc->UID() == SonorkApp.ProfileCtrlValue( SONORK_PCV_EMAIL_ACCOUNT_UID ) )
			def_index = index;
	}
	acc_queue.EndEnum(I);
	ComboBox_SetCurSel( hWnd, def_index );
}
void	TSonorkSmailWin::OnBeforeDestroy()
{
//	SonorkApp.TransferWinStartInfo( this , false , "Smail", NULL);
	EndSend();
}
void	TSonorkSmailWin::OnTimer(UINT)
{
	DWORD	code;
	if( !TestWinUsrFlag(SENDING_MAIL) )return;
	if( !GetExitCodeProcess(proc_info.hProcess , &code ) )
	{
		code = GetLastError();

	}
	else
	{
		if( code == STILL_ACTIVE)
			return;
	}
	EndSend();
	PostPoke( SONORK_WIN_POKE_SONORK_TASK_RESULT , code );
}

void	TSonorkSmailWin::EndSend()
{
	if( !TestWinUsrFlag(SENDING_MAIL) )return;
	KillAuxTimer();
	ClearWinUsrFlag(SENDING_MAIL);
	CloseHandle( proc_info.hProcess );
	DelTempFile();
}
void	TSonorkSmailWin::DelTempFile()
{
	if(*tmp_file.CStr())
	{
		::DeleteFile(tmp_file.CStr());
		tmp_file.Clear();
	}
}
bool
	TSonorkSmailWin::PrepareParameters(SONORK_C_STR buffer, TSonorkEmailAccount* acc, SONORK_C_CSTR szTo)
{
	SONORK_C_STR		pParams;
	SONORK_C_STR		pMsgText;
	SONORK_C_CSTR		src;
	SONORK_C_STR		port;
	char			tmp[ITEMS_SIZE];
	SONORK_FILE_HANDLE	file;
	TSonorkShortString	body;
	int				body_len;
	int				aux;
	char		*	msg_text;

	SonorkApp.GetDirPath(buffer,SONORK_APP_DIR_BIN,szMailerImage);

	pParams = buffer + strlen(buffer);

	/*GetCtrlText(IDC_SMAIL_FILE,attached_file);
	while( *attached_file.CStr() )
	{
		body_len = GetFileAttributes( attached_file.CStr() );
		if( body_len != (DWORD)-1)
			if(!(body_len&FILE_ATTRIBUTE_DIRECTORY))
				break;
		wsprintf(buffer , "%s\n'%s'",SonorkApp.SysString(GSS_FILENEXISTS), attached_file.CStr());
		return false;
	}
	*/

	// -------------------------------
	// body & temp file

	GetCtrlText(IDC_SMAIL_BODY,body);
	body_len = body.Length();
	if( body_len )
	{
		SonorkApp.GetTempPath(tmp_file,"smail", NULL, (DWORD)this);
		file = SONORK_IO_OpenFile(
			tmp_file.CStr()
			, SONORK_OPEN_FILE_METHOD_CREATE_ALWAYS);
		if( file == SONORK_INVALID_FILE_HANDLE )
		{
			SonorkApp.LangSprintf(buffer , GLS_MS_NOOPENFILE , tmp_file.CStr());
			return false;
		}
		SONORK_IO_WriteFile(file , body.CStr() , body_len );

		if(GetCtrlChecked(IDC_SMAIL_INCLUDE_SIGNATURE))
		{
			aux=wsprintf(tmp
				,"\r\n%s\r\nSonork ID: %u.%u\r\nhttp://www.sonork.com"
				,SonorkApp.ProfileUser().name.CStr()
				,SonorkApp.ProfileUserId().v[0]
				,SonorkApp.ProfileUserId().v[1]);
			SONORK_IO_WriteFile(file,tmp,aux);
		}
		SONORK_IO_CloseFile(file);
		pParams+=wsprintf(pParams
			, " -file \"%s\""
			, tmp_file.CStr());
	}
	else
		tmp_file.Clear();


	// -------------------------------
	// To / From

	pMsgText = msg_text = SONORK_MEM_ALLOC ( char ,body_len + BUFFER_SIZE );
	pMsgText += wsprintf(msg_text,"%s: %s\r\n",SonorkApp.LangString(GLS_MSG_TO),szTo);

	pParams+=wsprintf(pParams
		, " -to \"%s\" -from \"%s\""
		, szTo
		, acc->ReturnAddress()
		);

		
	// -------------------------------
	// Carbon Copy / Blind Carbon Copy

	GetCtrlText(IDC_SMAIL_CC , tmp , ITEMS_SIZE );
	if(tmp[0])
	{
		pParams+=wsprintf(pParams," -cc \"%s\"", tmp);
		pMsgText+=wsprintf(pMsgText,"%s: %s\r\n",SonorkApp.LangString(GLS_MSG_CC), tmp);
	}

	GetCtrlText(IDC_SMAIL_BCC , tmp , ITEMS_SIZE );
	if(tmp[0])
		pParams+=wsprintf(pParams," -bcc \"%s\"", tmp);


	// -------------------------------
	// Subject

	GetCtrlText(IDC_SMAIL_SUBJECT , tmp , ITEMS_SIZE);

	if(tmp[0])
	{
		pMsgText+=wsprintf(pMsgText,"%s: %s\r\n",SonorkApp.LangString(GLS_MSG_SUBJ), tmp);
		src = tmp;
		pParams+=wsprintf(pParams," -s \"");
		while(*src)
		{
			if(*src=='\"')
				*pParams++='\'';
			else
				*pParams++=*src;
			src++;
		}
		*pParams++='\"';
	}

	strcat(pMsgText , body.CStr() );
	msg.SetStr( 0 , msg_text );
	
	SONORK_MEM_FREE( msg_text );

	// -------------------------------
	// Server & Port

	lstrcpyn(tmp,acc->OutgoingServer(),ITEMS_SIZE );
	port=strchr(tmp,':');
	if(port)
	{
		*port++=0;
		pParams+=wsprintf(pParams , " -p %s", port );
	}
	wsprintf(pParams , " -h %s", tmp );

	return true;
}
void	TSonorkSmailWin::DoSend()
{
	TSonorkShortString	m_to;
	GLS_INDEX		missing_field;
	TSonorkTempBuffer	params(BUFFER_SIZE);
	int				index;
	TSonorkEmailAccount	*acc=NULL;
	DWORD			acc_uid;
	HWND			hWnd;
	TSonorkListIterator	I;
	STARTUPINFO		start_info;

	if( TestWinUsrFlag(SENDING_MAIL))
		return;
	for(;;)
	{
		GetCtrlText( IDC_SMAIL_TO 		, m_to);
		if(!strchr(m_to.CStr(),'@'))
		{
			missing_field = GLS_MSG_TO;
			break;
		}
		missing_field = GLS_EM_SNDAS;
		hWnd = GetDlgItem( IDC_SMAIL_FROM );
		index = ComboBox_GetCurSel( hWnd );
		if( index == CB_ERR )break;
		acc_uid = ComboBox_GetItemData( hWnd , index );
		acc_queue.BeginEnum(I);
		while( (acc=acc_queue.EnumNext(I)) != NULL )
			if( acc->UID() == acc_uid )
				break;
		acc_queue.EndEnum(I);
		break;
	}
	if( acc == NULL  )
	{
		SonorkApp.LangSprintf(params.CStr(),GLS_MS_FLDMISS,SonorkApp.LangString(missing_field));
	}
	else
	if(PrepareParameters(params.CStr() , acc , m_to.CStr() ))
	{
		start_info.cb = sizeof(start_info);
		GetStartupInfo(&start_info);
		start_info.dwFlags|=STARTF_USESHOWWINDOW;
		start_info.wShowWindow = SW_HIDE;
		sonork_puts("****************");
		sonork_puts(params.CStr() );
		sonork_puts("****************");
		if(CreateProcess(
				NULL
			,	params.CStr()
			, 	NULL
			,	NULL
			,	false
			,	0
			,	NULL
			, 	NULL
			,	&start_info
			,	&proc_info))
		{
			CloseHandle( proc_info.hThread );
			SetWinUsrFlag(SENDING_MAIL);
			SetAuxTimer(TIMER_MSECS);
			SetCtrlEnabled(IDOK,false);
			SetCtrlEnabled(IDC_SMAIL_BODY,false);
			return;
		}
		SonorkApp.LangSprintf(params.CStr()
			, GLS_ERR_SPAWN
			, szMailerImage
			, GetLastError());
	}
	DelTempFile();
	MessageBox(params.CStr(),GLS_EM_SND,MB_ICONERROR|MB_OK);
}

bool 	TSonorkSmailWin::OnCommand(UINT id,HWND , UINT code)
{
	if( (id == IDOK || id == IDCANCEL) && code == BN_CLICKED)
	{
		if( id == IDOK )
			DoSend();
		else
			Destroy();
		return true;
	}
	return false;
}
void
 TSonorkSmailWin::SaveMsg()
{
	TSonorkMsgHandleEx	handle;

	msg.header.dataSvcInfo.SetInfo(SONORK_SERVICE_ID_NONE
		, SONORK_SERVICE_TYPE_EMAIL
		, 0);
	SonorkApp.PrepareMsg(
		handle
		, &msg
		, 0					// sys_flags
		, SONORK_MSG_UF_AUTOMATIC		// usr_flags
		, SONORK_APP_MPF_NO_SERVICE_PARSE
		| SONORK_APP_MPF_DONT_SEND        	// Don't actually send this message
		, 0					// reply_tracking_no
		, NULL
		);
	SonorkApp.SendMsgUser(handle
			, this
			, user_data
			, &msg);
}

LRESULT
	TSonorkSmailWin::OnPoke(SONORK_WIN_POKE poke ,LPARAM lParam)
{
	if( poke == SONORK_WIN_POKE_SONORK_TASK_RESULT )
	{
		TSonorkTempBuffer tmp(128 );
		SetCtrlEnabled(IDOK,true);
		SetCtrlEnabled(IDC_SMAIL_BODY,true);
		if( lParam == 0
			&& user_data!=NULL
			&& GetCtrlChecked( IDC_SMAIL_STORE ) )
		{
			SaveMsg();
		}
		SonorkApp.LangSprintf(tmp.CStr(),lParam==0?GLS_EM_SNDOK:GLS_EM_SNDERR,lParam);
		MessageBox(tmp.CStr(),csNULL,MB_OK);
		if( lParam == 0)
		{
						
			//SONORK_SERVICE_TYPE_EMAIL
			Destroy(IDOK);
		}
	}
	return 0;
}

bool
	TSonorkSmailWin::OnAppEvent(UINT event, UINT , void* data)
{
	switch( event )
	{
		case SONORK_APP_EVENT_DEL_USER:
			if( user_data )
			{
				TSonorkId* deletedUserId=(TSonorkId*)data;
				if( *deletedUserId == user_data->userId )
					Destroy(IDCANCEL);
			}
			return true;
	}
	return false;
}

